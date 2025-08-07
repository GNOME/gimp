/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawabletreeview.c
 * Copyright (C) 2001-2009 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"

#include "gimphelp-ids.h"
#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-edit.h"
#include "core/gimpdrawable-filters.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimpfilloptions.h"
#include "core/gimplist.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimppattern.h"
#include "core/gimptreehandler.h"

#include "gimpcontainertreestore.h"
#include "gimpcontainerview.h"
#include "gimpcontainerview-cruft.h"
#include "gimpdnd.h"
#include "gimpdrawabletreeview.h"
#include "gimpdrawabletreeview-filters.h"
#include "gimpviewrenderer.h"

#include "gimp-intl.h"


struct _GimpDrawableTreeViewPrivate
{
  GtkWidget       *filters_header_image;

  gint             model_column_filters;
  GtkCellRenderer *filters_cell;

  GimpTreeHandler *filters_changed_handler;
};


static void   gimp_drawable_tree_view_view_iface_init (GimpContainerViewInterface *iface);

static void     gimp_drawable_tree_view_constructed   (GObject                    *object);
static void     gimp_drawable_tree_view_dispose       (GObject                    *object);

static void     gimp_drawable_tree_view_unmap         (GtkWidget                  *widget);
static void     gimp_drawable_tree_view_style_updated (GtkWidget                  *widget);

static void     gimp_drawable_tree_view_set_container (GimpContainerView          *view,
                                                       GimpContainer              *container);
static gboolean gimp_drawable_tree_view_set_selected  (GimpContainerView          *view,
                                                       GList                      *items);

static gpointer gimp_drawable_tree_view_insert_item   (GimpContainerView          *view,
                                                       GimpViewable               *viewable,
                                                       gpointer                    parent_insert_data,
                                                       gint                        index);

static gboolean gimp_drawable_tree_view_drop_possible (GimpContainerTreeView      *view,
                                                       GimpDndType                 src_type,
                                                       GList                      *src_viewables,
                                                       GimpViewable               *dest_viewable,
                                                       GtkTreePath                *drop_path,
                                                       GtkTreeViewDropPosition     drop_pos,
                                                       GtkTreeViewDropPosition    *return_drop_pos,
                                                       GdkDragAction              *return_drag_action);
static void   gimp_drawable_tree_view_drop_viewables  (GimpContainerTreeView      *view,
                                                       GList                      *src_viewables,
                                                       GimpViewable               *dest_viewable,
                                                       GtkTreeViewDropPosition     drop_pos);
static void   gimp_drawable_tree_view_drop_color      (GimpContainerTreeView      *view,
                                                       GeglColor                  *color,
                                                       GimpViewable               *dest_viewable,
                                                       GtkTreeViewDropPosition     drop_pos);

static void   gimp_drawable_tree_view_set_image       (GimpItemTreeView            *view,
                                                       GimpImage                   *image);

static void   gimp_drawable_tree_view_floating_selection_changed
                                                      (GimpImage                   *image,
                                                       GimpDrawableTreeView        *view);

static void   gimp_drawable_tree_view_filters_changed (GimpDrawable                *drawable,
                                                       GimpDrawableTreeView        *view);

static void   gimp_drawable_tree_view_filters_cell_clicked
                                                      (GtkCellRendererToggle       *toggle,
                                                       gchar                       *path,
                                                       GdkModifierType              state,
                                                       GimpDrawableTreeView        *view);

static void   gimp_drawable_tree_view_new_pattern_dropped
                                                      (GtkWidget                   *widget,
                                                       gint                         x,
                                                       gint                         y,
                                                       GimpViewable                *viewable,
                                                       gpointer                     data);
static void   gimp_drawable_tree_view_new_color_dropped
                                                      (GtkWidget                   *widget,
                                                       gint                         x,
                                                       gint                         y,
                                                       GeglColor                   *color,
                                                       gpointer                     data);


G_DEFINE_TYPE_WITH_CODE (GimpDrawableTreeView, gimp_drawable_tree_view,
                         GIMP_TYPE_ITEM_TREE_VIEW,
                         G_ADD_PRIVATE (GimpDrawableTreeView)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_drawable_tree_view_view_iface_init))

#define parent_class gimp_drawable_tree_view_parent_class

static GimpContainerViewInterface *parent_view_iface = NULL;


static void
gimp_drawable_tree_view_class_init (GimpDrawableTreeViewClass *klass)
{
  GObjectClass               *object_class;
  GtkWidgetClass             *widget_class;
  GimpContainerTreeViewClass *tree_view_class;
  GimpItemTreeViewClass      *item_view_class;

  object_class    = G_OBJECT_CLASS (klass);
  widget_class    = GTK_WIDGET_CLASS (klass);
  tree_view_class = GIMP_CONTAINER_TREE_VIEW_CLASS (klass);
  item_view_class = GIMP_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructed       = gimp_drawable_tree_view_constructed;
  object_class->dispose           = gimp_drawable_tree_view_dispose;

  widget_class->unmap             = gimp_drawable_tree_view_unmap;
  widget_class->style_updated     = gimp_drawable_tree_view_style_updated;

  tree_view_class->drop_possible  = gimp_drawable_tree_view_drop_possible;
  tree_view_class->drop_viewables = gimp_drawable_tree_view_drop_viewables;
  tree_view_class->drop_color     = gimp_drawable_tree_view_drop_color;

  item_view_class->set_image      = gimp_drawable_tree_view_set_image;

  item_view_class->lock_content_icon_name    = GIMP_ICON_LOCK_CONTENT;
  item_view_class->lock_content_tooltip      = _("Lock pixels");
  item_view_class->lock_position_icon_name   = GIMP_ICON_LOCK_POSITION;
  item_view_class->lock_position_tooltip     = _("Lock position and size");
  item_view_class->lock_visibility_icon_name = GIMP_ICON_LOCK_VISIBILITY;
  item_view_class->lock_visibility_tooltip   = _("Lock visibility");
}

static void
gimp_drawable_tree_view_view_iface_init (GimpContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->set_container = gimp_drawable_tree_view_set_container;
  iface->set_selected  = gimp_drawable_tree_view_set_selected;

  iface->insert_item   = gimp_drawable_tree_view_insert_item;
}

static void
gimp_drawable_tree_view_init (GimpDrawableTreeView *view)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);

  view->priv = gimp_drawable_tree_view_get_instance_private (view);

  view->priv->model_column_filters =
    gimp_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);
}

static void
gimp_drawable_tree_view_constructed (GObject *object)
{
  GimpContainerTreeView *tree_view        = GIMP_CONTAINER_TREE_VIEW (object);
  GimpItemTreeView      *item_view        = GIMP_ITEM_TREE_VIEW (object);
  GimpDrawableTreeView  *view             = GIMP_DRAWABLE_TREE_VIEW (object);
  GtkIconSize            button_icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
  GtkTreeViewColumn     *column;
  gint                   pixel_icon_size  = 16;
  gint                   button_spacing;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_widget_style_get (GTK_WIDGET (view),
                        "button-icon-size", &button_icon_size,
                        "button-spacing",   &button_spacing,
                        NULL);
  gtk_icon_size_lookup (button_icon_size, &pixel_icon_size, NULL);

  column = gtk_tree_view_column_new ();
  view->priv->filters_header_image =
    gtk_image_new_from_icon_name (GIMP_ICON_EFFECT, button_icon_size);
  gimp_widget_set_identifier (view->priv->filters_header_image,
                              "item-effect-header-icon");
  gtk_tree_view_column_set_widget (column, view->priv->filters_header_image);
  gtk_widget_show (view->priv->filters_header_image);
  gtk_tree_view_insert_column (tree_view->view, column, 2);

  view->priv->filters_cell = gimp_cell_renderer_toggle_new (GIMP_ICON_EFFECT);
  g_object_set (view->priv->filters_cell,
                "xpad",      0,
                "ypad",      0,
                "icon-size", pixel_icon_size,
                NULL);
  gtk_tree_view_column_pack_start (column, view->priv->filters_cell, FALSE);
  gtk_tree_view_column_set_attributes (column, view->priv->filters_cell,
                                       "active",
                                       view->priv->model_column_filters,
                                       NULL);

  gimp_container_tree_view_add_toggle_cell (tree_view,
                                            view->priv->filters_cell);

  g_signal_connect (view->priv->filters_cell, "clicked",
                    G_CALLBACK (gimp_drawable_tree_view_filters_cell_clicked),
                    item_view);

  gimp_dnd_viewable_dest_add (gimp_item_tree_view_get_new_button (item_view),
                              GIMP_TYPE_PATTERN,
                              gimp_drawable_tree_view_new_pattern_dropped,
                              item_view);
  gimp_dnd_color_dest_add (gimp_item_tree_view_get_new_button (item_view),
                           gimp_drawable_tree_view_new_color_dropped,
                           item_view);

  gimp_dnd_color_dest_add    (GTK_WIDGET (tree_view->view),
                              NULL, tree_view);
  gimp_dnd_viewable_dest_add (GTK_WIDGET (tree_view->view), GIMP_TYPE_PATTERN,
                              NULL, tree_view);
}

static void
gimp_drawable_tree_view_dispose (GObject *object)
{
  GimpDrawableTreeView *view = GIMP_DRAWABLE_TREE_VIEW (object);

  _gimp_drawable_tree_view_filter_editor_destroy (view);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_drawable_tree_view_unmap (GtkWidget *widget)
{
  GimpDrawableTreeView *view = GIMP_DRAWABLE_TREE_VIEW (widget);

  _gimp_drawable_tree_view_filter_editor_hide (view);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gimp_drawable_tree_view_style_updated (GtkWidget *widget)
{
  GimpDrawableTreeView *view = GIMP_DRAWABLE_TREE_VIEW (widget);
  const gchar          *old_icon_name;
  gint                  pixel_icon_size;
  GtkIconSize           old_size;
  GtkIconSize           button_icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;

  gtk_widget_style_get (widget,
                        "button-icon-size", &button_icon_size,
                        NULL);

  gtk_icon_size_lookup (button_icon_size, &pixel_icon_size, NULL);

  gtk_image_get_icon_name (GTK_IMAGE (view->priv->filters_header_image),
                           &old_icon_name, &old_size);

  if (button_icon_size != old_size)
    {
      gchar *icon_name = g_strdup (old_icon_name);

      gtk_image_set_from_icon_name (GTK_IMAGE (view->priv->filters_header_image),
                                    icon_name, button_icon_size);
      g_free (icon_name);
    }

  g_object_set (view->priv->filters_cell,
                "icon-name", GIMP_ICON_EFFECT,
                "icon-size", pixel_icon_size,
                NULL);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);
}


/*  GimpContainerView methods  */

static void
gimp_drawable_tree_view_set_container (GimpContainerView *view,
                                       GimpContainer     *container)
{
  GimpDrawableTreeView *drawable_view = GIMP_DRAWABLE_TREE_VIEW (view);
  GimpContainer        *old_container;

  old_container = gimp_container_view_get_container (view);

  if (old_container)
    g_clear_pointer (&drawable_view->priv->filters_changed_handler,
                     gimp_tree_handler_disconnect);

  parent_view_iface->set_container (view, container);

  if (container)
    drawable_view->priv->filters_changed_handler =
      gimp_tree_handler_connect (container, "filters-changed",
                                 G_CALLBACK (gimp_drawable_tree_view_filters_changed),
                                 view);
}

static gboolean
gimp_drawable_tree_view_set_selected (GimpContainerView *view,
                                      GList             *items)
{
  GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (view);
  GimpImage        *image     = gimp_item_tree_view_get_image (item_view);
  gboolean          success   = TRUE;

  if (image)
    {
      GimpLayer *floating_sel = gimp_image_get_floating_selection (image);

      success = (items        == NULL ||
                 floating_sel == NULL ||
                 (g_list_length (items) == 1 &&
                  items->data           == GIMP_VIEWABLE (floating_sel)));

      if (! success)
        {
          Gimp        *gimp    = image->gimp;
          GimpContext *context = gimp_get_user_context (gimp);
          GimpDisplay *display = gimp_context_get_display (context);

          gimp_message_literal (gimp, G_OBJECT (display), GIMP_MESSAGE_WARNING,
                                _("Cannot select items while a floating "
                                  "selection is active."));
        }
    }

  if (success)
    success = parent_view_iface->set_selected (view, items);

  return success;
}

static gpointer
gimp_drawable_tree_view_insert_item (GimpContainerView *view,
                                     GimpViewable      *viewable,
                                     gpointer           parent_insert_data,
                                     gint               index)
{
  GimpContainerTreeView *tree_view     = GIMP_CONTAINER_TREE_VIEW (view);
  GimpDrawableTreeView  *drawable_view = GIMP_DRAWABLE_TREE_VIEW (view);
  GimpContainer         *filters;
  GtkTreeIter           *iter;
  gint                   n_filters;

  iter = parent_view_iface->insert_item (view, viewable,
                                         parent_insert_data, index);

  filters   = gimp_drawable_get_filters (GIMP_DRAWABLE (viewable));
  n_filters = gimp_container_get_n_children (filters);

  gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                      drawable_view->priv->model_column_filters,
                      n_filters > 0,
                      -1);

  return iter;
}


/*  GimpContainerTreeView methods  */

static gboolean
gimp_drawable_tree_view_drop_possible (GimpContainerTreeView   *tree_view,
                                       GimpDndType              src_type,
                                       GList                   *src_viewables,
                                       GimpViewable            *dest_viewable,
                                       GtkTreePath             *drop_path,
                                       GtkTreeViewDropPosition  drop_pos,
                                       GtkTreeViewDropPosition *return_drop_pos,
                                       GdkDragAction           *return_drag_action)
{
  if (GIMP_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_possible (tree_view,
                                                                    src_type,
                                                                    src_viewables,
                                                                    dest_viewable,
                                                                    drop_path,
                                                                    drop_pos,
                                                                    return_drop_pos,
                                                                    return_drag_action))
    {
      if (src_type == GIMP_DND_TYPE_COLOR ||
          src_type == GIMP_DND_TYPE_PATTERN)
        {
          if (! dest_viewable ||
              gimp_item_is_content_locked (GIMP_ITEM (dest_viewable), NULL) ||
              gimp_viewable_get_children (GIMP_VIEWABLE (dest_viewable)))
            return FALSE;

          if (return_drop_pos)
            {
              *return_drop_pos = GTK_TREE_VIEW_DROP_INTO_OR_AFTER;
            }
        }

      return TRUE;
    }

  return FALSE;
}

static void
gimp_drawable_tree_view_drop_viewables (GimpContainerTreeView   *view,
                                        GList                   *src_viewables,
                                        GimpViewable            *dest_viewable,
                                        GtkTreeViewDropPosition  drop_pos)
{
  GList *iter;

  for (iter = src_viewables; iter; iter = iter->next)
    {
      GimpViewable *src_viewable = iter->data;

      if (dest_viewable && GIMP_IS_PATTERN (src_viewable))
        {
          GimpImage       *image   = gimp_item_get_image (GIMP_ITEM (dest_viewable));
          GimpFillOptions *options = gimp_fill_options_new (image->gimp, NULL, FALSE);

          gimp_fill_options_set_style (options, GIMP_FILL_STYLE_PATTERN);
          gimp_context_set_pattern (GIMP_CONTEXT (options),
                                    GIMP_PATTERN (src_viewable));

          gimp_drawable_edit_fill (GIMP_DRAWABLE (dest_viewable),
                                   options,
                                   C_("undo-type", "Drop pattern to layer"));

          g_object_unref (options);

          gimp_image_flush (image);
          return;
        }
    }

  GIMP_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_viewables (view,
                                                                 src_viewables,
                                                                 dest_viewable,
                                                                 drop_pos);
}

static void
gimp_drawable_tree_view_drop_color (GimpContainerTreeView   *view,
                                    GeglColor               *color,
                                    GimpViewable            *dest_viewable,
                                    GtkTreeViewDropPosition  drop_pos)
{
  if (dest_viewable)
    {
      GimpImage       *image   = gimp_item_get_image (GIMP_ITEM (dest_viewable));
      GimpFillOptions *options = gimp_fill_options_new (image->gimp, NULL, FALSE);

      gimp_fill_options_set_style (options, GIMP_FILL_STYLE_FG_COLOR);
      gimp_context_set_foreground (GIMP_CONTEXT (options), color);

      gimp_drawable_edit_fill (GIMP_DRAWABLE (dest_viewable),
                               options,
                               C_("undo-type", "Drop color to layer"));

      g_object_unref (options);

      gimp_image_flush (image);
    }
}


/*  GimpItemTreeView methods  */

static void
gimp_drawable_tree_view_set_image (GimpItemTreeView *view,
                                   GimpImage        *image)
{
  GimpDrawableTreeView *drawable_view = GIMP_DRAWABLE_TREE_VIEW (view);

  if (gimp_item_tree_view_get_image (view))
    {
      _gimp_drawable_tree_view_filter_editor_hide (drawable_view);

      g_signal_handlers_disconnect_by_func (gimp_item_tree_view_get_image (view),
                                            gimp_drawable_tree_view_floating_selection_changed,
                                            view);
      g_signal_handlers_disconnect_by_func (gimp_item_tree_view_get_image (view)->gimp->config,
                                            G_CALLBACK (gimp_drawable_tree_view_style_updated),
                                            view);

    }

  GIMP_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (view, image);

  if (gimp_item_tree_view_get_image (view))
    {
      g_signal_connect (gimp_item_tree_view_get_image (view),
                        "floating-selection-changed",
                        G_CALLBACK (gimp_drawable_tree_view_floating_selection_changed),
                        view);

      g_signal_connect_object (image->gimp->config,
                               "notify::theme",
                               G_CALLBACK (gimp_drawable_tree_view_style_updated),
                               view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (image->gimp->config,
                               "notify::override-theme-icon-size",
                               G_CALLBACK (gimp_drawable_tree_view_style_updated),
                               view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (image->gimp->config,
                               "notify::custom-icon-size",
                               G_CALLBACK (gimp_drawable_tree_view_style_updated),
                               view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
    }
}


/*  callbacks  */

static void
gimp_drawable_tree_view_floating_selection_changed (GimpImage            *image,
                                                    GimpDrawableTreeView *view)
{
  GType  item_type;
  GList *items;

  item_type = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->item_type;

  items = gimp_image_get_selected_items (image, item_type);
  items = g_list_copy (items);

  /*  update button states  */
  gimp_container_view_set_selected (GIMP_CONTAINER_VIEW (view), items);

  g_list_free (items);
}


/*  filters callbacks  */

static void
gimp_drawable_tree_view_filters_changed (GimpDrawable         *drawable,
                                         GimpDrawableTreeView *view)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter;

  iter = _gimp_container_view_lookup (GIMP_CONTAINER_VIEW (view),
                                      (GimpViewable *) drawable);

  if (iter)
    {
      gint n_editable;

      gimp_drawable_n_editable_filters (drawable, &n_editable, NULL, NULL);

      gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                          view->priv->model_column_filters,
                          n_editable > 0,
                          -1);
    }
}

static void
gimp_drawable_tree_view_filters_cell_clicked (GtkCellRendererToggle *toggle,
                                              gchar                 *path_str,
                                              GdkModifierType        state,
                                              GimpDrawableTreeView  *view)
{
  GtkTreePath *path;
  GtkTreeIter  iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (GIMP_CONTAINER_TREE_VIEW (view)->model,
                               &iter, path))
    {
      GimpViewRenderer       *renderer;
      GimpContainerTreeStore *store;
      GimpDrawable           *drawable;
      GdkRectangle            rect;

      /* Update the filter state. */
      store = GIMP_CONTAINER_TREE_STORE (GIMP_CONTAINER_TREE_VIEW (view)->model);
      renderer = gimp_container_tree_store_get_renderer (store, &iter);
      drawable = GIMP_DRAWABLE (renderer->viewable);
      g_object_unref (renderer);

      gtk_tree_view_get_cell_area (GIMP_CONTAINER_TREE_VIEW (view)->view, path,
                                   gtk_tree_view_get_column (GIMP_CONTAINER_TREE_VIEW (view)->view, 2),
                                   &rect);
      gtk_tree_view_convert_bin_window_to_widget_coords (GIMP_CONTAINER_TREE_VIEW (view)->view,
                                                         rect.x, rect.y, &rect.x, &rect.y);

      _gimp_drawable_tree_view_filter_editor_show (view, drawable, &rect);
    }
}


/*  dnd callbacks  */

static void
gimp_drawable_tree_view_new_dropped (GimpItemTreeView *view,
                                     GimpFillOptions  *options,
                                     const gchar      *undo_desc)
{
  GimpImage *image = gimp_item_tree_view_get_image (view);
  GimpItem  *item;

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE,
                               _("New Layer"));

  item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->new_item (image);

  if (item)
    gimp_drawable_edit_fill (GIMP_DRAWABLE (item), options, undo_desc);

  gimp_image_undo_group_end (image);

  gimp_image_flush (image);
}

static void
gimp_drawable_tree_view_new_pattern_dropped (GtkWidget    *widget,
                                             gint          x,
                                             gint          y,
                                             GimpViewable *viewable,
                                             gpointer      data)
{
  GimpItemTreeView *view    = GIMP_ITEM_TREE_VIEW (data);
  GimpImage        *image   = gimp_item_tree_view_get_image (view);
  GimpFillOptions  *options = gimp_fill_options_new (image->gimp, NULL, FALSE);

  gimp_fill_options_set_style (options, GIMP_FILL_STYLE_PATTERN);
  gimp_context_set_pattern (GIMP_CONTEXT (options), GIMP_PATTERN (viewable));

  gimp_drawable_tree_view_new_dropped (view, options,
                                       C_("undo-type", "Drop pattern to layer"));

  g_object_unref (options);
}

static void
gimp_drawable_tree_view_new_color_dropped (GtkWidget *widget,
                                           gint       x,
                                           gint       y,
                                           GeglColor *color,
                                           gpointer   data)
{
  GimpItemTreeView *view    = GIMP_ITEM_TREE_VIEW (data);
  GimpImage        *image   = gimp_item_tree_view_get_image (view);
  GimpFillOptions  *options = gimp_fill_options_new (image->gimp, NULL, FALSE);

  gimp_fill_options_set_style (options, GIMP_FILL_STYLE_FG_COLOR);
  gimp_context_set_foreground (GIMP_CONTEXT (options), color);

  gimp_drawable_tree_view_new_dropped (view, options,
                                       C_("undo-type", "Drop color to layer"));

  g_object_unref (options);
}
