/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimppalette.c
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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"

/**
 * gimp_palette_get_foreground:
 * @foreground: The foreground color.
 *
 * Get the current GIMP foreground color.
 *
 * This procedure retrieves the current GIMP foreground color. The
 * foreground color is used in a variety of tools such as paint tools,
 * blending, and bucket fill.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_palette_get_foreground (GimpRGB *foreground)
{
  return gimp_context_get_foreground (foreground);
}

/**
 * gimp_palette_get_background:
 * @background: The background color.
 *
 * Get the current GIMP background color.
 *
 * This procedure retrieves the current GIMP background color. The
 * background color is used in a variety of tools such as blending,
 * erasing (with non-alpha images), and image filling.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_palette_get_background (GimpRGB *background)
{
  return gimp_context_get_background (background);
}

/**
 * gimp_palette_set_foreground:
 * @foreground: The foreground color.
 *
 * Set the current GIMP foreground color.
 *
 * This procedure sets the current GIMP foreground color. After this is
 * set, operations which use foreground such as paint tools, blending,
 * and bucket fill will use the new value.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_palette_set_foreground (const GimpRGB *foreground)
{
  return gimp_context_set_foreground (foreground);
}

/**
 * gimp_palette_set_background:
 * @background: The background color.
 *
 * Set the current GIMP background color.
 *
 * This procedure sets the current GIMP background color. After this is
 * set, operations which use background such as blending, filling
 * images, clearing, and erasing (in non-alpha images) will use the new
 * value.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_palette_set_background (const GimpRGB *background)
{
  return gimp_context_set_background (background);
}

/**
 * gimp_palette_set_default_colors:
 *
 * Set the current GIMP foreground and background colors to black and
 * white.
 *
 * This procedure sets the current GIMP foreground and background
 * colors to their initial default values, black and white.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_palette_set_default_colors (void)
{
  return gimp_context_set_default_colors ();
}

/**
 * gimp_palette_swap_colors:
 *
 * Swap the current GIMP foreground and background colors.
 *
 * This procedure swaps the current GIMP foreground and background
 * colors, so that the new foreground color becomes the old background
 * color and vice versa.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_palette_swap_colors (void)
{
  return gimp_context_swap_colors ();
}
