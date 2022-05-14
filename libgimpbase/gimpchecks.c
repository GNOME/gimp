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
 * gimp_checks_get_colors:
 * @type:            the checkerboard type
 * @color1: (inout): current custom color and return location for the first color.
 * @color2: (inout): current custom color and return location for the second color.
 *
 * Retrieves the colors to use when drawing a checkerboard for a certain
 * #GimpCheckType and custom colors.
 * If @type is %GIMP_CHECK_TYPE_CUSTOM_CHECKS, then @color1 and @color2
 * will remain untouched, which means you must initialize them to the
 * values expected for custom checks.
 *
 * To obtain the user-set colors in Preferences, just call:
 * |[<!-- language="C" -->
 * GimpRGB color1 = *(gimp_check_custom_color1 ());
 * GimpRGB color2 = *(gimp_check_custom_color2 ());
 * gimp_checks_get_colors (gimp_check_type (), &color1, &color2);
 * ]|
 *
 * Since: 3.0
 **/
void
gimp_checks_get_colors (GimpCheckType  type,
                        GimpRGB       *color1,
                        GimpRGB       *color2)
{
  g_return_if_fail (color1 != NULL || color2 != NULL);

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
          /* Keep the current value. */
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
          /* Keep the current value. */
          break;
        default:
          *color2 = GIMP_CHECKS_GRAY_COLOR_DARK;
          break;
        }
    }
}
