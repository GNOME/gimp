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

#ifndef __LIGMA_ITEM_H__
#define __LIGMA_ITEM_H__


#include "ligmafilter.h"


#define LIGMA_TYPE_ITEM            (ligma_item_get_type ())
#define LIGMA_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ITEM, LigmaItem))
#define LIGMA_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ITEM, LigmaItemClass))
#define LIGMA_IS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ITEM))
#define LIGMA_IS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ITEM))
#define LIGMA_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ITEM, LigmaItemClass))


typedef struct _LigmaItemClass LigmaItemClass;

struct _LigmaItem
{
  LigmaFilter  parent_instance;
};

struct _LigmaItemClass
{
  LigmaFilterClass  parent_class;

  /*  signals  */
  void            (* removed)               (LigmaItem            *item);
  void            (* visibility_changed)    (LigmaItem            *item);
  void            (* color_tag_changed)     (LigmaItem            *item);
  void            (* lock_content_changed)  (LigmaItem            *item);
  void            (* lock_position_changed) (LigmaItem            *item);
  void            (* lock_visibility_changed) (LigmaItem            *item);

  /*  virtual functions  */
  void            (* unset_removed)      (LigmaItem               *item);
  gboolean        (* is_attached)        (LigmaItem               *item);
  gboolean        (* is_content_locked)  (LigmaItem               *item,
                                          LigmaItem              **locked_item);
  gboolean        (* is_position_locked) (LigmaItem               *item,
                                          LigmaItem              **locked_item,
                                          gboolean                checking_children);
  gboolean        (* is_visibility_locked) (LigmaItem               *item,
                                            LigmaItem              **locked_item);
  LigmaItemTree  * (* get_tree)           (LigmaItem               *item);
  gboolean        (* bounds)             (LigmaItem               *item,
                                          gdouble                *x,
                                          gdouble                *y,
                                          gdouble                *width,
                                          gdouble                *height);
  LigmaItem      * (* duplicate)          (LigmaItem               *item,
                                          GType                   new_type);
  void            (* convert)            (LigmaItem               *item,
                                          LigmaImage              *dest_image,
                                          GType                   old_type);
  gboolean        (* rename)             (LigmaItem               *item,
                                          const gchar            *new_name,
                                          const gchar            *undo_desc,
                                          GError                **error);
  void            (* start_move)         (LigmaItem               *item,
                                          gboolean                push_undo);
  void            (* end_move)           (LigmaItem               *item,
                                          gboolean                push_undo);
  void            (* start_transform)    (LigmaItem               *item,
                                          gboolean                push_undo);
  void            (* end_transform)      (LigmaItem               *item,
                                          gboolean                push_undo);
  void            (* translate)          (LigmaItem               *item,
                                          gdouble                 offset_x,
                                          gdouble                 offset_y,
                                          gboolean                push_undo);
  void            (* scale)              (LigmaItem               *item,
                                          gint                    new_width,
                                          gint                    new_height,
                                          gint                    new_offset_x,
                                          gint                    new_offset_y,
                                          LigmaInterpolationType   interpolation_type,
                                          LigmaProgress           *progress);
  void            (* resize)             (LigmaItem               *item,
                                          LigmaContext            *context,
                                          LigmaFillType            fill_type,
                                          gint                    new_width,
                                          gint                    new_height,
                                          gint                    offset_x,
                                          gint                    offset_y);
  void            (* flip)               (LigmaItem               *item,
                                          LigmaContext            *context,
                                          LigmaOrientationType     flip_type,
                                          gdouble                 axis,
                                          gboolean                clip_result);
  void            (* rotate)             (LigmaItem               *item,
                                          LigmaContext            *context,
                                          LigmaRotationType        rotate_type,
                                          gdouble                 center_x,
                                          gdouble                 center_y,
                                          gboolean                clip_result);
  void            (* transform)          (LigmaItem               *item,
                                          LigmaContext            *context,
                                          const LigmaMatrix3      *matrix,
                                          LigmaTransformDirection  direction,
                                          LigmaInterpolationType   interpolation_type,
                                          LigmaTransformResize     clip_result,
                                          LigmaProgress           *progress);
  LigmaTransformResize (* get_clip)       (LigmaItem               *item,
                                          LigmaTransformResize     clip_result);
  gboolean        (* fill)               (LigmaItem               *item,
                                          LigmaDrawable           *drawable,
                                          LigmaFillOptions        *fill_options,
                                          gboolean                push_undo,
                                          LigmaProgress           *progress,
                                          GError                **error);
  gboolean        (* stroke)             (LigmaItem               *item,
                                          LigmaDrawable           *drawable,
                                          LigmaStrokeOptions      *stroke_options,
                                          gboolean                push_undo,
                                          LigmaProgress           *progress,
                                          GError                **error);
  void            (* to_selection)       (LigmaItem               *item,
                                          LigmaChannelOps          op,
                                          gboolean                antialias,
                                          gboolean                feather,
                                          gdouble                 feather_radius_x,
                                          gdouble                 feather_radius_y);

  const gchar *default_name;
  const gchar *rename_desc;
  const gchar *translate_desc;
  const gchar *scale_desc;
  const gchar *resize_desc;
  const gchar *flip_desc;
  const gchar *rotate_desc;
  const gchar *transform_desc;
  const gchar *to_selection_desc;
  const gchar *fill_desc;
  const gchar *stroke_desc;

  const gchar *reorder_desc;
  const gchar *raise_desc;
  const gchar *raise_to_top_desc;
  const gchar *lower_desc;
  const gchar *lower_to_bottom_desc;

  const gchar *raise_failed;
  const gchar *lower_failed;
};


GType           ligma_item_get_type           (void) G_GNUC_CONST;

LigmaItem      * ligma_item_new                (GType               type,
                                              LigmaImage          *image,
                                              const gchar        *name,
                                              gint                offset_x,
                                              gint                offset_y,
                                              gint                width,
                                              gint                height);

void            ligma_item_removed            (LigmaItem           *item);
gboolean        ligma_item_is_removed         (LigmaItem           *item);
void            ligma_item_unset_removed      (LigmaItem           *item);

gboolean        ligma_item_is_attached        (LigmaItem           *item);

LigmaItem      * ligma_item_get_parent         (LigmaItem           *item);

LigmaItemTree  * ligma_item_get_tree           (LigmaItem           *item);
LigmaContainer * ligma_item_get_container      (LigmaItem           *item);
GList         * ligma_item_get_container_iter (LigmaItem           *item);
gint            ligma_item_get_index          (LigmaItem           *item);
GList         * ligma_item_get_path           (LigmaItem           *item);

gboolean        ligma_item_bounds             (LigmaItem           *item,
                                              gint               *x,
                                              gint               *y,
                                              gint               *width,
                                              gint               *height);
gboolean        ligma_item_bounds_f           (LigmaItem           *item,
                                              gdouble            *x,
                                              gdouble            *y,
                                              gdouble            *width,
                                              gdouble            *height);

LigmaItem      * ligma_item_duplicate          (LigmaItem           *item,
                                              GType               new_type);
LigmaItem      * ligma_item_convert            (LigmaItem           *item,
                                              LigmaImage          *dest_image,
                                              GType               new_type);

gboolean        ligma_item_rename             (LigmaItem           *item,
                                              const gchar        *new_name,
                                              GError            **error);

gint            ligma_item_get_width          (LigmaItem           *item);
gint            ligma_item_get_height         (LigmaItem           *item);
void            ligma_item_set_size           (LigmaItem           *item,
                                              gint                width,
                                              gint                height);

void            ligma_item_get_offset         (LigmaItem           *item,
                                              gint               *offset_x,
                                              gint               *offset_y);
void            ligma_item_set_offset         (LigmaItem           *item,
                                              gint                offset_x,
                                              gint                offset_y);
gint            ligma_item_get_offset_x       (LigmaItem           *item);
gint            ligma_item_get_offset_y       (LigmaItem           *item);

void            ligma_item_start_move         (LigmaItem           *item,
                                              gboolean            push_undo);
void            ligma_item_end_move           (LigmaItem           *item,
                                              gboolean            push_undo);

void            ligma_item_start_transform    (LigmaItem           *item,
                                              gboolean            push_undo);
void            ligma_item_end_transform      (LigmaItem           *item,
                                              gboolean            push_undo);

void            ligma_item_translate          (LigmaItem           *item,
                                              gdouble             offset_x,
                                              gdouble             offset_y,
                                              gboolean            push_undo);

gboolean        ligma_item_check_scaling      (LigmaItem           *item,
                                              gint                new_width,
                                              gint                new_height);
void            ligma_item_scale              (LigmaItem           *item,
                                              gint                new_width,
                                              gint                new_height,
                                              gint                new_offset_x,
                                              gint                new_offset_y,
                                              LigmaInterpolationType interpolation,
                                              LigmaProgress       *progress);
gboolean        ligma_item_scale_by_factors   (LigmaItem           *item,
                                              gdouble             w_factor,
                                              gdouble             h_factor,
                                              LigmaInterpolationType interpolation,
                                              LigmaProgress       *progress);
gboolean
      ligma_item_scale_by_factors_with_origin (LigmaItem           *item,
                                              gdouble             w_factor,
                                              gdouble             h_factor,
                                              gint                origin_x,
                                              gint                origin_y,
                                              gint                new_origin_x,
                                              gint                new_origin_y,
                                              LigmaInterpolationType interpolation,
                                              LigmaProgress       *progress);
void            ligma_item_scale_by_origin    (LigmaItem           *item,
                                              gint                new_width,
                                              gint                new_height,
                                              LigmaInterpolationType interpolation,
                                              LigmaProgress       *progress,
                                              gboolean            local_origin);
void            ligma_item_resize             (LigmaItem           *item,
                                              LigmaContext        *context,
                                              LigmaFillType        fill_type,
                                              gint                new_width,
                                              gint                new_height,
                                              gint                offset_x,
                                              gint                offset_y);
void            ligma_item_resize_to_image    (LigmaItem           *item);

void            ligma_item_flip               (LigmaItem           *item,
                                              LigmaContext        *context,
                                              LigmaOrientationType flip_type,
                                              gdouble             axis,
                                              gboolean            clip_result);
void            ligma_item_rotate             (LigmaItem           *item,
                                              LigmaContext        *context,
                                              LigmaRotationType    rotate_type,
                                              gdouble             center_x,
                                              gdouble             center_y,
                                              gboolean            clip_result);
void            ligma_item_transform          (LigmaItem           *item,
                                              LigmaContext        *context,
                                              const LigmaMatrix3  *matrix,
                                              LigmaTransformDirection direction,
                                              LigmaInterpolationType interpolation_type,
                                              LigmaTransformResize clip_result,
                                              LigmaProgress       *progress);
LigmaTransformResize   ligma_item_get_clip     (LigmaItem           *item,
                                              LigmaTransformResize clip_result);

gboolean        ligma_item_fill               (LigmaItem           *item,
                                              GList              *drawables,
                                              LigmaFillOptions    *fill_options,
                                              gboolean            push_undo,
                                              LigmaProgress       *progress,
                                              GError            **error);
gboolean        ligma_item_stroke             (LigmaItem           *item,
                                              GList              *drawables,
                                              LigmaContext        *context,
                                              LigmaStrokeOptions  *stroke_options,
                                              LigmaPaintOptions   *paint_options,
                                              gboolean            push_undo,
                                              LigmaProgress       *progress,
                                              GError            **error);

void            ligma_item_to_selection       (LigmaItem           *item,
                                              LigmaChannelOps      op,
                                              gboolean            antialias,
                                              gboolean            feather,
                                              gdouble             feather_radius_x,
                                              gdouble             feather_radius_y);

void            ligma_item_add_offset_node    (LigmaItem           *item,
                                              GeglNode           *node);
void            ligma_item_remove_offset_node (LigmaItem           *item,
                                              GeglNode           *node);

gint            ligma_item_get_id             (LigmaItem           *item);
LigmaItem      * ligma_item_get_by_id          (Ligma               *ligma,
                                              gint                id);

LigmaTattoo      ligma_item_get_tattoo         (LigmaItem           *item);
void            ligma_item_set_tattoo         (LigmaItem           *item,
                                              LigmaTattoo          tattoo);

LigmaImage     * ligma_item_get_image          (LigmaItem           *item);
void            ligma_item_set_image          (LigmaItem           *item,
                                              LigmaImage          *image);

void            ligma_item_replace_item       (LigmaItem           *item,
                                              LigmaItem           *replace);

void               ligma_item_set_parasites   (LigmaItem           *item,
                                              LigmaParasiteList   *parasites);
LigmaParasiteList * ligma_item_get_parasites   (LigmaItem           *item);

gboolean        ligma_item_parasite_validate  (LigmaItem           *item,
                                              const LigmaParasite *parasite,
                                              GError            **error);
void            ligma_item_parasite_attach    (LigmaItem           *item,
                                              const LigmaParasite *parasite,
                                              gboolean            push_undo);
void            ligma_item_parasite_detach    (LigmaItem           *item,
                                              const gchar        *name,
                                              gboolean            push_undo);
const LigmaParasite * ligma_item_parasite_find (LigmaItem           *item,
                                              const gchar        *name);
gchar        ** ligma_item_parasite_list      (LigmaItem           *item);

gboolean        ligma_item_set_visible        (LigmaItem           *item,
                                              gboolean            visible,
                                              gboolean            push_undo);
gboolean        ligma_item_get_visible        (LigmaItem           *item);
gboolean        ligma_item_is_visible         (LigmaItem           *item);

void        ligma_item_bind_visible_to_active (LigmaItem           *item,
                                              gboolean            bind);

void            ligma_item_set_color_tag      (LigmaItem           *item,
                                              LigmaColorTag        color_tag,
                                              gboolean            push_undo);
LigmaColorTag    ligma_item_get_color_tag      (LigmaItem           *item);
LigmaColorTag  ligma_item_get_merged_color_tag (LigmaItem           *item);

void            ligma_item_set_lock_content   (LigmaItem           *item,
                                              gboolean            lock_content,
                                              gboolean            push_undo);
gboolean        ligma_item_get_lock_content   (LigmaItem           *item);
gboolean        ligma_item_can_lock_content   (LigmaItem           *item);
gboolean        ligma_item_is_content_locked  (LigmaItem           *item,
                                              LigmaItem          **locked_item);

void            ligma_item_set_lock_position  (LigmaItem          *item,
                                              gboolean           lock_position,
                                              gboolean           push_undo);
gboolean        ligma_item_get_lock_position  (LigmaItem          *item);
gboolean        ligma_item_can_lock_position  (LigmaItem          *item);
gboolean        ligma_item_is_position_locked (LigmaItem          *item,
                                              LigmaItem         **locked_item);

void            ligma_item_set_lock_visibility  (LigmaItem        *item,
                                                gboolean         lock_visibility,
                                                gboolean         push_undo);
gboolean        ligma_item_get_lock_visibility  (LigmaItem        *item);
gboolean        ligma_item_can_lock_visibility  (LigmaItem        *item);
gboolean        ligma_item_is_visibility_locked (LigmaItem        *item,
                                                LigmaItem       **locked_item);

gboolean        ligma_item_mask_bounds        (LigmaItem           *item,
                                              gint               *x1,
                                              gint               *y1,
                                              gint               *x2,
                                              gint               *y2);
gboolean        ligma_item_mask_intersect     (LigmaItem           *item,
                                              gint               *x,
                                              gint               *y,
                                              gint               *width,
                                              gint               *height);

gboolean        ligma_item_is_in_set          (LigmaItem           *item,
                                              LigmaItemSet         set);


#endif /* __LIGMA_ITEM_H__ */
