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

#ifndef __TILE_PRIVATE_H__
#define __TILE_PRIVATE_H__

#include <sys/types.h>

#ifdef G_OS_WIN32

int gimp_win32_large_truncate (int    fd,
                               gint64 size);

#define LARGE_SEEK(f, o, w) _lseeki64 (f, o, w)
#define LARGE_TRUNCATE(f, s) gimp_win32_large_truncate (f, s)

#else

#define LARGE_SEEK(f, o, t) lseek (f, o, t)
#define LARGE_TRUNCATE(f, s) ftruncate (f, s)

#endif

typedef struct _TileLink TileLink;

struct _TileLink
{
  TileLink    *next;
  gint         tile_num; /* the number of this tile within the drawable */
  TileManager *tm;       /* A pointer to the tile manager for this tile.
                          *  We need this in order to call the tile managers
                          *  validate proc whenever the tile is referenced
                          *  yet invalid.
                          */
};

struct _Tile
{
  gshort  ref_count;    /* reference count. when the reference count is
                          *  non-zero then the "data" for this tile must
                         *  be valid. when the reference count for a tile
                         *  is 0 then the "data" for this tile must be
                         *  NULL.
                         */
  gshort  write_count;  /* write count: number of references that are
                           for write access */
  gshort  share_count;  /* share count: number of tile managers that
                           hold this tile */
  guint   dirty : 1;    /* is the tile dirty? has it been modified? */
  guint   valid : 1;    /* is the tile valid? */

  guchar  bpp;          /* the bytes per pixel (1, 2, 3 or 4) */
  gushort ewidth;       /* the effective width of the tile */
  gushort eheight;      /* the effective height of the tile
                         *  a tile's effective width and height may be smaller
                         *  (but not larger) than TILE_WIDTH and TILE_HEIGHT.
                         *  this is to handle edge tiles of a drawable.
                         */
  gint    size;         /* size of the tile data (ewidth * eheight * bpp) */

  TileRowHint *rowhint; /* An array of hints for rendering purposes */

  guchar *data;         /* the data for the tile. this may be NULL in which
                         *  case the tile data is on disk.
                         */

  gint64  swap_offset;  /* the offset within the swap file of the tile data.
                         * if the tile data is in memory this will be set
                         * to -1.
                         */

  TileLink *tlink;

  Tile     *next;
  Tile     *prev;       /* List pointers for the tile cache lists */
  gpointer  listhead;   /* Pointer to the head of the list this tile is on */
};


#endif /* __TILE_PRIVATE_H__ */
