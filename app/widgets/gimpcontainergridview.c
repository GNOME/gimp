/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainergridview.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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
#include <gdk/gdkkeysyms.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"
#include "core/gimpviewable.h"

#include "gimpcontainergridview.h"
#include "gimpcontainerview.h"
#include "gimpview.h"
#include "gimpviewrenderer.h"
#include "gimpwidgets-utils.h"
#include "gtkhwrapbox.h"

#include "gimp-intl.h"


enum
{
  MOVE_CURSOR,
  LAST_SIGNAL
};


static void     gimp_container_grid_view_class_init   (GimpContainerGridViewClass *klass);
static void     gimp_container_grid_view_init         (GimpContainerGridView      *view);

static void     gimp_container_grid_view_view_iface_init (GimpContainerViewInterface *view_iface);

static gboolean gimp_container_grid_view_move_cursor  (GimpContainerGridView  *view,
                                                       GtkMovementStep         step,
                                                       gint                    count);
static gboolean gimp_container_grid_view_focus        (GtkWidget              *widget,
                                                       GtkDirectionType        direction);
static gboolean  gimp_container_grid_view_popup_menu  (GtkWidget              *widget);

static gpointer gimp_container_grid_view_insert_item  (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gint                    index);
static void     gimp_container_grid_view_remove_item  (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gpointer                insert_data);
static void     gimp_container_grid_view_reorder_item (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gint                    new_index,
                                                       gpointer                insert_data);
static void     gimp_container_grid_view_rename_item  (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gpointer                insert_data);
static gboolean  gimp_container_grid_view_select_item (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gpointer                insert_data);
static void     gimp_container_grid_view_clear_items  (GimpContainerView      *view);
static void gimp_container_grid_view_set_preview_size (GimpContainerView      *view);
static gboolean gimp_container_grid_view_item_selected(GtkWidget              *widget,
                                                       GdkEventButton         *bevent,
                                                       gpointer                data);
static void   gimp_container_grid_view_item_activated (GtkWidget              *widget,
                                                       gpointer                data);
static void   gimp_container_grid_view_item_context   (GtkWidget              *widget,
                                                       gpointer                data);
static void   gimp_container_grid_view_highlight_item (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gpointer                insert_data);

static void gimp_container_grid_view_viewport_resized (GtkWidget              *widget,
                                                       GtkAllocation          *allocation,
                                                       GimpContainerGridView  *view);


static GimpContainerBoxClass      *parent_class      = NULL;
static GimpContainerViewInterface *parent_view_iface = NULL;

static guint grid_view_signals[LAST_SIGNAL] = { 0 };

static GimpRGB  white_color;
static GimpRGB  black_color;


GType
gimp_container_grid_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpContainerGridViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_container_grid_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpContainerGridView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_container_grid_view_init,
      };

      static const GInterfaceInfo view_iface_info =
      {
        (GInterfaceInitFunc) gimp_container_grid_view_view_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      view_type = g_type_register_static (GIMP_TYPE_CONTAINER_BOX,
                                          "GimpContainerGridView",
                                          &view_info, 0);

      g_type_add_interface_static (view_type, GIMP_TYPE_CONTAINER_VIEW,
                                   &view_iface_info);
    }

  return view_type;
}

static void
gimp_container_grid_view_class_init (GimpContainerGridViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkBindingSet  *binding_set;

  parent_class = g_type_class_peek_parent (klass);
  binding_set  = gtk_binding_set_by_class (klass);

  widget_class->focus      = gimp_container_grid_view_focus;
  widget_class->popup_menu = gimp_container_grid_view_popup_menu;

  klass->move_cursor       = gimp_container_grid_view_move_cursor;

  grid_view_signals[MOVE_CURSOR] =
    g_signal_new ("move_cursor",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpContainerGridViewClass, move_cursor),
                  NULL, NULL,
                  gimp_marshal_BOOLEAN__ENUM_INT,
                  G_TYPE_BOOLEAN, 2,
                  GTK_TYPE_MOVEMENT_STEP,
                  G_TYPE_INT);

  gtk_binding_entry_add_signal (binding_set, GDK_Home, 0,
                                "move_cursor", 2,
                                G_TYPE_ENUM, GTK_MOVEMENT_BUFFER_ENDS,
                                G_TYPE_INT, -1);
  gtk_binding_entry_add_signal (binding_set, GDK_End, 0,
                                "move_cursor", 2,
                                G_TYPE_ENUM, GTK_MOVEMENT_BUFFER_ENDS,
                                G_TYPE_INT, 1);
  gtk_binding_entry_add_signal (binding_set, GDK_Page_Up, 0,
                                "move_cursor", 2,
                                G_TYPE_ENUM, GTK_MOVEMENT_PAGES,
                                G_TYPE_INT, -1);
  gtk_binding_entry_add_signal (binding_set, GDK_Page_Down, 0,
                                "move_cursor", 2,
                                G_TYPE_ENUM, GTK_MOVEMENT_PAGES,
                                G_TYPE_INT, 1);

  gimp_rgba_set (&white_color, 1.0, 1.0, 1.0, 1.0);
  gimp_rgba_set (&black_color, 0.0, 0.0, 0.0, 1.0);
}

static void
gimp_container_grid_view_init (GimpContainerGridView *grid_view)
{
  GimpContainerBox *box = GIMP_CONTAINER_BOX (grid_view);

  grid_view->rows          = 1;
  grid_view->columns       = 1;
  grid_view->visible_rows  = 0;
  grid_view->selected_item = NULL;

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (box->scrolled_win),
                                  GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  grid_view->name_label = gtk_label_new (_("(None)"));
  gtk_misc_set_alignment (GTK_MISC (grid_view->name_label), 0.0, 0.5);
  gimp_label_set_attributes (GTK_LABEL (grid_view->name_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (grid_view), grid_view->name_label,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (grid_view), grid_view->name_label, 0);
  gtk_widget_show (grid_view->name_label);

  grid_view->wrap_box = gtk_hwrap_box_new (FALSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (box->scrolled_win),
                                         grid_view->wrap_box);
  gtk_widget_show (grid_view->wrap_box);

  g_signal_connect (grid_view->wrap_box->parent, "size_allocate",
                    G_CALLBACK (gimp_container_grid_view_viewport_resized),
                    grid_view);

  GTK_WIDGET_SET_FLAGS (grid_view, GTK_CAN_FOCUS);
}

static void
gimp_container_grid_view_view_iface_init (GimpContainerViewInterface *view_iface)
{
  parent_view_iface = g_type_interface_peek_parent (view_iface);

  view_iface->insert_item      = gimp_container_grid_view_insert_item;
  view_iface->remove_item      = gimp_container_grid_view_remove_item;
  view_iface->reorder_item     = gimp_container_grid_view_reorder_item;
  view_iface->rename_item      = gimp_container_grid_view_rename_item;
  view_iface->select_item      = gimp_container_grid_view_select_item;
  view_iface->clear_items      = gimp_container_grid_view_clear_items;
  view_iface->set_preview_size = gimp_container_grid_view_set_preview_size;
}

GtkWidget *
gimp_container_grid_view_new (GimpContainer *container,
                              GimpContext   *context,
                              gint           view_size,
                              gint           view_border_width)
{
  GimpContainerGridView *grid_view;
  GimpContainerView     *view;

  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size  > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  grid_view = g_object_new (GIMP_TYPE_CONTAINER_GRID_VIEW, NULL);

  view = GIMP_CONTAINER_VIEW (grid_view);

  gimp_container_view_set_preview_size (view, view_size,
                                        view_border_width);

  if (container)
    gimp_container_view_set_container (view, container);

  if (context)
    gimp_container_view_set_context (view, context);

  return GTK_WIDGET (grid_view);
}

static gboolean
gimp_container_grid_view_move_by (GimpContainerGridView *grid_view,
                                  gint                   x,
                                  gint                   y)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (grid_view);
  GimpContainer     *container;
  GimpViewable      *item;
  gint               index;

  if (! grid_view->selected_item)
    return FALSE;

  container = gimp_container_view_get_container (view);

  item = grid_view->selected_item->viewable;

  index = gimp_container_get_child_index (container, GIMP_OBJECT (item));

  index += x;
  index = CLAMP (index, 0, container->num_children - 1);

  index += y * grid_view->columns;
  while (index < 0)
    index += grid_view->columns;
  while (index >= container->num_children)
    index -= grid_view->columns;

  item = (GimpViewable *) gimp_container_get_child_by_index (container, index);
  if (item)
    gimp_container_view_item_selected (GIMP_CONTAINER_VIEW (view), item);

  return TRUE;
}

static gboolean
gimp_container_grid_view_move_cursor (GimpContainerGridView *grid_view,
                                      GtkMovementStep        step,
                                      gint                   count)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (grid_view);
  GimpContainer     *container;
  GimpViewable      *item;

  if (! GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (grid_view)) || count == 0)
    return FALSE;

  container = gimp_container_view_get_container (view);

  switch (step)
    {
    case GTK_MOVEMENT_PAGES:
      return gimp_container_grid_view_move_by (grid_view, 0,
                                               count * grid_view->visible_rows);

    case GTK_MOVEMENT_BUFFER_ENDS:
      count = count < 0 ? 0 : container->num_children - 1;

      item = (GimpViewable *) gimp_container_get_child_by_index (container,
                                                                 count);
      if (item)
        gimp_container_view_item_selected (view, item);

      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static gboolean
gimp_container_grid_view_focus (GtkWidget        *widget,
                                GtkDirectionType  direction)
{
  GimpContainerGridView *view = GIMP_CONTAINER_GRID_VIEW (widget);

  if (GTK_WIDGET_CAN_FOCUS (widget) && ! GTK_WIDGET_HAS_FOCUS (widget))
    {
      gtk_widget_grab_focus (GTK_WIDGET (widget));
      return TRUE;
    }

  switch (direction)
    {
    case GTK_DIR_UP:
      return gimp_container_grid_view_move_by (view,  0, -1);
    case GTK_DIR_DOWN:
      return gimp_container_grid_view_move_by (view,  0,  1);
    case GTK_DIR_LEFT:
      return gimp_container_grid_view_move_by (view, -1,  0);
    case GTK_DIR_RIGHT:
      return gimp_container_grid_view_move_by (view,  1,  0);

    case GTK_DIR_TAB_FORWARD:
    case GTK_DIR_TAB_BACKWARD:
      break;
    }

  return FALSE;
}

static void
gimp_container_grid_view_menu_position (GtkMenu  *menu,
                                        gint     *x,
                                        gint     *y,
                                        gpointer  data)
{
  GtkWidget *widget = GTK_WIDGET (data);

  gdk_window_get_origin (widget->window, x, y);

  if (GTK_WIDGET_NO_WINDOW (widget))
    {
      *x += widget->allocation.x;
      *y += widget->allocation.y;
    }

  *x += widget->allocation.width  / 2;
  *y += widget->allocation.height / 2;

  gimp_menu_position (menu, x, y);
}

static gboolean
gimp_container_grid_view_popup_menu (GtkWidget *widget)
{
  GimpContainerGridView *grid_view = GIMP_CONTAINER_GRID_VIEW (widget);

  if (grid_view->selected_item)
    {
      return gimp_editor_popup_menu (GIMP_EDITOR (grid_view),
                                     gimp_container_grid_view_menu_position,
                                     grid_view->selected_item);
    }

  return FALSE;
}

static gpointer
gimp_container_grid_view_insert_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gint               index)
{
  GimpContainerGridView *grid_view = GIMP_CONTAINER_GRID_VIEW (view);
  GtkWidget             *preview;
  gint                   preview_size;

  preview_size = gimp_container_view_get_preview_size (view, NULL);

  preview = gimp_view_new_full (viewable,
                                preview_size,
                                preview_size,
                                1,
                                FALSE, TRUE, TRUE);
  gimp_view_renderer_set_border_type (GIMP_VIEW (preview)->renderer,
                                      GIMP_VIEW_BORDER_WHITE);
  gimp_view_renderer_remove_idle (GIMP_VIEW (preview)->renderer);

  gtk_wrap_box_pack (GTK_WRAP_BOX (grid_view->wrap_box), preview,
                     FALSE, FALSE, FALSE, FALSE);

  if (index != -1)
    gtk_wrap_box_reorder_child (GTK_WRAP_BOX (grid_view->wrap_box),
                                preview, index);

  gtk_widget_show (preview);

  g_signal_connect (preview, "button_press_event",
                    G_CALLBACK (gimp_container_grid_view_item_selected),
                    view);
  g_signal_connect (preview, "double_clicked",
                    G_CALLBACK (gimp_container_grid_view_item_activated),
                    view);
  g_signal_connect (preview, "context",
                    G_CALLBACK (gimp_container_grid_view_item_context),
                    view);

  return (gpointer) preview;
}

static void
gimp_container_grid_view_remove_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gpointer           insert_data)
{
  GimpContainerGridView *grid_view = GIMP_CONTAINER_GRID_VIEW (view);
  GtkWidget             *preview   = GTK_WIDGET (insert_data);

  if (preview == (GtkWidget *) grid_view->selected_item)
    grid_view->selected_item = NULL;

  gtk_container_remove (GTK_CONTAINER (grid_view->wrap_box), preview);
}

static void
gimp_container_grid_view_reorder_item (GimpContainerView *view,
                                       GimpViewable      *viewable,
                                       gint               new_index,
                                       gpointer           insert_data)
{
  GimpContainerGridView *grid_view = GIMP_CONTAINER_GRID_VIEW (view);
  GtkWidget             *preview   = GTK_WIDGET (insert_data);

  gtk_wrap_box_reorder_child (GTK_WRAP_BOX (grid_view->wrap_box),
                              preview, new_index);
}

static void
gimp_container_grid_view_rename_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gpointer           insert_data)
{
  GimpContainerGridView *grid_view = GIMP_CONTAINER_GRID_VIEW (view);
  GtkWidget             *preview   = GTK_WIDGET (insert_data);

  if (preview == (GtkWidget *) grid_view->selected_item)
    {
      gchar *name = gimp_viewable_get_description (viewable, NULL);

      gtk_label_set_text (GTK_LABEL (grid_view->name_label), name);

      g_free (name);
    }
}

static gboolean
gimp_container_grid_view_select_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gpointer           insert_data)
{
  gimp_container_grid_view_highlight_item (view, viewable, insert_data);

  return TRUE;
}

static void
gimp_container_grid_view_clear_items (GimpContainerView *view)
{
  GimpContainerGridView *grid_view = GIMP_CONTAINER_GRID_VIEW (view);

  grid_view->selected_item = NULL;

  while (GTK_WRAP_BOX (grid_view->wrap_box)->children)
    gtk_container_remove (GTK_CONTAINER (grid_view->wrap_box),
                          GTK_WRAP_BOX (grid_view->wrap_box)->children->widget);

  parent_view_iface->clear_items (view);
}

static void
gimp_container_grid_view_set_preview_size (GimpContainerView *view)
{
  GimpContainerGridView *grid_view = GIMP_CONTAINER_GRID_VIEW (view);
  GtkWrapBoxChild       *child;
  gint                   preview_size;

  preview_size = gimp_container_view_get_preview_size (view, NULL);

  for (child = GTK_WRAP_BOX (grid_view->wrap_box)->children;
       child;
       child = child->next)
    {
      GimpView *view = GIMP_VIEW (child->widget);

      gimp_view_renderer_set_size (view->renderer,
                                   preview_size,
                                   view->renderer->border_width);
    }

  gtk_widget_queue_resize (grid_view->wrap_box);
}

static gboolean
gimp_container_grid_view_item_selected (GtkWidget      *widget,
                                        GdkEventButton *bevent,
                                        gpointer        data)
{
  if (bevent->type == GDK_BUTTON_PRESS && bevent->button == 1)
    {
      if (GTK_WIDGET_CAN_FOCUS (data) && ! GTK_WIDGET_HAS_FOCUS (data))
        gtk_widget_grab_focus (GTK_WIDGET (data));

      gimp_container_view_item_selected (GIMP_CONTAINER_VIEW (data),
                                         GIMP_VIEW (widget)->viewable);
    }

  return FALSE;
}

static void
gimp_container_grid_view_item_activated (GtkWidget *widget,
                                         gpointer   data)
{
  gimp_container_view_item_activated (GIMP_CONTAINER_VIEW (data),
                                      GIMP_VIEW (widget)->viewable);
}

static void
gimp_container_grid_view_item_context (GtkWidget *widget,
                                       gpointer   data)
{
  /*  ref the view because calling gimp_container_view_item_selected()
   *  may destroy the widget
   */
  g_object_ref (data);

  if (gimp_container_view_item_selected (GIMP_CONTAINER_VIEW (data),
                                         GIMP_VIEW (widget)->viewable))
    {
      gimp_container_view_item_context (GIMP_CONTAINER_VIEW (data),
                                        GIMP_VIEW (widget)->viewable);
    }

  g_object_unref (data);
}

static void
gimp_container_grid_view_highlight_item (GimpContainerView *view,
                                         GimpViewable      *viewable,
                                         gpointer           insert_data)
{
  GimpContainerGridView *grid_view = GIMP_CONTAINER_GRID_VIEW (view);
  GimpContainerBox      *box       = GIMP_CONTAINER_BOX (view);
  GimpContainer         *container;
  GimpView              *preview   = NULL;

  container = gimp_container_view_get_container (view);

  if (insert_data)
    preview = GIMP_VIEW (insert_data);

  if (grid_view->selected_item && grid_view->selected_item != preview)
    {
      gimp_view_renderer_set_border_type (grid_view->selected_item->renderer,
                                          GIMP_VIEW_BORDER_WHITE);
      gimp_view_renderer_update (grid_view->selected_item->renderer);
    }

  if (preview)
    {
      GtkRequisition  preview_requisition;
      GtkAdjustment  *adj;
      gint            item_height;
      gint            index;
      gint            row;
      gchar          *name;

      adj = gtk_scrolled_window_get_vadjustment
        (GTK_SCROLLED_WINDOW (box->scrolled_win));

      gtk_widget_size_request (GTK_WIDGET (preview), &preview_requisition);

      item_height = preview_requisition.height;

      index = gimp_container_get_child_index (container,
                                              GIMP_OBJECT (viewable));

      row = index / grid_view->columns;

      if (row * item_height < adj->value)
        {
          gtk_adjustment_set_value (adj, row * item_height);
        }
      else if ((row + 1) * item_height > adj->value + adj->page_size)
        {
          gtk_adjustment_set_value (adj,
                                    (row + 1) * item_height - adj->page_size);
        }

      gimp_view_renderer_set_border_type (preview->renderer,
                                          GIMP_VIEW_BORDER_BLACK);
      gimp_view_renderer_update (preview->renderer);

      name = gimp_viewable_get_description (preview->renderer->viewable, NULL);
      gtk_label_set_text (GTK_LABEL (grid_view->name_label), name);
      g_free (name);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (grid_view->name_label), _("(None)"));
    }

  grid_view->selected_item = preview;
}

static void
gimp_container_grid_view_viewport_resized (GtkWidget             *widget,
                                           GtkAllocation         *allocation,
                                           GimpContainerGridView *grid_view)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (grid_view);

  if (gimp_container_view_get_container (view))
    {
      GList *children;
      gint   n_children;

      children = gtk_container_get_children (GTK_CONTAINER (grid_view->wrap_box));
      n_children = g_list_length (children);

      if (children)
        {
          GtkRequisition preview_requisition;
          gint           columns;
          gint           rows;

          gtk_widget_size_request (GTK_WIDGET (children->data),
                                   &preview_requisition);

          g_list_free (children);

          columns = MAX (1, allocation->width / preview_requisition.width);

          rows = n_children / columns;

          if (n_children % columns)
            rows++;

          if ((rows != grid_view->rows) || (columns != grid_view->columns))
            {
              grid_view->rows    = rows;
              grid_view->columns = columns;

              gtk_widget_set_size_request (grid_view->wrap_box,
                                           columns * preview_requisition.width,
                                           rows    * preview_requisition.height);

            }

          grid_view->visible_rows = (allocation->height /
                                     preview_requisition.height);
        }

      if (grid_view->selected_item)
        {
          GimpView *preview = grid_view->selected_item;

          gimp_container_grid_view_highlight_item (view,
                                                   preview->viewable,
                                                   preview);
        }
    }
}
