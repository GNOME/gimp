/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GIMP Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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

#ifndef __MAIN_H__
#define __MAIN_H__


#ifdef ICO_DBG
#define D(x) \
{ \
  printf("ICO plugin: "); \
  printf x; \
}
#else
#define D(x)
#endif


#define MAXLEN 4096

typedef struct _MsIconEntry
{
  guint8        width;        /* Width of icon in pixels */
  guint8        height;       /* Height of icon in pixels */
  guint8        num_colors;   /* Maximum number of colors */
  guint8        reserved;     /* Not used */
  guint16       num_planes;   /* Not used */
  guint16       bpp;
  guint32       size;         /* Length of icon bitmap in bytes */
  guint32       offset;       /* Offset position of icon bitmap in file */
} MsIconEntry;

typedef struct _MsIconData
{
  /* Bitmap header data */
  guint32       header_size;  /* = 40 Bytes */
  guint32       width;
  guint32       height;
  guint16       planes;
  guint16       bpp;
  guint32       compression;  /* not used for icons */
  guint32       image_size;   /* size of image */
  guint32       x_res;
  guint32       y_res;
  guint32       used_clrs;
  guint32       important_clrs;

  guint32      *palette;      /* Color palette, only if bpp <= 8. */
  guint8       *xor_map;      /* Icon bitmap */
  guint8       *and_map;      /* Display bit mask */

  /* Only used when saving: */
  gint          palette_len;
  gint          xor_len;
  gint          and_len;
} MsIconData;

typedef struct _MsIcon
{
  FILE         *fp;
  guint         cp;
  const gchar  *filename;

  guint16       reserved;
  guint16       resource_type;
  guint16       icon_count;
  MsIconEntry  *icon_dir;
  MsIconData   *icon_data;
} MsIcon;


/* Miscellaneous helper functions below: */

/* Allocates a 32-bit padded bitmap for various color depths.
   Returns the allocated array directly, and the length of the
   array in the len pointer */
guint8 * ico_alloc_map  (gint     width,
			 gint     height,
			 gint     bpp,
			 gint    *len);

void     ico_cleanup    (MsIcon  *ico);


#endif /* __MAIN_H__ */
