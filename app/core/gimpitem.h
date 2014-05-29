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

#ifndef __GIMP_ITEM_H__
#define __GIMP_ITEM_H__


#include "gimpfilter.h"


#define GIMP_TYPE_ITEM            (gimp_item_get_type ())
#define GIMP_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ITEM, GimpItem))
#define GIMP_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ITEM, GimpItemClass))
#define GIMP_IS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ITEM))
#define GIMP_IS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ITEM))
#define GIMP_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ITEM, GimpItemClass))


typedef struct _GimpItemClass GimpItemClass;

struct _GimpItem
{
  GimpFilter  parent_instance;
};

struct _GimpItemClass
{
  GimpFilterClass  parent_class;

  /*  signals  */
  void            (* removed)               (GimpItem            *item);
  void            (* visibility_changed)    (GimpItem            *item);
  void            (* linked_changed)        (GimpItem            *item);
  void            (* lock_content_changed)  (GimpItem            *item);
  void            (* lock_position_changed) (GimpItem            *item);

  /*  virtual functions  */
  void            (* unset_removed)      (GimpItem               *item);
  gboolean        (* is_attached)        (const GimpItem         *item);
  gboolean        (* is_content_locked)  (const GimpItem         *item);
  gboolean        (* is_position_locked) (const GimpItem         *item);
  GimpItemTree  * (* get_tree)           (GimpItem               *item);
  GimpItem      * (* duplicate)          (GimpItem               *item,
                                          GType                   new_type);
  void            (* convert)            (GimpItem               *item,
                                          GimpImage              *dest_image);
  gboolean        (* rename)             (GimpItem               *item,
                                          const gchar            *new_name,
                                          const gchar            *undo_desc,
                                          GError                **error);
  void            (* translate)          (GimpItem               *item,
                                          gint                    offset_x,
                                          gint                    offset_y,
                                          gboolean                push_undo);
  void            (* scale)              (GimpItem               *item,
                                          gint                    new_width,
                                          gint                    new_height,
                                          gint                    new_offset_x,
                                          gint                    new_offset_y,
                                          GimpInterpolationType   interpolation_type,
                                          GimpProgress           *progress);
  void            (* resize)             (GimpItem               *item,
                                          GimpContext            *context,
                                          gint                    new_width,
                                          gint                    new_height,
                                          gint                    offset_x,
                                          gint                    offset_y);
  void            (* flip)               (GimpItem               *item,
                                          GimpContext            *context,
                                          GimpOrientationType     flip_type,
                                          gdouble                 axis,
                                          gboolean                clip_result);
  void            (* rotate)             (GimpItem               *item,
                                          GimpContext            *context,
                                          GimpRotationType        rotate_type,
                                          gdouble                 center_x,
                                          gdouble                 center_y,
                                          gboolean                clip_result);
  void            (* transform)          (GimpItem               *item,
                                          GimpContext            *context,
                                          const GimpMatrix3      *matrix,
                                          GimpTransformDirection  direction,
                                          GimpInterpolationType   interpolation_type,
                                          GimpTransformResize     clip_result,
                                          GimpProgress           *progress);
  gboolean        (* stroke)             (GimpItem               *item,
                                          GimpDrawable           *drawable,
                                          GimpStrokeOptions      *stroke_options,
                                          gboolean                push_undo,
                                          GimpProgress           *progress,
                                          GError                **error);
  void            (* to_selection)       (GimpItem               *item,
                                          GimpChannelOps          op,
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
  const gchar *stroke_desc;

  const gchar *reorder_desc;
  const gchar *raise_desc;
  const gchar *raise_to_top_desc;
  const gchar *lower_desc;
  const gchar *lower_to_bottom_desc;

  const gchar *raise_failed;
  const gchar *lower_failed;
};


GType           gimp_item_get_type           (void) G_GNUC_CONST;

GimpItem      * gimp_item_new                (GType               type,
                                              GimpImage          *image,
                                              const gchar        *name,
                                              gint                offset_x,
                                              gint                offset_y,
                                              gint                width,
                                              gint                height);

void            gimp_item_removed            (GimpItem           *item);
gboolean        gimp_item_is_removed         (const GimpItem     *item);
void            gimp_item_unset_removed      (GimpItem           *item);

gboolean        gimp_item_is_attached        (const GimpItem     *item);

GimpItem      * gimp_item_get_parent         (const GimpItem     *item);

GimpItemTree  * gimp_item_get_tree           (GimpItem           *item);
GimpContainer * gimp_item_get_container      (GimpItem           *item);
GList         * gimp_item_get_container_iter (GimpItem           *item);
gint            gimp_item_get_index          (GimpItem           *item);
GList         * gimp_item_get_path           (GimpItem           *item);

GimpItem      * gimp_item_duplicate          (GimpItem           *item,
                                              GType               new_type);
GimpItem      * gimp_item_convert            (GimpItem           *item,
                                              GimpImage          *dest_image,
                                              GType               new_type);

gboolean        gimp_item_rename             (GimpItem           *item,
                                              const gchar        *new_name,
                                              GError            **error);

gint            gimp_item_get_width          (const GimpItem     *item);
gint            gimp_item_get_height         (const GimpItem     *item);
void            gimp_item_set_size           (GimpItem           *item,
                                              gint                width,
                                              gint                height);

void            gimp_item_get_offset         (const GimpItem     *item,
                                              gint               *offset_x,
                                              gint               *offset_y);
void            gimp_item_set_offset         (GimpItem           *item,
                                              gint                offset_x,
                                              gint                offset_y);
gint            gimp_item_get_offset_x       (GimpItem           *item);
gint            gimp_item_get_offset_y       (GimpItem           *item);

void            gimp_item_translate          (GimpItem           *item,
                                              gint                offset_x,
                                              gint                offset_y,
                                              gboolean            push_undo);

gboolean        gimp_item_check_scaling      (const GimpItem     *item,
                                              gint                new_width,
                                              gint                new_height);
void            gimp_item_scale              (GimpItem           *item,
                                              gint                new_width,
                                              gint                new_height,
                                              gint                new_offset_x,
                                              gint                new_offset_y,
                                              GimpInterpolationType interpolation,
                                              GimpProgress       *progress);
gboolean        gimp_item_scale_by_factors   (GimpItem           *item,
                                              gdouble             w_factor,
                                              gdouble             h_factor,
                                              GimpInterpolationType interpolation,
                                              GimpProgress       *progress);
void            gimp_item_scale_by_origin    (GimpItem           *item,
                                              gint                new_width,
                                              gint                new_height,
                                              GimpInterpolationType interpolation,
                                              GimpProgress       *progress,
                                              gboolean            local_origin);
void            gimp_item_resize             (GimpItem           *item,
                                              GimpContext        *context,
                                              gint                new_width,
                                              gint                new_height,
                                              gint                offset_x,
                                              gint                offset_y);
void            gimp_item_resize_to_image    (GimpItem           *item);

void            gimp_item_flip               (GimpItem           *item,
                                              GimpContext        *context,
                                              GimpOrientationType flip_type,
                                              gdouble             axis,
                                              gboolean            flip_result);
void            gimp_item_rotate             (GimpItem           *item,
                                              GimpContext        *context,
                                              GimpRotationType    rotate_type,
                                              gdouble             center_x,
                                              gdouble             center_y,
                                              gboolean            flip_result);
void            gimp_item_transform          (GimpItem           *item,
                                              GimpContext        *context,
                                              const GimpMatrix3  *matrix,
                                              GimpTransformDirection direction,
                                              GimpInterpolationType interpolation_type,
                                              GimpTransformResize clip_result,
                                              GimpProgress       *progress);

gboolean        gimp_item_stroke             (GimpItem           *item,
                                              GimpDrawable       *drawable,
                                              GimpContext        *context,
                                              GimpStrokeOptions  *stroke_options,
                                              GimpPaintOptions   *paint_options,
                                              gboolean            push_undo,
                                              GimpProgress       *progress,
                                              GError            **error);

void            gimp_item_to_selection       (GimpItem           *item,
                                              GimpChannelOps      op,
                                              gboolean            antialias,
                                              gboolean            feather,
                                              gdouble             feather_radius_x,
                                              gdouble             feather_radius_y);

void            gimp_item_add_offset_node    (GimpItem           *item,
                                              GeglNode           *node);
void            gimp_item_remove_offset_node (GimpItem           *item,
                                              GeglNode           *node);

gint            gimp_item_get_ID             (GimpItem           *item);
GimpItem      * gimp_item_get_by_ID          (Gimp               *gimp,
                                              gint                id);

GimpTattoo      gimp_item_get_tattoo         (const GimpItem     *item);
void            gimp_item_set_tattoo         (GimpItem           *item,
                                              GimpTattoo          tattoo);

GimpImage     * gimp_item_get_image          (const GimpItem     *item);
void            gimp_item_set_image          (GimpItem           *item,
                                              GimpImage          *image);

void            gimp_item_replace_item       (GimpItem           *item,
                                              GimpItem           *replace);

void               gimp_item_set_parasites   (GimpItem           *item,
                                              GimpParasiteList   *parasites);
GimpParasiteList * gimp_item_get_parasites   (const GimpItem     *item);

gboolean        gimp_item_parasite_validate  (GimpItem           *item,
                                              const GimpParasite *parasite,
                                              GError            **error);
void            gimp_item_parasite_attach    (GimpItem           *item,
                                              const GimpParasite *parasite,
                                              gboolean            push_undo);
void            gimp_item_parasite_detach    (GimpItem           *item,
                                              const gchar        *name,
                                              gboolean            push_undo);
const GimpParasite * gimp_item_parasite_find (const GimpItem     *item,
                                              const gchar        *name);
gchar        ** gimp_item_parasite_list      (const GimpItem     *item,
                                              gint               *count);

void            gimp_item_set_visible        (GimpItem           *item,
                                              gboolean            visible,
                                              gboolean            push_undo);
gboolean        gimp_item_get_visible        (const GimpItem     *item);
gboolean        gimp_item_is_visible         (const GimpItem     *item);

void            gimp_item_set_linked         (GimpItem           *item,
                                              gboolean            linked,
                                              gboolean            push_undo);
gboolean        gimp_item_get_linked         (const GimpItem     *item);

void            gimp_item_set_lock_content   (GimpItem           *item,
                                              gboolean            lock_content,
                                              gboolean            push_undo);
gboolean        gimp_item_get_lock_content   (const GimpItem     *item);
gboolean        gimp_item_can_lock_content   (const GimpItem     *item);
gboolean        gimp_item_is_content_locked  (const GimpItem     *item);

void            gimp_item_set_lock_position  (GimpItem          *item,
                                              gboolean           lock_position,
                                              gboolean           push_undo);
gboolean        gimp_item_get_lock_position  (const GimpItem    *item);
gboolean        gimp_item_can_lock_position  (const GimpItem    *item);
gboolean        gimp_item_is_position_locked (const GimpItem    *item);

gboolean        gimp_item_mask_bounds        (GimpItem           *item,
                                              gint               *x1,
                                              gint               *y1,
                                              gint               *x2,
                                              gint               *y2);
gboolean        gimp_item_mask_intersect     (GimpItem           *item,
                                              gint               *x,
                                              gint               *y,
                                              gint               *width,
                                              gint               *height);

gboolean        gimp_item_is_in_set          (GimpItem           *item,
                                              GimpItemSet         set);


#endif /* __GIMP_ITEM_H__ */
