/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#ifndef __LAYER_H__
#define __LAYER_H__

#include "apptypes.h"
#include "drawable.h"
#include "boundary.h"
#include "channel.h"
#include "temp_buf.h"
#include "tile_manager.h"

#include "layerF.h"

/*  structure declarations  */

#define GIMP_TYPE_LAYER             (gimp_layer_get_type ())
#define GIMP_LAYER(obj)             (GTK_CHECK_CAST ((obj), GIMP_TYPE_LAYER, GimpLayer))
#define GIMP_LAYER_CLASS(klass)     (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LAYER, GimpLayerClass))
#define GIMP_IS_LAYER(obj)          (GTK_CHECK_TYPE ((obj), GIMP_TYPE_LAYER))
#define GIMP_IS_LAYER_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LAYER))

#define GIMP_TYPE_LAYER_MASK             (gimp_layer_mask_get_type ())
#define GIMP_LAYER_MASK(obj)             (GTK_CHECK_CAST ((obj), GIMP_TYPE_LAYER_MASK, GimpLayerMask))
#define GIMP_LAYER_MASK_CLASS(klass)     (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LAYER_MASK, GimpLayerMaskClass))
#define GIMP_IS_LAYER_MASK(obj)          (GTK_CHECK_TYPE ((obj), GIMP_TYPE_LAYER_MASK))
#define GIMP_IS_LAYER_MASK_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LAYER_MASK))

GtkType gimp_layer_get_type      (void);
GtkType gimp_layer_mask_get_type (void);

/*  Special undo types  */

typedef enum /*< skip >*/
{
    LAYER_ADD_UNDO = 0,
    LAYER_REMOVE_UNDO = 1
} LayerUndoType;

struct _layer_undo
{
  Layer          *layer;              /*  the actual layer          */
  gint            prev_position;      /*  former position in list   */
  Layer          *prev_layer;         /*  previous active layer     */
  LayerUndoType undo_type;            /*  is this a new layer undo  *
				       *  or a remove layer undo?   */
};

struct _layer_mask_undo
{
  Layer          *layer;         /*  the layer                 */
  gboolean        apply_mask;    /*  apply mask?               */
  gboolean        edit_mask;     /*  edit mask or layer?       */
  gboolean        show_mask;     /*  show the mask?            */
  LayerMask      *mask;          /*  the layer mask            */
  gint            mode;          /*  the application mode      */
  LayerUndoType   undo_type;     /*  is this a new layer mask  */
                  	         /*  or a remove layer mask    */
};

struct _fs_to_layer_undo
{
  Layer        *layer;      /*  the layer                 */
  GimpDrawable *drawable;   /*  drawable of floating sel  */
};

/*  function declarations  */

Layer *         layer_new   (GimpImage*, gint, gint,
			     GimpImageType,
			     gchar *, gint,
			     LayerModeEffects);
Layer *         layer_copy  (Layer *, gboolean);
Layer *		layer_ref   (Layer *);
void   		layer_unref (Layer *);

Layer *         layer_from_tiles            (void *, GimpDrawable *,
					     TileManager *,
					     gchar *, gint,
					     LayerModeEffects);
LayerMask *     layer_add_mask              (Layer *, LayerMask *);
LayerMask *     layer_create_mask           (Layer *, AddMaskType);
Layer *         layer_get_ID                (gint);
void            layer_delete                (Layer *);
void            layer_removed               (Layer *, gpointer);
void            layer_apply_mask            (Layer *, MaskApplyMode);
void            layer_temporarily_translate (Layer *, gint, gint);
void            layer_translate             (Layer *, gint, gint);
void            layer_add_alpha             (Layer *);
void            layer_scale                 (Layer *, gint, gint, gint);
void            layer_resize                (Layer *, gint, gint, gint, gint);
BoundSeg *      layer_boundary              (Layer *, gint *);
void            layer_invalidate_boundary   (Layer *);
gint            layer_pick_correlate        (Layer *, gint, gint);

LayerMask *     layer_mask_new	     (GimpImage*, gint, gint, gchar *, 
				      gint, guchar *);
LayerMask *	layer_mask_copy	     (LayerMask *);
void		layer_mask_delete    (LayerMask *);
LayerMask *	layer_mask_get_ID    (gint);
LayerMask *	layer_mask_ref       (LayerMask *);
void   		layer_mask_unref     (LayerMask *);
void            layer_mask_set_layer (LayerMask *, Layer *);
Layer *         layer_mask_get_layer (LayerMask *);

/*  access functions  */

void            layer_set_name        (Layer *, gchar *);
gchar *         layer_get_name        (Layer *);
guchar *        layer_data            (Layer *);
LayerMask *     layer_get_mask        (Layer *);
gboolean        layer_has_alpha       (Layer *);
gboolean        layer_is_floating_sel (Layer *);
gboolean        layer_linked          (Layer *);
TempBuf *       layer_preview         (Layer *, gint, gint);
TempBuf *       layer_mask_preview    (Layer *, gint, gint);

void            layer_invalidate_previews (GimpImage *);
Tattoo          layer_get_tattoo          (const Layer *);

#define drawable_layer      GIMP_IS_LAYER
#define drawable_layer_mask GIMP_IS_LAYER_MASK

/*  from channel.c  */

void            channel_layer_alpha (Channel *, Layer *);
void            channel_layer_mask  (Channel *, Layer *);

#endif /* __LAYER_H__ */
