/*
 *  Semi-Flatten plug-in v1.0 by Adam D. Moss, adam@foxbox.org.  1998/01/27
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
 * TODO:
 *     Use tile iteration instead of dumb row-walking
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


/* Declare local functions.
 */
static void      query  (void);
static void      run    (gchar   *name,
			 gint     nparams,
			 GimpParam  *param,
			 gint    *nreturn_vals,
			 GimpParam **return_vals);

static void      semiflatten            (GimpDrawable    *drawable);
static void      semiflatten_render_row (const guchar *src,
					 guchar       *dest,
					 gint          row_width,
					 gint          bytes);


static guchar bgred, bggreen, bgblue;


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" }
  };

  gimp_install_procedure ("plug_in_semiflatten",
			  "Flatten pixels in an RGBA image that aren't "
			  "completely transparent against the current GIMP "
			  "background color",
			  "This plugin flattens pixels in an RGBA image that "
			  "aren't completely transparent against the current "
			  "GIMP background color",
			  "Adam D. Moss (adam@foxbox.org)",
			  "Adam D. Moss (adam@foxbox.org)",
			  "27th January 1998",
			  N_("<Image>/Filters/Colors/Semi-Flatten"),
			  "RGBA",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);
}


static void
run (gchar   *name,
     gint     nparams,
     GimpParam  *param,
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[1];
  GimpDrawable *drawable;
  gint32 image_ID;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  INIT_I18N();

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  image_ID = param[1].data.d_image;

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is indexed or RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id))
	{
	  gimp_progress_init ( _("Semi-Flatten..."));
	  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width ()
				       + 1));
	  semiflatten (drawable);
	  gimp_displays_flush ();
	}
      else
	{
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
semiflatten_render_row (const guchar *src,
			guchar       *dest,
			gint          row_width,
			gint          bytes)
{
  gint col;

  for (col = 0; col < row_width ; col++)
    {
      dest[0] = (src[0] * src[3]) / 255 + (bgred * (255 - src[3])) / 255;
      dest[1] = (src[1] * src[3]) / 255 + (bggreen * (255 - src[3])) / 255;
      dest[2] = (src[2] * src[3]) / 255 + (bgblue * (255 - src[3])) / 255;
      dest[3] = (src[3] == 0) ? 0 : 255;

      src += bytes;
      dest += bytes;
    }
}

static void
semiflatten (GimpDrawable *drawable)
{
  GimpPixelRgn srcPR, destPR;
  GimpRGB      background;
  gint         width, height;
  gint         bytes;
  guchar      *src_row;
  guchar      *dest_row;
  gint         row;
  gint         x1, y1, x2, y2;

  /* Fetch the GIMP current background colour, to semi-flatten against */
  gimp_palette_get_background (&background);
  gimp_rgb_get_uchar (&background, &bgred, &bggreen, &bgblue);

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  /*  allocate row buffers  */
  src_row  = g_new (guchar, (x2 - x1) * bytes);
  dest_row = g_new (guchar, (x2 - x1) * bytes);

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  for (row = y1; row < y2; row++)
    {
      gimp_pixel_rgn_get_row (&srcPR, src_row, x1, row, x2 - x1);

      semiflatten_render_row (src_row,
			      dest_row,
			      (x2 - x1),
			      bytes);

      gimp_pixel_rgn_set_row (&destPR, dest_row, x1, row, x2 - x1);

      if ((row % 10) == 0)
	gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  /*  update the processed region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));

  g_free (src_row);
  g_free (dest_row);
}
