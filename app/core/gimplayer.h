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

#ifndef __GIMP_LAYER_H__
#define __GIMP_LAYER_H__


#include "gimpdrawable.h"


#define GIMP_TYPE_LAYER            (gimp_layer_get_type ())
#define GIMP_LAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LAYER, GimpLayer))
#define GIMP_LAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LAYER, GimpLayerClass))
#define GIMP_IS_LAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LAYER))
#define GIMP_IS_LAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LAYER))
#define GIMP_LAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LAYER, GimpLayerClass))


typedef struct _GimpLayerClass GimpLayerClass;

struct _GimpLayer
{
  GimpDrawable      parent_instance;

  guint             opacity;          /*  layer opacity                  */
  LayerModeEffects  mode;             /*  layer combination mode         */
  guint             preserve_trans;   /*  preserve transparency      */
  guint             linked;           /*  control linkage                */
  
  GimpLayerMask    *mask;             /*  possible layer mask            */

  /*  Floating selections  */
  struct
  {
    TileManager  *backing_store;      /*  for obscured regions           */
    GimpDrawable *drawable;           /*  floating sel is attached to    */
    guint         initial : 1;        /*  is fs composited yet?          */
    guint         boundary_known : 1; /*  is the current boundary valid  */
    BoundSeg     *segs;               /*  boundary of floating sel       */
    guint         num_segs;           /*  number of segs in boundary     */
  } fs;
};

struct _GimpLayerClass
{
  GimpDrawableClass  parent_class;

  void (* opacity_changed)        (GimpLayer *layer);
  void (* mode_changed)           (GimpLayer *layer);
  void (* preserve_trans_changed) (GimpLayer *layer);
  void (* linked_changed)         (GimpLayer *layer);
  void (* mask_changed)           (GimpLayer *layer);
};


/*  Special undo types  */

struct _LayerUndo
{
  GimpLayer *layer;           /*  the actual layer         */
  gint       prev_position;   /*  former position in list  */
  GimpLayer *prev_layer;      /*  previous active layer    */
};

struct _FStoLayerUndo
{
  GimpLayer    *layer;      /*  the layer                 */
  GimpDrawable *drawable;   /*  drawable of floating sel  */
};


/*  function declarations  */

GType           gimp_layer_get_type            (void) G_GNUC_CONST;

GimpLayer     * gimp_layer_new                 (GimpImage        *gimage,
						gint              width,
						gint              height,
						GimpImageType     type,
						const gchar      *name,
						gint              opacity,
						LayerModeEffects  mode);
GimpLayer     * gimp_layer_copy                (GimpLayer        *layer,
						gboolean          add_alpha);

GimpLayer     * gimp_layer_new_from_tiles      (GimpImage        *gimage,
						GimpImageType     layer_type,
						TileManager      *tiles,
						gchar            *name,
						gint              opacity,
						LayerModeEffects  mode);
gboolean        gimp_layer_check_scaling       (GimpLayer        *layer,
						gint              new_width,
						gint              new_height);
GimpLayerMask * gimp_layer_create_mask         (GimpLayer        *layer,
						AddMaskType       add_mask_type);
GimpLayerMask * gimp_layer_add_mask            (GimpLayer        *layer,
						GimpLayerMask    *mask,
                                                gboolean          push_undo);
void            gimp_layer_apply_mask          (GimpLayer        *layer,
						MaskApplyMode     mode,
                                                gboolean          push_undo);
void            gimp_layer_translate           (GimpLayer        *layer,
						gint              off_x,
						gint              off_y);
void            gimp_layer_add_alpha           (GimpLayer        *layer);
gboolean        gimp_layer_scale_by_factors    (GimpLayer        *layer, 
						gdouble           w_factor, 
						gdouble           h_factor);
void            gimp_layer_scale               (GimpLayer        *layer, 
						gint              new_width,
						gint              new_height,
						gboolean          local_origin);
void            gimp_layer_resize              (GimpLayer        *layer,
						gint              new_width,
						gint              new_height,
						gint              offx,
						gint              offy);
void            gimp_layer_resize_to_image     (GimpLayer        *layer);
BoundSeg      * gimp_layer_boundary            (GimpLayer        *layer, 
						gint             *num_segs);
void            gimp_layer_invalidate_boundary (GimpLayer        *layer);
gint            gimp_layer_pick_correlate      (GimpLayer        *layer, 
						gint              x, 
						gint              y);

GimpLayerMask * gimp_layer_get_mask            (GimpLayer        *layer);

gboolean        gimp_layer_has_alpha           (GimpLayer        *layer);
gboolean        gimp_layer_is_floating_sel     (GimpLayer        *layer);

void            gimp_layer_set_opacity         (GimpLayer        *layer,
                                                gdouble           opacity);
gdouble         gimp_layer_get_opacity         (GimpLayer        *layer);

void            gimp_layer_set_mode            (GimpLayer        *layer,
                                                LayerModeEffects  mode);
LayerModeEffects gimp_layer_get_mode           (GimpLayer        *layer);

void            gimp_layer_set_preserve_trans  (GimpLayer        *layer,
                                                gboolean          preserve);
gboolean        gimp_layer_get_preserve_trans  (GimpLayer        *layer);

void            gimp_layer_set_linked          (GimpLayer        *layer,
                                                gboolean          linked);
gboolean        gimp_layer_get_linked          (GimpLayer        *layer);


#endif /* __GIMP_LAYER_H__ */
