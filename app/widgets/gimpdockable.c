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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "menus/menus.h"

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
  gchar        *stock_id;
  gchar        *help_id;
  GimpTabStyle  tab_style;
  gboolean      locked;

  GimpDockbook *dockbook;

  GimpContext  *context;

  PangoLayout  *title_layout;
  GdkWindow    *title_window;
  GtkWidget    *menu_button;

  guint         blink_timeout_id;
  gint          blink_counter;

  GimpPanedBox *drag_handler;

  /*  drag icon hotspot  */
  gint          drag_x;
  gint          drag_y;
};


static void       gimp_dockable_set_property      (GObject        *object,
                                                   guint           property_id,
                                                   const GValue   *value,
                                                   GParamSpec     *pspec);
static void       gimp_dockable_get_property      (GObject        *object,
                                                   guint           property_id,
                                                   GValue         *value,
                                                   GParamSpec     *pspec);
static void       gimp_dockable_destroy           (GtkObject      *object);
static void       gimp_dockable_size_request      (GtkWidget      *widget,
                                                   GtkRequisition *requisition);
static void       gimp_dockable_size_allocate     (GtkWidget      *widget,
                                                   GtkAllocation  *allocation);
static void       gimp_dockable_realize           (GtkWidget      *widget);
static void       gimp_dockable_unrealize         (GtkWidget      *widget);
static void       gimp_dockable_map               (GtkWidget      *widget);
static void       gimp_dockable_unmap             (GtkWidget      *widget);

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
static gboolean   gimp_dockable_expose_event      (GtkWidget      *widget,
                                                   GdkEventExpose *event);
static gboolean   gimp_dockable_button_press      (GtkWidget      *widget,
                                                   GdkEventButton *event);
static gboolean   gimp_dockable_button_release    (GtkWidget      *widget);
static gboolean   gimp_dockable_popup_menu        (GtkWidget      *widget);

static void       gimp_dockable_add               (GtkContainer   *container,
                                                   GtkWidget      *widget);
static void       gimp_dockable_remove            (GtkContainer   *container,
                                                   GtkWidget      *widget);
static GType      gimp_dockable_child_type        (GtkContainer   *container);
static void       gimp_dockable_forall            (GtkContainer   *container,
                                                   gboolean        include_internals,
                                                   GtkCallback     callback,
                                                   gpointer        callback_data);

static void       gimp_dockable_cursor_setup      (GimpDockable   *dockable);

static void       gimp_dockable_get_title_area    (GimpDockable   *dockable,
                                                   GdkRectangle   *area);
static void       gimp_dockable_clear_title_area  (GimpDockable   *dockable);

static gboolean   gimp_dockable_menu_button_press (GtkWidget      *button,
                                                   GdkEventButton *bevent,
                                                   GimpDockable   *dockable);
static gboolean   gimp_dockable_show_menu         (GimpDockable   *dockable);
static gboolean   gimp_dockable_blink_timeout     (GimpDockable   *dockable);

static void       gimp_dockable_title_changed     (GimpDocked     *docked,
                                                   GimpDockable   *dockable);


G_DEFINE_TYPE (GimpDockable, gimp_dockable, GTK_TYPE_BIN)

#define parent_class gimp_dockable_parent_class

static const GtkTargetEntry dialog_target_table[] = { GIMP_TARGET_DIALOG };


static void
gimp_dockable_class_init (GimpDockableClass *klass)
{
  GObjectClass      *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass    *gtk_object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class     = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class  = GTK_CONTAINER_CLASS (klass);

  object_class->set_property  = gimp_dockable_set_property;
  object_class->get_property  = gimp_dockable_get_property;

  gtk_object_class->destroy   = gimp_dockable_destroy;

  widget_class->size_request  = gimp_dockable_size_request;
  widget_class->size_allocate = gimp_dockable_size_allocate;
  widget_class->realize       = gimp_dockable_realize;
  widget_class->unrealize     = gimp_dockable_unrealize;
  widget_class->map           = gimp_dockable_map;
  widget_class->unmap         = gimp_dockable_unmap;
  widget_class->style_set     = gimp_dockable_style_set;
  widget_class->drag_leave    = gimp_dockable_drag_leave;
  widget_class->drag_motion   = gimp_dockable_drag_motion;
  widget_class->drag_drop     = gimp_dockable_drag_drop;
  widget_class->expose_event  = gimp_dockable_expose_event;
  widget_class->popup_menu    = gimp_dockable_popup_menu;

  container_class->add        = gimp_dockable_add;
  container_class->remove     = gimp_dockable_remove;
  container_class->child_type = gimp_dockable_child_type;
  container_class->forall     = gimp_dockable_forall;

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
  GtkWidget *image;

  dockable->p = G_TYPE_INSTANCE_GET_PRIVATE (dockable,
                                             GIMP_TYPE_DOCKABLE,
                                             GimpDockablePrivate);
  dockable->p->tab_style = GIMP_TAB_STYLE_PREVIEW;
  dockable->p->drag_x    = GIMP_DOCKABLE_DRAG_OFFSET;
  dockable->p->drag_y    = GIMP_DOCKABLE_DRAG_OFFSET;

  gtk_widget_push_composite_child ();
  dockable->p->menu_button = gtk_button_new ();
  gtk_widget_pop_composite_child ();

  gtk_widget_set_can_focus (dockable->p->menu_button, FALSE);
  gtk_widget_set_parent (dockable->p->menu_button, GTK_WIDGET (dockable));
  gtk_button_set_relief (GTK_BUTTON (dockable->p->menu_button), GTK_RELIEF_NONE);
  gtk_widget_show (dockable->p->menu_button);

  image = gtk_image_new_from_stock (GIMP_STOCK_MENU_LEFT, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (dockable->p->menu_button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (dockable->p->menu_button, _("Configure this tab"),
                           GIMP_HELP_DOCK_TAB_MENU);

  g_signal_connect (dockable->p->menu_button, "button-press-event",
                    G_CALLBACK (gimp_dockable_menu_button_press),
                    dockable);

  gtk_drag_dest_set (GTK_WIDGET (dockable),
                     0,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);

  /*  Filter out all button-press events not coming from the event window
      over the title area.  This keeps events that originate from widgets
      in the dockable to start a drag.
   */
  g_signal_connect (dockable, "button-press-event",
                    G_CALLBACK (gimp_dockable_button_press),
                    NULL);
  g_signal_connect (dockable, "button-release-event",
                    G_CALLBACK (gimp_dockable_button_release),
                    NULL);
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
gimp_dockable_destroy (GtkObject *object)
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

  if (dockable->p->stock_id)
    {
      g_free (dockable->p->stock_id);
      dockable->p->stock_id = NULL;
    }

  if (dockable->p->help_id)
    {
      g_free (dockable->p->help_id);
      dockable->p->help_id = NULL;
    }

  if (dockable->p->title_layout)
    {
      g_object_unref (dockable->p->title_layout);
      dockable->p->title_layout = NULL;
    }

  if (dockable->p->menu_button)
    {
      gtk_widget_unparent (dockable->p->menu_button);
      dockable->p->menu_button = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_dockable_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
  GtkContainer   *container = GTK_CONTAINER (widget);
  GimpDockable   *dockable  = GIMP_DOCKABLE (widget);
  GtkWidget      *child     = gtk_bin_get_child (GTK_BIN (widget));
  GtkRequisition  child_requisition;
  gint            border_width;

  border_width = gtk_container_get_border_width (container);

  requisition->width  = border_width * 2;
  requisition->height = border_width * 2;

  if (dockable->p->menu_button && gtk_widget_get_visible (dockable->p->menu_button))
    {
      gtk_widget_size_request (dockable->p->menu_button, &child_requisition);

      if (! child)
        requisition->width += child_requisition.width;

      requisition->height += child_requisition.height;
    }

  if (child && gtk_widget_get_visible (child))
    {
      gtk_widget_size_request (child, &child_requisition);

      requisition->width  += child_requisition.width;
      requisition->height += child_requisition.height;
    }
}

static void
gimp_dockable_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  GtkContainer   *container = GTK_CONTAINER (widget);
  GimpDockable   *dockable  = GIMP_DOCKABLE (widget);
  GtkWidget      *child     = gtk_bin_get_child (GTK_BIN (widget));

  GtkRequisition  button_requisition = { 0, };
  GtkAllocation   child_allocation;
  gint            border_width;

  gtk_widget_set_allocation (widget, allocation);

  border_width = gtk_container_get_border_width (container);

  if (dockable->p->menu_button && gtk_widget_get_visible (dockable->p->menu_button))
    {
      gtk_widget_size_request (dockable->p->menu_button, &button_requisition);

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
        child_allocation.x    = (allocation->x +
                                 allocation->width -
                                 border_width -
                                 button_requisition.width);
      else
        child_allocation.x    = allocation->x + border_width;

      child_allocation.y      = allocation->y + border_width;
      child_allocation.width  = button_requisition.width;
      child_allocation.height = button_requisition.height;

      gtk_widget_size_allocate (dockable->p->menu_button, &child_allocation);
    }

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

  if (dockable->p->title_window)
    {
      GdkRectangle  area;

      gimp_dockable_get_title_area (dockable, &area);

      gdk_window_move_resize (dockable->p->title_window,
                              area.x, area.y, area.width, area.height);

      if (dockable->p->title_layout)
        pango_layout_set_width (dockable->p->title_layout,
                                PANGO_SCALE * area.width);
    }
}

static void
gimp_dockable_realize (GtkWidget *widget)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  if (! dockable->p->title_window)
    {
      GdkWindowAttr  attributes;
      GdkRectangle   area;

      gimp_dockable_get_title_area (dockable, &area);

      attributes.x                 = area.x;
      attributes.y                 = area.y;
      attributes.width             = area.width;
      attributes.height            = area.height;
      attributes.window_type       = GDK_WINDOW_CHILD;
      attributes.wclass            = GDK_INPUT_ONLY;
      attributes.override_redirect = TRUE;
      attributes.event_mask        = (GDK_BUTTON_PRESS_MASK   |
                                      GDK_BUTTON_RELEASE_MASK |
                                      GDK_BUTTON_MOTION_MASK  |
                                      gtk_widget_get_events (widget));

      dockable->p->title_window = gdk_window_new (gtk_widget_get_window (widget),
                                               &attributes,
                                               (GDK_WA_X |
                                                GDK_WA_Y |
                                                GDK_WA_NOREDIR));

      gdk_window_set_user_data (dockable->p->title_window, widget);
    }

  gimp_dockable_cursor_setup (dockable);
}

static void
gimp_dockable_unrealize (GtkWidget *widget)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);

  if (dockable->p->title_window)
    {
      gdk_window_set_user_data (dockable->p->title_window, NULL);
      gdk_window_destroy (dockable->p->title_window);
      dockable->p->title_window = NULL;
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_dockable_map (GtkWidget *widget)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  if (dockable->p->title_window)
    gdk_window_show (dockable->p->title_window);
}

static void
gimp_dockable_unmap (GtkWidget *widget)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);

  if (dockable->p->title_window)
    gdk_window_hide (dockable->p->title_window);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
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
  GimpDockable *dockable          = GIMP_DOCKABLE (widget);
  gboolean      other_will_handle = FALSE;

  other_will_handle = gimp_paned_box_will_handle_drag (dockable->p->drag_handler,
                                                       widget,
                                                       context,
                                                       x, y,
                                                       time);

  gdk_drag_status (context, other_will_handle ? 0 : GDK_ACTION_MOVE, time);
  gimp_highlight_widget (widget, ! other_will_handle);
  return other_will_handle ? FALSE : TRUE;
}

static gboolean
gimp_dockable_drag_drop (GtkWidget      *widget,
                         GdkDragContext *context,
                         gint            x,
                         gint            y,
                         guint           time)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);
  gboolean      handled  = FALSE;

  if (gimp_paned_box_will_handle_drag (dockable->p->drag_handler,
                                       widget,
                                       context,
                                       x, y,
                                       time))
    {
      /* Make event fall through to the drag handler */
      handled = FALSE;
    }
  else
    {
      handled =
        gimp_dockbook_drop_dockable (GIMP_DOCKABLE (widget)->p->dockbook,
                                     gtk_drag_get_source_widget (context));
    }

  /* We must call gtk_drag_finish() ourselves */
  if (handled)
    gtk_drag_finish (context, TRUE, TRUE, time);

  return handled;
}

static void
gimp_dockable_style_set (GtkWidget *widget,
                         GtkStyle  *prev_style)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);
  gint          content_border;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "content-border", &content_border,
                        NULL);

  gtk_container_set_border_width (GTK_CONTAINER (widget), content_border);

  if (dockable->p->title_layout)
    {
      g_object_unref (dockable->p->title_layout);
      dockable->p->title_layout = NULL;
    }
}

static PangoLayout *
gimp_dockable_create_title_layout (GimpDockable *dockable,
                                   GtkWidget    *widget,
                                   gint          width)
{
  GtkWidget   *child = gtk_bin_get_child (GTK_BIN (dockable));
  PangoLayout *layout;
  gchar       *title = NULL;

  if (child)
    title = gimp_docked_get_title (GIMP_DOCKED (child));

  layout = gtk_widget_create_pango_layout (widget,
                                           title ? title : dockable->p->blurb);
  g_free (title);

  gimp_pango_layout_set_weight (layout, PANGO_WEIGHT_SEMIBOLD);

  if (width > 0)
    {
      pango_layout_set_width (layout, PANGO_SCALE * width);
      pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
    }

  return layout;
}

static gboolean
gimp_dockable_expose_event (GtkWidget      *widget,
                            GdkEventExpose *event)
{
  if (gtk_widget_is_drawable (widget))
    {
      GimpDockable *dockable = GIMP_DOCKABLE (widget);
      GtkStyle     *style    = gtk_widget_get_style (widget);
      GdkRectangle  title_area;
      GdkRectangle  expose_area;

      gimp_dockable_get_title_area (dockable, &title_area);

      if (gdk_rectangle_intersect (&title_area, &event->area, &expose_area))
        {
          gint layout_width;
          gint layout_height;
          gint text_x;
          gint text_y;

          if (dockable->p->blink_counter & 1)
            {
              gtk_paint_box (style, gtk_widget_get_window (widget),
                             GTK_STATE_SELECTED, GTK_SHADOW_NONE,
                             &expose_area, widget, "",
                             title_area.x, title_area.y,
                             title_area.width, title_area.height);
            }

          if (! dockable->p->title_layout)
            {
              dockable->p->title_layout =
                gimp_dockable_create_title_layout (dockable, widget,
                                                   title_area.width);
            }

          pango_layout_get_pixel_size (dockable->p->title_layout,
                                       &layout_width, &layout_height);

          if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
            {
              text_x = title_area.x;
            }
          else
            {
              text_x = title_area.x + title_area.width - layout_width;
            }

          text_y = title_area.y + (title_area.height - layout_height) / 2;

          gtk_paint_layout (style, gtk_widget_get_window (widget),
                            (dockable->p->blink_counter & 1) ?
                            GTK_STATE_SELECTED : gtk_widget_get_state (widget),
                            TRUE,
                            &expose_area, widget, NULL,
                            text_x, text_y, dockable->p->title_layout);
        }
    }

  return GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
}

static gboolean
gimp_dockable_button_press (GtkWidget      *widget,
                            GdkEventButton *event)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);

  /*  stop processing of events not coming from the title event window  */
  if (event->window != dockable->p->title_window)
    return TRUE;

  dockable->p->drag_x = event->x;

  return FALSE;
}

static gboolean
gimp_dockable_button_release (GtkWidget *widget)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);

  dockable->p->drag_x = GIMP_DOCKABLE_DRAG_OFFSET;

  return FALSE;
}

static gboolean
gimp_dockable_popup_menu (GtkWidget *widget)
{
  return gimp_dockable_show_menu (GIMP_DOCKABLE (widget));
}

static void
gimp_dockable_add (GtkContainer *container,
                   GtkWidget    *widget)
{
  GimpDockable *dockable;

  g_return_if_fail (gtk_bin_get_child (GTK_BIN (container)) == NULL);

  GTK_CONTAINER_CLASS (parent_class)->add (container, widget);

  g_signal_connect (widget, "title-changed",
                    G_CALLBACK (gimp_dockable_title_changed),
                    container);

  /*  not all tab styles are supported by all children  */
  dockable = GIMP_DOCKABLE (container);
  gimp_dockable_set_tab_style (dockable, dockable->p->tab_style);
}

static void
gimp_dockable_remove (GtkContainer *container,
                      GtkWidget    *widget)
{
  g_return_if_fail (gtk_bin_get_child (GTK_BIN (container)) == widget);

  g_signal_handlers_disconnect_by_func (widget,
                                        gimp_dockable_title_changed,
                                        container);

  GTK_CONTAINER_CLASS (parent_class)->remove (container, widget);
}

static GType
gimp_dockable_child_type (GtkContainer *container)
{
  if (gtk_bin_get_child (GTK_BIN (container)))
    return G_TYPE_NONE;

  return GIMP_TYPE_DOCKED;
}

static void
gimp_dockable_forall (GtkContainer *container,
                      gboolean      include_internals,
                      GtkCallback   callback,
                      gpointer      callback_data)
{
  GimpDockable *dockable = GIMP_DOCKABLE (container);

  if (include_internals)
    {
      if (dockable->p->menu_button)
        (* callback) (dockable->p->menu_button, callback_data);
    }

  GTK_CONTAINER_CLASS (parent_class)->forall (container, include_internals,
                                              callback, callback_data);
}

static GtkWidget *
gimp_dockable_get_icon (GimpDockable *dockable,
                        GtkIconSize   size)
{
  GdkScreen    *screen = gtk_widget_get_screen (GTK_WIDGET (dockable));
  GtkIconTheme *theme  = gtk_icon_theme_get_for_screen (screen);

  if (gtk_icon_theme_has_icon (theme, dockable->p->stock_id))
    {
      return gtk_image_new_from_icon_name (dockable->p->stock_id, size);
    }

  return  gtk_image_new_from_stock (dockable->p->stock_id, size);
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
      tab_widget = gtk_hbox_new (FALSE, dnd ? 6 : 2);

      gtk_box_pack_start (GTK_BOX (tab_widget), icon, FALSE, FALSE, 0);
      gtk_widget_show (icon);

      gtk_box_pack_start (GTK_BOX (tab_widget), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
      break;
    }

  return tab_widget;
}

/*  public functions  */

GtkWidget *
gimp_dockable_new (const gchar *name,
                   const gchar *blurb,
                   const gchar *stock_id,
                   const gchar *help_id)
{
  GimpDockable *dockable;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);

  dockable = g_object_new (GIMP_TYPE_DOCKABLE, NULL);

  dockable->p->name     = g_strdup (name);
  dockable->p->stock_id = g_strdup (stock_id);
  dockable->p->help_id  = g_strdup (help_id);

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
gimp_dockable_set_aux_info (GimpDockable *dockable,
                            GList        *aux_info)
{
  GtkWidget *child;

  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child)
    gimp_docked_set_aux_info (GIMP_DOCKED (child), aux_info);
}

GList *
gimp_dockable_get_aux_info (GimpDockable *dockable)
{
  GtkWidget *child;

  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child)
    return gimp_docked_get_aux_info (GIMP_DOCKED (child));

  return NULL;
}


void
gimp_dockable_set_locked (GimpDockable *dockable,
                          gboolean      lock)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  if (dockable->p->locked != lock)
    {
      dockable->p->locked = lock ? TRUE : FALSE;

      gimp_dockable_cursor_setup (dockable);

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
  GtkWidget *child;

  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child && ! GIMP_DOCKED_GET_INTERFACE (child)->get_preview)
    {
      switch (tab_style)
        {
        case GIMP_TAB_STYLE_PREVIEW:
          tab_style = GIMP_TAB_STYLE_ICON;
          break;

        case GIMP_TAB_STYLE_PREVIEW_NAME:
          tab_style = GIMP_TAB_STYLE_ICON_BLURB;
          break;

        case GIMP_TAB_STYLE_PREVIEW_BLURB:
          tab_style = GIMP_TAB_STYLE_ICON_BLURB;
          break;

        default:
          break;
        }
    }

  dockable->p->tab_style = tab_style;
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
  GimpDockWindow *src_dock_window = NULL;
  GimpDock       *src_dock        = NULL;
  GtkWidget      *dock            = NULL;
  GimpDockWindow *dock_window     = NULL;
  GtkWidget      *dockbook        = NULL;

  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockable->p->dockbook));

  src_dock = gimp_dockbook_get_dock (dockable->p->dockbook);
  src_dock_window = gimp_dock_window_from_dock (src_dock);

  dock = gimp_dock_with_window_new (gimp_dialog_factory_get_singleton (),
                                    gtk_widget_get_screen (GTK_WIDGET (dockable)),
                                    FALSE /*toolbox*/);
  dock_window = gimp_dock_window_from_dock (GIMP_DOCK (dock));
  gtk_window_set_position (GTK_WINDOW (dock_window), GTK_WIN_POS_MOUSE);
  if (src_dock_window)
    gimp_dock_window_setup (dock_window, src_dock_window);

  dockbook = gimp_dockbook_new (global_menu_factory);

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

  gimp_dockable_blink_timeout (dockable);
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

      gimp_dockable_clear_title_area (dockable);
    }
}


/*  private functions  */

static void
gimp_dockable_cursor_setup (GimpDockable *dockable)
{
  if (! gtk_widget_get_realized (GTK_WIDGET (dockable)))
    return;

  if (! dockable->p->title_window)
    return;

  /*  only show a hand cursor for unlocked dockables  */

  if (gimp_dockable_is_locked (dockable))
    {
      gdk_window_set_cursor (dockable->p->title_window, NULL);
    }
  else
    {
      GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (dockable));
      GdkCursor  *cursor;

      cursor = gdk_cursor_new_for_display (display, GDK_HAND2);
      gdk_window_set_cursor (dockable->p->title_window, cursor);
      gdk_cursor_unref (cursor);
    }
}

static void
gimp_dockable_get_title_area (GimpDockable *dockable,
                              GdkRectangle *area)
{
  GtkWidget     *widget = GTK_WIDGET (dockable);
  GtkAllocation  allocation;
  GtkAllocation  button_allocation;
  gint           border;

  gtk_widget_get_allocation (widget, &allocation);
  gtk_widget_get_allocation (dockable->p->menu_button, &button_allocation);

  border = gtk_container_get_border_width (GTK_CONTAINER (dockable));

  area->x      = allocation.x + border;
  area->y      = allocation.y + border;
  area->width  = allocation.width - 2 * border - button_allocation.width;
  area->height = button_allocation.height;

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    area->x += button_allocation.width;
}

static void
gimp_dockable_clear_title_area (GimpDockable *dockable)
{
  if (gtk_widget_is_drawable (GTK_WIDGET (dockable)))
    {
      GdkRectangle area;

      gimp_dockable_get_title_area (dockable, &area);

      gtk_widget_queue_draw_area (GTK_WIDGET (dockable),
                                  area.x, area.y, area.width, area.height);
    }
}

static gboolean
gimp_dockable_menu_button_press (GtkWidget      *button,
                                 GdkEventButton *bevent,
                                 GimpDockable   *dockable)
{
  if (bevent->button == 1 && bevent->type == GDK_BUTTON_PRESS)
    {
      return gimp_dockable_show_menu (dockable);
    }

  return FALSE;
}

static void
gimp_dockable_menu_position (GtkMenu  *menu,
                             gint     *x,
                             gint     *y,
                             gpointer  data)
{
  GimpDockable *dockable = GIMP_DOCKABLE (data);

  gimp_button_menu_position (dockable->p->menu_button, menu, GTK_POS_LEFT, x, y);
}

#define GIMP_DOCKABLE_DETACH_REF_KEY "gimp-dockable-detach-ref"

static void
gimp_dockable_menu_end (GimpDockable *dockable)
{
  GimpUIManager   *dialog_ui_manager;
  const gchar     *dialog_ui_path;
  gpointer         dialog_popup_data;

  dialog_ui_manager = gimp_dockable_get_menu (dockable,
                                              &dialog_ui_path,
                                              &dialog_popup_data);

  if (dialog_ui_manager && dialog_ui_path)
    {
      GtkWidget *child_menu_widget =
        gtk_ui_manager_get_widget (GTK_UI_MANAGER (dialog_ui_manager),
                                   dialog_ui_path);

      if (child_menu_widget)
        gtk_menu_detach (GTK_MENU (child_menu_widget));
    }

  /*  release gimp_dockable_show_menu()'s references  */
  g_object_set_data (G_OBJECT (dockable), GIMP_DOCKABLE_DETACH_REF_KEY, NULL);
  g_object_unref (dockable);
}

static gboolean
gimp_dockable_show_menu (GimpDockable *dockable)
{
  GimpUIManager *dockbook_ui_manager = gimp_dockbook_get_ui_manager (dockable->p->dockbook);
  GimpUIManager *dialog_ui_manager;
  const gchar   *dialog_ui_path;
  gpointer       dialog_popup_data;
  GtkWidget     *parent_menu_widget;
  GtkAction     *parent_menu_action;

  if (! dockbook_ui_manager)
    return FALSE;

  parent_menu_widget =
    gtk_ui_manager_get_widget (GTK_UI_MANAGER (dockbook_ui_manager),
                               "/dockable-popup/dockable-menu");

  parent_menu_action =
    gtk_ui_manager_get_action (GTK_UI_MANAGER (dockbook_ui_manager),
                               "/dockable-popup/dockable-menu");

  if (! parent_menu_widget || ! parent_menu_action)
    return FALSE;

  dialog_ui_manager = gimp_dockable_get_menu (dockable,
                                              &dialog_ui_path,
                                              &dialog_popup_data);

  if (dialog_ui_manager && dialog_ui_path)
    {
      GtkWidget   *child_menu_widget;
      GtkAction   *child_menu_action;
      const gchar *label;

      child_menu_widget =
        gtk_ui_manager_get_widget (GTK_UI_MANAGER (dialog_ui_manager),
                                   dialog_ui_path);

      if (! child_menu_widget)
        {
          g_warning ("%s: UI manager '%s' has now widget at path '%s'",
                     G_STRFUNC, dialog_ui_manager->name, dialog_ui_path);
          return FALSE;
        }

      child_menu_action =
        gtk_ui_manager_get_action (GTK_UI_MANAGER (dialog_ui_manager),
                                   dialog_ui_path);

      if (! child_menu_action)
        {
          g_warning ("%s: UI manager '%s' has no action at path '%s'",
                     G_STRFUNC, dialog_ui_manager->name, dialog_ui_path);
          return FALSE;
        }

      g_object_get (child_menu_action,
                    "label", &label,
                    NULL);

      g_object_set (parent_menu_action,
                    "label",    label,
                    "stock-id", dockable->p->stock_id,
                    "visible",  TRUE,
                    NULL);

      if (dockable->p->stock_id)
        {
          if (gtk_icon_theme_has_icon (gtk_icon_theme_get_default (),
                                       dockable->p->stock_id))
            {
              gtk_action_set_icon_name (parent_menu_action, dockable->p->stock_id);
            }
        }

      if (! GTK_IS_MENU (child_menu_widget))
        {
          g_warning ("%s: child_menu_widget (%p) is not a GtkMenu",
                     G_STRFUNC, child_menu_widget);
          return FALSE;
        }

      /* FIXME */
      {
        GtkWidget *image = gimp_dockable_get_icon (dockable,
                                                   GTK_ICON_SIZE_MENU);

        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (parent_menu_widget),
                                       image);
        gtk_widget_show (image);
      }

      gtk_menu_item_set_submenu (GTK_MENU_ITEM (parent_menu_widget),
                                 child_menu_widget);

      gimp_ui_manager_update (dialog_ui_manager, dialog_popup_data);
    }
  else
    {
      g_object_set (parent_menu_action, "visible", FALSE, NULL);
    }

  /*  an action callback may destroy both dockable and dockbook, so
   *  reference them for gimp_dockable_menu_end()
   */
  g_object_ref (dockable);
  g_object_set_data_full (G_OBJECT (dockable), GIMP_DOCKABLE_DETACH_REF_KEY,
                          g_object_ref (dockable->p->dockbook),
                          g_object_unref);

  gimp_ui_manager_update (dockbook_ui_manager, dockable);
  gimp_ui_manager_ui_popup (dockbook_ui_manager, "/dockable-popup",
                            GTK_WIDGET (dockable),
                            gimp_dockable_menu_position, dockable,
                            (GDestroyNotify) gimp_dockable_menu_end, dockable);

  return TRUE;
}

static gboolean
gimp_dockable_blink_timeout (GimpDockable *dockable)
{
  gimp_dockable_clear_title_area (dockable);

  if (dockable->p->blink_counter++ > 3)
    {
      dockable->p->blink_timeout_id = 0;
      dockable->p->blink_counter    = 0;

      return FALSE;
    }

  return TRUE;
}

static void
gimp_dockable_title_changed (GimpDocked   *docked,
                             GimpDockable *dockable)
{
  if (dockable->p->title_layout)
    {
      g_object_unref (dockable->p->title_layout);
      dockable->p->title_layout = NULL;
    }

  if (gtk_widget_is_drawable (GTK_WIDGET (dockable)))
    {
      GdkRectangle area;

      gimp_dockable_get_title_area (dockable, &area);

      gdk_window_invalidate_rect (gtk_widget_get_window (GTK_WIDGET (dockable)),
                                  &area, FALSE);
    }
}
