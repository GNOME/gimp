/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpitemtreeview.c
 * Copyright (C) 2001-2011 Michael Natterer <mitch@gimp.org>
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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpitem-exclusive.h"
#include "core/gimpitemundo.h"
#include "core/gimpmarshal.h"
#include "core/gimptreehandler.h"
#include "core/gimpundostack.h"

#include "vectors/gimpvectors.h"

#include "gimpaction.h"
#include "gimpcontainertreestore.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimpdocked.h"
#include "gimpitemtreeview.h"
#include "gimpmenufactory.h"
#include "gimpviewrenderer.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  SET_IMAGE,
  LAST_SIGNAL
};


struct _GimpItemTreeViewPrivate
{
  GimpImage       *image;

  GtkWidget       *options_box;
  GtkSizeGroup    *options_group;
  GtkWidget       *lock_box;

  GtkWidget       *lock_content_toggle;
  GtkWidget       *lock_position_toggle;

  GtkWidget       *new_button;
  GtkWidget       *raise_button;
  GtkWidget       *lower_button;
  GtkWidget       *duplicate_button;
  GtkWidget       *delete_button;

  gint             model_column_visible;
  gint             model_column_viewable;
  gint             model_column_linked;
  gint             model_column_color_tag;
  GtkCellRenderer *eye_cell;
  GtkCellRenderer *chain_cell;

  GimpTreeHandler *visible_changed_handler;
  GimpTreeHandler *linked_changed_handler;
  GimpTreeHandler *color_tag_changed_handler;
  GimpTreeHandler *lock_content_changed_handler;
  GimpTreeHandler *lock_position_changed_handler;

  gboolean         inserting_item; /* EEK */
};


static void   gimp_item_tree_view_view_iface_init   (GimpContainerViewInterface *view_iface);
static void   gimp_item_tree_view_docked_iface_init (GimpDockedInterface *docked_iface);

static void   gimp_item_tree_view_constructed       (GObject           *object);
static void   gimp_item_tree_view_dispose           (GObject           *object);

static void   gimp_item_tree_view_style_set         (GtkWidget         *widget,
                                                     GtkStyle          *prev_style);

static void   gimp_item_tree_view_real_set_image    (GimpItemTreeView  *view,
                                                     GimpImage         *image);

static void   gimp_item_tree_view_image_flush       (GimpImage         *image,
                                                     gboolean           invalidate_preview,
                                                     GimpItemTreeView  *view);

static void   gimp_item_tree_view_set_container     (GimpContainerView *view,
                                                     GimpContainer     *container);
static void   gimp_item_tree_view_set_context       (GimpContainerView *view,
                                                     GimpContext       *context);

static gpointer gimp_item_tree_view_insert_item     (GimpContainerView *view,
                                                     GimpViewable      *viewable,
                                                     gpointer           parent_insert_data,
                                                     gint               index);
static void   gimp_item_tree_view_insert_item_after (GimpContainerView *view,
                                                     GimpViewable      *viewable,
                                                     gpointer           insert_data);
static gboolean gimp_item_tree_view_select_item     (GimpContainerView *view,
                                                     GimpViewable      *item,
                                                     gpointer           insert_data);
static void   gimp_item_tree_view_activate_item     (GimpContainerView *view,
                                                     GimpViewable      *item,
                                                     gpointer           insert_data);
static void   gimp_item_tree_view_context_item      (GimpContainerView *view,
                                                     GimpViewable      *item,
                                                     gpointer           insert_data);

static gboolean gimp_item_tree_view_drop_possible   (GimpContainerTreeView *view,
                                                     GimpDndType        src_type,
                                                     GimpViewable      *src_viewable,
                                                     GimpViewable      *dest_viewable,
                                                     GtkTreePath       *drop_path,
                                                     GtkTreeViewDropPosition  drop_pos,
                                                     GtkTreeViewDropPosition *return_drop_pos,
                                                     GdkDragAction     *return_drag_action);
static void     gimp_item_tree_view_drop_viewable   (GimpContainerTreeView *view,
                                                     GimpViewable      *src_viewable,
                                                     GimpViewable      *dest_viewable,
                                                     GtkTreeViewDropPosition  drop_pos);

static void   gimp_item_tree_view_new_dropped       (GtkWidget         *widget,
                                                     gint               x,
                                                     gint               y,
                                                     GimpViewable      *viewable,
                                                     gpointer           data);

static void   gimp_item_tree_view_item_changed      (GimpImage         *image,
                                                     GimpItemTreeView  *view);
static void   gimp_item_tree_view_size_changed      (GimpImage         *image,
                                                     GimpItemTreeView  *view);

static void   gimp_item_tree_view_name_edited       (GtkCellRendererText *cell,
                                                     const gchar       *path,
                                                     const gchar       *new_name,
                                                     GimpItemTreeView  *view);

static void   gimp_item_tree_view_visible_changed      (GimpItem          *item,
                                                        GimpItemTreeView  *view);
static void   gimp_item_tree_view_linked_changed       (GimpItem          *item,
                                                        GimpItemTreeView  *view);
static void   gimp_item_tree_view_color_tag_changed    (GimpItem          *item,
                                                        GimpItemTreeView  *view);
static void   gimp_item_tree_view_lock_content_changed (GimpItem          *item,
                                                        GimpItemTreeView  *view);
static void   gimp_item_tree_view_lock_position_changed(GimpItem          *item,
                                                        GimpItemTreeView  *view);

static void   gimp_item_tree_view_eye_clicked       (GtkCellRendererToggle *toggle,
                                                     gchar             *path,
                                                     GdkModifierType    state,
                                                     GimpItemTreeView  *view);
static void   gimp_item_tree_view_chain_clicked     (GtkCellRendererToggle *toggle,
                                                     gchar             *path,
                                                     GdkModifierType    state,
                                                     GimpItemTreeView  *view);
static void   gimp_item_tree_view_lock_content_toggled
                                                    (GtkWidget         *widget,
                                                     GimpItemTreeView  *view);
static void   gimp_item_tree_view_lock_position_toggled
                                                    (GtkWidget         *widget,
                                                     GimpItemTreeView  *view);
static void   gimp_item_tree_view_update_options    (GimpItemTreeView  *view,
                                                     GimpItem          *item);

static gboolean gimp_item_tree_view_item_pre_clicked(GimpCellRendererViewable *cell,
                                                     const gchar              *path_str,
                                                     GdkModifierType           state,
                                                     GimpItemTreeView         *item_view);

/*  utility function to avoid code duplication  */
static void   gimp_item_tree_view_toggle_clicked    (GtkCellRendererToggle *toggle,
                                                     gchar             *path_str,
                                                     GdkModifierType    state,
                                                     GimpItemTreeView  *view,
                                                     GimpUndoType       undo_type);

static void   gimp_item_tree_view_row_expanded      (GtkTreeView       *tree_view,
                                                     GtkTreeIter       *iter,
                                                     GtkTreePath       *path,
                                                     GimpItemTreeView  *item_view);


G_DEFINE_TYPE_WITH_CODE (GimpItemTreeView, gimp_item_tree_view,
                         GIMP_TYPE_CONTAINER_TREE_VIEW,
                         G_ADD_PRIVATE (GimpItemTreeView)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_item_tree_view_view_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_item_tree_view_docked_iface_init))

#define parent_class gimp_item_tree_view_parent_class

static GimpContainerViewInterface *parent_view_iface = NULL;

static guint view_signals[LAST_SIGNAL] = { 0 };


static void
gimp_item_tree_view_class_init (GimpItemTreeViewClass *klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass             *widget_class = GTK_WIDGET_CLASS (klass);
  GimpContainerTreeViewClass *tree_view_class;

  tree_view_class = GIMP_CONTAINER_TREE_VIEW_CLASS (klass);

  view_signals[SET_IMAGE] =
    g_signal_new ("set-image",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpItemTreeViewClass, set_image),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_OBJECT);

  object_class->constructed      = gimp_item_tree_view_constructed;
  object_class->dispose          = gimp_item_tree_view_dispose;

  widget_class->style_set        = gimp_item_tree_view_style_set;

  tree_view_class->drop_possible = gimp_item_tree_view_drop_possible;
  tree_view_class->drop_viewable = gimp_item_tree_view_drop_viewable;

  klass->set_image               = gimp_item_tree_view_real_set_image;

  klass->item_type               = G_TYPE_NONE;
  klass->signal_name             = NULL;

  klass->get_container           = NULL;
  klass->get_active_item         = NULL;
  klass->set_active_item         = NULL;
  klass->add_item                = NULL;
  klass->remove_item             = NULL;
  klass->new_item                = NULL;

  klass->action_group            = NULL;
  klass->new_action              = NULL;
  klass->new_default_action      = NULL;
  klass->raise_action            = NULL;
  klass->raise_top_action        = NULL;
  klass->lower_action            = NULL;
  klass->lower_bottom_action     = NULL;
  klass->duplicate_action        = NULL;
  klass->delete_action           = NULL;

  klass->lock_content_icon_name  = NULL;
  klass->lock_content_tooltip    = NULL;
  klass->lock_content_help_id    = NULL;

  klass->lock_position_icon_name  = NULL;
  klass->lock_position_tooltip    = NULL;
  klass->lock_position_help_id    = NULL;
}

static void
gimp_item_tree_view_view_iface_init (GimpContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->set_container     = gimp_item_tree_view_set_container;
  iface->set_context       = gimp_item_tree_view_set_context;
  iface->insert_item       = gimp_item_tree_view_insert_item;
  iface->insert_item_after = gimp_item_tree_view_insert_item_after;
  iface->select_item       = gimp_item_tree_view_select_item;
  iface->activate_item     = gimp_item_tree_view_activate_item;
  iface->context_item      = gimp_item_tree_view_context_item;
}

static void
gimp_item_tree_view_docked_iface_init (GimpDockedInterface *iface)
{
  iface->get_preview = NULL;
}

static void
gimp_item_tree_view_init (GimpItemTreeView *view)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);

  view->priv = gimp_item_tree_view_get_instance_private (view);

  view->priv->model_column_visible =
    gimp_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);

  view->priv->model_column_viewable =
    gimp_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);

  view->priv->model_column_linked =
    gimp_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);

  view->priv->model_column_color_tag =
    gimp_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           GDK_TYPE_COLOR);

  gimp_container_tree_view_set_dnd_drop_to_empty (tree_view, TRUE);

  view->priv->image  = NULL;
}

static void
gimp_item_tree_view_constructed (GObject *object)
{
  GimpItemTreeViewClass *item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (object);
  GimpEditor            *editor          = GIMP_EDITOR (object);
  GimpContainerTreeView *tree_view       = GIMP_CONTAINER_TREE_VIEW (object);
  GimpItemTreeView      *item_view       = GIMP_ITEM_TREE_VIEW (object);
  GtkTreeViewColumn     *column;
  GtkWidget             *hbox;
  GtkWidget             *image;
  GtkIconSize            icon_size;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_container_tree_view_connect_name_edited (tree_view,
                                                G_CALLBACK (gimp_item_tree_view_name_edited),
                                                item_view);

  g_signal_connect (tree_view->view, "row-expanded",
                    G_CALLBACK (gimp_item_tree_view_row_expanded),
                    tree_view);

  g_signal_connect (tree_view->renderer_cell, "pre-clicked",
                    G_CALLBACK (gimp_item_tree_view_item_pre_clicked),
                    item_view);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_insert_column (tree_view->view, column, 0);

  item_view->priv->eye_cell = gimp_cell_renderer_toggle_new (GIMP_ICON_VISIBLE);
  g_object_set (item_view->priv->eye_cell,
                "xpad",                0,
                "ypad",                0,
                "override-background", TRUE,
                NULL);
  gtk_tree_view_column_pack_start (column, item_view->priv->eye_cell, FALSE);
  gtk_tree_view_column_set_attributes (column, item_view->priv->eye_cell,
                                       "active",
                                       item_view->priv->model_column_visible,
                                       "inconsistent",
                                       item_view->priv->model_column_viewable,
                                       "cell-background-gdk",
                                       item_view->priv->model_column_color_tag,
                                       NULL);

  gimp_container_tree_view_add_toggle_cell (tree_view,
                                            item_view->priv->eye_cell);

  g_signal_connect (item_view->priv->eye_cell, "clicked",
                    G_CALLBACK (gimp_item_tree_view_eye_clicked),
                    item_view);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_insert_column (tree_view->view, column, 1);

  item_view->priv->chain_cell = gimp_cell_renderer_toggle_new (GIMP_ICON_LINKED);
  g_object_set (item_view->priv->chain_cell,
                "xpad", 0,
                "ypad", 0,
                NULL);
  gtk_tree_view_column_pack_start (column, item_view->priv->chain_cell, FALSE);
  gtk_tree_view_column_set_attributes (column, item_view->priv->chain_cell,
                                       "active",
                                       item_view->priv->model_column_linked,
                                       NULL);

  gimp_container_tree_view_add_toggle_cell (tree_view,
                                            item_view->priv->chain_cell);

  g_signal_connect (item_view->priv->chain_cell, "clicked",
                    G_CALLBACK (gimp_item_tree_view_chain_clicked),
                    item_view);

  /*  disable the default GimpContainerView drop handler  */
  gimp_container_view_set_dnd_widget (GIMP_CONTAINER_VIEW (item_view), NULL);

  gimp_dnd_drag_dest_set_by_type (GTK_WIDGET (tree_view->view),
                                  GTK_DEST_DEFAULT_HIGHLIGHT,
                                  item_view_class->item_type,
                                  GDK_ACTION_MOVE | GDK_ACTION_COPY);

  item_view->priv->new_button =
    gimp_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->new_action,
                                   item_view_class->new_default_action,
                                   GDK_SHIFT_MASK,
                                   NULL);
  /*  connect "drop to new" manually as it makes a difference whether
   *  it was clicked or dropped
   */
  gimp_dnd_viewable_dest_add (item_view->priv->new_button,
                              item_view_class->item_type,
                              gimp_item_tree_view_new_dropped,
                              item_view);

  item_view->priv->raise_button =
    gimp_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->raise_action,
                                   item_view_class->raise_top_action,
                                   GDK_SHIFT_MASK,
                                   NULL);

  item_view->priv->lower_button =
    gimp_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->lower_action,
                                   item_view_class->lower_bottom_action,
                                   GDK_SHIFT_MASK,
                                   NULL);

  item_view->priv->duplicate_button =
    gimp_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->duplicate_action, NULL);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (item_view),
                                  GTK_BUTTON (item_view->priv->duplicate_button),
                                  item_view_class->item_type);

  item_view->priv->delete_button =
    gimp_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->delete_action, NULL);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (item_view),
                                  GTK_BUTTON (item_view->priv->delete_button),
                                  item_view_class->item_type);

  hbox = gimp_item_tree_view_get_lock_box (item_view);

  /*  Lock content toggle  */
  item_view->priv->lock_content_toggle = gtk_toggle_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), item_view->priv->lock_content_toggle,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (hbox),
                         item_view->priv->lock_content_toggle, 0);
  gtk_widget_show (item_view->priv->lock_content_toggle);

  g_signal_connect (item_view->priv->lock_content_toggle, "toggled",
                    G_CALLBACK (gimp_item_tree_view_lock_content_toggled),
                    item_view);

  gimp_help_set_help_data (item_view->priv->lock_content_toggle,
                           item_view_class->lock_content_tooltip,
                           item_view_class->lock_content_help_id);

  gtk_widget_style_get (GTK_WIDGET (item_view),
                        "button-icon-size", &icon_size,
                        NULL);

  image = gtk_image_new_from_icon_name (item_view_class->lock_content_icon_name,
                                        icon_size);
  gtk_container_add (GTK_CONTAINER (item_view->priv->lock_content_toggle),
                     image);
  gtk_widget_show (image);

  /*  Lock position toggle  */
  item_view->priv->lock_position_toggle = gtk_toggle_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), item_view->priv->lock_position_toggle,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (hbox),
                         item_view->priv->lock_position_toggle, 1);
  gtk_widget_show (item_view->priv->lock_position_toggle);

  g_signal_connect (item_view->priv->lock_position_toggle, "toggled",
                    G_CALLBACK (gimp_item_tree_view_lock_position_toggled),
                    item_view);

  gimp_help_set_help_data (item_view->priv->lock_position_toggle,
                           item_view_class->lock_position_tooltip,
                           item_view_class->lock_position_help_id);

  image = gtk_image_new_from_icon_name (item_view_class->lock_position_icon_name,
                                        icon_size);
  gtk_container_add (GTK_CONTAINER (item_view->priv->lock_position_toggle),
                     image);
  gtk_widget_show (image);
}

static void
gimp_item_tree_view_dispose (GObject *object)
{
  GimpItemTreeView *view = GIMP_ITEM_TREE_VIEW (object);

  if (view->priv->image)
    gimp_item_tree_view_set_image (view, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_item_tree_view_style_set (GtkWidget *widget,
                               GtkStyle  *prev_style)
{
  GimpItemTreeView *view = GIMP_ITEM_TREE_VIEW (widget);
  GList            *children;
  GList            *list;
  GtkReliefStyle    button_relief;
  GtkIconSize       button_icon_size;
  gint              content_spacing;
  gint              button_spacing;

  gtk_widget_style_get (widget,
                        "button-relief",    &button_relief,
                        "button-icon-size", &button_icon_size,
                        "content-spacing",  &content_spacing,
                        "button-spacing",   &button_spacing,
                        NULL);

  if (view->priv->options_box)
    {
      gtk_box_set_spacing (GTK_BOX (view->priv->options_box), content_spacing);

      children =
        gtk_container_get_children (GTK_CONTAINER (view->priv->options_box));

      for (list = children; list; list = g_list_next (list))
        {
          GtkWidget *child = list->data;

          if (GTK_IS_BOX (child))
            gtk_box_set_spacing (GTK_BOX (child), button_spacing);
        }

      g_list_free (children);
    }

  if (view->priv->lock_box)
    {
      gtk_box_set_spacing (GTK_BOX (view->priv->lock_box), button_spacing);

      children =
        gtk_container_get_children (GTK_CONTAINER (view->priv->lock_box));

      for (list = children; list; list = g_list_next (list))
        {
          GtkWidget *child = list->data;

          if (GTK_IS_BUTTON (child))
            {
              GtkWidget *image;

              gtk_button_set_relief (GTK_BUTTON (child), button_relief);

              image = gtk_bin_get_child (GTK_BIN (child));

              if (GTK_IS_IMAGE (image))
                {
                  GtkIconSize  old_size;
                  const gchar *icon_name;

                  gtk_image_get_icon_name (GTK_IMAGE (image),
                                           &icon_name, &old_size);

                  if (button_icon_size != old_size)
                    gtk_image_set_from_icon_name (GTK_IMAGE (image),
                                                  icon_name, button_icon_size);
                }
            }
        }

      g_list_free (children);
    }

  /* force the toggle cells to recreate their icon */
  g_object_set (view->priv->eye_cell,
                "icon-name", GIMP_ICON_VISIBLE,
                NULL);
  g_object_set (view->priv->chain_cell,
                "icon-name", GIMP_ICON_LINKED,
                NULL);

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}

GtkWidget *
gimp_item_tree_view_new (GType            view_type,
                         gint             view_size,
                         gint             view_border_width,
                         GimpImage       *image,
                         GimpMenuFactory *menu_factory,
                         const gchar     *menu_identifier,
                         const gchar     *ui_path)
{
  GimpItemTreeView *item_view;

  g_return_val_if_fail (g_type_is_a (view_type, GIMP_TYPE_ITEM_TREE_VIEW), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (image == NULL || GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (menu_identifier != NULL, NULL);
  g_return_val_if_fail (ui_path != NULL, NULL);

  item_view = g_object_new (view_type,
                            "reorderable",     TRUE,
                            "menu-factory",    menu_factory,
                            "menu-identifier", menu_identifier,
                            "ui-path",         ui_path,
                            NULL);

  gimp_container_view_set_view_size (GIMP_CONTAINER_VIEW (item_view),
                                     view_size, view_border_width);

  gimp_item_tree_view_set_image (item_view, image);

  return GTK_WIDGET (item_view);
}

void
gimp_item_tree_view_set_image (GimpItemTreeView *view,
                               GimpImage        *image)
{
  g_return_if_fail (GIMP_IS_ITEM_TREE_VIEW (view));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  g_signal_emit (view, view_signals[SET_IMAGE], 0, image);

  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (view)), view);
}

GimpImage *
gimp_item_tree_view_get_image (GimpItemTreeView *view)
{
  g_return_val_if_fail (GIMP_IS_ITEM_TREE_VIEW (view), NULL);

  return view->priv->image;
}

void
gimp_item_tree_view_add_options (GimpItemTreeView *view,
                                 const gchar      *label,
                                 GtkWidget        *options)
{
  gint content_spacing;
  gint button_spacing;

  g_return_if_fail (GIMP_IS_ITEM_TREE_VIEW (view));
  g_return_if_fail (GTK_IS_WIDGET (options));

  gtk_widget_style_get (GTK_WIDGET (view),
                        "content-spacing", &content_spacing,
                        "button-spacing",  &button_spacing,
                        NULL);

  if (! view->priv->options_box)
    {
      GimpItemTreeViewClass *item_view_class;

      item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (view);

      view->priv->options_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, content_spacing);
      gtk_box_pack_start (GTK_BOX (view), view->priv->options_box,
                          FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (view), view->priv->options_box, 0);
      gtk_widget_show (view->priv->options_box);

      if (! view->priv->image ||
          ! item_view_class->get_active_item (view->priv->image))
        {
          gtk_widget_set_sensitive (view->priv->options_box, FALSE);
        }
    }

  if (label)
    {
      GtkWidget *hbox;
      GtkWidget *label_widget;
      gboolean   group_created = FALSE;

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, button_spacing);
      gtk_box_pack_start (GTK_BOX (view->priv->options_box), hbox,
                          FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      if (! view->priv->options_group)
        {
          view->priv->options_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
          group_created = TRUE;
        }

      label_widget = gtk_label_new (label);
      gtk_label_set_xalign (GTK_LABEL (label_widget), 0.0);
      gtk_size_group_add_widget (view->priv->options_group, label_widget);
      gtk_box_pack_start (GTK_BOX (hbox), label_widget, FALSE, FALSE, 0);
      gtk_widget_show (label_widget);

      if (group_created)
        g_object_unref (view->priv->options_group);

      gtk_box_pack_start (GTK_BOX (hbox), options, TRUE, TRUE, 0);
      gtk_widget_show (options);
    }
  else
    {
      gtk_box_pack_start (GTK_BOX (view->priv->options_box), options,
                          FALSE, FALSE, 0);
      gtk_widget_show (options);
    }
}

GtkWidget *
gimp_item_tree_view_get_lock_box (GimpItemTreeView *view)
{
  g_return_val_if_fail (GIMP_IS_ITEM_TREE_VIEW (view), NULL);

  if (! view->priv->lock_box)
    {
      gint button_spacing;

      gtk_widget_style_get (GTK_WIDGET (view),
                            "button-spacing", &button_spacing,
                            NULL);

      view->priv->lock_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, button_spacing);

      gimp_item_tree_view_add_options (view, _("Lock:"), view->priv->lock_box);

      gtk_box_set_child_packing (GTK_BOX (view->priv->options_box),
                                 gtk_widget_get_parent (view->priv->lock_box),
                                 FALSE, FALSE, 0, GTK_PACK_END);
    }

  return view->priv->lock_box;
}

GtkWidget *
gimp_item_tree_view_get_new_button (GimpItemTreeView *view)
{
  g_return_val_if_fail (GIMP_IS_ITEM_TREE_VIEW (view), NULL);

  return view->priv->new_button;
}

GtkWidget *
gimp_item_tree_view_get_delete_button (GimpItemTreeView *view)
{
  g_return_val_if_fail (GIMP_IS_ITEM_TREE_VIEW (view), NULL);

  return view->priv->delete_button;
}

gint
gimp_item_tree_view_get_drop_index (GimpItemTreeView         *view,
                                    GimpViewable             *dest_viewable,
                                    GtkTreeViewDropPosition   drop_pos,
                                    GimpViewable            **parent)
{
  gint index = -1;

  g_return_val_if_fail (GIMP_IS_ITEM_TREE_VIEW (view), -1);
  g_return_val_if_fail (dest_viewable == NULL ||
                        GIMP_IS_VIEWABLE (dest_viewable), -1);
  g_return_val_if_fail (parent != NULL, -1);

  *parent = NULL;

  if (dest_viewable)
    {
      *parent = gimp_viewable_get_parent (dest_viewable);
      index   = gimp_item_get_index (GIMP_ITEM (dest_viewable));

      if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
        {
          GimpContainer *children = gimp_viewable_get_children (dest_viewable);

          if (children)
            {
              *parent = dest_viewable;
              index   = 0;
            }
          else
            {
              index++;
            }
        }
      else if (drop_pos == GTK_TREE_VIEW_DROP_AFTER)
        {
          index++;
        }
    }

  return index;
}

static void
gimp_item_tree_view_real_set_image (GimpItemTreeView *view,
                                    GimpImage        *image)
{
  if (view->priv->image == image)
    return;

  if (view->priv->image)
    {
      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            gimp_item_tree_view_item_changed,
                                            view);
      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            gimp_item_tree_view_size_changed,
                                            view);

      gimp_container_view_set_container (GIMP_CONTAINER_VIEW (view), NULL);

      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            gimp_item_tree_view_image_flush,
                                            view);
    }

  view->priv->image = image;

  if (view->priv->image)
    {
      GimpContainer *container;

      container =
        GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_container (view->priv->image);

      gimp_container_view_set_container (GIMP_CONTAINER_VIEW (view), container);

      g_signal_connect (view->priv->image,
                        GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->signal_name,
                        G_CALLBACK (gimp_item_tree_view_item_changed),
                        view);
      g_signal_connect (view->priv->image, "size-changed",
                        G_CALLBACK (gimp_item_tree_view_size_changed),
                        view);

      g_signal_connect (view->priv->image, "flush",
                        G_CALLBACK (gimp_item_tree_view_image_flush),
                        view);

      gimp_item_tree_view_item_changed (view->priv->image, view);
    }
}

static void
gimp_item_tree_view_image_flush (GimpImage        *image,
                                 gboolean          invalidate_preview,
                                 GimpItemTreeView *view)
{
  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (view)), view);
}


/*  GimpContainerView methods  */

static void
gimp_item_tree_view_set_container (GimpContainerView *view,
                                   GimpContainer     *container)
{
  GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (view);
  GimpContainer    *old_container;

  old_container = gimp_container_view_get_container (view);

  if (old_container)
    {
      gimp_tree_handler_disconnect (item_view->priv->visible_changed_handler);
      item_view->priv->visible_changed_handler = NULL;

      gimp_tree_handler_disconnect (item_view->priv->linked_changed_handler);
      item_view->priv->linked_changed_handler = NULL;

      gimp_tree_handler_disconnect (item_view->priv->color_tag_changed_handler);
      item_view->priv->color_tag_changed_handler = NULL;

      gimp_tree_handler_disconnect (item_view->priv->lock_content_changed_handler);
      item_view->priv->lock_content_changed_handler = NULL;

      gimp_tree_handler_disconnect (item_view->priv->lock_position_changed_handler);
      item_view->priv->lock_position_changed_handler = NULL;
    }

  parent_view_iface->set_container (view, container);

  if (container)
    {
      item_view->priv->visible_changed_handler =
        gimp_tree_handler_connect (container, "visibility-changed",
                                   G_CALLBACK (gimp_item_tree_view_visible_changed),
                                   view);

      item_view->priv->linked_changed_handler =
        gimp_tree_handler_connect (container, "linked-changed",
                                   G_CALLBACK (gimp_item_tree_view_linked_changed),
                                   view);

      item_view->priv->color_tag_changed_handler =
        gimp_tree_handler_connect (container, "color-tag-changed",
                                   G_CALLBACK (gimp_item_tree_view_color_tag_changed),
                                   view);

      item_view->priv->lock_content_changed_handler =
        gimp_tree_handler_connect (container, "lock-content-changed",
                                   G_CALLBACK (gimp_item_tree_view_lock_content_changed),
                                   view);

      item_view->priv->lock_position_changed_handler =
        gimp_tree_handler_connect (container, "lock-position-changed",
                                   G_CALLBACK (gimp_item_tree_view_lock_position_changed),
                                   view);
    }
}

static void
gimp_item_tree_view_set_context (GimpContainerView *view,
                                 GimpContext       *context)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GimpItemTreeView      *item_view = GIMP_ITEM_TREE_VIEW (view);
  GimpImage             *image     = NULL;
  GimpContext           *old_context;

  old_context = gimp_container_view_get_context (view);

  if (old_context)
    {
      g_signal_handlers_disconnect_by_func (old_context,
                                            gimp_item_tree_view_set_image,
                                            item_view);
    }

  parent_view_iface->set_context (view, context);

  if (context)
    {
      if (! tree_view->dnd_gimp)
        tree_view->dnd_gimp = context->gimp;

      g_signal_connect_swapped (context, "image-changed",
                                G_CALLBACK (gimp_item_tree_view_set_image),
                                item_view);

      image = gimp_context_get_image (context);
    }

  gimp_item_tree_view_set_image (item_view, image);
}

static gpointer
gimp_item_tree_view_insert_item (GimpContainerView *view,
                                 GimpViewable      *viewable,
                                 gpointer           parent_insert_data,
                                 gint               index)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GimpItemTreeView      *item_view = GIMP_ITEM_TREE_VIEW (view);
  GimpItem              *item      = GIMP_ITEM (viewable);
  GtkTreeIter           *iter;
  GimpRGB                color;
  GdkColor               gdk_color;
  gboolean               has_color;

  item_view->priv->inserting_item = TRUE;

  iter = parent_view_iface->insert_item (view, viewable,
                                         parent_insert_data, index);

  item_view->priv->inserting_item = FALSE;

  has_color = gimp_get_color_tag_color (gimp_item_get_merged_color_tag (item),
                                        &color,
                                        gimp_item_get_color_tag (item) ==
                                        GIMP_COLOR_TAG_NONE);
  if (has_color)
    gimp_rgb_get_gdk_color (&color, &gdk_color);

  gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                      item_view->priv->model_column_visible,
                      gimp_item_get_visible (item),
                      item_view->priv->model_column_viewable,
                      gimp_item_get_visible (item) &&
                      ! gimp_item_is_visible (item),
                      item_view->priv->model_column_linked,
                      gimp_item_get_linked (item),
                      item_view->priv->model_column_color_tag,
                      has_color ? &gdk_color : NULL,
                      -1);

  return iter;
}

static void
gimp_item_tree_view_insert_item_after (GimpContainerView *view,
                                       GimpViewable      *viewable,
                                       gpointer           insert_data)
{
  GimpItemTreeView      *item_view = GIMP_ITEM_TREE_VIEW (view);
  GimpItemTreeViewClass *item_view_class;
  GimpItem              *active_item;

  item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (item_view);

  active_item = item_view_class->get_active_item (item_view->priv->image);

  if (active_item == (GimpItem *) viewable)
    gimp_container_view_select_item (view, viewable);
}

static gboolean
gimp_item_tree_view_select_item (GimpContainerView *view,
                                 GimpViewable      *item,
                                 gpointer           insert_data)
{
  GimpItemTreeView *tree_view         = GIMP_ITEM_TREE_VIEW (view);
  gboolean          options_sensitive = FALSE;
  gboolean          success;

  success = parent_view_iface->select_item (view, item, insert_data);

  if (item)
    {
      GimpItemTreeViewClass *item_view_class;
      GimpItem              *active_item;

      item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (tree_view);

      active_item = item_view_class->get_active_item (tree_view->priv->image);

      if (active_item != (GimpItem *) item)
        {
          item_view_class->set_active_item (tree_view->priv->image,
                                            GIMP_ITEM (item));

          gimp_image_flush (tree_view->priv->image);
        }

      options_sensitive = TRUE;

      gimp_item_tree_view_update_options (tree_view, GIMP_ITEM (item));
    }

  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (tree_view)), tree_view);

  if (tree_view->priv->options_box)
    gtk_widget_set_sensitive (tree_view->priv->options_box, options_sensitive);

  return success;
}

static void
gimp_item_tree_view_activate_item (GimpContainerView *view,
                                   GimpViewable      *item,
                                   gpointer           insert_data)
{
  GimpItemTreeViewClass *item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (view);

  if (parent_view_iface->activate_item)
    parent_view_iface->activate_item (view, item, insert_data);

  if (item_view_class->activate_action)
    {
      gimp_ui_manager_activate_action (gimp_editor_get_ui_manager (GIMP_EDITOR (view)),
                                       item_view_class->action_group,
                                       item_view_class->activate_action);
    }
}

static void
gimp_item_tree_view_context_item (GimpContainerView *view,
                                  GimpViewable      *item,
                                  gpointer           insert_data)
{
  if (parent_view_iface->context_item)
    parent_view_iface->context_item (view, item, insert_data);

  gimp_editor_popup_menu (GIMP_EDITOR (view), NULL, NULL);
}

static gboolean
gimp_item_tree_view_drop_possible (GimpContainerTreeView   *tree_view,
                                   GimpDndType              src_type,
                                   GimpViewable            *src_viewable,
                                   GimpViewable            *dest_viewable,
                                   GtkTreePath             *drop_path,
                                   GtkTreeViewDropPosition  drop_pos,
                                   GtkTreeViewDropPosition *return_drop_pos,
                                   GdkDragAction           *return_drag_action)
{
  if (GIMP_IS_ITEM (src_viewable) &&
      (dest_viewable == NULL ||
       gimp_item_get_image (GIMP_ITEM (src_viewable)) !=
       gimp_item_get_image (GIMP_ITEM (dest_viewable))))
    {
      if (return_drop_pos)
        *return_drop_pos = drop_pos;

      if (return_drag_action)
        *return_drag_action = GDK_ACTION_COPY;

      return TRUE;
    }

  return GIMP_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_possible (tree_view,
                                                                       src_type,
                                                                       src_viewable,
                                                                       dest_viewable,
                                                                       drop_path,
                                                                       drop_pos,
                                                                       return_drop_pos,
                                                                       return_drag_action);
}

static void
gimp_item_tree_view_drop_viewable (GimpContainerTreeView   *tree_view,
                                   GimpViewable            *src_viewable,
                                   GimpViewable            *dest_viewable,
                                   GtkTreeViewDropPosition  drop_pos)
{
  GimpItemTreeViewClass *item_view_class;
  GimpItemTreeView      *item_view  = GIMP_ITEM_TREE_VIEW (tree_view);
  gint                   dest_index = -1;

  item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (item_view);

  if (item_view->priv->image != gimp_item_get_image (GIMP_ITEM (src_viewable)) ||
      ! g_type_is_a (G_TYPE_FROM_INSTANCE (src_viewable),
                     item_view_class->item_type))
    {
      GType     item_type = item_view_class->item_type;
      GimpItem *new_item;
      GimpItem *parent;

      if (g_type_is_a (G_TYPE_FROM_INSTANCE (src_viewable), item_type))
        item_type = G_TYPE_FROM_INSTANCE (src_viewable);

      dest_index = gimp_item_tree_view_get_drop_index (item_view, dest_viewable,
                                                       drop_pos,
                                                       (GimpViewable **) &parent);

      new_item = gimp_item_convert (GIMP_ITEM (src_viewable),
                                    item_view->priv->image, item_type);

      gimp_item_set_linked (new_item, FALSE, FALSE);

      item_view_class->add_item (item_view->priv->image, new_item,
                                 parent, dest_index, TRUE);
    }
  else if (dest_viewable)
    {
      GimpItem *src_parent;
      GimpItem *dest_parent;
      gint      src_index;
      gint      dest_index;

      src_parent = GIMP_ITEM (gimp_viewable_get_parent (src_viewable));
      src_index  = gimp_item_get_index (GIMP_ITEM (src_viewable));

      dest_index = gimp_item_tree_view_get_drop_index (item_view, dest_viewable,
                                                       drop_pos,
                                                       (GimpViewable **) &dest_parent);

      if (src_parent == dest_parent)
        {
          if (src_index < dest_index)
            dest_index--;
        }

      gimp_image_reorder_item (item_view->priv->image,
                               GIMP_ITEM (src_viewable),
                               dest_parent,
                               dest_index,
                               TRUE, NULL);
    }

  gimp_image_flush (item_view->priv->image);
}


/*  "New" functions  */

static void
gimp_item_tree_view_new_dropped (GtkWidget    *widget,
                                 gint          x,
                                 gint          y,
                                 GimpViewable *viewable,
                                 gpointer      data)
{
  GimpItemTreeViewClass *item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (data);
  GimpContainerView     *view            = GIMP_CONTAINER_VIEW (data);

  if (item_view_class->new_default_action &&
      viewable && gimp_container_view_lookup (view, viewable))
    {
      GimpAction *action;

      action = gimp_ui_manager_find_action (gimp_editor_get_ui_manager (GIMP_EDITOR (view)),
                                            item_view_class->action_group,
                                            item_view_class->new_default_action);

      if (action)
        {
          g_object_set (action, "viewable", viewable, NULL);
          gimp_action_activate (action);
          g_object_set (action, "viewable", NULL, NULL);
        }
    }
}


/*  GimpImage callbacks  */

static void
gimp_item_tree_view_item_changed (GimpImage        *image,
                                  GimpItemTreeView *view)
{
  GimpItem *item;

  item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (view->priv->image);

  gimp_container_view_select_item (GIMP_CONTAINER_VIEW (view),
                                   (GimpViewable *) item);
}

static void
gimp_item_tree_view_size_changed (GimpImage        *image,
                                  GimpItemTreeView *tree_view)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (tree_view);
  gint               view_size;
  gint               border_width;

  view_size = gimp_container_view_get_view_size (view, &border_width);

  gimp_container_view_set_view_size (view, view_size, border_width);
}

static void
gimp_item_tree_view_name_edited (GtkCellRendererText *cell,
                                 const gchar         *path_str,
                                 const gchar         *new_name,
                                 GimpItemTreeView    *view)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreePath           *path;
  GtkTreeIter            iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpViewRenderer *renderer;
      GimpItem         *item;
      const gchar      *old_name;
      GError           *error = NULL;

      gtk_tree_model_get (tree_view->model, &iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      item = GIMP_ITEM (renderer->viewable);

      old_name = gimp_object_get_name (item);

      if (! old_name) old_name = "";
      if (! new_name) new_name = "";

      if (strcmp (old_name, new_name) &&
          gimp_item_rename (item, new_name, &error))
        {
          gimp_image_flush (gimp_item_get_image (item));
        }
      else
        {
          gchar *name = gimp_viewable_get_description (renderer->viewable, NULL);

          gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), &iter,
                              GIMP_CONTAINER_TREE_STORE_COLUMN_NAME, name,
                              -1);
          g_free (name);

          if (error)
            {
              gimp_message_literal (view->priv->image->gimp, G_OBJECT (view),
                                    GIMP_MESSAGE_WARNING,
                                    error->message);
              g_clear_error (&error);
            }
        }

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}


/*  "Visible" callbacks  */

static void
gimp_item_tree_view_visible_changed (GimpItem         *item,
                                     GimpItemTreeView *view)
{
  GimpContainerView     *container_view = GIMP_CONTAINER_VIEW (view);
  GimpContainerTreeView *tree_view      = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter;

  iter = gimp_container_view_lookup (container_view,
                                     (GimpViewable *) item);

  if (iter)
    {
      GimpContainer *children;

      gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                          view->priv->model_column_visible,
                          gimp_item_get_visible (item),
                          view->priv->model_column_viewable,
                          gimp_item_get_visible (item) &&
                          ! gimp_item_is_visible (item),
                          -1);

      children = gimp_viewable_get_children (GIMP_VIEWABLE (item));

      if (children)
        gimp_container_foreach (children,
                                (GFunc) gimp_item_tree_view_visible_changed,
                                view);
    }
}

static void
gimp_item_tree_view_eye_clicked (GtkCellRendererToggle *toggle,
                                 gchar                 *path_str,
                                 GdkModifierType        state,
                                 GimpItemTreeView      *view)
{
  gimp_item_tree_view_toggle_clicked (toggle, path_str, state, view,
                                      GIMP_UNDO_ITEM_VISIBILITY);
}


/*  "Linked" callbacks  */

static void
gimp_item_tree_view_linked_changed (GimpItem         *item,
                                    GimpItemTreeView *view)
{
  GimpContainerView     *container_view = GIMP_CONTAINER_VIEW (view);
  GimpContainerTreeView *tree_view      = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter;

  iter = gimp_container_view_lookup (container_view,
                                     (GimpViewable *) item);

  if (iter)
    gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                        view->priv->model_column_linked,
                        gimp_item_get_linked (item),
                        -1);
}

static void
gimp_item_tree_view_chain_clicked (GtkCellRendererToggle *toggle,
                                   gchar                 *path_str,
                                   GdkModifierType        state,
                                   GimpItemTreeView      *view)
{
  gimp_item_tree_view_toggle_clicked (toggle, path_str, state, view,
                                      GIMP_UNDO_ITEM_LINKED);
}


/*  "Color Tag" callbacks  */

static void
gimp_item_tree_view_color_tag_changed (GimpItem         *item,
                                       GimpItemTreeView *view)
{
  GimpContainerView     *container_view = GIMP_CONTAINER_VIEW (view);
  GimpContainerTreeView *tree_view      = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter;

  iter = gimp_container_view_lookup (container_view,
                                     (GimpViewable *) item);

  if (iter)
    {
      GimpContainer *children;
      GimpRGB        color;
      GdkColor       gdk_color;
      gboolean       has_color;

      has_color = gimp_get_color_tag_color (gimp_item_get_merged_color_tag (item),
                                            &color,
                                            gimp_item_get_color_tag (item) ==
                                            GIMP_COLOR_TAG_NONE);
      if (has_color)
        gimp_rgb_get_gdk_color (&color, &gdk_color);

      gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                          view->priv->model_column_color_tag,
                          has_color ? &gdk_color : NULL,
                          -1);

      children = gimp_viewable_get_children (GIMP_VIEWABLE (item));

      if (children)
        gimp_container_foreach (children,
                                (GFunc) gimp_item_tree_view_color_tag_changed,
                                view);
    }
}


/*  "Lock Content" callbacks  */

static void
gimp_item_tree_view_lock_content_changed (GimpItem         *item,
                                          GimpItemTreeView *view)
{
  GimpImage *image = view->priv->image;
  GimpItem  *active_item;

  active_item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (image);

  if (active_item == item)
    gimp_item_tree_view_update_options (view, item);
}

static void
gimp_item_tree_view_lock_content_toggled (GtkWidget         *widget,
                                          GimpItemTreeView  *view)
{
  GimpImage *image = view->priv->image;
  GimpItem  *item;

  item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (image);

  if (item)
    {
      gboolean lock_content;

      lock_content = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

      if (gimp_item_get_lock_content (item) != lock_content)
        {
#if 0
          GimpUndo *undo;
#endif
          gboolean  push_undo = TRUE;

#if 0
          /*  compress lock content undos  */
          undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                               GIMP_UNDO_ITEM_LOCK_CONTENT);

          if (undo && GIMP_ITEM_UNDO (undo)->item == item)
            push_undo = FALSE;
#endif

          g_signal_handlers_block_by_func (item,
                                           gimp_item_tree_view_lock_content_changed,
                                           view);

          gimp_item_set_lock_content (item, lock_content, push_undo);

          g_signal_handlers_unblock_by_func (item,
                                             gimp_item_tree_view_lock_content_changed,
                                             view);

          gimp_image_flush (image);
        }
    }
}

static void
gimp_item_tree_view_lock_position_changed (GimpItem         *item,
                                           GimpItemTreeView *view)
{
  GimpImage *image = view->priv->image;
  GimpItem  *active_item;

  active_item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (image);

  if (active_item == item)
    gimp_item_tree_view_update_options (view, item);
}

static void
gimp_item_tree_view_lock_position_toggled (GtkWidget         *widget,
                                           GimpItemTreeView  *view)
{
  GimpImage *image = view->priv->image;
  GimpItem  *item;

  item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (image);

  if (item)
    {
      gboolean lock_position;

      lock_position = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

      if (gimp_item_get_lock_position (item) != lock_position)
        {
          GimpUndo *undo;
          gboolean  push_undo = TRUE;

          /*  compress lock position undos  */
          undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                               GIMP_UNDO_ITEM_LOCK_POSITION);

          if (undo && GIMP_ITEM_UNDO (undo)->item == item)
            push_undo = FALSE;

          g_signal_handlers_block_by_func (item,
                                           gimp_item_tree_view_lock_position_changed,
                                           view);

          gimp_item_set_lock_position (item, lock_position, push_undo);

          g_signal_handlers_unblock_by_func (item,
                                             gimp_item_tree_view_lock_position_changed,
                                             view);

          gimp_image_flush (image);
        }
    }
}

static gboolean
gimp_item_tree_view_item_pre_clicked (GimpCellRendererViewable *cell,
                                      const gchar              *path_str,
                                      GdkModifierType           state,
                                      GimpItemTreeView         *item_view)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (item_view);
  GtkTreePath           *path;
  GtkTreeIter            iter;
  gboolean               handled = FALSE;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path) &&
      (state & GDK_MOD1_MASK))
    {
      GimpImage        *image    = gimp_item_tree_view_get_image (item_view);
      GimpViewRenderer *renderer = NULL;

      gtk_tree_model_get (tree_view->model, &iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      if (renderer)
        {
          GimpItem       *item = GIMP_ITEM (renderer->viewable);
          GimpChannelOps  op   = gimp_modifiers_to_channel_op (state);

          gimp_item_to_selection (item, op,
                                  TRUE, FALSE, 0.0, 0.0);
          gimp_image_flush (image);

          g_object_unref (renderer);

          /* Don't select the clicked layer */
          handled = TRUE;
        }
    }

  gtk_tree_path_free (path);

  return handled;
}

static void
gimp_item_tree_view_update_options (GimpItemTreeView *view,
                                    GimpItem         *item)
{
  if (gimp_item_get_lock_content (item) !=
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (view->priv->lock_content_toggle)))
    {
      g_signal_handlers_block_by_func (view->priv->lock_content_toggle,
                                       gimp_item_tree_view_lock_content_toggled,
                                       view);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (view->priv->lock_content_toggle),
                                    gimp_item_get_lock_content (item));

      g_signal_handlers_unblock_by_func (view->priv->lock_content_toggle,
                                         gimp_item_tree_view_lock_content_toggled,
                                         view);
    }

  if (gimp_item_get_lock_position (item) !=
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (view->priv->lock_position_toggle)))
    {
      g_signal_handlers_block_by_func (view->priv->lock_position_toggle,
                                       gimp_item_tree_view_lock_position_toggled,
                                       view);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (view->priv->lock_position_toggle),
                                    gimp_item_get_lock_position (item));

      g_signal_handlers_unblock_by_func (view->priv->lock_position_toggle,
                                         gimp_item_tree_view_lock_position_toggled,
                                         view);
    }

  gtk_widget_set_sensitive (view->priv->lock_content_toggle,
                            gimp_item_can_lock_content (item));

  gtk_widget_set_sensitive (view->priv->lock_position_toggle,
                            gimp_item_can_lock_position (item));
}


/*  Utility functions used from eye_clicked and chain_clicked.
 *  Would make sense to do this in a generic fashion using
 *  properties, but for now it's better than duplicating the code.
 */
static void
gimp_item_tree_view_toggle_clicked (GtkCellRendererToggle *toggle,
                                    gchar                 *path_str,
                                    GdkModifierType        state,
                                    GimpItemTreeView      *view,
                                    GimpUndoType           undo_type)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreePath           *path;
  GtkTreeIter            iter;

  void (* setter)    (GimpItem    *item,
                      gboolean     value,
                      gboolean     push_undo);
  void (* exclusive) (GimpItem    *item,
                      GimpContext *context);

  switch (undo_type)
    {
    case GIMP_UNDO_ITEM_VISIBILITY:
      setter     = gimp_item_set_visible;
      exclusive  = gimp_item_toggle_exclusive_visible;
      break;

    case GIMP_UNDO_ITEM_LINKED:
      setter     = gimp_item_set_linked;
      exclusive  = gimp_item_toggle_exclusive_linked;
      break;

    default:
      return;
    }

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpContext      *context;
      GimpViewRenderer *renderer;
      GimpItem         *item;
      GimpImage        *image;
      gboolean          active;

      context = gimp_container_view_get_context (GIMP_CONTAINER_VIEW (view));

      gtk_tree_model_get (tree_view->model, &iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);
      g_object_get (toggle,
                    "active", &active,
                    NULL);

      item = GIMP_ITEM (renderer->viewable);
      g_object_unref (renderer);

      image = gimp_item_get_image (item);

      if ((state & GDK_SHIFT_MASK) && exclusive)
        {
          exclusive (item, context);
        }
      else
        {
          GimpUndo *undo;
          gboolean  push_undo = TRUE;

          undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                               undo_type);

          if (undo && GIMP_ITEM_UNDO (undo)->item == item)
            push_undo = FALSE;

          setter (item, ! active, push_undo);

          if (!push_undo)
            gimp_undo_refresh_preview (undo, context);
        }

      gimp_image_flush (image);
    }

  gtk_tree_path_free (path);
}


/*  GtkTreeView callbacks  */

static void
gimp_item_tree_view_row_expanded (GtkTreeView      *tree_view,
                                  GtkTreeIter      *iter,
                                  GtkTreePath      *path,
                                  GimpItemTreeView *item_view)
{
  /*  don't select the item while it is being inserted  */
  if (! item_view->priv->inserting_item)
    {
      GimpItemTreeViewClass *item_view_class;
      GimpViewRenderer      *renderer;
      GimpItem              *expanded_item;
      GimpItem              *active_item;

      gtk_tree_model_get (GIMP_CONTAINER_TREE_VIEW (item_view)->model, iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);
      expanded_item = GIMP_ITEM (renderer->viewable);
      g_object_unref (renderer);

      item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (item_view);

      active_item = item_view_class->get_active_item (item_view->priv->image);

      /*  select the active item only if it was made visible by expanding
       *  its immediate parent. See bug #666561.
       */
      if (active_item &&
          gimp_item_get_parent (active_item) == expanded_item)
        {
          gimp_container_view_select_item (GIMP_CONTAINER_VIEW (item_view),
                                           GIMP_VIEWABLE (active_item));
        }
    }
}
