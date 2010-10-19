/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockable.c
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"

#include "gimpdialogfactory.h"
#include "gimpdnd.h"
#include "gimpdock.h"
#include "gimpdockable.h"
#include "gimpdockbook.h"
#include "gimpdocked.h"
#include "gimpdockwindow.h"
#include "gimphelp-ids.h"
#include "gimppanedbox.h"
#include "gimpsessioninfo-aux.h"
#include "gimpsessionmanaged.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_LOCKED
};


struct _GimpDockablePrivate
{
  gchar        *name;
  gchar        *blurb;
  gchar        *icon_name;
  gchar        *help_id;
  GimpTabStyle  tab_style;
  GimpTabStyle  actual_tab_style;
  gboolean      locked;

  GimpDockbook *dockbook;

  GimpContext  *context;

  guint         blink_timeout_id;
  gint          blink_counter;

  GimpPanedBox *drag_handler;

  /*  drag icon hotspot  */
  gint          drag_x;
  gint          drag_y;
};


static void       gimp_dockable_session_managed_iface_init
                                                  (GimpSessionManagedInterface
                                                                  *iface);
static void       gimp_dockable_dispose           (GObject        *object);
static void       gimp_dockable_set_property      (GObject        *object,
                                                   guint           property_id,
                                                   const GValue   *value,
                                                   GParamSpec     *pspec);
static void       gimp_dockable_get_property      (GObject        *object,
                                                   guint           property_id,
                                                   GValue         *value,
                                                   GParamSpec     *pspec);

static void       gimp_dockable_size_request      (GtkWidget      *widget,
                                                   GtkRequisition *requisition);
static void       gimp_dockable_size_allocate     (GtkWidget      *widget,
                                                   GtkAllocation  *allocation);
static void       gimp_dockable_drag_leave        (GtkWidget      *widget,
                                                   GdkDragContext *context,
                                                   guint           time);
static gboolean   gimp_dockable_drag_motion       (GtkWidget      *widget,
                                                   GdkDragContext *context,
                                                   gint            x,
                                                   gint            y,
                                                   guint           time);
static gboolean   gimp_dockable_drag_drop         (GtkWidget      *widget,
                                                   GdkDragContext *context,
                                                   gint            x,
                                                   gint            y,
                                                   guint           time);

static void       gimp_dockable_style_set         (GtkWidget      *widget,
                                                   GtkStyle       *prev_style);

static void       gimp_dockable_add               (GtkContainer   *container,
                                                   GtkWidget      *widget);
static GType      gimp_dockable_child_type        (GtkContainer   *container);
static GList    * gimp_dockable_get_aux_info      (GimpSessionManaged
                                                                  *session_managed);
static void       gimp_dockable_set_aux_info      (GimpSessionManaged
                                                                  *session_managed,
                                                   GList          *aux_info);

static GimpTabStyle
                  gimp_dockable_convert_tab_style (GimpDockable   *dockable,
                                                   GimpTabStyle    tab_style);
static gboolean   gimp_dockable_blink_timeout     (GimpDockable   *dockable);


G_DEFINE_TYPE_WITH_CODE (GimpDockable, gimp_dockable, GTK_TYPE_BIN,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_SESSION_MANAGED,
                                                gimp_dockable_session_managed_iface_init))

#define parent_class gimp_dockable_parent_class

static const GtkTargetEntry dialog_target_table[] = { GIMP_TARGET_DIALOG };


static void
gimp_dockable_class_init (GimpDockableClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class    = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->dispose       = gimp_dockable_dispose;
  object_class->set_property  = gimp_dockable_set_property;
  object_class->get_property  = gimp_dockable_get_property;

  widget_class->size_request  = gimp_dockable_size_request;
  widget_class->size_allocate = gimp_dockable_size_allocate;
  widget_class->style_set     = gimp_dockable_style_set;
  widget_class->drag_leave    = gimp_dockable_drag_leave;
  widget_class->drag_motion   = gimp_dockable_drag_motion;
  widget_class->drag_drop     = gimp_dockable_drag_drop;

  container_class->add        = gimp_dockable_add;
  container_class->child_type = gimp_dockable_child_type;

  g_object_class_install_property (object_class, PROP_LOCKED,
                                   g_param_spec_boolean ("locked", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content-border",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             GIMP_PARAM_READABLE));

  g_type_class_add_private (klass, sizeof (GimpDockablePrivate));
}

static void
gimp_dockable_init (GimpDockable *dockable)
{
  dockable->p = G_TYPE_INSTANCE_GET_PRIVATE (dockable,
                                             GIMP_TYPE_DOCKABLE,
                                             GimpDockablePrivate);
  dockable->p->tab_style        = GIMP_TAB_STYLE_AUTOMATIC;
  dockable->p->actual_tab_style = GIMP_TAB_STYLE_UNDEFINED;
  dockable->p->drag_x           = GIMP_DOCKABLE_DRAG_OFFSET;
  dockable->p->drag_y           = GIMP_DOCKABLE_DRAG_OFFSET;

  gtk_drag_dest_set (GTK_WIDGET (dockable),
                     0,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);
}

static void
gimp_dockable_session_managed_iface_init (GimpSessionManagedInterface *iface)
{
  iface->get_aux_info = gimp_dockable_get_aux_info;
  iface->set_aux_info = gimp_dockable_set_aux_info;
}

static void
gimp_dockable_dispose (GObject *object)
{
  GimpDockable *dockable = GIMP_DOCKABLE (object);

  gimp_dockable_blink_cancel (dockable);

  if (dockable->p->context)
    gimp_dockable_set_context (dockable, NULL);

  if (dockable->p->blurb)
    {
      if (dockable->p->blurb != dockable->p->name)
        g_free (dockable->p->blurb);

      dockable->p->blurb = NULL;
    }

  if (dockable->p->name)
    {
      g_free (dockable->p->name);
      dockable->p->name = NULL;
    }

  if (dockable->p->icon_name)
    {
      g_free (dockable->p->icon_name);
      dockable->p->icon_name = NULL;
    }

  if (dockable->p->help_id)
    {
      g_free (dockable->p->help_id);
      dockable->p->help_id = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_dockable_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpDockable *dockable = GIMP_DOCKABLE (object);

  switch (property_id)
    {
    case PROP_LOCKED:
      gimp_dockable_set_locked (dockable, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dockable_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpDockable *dockable = GIMP_DOCKABLE (object);

  switch (property_id)
    {
    case PROP_LOCKED:
      g_value_set_boolean (value, dockable->p->locked);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dockable_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
  GtkContainer   *container = GTK_CONTAINER (widget);
  GtkWidget      *child     = gtk_bin_get_child (GTK_BIN (widget));
  GtkRequisition  child_requisition;
  gint            border_width;

  border_width = gtk_container_get_border_width (container);

  requisition->width  = border_width * 2;
  requisition->height = border_width * 2;

  if (child && gtk_widget_get_visible (child))
    {
      gtk_widget_get_preferred_size (child, &child_requisition, NULL);

      requisition->width  += child_requisition.width;
      requisition->height += child_requisition.height;
    }
}

static void
gimp_dockable_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  GtkContainer   *container = GTK_CONTAINER (widget);
  GtkWidget      *child     = gtk_bin_get_child (GTK_BIN (widget));

  GtkRequisition  button_requisition = { 0, };
  GtkAllocation   child_allocation;
  gint            border_width;


  gtk_widget_set_allocation (widget, allocation);

  border_width = gtk_container_get_border_width (container);

  if (child && gtk_widget_get_visible (child))
    {
      child_allocation.x      = allocation->x + border_width;
      child_allocation.y      = allocation->y + border_width;
      child_allocation.width  = MAX (allocation->width  -
                                     border_width * 2,
                                     0);
      child_allocation.height = MAX (allocation->height -
                                     border_width * 2 -
                                     button_requisition.height,
                                     0);

      child_allocation.y += button_requisition.height;

      gtk_widget_size_allocate (child, &child_allocation);
    }
}

static void
gimp_dockable_drag_leave (GtkWidget      *widget,
                          GdkDragContext *context,
                          guint           time)
{
  gimp_highlight_widget (widget, FALSE);
}

static gboolean
gimp_dockable_drag_motion (GtkWidget      *widget,
                           GdkDragContext *context,
                           gint            x,
                           gint            y,
                           guint           time)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);

  if (gimp_paned_box_will_handle_drag (dockable->p->drag_handler,
                                       widget,
                                       context,
                                       x, y,
                                       time))
    {
      gdk_drag_status (context, 0, time);
      gimp_highlight_widget (widget, FALSE);

      return FALSE;
    }

  gdk_drag_status (context, GDK_ACTION_MOVE, time);
  gimp_highlight_widget (widget, TRUE);

  /* Return TRUE so drag_leave() is called */
  return TRUE;
}

static gboolean
gimp_dockable_drag_drop (GtkWidget      *widget,
                         GdkDragContext *context,
                         gint            x,
                         gint            y,
                         guint           time)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);
  gboolean      dropped;

  if (gimp_paned_box_will_handle_drag (dockable->p->drag_handler,
                                       widget,
                                       context,
                                       x, y,
                                       time))
    {
      return FALSE;
    }

  dropped = gimp_dockbook_drop_dockable (GIMP_DOCKABLE (widget)->p->dockbook,
                                         gtk_drag_get_source_widget (context));

  gtk_drag_finish (context, dropped, TRUE, time);

  return TRUE;
}

static void
gimp_dockable_style_set (GtkWidget *widget,
                         GtkStyle  *prev_style)
{
  gint content_border;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "content-border", &content_border,
                        NULL);

  gtk_container_set_border_width (GTK_CONTAINER (widget), content_border);
}


static void
gimp_dockable_add (GtkContainer *container,
                   GtkWidget    *widget)
{
  GimpDockable *dockable;

  g_return_if_fail (gtk_bin_get_child (GTK_BIN (container)) == NULL);

  GTK_CONTAINER_CLASS (parent_class)->add (container, widget);

  /*  not all tab styles are supported by all children  */
  dockable = GIMP_DOCKABLE (container);
  gimp_dockable_set_tab_style (dockable, dockable->p->tab_style);
}

static GType
gimp_dockable_child_type (GtkContainer *container)
{
  if (gtk_bin_get_child (GTK_BIN (container)))
    return G_TYPE_NONE;

  return GIMP_TYPE_DOCKED;
}

static GtkWidget *
gimp_dockable_new_tab_widget_internal (GimpDockable *dockable,
                                       GimpContext  *context,
                                       GimpTabStyle  tab_style,
                                       GtkIconSize   size,
                                       gboolean      dnd)
{
  GtkWidget *tab_widget = NULL;
  GtkWidget *label      = NULL;
  GtkWidget *icon       = NULL;

  switch (tab_style)
    {
    case GIMP_TAB_STYLE_NAME:
    case GIMP_TAB_STYLE_ICON_NAME:
    case GIMP_TAB_STYLE_PREVIEW_NAME:
      label = gtk_label_new (dockable->p->name);
      break;

    case GIMP_TAB_STYLE_BLURB:
    case GIMP_TAB_STYLE_ICON_BLURB:
    case GIMP_TAB_STYLE_PREVIEW_BLURB:
      label = gtk_label_new (dockable->p->blurb);
      break;

    default:
      break;
    }

  switch (tab_style)
    {
    case GIMP_TAB_STYLE_ICON:
    case GIMP_TAB_STYLE_ICON_NAME:
    case GIMP_TAB_STYLE_ICON_BLURB:
      icon = gimp_dockable_get_icon (dockable, size);
      break;

    case GIMP_TAB_STYLE_PREVIEW:
    case GIMP_TAB_STYLE_PREVIEW_NAME:
    case GIMP_TAB_STYLE_PREVIEW_BLURB:
      {
        GtkWidget *child = gtk_bin_get_child (GTK_BIN (dockable));

        if (child)
          icon = gimp_docked_get_preview (GIMP_DOCKED (child),
                                          context, size);

        if (! icon)
          icon = gimp_dockable_get_icon (dockable, size);
      }
      break;

    default:
      break;
    }

  if (label && dnd)
    gimp_label_set_attributes (GTK_LABEL (label),
                               PANGO_ATTR_WEIGHT, PANGO_WEIGHT_SEMIBOLD,
                               -1);

  switch (tab_style)
    {
    case GIMP_TAB_STYLE_ICON:
    case GIMP_TAB_STYLE_PREVIEW:
      tab_widget = icon;
      break;

    case GIMP_TAB_STYLE_NAME:
    case GIMP_TAB_STYLE_BLURB:
      tab_widget = label;
      break;

    case GIMP_TAB_STYLE_ICON_NAME:
    case GIMP_TAB_STYLE_ICON_BLURB:
    case GIMP_TAB_STYLE_PREVIEW_NAME:
    case GIMP_TAB_STYLE_PREVIEW_BLURB:
      tab_widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, dnd ? 6 : 2);

      gtk_box_pack_start (GTK_BOX (tab_widget), icon, FALSE, FALSE, 0);
      gtk_widget_show (icon);

      gtk_box_pack_start (GTK_BOX (tab_widget), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
      break;

    case GIMP_TAB_STYLE_UNDEFINED:
    case GIMP_TAB_STYLE_AUTOMATIC:
      g_warning ("Tab style error, unexpected code path taken, fix!");
      break;
    }

  return tab_widget;
}

/*  public functions  */

GtkWidget *
gimp_dockable_new (const gchar *name,
                   const gchar *blurb,
                   const gchar *icon_name,
                   const gchar *help_id)
{
  GimpDockable *dockable;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);

  dockable = g_object_new (GIMP_TYPE_DOCKABLE, NULL);

  dockable->p->name      = g_strdup (name);
  dockable->p->icon_name = g_strdup (icon_name);
  dockable->p->help_id   = g_strdup (help_id);

  if (blurb)
    dockable->p->blurb  = g_strdup (blurb);
  else
    dockable->p->blurb  = dockable->p->name;

  gimp_help_set_help_data (GTK_WIDGET (dockable), NULL, help_id);

  return GTK_WIDGET (dockable);
}

void
gimp_dockable_set_dockbook (GimpDockable *dockable,
                            GimpDockbook *dockbook)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (dockbook == NULL ||
                    GIMP_IS_DOCKBOOK (dockbook));

  dockable->p->dockbook = dockbook;
}

GimpDockbook *
gimp_dockable_get_dockbook (GimpDockable *dockable)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);

  return dockable->p->dockbook;
}

GimpTabStyle
gimp_dockable_get_tab_style (GimpDockable *dockable)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), -1);

  return dockable->p->tab_style;
}

/**
 * gimp_dockable_get_actual_tab_style:
 * @dockable:
 *
 * Get actual tab style, i.e. never "automatic". This state should
 * actually be hold on a per-dockbook basis, but at this point that
 * feels like over-engineering...
 **/
GimpTabStyle
gimp_dockable_get_actual_tab_style (GimpDockable *dockable)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), -1);

  return dockable->p->actual_tab_style;
}

const gchar *
gimp_dockable_get_name (GimpDockable *dockable)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);

  return dockable->p->name;
}

const gchar *
gimp_dockable_get_blurb (GimpDockable *dockable)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);

  return dockable->p->blurb;
}

const gchar *
gimp_dockable_get_help_id (GimpDockable *dockable)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);

  return dockable->p->help_id;
}

const gchar *
gimp_dockable_get_icon_name (GimpDockable *dockable)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);

  return dockable->p->icon_name;
}

GtkWidget *
gimp_dockable_get_icon (GimpDockable *dockable,
                        GtkIconSize   size)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);

  return gtk_image_new_from_icon_name (dockable->p->icon_name, size);
}

gboolean
gimp_dockable_get_locked (GimpDockable *dockable)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), FALSE);

  return dockable->p->locked;
}

void
gimp_dockable_set_drag_pos (GimpDockable *dockable,
                            gint          drag_x,
                            gint          drag_y)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  dockable->p->drag_x = drag_x;
  dockable->p->drag_y = drag_y;
}

void
gimp_dockable_get_drag_pos (GimpDockable *dockable,
                            gint         *drag_x,
                            gint         *drag_y)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  if (drag_x != NULL)
    *drag_x = dockable->p->drag_x;
  if (drag_y != NULL)
    *drag_y = dockable->p->drag_y;
}

GimpPanedBox *
gimp_dockable_get_drag_handler (GimpDockable *dockable)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);

  return dockable->p->drag_handler;
}

void
gimp_dockable_set_locked (GimpDockable *dockable,
                          gboolean      lock)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  if (dockable->p->locked != lock)
    {
      dockable->p->locked = lock ? TRUE : FALSE;

      g_object_notify (G_OBJECT (dockable), "locked");
    }
}

gboolean
gimp_dockable_is_locked (GimpDockable *dockable)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), FALSE);

  return dockable->p->locked;
}


void
gimp_dockable_set_tab_style (GimpDockable *dockable,
                             GimpTabStyle  tab_style)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  dockable->p->tab_style = gimp_dockable_convert_tab_style (dockable, tab_style);

  if (tab_style == GIMP_TAB_STYLE_AUTOMATIC)
    gimp_dockable_set_actual_tab_style (dockable, GIMP_TAB_STYLE_UNDEFINED);
  else
    gimp_dockable_set_actual_tab_style (dockable, tab_style);

  if (dockable->p->dockbook)
    gimp_dockbook_update_auto_tab_style (dockable->p->dockbook);
}

/**
 * gimp_dockable_set_actual_tab_style:
 * @dockable:
 * @tab_style:
 *
 * Sets actual tab style, meant for those that decides what
 * "automatic" tab style means.
 *
 * Returns: %TRUE if changed, %FALSE otherwise.
 **/
gboolean
gimp_dockable_set_actual_tab_style (GimpDockable *dockable,
                                    GimpTabStyle  tab_style)
{
  GimpTabStyle new_tab_style = gimp_dockable_convert_tab_style (dockable, tab_style);
  GimpTabStyle old_tab_style = dockable->p->actual_tab_style;

  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), FALSE);
  g_return_val_if_fail (tab_style != GIMP_TAB_STYLE_AUTOMATIC, FALSE);

  dockable->p->actual_tab_style = new_tab_style;

  return new_tab_style != old_tab_style;
}

GtkWidget *
gimp_dockable_create_tab_widget (GimpDockable *dockable,
                                 GimpContext  *context,
                                 GimpTabStyle  tab_style,
                                 GtkIconSize   size)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return gimp_dockable_new_tab_widget_internal (dockable, context,
                                                tab_style, size, FALSE);
}

GtkWidget *
gimp_dockable_create_drag_widget (GimpDockable *dockable)
{
  GtkWidget *frame;
  GtkWidget *widget;

  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

  widget = gimp_dockable_new_tab_widget_internal (dockable,
                                                  dockable->p->context,
                                                  GIMP_TAB_STYLE_ICON_BLURB,
                                                  GTK_ICON_SIZE_DND,
                                                  TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (widget), 6);
  gtk_container_add (GTK_CONTAINER (frame), widget);
  gtk_widget_show (widget);

  return frame;
}

void
gimp_dockable_set_context (GimpDockable *dockable,
                           GimpContext  *context)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  if (context != dockable->p->context)
    {
      GtkWidget *child = gtk_bin_get_child (GTK_BIN (dockable));

      if (child)
        gimp_docked_set_context (GIMP_DOCKED (child), context);

      dockable->p->context = context;
    }
}

GimpUIManager *
gimp_dockable_get_menu (GimpDockable  *dockable,
                        const gchar  **ui_path,
                        gpointer      *popup_data)
{
  GtkWidget *child;

  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);
  g_return_val_if_fail (ui_path != NULL, NULL);
  g_return_val_if_fail (popup_data != NULL, NULL);

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child)
    return gimp_docked_get_menu (GIMP_DOCKED (child), ui_path, popup_data);

  return NULL;
}

/**
 * gimp_dockable_set_drag_handler:
 * @dockable:
 * @handler:
 *
 * Set a drag handler that will be asked if it will handle drag events
 * before the dockable handles the event itself.
 **/
void
gimp_dockable_set_drag_handler (GimpDockable *dockable,
                                GimpPanedBox *handler)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  dockable->p->drag_handler = handler;
}

void
gimp_dockable_detach (GimpDockable *dockable)
{
  GimpDialogFactory *dialog_factory;
  GimpMenuFactory   *menu_factory;
  GimpDockWindow    *src_dock_window;
  GimpDock          *src_dock;
  GtkWidget         *dock;
  GimpDockWindow    *dock_window;
  GtkWidget         *dockbook;

  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockable->p->dockbook));

  src_dock        = gimp_dockbook_get_dock (dockable->p->dockbook);
  src_dock_window = gimp_dock_window_from_dock (src_dock);

  dialog_factory = gimp_dock_get_dialog_factory (src_dock);
  menu_factory   = gimp_dialog_factory_get_menu_factory (dialog_factory);

  dock = gimp_dock_with_window_new (dialog_factory,
                                    gtk_widget_get_screen (GTK_WIDGET (dockable)),
                                    gimp_widget_get_monitor (GTK_WIDGET (dockable)),
                                    FALSE /*toolbox*/);
  dock_window = gimp_dock_window_from_dock (GIMP_DOCK (dock));
  gtk_window_set_position (GTK_WINDOW (dock_window), GTK_WIN_POS_MOUSE);
  if (src_dock_window)
    gimp_dock_window_setup (dock_window, src_dock_window);

  dockbook = gimp_dockbook_new (menu_factory);

  gimp_dock_add_book (GIMP_DOCK (dock), GIMP_DOCKBOOK (dockbook), 0);

  g_object_ref (dockable);

  gimp_dockbook_remove (dockable->p->dockbook, dockable);
  gimp_dockbook_add (GIMP_DOCKBOOK (dockbook), dockable, 0);

  g_object_unref (dockable);

  gtk_widget_show (GTK_WIDGET (dock_window));
  gtk_widget_show (dock);
}

void
gimp_dockable_blink (GimpDockable *dockable)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  if (dockable->p->blink_timeout_id)
    g_source_remove (dockable->p->blink_timeout_id);

  dockable->p->blink_timeout_id =
    g_timeout_add (150, (GSourceFunc) gimp_dockable_blink_timeout, dockable);

  gimp_highlight_widget (GTK_WIDGET (dockable), TRUE);
}

void
gimp_dockable_blink_cancel (GimpDockable *dockable)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  if (dockable->p->blink_timeout_id)
    {
      g_source_remove (dockable->p->blink_timeout_id);

      dockable->p->blink_timeout_id = 0;
      dockable->p->blink_counter    = 0;

      gimp_highlight_widget (GTK_WIDGET (dockable), FALSE);
    }
}


/*  private functions  */

static GList *
gimp_dockable_get_aux_info (GimpSessionManaged *session_managed)
{
  GimpDockable *dockable;
  GtkWidget    *child;

  g_return_val_if_fail (GIMP_IS_DOCKABLE (session_managed), NULL);

  dockable = GIMP_DOCKABLE (session_managed);

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child)
    return gimp_docked_get_aux_info (GIMP_DOCKED (child));

  return NULL;
}

static void
gimp_dockable_set_aux_info (GimpSessionManaged *session_managed,
                            GList              *aux_info)
{
  GimpDockable *dockable;
  GtkWidget    *child;

  g_return_if_fail (GIMP_IS_DOCKABLE (session_managed));

  dockable = GIMP_DOCKABLE (session_managed);

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child)
    gimp_docked_set_aux_info (GIMP_DOCKED (child), aux_info);
}

static GimpTabStyle
gimp_dockable_convert_tab_style (GimpDockable   *dockable,
                                 GimpTabStyle    tab_style)
{
  GtkWidget *child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child && ! GIMP_DOCKED_GET_INTERFACE (child)->get_preview)
    tab_style = gimp_preview_tab_style_to_icon (tab_style);

  return tab_style;
}

static gboolean
gimp_dockable_blink_timeout (GimpDockable *dockable)
{
  gimp_highlight_widget (GTK_WIDGET (dockable),
                         dockable->p->blink_counter % 2 == 1);
  dockable->p->blink_counter++;

  if (dockable->p->blink_counter == 3)
    {
      dockable->p->blink_timeout_id = 0;
      dockable->p->blink_counter    = 0;

      return FALSE;
    }

  return TRUE;
}
