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
 * if used by gimp.exe itself)
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

#ifdef G_OS_WIN32 /* Bypass whole file otherwise */

#include "libgimpcolor/gimpcolortypes.h"
#include "libgimpbase/gimpbasetypes.h"
#include "libgimpbase/gimpunit.h"

#include "libgimp/gimppalette_pdb.h"

/*
 too much depencencies
#include "gimphelpui.h"
*/

#include <windows.h>

typedef int voidish;

/* function pointer prototypes */
typedef gboolean (* PFN_QueryColor) (GimpRGB *color);
typedef gchar* (* PFN_GetUnitStr) (GimpUnit);
typedef gdouble (* PFN_GetUnitInt) (GimpUnit);
typedef gdouble (* PFN_GetUnitDouble) (GimpUnit);
typedef gint (* PFN_GetNumber) (void);
typedef voidish (* PFN_Help) (const char*);

static FARPROC 
dynamic_resolve (const gchar* name, HMODULE* hMod)
{
  FARPROC fn = NULL;
  *hMod = NULL; /* if != NULL, call FreeLibrary */

  /* from application ? */
  fn = GetProcAddress(GetModuleHandle(NULL), name); 

  if (!fn)
    {
#if defined (LT_RELEASE) && defined (LT_CURRENT_MINUS_AGE)
      /* First try the libtool style name */
      *hMod = LoadLibrary ("libgimp-" LT_RELEASE "-" LT_CURRENT_MINUS_AGE ".dll");
#endif
      /* If that didn't work, try the name style used by Hans Breuer */
      if (!*hMod)
	*hMod = LoadLibrary ("gimp-1.3.dll");
      fn = GetProcAddress (*hMod, name); 
    }

  if (!fn)
    g_warning ("dynamic_resolve of %s failed!", name);

  return fn;
}

#define ENTRY(type, name, parlist, defaultval, fntype, arglist)	\
type 								\
name parlist							\
{								\
  HMODULE h = NULL;						\
  type ret = defaultval;					\
  fntype fn = (fntype) dynamic_resolve (#name, &h);		\
  if (fn)							\
    ret = fn arglist;						\
								\
  if (h)							\
    FreeLibrary (h);						\
  return ret;							\
}

ENTRY (gboolean, gimp_palette_get_foreground, (GimpRGB *color), FALSE,  PFN_QueryColor, (color));

ENTRY (gboolean, gimp_palette_get_background, (GimpRGB *color), FALSE, PFN_QueryColor, (color));

ENTRY (voidish, gimp_standard_help_func, (const gchar *help_data), 0, PFN_Help, (help_data));

ENTRY (const gchar *, gimp_unit_get_abbreviation, (GimpUnit unit), NULL, PFN_GetUnitStr, (unit));

ENTRY (const gchar *, gimp_unit_get_singular, (GimpUnit unit), NULL, PFN_GetUnitStr, (unit));

ENTRY (const gchar *, gimp_unit_get_plural, (GimpUnit unit), NULL, PFN_GetUnitStr, (unit));

ENTRY (gint, gimp_unit_get_digits, (GimpUnit unit), 0, PFN_GetUnitInt, (unit));

ENTRY (gdouble, gimp_unit_get_factor, (GimpUnit unit), 0., PFN_GetUnitDouble, (unit));

ENTRY (gint, gimp_unit_get_number_of_built_in_units, (void), 0, PFN_GetNumber, ());

ENTRY (gint, gimp_unit_get_number_of_units, (void), 0, PFN_GetNumber, ());

ENTRY (const gchar *, gimp_unit_get_symbol, (GimpUnit unit), NULL, PFN_GetUnitStr, (unit));

#endif /* G_OS_WIN32 */
