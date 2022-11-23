/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmachanneltreeview.c
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmachannel.h"
#include "core/ligmacontainer.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmalayer.h"
#include "core/ligmalayermask.h"

#include "ligmaactiongroup.h"
#include "ligmachanneltreeview.h"
#include "ligmacomponenteditor.h"
#include "ligmacontainerview.h"
#include "ligmadnd.h"
#include "ligmadocked.h"
#include "ligmahelp-ids.h"
#include "ligmauimanager.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


struct _LigmaChannelTreeViewPrivate
{
  GtkWidget *component_editor;

  GtkWidget *toselection_button;
};


static void  ligma_channel_tree_view_view_iface_init   (LigmaContainerViewInterface *iface);

static void   ligma_channel_tree_view_constructed      (GObject                 *object);

static void   ligma_channel_tree_view_drop_viewables   (LigmaContainerTreeView   *view,
                                                       GList                   *src_viewables,
                                                       LigmaViewable            *dest_viewable,
                                                       GtkTreeViewDropPosition  drop_pos);
static void   ligma_channel_tree_view_drop_component   (LigmaContainerTreeView   *tree_view,
                                                       LigmaImage               *image,
                                                       LigmaChannelType          component,
                                                       LigmaViewable            *dest_viewable,
                                                       GtkTreeViewDropPosition  drop_pos);
static void   ligma_channel_tree_view_set_image        (LigmaItemTreeView        *item_view,
                                                       LigmaImage               *image);
static LigmaItem * ligma_channel_tree_view_item_new     (LigmaImage               *image);

static void   ligma_channel_tree_view_set_context      (LigmaContainerView       *view,
                                                       LigmaContext             *context);
static void   ligma_channel_tree_view_set_view_size    (LigmaContainerView       *view);


G_DEFINE_TYPE_WITH_CODE (LigmaChannelTreeView, ligma_channel_tree_view,
                         LIGMA_TYPE_DRAWABLE_TREE_VIEW,
                         G_ADD_PRIVATE (LigmaChannelTreeView)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONTAINER_VIEW,
                                                ligma_channel_tree_view_view_iface_init))

#define parent_class ligma_channel_tree_view_parent_class

static LigmaContainerViewInterface *parent_view_iface = NULL;


static void
ligma_channel_tree_view_class_init (LigmaChannelTreeViewClass *klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  LigmaContainerTreeViewClass *view_class   = LIGMA_CONTAINER_TREE_VIEW_CLASS (klass);
  LigmaItemTreeViewClass      *iv_class     = LIGMA_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructed  = ligma_channel_tree_view_constructed;

  view_class->drop_viewables = ligma_channel_tree_view_drop_viewables;
  view_class->drop_component = ligma_channel_tree_view_drop_component;

  iv_class->set_image        = ligma_channel_tree_view_set_image;

  iv_class->item_type        = LIGMA_TYPE_CHANNEL;
  iv_class->signal_name      = "selected-channels-changed";

  iv_class->get_container    = ligma_image_get_channels;
  iv_class->get_selected_items = (LigmaGetItemsFunc) ligma_image_get_selected_channels;
  iv_class->set_selected_items = (LigmaSetItemsFunc) ligma_image_set_selected_channels;
  iv_class->add_item         = (LigmaAddItemFunc) ligma_image_add_channel;
  iv_class->remove_item      = (LigmaRemoveItemFunc) ligma_image_remove_channel;
  iv_class->new_item         = ligma_channel_tree_view_item_new;

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
  iv_class->lock_content_help_id  = LIGMA_HELP_CHANNEL_LOCK_PIXELS;
  iv_class->lock_position_help_id = LIGMA_HELP_CHANNEL_LOCK_POSITION;
}

static void
ligma_channel_tree_view_view_iface_init (LigmaContainerViewInterface *view_iface)
{
  parent_view_iface = g_type_interface_peek_parent (view_iface);

  view_iface->set_context   = ligma_channel_tree_view_set_context;
  view_iface->set_view_size = ligma_channel_tree_view_set_view_size;
}

static void
ligma_channel_tree_view_init (LigmaChannelTreeView *view)
{
  view->priv = ligma_channel_tree_view_get_instance_private (view);

  view->priv->component_editor   = NULL;
  view->priv->toselection_button = NULL;
}

static void
ligma_channel_tree_view_constructed (GObject *object)
{
  LigmaChannelTreeView   *view      = LIGMA_CHANNEL_TREE_VIEW (object);
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (object);
  GdkModifierType        extend_mask;
  GdkModifierType        modify_mask;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  extend_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_EXTEND_SELECTION);
  modify_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_MODIFY_SELECTION);

  ligma_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), LIGMA_TYPE_LAYER,
                               NULL, tree_view);
  ligma_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), LIGMA_TYPE_LAYER_MASK,
                               NULL, tree_view);
  ligma_dnd_component_dest_add (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);

  view->priv->toselection_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (view), "channels",
                                   "channels-selection-replace",
                                   "channels-selection-add",
                                   extend_mask,
                                   "channels-selection-subtract",
                                   modify_mask,
                                   "channels-selection-intersect",
                                   extend_mask | modify_mask,
                                   NULL);
  ligma_container_view_enable_dnd (LIGMA_CONTAINER_VIEW (view),
                                  GTK_BUTTON (view->priv->toselection_button),
                                  LIGMA_TYPE_CHANNEL);
  gtk_box_reorder_child (ligma_editor_get_button_box (LIGMA_EDITOR (view)),
                         view->priv->toselection_button, 4);
}


/*  LigmaContainerTreeView methods  */

static void
ligma_channel_tree_view_drop_viewables (LigmaContainerTreeView   *tree_view,
                                       GList                   *src_viewables,
                                       LigmaViewable            *dest_viewable,
                                       GtkTreeViewDropPosition  drop_pos)
{
  LigmaItemTreeView      *item_view = LIGMA_ITEM_TREE_VIEW (tree_view);
  LigmaImage             *image     = ligma_item_tree_view_get_image (item_view);
  LigmaItemTreeViewClass *item_view_class;
  GList                 *iter;

  item_view_class = LIGMA_ITEM_TREE_VIEW_GET_CLASS (item_view);

  for (iter = src_viewables; iter; iter = iter->next)
    {
      LigmaViewable *src_viewable = iter->data;

      if (LIGMA_IS_DRAWABLE (src_viewable) &&
          (image != ligma_item_get_image (LIGMA_ITEM (src_viewable)) ||
           G_TYPE_FROM_INSTANCE (src_viewable) != item_view_class->item_type))
        {
          LigmaItem *new_item;
          LigmaItem *parent;
          gint      index;

          index = ligma_item_tree_view_get_drop_index (item_view, dest_viewable,
                                                      drop_pos,
                                                      (LigmaViewable **) &parent);

          new_item = ligma_item_convert (LIGMA_ITEM (src_viewable),
                                        ligma_item_tree_view_get_image (item_view),
                                        item_view_class->item_type);

          item_view_class->add_item (image, new_item, parent, index, TRUE);

          ligma_image_flush (image);

          return;
        }
    }

  LIGMA_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_viewables (tree_view,
                                                                 src_viewables,
                                                                 dest_viewable,
                                                                 drop_pos);
}

static void
ligma_channel_tree_view_drop_component (LigmaContainerTreeView   *tree_view,
                                       LigmaImage               *src_image,
                                       LigmaChannelType          component,
                                       LigmaViewable            *dest_viewable,
                                       GtkTreeViewDropPosition  drop_pos)
{
  LigmaItemTreeView *item_view = LIGMA_ITEM_TREE_VIEW (tree_view);
  LigmaImage        *image     = ligma_item_tree_view_get_image (item_view);
  LigmaItem         *new_item;
  LigmaChannel      *parent;
  gint              index;
  const gchar      *desc;
  gchar            *name;

  index = ligma_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (LigmaViewable **) &parent);

  ligma_enum_get_value (LIGMA_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);
  name = g_strdup_printf (_("%s Channel Copy"), desc);

  new_item = LIGMA_ITEM (ligma_channel_new_from_component (src_image, component,
                                                         name, NULL));

  /*  copied components are invisible by default so subsequent copies
   *  of components don't affect each other
   */
  ligma_item_set_visible (new_item, FALSE, FALSE);

  g_free (name);

  if (src_image != image)
    LIGMA_ITEM_GET_CLASS (new_item)->convert (new_item, image,
                                             LIGMA_TYPE_CHANNEL);

  ligma_image_add_channel (image, LIGMA_CHANNEL (new_item), parent, index, TRUE);

  ligma_image_flush (image);
}


/*  LigmaItemTreeView methods  */

static void
ligma_channel_tree_view_set_image (LigmaItemTreeView *item_view,
                                  LigmaImage        *image)
{
  LigmaChannelTreeView *channel_view = LIGMA_CHANNEL_TREE_VIEW (item_view);

  if (! channel_view->priv->component_editor)
    {
      LigmaContainerView *view = LIGMA_CONTAINER_VIEW (item_view);
      gint               view_size;

      view_size = ligma_container_view_get_view_size (view, NULL);

      channel_view->priv->component_editor =
        ligma_component_editor_new (view_size,
                                   ligma_editor_get_menu_factory (LIGMA_EDITOR (item_view)));
      ligma_docked_set_context (LIGMA_DOCKED (channel_view->priv->component_editor),
                               ligma_container_view_get_context (view));
      gtk_box_pack_start (GTK_BOX (item_view), channel_view->priv->component_editor,
                          FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (item_view),
                             channel_view->priv->component_editor, 0);
    }

  if (! image)
    gtk_widget_hide (channel_view->priv->component_editor);

  ligma_image_editor_set_image (LIGMA_IMAGE_EDITOR (channel_view->priv->component_editor),
                               image);

  LIGMA_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (item_view, image);

  if (ligma_item_tree_view_get_image (item_view))
    gtk_widget_show (channel_view->priv->component_editor);
}

static LigmaItem *
ligma_channel_tree_view_item_new (LigmaImage *image)
{
  LigmaChannel *new_channel;
  LigmaRGB      color;

  ligma_rgba_set (&color, 0.0, 0.0, 0.0, 0.5);

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_EDIT_PASTE,
                               _("New Channel"));

  new_channel = ligma_channel_new (image,
                                  ligma_image_get_width (image),
                                  ligma_image_get_height (image),
                                  _("Channel"), &color);

  ligma_image_add_channel (image, new_channel,
                          LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);

  ligma_image_undo_group_end (image);

  return LIGMA_ITEM (new_channel);
}


/*  LigmaContainerView methods  */

static void
ligma_channel_tree_view_set_context (LigmaContainerView *view,
                                    LigmaContext       *context)
{
  LigmaChannelTreeView *channel_view = LIGMA_CHANNEL_TREE_VIEW (view);

  parent_view_iface->set_context (view, context);

  if (channel_view->priv->component_editor)
    ligma_docked_set_context (LIGMA_DOCKED (channel_view->priv->component_editor),
                             context);
}

static void
ligma_channel_tree_view_set_view_size (LigmaContainerView *view)
{
  LigmaChannelTreeView *channel_view = LIGMA_CHANNEL_TREE_VIEW (view);
  gint                 view_size;

  parent_view_iface->set_view_size (view);

  view_size = ligma_container_view_get_view_size (view, NULL);

  if (channel_view->priv->component_editor)
    ligma_component_editor_set_view_size (LIGMA_COMPONENT_EDITOR (channel_view->priv->component_editor),
                                         view_size);
}
