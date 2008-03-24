/*
 *  Semi-Flatten plug-in v1.0 by Adam D. Moss, adam@foxbox.org.  1998/01/27
 */

/* GIMP - The GNU Image Manipulation Program
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


#define PLUG_IN_PROC "plug-in-semiflatten"


/* Declare local functions.
 */
static void   query       (void);
static void   run         (const gchar      *name,
			   gint              nparams,
			   const GimpParam  *param,
			   gint             *nreturn_vals,
			   GimpParam       **return_vals);

static void   semiflatten (GimpDrawable     *drawable);


static guchar bgred, bggreen, bgblue;

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
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)"         },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"               }
  };

  gimp_install_procedure (PLUG_IN_PROC,
			  N_("Replace partial transparency with the current background color"),
			  "This plugin flattens pixels in an RGBA image that "
			  "aren't completely transparent against the current "
			  "GIMP background color",
			  "Adam D. Moss (adam@foxbox.org)",
			  "Adam D. Moss (adam@foxbox.org)",
			  "27th January 1998",
			  N_("_Semi-Flatten"),
			  "RGBA",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Web");
  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Layer/Transparency/Modify");
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
  *return_vals = values;

  values[0].type          = GIMP_PDB_STATUS;
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
	  gimp_progress_init (_("Semi-Flattening"));
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
semiflatten_func (const guchar *src,
		  guchar       *dest,
		  gint          bpp,
		  gpointer      data)
{
  dest[0] = (src[0] * src[3]) / 255 + (bgred * (255 - src[3])) / 255;
  dest[1] = (src[1] * src[3]) / 255 + (bggreen * (255 - src[3])) / 255;
  dest[2] = (src[2] * src[3]) / 255 + (bgblue * (255 - src[3])) / 255;
  dest[3] = (src[3] == 0) ? 0 : 255;
}

static void
semiflatten (GimpDrawable *drawable)
{
  GimpRGB background;

  gimp_context_get_background (&background);
  gimp_rgb_get_uchar (&background, &bgred, &bggreen, &bgblue);

  gimp_rgn_iterate2 (drawable, 0 /* unused */, semiflatten_func, NULL);
}
