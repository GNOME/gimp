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
 * This filter tiles an image to arbitrary width and height
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


typedef struct
{
  gint new_width;
  gint new_height;
  gint constrain;
  gint new_image;
} TileVals;

typedef struct
{
  GtkWidget *sizeentry;
  GtkWidget *chainbutton;
  gint       run;
} TileInterface;

/* Declare a local function.
 */
static void      query  (void);
static void      run    (gchar     *name,
			 gint       nparams,
			 GParam    *param,
			 gint      *nreturn_vals,
			 GParam   **return_vals);

static gint32    tile   (gint32     image_id,
			 gint32     drawable_id,
			 gint32    *layer_id);

static gint      tile_dialog              (gint32         image_ID,
					   gint32         drawable_ID);

static void      tile_ok_callback         (GtkWidget     *widget,
					   gpointer       data);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static TileVals tvals =
{
  1,     /* new_width  */
  1,     /* new_height */
  TRUE,  /* constrain  */
  TRUE   /* new_image  */
};

static TileInterface tint =
{
  NULL,  /* sizeentry */
  FALSE  /* run       */
};

MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT32, "new_width", "New (tiled) image width" },
    { PARAM_INT32, "new_height", "New (tiled) image height" },
    { PARAM_INT32, "new_image", "Create a new image?" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  static GParamDef return_vals[] =
  {
    { PARAM_IMAGE, "new_image", "Output image (N/A if new_image == FALSE)" },
    { PARAM_LAYER, "new_layer", "Output layer (N/A if new_image == FALSE)" }
  };
  static gint nreturn_vals = sizeof (return_vals) / sizeof (return_vals[0]);

  gimp_install_procedure ("plug_in_tile",
			  "Create a new image which is a tiled version of the input drawable",
			  "This function creates a new image with a single "
			  "layer sized to the specified 'new_width' and "
			  "'new_height' parameters.  The specified drawable "
			  "is tiled into this layer.  The new layer will have "
			  "the same type as the specified drawable and the new "
			  "image will have a corresponding base type",
			  "Spencer Kimball & Peter Mattis",
			  "Spencer Kimball & Peter Mattis",
			  "1996-1997",
			  N_("<Image>/Filters/Map/Tile..."),
			  "RGB*, GRAY*, INDEXED*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[3];
  GRunModeType  run_mode;
  GStatusType   status = STATUS_SUCCESS;
  gint32        new_layer;
  gint          width, height;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 3;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  values[1].type = PARAM_IMAGE;
  values[2].type = PARAM_LAYER;

  width  = gimp_drawable_width (param[2].data.d_drawable);
  height = gimp_drawable_height (param[2].data.d_drawable);

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_tile", &tvals);

      /*  First acquire information with a dialog  */
      if (! tile_dialog (param[1].data.d_image,
			 param[2].data.d_drawable))
	return;
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Make sure all the arguments are there!  */
      if (nparams != 6)
	{
	  status = STATUS_CALLING_ERROR;
	}
      else
	{
	  tvals.new_width  = param[3].data.d_int32;
	  tvals.new_height = param[4].data.d_int32;
	  tvals.new_image  = param[5].data.d_int32 ? TRUE : FALSE;

	  if (tvals.new_width < 0 || tvals.new_height < 0)
	    status = STATUS_CALLING_ERROR;
	}
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_tile", &tvals);
      break;

    default:
      break;
    }

  /*  Make sure that the drawable is gray or RGB color  */
  if (status == STATUS_SUCCESS)
    {
      gimp_progress_init (_("Tiling..."));
      gimp_tile_cache_ntiles (2 * (width + 1) / gimp_tile_width ());

      values[1].data.d_image = tile (param[1].data.d_image,
				     param[2].data.d_drawable,
				     &new_layer);
      values[2].data.d_layer = new_layer;

      /*  Store data  */
      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_tile", &tvals, sizeof (TileVals));

      if (run_mode != RUN_NONINTERACTIVE)
	{
	  if (tvals.new_image)
	    gimp_display_new (values[1].data.d_image);
	  else
	    gimp_displays_flush ();
	}
    }

  values[0].data.d_status = status;
}

static gint32
tile (gint32  image_id,
      gint32  drawable_id,
      gint32 *layer_id)
{
  GPixelRgn src_rgn, dest_rgn;
  GDrawable *drawable, *new_layer;
  GImageType image_type;
  gint32 new_image_id;
  gint old_width, old_height;
  gint width, height;
  gint i, j, k;
  gint progress, max_progress;
  gint nreturn_vals;
  gpointer pr;

  /* initialize */

  image_type = RGB;
  new_image_id = 0;

  old_width  = gimp_drawable_width (drawable_id);
  old_height = gimp_drawable_height (drawable_id);

  if (tvals.new_image)
    {
      /*  create  a new image  */
      switch (gimp_drawable_type (drawable_id))
	{
	case RGB_IMAGE : case RGBA_IMAGE:
	  image_type = RGB;
	  break;
	case GRAY_IMAGE : case GRAYA_IMAGE:
	  image_type = GRAY;
	  break;
	case INDEXED_IMAGE : case INDEXEDA_IMAGE:
	  image_type = INDEXED;
	  break;
	}

      new_image_id = gimp_image_new (tvals.new_width, tvals.new_height,
				     image_type);
      *layer_id = gimp_layer_new (new_image_id, _("Background"),
				  tvals.new_width, tvals.new_height,
				  gimp_drawable_type (drawable_id),
				  100, NORMAL_MODE);
      gimp_image_add_layer (new_image_id, *layer_id, 0);
      new_layer = gimp_drawable_get (*layer_id);

      /*  Get the specified drawable  */
      drawable = gimp_drawable_get (drawable_id);
    }
  else
    {
      gimp_undo_push_group_start (image_id);

      gimp_run_procedure ("gimp_image_resize", &nreturn_vals,
			  PARAM_IMAGE, image_id,
			  PARAM_INT32, tvals.new_width,
			  PARAM_INT32, tvals.new_height,
			  PARAM_INT32, 0,
			  PARAM_INT32, 0,
			  PARAM_END);

      if (gimp_drawable_is_layer (drawable_id))
	gimp_run_procedure ("gimp_layer_resize", &nreturn_vals,
			    PARAM_LAYER, drawable_id,
			    PARAM_INT32, tvals.new_width,
			    PARAM_INT32, tvals.new_height,
			    PARAM_INT32, 0,
			    PARAM_INT32, 0,
			    PARAM_END);

      /*  Get the specified drawable  */
      drawable = gimp_drawable_get (drawable_id);
      new_layer = drawable;
    }

  /*  progress  */
  progress = 0;
  max_progress = tvals.new_width * tvals.new_height;

  /*  tile...  */
  for (i = 0; i < tvals.new_height; i += old_height)
    {
      height = old_height;
      if (height + i > tvals.new_height)
	height = tvals.new_height - i;

      for (j = 0; j < tvals.new_width; j += old_width)
	{
	  width = old_width;
	  if (width + j > tvals.new_width)
	    width = tvals.new_width - j;

	  gimp_pixel_rgn_init (&src_rgn, drawable,
			       0, 0, width, height, FALSE, FALSE);
	  gimp_pixel_rgn_init (&dest_rgn, new_layer,
			       j, i, width, height, TRUE, FALSE);

	  for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
	       pr != NULL;
	       pr = gimp_pixel_rgns_process (pr))
	    {
	      for (k = 0; k < src_rgn.h; k++)
		memcpy (dest_rgn.data + k * dest_rgn.rowstride,
			src_rgn.data + k * src_rgn.rowstride,
			src_rgn.w * src_rgn.bpp);

	      progress += src_rgn.w * src_rgn.h;
	      gimp_progress_update ((double) progress / (double) max_progress);
	    }
	}
    }

  /*  copy the colormap, if necessary  */
  if (image_type == INDEXED && tvals.new_image)
    {
      gint    ncols;
      guchar *cmap;

      cmap = gimp_image_get_cmap (image_id, &ncols);
      gimp_image_set_cmap (new_image_id, cmap, ncols);
      g_free (cmap);
    }

  if (tvals.new_image)
    {
      gimp_drawable_flush (new_layer);
      gimp_drawable_detach (new_layer);
    }
  else
    gimp_run_procedure ("gimp_undo_push_group_end", &nreturn_vals,
			PARAM_IMAGE, image_id,
			PARAM_END);

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);

  return new_image_id;
}

static gint
tile_dialog (gint32 image_ID,
	     gint32 drawable_ID)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *sizeentry;
  GtkWidget *toggle;
  gint     width;
  gint     height;
  gdouble  xres;
  gdouble  yres;
  GimpUnit unit;

  gimp_ui_init ("tile", FALSE);

  width  = gimp_drawable_width (drawable_ID);
  height = gimp_drawable_height (drawable_ID);
  unit   = gimp_image_get_unit (image_ID);
  gimp_image_get_resolution (image_ID, &xres, &yres);

  tvals.new_width  = width;
  tvals.new_height = height;

  dlg = gimp_dialog_new (_("Tile"), "tile",
			 gimp_plugin_help_func, "filters/tile.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), tile_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Tile to New Size"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  sizeentry = gimp_coordinates_new (unit, "%a", TRUE, TRUE, 75,
				    GIMP_SIZE_ENTRY_UPDATE_SIZE,

				    tvals.constrain, TRUE,

				    _("Width:"), width, xres,
				    1, GIMP_MAX_IMAGE_SIZE,
				    0, width,

				    _("Height:"), height, yres,
				    1, GIMP_MAX_IMAGE_SIZE,
				    0, height);
  gtk_container_set_border_width (GTK_CONTAINER (sizeentry), 4);
  gtk_container_add (GTK_CONTAINER (frame), sizeentry);
  gtk_table_set_row_spacing (GTK_TABLE (sizeentry), 1, 4);
  gtk_widget_show (sizeentry);

  tint.sizeentry = sizeentry;
  tint.chainbutton = GTK_WIDGET (GIMP_COORDINATES_CHAINBUTTON (sizeentry));

  toggle = gtk_check_button_new_with_label (_("Create New Image"));
  gtk_table_attach (GTK_TABLE (sizeentry), toggle, 0, 4, 2, 3,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &tvals.new_image);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), tvals.new_image);
  gtk_widget_show (toggle);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return tint.run;
}

static void
tile_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  tvals.new_width =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (tint.sizeentry), 0);
  tvals.new_height =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (tint.sizeentry), 1);

  tvals.constrain =
    gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (tint.chainbutton));

  tint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
