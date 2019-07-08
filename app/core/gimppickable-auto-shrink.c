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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimppickable.h"
#include "gimppickable-auto-shrink.h"


typedef enum
{
  AUTO_SHRINK_NOTHING = 0,
  AUTO_SHRINK_ALPHA   = 1,
  AUTO_SHRINK_COLOR   = 2
} AutoShrinkType;


typedef gboolean (* ColorsEqualFunc) (guchar *col1,
                                      guchar *col2);


/*  local function prototypes  */

static AutoShrinkType   gimp_pickable_guess_bgcolor (GimpPickable *pickable,
                                                     guchar       *color,
                                                     gint          x1,
                                                     gint          x2,
                                                     gint          y1,
                                                     gint          y2);
static gboolean         gimp_pickable_colors_equal  (guchar       *col1,
                                                     guchar       *col2);
static gboolean         gimp_pickable_colors_alpha  (guchar       *col1,
                                                     guchar       *col2);


/*  public functions  */

GimpAutoShrink
gimp_pickable_auto_shrink (GimpPickable *pickable,
                           gint          start_x,
                           gint          start_y,
                           gint          start_width,
                           gint          start_height,
                           gint         *shrunk_x,
                           gint         *shrunk_y,
                           gint         *shrunk_width,
                           gint         *shrunk_height)
{
  GeglBuffer      *buffer;
  GeglRectangle    rect;
  ColorsEqualFunc  colors_equal_func;
  guchar           bgcolor[MAX_CHANNELS] = { 0, 0, 0, 0 };
  guchar          *buf = NULL;
  gint             x1, y1, x2, y2;
  gint             width, height;
  const Babl      *format;
  gint             x, y, abort;
  GimpAutoShrink   retval = GIMP_AUTO_SHRINK_UNSHRINKABLE;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), FALSE);
  g_return_val_if_fail (shrunk_x != NULL, FALSE);
  g_return_val_if_fail (shrunk_y != NULL, FALSE);
  g_return_val_if_fail (shrunk_width != NULL, FALSE);
  g_return_val_if_fail (shrunk_height != NULL, FALSE);

  gimp_set_busy (gimp_pickable_get_image (pickable)->gimp);

  /* You should always keep in mind that x2 and y2 are the NOT the
   * coordinates of the bottomright corner of the area to be
   * cropped. They point at the pixel located one to the right and one
   * to the bottom.
   */

  gimp_pickable_flush (pickable);

  buffer = gimp_pickable_get_buffer (pickable);

  x1 = MAX (start_x, 0);
  y1 = MAX (start_y, 0);
  x2 = MIN (start_x + start_width,  gegl_buffer_get_width  (buffer));
  y2 = MIN (start_y + start_height, gegl_buffer_get_height (buffer));

  /* By default, return the start values */
  *shrunk_x      = x1;
  *shrunk_y      = y1;
  *shrunk_width  = x2 - x1;
  *shrunk_height = y2 - y1;

  format = babl_format ("R'G'B'A u8");

  switch (gimp_pickable_guess_bgcolor (pickable, bgcolor,
                                       x1, x2 - 1, y1, y2 - 1))
    {
    case AUTO_SHRINK_ALPHA:
      colors_equal_func = gimp_pickable_colors_alpha;
      break;
    case AUTO_SHRINK_COLOR:
      colors_equal_func = gimp_pickable_colors_equal;
      break;
    default:
      goto FINISH;
      break;
    }

  width  = x2 - x1;
  height = y2 - y1;

  /* The following could be optimized further by processing
   * the smaller side first instead of defaulting to width    --Sven
   */

  buf = g_malloc (MAX (width, height) * 4);

  /* Check how many of the top lines are uniform/transparent. */
  rect.x      = x1;
  rect.y      = 0;
  rect.width  = width;
  rect.height = 1;

  abort = FALSE;
  for (y = y1; y < y2 && !abort; y++)
    {
      rect.y = y;
      gegl_buffer_get (buffer, &rect, 1.0, format, buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      for (x = 0; x < width && !abort; x++)
        abort = ! colors_equal_func (bgcolor, buf + x * 4);
    }
  if (y == y2 && !abort)
    {
      retval = GIMP_AUTO_SHRINK_EMPTY;
      goto FINISH;
    }
  y1 = y - 1;

  /* Check how many of the bottom lines are uniform/transparent. */
  rect.x      = x1;
  rect.y      = 0;
  rect.width  = width;
  rect.height = 1;

  abort = FALSE;
  for (y = y2; y > y1 && !abort; y--)
    {
      rect.y = y - 1;
      gegl_buffer_get (buffer, &rect, 1.0, format, buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      for (x = 0; x < width && !abort; x++)
        abort = ! colors_equal_func  (bgcolor, buf + x * 4);
    }
  y2 = y + 1;

  /* compute a new height for the next operations */
  height = y2 - y1;

  /* Check how many of the left lines are uniform/transparent. */
  rect.x      = 0;
  rect.y      = y1;
  rect.width  = 1;
  rect.height = height;

  abort = FALSE;
  for (x = x1; x < x2 && !abort; x++)
    {
      rect.x = x;
      gegl_buffer_get (buffer, &rect, 1.0, format, buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      for (y = 0; y < height && !abort; y++)
        abort = ! colors_equal_func (bgcolor, buf + y * 4);
    }
  x1 = x - 1;

  /* Check how many of the right lines are uniform/transparent. */
  rect.x      = 0;
  rect.y      = y1;
  rect.width  = 1;
  rect.height = height;

  abort = FALSE;
  for (x = x2; x > x1 && !abort; x--)
    {
      rect.x = x - 1;
      gegl_buffer_get (buffer, &rect, 1.0, format, buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      for (y = 0; y < height && !abort; y++)
        abort = ! colors_equal_func (bgcolor, buf + y * 4);
    }
  x2 = x + 1;

  if (x1      != start_x     ||
      y1      != start_y     ||
      x2 - x1 != start_width ||
      y2 - y1 != start_height)
    {
      *shrunk_x      = x1;
      *shrunk_y      = y1;
      *shrunk_width  = x2 - x1;
      *shrunk_height = y2 - y1;

      retval = GIMP_AUTO_SHRINK_SHRINK;
    }

 FINISH:

  g_free (buf);
  gimp_unset_busy (gimp_pickable_get_image (pickable)->gimp);

  return retval;
}


/*  private functions  */

static AutoShrinkType
gimp_pickable_guess_bgcolor (GimpPickable *pickable,
                             guchar       *color,
                             gint          x1,
                             gint          x2,
                             gint          y1,
                             gint          y2)
{
  const Babl *format = babl_format ("R'G'B'A u8");
  guchar      tl[4];
  guchar      tr[4];
  guchar      bl[4];
  guchar      br[4];
  gint        i;

  for (i = 0; i < 4; i++)
    color[i] = 0;

  /* First check if there's transparency to crop. If not, guess the
   * background-color to see if at least 2 corners are equal.
   */

  if (! gimp_pickable_get_pixel_at (pickable, x1, y1, format, tl) ||
      ! gimp_pickable_get_pixel_at (pickable, x1, y2, format, tr) ||
      ! gimp_pickable_get_pixel_at (pickable, x2, y1, format, bl) ||
      ! gimp_pickable_get_pixel_at (pickable, x2, y2, format, br))
    {
      return AUTO_SHRINK_NOTHING;
    }

  if ((tl[ALPHA] == 0 && tr[ALPHA] == 0) ||
      (tl[ALPHA] == 0 && bl[ALPHA] == 0) ||
      (tr[ALPHA] == 0 && br[ALPHA] == 0) ||
      (bl[ALPHA] == 0 && br[ALPHA] == 0))
    {
      return AUTO_SHRINK_ALPHA;
    }

  if (gimp_pickable_colors_equal (tl, tr) ||
      gimp_pickable_colors_equal (tl, bl))
    {
      memcpy (color, tl, 4);
      return AUTO_SHRINK_COLOR;
    }

  if (gimp_pickable_colors_equal (br, bl) ||
      gimp_pickable_colors_equal (br, tr))
    {
      memcpy (color, br, 4);
      return AUTO_SHRINK_COLOR;
    }

  return AUTO_SHRINK_NOTHING;
}

static gboolean
gimp_pickable_colors_equal (guchar *col1,
                            guchar *col2)
{
  gint b;

  for (b = 0; b < 4; b++)
    {
      if (col1[b] != col2[b])
        return FALSE;
    }

  return TRUE;
}

static gboolean
gimp_pickable_colors_alpha (guchar *dummy,
                            guchar *col)
{
  return (col[ALPHA] == 0);
}
