/*
 * GIMP - The GNU Image Manipulation Program
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

#ifndef __BMP_H__
#define __BMP_H__


#define LOAD_PROC      "file-bmp-load"
#define EXPORT_PROC    "file-bmp-export"
#define PLUG_IN_BINARY "file-bmp"
#define PLUG_IN_ROLE   "gimp-file-bmp"

#define MAXCOLORS   256

#define BitSet(byte, bit)        (((byte) & (bit)) == (bit))

#define ReadOK(file,buffer,len)  (fread(buffer, len, 1, file) != 0)
#define Write(file,buffer,len)   fwrite(buffer, len, 1, file)
#define WriteOK(file,buffer,len) (Write(buffer, len, file) != 0)


typedef struct
{
  gchar    zzMagic[2];  /* 00 "BM" */
  guint32  bfSize;      /* 02 */
  guint16  zzHotX;      /* 06 */
  guint16  zzHotY;      /* 08 */
  guint32  bfOffs;      /* 0A */
  guint32  biSize;      /* 0E */
} BitmapFileHead;

typedef struct
{
  gint32   biWidth;     /* 12 */
  gint32   biHeight;    /* 16 */
  guint16  biPlanes;    /* 1A */
  guint16  biBitCnt;    /* 1C */
  guint32  biCompr;     /* 1E */
  guint32  biSizeIm;    /* 22 */
  guint32  biXPels;     /* 26 */
  guint32  biYPels;     /* 2A */
  guint32  biClrUsed;   /* 2E */
  guint32  biClrImp;    /* 32 */
  guint32  masks[4];    /* 36 */
} BitmapHead;

typedef struct
{
  guint32 mask;
  guint32 shiftin;
  gfloat  max_value;
} BitmapChannel;


#endif /* __BMP_H__ */
