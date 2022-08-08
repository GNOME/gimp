/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GIMP Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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

#ifndef __ICO_H__
#define __ICO_H__


#ifdef ICO_DBG
#define D(x) \
{ \
  printf("ICO plugin: "); \
  printf x; \
}
#else
#define D(x)
#endif

#define PLUG_IN_BINARY      "file-ico"
#define PLUG_IN_ROLE        "gimp-file-ico"

#define ICO_PNG_MAGIC       0x474e5089
#define ICO_ALPHA_THRESHOLD 127
#define ICO_MAXBUF          4096


typedef struct _IcoFileHeader
{
  guint16   reserved;
  guint16   resource_type;
  guint16   icon_count;
} IcoFileHeader;

typedef struct _IcoFileEntry
{
  guint8        width;      /* Width of icon in pixels */
  guint8        height;     /* Height of icon in pixels */
  guint8        num_colors; /* Number of colors of paletted image */
  guint8        reserved;   /* Must be 0 */
  guint16       planes;     /* Must be 1 for ICO, x position of hot spot for CUR */
  guint16       bpp;        /* 1, 4, 8, 24 or 32 bits per pixel for ICO, y position of hot spot for CUR */
  guint32       size;       /* Size of icon (including data header) */
  guint32       offset;     /* Absolute offset of data in a file */
 } IcoFileEntry;

typedef struct _IcoFileDataHeader
{
  guint32       header_size; /* 40 bytes */
  guint32       width;       /* Width of image in pixels */
  guint32       height;      /* Height of image in pixels */
  guint16       planes;      /* Must be 1 */
  guint16       bpp;
  guint32       compression; /* Not used for icons */
  guint32       image_size;  /* Size of image (without this header) */
  guint32       x_res;
  guint32       y_res;
  guint32       used_clrs;
  guint32       important_clrs;
} IcoFileDataHeader;


typedef struct _IcoLoadInfo
{
    guint    width;
    guint    height;
    gint     bpp;
    gint     planes;
    gint     offset;
    gint     size;
} IcoLoadInfo;

typedef struct _IcoSaveInfo
{
    gint        *depths;
    gint        *default_depths;
    gboolean    *compress;
    GList       *layers;
    gint         num_icons;
    gboolean     is_cursor;
    gint        *hot_spot_x;
    gint        *hot_spot_y;
} IcoSaveInfo;

typedef struct _AniFileHeader
{
    guint32 bSizeOf;     /* Number of bytes in AniFileHeader (36 bytes) */
    guint32 frames;      /* Number of unique icons in this cursor */
    guint32 steps;       /* Number of Blits before the animation cycles */
    guint32 x, y;        /* Reserved, must be zero. */
    guint32 bpp, planes; /* Reserved, must be zero. */
    guint32 jif_rate;    /* Default Jiffies (1/60th of a second) if rate chunk's not present. */
    guint32 flags;       /* Animation Flag */
} AniFileHeader;

typedef struct _AniSaveInfo
{
    gchar *inam;   /* Cursor name metadata */
    gchar *iart;   /* Author name metadata */
} AniSaveInfo;

/* Miscellaneous helper functions below: */

gint     ico_rowstride (gint width,
                        gint bpp);

/* Allocates a 32-bit padded bitmap for various color depths.
   Returns the allocated array directly, and the length of the
   array in the len pointer */
guint8 * ico_alloc_map  (gint     width,
                         gint     height,
                         gint     bpp,
                         gint    *len);

#endif /* __ICO_H__ */
