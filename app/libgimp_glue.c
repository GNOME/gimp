/* The GIMP -- an image manipulation program
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

#include <gtk/gtk.h>

#include "apptypes.h"

#include "gimpcontext.h"
#include "libgimp_glue.h"


gboolean
gimp_palette_set_foreground (guchar r,
			     guchar g,
			     guchar b)
{
  GimpRGB color;

  gimp_rgba_set_uchar (&color, r, g, b, 255);

  gimp_context_set_foreground (NULL, &color);

  return TRUE;
}

gboolean
gimp_palette_get_foreground (guchar *r,
			     guchar *g,
			     guchar *b)
{
  GimpRGB color;

  gimp_context_get_foreground (NULL, &color);

  gimp_rgb_get_uchar (&color, r, g, b);

  return TRUE;
}

gboolean
gimp_palette_set_foreground_rgb (const GimpRGB *rgb)
{
  guchar r, g, b;

  g_return_val_if_fail (rgb != NULL, FALSE);

  gimp_rgb_get_uchar (rgb, &r, &g, &b);

  return gimp_palette_set_foreground (r, g, b);
}

gboolean
gimp_palette_get_foreground_rgb (GimpRGB *rgb)
{
  guchar r, g, b;
  
  g_return_val_if_fail (rgb != NULL, FALSE);

  if (gimp_palette_get_foreground (&r, &g, &b))
    {
      gimp_rgb_set_uchar (rgb, r, g, b);
      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_palette_set_background (guchar r,
			     guchar g,
			     guchar b)
{
  GimpRGB color;

  gimp_rgba_set_uchar (&color, r, g, b, 255);

  gimp_context_set_background (NULL, &color);

  return TRUE;
}

gboolean
gimp_palette_get_background (guchar *r,
			     guchar *g,
			     guchar *b)
{
  GimpRGB color;

  gimp_context_get_background (NULL, &color);

  gimp_rgb_get_uchar (&color, r, g, b);

  return TRUE;
}

gboolean
gimp_palette_set_background_rgb (const GimpRGB *rgb)
{
  guchar r, g, b;

  g_return_val_if_fail (rgb != NULL, FALSE);

  gimp_rgb_get_uchar (rgb, &r, &g, &b);

  return gimp_palette_set_background (r, g, b);
}

gboolean
gimp_palette_get_background_rgb (GimpRGB *rgb)
{
  guchar r, g, b;
  
  g_return_val_if_fail (rgb != NULL, FALSE);

  if (gimp_palette_get_background (&r, &g, &b))
    {
      gimp_rgb_set_uchar (rgb, r, g, b);
      return TRUE;
    }

  return FALSE;
}
