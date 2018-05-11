/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimptile.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_TILE_H__
#define __GIMP_TILE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


struct _GimpTile
{
  guint         ewidth;     /* the effective width of the tile */
  guint         eheight;    /* the effective height of the tile */
  guint         bpp;        /* the bytes per pixel (1, 2, 3 or 4 ) */
  guint         tile_num;   /* the number of this tile within the drawable */
  guint16       ref_count;  /* reference count for the tile */
  guint         dirty : 1;  /* is the tile dirty? has it been modified? */
  guint         shadow: 1;  /* is this a shadow tile */
  guchar       *data;       /* the pixel data for the tile */
  GimpDrawable *drawable;   /* the drawable this tile came from */
};


GIMP_DEPRECATED
void    gimp_tile_ref          (GimpTile  *tile);
GIMP_DEPRECATED
void    gimp_tile_unref        (GimpTile  *tile,
                                gboolean   dirty);
GIMP_DEPRECATED
void    gimp_tile_flush        (GimpTile  *tile);

GIMP_DEPRECATED
void    gimp_tile_cache_ntiles (gulong     ntiles);


/*  private function  */

G_GNUC_INTERNAL void _gimp_tile_cache_flush_drawable (GimpDrawable *drawable);


G_END_DECLS

#endif /* __GIMP_TILE_H__ */
