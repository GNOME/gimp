/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpchanneltreeview.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"

#include "gimpchanneltreeview.h"
#include "gimpcomponenteditor.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void   gimp_channel_tree_view_class_init (GimpChannelTreeViewClass *klass);
static void   gimp_channel_tree_view_init       (GimpChannelTreeView      *view);

static void   gimp_channel_tree_view_view_iface_init (GimpContainerViewInterface *view_iface);

static GObject * gimp_channel_tree_view_constructor   (GType              type,
                                                       guint              n_params,
                                                       GObjectConstructParam *params);
static void   gimp_channel_tree_view_set_image        (GimpItemTreeView  *item_view,
                                                       GimpImage         *gimage);

static void   gimp_channel_tree_view_set_preview_size (GimpContainerView *view);


static GimpDrawableTreeViewClass  *parent_class      = NULL;
static GimpContainerViewInterface *parent_view_iface = NULL;


GType
gimp_channel_tree_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpChannelTreeViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_channel_tree_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpChannelTreeView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_channel_tree_view_init,
      };

      static const GInterfaceInfo view_iface_info =
      {
        (GInterfaceInitFunc) gimp_channel_tree_view_view_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      view_type = g_type_register_static (GIMP_TYPE_DRAWABLE_TREE_VIEW,
                                          "GimpChannelTreeView",
                                          &view_info, 0);

      g_type_add_interface_static (view_type, GIMP_TYPE_CONTAINER_VIEW,
                                   &view_iface_info);
    }

  return view_type;
}

static void
gimp_channel_tree_view_class_init (GimpChannelTreeViewClass *klass)
{
  GObjectClass          *object_class    = G_OBJECT_CLASS (klass);
  GimpItemTreeViewClass *item_view_class = GIMP_ITEM_TREE_VIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor        = gimp_channel_tree_view_constructor;

  item_view_class->set_image       = gimp_channel_tree_view_set_image;

  item_view_class->get_container   = gimp_image_get_channels;
  item_view_class->get_active_item = (GimpGetItemFunc) gimp_image_get_active_channel;
  item_view_class->set_active_item = (GimpSetItemFunc) gimp_image_set_active_channel;
  item_view_class->reorder_item    = (GimpReorderItemFunc) gimp_image_position_channel;
  item_view_class->add_item        = (GimpAddItemFunc) gimp_image_add_channel;
  item_view_class->remove_item     = (GimpRemoveItemFunc) gimp_image_remove_channel;

  item_view_class->edit_desc               = _("Edit Channel Attributes");
  item_view_class->edit_help_id            = GIMP_HELP_CHANNEL_EDIT;
  item_view_class->new_desc                = _("New Channel\n%s New Channel Dialog");
  item_view_class->new_help_id             = GIMP_HELP_CHANNEL_NEW;
  item_view_class->duplicate_desc          = _("Duplicate Channel");
  item_view_class->duplicate_help_id       =  GIMP_HELP_CHANNEL_DUPLICATE;
  item_view_class->delete_desc             = _("Delete Channel");
  item_view_class->delete_help_id          = GIMP_HELP_CHANNEL_DELETE;
  item_view_class->raise_desc              = _("Raise Channel");
  item_view_class->raise_help_id           = GIMP_HELP_CHANNEL_RAISE;
  item_view_class->raise_to_top_desc       = _("Raise Channel to Top");
  item_view_class->raise_to_top_help_id    = GIMP_HELP_CHANNEL_RAISE_TO_TOP;
  item_view_class->lower_desc              = _("Lower Channel");
  item_view_class->lower_help_id           = GIMP_HELP_CHANNEL_LOWER;
  item_view_class->lower_to_bottom_desc    = _("Lower Channel to Bottom");
  item_view_class->lower_to_bottom_help_id = GIMP_HELP_CHANNEL_LOWER_TO_BOTTOM;
  item_view_class->reorder_desc            = _("Reorder Channel");
}

static void
gimp_channel_tree_view_init (GimpChannelTreeView *view)
{
}

static void
gimp_channel_tree_view_view_iface_init (GimpContainerViewInterface *view_iface)
{
  parent_view_iface = g_type_interface_peek_parent (view_iface);

  view_iface->set_preview_size = gimp_channel_tree_view_set_preview_size;
}

static GObject *
gimp_channel_tree_view_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
  GObject             *object;
  GimpEditor          *editor;
  GimpChannelTreeView *view;
  gchar               *str;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_EDITOR (object);
  view   = GIMP_CHANNEL_TREE_VIEW (object);

  str = g_strdup_printf (_("Channel to selection\n"
                           "%s  Add\n"
                           "%s  Subtract\n"
                           "%s%s%s  Intersect"),
                         gimp_get_mod_name_shift (),
                         gimp_get_mod_name_control (),
                         gimp_get_mod_name_shift (),
                         gimp_get_mod_separator (),
                         gimp_get_mod_name_control ());

  view->toselection_button =
    gimp_editor_add_action_button (GIMP_EDITOR (view), "channels",
                                   "channels-selection-replace",
                                   "channels-selection-intersect",
                                   GDK_SHIFT_MASK | GDK_CONTROL_MASK,
                                   "channels-selection-subtract",
                                   GDK_CONTROL_MASK,
                                   "channels-selection-add",
                                   GDK_SHIFT_MASK,
                                   NULL);

  gimp_help_set_help_data (view->toselection_button, str,
                           GIMP_HELP_CHANNEL_SELECTION_REPLACE);

  g_free (str);

  gtk_box_reorder_child (GTK_BOX (GIMP_EDITOR (view)->button_box),
			 view->toselection_button, 5);

  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (view),
				  GTK_BUTTON (view->toselection_button),
				  GIMP_TYPE_CHANNEL);

  return object;
}


/*  GimpItemTreeView methods  */

static void
gimp_channel_tree_view_set_image (GimpItemTreeView *item_view,
				  GimpImage        *gimage)
{
  GimpChannelTreeView *channel_view = GIMP_CHANNEL_TREE_VIEW (item_view);

  if (! channel_view->component_editor)
    {
      GimpContainerView *view = GIMP_CONTAINER_VIEW (item_view);
      gint               preview_size;

      preview_size = gimp_container_view_get_preview_size (view, NULL);

      channel_view->component_editor =
        gimp_component_editor_new (preview_size,
                                   GIMP_EDITOR (item_view)->menu_factory);
      gtk_box_pack_start (GTK_BOX (item_view), channel_view->component_editor,
                          FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (item_view),
                             channel_view->component_editor, 0);
    }

  if (! gimage)
    gtk_widget_hide (channel_view->component_editor);

  gimp_image_editor_set_image (GIMP_IMAGE_EDITOR (channel_view->component_editor),
                               gimage);

  GIMP_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (item_view, gimage);

  if (item_view->gimage)
    gtk_widget_show (channel_view->component_editor);
}


/*  GimpContainerView methods  */

static void
gimp_channel_tree_view_set_preview_size (GimpContainerView *view)
{
  GimpChannelTreeView *channel_view = GIMP_CHANNEL_TREE_VIEW (view);
  gint                 preview_size;

  parent_view_iface->set_preview_size (view);

  preview_size = gimp_container_view_get_preview_size (view, NULL);

  if (channel_view->component_editor)
    gimp_component_editor_set_preview_size (GIMP_COMPONENT_EDITOR (channel_view->component_editor),
                                            preview_size);
}
