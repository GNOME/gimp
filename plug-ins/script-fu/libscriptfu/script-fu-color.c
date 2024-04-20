/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <libgimp/gimp.h>

#include "script-fu-color.h"
#include "script-fu-types.h"


/* This encapsulates the implementation of the SFColorType.
 * It knows how to manipulate a SFArg of that type.
 * SFArg is the original abstraction
 * that in other parts of Gimp is a property and its metadata GParamSpec.
 *
 * Separate because it is likely to change and is slightly complex.
 *
 * Likely to change:
 *  - recently changed to use GeglColor in the PDB and GUI
 *  - planned change is to eliminate the duplicate old-style interface.
 *  - New-style ScriptFu scripts may not need to keep actual values
 *    and default values, only the declarations of defaults etc.
 *    Since a GimpProcedureConfig carries the values
 *    and GParamSpec carries the defaults.
 *  - ScriptFu might not support RGB triplet repr
 *
 * Complex:
 * PDB and widgets traffic in GeglColor but SF dumbs it down to a Scheme list (r g b)
 *
 * More SF code deals with GeglColor:
 * see scheme_marshall.c we marshall from GeglColor to/from Scheme lists of numbers.
 */


/* Return the Scheme representation.
 * Caller owns returned string.
 */
gchar*
sf_color_get_repr (SFColorType arg_value)
{
  guchar rgb[3] = { 0 };

  if (arg_value)
    gegl_color_get_pixel (arg_value, babl_format ("R'G'B' u8"), rgb);

  return g_strdup_printf ("'(%d %d %d)", (gint) rgb[0], (gint) rgb[1], (gint) rgb[2]);
}

/* Returns GeglColor from SFColorType.
 *
 * Returned GeglColor is owned by caller.
 */
GeglColor *
sf_color_get_gegl_color (SFColorType arg_value)
{
  return arg_value ? gegl_color_duplicate (arg_value) : gegl_color_new ("transparent");
}

/* Set an SFArg of type SFColorType from a GeglColor.
 * color is owned by caller.
 */
void
sf_color_set_from_gegl_color (SFColorType *arg_value,
                              GeglColor   *color)
{
  const Babl *format = gegl_color_get_format (color);
  guint8      pixel[48];

  gegl_color_get_pixel (color, format, pixel);
  if (*arg_value)
    gegl_color_set_pixel (*arg_value, format, pixel);
  else
    *arg_value = gegl_color_duplicate (color);
}

/* Set the default for an arg of type SFColorType from a string name.
 *
 * Keep default value to put in default of param spec when creating procedure.
 *
 * Also, the old-style dialog resets from the kept default value.
 * Versus new-style dialog, using ProcedureConfig, which resets from a paramspec.
 *
 * Called at registration time.
 *
 * Returns FALSE when name_of_default is not the name of a color,
 * and then the default is not set properly.
 */
gboolean
sf_color_arg_set_default_by_name (SFArg *arg,
                                  gchar *name_of_default)
{
  gboolean   result = TRUE;
  GeglColor *color;

  /* Create a default value for the old-style interface.
   */
  if (! (color = gimp_color_parse_css (name_of_default)))
    {
      result = FALSE;
    }
  else
    {
      /* ScriptFu does not let an author specify RGBA, only RGB. */
      gimp_color_set_alpha (color, 1.0);

      /* Copying a struct that is not allocated, not setting a pointer. */
      g_clear_object (&arg->default_value.sfa_color);
      arg->default_value.sfa_color = color;
    }
  return result;
}

/* Set the default for an arg of type SFColorType from a GeglColor.
 *
 * This knows the proper field of the arg.
 */
void
sf_color_arg_set_default_by_color (SFArg     *arg,
                                   GeglColor *color)
{
  sf_color_set_from_gegl_color (&arg->default_value.sfa_color, color);
}

/* Return the color of the default.
 *
 * Agnostic whether the default was declared by name or color (list of numbers.)
 *
 * The returned color is owned by the caller and must be freed.
 */
GeglColor*
sf_color_arg_get_default_color (SFArg *arg)
{
  /* require the default was set earlier.
   * No easy way to assert it was set,
   */
  return sf_color_get_gegl_color (arg->default_value.sfa_color);
}


/* Methods for conversion GeglColor to/from Scheme representation. */

/* Caller owns returned string.*/
gchar*
sf_color_get_repr_from_gegl_color (GeglColor *color)
{
  guchar rgb[3] = { 0 };

  /* Warn when color has different count of components than
   * the 3 of the pixel we are converting to.
   */
  if (babl_format_get_n_components (gegl_color_get_format (color)) != 3)
    {
      g_warning ("%s converting to pixel with loss/gain of components", G_STRFUNC);
    }

  gegl_color_get_pixel (color, babl_format ("R'G'B' u8"), rgb);
  return g_strdup_printf ("'(%d %d %d)", (gint) rgb[0], (gint) rgb[1], (gint) rgb[2]);
}

/* Caller owns the returned GeglColor.
 * Returns NULL when name is not of a color.
 */
GeglColor *
sf_color_get_color_from_name (gchar *color_name)
{
  return gimp_color_parse_css (color_name);
}
