/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */                                                                             
#include "gimp.h"


gint32
gimp_display_new (gint32 image_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  gint32 display_ID;

  return_vals = gimp_run_procedure ("gimp_display_new",
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_ID,
                                    PARAM_END);

  display_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    display_ID = return_vals[1].data.d_display;

  gimp_destroy_params (return_vals, nreturn_vals);

  return display_ID;
}

void
gimp_display_delete (gint32 display_ID)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_display_delete",
                                    &nreturn_vals,
                                    PARAM_DISPLAY, display_ID,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_displays_flush ()
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_displays_flush",
                                    &nreturn_vals,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}
