/*
 * Animation Playback plug-in version 0.99.1
 *
 * (c) Jehan : 2012-2014 : jehan at girinstud.io
 *
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#undef GDK_DISABLE_DEPRECATED
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "animation-utils.h"

/* total_alpha_preview:
 * Fill the @drawing_data with an alpha (grey chess) pattern.
 * This uses a static array, copied over each line (with some shift to
 * reproduce the pattern), using `memcpy()`.
 * The reason why we keep the pattern in the statically allocated memory,
 * instead of simply looping through @drawing_data and recreating the
 * pattern is simply because `memcpy()` implementations are supposed to
 * be more efficient than loops over an array. */
void
total_alpha_preview (guchar *drawing_data,
                     guint   drawing_width,
                     guint   drawing_height)
{
  static guint   alpha_line_width = 0;
  static guchar *alpha_line       = NULL;
  gint           i;

  g_assert (drawing_width > 0);

  /* If width change, we update the "alpha" line. */
  if (alpha_line_width < drawing_width + 8)
    {
      alpha_line_width = drawing_width + 8;

      g_free (alpha_line);

      /* A full line + 8 pixels (1 square). */
      alpha_line = g_malloc (alpha_line_width * 3);

      for (i = 0; i < alpha_line_width; i++)
        {
          /* 8 pixels dark grey, 8 pixels light grey, and so on. */
          if (i & 8)
            {
              alpha_line[i * 3 + 0] =
                alpha_line[i * 3 + 1] =
                alpha_line[i * 3 + 2] = 102;
            }
          else
            {
              alpha_line[i * 3 + 0] =
                alpha_line[i * 3 + 1] =
                alpha_line[i * 3 + 2] = 154;
            }
        }
    }

  for (i = 0; i < drawing_height; i++)
    {
      if (i & 8)
        {
          memcpy (&drawing_data[i * 3 * drawing_width],
                  alpha_line,
                  3 * drawing_width);
        }
      else
        {
          /* Every 8 vertical pixels, we shift the horizontal line by 8 pixels. */
          memcpy (&drawing_data[i * 3 * drawing_width],
                  alpha_line + 24,
                  3 * drawing_width);
        }
    }
}
