/*
 * Value-Invert plug-in v1.1 by Adam D. Moss, adam@foxbox.org.  1999/02/27
 */

/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * BUGS:
 *     Is not undoable when operating on indexed images - GIMP's fault.
 */



#include <stdio.h>
#include <stdlib.h>
#include "libgimp/gimp.h"
#include "libgimp/gimpcolorspace.h"


/* Declare local functions.
 */
static void      query  (void);
static void      indexed_vinvert (gint32 image_ID);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

static void      vinvert            (GDrawable  *drawable);
static void      vinvert_render_row (const guchar *src_row,
				     guchar *dest_row,
				     gint row_width,
				     const gint bytes);


static GRunModeType run_mode;

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (used for indexed images)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_vinvert",
			  "Invert the 'value' componant of an indexed/RGB image in HSV colourspace",
			  "This function takes an indexed/RGB image and inverts its 'value' in HSV space.  The upshot of this is that the colour and saturation at any given point remains the same, but its brightness is effectively inverted.  Quite strange.  Sometimes produces unpleasant colour artifacts on images from lossy sources (ie. JPEG).",
			  "Adam D. Moss (adam@foxbox.org)",
			  "Adam D. Moss (adam@foxbox.org)",
			  "27th March 1997",
			  "<Image>/Filters/Colors/Value Invert",
			  "RGB*, INDEXED*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  gint32 image_ID;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;


  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  image_ID = param[1].data.d_image;

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is indexed or RGB color  */
      if (gimp_drawable_is_rgb (drawable->id))
	{
          if (run_mode != RUN_NONINTERACTIVE)
	    gimp_progress_init ("Value Invert...");

	  vinvert (drawable);
          if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();
	}
      else
	if (gimp_drawable_is_indexed (drawable->id))
	  {
	    indexed_vinvert (image_ID);
            if (run_mode != RUN_NONINTERACTIVE)
	      gimp_displays_flush ();
	  }
	else
	  {
	    status = STATUS_EXECUTION_ERROR;
	  }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


static void
indexed_vinvert(gint32 image_ID)
{
  guchar *cmap;
  gint ncols;

  cmap = gimp_image_get_cmap (image_ID, &ncols);

  if (cmap==NULL)
    {
      printf("vinvert: cmap was NULL!  Quitting...\n");
      gimp_quit();
    }

  vinvert_render_row(
		     cmap,
		     cmap,
		     ncols,
		     3
		     );

  gimp_image_set_cmap (image_ID, cmap, ncols);
}



static void
vinvert_render_row (const guchar *src_data,
		    guchar *dest_data,
		    gint col,               /* row width in pixels */
		    const gint bytes)
{
  while (col--)
    {
      int v1, v2, v3;

      v1 = src_data[col*bytes   ];
      v2 = src_data[col*bytes +1];
      v3 = src_data[col*bytes +2];

      rgb_to_hsv(&v1, &v2, &v3);
      v3 = 255-v3;
      hsv_to_rgb(&v1, &v2, &v3);

      dest_data[col*bytes   ] = v1;
      dest_data[col*bytes +1] = v2;
      dest_data[col*bytes +2] = v3;

      if (bytes>3)
	{
	  gint bytenum;

	  for (bytenum = 3; bytenum<bytes; bytenum++)
	    {
	      dest_data[col*bytes+bytenum] =
		src_data[col*bytes+bytenum];
	    }
	}
    }
}



static void
vinvert_render_region (const GPixelRgn srcPR,
		       const GPixelRgn destPR)
{
  gint row;
  guchar* src_ptr  = srcPR.data;
  guchar* dest_ptr = destPR.data;
  
  for (row = 0; row < srcPR.h ; row++)
    {
      vinvert_render_row (src_ptr, dest_ptr,
			  srcPR.w,
			  srcPR.bpp);

      src_ptr  += srcPR.rowstride;
      dest_ptr += destPR.rowstride;
    }
}





static void
vinvert (GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint x1, y1, x2, y2;
  gpointer pr;
  gint total_area, area_so_far;
  gint progress_skip;


  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  total_area = (x2 - x1) * (y2 - y1);
  area_so_far = 0;
  progress_skip = 0;

  /* Initialize the pixel regions. */
  gimp_pixel_rgn_init (&srcPR, drawable, x1, y1, (x2 - x1), (y2 - y1),
		       FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, x1, y1, (x2 - x1), (y2 - y1),
		       TRUE, TRUE);
  
  for (pr = gimp_pixel_rgns_register (2, &srcPR, &destPR);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      vinvert_render_region (srcPR, destPR);

      if ((run_mode != RUN_NONINTERACTIVE))
	{
	  area_so_far += srcPR.w * srcPR.h;
	  if (((progress_skip++)%10) == 0)
	    gimp_progress_update ((double) area_so_far / (double) total_area);
	}
    }


  /*  update the processed region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}





