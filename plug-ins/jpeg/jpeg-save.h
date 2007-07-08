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

#define SCALE_WIDTH         125

/* See bugs #63610 and #61088 for a discussion about the quality settings */
#define DEFAULT_QUALITY     85.0
#define DEFAULT_SMOOTHING   0.0
#define DEFAULT_OPTIMIZE    TRUE
#define DEFAULT_PROGRESSIVE FALSE
#define DEFAULT_BASELINE    TRUE
#define DEFAULT_SUBSMP      0
#define DEFAULT_RESTART     0
#define DEFAULT_DCT         0
#define DEFAULT_PREVIEW     FALSE
#define DEFAULT_EXIF        TRUE
#define DEFAULT_THUMBNAIL   FALSE
#define DEFAULT_XMP         TRUE

typedef struct
{
  gdouble  quality;
  gdouble  smoothing;
  gboolean optimize;
  gboolean progressive;
  gboolean baseline;
  gint     subsmp;
  gint     restart;
  gint     dct;
  gboolean preview;
  gboolean save_exif;
  gboolean save_thumbnail;
  gboolean save_xmp;
} JpegSaveVals;

extern JpegSaveVals     jsvals;

extern gint32           orig_image_ID_global;
extern gint32           drawable_ID_global;


gboolean    save_image    (const gchar      *filename,
                           gint32            image_ID,
                           gint32            drawable_ID,
                           gint32            orig_image_ID,
                           gboolean          preview);
gboolean    save_dialog   (void);
gboolean    load_defaults (void);
