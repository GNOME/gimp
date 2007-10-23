/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplayertreeview.c
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimplist.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpitemundo.h"

#include "text/gimptextlayer.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "gimpactiongroup.h"
#include "gimpcellrendererviewable.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimplayertreeview.h"
#include "gimpviewrenderer.h"
#include "gimpuimanager.h"
#include "gimpwidgets-constructors.h"

#include "gimp-intl.h"


static void  gimp_layer_tree_view_view_iface_init (GimpContainerViewInterface *iface);

static GObject * gimp_layer_tree_view_constructor (GType                type,
                                                   guint                n_params,
                                                   GObjectConstructParam *params);
static void   gimp_layer_tree_view_finalize       (GObject             *object);

static void   gimp_layer_tree_view_unrealize      (GtkWidget           *widget);
static void   gimp_layer_tree_view_style_set      (GtkWidget           *widget,
                                                   GtkStyle            *prev_style);

static void   gimp_layer_tree_view_set_container  (GimpContainerView   *view,
                                                   GimpContainer       *container);
static void   gimp_layer_tree_view_set_context    (GimpContainerView   *view,
                                                   GimpContext         *context);
static gpointer gimp_layer_tree_view_insert_item  (GimpContainerView   *view,
                                                   GimpViewable        *viewable,
                                                   gint                 index);
static gboolean gimp_layer_tree_view_select_item  (GimpContainerView   *view,
                                                   GimpViewable        *item,
                                                   gpointer             insert_data);
static void    gimp_layer_tree_view_set_view_size (GimpContainerView   *view);

static gboolean gimp_layer_tree_view_drop_possible(GimpContainerTreeView *view,
                                                   GimpDndType          src_type,
                                                   GimpViewable        *src_viewable,
                                                   GimpViewable        *dest_viewable,
                                                   GtkTreeViewDropPosition  drop_pos,
                                                   GtkTreeViewDropPosition *return_drop_pos,
                                                   GdkDragAction       *return_drag_action);
static void    gimp_layer_tree_view_drop_color    (GimpContainerTreeView *view,
                                                   const GimpRGB       *color,
                                                   GimpViewable        *dest_viewable,
                                                   GtkTreeViewDropPosition  drop_pos);
static void    gimp_layer_tree_view_drop_uri_list (GimpContainerTreeView *view,
                                                   GList               *uri_list,
                                                   GimpViewable        *dest_viewable,
                                                   GtkTreeViewDropPosition  drop_pos);
static void   gimp_layer_tree_view_drop_component (GimpContainerTreeView *tree_view,
                                                   GimpImage           *image,
                                                   GimpChannelType      component,
                                                   GimpViewable        *dest_viewable,
                                                   GtkTreeViewDropPosition  drop_pos);
static void   gimp_layer_tree_view_drop_pixbuf    (GimpContainerTreeView *tree_view,
                                                   GdkPixbuf           *pixbuf,
                                                   GimpViewable        *dest_viewable,
                                                   GtkTreeViewDropPosition  drop_pos);

static void   gimp_layer_tree_view_set_image      (GimpItemTreeView    *view,
                                                   GimpImage           *image);
static GimpItem * gimp_layer_tree_view_item_new   (GimpImage           *image);

static void   gimp_layer_tree_view_floating_selection_changed
                                                  (GimpImage           *image,
                                                   GimpLayerTreeView   *view);

static void   gimp_layer_tree_view_paint_mode_menu_callback
                                                  (GtkWidget           *widget,
                                                   GimpLayerTreeView   *view);
static void   gimp_layer_tree_view_opacity_scale_changed
                                                  (GtkAdjustment       *adj,
                                                   GimpLayerTreeView   *view);
static void   gimp_layer_tree_view_lock_alpha_button_toggled
                                                  (GtkWidget           *widget,
                                                   GimpLayerTreeView   *view);

static void   gimp_layer_tree_view_layer_signal_handler
                                                  (GimpLayer           *layer,
                                                   GimpLayerTreeView   *view);
static void   gimp_layer_tree_view_update_options (GimpLayerTreeView   *view,
                                                   GimpLayer           *layer);
static void   gimp_layer_tree_view_update_menu    (GimpLayerTreeView   *view,
                                                   GimpLayer           *layer);

static void   gimp_layer_tree_view_mask_update    (GimpLayerTreeView   *view,
                                                   GtkTreeIter         *iter,
                                                   GimpLayer           *layer);
static void   gimp_layer_tree_view_mask_changed   (GimpLayer           *layer,
                                                   GimpLayerTreeView   *view);
static void   gimp_layer_tree_view_renderer_update(GimpViewRenderer    *renderer,
                                                   GimpLayerTreeView   *view);

static void   gimp_layer_tree_view_update_borders (GimpLayerTreeView   *view,
                                                   GtkTreeIter         *iter);
static void   gimp_layer_tree_view_mask_callback  (GimpLayerMask       *mask,
                                                   GimpLayerTreeView   *view);
static void   gimp_layer_tree_view_layer_clicked  (GimpCellRendererViewable *cell,
                                                   const gchar         *path,
                                                   GdkModifierType      state,
                                                   GimpLayerTreeView   *view);
static void   gimp_layer_tree_view_mask_clicked   (GimpCellRendererViewable *cell,
                                                   const gchar         *path,
                                                   GdkModifierType      state,
                                                   GimpLayerTreeView   *view);

static void   gimp_layer_tree_view_alpha_update   (GimpLayerTreeView   *view,
                                                   GtkTreeIter         *iter,
                                                   GimpLayer           *layer);
static void   gimp_layer_tree_view_alpha_changed  (GimpLayer           *layer,
                                                   GimpLayerTreeView   *view);


G_DEFINE_TYPE_WITH_CODE (GimpLayerTreeView, gimp_layer_tree_view,
                         GIMP_TYPE_DRAWABLE_TREE_VIEW,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_layer_tree_view_view_iface_init))

#define parent_class gimp_layer_tree_view_parent_class

static GimpContainerViewInterface *parent_view_iface = NULL;


static void
gimp_layer_tree_view_class_init (GimpLayerTreeViewClass *klass)
{
  GObjectClass               *object_class;
  GtkWidgetClass             *widget_class;
  GimpContainerTreeViewClass *tree_view_class;
  GimpItemTreeViewClass      *item_view_class;

  object_class    = G_OBJECT_CLASS (klass);
  widget_class    = GTK_WIDGET_CLASS (klass);
  tree_view_class = GIMP_CONTAINER_TREE_VIEW_CLASS (klass);
  item_view_class = GIMP_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructor = gimp_layer_tree_view_constructor;
  object_class->finalize    = gimp_layer_tree_view_finalize;

  widget_class->unrealize   = gimp_layer_tree_view_unrealize;
  widget_class->style_set   = gimp_layer_tree_view_style_set;

  tree_view_class->drop_possible   = gimp_layer_tree_view_drop_possible;
  tree_view_class->drop_color      = gimp_layer_tree_view_drop_color;
  tree_view_class->drop_uri_list   = gimp_layer_tree_view_drop_uri_list;
  tree_view_class->drop_component  = gimp_layer_tree_view_drop_component;
  tree_view_class->drop_pixbuf     = gimp_layer_tree_view_drop_pixbuf;

  item_view_class->item_type       = GIMP_TYPE_LAYER;
  item_view_class->signal_name     = "active-layer-changed";

  item_view_class->set_image       = gimp_layer_tree_view_set_image;
  item_view_class->get_container   = gimp_image_get_layers;
  item_view_class->get_active_item = (GimpGetItemFunc) gimp_image_get_active_layer;
  item_view_class->set_active_item = (GimpSetItemFunc) gimp_image_set_active_layer;
  item_view_class->reorder_item    = (GimpReorderItemFunc) gimp_image_position_layer;
  item_view_class->add_item        = (GimpAddItemFunc) gimp_image_add_layer;
  item_view_class->remove_item     = (GimpRemoveItemFunc) gimp_image_remove_layer;
  item_view_class->new_item        = gimp_layer_tree_view_item_new;

  item_view_class->action_group        = "layers";
  item_view_class->activate_action     = "layers-text-tool";
  item_view_class->edit_action         = "layers-edit-attributes";
  item_view_class->new_action          = "layers-new";
  item_view_class->new_default_action  = "layers-new-last-values";
  item_view_class->raise_action        = "layers-raise";
  item_view_class->raise_top_action    = "layers-raise-to-top";
  item_view_class->lower_action        = "layers-lower";
  item_view_class->lower_bottom_action = "layers-lower-to-bottom";
  item_view_class->duplicate_action    = "layers-duplicate";
  item_view_class->delete_action       = "layers-delete";
  item_view_class->reorder_desc        = _("Reorder Layer");
}

static void
gimp_layer_tree_view_view_iface_init (GimpContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->set_container = gimp_layer_tree_view_set_container;
  iface->set_context   = gimp_layer_tree_view_set_context;
  iface->insert_item   = gimp_layer_tree_view_insert_item;
  iface->select_item   = gimp_layer_tree_view_select_item;
  iface->set_view_size = gimp_layer_tree_view_set_view_size;
}

static void
gimp_layer_tree_view_init (GimpLayerTreeView *view)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GtkWidget             *hbox;
  GtkWidget             *image;
  GtkIconSize            icon_size;
  PangoAttribute        *attr;

  /* The following used to read:
   *
   * tree_view->model_columns[tree_view->n_model_columns++] = ...
   *
   * but combining the two lead to gcc miscompiling the function on ppc/ia64
   * (model_column_mask and model_column_mask_visible would have the same
   * value, probably due to bad instruction reordering). See bug #113144 for
   * more info.
   */
  view->model_column_mask = tree_view->n_model_columns;
  tree_view->model_columns[tree_view->n_model_columns] = GIMP_TYPE_VIEW_RENDERER;
  tree_view->n_model_columns++;

  view->model_column_mask_visible = tree_view->n_model_columns;
  tree_view->model_columns[tree_view->n_model_columns] = G_TYPE_BOOLEAN;
  tree_view->n_model_columns++;

  view->options_box = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (view->options_box), 2);
  gtk_box_pack_start (GTK_BOX (view), view->options_box, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (view), view->options_box, 0);
  gtk_widget_show (view->options_box);

  /*  Paint mode menu  */

  view->paint_mode_menu = gimp_paint_mode_menu_new (FALSE, FALSE);
  gimp_table_attach_aligned (GTK_TABLE (view->options_box), 0, 0,
                             _("Mode:"), 0.0, 0.5,
                             view->paint_mode_menu, 2, FALSE);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (view->paint_mode_menu),
                              GIMP_NORMAL_MODE,
                              G_CALLBACK (gimp_layer_tree_view_paint_mode_menu_callback),
                              view);

  gimp_help_set_help_data (view->paint_mode_menu, NULL,
                           GIMP_HELP_LAYER_DIALOG_PAINT_MODE_MENU);

  /*  Opacity scale  */

  view->opacity_adjustment =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (view->options_box), 0, 1,
                                          _("Opacity:"), -1, -1,
                                          100.0, 0.0, 100.0, 1.0, 10.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL,
                                          GIMP_HELP_LAYER_DIALOG_OPACITY_SCALE));

  g_signal_connect (view->opacity_adjustment, "value-changed",
                    G_CALLBACK (gimp_layer_tree_view_opacity_scale_changed),
                    view);

  /*  Lock alpha toggle  */

  hbox = gtk_hbox_new (FALSE, 6);

  view->lock_alpha_toggle = gtk_check_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), view->lock_alpha_toggle, FALSE, FALSE, 0);
  gtk_widget_show (view->lock_alpha_toggle);

  g_signal_connect (view->lock_alpha_toggle, "toggled",
                    G_CALLBACK (gimp_layer_tree_view_lock_alpha_button_toggled),
                    view);

  gimp_help_set_help_data (view->lock_alpha_toggle, _("Lock alpha channel"),
                           GIMP_HELP_LAYER_DIALOG_LOCK_ALPHA_BUTTON);

  gtk_widget_style_get (GTK_WIDGET (view),
                        "button-icon-size", &icon_size,
                        NULL);

  image = gtk_image_new_from_stock (GIMP_STOCK_TRANSPARENCY, icon_size);
  gtk_container_add (GTK_CONTAINER (view->lock_alpha_toggle), image);
  gtk_widget_show (image);

  gimp_table_attach_aligned (GTK_TABLE (view->options_box), 0, 2,
                             _("Lock:"), 0.0, 0.5,
                             hbox, 2, FALSE);

  gtk_widget_set_sensitive (view->options_box, FALSE);

  view->italic_attrs = pango_attr_list_new ();
  attr = pango_attr_style_new (PANGO_STYLE_ITALIC);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (view->italic_attrs, attr);

  view->bold_attrs = pango_attr_list_new ();
  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (view->bold_attrs, attr);

  view->mode_changed_handler_id       = 0;
  view->opacity_changed_handler_id    = 0;
  view->lock_alpha_changed_handler_id = 0;
  view->mask_changed_handler_id       = 0;
}

static GObject *
gimp_layer_tree_view_constructor (GType                  type,
                                  guint                  n_params,
                                  GObjectConstructParam *params)
{
  GimpContainerTreeView *tree_view;
  GimpLayerTreeView     *layer_view;
  GtkWidget             *button;
  GObject               *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  tree_view  = GIMP_CONTAINER_TREE_VIEW (object);
  layer_view = GIMP_LAYER_TREE_VIEW (object);

  layer_view->mask_cell = gimp_cell_renderer_viewable_new ();
  gtk_tree_view_column_pack_start (tree_view->main_column,
                                   layer_view->mask_cell,
                                   FALSE);
  gtk_tree_view_column_set_attributes (tree_view->main_column,
                                       layer_view->mask_cell,
                                       "renderer",
                                       layer_view->model_column_mask,
                                       "visible",
                                       layer_view->model_column_mask_visible,
                                       NULL);

  tree_view->renderer_cells = g_list_prepend (tree_view->renderer_cells,
                                              layer_view->mask_cell);

  g_signal_connect (tree_view->renderer_cell, "clicked",
                    G_CALLBACK (gimp_layer_tree_view_layer_clicked),
                    layer_view);
  g_signal_connect (layer_view->mask_cell, "clicked",
                    G_CALLBACK (gimp_layer_tree_view_mask_clicked),
                    layer_view);

  gimp_dnd_uri_list_dest_add  (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);
  gimp_dnd_component_dest_add (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);
  gimp_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), GIMP_TYPE_CHANNEL,
                               NULL, tree_view);
  gimp_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), GIMP_TYPE_LAYER_MASK,
                               NULL, tree_view);
  gimp_dnd_pixbuf_dest_add    (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);

  /*  hide basically useless edit button  */
  gtk_widget_hide (GIMP_ITEM_TREE_VIEW (layer_view)->edit_button);

  button = gimp_editor_add_action_button (GIMP_EDITOR (layer_view), "layers",
                                          "layers-anchor", NULL);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (layer_view),
                                  GTK_BUTTON (button),
                                  GIMP_TYPE_LAYER);
  gtk_box_reorder_child (GTK_BOX (GIMP_EDITOR (layer_view)->button_box),
                         button, 5);

  return object;
}

static void
gimp_layer_tree_view_finalize (GObject *object)
{
  GimpLayerTreeView *layer_view = GIMP_LAYER_TREE_VIEW (object);

  if (layer_view->italic_attrs)
    {
      pango_attr_list_unref (layer_view->italic_attrs);
      layer_view->italic_attrs = NULL;
    }

  if (layer_view->bold_attrs)
    {
      pango_attr_list_unref (layer_view->bold_attrs);
      layer_view->bold_attrs = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_layer_tree_view_unrealize (GtkWidget *widget)
{
  GimpContainerTreeView *tree_view  = GIMP_CONTAINER_TREE_VIEW (widget);
  GimpLayerTreeView     *layer_view = GIMP_LAYER_TREE_VIEW (widget);
  GtkTreeIter            iter;
  gboolean               iter_valid;

  for (iter_valid = gtk_tree_model_get_iter_first (tree_view->model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (tree_view->model, &iter))
    {
      GimpViewRenderer *renderer;

      gtk_tree_model_get (tree_view->model, &iter,
                          layer_view->model_column_mask, &renderer,
                          -1);

      if (renderer)
        {
          gimp_view_renderer_unrealize (renderer);
          g_object_unref (renderer);
        }
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_layer_tree_view_style_set (GtkWidget *widget,
                                GtkStyle  *prev_style)
{
  GimpLayerTreeView *layer_view = GIMP_LAYER_TREE_VIEW (widget);
  gint               content_spacing;
  gint               button_spacing;

  gtk_widget_style_get (widget,
                        "content-spacing", &content_spacing,
                        "button-spacing",  &button_spacing,
                        NULL);

  gtk_table_set_col_spacings (GTK_TABLE (layer_view->options_box),
                              button_spacing);
  gtk_table_set_row_spacings (GTK_TABLE (layer_view->options_box),
                              content_spacing);

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
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
      gimp_container_remove_handler (old_container,
                                     layer_view->mode_changed_handler_id);
      gimp_container_remove_handler (old_container,
                                     layer_view->opacity_changed_handler_id);
      gimp_container_remove_handler (old_container,
                                     layer_view->lock_alpha_changed_handler_id);
      gimp_container_remove_handler (old_container,
                                     layer_view->mask_changed_handler_id);
      gimp_container_remove_handler (old_container,
                                     layer_view->alpha_changed_handler_id);
    }

  parent_view_iface->set_container (view, container);

  if (container)
    {
      layer_view->mode_changed_handler_id =
        gimp_container_add_handler (container, "mode-changed",
                                    G_CALLBACK (gimp_layer_tree_view_layer_signal_handler),
                                    view);
      layer_view->opacity_changed_handler_id =
        gimp_container_add_handler (container, "opacity-changed",
                                    G_CALLBACK (gimp_layer_tree_view_layer_signal_handler),
                                    view);
      layer_view->lock_alpha_changed_handler_id =
        gimp_container_add_handler (container, "lock-alpha-changed",
                                    G_CALLBACK (gimp_layer_tree_view_layer_signal_handler),
                                    view);
      layer_view->mask_changed_handler_id =
        gimp_container_add_handler (container, "mask-changed",
                                    G_CALLBACK (gimp_layer_tree_view_mask_changed),
                                    view);
      layer_view->alpha_changed_handler_id =
        gimp_container_add_handler (container, "alpha-changed",
                                    G_CALLBACK (gimp_layer_tree_view_alpha_changed),
                                    view);
    }
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
      GtkTreeIter iter;
      gboolean    iter_valid;

      for (iter_valid = gtk_tree_model_get_iter_first (tree_view->model, &iter);
           iter_valid;
           iter_valid = gtk_tree_model_iter_next (tree_view->model, &iter))
        {
          GimpViewRenderer *renderer;

          gtk_tree_model_get (tree_view->model, &iter,
                              layer_view->model_column_mask, &renderer,
                              -1);

          if (renderer)
            {
              gimp_view_renderer_set_context (renderer, context);
              g_object_unref (renderer);
            }
        }
    }
}

static gpointer
gimp_layer_tree_view_insert_item (GimpContainerView *view,
                                  GimpViewable      *viewable,
                                  gint               index)
{
  GimpLayerTreeView *layer_view = GIMP_LAYER_TREE_VIEW (view);
  GimpLayer         *layer;
  GtkTreeIter       *iter;

  iter = parent_view_iface->insert_item (view, viewable, index);

  layer = GIMP_LAYER (viewable);

  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    gimp_layer_tree_view_alpha_update (layer_view, iter, layer);

  gimp_layer_tree_view_mask_update (layer_view, iter, layer);

  return iter;
}

static gboolean
gimp_layer_tree_view_select_item (GimpContainerView *view,
                                  GimpViewable      *item,
                                  gpointer           insert_data)
{
  GimpItemTreeView  *item_view         = GIMP_ITEM_TREE_VIEW (view);
  GimpLayerTreeView *layer_view        = GIMP_LAYER_TREE_VIEW (view);
  gboolean           options_sensitive = FALSE;
  gboolean           success;

  success = parent_view_iface->select_item (view, item, insert_data);

  if (item)
    {
      if (success)
        {
          gimp_layer_tree_view_update_borders (layer_view,
                                               (GtkTreeIter *) insert_data);
          gimp_layer_tree_view_update_options (layer_view, GIMP_LAYER (item));
          gimp_layer_tree_view_update_menu (layer_view, GIMP_LAYER (item));
        }

      options_sensitive = TRUE;

      if (! success || gimp_layer_is_floating_sel (GIMP_LAYER (item)))
        {
          gtk_widget_set_sensitive (item_view->edit_button,  FALSE);
        }
    }

  gtk_widget_set_sensitive (layer_view->options_box, options_sensitive);

  return success;
}

static void
gimp_layer_tree_view_set_view_size (GimpContainerView *view)
{
  GimpContainerTreeView *tree_view  = GIMP_CONTAINER_TREE_VIEW (view);

  if (tree_view->model)
    {
      GimpLayerTreeView *layer_view = GIMP_LAYER_TREE_VIEW (view);
      GtkTreeIter        iter;
      gboolean           iter_valid;
      gint               view_size;
      gint               border_width;

      view_size = gimp_container_view_get_view_size (view, &border_width);

      for (iter_valid = gtk_tree_model_get_iter_first (tree_view->model, &iter);
           iter_valid;
           iter_valid = gtk_tree_model_iter_next (tree_view->model, &iter))
        {
          GimpViewRenderer *renderer;

          gtk_tree_model_get (tree_view->model, &iter,
                              layer_view->model_column_mask, &renderer,
                              -1);

          if (renderer)
            {
              gimp_view_renderer_set_size (renderer, view_size, border_width);
              g_object_unref (renderer);
            }
        }
    }

  parent_view_iface->set_view_size (view);
}


/*  GimpContainerTreeView methods  */

static gboolean
gimp_layer_tree_view_drop_possible (GimpContainerTreeView   *tree_view,
                                    GimpDndType              src_type,
                                    GimpViewable            *src_viewable,
                                    GimpViewable            *dest_viewable,
                                    GtkTreeViewDropPosition  drop_pos,
                                    GtkTreeViewDropPosition *return_drop_pos,
                                    GdkDragAction           *return_drag_action)
{
  /* If we are dropping a new layer, check if the destionation image
   * has a floating selection.
   */
  if  (src_type == GIMP_DND_TYPE_URI_LIST     ||
       src_type == GIMP_DND_TYPE_TEXT_PLAIN   ||
       src_type == GIMP_DND_TYPE_NETSCAPE_URL ||
       src_type == GIMP_DND_TYPE_COMPONENT    ||
       src_type == GIMP_DND_TYPE_PIXBUF       ||
       GIMP_IS_DRAWABLE (src_viewable))
    {
      GimpImage *dest_image = GIMP_ITEM_TREE_VIEW (tree_view)->image;

      if (gimp_image_floating_sel (dest_image))
        return FALSE;
    }

  return GIMP_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_possible (tree_view,
                                                                       src_type,
                                                                       src_viewable,
                                                                       dest_viewable,
                                                                       drop_pos,
                                                                       return_drop_pos,
                                                                       return_drag_action);
}

static void
gimp_layer_tree_view_drop_color (GimpContainerTreeView   *view,
                                 const GimpRGB           *color,
                                 GimpViewable            *dest_viewable,
                                 GtkTreeViewDropPosition  drop_pos)
{
  if (gimp_drawable_is_text_layer (GIMP_DRAWABLE (dest_viewable)))
    {
      gimp_text_layer_set (GIMP_TEXT_LAYER (dest_viewable), NULL,
                           "color", color,
                           NULL);
      gimp_image_flush (GIMP_ITEM_TREE_VIEW (view)->image);
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
  GimpImage         *image     = item_view->image;
  gint               index     = -1;
  GList             *list;

  if (dest_viewable)
    {
      index = gimp_image_get_layer_index (image, GIMP_LAYER (dest_viewable));

      if (drop_pos == GTK_TREE_VIEW_DROP_AFTER)
        index++;
    }

  for (list = uri_list; list; list = g_list_next (list))
    {
      const gchar       *uri   = list->data;
      GList             *new_layers;
      GimpPDBStatusType  status;
      GError            *error = NULL;

      new_layers = file_open_layers (image->gimp,
                                     gimp_container_view_get_context (cont_view),
                                     NULL,
                                     image, FALSE,
                                     uri, GIMP_RUN_INTERACTIVE, NULL,
                                     &status, &error);

      if (new_layers)
        {
          gimp_image_add_layers (image, new_layers, index,
                                 0, 0,
                                 gimp_image_get_width (image),
                                 gimp_image_get_height (image),
                                 _("Drop layers"));

          index += g_list_length (new_layers);

          g_list_free (new_layers);
        }
      else if (status != GIMP_PDB_CANCEL)
        {
          gchar *filename = file_utils_uri_display_name (uri);

          gimp_message (image->gimp, G_OBJECT (view), GIMP_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        filename, error->message);

          g_clear_error (&error);
          g_free (filename);
        }
    }

  gimp_image_flush (image);
}

static void
gimp_layer_tree_view_drop_component (GimpContainerTreeView   *tree_view,
                                     GimpImage               *src_image,
                                     GimpChannelType          component,
                                     GimpViewable            *dest_viewable,
                                     GtkTreeViewDropPosition  drop_pos)
{
  GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (tree_view);
  GimpChannel      *channel;
  GimpItem         *new_item;
  const gchar      *desc;
  gint              index = -1;

  if (dest_viewable)
    {
      index = gimp_image_get_layer_index (item_view->image,
                                          GIMP_LAYER (dest_viewable));

      if (drop_pos == GTK_TREE_VIEW_DROP_AFTER)
        index++;
    }

  channel = gimp_channel_new_from_component (src_image, component, NULL, NULL);

  new_item = gimp_item_convert (GIMP_ITEM (channel), item_view->image,
                                GIMP_TYPE_LAYER, TRUE);

  g_object_unref (channel);

  gimp_enum_get_value (GIMP_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);
  gimp_object_take_name (GIMP_OBJECT (new_item),
                         g_strdup_printf (_("%s Channel Copy"), desc));

  gimp_image_add_layer (item_view->image, GIMP_LAYER (new_item), index);
  gimp_image_flush (item_view->image);
}

static void
gimp_layer_tree_view_drop_pixbuf (GimpContainerTreeView   *tree_view,
                                  GdkPixbuf               *pixbuf,
                                  GimpViewable            *dest_viewable,
                                  GtkTreeViewDropPosition  drop_pos)
{
  GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (tree_view);
  GimpImage        *image     = item_view->image;
  GimpLayer        *new_layer;
  gint              index     = -1;

  if (dest_viewable)
    {
      index = gimp_image_get_layer_index (image, GIMP_LAYER (dest_viewable));

      if (drop_pos == GTK_TREE_VIEW_DROP_AFTER)
        index++;
    }

  new_layer =
    gimp_layer_new_from_pixbuf (pixbuf, image,
                                gimp_image_base_type_with_alpha (image),
                                _("Dropped Buffer"),
                                GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

  gimp_image_add_layer (image, new_layer, index);
  gimp_image_flush (image);
}


/*  GimpItemTreeView methods  */

static void
gimp_layer_tree_view_set_image (GimpItemTreeView *view,
                                GimpImage        *image)
{
  if (view->image)
    g_signal_handlers_disconnect_by_func (view->image,
                                          gimp_layer_tree_view_floating_selection_changed,
                                          view);

  GIMP_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (view, image);

  if (view->image)
    g_signal_connect (view->image,
                      "floating-selection-changed",
                      G_CALLBACK (gimp_layer_tree_view_floating_selection_changed),
                      view);
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
                              gimp_image_base_type_with_alpha (image),
                              _("Empty Layer"), 1.0, GIMP_NORMAL_MODE);

  gimp_image_add_layer (image, new_layer, -1);

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

  floating_sel = gimp_image_floating_sel (image);

  if (floating_sel)
    {
      iter = gimp_container_view_lookup (view, (GimpViewable *) floating_sel);

      if (iter)
        gtk_list_store_set (GTK_LIST_STORE (tree_view->model), iter,
                            tree_view->model_column_name_attributes,
                            layer_view->italic_attrs,
                            -1);
    }
  else
    {
      GList *list;

      for (list = GIMP_LIST (image->layers)->list;
           list;
           list = g_list_next (list))
        {
          GimpDrawable *drawable = list->data;

          iter = gimp_container_view_lookup (view, (GimpViewable *) drawable);

          if (iter)
            gtk_list_store_set (GTK_LIST_STORE (tree_view->model), iter,
                                tree_view->model_column_name_attributes,
                                gimp_drawable_has_alpha (drawable) ?
                                NULL : layer_view->bold_attrs,
                                -1);
        }
    }
}


/*  Paint Mode, Opacity and Lock alpha callbacks  */

#define BLOCK() \
        g_signal_handlers_block_by_func (layer, \
        gimp_layer_tree_view_layer_signal_handler, view)

#define UNBLOCK() \
        g_signal_handlers_unblock_by_func (layer, \
        gimp_layer_tree_view_layer_signal_handler, view)


static void
gimp_layer_tree_view_paint_mode_menu_callback (GtkWidget         *widget,
                                               GimpLayerTreeView *view)
{
  GimpImage *image;
  GimpLayer *layer = NULL;

  image = GIMP_ITEM_TREE_VIEW (view)->image;

  if (image)
    layer = (GimpLayer *)
      GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (image);

  if (layer)
    {
      gint mode;

      if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget),
                                         &mode) &&
          gimp_layer_get_mode (layer) != (GimpLayerModeEffects) mode)
        {
          GimpUndo *undo;
          gboolean  push_undo = TRUE;

          /*  compress layer mode undos  */
          undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                               GIMP_UNDO_LAYER_MODE);

          if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
            push_undo = FALSE;

          BLOCK();
          gimp_layer_set_mode (layer, (GimpLayerModeEffects) mode, push_undo);
          UNBLOCK();

          gimp_image_flush (image);

          if (! push_undo)
            gimp_undo_refresh_preview (undo, gimp_container_view_get_context (GIMP_CONTAINER_VIEW (view)));
        }
    }
}

static void
gimp_layer_tree_view_lock_alpha_button_toggled (GtkWidget         *widget,
                                                GimpLayerTreeView *view)
{
  GimpImage *image;
  GimpLayer *layer;

  image = GIMP_ITEM_TREE_VIEW (view)->image;

  layer = (GimpLayer *)
    GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (image);

  if (layer)
    {
      gboolean lock_alpha;

      lock_alpha = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

      if (gimp_layer_get_lock_alpha (layer) != lock_alpha)
        {
          GimpUndo *undo;
          gboolean  push_undo = TRUE;

          /*  compress opacity undos  */
          undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                               GIMP_UNDO_LAYER_LOCK_ALPHA);

          if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
            push_undo = FALSE;

          BLOCK();
          gimp_layer_set_lock_alpha (layer, lock_alpha, push_undo);
          UNBLOCK();

          gimp_image_flush (image);
        }
    }
}

static void
gimp_layer_tree_view_opacity_scale_changed (GtkAdjustment     *adjustment,
                                            GimpLayerTreeView *view)
{
  GimpImage *image;
  GimpLayer *layer;

  image = GIMP_ITEM_TREE_VIEW (view)->image;

  layer = (GimpLayer *)
    GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (image);

  if (layer)
    {
      gdouble opacity = adjustment->value / 100.0;

      if (gimp_layer_get_opacity (layer) != opacity)
        {
          GimpUndo *undo;
          gboolean  push_undo = TRUE;

          /*  compress opacity undos  */
          undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                               GIMP_UNDO_LAYER_OPACITY);

          if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
            push_undo = FALSE;

          BLOCK();
          gimp_layer_set_opacity (layer, opacity, push_undo);
          UNBLOCK();

          gimp_image_flush (image);

          if (! push_undo)
            gimp_undo_refresh_preview (undo, gimp_container_view_get_context (GIMP_CONTAINER_VIEW (view)));
        }
    }
}

#undef BLOCK
#undef UNBLOCK


static void
gimp_layer_tree_view_layer_signal_handler (GimpLayer         *layer,
                                           GimpLayerTreeView *view)
{
  GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (view);
  GimpLayer        *active_layer;

  active_layer = (GimpLayer *)
    GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (item_view->image);

  if (active_layer == layer)
    gimp_layer_tree_view_update_options (view, layer);
}


#define BLOCK(object,function) \
        g_signal_handlers_block_by_func ((object), (function), view)

#define UNBLOCK(object,function) \
        g_signal_handlers_unblock_by_func ((object), (function), view)

static void
gimp_layer_tree_view_update_options (GimpLayerTreeView *view,
                                     GimpLayer         *layer)
{
  BLOCK (view->paint_mode_menu,
         gimp_layer_tree_view_paint_mode_menu_callback);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (view->paint_mode_menu),
                                 layer->mode);

  UNBLOCK (view->paint_mode_menu,
           gimp_layer_tree_view_paint_mode_menu_callback);

  if (layer->lock_alpha !=
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (view->lock_alpha_toggle)))
    {
      BLOCK (view->lock_alpha_toggle,
             gimp_layer_tree_view_lock_alpha_button_toggled);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (view->lock_alpha_toggle),
                                    layer->lock_alpha);

      UNBLOCK (view->lock_alpha_toggle,
               gimp_layer_tree_view_lock_alpha_button_toggled);
    }

  if (layer->opacity * 100.0 != view->opacity_adjustment->value)
    {
      BLOCK (view->opacity_adjustment,
             gimp_layer_tree_view_opacity_scale_changed);

      gtk_adjustment_set_value (view->opacity_adjustment,
                                layer->opacity * 100.0);

      UNBLOCK (view->opacity_adjustment,
               gimp_layer_tree_view_opacity_scale_changed);
    }
}

#undef BLOCK
#undef UNBLOCK


static void
gimp_layer_tree_view_update_menu (GimpLayerTreeView *layer_view,
                                  GimpLayer         *layer)
{
  GimpUIManager   *ui_manager = GIMP_EDITOR (layer_view)->ui_manager;
  GimpActionGroup *group;
  GimpLayerMask   *mask;

  group = gimp_ui_manager_get_action_group (ui_manager, "layers");

  mask = gimp_layer_get_mask (layer);

  gimp_action_group_set_action_active (group, "layers-mask-show",
                                       mask &&
                                       gimp_layer_mask_get_show (mask));
  gimp_action_group_set_action_active (group, "layers-mask-disable",
                                       mask &&
                                       ! gimp_layer_mask_get_apply (mask));
  gimp_action_group_set_action_active (group, "layers-mask-edit",
                                       mask &&
                                       gimp_layer_mask_get_edit (mask));
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
      g_signal_connect_closure (mask, "apply-changed", closure, FALSE);
      g_signal_connect_closure (mask, "edit-changed",  closure, FALSE);
      g_signal_connect_closure (mask, "show-changed",  closure, FALSE);
    }

  gtk_list_store_set (GTK_LIST_STORE (tree_view->model), iter,
                      layer_view->model_column_mask,         renderer,
                      layer_view->model_column_mask_visible, mask_visible,
                      -1);

  if (renderer)
    {
      gimp_layer_tree_view_update_borders (layer_view, iter);
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

  iter = gimp_container_view_lookup (view, GIMP_VIEWABLE (layer));

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

  iter = gimp_container_view_lookup (view, (GimpViewable *)
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
  GimpLayerMask         *mask       = NULL;
  GimpViewBorderType     layer_type = GIMP_VIEW_BORDER_BLACK;

  gtk_tree_model_get (tree_view->model, iter,
                      tree_view->model_column_renderer, &layer_renderer,
                      layer_view->model_column_mask,    &mask_renderer,
                      -1);

  if (mask_renderer)
    mask = GIMP_LAYER_MASK (mask_renderer->viewable);

  if (! mask || (mask && ! gimp_layer_mask_get_edit (mask)))
    layer_type = GIMP_VIEW_BORDER_WHITE;

  gimp_view_renderer_set_border_type (layer_renderer, layer_type);

  if (mask)
    {
      GimpViewBorderType mask_color = GIMP_VIEW_BORDER_BLACK;

      if (gimp_layer_mask_get_show (mask))
        {
          mask_color = GIMP_VIEW_BORDER_GREEN;
        }
      else if (! gimp_layer_mask_get_apply (mask))
        {
          mask_color = GIMP_VIEW_BORDER_RED;
        }
      else if (gimp_layer_mask_get_edit (mask))
        {
          mask_color = GIMP_VIEW_BORDER_WHITE;
        }

      gimp_view_renderer_set_border_type (mask_renderer, mask_color);
    }

  if (layer_renderer)
    g_object_unref (layer_renderer);

  if (mask_renderer)
    g_object_unref (mask_renderer);
}

static void
gimp_layer_tree_view_mask_callback (GimpLayerMask     *mask,
                                    GimpLayerTreeView *layer_view)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (layer_view);
  GtkTreeIter       *iter;

  iter = gimp_container_view_lookup (view, (GimpViewable *)
                                     gimp_layer_mask_get_layer (mask));

  gimp_layer_tree_view_update_borders (layer_view, iter);
}

static void
gimp_layer_tree_view_layer_clicked (GimpCellRendererViewable *cell,
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

      ui_manager = GIMP_EDITOR (tree_view)->ui_manager;
      group      = gimp_ui_manager_get_action_group (ui_manager, "layers");

      gtk_tree_model_get (tree_view->model, &iter,
                          layer_view->model_column_mask, &renderer,
                          -1);

      if (renderer)
        {
          GimpLayerMask *mask = GIMP_LAYER_MASK (renderer->viewable);

          if (gimp_layer_mask_get_edit (mask))
            gimp_action_group_set_action_active (group,
                                                 "layers-mask-edit", FALSE);
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

      ui_manager = GIMP_EDITOR (tree_view)->ui_manager;
      group      = gimp_ui_manager_get_action_group (ui_manager, "layers");

      gtk_tree_model_get (tree_view->model, &iter,
                          layer_view->model_column_mask, &renderer,
                          -1);

      if (renderer)
        {
          GimpLayerMask *mask = GIMP_LAYER_MASK (renderer->viewable);

          if (state & GDK_MOD1_MASK)
            gimp_action_group_set_action_active (group, "layers-mask-show",
                                                 ! gimp_layer_mask_get_show (mask));
          else if (state & GDK_CONTROL_MASK)
            gimp_action_group_set_action_active (group, "layers-mask-disable",
                                                 gimp_layer_mask_get_apply (mask));
          else if (! gimp_layer_mask_get_edit (mask))
            gimp_action_group_set_action_active (group,
                                                 "layers-mask-edit", TRUE);

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

  gtk_list_store_set (GTK_LIST_STORE (tree_view->model), iter,
                      tree_view->model_column_name_attributes,
                      gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)) ?
                      NULL : view->bold_attrs,
                      -1);
}

static void
gimp_layer_tree_view_alpha_changed (GimpLayer         *layer,
                                    GimpLayerTreeView *layer_view)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (layer_view);
  GtkTreeIter       *iter;

  iter = gimp_container_view_lookup (view, (GimpViewable *) layer);

  if (iter)
    {
      GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (view);

      gimp_layer_tree_view_alpha_update (layer_view, iter, layer);

      /*  update button states  */
      if (gimp_image_get_active_layer (item_view->image) == layer)
        gimp_container_view_select_item (GIMP_CONTAINER_VIEW (view),
                                         GIMP_VIEWABLE (layer));
    }
}
