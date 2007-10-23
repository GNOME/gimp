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

#include "config.h"

#include <glib-object.h>

#include "base-types.h"

#include "tile.h"
#include "tile-rowhints.h"
#include "tile-private.h"


/*  Sanity checks on tile hinting code  */
/*  #define HINTS_SANITY */


void
tile_allocate_rowhints (Tile *tile)
{
  if (! tile->rowhint)
    tile->rowhint = g_slice_alloc0 (sizeof (TileRowHint) * TILE_HEIGHT);
}

TileRowHint
tile_get_rowhint (Tile *tile,
                  gint  yoff)
{
  if (! tile->rowhint)
    return  TILEROWHINT_UNKNOWN;

#ifdef HINTS_SANITY
  if (yoff < tile_eheight(tile) && yoff >= 0)
    {
      return tile->rowhint[yoff];
    }
  else
    {
      g_error ("GET_ROWHINT OUT OF RANGE");
    }

  return TILEROWHINT_OUTOFRANGE;
#else
  return tile->rowhint[yoff];
#endif
}

void
tile_set_rowhint (Tile        *tile,
                  gint         yoff,
                  TileRowHint  rowhint)
{
#ifdef HINTS_SANITY
  if (yoff < tile_eheight (tile) && yoff >= 0)
    {
      tile->rowhint[yoff] = rowhint;
    }
  else
    {
      g_error ("SET_ROWHINT OUT OF RANGE");
    }
#else

  tile->rowhint[yoff] = rowhint;
#endif
}

void
tile_update_rowhints (Tile *tile,
                      gint  start,
                      gint  rows)
{
  const guchar *ptr;
  gint          bpp, ewidth;
  gint          x, y;

#ifdef HINTS_SANITY
  g_assert (tile != NULL);
#endif

  tile_allocate_rowhints (tile);

  bpp = tile_bpp (tile);
  ewidth = tile_ewidth (tile);

  switch (bpp)
    {
    case 1:
    case 3:
      for (y = start; y < start + rows; y++)
        tile_set_rowhint (tile, y, TILEROWHINT_OPAQUE);
      break;

    case 4:
#ifdef HINTS_SANITY
      g_assert (tile != NULL);
#endif

      ptr = tile_data_pointer (tile, 0, start);

#ifdef HINTS_SANITY
      g_assert (ptr != NULL);
#endif

      for (y = start; y < start + rows; y++)
        {
          TileRowHint hint = tile_get_rowhint (tile, y);

#ifdef HINTS_SANITY
          if (hint == TILEROWHINT_BROKEN)
            g_error ("BROKEN y=%d", y);
          if (hint == TILEROWHINT_OUTOFRANGE)
            g_error ("OOR y=%d", y);
          if (hint == TILEROWHINT_UNDEFINED)
            g_error ("UNDEFINED y=%d - bpp=%d ew=%d eh=%d",
                     y, bpp, ewidth, eheight);
#endif

#ifdef HINTS_SANITY
          if (hint == TILEROWHINT_TRANSPARENT ||
              hint == TILEROWHINT_MIXED ||
              hint == TILEROWHINT_OPAQUE)
            {
              goto next_row4;
            }

          if (hint != TILEROWHINT_UNKNOWN)
            {
              g_error ("MEGABOGUS y=%d - bpp=%d ew=%d eh=%d",
                       y, bpp, ewidth, eheight);
            }
#endif

          if (hint == TILEROWHINT_UNKNOWN)
            {
              const guchar alpha = ptr[3];

              /* row is all-opaque or all-transparent? */
              if (alpha == 0 || alpha == 255)
                {
                  if (ewidth > 1)
                    {
                      for (x = 1; x < ewidth; x++)
                        {
                          if (ptr[x * 4 + 3] != alpha)
                            {
                              tile_set_rowhint (tile, y, TILEROWHINT_MIXED);
                              goto next_row4;
                            }
                        }
                    }

                  tile_set_rowhint (tile, y,
                                    (alpha == 0) ?
                                    TILEROWHINT_TRANSPARENT :
                                    TILEROWHINT_OPAQUE);
                }
              else
                {
                  tile_set_rowhint (tile, y, TILEROWHINT_MIXED);
                }
            }

        next_row4:
          ptr += 4 * ewidth;
        }
      break;

    case 2:
#ifdef HINTS_SANITY
      g_assert (tile != NULL);
#endif

      ptr = tile_data_pointer (tile, 0, start);

#ifdef HINTS_SANITY
      g_assert (ptr != NULL);
#endif

      for (y = start; y < start + rows; y++)
        {
          TileRowHint hint = tile_get_rowhint (tile, y);

#ifdef HINTS_SANITY
          if (hint == TILEROWHINT_BROKEN)
            g_error ("BROKEN y=%d",y);
          if (hint == TILEROWHINT_OUTOFRANGE)
            g_error ("OOR y=%d",y);
          if (hint == TILEROWHINT_UNDEFINED)
            g_error ("UNDEFINED y=%d - bpp=%d ew=%d eh=%d",
                     y, bpp, ewidth, eheight);
#endif

#ifdef HINTS_SANITY
          if (hint == TILEROWHINT_TRANSPARENT ||
              hint == TILEROWHINT_MIXED ||
              hint == TILEROWHINT_OPAQUE)
            {
              goto next_row2;
            }

          if (hint != TILEROWHINT_UNKNOWN)
            {
              g_error ("MEGABOGUS y=%d - bpp=%d ew=%d eh=%d",
                       y, bpp, ewidth, eheight);
            }
#endif

          if (hint == TILEROWHINT_UNKNOWN)
            {
              const guchar alpha = ptr[1];

              /* row is all-opaque or all-transparent? */
              if (alpha == 0 || alpha == 255)
                {
                  if (ewidth > 1)
                    {
                      for (x = 1; x < ewidth; x++)
                        {
                          if (ptr[x * 2 + 1] != alpha)
                            {
                              tile_set_rowhint (tile, y, TILEROWHINT_MIXED);
                              goto next_row2;
                            }
                        }
                    }
                  tile_set_rowhint (tile, y,
                                    (alpha == 0) ?
                                    TILEROWHINT_TRANSPARENT :
                                    TILEROWHINT_OPAQUE);
                }
              else
                {
                  tile_set_rowhint (tile, y, TILEROWHINT_MIXED);
                }
            }

        next_row2:
          ptr += 2 * ewidth;
        }
      break;

    default:
      g_return_if_reached ();
      break;
    }
}
