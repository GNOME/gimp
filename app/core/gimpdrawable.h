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

#ifndef __GIMP_DRAWABLE_H__
#define __GIMP_DRAWABLE_H__


#include "gimpitem.h"


#define GIMP_TYPE_DRAWABLE            (gimp_drawable_get_type ())
#define GIMP_DRAWABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DRAWABLE, GimpDrawable))
#define GIMP_DRAWABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DRAWABLE, GimpDrawableClass))
#define GIMP_IS_DRAWABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DRAWABLE))
#define GIMP_IS_DRAWABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DRAWABLE))
#define GIMP_DRAWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DRAWABLE, GimpDrawableClass))


typedef struct _GimpDrawablePrivate GimpDrawablePrivate;
typedef struct _GimpDrawableClass   GimpDrawableClass;

struct _GimpDrawable
{
  GimpItem             parent_instance;

  GimpDrawablePrivate *private;
};

struct _GimpDrawableClass
{
  GimpItemClass  parent_class;

  /*  signals  */
  void          (* update)                (GimpDrawable         *drawable,
                                           gint                  x,
                                           gint                  y,
                                           gint                  width,
                                           gint                  height);
  void          (* format_changed)        (GimpDrawable         *drawable);
  void          (* alpha_changed)         (GimpDrawable         *drawable);
  void          (* bounding_box_changed)  (GimpDrawable         *drawable);
  void          (* filters_changed)       (GimpDrawable         *drawable);

  /*  virtual functions  */
  gint64        (* estimate_memsize)        (GimpDrawable         *drawable,
                                             GimpComponentType     component_type,
                                             gint                  width,
                                             gint                  height);
  void          (* update_all)              (GimpDrawable         *drawable);
  void          (* invalidate_boundary)     (GimpDrawable         *drawable);
  void          (* get_active_components)   (GimpDrawable         *drawable,
                                             gboolean             *active);
  GimpComponentMask (* get_active_mask)     (GimpDrawable         *drawable);
  gboolean      (* supports_alpha)          (GimpDrawable         *drawable);
  void          (* convert_type)            (GimpDrawable         *drawable,
                                             GimpImage            *dest_image,
                                             const Babl           *new_format,
                                             GimpColorProfile     *src_profile,
                                             GimpColorProfile     *dest_profile,
                                             GeglDitherMethod      layer_dither_type,
                                             GeglDitherMethod      mask_dither_type,
                                             gboolean              push_undo,
                                             GimpProgress         *progress);
  void          (* apply_buffer)            (GimpDrawable         *drawable,
                                             GeglBuffer           *buffer,
                                             const GeglRectangle  *buffer_region,
                                             gboolean              push_undo,
                                             const gchar          *undo_desc,
                                             gdouble               opacity,
                                             GimpLayerMode         mode,
                                             GimpLayerColorSpace   blend_space,
                                             GimpLayerColorSpace   composite_space,
                                             GimpLayerCompositeMode composite_mode,
                                             GeglBuffer           *base_buffer,
                                             gint                  base_x,
                                             gint                  base_y);
  GeglBuffer  * (* get_buffer)              (GimpDrawable         *drawable);
  void          (* set_buffer)              (GimpDrawable         *drawable,
                                             gboolean              push_undo,
                                             const gchar          *undo_desc,
                                             GeglBuffer           *buffer,
                                             const GeglRectangle  *bounds);
  GeglBuffer  * (* get_buffer_with_effects) (GimpDrawable         *drawable);
  GeglRectangle (* get_bounding_box)        (GimpDrawable         *drawable);
  void          (* push_undo)               (GimpDrawable         *drawable,
                                             const gchar          *undo_desc,
                                             GeglBuffer           *buffer,
                                             gint                  x,
                                             gint                  y,
                                             gint                  width,
                                             gint                  height);
  void          (* swap_pixels)             (GimpDrawable         *drawable,
                                             GeglBuffer           *buffer,
                                             gint                  x,
                                             gint                  y);
  GeglNode    * (* get_source_node)         (GimpDrawable         *drawable);
};


GType           gimp_drawable_get_type                (void) G_GNUC_CONST;

GimpDrawable  * gimp_drawable_new                     (GType               type,
                                                       GimpImage          *image,
                                                       const gchar        *name,
                                                       gint                offset_x,
                                                       gint                offset_y,
                                                       gint                width,
                                                       gint                height,
                                                       const Babl         *format);

gint64          gimp_drawable_estimate_memsize        (GimpDrawable       *drawable,
                                                       GimpComponentType   component_type,
                                                       gint                width,
                                                       gint                height);

void            gimp_drawable_update                  (GimpDrawable       *drawable,
                                                       gint                x,
                                                       gint                y,
                                                       gint                width,
                                                       gint                height);
void            gimp_drawable_update_all              (GimpDrawable       *drawable);

void           gimp_drawable_invalidate_boundary      (GimpDrawable       *drawable);
void         gimp_drawable_get_active_components      (GimpDrawable       *drawable,
                                                       gboolean           *active);
GimpComponentMask gimp_drawable_get_active_mask       (GimpDrawable       *drawable);

gboolean        gimp_drawable_supports_alpha          (GimpDrawable       *drawable);

void            gimp_drawable_convert_type            (GimpDrawable       *drawable,
                                                       GimpImage          *dest_image,
                                                       GimpImageBaseType   new_base_type,
                                                       GimpPrecision       new_precision,
                                                       gboolean            new_has_alpha,
                                                       GimpColorProfile   *src_profile,
                                                       GimpColorProfile   *dest_profile,
                                                       GeglDitherMethod    layer_dither_type,
                                                       GeglDitherMethod    mask_dither_type,
                                                       gboolean            push_undo,
                                                       GimpProgress       *progress);

void            gimp_drawable_apply_buffer            (GimpDrawable        *drawable,
                                                       GeglBuffer          *buffer,
                                                       const GeglRectangle *buffer_rect,
                                                       gboolean             push_undo,
                                                       const gchar         *undo_desc,
                                                       gdouble              opacity,
                                                       GimpLayerMode        mode,
                                                       GimpLayerColorSpace  blend_space,
                                                       GimpLayerColorSpace  composite_space,
                                                       GimpLayerCompositeMode composite_mode,
                                                       GeglBuffer          *base_buffer,
                                                       gint                 base_x,
                                                       gint                 base_y);

GeglBuffer    * gimp_drawable_get_buffer              (GimpDrawable       *drawable);
void            gimp_drawable_set_buffer              (GimpDrawable       *drawable,
                                                       gboolean            push_undo,
                                                       const gchar        *undo_desc,
                                                       GeglBuffer         *buffer);
void            gimp_drawable_set_buffer_full         (GimpDrawable       *drawable,
                                                       gboolean            push_undo,
                                                       const gchar        *undo_desc,
                                                       GeglBuffer         *buffer,
                                                       const GeglRectangle *bounds,
                                                       gboolean            update);
GeglBuffer    * gimp_drawable_get_buffer_with_effects (GimpDrawable       *drawable);

void            gimp_drawable_steal_buffer            (GimpDrawable       *drawable,
                                                       GimpDrawable       *src_drawable);

void            gimp_drawable_set_format              (GimpDrawable       *drawable,
                                                       const Babl         *format,
                                                       gboolean            copy_buffer,
                                                       gboolean            push_undo);

GeglNode      * gimp_drawable_get_source_node         (GimpDrawable       *drawable);
GeglNode      * gimp_drawable_get_mode_node           (GimpDrawable       *drawable);

GeglRectangle   gimp_drawable_get_bounding_box        (GimpDrawable       *drawable);
gboolean        gimp_drawable_update_bounding_box
                                                      (GimpDrawable       *drawable);

void            gimp_drawable_swap_pixels             (GimpDrawable       *drawable,
                                                       GeglBuffer         *buffer,
                                                       gint                x,
                                                       gint                y);

void            gimp_drawable_push_undo               (GimpDrawable       *drawable,
                                                       const gchar        *undo_desc,
                                                       GeglBuffer         *buffer,
                                                       gint                x,
                                                       gint                y,
                                                       gint                width,
                                                       gint                height);

void           gimp_drawable_disable_resize_undo      (GimpDrawable       *drawable);
void           gimp_drawable_enable_resize_undo       (GimpDrawable       *drawable);

const Babl      * gimp_drawable_get_space            (GimpDrawable    *drawable);
const Babl      * gimp_drawable_get_format           (GimpDrawable    *drawable);
const Babl      * gimp_drawable_get_format_with_alpha(GimpDrawable    *drawable);
const Babl      * gimp_drawable_get_format_without_alpha
                                                     (GimpDrawable    *drawable);
GimpTRCType       gimp_drawable_get_trc              (GimpDrawable    *drawable);
gboolean          gimp_drawable_has_alpha            (GimpDrawable    *drawable);
GimpImageBaseType gimp_drawable_get_base_type        (GimpDrawable    *drawable);
GimpComponentType gimp_drawable_get_component_type   (GimpDrawable    *drawable);
GimpPrecision     gimp_drawable_get_precision        (GimpDrawable    *drawable);
gboolean          gimp_drawable_is_rgb               (GimpDrawable    *drawable);
gboolean          gimp_drawable_is_gray              (GimpDrawable    *drawable);
gboolean          gimp_drawable_is_indexed           (GimpDrawable    *drawable);

const Babl      * gimp_drawable_get_component_format (GimpDrawable    *drawable,
                                                      GimpChannelType  channel);
gint              gimp_drawable_get_component_index  (GimpDrawable    *drawable,
                                                      GimpChannelType  channel);

void              gimp_drawable_start_paint          (GimpDrawable    *drawable);
gboolean          gimp_drawable_end_paint            (GimpDrawable    *drawable);
gboolean          gimp_drawable_flush_paint          (GimpDrawable    *drawable);
gboolean          gimp_drawable_is_painting          (GimpDrawable    *drawable);

void              gimp_drawable_filters_changed      (GimpDrawable    *drawable);


#endif /* __GIMP_DRAWABLE_H__ */
