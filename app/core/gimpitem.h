/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_ITEM_H__
#define __GIMP_ITEM_H__


#include "gimpviewable.h"


#define GIMP_TYPE_ITEM            (gimp_item_get_type ())
#define GIMP_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ITEM, GimpItem))
#define GIMP_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ITEM, GimpItemClass))
#define GIMP_IS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ITEM))
#define GIMP_IS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ITEM))
#define GIMP_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ITEM, GimpItemClass))


typedef struct _GimpItemClass GimpItemClass;

struct _GimpItem
{
  GimpViewable      parent_instance;

  gint              ID;                 /*  provides a unique ID     */
  guint32           tattoo;             /*  provides a permanent ID  */

  GimpImage        *image;              /*  item owner               */

  GimpParasiteList *parasites;          /*  Plug-in parasite data    */

  gint              width, height;      /*  size in pixels           */
  gint              offset_x, offset_y; /*  pixel offset in image    */

  gboolean          visible;            /*  control visibility       */
  gboolean          linked;             /*  control linkage          */

  gboolean          removed;            /*  removed from the image?  */
};

struct _GimpItemClass
{
  GimpViewableClass  parent_class;

  /*  signals  */
  void       (* removed)            (GimpItem      *item);
  void       (* visibility_changed) (GimpItem      *item);
  void       (* linked_changed)     (GimpItem      *item);

  /*  virtual functions  */
  gboolean   (* is_attached)  (GimpItem               *item);
  GimpItem * (* duplicate)    (GimpItem               *item,
                               GType                   new_type,
                               gboolean                add_alpha);
  void       (* convert)      (GimpItem               *item,
                               GimpImage              *dest_image);
  gboolean   (* rename)       (GimpItem               *item,
                               const gchar            *new_name,
                               const gchar            *undo_desc);
  void       (* translate)    (GimpItem               *item,
                               gint                    offset_x,
                               gint                    offset_y,
                               gboolean                push_undo);
  void       (* scale)        (GimpItem               *item,
                               gint                    new_width,
                               gint                    new_height,
                               gint                    new_offset_x,
                               gint                    new_offset_y,
                               GimpInterpolationType   interpolation_type,
                               GimpProgress           *progress);
  void       (* resize)       (GimpItem               *item,
                               GimpContext            *context,
                               gint                    new_width,
                               gint                    new_height,
                               gint                    offset_x,
                               gint                    offset_y);
  void       (* flip)         (GimpItem               *item,
                               GimpContext            *context,
                               GimpOrientationType     flip_type,
                               gdouble                 axis,
                               gboolean                clip_result);
  void       (* rotate)       (GimpItem               *item,
                               GimpContext            *context,
                               GimpRotationType        rotate_type,
                               gdouble                 center_x,
                               gdouble                 center_y,
                               gboolean                clip_result);
  void       (* transform)    (GimpItem               *item,
                               GimpContext            *context,
                               const GimpMatrix3      *matrix,
                               GimpTransformDirection  direction,
                               GimpInterpolationType   interpolation_type,
                               gboolean                supersample,
                               gint                    recursion_level,
                               GimpTransformResize     clip_result,
                               GimpProgress           *progress);
  gboolean   (* stroke)       (GimpItem               *item,
                               GimpDrawable           *drawable,
                               GimpStrokeDesc         *stroke_desc,
                               GimpProgress           *progress);

  const gchar *default_name;
  const gchar *rename_desc;
  const gchar *translate_desc;
  const gchar *scale_desc;
  const gchar *resize_desc;
  const gchar *flip_desc;
  const gchar *rotate_desc;
  const gchar *transform_desc;
  const gchar *stroke_desc;
};


GType           gimp_item_get_type         (void) G_GNUC_CONST;

void            gimp_item_removed          (GimpItem           *item);
gboolean        gimp_item_is_removed       (const GimpItem     *item);

gboolean        gimp_item_is_attached      (GimpItem           *item);

void            gimp_item_configure        (GimpItem           *item,
                                            GimpImage          *image,
                                            gint                offset_x,
                                            gint                offset_y,
                                            gint                width,
                                            gint                height,
                                            const gchar        *name);
GimpItem      * gimp_item_duplicate        (GimpItem           *item,
                                            GType               new_type,
                                            gboolean            add_alpha);
GimpItem      * gimp_item_convert          (GimpItem           *item,
                                            GimpImage          *dest_image,
                                            GType               new_type,
                                            gboolean            add_alpha);

gboolean        gimp_item_rename           (GimpItem           *item,
                                            const gchar        *new_name);

gint            gimp_item_width            (const GimpItem     *item);
gint            gimp_item_height           (const GimpItem     *item);
void            gimp_item_offsets          (const GimpItem     *item,
                                            gint               *offset_x,
                                            gint               *offset_y);

void            gimp_item_translate        (GimpItem           *item,
                                            gint                offset_x,
                                            gint                offset_y,
                                            gboolean            push_undo);

gboolean        gimp_item_check_scaling    (const GimpItem     *item,
                                            gint                new_width,
                                            gint                new_height);
void            gimp_item_scale            (GimpItem           *item,
                                            gint                new_width,
                                            gint                new_height,
                                            gint                new_offset_x,
                                            gint                new_offset_y,
                                            GimpInterpolationType interpolation,
                                            GimpProgress       *progress);
gboolean        gimp_item_scale_by_factors (GimpItem           *item,
                                            gdouble             w_factor,
                                            gdouble             h_factor,
                                            GimpInterpolationType interpolation,
                                            GimpProgress       *progress);
void            gimp_item_scale_by_origin  (GimpItem           *item,
                                            gint                new_width,
                                            gint                new_height,
                                            GimpInterpolationType interpolation,
                                            GimpProgress       *progress,
                                            gboolean            local_origin);
void            gimp_item_resize           (GimpItem           *item,
                                            GimpContext        *context,
                                            gint                new_width,
                                            gint                new_height,
                                            gint                offset_x,
                                            gint                offset_y);
void            gimp_item_resize_to_image  (GimpItem           *item);

void            gimp_item_flip             (GimpItem           *item,
                                            GimpContext        *context,
                                            GimpOrientationType flip_type,
                                            gdouble             axis,
                                            gboolean            flip_result);
void            gimp_item_rotate           (GimpItem           *item,
                                            GimpContext        *context,
                                            GimpRotationType    rotate_type,
                                            gdouble             center_x,
                                            gdouble             center_y,
                                            gboolean            flip_result);
void            gimp_item_transform        (GimpItem           *item,
                                            GimpContext        *context,
                                            const GimpMatrix3  *matrix,
                                            GimpTransformDirection direction,
                                            GimpInterpolationType interpolation_type,
                                            gboolean            supersample,
                                            gint                recursion_level,
                                            GimpTransformResize clip_result,
                                            GimpProgress       *progress);

gboolean        gimp_item_stroke           (GimpItem           *item,
                                            GimpDrawable       *drawable,
                                            GimpContext        *context,
                                            GimpStrokeDesc     *stroke_desc,
                                            gboolean            use_default_values,
                                            GimpProgress       *progress);

gint            gimp_item_get_ID           (GimpItem           *item);
GimpItem      * gimp_item_get_by_ID        (Gimp               *gimp,
                                            gint                id);

GimpTattoo      gimp_item_get_tattoo       (const GimpItem     *item);
void            gimp_item_set_tattoo       (GimpItem           *item,
                                            GimpTattoo          tattoo);

GimpImage     * gimp_item_get_image        (const GimpItem     *item);
void            gimp_item_set_image        (GimpItem           *item,
                                            GimpImage          *image);

void            gimp_item_parasite_attach  (GimpItem           *item,
                                            const GimpParasite *parasite);
void            gimp_item_parasite_detach  (GimpItem           *item,
                                            const gchar        *name);
const GimpParasite *gimp_item_parasite_find(const GimpItem     *item,
                                            const gchar        *name);
gchar        ** gimp_item_parasite_list    (const GimpItem     *item,
                                            gint               *count);

gboolean        gimp_item_get_visible      (const GimpItem     *item);
void            gimp_item_set_visible      (GimpItem           *item,
                                            gboolean            visible,
                                            gboolean            push_undo);

void            gimp_item_set_linked       (GimpItem           *item,
                                            gboolean            linked,
                                            gboolean            push_undo);
gboolean        gimp_item_get_linked       (const GimpItem     *item);

gboolean        gimp_item_is_in_set        (GimpItem           *item,
                                            GimpItemSet         set);


#endif /* __GIMP_ITEM_H__ */
