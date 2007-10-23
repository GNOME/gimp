/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockable.c
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

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
#include "gimphelp-ids.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void       gimp_dockable_destroy           (GtkObject      *object);
static void       gimp_dockable_size_request      (GtkWidget      *widget,
                                                   GtkRequisition *requisition);
static void       gimp_dockable_size_allocate     (GtkWidget      *widget,
                                                   GtkAllocation  *allocation);
static void       gimp_dockable_realize           (GtkWidget      *widget);
static void       gimp_dockable_unrealize         (GtkWidget      *widget);
static void       gimp_dockable_map               (GtkWidget      *widget);
static void       gimp_dockable_unmap             (GtkWidget      *widget);

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
                                                   gboolean       include_internals,
                                                   GtkCallback     callback,
                                                   gpointer        callback_data);

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
  GtkObjectClass    *object_class    = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class    = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->destroy       = gimp_dockable_destroy;

  widget_class->size_request  = gimp_dockable_size_request;
  widget_class->size_allocate = gimp_dockable_size_allocate;
  widget_class->realize       = gimp_dockable_realize;
  widget_class->unrealize     = gimp_dockable_unrealize;
  widget_class->map           = gimp_dockable_map;
  widget_class->unmap         = gimp_dockable_unmap;
  widget_class->style_set     = gimp_dockable_style_set;
  widget_class->drag_drop     = gimp_dockable_drag_drop;
  widget_class->expose_event  = gimp_dockable_expose_event;
  widget_class->popup_menu    = gimp_dockable_popup_menu;

  container_class->add        = gimp_dockable_add;
  container_class->remove     = gimp_dockable_remove;
  container_class->child_type = gimp_dockable_child_type;
  container_class->forall     = gimp_dockable_forall;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content-border",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             GIMP_PARAM_READABLE));
}

static void
gimp_dockable_init (GimpDockable *dockable)
{
  GtkWidget *image;

  dockable->name         = NULL;
  dockable->blurb        = NULL;
  dockable->stock_id     = NULL;
  dockable->help_id      = NULL;
  dockable->tab_style    = GIMP_TAB_STYLE_PREVIEW;
  dockable->dockbook     = NULL;
  dockable->context      = NULL;

  dockable->title_layout = NULL;
  dockable->title_window = NULL;

  dockable->drag_x       = GIMP_DOCKABLE_DRAG_OFFSET;
  dockable->drag_y       = GIMP_DOCKABLE_DRAG_OFFSET;

  gtk_widget_push_composite_child ();
  dockable->menu_button = gtk_button_new ();
  gtk_widget_pop_composite_child ();

  GTK_WIDGET_UNSET_FLAGS (dockable->menu_button, GTK_CAN_FOCUS);
  gtk_widget_set_parent (dockable->menu_button, GTK_WIDGET (dockable));
  gtk_button_set_relief (GTK_BUTTON (dockable->menu_button), GTK_RELIEF_NONE);
  gtk_widget_show (dockable->menu_button);

  image = gtk_image_new_from_stock (GIMP_STOCK_MENU_LEFT, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (dockable->menu_button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (dockable->menu_button, _("Configure this tab"),
                           GIMP_HELP_DOCK_TAB_MENU);

  g_signal_connect (dockable->menu_button, "button-press-event",
                    G_CALLBACK (gimp_dockable_menu_button_press),
                    dockable);

  gtk_drag_dest_set (GTK_WIDGET (dockable),
                     GTK_DEST_DEFAULT_ALL,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);

  /*  Filter out all button_press events not coming from the event window
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
gimp_dockable_destroy (GtkObject *object)
{
  GimpDockable *dockable = GIMP_DOCKABLE (object);

  gimp_dockable_blink_cancel (dockable);

  if (dockable->context)
    gimp_dockable_set_context (dockable, NULL);

  if (dockable->blurb)
    {
      if (dockable->blurb != dockable->name)
        g_free (dockable->blurb);

      dockable->blurb = NULL;
    }

  if (dockable->name)
    {
      g_free (dockable->name);
      dockable->name = NULL;
    }

  if (dockable->stock_id)
    {
      g_free (dockable->stock_id);
      dockable->stock_id = NULL;
    }

  if (dockable->help_id)
    {
      g_free (dockable->help_id);
      dockable->help_id = NULL;
    }

  if (dockable->title_layout)
    {
      g_object_unref (dockable->title_layout);
      dockable->title_layout = NULL;
    }

  if (dockable->menu_button)
    {
      gtk_widget_unparent (dockable->menu_button);
      dockable->menu_button = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_dockable_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
  GtkContainer   *container = GTK_CONTAINER (widget);
  GtkBin         *bin       = GTK_BIN (widget);
  GimpDockable   *dockable  = GIMP_DOCKABLE (widget);
  GtkRequisition  child_requisition;

  requisition->width  = container->border_width * 2;
  requisition->height = container->border_width * 2;

  if (dockable->menu_button && GTK_WIDGET_VISIBLE (dockable->menu_button))
    {
      gtk_widget_size_request (dockable->menu_button, &child_requisition);

      if (! bin->child)
        requisition->width += child_requisition.width;

      requisition->height += child_requisition.height;
    }

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    {
      gtk_widget_size_request (bin->child, &child_requisition);

      requisition->width  += child_requisition.width;
      requisition->height += child_requisition.height;
    }
}

static void
gimp_dockable_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  GtkContainer   *container = GTK_CONTAINER (widget);
  GtkBin         *bin       = GTK_BIN (widget);
  GimpDockable   *dockable  = GIMP_DOCKABLE (widget);

  GtkRequisition  button_requisition = { 0, };
  GtkAllocation   child_allocation;

  widget->allocation = *allocation;

  if (dockable->menu_button && GTK_WIDGET_VISIBLE (dockable->menu_button))
    {
      gtk_widget_size_request (dockable->menu_button, &button_requisition);

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
        child_allocation.x    = (allocation->x +
                                 allocation->width -
                                 container->border_width -
                                 button_requisition.width);
      else
        child_allocation.x    = allocation->x + container->border_width;

      child_allocation.y      = allocation->y + container->border_width;
      child_allocation.width  = button_requisition.width;
      child_allocation.height = button_requisition.height;

      gtk_widget_size_allocate (dockable->menu_button, &child_allocation);
    }

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    {
      child_allocation.x      = allocation->x + container->border_width;
      child_allocation.y      = allocation->y + container->border_width;
      child_allocation.width  = MAX (allocation->width  -
                                     container->border_width * 2,
                                     0);
      child_allocation.height = MAX (allocation->height -
                                     container->border_width * 2 -
                                     button_requisition.height,
                                     0);

      child_allocation.y += button_requisition.height;

      gtk_widget_size_allocate (bin->child, &child_allocation);
    }

  if (dockable->title_window)
    {
      GdkRectangle  area;

      gimp_dockable_get_title_area (dockable, &area);

      gdk_window_move_resize (dockable->title_window,
                              area.x, area.y, area.width, area.height);

      if (dockable->title_layout)
        pango_layout_set_width (dockable->title_layout,
                                PANGO_SCALE * area.width);
    }
}

static void
gimp_dockable_realize (GtkWidget *widget)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  if (! dockable->title_window)
    {
      GdkWindowAttr  attributes;
      GdkRectangle   area;
      GdkCursor     *cursor;

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

      dockable->title_window = gdk_window_new (widget->window,
                                               &attributes,
                                               (GDK_WA_X |
                                                GDK_WA_Y |
                                                GDK_WA_NOREDIR));

      gdk_window_set_user_data (dockable->title_window, widget);

      cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                           GDK_HAND2);
      gdk_window_set_cursor (dockable->title_window, cursor);
      gdk_cursor_unref (cursor);
    }
}

static void
gimp_dockable_unrealize (GtkWidget *widget)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);

  if (dockable->title_window)
    {
      gdk_window_set_user_data (dockable->title_window, NULL);
      gdk_window_destroy (dockable->title_window);
      dockable->title_window = NULL;
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_dockable_map (GtkWidget *widget)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  if (dockable->title_window)
    gdk_window_show (dockable->title_window);
}

static void
gimp_dockable_unmap (GtkWidget *widget)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);

  if (dockable->title_window)
    gdk_window_hide (dockable->title_window);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static gboolean
gimp_dockable_drag_drop (GtkWidget      *widget,
                         GdkDragContext *context,
                         gint            x,
                         gint            y,
                         guint           time)
{
  return gimp_dockbook_drop_dockable (GIMP_DOCKABLE (widget)->dockbook,
                                      gtk_drag_get_source_widget (context));
}

static void
gimp_dockable_style_set (GtkWidget *widget,
                         GtkStyle  *prev_style)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);
  gint          content_border;

  gtk_widget_style_get (widget,
                        "content-border", &content_border,
                        NULL);

  gtk_container_set_border_width (GTK_CONTAINER (widget), content_border);

  if (dockable->title_layout)
    {
      g_object_unref (dockable->title_layout);
      dockable->title_layout = NULL;
    }

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}

static PangoLayout *
gimp_dockable_create_title_layout (GimpDockable *dockable,
                                   GtkWidget    *widget,
                                   gint          width)
{
  PangoLayout *layout;
  GtkBin      *bin  = GTK_BIN (dockable);
  gchar       *title = NULL;

  if (bin->child)
    title = gimp_docked_get_title (GIMP_DOCKED (bin->child));

  layout = gtk_widget_create_pango_layout (widget,
                                           title ? title : dockable->blurb);
  g_free (title);

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
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GimpDockable *dockable = GIMP_DOCKABLE (widget);
      GdkRectangle  title_area;
      GdkRectangle  expose_area;

      gimp_dockable_get_title_area (dockable, &title_area);

      if (gdk_rectangle_intersect (&title_area, &event->area, &expose_area))
        {
          gint layout_width;
          gint layout_height;
          gint text_x;
          gint text_y;

          if (dockable->blink_counter & 1)
            {
              gtk_paint_box (widget->style, widget->window,
                             GTK_STATE_SELECTED, GTK_SHADOW_NONE,
                             &expose_area, widget, "",
                             title_area.x, title_area.y,
                             title_area.width, title_area.height);
            }

          if (! dockable->title_layout)
            {
              dockable->title_layout =
                gimp_dockable_create_title_layout (dockable, widget,
                                                   title_area.width);
            }

          pango_layout_get_pixel_size (dockable->title_layout,
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

          gtk_paint_layout (widget->style, widget->window,
                            (dockable->blink_counter & 1) ?
                            GTK_STATE_SELECTED : widget->state, TRUE,
                            &expose_area, widget, NULL,
                            text_x, text_y, dockable->title_layout);
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
  if (event->window != dockable->title_window)
    return TRUE;

  dockable->drag_x = event->x;

  return FALSE;
}

static gboolean
gimp_dockable_button_release (GtkWidget *widget)
{
  GimpDockable *dockable = GIMP_DOCKABLE (widget);

  dockable->drag_x = GIMP_DOCKABLE_DRAG_OFFSET;

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

  g_return_if_fail (GTK_BIN (container)->child == NULL);

  GTK_CONTAINER_CLASS (parent_class)->add (container, widget);

  g_signal_connect (widget, "title-changed",
                    G_CALLBACK (gimp_dockable_title_changed),
                    container);

  /*  not all tab styles are supported by all children  */
  dockable = GIMP_DOCKABLE (container);
  gimp_dockable_set_tab_style (dockable, dockable->tab_style);
}

static void
gimp_dockable_remove (GtkContainer *container,
                      GtkWidget    *widget)
{
  g_return_if_fail (GTK_BIN (container)->child == widget);

  g_signal_handlers_disconnect_by_func (widget,
                                        gimp_dockable_title_changed,
                                        container);

  GTK_CONTAINER_CLASS (parent_class)->remove (container, widget);
}

static GType
gimp_dockable_child_type (GtkContainer *container)
{
  if (GTK_BIN (container)->child)
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
      if (dockable->menu_button)
        (* callback) (dockable->menu_button, callback_data);
    }

  GTK_CONTAINER_CLASS (parent_class)->forall (container, include_internals,
                                              callback, callback_data);
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

  dockable->name     = g_strdup (name);
  dockable->stock_id = g_strdup (stock_id);
  dockable->help_id  = g_strdup (help_id);

  if (blurb)
    dockable->blurb  = g_strdup (blurb);
  else
    dockable->blurb  = dockable->name;

  gimp_help_set_help_data (GTK_WIDGET (dockable), NULL, help_id);

  return GTK_WIDGET (dockable);
}

void
gimp_dockable_set_aux_info (GimpDockable *dockable,
                            GList        *aux_info)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  if (GTK_BIN (dockable)->child)
    gimp_docked_set_aux_info (GIMP_DOCKED (GTK_BIN (dockable)->child),
                              aux_info);
}

GList *
gimp_dockable_get_aux_info (GimpDockable *dockable)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);


  if (GTK_BIN (dockable)->child)
    return gimp_docked_get_aux_info (GIMP_DOCKED (GTK_BIN (dockable)->child));

  return NULL;
}

void
gimp_dockable_set_tab_style (GimpDockable *dockable,
                             GimpTabStyle  tab_style)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  if (GTK_BIN (dockable)->child &&
      ! GIMP_DOCKED_GET_INTERFACE (GTK_BIN (dockable)->child)->get_preview)
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

  dockable->tab_style = tab_style;
}

GtkWidget *
gimp_dockable_get_tab_widget (GimpDockable *dockable,
                              GimpContext  *context,
                              GimpTabStyle  tab_style,
                              GtkIconSize   size)
{
  GtkWidget *tab_widget = NULL;
  GtkWidget *label      = NULL;
  GtkWidget *icon       = NULL;

  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  switch (tab_style)
    {
    case GIMP_TAB_STYLE_NAME:
    case GIMP_TAB_STYLE_ICON_NAME:
    case GIMP_TAB_STYLE_PREVIEW_NAME:
      label = gtk_label_new (dockable->name);
      break;

    case GIMP_TAB_STYLE_BLURB:
    case GIMP_TAB_STYLE_ICON_BLURB:
    case GIMP_TAB_STYLE_PREVIEW_BLURB:
      label = gtk_label_new (dockable->blurb);
      break;

    default:
      break;
    }

  switch (tab_style)
    {
    case GIMP_TAB_STYLE_ICON:
    case GIMP_TAB_STYLE_ICON_NAME:
    case GIMP_TAB_STYLE_ICON_BLURB:
      icon = gtk_image_new_from_stock (dockable->stock_id, size);
      break;

    case GIMP_TAB_STYLE_PREVIEW:
    case GIMP_TAB_STYLE_PREVIEW_NAME:
    case GIMP_TAB_STYLE_PREVIEW_BLURB:
      if (GTK_BIN (dockable)->child)
        icon = gimp_docked_get_preview (GIMP_DOCKED (GTK_BIN (dockable)->child),
                                        context, size);

      if (! icon)
        icon = gtk_image_new_from_stock (dockable->stock_id, size);
      break;

    default:
      break;
    }

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
      tab_widget = gtk_hbox_new (FALSE, 2);

      gtk_box_pack_start (GTK_BOX (tab_widget), icon, FALSE, FALSE, 0);
      gtk_widget_show (icon);

      gtk_box_pack_start (GTK_BOX (tab_widget), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
      break;
    }

  return tab_widget;
}

void
gimp_dockable_set_context (GimpDockable *dockable,
                           GimpContext  *context)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  if (context != dockable->context)
    {
      if (GTK_BIN (dockable)->child)
        gimp_docked_set_context (GIMP_DOCKED (GTK_BIN (dockable)->child),
                                 context);

      dockable->context = context;
    }
}

GimpUIManager *
gimp_dockable_get_menu (GimpDockable  *dockable,
                        const gchar  **ui_path,
                        gpointer      *popup_data)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);
  g_return_val_if_fail (ui_path != NULL, NULL);
  g_return_val_if_fail (popup_data != NULL, NULL);

  if (GTK_BIN (dockable)->child)
    return gimp_docked_get_menu (GIMP_DOCKED (GTK_BIN (dockable)->child),
                                 ui_path, popup_data);

  return NULL;
}

void
gimp_dockable_detach (GimpDockable *dockable)
{
  GimpDock  *src_dock;
  GtkWidget *dock;
  GtkWidget *dockbook;

  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockable->dockbook));

  src_dock = dockable->dockbook->dock;

  dock = gimp_dialog_factory_dock_new (src_dock->dialog_factory,
                                       gtk_widget_get_screen (GTK_WIDGET (dockable)));
  gimp_dock_setup (GIMP_DOCK (dock), src_dock);

  dockbook = gimp_dockbook_new (GIMP_DOCK (dock)->dialog_factory->menu_factory);

  gimp_dock_add_book (GIMP_DOCK (dock), GIMP_DOCKBOOK (dockbook), 0);

  g_object_ref (dockable);

  gimp_dockbook_remove (dockable->dockbook, dockable);
  gimp_dockbook_add (GIMP_DOCKBOOK (dockbook), dockable, 0);

  g_object_unref (dockable);

  gtk_widget_show (dock);
}

void
gimp_dockable_blink (GimpDockable *dockable)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  if (dockable->blink_timeout_id)
    g_source_remove (dockable->blink_timeout_id);

  dockable->blink_timeout_id =
    g_timeout_add (150, (GSourceFunc) gimp_dockable_blink_timeout, dockable);

  gimp_dockable_blink_timeout (dockable);
}

void
gimp_dockable_blink_cancel (GimpDockable *dockable)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  if (dockable->blink_timeout_id)
    {
      g_source_remove (dockable->blink_timeout_id);

      dockable->blink_timeout_id = 0;
      dockable->blink_counter    = 0;

      gimp_dockable_clear_title_area (dockable);
    }
}

/*  private functions  */

static void
gimp_dockable_get_title_area (GimpDockable *dockable,
                              GdkRectangle *area)
{
  GtkWidget *widget = GTK_WIDGET (dockable);
  gint       border = GTK_CONTAINER (dockable)->border_width;

  area->x      = widget->allocation.x + border;
  area->y      = widget->allocation.y + border;
  area->width  = (widget->allocation.width -
                  2 * border - dockable->menu_button->allocation.width);
  area->height = dockable->menu_button->allocation.height;

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    area->x += dockable->menu_button->allocation.width;
}

static void
gimp_dockable_clear_title_area (GimpDockable *dockable)
{
  if (GTK_WIDGET_DRAWABLE (dockable))
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

  gimp_button_menu_position (dockable->menu_button, menu, GTK_POS_LEFT, x, y);
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
  GimpUIManager *dockbook_ui_manager = dockable->dockbook->ui_manager;
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
                    "stock-id", dockable->stock_id,
                    "visible",  TRUE,
                    NULL);

      if (! GTK_IS_MENU (child_menu_widget))
        {
          g_warning ("%s: child_menu_widget (%p) is not a GtkMenu",
                     G_STRFUNC, child_menu_widget);
          return FALSE;
        }

      /* FIXME */
      {
        GtkWidget *image = gtk_image_new_from_stock (dockable->stock_id,
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
                          g_object_ref (dockable->dockbook),
                          g_object_unref);

  gimp_ui_manager_update (dockbook_ui_manager, dockable);
  gimp_ui_manager_ui_popup (dockbook_ui_manager, "/dockable-popup",
                            GTK_WIDGET (dockable),
                            gimp_dockable_menu_position, dockable,
                            (GtkDestroyNotify) gimp_dockable_menu_end, dockable);

  return TRUE;
}

static gboolean
gimp_dockable_blink_timeout (GimpDockable *dockable)
{
  gimp_dockable_clear_title_area (dockable);

  if (dockable->blink_counter++ > 3)
    {
      dockable->blink_timeout_id = 0;
      dockable->blink_counter    = 0;

      return FALSE;
    }

  return TRUE;
}

static void
gimp_dockable_title_changed (GimpDocked   *docked,
                             GimpDockable *dockable)
{
  if (dockable->title_layout)
    {
      g_object_unref (dockable->title_layout);
      dockable->title_layout = NULL;
    }

  if (GTK_WIDGET_DRAWABLE (dockable))
    {
      GdkRectangle area;

      gimp_dockable_get_title_area (dockable, &area);

      gdk_window_invalidate_rect (GTK_WIDGET (dockable)->window, &area, FALSE);
    }
}
