/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmacolortransform.h
 * Copyright (C) 2014  Michael Natterer <mitch@ligma.org>
 *                     Elle Stone <ellestone@ninedegreesbelow.com>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_COLOR_H_INSIDE__) && !defined (LIGMA_COLOR_COMPILATION)
#error "Only <libligmacolor/ligmacolor.h> can be included directly."
#endif

#ifndef __LIGMA_COLOR_TRANSFORM_H__
#define __LIGMA_COLOR_TRANSFORM_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * LigmaColorTransformFlags:
 * @LIGMA_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE: optimize for accuracy rather
 *   than for speed
 * @LIGMA_COLOR_TRANSFORM_FLAGS_GAMUT_CHECK: mark out of gamut colors in the
 *   transform result
 * @LIGMA_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION: do black point
 *   compensation
 *
 * Flags for modifying #LigmaColorTransform's behavior.
 **/
typedef enum
{
  LIGMA_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE               = 0x0100,
  LIGMA_COLOR_TRANSFORM_FLAGS_GAMUT_CHECK              = 0x1000,
  LIGMA_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION = 0x2000,
} LigmaColorTransformFlags;


#define LIGMA_TYPE_COLOR_TRANSFORM            (ligma_color_transform_get_type ())
#define LIGMA_COLOR_TRANSFORM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_TRANSFORM, LigmaColorTransform))
#define LIGMA_COLOR_TRANSFORM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_TRANSFORM, LigmaColorTransformClass))
#define LIGMA_IS_COLOR_TRANSFORM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_TRANSFORM))
#define LIGMA_IS_COLOR_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_TRANSFORM))
#define LIGMA_COLOR_TRANSFORM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_TRANSFORM, LigmaColorTransformClass))


typedef struct _LigmaColorTransformPrivate LigmaColorTransformPrivate;
typedef struct _LigmaColorTransformClass   LigmaColorTransformClass;

struct _LigmaColorTransform
{
  GObject                    parent_instance;

  LigmaColorTransformPrivate *priv;
};

struct _LigmaColorTransformClass
{
  GObjectClass  parent_class;

  /* signals */
  void (* progress) (LigmaColorTransform *transform,
                     gdouble             fraction);

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType   ligma_color_transform_get_type (void) G_GNUC_CONST;

LigmaColorTransform *
        ligma_color_transform_new              (LigmaColorProfile         *src_profile,
                                               const Babl               *src_format,
                                               LigmaColorProfile         *dest_profile,
                                               const Babl               *dest_format,
                                               LigmaColorRenderingIntent  rendering_intent,
                                               LigmaColorTransformFlags   flags);

LigmaColorTransform *
        ligma_color_transform_new_proofing     (LigmaColorProfile         *src_profile,
                                               const Babl               *src_format,
                                               LigmaColorProfile         *dest_profile,
                                               const Babl               *dest_format,
                                               LigmaColorProfile         *proof_profile,
                                               LigmaColorRenderingIntent  proof_intent,
                                               LigmaColorRenderingIntent  display_intent,
                                               LigmaColorTransformFlags   flags);

void    ligma_color_transform_process_pixels   (LigmaColorTransform       *transform,
                                               const Babl               *src_format,
                                               gconstpointer             src_pixels,
                                               const Babl               *dest_format,
                                               gpointer                  dest_pixels,
                                               gsize                     length);

void    ligma_color_transform_process_buffer   (LigmaColorTransform       *transform,
                                               GeglBuffer               *src_buffer,
                                               const GeglRectangle      *src_rect,
                                               GeglBuffer               *dest_buffer,
                                               const GeglRectangle      *dest_rect);

gboolean ligma_color_transform_can_gegl_copy   (LigmaColorProfile         *src_profile,
                                               LigmaColorProfile         *dest_profile);


G_END_DECLS

#endif  /* __LIGMA_COLOR_TRANSFORM_H__ */
