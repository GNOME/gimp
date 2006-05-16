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

#include "config.h"

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC "plug-in-vinvert"


/* Declare local functions.
 */
static void   query              (void);
static void   run                (const gchar      *name,
                                  gint              nparams,
                                  const GimpParam  *param,
                                  gint             *nreturn_vals,
                                  GimpParam       **return_vals);

static void   vinvert            (GimpDrawable     *drawable);
static void   indexed_vinvert    (gint32            image_ID);
static void   vinvert_render_row (const guchar     *src,
                                  guchar           *dest,
                                  gint              row_width,
                                  gint              bpp);


const GimpPlugInInfo PLUG_IN_INFO =
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
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive"          },
    { GIMP_PDB_IMAGE,    "image",    "Input image (used for indexed images)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"                        }
  };

  gimp_install_procedure (PLUG_IN_PROC,
			  N_("Invert the brightness of each pixel"),
			  "This function takes an indexed/RGB image and "
			  "inverts its 'value' in HSV space.  The upshot of "
			  "this is that the color and saturation at any given "
			  "point remains the same, but its brightness is "
			  "effectively inverted.  Quite strange.  Sometimes "
			  "produces unpleasant color artifacts on images from "
			  "lossy sources (ie. JPEG).",
			  "Adam D. Moss (adam@foxbox.org)",
			  "Adam D. Moss (adam@foxbox.org)",
			  "27th March 1997",
			  N_("_Value Invert"),
			  "RGB*, INDEXED*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Colors/Invert");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  gint32             image_ID;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  image_ID = param[1].data.d_image;

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is indexed or RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id))
	{
	  if (run_mode != GIMP_RUN_NONINTERACTIVE)
	    {
	      INIT_I18N();
	      gimp_progress_init (_("Value Invert"));
	    }

	  vinvert (drawable);
          if (run_mode != GIMP_RUN_NONINTERACTIVE)
	    gimp_displays_flush ();
	}
      else
	if (gimp_drawable_is_indexed (drawable->drawable_id))
	  {
	    indexed_vinvert (image_ID);
            if (run_mode != GIMP_RUN_NONINTERACTIVE)
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
indexed_vinvert (gint32 image_ID)
{
  guchar *cmap;
  gint    ncols;

  cmap = gimp_image_get_colormap (image_ID, &ncols);

  g_return_if_fail (cmap != NULL);

  vinvert_render_row (cmap, cmap, ncols, 3);

  gimp_image_set_colormap (image_ID, cmap, ncols);

  g_free (cmap);
}

static void
vinvert_func (const guchar *src,
	      guchar       *dest,
	      gint          bpp,
	      gpointer      data)
{
  gint v1, v2, v3;

  v1 = src[0];
  v2 = src[1];
  v3 = src[2];

  gimp_rgb_to_hsv_int (&v1, &v2, &v3);
  v3 = 255 - v3;
  gimp_hsv_to_rgb_int (&v1, &v2, &v3);

  dest[0] = v1;
  dest[1] = v2;
  dest[2] = v3;

  if (bpp == 4)
    dest[3] = src[3];
}

static void
vinvert_render_row (const guchar *src,
		    guchar       *dest,
		    gint          row_width, /* in pixels */
		    gint          bpp)
{
  while (row_width--)
    {
      vinvert_func (src, dest, bpp, NULL);
      src += bpp;
      dest += bpp;
    }
}

static void
vinvert (GimpDrawable *drawable)
{
  gimp_rgn_iterate2 (drawable, 0 /* unused */, vinvert_func, NULL);
}
