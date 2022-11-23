/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_LAYER_H__
#define __LIGMA_LAYER_H__


#include "ligmadrawable.h"


#define LIGMA_TYPE_LAYER            (ligma_layer_get_type ())
#define LIGMA_LAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LAYER, LigmaLayer))
#define LIGMA_LAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LAYER, LigmaLayerClass))
#define LIGMA_IS_LAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LAYER))
#define LIGMA_IS_LAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LAYER))
#define LIGMA_LAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_LAYER, LigmaLayerClass))


typedef struct _LigmaLayerClass LigmaLayerClass;

struct _LigmaLayer
{
  LigmaDrawable            parent_instance;

  gdouble                 opacity;                    /*  layer opacity                     */
  LigmaLayerMode           mode;                       /*  layer combination mode            */
  LigmaLayerColorSpace     blend_space;                /*  layer blend space                 */
  LigmaLayerColorSpace     composite_space;            /*  layer composite space             */
  LigmaLayerCompositeMode  composite_mode;             /*  layer composite mode              */
  LigmaLayerMode           effective_mode;             /*  layer effective combination mode  */
  LigmaLayerColorSpace     effective_blend_space;      /*  layer effective blend space       */
  LigmaLayerColorSpace     effective_composite_space;  /*  layer effective composite space   */
  LigmaLayerCompositeMode  effective_composite_mode;   /*  layer effective composite mode    */
  gboolean                excludes_backdrop;          /*  layer clips backdrop              */
  gboolean                lock_alpha;                 /*  lock the alpha channel            */

  LigmaLayerMask          *mask;                       /*  possible layer mask               */
  gboolean                apply_mask;                 /*  controls mask application         */
  gboolean                edit_mask;                  /*  edit mask or layer?               */
  gboolean                show_mask;                  /*  show mask or layer?               */

  GSList                 *move_stack;                 /*  ancestors affected by move        */

  GeglNode               *layer_offset_node;
  GeglNode               *mask_offset_node;

  /*  Floating selections  */
  struct
  {
    LigmaDrawable *drawable;           /*  floating sel is attached to    */
    gboolean      boundary_known;     /*  is the current boundary valid  */
    LigmaBoundSeg *segs;               /*  boundary of floating sel       */
    gint          num_segs;           /*  number of segs in boundary     */
  } fs;
};

struct _LigmaLayerClass
{
  LigmaDrawableClass  parent_class;

  /*  signals  */
  void          (* opacity_changed)           (LigmaLayer              *layer);
  void          (* mode_changed)              (LigmaLayer              *layer);
  void          (* blend_space_changed)       (LigmaLayer              *layer);
  void          (* composite_space_changed)   (LigmaLayer              *layer);
  void          (* composite_mode_changed)    (LigmaLayer              *layer);
  void          (* effective_mode_changed)    (LigmaLayer              *layer);
  void          (* excludes_backdrop_changed) (LigmaLayer              *layer);
  void          (* lock_alpha_changed)        (LigmaLayer              *layer);
  void          (* mask_changed)              (LigmaLayer              *layer);
  void          (* apply_mask_changed)        (LigmaLayer              *layer);
  void          (* edit_mask_changed)         (LigmaLayer              *layer);
  void          (* show_mask_changed)         (LigmaLayer              *layer);

  /*  virtual functions  */
  gboolean      (* is_alpha_locked)           (LigmaLayer              *layer,
                                               LigmaLayer             **locked_layer);

  void          (* translate)                 (LigmaLayer              *layer,
                                               gint                    offset_x,
                                               gint                    offset_y);
  void          (* scale)                     (LigmaLayer              *layer,
                                               gint                    new_width,
                                               gint                    new_height,
                                               gint                    new_offset_x,
                                               gint                    new_offset_y,
                                               LigmaInterpolationType   interpolation_type,
                                               LigmaProgress           *progress);
  void          (* resize)                    (LigmaLayer              *layer,
                                               LigmaContext            *context,
                                               LigmaFillType            fill_type,
                                               gint                    new_width,
                                               gint                    new_height,
                                               gint                    offset_x,
                                               gint                    offset_y);
  void          (* flip)                      (LigmaLayer              *layer,
                                               LigmaContext            *context,
                                               LigmaOrientationType     flip_type,
                                               gdouble                 axis,
                                               gboolean                clip_result);
  void          (* rotate)                    (LigmaLayer              *layer,
                                               LigmaContext            *context,
                                               LigmaRotationType        rotate_type,
                                               gdouble                 center_x,
                                               gdouble                 center_y,
                                               gboolean                clip_result);
  void          (* transform)                 (LigmaLayer              *layer,
                                               LigmaContext            *context,
                                               const LigmaMatrix3      *matrix,
                                               LigmaTransformDirection  direction,
                                               LigmaInterpolationType   interpolation_type,
                                               LigmaTransformResize     clip_result,
                                               LigmaProgress           *progress);
  void          (* convert_type)              (LigmaLayer              *layer,
                                               LigmaImage              *dest_image,
                                               const Babl             *new_format,
                                               LigmaColorProfile       *src_profile,
                                               LigmaColorProfile       *dest_profile,
                                               GeglDitherMethod        layer_dither_type,
                                               GeglDitherMethod        mask_dither_type,
                                               gboolean                push_undo,
                                               LigmaProgress           *progress);
  GeglRectangle (* get_bounding_box)          (LigmaLayer              *layer);
  void          (* get_effective_mode)        (LigmaLayer              *layer,
                                               LigmaLayerMode          *mode,
                                               LigmaLayerColorSpace    *blend_space,
                                               LigmaLayerColorSpace    *composite_space,
                                               LigmaLayerCompositeMode *composite_mode);
  gboolean      (* get_excludes_backdrop)     (LigmaLayer              *layer);
};


/*  function declarations  */

GType           ligma_layer_get_type            (void) G_GNUC_CONST;

void            ligma_layer_fix_format_space    (LigmaLayer            *layer,
                                                gboolean              copy_buffer,
                                                gboolean              push_undo);

LigmaLayer     * ligma_layer_get_parent          (LigmaLayer            *layer);

LigmaLayerMask * ligma_layer_get_mask            (LigmaLayer            *layer);
LigmaLayerMask * ligma_layer_create_mask         (LigmaLayer            *layer,
                                                LigmaAddMaskType       mask_type,
                                                LigmaChannel          *channel);
LigmaLayerMask * ligma_layer_add_mask            (LigmaLayer            *layer,
                                                LigmaLayerMask        *mask,
                                                gboolean              push_undo,
                                                GError              **error);
void            ligma_layer_apply_mask          (LigmaLayer            *layer,
                                                LigmaMaskApplyMode     mode,
                                                gboolean              push_undo);

void            ligma_layer_set_apply_mask      (LigmaLayer           *layer,
                                                gboolean             apply,
                                                gboolean             push_undo);
gboolean        ligma_layer_get_apply_mask      (LigmaLayer           *layer);

void            ligma_layer_set_edit_mask       (LigmaLayer           *layer,
                                                gboolean             edit);
gboolean        ligma_layer_get_edit_mask       (LigmaLayer           *layer);

void            ligma_layer_set_show_mask       (LigmaLayer           *layer,
                                                gboolean             show,
                                                gboolean             push_undo);
gboolean        ligma_layer_get_show_mask       (LigmaLayer           *layer);

void            ligma_layer_add_alpha           (LigmaLayer            *layer);
void            ligma_layer_remove_alpha        (LigmaLayer            *layer,
                                                LigmaContext          *context);

void            ligma_layer_resize_to_image     (LigmaLayer            *layer,
                                                LigmaContext          *context,
                                                LigmaFillType          fill_type);

LigmaDrawable * ligma_layer_get_floating_sel_drawable (LigmaLayer       *layer);
void           ligma_layer_set_floating_sel_drawable (LigmaLayer       *layer,
                                                     LigmaDrawable    *drawable);
gboolean        ligma_layer_is_floating_sel     (LigmaLayer            *layer);

void            ligma_layer_set_opacity         (LigmaLayer            *layer,
                                                gdouble               opacity,
                                                gboolean              push_undo);
gdouble         ligma_layer_get_opacity         (LigmaLayer            *layer);

void            ligma_layer_set_mode            (LigmaLayer            *layer,
                                                LigmaLayerMode         mode,
                                                gboolean              push_undo);
LigmaLayerMode   ligma_layer_get_mode            (LigmaLayer            *layer);

void            ligma_layer_set_blend_space     (LigmaLayer            *layer,
                                                LigmaLayerColorSpace   blend_space,
                                                gboolean              push_undo);
LigmaLayerColorSpace
                ligma_layer_get_blend_space     (LigmaLayer            *layer);
LigmaLayerColorSpace
               ligma_layer_get_real_blend_space (LigmaLayer            *layer);

void            ligma_layer_set_composite_space (LigmaLayer            *layer,
                                                LigmaLayerColorSpace   composite_space,
                                                gboolean              push_undo);
LigmaLayerColorSpace
                ligma_layer_get_composite_space (LigmaLayer            *layer);
LigmaLayerColorSpace
           ligma_layer_get_real_composite_space (LigmaLayer            *layer);

void            ligma_layer_set_composite_mode  (LigmaLayer            *layer,
                                                LigmaLayerCompositeMode  composite_mode,
                                                gboolean              push_undo);
LigmaLayerCompositeMode
                ligma_layer_get_composite_mode  (LigmaLayer            *layer);
LigmaLayerCompositeMode
           ligma_layer_get_real_composite_mode  (LigmaLayer            *layer);

void            ligma_layer_get_effective_mode  (LigmaLayer            *layer,
                                                LigmaLayerMode          *mode,
                                                LigmaLayerColorSpace    *blend_space,
                                                LigmaLayerColorSpace    *composite_space,
                                                LigmaLayerCompositeMode *composite_mode);

gboolean      ligma_layer_get_excludes_backdrop (LigmaLayer            *layer);

void            ligma_layer_set_lock_alpha      (LigmaLayer            *layer,
                                                gboolean              lock_alpha,
                                                gboolean              push_undo);
gboolean        ligma_layer_get_lock_alpha      (LigmaLayer            *layer);
gboolean        ligma_layer_can_lock_alpha      (LigmaLayer            *layer);
gboolean        ligma_layer_is_alpha_locked     (LigmaLayer            *layer,
                                                LigmaLayer           **locked_layer);


/*  protected  */

void          ligma_layer_update_effective_mode (LigmaLayer            *layer);
void       ligma_layer_update_excludes_backdrop (LigmaLayer            *layer);


#endif /* __LIGMA_LAYER_H__ */
