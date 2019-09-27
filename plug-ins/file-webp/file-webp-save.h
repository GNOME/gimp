/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-webp - WebP file format plug-in for the GIMP
 * Copyright (C) 2015  Nathan Osman
 * Copyright (C) 2016  Ben Touchette
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#ifndef __WEBP_SAVE_H__
#define __WEBP_SAVE_H__


typedef struct
{
  WebPPreset preset;
  gboolean   lossless;
  gboolean   animation;
  gboolean   loop;
  gboolean   minimize_size;
  gint       kf_distance;
  gfloat     quality;
  gfloat     alpha_quality;
  gboolean   save_exif;
  gboolean   save_iptc;
  gboolean   save_xmp;
  gboolean   save_profile;
  gint       delay;
  gboolean   force_delay;
} WebPSaveParams;


gboolean   save_layer     (GFile           *file,
                           GimpImage       *image,
                           GimpDrawable    *drawable,
                           WebPSaveParams  *params,
                           GError         **error);

gboolean   save_animation (GFile           *file,
                           GimpImage       *image,
                           GimpDrawable    *drawable,
                           WebPSaveParams  *params,
                           GError         **error);


#endif /* __WEBP_SAVE_H__ */
