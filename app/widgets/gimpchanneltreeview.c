/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpchanneltreeview.c
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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"

#include "gimpchanneltreeview.h"
#include "gimpcomponenteditor.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void   gimp_channel_tree_view_class_init (GimpChannelTreeViewClass *klass);
static void   gimp_channel_tree_view_init       (GimpChannelTreeView      *view);

static void   gimp_channel_tree_view_set_image      (GimpItemTreeView     *item_view,
						     GimpImage            *gimage);

static gboolean  gimp_channel_tree_view_select_item (GimpContainerView   *view,
						     GimpViewable        *item,
						     gpointer             insert_data);
static void   gimp_channel_tree_view_set_preview_size (GimpContainerView *view);

static void   gimp_channel_tree_view_toselection_clicked
                                                    (GtkWidget           *widget,
						     GimpChannelTreeView *view);
static void   gimp_channel_tree_view_toselection_extended_clicked
                                                    (GtkWidget           *widget,
						     guint                state,
						     GimpChannelTreeView *view);


static GimpDrawableTreeViewClass *parent_class = NULL;


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

      view_type = g_type_register_static (GIMP_TYPE_DRAWABLE_TREE_VIEW,
                                          "GimpChannelTreeView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_channel_tree_view_class_init (GimpChannelTreeViewClass *klass)
{
  GimpContainerViewClass *container_view_class;
  GimpItemTreeViewClass  *item_view_class;

  container_view_class = GIMP_CONTAINER_VIEW_CLASS (klass);
  item_view_class      = GIMP_ITEM_TREE_VIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  container_view_class->select_item      = gimp_channel_tree_view_select_item;
  container_view_class->set_preview_size = gimp_channel_tree_view_set_preview_size;

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
  gchar *str;

  str = g_strdup_printf (_("Channel to Selection\n"
                           "%s  Add\n"
                           "%s  Subtract\n"
                           "%s%s%s  Intersect"),
                         gimp_get_mod_name_shift (),
                         gimp_get_mod_name_control (),
                         gimp_get_mod_name_shift (),
                         gimp_get_mod_separator (),
                         gimp_get_mod_name_control ());

  /*  To Selection button  */
  view->toselection_button =
    gimp_editor_add_button (GIMP_EDITOR (view),
                            GIMP_STOCK_SELECTION_REPLACE, str,
                            GIMP_HELP_CHANNEL_SELECTION_REPLACE,
                            G_CALLBACK (gimp_channel_tree_view_toselection_clicked),
                            G_CALLBACK (gimp_channel_tree_view_toselection_extended_clicked),
                            view);

  g_free (str);

  gtk_box_reorder_child (GTK_BOX (GIMP_EDITOR (view)->button_box),
			 view->toselection_button, 5);

  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (view),
				  GTK_BUTTON (view->toselection_button),
				  GIMP_TYPE_CHANNEL);

  gtk_widget_set_sensitive (view->toselection_button, FALSE);
}


/*  GimpItemTreeView methods  */

static void
gimp_channel_tree_view_set_image (GimpItemTreeView *item_view,
				  GimpImage        *gimage)
{
  GimpChannelTreeView *channel_view;

  channel_view = GIMP_CHANNEL_TREE_VIEW (item_view);

  if (! channel_view->component_editor)
    {
      channel_view->component_editor =
        gimp_component_editor_new (GIMP_CONTAINER_VIEW (item_view)->preview_size,
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

static gboolean
gimp_channel_tree_view_select_item (GimpContainerView *view,
				    GimpViewable      *item,
				    gpointer           insert_data)
{
  GimpItemTreeView    *item_view;
  GimpChannelTreeView *tree_view;
  gboolean             success;

  item_view = GIMP_ITEM_TREE_VIEW (view);
  tree_view = GIMP_CHANNEL_TREE_VIEW (view);

  success = GIMP_CONTAINER_VIEW_CLASS (parent_class)->select_item (view, item,
                                                                   insert_data);

  if (item_view->gimage)
    {
      gboolean floating_sel;

      floating_sel = (gimp_image_floating_sel (item_view->gimage) != NULL);

      gtk_widget_set_sensitive (GIMP_EDITOR (view)->button_box, ! floating_sel);
    }

  gtk_widget_set_sensitive (tree_view->toselection_button,
                            success && item != NULL);

  return success;
}

static void
gimp_channel_tree_view_set_preview_size (GimpContainerView *view)
{
  GimpChannelTreeView *channel_view;

  channel_view = GIMP_CHANNEL_TREE_VIEW (view);

  GIMP_CONTAINER_VIEW_CLASS (parent_class)->set_preview_size (view);

  if (channel_view->component_editor)
    gimp_component_editor_set_preview_size (GIMP_COMPONENT_EDITOR (channel_view->component_editor),
                                            view->preview_size);
}

static void
gimp_channel_tree_view_toselection_clicked (GtkWidget           *widget,
					    GimpChannelTreeView *view)
{
  gimp_channel_tree_view_toselection_extended_clicked (widget, 0, view);
}

static void
gimp_channel_tree_view_toselection_extended_clicked (GtkWidget           *widget,
						     guint                state,
						     GimpChannelTreeView *view)
{
  GimpImage *gimage;
  GimpItem  *item;

  gimage = GIMP_ITEM_TREE_VIEW (view)->gimage;

  item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (gimage);

  if (item)
    {
      GimpChannelOps operation = GIMP_CHANNEL_OP_REPLACE;

      if (state & GDK_SHIFT_MASK)
	{
	  if (state & GDK_CONTROL_MASK)
	    operation = GIMP_CHANNEL_OP_INTERSECT;
	  else
	    operation = GIMP_CHANNEL_OP_ADD;
	}
      else if (state & GDK_CONTROL_MASK)
	{
	  operation = GIMP_CHANNEL_OP_SUBTRACT;
	}

      gimp_channel_select_channel (gimp_image_get_mask (gimage),
                                   _("Channel to Selection"),
                                   GIMP_CHANNEL (item),
                                   0, 0,
                                   operation,
                                   FALSE, 0.0, 0.0);
      gimp_image_flush (gimage);
    }
}
