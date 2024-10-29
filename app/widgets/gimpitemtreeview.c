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
#include "libgimpwidgets/gimpwidgets-private.h"

#include "gimphelp-ids.h"
#include "widgets-types.h"

#include "actions/gimpgeglprocedure.h"
#include "actions/filters-commands.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimp-filter-history.h"
#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable-filters.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimpdrawablefilterundo.h"
#include "core/gimpfilterstack.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpitem-exclusive.h"
#include "core/gimpitemlist.h"
#include "core/gimpitemundo.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimptreehandler.h"
#include "core/gimpundostack.h"

#include "vectors/gimppath.h"

#include "gimpaction.h"
#include "gimpcontainertreestore.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimpdocked.h"
#include "gimpitemtreeview.h"
#include "gimplayertreeview.h"
#include "gimpmenufactory.h"
#include "gimpviewrenderer.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


#define COLUMN_EFFECTS_ACTIVE 3


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

  GtkWidget       *eye_header_image;
  GtkWidget       *lock_header_image;
  GtkWidget       *effect_header_image;

  GimpItem        *lock_box_item;
  GtkTreePath     *lock_box_path;
  GtkWidget       *lock_popover;
  GtkWidget       *lock_box;

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
  gint             model_column_effects;
  GtkCellRenderer *eye_cell;
  GtkCellRenderer *lock_cell;
  GtkCellRenderer *effects_cell;

  GimpTreeHandler *visible_changed_handler;
  GimpTreeHandler *color_tag_changed_handler;
  GimpTreeHandler *filters_changed_handler;
  GimpTreeHandler *filters_active_changed_handler;

  GtkWidget       *multi_selection_label;
  GtkWidget       *search_button;
  GtkWidget       *search_popover;
  GtkWidget       *new_link_button;
  GtkWidget       *link_list;
  GtkWidget       *link_entry;
  GtkWidget       *link_search_entry;
  GimpItemList    *link_pattern_set;
};

typedef struct
{
  GtkWidget        *toggle;
  const gchar      *icon_name;

  GimpIsLockedFunc  is_locked;
  GimpCanLockFunc   can_lock;
  GimpSetLockFunc   lock;
  GimpUndoLockPush  undo_push;

  const gchar      *tooltip;
  const gchar      *help_id;

  /* Signal handling when lock is changed from core. */
  const gchar      *signal_name;
  GimpTreeHandler  *changed_handler;

  /* Undo types and labels. */
  GimpUndoType      undo_type;
  GimpUndoType      group_undo_type;
  const gchar      *undo_lock_desc;
  const gchar      *undo_unlock_desc;
  const gchar      *undo_exclusive_desc;
} LockToggle;


static void   gimp_item_tree_view_view_iface_init   (GimpContainerViewInterface *view_iface);
static void   gimp_item_tree_view_docked_iface_init (GimpDockedInterface *docked_iface);

static void   gimp_item_tree_view_constructed       (GObject           *object);
static void   gimp_item_tree_view_dispose           (GObject           *object);

static void   gimp_item_tree_view_style_updated     (GtkWidget         *widget);

static void   gimp_item_tree_view_real_set_image    (GimpItemTreeView  *view,
                                                     GimpImage         *image);
static gboolean gimp_item_tree_view_key_press_event (GtkTreeView       *tree_view,
                                                     GdkEventKey       *event,
                                                     GimpItemTreeView *view);

static void   gimp_item_tree_view_image_flush       (GimpImage         *image,
                                                     gboolean           invalidate_preview,
                                                     GimpItemTreeView  *view);
static gboolean
              gimp_item_tree_view_image_flush_idle  (gpointer           user_data);

static void   gimp_item_tree_view_set_container     (GimpContainerView *view,
                                                     GimpContainer     *container);
static void   gimp_item_tree_view_set_context       (GimpContainerView *view,
                                                     GimpContext       *context);

static gpointer gimp_item_tree_view_insert_item     (GimpContainerView *view,
                                                     GimpViewable      *viewable,
                                                     gpointer           parent_insert_data,
                                                     gint               index);
static void  gimp_item_tree_view_insert_items_after (GimpContainerView *view);
static gboolean gimp_item_tree_view_select_items    (GimpContainerView *view,
                                                     GList             *items,
                                                     GList             *paths);
static void   gimp_item_tree_view_activate_item     (GimpContainerView *view,
                                                     GimpViewable      *item,
                                                     gpointer           insert_data);

static gboolean gimp_item_tree_view_drop_possible   (GimpContainerTreeView *view,
                                                     GimpDndType        src_type,
                                                     GList             *src_viewables,
                                                     GimpViewable      *dest_viewable,
                                                     GtkTreePath       *drop_path,
                                                     GtkTreeViewDropPosition  drop_pos,
                                                     GtkTreeViewDropPosition *return_drop_pos,
                                                     GdkDragAction     *return_drag_action);
static void     gimp_item_tree_view_drop_viewables  (GimpContainerTreeView   *view,
                                                     GList                   *src_viewables,
                                                     GimpViewable            *dest_viewable,
                                                     GtkTreeViewDropPosition  drop_pos);

static void   gimp_item_tree_view_new_dropped       (GtkWidget         *widget,
                                                     gint               x,
                                                     gint               y,
                                                     GimpViewable      *viewable,
                                                     gpointer           data);
static void gimp_item_tree_view_new_list_dropped    (GtkWidget         *widget,
                                                     gint               x,
                                                     gint               y,
                                                     GList             *viewables,
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
static void   gimp_item_tree_view_color_tag_changed    (GimpItem          *item,
                                                        GimpItemTreeView  *view);
static void   gimp_item_tree_view_lock_changed         (GimpItem          *item,
                                                        GimpItemTreeView  *view);
static void   gimp_item_tree_view_filters_changed      (GimpItem          *item,
                                                        GimpItemTreeView  *view);

static void   gimp_item_tree_view_eye_clicked       (GtkCellRendererToggle *toggle,
                                                     gchar             *path,
                                                     GdkModifierType    state,
                                                     GimpItemTreeView  *view);
static void   gimp_item_tree_view_lock_clicked      (GtkCellRendererToggle *toggle,
                                                     gchar             *path,
                                                     GdkModifierType    state,
                                                     GimpItemTreeView  *view);
static void   gimp_item_tree_view_effects_clicked   (GtkCellRendererToggle *toggle,
                                                     gchar             *path,
                                                     GdkModifierType    state,
                                                     GimpItemTreeView  *view);
static gboolean
              gimp_item_tree_view_effects_filters_selected
                                                    (GimpContainerView *view,
                                                     GList             *filters,
                                                     GList             *paths,
                                                     GimpItemTreeView  *item_view);
static void   gimp_item_tree_view_effects_activate_filter
                                                    (GtkWidget         *widget,
                                                     GimpViewable      *viewable,
                                                     gpointer           insert_data,
                                                     GimpItemTreeView  *view);
static void   gimp_item_tree_view_filter_active_changed
                                                    (GimpFilter        *item,
                                                     GimpContainerTreeView
                                                                       *view);

static void   gimp_item_tree_view_effects_visible_toggled
                                                    (GtkCellRendererToggle *toggle,
                                                     gchar                 *path_str,
                                                     GdkModifierType        state,
                                                     GimpContainerTreeView *view);

static void   gimp_item_tree_view_effects_visible_all_toggled
                                                    (GtkWidget         *widget,
                                                     GimpItemTreeView  *view);
static void   gimp_item_tree_view_effects_edited_clicked
                                                    (GtkWidget         *widget,
                                                     GimpItemTreeView  *view);
static void   gimp_item_tree_view_effects_raised_clicked
                                                    (GtkWidget         *widget,
                                                     GimpItemTreeView  *view);
static void   gimp_item_tree_view_effects_lowered_clicked
                                                    (GtkWidget         *widget,
                                                     GimpItemTreeView  *view);
static void   gimp_item_tree_view_effects_merged_clicked
                                                    (GtkWidget         *widget,
                                                     GimpItemTreeView  *view);
static void   gimp_item_tree_view_effects_removed_clicked
                                                    (GtkWidget         *widget,
                                                     GimpItemTreeView  *view);
static gboolean gimp_item_tree_view_lock_button_release (GtkWidget        *widget,
                                                         GdkEvent         *event,
                                                         GimpItemTreeView *view);
static void   gimp_item_tree_view_lock_toggled      (GtkWidget         *widget,
                                                     GimpItemTreeView  *view);
static void   gimp_item_tree_view_update_lock_box   (GimpItemTreeView  *view,
                                                     GimpItem          *item,
                                                     GtkTreePath       *path);
static gboolean gimp_item_tree_view_popover_button_press (GtkWidget        *widget,
                                                          GdkEvent         *event,
                                                          GimpItemTreeView *view);

static gboolean gimp_item_tree_view_item_pre_clicked(GimpCellRendererViewable *cell,
                                                     const gchar              *path_str,
                                                     GdkModifierType           state,
                                                     GimpItemTreeView         *item_view);

static void   gimp_item_tree_view_row_expanded      (GtkTreeView       *tree_view,
                                                     GtkTreeIter       *iter,
                                                     GtkTreePath       *path,
                                                     GimpItemTreeView  *item_view);

static gint   gimp_item_tree_view_get_n_locks       (GimpItemTreeView *view,
                                                     GimpItem         *item,
                                                     const gchar     **icon_name);

static void  gimp_item_tree_effects_set_sensitive   (GimpItemTreeView *view,
                                                     gboolean          is_sensitive);

/* Functions for the item search/selection feature. */

static void     gimp_item_tree_view_create_search_popover      (GimpItemTreeView *view,
                                                                GtkIconSize       button_size);
static void     gimp_item_tree_view_search_popover_shown       (GtkPopover       *popover,
                                                                GimpItemTreeView *view);

static void     gimp_item_tree_view_floating_selection_changed (GimpImage        *image,
                                                                GimpItemTreeView *view);
static void     gimp_item_tree_view_item_links_changed         (GimpImage        *image,
                                                                GType             item_type,
                                                                GimpItemTreeView *view);
static gboolean gimp_item_tree_view_link_clicked               (GtkWidget        *box,
                                                                GdkEvent         *event,
                                                                GimpItemTreeView *view);
static gboolean gimp_item_tree_view_unlink_clicked             (GtkWidget        *widget,
                                                                GdkEvent         *event,
                                                                GimpItemTreeView *view);
static gboolean gimp_item_tree_view_search_key_release         (GtkWidget        *widget,
                                                                GdkEventKey      *event,
                                                                GimpItemTreeView *view);
static gboolean gimp_item_tree_view_start_interactive_search   (GtkTreeView      *tree_view,
                                                                GimpItemTreeView *layer_view);
static gboolean gimp_item_tree_view_new_link_clicked           (GimpItemTreeView *view);
static void     gimp_item_tree_view_new_link_exit              (GimpItemTreeView *view);
static gboolean gimp_item_tree_view_search_clicked             (GtkWidget        *main_column_button,
                                                                GdkEventButton   *event,
                                                                GimpItemTreeView *view);

static gboolean gimp_item_tree_view_move_cursor                (GimpItemTreeView *tree_view,
                                                                guint             keyval,
                                                                GdkModifierType   modifiers);


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
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_OBJECT);

  object_class->constructed      = gimp_item_tree_view_constructed;
  object_class->dispose          = gimp_item_tree_view_dispose;

  widget_class->style_updated    = gimp_item_tree_view_style_updated;

  tree_view_class->drop_possible  = gimp_item_tree_view_drop_possible;
  tree_view_class->drop_viewables = gimp_item_tree_view_drop_viewables;

  klass->set_image               = gimp_item_tree_view_real_set_image;

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

  klass->move_cursor_up_action        = NULL;
  klass->move_cursor_down_action      = NULL;
  klass->move_cursor_up_flat_action   = NULL;
  klass->move_cursor_down_flat_action = NULL;
  klass->move_cursor_start_action     = NULL;
  klass->move_cursor_end_action       = NULL;
}

static void
gimp_item_tree_view_view_iface_init (GimpContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->set_container      = gimp_item_tree_view_set_container;
  iface->set_context        = gimp_item_tree_view_set_context;
  iface->insert_item        = gimp_item_tree_view_insert_item;
  iface->insert_items_after = gimp_item_tree_view_insert_items_after;
  iface->select_items       = gimp_item_tree_view_select_items;
  iface->activate_item      = gimp_item_tree_view_activate_item;
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

  view->priv->model_column_locked =
    gimp_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);

  view->priv->model_column_lock_icon =
    gimp_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_STRING);

  view->priv->model_column_color_tag =
    gimp_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           GDK_TYPE_RGBA);

  view->priv->model_column_effects =
    gimp_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);

  gimp_container_tree_view_set_dnd_drop_to_empty (tree_view, TRUE);

  view->priv->image            = NULL;
  view->priv->effects_drawable = NULL;
  view->priv->effects_filter   = NULL;

  view->priv->link_pattern_set = NULL;
}

static void
gimp_item_tree_view_constructed (GObject *object)
{
  GimpItemTreeViewClass *item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (object);
  GimpEditor            *editor          = GIMP_EDITOR (object);
  GimpContainerTreeView *tree_view       = GIMP_CONTAINER_TREE_VIEW (object);
  GimpItemTreeView      *item_view       = GIMP_ITEM_TREE_VIEW (object);
  GtkTreeViewColumn     *column;
  GtkWidget             *image;
  GtkWidget             *label;
  GtkWidget             *items_header;
  gchar                 *text;
  GtkIconSize            button_icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
  gint                   pixel_icon_size  = 16;
  gint                   button_spacing;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_signal_connect (tree_view->view, "key-press-event",
                    G_CALLBACK (gimp_item_tree_view_key_press_event),
                    tree_view);

  gtk_tree_view_set_headers_visible (tree_view->view, TRUE);

  gtk_widget_style_get (GTK_WIDGET (item_view),
                        "button-icon-size", &button_icon_size,
                        "button-spacing", &button_spacing,
                        NULL);
  gtk_icon_size_lookup (button_icon_size, &pixel_icon_size, NULL);

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
  image = gtk_image_new_from_icon_name (GIMP_ICON_VISIBLE, button_icon_size);
  gtk_tree_view_column_set_widget (column, image);
  gtk_widget_show (image);
  gtk_tree_view_insert_column (tree_view->view, column, 0);
  item_view->priv->eye_header_image = image;

  item_view->priv->eye_cell = gimp_cell_renderer_toggle_new (GIMP_ICON_VISIBLE);
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

  gimp_container_tree_view_add_toggle_cell (tree_view,
                                            item_view->priv->eye_cell);

  g_signal_connect (item_view->priv->eye_cell, "clicked",
                    G_CALLBACK (gimp_item_tree_view_eye_clicked),
                    item_view);

  column = gtk_tree_view_column_new ();
  image = gtk_image_new_from_icon_name (GIMP_ICON_LOCK, button_icon_size);
  gtk_tree_view_column_set_widget (column, image);
  gtk_widget_show (image);
  gtk_tree_view_insert_column (tree_view->view, column, 1);
  item_view->priv->lock_header_image = image;

  item_view->priv->lock_cell = gimp_cell_renderer_toggle_new (GIMP_ICON_LOCK_MULTI);
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

  gimp_container_tree_view_add_toggle_cell (tree_view,
                                            item_view->priv->lock_cell);

  g_signal_connect (item_view->priv->lock_cell, "clicked",
                    G_CALLBACK (gimp_item_tree_view_lock_clicked),
                    item_view);

  /* TODO: Expand layer effects to other drawable types */
  if (GIMP_IS_LAYER_TREE_VIEW (object))
    {
      column = gtk_tree_view_column_new ();
      image = gtk_image_new_from_icon_name (GIMP_ICON_EFFECT, button_icon_size);
      gimp_widget_set_identifier (image, "item-effect-header-icon");
      gtk_tree_view_column_set_widget (column, image);
      gtk_widget_set_visible (image, TRUE);
      gtk_tree_view_insert_column (tree_view->view, column, 2);
      item_view->priv->effect_header_image = image;

      item_view->priv->effects_cell = gimp_cell_renderer_toggle_new (GIMP_ICON_EFFECT);
      g_object_set (item_view->priv->effects_cell,
                    "xpad",      0,
                    "ypad",      0,
                    "icon-size", pixel_icon_size,
                    NULL);
      gtk_tree_view_column_pack_start (column, item_view->priv->effects_cell, FALSE);
      gtk_tree_view_column_set_attributes (column, item_view->priv->effects_cell,
                                           "active",
                                           item_view->priv->model_column_effects,
                                           NULL);

      gimp_container_tree_view_add_toggle_cell (tree_view,
                                                item_view->priv->effects_cell);

      g_signal_connect (item_view->priv->effects_cell, "clicked",
                        G_CALLBACK (gimp_item_tree_view_effects_clicked),
                        item_view);
    }

  /*  disable the default GimpContainerView drop handler  */
  gimp_container_view_set_dnd_widget (GIMP_CONTAINER_VIEW (item_view), NULL);

  gimp_dnd_drag_dest_set_by_type (GTK_WIDGET (tree_view->view),
                                  GTK_DEST_DEFAULT_HIGHLIGHT,
                                  item_view_class->item_type,
                                  TRUE,
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
  gimp_dnd_viewable_list_dest_add (item_view->priv->new_button,
                                   item_view_class->item_type,
                                   gimp_item_tree_view_new_list_dropped,
                                   item_view);
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

  /* Lock box. */
  item_view->priv->lock_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, button_spacing);

  /*  Lock box: content toggle  */
  gimp_item_tree_view_add_lock (item_view,
                                item_view_class->lock_content_icon_name,
                                (GimpIsLockedFunc) gimp_item_get_lock_content,
                                (GimpCanLockFunc)  gimp_item_can_lock_content,
                                (GimpSetLockFunc)  gimp_item_set_lock_content,
                                (GimpUndoLockPush) gimp_image_undo_push_item_lock_content,
                                "lock-content-changed",
                                GIMP_UNDO_ITEM_LOCK_CONTENT,
                                GIMP_UNDO_GROUP_ITEM_LOCK_CONTENTS,
                                _("Lock content"),
                                _("Unlock content"),
                                _("Set Item Exclusive Content Lock"),
                                item_view_class->lock_content_tooltip,
                                item_view_class->lock_content_help_id);

  /*  Lock box: position toggle  */
  gimp_item_tree_view_add_lock (item_view,
                                item_view_class->lock_position_icon_name,
                                (GimpIsLockedFunc) gimp_item_get_lock_position,
                                (GimpCanLockFunc)  gimp_item_can_lock_position,
                                (GimpSetLockFunc)  gimp_item_set_lock_position,
                                (GimpUndoLockPush) gimp_image_undo_push_item_lock_position,
                                "lock-position-changed",
                                GIMP_UNDO_ITEM_LOCK_POSITION,
                                GIMP_UNDO_GROUP_ITEM_LOCK_POSITION,
                                _("Lock position"),
                                _("Unlock position"),
                                _("Set Item Exclusive Position Lock"),
                                item_view_class->lock_position_tooltip,
                                item_view_class->lock_position_help_id);

  /*  Lock box: visibility toggle  */
  gimp_item_tree_view_add_lock (item_view,
                                item_view_class->lock_visibility_icon_name,
                                (GimpIsLockedFunc) gimp_item_get_lock_visibility,
                                (GimpCanLockFunc)  gimp_item_can_lock_visibility,
                                (GimpSetLockFunc)  gimp_item_set_lock_visibility,
                                (GimpUndoLockPush) gimp_image_undo_push_item_lock_visibility,
                                "lock-visibility-changed",
                                GIMP_UNDO_ITEM_LOCK_VISIBILITY,
                                GIMP_UNDO_GROUP_ITEM_LOCK_VISIBILITY,
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
                    G_CALLBACK (gimp_item_tree_view_popover_button_press),
                    item_view);
  gtk_container_add (GTK_CONTAINER (item_view->priv->lock_popover), item_view->priv->lock_box);
  gtk_widget_show (item_view->priv->lock_box);

  /* Effects box. */
  if (GIMP_IS_LAYER_TREE_VIEW (object))
    {
      item_view->priv->effects_filters = gtk_box_new (GTK_ORIENTATION_VERTICAL, button_spacing);
      item_view->priv->effects_box     = gtk_box_new (GTK_ORIENTATION_VERTICAL, button_spacing);
      item_view->priv->effects_options = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, button_spacing);

      /* Effects Buttons */
      item_view->priv->effects_visible_button = gtk_toggle_button_new ();
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item_view->priv->effects_visible_button),
                                    TRUE);
      image = gtk_image_new_from_icon_name (GIMP_ICON_VISIBLE,
                                            GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_container_add (GTK_CONTAINER (item_view->priv->effects_visible_button), image);
      gimp_help_set_help_data (item_view->priv->effects_visible_button,
                               _("Toggle the visibility of all filters."),
                               GIMP_HELP_LAYER_EFFECTS);
      g_signal_connect (item_view->priv->effects_visible_button, "toggled",
                        G_CALLBACK (gimp_item_tree_view_effects_visible_all_toggled),
                        item_view);
      gtk_box_pack_start (GTK_BOX (item_view->priv->effects_options),
                          item_view->priv->effects_visible_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (image, TRUE);
      gtk_widget_set_visible (item_view->priv->effects_visible_button, TRUE);

      item_view->priv->effects_edit_button =
        gtk_button_new_from_icon_name (GIMP_ICON_EDIT,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gimp_help_set_help_data (item_view->priv->effects_edit_button,
                               _("Edit the selected filter."),
                               GIMP_HELP_LAYER_EFFECTS);
      g_signal_connect (item_view->priv->effects_edit_button, "clicked",
                        G_CALLBACK (gimp_item_tree_view_effects_edited_clicked),
                        item_view);
      gtk_box_pack_start (GTK_BOX (item_view->priv->effects_options),
                          item_view->priv->effects_edit_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (item_view->priv->effects_edit_button, TRUE);

      item_view->priv->effects_raise_button =
        gtk_button_new_from_icon_name (GIMP_ICON_GO_UP,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gimp_help_set_help_data (item_view->priv->effects_raise_button,
                               _("Raise filter one step up in the stack."),
                               GIMP_HELP_LAYER_EFFECTS);
      g_signal_connect (item_view->priv->effects_raise_button, "clicked",
                        G_CALLBACK (gimp_item_tree_view_effects_raised_clicked),
                        item_view);
      gtk_box_pack_start (GTK_BOX (item_view->priv->effects_options),
                          item_view->priv->effects_raise_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (item_view->priv->effects_raise_button, TRUE);

      item_view->priv->effects_lower_button =
        gtk_button_new_from_icon_name (GIMP_ICON_GO_DOWN,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gimp_help_set_help_data (item_view->priv->effects_lower_button,
                               _("Lower filter one step down in the stack."),
                               GIMP_HELP_LAYER_EFFECTS);
      g_signal_connect (item_view->priv->effects_lower_button, "clicked",
                        G_CALLBACK (gimp_item_tree_view_effects_lowered_clicked),
                        item_view);
      gtk_box_pack_start (GTK_BOX (item_view->priv->effects_options),
                          item_view->priv->effects_lower_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (item_view->priv->effects_lower_button, TRUE);

      item_view->priv->effects_merge_button =
        gtk_button_new_from_icon_name (GIMP_ICON_LAYER_MERGE_DOWN,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gimp_help_set_help_data (item_view->priv->effects_merge_button,
                               _("Merge all active filters down."),
                               GIMP_HELP_LAYER_EFFECTS);
      g_signal_connect (item_view->priv->effects_merge_button, "clicked",
                        G_CALLBACK (gimp_item_tree_view_effects_merged_clicked),
                        item_view);
      gtk_box_pack_start (GTK_BOX (item_view->priv->effects_options),
                          item_view->priv->effects_merge_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (item_view->priv->effects_merge_button, TRUE);

      item_view->priv->effects_remove_button =
        gtk_button_new_from_icon_name (GIMP_ICON_EDIT_DELETE,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gimp_help_set_help_data (item_view->priv->effects_remove_button,
                               _("Remove the selected filter."),
                               GIMP_HELP_LAYER_EFFECTS);
      g_signal_connect (item_view->priv->effects_remove_button, "clicked",
                        G_CALLBACK (gimp_item_tree_view_effects_removed_clicked),
                        item_view);
      gtk_box_pack_start (GTK_BOX (item_view->priv->effects_options),
                          item_view->priv->effects_remove_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (item_view->priv->effects_remove_button, TRUE);

      label = gtk_label_new (NULL);
      text = g_strdup_printf ("<b>%s</b>",
                              _("Layer Effects"));
      gtk_label_set_markup (GTK_LABEL (label), text);
      gtk_widget_set_visible (label, TRUE);
      g_free (text);

      /* Effects popover. */
      item_view->priv->effects_popover = gtk_popover_new (GTK_WIDGET (tree_view->view));
      gtk_popover_set_modal (GTK_POPOVER (item_view->priv->effects_popover), TRUE);
      gtk_container_add (GTK_CONTAINER (item_view->priv->effects_popover),
                         item_view->priv->effects_filters);
      gimp_help_connect (item_view->priv->effects_filters, NULL,
                         gimp_standard_help_func, GIMP_HELP_LAYER_EFFECTS, NULL, NULL);
      gtk_box_pack_start (GTK_BOX (item_view->priv->effects_filters), label,
                          FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (item_view->priv->effects_filters),
                          item_view->priv->effects_box, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (item_view->priv->effects_filters),
                          item_view->priv->effects_options, FALSE, FALSE, 0);
      gtk_widget_set_visible (item_view->priv->effects_box, TRUE);
      gtk_widget_set_visible (item_view->priv->effects_options, TRUE);
      gtk_widget_set_visible (item_view->priv->effects_filters, TRUE);
    }

  items_header = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_widget_set_hexpand (items_header, TRUE);
  gtk_widget_set_halign (items_header, GTK_ALIGN_FILL);
  gtk_widget_set_margin_end (items_header, 1);

  /* Link popover menu. */

  item_view->priv->search_button = gtk_image_new_from_icon_name (GIMP_ICON_EDIT_FIND,
                                                                 button_icon_size);
  gtk_widget_set_tooltip_text (item_view->priv->search_button,
                               _("Select items by patterns and store item sets"));
  gtk_box_pack_end (GTK_BOX (items_header), item_view->priv->search_button,
                    FALSE, FALSE, 2);
  gimp_item_tree_view_create_search_popover (item_view, button_icon_size);
  /* TODO: Currently the search feature only works on layers.
   * In the future, we should expand this to channels and paths
   * dockables as well */
  if (GIMP_IS_LAYER_TREE_VIEW (object))
    gtk_widget_show (item_view->priv->search_button);

  /* Search label */

  item_view->priv->multi_selection_label = gtk_label_new (NULL);
  gtk_label_set_selectable (GTK_LABEL (item_view->priv->multi_selection_label), FALSE);
  gtk_widget_set_halign (item_view->priv->multi_selection_label, GTK_ALIGN_START);
  gtk_box_pack_end (GTK_BOX (items_header), item_view->priv->multi_selection_label,
                    FALSE, FALSE, 4);
  gtk_widget_show (item_view->priv->multi_selection_label);

  gtk_tree_view_column_set_widget (GIMP_CONTAINER_TREE_VIEW (item_view)->main_column, items_header);
  gtk_tree_view_column_set_alignment (GIMP_CONTAINER_TREE_VIEW (item_view)->main_column, 1.0);
  gtk_tree_view_column_set_clickable (GIMP_CONTAINER_TREE_VIEW (item_view)->main_column, TRUE);
  g_signal_connect (gtk_tree_view_column_get_button (GIMP_CONTAINER_TREE_VIEW (item_view)->main_column),
                    "button-release-event",
                    G_CALLBACK (gtk_true),
                    item_view);
  /* XXX: I don't use the "clicked" signal of the GtkTreeViewColumn main_column,
   * because the event goes through and reaches gimp_container_tree_view_button()
   * code, provoking selection of the top item, depending on where we click.
   * Catching the signal at the column's button level, we manage to block the
   * event from having any side effect other than opening our popup.
   */
  g_signal_connect (gtk_tree_view_column_get_button (GIMP_CONTAINER_TREE_VIEW (item_view)->main_column),
                    "button-press-event",
                    G_CALLBACK (gimp_item_tree_view_search_clicked),
                    item_view);
  gtk_widget_show (items_header);
}

static void
gimp_item_tree_view_dispose (GObject *object)
{
  GimpItemTreeView *view = GIMP_ITEM_TREE_VIEW (object);

  if (view->priv->image)
    gimp_item_tree_view_set_image (view, NULL);

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

  if (view->priv->effects_popover)
    {
      gtk_widget_destroy (view->priv->effects_popover);
      view->priv->effects_popover = NULL;
    }
  view->priv->effects_drawable = NULL;
  view->priv->effects_filter   = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_item_tree_view_style_updated (GtkWidget *widget)
{
  GimpItemTreeView *view = GIMP_ITEM_TREE_VIEW (widget);
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

  gtk_image_get_icon_name (GTK_IMAGE (view->priv->search_button),
                           &old_icon_name, &old_size);
  if (button_icon_size != old_size)
    {
      GtkWidget *image;

      icon_name = g_strdup (old_icon_name);
      gtk_image_set_from_icon_name (GTK_IMAGE (view->priv->search_button),
                                    icon_name, button_icon_size);
      g_free (icon_name);

      /* Changing the new link button inside the popover. */
      image = gtk_button_get_image (GTK_BUTTON (view->priv->new_link_button));
      gtk_image_get_icon_name (GTK_IMAGE (image), &old_icon_name, &old_size);
      icon_name = g_strdup (old_icon_name);
      gtk_image_set_from_icon_name (GTK_IMAGE (image), icon_name,
                                    button_icon_size);
      g_free (icon_name);
    }

  if (GIMP_IS_LAYER_TREE_VIEW (view))
    {
      gtk_image_get_icon_name (GTK_IMAGE (view->priv->effect_header_image),
                               &old_icon_name, &old_size);
      if (button_icon_size != old_size)
        {
          icon_name = g_strdup (old_icon_name);
          gtk_image_set_from_icon_name (GTK_IMAGE (view->priv->effect_header_image),
                                        icon_name, button_icon_size);
          g_free (icon_name);
        }
    }

  /* force the eye and toggle cells to recreate their icon */
  gtk_icon_size_lookup (button_icon_size, &pixel_icon_size, NULL);
  g_object_set (view->priv->eye_cell,
                "icon-name", GIMP_ICON_VISIBLE,
                "icon-size", pixel_icon_size,
                NULL);
  g_object_set (view->priv->lock_cell,
                "icon-name", GIMP_ICON_LOCK_MULTI,
                "icon-size", pixel_icon_size,
                NULL);

  if (GIMP_IS_LAYER_TREE_VIEW (view))
    g_object_set (view->priv->effects_cell,
                  "icon-name", GIMP_ICON_EFFECT,
                  "icon-size", pixel_icon_size,
                  NULL);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);
}

GtkWidget *
gimp_item_tree_view_new (GType            view_type,
                         gint             view_size,
                         gint             view_border_width,
                         gboolean         multiple_selection,
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
                            "selection-mode",  multiple_selection ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE,
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
gimp_item_tree_view_add_lock (GimpItemTreeView *view,
                              const gchar      *icon_name,
                              GimpIsLockedFunc  is_locked,
                              GimpCanLockFunc   can_lock,
                              GimpSetLockFunc   lock,
                              GimpUndoLockPush  undo_push,
                              const gchar      *signal_name,
                              GimpUndoType      undo_type,
                              GimpUndoType      group_undo_type,
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
                    G_CALLBACK (gimp_item_tree_view_lock_toggled),
                    view);
  g_signal_connect (toggle, "button-release-event",
                    G_CALLBACK (gimp_item_tree_view_lock_button_release),
                    view);

  gimp_help_connect (toggle, tooltip, gimp_standard_help_func, help_id, NULL, NULL);

  gtk_widget_style_get (GTK_WIDGET (view),
                        "button-icon-size", &icon_size,
                        NULL);

  image = gtk_image_new_from_icon_name (icon_name, icon_size);
  gtk_container_add (GTK_CONTAINER (toggle), image);
  gtk_widget_show (image);
}

void
gimp_item_tree_view_blink_lock (GimpItemTreeView *view,
                                GimpItem         *item)
{
  GtkTreeIter  *iter;
  GtkTreePath  *path;
  GdkRectangle  rect;

  g_return_if_fail (GIMP_IS_ITEM_TREE_VIEW (view));
  g_return_if_fail (GIMP_IS_ITEM (item));

  /* Find the item in the tree view. */
  iter = gimp_container_view_lookup (GIMP_CONTAINER_VIEW (view),
                                     (GimpViewable *) item);
  path = gtk_tree_model_get_path (GIMP_CONTAINER_TREE_VIEW (view)->model, iter);

  /* Scroll dockable to make sure the cell is showing. */
  gtk_tree_view_scroll_to_cell (GIMP_CONTAINER_TREE_VIEW (view)->view, path,
                                gtk_tree_view_get_column (GIMP_CONTAINER_TREE_VIEW (view)->view, 1),
                                FALSE, 0.0, 0.0);

  /* Now blink the lock cell of the specified item. */
  gtk_tree_view_get_cell_area (GIMP_CONTAINER_TREE_VIEW (view)->view, path,
                               gtk_tree_view_get_column (GIMP_CONTAINER_TREE_VIEW (view)->view, 1),
                               &rect);
  gtk_tree_view_convert_bin_window_to_widget_coords (GIMP_CONTAINER_TREE_VIEW (view)->view,
                                                     rect.x, rect.y, &rect.x, &rect.y);
  gimp_widget_blink_rect (GTK_WIDGET (GIMP_CONTAINER_TREE_VIEW (view)->view), &rect);

  gtk_tree_path_free (path);
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
  GimpItemTreeViewClass *item_view_class;

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

      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            G_CALLBACK (gimp_item_tree_view_item_links_changed),
                                            view);
      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            G_CALLBACK (gimp_item_tree_view_floating_selection_changed),
                                            view);
    }

  view->priv->image                          = image;
  view->priv->effects_drawable               = NULL;
  view->priv->effects_filter                 = NULL;
  view->priv->filters_active_changed_handler = NULL;
  if (view->priv->effects_popover)
    gtk_widget_set_visible (view->priv->effects_popover, FALSE);

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

      g_signal_connect (view->priv->image,
                        "item-sets-changed",
                        G_CALLBACK (gimp_item_tree_view_item_links_changed),
                        view);
      g_signal_connect (view->priv->image,
                        "floating-selection-changed",
                        G_CALLBACK (gimp_item_tree_view_floating_selection_changed),
                        view);

      gimp_item_tree_view_item_changed (view->priv->image, view);
    }

  /* Call this even with no image, allowing to empty the link list. */
  item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (view);
  gimp_item_tree_view_item_links_changed (view->priv->image,
                                          item_view_class->item_type,
                                          view);
  gimp_item_tree_view_floating_selection_changed (view->priv->image, view);
}

static gboolean
gimp_item_tree_view_key_press_event (GtkTreeView      *tree_view,
                                     GdkEventKey      *event,
                                     GimpItemTreeView *view)
{
  if (event->keyval == GDK_KEY_Up           ||
      event->keyval == GDK_KEY_KP_Up        ||
      event->keyval == GDK_KEY_Down         ||
      event->keyval == GDK_KEY_KP_Down      ||
      event->keyval == GDK_KEY_Page_Up      ||
      event->keyval == GDK_KEY_KP_Page_Up   ||
      event->keyval == GDK_KEY_Down         ||
      event->keyval == GDK_KEY_KP_Down      ||
      event->keyval == GDK_KEY_Page_Down    ||
      event->keyval == GDK_KEY_KP_Page_Down ||
      event->keyval == GDK_KEY_Home         ||
      event->keyval == GDK_KEY_KP_Home      ||
      event->keyval == GDK_KEY_End          ||
      event->keyval == GDK_KEY_KP_End)
    {
      if (event->state & gimp_get_all_modifiers_mask ())
        /* Let the event propagate, though to be fair, it would be nice
         * to specify what happens with a Shift-arrow, or Ctrl-arrow,
         * with a multi-item aware logic. It may also require a concept
         * of "active" item among the selected one (very likely the last
         * clicked item).
         * TODO for a proper gimp-ux spec.
         */
        return FALSE;

      gimp_item_tree_view_move_cursor (view, event->keyval, event->state);

      return TRUE;
    }

  return FALSE;
}

static void
gimp_item_tree_view_image_flush (GimpImage        *image,
                                 gboolean          invalidate_preview,
                                 GimpItemTreeView *view)
{
  g_idle_add_full (G_PRIORITY_LOW,
                   (GSourceFunc) gimp_item_tree_view_image_flush_idle,
                   g_object_ref (view), g_object_unref);
}

static gboolean
gimp_item_tree_view_image_flush_idle (gpointer user_data)
{
  GimpItemTreeView *view = user_data;

  /* This needs to be run as idle because we want this to be run in the main
   * thread even when the flush happened in a thread (e.g. from the paint
   * thread).
   */
  if (gimp_editor_get_ui_manager (GIMP_EDITOR (view)))
    gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (view)), view);

  return G_SOURCE_REMOVE;
}


/*  GimpContainerView methods  */

static void
gimp_item_tree_view_set_container (GimpContainerView *view,
                                   GimpContainer     *container)
{
  GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (view);
  GimpContainer    *old_container;
  GList            *list;

  old_container = gimp_container_view_get_container (view);

  if (old_container)
    {
      gimp_tree_handler_disconnect (item_view->priv->visible_changed_handler);
      item_view->priv->visible_changed_handler = NULL;

      gimp_tree_handler_disconnect (item_view->priv->color_tag_changed_handler);
      item_view->priv->color_tag_changed_handler = NULL;

      if (GIMP_IS_LAYER_TREE_VIEW (item_view))
        {
          gimp_tree_handler_disconnect (item_view->priv->filters_changed_handler);
          item_view->priv->filters_changed_handler = NULL;

          if (item_view->priv->filters_active_changed_handler)
            gimp_tree_handler_disconnect (item_view->priv->filters_active_changed_handler);

          item_view->priv->filters_active_changed_handler = NULL;
        }

      for (list = item_view->priv->locks; list; list = list->next)
        {
          LockToggle *data = list->data;

          gimp_tree_handler_disconnect (data->changed_handler);
          data->changed_handler = NULL;
        }
    }

  parent_view_iface->set_container (view, container);

  if (container)
    {
      item_view->priv->visible_changed_handler =
        gimp_tree_handler_connect (container, "visibility-changed",
                                   G_CALLBACK (gimp_item_tree_view_visible_changed),
                                   view);

      item_view->priv->color_tag_changed_handler =
        gimp_tree_handler_connect (container, "color-tag-changed",
                                   G_CALLBACK (gimp_item_tree_view_color_tag_changed),
                                   view);

      for (list = item_view->priv->locks; list; list = list->next)
        {
          LockToggle *data = list->data;

          data->changed_handler = gimp_tree_handler_connect (container,
                                                             data->signal_name,
                                                             G_CALLBACK (gimp_item_tree_view_lock_changed),
                                                             view);
        }

      if (GIMP_IS_LAYER_TREE_VIEW (item_view))
        item_view->priv->filters_changed_handler =
          gimp_tree_handler_connect (container, "filters-changed",
                                     G_CALLBACK (gimp_item_tree_view_filters_changed),
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
      g_signal_handlers_disconnect_by_func (old_context->gimp->config,
                                            G_CALLBACK (gimp_item_tree_view_style_updated),
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

      g_signal_connect_object (context->gimp->config,
                               "notify::theme",
                               G_CALLBACK (gimp_item_tree_view_style_updated),
                               item_view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (context->gimp->config,
                               "notify::override-theme-icon-size",
                               G_CALLBACK (gimp_item_tree_view_style_updated),
                               item_view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (context->gimp->config,
                               "notify::custom-icon-size",
                               G_CALLBACK (gimp_item_tree_view_style_updated),
                               item_view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

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
  GeglColor             *color     = gegl_color_new ("none");
  GdkRGBA                rgba;
  gboolean               has_color;
  const gchar           *icon_name;
  gint                   n_locks;

  iter = parent_view_iface->insert_item (view, viewable,
                                         parent_insert_data, index);

  has_color = gimp_get_color_tag_color (gimp_item_get_merged_color_tag (item),
                                        color,
                                        gimp_item_get_color_tag (item) ==
                                        GIMP_COLOR_TAG_NONE);
  gegl_color_get_pixel (color, babl_format ("R'G'B'A double"), &rgba);
  g_object_unref (color);

  n_locks = gimp_item_tree_view_get_n_locks (item_view, item, &icon_name);

  gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,

                      item_view->priv->model_column_visible,
                      gimp_item_get_visible (item),

                      item_view->priv->model_column_viewable,
                      gimp_item_get_visible (item) &&
                      ! gimp_item_is_visible (item),

                      item_view->priv->model_column_locked,
                      n_locks > 0,

                      item_view->priv->model_column_lock_icon,
                      icon_name,

                      item_view->priv->model_column_color_tag,
                      has_color ? &rgba : NULL,
                      -1);

  if (GIMP_IS_LAYER_TREE_VIEW (item_view))
    {
      GimpContainer *filters;
      gint           n_filters;

      filters   = gimp_drawable_get_filters (GIMP_DRAWABLE (item));
      n_filters = gimp_container_get_n_children (filters);

      gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                          item_view->priv->model_column_effects,
                          n_filters > 0,
                          -1);
    }

  return iter;
}

static void
gimp_item_tree_view_insert_items_after (GimpContainerView *view)
{
  GimpItemTreeView      *item_view = GIMP_ITEM_TREE_VIEW (view);
  GimpItemTreeViewClass *item_view_class;
  GList                 *selected_items;

  item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (item_view);

  selected_items = item_view_class->get_selected_items (item_view->priv->image);
  gimp_container_view_select_items (view, selected_items);
}

static gboolean
gimp_item_tree_view_select_items (GimpContainerView *view,
                                  GList             *items,
                                  GList             *paths)
{
  GimpItemTreeView *tree_view         = GIMP_ITEM_TREE_VIEW (view);
  gboolean          options_sensitive = FALSE;
  gboolean          success;

  success = parent_view_iface->select_items (view, items, paths);

  if (items)
    {
      GimpItemTreeViewClass *item_view_class;

      item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (tree_view);
      if (TRUE) /* XXX: test if new selection same as old. */
        {
          item_view_class->set_selected_items (tree_view->priv->image, items);

          items = item_view_class->get_selected_items (tree_view->priv->image);
        }

      options_sensitive = TRUE;
    }

  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (tree_view)), tree_view);

  if (tree_view->priv->options_box)
    gtk_widget_set_sensitive (tree_view->priv->options_box, options_sensitive);

  if (g_list_length (items) > 1)
    {
      gchar *str;

      str = g_strdup_printf (ngettext ("%d item selected", "%d items selected",
                                       g_list_length (items)),
                             g_list_length (items));
      gtk_label_set_text (GTK_LABEL (tree_view->priv->multi_selection_label), str);
      g_free (str);
      gtk_widget_show (tree_view->priv->multi_selection_label);
    }
  else
    {
      gtk_widget_hide (tree_view->priv->multi_selection_label);
    }

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

static gboolean
gimp_item_tree_view_drop_possible (GimpContainerTreeView   *tree_view,
                                   GimpDndType              src_type,
                                   GList                   *src_viewables,
                                   GimpViewable            *dest_viewable,
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
      GimpViewable *src_viewable = iter->data;

      if (! GIMP_IS_ITEM (src_viewable) ||
          (dest_viewable != NULL &&
           gimp_item_get_image (GIMP_ITEM (src_viewable)) ==
           gimp_item_get_image (GIMP_ITEM (dest_viewable))))
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
gimp_item_tree_view_drop_viewables (GimpContainerTreeView   *tree_view,
                                    GList                   *src_viewables,
                                    GimpViewable            *dest_viewable,
                                    GtkTreeViewDropPosition  drop_pos)
{
  GimpItemTreeViewClass *item_view_class;
  GimpItemTreeView      *item_view              = GIMP_ITEM_TREE_VIEW (tree_view);
  GList                 *iter;
  GimpImage             *src_image              = NULL;
  GType                  src_viewable_type      = G_TYPE_NONE;
  gint                   dest_index             = -1;
  gboolean               src_viewables_reversed = FALSE;

  g_return_if_fail (g_list_length (src_viewables) > 0);

  item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (item_view);

  for (iter = src_viewables; iter; iter = iter->next)
    {
      GimpViewable *src_viewable = iter->data;

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
             * parenting hierarchy, for instance GimpLayer and
             * GimpGroupLayer.
             */
            src_viewable_type = G_TYPE_FROM_INSTANCE (src_viewable);

          g_return_if_fail (g_type_is_a (G_TYPE_FROM_INSTANCE (src_viewable),
                                         src_viewable_type));
        }

      if (src_image == NULL)
        src_image = gimp_item_get_image (GIMP_ITEM (iter->data));
      else
        g_return_if_fail (src_image == gimp_item_get_image (GIMP_ITEM (iter->data)));
    }

  if (drop_pos == GTK_TREE_VIEW_DROP_AFTER ||
      (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER &&
       dest_viewable                                &&
       gimp_viewable_get_children (dest_viewable)))
    {
      src_viewables_reversed = TRUE;
      src_viewables = g_list_reverse (src_viewables);
    }

  if (item_view->priv->image != src_image ||
      ! g_type_is_a (src_viewable_type, item_view_class->item_type))
    {
      GType item_type = item_view_class->item_type;

      gimp_image_undo_group_start (item_view->priv->image,
                                   GIMP_UNDO_GROUP_LAYER_ADD,
                                   _("Drop layers"));

      for (iter = src_viewables; iter; iter = iter->next)
        {
          GimpViewable *src_viewable = iter->data;
          GimpItem     *new_item;
          GimpItem     *parent;

          if (g_type_is_a (src_viewable_type, item_type))
            item_type = G_TYPE_FROM_INSTANCE (src_viewable);

          dest_index = gimp_item_tree_view_get_drop_index (item_view, dest_viewable,
                                                           drop_pos,
                                                           (GimpViewable **) &parent);

          new_item = gimp_item_convert (GIMP_ITEM (src_viewable),
                                        item_view->priv->image, item_type);

          item_view_class->add_item (item_view->priv->image, new_item,
                                     parent, dest_index, TRUE);
        }
    }
  else if (dest_viewable)
    {
      gimp_image_undo_group_start (item_view->priv->image,
                                   GIMP_UNDO_GROUP_IMAGE_ITEM_REORDER,
                                   GIMP_ITEM_GET_CLASS (src_viewables->data)->reorder_desc);

      for (iter = src_viewables; iter; iter = iter->next)
        {
          GimpViewable *src_viewable = iter->data;
          GimpItem     *src_parent;
          GimpItem     *dest_parent;
          gint          src_index;
          gint          dest_index;

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
    }

  if (src_viewables_reversed)
    /* The caller keeps a copy to src_viewables to free it. If we
     * reverse it, the pointer stays valid yet ends up pointing to the
     * now last (previously first) element of the list. So we leak the
     * whole list but this element. Let's reverse back the list to have
     * next and prev pointers same as call time.
     */
    src_viewables = g_list_reverse (src_viewables);

  gimp_image_undo_group_end (item_view->priv->image);
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

static void
gimp_item_tree_view_new_list_dropped (GtkWidget    *widget,
                                      gint          x,
                                      gint          y,
                                      GList        *viewables,
                                      gpointer      data)
{
  GimpItemTreeViewClass *item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (data);
  GimpContainerView     *view            = GIMP_CONTAINER_VIEW (data);
  GimpAction            *action;

  action = gimp_ui_manager_find_action (gimp_editor_get_ui_manager (GIMP_EDITOR (view)),
                                        item_view_class->action_group,
                                        item_view_class->new_default_action);

  if (item_view_class->new_default_action && viewables && action &&
      gimp_container_view_contains (view, viewables))
    gimp_action_activate (action);
}

/*  GimpImage callbacks  */

static void
gimp_item_tree_view_item_changed (GimpImage        *image,
                                  GimpItemTreeView *view)
{
  GList *items;

  items = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_selected_items (view->priv->image);

  gimp_container_view_select_items (GIMP_CONTAINER_VIEW (view), items);
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

      renderer = gimp_container_tree_store_get_renderer (GIMP_CONTAINER_TREE_STORE (tree_view->model),
                                                         &iter);

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
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreePath           *path;
  GtkTreeIter            iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpContext      *context;
      GimpViewRenderer *renderer;
      GimpItem         *item;
      GimpImage        *image;
      gboolean          active;

      context = gimp_container_view_get_context (GIMP_CONTAINER_VIEW (view));

      renderer = gimp_container_tree_store_get_renderer (GIMP_CONTAINER_TREE_STORE (tree_view->model),
                                                         &iter);
      g_object_get (toggle,
                    "active", &active,
                    NULL);

      item = GIMP_ITEM (renderer->viewable);
      g_object_unref (renderer);

      image = gimp_item_get_image (item);

      if ((state & GDK_SHIFT_MASK) ||
          (state & GDK_MOD1_MASK))
        {
          gimp_item_toggle_exclusive_visible (item, (state & GDK_MOD1_MASK), context);
        }
      else
        {
          GimpUndo *undo;
          gboolean  push_undo = TRUE;

          undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                               GIMP_UNDO_ITEM_VISIBILITY);

          if (undo && GIMP_ITEM_UNDO (undo)->item == item)
            push_undo = FALSE;

          if (! gimp_item_set_visible (item, ! active, push_undo))
            gimp_item_tree_view_blink_lock (view, item);

          if (!push_undo)
            gimp_undo_refresh_preview (undo, context);
        }

      gimp_image_flush (image);
    }

  gtk_tree_path_free (path);
}


/*  "Locked" callbacks  */

static void
gimp_item_tree_view_lock_clicked (GtkCellRendererToggle *toggle,
                                  gchar                 *path_str,
                                  GdkModifierType        state,
                                  GimpItemTreeView      *view)
{
  GtkTreePath *path;
  GtkTreeIter  iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (GIMP_CONTAINER_TREE_VIEW (view)->model,
                               &iter, path))
    {
      GimpViewRenderer       *renderer;
      GimpContainerTreeStore *store;
      GimpItem               *item;
      GdkRectangle            rect;

      /* Update the lock state. */
      store = GIMP_CONTAINER_TREE_STORE (GIMP_CONTAINER_TREE_VIEW (view)->model);
      renderer = gimp_container_tree_store_get_renderer (store, &iter);
      item = GIMP_ITEM (renderer->viewable);
      g_object_unref (renderer);
      gimp_item_tree_view_update_lock_box (view, item, path);

      /* Change popover position. */
      gtk_tree_view_get_cell_area (GIMP_CONTAINER_TREE_VIEW (view)->view, path,
                                   gtk_tree_view_get_column (GIMP_CONTAINER_TREE_VIEW (view)->view, 1),
                                   &rect);
      gtk_tree_view_convert_bin_window_to_widget_coords (GIMP_CONTAINER_TREE_VIEW (view)->view,
                                                         rect.x, rect.y, &rect.x, &rect.y);
      gtk_popover_set_pointing_to (GTK_POPOVER (view->priv->lock_popover), &rect);

      gtk_widget_show (view->priv->lock_popover);
    }
}

/*  "Effects" callbacks */
static void
gimp_item_tree_view_effects_clicked (GtkCellRendererToggle *toggle,
                                     gchar                 *path_str,
                                     GdkModifierType        state,
                                     GimpItemTreeView      *view)
{
  GtkTreePath *path;
  GtkTreeIter  iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (GIMP_CONTAINER_TREE_VIEW (view)->model,
                               &iter, path))
    {
      GimpViewRenderer       *renderer;
      GimpContainerTreeStore *store;
      GimpItem               *item;
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
      item = GIMP_ITEM (renderer->viewable);
      g_object_unref (renderer);

      /* Get filters */
      if (children)
        {
          g_signal_handlers_disconnect_by_func (children->data,
                                                gimp_item_tree_view_effects_filters_selected,
                                                view);
          g_signal_handlers_disconnect_by_func (children->data,
                                                gimp_item_tree_view_effects_activate_filter,
                                                view);
          gtk_widget_destroy (children->data);
          g_list_free (children);
        }

      view->priv->effects_drawable = GIMP_DRAWABLE (item);
      filters = gimp_drawable_get_filters (GIMP_DRAWABLE (item));
      for (filter_list = GIMP_LIST (filters)->queue->tail; filter_list;
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
                                       gimp_item_tree_view_effects_visible_all_toggled,
                                       view);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (view->priv->effects_visible_button),
                                    visible);
      g_signal_handlers_unblock_by_func (view->priv->effects_visible_button,
                                         gimp_item_tree_view_effects_visible_all_toggled,
                                         view);

      /* Only show if we have at least one active filter */
      if (n_children > 0)
        {
          GtkCellRenderer       *renderer;
          GtkTreeViewColumn     *column;
          GimpContainerTreeView *filter_tree_view = NULL;
          GtkWidget             *scrolled_window  = NULL;

          filter_view = gimp_container_tree_view_new (filters,
                                                      gimp_container_view_get_context (GIMP_CONTAINER_VIEW (view)),
                                                      GIMP_VIEW_SIZE_SMALL, 0);
          filter_tree_view = GIMP_CONTAINER_TREE_VIEW (filter_view);

          /* Connect filter active signal */
          if (view->priv->filters_active_changed_handler)
            gimp_tree_handler_disconnect (view->priv->filters_active_changed_handler);

          view->priv->filters_active_changed_handler =
            gimp_tree_handler_connect (filters, "active-changed",
                                       G_CALLBACK (gimp_item_tree_view_filter_active_changed),
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
                                   G_CALLBACK (gimp_item_tree_view_effects_visible_toggled),
                                   filter_tree_view, 0);

          /* Update filter visible icon */
          for (filter_list = GIMP_LIST (filters)->queue->tail; filter_list;
               filter_list = g_list_previous (filter_list))
            {
              if (GIMP_IS_DRAWABLE_FILTER (filter_list->data))
                gimp_item_tree_view_filter_active_changed (GIMP_FILTER (filter_list->data),
                                                           filter_tree_view);
            }

          g_signal_connect (filter_tree_view, "select-items",
                            G_CALLBACK (gimp_item_tree_view_effects_filters_selected),
                            view);
          g_signal_connect_object (filter_tree_view,
                                   "activate-item",
                                   G_CALLBACK (gimp_item_tree_view_effects_activate_filter),
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

          gimp_item_tree_view_filters_changed (item, view);
          gtk_widget_show (view->priv->effects_popover);
        }
    }
}

static gboolean
gimp_item_tree_view_effects_filters_selected (GimpContainerView  *view,
                                              GList              *filters,
                                              GList              *paths,
                                              GimpItemTreeView   *item_view)
{
  g_return_val_if_fail (g_list_length (filters) <= 1, FALSE);

  if (filters                           &&
      item_view->priv->effects_drawable &&
      GIMP_IS_DRAWABLE (item_view->priv->effects_drawable))
    {
      GimpDrawableFilter *filter = filters->data;
      GimpContainer      *container;
      gint                index;
      gint                n_children;
      gboolean            is_tool_op = FALSE;
      GeglNode           *op_node    = NULL;

      item_view->priv->effects_filter = filter;

      container =
        gimp_drawable_get_filters (GIMP_DRAWABLE (item_view->priv->effects_drawable));

      index = gimp_container_get_child_index (container,
                                              GIMP_OBJECT (filter));

      n_children = gimp_container_get_n_children (container);

      /* TODO: For now, prevent raising/lowering tool operations like Warp. */
      op_node = gimp_drawable_filter_get_operation (filter);
      if (op_node &&
          ! strcmp (gegl_node_get_operation (op_node), "GraphNode"))
        is_tool_op = TRUE;

      gtk_widget_set_sensitive (item_view->priv->effects_remove_button,
                                ! is_tool_op);
      gtk_widget_set_sensitive (item_view->priv->effects_raise_button,
                                (index != 0) && ! is_tool_op);
      gtk_widget_set_sensitive (item_view->priv->effects_lower_button,
                                (index != n_children - 1) && ! is_tool_op);
    }

  return TRUE;
}

static void
gimp_item_tree_view_effects_activate_filter (GtkWidget         *widget,
                                             GimpViewable      *viewable,
                                             gpointer           insert_data,
                                             GimpItemTreeView  *view)
{
  if (gtk_widget_get_sensitive (view->priv->effects_edit_button))
    gimp_item_tree_view_effects_edited_clicked (widget, view);
}

static void
gimp_item_tree_view_filter_active_changed (GimpFilter            *filter,
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
gimp_item_tree_view_effects_visible_toggled (GtkCellRendererToggle *toggle,
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
          GimpImage    *image    = NULL;
          GimpDrawable *drawable = NULL;

          drawable = gimp_drawable_filter_get_drawable (filter);

          if (drawable)
            {
              image   = gimp_item_get_image (GIMP_ITEM (drawable));
              visible = gimp_filter_get_active (GIMP_FILTER (filter));

              gimp_filter_set_active (GIMP_FILTER (filter), ! visible);

              /* Refresh the drawable */
              gimp_item_set_visible (GIMP_ITEM (drawable), FALSE, FALSE);
              gimp_image_flush (image);
              gimp_item_set_visible (GIMP_ITEM (drawable), TRUE, FALSE);
              gimp_image_flush (image);
            }
        }
    }
}

static void
gimp_item_tree_view_effects_visible_all_toggled (GtkWidget        *widget,
                                                 GimpItemTreeView *view)
{
  if (view->priv->effects_drawable)
    {
      gboolean       visible;
      GimpContainer *filter_stack;
      GList         *list;
      GimpImage     *image = view->priv->image;

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

      /* Hack to make the effects visibly change */
      gimp_item_set_visible (GIMP_ITEM (view->priv->effects_drawable), FALSE, FALSE);
      gimp_image_flush (image);
      gimp_item_set_visible (GIMP_ITEM (view->priv->effects_drawable), TRUE, FALSE);
      gimp_image_flush (image);
    }
}

static void
gimp_item_tree_view_effects_edited_clicked (GtkWidget        *widget,
                                            GimpItemTreeView *view)
{
  GimpImage *image     = view->priv->image;
  GList     *drawables = gimp_image_get_selected_drawables (image);

  /* TODO: Revisit when we can apply filters to multiple layers */
  if (g_list_length (drawables) != 1)
    {
      gimp_message_literal (image->gimp, G_OBJECT (view), GIMP_MESSAGE_ERROR,
                            _("Cannot modify multiple drawables. Select only one."));

      g_list_free (drawables);
      return;
    }
  else
    {
      if (! gimp_item_is_visible (GIMP_ITEM (drawables->data)) &&
          ! GIMP_GUI_CONFIG (image->gimp->config)->edit_non_visible)
        {
          gimp_message_literal (image->gimp, G_OBJECT (view),
                                GIMP_MESSAGE_ERROR,
                               _("A selected layer is not visible."));

          g_list_free (drawables);
          return;
        }
      else if (gimp_item_get_lock_content (GIMP_ITEM (drawables->data)))
        {
          gimp_message_literal (view->priv->image->gimp, G_OBJECT (view),
                                GIMP_MESSAGE_WARNING,
                                _("A selected layer's pixels are locked."));

          g_list_free (drawables);
          return;
        }
    }
  g_list_free (drawables);

  if (! view->priv->effects_filter ||
      ! GIMP_IS_DRAWABLE_FILTER (view->priv->effects_filter))
    return;

  if (view->priv->effects_drawable)
    {
      GeglNode *op =
        gimp_drawable_filter_get_operation (view->priv->effects_filter);

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

              gimp_filter_history_add (image->gimp, procedure);

              variant = g_variant_new_uint64 (GPOINTER_TO_SIZE (procedure));
              g_variant_take_ref (variant);
              filters_run_procedure (image->gimp,
                                     gimp_context_get_display (gimp_get_user_context (image->gimp)),
                                     procedure, GIMP_RUN_INTERACTIVE);

              g_variant_unref (variant);
              g_object_unref (procedure);

              /* Disable buttons until we're done editing */
              gimp_item_tree_effects_set_sensitive (view, FALSE);
            }

          g_free (name);
          g_free (operation);
        }
    }
}

static void
gimp_item_tree_view_effects_raised_clicked (GtkWidget        *widget,
                                            GimpItemTreeView *view)
{
  GimpImage    *image    = view->priv->image;
  GimpDrawable *drawable = NULL;

  if (! view->priv->effects_filter ||
      ! GIMP_IS_DRAWABLE_FILTER (view->priv->effects_filter))
    return;

  if (gimp_drawable_filter_get_mask (view->priv->effects_filter) == NULL)
    {
      gimp_message_literal (image->gimp, G_OBJECT (view), GIMP_MESSAGE_ERROR,
                            _("Cannot reorder a filter that is being edited."));
      return;
    }

  drawable = gimp_drawable_filter_get_drawable (view->priv->effects_filter);
  if (drawable)
    {
      GimpContainer *filters;
      gint           index;

      filters = gimp_drawable_get_filters (drawable);

      index = gimp_container_get_child_index (filters,
                                              GIMP_OBJECT (view->priv->effects_filter));
      index--;

      if (index >= 0)
        {
          GimpChannel   *mask = NULL;
          GeglRectangle  rect;

          gimp_image_undo_push_filter_reorder (image, _("Reorder filter"),
                                               drawable,
                                               view->priv->effects_filter);

          gimp_container_reorder (filters, GIMP_OBJECT (view->priv->effects_filter),
                                  index);

          if (gtk_widget_get_sensitive (view->priv->effects_edit_button))
            {
              gtk_widget_set_sensitive (view->priv->effects_lower_button, TRUE);
              if (index == 0)
                gtk_widget_set_sensitive (view->priv->effects_raise_button, FALSE);
            }

          mask = gimp_drawable_filter_get_mask (view->priv->effects_filter);
          gimp_filter_stack_get_bounding_box (GIMP_FILTER_STACK (filters),
                                              &rect);

          if (gimp_channel_is_empty (mask))
            gimp_drawable_filter_refresh_crop (view->priv->effects_filter,
                                               &rect);

          /* Hack to make the effects visibly change */
          gimp_item_set_visible (GIMP_ITEM (drawable), FALSE, FALSE);
          gimp_image_flush (image);
          gimp_item_set_visible (GIMP_ITEM (drawable), TRUE, FALSE);
          gimp_image_flush (image);
        }
    }
}

static void
gimp_item_tree_view_effects_lowered_clicked (GtkWidget        *widget,
                                             GimpItemTreeView *view)
{
  GimpImage    *image    = view->priv->image;
  GimpDrawable *drawable = NULL;

  if (! view->priv->effects_filter ||
      ! GIMP_IS_DRAWABLE_FILTER (view->priv->effects_filter))
    return;

  if (gimp_drawable_filter_get_mask (view->priv->effects_filter) == NULL)
    {
      gimp_message_literal (image->gimp, G_OBJECT (view), GIMP_MESSAGE_ERROR,
                            _("Cannot reorder a filter that is being edited."));
      return;
    }

  drawable = gimp_drawable_filter_get_drawable (view->priv->effects_filter);
  if (drawable)
    {
      GimpContainer *filters;
      gint           index;

      filters = gimp_drawable_get_filters (drawable);

      index = gimp_container_get_child_index (filters,
                                              GIMP_OBJECT (view->priv->effects_filter));
      index++;

      if (index < gimp_container_get_n_children (filters))
        {
          GimpDrawableFilter *moved_filter = NULL;
          GimpChannel        *mask         = NULL;
          GeglRectangle       rect;

          gimp_image_undo_push_filter_reorder (image, _("Reorder filter"),
                                               drawable,
                                               view->priv->effects_filter);

          gimp_container_reorder (filters, GIMP_OBJECT (view->priv->effects_filter),
                                  index);

          if (gtk_widget_get_sensitive (view->priv->effects_edit_button))
            {
              gtk_widget_set_sensitive (view->priv->effects_raise_button, TRUE);
              if (index == gimp_container_get_n_children (filters) - 1)
                gtk_widget_set_sensitive (view->priv->effects_lower_button, FALSE);
            }

          moved_filter = (GimpDrawableFilter *)
            gimp_container_get_child_by_index (filters, index - 1);

          mask = gimp_drawable_filter_get_mask (moved_filter);
          gimp_filter_stack_get_bounding_box (GIMP_FILTER_STACK (filters),
                                              &rect);

          if (gimp_channel_is_empty (mask))
            gimp_drawable_filter_refresh_crop (moved_filter, &rect);


          /* Hack to make the effects visibly change */
          gimp_item_set_visible (GIMP_ITEM (drawable), FALSE, FALSE);
          gimp_image_flush (image);
          gimp_item_set_visible (GIMP_ITEM (drawable), TRUE, FALSE);
          gimp_image_flush (image);
        }
    }
}

static void
gimp_item_tree_view_effects_merged_clicked (GtkWidget        *widget,
                                            GimpItemTreeView *view)
{
  GimpContext  *context     = NULL;
  GimpToolInfo *active_tool = NULL;

  if (! view->priv->effects_filter ||
      ! GIMP_IS_DRAWABLE_FILTER (view->priv->effects_filter))
    return;

  /* Don't merge if certain tools with active filters are in use */
  context     = gimp_container_view_get_context (GIMP_CONTAINER_VIEW (view));
  active_tool = gimp_context_get_tool (context);

  if (! strcmp (gimp_object_get_name (active_tool), "gimp-cage-tool")     ||
      ! strcmp (gimp_object_get_name (active_tool), "gimp-gradient-tool") ||
      ! strcmp (gimp_object_get_name (active_tool), "gimp-warp-tool"))
    {
      view->priv->effects_filter = NULL;
      gimp_message_literal (view->priv->image->gimp, G_OBJECT (view),
                            GIMP_MESSAGE_ERROR,
                            _("Effects from active tools can not be merged."));

      return;
    }

  if (view->priv->effects_drawable &&
      ! gimp_viewable_get_children (GIMP_VIEWABLE (view->priv->effects_drawable)))
    {
      GimpImage *image = view->priv->image;
      GeglNode  *op    = gimp_drawable_filter_get_operation (view->priv->effects_filter);

      /* Don't merge if the layer is currently locked */
      if (gimp_item_get_lock_content (GIMP_ITEM (view->priv->effects_drawable)))
        {
          gimp_message_literal (view->priv->image->gimp, G_OBJECT (view),
                                GIMP_MESSAGE_WARNING,
                                _("The layer to merge down to is locked."));
          return;
        }

      if (op)
        {
          gimp_drawable_merge_filters (GIMP_DRAWABLE (view->priv->effects_drawable));
          gimp_drawable_clear_filters (GIMP_DRAWABLE (view->priv->effects_drawable));

          view->priv->effects_filter = NULL;

          /* Close NDE pop-over on successful merge */
          gtk_widget_set_visible (view->priv->effects_popover, FALSE);

          /* Hack to make the effects visibly change */
          gimp_item_set_visible (GIMP_ITEM (view->priv->effects_drawable), FALSE, FALSE);
          gimp_image_flush (image);
          gimp_item_set_visible (GIMP_ITEM (view->priv->effects_drawable), TRUE, FALSE);
          gimp_image_flush (image);
        }
    }
}

static void
gimp_item_tree_view_effects_removed_clicked (GtkWidget        *widget,
                                             GimpItemTreeView *view)
{
  if (view->priv->effects_drawable)
    {
      GimpImage     *image   = view->priv->image;
      GimpContainer *filters = NULL;

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

      /* Hack to make the effects visibly change */
      gimp_item_set_visible (GIMP_ITEM (view->priv->effects_drawable), FALSE, FALSE);
      gimp_image_flush (image);
      gimp_item_set_visible (GIMP_ITEM (view->priv->effects_drawable), TRUE, FALSE);
      gimp_image_flush (image);
    }
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
      GeglColor     *color = gegl_color_new ("none");
      GdkRGBA        rgba;
      gboolean       has_color;

      has_color = gimp_get_color_tag_color (gimp_item_get_merged_color_tag (item),
                                            color,
                                            gimp_item_get_color_tag (item) ==
                                            GIMP_COLOR_TAG_NONE);
      gegl_color_get_pixel (color, babl_format ("R'G'B'A double"), &rgba);
      g_object_unref (color);

      gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                          view->priv->model_column_color_tag,
                          has_color ? &rgba : NULL,
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
gimp_item_tree_view_lock_changed (GimpItem         *item,
                                  GimpItemTreeView *view)
{
  GimpContainerView     *container_view  = GIMP_CONTAINER_VIEW (view);
  GimpContainerTreeView *tree_view       = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter;
  const gchar           *icon_name;
  gint                   n_locks;

  iter = gimp_container_view_lookup (container_view,
                                     (GimpViewable *) item);

  n_locks = gimp_item_tree_view_get_n_locks (view, item, &icon_name);

  if (iter)
    gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,

                        view->priv->model_column_locked,
                        n_locks > 0,

                        view->priv->model_column_lock_icon,
                        icon_name,

                        -1);

  if (view->priv->lock_box_item == item)
    gimp_item_tree_view_update_lock_box (view, item, NULL);
}

static void
gimp_item_tree_view_filters_changed (GimpItem         *item,
                                     GimpItemTreeView *view)
{
  GimpContainerView     *container_view = GIMP_CONTAINER_VIEW (view);
  GimpContainerTreeView *tree_view      = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter;
  GimpContainer         *filters;
  GList                 *filter_list    = NULL;
  gint                   n_filters      = 0;
  gboolean               fs_disabled    = FALSE;

  iter = gimp_container_view_lookup (container_view,
                                     (GimpViewable *) item);

  filters = gimp_drawable_get_filters (GIMP_DRAWABLE (item));
  /* Since floating selections are also stored in the filter stack,
   * we need to verify what's in there to get the correct count */
  if (filters)
    {
      for (filter_list = GIMP_LIST (filters)->queue->tail; filter_list;
           filter_list = g_list_previous (filter_list))
        {
          if (GIMP_IS_DRAWABLE_FILTER (filter_list->data))
            n_filters++;
          else
            fs_disabled = TRUE;
        }
    }

  if (n_filters == 0 || fs_disabled)
    view->priv->effects_filter = NULL;

  if (iter)
    gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                        view->priv->model_column_effects,
                        n_filters > 0,
                        -1);

  /* Re-enable buttons after editing */
  gimp_item_tree_effects_set_sensitive (view, ! fs_disabled);

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

static gboolean
gimp_item_tree_view_lock_button_release (GtkWidget        *widget,
                                         GdkEvent         *event,
                                         GimpItemTreeView *view)
{
  GdkEventButton  *bevent = (GdkEventButton *) event;
  LockToggle      *data;
  GdkModifierType  modifiers;

  data      = g_object_get_data (G_OBJECT (widget), "lock-data");
  modifiers = bevent->state & gimp_get_all_modifiers_mask ();

  if (modifiers == GDK_SHIFT_MASK ||
      modifiers == GDK_MOD1_MASK)
    gimp_item_toggle_exclusive (view->priv->lock_box_item,
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
gimp_item_tree_view_lock_toggled (GtkWidget         *widget,
                                  GimpItemTreeView  *view)
{
  GimpImage   *image = view->priv->image;
  LockToggle  *data;
  GimpUndo    *undo;
  const gchar *undo_label;
  gboolean     push_undo = TRUE;
  gboolean     locked;

  locked = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
  data = g_object_get_data (G_OBJECT (widget), "lock-data");

  if (! data->can_lock (view->priv->lock_box_item) ||
      locked == data->is_locked (view->priv->lock_box_item))
    return;

  undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                       data->undo_type);

  if (undo && GIMP_ITEM_UNDO (undo)->item == view->priv->lock_box_item)
    push_undo = FALSE;

  if (push_undo)
    {
      if (locked)
        undo_label = data->undo_lock_desc;
      else
        undo_label = data->undo_unlock_desc;

      gimp_image_undo_group_start (image, data->group_undo_type,
                                   undo_label);
    }

  /* TODO: maybe we should also have a modifier-interaction where we can
   * lock all same-level items (Shift-click for instance like eye click)
   * and maybe another modifier for all selected items at once.
   */
  data->lock (view->priv->lock_box_item, locked, push_undo);

  gimp_image_flush (image);
  if (push_undo)
    gimp_image_undo_group_end (image);
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
  GdkModifierType        modifiers = (state & gimp_get_all_modifiers_mask ());
  gboolean               handled   = FALSE;

  path = gtk_tree_path_new_from_string (path_str);

  /* Alt modifier at least and none other than Alt, Shift and Ctrl. */
  if ((modifiers & GDK_MOD1_MASK)                                          &&
      ! (modifiers & ~(GDK_MOD1_MASK | GDK_SHIFT_MASK | GDK_CONTROL_MASK)) &&
      gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpImage        *image    = gimp_item_tree_view_get_image (item_view);
      GimpViewRenderer *renderer = NULL;

      renderer = gimp_container_tree_store_get_renderer (GIMP_CONTAINER_TREE_STORE (tree_view->model),
                                                         &iter);

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
gimp_item_tree_view_update_lock_box (GimpItemTreeView *view,
                                     GimpItem         *item,
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
                                           gimp_item_tree_view_lock_toggled,
                                           view);
          g_signal_handlers_block_by_func (data->toggle,
                                           gimp_item_tree_view_lock_button_release,
                                           view);

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->toggle),
                                        data->is_locked (item));

          g_signal_handlers_unblock_by_func (data->toggle,
                                             gimp_item_tree_view_lock_toggled,
                                             view);
          g_signal_handlers_unblock_by_func (data->toggle,
                                           gimp_item_tree_view_lock_button_release,
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
gimp_item_tree_view_popover_button_press (GtkWidget        *widget,
                                          GdkEvent         *event,
                                          GimpItemTreeView *view)
{
  GimpContainerTreeView *tree_view    = GIMP_CONTAINER_TREE_VIEW (view);
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
          column != gtk_tree_view_get_column (GIMP_CONTAINER_TREE_VIEW (view)->view, 1))
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
gimp_item_tree_view_row_expanded (GtkTreeView      *tree_view,
                                  GtkTreeIter      *iter,
                                  GtkTreePath      *path,
                                  GimpItemTreeView *item_view)
{
  GimpItemTreeViewClass *item_view_class;
  GList                 *selected_items;
  GList                 *list;

  item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (item_view);
  selected_items  = item_view_class->get_selected_items (item_view->priv->image);

  if (selected_items)
    {
      GimpContainerTreeStore *store;
      GimpViewRenderer       *renderer;
      GimpItem               *expanded_item;

      store    = GIMP_CONTAINER_TREE_STORE (GIMP_CONTAINER_TREE_VIEW (item_view)->model);
      renderer = gimp_container_tree_store_get_renderer (store, iter);
      expanded_item = GIMP_ITEM (renderer->viewable);
      g_object_unref (renderer);

      for (list = selected_items; list; list = list->next)
        {
          /*  don't select an item while it is being inserted  */
          if (! gimp_container_view_lookup (GIMP_CONTAINER_VIEW (item_view),
                                            list->data))
            return;

          /*  select items only if they were made visible by expanding
           *  their immediate parent. See bug #666561.
           */
          if (gimp_item_get_parent (list->data) != expanded_item)
            return;
        }

      gimp_container_view_select_items (GIMP_CONTAINER_VIEW (item_view),
                                        selected_items);
    }
}


/*  Helper functions.  */

static gint
gimp_item_tree_view_get_n_locks (GimpItemTreeView *view,
                                 GimpItem         *item,
                                 const gchar     **icon_name)
{
  GList *list;
  gint   n_locks = 0;

  *icon_name = GIMP_ICON_LOCK_MULTI;

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

static void
gimp_item_tree_effects_set_sensitive (GimpItemTreeView *view,
                                      gboolean          is_sensitive)
{
  gboolean is_group = FALSE;

  /* Do not allow merging down effects on group layers */
  if (view->priv->effects_drawable &&
      gimp_viewable_get_children (GIMP_VIEWABLE (view->priv->effects_drawable)))
    is_group = TRUE;

  gtk_widget_set_sensitive (view->priv->effects_box, is_sensitive);
  gtk_widget_set_sensitive (view->priv->effects_visible_button, is_sensitive);
  gtk_widget_set_sensitive (view->priv->effects_edit_button, is_sensitive);
  gtk_widget_set_sensitive (view->priv->effects_raise_button, is_sensitive);
  gtk_widget_set_sensitive (view->priv->effects_lower_button, is_sensitive);
  gtk_widget_set_sensitive (view->priv->effects_merge_button,
                            (is_sensitive && ! is_group));
  gtk_widget_set_sensitive (view->priv->effects_remove_button, is_sensitive);
}

/* Functions for the item search/selection feature. */

static void
gimp_item_tree_view_create_search_popover (GimpItemTreeView *view,
                                           GtkIconSize       button_size)
{
  PangoAttrList *attrs;
  GtkWidget     *grid;
  GtkWidget     *placeholder;
  GtkWidget     *button;

  g_return_if_fail (GIMP_IS_ITEM_TREE_VIEW (view));
  g_return_if_fail (view->priv->search_popover == NULL);
  g_return_if_fail (GTK_IS_IMAGE (view->priv->search_button));

  view->priv->search_popover = gtk_popover_new (view->priv->search_button);
  gtk_popover_set_modal (GTK_POPOVER (view->priv->search_popover), TRUE);

  g_signal_connect (view->priv->search_popover,
                    "show",
                    G_CALLBACK (gimp_item_tree_view_search_popover_shown),
                    view);

  grid = gtk_grid_new ();

  /* Link popover: regexp search. */
  view->priv->link_search_entry = gtk_entry_new ();
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (view->priv->link_search_entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     "system-search");
  gtk_grid_attach (GTK_GRID (grid),
                   view->priv->link_search_entry,
                   0, 0, 2, 1);
  gtk_widget_show (view->priv->link_search_entry);
  g_signal_connect (view->priv->link_search_entry,
                    "key-release-event",
                    G_CALLBACK (gimp_item_tree_view_search_key_release),
                    view);

  g_signal_connect (GIMP_CONTAINER_TREE_VIEW (view)->view,
                    "start-interactive-search",
                    G_CALLBACK (gimp_item_tree_view_start_interactive_search),
                    view);

  /* Link popover: existing links. */
  view->priv->link_list = gtk_list_box_new ();
  placeholder = gtk_label_new (_("No layer set stored"));
  attrs = pango_attr_list_new ();
  gtk_label_set_attributes (GTK_LABEL (placeholder),
                            attrs);
  pango_attr_list_insert (attrs, pango_attr_style_new (PANGO_STYLE_ITALIC));
  pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_ULTRALIGHT));
  pango_attr_list_unref (attrs);
  gtk_list_box_set_placeholder (GTK_LIST_BOX (view->priv->link_list),
                                placeholder);
  gtk_widget_show (placeholder);
  gtk_grid_attach (GTK_GRID (grid),
                   view->priv->link_list,
                   0, 1, 2, 1);
  gtk_widget_show (view->priv->link_list);

  gtk_list_box_set_activate_on_single_click (GTK_LIST_BOX (view->priv->link_list),
                                             TRUE);

  /* Link popover: new links. */

  view->priv->link_entry = gtk_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (view->priv->link_entry),
                                  _("New layer set's name"));
  gtk_grid_attach (GTK_GRID (grid),
                   view->priv->link_entry,
                   0, 2, 1, 1);
  gtk_widget_show (view->priv->link_entry);

  button = gtk_button_new_from_icon_name (GIMP_ICON_LIST_ADD, button_size);
  gtk_grid_attach (GTK_GRID (grid),
                   button,
                   1, 2, 1, 1);
  g_signal_connect_swapped (button,
                            "clicked",
                            G_CALLBACK (gimp_item_tree_view_new_link_clicked),
                            view);
  gtk_widget_show (button);
  view->priv->new_link_button = button;

  /* Enter on any entry activates the link creation then exits in case
   * of success.
   */
  g_signal_connect_swapped (view->priv->link_entry,
                            "activate",
                            G_CALLBACK (gimp_item_tree_view_new_link_exit),
                            view);

  gtk_container_add (GTK_CONTAINER (view->priv->search_popover), grid);
  gtk_widget_show (grid);
}

static void
gimp_item_tree_view_search_popover_shown (GtkPopover       *popover,
                                          GimpItemTreeView *view)
{
  GimpImage        *image;
  GimpSelectMethod  pattern_syntax;

  image = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));

  gtk_widget_set_sensitive (view->priv->link_entry, FALSE);
  if (! image)
    {
      gtk_widget_set_tooltip_text (view->priv->link_search_entry,
                                   _("Select layers by text search"));
      gtk_entry_set_placeholder_text (GTK_ENTRY (view->priv->link_search_entry),
                                      _("Text search"));
      return;
    }

  gtk_widget_set_sensitive (view->priv->link_entry, TRUE);
  gtk_widget_grab_focus (view->priv->link_search_entry);
  g_object_get (image->gimp->config,
                "items-select-method", &pattern_syntax,
                NULL);
  switch (pattern_syntax)
    {
    case GIMP_SELECT_PLAIN_TEXT:
      gtk_widget_set_tooltip_text (view->priv->link_search_entry,
                                   _("Select layers by text search"));
      gtk_entry_set_placeholder_text (GTK_ENTRY (view->priv->link_search_entry),
                                      _("Text search"));
      break;
    case GIMP_SELECT_GLOB_PATTERN:
      gtk_widget_set_tooltip_text (view->priv->link_search_entry,
                                   _("Select layers by glob patterns"));
      gtk_entry_set_placeholder_text (GTK_ENTRY (view->priv->link_search_entry),
                                      _("Glob pattern search"));
      break;
    case GIMP_SELECT_REGEX_PATTERN:
      gtk_widget_set_tooltip_text (view->priv->link_search_entry,
                                   _("Select layers by regular expressions"));
      gtk_entry_set_placeholder_text (GTK_ENTRY (view->priv->link_search_entry),
                                      _("Regular Expression search"));
      break;
    }
}

static void
gimp_item_tree_view_floating_selection_changed (GimpImage        *image,
                                                GimpItemTreeView *view)
{
  gtk_widget_set_sensitive (view->priv->search_button,
                            image && ! gimp_image_get_floating_selection (image));
}

static void
gimp_item_tree_view_item_links_changed (GimpImage        *image,
                                        GType             item_type,
                                        GimpItemTreeView *view)
{
  GimpItemTreeViewClass *item_view_class;
  GtkWidget             *grid;
  GtkWidget             *label;
  GtkWidget             *event_box;
  GtkWidget             *icon;
  GtkSizeGroup          *label_size;
  GList                 *links;
  GList                 *iter;

  item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (view);
  if (item_type != item_view_class->item_type)
    return;

  gtk_container_foreach (GTK_CONTAINER (view->priv->link_list),
                         (GtkCallback) gtk_widget_destroy, NULL);
  gtk_widget_set_sensitive (view->priv->search_button, image != NULL);

  if (! image)
    return;

  links = gimp_image_get_stored_item_sets (image, item_type);

  label_size = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
  for (iter = links; iter; iter = iter->next)
    {
      GimpSelectMethod method;

      grid = gtk_grid_new ();

      label = gtk_label_new (gimp_object_get_name (iter->data));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      if (gimp_item_list_is_pattern (iter->data, &method))
        {
          gchar         *display_name;
          PangoAttrList *attrs;

          display_name = g_strdup_printf ("<small>[%s]</small> %s",
                                          method == GIMP_SELECT_PLAIN_TEXT ? _("search") :
                                          (method == GIMP_SELECT_GLOB_PATTERN ? _("glob") : _("regexp")),
                                          gimp_object_get_name (iter->data));
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
                        G_CALLBACK (gimp_item_tree_view_unlink_clicked),
                        view);
      gtk_grid_attach (GTK_GRID (grid), event_box, 2, 0, 1, 1);
      gtk_widget_show (event_box);

      icon = gtk_image_new_from_icon_name (GIMP_ICON_EDIT_DELETE, GTK_ICON_SIZE_MENU);
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
                        G_CALLBACK (gimp_item_tree_view_link_clicked),
                        view);

      gtk_widget_show (grid);
    }
  g_object_unref (label_size);
  gtk_list_box_unselect_all (GTK_LIST_BOX (view->priv->link_list));
}

static gboolean
gimp_item_tree_view_link_clicked (GtkWidget        *box,
                                  GdkEvent         *event,
                                  GimpItemTreeView *view)
{
  GimpImage       *image;
  GdkEventButton  *bevent = (GdkEventButton *) event;
  GdkModifierType  modifiers;

  image = gimp_item_tree_view_get_image (view);

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GTK_IS_EVENT_BOX (box), FALSE);

  modifiers = bevent->state & gimp_get_all_modifiers_mask ();
  if (modifiers == GDK_SHIFT_MASK)
    gimp_image_add_item_set (image, g_object_get_data (G_OBJECT (box), "link-set"));
  else if (modifiers == GDK_CONTROL_MASK)
    gimp_image_remove_item_set (image, g_object_get_data (G_OBJECT (box), "link-set"));
  else if (modifiers == (GDK_CONTROL_MASK | GDK_SHIFT_MASK))
    gimp_image_intersect_item_set (image, g_object_get_data (G_OBJECT (box), "link-set"));
  else
    gimp_image_select_item_set (image, g_object_get_data (G_OBJECT (box), "link-set"));

  gtk_entry_set_text (GTK_ENTRY (view->priv->link_search_entry), "");
  /* TODO: if clicking on pattern link, fill in the pattern field? */

  return FALSE;
}

static gboolean
gimp_item_tree_view_unlink_clicked (GtkWidget        *widget,
                                    GdkEvent         *event,
                                    GimpItemTreeView *view)
{
  GimpImage *image;

  image = gimp_item_tree_view_get_image (view);

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  gimp_image_unlink_item_set (image, g_object_get_data (G_OBJECT (widget), "link-set"));

  return TRUE;
}


static gboolean
gimp_item_tree_view_search_key_release (GtkWidget        *widget,
                                        GdkEventKey      *event,
                                        GimpItemTreeView *view)
{
  GimpItemTreeViewClass *item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (view);
  GimpImage        *image;
  const gchar      *pattern;
  GimpSelectMethod  pattern_syntax;

  if (event->keyval == GDK_KEY_Escape   ||
      event->keyval == GDK_KEY_Return   ||
      event->keyval == GDK_KEY_KP_Enter ||
      event->keyval == GDK_KEY_ISO_Enter)
    {
      if (event->state & GDK_SHIFT_MASK)
        {
          if (gimp_item_tree_view_new_link_clicked (view))
            gtk_widget_hide (view->priv->search_popover);
        }
      else
        {
          gtk_widget_hide (view->priv->search_popover);
        }
      return TRUE;
    }

  gtk_entry_set_attributes (GTK_ENTRY (view->priv->link_search_entry),
                            NULL);

  image = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));
  g_clear_object (&view->priv->link_pattern_set);

  if (! image)
    return TRUE;

  g_object_get (image->gimp->config,
                "items-select-method", &pattern_syntax,
                NULL);
  pattern = gtk_entry_get_text (GTK_ENTRY (view->priv->link_search_entry));
  if (pattern && strlen (pattern) > 0)
    {
      GList  *items;
      GError *error = NULL;

      gtk_entry_set_text (GTK_ENTRY (view->priv->link_entry), "");
      gtk_widget_set_sensitive (view->priv->link_entry, FALSE);

      view->priv->link_pattern_set = gimp_item_list_pattern_new (image,
                                                                 item_view_class->item_type,
                                                                 pattern_syntax,
                                                                 pattern);
      items = gimp_item_list_get_items (view->priv->link_pattern_set, &error);
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
          gimp_image_set_selected_layers (image, NULL);
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
          gimp_image_set_selected_layers (image, NULL);
          gimp_widget_blink (view->priv->link_search_entry);
        }
      else
        {
          gimp_image_set_selected_layers (image, items);
          g_list_free (items);
        }
    }
  else
    {
      gtk_widget_set_sensitive (view->priv->link_entry, TRUE);
    }

  return TRUE;
}

static gboolean
gimp_item_tree_view_start_interactive_search (GtkTreeView      *tree_view,
                                              GimpItemTreeView *layer_view)
{
  gtk_widget_show (layer_view->priv->search_popover);
  gtk_widget_grab_focus (layer_view->priv->link_search_entry);

  return FALSE;
}

static gboolean
gimp_item_tree_view_new_link_clicked (GimpItemTreeView *view)
{
  GimpItemTreeViewClass *item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (view);
  GimpImage   *image;
  const gchar *name;

  image = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));

  if (! image)
    return TRUE;

  name = gtk_entry_get_text (GTK_ENTRY (view->priv->link_entry));
  if (name && strlen (name) > 0)
    {
      GimpItemList *set;

      set = gimp_item_list_named_new (image,
                                      item_view_class->item_type,
                                      name, NULL);
      if (set)
        {
          gimp_image_store_item_set (image, set);
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
      gimp_image_store_item_set (image, view->priv->link_pattern_set);
      view->priv->link_pattern_set = NULL;
      gtk_entry_set_text (GTK_ENTRY (view->priv->link_search_entry), "");
    }
  else
    {
      gimp_widget_blink (view->priv->link_entry);
      gimp_widget_blink (view->priv->link_search_entry);

      return FALSE;
    }

  return TRUE;
}

static void
gimp_item_tree_view_new_link_exit (GimpItemTreeView *view)
{
  if (gimp_item_tree_view_new_link_clicked (view))
    gtk_widget_hide (view->priv->search_popover);
}

static gboolean
gimp_item_tree_view_search_clicked (GtkWidget        *main_column_button,
                                    GdkEventButton   *event,
                                    GimpItemTreeView *view)
{
  gtk_popover_popup (GTK_POPOVER (view->priv->search_popover));

  return TRUE;
}

static gboolean
gimp_item_tree_view_move_cursor (GimpItemTreeView *view,
                                 guint             keyval,
                                 GdkModifierType   modifiers)
{
  GimpItemTreeViewClass *klass;
  const gchar           *action_name = NULL;

  g_return_val_if_fail (GIMP_IS_ITEM_TREE_VIEW (view), FALSE);

  klass = GIMP_ITEM_TREE_VIEW_GET_CLASS (view);
  switch (keyval)
    {
    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
      if (klass->move_cursor_down_flat_action)
        action_name = klass->move_cursor_down_flat_action;
      else
        action_name = klass->move_cursor_down_action;
      break;
    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
      if (klass->move_cursor_up_flat_action)
        action_name = klass->move_cursor_up_flat_action;
      else
        action_name = klass->move_cursor_up_action;
      break;
    case GDK_KEY_Page_Down:
    case GDK_KEY_KP_Page_Down:
      action_name = klass->move_cursor_down_action;
      break;
    case GDK_KEY_Page_Up:
    case GDK_KEY_KP_Page_Up:
      action_name = klass->move_cursor_up_action;
      break;
    case GDK_KEY_End:
    case GDK_KEY_KP_End:
      action_name = klass->move_cursor_end_action;
      break;
    case GDK_KEY_Home:
    case GDK_KEY_KP_Home:
      action_name = klass->move_cursor_start_action;
      break;
    default:
      break;
    }

  if (action_name != NULL)
    {
      GAction *action;

      action = g_action_map_lookup_action (G_ACTION_MAP (g_application_get_default ()),
                                           action_name);
      g_return_val_if_fail (action != NULL, FALSE);

      gimp_action_activate (GIMP_ACTION (action));
    }

  return TRUE;
}
