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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
  GimpDrawable   parent_instance;

  gdouble        opacity;          /*  layer opacity              */
  GimpLayerMode  mode;             /*  layer combination mode     */
  gboolean       lock_alpha;       /*  lock the alpha channel     */

  GimpLayerMask *mask;             /*  possible layer mask        */
  gboolean       apply_mask;       /*  controls mask application  */
  gboolean       edit_mask;        /*  edit mask or layer?        */
  gboolean       show_mask;        /*  show mask or layer?        */

  GeglNode      *layer_offset_node;
  GeglNode      *mask_offset_node;

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

  void (* opacity_changed)    (GimpLayer *layer);
  void (* mode_changed)       (GimpLayer *layer);
  void (* lock_alpha_changed) (GimpLayer *layer);
  void (* mask_changed)       (GimpLayer *layer);
  void (* apply_mask_changed) (GimpLayer *layer);
  void (* edit_mask_changed)  (GimpLayer *layer);
  void (* show_mask_changed)  (GimpLayer *layer);
};


/*  function declarations  */

GType           gimp_layer_get_type            (void) G_GNUC_CONST;

GimpLayer     * gimp_layer_get_parent          (GimpLayer            *layer);

GimpLayerMask * gimp_layer_get_mask            (GimpLayer            *layer);
GimpLayerMask * gimp_layer_create_mask         (GimpLayer            *layer,
                                                GimpAddMaskType       mask_type,
                                                GimpChannel          *channel);
GimpLayerMask * gimp_layer_add_mask            (GimpLayer            *layer,
                                                GimpLayerMask        *mask,
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

void            gimp_layer_set_lock_alpha      (GimpLayer            *layer,
                                                gboolean              lock_alpha,
                                                gboolean              push_undo);
gboolean        gimp_layer_get_lock_alpha      (GimpLayer            *layer);
gboolean        gimp_layer_can_lock_alpha      (GimpLayer            *layer);


#endif /* __GIMP_LAYER_H__ */
