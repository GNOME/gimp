/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaitemtreeview.c
 * Copyright (C) 2001-2011 Michael Natterer <mitch@ligma.org>
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

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaimage-undo-push.h"
#include "core/ligmaitem-exclusive.h"
#include "core/ligmaitemundo.h"
#include "core/ligmatreehandler.h"
#include "core/ligmaundostack.h"

#include "vectors/ligmavectors.h"

#include "ligmaaction.h"
#include "ligmacontainertreestore.h"
#include "ligmacontainerview.h"
#include "ligmadnd.h"
#include "ligmadocked.h"
#include "ligmaitemtreeview.h"
#include "ligmamenufactory.h"
#include "ligmaviewrenderer.h"
#include "ligmauimanager.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  SET_IMAGE,
  LAST_SIGNAL
};


struct _LigmaItemTreeViewPrivate
{
  LigmaImage       *image;

  GtkWidget       *options_box;
  GtkSizeGroup    *options_group;

  GtkWidget       *eye_header_image;
  GtkWidget       *lock_header_image;

  LigmaItem        *lock_box_item;
  GtkTreePath     *lock_box_path;
  GtkWidget       *lock_popover;
  GtkWidget       *lock_box;

  GList           *locks;

  GtkWidget       *new_button;
  GtkWidget       *raise_button;
  GtkWidget       *lower_button;
  GtkWidget       *duplicate_button;
  GtkWidget       *delete_button;

  gint             model_column_visible;
  gint             model_column_viewable;
  gint             model_column_locked;
  gint             model_column_lock_icon;
  gint             model_column_color_tag;
  GtkCellRenderer *eye_cell;
  GtkCellRenderer *lock_cell;

  LigmaTreeHandler *visible_changed_handler;
  LigmaTreeHandler *color_tag_changed_handler;
};

typedef struct
{
  GtkWidget        *toggle;
  const gchar      *icon_name;

  LigmaIsLockedFunc  is_locked;
  LigmaCanLockFunc   can_lock;
  LigmaSetLockFunc   lock;
  LigmaUndoLockPush  undo_push;

  const gchar      *tooltip;
  const gchar      *help_id;

  /* Signal handling when lock is changed from core. */
  const gchar      *signal_name;
  LigmaTreeHandler  *changed_handler;

  /* Undo types and labels. */
  LigmaUndoType      undo_type;
  LigmaUndoType      group_undo_type;
  const gchar      *undo_lock_desc;
  const gchar      *undo_unlock_desc;
  const gchar      *undo_exclusive_desc;
} LockToggle;


static void   ligma_item_tree_view_view_iface_init   (LigmaContainerViewInterface *view_iface);
static void   ligma_item_tree_view_docked_iface_init (LigmaDockedInterface *docked_iface);

static void   ligma_item_tree_view_constructed       (GObject           *object);
static void   ligma_item_tree_view_dispose           (GObject           *object);

static void   ligma_item_tree_view_style_updated     (GtkWidget         *widget);

static void   ligma_item_tree_view_real_set_image    (LigmaItemTreeView  *view,
                                                     LigmaImage         *image);

static void   ligma_item_tree_view_image_flush       (LigmaImage         *image,
                                                     gboolean           invalidate_preview,
                                                     LigmaItemTreeView  *view);

static void   ligma_item_tree_view_set_container     (LigmaContainerView *view,
                                                     LigmaContainer     *container);
static void   ligma_item_tree_view_set_context       (LigmaContainerView *view,
                                                     LigmaContext       *context);

static gpointer ligma_item_tree_view_insert_item     (LigmaContainerView *view,
                                                     LigmaViewable      *viewable,
                                                     gpointer           parent_insert_data,
                                                     gint               index);
static void  ligma_item_tree_view_insert_items_after (LigmaContainerView *view);
static gboolean ligma_item_tree_view_select_items    (LigmaContainerView *view,
                                                     GList             *items,
                                                     GList             *paths);
static void   ligma_item_tree_view_activate_item     (LigmaContainerView *view,
                                                     LigmaViewable      *item,
                                                     gpointer           insert_data);

static gboolean ligma_item_tree_view_drop_possible   (LigmaContainerTreeView *view,
                                                     LigmaDndType        src_type,
                                                     GList             *src_viewables,
                                                     LigmaViewable      *dest_viewable,
                                                     GtkTreePath       *drop_path,
                                                     GtkTreeViewDropPosition  drop_pos,
                                                     GtkTreeViewDropPosition *return_drop_pos,
                                                     GdkDragAction     *return_drag_action);
static void     ligma_item_tree_view_drop_viewables  (LigmaContainerTreeView   *view,
                                                     GList                   *src_viewables,
                                                     LigmaViewable            *dest_viewable,
                                                     GtkTreeViewDropPosition  drop_pos);

static void   ligma_item_tree_view_new_dropped       (GtkWidget         *widget,
                                                     gint               x,
                                                     gint               y,
                                                     LigmaViewable      *viewable,
                                                     gpointer           data);
static void ligma_item_tree_view_new_list_dropped    (GtkWidget         *widget,
                                                     gint               x,
                                                     gint               y,
                                                     GList             *viewables,
                                                     gpointer           data);

static void   ligma_item_tree_view_item_changed      (LigmaImage         *image,
                                                     LigmaItemTreeView  *view);
static void   ligma_item_tree_view_size_changed      (LigmaImage         *image,
                                                     LigmaItemTreeView  *view);

static void   ligma_item_tree_view_name_edited       (GtkCellRendererText *cell,
                                                     const gchar       *path,
                                                     const gchar       *new_name,
                                                     LigmaItemTreeView  *view);

static void   ligma_item_tree_view_visible_changed      (LigmaItem          *item,
                                                        LigmaItemTreeView  *view);
static void   ligma_item_tree_view_color_tag_changed    (LigmaItem          *item,
                                                        LigmaItemTreeView  *view);
static void   ligma_item_tree_view_lock_changed         (LigmaItem          *item,
                                                        LigmaItemTreeView  *view);

static void   ligma_item_tree_view_eye_clicked       (GtkCellRendererToggle *toggle,
                                                     gchar             *path,
                                                     GdkModifierType    state,
                                                     LigmaItemTreeView  *view);
static void   ligma_item_tree_view_lock_clicked      (GtkCellRendererToggle *toggle,
                                                     gchar             *path,
                                                     GdkModifierType    state,
                                                     LigmaItemTreeView  *view);
static gboolean ligma_item_tree_view_lock_button_release (GtkWidget        *widget,
                                         GdkEvent         *event,
                                         LigmaItemTreeView *view);
static void   ligma_item_tree_view_lock_toggled      (GtkWidget         *widget,
                                                     LigmaItemTreeView  *view);
static void   ligma_item_tree_view_update_lock_box   (LigmaItemTreeView  *view,
                                                     LigmaItem          *item,
                                                     GtkTreePath       *path);
static gboolean ligma_item_tree_view_popover_button_press (GtkWidget        *widget,
                                                          GdkEvent         *event,
                                                          LigmaItemTreeView *view);

static gboolean ligma_item_tree_view_item_pre_clicked(LigmaCellRendererViewable *cell,
                                                     const gchar              *path_str,
                                                     GdkModifierType           state,
                                                     LigmaItemTreeView         *item_view);

static void   ligma_item_tree_view_row_expanded      (GtkTreeView       *tree_view,
                                                     GtkTreeIter       *iter,
                                                     GtkTreePath       *path,
                                                     LigmaItemTreeView  *item_view);

static gint   ligma_item_tree_view_get_n_locks       (LigmaItemTreeView *view,
                                                     LigmaItem         *item,
                                                     const gchar     **icon_name);


G_DEFINE_TYPE_WITH_CODE (LigmaItemTreeView, ligma_item_tree_view,
                         LIGMA_TYPE_CONTAINER_TREE_VIEW,
                         G_ADD_PRIVATE (LigmaItemTreeView)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONTAINER_VIEW,
                                                ligma_item_tree_view_view_iface_init)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_item_tree_view_docked_iface_init))

#define parent_class ligma_item_tree_view_parent_class

static LigmaContainerViewInterface *parent_view_iface = NULL;

static guint view_signals[LAST_SIGNAL] = { 0 };


static void
ligma_item_tree_view_class_init (LigmaItemTreeViewClass *klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass             *widget_class = GTK_WIDGET_CLASS (klass);
  LigmaContainerTreeViewClass *tree_view_class;

  tree_view_class = LIGMA_CONTAINER_TREE_VIEW_CLASS (klass);

  view_signals[SET_IMAGE] =
    g_signal_new ("set-image",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaItemTreeViewClass, set_image),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_OBJECT);

  object_class->constructed      = ligma_item_tree_view_constructed;
  object_class->dispose          = ligma_item_tree_view_dispose;

  widget_class->style_updated    = ligma_item_tree_view_style_updated;

  tree_view_class->drop_possible  = ligma_item_tree_view_drop_possible;
  tree_view_class->drop_viewables = ligma_item_tree_view_drop_viewables;

  klass->set_image               = ligma_item_tree_view_real_set_image;

  klass->item_type               = G_TYPE_NONE;
  klass->signal_name             = NULL;

  klass->get_container           = NULL;
  klass->get_selected_items      = NULL;
  klass->set_selected_items      = NULL;
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

  klass->lock_visibility_icon_name = NULL;
  klass->lock_visibility_tooltip   = NULL;
  klass->lock_visibility_help_id   = NULL;
}

static void
ligma_item_tree_view_view_iface_init (LigmaContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->set_container      = ligma_item_tree_view_set_container;
  iface->set_context        = ligma_item_tree_view_set_context;
  iface->insert_item        = ligma_item_tree_view_insert_item;
  iface->insert_items_after = ligma_item_tree_view_insert_items_after;
  iface->select_items       = ligma_item_tree_view_select_items;
  iface->activate_item      = ligma_item_tree_view_activate_item;
}

static void
ligma_item_tree_view_docked_iface_init (LigmaDockedInterface *iface)
{
  iface->get_preview = NULL;
}

static void
ligma_item_tree_view_init (LigmaItemTreeView *view)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);

  view->priv = ligma_item_tree_view_get_instance_private (view);

  view->priv->model_column_visible =
    ligma_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);

  view->priv->model_column_viewable =
    ligma_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);

  view->priv->model_column_locked =
    ligma_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);

  view->priv->model_column_lock_icon =
    ligma_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_STRING);

  view->priv->model_column_color_tag =
    ligma_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           GDK_TYPE_RGBA);

  ligma_container_tree_view_set_dnd_drop_to_empty (tree_view, TRUE);

  view->priv->image  = NULL;
}

static void
ligma_item_tree_view_constructed (GObject *object)
{
  LigmaItemTreeViewClass *item_view_class = LIGMA_ITEM_TREE_VIEW_GET_CLASS (object);
  LigmaEditor            *editor          = LIGMA_EDITOR (object);
  LigmaContainerTreeView *tree_view       = LIGMA_CONTAINER_TREE_VIEW (object);
  LigmaItemTreeView      *item_view       = LIGMA_ITEM_TREE_VIEW (object);
  GtkTreeViewColumn     *column;
  GtkWidget             *image;
  GtkIconSize            button_icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
  gint                   pixel_icon_size  = 16;
  gint                   button_spacing;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_tree_view_set_headers_visible (tree_view->view, TRUE);

  gtk_widget_style_get (GTK_WIDGET (item_view),
                        "button-icon-size", &button_icon_size,
                        "button-spacing", &button_spacing,
                        NULL);
  gtk_icon_size_lookup (button_icon_size, &pixel_icon_size, NULL);

  ligma_container_tree_view_connect_name_edited (tree_view,
                                                G_CALLBACK (ligma_item_tree_view_name_edited),
                                                item_view);

  g_signal_connect (tree_view->view, "row-expanded",
                    G_CALLBACK (ligma_item_tree_view_row_expanded),
                    tree_view);

  g_signal_connect (tree_view->renderer_cell, "pre-clicked",
                    G_CALLBACK (ligma_item_tree_view_item_pre_clicked),
                    item_view);

  column = gtk_tree_view_column_new ();
  image = gtk_image_new_from_icon_name (LIGMA_ICON_VISIBLE, button_icon_size);
  gtk_tree_view_column_set_widget (column, image);
  gtk_widget_show (image);
  gtk_tree_view_insert_column (tree_view->view, column, 0);
  item_view->priv->eye_header_image = image;

  item_view->priv->eye_cell = ligma_cell_renderer_toggle_new (LIGMA_ICON_VISIBLE);
  g_object_set (item_view->priv->eye_cell,
                "xpad",                0,
                "ypad",                0,
                "icon-size",           pixel_icon_size,
                "override-background", TRUE,
                NULL);
  gtk_tree_view_column_pack_start (column, item_view->priv->eye_cell, FALSE);
  gtk_tree_view_column_set_attributes (column, item_view->priv->eye_cell,
                                       "active",
                                       item_view->priv->model_column_visible,
                                       "inconsistent",
                                       item_view->priv->model_column_viewable,
                                       "cell-background-rgba",
                                       item_view->priv->model_column_color_tag,
                                       NULL);

  ligma_container_tree_view_add_toggle_cell (tree_view,
                                            item_view->priv->eye_cell);

  g_signal_connect (item_view->priv->eye_cell, "clicked",
                    G_CALLBACK (ligma_item_tree_view_eye_clicked),
                    item_view);

  column = gtk_tree_view_column_new ();
  image = gtk_image_new_from_icon_name (LIGMA_ICON_LOCK, button_icon_size);
  gtk_tree_view_column_set_widget (column, image);
  gtk_widget_show (image);
  gtk_tree_view_insert_column (tree_view->view, column, 1);
  item_view->priv->lock_header_image = image;

  item_view->priv->lock_cell = ligma_cell_renderer_toggle_new (LIGMA_ICON_LOCK_MULTI);
  g_object_set (item_view->priv->lock_cell,
                "xpad",      0,
                "ypad",      0,
                "icon-size", pixel_icon_size,
                NULL);
  gtk_tree_view_column_pack_start (column, item_view->priv->lock_cell, FALSE);
  gtk_tree_view_column_set_attributes (column, item_view->priv->lock_cell,
                                       "active",
                                       item_view->priv->model_column_locked,
                                       "icon-name",
                                       item_view->priv->model_column_lock_icon,
                                       NULL);

  ligma_container_tree_view_add_toggle_cell (tree_view,
                                            item_view->priv->lock_cell);

  g_signal_connect (item_view->priv->lock_cell, "clicked",
                    G_CALLBACK (ligma_item_tree_view_lock_clicked),
                    item_view);

  /*  disable the default LigmaContainerView drop handler  */
  ligma_container_view_set_dnd_widget (LIGMA_CONTAINER_VIEW (item_view), NULL);

  ligma_dnd_drag_dest_set_by_type (GTK_WIDGET (tree_view->view),
                                  GTK_DEST_DEFAULT_HIGHLIGHT,
                                  item_view_class->item_type,
                                  TRUE,
                                  GDK_ACTION_MOVE | GDK_ACTION_COPY);

  item_view->priv->new_button =
    ligma_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->new_action,
                                   item_view_class->new_default_action,
                                   GDK_SHIFT_MASK,
                                   NULL);
  /*  connect "drop to new" manually as it makes a difference whether
   *  it was clicked or dropped
   */
  ligma_dnd_viewable_list_dest_add (item_view->priv->new_button,
                                   item_view_class->item_type,
                                   ligma_item_tree_view_new_list_dropped,
                                   item_view);
  ligma_dnd_viewable_dest_add (item_view->priv->new_button,
                              item_view_class->item_type,
                              ligma_item_tree_view_new_dropped,
                              item_view);

  item_view->priv->raise_button =
    ligma_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->raise_action,
                                   item_view_class->raise_top_action,
                                   GDK_SHIFT_MASK,
                                   NULL);

  item_view->priv->lower_button =
    ligma_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->lower_action,
                                   item_view_class->lower_bottom_action,
                                   GDK_SHIFT_MASK,
                                   NULL);

  item_view->priv->duplicate_button =
    ligma_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->duplicate_action, NULL);
  ligma_container_view_enable_dnd (LIGMA_CONTAINER_VIEW (item_view),
                                  GTK_BUTTON (item_view->priv->duplicate_button),
                                  item_view_class->item_type);

  item_view->priv->delete_button =
    ligma_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->delete_action, NULL);
  ligma_container_view_enable_dnd (LIGMA_CONTAINER_VIEW (item_view),
                                  GTK_BUTTON (item_view->priv->delete_button),
                                  item_view_class->item_type);

  /* Lock box. */
  item_view->priv->lock_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, button_spacing);

  /*  Lock box: content toggle  */
  ligma_item_tree_view_add_lock (item_view,
                                item_view_class->lock_content_icon_name,
                                (LigmaIsLockedFunc) ligma_item_get_lock_content,
                                (LigmaCanLockFunc)  ligma_item_can_lock_content,
                                (LigmaSetLockFunc)  ligma_item_set_lock_content,
                                (LigmaUndoLockPush) ligma_image_undo_push_item_lock_content,
                                "lock-content-changed",
                                LIGMA_UNDO_ITEM_LOCK_CONTENT,
                                LIGMA_UNDO_GROUP_ITEM_LOCK_CONTENTS,
                                _("Lock content"),
                                _("Unlock content"),
                                _("Set Item Exclusive Content Lock"),
                                item_view_class->lock_content_tooltip,
                                item_view_class->lock_content_help_id);

  /*  Lock box: position toggle  */
  ligma_item_tree_view_add_lock (item_view,
                                item_view_class->lock_position_icon_name,
                                (LigmaIsLockedFunc) ligma_item_get_lock_position,
                                (LigmaCanLockFunc)  ligma_item_can_lock_position,
                                (LigmaSetLockFunc)  ligma_item_set_lock_position,
                                (LigmaUndoLockPush) ligma_image_undo_push_item_lock_position,
                                "lock-position-changed",
                                LIGMA_UNDO_ITEM_LOCK_POSITION,
                                LIGMA_UNDO_GROUP_ITEM_LOCK_POSITION,
                                _("Lock position"),
                                _("Unlock position"),
                                _("Set Item Exclusive Position Lock"),
                                item_view_class->lock_position_tooltip,
                                item_view_class->lock_position_help_id);

  /*  Lock box: visibility toggle  */
  ligma_item_tree_view_add_lock (item_view,
                                item_view_class->lock_visibility_icon_name,
                                (LigmaIsLockedFunc) ligma_item_get_lock_visibility,
                                (LigmaCanLockFunc)  ligma_item_can_lock_visibility,
                                (LigmaSetLockFunc)  ligma_item_set_lock_visibility,
                                (LigmaUndoLockPush) ligma_image_undo_push_item_lock_visibility,
                                "lock-visibility-changed",
                                LIGMA_UNDO_ITEM_LOCK_VISIBILITY,
                                LIGMA_UNDO_GROUP_ITEM_LOCK_VISIBILITY,
                                _("Lock visibility"),
                                _("Unlock visibility"),
                                _("Set Item Exclusive Visibility Lock"),
                                item_view_class->lock_visibility_tooltip,
                                item_view_class->lock_visibility_help_id);

  /* Lock popover. */
  item_view->priv->lock_popover = gtk_popover_new (GTK_WIDGET (tree_view->view));
  gtk_popover_set_modal (GTK_POPOVER (item_view->priv->lock_popover), TRUE);
  g_signal_connect (item_view->priv->lock_popover,
                    "button-press-event",
                    G_CALLBACK (ligma_item_tree_view_popover_button_press),
                    item_view);
  gtk_container_add (GTK_CONTAINER (item_view->priv->lock_popover), item_view->priv->lock_box);
  gtk_widget_show (item_view->priv->lock_box);
}

static void
ligma_item_tree_view_dispose (GObject *object)
{
  LigmaItemTreeView *view = LIGMA_ITEM_TREE_VIEW (object);

  if (view->priv->image)
    ligma_item_tree_view_set_image (view, NULL);

  if (view->priv->lock_popover)
    {
      gtk_widget_destroy (view->priv->lock_popover);
      view->priv->lock_popover = NULL;
    }

  if (view->priv->lock_box_path)
    {
      gtk_tree_path_free (view->priv->lock_box_path);
      view->priv->lock_box_path = NULL;
    }

  if (view->priv->locks)
    {
      g_list_free_full (view->priv->locks,
                        (GDestroyNotify) g_free);
      view->priv->locks = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_item_tree_view_style_updated (GtkWidget *widget)
{
  LigmaItemTreeView *view = LIGMA_ITEM_TREE_VIEW (widget);
  GList            *children;
  GList            *list;
  const gchar      *old_icon_name;
  gchar            *icon_name;
  GtkReliefStyle    button_relief;
  GtkIconSize       old_size;
  GtkIconSize       button_icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
  gint              pixel_icon_size  = 16;
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

                  gtk_image_get_icon_name (GTK_IMAGE (image),
                                           &old_icon_name, &old_size);

                  if (button_icon_size != old_size)
                    {
                      icon_name = g_strdup (old_icon_name);
                      gtk_image_set_from_icon_name (GTK_IMAGE (image),
                                                    icon_name, button_icon_size);
                      g_free (icon_name);
                    }
                }
            }
        }

      g_list_free (children);
    }

  gtk_image_get_icon_name (GTK_IMAGE (view->priv->lock_header_image),
                           &old_icon_name, &old_size);

  if (button_icon_size != old_size)
    {
      icon_name = g_strdup (old_icon_name);
      gtk_image_set_from_icon_name (GTK_IMAGE (view->priv->lock_header_image),
                                    icon_name, button_icon_size);
      g_free (icon_name);
    }

  gtk_image_get_icon_name (GTK_IMAGE (view->priv->eye_header_image),
                           &old_icon_name, &old_size);
  if (button_icon_size != old_size)
    {
      icon_name = g_strdup (old_icon_name);
      gtk_image_set_from_icon_name (GTK_IMAGE (view->priv->eye_header_image),
                                    icon_name, button_icon_size);
      g_free (icon_name);
    }

  /* force the eye and toggle cells to recreate their icon */
  gtk_icon_size_lookup (button_icon_size, &pixel_icon_size, NULL);
  g_object_set (view->priv->eye_cell,
                "icon-name", LIGMA_ICON_VISIBLE,
                "icon-size", pixel_icon_size,
                NULL);
  g_object_set (view->priv->lock_cell,
                "icon-name", LIGMA_ICON_LOCK_MULTI,
                "icon-size", pixel_icon_size,
                NULL);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);
}

GtkWidget *
ligma_item_tree_view_new (GType            view_type,
                         gint             view_size,
                         gint             view_border_width,
                         gboolean         multiple_selection,
                         LigmaImage       *image,
                         LigmaMenuFactory *menu_factory,
                         const gchar     *menu_identifier,
                         const gchar     *ui_path)
{
  LigmaItemTreeView *item_view;

  g_return_val_if_fail (g_type_is_a (view_type, LIGMA_TYPE_ITEM_TREE_VIEW), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= LIGMA_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= LIGMA_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (image == NULL || LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (menu_identifier != NULL, NULL);
  g_return_val_if_fail (ui_path != NULL, NULL);

  item_view = g_object_new (view_type,
                            "reorderable",     TRUE,
                            "menu-factory",    menu_factory,
                            "menu-identifier", menu_identifier,
                            "ui-path",         ui_path,
                            "selection-mode",  multiple_selection ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE,
                            NULL);

  ligma_container_view_set_view_size (LIGMA_CONTAINER_VIEW (item_view),
                                     view_size, view_border_width);

  ligma_item_tree_view_set_image (item_view, image);

  return GTK_WIDGET (item_view);
}

void
ligma_item_tree_view_set_image (LigmaItemTreeView *view,
                               LigmaImage        *image)
{
  g_return_if_fail (LIGMA_IS_ITEM_TREE_VIEW (view));
  g_return_if_fail (image == NULL || LIGMA_IS_IMAGE (image));

  g_signal_emit (view, view_signals[SET_IMAGE], 0, image);

  ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (view)), view);
}

LigmaImage *
ligma_item_tree_view_get_image (LigmaItemTreeView *view)
{
  g_return_val_if_fail (LIGMA_IS_ITEM_TREE_VIEW (view), NULL);

  return view->priv->image;
}

void
ligma_item_tree_view_add_options (LigmaItemTreeView *view,
                                 const gchar      *label,
                                 GtkWidget        *options)
{
  gint content_spacing;
  gint button_spacing;

  g_return_if_fail (LIGMA_IS_ITEM_TREE_VIEW (view));
  g_return_if_fail (GTK_IS_WIDGET (options));

  gtk_widget_style_get (GTK_WIDGET (view),
                        "content-spacing", &content_spacing,
                        "button-spacing",  &button_spacing,
                        NULL);

  if (! view->priv->options_box)
    {
      LigmaItemTreeViewClass *item_view_class;

      item_view_class = LIGMA_ITEM_TREE_VIEW_GET_CLASS (view);

      view->priv->options_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, content_spacing);
      gtk_box_pack_start (GTK_BOX (view), view->priv->options_box,
                          FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (view), view->priv->options_box, 0);
      gtk_widget_show (view->priv->options_box);

      if (! view->priv->image ||
          ! item_view_class->get_selected_items (view->priv->image))
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

void
ligma_item_tree_view_add_lock (LigmaItemTreeView *view,
                              const gchar      *icon_name,
                              LigmaIsLockedFunc  is_locked,
                              LigmaCanLockFunc   can_lock,
                              LigmaSetLockFunc   lock,
                              LigmaUndoLockPush  undo_push,
                              const gchar      *signal_name,
                              LigmaUndoType      undo_type,
                              LigmaUndoType      group_undo_type,
                              const gchar      *undo_lock_desc,
                              const gchar      *undo_unlock_desc,
                              const gchar      *undo_exclusive_desc,
                              const gchar      *tooltip,
                              const gchar      *help_id)
{
  LockToggle  *data;
  GtkWidget   *toggle = gtk_toggle_button_new ();
  GtkWidget   *image;
  GtkIconSize  icon_size;

  data = g_new0 (LockToggle, 1);
  data->toggle       = toggle;
  data->icon_name    = icon_name;
  data->is_locked    = is_locked;
  data->can_lock     = can_lock;
  data->lock         = lock;
  data->undo_push    = undo_push;
  data->signal_name  = signal_name;
  data->tooltip      = tooltip;
  data->help_id      = help_id;

  data->undo_type           = undo_type;
  data->group_undo_type     = group_undo_type;
  data->undo_lock_desc      = undo_lock_desc;
  data->undo_unlock_desc    = undo_unlock_desc;
  data->undo_exclusive_desc = undo_exclusive_desc;

  view->priv->locks = g_list_prepend (view->priv->locks, data);

  gtk_box_pack_end (GTK_BOX (view->priv->lock_box), toggle, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (view->priv->lock_box), toggle, 0);
  gtk_widget_show (toggle);

  g_object_set_data (G_OBJECT (toggle), "lock-data", data);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (ligma_item_tree_view_lock_toggled),
                    view);
  g_signal_connect (toggle, "button-release-event",
                    G_CALLBACK (ligma_item_tree_view_lock_button_release),
                    view);

  ligma_help_set_help_data (toggle, tooltip, help_id);

  gtk_widget_style_get (GTK_WIDGET (view),
                        "button-icon-size", &icon_size,
                        NULL);

  image = gtk_image_new_from_icon_name (icon_name, icon_size);
  gtk_container_add (GTK_CONTAINER (toggle), image);
  gtk_widget_show (image);
}

void
ligma_item_tree_view_blink_lock (LigmaItemTreeView *view,
                                LigmaItem         *item)
{
  GtkTreeIter  *iter;
  GtkTreePath  *path;
  GdkRectangle  rect;

  g_return_if_fail (LIGMA_IS_ITEM_TREE_VIEW (view));
  g_return_if_fail (LIGMA_IS_ITEM (item));

  /* Find the item in the tree view. */
  iter = ligma_container_view_lookup (LIGMA_CONTAINER_VIEW (view),
                                     (LigmaViewable *) item);
  path = gtk_tree_model_get_path (LIGMA_CONTAINER_TREE_VIEW (view)->model, iter);

  /* Scroll dockable to make sure the cell is showing. */
  gtk_tree_view_scroll_to_cell (LIGMA_CONTAINER_TREE_VIEW (view)->view, path,
                                gtk_tree_view_get_column (LIGMA_CONTAINER_TREE_VIEW (view)->view, 1),
                                FALSE, 0.0, 0.0);

  /* Now blink the lock cell of the specified item. */
  gtk_tree_view_get_cell_area (LIGMA_CONTAINER_TREE_VIEW (view)->view, path,
                               gtk_tree_view_get_column (LIGMA_CONTAINER_TREE_VIEW (view)->view, 1),
                               &rect);
  gtk_tree_view_convert_bin_window_to_widget_coords (LIGMA_CONTAINER_TREE_VIEW (view)->view,
                                                     rect.x, rect.y, &rect.x, &rect.y);
  ligma_widget_blink_rect (GTK_WIDGET (LIGMA_CONTAINER_TREE_VIEW (view)->view), &rect);

  gtk_tree_path_free (path);
}

GtkWidget *
ligma_item_tree_view_get_new_button (LigmaItemTreeView *view)
{
  g_return_val_if_fail (LIGMA_IS_ITEM_TREE_VIEW (view), NULL);

  return view->priv->new_button;
}

GtkWidget *
ligma_item_tree_view_get_delete_button (LigmaItemTreeView *view)
{
  g_return_val_if_fail (LIGMA_IS_ITEM_TREE_VIEW (view), NULL);

  return view->priv->delete_button;
}

gint
ligma_item_tree_view_get_drop_index (LigmaItemTreeView         *view,
                                    LigmaViewable             *dest_viewable,
                                    GtkTreeViewDropPosition   drop_pos,
                                    LigmaViewable            **parent)
{
  gint index = -1;

  g_return_val_if_fail (LIGMA_IS_ITEM_TREE_VIEW (view), -1);
  g_return_val_if_fail (dest_viewable == NULL ||
                        LIGMA_IS_VIEWABLE (dest_viewable), -1);
  g_return_val_if_fail (parent != NULL, -1);

  *parent = NULL;

  if (dest_viewable)
    {
      *parent = ligma_viewable_get_parent (dest_viewable);
      index   = ligma_item_get_index (LIGMA_ITEM (dest_viewable));

      if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
        {
          LigmaContainer *children = ligma_viewable_get_children (dest_viewable);

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
ligma_item_tree_view_real_set_image (LigmaItemTreeView *view,
                                    LigmaImage        *image)
{
  if (view->priv->image == image)
    return;

  if (view->priv->image)
    {
      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            ligma_item_tree_view_item_changed,
                                            view);
      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            ligma_item_tree_view_size_changed,
                                            view);

      ligma_container_view_set_container (LIGMA_CONTAINER_VIEW (view), NULL);

      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            ligma_item_tree_view_image_flush,
                                            view);
    }

  view->priv->image = image;

  if (view->priv->image)
    {
      LigmaContainer *container;

      container =
        LIGMA_ITEM_TREE_VIEW_GET_CLASS (view)->get_container (view->priv->image);

      ligma_container_view_set_container (LIGMA_CONTAINER_VIEW (view), container);

      g_signal_connect (view->priv->image,
                        LIGMA_ITEM_TREE_VIEW_GET_CLASS (view)->signal_name,
                        G_CALLBACK (ligma_item_tree_view_item_changed),
                        view);
      g_signal_connect (view->priv->image, "size-changed",
                        G_CALLBACK (ligma_item_tree_view_size_changed),
                        view);

      g_signal_connect (view->priv->image, "flush",
                        G_CALLBACK (ligma_item_tree_view_image_flush),
                        view);

      ligma_item_tree_view_item_changed (view->priv->image, view);
    }
}

static void
ligma_item_tree_view_image_flush (LigmaImage        *image,
                                 gboolean          invalidate_preview,
                                 LigmaItemTreeView *view)
{
  ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (view)), view);
}


/*  LigmaContainerView methods  */

static void
ligma_item_tree_view_set_container (LigmaContainerView *view,
                                   LigmaContainer     *container)
{
  LigmaItemTreeView *item_view = LIGMA_ITEM_TREE_VIEW (view);
  LigmaContainer    *old_container;
  GList            *list;

  old_container = ligma_container_view_get_container (view);

  if (old_container)
    {
      ligma_tree_handler_disconnect (item_view->priv->visible_changed_handler);
      item_view->priv->visible_changed_handler = NULL;

      ligma_tree_handler_disconnect (item_view->priv->color_tag_changed_handler);
      item_view->priv->color_tag_changed_handler = NULL;

      for (list = item_view->priv->locks; list; list = list->next)
        {
          LockToggle *data = list->data;

          ligma_tree_handler_disconnect (data->changed_handler);
          data->changed_handler = NULL;
        }
    }

  parent_view_iface->set_container (view, container);

  if (container)
    {
      item_view->priv->visible_changed_handler =
        ligma_tree_handler_connect (container, "visibility-changed",
                                   G_CALLBACK (ligma_item_tree_view_visible_changed),
                                   view);

      item_view->priv->color_tag_changed_handler =
        ligma_tree_handler_connect (container, "color-tag-changed",
                                   G_CALLBACK (ligma_item_tree_view_color_tag_changed),
                                   view);

      for (list = item_view->priv->locks; list; list = list->next)
        {
          LockToggle *data = list->data;

          data->changed_handler = ligma_tree_handler_connect (container,
                                                             data->signal_name,
                                                             G_CALLBACK (ligma_item_tree_view_lock_changed),
                                                             view);
        }
    }
}

static void
ligma_item_tree_view_set_context (LigmaContainerView *view,
                                 LigmaContext       *context)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
  LigmaItemTreeView      *item_view = LIGMA_ITEM_TREE_VIEW (view);
  LigmaImage             *image     = NULL;
  LigmaContext           *old_context;

  old_context = ligma_container_view_get_context (view);

  if (old_context)
    {
      g_signal_handlers_disconnect_by_func (old_context,
                                            ligma_item_tree_view_set_image,
                                            item_view);
      g_signal_handlers_disconnect_by_func (old_context->ligma->config,
                                            G_CALLBACK (ligma_item_tree_view_style_updated),
                                            item_view);
    }

  parent_view_iface->set_context (view, context);

  if (context)
    {
      if (! tree_view->dnd_ligma)
        tree_view->dnd_ligma = context->ligma;

      g_signal_connect_swapped (context, "image-changed",
                                G_CALLBACK (ligma_item_tree_view_set_image),
                                item_view);

      g_signal_connect_object (context->ligma->config,
                               "notify::theme",
                               G_CALLBACK (ligma_item_tree_view_style_updated),
                               item_view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (context->ligma->config,
                               "notify::override-theme-icon-size",
                               G_CALLBACK (ligma_item_tree_view_style_updated),
                               item_view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (context->ligma->config,
                               "notify::custom-icon-size",
                               G_CALLBACK (ligma_item_tree_view_style_updated),
                               item_view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

      image = ligma_context_get_image (context);
    }

  ligma_item_tree_view_set_image (item_view, image);
}

static gpointer
ligma_item_tree_view_insert_item (LigmaContainerView *view,
                                 LigmaViewable      *viewable,
                                 gpointer           parent_insert_data,
                                 gint               index)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
  LigmaItemTreeView      *item_view = LIGMA_ITEM_TREE_VIEW (view);
  LigmaItem              *item      = LIGMA_ITEM (viewable);
  GtkTreeIter           *iter;
  LigmaRGB                color;
  gboolean               has_color;
  const gchar           *icon_name;
  gint                   n_locks;

  iter = parent_view_iface->insert_item (view, viewable,
                                         parent_insert_data, index);

  has_color = ligma_get_color_tag_color (ligma_item_get_merged_color_tag (item),
                                        &color,
                                        ligma_item_get_color_tag (item) ==
                                        LIGMA_COLOR_TAG_NONE);

  n_locks = ligma_item_tree_view_get_n_locks (item_view, item, &icon_name);

  gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,

                      item_view->priv->model_column_visible,
                      ligma_item_get_visible (item),

                      item_view->priv->model_column_viewable,
                      ligma_item_get_visible (item) &&
                      ! ligma_item_is_visible (item),

                      item_view->priv->model_column_locked,
                      n_locks > 0,

                      item_view->priv->model_column_lock_icon,
                      icon_name,

                      item_view->priv->model_column_color_tag,
                      has_color ? (GdkRGBA *) &color : NULL,
                      -1);

  return iter;
}

static void
ligma_item_tree_view_insert_items_after (LigmaContainerView *view)
{
  LigmaItemTreeView      *item_view = LIGMA_ITEM_TREE_VIEW (view);
  LigmaItemTreeViewClass *item_view_class;
  GList                 *selected_items;

  item_view_class = LIGMA_ITEM_TREE_VIEW_GET_CLASS (item_view);

  selected_items = item_view_class->get_selected_items (item_view->priv->image);
  ligma_container_view_select_items (view, selected_items);
}

static gboolean
ligma_item_tree_view_select_items (LigmaContainerView *view,
                                  GList             *items,
                                  GList             *paths)
{
  LigmaItemTreeView *tree_view         = LIGMA_ITEM_TREE_VIEW (view);
  gboolean          options_sensitive = FALSE;
  gboolean          success;

  success = parent_view_iface->select_items (view, items, paths);

  if (items)
    {
      LigmaItemTreeViewClass *item_view_class;

      item_view_class = LIGMA_ITEM_TREE_VIEW_GET_CLASS (tree_view);
      if (TRUE) /* XXX: test if new selection same as old. */
        {
          item_view_class->set_selected_items (tree_view->priv->image, items);
          ligma_image_flush (tree_view->priv->image);

          items = item_view_class->get_selected_items (tree_view->priv->image);
        }

      options_sensitive = TRUE;
    }

  ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (tree_view)), tree_view);

  if (tree_view->priv->options_box)
    gtk_widget_set_sensitive (tree_view->priv->options_box, options_sensitive);

  return success;
}

static void
ligma_item_tree_view_activate_item (LigmaContainerView *view,
                                   LigmaViewable      *item,
                                   gpointer           insert_data)
{
  LigmaItemTreeViewClass *item_view_class = LIGMA_ITEM_TREE_VIEW_GET_CLASS (view);

  if (parent_view_iface->activate_item)
    parent_view_iface->activate_item (view, item, insert_data);

  if (item_view_class->activate_action)
    {
      ligma_ui_manager_activate_action (ligma_editor_get_ui_manager (LIGMA_EDITOR (view)),
                                       item_view_class->action_group,
                                       item_view_class->activate_action);
    }
}

static gboolean
ligma_item_tree_view_drop_possible (LigmaContainerTreeView   *tree_view,
                                   LigmaDndType              src_type,
                                   GList                   *src_viewables,
                                   LigmaViewable            *dest_viewable,
                                   GtkTreePath             *drop_path,
                                   GtkTreeViewDropPosition  drop_pos,
                                   GtkTreeViewDropPosition *return_drop_pos,
                                   GdkDragAction           *return_drag_action)
{
  GList    *iter;
  gboolean  other_image_items;

  if (src_viewables)
    other_image_items = TRUE;
  else
    other_image_items = FALSE;

  for (iter = src_viewables; iter; iter = iter->next)
    {
      LigmaViewable *src_viewable = iter->data;

      if (! LIGMA_IS_ITEM (src_viewable) ||
          (dest_viewable != NULL &&
           ligma_item_get_image (LIGMA_ITEM (src_viewable)) ==
           ligma_item_get_image (LIGMA_ITEM (dest_viewable))))
        {
          /* Not an item or from the same image. */
          other_image_items = FALSE;
          break;
        }
    }

  if (other_image_items)
    {
      if (return_drop_pos)
        *return_drop_pos = drop_pos;

      if (return_drag_action)
        *return_drag_action = GDK_ACTION_COPY;

      return TRUE;
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
ligma_item_tree_view_drop_viewables (LigmaContainerTreeView   *tree_view,
                                    GList                   *src_viewables,
                                    LigmaViewable            *dest_viewable,
                                    GtkTreeViewDropPosition  drop_pos)
{
  LigmaItemTreeViewClass *item_view_class;
  LigmaItemTreeView      *item_view              = LIGMA_ITEM_TREE_VIEW (tree_view);
  GList                 *iter;
  LigmaImage             *src_image              = NULL;
  GType                  src_viewable_type      = G_TYPE_NONE;
  gint                   dest_index             = -1;
  gboolean               src_viewables_reversed = FALSE;

  g_return_if_fail (g_list_length (src_viewables) > 0);

  item_view_class = LIGMA_ITEM_TREE_VIEW_GET_CLASS (item_view);

  for (iter = src_viewables; iter; iter = iter->next)
    {
      LigmaViewable *src_viewable = iter->data;

      /* All dropped viewables must be of the same finale type and come
       * from the same source image.
       */
      if (src_viewable_type == G_TYPE_NONE)
        src_viewable_type = G_TYPE_FROM_INSTANCE (src_viewable);
      else
        {
          if (g_type_is_a (src_viewable_type,
                           G_TYPE_FROM_INSTANCE (src_viewable)))
            /* It is possible to move different types of a same
             * parenting hierarchy, for instance LigmaLayer and
             * LigmaGroupLayer.
             */
            src_viewable_type = G_TYPE_FROM_INSTANCE (src_viewable);

          g_return_if_fail (g_type_is_a (G_TYPE_FROM_INSTANCE (src_viewable),
                                         src_viewable_type));
        }

      if (src_image == NULL)
        src_image = ligma_item_get_image (LIGMA_ITEM (iter->data));
      else
        g_return_if_fail (src_image == ligma_item_get_image (LIGMA_ITEM (iter->data)));
    }

  if (drop_pos == GTK_TREE_VIEW_DROP_AFTER ||
      (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER &&
       dest_viewable                                &&
       ligma_viewable_get_children (dest_viewable)))
    {
      src_viewables_reversed = TRUE;
      src_viewables = g_list_reverse (src_viewables);
    }

  if (item_view->priv->image != src_image ||
      ! g_type_is_a (src_viewable_type, item_view_class->item_type))
    {
      GType item_type = item_view_class->item_type;

      ligma_image_undo_group_start (item_view->priv->image,
                                   LIGMA_UNDO_GROUP_LAYER_ADD,
                                   _("Drop layers"));

      for (iter = src_viewables; iter; iter = iter->next)
        {
          LigmaViewable *src_viewable = iter->data;
          LigmaItem     *new_item;
          LigmaItem     *parent;

          if (g_type_is_a (src_viewable_type, item_type))
            item_type = G_TYPE_FROM_INSTANCE (src_viewable);

          dest_index = ligma_item_tree_view_get_drop_index (item_view, dest_viewable,
                                                           drop_pos,
                                                           (LigmaViewable **) &parent);

          new_item = ligma_item_convert (LIGMA_ITEM (src_viewable),
                                        item_view->priv->image, item_type);

          item_view_class->add_item (item_view->priv->image, new_item,
                                     parent, dest_index, TRUE);
        }
    }
  else if (dest_viewable)
    {
      ligma_image_undo_group_start (item_view->priv->image,
                                   LIGMA_UNDO_GROUP_IMAGE_ITEM_REORDER,
                                   LIGMA_ITEM_GET_CLASS (src_viewables->data)->reorder_desc);

      for (iter = src_viewables; iter; iter = iter->next)
        {
          LigmaViewable *src_viewable = iter->data;
          LigmaItem     *src_parent;
          LigmaItem     *dest_parent;
          gint          src_index;
          gint          dest_index;

          src_parent = LIGMA_ITEM (ligma_viewable_get_parent (src_viewable));
          src_index  = ligma_item_get_index (LIGMA_ITEM (src_viewable));

          dest_index = ligma_item_tree_view_get_drop_index (item_view, dest_viewable,
                                                           drop_pos,
                                                           (LigmaViewable **) &dest_parent);

          if (src_parent == dest_parent)
            {
              if (src_index < dest_index)
                dest_index--;
            }

          ligma_image_reorder_item (item_view->priv->image,
                                   LIGMA_ITEM (src_viewable),
                                   dest_parent,
                                   dest_index,
                                   TRUE, NULL);
        }
    }

  if (src_viewables_reversed)
    /* The caller keeps a copy to src_viewables to free it. If we
     * reverse it, the pointer stays valid yet ends up pointing to the
     * now last (previously first) element of the list. So we leak the
     * whole list but this element. Let's reverse back the list to have
     * next and prev pointers same as call time.
     */
    src_viewables = g_list_reverse (src_viewables);

  ligma_image_undo_group_end (item_view->priv->image);
  ligma_image_flush (item_view->priv->image);
}


/*  "New" functions  */

static void
ligma_item_tree_view_new_dropped (GtkWidget    *widget,
                                 gint          x,
                                 gint          y,
                                 LigmaViewable *viewable,
                                 gpointer      data)
{
  LigmaItemTreeViewClass *item_view_class = LIGMA_ITEM_TREE_VIEW_GET_CLASS (data);
  LigmaContainerView     *view            = LIGMA_CONTAINER_VIEW (data);

  if (item_view_class->new_default_action &&
      viewable && ligma_container_view_lookup (view, viewable))
    {
      LigmaAction *action;

      action = ligma_ui_manager_find_action (ligma_editor_get_ui_manager (LIGMA_EDITOR (view)),
                                            item_view_class->action_group,
                                            item_view_class->new_default_action);

      if (action)
        {
          g_object_set (action, "viewable", viewable, NULL);
          ligma_action_activate (action);
          g_object_set (action, "viewable", NULL, NULL);
        }
    }
}

static void
ligma_item_tree_view_new_list_dropped (GtkWidget    *widget,
                                      gint          x,
                                      gint          y,
                                      GList        *viewables,
                                      gpointer      data)
{
  LigmaItemTreeViewClass *item_view_class = LIGMA_ITEM_TREE_VIEW_GET_CLASS (data);
  LigmaContainerView     *view            = LIGMA_CONTAINER_VIEW (data);
  LigmaAction            *action;

  action = ligma_ui_manager_find_action (ligma_editor_get_ui_manager (LIGMA_EDITOR (view)),
                                        item_view_class->action_group,
                                        item_view_class->new_default_action);

  if (item_view_class->new_default_action && viewables && action &&
      ligma_container_view_contains (view, viewables))
    ligma_action_activate (action);
}

/*  LigmaImage callbacks  */

static void
ligma_item_tree_view_item_changed (LigmaImage        *image,
                                  LigmaItemTreeView *view)
{
  GList *items;

  items = LIGMA_ITEM_TREE_VIEW_GET_CLASS (view)->get_selected_items (view->priv->image);

  ligma_container_view_select_items (LIGMA_CONTAINER_VIEW (view), items);
}

static void
ligma_item_tree_view_size_changed (LigmaImage        *image,
                                  LigmaItemTreeView *tree_view)
{
  LigmaContainerView *view = LIGMA_CONTAINER_VIEW (tree_view);
  gint               view_size;
  gint               border_width;

  view_size = ligma_container_view_get_view_size (view, &border_width);

  ligma_container_view_set_view_size (view, view_size, border_width);
}

static void
ligma_item_tree_view_name_edited (GtkCellRendererText *cell,
                                 const gchar         *path_str,
                                 const gchar         *new_name,
                                 LigmaItemTreeView    *view)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
  GtkTreePath           *path;
  GtkTreeIter            iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      LigmaViewRenderer *renderer;
      LigmaItem         *item;
      const gchar      *old_name;
      GError           *error = NULL;

      renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (tree_view->model),
                                                         &iter);

      item = LIGMA_ITEM (renderer->viewable);

      old_name = ligma_object_get_name (item);

      if (! old_name) old_name = "";
      if (! new_name) new_name = "";

      if (strcmp (old_name, new_name) &&
          ligma_item_rename (item, new_name, &error))
        {
          ligma_image_flush (ligma_item_get_image (item));
        }
      else
        {
          gchar *name = ligma_viewable_get_description (renderer->viewable, NULL);

          gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), &iter,
                              LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME, name,
                              -1);
          g_free (name);

          if (error)
            {
              ligma_message_literal (view->priv->image->ligma, G_OBJECT (view),
                                    LIGMA_MESSAGE_WARNING,
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
ligma_item_tree_view_visible_changed (LigmaItem         *item,
                                     LigmaItemTreeView *view)
{
  LigmaContainerView     *container_view = LIGMA_CONTAINER_VIEW (view);
  LigmaContainerTreeView *tree_view      = LIGMA_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter;

  iter = ligma_container_view_lookup (container_view,
                                     (LigmaViewable *) item);

  if (iter)
    {
      LigmaContainer *children;

      gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                          view->priv->model_column_visible,
                          ligma_item_get_visible (item),
                          view->priv->model_column_viewable,
                          ligma_item_get_visible (item) &&
                          ! ligma_item_is_visible (item),
                          -1);

      children = ligma_viewable_get_children (LIGMA_VIEWABLE (item));

      if (children)
        ligma_container_foreach (children,
                                (GFunc) ligma_item_tree_view_visible_changed,
                                view);
    }
}

static void
ligma_item_tree_view_eye_clicked (GtkCellRendererToggle *toggle,
                                 gchar                 *path_str,
                                 GdkModifierType        state,
                                 LigmaItemTreeView      *view)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
  GtkTreePath           *path;
  GtkTreeIter            iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      LigmaContext      *context;
      LigmaViewRenderer *renderer;
      LigmaItem         *item;
      LigmaImage        *image;
      gboolean          active;

      context = ligma_container_view_get_context (LIGMA_CONTAINER_VIEW (view));

      renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (tree_view->model),
                                                         &iter);
      g_object_get (toggle,
                    "active", &active,
                    NULL);

      item = LIGMA_ITEM (renderer->viewable);
      g_object_unref (renderer);

      image = ligma_item_get_image (item);

      if ((state & GDK_SHIFT_MASK) ||
          (state & GDK_MOD1_MASK))
        {
          ligma_item_toggle_exclusive_visible (item, (state & GDK_MOD1_MASK), context);
        }
      else
        {
          LigmaUndo *undo;
          gboolean  push_undo = TRUE;

          undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_ITEM_UNDO,
                                               LIGMA_UNDO_ITEM_VISIBILITY);

          if (undo && LIGMA_ITEM_UNDO (undo)->item == item)
            push_undo = FALSE;

          if (! ligma_item_set_visible (item, ! active, push_undo))
            ligma_item_tree_view_blink_lock (view, item);

          if (!push_undo)
            ligma_undo_refresh_preview (undo, context);
        }

      ligma_image_flush (image);
    }

  gtk_tree_path_free (path);
}


/*  "Locked" callbacks  */

static void
ligma_item_tree_view_lock_clicked (GtkCellRendererToggle *toggle,
                                  gchar                 *path_str,
                                  GdkModifierType        state,
                                  LigmaItemTreeView      *view)
{
  GtkTreePath *path;
  GtkTreeIter  iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (LIGMA_CONTAINER_TREE_VIEW (view)->model,
                               &iter, path))
    {
      LigmaViewRenderer       *renderer;
      LigmaContainerTreeStore *store;
      LigmaItem               *item;
      GdkRectangle            rect;

      /* Update the lock state. */
      store = LIGMA_CONTAINER_TREE_STORE (LIGMA_CONTAINER_TREE_VIEW (view)->model);
      renderer = ligma_container_tree_store_get_renderer (store, &iter);
      item = LIGMA_ITEM (renderer->viewable);
      g_object_unref (renderer);
      ligma_item_tree_view_update_lock_box (view, item, path);

      /* Change popover position. */
      gtk_tree_view_get_cell_area (LIGMA_CONTAINER_TREE_VIEW (view)->view, path,
                                   gtk_tree_view_get_column (LIGMA_CONTAINER_TREE_VIEW (view)->view, 1),
                                   &rect);
      gtk_tree_view_convert_bin_window_to_widget_coords (LIGMA_CONTAINER_TREE_VIEW (view)->view,
                                                         rect.x, rect.y, &rect.x, &rect.y);
      gtk_popover_set_pointing_to (GTK_POPOVER (view->priv->lock_popover), &rect);

      gtk_widget_show (view->priv->lock_popover);
    }
}


/*  "Color Tag" callbacks  */

static void
ligma_item_tree_view_color_tag_changed (LigmaItem         *item,
                                       LigmaItemTreeView *view)
{
  LigmaContainerView     *container_view = LIGMA_CONTAINER_VIEW (view);
  LigmaContainerTreeView *tree_view      = LIGMA_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter;

  iter = ligma_container_view_lookup (container_view,
                                     (LigmaViewable *) item);

  if (iter)
    {
      LigmaContainer *children;
      LigmaRGB        color;
      gboolean       has_color;

      has_color = ligma_get_color_tag_color (ligma_item_get_merged_color_tag (item),
                                            &color,
                                            ligma_item_get_color_tag (item) ==
                                            LIGMA_COLOR_TAG_NONE);

      gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                          view->priv->model_column_color_tag,
                          has_color ? (GdkRGBA *) &color : NULL,
                          -1);

      children = ligma_viewable_get_children (LIGMA_VIEWABLE (item));

      if (children)
        ligma_container_foreach (children,
                                (GFunc) ligma_item_tree_view_color_tag_changed,
                                view);
    }
}


/*  "Lock Content" callbacks  */

static void
ligma_item_tree_view_lock_changed (LigmaItem         *item,
                                  LigmaItemTreeView *view)
{
  LigmaContainerView     *container_view  = LIGMA_CONTAINER_VIEW (view);
  LigmaContainerTreeView *tree_view       = LIGMA_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter;
  const gchar           *icon_name;
  gint                   n_locks;

  iter = ligma_container_view_lookup (container_view,
                                     (LigmaViewable *) item);

  n_locks = ligma_item_tree_view_get_n_locks (view, item, &icon_name);

  if (iter)
    gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,

                        view->priv->model_column_locked,
                        n_locks > 0,

                        view->priv->model_column_lock_icon,
                        icon_name,

                        -1);

  if (view->priv->lock_box_item == item)
    ligma_item_tree_view_update_lock_box (view, item, NULL);
}

static gboolean
ligma_item_tree_view_lock_button_release (GtkWidget        *widget,
                                         GdkEvent         *event,
                                         LigmaItemTreeView *view)
{
  GdkEventButton  *bevent = (GdkEventButton *) event;
  LockToggle      *data;
  GdkModifierType  modifiers;

  data      = g_object_get_data (G_OBJECT (widget), "lock-data");
  modifiers = bevent->state & ligma_get_all_modifiers_mask ();

  if (modifiers == GDK_SHIFT_MASK ||
      modifiers == GDK_MOD1_MASK)
    ligma_item_toggle_exclusive (view->priv->lock_box_item,
                                data->is_locked,
                                data->lock,
                                data->can_lock,
                                NULL,
                                data->undo_push,
                                data->undo_exclusive_desc,
                                data->group_undo_type,
                                (modifiers == GDK_MOD1_MASK),
                                NULL);
  else
    return GDK_EVENT_PROPAGATE;

  return GDK_EVENT_STOP;
}

static void
ligma_item_tree_view_lock_toggled (GtkWidget         *widget,
                                  LigmaItemTreeView  *view)
{
  LigmaImage   *image = view->priv->image;
  LockToggle  *data;
  LigmaUndo    *undo;
  const gchar *undo_label;
  gboolean     push_undo = TRUE;
  gboolean     locked;

  locked = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
  data = g_object_get_data (G_OBJECT (widget), "lock-data");

  if (! data->can_lock (view->priv->lock_box_item) ||
      locked == data->is_locked (view->priv->lock_box_item))
    return;

  undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_ITEM_UNDO,
                                       data->undo_type);

  if (undo && LIGMA_ITEM_UNDO (undo)->item == view->priv->lock_box_item)
    push_undo = FALSE;

  if (push_undo)
    {
      if (locked)
        undo_label = data->undo_lock_desc;
      else
        undo_label = data->undo_unlock_desc;

      ligma_image_undo_group_start (image, data->group_undo_type,
                                   undo_label);
    }

  /* TODO: maybe we should also have a modifier-interaction where we can
   * lock all same-level items (Shift-click for instance like eye click)
   * and maybe another modifier for all selected items at once.
   */
  data->lock (view->priv->lock_box_item, locked, push_undo);

  ligma_image_flush (image);
  if (push_undo)
    ligma_image_undo_group_end (image);
}

static gboolean
ligma_item_tree_view_item_pre_clicked (LigmaCellRendererViewable *cell,
                                      const gchar              *path_str,
                                      GdkModifierType           state,
                                      LigmaItemTreeView         *item_view)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (item_view);
  GtkTreePath           *path;
  GtkTreeIter            iter;
  GdkModifierType        modifiers = (state & ligma_get_all_modifiers_mask ());
  gboolean               handled   = FALSE;

  path = gtk_tree_path_new_from_string (path_str);

  /* Alt modifier at least and none other than Alt, Shift and Ctrl. */
  if ((modifiers & GDK_MOD1_MASK)                                          &&
      ! (modifiers & ~(GDK_MOD1_MASK | GDK_SHIFT_MASK | GDK_CONTROL_MASK)) &&
      gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      LigmaImage        *image    = ligma_item_tree_view_get_image (item_view);
      LigmaViewRenderer *renderer = NULL;

      renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (tree_view->model),
                                                         &iter);

      if (renderer)
        {
          LigmaItem       *item = LIGMA_ITEM (renderer->viewable);
          LigmaChannelOps  op   = ligma_modifiers_to_channel_op (state);

          ligma_item_to_selection (item, op,
                                  TRUE, FALSE, 0.0, 0.0);
          ligma_image_flush (image);

          g_object_unref (renderer);

          /* Don't select the clicked layer */
          handled = TRUE;
        }
    }

  gtk_tree_path_free (path);

  return handled;
}

static void
ligma_item_tree_view_update_lock_box (LigmaItemTreeView *view,
                                     LigmaItem         *item,
                                     GtkTreePath      *path)
{
  GList *list;

  for (list = view->priv->locks; list; list = list->next)
    {
      LockToggle *data = list->data;

      if (data->is_locked (item) !=
          gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->toggle)))
        {
          g_signal_handlers_block_by_func (data->toggle,
                                           ligma_item_tree_view_lock_toggled,
                                           view);
          g_signal_handlers_block_by_func (data->toggle,
                                           ligma_item_tree_view_lock_button_release,
                                           view);

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->toggle),
                                        data->is_locked (item));

          g_signal_handlers_unblock_by_func (data->toggle,
                                             ligma_item_tree_view_lock_toggled,
                                             view);
          g_signal_handlers_unblock_by_func (data->toggle,
                                           ligma_item_tree_view_lock_button_release,
                                           view);
        }
      gtk_widget_set_sensitive (data->toggle, data->can_lock (item));
    }
  view->priv->lock_box_item = item;

  if (path)
    {
      if (view->priv->lock_box_path)
        gtk_tree_path_free (view->priv->lock_box_path);
      view->priv->lock_box_path = path;
    }
}

static gboolean
ligma_item_tree_view_popover_button_press (GtkWidget        *widget,
                                          GdkEvent         *event,
                                          LigmaItemTreeView *view)
{
  LigmaContainerTreeView *tree_view    = LIGMA_CONTAINER_TREE_VIEW (view);
  GdkEventButton        *bevent       = (GdkEventButton *) event;
  GdkEvent              *new_event;
  GtkTreeViewColumn     *column;
  GtkTreePath           *path;

  /* If we get to the popover signal handling, it means we didn't click
   * inside popover's buttons, which would have stopped the signal
   * first. So we were going to hide the popover anyway.
   * Nevertheless I do with with gtk_widget_hide() instead of
   * gtk_popover_popdown() because the animation stuff is very buggy
   * (sometimes the popup stays displayed even though it doesn't react,
   * blocking the GUI for a second or so).
   * Moreover I directly retransfer the event as though it was sent to
   * the item tree view. This allows much faster GUI interaction, not
   * having to click twice, once to close the popup, then a second for
   * the actual action (like eye switching or lock on another item or
   * item selection).
   */
  gtk_widget_hide (widget);

  if (gtk_tree_view_get_path_at_pos (tree_view->view,
                                     bevent->x, bevent->y,
                                     &path, &column, NULL, NULL))
    {
      /* Clicking on the lock cell from the currently displayed lock box
       * should only toggle-hide the lock box.
       */
      if (gtk_tree_path_compare (path, view->priv->lock_box_path) != 0 ||
          column != gtk_tree_view_get_column (LIGMA_CONTAINER_TREE_VIEW (view)->view, 1))
        {
          /* Propagate the press event to the tree view. */
          new_event = gdk_event_copy (event);
          g_object_unref (new_event->any.window);
          new_event->any.window     = g_object_ref (gtk_widget_get_window (GTK_WIDGET (tree_view->view)));
          new_event->any.send_event = TRUE;
          gtk_main_do_event (new_event);

          /* Also immediately pass a release event at same position.
           * Without this, we get weird pointer as though a quick drag'n
           * drop occured.
           */
          new_event->type = GDK_BUTTON_RELEASE;
          gtk_main_do_event (new_event);

          gdk_event_free (new_event);
        }
      gtk_tree_path_free (path);
    }

  return GDK_EVENT_STOP;
}


/*  GtkTreeView callbacks  */

static void
ligma_item_tree_view_row_expanded (GtkTreeView      *tree_view,
                                  GtkTreeIter      *iter,
                                  GtkTreePath      *path,
                                  LigmaItemTreeView *item_view)
{
  LigmaItemTreeViewClass *item_view_class;
  GList                 *selected_items;
  GList                 *list;

  item_view_class = LIGMA_ITEM_TREE_VIEW_GET_CLASS (item_view);
  selected_items  = item_view_class->get_selected_items (item_view->priv->image);

  if (selected_items)
    {
      LigmaContainerTreeStore *store;
      LigmaViewRenderer       *renderer;
      LigmaItem               *expanded_item;

      store    = LIGMA_CONTAINER_TREE_STORE (LIGMA_CONTAINER_TREE_VIEW (item_view)->model);
      renderer = ligma_container_tree_store_get_renderer (store, iter);
      expanded_item = LIGMA_ITEM (renderer->viewable);
      g_object_unref (renderer);

      for (list = selected_items; list; list = list->next)
        {
          /*  don't select an item while it is being inserted  */
          if (! ligma_container_view_lookup (LIGMA_CONTAINER_VIEW (item_view),
                                            list->data))
            return;

          /*  select items only if they were made visible by expanding
           *  their immediate parent. See bug #666561.
           */
          if (ligma_item_get_parent (list->data) != expanded_item)
            return;
        }

      ligma_container_view_select_items (LIGMA_CONTAINER_VIEW (item_view),
                                        selected_items);
    }
}


/*  Helper functions.  */

static gint
ligma_item_tree_view_get_n_locks (LigmaItemTreeView *view,
                                 LigmaItem         *item,
                                 const gchar     **icon_name)
{
  GList *list;
  gint   n_locks = 0;

  *icon_name = LIGMA_ICON_LOCK_MULTI;

  for (list = view->priv->locks; list; list = list->next)
    {
      LockToggle *data = list->data;

      n_locks += (gint) data->is_locked (item);
    }

  if (n_locks == 1)
    {
      for (list = view->priv->locks; list; list = list->next)
        {
          LockToggle *data = list->data;

          if (data->is_locked (item))
            {
              *icon_name = data->icon_name;
              break;
            }
        }
    }

  return n_locks;
}
