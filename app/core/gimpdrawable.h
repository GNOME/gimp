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

#ifndef __LIGMA_DRAWABLE_H__
#define __LIGMA_DRAWABLE_H__


#include "ligmaitem.h"


#define LIGMA_TYPE_DRAWABLE            (ligma_drawable_get_type ())
#define LIGMA_DRAWABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DRAWABLE, LigmaDrawable))
#define LIGMA_DRAWABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DRAWABLE, LigmaDrawableClass))
#define LIGMA_IS_DRAWABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DRAWABLE))
#define LIGMA_IS_DRAWABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DRAWABLE))
#define LIGMA_DRAWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DRAWABLE, LigmaDrawableClass))


typedef struct _LigmaDrawablePrivate LigmaDrawablePrivate;
typedef struct _LigmaDrawableClass   LigmaDrawableClass;

struct _LigmaDrawable
{
  LigmaItem             parent_instance;

  LigmaDrawablePrivate *private;
};

struct _LigmaDrawableClass
{
  LigmaItemClass  parent_class;

  /*  signals  */
  void          (* update)                (LigmaDrawable         *drawable,
                                           gint                  x,
                                           gint                  y,
                                           gint                  width,
                                           gint                  height);
  void          (* format_changed)        (LigmaDrawable         *drawable);
  void          (* alpha_changed)         (LigmaDrawable         *drawable);
  void          (* bounding_box_changed)  (LigmaDrawable         *drawable);

  /*  virtual functions  */
  gint64        (* estimate_memsize)      (LigmaDrawable         *drawable,
                                           LigmaComponentType     component_type,
                                           gint                  width,
                                           gint                  height);
  void          (* update_all)            (LigmaDrawable         *drawable);
  void          (* invalidate_boundary)   (LigmaDrawable         *drawable);
  void          (* get_active_components) (LigmaDrawable         *drawable,
                                           gboolean             *active);
  LigmaComponentMask (* get_active_mask)   (LigmaDrawable         *drawable);
  gboolean      (* supports_alpha)        (LigmaDrawable         *drawable);
  void          (* convert_type)          (LigmaDrawable         *drawable,
                                           LigmaImage            *dest_image,
                                           const Babl           *new_format,
                                           LigmaColorProfile     *src_profile,
                                           LigmaColorProfile     *dest_profile,
                                           GeglDitherMethod      layer_dither_type,
                                           GeglDitherMethod      mask_dither_type,
                                           gboolean              push_undo,
                                           LigmaProgress         *progress);
  void          (* apply_buffer)          (LigmaDrawable         *drawable,
                                           GeglBuffer           *buffer,
                                           const GeglRectangle  *buffer_region,
                                           gboolean              push_undo,
                                           const gchar          *undo_desc,
                                           gdouble               opacity,
                                           LigmaLayerMode         mode,
                                           LigmaLayerColorSpace   blend_space,
                                           LigmaLayerColorSpace   composite_space,
                                           LigmaLayerCompositeMode composite_mode,
                                           GeglBuffer           *base_buffer,
                                           gint                  base_x,
                                           gint                  base_y);
  GeglBuffer  * (* get_buffer)            (LigmaDrawable         *drawable);
  void          (* set_buffer)            (LigmaDrawable         *drawable,
                                           gboolean              push_undo,
                                           const gchar          *undo_desc,
                                           GeglBuffer           *buffer,
                                           const GeglRectangle  *bounds);
  GeglRectangle (* get_bounding_box)      (LigmaDrawable         *drawable);
  void          (* push_undo)             (LigmaDrawable         *drawable,
                                           const gchar          *undo_desc,
                                           GeglBuffer           *buffer,
                                           gint                  x,
                                           gint                  y,
                                           gint                  width,
                                           gint                  height);
  void          (* swap_pixels)           (LigmaDrawable         *drawable,
                                           GeglBuffer           *buffer,
                                           gint                  x,
                                           gint                  y);
  GeglNode    * (* get_source_node)       (LigmaDrawable         *drawable);
};


GType           ligma_drawable_get_type           (void) G_GNUC_CONST;

LigmaDrawable  * ligma_drawable_new                (GType               type,
                                                  LigmaImage          *image,
                                                  const gchar        *name,
                                                  gint                offset_x,
                                                  gint                offset_y,
                                                  gint                width,
                                                  gint                height,
                                                  const Babl         *format);

gint64          ligma_drawable_estimate_memsize   (LigmaDrawable       *drawable,
                                                  LigmaComponentType   component_type,
                                                  gint                width,
                                                  gint                height);

void            ligma_drawable_update             (LigmaDrawable       *drawable,
                                                  gint                x,
                                                  gint                y,
                                                  gint                width,
                                                  gint                height);
void            ligma_drawable_update_all         (LigmaDrawable       *drawable);

void           ligma_drawable_invalidate_boundary (LigmaDrawable       *drawable);
void         ligma_drawable_get_active_components (LigmaDrawable       *drawable,
                                                  gboolean           *active);
LigmaComponentMask ligma_drawable_get_active_mask  (LigmaDrawable       *drawable);

gboolean        ligma_drawable_supports_alpha     (LigmaDrawable       *drawable);

void            ligma_drawable_convert_type       (LigmaDrawable       *drawable,
                                                  LigmaImage          *dest_image,
                                                  LigmaImageBaseType   new_base_type,
                                                  LigmaPrecision       new_precision,
                                                  gboolean            new_has_alpha,
                                                  LigmaColorProfile   *src_profile,
                                                  LigmaColorProfile   *dest_profile,
                                                  GeglDitherMethod    layer_dither_type,
                                                  GeglDitherMethod    mask_dither_type,
                                                  gboolean            push_undo,
                                                  LigmaProgress       *progress);

void            ligma_drawable_apply_buffer       (LigmaDrawable        *drawable,
                                                  GeglBuffer          *buffer,
                                                  const GeglRectangle *buffer_rect,
                                                  gboolean             push_undo,
                                                  const gchar         *undo_desc,
                                                  gdouble              opacity,
                                                  LigmaLayerMode        mode,
                                                  LigmaLayerColorSpace  blend_space,
                                                  LigmaLayerColorSpace  composite_space,
                                                  LigmaLayerCompositeMode composite_mode,
                                                  GeglBuffer          *base_buffer,
                                                  gint                 base_x,
                                                  gint                 base_y);

GeglBuffer    * ligma_drawable_get_buffer         (LigmaDrawable       *drawable);
void            ligma_drawable_set_buffer         (LigmaDrawable       *drawable,
                                                  gboolean            push_undo,
                                                  const gchar        *undo_desc,
                                                  GeglBuffer         *buffer);
void            ligma_drawable_set_buffer_full    (LigmaDrawable       *drawable,
                                                  gboolean            push_undo,
                                                  const gchar        *undo_desc,
                                                  GeglBuffer         *buffer,
                                                  const GeglRectangle *bounds,
                                                  gboolean            update);

void            ligma_drawable_steal_buffer       (LigmaDrawable       *drawable,
                                                  LigmaDrawable       *src_drawable);

void            ligma_drawable_set_format         (LigmaDrawable       *drawable,
                                                  const Babl         *format,
                                                  gboolean            copy_buffer,
                                                  gboolean            push_undo);

GeglNode      * ligma_drawable_get_source_node    (LigmaDrawable       *drawable);
GeglNode      * ligma_drawable_get_mode_node      (LigmaDrawable       *drawable);

GeglRectangle   ligma_drawable_get_bounding_box   (LigmaDrawable       *drawable);
gboolean        ligma_drawable_update_bounding_box
                                                 (LigmaDrawable       *drawable);

void            ligma_drawable_swap_pixels        (LigmaDrawable       *drawable,
                                                  GeglBuffer         *buffer,
                                                  gint                x,
                                                  gint                y);

void            ligma_drawable_push_undo          (LigmaDrawable       *drawable,
                                                  const gchar        *undo_desc,
                                                  GeglBuffer         *buffer,
                                                  gint                x,
                                                  gint                y,
                                                  gint                width,
                                                  gint                height);

const Babl      * ligma_drawable_get_space            (LigmaDrawable    *drawable);
const Babl      * ligma_drawable_get_format           (LigmaDrawable    *drawable);
const Babl      * ligma_drawable_get_format_with_alpha(LigmaDrawable    *drawable);
const Babl      * ligma_drawable_get_format_without_alpha
                                                     (LigmaDrawable    *drawable);
LigmaTRCType       ligma_drawable_get_trc              (LigmaDrawable    *drawable);
gboolean          ligma_drawable_has_alpha            (LigmaDrawable    *drawable);
LigmaImageBaseType ligma_drawable_get_base_type        (LigmaDrawable    *drawable);
LigmaComponentType ligma_drawable_get_component_type   (LigmaDrawable    *drawable);
LigmaPrecision     ligma_drawable_get_precision        (LigmaDrawable    *drawable);
gboolean          ligma_drawable_is_rgb               (LigmaDrawable    *drawable);
gboolean          ligma_drawable_is_gray              (LigmaDrawable    *drawable);
gboolean          ligma_drawable_is_indexed           (LigmaDrawable    *drawable);

const Babl      * ligma_drawable_get_component_format (LigmaDrawable    *drawable,
                                                      LigmaChannelType  channel);
gint              ligma_drawable_get_component_index  (LigmaDrawable    *drawable,
                                                      LigmaChannelType  channel);

guchar          * ligma_drawable_get_colormap         (LigmaDrawable    *drawable);

void              ligma_drawable_start_paint          (LigmaDrawable    *drawable);
gboolean          ligma_drawable_end_paint            (LigmaDrawable    *drawable);
gboolean          ligma_drawable_flush_paint          (LigmaDrawable    *drawable);
gboolean          ligma_drawable_is_painting          (LigmaDrawable    *drawable);


#endif /* __LIGMA_DRAWABLE_H__ */
