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

#include "libgimpcolor/gimpcolor.h"
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
#include "gimplayermask.h"
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
static void      gimp_layer_list_item_state_changed (GtkWidget         *widget,
                                                     GtkStateType       old_state);

static void       gimp_layer_list_item_mask_changed (GimpLayer         *layer,
                                                     GimpLayerListItem *layer_item);
static void       gimp_layer_list_item_update_state (GtkWidget         *widget);

static void      gimp_layer_list_item_layer_clicked (GtkWidget         *widget,
                                                     GimpLayer         *layer);
static void       gimp_layer_list_item_mask_clicked (GtkWidget         *widget,
                                                     GimpLayerMask     *mask);


static GimpDrawableListItemClass *parent_class = NULL;

static GimpRGB  black_color;
static GimpRGB  white_color;
static GimpRGB  green_color;
static GimpRGB  red_color;


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
  widget_class->state_changed   = gimp_layer_list_item_state_changed;

  list_item_class->set_viewable = gimp_layer_list_item_set_viewable;

  gimp_rgba_set (&black_color, 0.0, 0.0, 0.0, 1.0);
  gimp_rgba_set (&white_color, 1.0, 1.0, 1.0, 1.0);
  gimp_rgba_set (&green_color, 0.0, 1.0, 0.0, 1.0);
  gimp_rgba_set (&red_color,   1.0, 0.0, 0.0, 1.0);
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

  gimp_preview_set_size (GIMP_PREVIEW (list_item->preview),
                         list_item->preview_size, 2);

  layer_item = GIMP_LAYER_LIST_ITEM (list_item);
  layer      = GIMP_LAYER (GIMP_PREVIEW (list_item->preview)->viewable);

  gtk_signal_connect (GTK_OBJECT (list_item->preview), "clicked",
                      GTK_SIGNAL_FUNC (gimp_layer_list_item_layer_clicked),
                      layer);

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
gimp_layer_list_item_state_changed (GtkWidget    *widget,
                                    GtkStateType  old_state)
{
  if (GTK_WIDGET_CLASS (parent_class)->state_changed)
    GTK_WIDGET_CLASS (parent_class)->state_changed (widget, old_state);

  gimp_layer_list_item_update_state (widget);
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
                                                   2, FALSE);

      GIMP_PREVIEW (layer_item->mask_preview)->clickable = TRUE;

      gtk_box_pack_start (GTK_BOX (list_item->hbox), layer_item->mask_preview,
                          FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (list_item->hbox),
                             layer_item->mask_preview, 2);
      gtk_widget_show (layer_item->mask_preview);

      gtk_signal_connect (GTK_OBJECT (layer_item->mask_preview), "clicked",
                          GTK_SIGNAL_FUNC (gimp_layer_list_item_mask_clicked),
                          mask);

      gtk_signal_connect_object
        (GTK_OBJECT (mask), "apply_changed",
         GTK_SIGNAL_FUNC (gimp_layer_list_item_update_state),
         GTK_OBJECT (layer_item));

      gtk_signal_connect_object_while_alive
        (GTK_OBJECT (mask), "edit_changed",
         GTK_SIGNAL_FUNC (gimp_layer_list_item_update_state),
         GTK_OBJECT (layer_item));

      gtk_signal_connect_object_while_alive
        (GTK_OBJECT (mask), "show_changed",
         GTK_SIGNAL_FUNC (gimp_layer_list_item_update_state),
         GTK_OBJECT (layer_item));
    }
  else if (! mask && layer_item->mask_preview)
    {
      gtk_widget_destroy (layer_item->mask_preview);
      layer_item->mask_preview = NULL;
    }

  gimp_layer_list_item_update_state (GTK_WIDGET (layer_item));
}

static void
gimp_layer_list_item_update_state (GtkWidget *widget)
{
  GimpLayerListItem *layer_item;
  GimpListItem      *list_item;
  GimpLayer         *layer;
  GimpLayerMask     *mask;
  GimpPreview       *preview;
  GimpRGB           *layer_color = &black_color;
  GimpRGB           *mask_color  = &black_color;

  layer_item = GIMP_LAYER_LIST_ITEM (widget);
  list_item  = GIMP_LIST_ITEM (widget);
  layer      = GIMP_LAYER (GIMP_PREVIEW (list_item->preview)->viewable);
  mask       = gimp_layer_get_mask (layer);

  switch (widget->state)
    {
    case GTK_STATE_NORMAL:
      break;

    case GTK_STATE_SELECTED:
      if (! mask || (mask && ! gimp_layer_mask_get_edit (mask)))
        {
          layer_color = &white_color;
        }
      else
        {
          mask_color = &white_color;
        }
      break;

    default:
      g_print ("%s(): unhandled state\n", G_GNUC_FUNCTION);
    }

  preview = GIMP_PREVIEW (list_item->preview);

  gimp_preview_set_border_color (preview, layer_color);

  if (mask)
    {
      preview = GIMP_PREVIEW (layer_item->mask_preview);

      gimp_preview_set_border_color (preview, mask_color);
    }
}

static void
gimp_layer_list_item_layer_clicked (GtkWidget *widget,
                                    GimpLayer *layer)
{
  GimpLayerMask *mask;

  g_print ("layer clicked\n");

  mask = gimp_layer_get_mask (layer);

  if (mask)
    {
      if (gimp_layer_mask_get_edit (mask))
        gimp_layer_mask_set_edit (mask, FALSE);
    }
}

static void
gimp_layer_list_item_mask_clicked (GtkWidget     *widget,
                                   GimpLayerMask *mask)
{
  g_print ("mask clicked\n");

  if (! gimp_layer_mask_get_edit (mask))
    gimp_layer_mask_set_edit (mask, TRUE);
}
