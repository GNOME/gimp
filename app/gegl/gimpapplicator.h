/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaapplicator.h
 * Copyright (C) 2012-2013 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_APPLICATOR_H__
#define __LIGMA_APPLICATOR_H__


#define LIGMA_TYPE_APPLICATOR            (ligma_applicator_get_type ())
#define LIGMA_APPLICATOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_APPLICATOR, LigmaApplicator))
#define LIGMA_APPLICATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_APPLICATOR, LigmaApplicatorClass))
#define LIGMA_IS_APPLICATOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_APPLICATOR))
#define LIGMA_IS_APPLICATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_APPLICATOR))
#define LIGMA_APPLICATOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_APPLICATOR, LigmaApplicatorClass))


typedef struct _LigmaApplicatorClass LigmaApplicatorClass;

struct _LigmaApplicator
{
  GObject                 parent_instance;

  GeglNode               *node;
  GeglNode               *input_node;
  GeglNode               *aux_node;
  GeglNode               *output_node;

  gboolean                active;

  GeglBuffer             *apply_buffer;
  GeglNode               *apply_src_node;

  gint                    apply_offset_x;
  gint                    apply_offset_y;
  GeglNode               *apply_offset_node;

  gdouble                 opacity;
  LigmaLayerMode           paint_mode;
  LigmaLayerColorSpace     blend_space;
  LigmaLayerColorSpace     composite_space;
  LigmaLayerCompositeMode  composite_mode;
  GeglNode               *mode_node;

  LigmaComponentMask       affect;
  GeglNode               *affect_node;

  const Babl             *output_format;
  GeglNode               *convert_format_node;

  gboolean                cache_enabled;
  GeglNode               *cache_node;

  gboolean                crop_enabled;
  GeglRectangle           crop_rect;
  GeglNode               *crop_node;

  GeglBuffer             *src_buffer;
  GeglNode               *src_node;

  GeglBuffer             *dest_buffer;
  GeglNode               *dest_node;

  GeglBuffer             *mask_buffer;
  GeglNode               *mask_node;

  gint                    mask_offset_x;
  gint                    mask_offset_y;
  GeglNode               *mask_offset_node;
};

struct _LigmaApplicatorClass
{
  GObjectClass  parent_class;
};


GType        ligma_applicator_get_type          (void) G_GNUC_CONST;

LigmaApplicator * ligma_applicator_new           (GeglNode             *parent);

void         ligma_applicator_set_active        (LigmaApplicator       *applicator,
                                                gboolean              active);

void         ligma_applicator_set_src_buffer    (LigmaApplicator       *applicator,
                                                GeglBuffer           *dest_buffer);
void         ligma_applicator_set_dest_buffer   (LigmaApplicator       *applicator,
                                                GeglBuffer           *dest_buffer);

void         ligma_applicator_set_mask_buffer   (LigmaApplicator       *applicator,
                                                GeglBuffer           *mask_buffer);
void         ligma_applicator_set_mask_offset   (LigmaApplicator       *applicator,
                                                gint                  mask_offset_x,
                                                gint                  mask_offset_y);

void         ligma_applicator_set_apply_buffer  (LigmaApplicator       *applicator,
                                                GeglBuffer           *apply_buffer);
void         ligma_applicator_set_apply_offset  (LigmaApplicator       *applicator,
                                                gint                  apply_offset_x,
                                                gint                  apply_offset_y);

void         ligma_applicator_set_opacity       (LigmaApplicator       *applicator,
                                                gdouble               opacity);
void         ligma_applicator_set_mode          (LigmaApplicator       *applicator,
                                                LigmaLayerMode         paint_mode,
                                                LigmaLayerColorSpace   blend_space,
                                                LigmaLayerColorSpace   composite_space,
                                                LigmaLayerCompositeMode composite_mode);
void         ligma_applicator_set_affect        (LigmaApplicator       *applicator,
                                                LigmaComponentMask     affect);

void         ligma_applicator_set_output_format (LigmaApplicator       *applicator,
                                                const Babl           *format);
const Babl * ligma_applicator_get_output_format (LigmaApplicator       *applicator);

void         ligma_applicator_set_cache         (LigmaApplicator       *applicator,
                                                gboolean              enable);
gboolean     ligma_applicator_get_cache         (LigmaApplicator       *applicator);
GeglBuffer * ligma_applicator_get_cache_buffer  (LigmaApplicator       *applicator,
                                                GeglRectangle       **rectangles,
                                                gint                 *n_rectangles);

void         ligma_applicator_set_crop          (LigmaApplicator       *applicator,
                                                const GeglRectangle  *rect);
const GeglRectangle * ligma_applicator_get_crop (LigmaApplicator       *applicator);

void         ligma_applicator_blit              (LigmaApplicator       *applicator,
                                                const GeglRectangle  *rect);


#endif  /*  __LIGMA_APPLICATOR_H__  */
