/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawablelistitem.c
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

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "gimpdnd.h"
#include "gimpdrawablelistitem.h"
#include "gimpdrawablelistview.h"
#include "gimppreview.h"


static void   gimp_drawable_list_item_class_init (GimpDrawableListItemClass *klass);
static void   gimp_drawable_list_item_init       (GimpDrawableListItem      *list_item);

static void   gimp_drawable_list_item_set_viewable       (GimpListItem   *list_item,
                                                          GimpViewable   *viewable);

static void   gimp_drawable_list_item_eye_toggled        (GtkWidget      *widget,
                                                          gpointer        data);

static void   gimp_drawable_list_item_visibility_changed (GimpDrawable   *drawable,
                                                          gpointer        data);


static GimpListItemClass *parent_class = NULL;


GType
gimp_drawable_list_item_get_type (void)
{
  static GType list_item_type = 0;

  if (! list_item_type)
    {
      static const GTypeInfo list_item_info =
      {
        sizeof (GimpDrawableListItemClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_drawable_list_item_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpDrawableListItem),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_drawable_list_item_init,
      };

      list_item_type = g_type_register_static (GIMP_TYPE_ITEM_LIST_ITEM,
                                               "GimpDrawableListItem",
                                               &list_item_info, 0);
    }

  return list_item_type;
}

static void
gimp_drawable_list_item_class_init (GimpDrawableListItemClass *klass)
{
  GimpListItemClass *list_item_class;

  list_item_class = GIMP_LIST_ITEM_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  list_item_class->set_viewable = gimp_drawable_list_item_set_viewable;
}

static void
gimp_drawable_list_item_init (GimpDrawableListItem *list_item)
{
  GtkWidget *abox;
  GtkWidget *image;

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (GIMP_LIST_ITEM (list_item)->hbox), abox,
                      FALSE, FALSE, 0);
  gtk_widget_show (abox);

  list_item->eye_button = gtk_toggle_button_new ();
  gtk_button_set_relief (GTK_BUTTON (list_item->eye_button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (abox), list_item->eye_button);
  gtk_widget_show (list_item->eye_button);

  g_signal_connect (list_item->eye_button, "realize",
		    G_CALLBACK (gimp_list_item_button_realize),
		    list_item);

  g_signal_connect (list_item->eye_button, "state_changed",
		    G_CALLBACK (gimp_list_item_button_state_changed),
		    list_item);

  image = gtk_image_new_from_stock (GIMP_STOCK_VISIBLE,
				    GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (list_item->eye_button), image);
  gtk_widget_show (image);
}

static void
gimp_drawable_list_item_set_viewable (GimpListItem *list_item,
                                      GimpViewable *viewable)
{
  GimpDrawableListItem *drawable_item;
  GimpDrawable         *drawable;
  gboolean              visible;

  if (GIMP_LIST_ITEM_CLASS (parent_class)->set_viewable)
    GIMP_LIST_ITEM_CLASS (parent_class)->set_viewable (list_item, viewable);

  GIMP_PREVIEW (list_item->preview)->clickable         = TRUE;
  GIMP_PREVIEW (list_item->preview)->eat_button_events = FALSE;

  drawable_item = GIMP_DRAWABLE_LIST_ITEM (list_item);
  drawable      = GIMP_DRAWABLE (GIMP_PREVIEW (list_item->preview)->viewable);
  visible       = gimp_drawable_get_visible (drawable);

  if (! visible)
    {
      GtkRequisition requisition;

      gtk_widget_size_request (drawable_item->eye_button, &requisition);

      gtk_widget_set_size_request (drawable_item->eye_button,
                                   requisition.width,
                                   requisition.height);
      gtk_widget_hide (GTK_BIN (drawable_item->eye_button)->child);
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (drawable_item->eye_button),
                                visible);

  g_signal_connect (drawable_item->eye_button, "toggled",
		    G_CALLBACK (gimp_drawable_list_item_eye_toggled),
		    list_item);

  g_signal_connect_object (viewable, "visibility_changed",
			   G_CALLBACK (gimp_drawable_list_item_visibility_changed),
			   list_item,
			   0);
}

static void
gimp_drawable_list_item_eye_toggled (GtkWidget *widget,
                                     gpointer   data)
{
  GimpListItem *list_item;
  GimpDrawable *drawable;
  gboolean      visible;

  list_item = GIMP_LIST_ITEM (data);
  drawable  = GIMP_DRAWABLE (GIMP_PREVIEW (list_item->preview)->viewable);
  visible   = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  if (visible != gimp_drawable_get_visible (drawable))
    {
      if (! visible)
        {
          gtk_widget_set_size_request (GTK_WIDGET (widget),
                                       GTK_WIDGET (widget)->allocation.width,
                                       GTK_WIDGET (widget)->allocation.height);
          gtk_widget_hide (GTK_BIN (widget)->child);
        }
      else
        {
          gtk_widget_show (GTK_BIN (widget)->child);
          gtk_widget_set_size_request (GTK_WIDGET (widget), -1, -1);
        }

      g_signal_handlers_block_by_func (drawable,
				       gimp_drawable_list_item_visibility_changed,
				       list_item);

      gimp_drawable_set_visible (drawable, visible);

      g_signal_handlers_unblock_by_func (drawable,
					 gimp_drawable_list_item_visibility_changed,
					 list_item);

      gimp_image_flush (gimp_item_get_image (GIMP_ITEM (drawable)));
    }
}

static void
gimp_drawable_list_item_visibility_changed (GimpDrawable *drawable,
                                            gpointer      data)
{
  GimpListItem    *list_item;
  GtkToggleButton *toggle;
  gboolean         visible;

  list_item = GIMP_LIST_ITEM (data);
  toggle    = GTK_TOGGLE_BUTTON (GIMP_DRAWABLE_LIST_ITEM (data)->eye_button);
  visible   = gimp_drawable_get_visible (drawable);

  if (visible != toggle->active)
    {
      if (! visible)
        {
          gtk_widget_set_size_request (GTK_WIDGET (toggle),
                                       GTK_WIDGET (toggle)->allocation.width,
                                       GTK_WIDGET (toggle)->allocation.height);
          gtk_widget_hide (GTK_BIN (toggle)->child);
        }
      else
        {
          gtk_widget_show (GTK_BIN (toggle)->child);
          gtk_widget_set_size_request (GTK_WIDGET (toggle), -1, -1);
        }

      g_signal_handlers_block_by_func (toggle,
				       gimp_drawable_list_item_eye_toggled,
				       list_item);

      gtk_toggle_button_set_active (toggle, visible);

      g_signal_handlers_unblock_by_func (toggle,
					 gimp_drawable_list_item_eye_toggled,
					 list_item);
    }
}
