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
#include "tools/tools-types.h" /* FIXME */

#include "actions/gimpgeglprocedure.h"
#include "actions/filters-commands.h"

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

#include "tools/tool_manager.h" /* FIXME */

#include "gimpcontainertreestore.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimpdrawabletreeview.h"
#include "gimplayertreeview.h" /* FIXME */
#include "gimpviewrenderer.h"

#include "gimp-intl.h"


#define COLUMN_EFFECTS_ACTIVE 3


struct _GimpDrawableTreeViewPrivate
{
  GtkWidget       *effect_header_image;

  gint             model_column_effects;
  GtkCellRenderer *effects_cell;

  GtkWidget       *effects_popover;
  GtkWidget       *effects_box;
  GtkWidget       *effects_filters;
  GtkWidget       *effects_options;

  GtkWidget       *effects_visible_button;
  GtkWidget       *effects_edit_button;
  GtkWidget       *effects_raise_button;
  GtkWidget       *effects_lower_button;
  GtkWidget       *effects_merge_button;
  GtkWidget       *effects_remove_button;
  GimpDrawable    *effects_drawable;
  GimpDrawableFilter
                  *effects_filter;

  GimpTreeHandler *filters_changed_handler;
  GimpTreeHandler *filters_active_changed_handler;
};


static void   gimp_drawable_tree_view_view_iface_init (GimpContainerViewInterface *iface);

static void     gimp_drawable_tree_view_constructed   (GObject                    *object);
static void     gimp_drawable_tree_view_dispose       (GObject                    *object);

static void     gimp_drawable_tree_view_style_updated (GtkWidget                  *widget);

static void     gimp_drawable_tree_view_set_container (GimpContainerView          *view,
                                                       GimpContainer              *container);
static gpointer gimp_drawable_tree_view_insert_item   (GimpContainerView          *view,
                                                       GimpViewable               *viewable,
                                                       gpointer                    parent_insert_data,
                                                       gint                        index);

static gboolean gimp_drawable_tree_view_select_items  (GimpContainerView          *view,
                                                       GList                      *items,
                                                       GList                      *paths);

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

static void   gimp_drawable_tree_effects_set_sensitive(GimpDrawableTreeView        *view,
                                                       gboolean                     is_sensitive);

static void   gimp_drawable_tree_view_effects_clicked (GtkCellRendererToggle       *toggle,
                                                       gchar                       *path,
                                                       GdkModifierType              state,
                                                       GimpDrawableTreeView        *view);
static gboolean
              gimp_drawable_tree_view_effects_filters_selected
                                                      (GimpContainerView           *view,
                                                       GList                       *filters,
                                                       GList                       *paths,
                                                       GimpDrawableTreeView        *drawable_view);
static void   gimp_drawable_tree_view_effects_activate_filter
                                                      (GtkWidget                   *widget,
                                                       GimpViewable                *viewable,
                                                       gpointer                     insert_data,
                                                       GimpDrawableTreeView        *view);
static void   gimp_drawable_tree_view_filter_active_changed
                                                      (GimpFilter                  *item,
                                                       GimpContainerTreeView       *view);

static void   gimp_drawable_tree_view_effects_visible_toggled
                                                      (GtkCellRendererToggle       *toggle,
                                                       gchar                       *path_str,
                                                       GdkModifierType              state,
                                                       GimpContainerTreeView       *view);

static void   gimp_drawable_tree_view_effects_visible_all_toggled
                                                      (GtkWidget                   *widget,
                                                       GimpDrawableTreeView        *view);
static void   gimp_drawable_tree_view_effects_edit_clicked
                                                      (GtkWidget                   *widget,
                                                       GimpDrawableTreeView        *view);
static void   gimp_drawable_tree_view_effects_raise_clicked
                                                      (GtkWidget                   *widget,
                                                       GimpDrawableTreeView        *view);
static void   gimp_drawable_tree_view_effects_lower_clicked
                                                      (GtkWidget                   *widget,
                                                       GimpDrawableTreeView        *view);
static void   gimp_drawable_tree_view_effects_merge_clicked
                                                      (GtkWidget                   *widget,
                                                       GimpDrawableTreeView        *view);
static void   gimp_drawable_tree_view_effects_remove_clicked
                                                      (GtkWidget                   *widget,
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
  iface->insert_item   = gimp_drawable_tree_view_insert_item;
  iface->select_items  = gimp_drawable_tree_view_select_items;
}

static void
gimp_drawable_tree_view_init (GimpDrawableTreeView *view)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);

  view->priv = gimp_drawable_tree_view_get_instance_private (view);

  view->priv->model_column_effects =
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
  gint                   pixel_icon_size  = 16;
  gint                   button_spacing;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_widget_style_get (GTK_WIDGET (view),
                        "button-icon-size", &button_icon_size,
                        "button-spacing",   &button_spacing,
                        NULL);
  gtk_icon_size_lookup (button_icon_size, &pixel_icon_size, NULL);

  /* TODO: Expand layer effects to other drawable types */
  if (GIMP_IS_LAYER_TREE_VIEW (object))
    {
      GtkTreeViewColumn *column;
      GtkWidget         *image;

      column = gtk_tree_view_column_new ();
      image = gtk_image_new_from_icon_name (GIMP_ICON_EFFECT, button_icon_size);
      gimp_widget_set_identifier (image, "item-effect-header-icon");
      gtk_tree_view_column_set_widget (column, image);
      gtk_widget_set_visible (image, TRUE);
      gtk_tree_view_insert_column (tree_view->view, column, 2);
      view->priv->effect_header_image = image;

      view->priv->effects_cell = gimp_cell_renderer_toggle_new (GIMP_ICON_EFFECT);
      g_object_set (view->priv->effects_cell,
                    "xpad",      0,
                    "ypad",      0,
                    "icon-size", pixel_icon_size,
                    NULL);
      gtk_tree_view_column_pack_start (column, view->priv->effects_cell, FALSE);
      gtk_tree_view_column_set_attributes (column, view->priv->effects_cell,
                                           "active",
                                           view->priv->model_column_effects,
                                           NULL);

      gimp_container_tree_view_add_toggle_cell (tree_view,
                                                view->priv->effects_cell);

      g_signal_connect (view->priv->effects_cell, "clicked",
                        G_CALLBACK (gimp_drawable_tree_view_effects_clicked),
                        item_view);
    }

  /* Effects box. */
  if (GIMP_IS_LAYER_TREE_VIEW (object))
    {
      GtkWidget *image;
      GtkWidget *label;
      gchar     *text;

      view->priv->effects_filters = gtk_box_new (GTK_ORIENTATION_VERTICAL, button_spacing);
      view->priv->effects_box     = gtk_box_new (GTK_ORIENTATION_VERTICAL, button_spacing);
      view->priv->effects_options = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, button_spacing);

      /* Effects Buttons */
      view->priv->effects_visible_button = gtk_toggle_button_new ();
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (view->priv->effects_visible_button),
                                    TRUE);
      image = gtk_image_new_from_icon_name (GIMP_ICON_VISIBLE,
                                            GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_container_add (GTK_CONTAINER (view->priv->effects_visible_button), image);
      gimp_help_set_help_data (view->priv->effects_visible_button,
                               _("Toggle the visibility of all filters."),
                               GIMP_HELP_LAYER_EFFECTS);
      g_signal_connect (view->priv->effects_visible_button, "toggled",
                        G_CALLBACK (gimp_drawable_tree_view_effects_visible_all_toggled),
                        view);
      gtk_box_pack_start (GTK_BOX (view->priv->effects_options),
                          view->priv->effects_visible_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (image, TRUE);
      gtk_widget_set_visible (view->priv->effects_visible_button, TRUE);

      view->priv->effects_edit_button =
        gtk_button_new_from_icon_name (GIMP_ICON_EDIT,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gimp_help_set_help_data (view->priv->effects_edit_button,
                               _("Edit the selected filter."),
                               GIMP_HELP_LAYER_EFFECTS);
      g_signal_connect (view->priv->effects_edit_button, "clicked",
                        G_CALLBACK (gimp_drawable_tree_view_effects_edit_clicked),
                        view);
      gtk_box_pack_start (GTK_BOX (view->priv->effects_options),
                          view->priv->effects_edit_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (view->priv->effects_edit_button, TRUE);

      view->priv->effects_raise_button =
        gtk_button_new_from_icon_name (GIMP_ICON_GO_UP,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gimp_help_set_help_data (view->priv->effects_raise_button,
                               _("Raise filter one step up in the stack."),
                               GIMP_HELP_LAYER_EFFECTS);
      g_signal_connect (view->priv->effects_raise_button, "clicked",
                        G_CALLBACK (gimp_drawable_tree_view_effects_raise_clicked),
                        view);
      gtk_box_pack_start (GTK_BOX (view->priv->effects_options),
                          view->priv->effects_raise_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (view->priv->effects_raise_button, TRUE);

      view->priv->effects_lower_button =
        gtk_button_new_from_icon_name (GIMP_ICON_GO_DOWN,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gimp_help_set_help_data (view->priv->effects_lower_button,
                               _("Lower filter one step down in the stack."),
                               GIMP_HELP_LAYER_EFFECTS);
      g_signal_connect (view->priv->effects_lower_button, "clicked",
                        G_CALLBACK (gimp_drawable_tree_view_effects_lower_clicked),
                        view);
      gtk_box_pack_start (GTK_BOX (view->priv->effects_options),
                          view->priv->effects_lower_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (view->priv->effects_lower_button, TRUE);

      view->priv->effects_merge_button =
        gtk_button_new_from_icon_name (GIMP_ICON_LAYER_MERGE_DOWN,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gimp_help_set_help_data (view->priv->effects_merge_button,
                               _("Merge all active filters down."),
                               GIMP_HELP_LAYER_EFFECTS);
      g_signal_connect (view->priv->effects_merge_button, "clicked",
                        G_CALLBACK (gimp_drawable_tree_view_effects_merge_clicked),
                        view);
      gtk_box_pack_start (GTK_BOX (view->priv->effects_options),
                          view->priv->effects_merge_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (view->priv->effects_merge_button, TRUE);

      view->priv->effects_remove_button =
        gtk_button_new_from_icon_name (GIMP_ICON_EDIT_DELETE,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gimp_help_set_help_data (view->priv->effects_remove_button,
                               _("Remove the selected filter."),
                               GIMP_HELP_LAYER_EFFECTS);
      g_signal_connect (view->priv->effects_remove_button, "clicked",
                        G_CALLBACK (gimp_drawable_tree_view_effects_remove_clicked),
                        view);
      gtk_box_pack_start (GTK_BOX (view->priv->effects_options),
                          view->priv->effects_remove_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (view->priv->effects_remove_button, TRUE);

      label = gtk_label_new (NULL);
      text = g_strdup_printf ("<b>%s</b>",
                              _("Layer Effects"));
      gtk_label_set_markup (GTK_LABEL (label), text);
      gtk_widget_set_visible (label, TRUE);
      g_free (text);

      /* Effects popover. */
      view->priv->effects_popover = gtk_popover_new (GTK_WIDGET (tree_view->view));
      gtk_popover_set_modal (GTK_POPOVER (view->priv->effects_popover), TRUE);
      gtk_container_add (GTK_CONTAINER (view->priv->effects_popover),
                         view->priv->effects_filters);
      gimp_help_connect (view->priv->effects_filters, NULL,
                         gimp_standard_help_func, GIMP_HELP_LAYER_EFFECTS, NULL, NULL);
      gtk_box_pack_start (GTK_BOX (view->priv->effects_filters), label,
                          FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (view->priv->effects_filters),
                          view->priv->effects_box, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (view->priv->effects_filters),
                          view->priv->effects_options, FALSE, FALSE, 0);
      gtk_widget_set_visible (view->priv->effects_box, TRUE);
      gtk_widget_set_visible (view->priv->effects_options, TRUE);
      gtk_widget_set_visible (view->priv->effects_filters, TRUE);
    }

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

  g_clear_pointer (&view->priv->effects_popover, gtk_widget_destroy);

  view->priv->effects_drawable = NULL;
  view->priv->effects_filter   = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_drawable_tree_view_style_updated (GtkWidget *widget)
{
  GimpDrawableTreeView *view = GIMP_DRAWABLE_TREE_VIEW (widget);
  const gchar          *old_icon_name;
  gint                  pixel_icon_size;
  GtkIconSize           old_size;
  GtkIconSize           button_icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;

  gtk_icon_size_lookup (button_icon_size, &pixel_icon_size, NULL);

  if (GIMP_IS_LAYER_TREE_VIEW (view))
    {
      gtk_image_get_icon_name (GTK_IMAGE (view->priv->effect_header_image),
                               &old_icon_name, &old_size);

      if (button_icon_size != old_size)
        {
          gchar *icon_name = g_strdup (old_icon_name);

          gtk_image_set_from_icon_name (GTK_IMAGE (view->priv->effect_header_image),
                                        icon_name, button_icon_size);
          g_free (icon_name);
        }

      g_object_set (view->priv->effects_cell,
                    "icon-name", GIMP_ICON_EFFECT,
                    "icon-size", pixel_icon_size,
                    NULL);
    }

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
    {
      if (GIMP_IS_LAYER_TREE_VIEW (drawable_view))
        {
          gimp_tree_handler_disconnect (drawable_view->priv->filters_changed_handler);
          drawable_view->priv->filters_changed_handler = NULL;

          if (drawable_view->priv->filters_active_changed_handler)
            gimp_tree_handler_disconnect (drawable_view->priv->filters_active_changed_handler);

          drawable_view->priv->filters_active_changed_handler = NULL;
        }
    }

  parent_view_iface->set_container (view, container);

  if (container)
    {
      if (GIMP_IS_LAYER_TREE_VIEW (drawable_view))
        {
          drawable_view->priv->filters_changed_handler =
            gimp_tree_handler_connect (container, "filters-changed",
                                       G_CALLBACK (gimp_drawable_tree_view_filters_changed),
                                       view);
        }
    }
}

static gpointer
gimp_drawable_tree_view_insert_item (GimpContainerView *view,
                                     GimpViewable      *viewable,
                                     gpointer           parent_insert_data,
                                     gint               index)
{
  GimpContainerTreeView *tree_view     = GIMP_CONTAINER_TREE_VIEW (view);
  GimpDrawableTreeView  *drawable_view = GIMP_DRAWABLE_TREE_VIEW (view);
  GtkTreeIter           *iter;

  iter = parent_view_iface->insert_item (view, viewable,
                                         parent_insert_data, index);

  if (GIMP_IS_LAYER_TREE_VIEW (view))
    {
      GimpContainer *filters;
      gint           n_filters;

      filters   = gimp_drawable_get_filters (GIMP_DRAWABLE (viewable));
      n_filters = gimp_container_get_n_children (filters);

      gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                          drawable_view->priv->model_column_effects,
                          n_filters > 0,
                          -1);
    }

  return iter;
}

static gboolean
gimp_drawable_tree_view_select_items (GimpContainerView *view,
                                      GList             *items,
                                      GList             *paths)
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
    success = parent_view_iface->select_items (view, items, paths);

  return success;
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
    g_signal_handlers_disconnect_by_func (gimp_item_tree_view_get_image (view),
                                          gimp_drawable_tree_view_floating_selection_changed,
                                          view);

  GIMP_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (view, image);

  if (gimp_item_tree_view_get_image (view))
    g_signal_connect (gimp_item_tree_view_get_image (view),
                      "floating-selection-changed",
                      G_CALLBACK (gimp_drawable_tree_view_floating_selection_changed),
                      view);

  drawable_view->priv->effects_drawable               = NULL;
  drawable_view->priv->effects_filter                 = NULL;
  drawable_view->priv->filters_active_changed_handler = NULL;

  if (drawable_view->priv->effects_popover)
    gtk_widget_set_visible (drawable_view->priv->effects_popover, FALSE);
}


/*  callbacks  */

static void
gimp_drawable_tree_view_floating_selection_changed (GimpImage            *image,
                                                    GimpDrawableTreeView *view)
{
  GList *items;

  items = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_selected_items (image);
  items = g_list_copy (items);

  /*  update button states  */
  gimp_container_view_select_items (GIMP_CONTAINER_VIEW (view), items);

  g_list_free (items);
}


/*  filters callbacks  */

static void
gimp_drawable_tree_view_filters_changed (GimpDrawable         *drawable,
                                         GimpDrawableTreeView *view)
{
  GimpContainerView     *container_view = GIMP_CONTAINER_VIEW (view);
  GimpContainerTreeView *tree_view      = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter;
  GimpContainer         *filters;
  GList                 *list;
  gint                   n_filters      = 0;
  gboolean               fs_disabled    = FALSE;
  gboolean               temporary_only = TRUE;

  iter = gimp_container_view_lookup (container_view,
                                     (GimpViewable *) drawable);

  filters = gimp_drawable_get_filters (drawable);

  /* Since floating selections are also stored in the filter stack,
   * we need to verify what's in there to get the correct count
   */
  if (filters)
    {
      for (list = GIMP_LIST (filters)->queue->tail;
           list;
           list = g_list_previous (list))
        {
          if (GIMP_IS_DRAWABLE_FILTER (list->data))
            {
              n_filters++;

              if (temporary_only)
                g_object_get (list->data,
                              "temporary", &temporary_only,
                              NULL);
            }
          else
            {
              fs_disabled = TRUE;
            }
        }
    }

  /* Don't show icon if we only have a temporary filter
   * like a tool-based filter */
  if (temporary_only)
    n_filters = 0;

  if (n_filters == 0 || fs_disabled)
    view->priv->effects_filter = NULL;

  if (iter)
    gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                        view->priv->model_column_effects,
                        n_filters > 0,
                        -1);

  /* Re-enable buttons after editing */
  gimp_drawable_tree_effects_set_sensitive (view, ! fs_disabled);

  if (view->priv->effects_popover &&
      view->priv->effects_filter  &&
      ! fs_disabled)
    {
      if (GIMP_IS_DRAWABLE_FILTER (view->priv->effects_filter))
        {
          gint index = gimp_container_get_child_index (filters,
                                                       GIMP_OBJECT (view->priv->effects_filter));

          if (index == 0)
            gtk_widget_set_sensitive (view->priv->effects_raise_button, FALSE);
          else if (index == (n_filters - 1))
            gtk_widget_set_sensitive (view->priv->effects_lower_button, FALSE);
        }
    }
}

static void
gimp_drawable_tree_effects_set_sensitive (GimpDrawableTreeView *view,
                                          gboolean              is_sensitive)
{
  gboolean is_group = FALSE;

  /* Do not allow merging down effects on group layers */
  if (view->priv->effects_drawable &&
      gimp_viewable_get_children (GIMP_VIEWABLE (view->priv->effects_drawable)))
    is_group = TRUE;

  gtk_widget_set_sensitive (view->priv->effects_box,            is_sensitive);
  gtk_widget_set_sensitive (view->priv->effects_visible_button, is_sensitive);
  gtk_widget_set_sensitive (view->priv->effects_edit_button,    is_sensitive);
  gtk_widget_set_sensitive (view->priv->effects_raise_button,   is_sensitive);
  gtk_widget_set_sensitive (view->priv->effects_lower_button,   is_sensitive);
  gtk_widget_set_sensitive (view->priv->effects_merge_button,  (is_sensitive &&
                                                                ! is_group));
  gtk_widget_set_sensitive (view->priv->effects_remove_button,  is_sensitive);
}

static void
gimp_drawable_tree_view_effects_clicked (GtkCellRendererToggle *toggle,
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
      GimpContainer          *filters;
      GdkRectangle            rect;
      GtkWidget              *filter_view;
      GList                  *filter_list;
      GList                  *children;
      gint                    n_children = 0;
      gboolean                visible    = TRUE;

      children = gtk_container_get_children (GTK_CONTAINER (view->priv->effects_box));

      /* Update the filter state. */
      store = GIMP_CONTAINER_TREE_STORE (GIMP_CONTAINER_TREE_VIEW (view)->model);
      renderer = gimp_container_tree_store_get_renderer (store, &iter);
      drawable = GIMP_DRAWABLE (renderer->viewable);
      g_object_unref (renderer);

      /* Get filters */
      if (children)
        {
          g_signal_handlers_disconnect_by_func (children->data,
                                                gimp_drawable_tree_view_effects_filters_selected,
                                                view);
          g_signal_handlers_disconnect_by_func (children->data,
                                                gimp_drawable_tree_view_effects_activate_filter,
                                                view);
          gtk_widget_destroy (children->data);
          g_list_free (children);
        }

      view->priv->effects_drawable = drawable;

      filters = gimp_drawable_get_filters (drawable);

      for (filter_list = GIMP_LIST (filters)->queue->tail;
           filter_list;
           filter_list = g_list_previous (filter_list))
        {
          if (GIMP_IS_DRAWABLE_FILTER (filter_list->data))
            {
              if (! gimp_filter_get_active (GIMP_FILTER (filter_list->data)))
                visible = FALSE;

              n_children++;
            }
        }

      /* Set the initial value for the effect visibility toggle */
      g_signal_handlers_block_by_func (view->priv->effects_visible_button,
                                       gimp_drawable_tree_view_effects_visible_all_toggled,
                                       view);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (view->priv->effects_visible_button),
                                    visible);
      g_signal_handlers_unblock_by_func (view->priv->effects_visible_button,
                                         gimp_drawable_tree_view_effects_visible_all_toggled,
                                         view);

      /* Only show if we have at least one active filter */
      if (n_children > 0)
        {
          GtkCellRenderer       *renderer;
          GtkTreeViewColumn     *column;
          GimpContainerTreeView *filter_tree_view = NULL;
          GtkWidget             *scrolled_window  = NULL;
          gboolean               is_editing       = FALSE;

          filter_view = gimp_container_tree_view_new (filters,
                                                      gimp_container_view_get_context (GIMP_CONTAINER_VIEW (view)),
                                                      GIMP_VIEW_SIZE_SMALL, 0);
          filter_tree_view = GIMP_CONTAINER_TREE_VIEW (filter_view);

          /* Connect filter active signal */
          if (view->priv->filters_active_changed_handler)
            gimp_tree_handler_disconnect (view->priv->filters_active_changed_handler);

          view->priv->filters_active_changed_handler =
            gimp_tree_handler_connect (filters, "active-changed",
                                       G_CALLBACK (gimp_drawable_tree_view_filter_active_changed),
                                       filter_view);

          gimp_container_tree_store_columns_add (filter_tree_view->model_columns,
                                                 &filter_tree_view->n_model_columns,
                                                 G_TYPE_BOOLEAN);

          /* Set up individual visibility toggles */
          column   = gtk_tree_view_column_new ();
          renderer = gimp_cell_renderer_toggle_new (GIMP_ICON_VISIBLE);
          gtk_cell_renderer_toggle_set_active (GTK_CELL_RENDERER_TOGGLE (renderer), TRUE);
          gtk_tree_view_column_pack_end (column, renderer, FALSE);
          gtk_tree_view_column_set_attributes (column, renderer,
                                               "active",
                                               COLUMN_EFFECTS_ACTIVE,
                                               NULL);

          gtk_tree_view_append_column (filter_tree_view->view,
                                       column);
          gtk_tree_view_move_column_after (filter_tree_view->view,
                                           column, NULL);
          gimp_container_tree_view_add_toggle_cell (filter_tree_view,
                                                    renderer);

          g_signal_connect_object (renderer, "clicked",
                                   G_CALLBACK (gimp_drawable_tree_view_effects_visible_toggled),
                                   filter_tree_view, 0);

          /* Update filter visible icon */
          for (filter_list = GIMP_LIST (filters)->queue->tail; filter_list;
               filter_list = g_list_previous (filter_list))
            {
              if (GIMP_IS_DRAWABLE_FILTER (filter_list->data))
                {
                  gboolean is_temporary;

                  gimp_drawable_tree_view_filter_active_changed (GIMP_FILTER (filter_list->data),
                                                                 filter_tree_view);

                  g_object_get (filter_list->data,
                                "temporary", &is_temporary,
                                NULL);
                  if (is_temporary)
                    is_editing = TRUE;
                }
            }

          g_signal_connect (filter_tree_view, "select-items",
                            G_CALLBACK (gimp_drawable_tree_view_effects_filters_selected),
                            view);
          g_signal_connect_object (filter_tree_view,
                                   "activate-item",
                                   G_CALLBACK (gimp_drawable_tree_view_effects_activate_filter),
                                   view, 0);

          gtk_box_pack_start (GTK_BOX (view->priv->effects_box), filter_view, TRUE, TRUE, 0);
          gtk_widget_set_visible (filter_view, TRUE);

          scrolled_window = gtk_widget_get_parent (GTK_WIDGET (filter_tree_view->view));
          gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                          GTK_POLICY_NEVER,
                                          GTK_POLICY_NEVER);
          gtk_widget_set_size_request (view->priv->effects_box, -1, 24 * (n_children + 1));

          /* Change popover position. */
          gtk_tree_view_get_cell_area (GIMP_CONTAINER_TREE_VIEW (view)->view, path,
                                       gtk_tree_view_get_column (GIMP_CONTAINER_TREE_VIEW (view)->view, 2),
                                       &rect);
          gtk_tree_view_convert_bin_window_to_widget_coords (GIMP_CONTAINER_TREE_VIEW (view)->view,
                                                             rect.x, rect.y, &rect.x, &rect.y);
          gtk_popover_set_pointing_to (GTK_POPOVER (view->priv->effects_popover), &rect);

          gimp_drawable_tree_view_filters_changed (drawable, view);
          gtk_widget_show (view->priv->effects_popover);

          /* Lock filter options if we're actively editing a filter */
          gimp_drawable_tree_effects_set_sensitive (view, ! is_editing);
        }
    }
}

static gboolean
gimp_drawable_tree_view_effects_filters_selected (GimpContainerView    *view,
                                                  GList                *filters,
                                                  GList                *paths,
                                                  GimpDrawableTreeView *drawable_view)
{
  g_return_val_if_fail (g_list_length (filters) <= 1, FALSE);

  if (filters && GIMP_IS_DRAWABLE (drawable_view->priv->effects_drawable))
    {
      GimpDrawableFilter *filter;
      GimpContainer      *container;
      gint                index         = -1;
      gint                n_children    = 0;
      gboolean            is_blocked_op = FALSE;
      GeglNode           *op_node       = NULL;

      /* Don't set floating selection as active filter */
      if (GIMP_IS_DRAWABLE_FILTER (filters->data))
        {
          filter = filters->data;

          drawable_view->priv->effects_filter = filter;

          container =
            gimp_drawable_get_filters (GIMP_DRAWABLE (drawable_view->priv->effects_drawable));

          index = gimp_container_get_child_index (container,
                                                  GIMP_OBJECT (filter));

          n_children = gimp_container_get_n_children (container);

          /* TODO: For now, prevent raising/lowering tool operations like Warp. */
          op_node = gimp_drawable_filter_get_operation (filter);
          if (op_node &&
              ! strcmp (gegl_node_get_operation (op_node), "GraphNode"))
            is_blocked_op = TRUE;
        }
      else
        {
          is_blocked_op = TRUE;
        }

      gtk_widget_set_sensitive (drawable_view->priv->effects_remove_button,
                                ! is_blocked_op);
      gtk_widget_set_sensitive (drawable_view->priv->effects_raise_button,
                                (index != 0) && ! is_blocked_op);
      gtk_widget_set_sensitive (drawable_view->priv->effects_lower_button,
                                (index != n_children - 1) && ! is_blocked_op);
    }

  return TRUE;
}

static void
gimp_drawable_tree_view_effects_activate_filter (GtkWidget             *widget,
                                                 GimpViewable          *viewable,
                                                 gpointer               insert_data,
                                                 GimpDrawableTreeView  *view)
{
  if (gtk_widget_get_sensitive (view->priv->effects_edit_button))
    gimp_drawable_tree_view_effects_edit_clicked (widget, view);
}

static void
gimp_drawable_tree_view_filter_active_changed (GimpFilter            *filter,
                                               GimpContainerTreeView *view)
{
  GtkTreeIter *iter;

  iter = gimp_container_view_lookup (GIMP_CONTAINER_VIEW (view),
                                     (GimpViewable *) filter);

  if (iter)
    gtk_tree_store_set (GTK_TREE_STORE (view->model), iter,
                        COLUMN_EFFECTS_ACTIVE,
                        gimp_filter_get_active (filter),
                        -1);
}

static void
gimp_drawable_tree_view_effects_visible_toggled (GtkCellRendererToggle *toggle,
                                                 gchar                 *path_str,
                                                 GdkModifierType        state,
                                                 GimpContainerTreeView *view)
{
  GtkTreePath *path;
  GtkTreeIter  iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (view->model, &iter, path))
    {
      GimpViewRenderer       *renderer;
      GimpContainerTreeStore *store;
      GimpDrawableFilter     *filter;
      gboolean                visible;

      /* Update the filter state. */
      store = GIMP_CONTAINER_TREE_STORE (view->model);
      renderer = gimp_container_tree_store_get_renderer (store, &iter);
      filter = GIMP_DRAWABLE_FILTER (renderer->viewable);
      g_object_unref (renderer);

      if (GIMP_IS_DRAWABLE_FILTER (filter))
        {
          GimpDrawable *drawable;

          drawable = gimp_drawable_filter_get_drawable (filter);

          if (drawable)
            {
              visible = gimp_filter_get_active (GIMP_FILTER (filter));

              gimp_filter_set_active (GIMP_FILTER (filter), ! visible);

              gimp_item_refresh_filters (GIMP_ITEM (drawable));
            }
        }
    }
}

static void
gimp_drawable_tree_view_effects_visible_all_toggled (GtkWidget            *widget,
                                                     GimpDrawableTreeView *view)
{
  if (view->priv->effects_drawable)
    {
      gboolean       visible;
      GimpContainer *filter_stack;
      GList         *list;

      visible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
      filter_stack = gimp_drawable_get_filters (GIMP_DRAWABLE (view->priv->effects_drawable));

      for (list = GIMP_LIST (filter_stack)->queue->head; list;
           list = g_list_next (list))
        {
          if (GIMP_IS_DRAWABLE_FILTER (list->data))
            {
              GimpFilter *filter = list->data;

              gimp_filter_set_active (filter, visible);
            }
        }

      gimp_item_refresh_filters (GIMP_ITEM (view->priv->effects_drawable));
    }
}

static void
gimp_drawable_tree_view_effects_edit_clicked (GtkWidget            *widget,
                                              GimpDrawableTreeView *view)
{
  GimpImage    *image = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));
  GimpDrawable *drawable;
  GeglNode     *op;

  if (! GIMP_IS_DRAWABLE_FILTER (view->priv->effects_filter))
    return;

  drawable = gimp_drawable_filter_get_drawable (view->priv->effects_filter);

  if (drawable && GIMP_IS_DRAWABLE (drawable))
    {
      if (! gimp_item_is_visible (GIMP_ITEM (drawable)) &&
          ! GIMP_GUI_CONFIG (image->gimp->config)->edit_non_visible)
        {
          gimp_message_literal (image->gimp, G_OBJECT (view),
                                GIMP_MESSAGE_ERROR,
                               _("A selected layer is not visible."));
          return;
        }
      else if (gimp_item_get_lock_content (GIMP_ITEM (drawable)))
        {
          gimp_message_literal (image->gimp, G_OBJECT (view),
                                GIMP_MESSAGE_WARNING,
                                _("A selected layer's pixels are locked."));
          return;
        }

      op = gimp_drawable_filter_get_operation (view->priv->effects_filter);
      if (op)
        {
          GimpProcedure *procedure;
          GVariant      *variant;
          gchar         *operation;
          gchar         *name;

          g_object_get (view->priv->effects_filter,
                        "name", &name,
                        NULL);
          g_object_get (op,
                        "operation", &operation,
                        NULL);

          if (operation)
            {
              procedure = gimp_gegl_procedure_new (image->gimp,
                                                   view->priv->effects_filter,
                                                   GIMP_RUN_INTERACTIVE, NULL,
                                                   operation,
                                                   name,
                                                   name,
                                                   NULL, NULL, NULL);

              variant = g_variant_new_uint64 (GPOINTER_TO_SIZE (procedure));
              g_variant_take_ref (variant);
              filters_run_procedure (image->gimp,
                                     gimp_context_get_display (gimp_get_user_context (image->gimp)),
                                     procedure, GIMP_RUN_INTERACTIVE);

              g_variant_unref (variant);
              g_object_unref (procedure);

              /* Disable buttons until we're done editing */
              gimp_drawable_tree_effects_set_sensitive (view, FALSE);
            }

          g_free (name);
          g_free (operation);
        }
    }
}

static void
gimp_drawable_tree_view_effects_raise_clicked (GtkWidget            *widget,
                                               GimpDrawableTreeView *view)
{
  GimpImage *image = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));

  if (! GIMP_IS_DRAWABLE_FILTER (view->priv->effects_filter))
    return;

  if (gimp_drawable_filter_get_mask (view->priv->effects_filter) == NULL)
    {
      gimp_message_literal (image->gimp, G_OBJECT (view), GIMP_MESSAGE_ERROR,
                            _("Cannot reorder a filter that is being edited."));
      return;
    }

  if (gimp_drawable_raise_filter (view->priv->effects_drawable,
                                  GIMP_FILTER (view->priv->effects_filter)))
    {
      if (gtk_widget_get_sensitive (view->priv->effects_edit_button))
        {
          GimpContainer *container;
          gint           index;

          container =
            gimp_drawable_get_filters (GIMP_DRAWABLE (view->priv->effects_drawable));

          index = gimp_container_get_child_index (container,
                                                  GIMP_OBJECT (view->priv->effects_filter));

          gtk_widget_set_sensitive (view->priv->effects_lower_button, TRUE);
          if (index == 0)
            gtk_widget_set_sensitive (view->priv->effects_raise_button, FALSE);
        }
    }
}

static void
gimp_drawable_tree_view_effects_lower_clicked (GtkWidget            *widget,
                                               GimpDrawableTreeView *view)
{
  GimpImage *image = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));

  if (! GIMP_IS_DRAWABLE_FILTER (view->priv->effects_filter))
    return;

  if (gimp_drawable_filter_get_mask (view->priv->effects_filter) == NULL)
    {
      gimp_message_literal (image->gimp, G_OBJECT (view), GIMP_MESSAGE_ERROR,
                            _("Cannot reorder a filter that is being edited."));
      return;
    }

  if (gimp_drawable_lower_filter (view->priv->effects_drawable,
                                  GIMP_FILTER (view->priv->effects_filter)))
    {
      if (gtk_widget_get_sensitive (view->priv->effects_edit_button))
        {
          GimpContainer *container;
          gint           index;

          container =
            gimp_drawable_get_filters (GIMP_DRAWABLE (view->priv->effects_drawable));

          index = gimp_container_get_child_index (container,
                                                  GIMP_OBJECT (view->priv->effects_filter));

          gtk_widget_set_sensitive (view->priv->effects_raise_button, TRUE);
          if (index == gimp_container_get_n_children (container) - 1)
            gtk_widget_set_sensitive (view->priv->effects_lower_button, FALSE);
        }
    }
}

static void
gimp_drawable_tree_view_effects_merge_clicked (GtkWidget            *widget,
                                               GimpDrawableTreeView *view)
{
  GimpContext  *context;
  GimpToolInfo *active_tool;

  if (! GIMP_IS_DRAWABLE_FILTER (view->priv->effects_filter))
    return;

  /* Commit GEGL-based tools before trying to merge filters */
  context     = gimp_container_view_get_context (GIMP_CONTAINER_VIEW (view));
  active_tool = gimp_context_get_tool (context);

  if (! strcmp (gimp_object_get_name (active_tool), "gimp-cage-tool")     ||
      ! strcmp (gimp_object_get_name (active_tool), "gimp-gradient-tool") ||
      ! strcmp (gimp_object_get_name (active_tool), "gimp-warp-tool"))
    {
      tool_manager_control_active (context->gimp, GIMP_TOOL_ACTION_COMMIT,
                                   gimp_context_get_display (context));
    }

  if (view->priv->effects_drawable &&
      ! gimp_viewable_get_children (GIMP_VIEWABLE (view->priv->effects_drawable)))
    {
      GimpImage *image = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));

      /* Don't merge if the layer is currently locked */
      if (gimp_item_get_lock_content (GIMP_ITEM (view->priv->effects_drawable)))
        {
          gimp_message_literal (image->gimp, G_OBJECT (view),
                                GIMP_MESSAGE_WARNING,
                                _("The layer to merge down to is locked."));
          return;
        }

      gimp_drawable_merge_filters (GIMP_DRAWABLE (view->priv->effects_drawable));
      gimp_drawable_clear_filters (GIMP_DRAWABLE (view->priv->effects_drawable));

      view->priv->effects_filter = NULL;

      /* Close NDE pop-over on successful merge */
      gtk_widget_set_visible (view->priv->effects_popover, FALSE);

      gimp_item_refresh_filters (GIMP_ITEM (view->priv->effects_drawable));
    }
}

static void
gimp_drawable_tree_view_effects_remove_clicked (GtkWidget            *widget,
                                                GimpDrawableTreeView *view)
{
  if (view->priv->effects_drawable)
    {
      GimpImage     *image = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));
      GimpContainer *filters;

      filters = gimp_drawable_get_filters (view->priv->effects_drawable);

      if (filters != NULL &&
          gimp_container_have (filters, GIMP_OBJECT (view->priv->effects_filter)))
        {
          gimp_image_undo_push_filter_remove (image, _("Remove filter"),
                                              view->priv->effects_drawable,
                                              view->priv->effects_filter);

          gimp_drawable_filter_abort (view->priv->effects_filter);

          /* Toggle the popover off if all effects are deleted */
          if (gimp_container_get_n_children (filters) == 0)
            gtk_widget_set_visible (view->priv->effects_popover, FALSE);
        }

      gimp_item_refresh_filters (GIMP_ITEM (view->priv->effects_drawable));
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
