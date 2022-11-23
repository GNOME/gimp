/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmalayertreeview.c
 * Copyright (C) 2001-2009 Michael Natterer <mitch@ligma.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#if 0
/* For mask features in ligma_layer_tree_view_layer_clicked() */
#include "config/ligmadialogconfig.h"
#endif

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaimage-undo-push.h"
#include "core/ligmaimage.h"
#include "core/ligmaitemlist.h"
#include "core/ligmaitemundo.h"
#include "core/ligmalayer.h"
#include "core/ligmalayer-floating-selection.h"
#include "core/ligmalayer-new.h"
#include "core/ligmalayermask.h"
#include "core/ligmatreehandler.h"

#include "text/ligmatextlayer.h"

#include "file/file-open.h"

#include "ligmaactiongroup.h"
#include "ligmacellrendererviewable.h"
#include "ligmacontainertreestore.h"
#include "ligmacontainerview.h"
#include "ligmadnd.h"
#include "ligmahelp-ids.h"
#include "ligmalayermodebox.h"
#include "ligmalayertreeview.h"
#include "ligmauimanager.h"
#include "ligmaviewrenderer.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


struct _LigmaLayerTreeViewPrivate
{
  GtkWidget       *layer_mode_box;
  GtkAdjustment   *opacity_adjustment;
  GtkWidget       *anchor_button;

  GtkWidget       *link_button;
  GtkWidget       *link_popover;
  GtkWidget       *new_link_button;
  GtkWidget       *link_list;
  GtkWidget       *link_entry;
  GtkWidget       *link_search_entry;
  LigmaItemList    *link_pattern_set;

  gint             model_column_mask;
  gint             model_column_mask_visible;

  GtkCellRenderer *mask_cell;

  PangoAttrList   *italic_attrs;
  PangoAttrList   *bold_attrs;

  LigmaTreeHandler *mode_changed_handler;
  LigmaTreeHandler *opacity_changed_handler;
  LigmaTreeHandler *mask_changed_handler;
  LigmaTreeHandler *alpha_changed_handler;
};


static void       ligma_layer_tree_view_view_iface_init            (LigmaContainerViewInterface *iface);

static void       ligma_layer_tree_view_constructed                (GObject                    *object);
static void       ligma_layer_tree_view_finalize                   (GObject                    *object);

static void       ligma_layer_tree_view_style_updated              (GtkWidget                  *widget);

static void       ligma_layer_tree_view_set_container              (LigmaContainerView          *view,
                                                                   LigmaContainer              *container);
static void       ligma_layer_tree_view_set_context                (LigmaContainerView          *view,
                                                                   LigmaContext                *context);
static gpointer   ligma_layer_tree_view_insert_item                (LigmaContainerView          *view,
                                                                   LigmaViewable               *viewable,
                                                                   gpointer                    parent_insert_data,
                                                                   gint                        index);
static gboolean   ligma_layer_tree_view_select_items               (LigmaContainerView          *view,
                                                                   GList                      *items,
                                                                   GList                      *paths);
static void       ligma_layer_tree_view_set_view_size              (LigmaContainerView          *view);
static gboolean   ligma_layer_tree_view_drop_possible              (LigmaContainerTreeView      *view,
                                                                   LigmaDndType                 src_type,
                                                                   GList                      *src_viewables,
                                                                   LigmaViewable               *dest_viewable,
                                                                   GtkTreePath                *drop_path,
                                                                   GtkTreeViewDropPosition     drop_pos,
                                                                   GtkTreeViewDropPosition    *return_drop_pos,
                                                                   GdkDragAction              *return_drag_action);
static void       ligma_layer_tree_view_drop_color                 (LigmaContainerTreeView      *view,
                                                                   const LigmaRGB              *color,
                                                                   LigmaViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);
static void       ligma_layer_tree_view_drop_uri_list              (LigmaContainerTreeView      *view,
                                                                   GList                      *uri_list,
                                                                   LigmaViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);
static void       ligma_layer_tree_view_drop_component             (LigmaContainerTreeView      *tree_view,
                                                                   LigmaImage                  *image,
                                                                   LigmaChannelType             component,
                                                                   LigmaViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);
static void       ligma_layer_tree_view_drop_pixbuf                (LigmaContainerTreeView      *tree_view,
                                                                   GdkPixbuf                  *pixbuf,
                                                                   LigmaViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);
static void       ligma_layer_tree_view_set_image                  (LigmaItemTreeView           *view,
                                                                   LigmaImage                  *image);
static LigmaItem * ligma_layer_tree_view_item_new                   (LigmaImage                  *image);
static void       ligma_layer_tree_view_floating_selection_changed (LigmaImage                  *image,
                                                                   LigmaLayerTreeView          *view);

static void       ligma_layer_tree_view_layer_links_changed        (LigmaImage                  *image,
                                                                   LigmaLayerTreeView          *view);
static gboolean   ligma_layer_tree_view_link_clicked               (GtkWidget                  *box,
                                                                   GdkEvent                   *event,
                                                                   LigmaLayerTreeView          *view);
static void       ligma_layer_tree_view_link_popover_shown         (GtkPopover                 *popover,
                                                                   LigmaLayerTreeView          *view);

static gboolean   ligma_layer_tree_view_search_key_release         (GtkWidget                  *widget,
                                                                   GdkEventKey                *event,
                                                                   LigmaLayerTreeView          *view);

static void       ligma_layer_tree_view_new_link_exit              (LigmaLayerTreeView          *view);
static gboolean   ligma_layer_tree_view_new_link_clicked           (LigmaLayerTreeView          *view);
static gboolean   ligma_layer_tree_view_unlink_clicked             (GtkWidget                  *widget,
                                                                   GdkEvent                   *event,
                                                                   LigmaLayerTreeView          *view);

static void       ligma_layer_tree_view_layer_mode_box_callback    (GtkWidget                  *widget,
                                                                   const GParamSpec           *pspec,
                                                                   LigmaLayerTreeView          *view);
static void       ligma_layer_tree_view_opacity_scale_changed      (GtkAdjustment              *adj,
                                                                   LigmaLayerTreeView          *view);
static void       ligma_layer_tree_view_layer_signal_handler       (LigmaLayer                  *layer,
                                                                   LigmaLayerTreeView          *view);
static void       ligma_layer_tree_view_update_options             (LigmaLayerTreeView          *view,
                                                                   GList                      *layers);
static void       ligma_layer_tree_view_update_menu                (LigmaLayerTreeView          *view,
                                                                   GList                      *layers);
static void       ligma_layer_tree_view_update_highlight           (LigmaLayerTreeView          *view);
static void       ligma_layer_tree_view_mask_update                (LigmaLayerTreeView          *view,
                                                                   GtkTreeIter                *iter,
                                                                   LigmaLayer                  *layer);
static void       ligma_layer_tree_view_mask_changed               (LigmaLayer                  *layer,
                                                                   LigmaLayerTreeView          *view);
static void       ligma_layer_tree_view_renderer_update            (LigmaViewRenderer           *renderer,
                                                                   LigmaLayerTreeView          *view);
static void       ligma_layer_tree_view_update_borders             (LigmaLayerTreeView          *view,
                                                                   GtkTreeIter                *iter);
static void       ligma_layer_tree_view_mask_callback              (LigmaLayer                  *mask,
                                                                   LigmaLayerTreeView          *view);
static void       ligma_layer_tree_view_layer_clicked              (LigmaCellRendererViewable   *cell,
                                                                   const gchar                *path,
                                                                   GdkModifierType             state,
                                                                   LigmaLayerTreeView          *view);
static void       ligma_layer_tree_view_mask_clicked               (LigmaCellRendererViewable   *cell,
                                                                   const gchar                *path,
                                                                   GdkModifierType             state,
                                                                   LigmaLayerTreeView          *view);
static void       ligma_layer_tree_view_alpha_update               (LigmaLayerTreeView          *view,
                                                                   GtkTreeIter                *iter,
                                                                   LigmaLayer                  *layer);
static void       ligma_layer_tree_view_alpha_changed              (LigmaLayer                  *layer,
                                                                   LigmaLayerTreeView          *view);


G_DEFINE_TYPE_WITH_CODE (LigmaLayerTreeView, ligma_layer_tree_view,
                         LIGMA_TYPE_DRAWABLE_TREE_VIEW,
                         G_ADD_PRIVATE (LigmaLayerTreeView)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONTAINER_VIEW,
                                                ligma_layer_tree_view_view_iface_init))

#define parent_class ligma_layer_tree_view_parent_class

static LigmaContainerViewInterface *parent_view_iface = NULL;


static void
ligma_layer_tree_view_class_init (LigmaLayerTreeViewClass *klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  LigmaContainerTreeViewClass *tree_view_class;
  LigmaItemTreeViewClass      *item_view_class;
  GtkWidgetClass             *widget_class;

  widget_class    = GTK_WIDGET_CLASS (klass);
  tree_view_class = LIGMA_CONTAINER_TREE_VIEW_CLASS (klass);
  item_view_class = LIGMA_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructed = ligma_layer_tree_view_constructed;
  object_class->finalize    = ligma_layer_tree_view_finalize;

  widget_class->style_updated      = ligma_layer_tree_view_style_updated;

  tree_view_class->drop_possible   = ligma_layer_tree_view_drop_possible;
  tree_view_class->drop_color      = ligma_layer_tree_view_drop_color;
  tree_view_class->drop_uri_list   = ligma_layer_tree_view_drop_uri_list;
  tree_view_class->drop_component  = ligma_layer_tree_view_drop_component;
  tree_view_class->drop_pixbuf     = ligma_layer_tree_view_drop_pixbuf;

  item_view_class->item_type       = LIGMA_TYPE_LAYER;
  item_view_class->signal_name     = "selected-layers-changed";

  item_view_class->set_image       = ligma_layer_tree_view_set_image;
  item_view_class->get_container   = ligma_image_get_layers;
  item_view_class->get_selected_items = (LigmaGetItemsFunc) ligma_image_get_selected_layers;
  item_view_class->set_selected_items = (LigmaSetItemsFunc) ligma_image_set_selected_layers;
  item_view_class->add_item        = (LigmaAddItemFunc) ligma_image_add_layer;
  item_view_class->remove_item     = (LigmaRemoveItemFunc) ligma_image_remove_layer;
  item_view_class->new_item        = ligma_layer_tree_view_item_new;

  item_view_class->action_group          = "layers";
  item_view_class->activate_action       = "layers-edit";
  item_view_class->new_action            = "layers-new";
  item_view_class->new_default_action    = "layers-new-last-values";
  item_view_class->raise_action          = "layers-raise";
  item_view_class->raise_top_action      = "layers-raise-to-top";
  item_view_class->lower_action          = "layers-lower";
  item_view_class->lower_bottom_action   = "layers-lower-to-bottom";
  item_view_class->duplicate_action      = "layers-duplicate";
  item_view_class->delete_action         = "layers-delete";
  item_view_class->lock_content_help_id  = LIGMA_HELP_LAYER_LOCK_PIXELS;
  item_view_class->lock_position_help_id = LIGMA_HELP_LAYER_LOCK_POSITION;
}

static void
ligma_layer_tree_view_view_iface_init (LigmaContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->set_container = ligma_layer_tree_view_set_container;
  iface->set_context   = ligma_layer_tree_view_set_context;
  iface->insert_item   = ligma_layer_tree_view_insert_item;
  iface->select_items  = ligma_layer_tree_view_select_items;
  iface->set_view_size = ligma_layer_tree_view_set_view_size;

  iface->model_is_tree = TRUE;
}

static void
ligma_layer_tree_view_init (LigmaLayerTreeView *view)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
  GtkWidget             *scale;
  PangoAttribute        *attr;

  view->priv = ligma_layer_tree_view_get_instance_private (view);

  view->priv->link_pattern_set = NULL;

  view->priv->model_column_mask =
    ligma_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           LIGMA_TYPE_VIEW_RENDERER);

  view->priv->model_column_mask_visible =
    ligma_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);

  /*  Paint mode menu  */

  view->priv->layer_mode_box = ligma_layer_mode_box_new (LIGMA_LAYER_MODE_CONTEXT_LAYER);
  ligma_layer_mode_box_set_label (LIGMA_LAYER_MODE_BOX (view->priv->layer_mode_box),
                                 _("Mode"));
  ligma_item_tree_view_add_options (LIGMA_ITEM_TREE_VIEW (view), NULL,
                                   view->priv->layer_mode_box);

  g_signal_connect (view->priv->layer_mode_box, "notify::layer-mode",
                    G_CALLBACK (ligma_layer_tree_view_layer_mode_box_callback),
                    view);

  ligma_help_set_help_data (view->priv->layer_mode_box, NULL,
                           LIGMA_HELP_LAYER_DIALOG_PAINT_MODE_MENU);

  /*  Opacity scale  */

  view->priv->opacity_adjustment = gtk_adjustment_new (100.0, 0.0, 100.0,
                                                       1.0, 10.0, 0.0);
  scale = ligma_spin_scale_new (view->priv->opacity_adjustment, _("Opacity"), 1);
  ligma_spin_scale_set_constrain_drag (LIGMA_SPIN_SCALE (scale), TRUE);
  ligma_help_set_help_data (scale, NULL,
                           LIGMA_HELP_LAYER_DIALOG_OPACITY_SCALE);
  ligma_item_tree_view_add_options (LIGMA_ITEM_TREE_VIEW (view),
                                   NULL, scale);

  g_signal_connect (view->priv->opacity_adjustment, "value-changed",
                    G_CALLBACK (ligma_layer_tree_view_opacity_scale_changed),
                    view);

  view->priv->italic_attrs = pango_attr_list_new ();
  attr = pango_attr_style_new (PANGO_STYLE_ITALIC);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (view->priv->italic_attrs, attr);

  view->priv->bold_attrs = pango_attr_list_new ();
  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (view->priv->bold_attrs, attr);
}

static void
ligma_layer_tree_view_constructed (GObject *object)
{
  LigmaContainerTreeView *tree_view  = LIGMA_CONTAINER_TREE_VIEW (object);
  LigmaLayerTreeView     *layer_view = LIGMA_LAYER_TREE_VIEW (object);
  GtkWidget             *button;
  GtkWidget             *placeholder;
  GtkWidget             *grid;
  PangoAttrList         *attrs;
  GtkIconSize            button_size;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  layer_view->priv->mask_cell = ligma_cell_renderer_viewable_new ();
  gtk_tree_view_column_pack_start (tree_view->main_column,
                                   layer_view->priv->mask_cell,
                                   FALSE);
  gtk_tree_view_column_set_attributes (tree_view->main_column,
                                       layer_view->priv->mask_cell,
                                       "renderer",
                                       layer_view->priv->model_column_mask,
                                       "visible",
                                       layer_view->priv->model_column_mask_visible,
                                       NULL);

  ligma_container_tree_view_add_renderer_cell (tree_view,
                                              layer_view->priv->mask_cell,
                                              layer_view->priv->model_column_mask);

  g_signal_connect (tree_view->renderer_cell, "clicked",
                    G_CALLBACK (ligma_layer_tree_view_layer_clicked),
                    layer_view);
  g_signal_connect (layer_view->priv->mask_cell, "clicked",
                    G_CALLBACK (ligma_layer_tree_view_mask_clicked),
                    layer_view);

  ligma_dnd_component_dest_add (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);
  ligma_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), LIGMA_TYPE_CHANNEL,
                               NULL, tree_view);
  ligma_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), LIGMA_TYPE_LAYER_MASK,
                               NULL, tree_view);
  ligma_dnd_uri_list_dest_add  (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);
  ligma_dnd_pixbuf_dest_add    (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);

  button = ligma_editor_add_action_button (LIGMA_EDITOR (layer_view), "layers",
                                          "layers-new-group", NULL);
  gtk_box_reorder_child (ligma_editor_get_button_box (LIGMA_EDITOR (layer_view)),
                         button, 1);

  button = ligma_editor_add_action_button (LIGMA_EDITOR (layer_view), "layers",
                                          "layers-anchor", NULL);
  layer_view->priv->anchor_button = button;
  ligma_container_view_enable_dnd (LIGMA_CONTAINER_VIEW (layer_view),
                                  GTK_BUTTON (button),
                                  LIGMA_TYPE_LAYER);
  gtk_box_reorder_child (ligma_editor_get_button_box (LIGMA_EDITOR (layer_view)),
                         button, 5);

  button = ligma_editor_add_action_button (LIGMA_EDITOR (layer_view), "layers",
                                          "layers-merge-down-button",
                                          "layers-merge-group",
                                          GDK_SHIFT_MASK,
                                          "layers-merge-layers",
                                          GDK_CONTROL_MASK,
                                          "layers-merge-layers-last-values",
                                          GDK_CONTROL_MASK |
                                          GDK_SHIFT_MASK,
                                          NULL);
  ligma_container_view_enable_dnd (LIGMA_CONTAINER_VIEW (layer_view),
                                  GTK_BUTTON (button),
                                  LIGMA_TYPE_LAYER);
  gtk_box_reorder_child (ligma_editor_get_button_box (LIGMA_EDITOR (layer_view)),
                         button, 6);

  button = ligma_editor_add_action_button (LIGMA_EDITOR (layer_view), "layers",
                                          "layers-mask-add-button",
                                          "layers-mask-add-last-values",
                                          ligma_get_extend_selection_mask (),
                                          "layers-mask-delete",
                                          ligma_get_modify_selection_mask (),
                                          "layers-mask-apply",
                                          ligma_get_extend_selection_mask () |
                                          ligma_get_modify_selection_mask (),
                                          NULL);
  ligma_container_view_enable_dnd (LIGMA_CONTAINER_VIEW (layer_view),
                                  GTK_BUTTON (button),
                                  LIGMA_TYPE_LAYER);
  gtk_box_reorder_child (ligma_editor_get_button_box (LIGMA_EDITOR (layer_view)),
                         button, 7);

  /* Link popover menu. */

  layer_view->priv->link_button = gtk_menu_button_new ();
  gtk_widget_set_tooltip_text (layer_view->priv->link_button,
                               _("Select layers by patterns and store layer sets"));
  gtk_widget_style_get (GTK_WIDGET (layer_view),
                        "button-icon-size", &button_size,
                        NULL);
  gtk_button_set_image (GTK_BUTTON (layer_view->priv->link_button),
                        gtk_image_new_from_icon_name (LIGMA_ICON_LINKED,
                                                      button_size));
  gtk_box_pack_start (ligma_editor_get_button_box (LIGMA_EDITOR (layer_view)),
                      layer_view->priv->link_button, TRUE, TRUE, 0);
  gtk_widget_show (layer_view->priv->link_button);
  ligma_container_view_enable_dnd (LIGMA_CONTAINER_VIEW (layer_view),
                                  GTK_BUTTON (layer_view->priv->link_button),
                                  LIGMA_TYPE_LAYER);
  gtk_box_reorder_child (ligma_editor_get_button_box (LIGMA_EDITOR (layer_view)),
                         layer_view->priv->link_button, 8);

  layer_view->priv->link_popover = gtk_popover_new (layer_view->priv->link_button);
  gtk_popover_set_modal (GTK_POPOVER (layer_view->priv->link_popover), TRUE);
  gtk_menu_button_set_popover (GTK_MENU_BUTTON (layer_view->priv->link_button),
                               layer_view->priv->link_popover);

  g_signal_connect (layer_view->priv->link_popover,
                    "show",
                    G_CALLBACK (ligma_layer_tree_view_link_popover_shown),
                    layer_view);

  grid = gtk_grid_new ();

  /* Link popover: regexp search. */
  layer_view->priv->link_search_entry = gtk_entry_new ();
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (layer_view->priv->link_search_entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     "system-search");
  gtk_grid_attach (GTK_GRID (grid),
                   layer_view->priv->link_search_entry,
                   0, 0, 2, 1);
  gtk_widget_show (layer_view->priv->link_search_entry);
  g_signal_connect (layer_view->priv->link_search_entry,
                    "key-release-event",
                    G_CALLBACK (ligma_layer_tree_view_search_key_release),
                    layer_view);

  /* Link popover: existing links. */
  layer_view->priv->link_list = gtk_list_box_new ();
  placeholder = gtk_label_new (_("No layer set stored"));
  attrs = pango_attr_list_new ();
  gtk_label_set_attributes (GTK_LABEL (placeholder),
                            attrs);
  pango_attr_list_insert (attrs, pango_attr_style_new (PANGO_STYLE_ITALIC));
  pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_ULTRALIGHT));
  pango_attr_list_unref (attrs);
  gtk_list_box_set_placeholder (GTK_LIST_BOX (layer_view->priv->link_list),
                                placeholder);
  gtk_widget_show (placeholder);
  gtk_grid_attach (GTK_GRID (grid),
                   layer_view->priv->link_list,
                   0, 1, 2, 1);
  gtk_widget_show (layer_view->priv->link_list);

  gtk_list_box_set_activate_on_single_click (GTK_LIST_BOX (layer_view->priv->link_list),
                                             TRUE);

  /* Link popover: new links. */

  layer_view->priv->link_entry = gtk_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (layer_view->priv->link_entry),
                                  _("New layer set's name"));
  gtk_grid_attach (GTK_GRID (grid),
                   layer_view->priv->link_entry,
                   0, 2, 1, 1);
  gtk_widget_show (layer_view->priv->link_entry);

  button = gtk_button_new_from_icon_name (LIGMA_ICON_LIST_ADD, button_size);
  gtk_grid_attach (GTK_GRID (grid),
                   button,
                   1, 2, 1, 1);
  g_signal_connect_swapped (button,
                            "clicked",
                            G_CALLBACK (ligma_layer_tree_view_new_link_clicked),
                            layer_view);
  gtk_widget_show (button);
  layer_view->priv->new_link_button = button;

  /* Enter on any entry activates the link creation then exits in case
   * of success.
   */
  g_signal_connect_swapped (layer_view->priv->link_entry,
                            "activate",
                            G_CALLBACK (ligma_layer_tree_view_new_link_exit),
                            layer_view);

  gtk_container_add (GTK_CONTAINER (layer_view->priv->link_popover), grid);
  gtk_widget_show (grid);

  /*  Lock alpha toggle  */

  ligma_item_tree_view_add_lock (LIGMA_ITEM_TREE_VIEW (tree_view),
                                LIGMA_ICON_LOCK_ALPHA,
                                (LigmaIsLockedFunc) ligma_layer_get_lock_alpha,
                                (LigmaCanLockFunc)  ligma_layer_can_lock_alpha,
                                (LigmaSetLockFunc)  ligma_layer_set_lock_alpha,
                                (LigmaUndoLockPush) ligma_image_undo_push_layer_lock_alpha,
                                "lock-alpha-changed",
                                LIGMA_UNDO_LAYER_LOCK_ALPHA,
                                LIGMA_UNDO_GROUP_LAYER_LOCK_ALPHA,
                                _("Lock alpha channel"),
                                _("Unlock alpha channel"),
                                _("Set Item Exclusive Alpha Channel lock"),
                                _("Lock alpha channel"),
                                LIGMA_HELP_LAYER_LOCK_ALPHA);
}

static void
ligma_layer_tree_view_finalize (GObject *object)
{
  LigmaLayerTreeView *layer_view = LIGMA_LAYER_TREE_VIEW (object);

  if (layer_view->priv->italic_attrs)
    {
      pango_attr_list_unref (layer_view->priv->italic_attrs);
      layer_view->priv->italic_attrs = NULL;
    }

  if (layer_view->priv->bold_attrs)
    {
      pango_attr_list_unref (layer_view->priv->bold_attrs);
      layer_view->priv->bold_attrs = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  LigmaWidget methods  */

static void
ligma_layer_tree_view_style_updated (GtkWidget *widget)
{
  LigmaLayerTreeView *view = LIGMA_LAYER_TREE_VIEW (widget);
  GtkWidget         *image;
  const gchar       *old_icon_name;
  GtkReliefStyle     button_relief;
  GtkIconSize        old_size;
  GtkIconSize        button_size;

  gtk_widget_style_get (widget,
                        "button-relief",    &button_relief,
                        "button-icon-size", &button_size,
                        NULL);

  gtk_button_set_relief (GTK_BUTTON (view->priv->link_button),
                         button_relief);

  image = gtk_button_get_image (GTK_BUTTON (view->priv->link_button));

  gtk_image_get_icon_name (GTK_IMAGE (image), &old_icon_name, &old_size);

  if (button_size != old_size)
    {
      gchar *icon_name;

      /* Changing the link button in dockable button box. */
      icon_name = g_strdup (old_icon_name);
      gtk_image_set_from_icon_name (GTK_IMAGE (image),
                                    icon_name, button_size);
      g_free (icon_name);

      /* Changing the new link button inside the popover. */
      image = gtk_button_get_image (GTK_BUTTON (view->priv->new_link_button));
      gtk_image_get_icon_name (GTK_IMAGE (image), &old_icon_name, &old_size);
      icon_name = g_strdup (old_icon_name);
      gtk_image_set_from_icon_name (GTK_IMAGE (image),
                                    icon_name, button_size);
      g_free (icon_name);
    }
}

/*  LigmaContainerView methods  */

static void
ligma_layer_tree_view_set_container (LigmaContainerView *view,
                                    LigmaContainer     *container)
{
  LigmaLayerTreeView *layer_view = LIGMA_LAYER_TREE_VIEW (view);
  LigmaContainer     *old_container;

  old_container = ligma_container_view_get_container (view);

  if (old_container)
    {
      ligma_tree_handler_disconnect (layer_view->priv->mode_changed_handler);
      layer_view->priv->mode_changed_handler = NULL;

      ligma_tree_handler_disconnect (layer_view->priv->opacity_changed_handler);
      layer_view->priv->opacity_changed_handler = NULL;

      ligma_tree_handler_disconnect (layer_view->priv->mask_changed_handler);
      layer_view->priv->mask_changed_handler = NULL;

      ligma_tree_handler_disconnect (layer_view->priv->alpha_changed_handler);
      layer_view->priv->alpha_changed_handler = NULL;
    }

  parent_view_iface->set_container (view, container);

  if (container)
    {
      layer_view->priv->mode_changed_handler =
        ligma_tree_handler_connect (container, "mode-changed",
                                   G_CALLBACK (ligma_layer_tree_view_layer_signal_handler),
                                   view);

      layer_view->priv->opacity_changed_handler =
        ligma_tree_handler_connect (container, "opacity-changed",
                                   G_CALLBACK (ligma_layer_tree_view_layer_signal_handler),
                                   view);

      layer_view->priv->mask_changed_handler =
        ligma_tree_handler_connect (container, "mask-changed",
                                   G_CALLBACK (ligma_layer_tree_view_mask_changed),
                                   view);

      layer_view->priv->alpha_changed_handler =
        ligma_tree_handler_connect (container, "alpha-changed",
                                   G_CALLBACK (ligma_layer_tree_view_alpha_changed),
                                   view);
    }
}

typedef struct
{
  gint         mask_column;
  LigmaContext *context;
} SetContextForeachData;

static gboolean
ligma_layer_tree_view_set_context_foreach (GtkTreeModel *model,
                                          GtkTreePath  *path,
                                          GtkTreeIter  *iter,
                                          gpointer      data)
{
  SetContextForeachData *context_data = data;
  LigmaViewRenderer      *renderer;

  gtk_tree_model_get (model, iter,
                      context_data->mask_column, &renderer,
                      -1);

  if (renderer)
    {
      ligma_view_renderer_set_context (renderer, context_data->context);

      g_object_unref (renderer);
    }

  return FALSE;
}

static void
ligma_layer_tree_view_set_context (LigmaContainerView *view,
                                  LigmaContext       *context)
{
  LigmaContainerTreeView *tree_view  = LIGMA_CONTAINER_TREE_VIEW (view);
  LigmaLayerTreeView     *layer_view = LIGMA_LAYER_TREE_VIEW (view);
  LigmaContext           *old_context;

  old_context = ligma_container_view_get_context (view);

  if (old_context)
    {
      g_signal_handlers_disconnect_by_func (old_context->ligma->config,
                                            G_CALLBACK (ligma_layer_tree_view_style_updated),
                                            view);
    }

  parent_view_iface->set_context (view, context);

  if (context)
    {
      g_signal_connect_object (context->ligma->config,
                               "notify::theme",
                               G_CALLBACK (ligma_layer_tree_view_style_updated),
                               view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (context->ligma->config,
                               "notify::override-theme-icon-size",
                               G_CALLBACK (ligma_layer_tree_view_style_updated),
                               view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (context->ligma->config,
                               "notify::custom-icon-size",
                               G_CALLBACK (ligma_layer_tree_view_style_updated),
                               view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
    }

  if (tree_view->model)
    {
      SetContextForeachData context_data = { layer_view->priv->model_column_mask,
                                             context };

      gtk_tree_model_foreach (tree_view->model,
                              ligma_layer_tree_view_set_context_foreach,
                              &context_data);
    }
}

static gpointer
ligma_layer_tree_view_insert_item (LigmaContainerView *view,
                                  LigmaViewable      *viewable,
                                  gpointer           parent_insert_data,
                                  gint               index)
{
  LigmaLayerTreeView *layer_view = LIGMA_LAYER_TREE_VIEW (view);
  LigmaLayer         *layer;
  GtkTreeIter       *iter;

  iter = parent_view_iface->insert_item (view, viewable,
                                         parent_insert_data, index);

  layer = LIGMA_LAYER (viewable);

  if (! ligma_drawable_has_alpha (LIGMA_DRAWABLE (layer)))
    ligma_layer_tree_view_alpha_update (layer_view, iter, layer);

  ligma_layer_tree_view_mask_update (layer_view, iter, layer);

  if (LIGMA_IS_LAYER (viewable) && ligma_layer_is_floating_sel (LIGMA_LAYER (viewable)))
    {
      LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
      LigmaLayer             *floating  = LIGMA_LAYER (viewable);
      LigmaDrawable          *drawable  = ligma_layer_get_floating_sel_drawable (floating);

      if (LIGMA_IS_LAYER_MASK (drawable))
        {
          /* Display floating mask in the mask column. */
          LigmaViewRenderer *renderer  = NULL;

          gtk_tree_model_get (GTK_TREE_MODEL (tree_view->model), iter,
                              LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                              -1);
          if (renderer)
            {
              gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                                  layer_view->priv->model_column_mask,         renderer,
                                  layer_view->priv->model_column_mask_visible, TRUE,
                                  LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER,   NULL,
                                  -1);
              g_object_unref (renderer);
            }
        }
    }

  return iter;
}

static gboolean
ligma_layer_tree_view_select_items (LigmaContainerView *view,
                                   GList             *items,
                                   GList             *paths)
{
  LigmaContainerTreeView *tree_view  = LIGMA_CONTAINER_TREE_VIEW (view);
  LigmaLayerTreeView *layer_view = LIGMA_LAYER_TREE_VIEW (view);
  GList             *layers = items;
  GList             *path   = paths;
  gboolean           success;

  success = parent_view_iface->select_items (view, items, paths);

  if (layers)
    {
      if (success)
        {
          for (layers = items, path = paths; layers && path; layers = layers->next, path = path->next)
            {
              GtkTreeIter iter;

              gtk_tree_model_get_iter (tree_view->model, &iter, path->data);
              ligma_layer_tree_view_update_borders (layer_view, &iter);
            }
          ligma_layer_tree_view_update_options (layer_view, items);
          ligma_layer_tree_view_update_menu (layer_view, items);
        }
    }

  if (! success)
    {
      LigmaEditor *editor = LIGMA_EDITOR (view);

      /* currently, select_items() only ever fails when there is a floating
       * selection, which can be committed/canceled through the editor buttons.
       */
      ligma_widget_blink (GTK_WIDGET (ligma_editor_get_button_box (editor)));
    }

  return success;
}

typedef struct
{
  gint mask_column;
  gint view_size;
  gint border_width;
} SetSizeForeachData;

static gboolean
ligma_layer_tree_view_set_view_size_foreach (GtkTreeModel *model,
                                            GtkTreePath  *path,
                                            GtkTreeIter  *iter,
                                            gpointer      data)
{
  SetSizeForeachData *size_data = data;
  LigmaViewRenderer   *renderer;

  gtk_tree_model_get (model, iter,
                      size_data->mask_column, &renderer,
                      -1);

  if (renderer)
    {
      ligma_view_renderer_set_size (renderer,
                                   size_data->view_size,
                                   size_data->border_width);

      g_object_unref (renderer);
    }

  return FALSE;
}

static void
ligma_layer_tree_view_set_view_size (LigmaContainerView *view)
{
  LigmaContainerTreeView *tree_view  = LIGMA_CONTAINER_TREE_VIEW (view);

  if (tree_view->model)
    {
      LigmaLayerTreeView *layer_view = LIGMA_LAYER_TREE_VIEW (view);
      SetSizeForeachData size_data;

      size_data.mask_column = layer_view->priv->model_column_mask;

      size_data.view_size =
        ligma_container_view_get_view_size (view, &size_data.border_width);

      gtk_tree_model_foreach (tree_view->model,
                              ligma_layer_tree_view_set_view_size_foreach,
                              &size_data);
    }

  parent_view_iface->set_view_size (view);
}


/*  LigmaContainerTreeView methods  */

static gboolean
ligma_layer_tree_view_drop_possible (LigmaContainerTreeView   *tree_view,
                                    LigmaDndType              src_type,
                                    GList                   *src_viewables,
                                    LigmaViewable            *dest_viewable,
                                    GtkTreePath             *drop_path,
                                    GtkTreeViewDropPosition  drop_pos,
                                    GtkTreeViewDropPosition *return_drop_pos,
                                    GdkDragAction           *return_drag_action)
{
  /* If we are dropping a new layer, check if the destination image
   * has a floating selection.
   */
  if  (src_type == LIGMA_DND_TYPE_URI_LIST     ||
       src_type == LIGMA_DND_TYPE_TEXT_PLAIN   ||
       src_type == LIGMA_DND_TYPE_NETSCAPE_URL ||
       src_type == LIGMA_DND_TYPE_COMPONENT    ||
       src_type == LIGMA_DND_TYPE_PIXBUF       ||
       g_list_length (src_viewables) > 0)
    {
      LigmaImage *dest_image = ligma_item_tree_view_get_image (LIGMA_ITEM_TREE_VIEW (tree_view));

      if (ligma_image_get_floating_selection (dest_image))
        return FALSE;
    }

  return LIGMA_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_possible (tree_view,
                                                                       src_type,
                                                                       src_viewables,
                                                                       dest_viewable,
                                                                       drop_path,
                                                                       drop_pos,
                                                                       return_drop_pos,
                                                                       return_drag_action);
}

static void
ligma_layer_tree_view_drop_color (LigmaContainerTreeView   *view,
                                 const LigmaRGB           *color,
                                 LigmaViewable            *dest_viewable,
                                 GtkTreeViewDropPosition  drop_pos)
{
  if (ligma_item_is_text_layer (LIGMA_ITEM (dest_viewable)))
    {
      ligma_text_layer_set (LIGMA_TEXT_LAYER (dest_viewable), NULL,
                           "color", color,
                           NULL);
      ligma_image_flush (ligma_item_tree_view_get_image (LIGMA_ITEM_TREE_VIEW (view)));
      return;
    }

  LIGMA_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_color (view, color,
                                                             dest_viewable,
                                                             drop_pos);
}

static void
ligma_layer_tree_view_drop_uri_list (LigmaContainerTreeView   *view,
                                    GList                   *uri_list,
                                    LigmaViewable            *dest_viewable,
                                    GtkTreeViewDropPosition  drop_pos)
{
  LigmaItemTreeView  *item_view = LIGMA_ITEM_TREE_VIEW (view);
  LigmaContainerView *cont_view = LIGMA_CONTAINER_VIEW (view);
  LigmaImage         *image     = ligma_item_tree_view_get_image (item_view);
  LigmaLayer         *parent;
  gint               index;
  GList             *list;

  index = ligma_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (LigmaViewable **) &parent);

  g_object_ref (image);

  for (list = uri_list; list; list = g_list_next (list))
    {
      const gchar       *uri   = list->data;
      GFile             *file  = g_file_new_for_uri (uri);
      GList             *new_layers;
      LigmaPDBStatusType  status;
      GError            *error = NULL;

      new_layers = file_open_layers (image->ligma,
                                     ligma_container_view_get_context (cont_view),
                                     NULL,
                                     image, FALSE,
                                     file, LIGMA_RUN_INTERACTIVE, NULL,
                                     &status, &error);

      if (new_layers)
        {
          ligma_image_add_layers (image, new_layers, parent, index,
                                 0, 0,
                                 ligma_image_get_width (image),
                                 ligma_image_get_height (image),
                                 _("Drop layers"));

          index += g_list_length (new_layers);

          g_list_free (new_layers);
        }
      else if (status != LIGMA_PDB_CANCEL)
        {
          ligma_message (image->ligma, G_OBJECT (view), LIGMA_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        ligma_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }

      g_object_unref (file);
    }

  ligma_image_flush (image);

  g_object_unref (image);
}

static void
ligma_layer_tree_view_drop_component (LigmaContainerTreeView   *tree_view,
                                     LigmaImage               *src_image,
                                     LigmaChannelType          component,
                                     LigmaViewable            *dest_viewable,
                                     GtkTreeViewDropPosition  drop_pos)
{
  LigmaItemTreeView *item_view = LIGMA_ITEM_TREE_VIEW (tree_view);
  LigmaImage        *image     = ligma_item_tree_view_get_image (item_view);
  LigmaChannel      *channel;
  LigmaItem         *new_item;
  LigmaLayer        *parent;
  gint              index;
  const gchar      *desc;

  index = ligma_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (LigmaViewable **) &parent);

  channel = ligma_channel_new_from_component (src_image, component, NULL, NULL);

  new_item = ligma_item_convert (LIGMA_ITEM (channel), image,
                                LIGMA_TYPE_LAYER);

  g_object_unref (channel);

  ligma_enum_get_value (LIGMA_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);
  ligma_object_take_name (LIGMA_OBJECT (new_item),
                         g_strdup_printf (_("%s Channel Copy"), desc));

  ligma_image_add_layer (image, LIGMA_LAYER (new_item), parent, index, TRUE);

  ligma_image_flush (image);
}

static void
ligma_layer_tree_view_drop_pixbuf (LigmaContainerTreeView   *tree_view,
                                  GdkPixbuf               *pixbuf,
                                  LigmaViewable            *dest_viewable,
                                  GtkTreeViewDropPosition  drop_pos)
{
  LigmaItemTreeView *item_view = LIGMA_ITEM_TREE_VIEW (tree_view);
  LigmaImage        *image     = ligma_item_tree_view_get_image (item_view);
  LigmaLayer        *new_layer;
  LigmaLayer        *parent;
  gint              index;

  index = ligma_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (LigmaViewable **) &parent);

  new_layer =
    ligma_layer_new_from_pixbuf (pixbuf, image,
                                ligma_image_get_layer_format (image, TRUE),
                                _("Dropped Buffer"),
                                LIGMA_OPACITY_OPAQUE,
                                ligma_image_get_default_new_layer_mode (image));

  ligma_image_add_layer (image, new_layer, parent, index, TRUE);

  ligma_image_flush (image);
}


/*  LigmaItemTreeView methods  */

static void
ligma_layer_tree_view_set_image (LigmaItemTreeView *view,
                                LigmaImage        *image)
{
  LigmaLayerTreeView *layer_view = LIGMA_LAYER_TREE_VIEW (view);

  if (ligma_item_tree_view_get_image (view))
    {
      g_signal_handlers_disconnect_by_func (ligma_item_tree_view_get_image (view),
                                            ligma_layer_tree_view_floating_selection_changed,
                                            view);
      g_signal_handlers_disconnect_by_func (ligma_item_tree_view_get_image (view),
                                            G_CALLBACK (ligma_layer_tree_view_layer_links_changed),
                                            view);
    }

  LIGMA_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (view, image);

  if (ligma_item_tree_view_get_image (view))
    {
      g_signal_connect (ligma_item_tree_view_get_image (view),
                        "floating-selection-changed",
                        G_CALLBACK (ligma_layer_tree_view_floating_selection_changed),
                        view);
      g_signal_connect (ligma_item_tree_view_get_image (view),
                        "layer-sets-changed",
                        G_CALLBACK (ligma_layer_tree_view_layer_links_changed),
                        view);

      /* call ligma_layer_tree_view_floating_selection_changed() now, to update
       * the floating selection's row attributes.
       */
      ligma_layer_tree_view_floating_selection_changed (
        ligma_item_tree_view_get_image (view),
        layer_view);
    }
  /* Call this even with no image, allowing to empty the link list. */
  ligma_layer_tree_view_layer_links_changed (ligma_item_tree_view_get_image (view),
                                            layer_view);

  ligma_layer_tree_view_update_highlight (layer_view);
}

static LigmaItem *
ligma_layer_tree_view_item_new (LigmaImage *image)
{
  LigmaLayer *new_layer;

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_EDIT_PASTE,
                               _("New Layer"));

  new_layer = ligma_layer_new (image,
                              ligma_image_get_width (image),
                              ligma_image_get_height (image),
                              ligma_image_get_layer_format (image, TRUE),
                              NULL,
                              LIGMA_OPACITY_OPAQUE,
                              ligma_image_get_default_new_layer_mode (image));

  ligma_image_add_layer (image, new_layer,
                        LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);

  ligma_image_undo_group_end (image);

  return LIGMA_ITEM (new_layer);
}


/*  callbacks  */

static void
ligma_layer_tree_view_floating_selection_changed (LigmaImage         *image,
                                                 LigmaLayerTreeView *layer_view)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (layer_view);
  LigmaContainerView     *view      = LIGMA_CONTAINER_VIEW (layer_view);
  LigmaLayer             *floating_sel;
  GtkTreeIter           *iter;

  floating_sel = ligma_image_get_floating_selection (image);

  if (floating_sel)
    {
      iter = ligma_container_view_lookup (view, (LigmaViewable *) floating_sel);

      if (iter)
        gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                            LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES,
                            layer_view->priv->italic_attrs,
                            -1);
    }
  else
    {
      GList *all_layers;
      GList *list;

      all_layers = ligma_image_get_layer_list (image);

      for (list = all_layers; list; list = g_list_next (list))
        {
          LigmaDrawable *drawable = list->data;

          iter = ligma_container_view_lookup (view, (LigmaViewable *) drawable);

          if (iter)
            gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                                LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES,
                                ligma_drawable_has_alpha (drawable) ?
                                NULL : layer_view->priv->bold_attrs,
                                -1);
        }

      g_list_free (all_layers);
    }

  ligma_layer_tree_view_update_highlight (layer_view);
}

static void
ligma_layer_tree_view_layer_links_changed (LigmaImage         *image,
                                          LigmaLayerTreeView *view)
{
  GtkWidget    *grid;
  GtkWidget    *label;
  GtkWidget    *event_box;
  GtkWidget    *icon;
  GtkSizeGroup *label_size;
  GList        *links;
  GList        *iter;

  gtk_container_foreach (GTK_CONTAINER (view->priv->link_list),
                         (GtkCallback) gtk_widget_destroy, NULL);
  gtk_widget_set_sensitive (view->priv->link_button, image != NULL);

  if (! image)
    return;

  links = ligma_image_get_stored_item_sets (image, LIGMA_TYPE_LAYER);

  label_size = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
  for (iter = links; iter; iter = iter->next)
    {
      LigmaSelectMethod method;

      grid = gtk_grid_new ();

      label = gtk_label_new (ligma_object_get_name (iter->data));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      if (ligma_item_list_is_pattern (iter->data, &method))
        {
          gchar         *display_name;
          PangoAttrList *attrs;

          display_name = g_strdup_printf ("<small>[%s]</small> %s",
                                          method == LIGMA_SELECT_PLAIN_TEXT ? _("search") :
                                          (method == LIGMA_SELECT_GLOB_PATTERN ? _("glob") : _("regexp")),
                                          ligma_object_get_name (iter->data));
          gtk_label_set_markup (GTK_LABEL (label), display_name);
          g_free (display_name);

          attrs = pango_attr_list_new ();
          pango_attr_list_insert (attrs, pango_attr_style_new (PANGO_STYLE_OBLIQUE));
          gtk_label_set_attributes (GTK_LABEL (label), attrs);
          pango_attr_list_unref (attrs);
        }
      gtk_widget_set_hexpand (GTK_WIDGET (label), TRUE);
      gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
      gtk_size_group_add_widget (label_size, label);
      gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
      gtk_widget_show (label);

      /* I don't use a GtkButton because the minimum size is 16 which is
       * weird and ugly here. And somehow if I force smaller GtkImage
       * size then add it to the GtkButton, I still get a giant button
       * with a small image in it, which is even worse. XXX
       */
      event_box = gtk_event_box_new ();
      gtk_event_box_set_above_child (GTK_EVENT_BOX (event_box), TRUE);
      gtk_widget_add_events (event_box, GDK_BUTTON_RELEASE_MASK);
      g_object_set_data (G_OBJECT (event_box), "link-set", iter->data);
      g_signal_connect (event_box, "button-release-event",
                        G_CALLBACK (ligma_layer_tree_view_unlink_clicked),
                        view);
      gtk_grid_attach (GTK_GRID (grid), event_box, 2, 0, 1, 1);
      gtk_widget_show (event_box);

      icon = gtk_image_new_from_icon_name (LIGMA_ICON_EDIT_DELETE, GTK_ICON_SIZE_MENU);
      gtk_image_set_pixel_size (GTK_IMAGE (icon), 10);
      gtk_container_add (GTK_CONTAINER (event_box), icon);
      gtk_widget_show (icon);

      /* Now using again an event box on the whole grid, but behind its
       * child (so that the delete button is processed first. I do it
       * this way instead of using the "row-activated" of GtkListBox
       * because this signal does not give us event info, and in
       * particular modifier state. Yet I want to be able to process
       * Shift/Ctrl state for logical operations on layer sets.
       */
      event_box = gtk_event_box_new ();
      gtk_event_box_set_above_child (GTK_EVENT_BOX (event_box), FALSE);
      gtk_widget_add_events (event_box, GDK_BUTTON_RELEASE_MASK);
      g_object_set_data (G_OBJECT (event_box), "link-set", iter->data);
      gtk_container_add (GTK_CONTAINER (event_box), grid);
      gtk_list_box_prepend (GTK_LIST_BOX (view->priv->link_list), event_box);
      gtk_widget_show (event_box);

      g_signal_connect (event_box,
                        "button-release-event",
                        G_CALLBACK (ligma_layer_tree_view_link_clicked),
                        view);

      gtk_widget_show (grid);
    }
  g_object_unref (label_size);
  gtk_list_box_unselect_all (GTK_LIST_BOX (view->priv->link_list));
}

static gboolean
ligma_layer_tree_view_link_clicked (GtkWidget         *box,
                                   GdkEvent          *event,
                                   LigmaLayerTreeView *view)
{
  LigmaImage       *image;
  GdkEventButton  *bevent = (GdkEventButton *) event;
  GdkModifierType  modifiers;

  image = ligma_item_tree_view_get_image (LIGMA_ITEM_TREE_VIEW (view));

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GTK_IS_EVENT_BOX (box), FALSE);

  modifiers = bevent->state & ligma_get_all_modifiers_mask ();
  if (modifiers == GDK_SHIFT_MASK)
    ligma_image_add_item_set (image, g_object_get_data (G_OBJECT (box), "link-set"));
  else if (modifiers == GDK_CONTROL_MASK)
    ligma_image_remove_item_set (image, g_object_get_data (G_OBJECT (box), "link-set"));
  else if (modifiers == (GDK_CONTROL_MASK | GDK_SHIFT_MASK))
    ligma_image_intersect_item_set (image, g_object_get_data (G_OBJECT (box), "link-set"));
  else
    ligma_image_select_item_set (image, g_object_get_data (G_OBJECT (box), "link-set"));

  gtk_entry_set_text (GTK_ENTRY (view->priv->link_search_entry), "");
  /* TODO: if clicking on pattern link, fill in the pattern field? */

  return FALSE;
}

static void
ligma_layer_tree_view_link_popover_shown (GtkPopover    *popover,
                                         LigmaLayerTreeView *view)
{
  LigmaImage        *image;
  LigmaSelectMethod  pattern_syntax;

  image = ligma_item_tree_view_get_image (LIGMA_ITEM_TREE_VIEW (view));

  if (! image)
    {
      gtk_widget_set_tooltip_text (view->priv->link_search_entry,
                                   _("Select layers by text search"));
      gtk_entry_set_placeholder_text (GTK_ENTRY (view->priv->link_search_entry),
                                      _("Text search"));
      return;
    }

  g_object_get (image->ligma->config,
                "items-select-method", &pattern_syntax,
                NULL);
  switch (pattern_syntax)
    {
    case LIGMA_SELECT_PLAIN_TEXT:
      gtk_widget_set_tooltip_text (view->priv->link_search_entry,
                                   _("Select layers by text search"));
      gtk_entry_set_placeholder_text (GTK_ENTRY (view->priv->link_search_entry),
                                      _("Text search"));
      break;
    case LIGMA_SELECT_GLOB_PATTERN:
      gtk_widget_set_tooltip_text (view->priv->link_search_entry,
                                   _("Select layers by glob patterns"));
      gtk_entry_set_placeholder_text (GTK_ENTRY (view->priv->link_search_entry),
                                      _("Glob pattern search"));
      break;
    case LIGMA_SELECT_REGEX_PATTERN:
      gtk_widget_set_tooltip_text (view->priv->link_search_entry,
                                   _("Select layers by regular expressions"));
      gtk_entry_set_placeholder_text (GTK_ENTRY (view->priv->link_search_entry),
                                      _("Regular Expression search"));
      break;
    }
}

static gboolean
ligma_layer_tree_view_search_key_release (GtkWidget         *widget,
                                         GdkEventKey       *event,
                                         LigmaLayerTreeView *view)
{
  LigmaImage        *image;
  const gchar      *pattern;
  LigmaSelectMethod  pattern_syntax;

  if (event->keyval == GDK_KEY_Escape   ||
      event->keyval == GDK_KEY_Return   ||
      event->keyval == GDK_KEY_KP_Enter ||
      event->keyval == GDK_KEY_ISO_Enter)
    {
      if (event->state & GDK_SHIFT_MASK)
        {
          if (ligma_layer_tree_view_new_link_clicked (view))
            gtk_widget_hide (view->priv->link_popover);
        }
      else
        {
          gtk_widget_hide (view->priv->link_popover);
        }
      return TRUE;
    }

  gtk_entry_set_attributes (GTK_ENTRY (view->priv->link_search_entry),
                            NULL);

  image = ligma_item_tree_view_get_image (LIGMA_ITEM_TREE_VIEW (view));
  g_clear_object (&view->priv->link_pattern_set);

  if (! image)
    return TRUE;

  g_object_get (image->ligma->config,
                "items-select-method", &pattern_syntax,
                NULL);
  pattern = gtk_entry_get_text (GTK_ENTRY (view->priv->link_search_entry));
  if (pattern && strlen (pattern) > 0)
    {
      GList  *items;
      GError *error = NULL;

      gtk_entry_set_text (GTK_ENTRY (view->priv->link_entry), "");
      gtk_widget_set_sensitive (view->priv->link_entry, FALSE);

      view->priv->link_pattern_set = ligma_item_list_pattern_new (image,
                                                                 LIGMA_TYPE_LAYER,
                                                                 pattern_syntax,
                                                                 pattern);
      items = ligma_item_list_get_items (view->priv->link_pattern_set, &error);
      if (error)
        {
          /* Invalid regular expression. */
          PangoAttrList *attrs = pango_attr_list_new ();
          gchar         *tooltip;

          pango_attr_list_insert (attrs, pango_attr_strikethrough_new (TRUE));
          tooltip = g_strdup_printf (_("Invalid regular expression: %s\n"),
                                     error->message);
          gtk_widget_set_tooltip_text (view->priv->link_search_entry,
                                       tooltip);
          ligma_image_set_selected_layers (image, NULL);
          gtk_entry_set_attributes (GTK_ENTRY (view->priv->link_search_entry),
                                    attrs);
          g_free (tooltip);
          g_error_free (error);
          pango_attr_list_unref (attrs);

          g_clear_object (&view->priv->link_pattern_set);
        }
      else if (items == NULL)
        {
          /* Pattern does not match any results. */
          ligma_image_set_selected_layers (image, NULL);
          ligma_widget_blink (view->priv->link_search_entry);
        }
      else
        {
          ligma_image_set_selected_layers (image, items);
          g_list_free (items);
        }
    }
  else
    {
      gtk_widget_set_sensitive (view->priv->link_entry, TRUE);
    }

  return TRUE;
}

static void
ligma_layer_tree_view_new_link_exit (LigmaLayerTreeView *view)
{
  if (ligma_layer_tree_view_new_link_clicked (view))
    gtk_widget_hide (view->priv->link_popover);
}

static gboolean
ligma_layer_tree_view_new_link_clicked (LigmaLayerTreeView *view)
{
  LigmaImage   *image;
  const gchar *name;

  image = ligma_item_tree_view_get_image (LIGMA_ITEM_TREE_VIEW (view));

  if (! image)
    return TRUE;

  name = gtk_entry_get_text (GTK_ENTRY (view->priv->link_entry));
  if (name && strlen (name) > 0)
    {
      LigmaItemList *set;

      set = ligma_item_list_named_new (image, LIGMA_TYPE_LAYER, name, NULL);
      if (set)
        {
          ligma_image_store_item_set (image, set);
          gtk_entry_set_text (GTK_ENTRY (view->priv->link_entry), "");
        }
      else
        {
          /* No existing selection. */
          return FALSE;
        }
    }
  else if (view->priv->link_pattern_set != NULL)
    {
      ligma_image_store_item_set (image, view->priv->link_pattern_set);
      view->priv->link_pattern_set = NULL;
      gtk_entry_set_text (GTK_ENTRY (view->priv->link_search_entry), "");
    }
  else
    {
      ligma_widget_blink (view->priv->link_entry);
      ligma_widget_blink (view->priv->link_search_entry);

      return FALSE;
    }

  return TRUE;
}

static gboolean
ligma_layer_tree_view_unlink_clicked (GtkWidget         *widget,
                                     GdkEvent          *event,
                                     LigmaLayerTreeView *view)
{
  LigmaImage *image;

  image = ligma_item_tree_view_get_image (LIGMA_ITEM_TREE_VIEW (view));

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  ligma_image_unlink_item_set (image,
                              g_object_get_data (G_OBJECT (widget),
                                                 "link-set"));

  return TRUE;
}


/*  Paint Mode, Opacity and Lock alpha callbacks  */

#define BLOCK() \
        g_signal_handlers_block_by_func (layer, \
        ligma_layer_tree_view_layer_signal_handler, view)

#define UNBLOCK() \
        g_signal_handlers_unblock_by_func (layer, \
        ligma_layer_tree_view_layer_signal_handler, view)


static void
ligma_layer_tree_view_layer_mode_box_callback (GtkWidget         *widget,
                                              const GParamSpec  *pspec,
                                              LigmaLayerTreeView *view)
{
  LigmaImage     *image;
  GList         *layers    = NULL;
  GList         *iter;
  LigmaUndo      *undo;
  gboolean       push_undo = TRUE;
  gint           n_layers  = 0;
  LigmaLayerMode  mode;

  image = ligma_item_tree_view_get_image (LIGMA_ITEM_TREE_VIEW (view));
  mode  = ligma_layer_mode_box_get_mode (LIGMA_LAYER_MODE_BOX (widget));
  undo  = ligma_image_undo_can_compress (image, LIGMA_TYPE_ITEM_UNDO,
                                        LIGMA_UNDO_LAYER_MODE);

  if (image)
    layers = LIGMA_ITEM_TREE_VIEW_GET_CLASS (view)->get_selected_items (image);

  for (iter = layers; iter; iter = iter->next)
    {
      if (ligma_layer_get_mode (iter->data) != mode)
        {
          n_layers++;

          if (undo && LIGMA_ITEM_UNDO (undo)->item == LIGMA_ITEM (iter->data))
            push_undo = FALSE;
        }
    }

  if (n_layers > 1)
    {
      /*  Don't compress mode undos with more than 1 layer changed. */
      push_undo = TRUE;

      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_LAYER_OPACITY,
                                   _("Set layers mode"));
    }

  for (iter = layers; iter; iter = iter->next)
    {
      LigmaLayer *layer = iter->data;

      if (ligma_layer_get_mode (layer) != mode)
        {
          BLOCK();
          ligma_layer_set_mode (layer, (LigmaLayerMode) mode, push_undo);
          UNBLOCK();
        }
    }
  ligma_image_flush (image);

  if (! push_undo)
    ligma_undo_refresh_preview (undo, ligma_container_view_get_context (LIGMA_CONTAINER_VIEW (view)));

  if (n_layers > 1)
    ligma_image_undo_group_end (image);
}

static void
ligma_layer_tree_view_opacity_scale_changed (GtkAdjustment     *adjustment,
                                            LigmaLayerTreeView *view)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  LigmaUndo  *undo;
  gboolean   push_undo = TRUE;
  gint       n_layers = 0;
  gdouble    opacity;

  image   = ligma_item_tree_view_get_image (LIGMA_ITEM_TREE_VIEW (view));
  layers  = LIGMA_ITEM_TREE_VIEW_GET_CLASS (view)->get_selected_items (image);
  undo    = ligma_image_undo_can_compress (image, LIGMA_TYPE_ITEM_UNDO,
                                          LIGMA_UNDO_LAYER_OPACITY);
  opacity = gtk_adjustment_get_value (adjustment) / 100.0;

  for (iter = layers; iter; iter = iter->next)
    {
      if (ligma_layer_get_opacity (iter->data) != opacity)
        {
          n_layers++;

          if (undo && LIGMA_ITEM_UNDO (undo)->item == LIGMA_ITEM (iter->data))
            push_undo = FALSE;
        }
    }
  if (n_layers > 1)
    {
      /*  Don't compress opacity undos with more than 1 layer changed. */
      push_undo = TRUE;

      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_LAYER_OPACITY,
                                   _("Set layers opacity"));
    }

  for (iter = layers; iter; iter = iter->next)
    {
      LigmaLayer *layer = iter->data;

      if (ligma_layer_get_opacity (layer) != opacity)
        {
          BLOCK();
          ligma_layer_set_opacity (layer, opacity, push_undo);
          UNBLOCK();
        }
    }
  ligma_image_flush (image);

  if (! push_undo)
    ligma_undo_refresh_preview (undo, ligma_container_view_get_context (LIGMA_CONTAINER_VIEW (view)));

  if (n_layers > 1)
    ligma_image_undo_group_end (image);
}

#undef BLOCK
#undef UNBLOCK


static void
ligma_layer_tree_view_layer_signal_handler (LigmaLayer         *layer,
                                           LigmaLayerTreeView *view)
{
  LigmaItemTreeView *item_view = LIGMA_ITEM_TREE_VIEW (view);
  GList            *selected_layers;

  selected_layers =
    LIGMA_ITEM_TREE_VIEW_GET_CLASS (view)->get_selected_items (ligma_item_tree_view_get_image (item_view));

  if (g_list_find (selected_layers, layer))
    ligma_layer_tree_view_update_options (view, selected_layers);
}


#define BLOCK(object,function) \
        g_signal_handlers_block_by_func ((object), (function), view)

#define UNBLOCK(object,function) \
        g_signal_handlers_unblock_by_func ((object), (function), view)

static void
ligma_layer_tree_view_update_options (LigmaLayerTreeView *view,
                                     GList             *layers)
{
  GList                *iter;
  LigmaLayerMode         mode = LIGMA_LAYER_MODE_SEPARATOR;
  LigmaLayerModeContext  context = 0;
  /*gboolean              inconsistent_opacity    = FALSE;*/
  gboolean              inconsistent_mode       = FALSE;
  gdouble               opacity = -1.0;

  for (iter = layers; iter; iter = iter->next)
    {
#if 0
      if (opacity != -1.0 &&
          opacity != ligma_layer_get_opacity (iter->data))
        /* We don't really have a way to show an inconsistent
         * LigmaSpinScale. This is currently unused.
         */
        inconsistent_opacity = TRUE;
#endif

      opacity = ligma_layer_get_opacity (iter->data);

      if (ligma_viewable_get_children (iter->data))
        context |= LIGMA_LAYER_MODE_CONTEXT_GROUP;
      else
        context |= LIGMA_LAYER_MODE_CONTEXT_LAYER;

      if (mode != LIGMA_LAYER_MODE_SEPARATOR &&
          mode != ligma_layer_get_mode (iter->data))
        inconsistent_mode = TRUE;

      mode = ligma_layer_get_mode (iter->data);
    }
  if (opacity == -1.0)
    opacity = 1.0;

  if (inconsistent_mode)
    mode = LIGMA_LAYER_MODE_SEPARATOR;

  if (! context)
    context = LIGMA_LAYER_MODE_CONTEXT_LAYER;

  BLOCK (view->priv->layer_mode_box,
         ligma_layer_tree_view_layer_mode_box_callback);

  ligma_layer_mode_box_set_context (LIGMA_LAYER_MODE_BOX (view->priv->layer_mode_box),
                                   context);
  ligma_layer_mode_box_set_mode (LIGMA_LAYER_MODE_BOX (view->priv->layer_mode_box),
                                mode);

  UNBLOCK (view->priv->layer_mode_box,
           ligma_layer_tree_view_layer_mode_box_callback);

  if (opacity * 100.0 !=
      gtk_adjustment_get_value (view->priv->opacity_adjustment))
    {
      BLOCK (view->priv->opacity_adjustment,
             ligma_layer_tree_view_opacity_scale_changed);

      gtk_adjustment_set_value (view->priv->opacity_adjustment,
                                opacity * 100.0);

      UNBLOCK (view->priv->opacity_adjustment,
               ligma_layer_tree_view_opacity_scale_changed);
    }
}

#undef BLOCK
#undef UNBLOCK


static void
ligma_layer_tree_view_update_menu (LigmaLayerTreeView *layer_view,
                                  GList             *layers)
{
  LigmaUIManager   *ui_manager = ligma_editor_get_ui_manager (LIGMA_EDITOR (layer_view));
  LigmaActionGroup *group;
  GList           *iter;
  gboolean         have_masks         = FALSE;
  gboolean         all_masks_shown    = TRUE;
  gboolean         all_masks_disabled = TRUE;

  group = ligma_ui_manager_get_action_group (ui_manager, "layers");

  for (iter = layers; iter; iter = iter->next)
    {
      if (ligma_layer_get_mask (iter->data))
        {
          have_masks = TRUE;
          if (! ligma_layer_get_show_mask (iter->data))
            all_masks_shown = FALSE;
          if (ligma_layer_get_apply_mask (iter->data))
            all_masks_disabled = FALSE;
        }
    }

  ligma_action_group_set_action_active (group, "layers-mask-show",
                                       have_masks && all_masks_shown);
  ligma_action_group_set_action_active (group, "layers-mask-disable",
                                       have_masks && all_masks_disabled);

  /* Only one layer mask at a time can be edited. */
  ligma_action_group_set_action_active (group, "layers-mask-edit",
                                       g_list_length (layers) == 1 &&
                                       ligma_layer_get_mask (layers->data) &&
                                       ligma_layer_get_edit_mask (layers->data));
}

static void
ligma_layer_tree_view_update_highlight (LigmaLayerTreeView *layer_view)
{
  LigmaItemTreeView *item_view    = LIGMA_ITEM_TREE_VIEW (layer_view);
  LigmaImage        *image        = ligma_item_tree_view_get_image (item_view);
  LigmaLayer        *floating_sel = NULL;
  GtkReliefStyle    default_relief;

  if (image)
    floating_sel = ligma_image_get_floating_selection (image);

  gtk_widget_style_get (GTK_WIDGET (layer_view),
                        "button-relief", &default_relief,
                        NULL);

  ligma_button_set_suggested (ligma_item_tree_view_get_new_button (item_view),
                             floating_sel &&
                             ! LIGMA_IS_CHANNEL (ligma_layer_get_floating_sel_drawable (floating_sel)),
                             default_relief);

  ligma_button_set_destructive (ligma_item_tree_view_get_delete_button (item_view),
                               floating_sel != NULL,
                               default_relief);

  ligma_button_set_suggested (layer_view->priv->anchor_button,
                             floating_sel != NULL,
                             default_relief);
}


/*  Layer Mask callbacks  */

static void
ligma_layer_tree_view_mask_update (LigmaLayerTreeView *layer_view,
                                  GtkTreeIter       *iter,
                                  LigmaLayer         *layer)
{
  LigmaContainerView     *view         = LIGMA_CONTAINER_VIEW (layer_view);
  LigmaContainerTreeView *tree_view    = LIGMA_CONTAINER_TREE_VIEW (layer_view);
  LigmaLayerMask         *mask;
  LigmaViewRenderer      *renderer     = NULL;
  gboolean               mask_visible = FALSE;

  mask = ligma_layer_get_mask (layer);

  if (mask)
    {
      GClosure *closure;
      gint      view_size;
      gint      border_width;

      view_size = ligma_container_view_get_view_size (view, &border_width);

      mask_visible = TRUE;

      renderer = ligma_view_renderer_new (ligma_container_view_get_context (view),
                                         G_TYPE_FROM_INSTANCE (mask),
                                         view_size, border_width,
                                         FALSE);
      ligma_view_renderer_set_viewable (renderer, LIGMA_VIEWABLE (mask));

      g_signal_connect (renderer, "update",
                        G_CALLBACK (ligma_layer_tree_view_renderer_update),
                        layer_view);

      closure = g_cclosure_new (G_CALLBACK (ligma_layer_tree_view_mask_callback),
                                layer_view, NULL);
      g_object_watch_closure (G_OBJECT (renderer), closure);
      g_signal_connect_closure (layer, "apply-mask-changed", closure, FALSE);
      g_signal_connect_closure (layer, "edit-mask-changed",  closure, FALSE);
      g_signal_connect_closure (layer, "show-mask-changed",  closure, FALSE);
    }

  gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                      layer_view->priv->model_column_mask,         renderer,
                      layer_view->priv->model_column_mask_visible, mask_visible,
                      -1);

  ligma_layer_tree_view_update_borders (layer_view, iter);

  if (renderer)
    {
      ligma_view_renderer_remove_idle (renderer);
      g_object_unref (renderer);
    }
}

static void
ligma_layer_tree_view_mask_changed (LigmaLayer         *layer,
                                   LigmaLayerTreeView *layer_view)
{
  LigmaContainerView *view = LIGMA_CONTAINER_VIEW (layer_view);
  GtkTreeIter       *iter;

  iter = ligma_container_view_lookup (view, LIGMA_VIEWABLE (layer));

  if (iter)
    ligma_layer_tree_view_mask_update (layer_view, iter, layer);
}

static void
ligma_layer_tree_view_renderer_update (LigmaViewRenderer  *renderer,
                                      LigmaLayerTreeView *layer_view)
{
  LigmaContainerView     *view      = LIGMA_CONTAINER_VIEW (layer_view);
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (layer_view);
  LigmaLayerMask         *mask;
  GtkTreeIter           *iter;

  mask = LIGMA_LAYER_MASK (renderer->viewable);

  iter = ligma_container_view_lookup (view, (LigmaViewable *)
                                     ligma_layer_mask_get_layer (mask));

  if (iter)
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (tree_view->model, iter);

      gtk_tree_model_row_changed (tree_view->model, path, iter);

      gtk_tree_path_free (path);
    }
}

static void
ligma_layer_tree_view_update_borders (LigmaLayerTreeView *layer_view,
                                     GtkTreeIter       *iter)
{
  LigmaContainerTreeView *tree_view  = LIGMA_CONTAINER_TREE_VIEW (layer_view);
  LigmaViewRenderer      *layer_renderer;
  LigmaViewRenderer      *mask_renderer;
  LigmaLayer             *layer      = NULL;
  LigmaLayerMask         *mask       = NULL;
  LigmaViewBorderType     layer_type = LIGMA_VIEW_BORDER_BLACK;

  gtk_tree_model_get (tree_view->model, iter,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &layer_renderer,
                      layer_view->priv->model_column_mask,       &mask_renderer,
                      -1);

  if (layer_renderer)
    layer = LIGMA_LAYER (layer_renderer->viewable);
  else if (mask_renderer && LIGMA_IS_LAYER (mask_renderer->viewable))
    /* This happens when floating masks are displayed (in the mask column).
     * In such a case, the mask_renderer will actually be a layer renderer
     * showing the floating mask.
     */
    layer = LIGMA_LAYER (mask_renderer->viewable);

  g_return_if_fail (layer != NULL);

  if (mask_renderer && LIGMA_IS_LAYER_MASK (mask_renderer->viewable))
    mask = LIGMA_LAYER_MASK (mask_renderer->viewable);

  if (! mask || (mask && ! ligma_layer_get_edit_mask (layer)))
    layer_type = LIGMA_VIEW_BORDER_WHITE;

  if (layer_renderer)
    ligma_view_renderer_set_border_type (layer_renderer, layer_type);

  if (mask)
    {
      LigmaViewBorderType mask_color = LIGMA_VIEW_BORDER_BLACK;

      if (ligma_layer_get_show_mask (layer))
        {
          mask_color = LIGMA_VIEW_BORDER_GREEN;
        }
      else if (! ligma_layer_get_apply_mask (layer))
        {
          mask_color = LIGMA_VIEW_BORDER_RED;
        }
      else if (ligma_layer_get_edit_mask (layer))
        {
          mask_color = LIGMA_VIEW_BORDER_WHITE;
        }

      ligma_view_renderer_set_border_type (mask_renderer, mask_color);
    }
  else if (mask_renderer)
    {
      /* Floating mask. */
      ligma_view_renderer_set_border_type (mask_renderer, LIGMA_VIEW_BORDER_WHITE);
    }

  if (layer_renderer)
    g_object_unref (layer_renderer);

  if (mask_renderer)
    g_object_unref (mask_renderer);
}

static void
ligma_layer_tree_view_mask_callback (LigmaLayer         *layer,
                                    LigmaLayerTreeView *layer_view)
{
  LigmaContainerView *view = LIGMA_CONTAINER_VIEW (layer_view);
  GtkTreeIter       *iter;

  iter = ligma_container_view_lookup (view, (LigmaViewable *) layer);

  ligma_layer_tree_view_update_borders (layer_view, iter);
}

static void
ligma_layer_tree_view_layer_clicked (LigmaCellRendererViewable *cell,
                                    const gchar              *path_str,
                                    GdkModifierType           state,
                                    LigmaLayerTreeView        *layer_view)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (layer_view);
  GtkTreePath           *path      = gtk_tree_path_new_from_string (path_str);
  GtkTreeIter            iter;

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      LigmaUIManager    *ui_manager;
      LigmaActionGroup  *group;
      LigmaViewRenderer *renderer;

      ui_manager = ligma_editor_get_ui_manager (LIGMA_EDITOR (tree_view));
      group      = ligma_ui_manager_get_action_group (ui_manager, "layers");

      gtk_tree_model_get (tree_view->model, &iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      if (renderer)
        {
          LigmaLayer       *layer     = LIGMA_LAYER (renderer->viewable);
          LigmaLayerMask   *mask      = ligma_layer_get_mask (layer);
          GdkModifierType  modifiers = (state & ligma_get_all_modifiers_mask ());

#if 0
/* This feature has been removed because it clashes with the
 * multi-selection (Shift/Ctrl are reserved), and we can't move it to
 * Alt+ because then it would clash with the alpha-to-selection features
 * (cf. ligma_item_tree_view_item_pre_clicked()).
 * Just macro-ing them out for now, just in case, but I don't think
 * there is a chance to revive them as there is no infinite modifiers.
 */
          LigmaImage       *image;

          image = ligma_item_get_image (LIGMA_ITEM (layer));

          if ((modifiers & GDK_MOD1_MASK))
            {
              /* Alternative functions (Alt key or equivalent) when
               * clicking on a layer preview. The actions happen only on
               * the clicked layer, not on selected layers.
               *
               * Note: Alt-click is already reserved for Alpha to
               * selection in LigmaItemTreeView.
               */
              if (modifiers == (GDK_MOD1_MASK | GDK_SHIFT_MASK))
                {
                  /* Alt-Shift-click adds a layer mask with last values */
                  LigmaDialogConfig *config;
                  LigmaChannel      *channel = NULL;

                  if (! mask)
                    {
                      config = LIGMA_DIALOG_CONFIG (image->ligma->config);

                      if (config->layer_add_mask_type == LIGMA_ADD_MASK_CHANNEL)
                        {
                          channel = ligma_image_get_active_channel (image);

                          if (! channel)
                            {
                              LigmaContainer *channels = ligma_image_get_channels (image);

                              channel = LIGMA_CHANNEL (ligma_container_get_first_child (channels));
                            }

                          if (! channel)
                            {
                              /* No channel. We cannot perform the add
                               * mask action. */
                              g_message (_("No channels to create a layer mask from."));
                            }
                        }

                      if (config->layer_add_mask_type != LIGMA_ADD_MASK_CHANNEL || channel)
                        {
                          mask = ligma_layer_create_mask (layer,
                                                         config->layer_add_mask_type,
                                                         channel);

                          if (config->layer_add_mask_invert)
                            ligma_channel_invert (LIGMA_CHANNEL (mask), FALSE);

                          ligma_layer_add_mask (layer, mask, TRUE, NULL);
                          ligma_image_flush (image);
                        }
                    }
                }
              else if (modifiers == (GDK_MOD1_MASK | GDK_CONTROL_MASK))
                {
                  /* Alt-Control-click removes a layer mask */
                  if (mask)
                    {
                      ligma_layer_apply_mask (layer, LIGMA_MASK_DISCARD, TRUE);
                      ligma_image_flush (image);
                    }
                }
              else if (modifiers == (GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK))
                {
                  /* Alt-Shift-Control-click applies a layer mask */
                  if (mask)
                    {
                      ligma_layer_apply_mask (layer, LIGMA_MASK_APPLY, TRUE);
                      ligma_image_flush (image);
                    }
                }
            }
          else
#endif
          if (! modifiers && mask && ligma_layer_get_edit_mask (layer))
            {
              /* Simple clicks (without modifiers) activate the layer */

              if (mask)
                ligma_action_group_set_action_active (group,
                                                     "layers-mask-edit", FALSE);
            }

          g_object_unref (renderer);
        }
    }

  gtk_tree_path_free (path);
}

static void
ligma_layer_tree_view_mask_clicked (LigmaCellRendererViewable *cell,
                                   const gchar              *path_str,
                                   GdkModifierType           state,
                                   LigmaLayerTreeView        *layer_view)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (layer_view);
  GtkTreePath           *path;
  GtkTreeIter            iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      LigmaViewRenderer *renderer;
      LigmaUIManager    *ui_manager;
      LigmaActionGroup  *group;

      ui_manager = ligma_editor_get_ui_manager (LIGMA_EDITOR (tree_view));
      group      = ligma_ui_manager_get_action_group (ui_manager, "layers");

      gtk_tree_model_get (tree_view->model, &iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      if (renderer)
        {
          LigmaLayer       *layer     = LIGMA_LAYER (renderer->viewable);
          GdkModifierType  modifiers = ligma_get_all_modifiers_mask ();

          if ((state & GDK_MOD1_MASK))
            {
              LigmaImage *image;

              image = ligma_item_get_image (LIGMA_ITEM (layer));

              if ((state & modifiers) == GDK_MOD1_MASK)
                {
                  /* Alt-click shows/hides a layer mask */
                  ligma_layer_set_show_mask (layer, ! ligma_layer_get_show_mask (layer), TRUE);
                  ligma_image_flush (image);
                }
              if ((state & modifiers) == (GDK_MOD1_MASK | GDK_CONTROL_MASK))
                {
                  /* Alt-Control-click enables/disables a layer mask */
                  ligma_layer_set_apply_mask (layer, ! ligma_layer_get_apply_mask (layer), TRUE);
                  ligma_image_flush (image);
                }
            }
          else if (! ligma_layer_get_edit_mask (layer))
            {
              /* Simple click selects the mask for edition. */
              ligma_action_group_set_action_active (group,
                                                   "layers-mask-edit", TRUE);
            }

          g_object_unref (renderer);
        }
    }

  gtk_tree_path_free (path);
}


/*  LigmaDrawable alpha callbacks  */

static void
ligma_layer_tree_view_alpha_update (LigmaLayerTreeView *view,
                                   GtkTreeIter       *iter,
                                   LigmaLayer         *layer)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);

  gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES,
                      ligma_drawable_has_alpha (LIGMA_DRAWABLE (layer)) ?
                      NULL : view->priv->bold_attrs,
                      -1);
}

static void
ligma_layer_tree_view_alpha_changed (LigmaLayer         *layer,
                                    LigmaLayerTreeView *layer_view)
{
  LigmaContainerView *view = LIGMA_CONTAINER_VIEW (layer_view);
  GtkTreeIter       *iter;

  iter = ligma_container_view_lookup (view, (LigmaViewable *) layer);

  if (iter)
    {
      LigmaItemTreeView *item_view = LIGMA_ITEM_TREE_VIEW (view);

      ligma_layer_tree_view_alpha_update (layer_view, iter, layer);

      /*  update button states  */
      if (g_list_find (ligma_image_get_selected_layers (ligma_item_tree_view_get_image (item_view)),
                       layer))
        ligma_container_view_select_items (LIGMA_CONTAINER_VIEW (view),
                                          ligma_image_get_selected_layers (ligma_item_tree_view_get_image (item_view)));
    }
}
