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
 * @light: return location for the light shade
 * @dark:  return location for the dark shade
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
 * @type:         the checkerboard type
 * @light:        return location for the light color
 * @dark:         return location for the dark color
 * @light_custom: the light color to use if type is custom
 * @dark_custom:  the dark color to use if type is custom
 **/
void
gimp_checks_get_colors (GimpCheckType  type,
                        GimpRGB       *light,
                        GimpRGB       *dark,
                        GimpRGB       *light_custom,
                        GimpRGB       *dark_custom)
{
  if (light)
    {
      switch (type)
        {
        case GIMP_CHECK_TYPE_LIGHT_CHECKS:
          *light = GIMP_CHECKS_LIGHT_COLOR_LIGHT;
          break;
        case GIMP_CHECK_TYPE_DARK_CHECKS:
          *light = GIMP_CHECKS_DARK_COLOR_LIGHT;
          break;
        case GIMP_CHECK_TYPE_WHITE_ONLY:
          *light = GIMP_CHECKS_WHITE_COLOR;
          break;
        case GIMP_CHECK_TYPE_GRAY_ONLY:
          *light = GIMP_CHECKS_GRAY_COLOR;
          break;
        case GIMP_CHECK_TYPE_BLACK_ONLY:
          *light = GIMP_CHECKS_BLACK_COLOR;
          break;
        case GIMP_CHECK_TYPE_CUSTOM_CHECKS:
          *light = *light_custom;
          break;
        default:
          *light = GIMP_CHECKS_GRAY_COLOR_LIGHT;
          break;
        }
    }

  if (dark)
    {
      switch (type)
        {
        case GIMP_CHECK_TYPE_LIGHT_CHECKS:
          *dark = GIMP_CHECKS_LIGHT_COLOR_DARK;
          break;
        case GIMP_CHECK_TYPE_DARK_CHECKS:
          *dark = GIMP_CHECKS_DARK_COLOR_DARK;
          break;
        case GIMP_CHECK_TYPE_WHITE_ONLY:
          *dark = GIMP_CHECKS_WHITE_COLOR;
          break;
        case GIMP_CHECK_TYPE_GRAY_ONLY:
          *dark = GIMP_CHECKS_GRAY_COLOR;
          break;
        case GIMP_CHECK_TYPE_BLACK_ONLY:
          *dark = GIMP_CHECKS_BLACK_COLOR;
          break;
        case GIMP_CHECK_TYPE_CUSTOM_CHECKS:
          *dark = *dark_custom;
          break;
        default:
          *dark = GIMP_CHECKS_GRAY_COLOR_DARK;
          break;
        }
    }
}
