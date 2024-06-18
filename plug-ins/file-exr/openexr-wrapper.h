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

#ifndef __OPENEXR_WRAPPER_H__
#define __OPENEXR_WRAPPER_H__

G_BEGIN_DECLS

/* This is fully opaque on purpose, as the calling C code must not be
 * exposed to more than this.
 */
typedef struct _EXRLoader EXRLoader;

typedef enum
{
  PREC_UINT,
  PREC_HALF,
  PREC_FLOAT
} EXRPrecision;

typedef enum
{
  IMAGE_TYPE_RGB,
  IMAGE_TYPE_YUV,
  IMAGE_TYPE_GRAY,
  IMAGE_TYPE_UNKNOWN_1_CHANNEL
} EXRImageType;


EXRLoader        * exr_loader_new            (const char *filename);

EXRLoader        * exr_loader_ref            (EXRLoader  *loader);
void               exr_loader_unref          (EXRLoader  *loader);

int                exr_loader_get_width      (EXRLoader  *loader);
int                exr_loader_get_height     (EXRLoader  *loader);

EXRPrecision       exr_loader_get_precision  (EXRLoader  *loader);
EXRImageType       exr_loader_get_image_type (EXRLoader  *loader);
int                exr_loader_has_alpha      (EXRLoader  *loader);

GimpColorProfile * exr_loader_get_profile    (EXRLoader  *loader);
gchar            * exr_loader_get_comment    (EXRLoader  *loader);
guchar           * exr_loader_get_exif       (EXRLoader  *loader,
                                              guint      *size);
guchar           * exr_loader_get_xmp        (EXRLoader  *loader,
                                              guint      *size);

int                exr_loader_read_pixel_row (EXRLoader *loader,
                                              char      *pixels,
                                              int        bpp,
                                              int        row);

G_END_DECLS

#endif /* __OPENEXR_WRAPPER_H__ */
