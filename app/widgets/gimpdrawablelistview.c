/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawablelistview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-bucket-fill.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"
#include "core/gimpmarshal.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "gimpchannellistview.h"
#include "gimpdnd.h"
#include "gimpdrawablelistview.h"
#include "gimpitemfactory.h"
#include "gimplayerlistview.h"
#include "gimplistitem.h"
#include "gimppreview.h"

#include "libgimp/gimpintl.h"


static void   gimp_drawable_list_view_class_init (GimpDrawableListViewClass *klass);
static void   gimp_drawable_list_view_init       (GimpDrawableListView      *view);

static void   gimp_drawable_list_view_set_image  (GimpItemListView     *view,
                                                  GimpImage            *gimage);

static void   gimp_drawable_list_view_floating_selection_changed
                                                 (GimpImage            *gimage,
                                                  GimpDrawableListView *view);

static void   gimp_drawable_list_view_new_pattern_dropped
                                                 (GtkWidget            *widget,
                                                  GimpViewable         *viewable,
                                                  gpointer              data);
static void   gimp_drawable_list_view_new_color_dropped
                                                 (GtkWidget            *widget,
                                                  const GimpRGB        *color,
                                                  gpointer              data);


static GimpItemListViewClass *parent_class = NULL;


GType
gimp_drawable_list_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpDrawableListViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_drawable_list_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpDrawableListView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_drawable_list_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_ITEM_LIST_VIEW,
                                          "GimpDrawableListView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_drawable_list_view_class_init (GimpDrawableListViewClass *klass)
{
  GimpItemListViewClass *item_view_class;

  item_view_class = GIMP_ITEM_LIST_VIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  item_view_class->set_image = gimp_drawable_list_view_set_image;
}

static void
gimp_drawable_list_view_init (GimpDrawableListView *view)
{
  GimpItemListView *item_view;

  item_view = GIMP_ITEM_LIST_VIEW (view);

  gimp_dnd_viewable_dest_add (item_view->new_button, GIMP_TYPE_PATTERN,
			      gimp_drawable_list_view_new_pattern_dropped,
                              view);
  gimp_dnd_color_dest_add (item_view->new_button,
                           gimp_drawable_list_view_new_color_dropped,
                           view);
}

static void
gimp_drawable_list_view_set_image (GimpItemListView *item_view,
                                   GimpImage        *gimage)
{
  GimpDrawableListView *view;

  g_return_if_fail (GIMP_IS_DRAWABLE_LIST_VIEW (item_view));
  g_return_if_fail (! gimage || GIMP_IS_IMAGE (gimage));

  view = GIMP_DRAWABLE_LIST_VIEW (item_view);

  if (item_view->gimage)
    {
      g_signal_handlers_disconnect_by_func (item_view->gimage,
					    gimp_drawable_list_view_floating_selection_changed,
					    view);
    }

  GIMP_ITEM_LIST_VIEW_CLASS (parent_class)->set_image (item_view, gimage);

  if (item_view->gimage)
    {
      g_signal_connect (item_view->gimage,
                        "floating_selection_changed",
			G_CALLBACK (gimp_drawable_list_view_floating_selection_changed),
			view);

      if (gimp_image_floating_sel (item_view->gimage))
	gimp_drawable_list_view_floating_selection_changed (item_view->gimage,
                                                            view);
    }
}

static void
gimp_drawable_list_view_floating_selection_changed (GimpImage            *gimage,
						    GimpDrawableListView *view)
{
  GimpViewable *floating_sel;
  GList        *list;
  GList        *free_list;

  floating_sel = (GimpViewable *) gimp_image_floating_sel (gimage);

  list = free_list = gtk_container_get_children
    (GTK_CONTAINER (GIMP_CONTAINER_LIST_VIEW (view)->gtk_list));

  for (; list; list = g_list_next (list))
    {
      if (! (GIMP_PREVIEW (GIMP_LIST_ITEM (list->data)->preview)->viewable ==
	     floating_sel))
	{
	  gtk_widget_set_sensitive (GTK_WIDGET (list->data),
				    floating_sel == NULL);
	}
    }

  g_list_free (free_list);

  /*  update button states  */
  /* gimp_drawable_list_view_drawable_changed (gimage, view); */
}

static void
gimp_drawable_list_view_new_dropped (GimpItemListView   *view,
                                     GimpBucketFillMode  fill_mode,
                                     const GimpRGB      *color,
                                     GimpPattern        *pattern)
{
  GimpDrawable *drawable;
  GimpToolInfo *tool_info;
  GimpContext  *context;

  gimp_image_undo_group_start (view->gimage, GIMP_UNDO_GROUP_EDIT_PASTE,
                               _("New Layer"));

  view->new_item_func (view->gimage, NULL, FALSE);

  drawable = gimp_image_active_drawable (view->gimage);

  /*  Get the bucket fill context  */
  tool_info = (GimpToolInfo *)
    gimp_container_get_child_by_name (view->gimage->gimp->tool_info_list,
                                      "gimp-bucket-fill-tool");

  if (tool_info && tool_info->tool_options)
    {
      context = GIMP_CONTEXT (tool_info->tool_options);
    }
  else
    {
      context = gimp_get_user_context (view->gimage->gimp);
    }

  gimp_drawable_bucket_fill_full (drawable,
                                  fill_mode,
                                  color, pattern,
                                  gimp_context_get_paint_mode (context),
                                  gimp_context_get_opacity (context),
                                  FALSE /* no seed fill */,
                                  FALSE, 0.0, FALSE, 0.0, 0.0 /* fill params */);

  gimp_image_undo_group_end (view->gimage);

  gimp_image_flush (view->gimage);
}

static void
gimp_drawable_list_view_new_pattern_dropped (GtkWidget    *widget,
                                             GimpViewable *viewable,
                                             gpointer      data)
{
  gimp_drawable_list_view_new_dropped (GIMP_ITEM_LIST_VIEW (data),
                                       GIMP_PATTERN_BUCKET_FILL,
                                       NULL,
                                       GIMP_PATTERN (viewable));
}

static void
gimp_drawable_list_view_new_color_dropped (GtkWidget     *widget,
                                           const GimpRGB *color,
                                           gpointer       data)
{
  gimp_drawable_list_view_new_dropped (GIMP_ITEM_LIST_VIEW (data),
                                       GIMP_FG_BUCKET_FILL,
                                       color,
                                       NULL);
}
