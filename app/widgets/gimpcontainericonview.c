/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainericonview.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpviewable.h"

#include "gimpcellrendererviewable.h"
#include "gimpcontainertreestore.h"
#include "gimpcontainericonview.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimpviewrenderer.h"
#include "gimpwidgets-utils.h"


struct _GimpContainerIconViewPriv
{
  GimpViewRenderer *dnd_renderer;
};


static void          gimp_container_icon_view_view_iface_init   (GimpContainerViewInterface  *iface);

static void          gimp_container_icon_view_constructed       (GObject                     *object);
static void          gimp_container_icon_view_finalize          (GObject                     *object);

static void          gimp_container_icon_view_unmap             (GtkWidget                   *widget);
static gboolean      gimp_container_icon_view_popup_menu        (GtkWidget                   *widget);

static void          gimp_container_icon_view_set_container     (GimpContainerView           *view,
                                                                 GimpContainer               *container);
static void          gimp_container_icon_view_set_context       (GimpContainerView           *view,
                                                                 GimpContext                 *context);
static void          gimp_container_icon_view_set_selection_mode(GimpContainerView           *view,
                                                                 GtkSelectionMode             mode);

static gpointer      gimp_container_icon_view_insert_item       (GimpContainerView           *view,
                                                                 GimpViewable                *viewable,
                                                                 gpointer                     parent_insert_data,
                                                                 gint                         index);
static void          gimp_container_icon_view_remove_item       (GimpContainerView           *view,
                                                                 GimpViewable                *viewable,
                                                                 gpointer                     insert_data);
static void          gimp_container_icon_view_reorder_item      (GimpContainerView           *view,
                                                                 GimpViewable                *viewable,
                                                                 gint                         new_index,
                                                                 gpointer                     insert_data);
static void          gimp_container_icon_view_rename_item       (GimpContainerView           *view,
                                                                 GimpViewable                *viewable,
                                                                 gpointer                     insert_data);
static gboolean      gimp_container_icon_view_select_item       (GimpContainerView           *view,
                                                                 GimpViewable                *viewable,
                                                                 gpointer                     insert_data);
static void          gimp_container_icon_view_clear_items       (GimpContainerView           *view);
static void          gimp_container_icon_view_set_view_size     (GimpContainerView           *view);

static void          gimp_container_icon_view_selection_changed (GtkIconView                 *view,
                                                                 GimpContainerIconView       *icon_view);
static void          gimp_container_icon_view_item_activated    (GtkIconView                 *view,
                                                                 GtkTreePath                 *path,
                                                                 GimpContainerIconView       *icon_view);
static gboolean      gimp_container_icon_view_button_press      (GtkWidget                   *widget,
                                                                 GdkEventButton              *bevent,
                                                                 GimpContainerIconView       *icon_view);
static gboolean      gimp_container_icon_view_tooltip           (GtkWidget                   *widget,
                                                                 gint                         x,
                                                                 gint                         y,
                                                                 gboolean                     keyboard_tip,
                                                                 GtkTooltip                  *tooltip,
                                                                 GimpContainerIconView       *icon_view);

static GimpViewable * gimp_container_icon_view_drag_viewable    (GtkWidget    *widget,
                                                                 GimpContext **context,
                                                                 gpointer      data);
static GdkPixbuf    * gimp_container_icon_view_drag_pixbuf        (GtkWidget *widget,
                                                                   gpointer   data);
static gboolean      gimp_container_icon_view_get_selected_single (GimpContainerIconView  *icon_view,
                                                                   GtkTreeIter            *iter);
static gint          gimp_container_icon_view_get_selected        (GimpContainerView    *view,
                                                                   GList               **items);


G_DEFINE_TYPE_WITH_CODE (GimpContainerIconView, gimp_container_icon_view,
                         GIMP_TYPE_CONTAINER_BOX,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_container_icon_view_view_iface_init))

#define parent_class gimp_container_icon_view_parent_class

static GimpContainerViewInterface *parent_view_iface = NULL;


static void
gimp_container_icon_view_class_init (GimpContainerIconViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = gimp_container_icon_view_constructed;
  object_class->finalize    = gimp_container_icon_view_finalize;

  widget_class->unmap       = gimp_container_icon_view_unmap;
  widget_class->popup_menu  = gimp_container_icon_view_popup_menu;

  g_type_class_add_private (klass, sizeof (GimpContainerIconViewPriv));
}

static void
gimp_container_icon_view_view_iface_init (GimpContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  if (! parent_view_iface)
    parent_view_iface = g_type_default_interface_peek (GIMP_TYPE_CONTAINER_VIEW);

  iface->set_container      = gimp_container_icon_view_set_container;
  iface->set_context        = gimp_container_icon_view_set_context;
  iface->set_selection_mode = gimp_container_icon_view_set_selection_mode;
  iface->insert_item        = gimp_container_icon_view_insert_item;
  iface->remove_item        = gimp_container_icon_view_remove_item;
  iface->reorder_item       = gimp_container_icon_view_reorder_item;
  iface->rename_item        = gimp_container_icon_view_rename_item;
  iface->select_item        = gimp_container_icon_view_select_item;
  iface->clear_items        = gimp_container_icon_view_clear_items;
  iface->set_view_size      = gimp_container_icon_view_set_view_size;
  iface->get_selected       = gimp_container_icon_view_get_selected;

  iface->insert_data_free = (GDestroyNotify) gtk_tree_iter_free;
}

static void
gimp_container_icon_view_init (GimpContainerIconView *icon_view)
{
  GimpContainerBox *box = GIMP_CONTAINER_BOX (icon_view);

  icon_view->priv = G_TYPE_INSTANCE_GET_PRIVATE (icon_view,
                                                 GIMP_TYPE_CONTAINER_ICON_VIEW,
                                                 GimpContainerIconViewPriv);

  gimp_container_tree_store_columns_init (icon_view->model_columns,
                                          &icon_view->n_model_columns);

  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (box->scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (box->scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
}

static void
gimp_container_icon_view_constructed (GObject *object)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (object);
  GimpContainerView     *view      = GIMP_CONTAINER_VIEW (object);
  GimpContainerBox      *box       = GIMP_CONTAINER_BOX (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  icon_view->model = gimp_container_tree_store_new (view,
                                                    icon_view->n_model_columns,
                                                    icon_view->model_columns);

  icon_view->view = g_object_new (GTK_TYPE_ICON_VIEW,
                                  "model",          icon_view->model,
                                  "row-spacing",    0,
                                  "column-spacing", 0,
                                  "margin",         0,
                                  "item-padding",   1,
                                  "has-tooltip",    TRUE,
                                  NULL);
  g_object_unref (icon_view->model);

  gtk_container_add (GTK_CONTAINER (box->scrolled_win),
                     GTK_WIDGET (icon_view->view));
  gtk_widget_show (GTK_WIDGET (icon_view->view));

  gimp_container_view_set_dnd_widget (view, GTK_WIDGET (icon_view->view));

  icon_view->renderer_cell = gimp_cell_renderer_viewable_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon_view->view),
                              icon_view->renderer_cell,
                              FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view->view),
                                  icon_view->renderer_cell,
                                  "renderer", GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER,
                                  NULL);

  gimp_container_tree_store_add_renderer_cell (GIMP_CONTAINER_TREE_STORE (icon_view->model),
                                               icon_view->renderer_cell);

  g_signal_connect (icon_view->view, "selection-changed",
                    G_CALLBACK (gimp_container_icon_view_selection_changed),
                    icon_view);
  g_signal_connect (icon_view->view, "item-activated",
                    G_CALLBACK (gimp_container_icon_view_item_activated),
                    icon_view);
  g_signal_connect (icon_view->view, "query-tooltip",
                    G_CALLBACK (gimp_container_icon_view_tooltip),
                    icon_view);
}

static void
gimp_container_icon_view_finalize (GObject *object)
{
  //GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_container_icon_view_unmap (GtkWidget *widget)
{
  //GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (widget);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gimp_container_icon_view_menu_position (GtkMenu  *menu,
                                        gint     *x,
                                        gint     *y,
                                        gpointer  data)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (data);
  GtkWidget             *widget    = GTK_WIDGET (icon_view->view);
  GtkAllocation          allocation;
#if 0
  GtkTreeIter            selected_iter;
#endif

  gtk_widget_get_allocation (widget, &allocation);

  gdk_window_get_origin (gtk_widget_get_window (widget), x, y);

  if (! gtk_widget_get_has_window (widget))
    {
      *x += allocation.x;
      *y += allocation.y;
    }

#if 0
  if (gimp_container_icon_view_get_selected_single (icon_view, &selected_iter))
    {
      GtkTreePath  *path;
      GdkRectangle  cell_rect;
      gint          center;

      path = gtk_tree_model_get_path (icon_view->model, &selected_iter);
      gtk_icon_view_get_cell_area (icon_view->view, path,
                                   icon_view->main_column, &cell_rect);
      gtk_tree_path_free (path);

      center = cell_rect.y + cell_rect.height / 2;
      center = CLAMP (center, 0, allocation.height);

      *x += allocation.width / 2;
      *y += center;
    }
  else
#endif
    {
      GtkStyle *style = gtk_widget_get_style (widget);

      *x += style->xthickness;
      *y += style->ythickness;
    }

  gimp_menu_position (menu, x, y);
}

static gboolean
gimp_container_icon_view_popup_menu (GtkWidget *widget)
{
  return gimp_editor_popup_menu (GIMP_EDITOR (widget),
                                 gimp_container_icon_view_menu_position,
                                 widget);
}

GtkWidget *
gimp_container_icon_view_new (GimpContainer *container,
                              GimpContext   *context,
                              gint           view_size,
                              gint           view_border_width)
{
  GimpContainerIconView *icon_view;
  GimpContainerView     *view;

  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  icon_view = g_object_new (GIMP_TYPE_CONTAINER_ICON_VIEW, NULL);

  view = GIMP_CONTAINER_VIEW (icon_view);

  gimp_container_view_set_view_size (view, view_size, 0 /* ignore border */);

  if (container)
    gimp_container_view_set_container (view, container);

  if (context)
    gimp_container_view_set_context (view, context);

  return GTK_WIDGET (icon_view);
}


/*  GimpContainerView methods  */

static void
gimp_container_icon_view_set_container (GimpContainerView *view,
                                        GimpContainer     *container)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (view);
  GimpContainer         *old_container;

  old_container = gimp_container_view_get_container (view);

  if (old_container)
    {
      if (! container)
        {
          if (gimp_dnd_viewable_source_remove (GTK_WIDGET (icon_view->view),
                                               gimp_container_get_children_type (old_container)))
            {
              if (GIMP_VIEWABLE_CLASS (g_type_class_peek (gimp_container_get_children_type (old_container)))->get_size)
                gimp_dnd_pixbuf_source_remove (GTK_WIDGET (icon_view->view));

              gtk_drag_source_unset (GTK_WIDGET (icon_view->view));
            }

          g_signal_handlers_disconnect_by_func (icon_view->view,
                                                gimp_container_icon_view_button_press,
                                                icon_view);
        }
    }
  else if (container)
    {
      if (gimp_dnd_drag_source_set_by_type (GTK_WIDGET (icon_view->view),
                                            GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                                            gimp_container_get_children_type (container),
                                            GDK_ACTION_COPY))
        {
          gimp_dnd_viewable_source_add (GTK_WIDGET (icon_view->view),
                                        gimp_container_get_children_type (container),
                                        gimp_container_icon_view_drag_viewable,
                                        icon_view);

          if (GIMP_VIEWABLE_CLASS (g_type_class_peek (gimp_container_get_children_type (container)))->get_size)
            gimp_dnd_pixbuf_source_add (GTK_WIDGET (icon_view->view),
                                        gimp_container_icon_view_drag_pixbuf,
                                        icon_view);
        }

      g_signal_connect (icon_view->view, "button-press-event",
                        G_CALLBACK (gimp_container_icon_view_button_press),
                        icon_view);
    }

  parent_view_iface->set_container (view, container);
}

static void
gimp_container_icon_view_set_context (GimpContainerView *view,
                                      GimpContext       *context)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (view);

  parent_view_iface->set_context (view, context);

  if (icon_view->model)
    gimp_container_tree_store_set_context (GIMP_CONTAINER_TREE_STORE (icon_view->model),
                                           context);
}

static void
gimp_container_icon_view_set_selection_mode (GimpContainerView *view,
                                             GtkSelectionMode   mode)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (view);

  gtk_icon_view_set_selection_mode (icon_view->view, mode);

  parent_view_iface->set_selection_mode (view, mode);
}

static gpointer
gimp_container_icon_view_insert_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gpointer           parent_insert_data,
                                      gint               index)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (view);
  GtkTreeIter           *iter;

  iter = gimp_container_tree_store_insert_item (GIMP_CONTAINER_TREE_STORE (icon_view->model),
                                                viewable,
                                                parent_insert_data,
                                                index);

  if (parent_insert_data)
    {
#if 0
      GtkTreePath *path = gtk_tree_model_get_path (icon_view->model, iter);

      gtk_icon_view_expand_to_path (icon_view->view, path);

      gtk_tree_path_free (path);
#endif
    }

  return iter;
}

static void
gimp_container_icon_view_remove_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gpointer           insert_data)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (view);

  gimp_container_tree_store_remove_item (GIMP_CONTAINER_TREE_STORE (icon_view->model),
                                         viewable,
                                         insert_data);
}

static void
gimp_container_icon_view_reorder_item (GimpContainerView *view,
                                       GimpViewable      *viewable,
                                       gint               new_index,
                                       gpointer           insert_data)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (view);
  GtkTreeIter           *iter      = (GtkTreeIter *) insert_data;
  gboolean               selected  = FALSE;

  if (iter)
    {
      GtkTreeIter selected_iter;

      selected = gimp_container_icon_view_get_selected_single (icon_view,
                                                               &selected_iter);

      if (selected)
        {
          GimpViewRenderer *renderer;

          gtk_tree_model_get (icon_view->model, &selected_iter,
                              GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                              -1);

          if (renderer->viewable != viewable)
            selected = FALSE;

          g_object_unref (renderer);
        }
    }

  gimp_container_tree_store_reorder_item (GIMP_CONTAINER_TREE_STORE (icon_view->model),
                                          viewable,
                                          new_index,
                                          iter);

  if (selected)
    gimp_container_view_select_item (view, viewable);
}

static void
gimp_container_icon_view_rename_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gpointer           insert_data)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (view);
  GtkTreeIter           *iter      = (GtkTreeIter *) insert_data;

  gimp_container_tree_store_rename_item (GIMP_CONTAINER_TREE_STORE (icon_view->model),
                                         viewable,
                                         iter);
}

static gboolean
gimp_container_icon_view_select_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gpointer           insert_data)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (view);

  if (viewable && insert_data)
    {
      GtkTreePath *path;
      GtkTreePath *parent_path;
      GtkTreeIter *iter = (GtkTreeIter *) insert_data;

      path = gtk_tree_model_get_path (icon_view->model, iter);

      parent_path = gtk_tree_path_copy (path);

      if (gtk_tree_path_up (parent_path))
        ;
#if 0
        gtk_icon_view_expand_to_path (icon_view->view, parent_path);
#endif

      gtk_tree_path_free (parent_path);

      g_signal_handlers_block_by_func (icon_view->view,
                                       gimp_container_icon_view_selection_changed,
                                       icon_view);

      gtk_icon_view_select_path (icon_view->view, path);
      gtk_icon_view_set_cursor (icon_view->view, path, NULL, FALSE);

      g_signal_handlers_unblock_by_func (icon_view->view,
                                         gimp_container_icon_view_selection_changed,
                                         icon_view);

      gtk_icon_view_scroll_to_path (icon_view->view, path, FALSE, 0.0, 0.0);

      gtk_tree_path_free (path);
    }
  else if (insert_data == NULL)
    {
      /* viewable == NULL && insert_data != NULL means multiple selection.
       * viewable == NULL && insert_data == NULL means no selection. */
      gtk_icon_view_unselect_all (icon_view->view);
    }

  return TRUE;
}

static void
gimp_container_icon_view_clear_items (GimpContainerView *view)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (view);

  gimp_container_tree_store_clear_items (GIMP_CONTAINER_TREE_STORE (icon_view->model));

  parent_view_iface->clear_items (view);
}

static void
gimp_container_icon_view_set_view_size (GimpContainerView *view)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (view);

  if (icon_view->model)
    gimp_container_tree_store_set_view_size (GIMP_CONTAINER_TREE_STORE (icon_view->model));

  if (icon_view->view)
    {
      gtk_icon_view_set_columns (icon_view->view, -1);
      gtk_icon_view_set_item_width (icon_view->view, -1);

      /* ugly workaround to force the icon view to invalidate all its
       * cached icon sizes
       */
      gtk_icon_view_set_item_orientation (icon_view->view,
                                          GTK_ORIENTATION_VERTICAL);
      gtk_icon_view_set_item_orientation (icon_view->view,
                                          GTK_ORIENTATION_HORIZONTAL);
    }
}


/*  callbacks  */

static void
gimp_container_icon_view_selection_changed (GtkIconView           *gtk_icon_view,
                                            GimpContainerIconView *icon_view)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (icon_view);
  GList             *items;

  gimp_container_icon_view_get_selected (view, &items);
  gimp_container_view_multi_selected (view, items);
  g_list_free (items);
}

static void
gimp_container_icon_view_item_activated (GtkIconView           *view,
                                         GtkTreePath           *path,
                                         GimpContainerIconView *icon_view)
{
  GtkTreeIter       iter;
  GimpViewRenderer *renderer;

  gtk_tree_model_get_iter (icon_view->model, &iter, path);

  gtk_tree_model_get (icon_view->model, &iter,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  gimp_container_view_item_activated (GIMP_CONTAINER_VIEW (icon_view),
                                      renderer->viewable);

  g_object_unref (renderer);
}

static gboolean
gimp_container_icon_view_button_press (GtkWidget             *widget,
                                       GdkEventButton        *bevent,
                                       GimpContainerIconView *icon_view)
{
  GtkTreePath *path;

  icon_view->priv->dnd_renderer = NULL;

  path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (widget),
                                        bevent->x, bevent->y);

  if (path)
    {
      GimpViewRenderer *renderer;
      GtkTreeIter       iter;

      gtk_tree_model_get_iter (icon_view->model, &iter, path);

      gtk_tree_model_get (icon_view->model, &iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      icon_view->priv->dnd_renderer = renderer;

      g_object_unref (renderer);

      gtk_tree_path_free (path);
    }

  return FALSE;
}

static gboolean
gimp_container_icon_view_tooltip (GtkWidget             *widget,
                                  gint                   x,
                                  gint                   y,
                                  gboolean               keyboard_tip,
                                  GtkTooltip            *tooltip,
                                  GimpContainerIconView *icon_view)
{
  GimpViewRenderer *renderer;
  GtkTreeIter       iter;
  GtkTreePath      *path;
  gboolean          show_tip = FALSE;

  if (! gtk_icon_view_get_tooltip_context (GTK_ICON_VIEW (widget), &x, &y,
                                           keyboard_tip,
                                           NULL, &path, &iter))
    return FALSE;

  gtk_tree_model_get (icon_view->model, &iter,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  if (renderer)
    {
      gchar *desc;
      gchar *tip;

      desc = gimp_viewable_get_description (renderer->viewable, &tip);

      if (tip)
        {
          gtk_tooltip_set_text (tooltip, tip);
          gtk_icon_view_set_tooltip_cell (GTK_ICON_VIEW (widget), tooltip, path,
                                          icon_view->renderer_cell);

          show_tip = TRUE;

          g_free (tip);
        }

      g_free (desc);
      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);

  return show_tip;
}

static GimpViewable *
gimp_container_icon_view_drag_viewable (GtkWidget    *widget,
                                        GimpContext **context,
                                        gpointer      data)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (data);

  if (context)
    *context = gimp_container_view_get_context (GIMP_CONTAINER_VIEW (data));

  if (icon_view->priv->dnd_renderer)
    return icon_view->priv->dnd_renderer->viewable;

  return NULL;
}

static GdkPixbuf *
gimp_container_icon_view_drag_pixbuf (GtkWidget *widget,
                                      gpointer   data)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (data);
  GimpViewRenderer      *renderer  = icon_view->priv->dnd_renderer;
  gint                   width;
  gint                   height;

  if (renderer && gimp_viewable_get_size (renderer->viewable, &width, &height))
    return gimp_viewable_get_new_pixbuf (renderer->viewable,
                                         renderer->context,
                                         width, height);

  return NULL;
}

static gboolean
gimp_container_icon_view_get_selected_single (GimpContainerIconView  *icon_view,
                                              GtkTreeIter            *iter)
{
  GList    *selected_items;
  gboolean  retval;

  selected_items = gtk_icon_view_get_selected_items (icon_view->view);

  if (g_list_length (selected_items) == 1)
    {
      gtk_tree_model_get_iter (GTK_TREE_MODEL (icon_view->model), iter,
                               (GtkTreePath *) selected_items->data);

      retval = TRUE;
    }
  else
    {
      retval = FALSE;
    }

  g_list_free_full (selected_items, (GDestroyNotify) gtk_tree_path_free);

  return retval;
}

static gint
gimp_container_icon_view_get_selected (GimpContainerView    *view,
                                       GList               **items)
{
  GimpContainerIconView *icon_view = GIMP_CONTAINER_ICON_VIEW (view);
  GList                 *selected_items;
  gint                   selected_count;

  selected_items = gtk_icon_view_get_selected_items (icon_view->view);
  selected_count = g_list_length (selected_items);

  if (items)
    {
      GList *list;

      *items = NULL;

      for (list = selected_items;
           list;
           list = g_list_next (list))
        {
          GtkTreeIter       iter;
          GimpViewRenderer *renderer;

          gtk_tree_model_get_iter (GTK_TREE_MODEL (icon_view->model), &iter,
                                   (GtkTreePath *) list->data);

          gtk_tree_model_get (icon_view->model, &iter,
                              GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                              -1);

          if (renderer->viewable)
            *items = g_list_prepend (*items, renderer->viewable);

          g_object_unref (renderer);
        }

      *items = g_list_reverse (*items);
    }

  g_list_free_full (selected_items, (GDestroyNotify) gtk_tree_path_free);

  return selected_count;
}
