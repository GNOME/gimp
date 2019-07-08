/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpchanneltreeview.c
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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"

#include "gimpactiongroup.h"
#include "gimpchanneltreeview.h"
#include "gimpcomponenteditor.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimpdocked.h"
#include "gimphelp-ids.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


struct _GimpChannelTreeViewPrivate
{
  GtkWidget *component_editor;

  GtkWidget *toselection_button;
};


static void  gimp_channel_tree_view_view_iface_init   (GimpContainerViewInterface *iface);

static void   gimp_channel_tree_view_constructed      (GObject                 *object);

static void   gimp_channel_tree_view_drop_viewable    (GimpContainerTreeView   *view,
                                                       GimpViewable            *src_viewable,
                                                       GimpViewable            *dest_viewable,
                                                       GtkTreeViewDropPosition  drop_pos);
static void   gimp_channel_tree_view_drop_component   (GimpContainerTreeView   *tree_view,
                                                       GimpImage               *image,
                                                       GimpChannelType          component,
                                                       GimpViewable            *dest_viewable,
                                                       GtkTreeViewDropPosition  drop_pos);
static void   gimp_channel_tree_view_set_image        (GimpItemTreeView        *item_view,
                                                       GimpImage               *image);
static GimpItem * gimp_channel_tree_view_item_new     (GimpImage               *image);

static void   gimp_channel_tree_view_set_context      (GimpContainerView       *view,
                                                       GimpContext             *context);
static void   gimp_channel_tree_view_set_view_size    (GimpContainerView       *view);


G_DEFINE_TYPE_WITH_CODE (GimpChannelTreeView, gimp_channel_tree_view,
                         GIMP_TYPE_DRAWABLE_TREE_VIEW,
                         G_ADD_PRIVATE (GimpChannelTreeView)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_channel_tree_view_view_iface_init))

#define parent_class gimp_channel_tree_view_parent_class

static GimpContainerViewInterface *parent_view_iface = NULL;


static void
gimp_channel_tree_view_class_init (GimpChannelTreeViewClass *klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  GimpContainerTreeViewClass *view_class   = GIMP_CONTAINER_TREE_VIEW_CLASS (klass);
  GimpItemTreeViewClass      *iv_class     = GIMP_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructed  = gimp_channel_tree_view_constructed;

  view_class->drop_viewable  = gimp_channel_tree_view_drop_viewable;
  view_class->drop_component = gimp_channel_tree_view_drop_component;

  iv_class->set_image        = gimp_channel_tree_view_set_image;

  iv_class->item_type        = GIMP_TYPE_CHANNEL;
  iv_class->signal_name      = "active-channel-changed";

  iv_class->get_container    = gimp_image_get_channels;
  iv_class->get_active_item  = (GimpGetItemFunc) gimp_image_get_active_channel;
  iv_class->set_active_item  = (GimpSetItemFunc) gimp_image_set_active_channel;
  iv_class->add_item         = (GimpAddItemFunc) gimp_image_add_channel;
  iv_class->remove_item      = (GimpRemoveItemFunc) gimp_image_remove_channel;
  iv_class->new_item         = gimp_channel_tree_view_item_new;

  iv_class->action_group          = "channels";
  iv_class->activate_action       = "channels-edit-attributes";
  iv_class->new_action            = "channels-new";
  iv_class->new_default_action    = "channels-new-last-values";
  iv_class->raise_action          = "channels-raise";
  iv_class->raise_top_action      = "channels-raise-to-top";
  iv_class->lower_action          = "channels-lower";
  iv_class->lower_bottom_action   = "channels-lower-to-bottom";
  iv_class->duplicate_action      = "channels-duplicate";
  iv_class->delete_action         = "channels-delete";
  iv_class->lock_content_help_id  = GIMP_HELP_CHANNEL_LOCK_PIXELS;
  iv_class->lock_position_help_id = GIMP_HELP_CHANNEL_LOCK_POSITION;
}

static void
gimp_channel_tree_view_view_iface_init (GimpContainerViewInterface *view_iface)
{
  parent_view_iface = g_type_interface_peek_parent (view_iface);

  view_iface->set_context   = gimp_channel_tree_view_set_context;
  view_iface->set_view_size = gimp_channel_tree_view_set_view_size;
}

static void
gimp_channel_tree_view_init (GimpChannelTreeView *view)
{
  view->priv = gimp_channel_tree_view_get_instance_private (view);

  view->priv->component_editor   = NULL;
  view->priv->toselection_button = NULL;
}

static void
gimp_channel_tree_view_constructed (GObject *object)
{
  GimpChannelTreeView   *view      = GIMP_CHANNEL_TREE_VIEW (object);
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (object);
  GdkModifierType        extend_mask;
  GdkModifierType        modify_mask;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  extend_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_EXTEND_SELECTION);
  modify_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_MODIFY_SELECTION);

  gimp_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), GIMP_TYPE_LAYER,
                               NULL, tree_view);
  gimp_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), GIMP_TYPE_LAYER_MASK,
                               NULL, tree_view);
  gimp_dnd_component_dest_add (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);

  view->priv->toselection_button =
    gimp_editor_add_action_button (GIMP_EDITOR (view), "channels",
                                   "channels-selection-replace",
                                   "channels-selection-add",
                                   extend_mask,
                                   "channels-selection-subtract",
                                   modify_mask,
                                   "channels-selection-intersect",
                                   extend_mask | modify_mask,
                                   NULL);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (view),
                                  GTK_BUTTON (view->priv->toselection_button),
                                  GIMP_TYPE_CHANNEL);
  gtk_box_reorder_child (gimp_editor_get_button_box (GIMP_EDITOR (view)),
                         view->priv->toselection_button, 4);
}


/*  GimpContainerTreeView methods  */

static void
gimp_channel_tree_view_drop_viewable (GimpContainerTreeView   *tree_view,
                                      GimpViewable            *src_viewable,
                                      GimpViewable            *dest_viewable,
                                      GtkTreeViewDropPosition  drop_pos)
{
  GimpItemTreeView      *item_view = GIMP_ITEM_TREE_VIEW (tree_view);
  GimpImage             *image     = gimp_item_tree_view_get_image (item_view);
  GimpItemTreeViewClass *item_view_class;

  item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (item_view);

  if (GIMP_IS_DRAWABLE (src_viewable) &&
      (image != gimp_item_get_image (GIMP_ITEM (src_viewable)) ||
       G_TYPE_FROM_INSTANCE (src_viewable) != item_view_class->item_type))
    {
      GimpItem *new_item;
      GimpItem *parent;
      gint      index;

      index = gimp_item_tree_view_get_drop_index (item_view, dest_viewable,
                                                  drop_pos,
                                                  (GimpViewable **) &parent);

      new_item = gimp_item_convert (GIMP_ITEM (src_viewable),
                                    gimp_item_tree_view_get_image (item_view),
                                    item_view_class->item_type);

      gimp_item_set_linked (new_item, FALSE, FALSE);

      item_view_class->add_item (image, new_item, parent, index, TRUE);

      gimp_image_flush (image);

      return;
    }

  GIMP_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_viewable (tree_view,
                                                                src_viewable,
                                                                dest_viewable,
                                                                drop_pos);
}

static void
gimp_channel_tree_view_drop_component (GimpContainerTreeView   *tree_view,
                                       GimpImage               *src_image,
                                       GimpChannelType          component,
                                       GimpViewable            *dest_viewable,
                                       GtkTreeViewDropPosition  drop_pos)
{
  GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (tree_view);
  GimpImage        *image     = gimp_item_tree_view_get_image (item_view);
  GimpItem         *new_item;
  GimpChannel      *parent;
  gint              index;
  const gchar      *desc;
  gchar            *name;

  index = gimp_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (GimpViewable **) &parent);

  gimp_enum_get_value (GIMP_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);
  name = g_strdup_printf (_("%s Channel Copy"), desc);

  new_item = GIMP_ITEM (gimp_channel_new_from_component (src_image, component,
                                                         name, NULL));

  /*  copied components are invisible by default so subsequent copies
   *  of components don't affect each other
   */
  gimp_item_set_visible (new_item, FALSE, FALSE);

  g_free (name);

  if (src_image != image)
    GIMP_ITEM_GET_CLASS (new_item)->convert (new_item, image,
                                             GIMP_TYPE_CHANNEL);

  gimp_image_add_channel (image, GIMP_CHANNEL (new_item), parent, index, TRUE);

  gimp_image_flush (image);
}


/*  GimpItemTreeView methods  */

static void
gimp_channel_tree_view_set_image (GimpItemTreeView *item_view,
                                  GimpImage        *image)
{
  GimpChannelTreeView *channel_view = GIMP_CHANNEL_TREE_VIEW (item_view);

  if (! channel_view->priv->component_editor)
    {
      GimpContainerView *view = GIMP_CONTAINER_VIEW (item_view);
      gint               view_size;

      view_size = gimp_container_view_get_view_size (view, NULL);

      channel_view->priv->component_editor =
        gimp_component_editor_new (view_size,
                                   gimp_editor_get_menu_factory (GIMP_EDITOR (item_view)));
      gimp_docked_set_context (GIMP_DOCKED (channel_view->priv->component_editor),
                               gimp_container_view_get_context (view));
      gtk_box_pack_start (GTK_BOX (item_view), channel_view->priv->component_editor,
                          FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (item_view),
                             channel_view->priv->component_editor, 0);
    }

  if (! image)
    gtk_widget_hide (channel_view->priv->component_editor);

  gimp_image_editor_set_image (GIMP_IMAGE_EDITOR (channel_view->priv->component_editor),
                               image);

  GIMP_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (item_view, image);

  if (gimp_item_tree_view_get_image (item_view))
    gtk_widget_show (channel_view->priv->component_editor);
}

static GimpItem *
gimp_channel_tree_view_item_new (GimpImage *image)
{
  GimpChannel *new_channel;
  GimpRGB      color;

  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 0.5);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE,
                               _("New Channel"));

  new_channel = gimp_channel_new (image,
                                  gimp_image_get_width (image),
                                  gimp_image_get_height (image),
                                  _("Channel"), &color);

  gimp_image_add_channel (image, new_channel,
                          GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

  gimp_image_undo_group_end (image);

  return GIMP_ITEM (new_channel);
}


/*  GimpContainerView methods  */

static void
gimp_channel_tree_view_set_context (GimpContainerView *view,
                                    GimpContext       *context)
{
  GimpChannelTreeView *channel_view = GIMP_CHANNEL_TREE_VIEW (view);

  parent_view_iface->set_context (view, context);

  if (channel_view->priv->component_editor)
    gimp_docked_set_context (GIMP_DOCKED (channel_view->priv->component_editor),
                             context);
}

static void
gimp_channel_tree_view_set_view_size (GimpContainerView *view)
{
  GimpChannelTreeView *channel_view = GIMP_CHANNEL_TREE_VIEW (view);
  gint                 view_size;

  parent_view_iface->set_view_size (view);

  view_size = gimp_container_view_get_view_size (view, NULL);

  if (channel_view->priv->component_editor)
    gimp_component_editor_set_view_size (GIMP_COMPONENT_EDITOR (channel_view->priv->component_editor),
                                         view_size);
}
