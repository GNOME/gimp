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

struct _layer_undo
{
  Layer          *layer;              /*  the actual layer          */
  gint            prev_position;      /*  former position in list   */
  Layer          *prev_layer;         /*  previous active layer     */
};

struct _layer_mask_undo
{
  Layer          *layer;         /*  the layer                 */
  gboolean        apply_mask;    /*  apply mask?               */
  gboolean        edit_mask;     /*  edit mask or layer?       */
  gboolean        show_mask;     /*  show the mask?            */
  LayerMask      *mask;          /*  the layer mask            */
  gint            mode;          /*  the application mode      */
};

struct _fs_to_layer_undo
{
  Layer        *layer;      /*  the layer                 */
  GimpDrawable *drawable;   /*  drawable of floating sel  */
};

/*  function declarations  */

Layer *         layer_new   (GimpImage        *gimage,
			     gint              width,
			     gint              height,
			     GimpImageType     type,
			     gchar            *name,
			     gint              opacity,
			     LayerModeEffects  mode);
Layer *         layer_copy  (Layer            *layer,
			     gboolean          add_alpha);
Layer *		layer_ref   (Layer            *layer);
void   		layer_unref (Layer            *layer);

Layer *         layer_new_from_tiles        (GimpImage        *gimage,
					     GimpImageType     layer_type,
					     TileManager      *tiles,
					     gchar            *name,
					     gint              opacity,
					     LayerModeEffects  mode);
gboolean        layer_check_scaling         (Layer            *layer,
					     gint              new_width,
					     gint              new_height);
LayerMask *     layer_create_mask           (Layer            *layer,
					     AddMaskType       add_mask_type);
LayerMask *     layer_add_mask              (Layer            *layer,
					     LayerMask        *mask);
Layer *         layer_get_ID                (gint              ID);
void            layer_delete                (Layer            *layer);
void            layer_removed               (Layer            *layer, 
					     gpointer          data);
void            layer_apply_mask            (Layer            *layer,
					     MaskApplyMode     mode);
void            layer_temporarily_translate (Layer            *layer,
					     gint              off_x,
					     gint              off_y);
void            layer_translate             (Layer            *layer,
					     gint              off_x,
					     gint              off_y);
void            layer_add_alpha             (Layer            *layer);
gboolean        layer_scale_by_factors      (Layer            *layer, 
					     gdouble           w_factor, 
					     gdouble           h_factor);
void            layer_scale                 (Layer            *layer, 
					     gint              new_width,
					     gint              new_height,
					     gboolean          local_origin);
void            layer_resize                (Layer            *layer,
					     gint              new_width,
					     gint              new_height,
					     gint              offx,
					     gint              offy);
void            layer_resize_to_image       (Layer            *layer);
BoundSeg *      layer_boundary              (Layer            *layer, 
					     gint             *num_segs);
void            layer_invalidate_boundary   (Layer           *layer);
gint            layer_pick_correlate        (Layer           *layer, 
					     gint             x, 
					     gint             y);

LayerMask *     layer_mask_new	     (GimpImage *gimage,
				      gint       width,
				      gint       height,
				      gchar     *name,
				      gint       opacity,
				      guchar    *col);
LayerMask *	layer_mask_copy	     (LayerMask *layer_mask);
void		layer_mask_delete    (LayerMask *layer_mask);
LayerMask *	layer_mask_get_ID    (gint       ID);
LayerMask *	layer_mask_ref       (LayerMask *layer_mask);
void   		layer_mask_unref     (LayerMask *layer_mask);
void            layer_mask_set_layer (LayerMask *layer_mask, 
				      Layer     *layer);
Layer *         layer_mask_get_layer (LayerMask *layer_mask);

/*  access functions  */

void            layer_set_name        (Layer *layer, 
				       gchar *name);
gchar *         layer_get_name        (Layer *layer);
guchar *        layer_data            (Layer *layer);
LayerMask *     layer_get_mask        (Layer *layer);
gboolean        layer_has_alpha       (Layer *layer);
gboolean        layer_is_floating_sel (Layer *layer);
gboolean        layer_linked          (Layer *layer);
TempBuf *       layer_preview         (Layer *layer, 
				       gint   width, 
				       gint   height);
TempBuf *       layer_mask_preview    (Layer *layer, 
				       gint   width, 
				       gint   height);

void            layer_invalidate_previews (GimpImage   *gimage);
Tattoo          layer_get_tattoo          (const Layer *layer);
void            layer_set_tattoo          (const Layer *layer, 
					   Tattoo       value);

#define drawable_layer      GIMP_IS_LAYER
#define drawable_layer_mask GIMP_IS_LAYER_MASK

void            channel_layer_alpha (Channel *mask, 
				     Layer   *layer);
/*  from channel.c  */
void            channel_layer_mask  (Channel *mask, 
				     Layer   *layer);

#endif /* __LAYER_H__ */
