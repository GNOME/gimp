/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpdashpattern.c
 * Copyright (C) 2003 Simon Budig
 * Copyright (C) 2005 Sven Neumann
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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpdashpattern.h"


GArray *
gimp_dash_pattern_from_preset (GimpDashPreset  preset)
{
  GArray *pattern;
  gdouble dash;
  gint    i;

  pattern = g_array_new (FALSE, FALSE, sizeof (gdouble));

  switch (preset)
    {
    case GIMP_DASH_LINE:
      break;
    case GIMP_DASH_LONG_DASH:
      dash = 9.0; g_array_append_val (pattern, dash);
      dash = 3.0; g_array_append_val (pattern, dash);
      break;
    case GIMP_DASH_MEDIUM_DASH:
      dash = 6.0; g_array_append_val (pattern, dash);
      dash = 6.0; g_array_append_val (pattern, dash);
      break;
    case GIMP_DASH_SHORT_DASH:
      dash = 3.0; g_array_append_val (pattern, dash);
      dash = 9.0; g_array_append_val (pattern, dash);
      break;
    case GIMP_DASH_SPARSE_DOTS:
      for (i = 0; i < 2; i++)
        {
          dash = 1.0; g_array_append_val (pattern, dash);
          dash = 5.0; g_array_append_val (pattern, dash);
        }
      break;
    case GIMP_DASH_NORMAL_DOTS:
      for (i = 0; i < 3; i++)
        {
          dash = 1.0; g_array_append_val (pattern, dash);
          dash = 3.0; g_array_append_val (pattern, dash);
        }
      break;
    case GIMP_DASH_DENSE_DOTS:
      for (i = 0; i < 12; i++)
        {
          dash = 1.0; g_array_append_val (pattern, dash);
        }
      break;
    case GIMP_DASH_STIPPLES:
      for (i = 0; i < 24; i++)
        {
          dash = 0.5; g_array_append_val (pattern, dash);
        }
      break;
    case GIMP_DASH_DASH_DOT:
      dash = 7.0; g_array_append_val (pattern, dash);
      dash = 2.0; g_array_append_val (pattern, dash);
      dash = 1.0; g_array_append_val (pattern, dash);
      dash = 2.0; g_array_append_val (pattern, dash);
      break;
    case GIMP_DASH_DASH_DOT_DOT:
      dash = 7.0; g_array_append_val (pattern, dash);
      for (i=0; i < 5; i++)
        {
          dash = 1.0; g_array_append_val (pattern, dash);
        }
      break;
    case GIMP_DASH_CUSTOM:
      g_warning ("GIMP_DASH_CUSTOM passed to gimp_dash_pattern_from_preset()");
    }

  if (pattern->len < 2)
    {
      g_array_free (pattern, TRUE);
      return NULL;
    }

  return pattern;
}
