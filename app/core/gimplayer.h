/* GIMP - The GNU Image Manipulation Program
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

#pragma once

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
  GimpDrawable            parent_instance;

  gdouble                 opacity;                    /*  layer opacity                     */
  GimpLayerMode           mode;                       /*  layer combination mode            */
  GimpLayerColorSpace     blend_space;                /*  layer blend space                 */
  GimpLayerColorSpace     composite_space;            /*  layer composite space             */
  GimpLayerCompositeMode  composite_mode;             /*  layer composite mode              */
  GimpLayerMode           effective_mode;             /*  layer effective combination mode  */
  GimpLayerColorSpace     effective_blend_space;      /*  layer effective blend space       */
  GimpLayerColorSpace     effective_composite_space;  /*  layer effective composite space   */
  GimpLayerCompositeMode  effective_composite_mode;   /*  layer effective composite mode    */
  gboolean                excludes_backdrop;          /*  layer clips backdrop              */
  gboolean                lock_alpha;                 /*  lock the alpha channel            */

  GimpLayerMask          *mask;                       /*  possible layer mask               */
  gboolean                apply_mask;                 /*  controls mask application         */
  gboolean                edit_mask;                  /*  edit mask or layer?               */
  gboolean                show_mask;                  /*  show mask or layer?               */

  GSList                 *move_stack;                 /*  ancestors affected by move        */

  GeglNode               *layer_offset_node;
  GeglNode               *mask_offset_node;

  /*  Floating selections  */
  struct
  {
    GimpDrawable *drawable;           /*  floating sel is attached to    */
    gboolean      boundary_known;     /*  is the current boundary valid  */
    GimpBoundSeg *segs;               /*  boundary of floating sel       */
    gint          num_segs;           /*  number of segs in boundary     */
  } fs;
};

struct _GimpLayerClass
{
  GimpDrawableClass  parent_class;

  /*  signals  */
  void          (* opacity_changed)           (GimpLayer              *layer);
  void          (* mode_changed)              (GimpLayer              *layer);
  void          (* blend_space_changed)       (GimpLayer              *layer);
  void          (* composite_space_changed)   (GimpLayer              *layer);
  void          (* composite_mode_changed)    (GimpLayer              *layer);
  void          (* effective_mode_changed)    (GimpLayer              *layer);
  void          (* excludes_backdrop_changed) (GimpLayer              *layer);
  void          (* lock_alpha_changed)        (GimpLayer              *layer);
  void          (* mask_changed)              (GimpLayer              *layer);
  void          (* apply_mask_changed)        (GimpLayer              *layer);
  void          (* edit_mask_changed)         (GimpLayer              *layer);
  void          (* show_mask_changed)         (GimpLayer              *layer);

  /*  virtual functions  */
  gboolean      (* is_alpha_locked)           (GimpLayer              *layer,
                                               GimpLayer             **locked_layer);

  void          (* translate)                 (GimpLayer              *layer,
                                               gint                    offset_x,
                                               gint                    offset_y);
  void          (* scale)                     (GimpLayer              *layer,
                                               gint                    new_width,
                                               gint                    new_height,
                                               gint                    new_offset_x,
                                               gint                    new_offset_y,
                                               GimpInterpolationType   interpolation_type,
                                               GimpProgress           *progress);
  void          (* resize)                    (GimpLayer              *layer,
                                               GimpContext            *context,
                                               GimpFillType            fill_type,
                                               gint                    new_width,
                                               gint                    new_height,
                                               gint                    offset_x,
                                               gint                    offset_y);
  void          (* flip)                      (GimpLayer              *layer,
                                               GimpContext            *context,
                                               GimpOrientationType     flip_type,
                                               gdouble                 axis,
                                               gboolean                clip_result);
  void          (* rotate)                    (GimpLayer              *layer,
                                               GimpContext            *context,
                                               GimpRotationType        rotate_type,
                                               gdouble                 center_x,
                                               gdouble                 center_y,
                                               gboolean                clip_result);
  void          (* transform)                 (GimpLayer              *layer,
                                               GimpContext            *context,
                                               const GimpMatrix3      *matrix,
                                               GimpTransformDirection  direction,
                                               GimpInterpolationType   interpolation_type,
                                               GimpTransformResize     clip_result,
                                               GimpProgress           *progress,
                                               gboolean                push_undo);
  void          (* convert_type)              (GimpLayer              *layer,
                                               GimpImage              *dest_image,
                                               const Babl             *new_format,
                                               GimpColorProfile       *src_profile,
                                               GimpColorProfile       *dest_profile,
                                               GeglDitherMethod        layer_dither_type,
                                               GeglDitherMethod        mask_dither_type,
                                               gboolean                push_undo,
                                               GimpProgress           *progress);
  GeglRectangle (* get_bounding_box)          (GimpLayer              *layer);
  void          (* get_effective_mode)        (GimpLayer              *layer,
                                               GimpLayerMode          *mode,
                                               GimpLayerColorSpace    *blend_space,
                                               GimpLayerColorSpace    *composite_space,
                                               GimpLayerCompositeMode *composite_mode);
  gboolean      (* get_excludes_backdrop)     (GimpLayer              *layer);
};


/*  function declarations  */

GType           gimp_layer_get_type            (void) G_GNUC_CONST;

void            gimp_layer_fix_format_space    (GimpLayer            *layer,
                                                gboolean              copy_buffer,
                                                gboolean              push_undo);

GimpLayer     * gimp_layer_get_parent          (GimpLayer            *layer);

GimpLayerMask * gimp_layer_get_mask            (GimpLayer            *layer);
GimpLayerMask * gimp_layer_create_mask         (GimpLayer            *layer,
                                                GimpAddMaskType       mask_type,
                                                GimpChannel          *channel);
GimpLayerMask * gimp_layer_add_mask            (GimpLayer            *layer,
                                                GimpLayerMask        *mask,
                                                gboolean              edit_mask,
                                                gboolean              push_undo,
                                                GError              **error);
void            gimp_layer_apply_mask          (GimpLayer            *layer,
                                                GimpMaskApplyMode     mode,
                                                gboolean              push_undo);

void            gimp_layer_set_apply_mask      (GimpLayer           *layer,
                                                gboolean             apply,
                                                gboolean             push_undo);
gboolean        gimp_layer_get_apply_mask      (GimpLayer           *layer);

void            gimp_layer_set_edit_mask       (GimpLayer           *layer,
                                                gboolean             edit);
gboolean        gimp_layer_get_edit_mask       (GimpLayer           *layer);

void            gimp_layer_set_show_mask       (GimpLayer           *layer,
                                                gboolean             show,
                                                gboolean             push_undo);
gboolean        gimp_layer_get_show_mask       (GimpLayer           *layer);

void            gimp_layer_add_alpha           (GimpLayer            *layer);
void            gimp_layer_remove_alpha        (GimpLayer            *layer,
                                                GimpContext          *context);

void            gimp_layer_resize_to_image     (GimpLayer            *layer,
                                                GimpContext          *context,
                                                GimpFillType          fill_type);

GimpDrawable * gimp_layer_get_floating_sel_drawable (GimpLayer       *layer);
void           gimp_layer_set_floating_sel_drawable (GimpLayer       *layer,
                                                     GimpDrawable    *drawable);
gboolean        gimp_layer_is_floating_sel     (GimpLayer            *layer);

void            gimp_layer_set_opacity         (GimpLayer            *layer,
                                                gdouble               opacity,
                                                gboolean              push_undo);
gdouble         gimp_layer_get_opacity         (GimpLayer            *layer);

void            gimp_layer_set_mode            (GimpLayer            *layer,
                                                GimpLayerMode         mode,
                                                gboolean              push_undo);
GimpLayerMode   gimp_layer_get_mode            (GimpLayer            *layer);

void            gimp_layer_set_blend_space     (GimpLayer            *layer,
                                                GimpLayerColorSpace   blend_space,
                                                gboolean              push_undo);
GimpLayerColorSpace
                gimp_layer_get_blend_space     (GimpLayer            *layer);
GimpLayerColorSpace
               gimp_layer_get_real_blend_space (GimpLayer            *layer);

void            gimp_layer_set_composite_space (GimpLayer            *layer,
                                                GimpLayerColorSpace   composite_space,
                                                gboolean              push_undo);
GimpLayerColorSpace
                gimp_layer_get_composite_space (GimpLayer            *layer);
GimpLayerColorSpace
           gimp_layer_get_real_composite_space (GimpLayer            *layer);

void            gimp_layer_set_composite_mode  (GimpLayer            *layer,
                                                GimpLayerCompositeMode  composite_mode,
                                                gboolean              push_undo);
GimpLayerCompositeMode
                gimp_layer_get_composite_mode  (GimpLayer            *layer);
GimpLayerCompositeMode
           gimp_layer_get_real_composite_mode  (GimpLayer            *layer);

void            gimp_layer_get_effective_mode  (GimpLayer            *layer,
                                                GimpLayerMode          *mode,
                                                GimpLayerColorSpace    *blend_space,
                                                GimpLayerColorSpace    *composite_space,
                                                GimpLayerCompositeMode *composite_mode);

gboolean      gimp_layer_get_excludes_backdrop (GimpLayer            *layer);

void            gimp_layer_set_lock_alpha      (GimpLayer            *layer,
                                                gboolean              lock_alpha,
                                                gboolean              push_undo);
gboolean        gimp_layer_get_lock_alpha      (GimpLayer            *layer);
gboolean        gimp_layer_can_lock_alpha      (GimpLayer            *layer);
gboolean        gimp_layer_is_alpha_locked     (GimpLayer            *layer,
                                                GimpLayer           **locked_layer);


/*  protected  */

void          gimp_layer_update_effective_mode (GimpLayer            *layer);
void       gimp_layer_update_excludes_backdrop (GimpLayer            *layer);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GimpLayer, g_object_unref);
