/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppalette_pdb.c
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

#include "gimp.h"


void
gimp_palette_get_background (guchar *red,
			     guchar *green,
			     guchar *blue)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_palette_get_background",
                                    &nreturn_vals,
                                    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *red = return_vals[1].data.d_color.red;
      *green = return_vals[1].data.d_color.green;
      *blue = return_vals[1].data.d_color.blue;
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_palette_get_foreground (guchar *red,
			     guchar *green,
			     guchar *blue)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_palette_get_foreground",
                                    &nreturn_vals,
                                    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *red = return_vals[1].data.d_color.red;
      *green = return_vals[1].data.d_color.green;
      *blue = return_vals[1].data.d_color.blue;
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_palette_set_background (guchar red,
			     guchar green,
			     guchar blue)
{
  GParam *return_vals;
  gint nreturn_vals;
  guchar color[3];

  color[0] = red;
  color[1] = green;
  color[2] = blue;

  return_vals = gimp_run_procedure ("gimp_palette_set_background",
                                    &nreturn_vals,
				    PARAM_COLOR, color,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_palette_set_foreground (guchar red,
			     guchar green,
			     guchar blue)
{
  GParam *return_vals;
  gint nreturn_vals;
  guchar color[3];

  color[0] = red;
  color[1] = green;
  color[2] = blue;

  return_vals = gimp_run_procedure ("gimp_palette_set_foreground",
                                    &nreturn_vals,
				    PARAM_COLOR, color,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_palette_set_default_colors (void)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_palette_set_default_colors",
                                    &nreturn_vals,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_palette_swap_colors (void)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_palette_swap_colors",
                                    &nreturn_vals,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}
