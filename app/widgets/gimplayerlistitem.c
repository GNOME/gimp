/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplayerlistitem.c
 * Copyright (C) 2001 Michael Natterer
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

#include "widgets-types.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"

#include "gimpdnd.h"
#include "gimplayerlistitem.h"
#include "gimppreview.h"

#include "gdisplay.h"

#include "pixmaps/linked.xpm"


static void   gimp_layer_list_item_class_init (GimpLayerListItemClass  *klass);
static void   gimp_layer_list_item_init       (GimpLayerListItem       *list_item);

static void      gimp_layer_list_item_set_viewable  (GimpListItem      *list_item,
                                                     GimpViewable      *viewable);
static void      gimp_layer_list_item_set_preview_size (GimpListItem   *list_item);

static gboolean  gimp_layer_list_item_drag_motion   (GtkWidget         *widget,
                                                     GdkDragContext    *context,
                                                     gint               x,
                                                     gint               y,
                                                     guint              time);
static gboolean  gimp_layer_list_item_drag_drop     (GtkWidget         *widget,
                                                     GdkDragContext    *context,
                                                     gint               x,
                                                     gint               y,
                                                     guint              time);
static void      gimp_layer_list_item_state_changed (GtkWidget         *widget,
                                                     GtkStateType       old_state);

static void      gimp_layer_list_item_linked_toggled (GtkWidget        *widget,
						      gpointer          data);
static void      gimp_layer_list_item_linked_changed (GimpLayer        *layer,
						      gpointer          data);

static void      gimp_layer_list_item_mask_changed  (GimpLayer         *layer,
                                                     GimpLayerListItem *layer_item);
static void      gimp_layer_list_item_update_state  (GtkWidget         *widget);

static void      gimp_layer_list_item_layer_clicked (GtkWidget         *widget,
                                                     GimpLayer         *layer);
static void      gimp_layer_list_item_mask_clicked  (GtkWidget         *widget,
                                                     GimpLayerMask     *mask);
static void      gimp_layer_list_item_mask_extended_clicked
                                                    (GtkWidget         *widget,
						     guint              state,
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

  widget_class->drag_motion         = gimp_layer_list_item_drag_motion;
  widget_class->drag_drop           = gimp_layer_list_item_drag_drop;
  widget_class->state_changed       = gimp_layer_list_item_state_changed;

  list_item_class->set_viewable     = gimp_layer_list_item_set_viewable;
  list_item_class->set_preview_size = gimp_layer_list_item_set_preview_size;

  gimp_rgba_set (&black_color, 0.0, 0.0, 0.0, 1.0);
  gimp_rgba_set (&white_color, 1.0, 1.0, 1.0, 1.0);
  gimp_rgba_set (&green_color, 0.0, 1.0, 0.0, 1.0);
  gimp_rgba_set (&red_color,   1.0, 0.0, 0.0, 1.0);
}

static void
gimp_layer_list_item_init (GimpLayerListItem *list_item)
{
  GtkWidget *abox;
  GtkWidget *pixmap;

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (GIMP_LIST_ITEM (list_item)->hbox), abox,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (GIMP_LIST_ITEM (list_item)->hbox), abox, 1);
  gtk_widget_show (abox);

  list_item->linked_button = gtk_toggle_button_new ();
  gtk_button_set_relief (GTK_BUTTON (list_item->linked_button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (abox), list_item->linked_button);
  gtk_widget_show (list_item->linked_button);

  gtk_signal_connect (GTK_OBJECT (list_item->linked_button), "realize",
                      GTK_SIGNAL_FUNC (gimp_list_item_button_realize),
                      list_item);

  gtk_signal_connect (GTK_OBJECT (list_item->linked_button), "state_changed",
                      GTK_SIGNAL_FUNC (gimp_list_item_button_state_changed),
                      list_item);

  pixmap = gimp_pixmap_new (linked_xpm);
  gtk_container_add (GTK_CONTAINER (list_item->linked_button), pixmap);
  gtk_widget_show (pixmap);

  list_item->mask_preview = NULL;
}

static void
gimp_layer_list_item_set_viewable (GimpListItem *list_item,
                                   GimpViewable *viewable)
{
  GimpLayerListItem *layer_item;
  GimpLayer         *layer;
  gboolean           linked;

  if (GIMP_LIST_ITEM_CLASS (parent_class)->set_viewable)
    GIMP_LIST_ITEM_CLASS (parent_class)->set_viewable (list_item, viewable);

  gimp_preview_set_size (GIMP_PREVIEW (list_item->preview),
                         list_item->preview_size, 2);

  layer_item = GIMP_LAYER_LIST_ITEM (list_item);
  layer      = GIMP_LAYER (GIMP_PREVIEW (list_item->preview)->viewable);
  linked     = gimp_layer_get_linked (layer);

  if (! linked)
    {
      GtkRequisition requisition;

      gtk_widget_size_request (layer_item->linked_button, &requisition);

      gtk_widget_set_usize (layer_item->linked_button,
                            requisition.width,
                            requisition.height);
      gtk_widget_hide (GTK_BIN (layer_item->linked_button)->child);
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (layer_item->linked_button),
                                linked);

  gtk_signal_connect (GTK_OBJECT (layer_item->linked_button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_layer_list_item_linked_toggled),
                      list_item);

  gtk_signal_connect_while_alive
    (GTK_OBJECT (viewable), "linked_changed",
     GTK_SIGNAL_FUNC (gimp_layer_list_item_linked_changed),
     list_item,
     GTK_OBJECT (list_item));

  gtk_signal_connect (GTK_OBJECT (list_item->preview), "clicked",
                      GTK_SIGNAL_FUNC (gimp_layer_list_item_layer_clicked),
                      layer);

  if (gimp_layer_get_mask (layer))
    {
      gimp_layer_list_item_mask_changed (layer, layer_item);
    }

  gtk_signal_connect_while_alive
    (GTK_OBJECT (layer), "mask_changed",
     GTK_SIGNAL_FUNC (gimp_layer_list_item_mask_changed),
     list_item,
     GTK_OBJECT (list_item));
}

static void
gimp_layer_list_item_set_preview_size (GimpListItem *list_item)
{
  GimpLayerListItem *layer_item;

  if (GIMP_LIST_ITEM_CLASS (parent_class)->set_preview_size)
    GIMP_LIST_ITEM_CLASS (parent_class)->set_preview_size (list_item);

  layer_item = GIMP_LAYER_LIST_ITEM (list_item);

  if (layer_item->mask_preview)
    {
      GimpPreview *preview;

      preview = GIMP_PREVIEW (layer_item->mask_preview);

      gimp_preview_set_size (preview,
			     list_item->preview_size, preview->border_width);
    }
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
gimp_layer_list_item_linked_toggled (GtkWidget *widget,
                                     gpointer   data)
{
  GimpListItem *list_item;
  GimpLayer    *layer;
  gboolean      linked;

  list_item = GIMP_LIST_ITEM (data);
  layer     = GIMP_LAYER (GIMP_PREVIEW (list_item->preview)->viewable);
  linked    = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  if (linked != gimp_layer_get_linked (layer))
    {
      if (! linked)
        {
          gtk_widget_set_usize (GTK_WIDGET (widget),
                                GTK_WIDGET (widget)->allocation.width,
                                GTK_WIDGET (widget)->allocation.height);
          gtk_widget_hide (GTK_BIN (widget)->child);
        }
      else
        {
          gtk_widget_show (GTK_BIN (widget)->child);
          gtk_widget_set_usize (GTK_WIDGET (widget), -1, -1);
        }

      gtk_signal_handler_block_by_func (GTK_OBJECT (layer),
                                        gimp_layer_list_item_linked_changed,
                                        list_item);

      gimp_layer_set_linked (layer, linked);

      gtk_signal_handler_unblock_by_func (GTK_OBJECT (layer),
                                          gimp_layer_list_item_linked_changed,
                                          list_item);
    }
}

static void
gimp_layer_list_item_linked_changed (GimpLayer *layer,
				     gpointer   data)
{
  GimpListItem    *list_item;
  GtkToggleButton *toggle;
  gboolean         linked;

  list_item = GIMP_LIST_ITEM (data);
  toggle    = GTK_TOGGLE_BUTTON (GIMP_LAYER_LIST_ITEM (data)->linked_button);
  linked    = gimp_layer_get_linked (layer);

  if (linked != toggle->active)
    {
      if (! linked)
        {
          gtk_widget_set_usize (GTK_WIDGET (toggle),
                                GTK_WIDGET (toggle)->allocation.width,
                                GTK_WIDGET (toggle)->allocation.height);
          gtk_widget_hide (GTK_BIN (toggle)->child);
        }
      else
        {
          gtk_widget_show (GTK_BIN (toggle)->child);
          gtk_widget_set_usize (GTK_WIDGET (toggle), -1, -1);
        }

      gtk_signal_handler_block_by_func (GTK_OBJECT (toggle),
                                        gimp_layer_list_item_linked_toggled,
                                        list_item);

      gtk_toggle_button_set_active (toggle, linked);

      gtk_signal_handler_unblock_by_func (GTK_OBJECT (toggle),
                                          gimp_layer_list_item_linked_toggled,
                                          list_item);
    }
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
                             layer_item->mask_preview, 3);
      gtk_widget_show (layer_item->mask_preview);

      gtk_signal_connect (GTK_OBJECT (layer_item->mask_preview), "clicked",
                          GTK_SIGNAL_FUNC (gimp_layer_list_item_mask_clicked),
                          mask);
      gtk_signal_connect (GTK_OBJECT (layer_item->mask_preview), "extended_clicked",
                          GTK_SIGNAL_FUNC (gimp_layer_list_item_mask_extended_clicked),
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
  GimpRGB           *layer_color;
  GimpRGB           *mask_color;
  GimpRGB            bg_color;

  gimp_rgb_set_uchar (&bg_color,
		      widget->style->base[widget->state].red   >> 8,
		      widget->style->base[widget->state].green >> 8,
		      widget->style->base[widget->state].blue  >> 8);

  layer_color = &bg_color;
  mask_color  = &bg_color;

  layer_item = GIMP_LAYER_LIST_ITEM (widget);
  list_item  = GIMP_LIST_ITEM (widget);
  layer      = GIMP_LAYER (GIMP_PREVIEW (list_item->preview)->viewable);
  mask       = gimp_layer_get_mask (layer);

  if (! mask || (mask && ! gimp_layer_mask_get_edit (mask)))
    {
      if (widget->state == GTK_STATE_SELECTED)
	layer_color = &white_color;
      else
	layer_color = &black_color;
    }

  if (mask)
    {
      if (gimp_layer_mask_get_show (mask))
	{
	  mask_color = &green_color;
	}
      else if (! gimp_layer_mask_get_apply (mask))
	{
	  mask_color = &red_color;
	}
      else if (gimp_layer_mask_get_edit (mask))
	{
	  if (widget->state == GTK_STATE_SELECTED)
	    mask_color = &white_color;
	  else
	    mask_color = &black_color;
	}
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
  if (! gimp_layer_mask_get_edit (mask))
    gimp_layer_mask_set_edit (mask, TRUE);
}

static void
gimp_layer_list_item_mask_extended_clicked (GtkWidget     *widget,
					    guint          state,
					    GimpLayerMask *mask)
{
  gboolean flush = FALSE;

  if (state & GDK_MOD1_MASK)
    {
      gimp_layer_mask_set_show (mask, ! gimp_layer_mask_get_show (mask));

      flush = TRUE;
    }
  else if (state & GDK_CONTROL_MASK)
    {
      gimp_layer_mask_set_apply (mask, ! gimp_layer_mask_get_apply (mask));

      flush = TRUE;
    }

  if (flush)
    gdisplays_flush ();
}
