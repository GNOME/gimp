/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplayerlistitem.c
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

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "drawable.h"
#include "gdisplay.h"
#include "gimpchannel.h"
#include "gimpcontainer.h"
#include "gimpdnd.h"
#include "gimplayerlistitem.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimppreview.h"
#include "gimpviewable.h"


static void   gimp_layer_list_item_class_init (GimpLayerListItemClass  *klass);
static void   gimp_layer_list_item_init       (GimpLayerListItem       *list_item);

static void       gimp_layer_list_item_set_viewable (GimpListItem      *list_item,
                                                     GimpViewable      *viewable);

static gboolean   gimp_layer_list_item_drag_motion  (GtkWidget         *widget,
                                                     GdkDragContext    *context,
                                                     gint               x,
                                                     gint               y,
                                                     guint              time);
static gboolean   gimp_layer_list_item_drag_drop    (GtkWidget         *widget,
                                                     GdkDragContext    *context,
                                                     gint               x,
                                                     gint               y,
                                                     guint              time);
static void       gimp_layer_list_item_mask_changed (GimpLayer         *layer,
                                                     GimpLayerListItem *layer_item);


static GimpDrawableListItemClass *parent_class = NULL;


GtkType
gimp_layer_list_item_get_type (void)
{
  static GtkType list_item_type = 0;

  if (! list_item_type)
    {
      static const GtkTypeInfo list_item_info =
      {
	"GimpLayerListItem",
	sizeof (GimpLayerListItem),
	sizeof (GimpLayerListItemClass),
	(GtkClassInitFunc) gimp_layer_list_item_class_init,
	(GtkObjectInitFunc) gimp_layer_list_item_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      list_item_type = gtk_type_unique (GIMP_TYPE_DRAWABLE_LIST_ITEM,
                                        &list_item_info);
    }

  return list_item_type;
}

static void
gimp_layer_list_item_class_init (GimpLayerListItemClass *klass)
{
  GtkObjectClass    *object_class;
  GtkWidgetClass    *widget_class;
  GimpListItemClass *list_item_class;

  object_class    = (GtkObjectClass *) klass;
  widget_class    = (GtkWidgetClass *) klass;
  list_item_class = (GimpListItemClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_DRAWABLE_LIST_ITEM);

  widget_class->drag_motion     = gimp_layer_list_item_drag_motion;
  widget_class->drag_drop       = gimp_layer_list_item_drag_drop;

  list_item_class->set_viewable = gimp_layer_list_item_set_viewable;
}

static void
gimp_layer_list_item_init (GimpLayerListItem *list_item)
{
  list_item->mask_preview = NULL;
}

static void
gimp_layer_list_item_set_viewable (GimpListItem *list_item,
                                   GimpViewable *viewable)
{
  GimpLayerListItem *layer_item;
  GimpLayer         *layer;

  if (GIMP_LIST_ITEM_CLASS (parent_class)->set_viewable)
    GIMP_LIST_ITEM_CLASS (parent_class)->set_viewable (list_item, viewable);

  layer_item = GIMP_LAYER_LIST_ITEM (list_item);
  layer      = GIMP_LAYER (GIMP_PREVIEW (list_item->preview)->viewable);

  if (gimp_layer_get_mask (layer))
    {
      gimp_layer_list_item_mask_changed (layer, layer_item);
    }

  gtk_signal_connect (GTK_OBJECT (layer), "mask_changed",
                      GTK_SIGNAL_FUNC (gimp_layer_list_item_mask_changed),
                      list_item);
}

static gboolean
gimp_layer_list_item_drag_motion (GtkWidget      *widget,
                                  GdkDragContext *context,
                                  gint            x,
                                  gint            y,
                                  guint           time)
{
  GimpListItem  *list_item;
  GimpLayer     *layer;
  GimpViewable  *src_viewable;
  gint           dest_index;
  GdkDragAction  drag_action;
  GimpDropType   drop_type;
  gboolean       return_val;

  list_item = GIMP_LIST_ITEM (widget);
  layer     = GIMP_LAYER (GIMP_PREVIEW (list_item->preview)->viewable);

  return_val = gimp_list_item_check_drag (list_item, context, x, y,
                                          &src_viewable,
                                          &dest_index,
                                          &drag_action,
                                          &drop_type);

  if (! src_viewable                                           ||
      ! gimp_drawable_has_alpha (GIMP_DRAWABLE (src_viewable)) ||
      ! layer                                                  ||
      ! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    {
      drag_action = GDK_ACTION_DEFAULT;
      drop_type   = GIMP_DROP_NONE;
      return_val  = FALSE;
    }

  gdk_drag_status (context, drag_action, time);

  list_item->drop_type = drop_type;

  return return_val;
}

static gboolean
gimp_layer_list_item_drag_drop (GtkWidget      *widget,
                                GdkDragContext *context,
                                gint            x,
                                gint            y,
                                guint           time)
{
  GimpListItem  *list_item;
  GimpLayer     *layer;
  GimpViewable  *src_viewable;
  gint           dest_index;
  GdkDragAction  drag_action;
  GimpDropType   drop_type;
  gboolean       return_val;

  list_item = GIMP_LIST_ITEM (widget);
  layer     = GIMP_LAYER (GIMP_PREVIEW (list_item->preview)->viewable);

  return_val = gimp_list_item_check_drag (list_item, context, x, y,
                                          &src_viewable,
                                          &dest_index,
                                          &drag_action,
                                          &drop_type);

  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (src_viewable)) ||
      ! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    {
      drag_action = GDK_ACTION_DEFAULT;
      drop_type   = GIMP_DROP_NONE;
      return_val  = FALSE;
    }

  gtk_drag_finish (context, return_val, FALSE, time);

  list_item->drop_type = GIMP_DROP_NONE;

  if (return_val)
    {
      gimp_image_position_layer (gimp_drawable_gimage (GIMP_DRAWABLE (src_viewable)),
                                 GIMP_LAYER (src_viewable),
                                 dest_index,
                                 TRUE);
      gdisplays_flush ();
    }

  return return_val;
}

static void
gimp_layer_list_item_mask_changed (GimpLayer         *layer,
                                   GimpLayerListItem *layer_item)
{
  GimpListItem  *list_item;
  GimpLayerMask *mask;

  list_item = GIMP_LIST_ITEM (layer_item);
  mask      = gimp_layer_get_mask (layer);

  if (mask && ! layer_item->mask_preview)
    {
      layer_item->mask_preview = gimp_preview_new (GIMP_VIEWABLE (mask),
                                                   list_item->preview_size,
                                                   1, FALSE);
      GIMP_PREVIEW (layer_item->mask_preview)->clickable = TRUE;
      gtk_box_pack_start (GTK_BOX (list_item->hbox), layer_item->mask_preview,
                          FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (list_item->hbox),
                             layer_item->mask_preview, 2);
      gtk_widget_show (layer_item->mask_preview);
    }
  else if (! mask && layer_item->mask_preview)
    {
      gtk_widget_destroy (layer_item->mask_preview);

      layer_item->mask_preview = NULL;
    }
}
