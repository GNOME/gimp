/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
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


gint32
gimp_channel_new (gint32   image_ID,
		  char    *name,
		  guint    width,
		  guint    height,
		  gdouble  opacity,
		  guchar  *color)
{
  GParam *return_vals;
  int nreturn_vals;
  gint32 channel_ID;

  return_vals = gimp_run_procedure ("gimp_channel_new",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, width,
				    PARAM_INT32, height,
				    PARAM_STRING, name,
				    PARAM_FLOAT, opacity,
				    PARAM_COLOR, color,
				    PARAM_END);

  channel_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    channel_ID = return_vals[1].data.d_channel;

  gimp_destroy_params (return_vals, nreturn_vals);

  return channel_ID;
}

gint32
gimp_channel_copy (gint32 channel_ID)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_channel_copy",
				    &nreturn_vals,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_END);

  channel_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    channel_ID = return_vals[1].data.d_channel;

  gimp_destroy_params (return_vals, nreturn_vals);

  return channel_ID;
}

void
gimp_channel_delete (gint32 channel_ID)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_channel_delete",
				    &nreturn_vals,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

guint
gimp_channel_width (gint32 channel_ID)
{
  return gimp_drawable_width (channel_ID);
}

guint
gimp_channel_height (gint32 channel_ID)
{
  return gimp_drawable_height (channel_ID);
}

gint32
gimp_channel_get_image_id (gint32 channel_ID)
{
  return gimp_drawable_image_id (channel_ID);
}

gint32
gimp_channel_get_layer_id (gint32 channel_ID)
{
  return -1;
}

void
gimp_channel_get_color (gint32  channel_ID,
			guchar *red,
			guchar *green,
			guchar *blue)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_channel_get_color",
				    &nreturn_vals,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *red = return_vals[1].data.d_color.red;
      *green = return_vals[1].data.d_color.green;
      *blue = return_vals[1].data.d_color.blue;
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

char*
gimp_channel_get_name (gint32 channel_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  char *name;

  return_vals = gimp_run_procedure ("gimp_channel_get_name",
				    &nreturn_vals,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_END);

  name = NULL;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    name = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return name;
}

gdouble
gimp_channel_get_opacity (gint32 channel_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  gdouble opacity;

  return_vals = gimp_run_procedure ("gimp_channel_get_opacity",
				    &nreturn_vals,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_END);

  opacity = 0.0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    opacity = return_vals[1].data.d_float;

  gimp_destroy_params (return_vals, nreturn_vals);

  return opacity;
}

gint
gimp_channel_get_visible (gint32 channel_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int visible;

  return_vals = gimp_run_procedure ("gimp_channel_get_visible",
				    &nreturn_vals,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_END);

  visible = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    visible = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return visible;
}

void
gimp_channel_set_color (gint32 channel_ID,
			guchar red,
			guchar green,
			guchar blue)
{
  GParam *return_vals;
  int nreturn_vals;
  guchar color[3];

  color[0] = red;
  color[1] = green;
  color[2] = blue;

  return_vals = gimp_run_procedure ("gimp_channel_set_color",
				    &nreturn_vals,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_COLOR, color,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_channel_set_name (gint32  channel_ID,
		       char   *name)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_channel_set_name",
				    &nreturn_vals,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_STRING, name,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_channel_set_opacity (gint32  channel_ID,
			  gdouble opacity)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_channel_set_opacity",
				    &nreturn_vals,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_FLOAT, opacity,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_channel_set_visible (gint32 channel_ID,
			  gint   visible)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_channel_set_visible",
				    &nreturn_vals,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_INT32, visible,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint
gimp_channel_get_show_masked (gint32 channel_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int show_masked;

  return_vals = gimp_run_procedure ("gimp_channel_get_show_masked",
				    &nreturn_vals,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_END);

  show_masked = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    show_masked = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return show_masked;
}

void
gimp_channel_set_show_masked (gint32 channel_ID,
			      gint   show_masked)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_channel_set_show_masked",
				    &nreturn_vals,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_INT32, show_masked,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint32
gimp_channel_get_tattoo (gint32 channel_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int tattoo;

  return_vals = gimp_run_procedure ("gimp_channel_get_tattoo",
				    &nreturn_vals,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_END);

  tattoo = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    tattoo = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return tattoo;
}

