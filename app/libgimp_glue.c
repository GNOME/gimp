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

#include "libgimpcolor/gimpcolor.h"

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpunit.h"

#include "app_procs.h"

#define __LIBGIMP_GLUE_C__
#include "libgimp_glue.h"


gboolean
gimp_palette_set_foreground (const GimpRGB *color)
{
  g_return_val_if_fail (color != NULL, FALSE);

  gimp_context_set_foreground (gimp_get_user_context (the_gimp), color);

  return TRUE;
}

gboolean
gimp_palette_get_foreground (GimpRGB *color)
{
  g_return_val_if_fail (color != NULL, FALSE);

  gimp_context_get_foreground (gimp_get_user_context (the_gimp), color);

  return TRUE;
}

gboolean
gimp_palette_set_background (const GimpRGB *color)
{
  g_return_val_if_fail (color != NULL, FALSE);

  gimp_context_set_background (gimp_get_user_context (the_gimp), color);

  return TRUE;
}

gboolean
gimp_palette_get_background (GimpRGB *color)
{
  g_return_val_if_fail (color != NULL, FALSE);

  gimp_context_get_background (gimp_get_user_context (the_gimp), color);

  return TRUE;
}

gint
gimp_unit_get_number_of_units (void)
{
  return _gimp_unit_get_number_of_units (the_gimp);
}

gint
gimp_unit_get_number_of_built_in_units (void)
{
  return GIMP_UNIT_END;
}

GimpUnit
gimp_unit_new (gchar   *identifier,
	       gdouble  factor,
	       gint     digits,
	       gchar   *symbol,
	       gchar   *abbreviation,
	       gchar   *singular,
	       gchar   *plural)
{
  return _gimp_unit_new (the_gimp,
			 identifier,
			 factor,
			 digits,
			 symbol,
			 abbreviation,
			 singular,
			 plural);
}


gboolean
gimp_unit_get_deletion_flag (GimpUnit unit)
{
  return _gimp_unit_get_deletion_flag (the_gimp, unit);
}

void
gimp_unit_set_deletion_flag (GimpUnit unit,
			     gboolean deletion_flag)
{
  _gimp_unit_set_deletion_flag (the_gimp, unit, deletion_flag);
}


gdouble
gimp_unit_get_factor (GimpUnit unit)
{
  return _gimp_unit_get_factor (the_gimp, unit);
}


gint
gimp_unit_get_digits (GimpUnit unit)
{
  return _gimp_unit_get_digits (the_gimp, unit);
}


const gchar * 
gimp_unit_get_identifier (GimpUnit unit)
{
  return _gimp_unit_get_identifier (the_gimp, unit);
}


const gchar *
gimp_unit_get_symbol (GimpUnit unit)
{
  return _gimp_unit_get_symbol (the_gimp, unit);
}


const gchar *
gimp_unit_get_abbreviation (GimpUnit unit)
{
  return _gimp_unit_get_abbreviation (the_gimp, unit);
}


const gchar *
gimp_unit_get_singular (GimpUnit unit)
{
  return _gimp_unit_get_singular (the_gimp, unit);
}


const gchar *
gimp_unit_get_plural (GimpUnit unit)
{
  return _gimp_unit_get_plural (the_gimp, unit);
}
