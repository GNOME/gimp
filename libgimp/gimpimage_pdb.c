/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpimage_pdb.c
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

#include <string.h>

#include "gimp.h"


gint32
gimp_image_new (guint      width,
		guint      height,
		GImageType type)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 image_ID;

  return_vals = gimp_run_procedure ("gimp_image_new",
				    &nreturn_vals,
				    PARAM_INT32, width,
				    PARAM_INT32, height,
				    PARAM_INT32, type,
				    PARAM_END);

  image_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    image_ID = return_vals[1].data.d_image;

  gimp_destroy_params (return_vals, nreturn_vals);

  return image_ID;
}

gint32
gimp_image_duplicate (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 new_image_ID;

  return_vals = gimp_run_procedure ("gimp_channel_ops_duplicate",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  new_image_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    new_image_ID = return_vals[1].data.d_image;

  gimp_destroy_params (return_vals, nreturn_vals);

  return (new_image_ID);
}     

void
gimp_image_delete (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_delete",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

guint
gimp_image_width (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint result;

  return_vals = gimp_run_procedure ("gimp_image_width",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  result = 0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

guint
gimp_image_height (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint result;

  return_vals = gimp_run_procedure ("gimp_image_height",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  result = 0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

GImageType
gimp_image_base_type (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint result;

  return_vals = gimp_run_procedure ("gimp_image_base_type",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  result = 0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint32
gimp_image_floating_selection (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 layer_ID;

  return_vals = gimp_run_procedure ("gimp_image_floating_selection",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  layer_ID = 0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    layer_ID = return_vals[1].data.d_layer;

  gimp_destroy_params (return_vals, nreturn_vals);

  return layer_ID;
}

void
gimp_image_add_channel (gint32 image_ID,
			gint32 channel_ID,
			int    position)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_add_channel",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_INT32, position,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_add_layer (gint32 image_ID,
		      gint32 layer_ID,
		      gint   position)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_add_layer",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, position,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_add_layer_mask (gint32 image_ID,
			   gint32 layer_ID,
			   gint32 mask_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_add_layer_mask",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_LAYER, layer_ID,
				    PARAM_CHANNEL, mask_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_undo_disable (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_undo_disable",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_undo_enable (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_undo_enable",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_undo_freeze (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_undo_freeze",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_undo_thaw (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_undo_thaw",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_undo_push_group_start (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_undo_push_group_start",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_undo_push_group_end (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_undo_push_group_end",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_clean_all (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_clean_all",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint32
gimp_image_flatten (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 layer_ID;

  return_vals = gimp_run_procedure ("gimp_image_flatten",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  layer_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    layer_ID = return_vals[1].data.d_layer;

  gimp_destroy_params (return_vals, nreturn_vals);

  return layer_ID;
}

void
gimp_image_lower_channel (gint32 image_ID,
			  gint32 channel_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_lower_channel",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_lower_layer (gint32 image_ID,
			gint32 layer_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_lower_layer",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint32
gimp_image_merge_visible_layers (gint32        image_ID,
				 GimpMergeType merge_type)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 layer_ID;

  return_vals = gimp_run_procedure ("gimp_image_merge_visible_layers",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, merge_type,
				    PARAM_END);

  layer_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    layer_ID = return_vals[1].data.d_layer;

  gimp_destroy_params (return_vals, nreturn_vals);

  return layer_ID;
}

gint32
gimp_image_pick_correlate_layer (gint32 image_ID,
				 gint   x,
				 gint   y)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 layer_ID;

  return_vals = gimp_run_procedure ("gimp_image_pick_correlate_layer",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, x,
				    PARAM_INT32, y,
				    PARAM_END);

  layer_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    layer_ID = return_vals[1].data.d_layer;

  gimp_destroy_params (return_vals, nreturn_vals);

  return layer_ID;
}

void
gimp_image_raise_channel (gint32 image_ID,
			  gint32 channel_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_raise_channel",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_raise_layer (gint32 image_ID,
			gint32 layer_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_raise_layer",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_remove_channel (gint32 image_ID,
			   gint32 channel_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_remove_channel",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_CHANNEL, channel_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_remove_layer (gint32 image_ID,
			 gint32 layer_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_remove_layer",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_remove_layer_mask (gint32 image_ID,
			      gint32 layer_ID,
			      gint   mode)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_remove_layer_mask",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_LAYER, layer_ID,
				    PARAM_INT32, mode,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_resize (gint32 image_ID,
		   guint  new_width,
		   guint  new_height,
		   gint   offset_x,
		   gint   offset_y)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_resize",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, new_width,
				    PARAM_INT32, new_height,
				    PARAM_INT32, offset_x,
				    PARAM_INT32, offset_y,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint32
gimp_image_get_active_channel (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 channel_ID;

  return_vals = gimp_run_procedure ("gimp_image_get_active_channel",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  channel_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    channel_ID = return_vals[1].data.d_channel;

  gimp_destroy_params (return_vals, nreturn_vals);

  return channel_ID;
}

gint32
gimp_image_get_active_layer (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 layer_ID;

  return_vals = gimp_run_procedure ("gimp_image_get_active_layer",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  layer_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    layer_ID = return_vals[1].data.d_layer;

  gimp_destroy_params (return_vals, nreturn_vals);

  return layer_ID;
}

gint32*
gimp_image_get_channels (gint32  image_ID,
			 gint   *nchannels)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 *channels;

  return_vals = gimp_run_procedure ("gimp_image_get_channels",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  channels = NULL;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *nchannels = return_vals[1].data.d_int32;
      channels = g_new (gint32, *nchannels);
      memcpy (channels, return_vals[2].data.d_int32array, *nchannels * 4);
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return channels;
}

guchar*
gimp_image_get_cmap (gint32  image_ID,
		     gint   *ncolors)
{
  GParam *return_vals;
  gint nreturn_vals;
  guchar *cmap;

  return_vals = gimp_run_procedure ("gimp_image_get_cmap",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  cmap = NULL;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *ncolors = return_vals[1].data.d_int32 / 3;
      cmap = g_new (guchar, *ncolors * 3);
      memcpy (cmap, return_vals[2].data.d_int8array, *ncolors * 3);
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return cmap;
}

gboolean
gimp_image_get_component_active (gint32 image_ID,
				 gint   component)
{
  GParam *return_vals;
  gint nreturn_vals;
  gboolean result;

  return_vals = gimp_run_procedure ("gimp_image_get_component_active",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_image_get_component_visible (gint32 image_ID,
				  gint   component)
{
  GParam *return_vals;
  gint nreturn_vals;
  gboolean result;

  return_vals = gimp_run_procedure ("gimp_image_get_component_visible",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gchar*
gimp_image_get_filename (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  char *filename;

  return_vals = gimp_run_procedure ("gimp_image_get_filename",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  filename = NULL;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    filename = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return filename;
}

gint32*
gimp_image_get_layers (gint32  image_ID,
		       gint   *nlayers)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 *layers;

  return_vals = gimp_run_procedure ("gimp_image_get_layers",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  layers = NULL;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *nlayers = return_vals[1].data.d_int32;
      layers = g_new (gint32, *nlayers);
      memcpy (layers, return_vals[2].data.d_int32array, *nlayers * 4);
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return layers;
}

gint32
gimp_image_get_selection (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 selection_ID;

  return_vals = gimp_run_procedure ("gimp_image_get_selection",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  selection_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    selection_ID = return_vals[1].data.d_selection;

  gimp_destroy_params (return_vals, nreturn_vals);

  return selection_ID;
}

void
gimp_image_set_active_channel (gint32 image_ID,
			       gint32 channel_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  if (channel_ID == -1)
    {
      return_vals = gimp_run_procedure ("gimp_image_unset_active_channel",
					&nreturn_vals,
					PARAM_IMAGE, image_ID,
					PARAM_END);
    }
  else
    {
      return_vals = gimp_run_procedure ("gimp_image_set_active_channel",
					&nreturn_vals,
					PARAM_IMAGE, image_ID,
					PARAM_CHANNEL, channel_ID,
					PARAM_END);
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_set_active_layer (gint32 image_ID,
			     gint32 layer_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_set_active_layer",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_LAYER, layer_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_set_cmap (gint32  image_ID,
		     guchar *cmap,
		     gint    ncolors)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_set_cmap",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, ncolors * 3,
				    PARAM_INT8ARRAY, cmap,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_set_component_active (gint32   image_ID,
				 gint     component,
				 gboolean active)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_set_component_active",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, component,
				    PARAM_INT32, active,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_set_component_visible (gint32   image_ID,
				  gint     component,
				  gboolean visible)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_set_component_visible",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, component,
				    PARAM_INT32, visible,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_set_filename (gint32  image_ID,
			 gchar  *name)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_set_filename",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_STRING, name,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_image_get_resolution (gint32  image_ID,
			   double *xresolution,
			   double *yresolution)
{
  GParam *return_vals;
  gint nreturn_vals;
  gdouble xres;
  gdouble yres;

  g_return_if_fail (xresolution && yresolution);

  return_vals = gimp_run_procedure ("gimp_image_get_resolution",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  /* error return values */
  xres = 0.0;
  yres = 0.0;

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
  {
    xres = return_vals[1].data.d_float;
    yres = return_vals[2].data.d_float;
  }

  gimp_destroy_params (return_vals, nreturn_vals);

  *xresolution = xres;
  *yresolution = yres;
}

void
gimp_image_set_resolution (gint32 image_ID,
			   double xresolution,
			   double yresolution)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_set_resolution",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_FLOAT, xresolution,
				    PARAM_FLOAT, yresolution,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

GimpUnit
gimp_image_get_unit (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  GimpUnit unit;

  return_vals = gimp_run_procedure ("gimp_image_get_unit",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  /* error return value */
  unit = GIMP_UNIT_INCH;

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    unit = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return unit;
}

void
gimp_image_set_unit (gint32   image_ID,
		     GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_set_unit",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, unit,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint32
gimp_image_get_layer_by_tattoo (gint32 image_ID, 
				gint32 tattoo)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 layer = 0;

  return_vals = gimp_run_procedure ("gimp_image_get_layer_by_tattoo",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, tattoo,
				    PARAM_END);

  layer = 0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    layer = return_vals[1].data.d_layer;

  gimp_destroy_params (return_vals, nreturn_vals);

  return layer;
}

gint32
gimp_image_get_channel_by_tattoo (gint32 image_ID, 
				  gint32 tattoo)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 channel = 0;

  return_vals = gimp_run_procedure ("gimp_image_get_channel_by_tattoo",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, tattoo,
				    PARAM_END);

  channel = 0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    channel = return_vals[1].data.d_channel;

  gimp_destroy_params (return_vals, nreturn_vals);

  return channel;
}

guchar *
gimp_image_get_thumbnail_data (gint32  image_ID,
			       gint   *width,
			       gint   *height,
			       gint   *bytes)
{
  GParam *return_vals;
  gint    nreturn_vals;
  guchar *image_data = NULL;

  return_vals = gimp_run_procedure ("gimp_image_thumbnail",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, *width,
				    PARAM_INT32, *height,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *width = return_vals[1].data.d_int32;
      *height = return_vals[2].data.d_int32;
      *bytes = return_vals[3].data.d_int32;
      image_data = g_new (guchar, return_vals[4].data.d_int32);
      g_memmove (image_data, return_vals[5].data.d_int32array, return_vals[4].data.d_int32);
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return image_data;
}

gint32 *
gimp_image_list (gint *nimages)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gint32 *images;

  return_vals = gimp_run_procedure ("gimp_image_list",
				    &nreturn_vals,
				    PARAM_END);

  images = NULL;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *nimages = return_vals[1].data.d_int32;
      images = g_new (gint32, *nimages);
      memcpy (images, return_vals[2].data.d_int32array, *nimages * 4);
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return images;
}

