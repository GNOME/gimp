/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitparser.h
 * Copyright (C) 2011 Enrico Schröder <enni.schroeder@gmail.com>
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
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_UNIT_PARSER_H__
#define __GIMP_UNIT_PARSER_H__

#include <glib-object.h>
#include "libgimpbase/gimpbase.h"

typedef struct _GimpUnitParserResult GimpUnitParserResult;

struct _GimpUnitParserResult
{
  gdouble   value;
  gdouble   resolution;
  GimpUnit  unit;
  gboolean  unit_found;
};

gboolean gimp_unit_parser_parse (const char *str, GimpUnitParserResult *result);

#endif /*__GIMP_UNIT_PARSER_H__*/
