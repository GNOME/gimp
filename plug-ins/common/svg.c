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

/* SVG loading and saving file filter for the GIMP
 * -Dom Lachowicz <cinamod@hotmail.com>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <rsvg.h>

/* TODO: remove me, initialize gimp i18n services */
#define _(String) (String)

static void     query                     (void);
static void     run                       (gchar            *name,
					   gint              nparams,
					   GimpParam        *param,
					   gint             *nreturn_vals,
					   GimpParam       **return_vals);
static gint32   load_image                (gchar            *filename, 
					   GimpRunMode       runmode, 
					   gboolean          preview);

GimpPlugInInfo PLUG_IN_INFO = {
  NULL,                         /* init_proc  */
  NULL,                         /* quit_proc  */
  query,                        /* query_proc */
  run,                          /* run_proc   */
};

MAIN ()

/*
 * 'query()' - Respond to a plug-in query...
 */
static void
query (void)
{
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING,   "raw_filename", "The name of the file to load" }
  };
  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,   "image",         "Output image" }
  };

  gimp_install_procedure ("file_svg_load",
                          "Loads files in the SVG file format",
                          "Loads files in the SVG file format",
                          "Dom Lachowicz <cinamod@hotmail.com>",
                          "Dom Lachowicz <cinamod@hotmail.com>",
                          "(c) 2003 - " VERSION,
			  "<Load>/SVG",
			  NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_magic_load_handler ("file_svg_load",
				    "svg", "",
				    "0,string,<?xml,0,string,<svg");
}

/*
 * 'run()' - Run the plug-in...
 */
static void
run (gchar      *name,
     gint        nparams,
     GimpParam  *param,
     gint       *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam     values[2];
  GimpRunMode          run_mode;
  GimpPDBStatusType    status = GIMP_PDB_SUCCESS;
  gint32               image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  /* MUST call this before any RSVG funcs */
  g_type_init ();

  if (strcmp (name, "file_svg_load") == 0)
    {
      /* INIT_I18N_UI (); */
      image_ID = load_image (param[1].data.d_string, run_mode, FALSE);

      if (image_ID != -1)
	{
	  *nreturn_vals = 2;
	  values[1].type         = GIMP_PDB_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
    }

  values[0].data.d_status = status;
}

/*
 * 'load_image()' - Load a SVG image into a new image window.
 */
static gint32
load_image (gchar       *filename, 	/* I - File to load */ 
	    GimpRunMode  runmode, 
	    gboolean     preview)
{
  gint32        image;	        /* Image */
  gint32	layer;		/* Layer */
  GimpDrawable *drawable;	/* Drawable for layer */
  GimpPixelRgn	pixel_rgn;	/* Pixel region for layer */
  GdkPixbuf    *pixbuf;        /* SVG, rasterized */
  gchar        *status;

  /* TODO: could possibly use preview abilities */

  pixbuf = rsvg_pixbuf_from_file (filename, NULL);

  if (!pixbuf)
    {
      g_message (_("can't open \"%s\"\n"), filename);
      gimp_quit ();
    }

  if (runmode != GIMP_RUN_NONINTERACTIVE)
    {
      status = g_strdup_printf (_("Loading %s:"), filename);
      gimp_progress_init (status);
      g_free (status);
    }

  image = gimp_image_new (gdk_pixbuf_get_width (pixbuf), 
			  gdk_pixbuf_get_height (pixbuf),
			  GIMP_RGB);
  if (image == -1)
    {
      g_message ("Can't allocate new image\n%s", filename);
      gimp_quit ();
    }

  gimp_image_set_filename (image, filename);

  layer = gimp_layer_new (image, _("Background"),
			  gdk_pixbuf_get_width (pixbuf), 
			  gdk_pixbuf_get_height (pixbuf),
			  GIMP_RGBA_IMAGE, 100, GIMP_NORMAL_MODE);
  
  drawable = gimp_drawable_get (layer);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
		       drawable->width, drawable->height, TRUE, FALSE);

  gimp_pixel_rgn_set_rect (&pixel_rgn, gdk_pixbuf_get_pixels (pixbuf),
			   0, 0, 
			   gdk_pixbuf_get_width (pixbuf),
			   gdk_pixbuf_get_height (pixbuf));

  g_object_unref (G_OBJECT(pixbuf));

  /* Tell the GIMP to display the image.
   */
  gimp_image_add_layer (image, layer, 0);
  gimp_drawable_flush (drawable);

  return image;
}
