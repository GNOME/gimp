/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * libgimp-glue.c
 * Copyright (C) 2001 Hans Breuer <Hans@Breuer.org> 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Supports dynamic linking at run-time against
 * libgimp (if used by a plug-in, or gimp.exe
 * if used by itself)
 *

gimp_palette_get_background
gimp_palette_get_foreground
gimp_standard_help_func
gimp_unit_get_abbreviation
gimp_unit_get_digits
gimp_unit_get_factor
gimp_unit_get_number_of_built_in_units
gimp_unit_get_number_of_units
gimp_unit_get_plural
gimp_unit_get_singular
gimp_unit_get_symbol

 */

#include <glib.h>

#include "libgimpcolor/gimpcolortypes.h"
#include "libgimpbase/gimpunit.h"

#include "libgimp/gimppalette_pdb.h"

/*
 too much depencencies
#include "gimphelpui.h"
*/

#include <windows.h>

/* function pointer prototypes */
typedef gboolean (* PFN_QueryColor) (GimpRGB *color);
typedef gchar* (* PFN_GetUnitStr) (GimpUnit);
typedef gdouble (* PFN_GetUnitInt) (GimpUnit);
typedef gdouble (* PFN_GetUnitDouble) (GimpUnit);
typedef gint (* PFN_GetNumber) (void);
typedef void (* PFN_Help) (const char*);

static FARPROC 
dynamic_resolve (const gchar* name, HMODULE* hMod)
{
  FARPROC fn = NULL;
  *hMod = NULL; /* if != NULL, call FreeLibrary */

  /* from application ? */
  fn = GetProcAddress(GetModuleHandle(NULL), name); 

  if (!fn)
    {
      *hMod = LoadLibrary ("gimp-1.3.dll");
      fn = GetProcAddress (*hMod, name); 
    }

  if (!fn)
    g_warning ("dynamic_resolve of %s failed!", name);

  return fn;
}

gboolean
gimp_palette_get_foreground (GimpRGB *color)
{
  HMODULE h = NULL;
  gboolean ret = FALSE;
  PFN_QueryColor fn = (PFN_QueryColor) dynamic_resolve ("gimp_palette_get_foreground", &h);
  if (fn)
    ret = fn (color);

  if (h)
    FreeLibrary (h);
  return ret;
}

gboolean
gimp_palette_get_background (GimpRGB *color)
{
  HMODULE h = NULL;
  gboolean ret = FALSE;
  PFN_QueryColor fn = (PFN_QueryColor) dynamic_resolve ("gimp_palette_get_background", &h);
  if (fn)
    ret = fn (color);

  if (h)
    FreeLibrary (h);
  return ret;
}

void
gimp_standard_help_func (const gchar *help_data)
{
  HMODULE h = NULL;
  PFN_Help fn = (PFN_Help) dynamic_resolve ("gimp_standard_help_func", &h);
  if (fn)
    fn (help_data);

  if (h)
    FreeLibrary (h);
}

gchar *
gimp_unit_get_abbreviation (GimpUnit unit)
{
  HMODULE h = NULL;
  gchar* ret = NULL;
  PFN_GetUnitStr fn = (PFN_GetUnitStr) dynamic_resolve ("gimp_unit_get_abbreviation", &h);
  if (fn)
    ret = fn (unit);

  if (h)
    FreeLibrary (h);
  return ret;
}

gchar *
gimp_unit_get_singular (GimpUnit unit)
{
  HMODULE h = NULL;
  gchar* ret = NULL;
  PFN_GetUnitStr fn = (PFN_GetUnitStr) dynamic_resolve ("gimp_unit_get_singular", &h);
  if (fn)
    ret = fn (unit);

  if (h)
    FreeLibrary (h);
  return ret;
}

gchar *
gimp_unit_get_plural (GimpUnit unit)
{
  HMODULE h = NULL;
  gchar* ret = NULL;
  PFN_GetUnitStr fn = (PFN_GetUnitStr) dynamic_resolve ("gimp_unit_get_plural", &h);
  if (fn)
    ret = fn (unit);

  if (h)
    FreeLibrary (h);
  return ret;
}

gint
gimp_unit_get_digits (GimpUnit unit)
{
  HMODULE h = NULL;
  gint ret = 0;
  PFN_GetUnitInt fn = (PFN_GetUnitInt) dynamic_resolve ("gimp_unit_get_digits", &h);
  if (fn)
    ret = fn (unit);

  if (h)
    FreeLibrary (h);
  return ret;
}

gdouble
gimp_unit_get_factor (GimpUnit unit)
{
  HMODULE h = NULL;
  gdouble ret = 0.0;
  PFN_GetUnitDouble fn = (PFN_GetUnitDouble) dynamic_resolve ("gimp_unit_get_factor", &h);
  if (fn)
    ret = fn (unit);

  if (h)
    FreeLibrary (h);
  return ret;
}

gint
gimp_unit_get_number_of_built_in_units (void)
{
  HMODULE h = NULL;
  gint ret = 0;
  PFN_GetNumber fn = (PFN_GetNumber) dynamic_resolve ("gimp_unit_get_number_of_built_in_units", &h);
  if (fn)
    ret = fn ();

  if (h)
    FreeLibrary (h);
  return ret;
}

gint
gimp_unit_get_number_of_units (void)
{
  HMODULE h = NULL;
  gint ret = 0;
  PFN_GetNumber fn = (PFN_GetNumber) dynamic_resolve ("gimp_unit_get_number_of_units", &h);
  if (fn)
    ret = fn ();

  if (h)
    FreeLibrary (h);
  return ret;
}

gchar *
gimp_unit_get_symbol (GimpUnit unit)
{
  HMODULE h = NULL;
  gchar* ret = NULL;
  PFN_GetUnitStr fn = (PFN_GetUnitStr) dynamic_resolve ("gimp_unit_get_symbol", &h);
  if (fn)
    ret = fn (unit);

  if (h)
    FreeLibrary (h);
  return ret;
}
