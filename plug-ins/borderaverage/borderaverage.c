/* borderaverage 0.01 - image processing plug-in for the Gimp 1.0 API
 *
 * Copyright (C) 1998 Philipp Klaus (webmaster@access.ch)
 *
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

#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Declare local functions.
 */
static void      query  (void);
static void      run    (gchar     *name,
			 gint       nparams,
			 GParam    *param,
			 gint      *nreturn_vals,
			 GParam   **return_vals);

static void      borderaverage (GDrawable *drawable,
				guchar    *res_r,
				guchar    *res_g,
				guchar    *res_b);

static gint      borderaverage_dialog (void);

static void      add_new_color (gint    bytes,
				guchar *buffer,
				gint   *cube,
				gint    bucket_expo);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init  */
  NULL,  /* quit  */
  query, /* query */
  run,   /* run   */
};

static gint  borderaverage_thickness       = 3;
static gint  borderaverage_bucket_exponent = 4;

struct borderaverage_data
{
  gint  thickness;
  gint  bucket_exponent;
}
borderaverage_data =
{
  3,
  4
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
    { PARAM_INT32, "thickness", "Border size to take in count" },
    { PARAM_INT32, "bucket_exponent", "Bits for bucket size (default=4: 16 Levels)" },
  };
  static GParamDef return_vals[] = 
  {
    { PARAM_INT32, "num_channels", "Number of color channels returned (always 3)" },
    { PARAM_INT8ARRAY, "color_vals", "The average color of the specified border"},
  };
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = sizeof (return_vals) / sizeof (return_vals[0]);

  INIT_I18N();

  gimp_install_procedure ("plug_in_borderaverage",
			  "Borderaverage",
			  "",
			  "Philipp Klaus",
			  "Internet Access AG",
			  "1998",
			  N_("<Image>/Filters/Colors/Border Average..."),
			  "RGB*",
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
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gint8		*result_color;

  INIT_I18N_UI();

  run_mode = param[0].data.d_int32;
	
  /* get the return memory */
  result_color = (gint8 *) g_new(gint8, 3);

  /*	Get the specified drawable	*/
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      gimp_get_data ("plug_in_borderaverage", &borderaverage_data);
      borderaverage_thickness       = borderaverage_data.thickness;
      borderaverage_bucket_exponent = borderaverage_data.bucket_exponent;
      if (! borderaverage_dialog ())
	status = STATUS_EXECUTION_ERROR;
      break;

    case RUN_NONINTERACTIVE:
      if (nparams != 5)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  borderaverage_thickness       = param[3].data.d_int32;
	  borderaverage_bucket_exponent = param[4].data.d_int32;
	}
      break;

    case RUN_WITH_LAST_VALS:
      gimp_get_data ("plug_in_borderaverage", &borderaverage_data);
      borderaverage_thickness       = borderaverage_data.thickness;
      borderaverage_bucket_exponent = borderaverage_data.bucket_exponent;
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is RGB color  */
      if (gimp_drawable_is_rgb (drawable->id))
	{
	  gimp_progress_init ( _("Border Average..."));
	  borderaverage (drawable,
			 &result_color[0], &result_color[1], &result_color[2]);

	  if (run_mode != RUN_NONINTERACTIVE)
	    {
	      gimp_palette_set_foreground (result_color[0],
					   result_color[1],
					   result_color[2]);
	    }
	  if (run_mode == RUN_INTERACTIVE)
	    {
	      borderaverage_data.thickness       = borderaverage_thickness;
	      borderaverage_data.bucket_exponent = borderaverage_bucket_exponent;
	      gimp_set_data ("plug_in_borderaverage",
			     &borderaverage_data, sizeof (borderaverage_data));
	    }
	}
      else
	{
	  status = STATUS_EXECUTION_ERROR;
	}
    }
  *nreturn_vals = 3;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
	
  values[1].type = PARAM_INT32;
  values[1].data.d_int32 = 3;
	
  values[2].type = PARAM_INT8ARRAY;
  values[2].data.d_int8array = result_color;

  gimp_drawable_detach (drawable);
}

static void
borderaverage (GDrawable *drawable, 
	       guchar    *res_r, 
	       guchar    *res_g, 
	       guchar    *res_b) 
{
  gint         width;
  gint         height;
  gint         x1, x2, y1, y2;
  gint         bytes;
  gint         max;
 
  guchar       r, g, b;

  guchar      *buffer;
  gint         bucket_num, bucket_expo, bucket_rexpo;

  gint        *cube;

  gint         row, col, i,j,k; /* index variables */
 
  GPixelRgn    myPR;

  /* allocate and clear the cube before */
  bucket_expo = borderaverage_bucket_exponent;
  bucket_rexpo = 8 - bucket_expo;
  cube = g_new (gint, 1 << (bucket_rexpo * 3));
  bucket_num = 1 << bucket_rexpo;
		
  for (i = 0; i < bucket_num; i++)
    {
      for (j = 0; j < bucket_num; j++)
	{
	  for (k = 0; k < bucket_num; k++)
	    {
	      cube[(i << (bucket_rexpo << 1)) + (j << bucket_rexpo) + k] = 0;
	    }
	}
    }

  /*  Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  /*  Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  /*  allocate row buffer  */
  buffer = g_new (guchar, (x2 - x1) * bytes);

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&myPR, drawable, 0, 0, width, height, FALSE, FALSE);

  /*  loop through the rows, performing our magic*/
  for (row = y1; row < y2; row++)
    {
      gimp_pixel_rgn_get_row (&myPR, buffer, x1, row, (x2-x1));
	
      if (row < y1 + borderaverage_thickness ||
	  row >= y2 - borderaverage_thickness)
	{
	  /* add the whole row */
	  for (col = 0; col < ((x2 - x1) * bytes); col += bytes)
	    {
	      add_new_color (bytes, &buffer[col], cube, bucket_expo);
	    }
	}
      else
	{
	  /* add the left border */
	  for (col = 0; col < (borderaverage_thickness * bytes); col += bytes)
	    {
	      add_new_color (bytes, &buffer[col], cube, bucket_expo);
	    }
	  /* add the right border */
	  for (col = ((x2 - x1 - borderaverage_thickness) * bytes);
	       col < ((x2 - x1) * bytes); col += bytes)
	    {
	      add_new_color (bytes, &buffer[col], cube, bucket_expo);
	    }
	}

      if ((row % 5) == 0)
	gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  max = 0; r = 0; g = 0; b = 0;

  /* get max of cube */
  for (i = 0; i < bucket_num; i++)
    {
      for (j = 0; j < bucket_num; j++)
	{
	  for (k = 0; k < bucket_num; k++)
	    {
	      if (cube[(i << (bucket_rexpo << 1)) +
		      (j << bucket_rexpo) + k] > max)
		{
		  max = cube[(i << (bucket_rexpo << 1)) +
			    (j << bucket_rexpo) + k];
		  r = (i<<bucket_expo) + (1<<(bucket_expo - 1));
		  g = (j<<bucket_expo) + (1<<(bucket_expo - 1));
		  b = (k<<bucket_expo) + (1<<(bucket_expo - 1));
		}
	    }
	}
    }

  /* return the color */
  *res_r = r;
  *res_g = g;
  *res_b = b;

  g_free (buffer);
  g_free (cube);
}

static void 
add_new_color (gint    bytes, 
	       guchar *buffer, 
	       gint   *cube, 
	       gint    bucket_expo) 
{
  guchar r, g, b;
  gint	 bucket_rexpo;

  bucket_rexpo = 8 - bucket_expo;
  r = buffer[0] >>bucket_expo;
  if (bytes > 1)
    {
      g = buffer[1] >>bucket_expo;
    }
  else
    {
      g = 0;
    }
  if (bytes > 2)
    {
      b = buffer[2] >>bucket_expo;
    }
  else
    {
      b = 0;
    }
  cube[(r << (bucket_rexpo << 1)) + (g << bucket_rexpo) + b]++;
}

static gboolean run_flag = FALSE;

static void
borderaverage_ok_callback (GtkWidget *widget,
			   gpointer   data)
{
  run_flag = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static gint 
borderaverage_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *menu;
  gchar **argv;
  gint    argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("borderaverage");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gimp_dialog_new (_("Borderaverage"), "borderaverage",
			 gimp_plugin_help_func, "filters/borderaverage.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), borderaverage_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dlg)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (_("Border Size"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Thickness:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&adj, borderaverage_thickness,
				     0, 256, 1, 5, 0, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &borderaverage_thickness);

  frame = gtk_frame_new (_("Number of Colors"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Bucket Size:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  menu = gimp_option_menu_new2 (FALSE, gimp_menu_item_update,
				&borderaverage_bucket_exponent,
				(gpointer) borderaverage_bucket_exponent,

				_("1 (nonsense?)"),   (gpointer) 0, NULL,
				"2",                  (gpointer) 1, NULL,
				"4",                  (gpointer) 2, NULL,
				"8",                  (gpointer) 3, NULL,
				"16",                 (gpointer) 4, NULL,
				"32",                 (gpointer) 5, NULL,
				"64",                 (gpointer) 6, NULL,
				"128",                (gpointer) 7, NULL,
				_("256 (nonsense?)"), (gpointer) 8, NULL,

				NULL);
  gtk_box_pack_start (GTK_BOX (hbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return run_flag;
}
