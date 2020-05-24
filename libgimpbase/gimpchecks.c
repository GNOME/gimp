/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpchecks.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#include <glib-object.h>

#include "gimpbasetypes.h"

#include "gimpchecks.h"


/**
 * SECTION: gimpchecks
 * @title: gimpchecks
 * @short_description: Constants and functions related to rendering
 *                     checkerboards.
 *
 * Constants and functions related to rendering checkerboards.
 **/


/**
 * gimp_checks_get_shades:
 * @type:  the checkerboard type
 * @light: (out) (optional): return location for the light shade
 * @dark:  (out) (optional): return location for the dark shade
 *
 * Deprecated: use gimp_checks_get_colors() instead.
 *
 * Retrieves the actual shades of gray to use when drawing a
 * checkerboard for a certain #GimpCheckType.
 *
 * Since: 2.2
 **/
void
gimp_checks_get_shades (GimpCheckType  type,
                        guchar        *light,
                        guchar        *dark)
{
  const guchar shades[6][2] =
    {
      { 204, 255 },  /*  LIGHT_CHECKS  */
      { 102, 153 },  /*  GRAY_CHECKS   */
      {   0,  51 },  /*  DARK_CHECKS   */
      { 255, 255 },  /*  WHITE_ONLY    */
      { 127, 127 },  /*  GRAY_ONLY     */
      {   0,   0 }   /*  BLACK_ONLY    */
    };

  type = MIN (type, 5);

  if (light)
    *light = shades[type][1];
  if (dark)
    *dark  = shades[type][0];
}

/**
 * gimp_checks_get_colors:
 * @type:          the checkerboard type
 * @color1:        return location for the first color,
 *                 usually the light color
 * @color2:        return location for the second color,
 *                 usually the dark color
 * @color1_custom: the first color to return if type is custom
 * @color2_custom: the second color to return if type is custom
 **/
void
gimp_checks_get_colors (GimpCheckType  type,
                        GimpRGB       *color1,
                        GimpRGB       *color2,
                        GimpRGB        color1_custom,
                        GimpRGB        color2_custom)
{
  if (color1)
    {
      switch (type)
        {
        case GIMP_CHECK_TYPE_LIGHT_CHECKS:
          *color1 = GIMP_CHECKS_LIGHT_COLOR_LIGHT;
          break;
        case GIMP_CHECK_TYPE_DARK_CHECKS:
          *color1 = GIMP_CHECKS_DARK_COLOR_LIGHT;
          break;
        case GIMP_CHECK_TYPE_WHITE_ONLY:
          *color1 = GIMP_CHECKS_WHITE_COLOR;
          break;
        case GIMP_CHECK_TYPE_GRAY_ONLY:
          *color1 = GIMP_CHECKS_GRAY_COLOR;
          break;
        case GIMP_CHECK_TYPE_BLACK_ONLY:
          *color1 = GIMP_CHECKS_BLACK_COLOR;
          break;
        case GIMP_CHECK_TYPE_CUSTOM_CHECKS:
          *color1 = color1_custom;
          break;
        default:
          *color1 = GIMP_CHECKS_GRAY_COLOR_LIGHT;
          break;
        }
    }

  if (color2)
    {
      switch (type)
        {
        case GIMP_CHECK_TYPE_LIGHT_CHECKS:
          *color2 = GIMP_CHECKS_LIGHT_COLOR_DARK;
          break;
        case GIMP_CHECK_TYPE_DARK_CHECKS:
          *color2 = GIMP_CHECKS_DARK_COLOR_DARK;
          break;
        case GIMP_CHECK_TYPE_WHITE_ONLY:
          *color2 = GIMP_CHECKS_WHITE_COLOR;
          break;
        case GIMP_CHECK_TYPE_GRAY_ONLY:
          *color2 = GIMP_CHECKS_GRAY_COLOR;
          break;
        case GIMP_CHECK_TYPE_BLACK_ONLY:
          *color2 = GIMP_CHECKS_BLACK_COLOR;
          break;
        case GIMP_CHECK_TYPE_CUSTOM_CHECKS:
          *color2 = color2_custom;
          break;
        default:
          *color2 = GIMP_CHECKS_GRAY_COLOR_DARK;
          break;
        }
    }
}
