/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattisbvf
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

#include "core-types.h"

#include "gimpimage.h"
#include "gimpimage-unit.h"
#include "gimpunit.h"


gdouble
gimp_image_unit_get_factor (GimpImage *gimage)
{
  return _gimp_unit_get_factor (gimage->gimp, gimage->unit);
}

gint
gimp_image_unit_get_digits (GimpImage *gimage)
{
  return _gimp_unit_get_digits (gimage->gimp, gimage->unit);
}

const gchar *
gimp_image_unit_get_identifier (GimpImage *gimage)
{
  return _gimp_unit_get_identifier (gimage->gimp, gimage->unit);
}

const gchar *
gimp_image_unit_get_symbol (GimpImage *gimage)
{
  return _gimp_unit_get_symbol (gimage->gimp, gimage->unit);
}

const gchar *
gimp_image_unit_get_abbreviation (GimpImage *gimage)
{
  return _gimp_unit_get_abbreviation (gimage->gimp, gimage->unit);
}

const gchar *
gimp_image_unit_get_singular (GimpImage *gimage)
{
  return _gimp_unit_get_singular (gimage->gimp, gimage->unit);
}

const gchar *
gimp_image_unit_get_plural (GimpImage *gimage)
{
  return _gimp_unit_get_plural (gimage->gimp, gimage->unit);
}
