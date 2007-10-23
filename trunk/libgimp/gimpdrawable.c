/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpdrawable.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "gimp.h"


#define TILE_WIDTH  gimp_tile_width()
#define TILE_HEIGHT gimp_tile_height()


/**
 * gimp_drawable_get:
 * @drawable_ID: the ID of the drawable
 *
 * This function creates a #GimpDrawable structure for the core
 * drawable identified by @drawable_ID. The returned structure
 * contains some basic information about the drawable and can also
 * hold tile data for transfer to and from the core.
 *
 * Note that the name of this function is somewhat misleading, because
 * it suggests that it simply returns a handle.  This is not the case:
 * if the function is called multiple times, it creates separate tile
 * lists each time, which will usually produce undesired results.
 *
 * When a plug-in has finished working with a drawable, before exiting
 * it should call gimp_drawable_detach() to make sure that all tile data is
 * transferred back to the core.
 *
 * Return value: a new #GimpDrawable wrapper
 **/
GimpDrawable *
gimp_drawable_get (gint32 drawable_ID)
{
  GimpDrawable *drawable;
  gint          width;
  gint          height;
  gint          bpp;

  width  = gimp_drawable_width  (drawable_ID);
  height = gimp_drawable_height (drawable_ID);
  bpp    = gimp_drawable_bpp    (drawable_ID);

  g_return_val_if_fail (width > 0 && height > 0 && bpp > 0, NULL);

  drawable = g_slice_new0 (GimpDrawable);

  drawable->drawable_id  = drawable_ID;
  drawable->width        = width;
  drawable->height       = height;
  drawable->bpp          = bpp;
  drawable->ntile_rows   = (height + TILE_HEIGHT - 1) / TILE_HEIGHT;
  drawable->ntile_cols   = (width  + TILE_WIDTH  - 1) / TILE_WIDTH;

  return drawable;
}

/**
 * gimp_drawable_detach:
 * @drawable: The #GimpDrawable to detach from the core
 *
 * This function is called when a plug-in is finished working
 * with a drawable.  It forces all tile data held in the tile
 * list of the #GimpDrawable to be transferred to the core, and
 * then frees all associated memory. You must not access the
 * @drawable after having called gimp_drawable_detach().
 **/
void
gimp_drawable_detach (GimpDrawable *drawable)
{
  g_return_if_fail (drawable != NULL);

  gimp_drawable_flush (drawable);

  if (drawable->tiles)
    g_free (drawable->tiles);

  if (drawable->shadow_tiles)
    g_free (drawable->shadow_tiles);

  g_slice_free (GimpDrawable, drawable);
}

/**
 * gimp_drawable_flush:
 * @drawable: The #GimpDrawable whose tile data is to be transferred
 * to the core.
 *
 * This function causes all tile data in the tile list of @drawable to be
 * transferred to the core.  It is usually called in situations where a
 * plug-in acts on a drawable, and then needs to read the results of its
 * actions.  Data transferred back from the core will not generally be valid
 * unless gimp_drawable_flush() has been called beforehand.
 **/
void
gimp_drawable_flush (GimpDrawable *drawable)
{
  GimpTile *tiles;
  gint      n_tiles;
  gint      i;

  g_return_if_fail (drawable != NULL);

  if (drawable->tiles)
    {
      tiles   = drawable->tiles;
      n_tiles = drawable->ntile_rows * drawable->ntile_cols;

      for (i = 0; i < n_tiles; i++)
        if ((tiles[i].ref_count > 0) && tiles[i].dirty)
          gimp_tile_flush (&tiles[i]);
    }

  if (drawable->shadow_tiles)
    {
      tiles   = drawable->shadow_tiles;
      n_tiles = drawable->ntile_rows * drawable->ntile_cols;

      for (i = 0; i < n_tiles; i++)
        if ((tiles[i].ref_count > 0) && tiles[i].dirty)
          gimp_tile_flush (&tiles[i]);
    }

  /*  nuke all references to this drawable from the cache  */
  _gimp_tile_cache_flush_drawable (drawable);
}

GimpTile *
gimp_drawable_get_tile (GimpDrawable *drawable,
                        gboolean      shadow,
                        gint          row,
                        gint          col)
{
  GimpTile *tiles;
  guint     right_tile;
  guint     bottom_tile;
  gint      n_tiles;
  gint      tile_num;
  gint      i, j, k;

  g_return_val_if_fail (drawable != NULL, NULL);

  if (shadow)
    tiles = drawable->shadow_tiles;
  else
    tiles = drawable->tiles;

  if (! tiles)
    {
      n_tiles = drawable->ntile_rows * drawable->ntile_cols;
      tiles = g_new (GimpTile, n_tiles);

      right_tile  = (drawable->width  -
                     ((drawable->ntile_cols - 1) * TILE_WIDTH));
      bottom_tile = (drawable->height -
                     ((drawable->ntile_rows - 1) * TILE_HEIGHT));

      for (i = 0, k = 0; i < drawable->ntile_rows; i++)
        {
          for (j = 0; j < drawable->ntile_cols; j++, k++)
            {
              tiles[k].bpp       = drawable->bpp;
              tiles[k].tile_num  = k;
              tiles[k].ref_count = 0;
              tiles[k].dirty     = FALSE;
              tiles[k].shadow    = shadow;
              tiles[k].data      = NULL;
              tiles[k].drawable  = drawable;

              if (j == (drawable->ntile_cols - 1))
                tiles[k].ewidth  = right_tile;
              else
                tiles[k].ewidth  = TILE_WIDTH;

              if (i == (drawable->ntile_rows - 1))
                tiles[k].eheight = bottom_tile;
              else
                tiles[k].eheight = TILE_HEIGHT;
            }
        }

      if (shadow)
        drawable->shadow_tiles = tiles;
      else
        drawable->tiles = tiles;
    }

  tile_num = row * drawable->ntile_cols + col;

  return &tiles[tile_num];
}

GimpTile *
gimp_drawable_get_tile2 (GimpDrawable *drawable,
                         gboolean      shadow,
                         gint          x,
                         gint          y)
{
  gint row;
  gint col;

  g_return_val_if_fail (drawable != NULL, NULL);

  col = x / TILE_WIDTH;
  row = y / TILE_HEIGHT;

  return gimp_drawable_get_tile (drawable, shadow, row, col);
}

void
gimp_drawable_get_color_uchar (gint32         drawable_ID,
                               const GimpRGB *color,
                               guchar        *color_uchar)
{
  g_return_if_fail (color != NULL);
  g_return_if_fail (color_uchar != NULL);

  switch (gimp_drawable_type (drawable_ID))
    {
    case GIMP_RGB_IMAGE:
      gimp_rgb_get_uchar (color,
                          &color_uchar[0], &color_uchar[1], &color_uchar[2]);
      color_uchar[3] = 255;
      break;

    case GIMP_RGBA_IMAGE:
      gimp_rgba_get_uchar (color,
                           &color_uchar[0], &color_uchar[1], &color_uchar[2],
                           &color_uchar[3]);
      break;

    case GIMP_GRAY_IMAGE:
      color_uchar[0] = gimp_rgb_luminance_uchar (color);
      color_uchar[1] = 255;
      break;

    case GIMP_GRAYA_IMAGE:
      color_uchar[0] = gimp_rgb_luminance_uchar (color);
      gimp_rgba_get_uchar (color, NULL, NULL, NULL, &color_uchar[1]);
      break;

    default:
      break;
    }
}

guchar *
gimp_drawable_get_thumbnail_data (gint32  drawable_ID,
                                  gint   *width,
                                  gint   *height,
                                  gint   *bpp)
{
  gint    ret_width;
  gint    ret_height;
  guchar *image_data;
  gint    data_size;

  _gimp_drawable_thumbnail (drawable_ID,
                            *width,
                            *height,
                            &ret_width,
                            &ret_height,
                            bpp,
                            &data_size,
                            &image_data);

  *width  = ret_width;
  *height = ret_height;

  return image_data;
}

guchar *
gimp_drawable_get_sub_thumbnail_data (gint32  drawable_ID,
                                      gint    src_x,
                                      gint    src_y,
                                      gint    src_width,
                                      gint    src_height,
                                      gint   *dest_width,
                                      gint   *dest_height,
                                      gint   *bpp)
{
  gint    ret_width;
  gint    ret_height;
  guchar *image_data;
  gint    data_size;

  _gimp_drawable_sub_thumbnail (drawable_ID,
                                src_x, src_y,
                                src_width, src_height,
                                *dest_width,
                                *dest_height,
                                &ret_width,
                                &ret_height,
                                bpp,
                                &data_size,
                                &image_data);

  *dest_width  = ret_width;
  *dest_height = ret_height;

  return image_data;
}

/**
 * gimp_drawable_attach_new_parasite:
 * @drawable_ID: the ID of the #GimpDrawable to attach the #GimpParasite to.
 * @name: the name of the #GimpParasite to create and attach.
 * @flags: the flags set on the #GimpParasite.
 * @size: the size of the parasite data in bytes.
 * @data: a pointer to the data attached with the #GimpParasite.
 *
 * Convenience function that creates a parasite and attaches it
 * to GIMP.
 *
 * Return value: TRUE on successful creation and attachment of
 * the new parasite.
 *
 * See Also: gimp_drawable_parasite_attach()
 */
gboolean
gimp_drawable_attach_new_parasite (gint32          drawable_ID,
                                   const gchar    *name,
                                   gint            flags,
                                   gint            size,
                                   gconstpointer   data)
{
  GimpParasite *parasite = gimp_parasite_new (name, flags, size, data);
  gboolean      success;

  success = gimp_drawable_parasite_attach (drawable_ID, parasite);

  gimp_parasite_free (parasite);

  return success;
}
