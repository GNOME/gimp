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


/* EEK */

#define layer_new                 gimp_layer_new
#define layer_copy                gimp_layer_copy
#define layer_create_mask         gimp_layer_create_mask
#define layer_scale               gimp_layer_scale
#define layer_resize              gimp_layer_resize
#define layer_delete              gimp_layer_delete
#define layer_translate           gimp_layer_translate
#define layer_get_name            gimp_layer_get_name
#define layer_set_name            gimp_layer_set_name
#define layer_get_tattoo          gimp_layer_get_tattoo
#define layer_set_tattoo          gimp_layer_set_tattoo
#define layer_add_alpha           gimp_layer_add_alpha
#define layer_has_alpha           gimp_layer_has_alpha
#define layer_preview             gimp_layer_preview
#define layer_is_floating_sel     gimp_layer_is_floating_sel
#define layer_invalidate_previews gimp_layer_invalidate_previews
#define layer_linked              gimp_layer_linked
#define layer_get_mask            gimp_layer_get_mask
#define layer_new_from_tiles      gimp_layer_new_from_tiles
#define layer_resize_to_image     gimp_layer_resize_to_image
#define layer_invalidate_boundary gimp_layer_invalidate_boundary
#define layer_scale_by_factors    gimp_layer_scale_by_factors
#define layer_boundary            gimp_layer_boundary
#define layer_pick_correlate      gimp_layer_pick_correlate
#define layer_apply_mask          gimp_layer_apply_mask
#define layer_add_mask            gimp_layer_add_mask
#define layer_check_scaling       gimp_layer_check_scaling


#define gimp_drawable_layer      GIMP_IS_LAYER


#define GIMP_TYPE_LAYER            (gimp_layer_get_type ())
#define GIMP_LAYER(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_LAYER, GimpLayer))
#define GIMP_LAYER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LAYER, GimpLayerClass))
#define GIMP_IS_LAYER(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_LAYER))
#define GIMP_IS_LAYER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LAYER))


typedef struct _GimpLayerClass      GimpLayerClass;

struct _GimpLayer
{
  GimpDrawable      parent_instance;

  gboolean          linked;           /*  control linkage                */
  gboolean          preserve_trans;   /*  preserve transparency          */

  GimpLayerMask    *mask;             /*  possible layer mask            */
  gint              apply_mask;       /*  controls mask application      */
  gboolean          edit_mask;        /*  edit mask or layer?            */
  gboolean          show_mask;        /*  show mask or layer?            */

  gint              opacity;          /*  layer opacity                  */
  LayerModeEffects  mode;             /*  layer combination mode         */

  /*  Floating selections  */
  struct
  {
    TileManager  *backing_store;      /*  for obscured regions           */
    GimpDrawable *drawable;           /*  floating sel is attached to    */
    gboolean      initial;            /*  is fs composited yet?          */

    gboolean      boundary_known;     /*  is the current boundary valid  */
    BoundSeg     *segs;               /*  boundary of floating sel       */
    gint          num_segs;           /*  number of segs in boundary     */
  } fs;
};

struct _GimpLayerClass
{
  GimpDrawableClass  parent_class;
};


/*  Special undo types  */

struct _LayerUndo
{
  GimpLayer *layer;              /*  the actual layer          */
  gint       prev_position;      /*  former position in list   */
  GimpLayer *prev_layer;         /*  previous active layer     */
};

struct _FStoLayerUndo
{
  GimpLayer    *layer;      /*  the layer                 */
  GimpDrawable *drawable;   /*  drawable of floating sel  */
};


/*  function declarations  */

GtkType         gimp_layer_get_type            (void);

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
						GimpLayerMask    *mask);
void            gimp_layer_delete              (GimpLayer        *layer);
void            gimp_layer_apply_mask          (GimpLayer        *layer,
						MaskApplyMode     mode);
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

void            gimp_layer_set_name            (GimpLayer        *layer, 
						const gchar      *name);
const gchar   * gimp_layer_get_name            (const GimpLayer  *layer);
GimpLayerMask * gimp_layer_get_mask            (GimpLayer        *layer);
gboolean        gimp_layer_has_alpha           (GimpLayer        *layer);
gboolean        gimp_layer_is_floating_sel     (GimpLayer        *layer);
gboolean        gimp_layer_linked              (GimpLayer        *layer);
Tattoo          gimp_layer_get_tattoo          (const GimpLayer  *layer);
void            gimp_layer_set_tattoo          (GimpLayer        *layer, 
						Tattoo            value);

TempBuf       * gimp_layer_preview             (GimpLayer        *layer, 
						gint              width, 
						gint              height);
TempBuf       * gimp_layer_mask_preview        (GimpLayer        *layer, 
						gint              width, 
						gint              height);


#endif /* __GIMP_LAYER_H__ */
