/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpapplicator.h
 * Copyright (C) 2012-2013 Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_APPLICATOR            (gimp_applicator_get_type ())
#define GIMP_APPLICATOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_APPLICATOR, GimpApplicator))
#define GIMP_APPLICATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_APPLICATOR, GimpApplicatorClass))
#define GIMP_IS_APPLICATOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_APPLICATOR))
#define GIMP_IS_APPLICATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_APPLICATOR))
#define GIMP_APPLICATOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_APPLICATOR, GimpApplicatorClass))


typedef struct _GimpApplicatorClass GimpApplicatorClass;

struct _GimpApplicator
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
  GimpLayerMode           paint_mode;
  GimpLayerColorSpace     blend_space;
  GimpLayerColorSpace     composite_space;
  GimpLayerCompositeMode  composite_mode;
  GeglNode               *mode_node;

  GimpComponentMask       affect;
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

struct _GimpApplicatorClass
{
  GObjectClass  parent_class;
};


GType        gimp_applicator_get_type          (void) G_GNUC_CONST;

GimpApplicator * gimp_applicator_new           (GeglNode             *parent);

void         gimp_applicator_set_active        (GimpApplicator       *applicator,
                                                gboolean              active);

void         gimp_applicator_set_src_buffer    (GimpApplicator       *applicator,
                                                GeglBuffer           *dest_buffer);
void         gimp_applicator_set_dest_buffer   (GimpApplicator       *applicator,
                                                GeglBuffer           *dest_buffer);

void         gimp_applicator_set_mask_buffer   (GimpApplicator       *applicator,
                                                GeglBuffer           *mask_buffer);
void         gimp_applicator_set_mask_offset   (GimpApplicator       *applicator,
                                                gint                  mask_offset_x,
                                                gint                  mask_offset_y);

void         gimp_applicator_set_apply_buffer  (GimpApplicator       *applicator,
                                                GeglBuffer           *apply_buffer);
void         gimp_applicator_set_apply_offset  (GimpApplicator       *applicator,
                                                gint                  apply_offset_x,
                                                gint                  apply_offset_y);

void         gimp_applicator_set_opacity       (GimpApplicator       *applicator,
                                                gdouble               opacity);
void         gimp_applicator_set_mode          (GimpApplicator       *applicator,
                                                GimpLayerMode         paint_mode,
                                                GimpLayerColorSpace   blend_space,
                                                GimpLayerColorSpace   composite_space,
                                                GimpLayerCompositeMode composite_mode);
void         gimp_applicator_set_affect        (GimpApplicator       *applicator,
                                                GimpComponentMask     affect);

gboolean     gimp_applicator_set_output_format (GimpApplicator       *applicator,
                                                const Babl           *format);
const Babl * gimp_applicator_get_output_format (GimpApplicator       *applicator);

void         gimp_applicator_set_cache         (GimpApplicator       *applicator,
                                                gboolean              enable);
gboolean     gimp_applicator_get_cache         (GimpApplicator       *applicator);
GeglBuffer * gimp_applicator_get_cache_buffer  (GimpApplicator       *applicator,
                                                GeglRectangle       **rectangles,
                                                gint                 *n_rectangles);

void         gimp_applicator_set_crop          (GimpApplicator       *applicator,
                                                const GeglRectangle  *rect);
const GeglRectangle * gimp_applicator_get_crop (GimpApplicator       *applicator);

void         gimp_applicator_blit              (GimpApplicator       *applicator,
                                                const GeglRectangle  *rect);
