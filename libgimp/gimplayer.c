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
gimp_layer_new (gint32        image_ID,
		char         *name,
		guint         width,
		guint         height,
		GDrawableType  type,
		gdouble       opacity,
		GLayerMode    mode)
{
  GParam *return_vals;
  int nreturn_vals;
  gint32 layer_ID;

  return_vals = gimp_run_procedure ("gimp_layer_new",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, width,
				    PARAM_INT32, height,
				    PARAM_INT32, type,
				    PARAM_STRING, name,
				    PARAM_FLOAT, opacity,
				    PARAM_INT32, mode,
				    PARAM_END);

  layer_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    layer_ID = return_vals[1].data.d_layer;

  gimp_destroy_params (return_vals, nreturn_vals);

  return layer_ID;
}

gint32
gimp_layer_copy (gint32 layer_ID)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_copy",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, 0,
				    PARAM_END);

  layer_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    layer_ID = return_vals[1].data.d_layer;

  gimp_destroy_params (return_vals, nreturn_vals);

  return layer_ID;
}

void
gimp_layer_delete (gint32 layer_ID)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_delete",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

guint
gimp_layer_width (gint32 layer_ID)
{
  return gimp_drawable_width (layer_ID);
}

guint
gimp_layer_height (gint32 layer_ID)
{
  return gimp_drawable_height (layer_ID);
}

guint
gimp_layer_bpp (gint32 layer_ID)
{
  return gimp_drawable_bpp (layer_ID);
}

GDrawableType
gimp_layer_type (gint32 layer_ID)
{
  return gimp_drawable_type (layer_ID);
}

void
gimp_layer_add_alpha (gint32 layer_ID)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_add_alpha",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint32
gimp_layer_create_mask (gint32 layer_ID,
			gint   mask_type)
{
  GParam *return_vals;
  int nreturn_vals;
  gint32 mask_ID;

  return_vals = gimp_run_procedure ("gimp_layer_create_mask",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, mask_type,
				    PARAM_END);

  mask_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    mask_ID = return_vals[1].data.d_channel;

  gimp_destroy_params (return_vals, nreturn_vals);

  return mask_ID;
}

void
gimp_layer_resize (gint32 layer_ID,
		   guint  new_width,
		   guint  new_height,
		   gint   offset_x,
		   gint   offset_y)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_resize",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, new_width,
				    PARAM_INT32, new_height,
				    PARAM_INT32, offset_x,
				    PARAM_INT32, offset_y,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_layer_scale (gint32 layer_ID,
		  guint  new_width,
		  guint  new_height,
		  gint   local_origin)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_scale",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, new_width,
				    PARAM_INT32, new_height,
				    PARAM_INT32, local_origin,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_layer_translate (gint32 layer_ID,
		      gint   offset_x,
		      gint   offset_y)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_translate",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, offset_x,
				    PARAM_INT32, offset_y,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint
gimp_layer_is_floating_selection (gint32 layer_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_layer_is_floating_sel",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint32
gimp_layer_get_image_id (gint32 layer_ID)
{
  return gimp_drawable_image_id (layer_ID);
}

gint32
gimp_layer_get_mask_id (gint32 layer_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  gint32 mask_ID;

  return_vals = gimp_run_procedure ("gimp_layer_mask",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  mask_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    mask_ID = return_vals[1].data.d_channel;

  gimp_destroy_params (return_vals, nreturn_vals);

  return mask_ID;
}

gint
gimp_layer_get_apply_mask (gint32 layer_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_layer_get_apply_mask",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  result = 0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint
gimp_layer_get_edit_mask (gint32 layer_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_layer_get_edit_mask",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  result = 0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

GLayerMode
gimp_layer_get_mode (gint32 layer_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_layer_get_mode",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  result = 0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

char*
gimp_layer_get_name (gint32 layer_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  char *result;

  return_vals = gimp_run_procedure ("gimp_layer_get_name",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  result = NULL;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gdouble
gimp_layer_get_opacity (gint32 layer_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  gdouble result;

  return_vals = gimp_run_procedure ("gimp_layer_get_opacity",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  result = 0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_float;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint
gimp_layer_get_preserve_transparency (gint32 layer_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_layer_get_preserve_trans",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  result = 0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint
gimp_layer_get_show_mask (gint32 layer_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_layer_get_show_mask",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  result = 0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint
gimp_layer_get_visible (gint32 layer_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_layer_get_visible",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  result = 0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

void
gimp_layer_set_apply_mask (gint32 layer_ID,
			   gint   apply_mask)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_set_apply_mask",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, apply_mask,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_layer_set_edit_mask (gint32 layer_ID,
			  gint   edit_mask)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_set_edit_mask",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, edit_mask,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_layer_set_mode (gint32     layer_ID,
		     GLayerMode mode)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_set_mode",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, mode,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_layer_set_name (gint32  layer_ID,
		     char   *name)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_set_name",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_STRING, name,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_layer_set_offsets (gint32 layer_ID,
			int    offset_x,
			int    offset_y)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_set_offsets",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, offset_x,
				    PARAM_INT32, offset_y,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_layer_set_opacity (gint32  layer_ID,
			gdouble opacity)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_set_opacity",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, opacity,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_layer_set_preserve_transparency (gint32 layer_ID,
				      int    preserve_transparency)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_set_preserve_trans",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, preserve_transparency,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_layer_set_show_mask (gint32 layer_ID,
			  gint   show_mask)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_set_show_mask",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, show_mask,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_layer_set_visible (gint32 layer_ID,
			gint   visible)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_layer_set_visible",
				    &nreturn_vals,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, visible,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}
