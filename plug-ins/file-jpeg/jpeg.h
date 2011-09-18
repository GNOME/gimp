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

#define LOAD_PROC       "file-jpeg-load"
#define LOAD_THUMB_PROC "file-jpeg-load-thumb"
#define SAVE_PROC       "file-jpeg-save"
#define PLUG_IN_BINARY  "file-jpeg"
#define PLUG_IN_ROLE    "gimp-file-jpeg"

/* headers used in some APPn markers */
#define JPEG_APP_HEADER_EXIF "Exif\0\0"
#define JPEG_APP_HEADER_XMP  "http://ns.adobe.com/xap/1.0/"

typedef struct my_error_mgr
{
  struct jpeg_error_mgr pub;            /* "public" fields */

#ifdef __ia64__
  /* Ugh, the jmp_buf field needs to be 16-byte aligned on ia64 and some
   * glibc/icc combinations don't guarantee this. So we pad. See bug #138357
   * for details.
   */
  long double           dummy;
#endif

  jmp_buf               setjmp_buffer;  /* for return to caller */
} *my_error_ptr;

typedef enum
{
  JPEG_SUBSAMPLING_2x2_1x1_1x1 = 0,  /* smallest file */
  JPEG_SUBSAMPLING_2x1_1x1_1x1 = 1,  /* 4:2:2         */
  JPEG_SUBSAMPLING_1x1_1x1_1x1 = 2,
  JPEG_SUBSAMPLING_1x2_1x1_1x1 = 3
} JpegSubsampling;

extern gint32 volatile  preview_image_ID;
extern gint32           preview_layer_ID;
extern GimpDrawable    *drawable_global;
extern gboolean         undo_touched;
extern gboolean         load_interactive;
extern gint32           display_ID;
extern gchar           *image_comment;
extern gboolean         has_metadata;
extern gint             orig_quality;
extern JpegSubsampling  orig_subsmp;
extern gint             num_quant_tables;


void      destroy_preview               (void);

void      my_error_exit                 (j_common_ptr   cinfo);
void      my_emit_message               (j_common_ptr   cinfo,
                                         int            msg_level);
void      my_output_message             (j_common_ptr   cinfo);

#ifdef HAVE_LIBEXIF

extern ExifData *exif_data;

ExifData * jpeg_exif_data_new_from_file (const gchar   *filename,
                                         GError       **error);

gint      jpeg_exif_get_orientation     (ExifData      *exif_data);

gboolean  jpeg_exif_get_resolution      (ExifData       *exif_data,
                                         gdouble        *xresolution,
                                         gdouble        *yresolution,
                                         gint           *unit);

void      jpeg_setup_exif_for_save      (ExifData      *exif_data,
                                         const gint32   image_ID);

void      jpeg_exif_rotate              (gint32         image_ID,
                                         gint           orientation);
void      jpeg_exif_rotate_query        (gint32         image_ID,
                                         gint           orientation);

#endif /* HAVE_LIBEXIF */

