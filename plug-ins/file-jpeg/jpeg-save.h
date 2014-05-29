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
  gboolean         save_thumbnail;
  gboolean         save_xmp;
  gboolean         save_iptc;
  gboolean         use_orig_quality;
} JpegSaveVals;

extern JpegSaveVals     jsvals;

extern gint32           orig_image_ID_global;
extern gint32           drawable_ID_global;


gboolean    save_image         (const gchar  *filename,
                                gint32        image_ID,
                                gint32        drawable_ID,
                                gint32        orig_image_ID,
                                gboolean      preview,
                                GError      **error);
gboolean    save_dialog        (void);
void        load_defaults      (void);

#endif /* __JPEG_SAVE_H__ */
