/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpdrawable_pdb.c
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


void
gimp_drawable_update (gint32 drawable_ID,
		      gint   x,
		      gint   y,
		      guint  width,
		      guint  height)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_drawable_update",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_INT32, x,
				    PARAM_INT32, y,
				    PARAM_INT32, width,
				    PARAM_INT32, height,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_drawable_merge_shadow (gint32 drawable_ID,
			    gint   undoable)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_drawable_merge_shadow",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_INT32, undoable,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint32
gimp_drawable_image_id (gint32 drawable_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 image_ID;

  return_vals = gimp_run_procedure ("gimp_drawable_image",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  image_ID = -1;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    image_ID = return_vals[1].data.d_image;

  gimp_destroy_params (return_vals, nreturn_vals);

  return image_ID;
}

guint
gimp_drawable_width (gint32 drawable_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  guint width;

  return_vals = gimp_run_procedure ("gimp_drawable_width",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  width = 0;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    width = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return width;
}

guint
gimp_drawable_height (gint32 drawable_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  guint height;

  return_vals = gimp_run_procedure ("gimp_drawable_height",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  height = 0;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    height = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return height;
}

guint
gimp_drawable_bpp (gint32 drawable_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  guint bpp;

  return_vals = gimp_run_procedure ("gimp_drawable_bytes",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  bpp = 0;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    bpp = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return bpp;
}

GDrawableType
gimp_drawable_type (gint32 drawable_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint result;

  return_vals = gimp_run_procedure ("gimp_drawable_type",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = -1;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_is_channel (gint32 drawable_ID)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_is_channel",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_is_rgb (gint32 drawable_ID)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_is_rgb",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_is_gray (gint32 drawable_ID)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_is_gray",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_has_alpha (gint32 drawable_ID)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_has_alpha",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_is_indexed (gint32 drawable_ID)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_is_indexed",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_is_layer (gint32 drawable_ID)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_is_layer",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_is_layer_mask (gint32 drawable_ID)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_is_layer_mask",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_mask_bounds (gint32  drawable_ID,
			   gint   *x1,
			   gint   *y1,
			   gint   *x2,
			   gint   *y2)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_mask_bounds",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      result = return_vals[1].data.d_int32;
      *x1 = return_vals[2].data.d_int32;
      *y1 = return_vals[3].data.d_int32;
      *x2 = return_vals[4].data.d_int32;
      *y2 = return_vals[5].data.d_int32;
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

void
gimp_drawable_offsets (gint32  drawable_ID,
		       gint   *offset_x,
		       gint   *offset_y)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_drawable_offsets",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *offset_x = return_vals[1].data.d_int32;
      *offset_y = return_vals[2].data.d_int32;
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_drawable_fill (gint32       drawable_ID,
		    GimpFillType fill_type)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_drawable_fill",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_INT32, fill_type,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

guchar *
gimp_drawable_get_thumbnail_data (gint32 drawable_ID,
				  gint  *width,
				  gint  *height,
				  gint  *bytes)
{
  GParam  *return_vals;
  gint     nreturn_vals;
  guchar  *drawable_data = NULL;

  return_vals = gimp_run_procedure ("gimp_drawable_thumbnail",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_INT32, *width,
				    PARAM_INT32, *height,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *width = return_vals[1].data.d_int32;
      *height = return_vals[2].data.d_int32;
      *bytes = return_vals[3].data.d_int32;
      drawable_data = g_new (guchar, return_vals[4].data.d_int32);
      g_memmove (drawable_data, return_vals[5].data.d_int32array, return_vals[4].data.d_int32);
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return drawable_data;
}
