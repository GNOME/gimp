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
  GimpDrawable          parent_instance;

  gdouble               opacity;          /*  layer opacity              */
  GimpLayerModeEffects  mode;             /*  layer combination mode     */
  gboolean              preserve_trans;   /*  preserve transparency      */

  GimpLayerMask        *mask;             /*  possible layer mask        */

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

  void (* opacity_changed)        (GimpLayer *layer);
  void (* mode_changed)           (GimpLayer *layer);
  void (* preserve_trans_changed) (GimpLayer *layer);
  void (* mask_changed)           (GimpLayer *layer);
};


/*  function declarations  */

GType           gimp_layer_get_type            (void) G_GNUC_CONST;

GimpLayer     * gimp_layer_new                 (GimpImage            *gimage,
                                                gint                  width,
                                                gint                  height,
                                                GimpImageType         type,
                                                const gchar          *name,
                                                gdouble               opacity,
                                                GimpLayerModeEffects  mode);

GimpLayer     * gimp_layer_new_from_tiles      (TileManager          *tiles,
                                                GimpImage            *dest_gimage,
                                                GimpImageType         type,
                                                const gchar          *name,
                                                gdouble               opacity,
                                                GimpLayerModeEffects  mode);

GimpLayerMask * gimp_layer_create_mask         (const GimpLayer      *layer,
                                                GimpAddMaskType       mask_type);
GimpLayerMask * gimp_layer_add_mask            (GimpLayer            *layer,
                                                GimpLayerMask        *mask,
                                                gboolean              push_undo);
void            gimp_layer_apply_mask          (GimpLayer            *layer,
                                                GimpMaskApplyMode     mode,
                                                gboolean              push_undo);
void            gimp_layer_add_alpha           (GimpLayer            *layer);

void            gimp_layer_resize_to_image     (GimpLayer            *layer,
                                                GimpContext          *context);
BoundSeg      * gimp_layer_boundary            (GimpLayer            *layer,
                                                gint                 *num_segs);
gboolean        gimp_layer_pick_correlate      (GimpLayer            *layer,
                                                gint                  x,
                                                gint                  y);

GimpLayerMask * gimp_layer_get_mask            (const GimpLayer      *layer);

gboolean        gimp_layer_is_floating_sel     (const GimpLayer      *layer);

void            gimp_layer_set_opacity         (GimpLayer            *layer,
                                                gdouble               opacity,
                                                gboolean              push_undo);
gdouble         gimp_layer_get_opacity         (const GimpLayer      *layer);

void            gimp_layer_set_mode            (GimpLayer            *layer,
                                                GimpLayerModeEffects  mode,
                                                gboolean              push_undo);
GimpLayerModeEffects gimp_layer_get_mode       (const GimpLayer      *layer);

void            gimp_layer_set_preserve_trans  (GimpLayer            *layer,
                                                gboolean              preserve,
                                                gboolean              push_undo);
gboolean        gimp_layer_get_preserve_trans  (const GimpLayer      *layer);


#endif /* __GIMP_LAYER_H__ */
