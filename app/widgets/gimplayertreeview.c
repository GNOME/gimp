/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplayertreeview.c
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#if 0
/* For mask features in gimp_layer_tree_view_layer_clicked() */
#include "config/gimpdialogconfig.h"
#endif

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpimage.h"
#include "core/gimpitemundo.h"
#include "core/gimplayer.h"
#include "core/gimplayer-floating-selection.h"
#include "core/gimplayer-new.h"
#include "core/gimplayermask.h"
#include "core/gimptreehandler.h"

#include "text/gimptextlayer.h"

#include "file/file-open.h"

#include "gimpactiongroup.h"
#include "gimpcellrendererviewable.h"
#include "gimpcontainertreestore.h"
#include "gimpcontainerview.h"
#include "gimpcontainerview-cruft.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimplayermodebox.h"
#include "gimplayertreeview.h"
#include "gimptoggleaction.h"
#include "gimpuimanager.h"
#include "gimpviewrenderer.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


struct _GimpLayerTreeViewPrivate
{
  GtkWidget       *layer_mode_box;
  GtkAdjustment   *opacity_adjustment;
  GtkWidget       *anchor_button;

  gint             model_column_mask;
  gint             model_column_mask_visible;

  GtkCellRenderer *mask_cell;

  PangoAttrList   *italic_attrs;
  PangoAttrList   *bold_attrs;

  GimpTreeHandler *mode_changed_handler;
  GimpTreeHandler *opacity_changed_handler;
  GimpTreeHandler *mask_changed_handler;
  GimpTreeHandler *alpha_changed_handler;
};


static void       gimp_layer_tree_view_view_iface_init            (GimpContainerViewInterface *iface);

static void       gimp_layer_tree_view_constructed                (GObject                    *object);
static void       gimp_layer_tree_view_finalize                   (GObject                    *object);

static void       gimp_layer_tree_view_set_container              (GimpContainerView          *view,
                                                                   GimpContainer              *container);
static void       gimp_layer_tree_view_set_context                (GimpContainerView          *view,
                                                                   GimpContext                *context);
static void       gimp_layer_tree_view_set_view_size              (GimpContainerView          *view);
static gboolean   gimp_layer_tree_view_set_selected               (GimpContainerView          *view,
                                                                   GList                      *items);

static gpointer   gimp_layer_tree_view_insert_item                (GimpContainerView          *view,
                                                                   GimpViewable               *viewable,
                                                                   gpointer                    parent_insert_data,
                                                                   gint                        index);
static gboolean   gimp_layer_tree_view_drop_possible              (GimpContainerTreeView      *view,
                                                                   GimpDndType                 src_type,
                                                                   GList                      *src_viewables,
                                                                   GimpViewable               *dest_viewable,
                                                                   GtkTreePath                *drop_path,
                                                                   GtkTreeViewDropPosition     drop_pos,
                                                                   GtkTreeViewDropPosition    *return_drop_pos,
                                                                   GdkDragAction              *return_drag_action);
static void       gimp_layer_tree_view_drop_color                 (GimpContainerTreeView      *view,
                                                                   GeglColor                  *color,
                                                                   GimpViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);
static void       gimp_layer_tree_view_drop_uri_list              (GimpContainerTreeView      *view,
                                                                   GList                      *uri_list,
                                                                   GimpViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);
static void       gimp_layer_tree_view_drop_component             (GimpContainerTreeView      *tree_view,
                                                                   GimpImage                  *image,
                                                                   GimpChannelType             component,
                                                                   GimpViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);
static void       gimp_layer_tree_view_drop_pixbuf                (GimpContainerTreeView      *tree_view,
                                                                   GdkPixbuf                  *pixbuf,
                                                                   GimpViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);
static void       gimp_layer_tree_view_set_image                  (GimpItemTreeView           *view,
                                                                   GimpImage                  *image);
static GimpItem * gimp_layer_tree_view_item_new                   (GimpImage                  *image);
static void       gimp_layer_tree_view_floating_selection_changed (GimpImage                  *image,
                                                                   GimpLayerTreeView          *view);

static void       gimp_layer_tree_view_layer_mode_box_callback    (GtkWidget                  *widget,
                                                                   const GParamSpec           *pspec,
                                                                   GimpLayerTreeView          *view);
static void       gimp_layer_tree_view_opacity_scale_changed      (GtkAdjustment              *adj,
                                                                   GimpLayerTreeView          *view);
static void       gimp_layer_tree_view_layer_signal_handler       (GimpLayer                  *layer,
                                                                   GimpLayerTreeView          *view);
static void       gimp_layer_tree_view_update_options             (GimpLayerTreeView          *view,
                                                                   GList                      *layers);
static void       gimp_layer_tree_view_update_highlight           (GimpLayerTreeView          *view);
static void       gimp_layer_tree_view_mask_update                (GimpLayerTreeView          *view,
                                                                   GtkTreeIter                *iter,
                                                                   GimpLayer                  *layer);
static void       gimp_layer_tree_view_mask_changed               (GimpLayer                  *layer,
                                                                   GimpLayerTreeView          *view);
static void       gimp_layer_tree_view_renderer_update            (GimpViewRenderer           *renderer,
                                                                   GimpLayerTreeView          *view);
static void       gimp_layer_tree_view_update_borders             (GimpLayerTreeView          *view,
                                                                   GtkTreeIter                *iter);
static void       gimp_layer_tree_view_mask_callback              (GimpLayer                  *mask,
                                                                   GimpLayerTreeView          *view);
static void       gimp_layer_tree_view_layer_clicked              (GimpCellRendererViewable   *cell,
                                                                   const gchar                *path,
                                                                   GdkModifierType             state,
                                                                   GimpLayerTreeView          *view);
static void       gimp_layer_tree_view_mask_clicked               (GimpCellRendererViewable   *cell,
                                                                   const gchar                *path,
                                                                   GdkModifierType             state,
                                                                   GimpLayerTreeView          *view);
static void       gimp_layer_tree_view_alpha_update               (GimpLayerTreeView          *view,
                                                                   GtkTreeIter                *iter,
                                                                   GimpLayer                  *layer);
static void       gimp_layer_tree_view_alpha_changed              (GimpLayer                  *layer,
                                                                   GimpLayerTreeView          *view);


G_DEFINE_TYPE_WITH_CODE (GimpLayerTreeView, gimp_layer_tree_view,
                         GIMP_TYPE_DRAWABLE_TREE_VIEW,
                         G_ADD_PRIVATE (GimpLayerTreeView)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_layer_tree_view_view_iface_init))

#define parent_class gimp_layer_tree_view_parent_class

static GimpContainerViewInterface *parent_view_iface = NULL;


static void
gimp_layer_tree_view_class_init (GimpLayerTreeViewClass *klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  GimpContainerTreeViewClass *tree_view_class;
  GimpItemTreeViewClass      *item_view_class;

  tree_view_class = GIMP_CONTAINER_TREE_VIEW_CLASS (klass);
  item_view_class = GIMP_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructed           = gimp_layer_tree_view_constructed;
  object_class->finalize              = gimp_layer_tree_view_finalize;

  tree_view_class->drop_possible      = gimp_layer_tree_view_drop_possible;
  tree_view_class->drop_color         = gimp_layer_tree_view_drop_color;
  tree_view_class->drop_uri_list      = gimp_layer_tree_view_drop_uri_list;
  tree_view_class->drop_component     = gimp_layer_tree_view_drop_component;
  tree_view_class->drop_pixbuf        = gimp_layer_tree_view_drop_pixbuf;

  item_view_class->item_type          = GIMP_TYPE_LAYER;
  item_view_class->signal_name        = "selected-layers-changed";

  item_view_class->set_image          = gimp_layer_tree_view_set_image;

  item_view_class->new_item           = gimp_layer_tree_view_item_new;

  item_view_class->action_group        = "layers";
  item_view_class->activate_action     = "layers-edit";
  item_view_class->new_action          = "layers-new";
  item_view_class->new_default_action  = "layers-new-last-values";
  item_view_class->raise_action        = "layers-raise";
  item_view_class->raise_top_action    = "layers-raise-to-top";
  item_view_class->lower_action        = "layers-lower";
  item_view_class->lower_bottom_action = "layers-lower-to-bottom";
  item_view_class->duplicate_action    = "layers-duplicate";
  item_view_class->delete_action       = "layers-delete";

  item_view_class->move_cursor_up_action        = "layers-select-previous";
  item_view_class->move_cursor_down_action      = "layers-select-next";
  item_view_class->move_cursor_up_flat_action   = "layers-select-flattened-previous";
  item_view_class->move_cursor_down_flat_action = "layers-select-flattened-next";
  item_view_class->move_cursor_start_action     = "layers-select-top";
  item_view_class->move_cursor_end_action       = "layers-select-bottom";

  item_view_class->lock_content_help_id    = GIMP_HELP_LAYER_LOCK_PIXELS;
  item_view_class->lock_position_help_id   = GIMP_HELP_LAYER_LOCK_POSITION;
  item_view_class->lock_visibility_help_id = GIMP_HELP_LAYER_LOCK_VISIBILITY;
}

static void
gimp_layer_tree_view_view_iface_init (GimpContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->set_container = gimp_layer_tree_view_set_container;
  iface->set_context   = gimp_layer_tree_view_set_context;
  iface->set_view_size = gimp_layer_tree_view_set_view_size;
  iface->set_selected  = gimp_layer_tree_view_set_selected;

  iface->insert_item   = gimp_layer_tree_view_insert_item;

  iface->model_is_tree = TRUE;
}

static void
gimp_layer_tree_view_init (GimpLayerTreeView *view)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GtkWidget             *scale;
  PangoAttribute        *attr;

  view->priv = gimp_layer_tree_view_get_instance_private (view);

  view->priv->model_column_mask =
    gimp_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           GIMP_TYPE_VIEW_RENDERER);

  view->priv->model_column_mask_visible =
    gimp_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);

  /*  Paint mode menu  */

  view->priv->layer_mode_box = gimp_layer_mode_box_new (GIMP_LAYER_MODE_CONTEXT_LAYER);
  gimp_layer_mode_box_set_label (GIMP_LAYER_MODE_BOX (view->priv->layer_mode_box),
                                 _("Mode"));
  gimp_item_tree_view_add_options (GIMP_ITEM_TREE_VIEW (view), NULL,
                                   view->priv->layer_mode_box);

  g_signal_connect (view->priv->layer_mode_box, "notify::layer-mode",
                    G_CALLBACK (gimp_layer_tree_view_layer_mode_box_callback),
                    view);
  gimp_help_connect (view->priv->layer_mode_box, NULL, gimp_standard_help_func,
                     GIMP_HELP_LAYER_DIALOG_PAINT_MODE_MENU, NULL, NULL);

  /*  Opacity scale  */

  view->priv->opacity_adjustment = gtk_adjustment_new (100.0, 0.0, 100.0,
                                                       1.0, 10.0, 0.0);
  scale = gimp_spin_scale_new (view->priv->opacity_adjustment, _("Opacity"), 1);
  gimp_spin_scale_set_constrain_drag (GIMP_SPIN_SCALE (scale), TRUE);
  gimp_help_connect (scale, NULL, gimp_standard_help_func,
                     GIMP_HELP_LAYER_DIALOG_OPACITY_SCALE, NULL, NULL);
  gimp_item_tree_view_add_options (GIMP_ITEM_TREE_VIEW (view),
                                   NULL, scale);

  g_signal_connect (view->priv->opacity_adjustment, "value-changed",
                    G_CALLBACK (gimp_layer_tree_view_opacity_scale_changed),
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
gimp_layer_tree_view_constructed (GObject *object)
{
  GimpContainerTreeView *tree_view  = GIMP_CONTAINER_TREE_VIEW (object);
  GimpLayerTreeView     *layer_view = GIMP_LAYER_TREE_VIEW (object);
  GtkWidget             *button;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  layer_view->priv->mask_cell = gimp_cell_renderer_viewable_new ();
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

  gimp_container_tree_view_add_renderer_cell (tree_view,
                                              layer_view->priv->mask_cell,
                                              layer_view->priv->model_column_mask);

  g_signal_connect (tree_view->renderer_cell, "clicked",
                    G_CALLBACK (gimp_layer_tree_view_layer_clicked),
                    layer_view);
  g_signal_connect (layer_view->priv->mask_cell, "clicked",
                    G_CALLBACK (gimp_layer_tree_view_mask_clicked),
                    layer_view);

  gimp_dnd_component_dest_add (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);
  gimp_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), GIMP_TYPE_CHANNEL,
                               NULL, tree_view);
  gimp_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), GIMP_TYPE_LAYER_MASK,
                               NULL, tree_view);
  gimp_dnd_uri_list_dest_add  (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);
  gimp_dnd_pixbuf_dest_add    (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);

  button = gimp_editor_add_action_button (GIMP_EDITOR (layer_view), "layers",
                                          "layers-new-group", NULL);
  gtk_box_reorder_child (gimp_editor_get_button_box (GIMP_EDITOR (layer_view)),
                         button, 1);

  button = gimp_editor_add_action_button (GIMP_EDITOR (layer_view), "layers",
                                          "layers-anchor", NULL);
  layer_view->priv->anchor_button = button;
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (layer_view),
                                  GTK_BUTTON (button),
                                  GIMP_TYPE_LAYER);
  gtk_box_reorder_child (gimp_editor_get_button_box (GIMP_EDITOR (layer_view)),
                         button, 5);

  button = gimp_editor_add_action_button (GIMP_EDITOR (layer_view), "layers",
                                          "layers-merge-down-button",
                                          "layers-merge-group",
                                          GDK_SHIFT_MASK,
                                          "layers-merge-layers",
                                          GDK_CONTROL_MASK,
                                          "layers-merge-layers-last-values",
                                          GDK_CONTROL_MASK |
                                          GDK_SHIFT_MASK,
                                          NULL);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (layer_view),
                                  GTK_BUTTON (button),
                                  GIMP_TYPE_LAYER);
  gtk_box_reorder_child (gimp_editor_get_button_box (GIMP_EDITOR (layer_view)),
                         button, 6);

  button = gimp_editor_add_action_button (GIMP_EDITOR (layer_view), "layers",
                                          "layers-mask-add-button",
                                          "layers-mask-add-last-values",
                                          gimp_get_extend_selection_mask (),
                                          "layers-mask-delete",
                                          gimp_get_modify_selection_mask (),
                                          "layers-mask-apply",
                                          gimp_get_extend_selection_mask () |
                                          gimp_get_modify_selection_mask (),
                                          NULL);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (layer_view),
                                  GTK_BUTTON (button),
                                  GIMP_TYPE_LAYER);
  gtk_box_reorder_child (gimp_editor_get_button_box (GIMP_EDITOR (layer_view)),
                         button, 7);

  /*  Lock alpha toggle  */

  gimp_item_tree_view_add_lock (GIMP_ITEM_TREE_VIEW (tree_view),
                                GIMP_ICON_LOCK_ALPHA,
                                (GimpIsLockedFunc) gimp_layer_get_lock_alpha,
                                (GimpCanLockFunc)  gimp_layer_can_lock_alpha,
                                (GimpSetLockFunc)  gimp_layer_set_lock_alpha,
                                (GimpUndoLockPush) gimp_image_undo_push_layer_lock_alpha,
                                "lock-alpha-changed",
                                GIMP_UNDO_LAYER_LOCK_ALPHA,
                                GIMP_UNDO_GROUP_LAYER_LOCK_ALPHA,
                                _("Lock alpha channel"),
                                _("Unlock alpha channel"),
                                _("Set Item Exclusive Alpha Channel lock"),
                                _("Lock alpha channel"),
                                GIMP_HELP_LAYER_LOCK_ALPHA);
}

static void
gimp_layer_tree_view_finalize (GObject *object)
{
  GimpLayerTreeView *layer_view = GIMP_LAYER_TREE_VIEW (object);

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


/*  GimpContainerView methods  */

static void
gimp_layer_tree_view_set_container (GimpContainerView *view,
                                    GimpContainer     *container)
{
  GimpLayerTreeView *layer_view = GIMP_LAYER_TREE_VIEW (view);
  GimpContainer     *old_container;

  old_container = gimp_container_view_get_container (view);

  if (old_container)
    {
      gimp_tree_handler_disconnect (layer_view->priv->mode_changed_handler);
      layer_view->priv->mode_changed_handler = NULL;

      gimp_tree_handler_disconnect (layer_view->priv->opacity_changed_handler);
      layer_view->priv->opacity_changed_handler = NULL;

      gimp_tree_handler_disconnect (layer_view->priv->mask_changed_handler);
      layer_view->priv->mask_changed_handler = NULL;

      gimp_tree_handler_disconnect (layer_view->priv->alpha_changed_handler);
      layer_view->priv->alpha_changed_handler = NULL;
    }

  parent_view_iface->set_container (view, container);

  if (container)
    {
      layer_view->priv->mode_changed_handler =
        gimp_tree_handler_connect (container, "mode-changed",
                                   G_CALLBACK (gimp_layer_tree_view_layer_signal_handler),
                                   view);

      layer_view->priv->opacity_changed_handler =
        gimp_tree_handler_connect (container, "opacity-changed",
                                   G_CALLBACK (gimp_layer_tree_view_layer_signal_handler),
                                   view);

      layer_view->priv->mask_changed_handler =
        gimp_tree_handler_connect (container, "mask-changed",
                                   G_CALLBACK (gimp_layer_tree_view_mask_changed),
                                   view);

      layer_view->priv->alpha_changed_handler =
        gimp_tree_handler_connect (container, "alpha-changed",
                                   G_CALLBACK (gimp_layer_tree_view_alpha_changed),
                                   view);
    }
}

typedef struct
{
  gint         mask_column;
  GimpContext *context;
} SetContextForeachData;

static gboolean
gimp_layer_tree_view_set_context_foreach (GtkTreeModel *model,
                                          GtkTreePath  *path,
                                          GtkTreeIter  *iter,
                                          gpointer      data)
{
  SetContextForeachData *context_data = data;
  GimpViewRenderer      *renderer;

  gtk_tree_model_get (model, iter,
                      context_data->mask_column, &renderer,
                      -1);

  if (renderer)
    {
      gimp_view_renderer_set_context (renderer, context_data->context);

      g_object_unref (renderer);
    }

  return FALSE;
}

static void
gimp_layer_tree_view_set_context (GimpContainerView *view,
                                  GimpContext       *context)
{
  GimpContainerTreeView *tree_view  = GIMP_CONTAINER_TREE_VIEW (view);
  GimpLayerTreeView     *layer_view = GIMP_LAYER_TREE_VIEW (view);

  parent_view_iface->set_context (view, context);

  if (tree_view->model)
    {
      SetContextForeachData context_data = { layer_view->priv->model_column_mask,
                                             context };

      gtk_tree_model_foreach (tree_view->model,
                              gimp_layer_tree_view_set_context_foreach,
                              &context_data);
    }
}

typedef struct
{
  gint mask_column;
  gint view_size;
  gint border_width;
} SetSizeForeachData;

static gboolean
gimp_layer_tree_view_set_view_size_foreach (GtkTreeModel *model,
                                            GtkTreePath  *path,
                                            GtkTreeIter  *iter,
                                            gpointer      data)
{
  SetSizeForeachData *size_data = data;
  GimpViewRenderer   *renderer;

  gtk_tree_model_get (model, iter,
                      size_data->mask_column, &renderer,
                      -1);

  if (renderer)
    {
      gimp_view_renderer_set_size (renderer,
                                   size_data->view_size,
                                   size_data->border_width);

      g_object_unref (renderer);
    }

  return FALSE;
}

static void
gimp_layer_tree_view_set_view_size (GimpContainerView *view)
{
  GimpContainerTreeView *tree_view  = GIMP_CONTAINER_TREE_VIEW (view);

  if (tree_view->model)
    {
      GimpLayerTreeView *layer_view = GIMP_LAYER_TREE_VIEW (view);
      SetSizeForeachData size_data;

      size_data.mask_column = layer_view->priv->model_column_mask;

      size_data.view_size =
        gimp_container_view_get_view_size (view, &size_data.border_width);

      gtk_tree_model_foreach (tree_view->model,
                              gimp_layer_tree_view_set_view_size_foreach,
                              &size_data);
    }

  parent_view_iface->set_view_size (view);
}

static gboolean
gimp_layer_tree_view_set_selected (GimpContainerView *view,
                                   GList             *items)
{
  GimpLayerTreeView *layer_view = GIMP_LAYER_TREE_VIEW (view);
  gboolean           success;

  success = parent_view_iface->set_selected (view, items);

  if (items && success)
    {
      GList *list;

      for (list = items; list; list = g_list_next (list))
        {
          GtkTreeIter *iter;

          iter = _gimp_container_view_lookup (view, list->data);

          if (iter)
            gimp_layer_tree_view_update_borders (layer_view, iter);
        }

      gimp_layer_tree_view_update_options (layer_view, items);
    }

  if (! success)
    {
      GimpEditor *editor = GIMP_EDITOR (view);

      /* currently, select_items() only ever fails when there is a
       * floating selection, which can be committed/canceled through
       * the editor buttons.
       */
      gimp_widget_blink (GTK_WIDGET (gimp_editor_get_button_box (editor)));
    }

  return success;
}

static gpointer
gimp_layer_tree_view_insert_item (GimpContainerView *view,
                                  GimpViewable      *viewable,
                                  gpointer           parent_insert_data,
                                  gint               index)
{
  GimpLayerTreeView *layer_view = GIMP_LAYER_TREE_VIEW (view);
  GimpLayer         *layer;
  GtkTreeIter       *iter;

  iter = parent_view_iface->insert_item (view, viewable,
                                         parent_insert_data, index);

  layer = GIMP_LAYER (viewable);

  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    gimp_layer_tree_view_alpha_update (layer_view, iter, layer);

  gimp_layer_tree_view_mask_update (layer_view, iter, layer);

  if (GIMP_IS_LAYER (viewable) && gimp_layer_is_floating_sel (GIMP_LAYER (viewable)))
    {
      GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
      GimpLayer             *floating  = GIMP_LAYER (viewable);
      GimpDrawable          *drawable  = gimp_layer_get_floating_sel_drawable (floating);

      if (GIMP_IS_LAYER_MASK (drawable))
        {
          /* Display floating mask in the mask column. */
          GimpViewRenderer *renderer  = NULL;

          gtk_tree_model_get (GTK_TREE_MODEL (tree_view->model), iter,
                              GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                              -1);
          if (renderer)
            {
              gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                                  layer_view->priv->model_column_mask,         renderer,
                                  layer_view->priv->model_column_mask_visible, TRUE,
                                  GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER,   NULL,
                                  -1);
              g_object_unref (renderer);
            }
        }
    }

  return iter;
}


/*  GimpContainerTreeView methods  */

static gboolean
gimp_layer_tree_view_drop_possible (GimpContainerTreeView   *tree_view,
                                    GimpDndType              src_type,
                                    GList                   *src_viewables,
                                    GimpViewable            *dest_viewable,
                                    GtkTreePath             *drop_path,
                                    GtkTreeViewDropPosition  drop_pos,
                                    GtkTreeViewDropPosition *return_drop_pos,
                                    GdkDragAction           *return_drag_action)
{
  /* If we are dropping a new layer, check if the destination image
   * has a floating selection.
   */
  if  (src_type == GIMP_DND_TYPE_URI_LIST     ||
       src_type == GIMP_DND_TYPE_TEXT_PLAIN   ||
       src_type == GIMP_DND_TYPE_NETSCAPE_URL ||
       src_type == GIMP_DND_TYPE_COMPONENT    ||
       src_type == GIMP_DND_TYPE_PIXBUF       ||
       g_list_length (src_viewables) > 0)
    {
      GimpImage *dest_image = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (tree_view));

      if (gimp_image_get_floating_selection (dest_image))
        return FALSE;
    }

  return GIMP_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_possible (tree_view,
                                                                       src_type,
                                                                       src_viewables,
                                                                       dest_viewable,
                                                                       drop_path,
                                                                       drop_pos,
                                                                       return_drop_pos,
                                                                       return_drag_action);
}

static void
gimp_layer_tree_view_drop_color (GimpContainerTreeView   *view,
                                 GeglColor               *color,
                                 GimpViewable            *dest_viewable,
                                 GtkTreeViewDropPosition  drop_pos)
{
  if (gimp_item_is_text_layer (GIMP_ITEM (dest_viewable)))
    {
      gimp_text_layer_set (GIMP_TEXT_LAYER (dest_viewable), NULL,
                           "color", color,
                           NULL);
      gimp_image_flush (gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view)));
      return;
    }

  GIMP_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_color (view, color,
                                                             dest_viewable,
                                                             drop_pos);
}

static void
gimp_layer_tree_view_drop_uri_list (GimpContainerTreeView   *view,
                                    GList                   *uri_list,
                                    GimpViewable            *dest_viewable,
                                    GtkTreeViewDropPosition  drop_pos)
{
  GimpItemTreeView  *item_view = GIMP_ITEM_TREE_VIEW (view);
  GimpContainerView *cont_view = GIMP_CONTAINER_VIEW (view);
  GimpImage         *image     = gimp_item_tree_view_get_image (item_view);
  GimpLayer         *parent;
  gint               index;
  GList             *list;

  index = gimp_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (GimpViewable **) &parent);

  g_object_ref (image);

  for (list = uri_list; list; list = g_list_next (list))
    {
      const gchar       *uri   = list->data;
      GFile             *file  = g_file_new_for_uri (uri);
      GList             *new_layers;
      GimpPDBStatusType  status;
      GError            *error = NULL;

      new_layers = file_open_layers (image->gimp,
                                     gimp_container_view_get_context (cont_view),
                                     NULL,
                                     image, FALSE, FALSE,
                                     file, GIMP_RUN_INTERACTIVE, NULL,
                                     &status, &error);

      if (new_layers)
        {
          gimp_image_add_layers (image, new_layers, parent, index,
                                 0, 0,
                                 gimp_image_get_width (image),
                                 gimp_image_get_height (image),
                                 _("Drop layers"));

          index += g_list_length (new_layers);

          g_list_free (new_layers);
        }
      else if (status != GIMP_PDB_CANCEL)
        {
          gimp_message (image->gimp, G_OBJECT (view), GIMP_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        gimp_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }

      g_object_unref (file);
    }

  gimp_image_flush (image);

  g_object_unref (image);
}

static void
gimp_layer_tree_view_drop_component (GimpContainerTreeView   *tree_view,
                                     GimpImage               *src_image,
                                     GimpChannelType          component,
                                     GimpViewable            *dest_viewable,
                                     GtkTreeViewDropPosition  drop_pos)
{
  GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (tree_view);
  GimpImage        *image     = gimp_item_tree_view_get_image (item_view);
  GimpChannel      *channel;
  GimpItem         *new_item;
  GimpLayer        *parent;
  gint              index;
  const gchar      *desc;

  index = gimp_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (GimpViewable **) &parent);

  channel = gimp_channel_new_from_component (src_image, component, NULL, NULL);

  new_item = gimp_item_convert (GIMP_ITEM (channel), image,
                                GIMP_TYPE_LAYER);

  g_object_unref (channel);

  gimp_enum_get_value (GIMP_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);
  gimp_object_take_name (GIMP_OBJECT (new_item),
                         g_strdup_printf (_("%s Channel Copy"), desc));

  gimp_image_add_layer (image, GIMP_LAYER (new_item), parent, index, TRUE);

  gimp_image_flush (image);
}

static void
gimp_layer_tree_view_drop_pixbuf (GimpContainerTreeView   *tree_view,
                                  GdkPixbuf               *pixbuf,
                                  GimpViewable            *dest_viewable,
                                  GtkTreeViewDropPosition  drop_pos)
{
  GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (tree_view);
  GimpImage        *image     = gimp_item_tree_view_get_image (item_view);
  GimpLayer        *new_layer;
  GimpLayer        *parent;
  gint              index;

  index = gimp_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (GimpViewable **) &parent);

  new_layer =
    gimp_layer_new_from_pixbuf (pixbuf, image,
                                gimp_image_get_layer_format (image, TRUE),
                                _("Dropped Buffer"),
                                GIMP_OPACITY_OPAQUE,
                                gimp_image_get_default_new_layer_mode (image));

  gimp_image_add_layer (image, new_layer, parent, index, TRUE);

  gimp_image_flush (image);
}


/*  GimpItemTreeView methods  */

static void
gimp_layer_tree_view_set_image (GimpItemTreeView *view,
                                GimpImage        *image)
{
  GimpLayerTreeView *layer_view = GIMP_LAYER_TREE_VIEW (view);

  if (gimp_item_tree_view_get_image (view))
    {
      g_signal_handlers_disconnect_by_func (gimp_item_tree_view_get_image (view),
                                            gimp_layer_tree_view_floating_selection_changed,
                                            view);
    }

  GIMP_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (view, image);

  if (gimp_item_tree_view_get_image (view))
    {
      g_signal_connect (gimp_item_tree_view_get_image (view),
                        "floating-selection-changed",
                        G_CALLBACK (gimp_layer_tree_view_floating_selection_changed),
                        view);

      /* call gimp_layer_tree_view_floating_selection_changed() now, to update
       * the floating selection's row attributes.
       */
      gimp_layer_tree_view_floating_selection_changed (
        gimp_item_tree_view_get_image (view),
        layer_view);
    }

  gimp_layer_tree_view_update_highlight (layer_view);
}

static GimpItem *
gimp_layer_tree_view_item_new (GimpImage *image)
{
  GimpLayer *new_layer;

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE,
                               _("New Layer"));

  new_layer = gimp_layer_new (image,
                              gimp_image_get_width (image),
                              gimp_image_get_height (image),
                              gimp_image_get_layer_format (image, TRUE),
                              NULL,
                              GIMP_OPACITY_OPAQUE,
                              gimp_image_get_default_new_layer_mode (image));

  gimp_image_add_layer (image, new_layer,
                        GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

  gimp_image_undo_group_end (image);

  return GIMP_ITEM (new_layer);
}


/*  callbacks  */

static void
gimp_layer_tree_view_floating_selection_changed (GimpImage         *image,
                                                 GimpLayerTreeView *layer_view)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (layer_view);
  GimpContainerView     *view      = GIMP_CONTAINER_VIEW (layer_view);
  GimpLayer             *floating_sel;
  GtkTreeIter           *iter;

  floating_sel = gimp_image_get_floating_selection (image);

  if (floating_sel)
    {
      iter = _gimp_container_view_lookup (view, (GimpViewable *) floating_sel);

      if (iter)
        gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                            GIMP_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES,
                            layer_view->priv->italic_attrs,
                            -1);
    }
  else
    {
      GList *all_layers;
      GList *list;

      all_layers = gimp_image_get_layer_list (image);

      for (list = all_layers; list; list = g_list_next (list))
        {
          GimpDrawable *drawable = list->data;

          iter = _gimp_container_view_lookup (view, (GimpViewable *) drawable);

          if (iter)
            gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                                GIMP_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES,
                                gimp_drawable_has_alpha (drawable) ?
                                NULL : layer_view->priv->bold_attrs,
                                -1);
        }

      g_list_free (all_layers);
    }

  gimp_layer_tree_view_update_highlight (layer_view);
}


/*  Paint Mode, Opacity and Lock alpha callbacks  */

#define BLOCK() \
        g_signal_handlers_block_by_func (layer, \
        gimp_layer_tree_view_layer_signal_handler, view)

#define UNBLOCK() \
        g_signal_handlers_unblock_by_func (layer, \
        gimp_layer_tree_view_layer_signal_handler, view)


static void
gimp_layer_tree_view_layer_mode_box_callback (GtkWidget         *widget,
                                              const GParamSpec  *pspec,
                                              GimpLayerTreeView *view)
{
  GimpImage     *image;
  GList         *layers    = NULL;
  GList         *iter;
  GimpUndo      *undo;
  gboolean       push_undo = TRUE;
  gint           n_layers  = 0;
  GimpLayerMode  mode;

  image = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));
  mode  = gimp_layer_mode_box_get_mode (GIMP_LAYER_MODE_BOX (widget));
  undo  = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                        GIMP_UNDO_LAYER_MODE);

  if (image)
    layers = gimp_image_get_selected_layers (image);

  for (iter = layers; iter; iter = iter->next)
    {
      if (gimp_layer_get_mode (iter->data) != mode)
        {
          n_layers++;

          if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (iter->data))
            push_undo = FALSE;
        }
    }

  if (n_layers > 1)
    {
      /*  Don't compress mode undos with more than 1 layer changed. */
      push_undo = TRUE;

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_OPACITY,
                                   _("Set layers mode"));
    }

  for (iter = layers; iter; iter = iter->next)
    {
      GimpLayer *layer = iter->data;

      if (gimp_layer_get_mode (layer) != mode)
        {
          BLOCK();
          gimp_layer_set_mode (layer, (GimpLayerMode) mode, push_undo);
          UNBLOCK();
        }
    }
  gimp_image_flush (image);

  if (! push_undo)
    gimp_undo_refresh_preview (undo, gimp_container_view_get_context (GIMP_CONTAINER_VIEW (view)));

  if (n_layers > 1)
    gimp_image_undo_group_end (image);
}

static void
gimp_layer_tree_view_opacity_scale_changed (GtkAdjustment     *adjustment,
                                            GimpLayerTreeView *view)
{
  GimpImage *image;
  GList     *layers;
  GList     *iter;
  GimpUndo  *undo;
  gboolean   push_undo = TRUE;
  gint       n_layers = 0;
  gdouble    opacity;

  image   = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));
  layers  = gimp_image_get_selected_layers (image);
  undo    = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                          GIMP_UNDO_LAYER_OPACITY);
  opacity = gtk_adjustment_get_value (adjustment) / 100.0;

  for (iter = layers; iter; iter = iter->next)
    {
      if (gimp_layer_get_opacity (iter->data) != opacity)
        {
          n_layers++;

          if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (iter->data))
            push_undo = FALSE;
        }
    }
  if (n_layers > 1)
    {
      /*  Don't compress opacity undos with more than 1 layer changed. */
      push_undo = TRUE;

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_OPACITY,
                                   _("Set layers opacity"));
    }

  for (iter = layers; iter; iter = iter->next)
    {
      GimpLayer *layer = iter->data;

      if (gimp_layer_get_opacity (layer) != opacity)
        {
          BLOCK();
          gimp_layer_set_opacity (layer, opacity, push_undo);
          UNBLOCK();
        }
    }
  gimp_image_flush (image);

  if (! push_undo)
    gimp_undo_refresh_preview (undo, gimp_container_view_get_context (GIMP_CONTAINER_VIEW (view)));

  if (n_layers > 1)
    gimp_image_undo_group_end (image);
}

#undef BLOCK
#undef UNBLOCK


static void
gimp_layer_tree_view_layer_signal_handler (GimpLayer         *layer,
                                           GimpLayerTreeView *view)
{
  GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (view);
  GList            *selected_layers;

  selected_layers =
    gimp_image_get_selected_layers (gimp_item_tree_view_get_image (item_view));

  if (g_list_find (selected_layers, layer))
    gimp_layer_tree_view_update_options (view, selected_layers);
}


#define BLOCK(object,function) \
        g_signal_handlers_block_by_func ((object), (function), view)

#define UNBLOCK(object,function) \
        g_signal_handlers_unblock_by_func ((object), (function), view)

static void
gimp_layer_tree_view_update_options (GimpLayerTreeView *view,
                                     GList             *layers)
{
  GList                *iter;
  GimpLayerMode         mode = GIMP_LAYER_MODE_SEPARATOR;
  GimpLayerModeContext  context = 0;
  /*gboolean              inconsistent_opacity    = FALSE;*/
  gboolean              inconsistent_mode       = FALSE;
  gdouble               opacity = -1.0;

  for (iter = layers; iter; iter = iter->next)
    {
#if 0
      if (opacity != -1.0 &&
          opacity != gimp_layer_get_opacity (iter->data))
        /* We don't really have a way to show an inconsistent
         * GimpSpinScale. This is currently unused.
         */
        inconsistent_opacity = TRUE;
#endif

      opacity = gimp_layer_get_opacity (iter->data);

      if (gimp_viewable_get_children (iter->data))
        context |= GIMP_LAYER_MODE_CONTEXT_GROUP;
      else
        context |= GIMP_LAYER_MODE_CONTEXT_LAYER;

      if (mode != GIMP_LAYER_MODE_SEPARATOR &&
          mode != gimp_layer_get_mode (iter->data))
        inconsistent_mode = TRUE;

      mode = gimp_layer_get_mode (iter->data);
    }
  if (opacity == -1.0)
    opacity = 1.0;

  if (inconsistent_mode)
    mode = GIMP_LAYER_MODE_SEPARATOR;

  if (! context)
    context = GIMP_LAYER_MODE_CONTEXT_LAYER;

  BLOCK (view->priv->layer_mode_box,
         gimp_layer_tree_view_layer_mode_box_callback);

  gimp_layer_mode_box_set_context (GIMP_LAYER_MODE_BOX (view->priv->layer_mode_box),
                                   context);
  gimp_layer_mode_box_set_mode (GIMP_LAYER_MODE_BOX (view->priv->layer_mode_box),
                                mode);

  UNBLOCK (view->priv->layer_mode_box,
           gimp_layer_tree_view_layer_mode_box_callback);

  if (opacity * 100.0 !=
      gtk_adjustment_get_value (view->priv->opacity_adjustment))
    {
      BLOCK (view->priv->opacity_adjustment,
             gimp_layer_tree_view_opacity_scale_changed);

      gtk_adjustment_set_value (view->priv->opacity_adjustment,
                                opacity * 100.0);

      UNBLOCK (view->priv->opacity_adjustment,
               gimp_layer_tree_view_opacity_scale_changed);
    }
}

#undef BLOCK
#undef UNBLOCK

static void
gimp_layer_tree_view_update_highlight (GimpLayerTreeView *layer_view)
{
  GimpItemTreeView *item_view    = GIMP_ITEM_TREE_VIEW (layer_view);
  GimpImage        *image        = gimp_item_tree_view_get_image (item_view);
  GimpLayer        *floating_sel = NULL;
  GtkReliefStyle    default_relief;

  if (image)
    floating_sel = gimp_image_get_floating_selection (image);

  gtk_widget_style_get (GTK_WIDGET (layer_view),
                        "button-relief", &default_relief,
                        NULL);

  gimp_button_set_suggested (gimp_item_tree_view_get_new_button (item_view),
                             floating_sel &&
                             ! GIMP_IS_CHANNEL (gimp_layer_get_floating_sel_drawable (floating_sel)),
                             default_relief);

  gimp_button_set_destructive (gimp_item_tree_view_get_delete_button (item_view),
                               floating_sel != NULL,
                               default_relief);

  gimp_button_set_suggested (layer_view->priv->anchor_button,
                             floating_sel != NULL,
                             default_relief);
  if (floating_sel != NULL)
    {
      if (GIMP_IS_LAYER_MASK (gimp_layer_get_floating_sel_drawable (floating_sel)))
        gtk_widget_set_tooltip_text (layer_view->priv->anchor_button,
                                     C_("layers-action", "Anchor the floating mask"));
      else
        gtk_widget_set_tooltip_text (layer_view->priv->anchor_button,
                                     C_("layers-action", "Anchor the floating layer"));
    }
  else
    {
      gtk_widget_set_tooltip_text (layer_view->priv->anchor_button,
                                   C_("layers-action", "Anchor the floating layer or mask"));
    }
}


/*  Layer Mask callbacks  */

static void
gimp_layer_tree_view_mask_update (GimpLayerTreeView *layer_view,
                                  GtkTreeIter       *iter,
                                  GimpLayer         *layer)
{
  GimpContainerView     *view         = GIMP_CONTAINER_VIEW (layer_view);
  GimpContainerTreeView *tree_view    = GIMP_CONTAINER_TREE_VIEW (layer_view);
  GimpLayerMask         *mask;
  GimpViewRenderer      *renderer     = NULL;
  gboolean               mask_visible = FALSE;

  mask = gimp_layer_get_mask (layer);

  if (mask)
    {
      GClosure *closure;
      gint      view_size;
      gint      border_width;

      view_size = gimp_container_view_get_view_size (view, &border_width);

      mask_visible = TRUE;

      renderer = gimp_view_renderer_new (gimp_container_view_get_context (view),
                                         G_TYPE_FROM_INSTANCE (mask),
                                         view_size, border_width,
                                         FALSE);
      gimp_view_renderer_set_viewable (renderer, GIMP_VIEWABLE (mask));

      g_signal_connect (renderer, "update",
                        G_CALLBACK (gimp_layer_tree_view_renderer_update),
                        layer_view);

      closure = g_cclosure_new (G_CALLBACK (gimp_layer_tree_view_mask_callback),
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

  gimp_layer_tree_view_update_borders (layer_view, iter);

  if (renderer)
    {
      gimp_view_renderer_remove_idle (renderer);
      g_object_unref (renderer);
    }
}

static void
gimp_layer_tree_view_mask_changed (GimpLayer         *layer,
                                   GimpLayerTreeView *layer_view)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (layer_view);
  GtkTreeIter       *iter;

  iter = _gimp_container_view_lookup (view, GIMP_VIEWABLE (layer));

  if (iter)
    gimp_layer_tree_view_mask_update (layer_view, iter, layer);
}

static void
gimp_layer_tree_view_renderer_update (GimpViewRenderer  *renderer,
                                      GimpLayerTreeView *layer_view)
{
  GimpContainerView     *view      = GIMP_CONTAINER_VIEW (layer_view);
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (layer_view);
  GimpLayerMask         *mask;
  GtkTreeIter           *iter;

  mask = GIMP_LAYER_MASK (renderer->viewable);

  iter = _gimp_container_view_lookup (view, (GimpViewable *)
                                      gimp_layer_mask_get_layer (mask));

  if (iter)
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (tree_view->model, iter);

      gtk_tree_model_row_changed (tree_view->model, path, iter);

      gtk_tree_path_free (path);
    }
}

static void
gimp_layer_tree_view_update_borders (GimpLayerTreeView *layer_view,
                                     GtkTreeIter       *iter)
{
  GimpContainerTreeView *tree_view  = GIMP_CONTAINER_TREE_VIEW (layer_view);
  GimpViewRenderer      *layer_renderer;
  GimpViewRenderer      *mask_renderer;
  GimpLayer             *layer      = NULL;
  GimpLayerMask         *mask       = NULL;
  GimpViewBorderType     layer_type = GIMP_VIEW_BORDER_BLACK;

  gtk_tree_model_get (tree_view->model, iter,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &layer_renderer,
                      layer_view->priv->model_column_mask,       &mask_renderer,
                      -1);

  if (layer_renderer)
    layer = GIMP_LAYER (layer_renderer->viewable);
  else if (mask_renderer && GIMP_IS_LAYER (mask_renderer->viewable))
    /* This happens when floating masks are displayed (in the mask column).
     * In such a case, the mask_renderer will actually be a layer renderer
     * showing the floating mask.
     */
    layer = GIMP_LAYER (mask_renderer->viewable);

  g_return_if_fail (layer != NULL);

  if (mask_renderer && GIMP_IS_LAYER_MASK (mask_renderer->viewable))
    mask = GIMP_LAYER_MASK (mask_renderer->viewable);

  if (! mask || (mask && ! gimp_layer_get_edit_mask (layer)))
    layer_type = GIMP_VIEW_BORDER_WHITE;

  if (layer_renderer)
    gimp_view_renderer_set_border_type (layer_renderer, layer_type);

  if (mask)
    {
      GimpViewBorderType mask_color = GIMP_VIEW_BORDER_BLACK;

      if (gimp_layer_get_show_mask (layer))
        {
          mask_color = GIMP_VIEW_BORDER_GREEN;
        }
      else if (! gimp_layer_get_apply_mask (layer))
        {
          mask_color = GIMP_VIEW_BORDER_RED;
        }
      else if (gimp_layer_get_edit_mask (layer))
        {
          mask_color = GIMP_VIEW_BORDER_WHITE;
        }

      gimp_view_renderer_set_border_type (mask_renderer, mask_color);
    }
  else if (mask_renderer)
    {
      /* Floating mask. */
      gimp_view_renderer_set_border_type (mask_renderer, GIMP_VIEW_BORDER_WHITE);
    }

  if (layer_renderer)
    g_object_unref (layer_renderer);

  if (mask_renderer)
    g_object_unref (mask_renderer);
}

static void
gimp_layer_tree_view_mask_callback (GimpLayer         *layer,
                                    GimpLayerTreeView *layer_view)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (layer_view);
  GtkTreeIter       *iter;

  iter = _gimp_container_view_lookup (view, (GimpViewable *) layer);

  gimp_layer_tree_view_update_borders (layer_view, iter);
}

static void
gimp_layer_tree_view_layer_clicked (GimpCellRendererViewable *cell,
                                    const gchar              *path_str,
                                    GdkModifierType           state,
                                    GimpLayerTreeView        *layer_view)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (layer_view);
  GtkTreePath           *path      = gtk_tree_path_new_from_string (path_str);
  GtkTreeIter            iter;

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpUIManager    *ui_manager;
      GimpActionGroup  *group;
      GimpViewRenderer *renderer;

      ui_manager = gimp_editor_get_ui_manager (GIMP_EDITOR (tree_view));
      group      = gimp_ui_manager_get_action_group (ui_manager, "layers");

      gtk_tree_model_get (tree_view->model, &iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      if (renderer)
        {
          GimpLayer       *layer     = GIMP_LAYER (renderer->viewable);
          GimpLayerMask   *mask      = gimp_layer_get_mask (layer);
          GdkModifierType  modifiers = (state & gimp_get_all_modifiers_mask ());

#if 0
/* This feature has been removed because it clashes with the
 * multi-selection (Shift/Ctrl are reserved), and we can't move it to
 * Alt+ because then it would clash with the alpha-to-selection features
 * (cf. gimp_item_tree_view_item_pre_clicked()).
 * Just macro-ing them out for now, just in case, but I don't think
 * there is a chance to revive them as there is no infinite modifiers.
 */
          GimpImage       *image;

          image = gimp_item_get_image (GIMP_ITEM (layer));

          if ((modifiers & GDK_MOD1_MASK))
            {
              /* Alternative functions (Alt key or equivalent) when
               * clicking on a layer preview. The actions happen only on
               * the clicked layer, not on selected layers.
               *
               * Note: Alt-click is already reserved for Alpha to
               * selection in GimpItemTreeView.
               */
              if (modifiers == (GDK_MOD1_MASK | GDK_SHIFT_MASK))
                {
                  /* Alt-Shift-click adds a layer mask with last values */
                  GimpDialogConfig *config;
                  GimpChannel      *channel = NULL;

                  if (! mask)
                    {
                      config = GIMP_DIALOG_CONFIG (image->gimp->config);

                      if (config->layer_add_mask_type == GIMP_ADD_MASK_CHANNEL)
                        {
                          channel = gimp_image_get_active_channel (image);

                          if (! channel)
                            {
                              GimpContainer *channels = gimp_image_get_channels (image);

                              channel = GIMP_CHANNEL (gimp_container_get_first_child (channels));
                            }

                          if (! channel)
                            {
                              /* No channel. We cannot perform the add
                               * mask action. */
                              g_message (_("No channels to create a layer mask from."));
                            }
                        }

                      if (config->layer_add_mask_type != GIMP_ADD_MASK_CHANNEL || channel)
                        {
                          mask = gimp_layer_create_mask (layer,
                                                         config->layer_add_mask_type,
                                                         channel);

                          if (config->layer_add_mask_invert)
                            gimp_channel_invert (GIMP_CHANNEL (mask), FALSE);

                          gimp_layer_add_mask (layer, mask, TRUE, NULL);
                          gimp_image_flush (image);
                        }
                    }
                }
              else if (modifiers == (GDK_MOD1_MASK | GDK_CONTROL_MASK))
                {
                  /* Alt-Control-click removes a layer mask */
                  if (mask)
                    {
                      gimp_layer_apply_mask (layer, GIMP_MASK_DISCARD, TRUE);
                      gimp_image_flush (image);
                    }
                }
              else if (modifiers == (GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK))
                {
                  /* Alt-Shift-Control-click applies a layer mask */
                  if (mask)
                    {
                      gimp_layer_apply_mask (layer, GIMP_MASK_APPLY, TRUE);
                      gimp_image_flush (image);
                    }
                }
            }
          else
#endif
          if (! modifiers)
            {
              /* Simple clicks (without modifiers) activate the layer */
              if (mask && gimp_layer_get_edit_mask (layer))
                gimp_action_group_set_action_active (group,
                                                     "layers-mask-edit", FALSE);
              else
                gimp_layer_tree_view_update_borders (layer_view, &iter);
            }

          g_object_unref (renderer);
        }
    }

  gtk_tree_path_free (path);
}

static void
gimp_layer_tree_view_mask_clicked (GimpCellRendererViewable *cell,
                                   const gchar              *path_str,
                                   GdkModifierType           state,
                                   GimpLayerTreeView        *layer_view)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (layer_view);
  GtkTreePath           *path;
  GtkTreeIter            iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpViewRenderer *renderer;
      GimpUIManager    *ui_manager;
      GimpActionGroup  *group;

      ui_manager = gimp_editor_get_ui_manager (GIMP_EDITOR (tree_view));
      group      = gimp_ui_manager_get_action_group (ui_manager, "layers");

      gtk_tree_model_get (tree_view->model, &iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      if (renderer)
        {
          GimpLayer       *layer     = GIMP_LAYER (renderer->viewable);
          GdkModifierType  modifiers = gimp_get_all_modifiers_mask ();

          if ((state & GDK_MOD1_MASK))
            {
              GimpImage *image;

              image = gimp_item_get_image (GIMP_ITEM (layer));

              if ((state & modifiers) == GDK_MOD1_MASK)
                {
                  /* Alt-click shows/hides a layer mask */
                  gimp_layer_set_show_mask (layer,
                                            ! gimp_layer_get_show_mask (layer),
                                            TRUE);
                  gimp_image_flush (image);
                }
              else if ((state & modifiers) == (GDK_MOD1_MASK | GDK_CONTROL_MASK))
                {
                  /* Alt-Control-click enables/disables a layer mask */
                  gimp_layer_set_apply_mask (layer,
                                             ! gimp_layer_get_apply_mask (layer),
                                             TRUE);
                  gimp_image_flush (image);
                }
            }
          else if (! gimp_layer_get_edit_mask (layer))
            {
              /* Simple click selects the mask for edition. */
              gimp_action_group_set_action_active (group, "layers-mask-edit",
                                                   TRUE);
            }

          g_object_unref (renderer);
        }
    }

  gtk_tree_path_free (path);
}


/*  GimpDrawable alpha callbacks  */

static void
gimp_layer_tree_view_alpha_update (GimpLayerTreeView *view,
                                   GtkTreeIter       *iter,
                                   GimpLayer         *layer)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);

  gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES,
                      gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)) ?
                      NULL : view->priv->bold_attrs,
                      -1);
}

static void
gimp_layer_tree_view_alpha_changed (GimpLayer         *layer,
                                    GimpLayerTreeView *layer_view)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (layer_view);
  GtkTreeIter       *iter;

  iter = _gimp_container_view_lookup (view, (GimpViewable *) layer);

  if (iter)
    {
      GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (view);
      GimpImage        *image;
      GList            *layers;

      gimp_layer_tree_view_alpha_update (layer_view, iter, layer);

      image  = gimp_item_tree_view_get_image (item_view);
      layers = gimp_image_get_selected_layers (image);

      /*  update button states  */
      if (g_list_find (layers, layer))
        gimp_container_view_set_selected (GIMP_CONTAINER_VIEW (view),
                                          layers);
    }
}
