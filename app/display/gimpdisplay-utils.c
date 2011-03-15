/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdisplay-utils.c
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

/**
 * Utility functions for the display module (not only the GimpDisplay
 * class).
 */

#include "config.h"

#include <gtk/gtk.h>

#include "display-types.h"

#include "gimpdisplay-utils.h"
#include "gimpdisplay.h"


/**
 * gimp_display_get_action_name:
 * @display:
 *
 * Returns: The action name for the given display. The action name
 * depends on the display ID. The result must be freed with g_free().
 **/
gchar *
gimp_display_get_action_name (GimpDisplay *display)
{
  return g_strdup_printf ("windows-display-%04d",
                          gimp_display_get_ID (display));
}
