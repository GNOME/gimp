/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Cineon image file format library definitions.
 * Cineon file format structures.
 *
 * Copyright 1999,2000,2001 David Hodson <hodsond@acm.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _CINEON_LIB_H_
#define _CINEON_LIB_H_

typedef struct
{
  /* specified in header */
  gint     width;
  gint     height;
  gint     depth;
  gint     bits_per_pixel;
  gint     image_offset;

  /* file buffer, measured in longwords (4 byte) */
  gint     line_buffer_length;
  guint   *line_buffer;

  /* pixel buffer, holds 10 bit pixel values */
  gushort *pixel_buffer;
  gint     pixel_buffer_used;

  /* io stuff */
  FILE    *file;
  gint     reading;
  gint     file_y_Pos;
} CineonFile;

typedef struct
{
  guint32 magic_num;        /* magic number */
  guint32 image_offset;     /* offset to image data in bytes */
  guint32 gen_hdr_size;     /* generic header length in bytes */
  guint32 ind_hdr_size;     /* industry header length in bytes */
  guint32 user_data_size;   /* user-defined data length in bytes */
  guint32 file_size;        /* file size in bytes */
  gchar   vers[8];          /* which header format version is being used (v4.5) */
  gchar   file_name[100];   /* image file name */
  gchar   create_date[12];  /* file creation date */
  gchar   create_time[12];  /* file creation time */
  gchar   reserved[36];     /* reserved field TBD (need to pad) */
} CineonFileInformation;

typedef struct
{
  guint8  designator1;
  guint8  designator2;
  guint8  bits_per_pixel;
  guint8  filler;
  guint32 pixels_per_line;
  guint32 lines_per_image;
  guint32 ref_low_data;     /* reference low data code value */
  gfloat  ref_low_quantity; /* reference low quantity represented */
  guint32 ref_high_data;    /* reference high data code value */
  gfloat  ref_high_quantity;/* reference high quantity represented */
} CineonChannelInformation;

typedef struct
{
  guint8                   orientation; /* image orientation */
  guint8                   channels_per_image;
  guint16                  filler;
  CineonChannelInformation channel[8];
  gfloat                   white_point_x;
  gfloat                   white_point_y;
  gfloat                   red_primary_x;
  gfloat                   red_primary_y;
  gfloat                   green_primary_x;
  gfloat                   green_primary_y;
  gfloat                   blue_primary_x;
  gfloat                   blue_primary_y;
  gchar                    label[200];
  gchar                    reserved[28];
} CineonImageInformation;

typedef struct
{
  guint8  interleave;
  guint8  packing;
  guint8  signage;
  guint8  sense;
  guint32 line_padding;
  guint32 channel_padding;
  gchar   reserved[20];
} CineonFormatInformation;

typedef struct
{
  gdouble  x_offset;
  gdouble  y_offset;
  gchar    file_name[100];
  gchar    create_date[12];  /* file creation date */
  gchar    create_time[12];  /* file creation time */
  gchar    input_device[64];
  gchar    model_number[32];
  gchar    serial_number[32];
  gfloat   x_input_samples_per_mm;
  gfloat   y_input_samples_per_mm;
  gfloat   input_device_gamma;
  gchar    reserved[40];
} CineonOriginationInformation;

typedef struct
{
  CineonFileInformation        fileInfo;
  CineonImageInformation       imageInfo;
  CineonFormatInformation      formatInfo;
  CineonOriginationInformation originInfo;
} CineonGenericHeader;

typedef struct
{
  guint8  filmCode;
  guint8  filmType;
  guint8  perfOffset;
  guint8  filler;
  guint32 keycodePrefix;
  guint32 keycodeCount;
  gchar   format[32];
  guint32 framePosition; /* in sequence */
  gfloat  frameRate;     /* frames per second */
  gchar   attribute[32];
  gchar   slate[200];
  gchar   reserved[740];
} CineonMPISpecificInformation;


GimpImage * cineon_open (GFile   *file,
                         GError **error);


#endif /* _CINEON_LIB_H_ */
