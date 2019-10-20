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

#ifndef __JPEG_SAVE_H__
#define __JPEG_SAVE_H__

typedef struct
{
  gdouble          quality;
  gdouble          smoothing;
  gboolean         optimize;
  gboolean         arithmetic_coding;
  gboolean         progressive;
  gboolean         baseline;
  JpegSubsampling  subsmp;
  gint             restart;
  gint             dct;
  gboolean         preview;
  gboolean         save_exif;
  gboolean         save_xmp;
  gboolean         save_iptc;
  gboolean         save_thumbnail;
  gboolean         save_profile;
  gboolean         use_orig_quality;
} JpegSaveVals;

extern JpegSaveVals     jsvals;

extern GimpImage       *orig_image_global;
extern GimpDrawable    *drawable_global;


gboolean    save_image         (GFile         *file,
                                GimpImage     *image,
                                GimpDrawable  *drawable,
                                GimpImage     *orig_image,
                                gboolean       preview,
                                GError       **error);
gboolean    save_dialog        (GimpDrawable  *drawable);
void        load_defaults      (void);
void        load_parasite      (void);

#endif /* __JPEG_SAVE_H__ */
