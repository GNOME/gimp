/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * DPX image file format library definitions.
 * DPX file format structures.
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

#ifndef _DPX_LIB_H_
#define _DPX_LIB_H_

typedef struct {
    guint32   magic_num;        /* magic number */
    guint32   offset;           /* offset to image data in bytes */
    guchar    vers[8];          /* which header format version is being used (v1.0) */
    guint32   file_size;        /* file size in bytes */
    guint32   ditto_key;        /* I bet some people use this */
    guint32   gen_hdr_size;     /* generic header length in bytes */
    guint32   ind_hdr_size;     /* industry header length in bytes */
    guint32   user_data_size;   /* user-defined data length in bytes */
    guchar    file_name[100];   /* image file name */
    guchar    create_date[24];  /* file creation date, yyyy:mm:dd:hh:mm:ss:LTZ */
    guchar    creator[100];
    guchar    project[200];
    guchar    copyright[200];
    guint32   key;              /* encryption key, FFFFFFF = unencrypted */
    guchar    reserved[104];    /* reserved field TBD (need to pad) */
} DpxFileInformation;

typedef struct {
    guint32   signage;
    guint32   ref_low_data;     /* reference low data code value */
    gfloat    ref_low_quantity; /* reference low quantity represented */
    guint32   ref_high_data;    /* reference high data code value */
    gfloat    ref_high_quantity;/* reference high quantity represented */
    guint8    designator1;
    guint8    transfer_characteristics;
    guint8    colourimetry;
    guint8    bits_per_pixel;
    guint16   packing;
    guint16   encoding;
    guint32   data_offset;
    guint32   line_padding;
    guint32   channel_padding;
    guchar    description[32];
} DpxChannelInformation;

typedef struct {
    guint16               orientation;
    guint16               channels_per_image;
    guint32               pixels_per_line;
    guint32               lines_per_image;
    DpxChannelInformation channel[8];
    guchar                reserved[52];
} DpxImageInformation;

typedef struct {
    guint32   x_offset;
    guint32   y_offset;
    gfloat    x_centre;
    gfloat    y_centre;
    guint32   x_original_size;
    guint32   y_original_size;
    guchar    file_name[100];
    guchar    creation_time[24];
    guchar    input_device[32];
    guchar    input_serial_number[32];
    guint16   border_validity[4];
    guint32   pixel_aspect_ratio[2]; /* h:v */
    guchar    reserved[28];
} DpxOriginationInformation;

typedef struct {
    guchar    film_manufacturer_id[2];
    guchar    film_type[2];
    guchar    edge_code_perforation_offset[2];
    guchar    edge_code_prefix[6];
    guchar    edge_code_count[4];
    guchar    film_format[32];
    guint32   frame_position;
    guint32   sequence_length;
    guint32   held_count;
    gfloat    frame_rate;
    gfloat    shutter_angle;
    guchar    frame_identification[32];
    guchar    slate_info[100];
    guchar    reserved[56];
} DpxMPIInformation;

typedef struct {
  DpxFileInformation        fileInfo;
  DpxImageInformation       imageInfo;
  DpxOriginationInformation originInfo;
  DpxMPIInformation         filmHeader;
} DpxMainHeader;

GimpImage * dpx_open (GFile   *file,
                      GError **error);


#endif /* _DPX_LIB_H_ */
