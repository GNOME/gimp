/* Deinterlace 1.00 - image processing plug-in for the Gimp 1.0 API
 *
 * Copyright (C) 1997 Andrew Kieschnick (andrewk@mail.utexas.edu)
 *
 * Original deinterlace for the Gimp 0.54 API by Federico Mena Quintero
 *
 * Copyright (C) 1996 Federico Mena Quintero
 *
 * Any bugs in this code are probably my (Andrew Kieschnick's) fault, as I
 * pretty much rewrote it from scratch.
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


enum
{
  ODD_FIELDS,
  EVEN_FIELDS
};

/* Declare local functions.
 */
static void      query  (void);
static void      run    (gchar     *name,
			 gint       nparams,
			 GParam    *param,
			 gint      *nreturn_vals,
			 GParam   **return_vals);

static void      deinterlace        (GDrawable  *drawable);

static gint      deinterlace_dialog (void);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static gint DeinterlaceValue = EVEN_FIELDS;

MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT32, "evenodd", "0 = keep odd, 1 = keep even" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("plug_in_deinterlace",
			  "Deinterlace",
			  "Deinterlace is useful for processing images from "
			  "video capture cards. When only the odd or even "
			  "fields get captured, deinterlace can be used to "
			  "interpolate between the existing fields to correct "
			  "this.",
			  "Andrew Kieschnick",
			  "Andrew Kieschnick",
			  "1997",
			  N_("<Image>/Filters/Enhance/Deinterlace..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, 0,
			  args, NULL);
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_get_data ("plug_in_deinterlace", &DeinterlaceValue);
      if (! deinterlace_dialog ())
	status = STATUS_EXECUTION_ERROR;
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      if (nparams != 4)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	DeinterlaceValue = param[3].data.d_int32;
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      gimp_get_data ("plug_in_deinterlace", &DeinterlaceValue);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->id) ||
	  gimp_drawable_is_gray (drawable->id))
	{
	  gimp_progress_init (_("Deinterlace..."));
	  gimp_tile_cache_ntiles (2 * (drawable->width /
				       gimp_tile_width () + 1));
	  deinterlace (drawable);

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();
	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_deinterlace",
			   &DeinterlaceValue, sizeof (DeinterlaceValue));
	}
      else
	{
	  /* gimp_message ("deinterlace: cannot operate on indexed color images"); */
	  status = STATUS_EXECUTION_ERROR;
	}
    }
  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
deinterlace (GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  guchar *dest;
  guchar *upper;
  guchar *lower;
  gint row, col;
  gint x1, y1, x2, y2;

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width  = drawable->width;
  height = drawable->height;
  bytes  = drawable->bpp;

  /*  allocate row buffers  */
  dest  = g_new (guchar, (x2 - x1) * bytes);
  upper = g_new (guchar, (x2 - x1) * bytes);
  lower = g_new (guchar, (x2 - x1) * bytes);

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  /*  loop through the rows, performing our magic*/
  for (row = y1; row < y2; row++)
    {
      gimp_pixel_rgn_get_row (&srcPR, dest, x1, row, (x2-x1));

      /* Only do interpolation if the row:
	 (1) Isn't one we want to keep
	 (2) Has both an upper and a lower row
	 Otherwise, just duplicate the source row
      */
      if (!((row % 2 == DeinterlaceValue) ||
	    (row - 1 < 0) ||
	    (row + 1 >= height)))
	{
	  gimp_pixel_rgn_get_row (&srcPR, upper, x1, row-1, (x2-x1));
	  gimp_pixel_rgn_get_row (&srcPR, lower, x1, row+1, (x2-x1));
	  for (col=0; col < ((x2-x1)*bytes); col++)
	    dest[col]=(upper[col]+lower[col])>>1;
	}
      gimp_pixel_rgn_set_row (&destPR, dest, x1, row, (x2-x1));

      if ((row % 5) == 0)
	gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  /*  update the deinterlaced region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

  g_free (lower);
  g_free (upper);
  g_free (dest);
}

gboolean run_flag = FALSE;

static void
deinterlace_ok_callback (GtkWidget *widget,
			 gpointer   data)
{
  run_flag = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static gint
deinterlace_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *vbox;
  GtkWidget *frame;

  gimp_ui_init ("deinterlace", FALSE);

  dlg = gimp_dialog_new (_("Deinterlace"), "deinterlace",
			 gimp_plugin_help_func, "filters/deinterlace.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), deinterlace_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame =
    gimp_radio_group_new2 (TRUE, _("Mode"),
			   gimp_radio_button_update,
			   &DeinterlaceValue, (gpointer) DeinterlaceValue,

			   _("Keep Odd Fields"), (gpointer) ODD_FIELDS, NULL,
			   _("Keep Even Fields"), (gpointer) EVEN_FIELDS, NULL,

				 NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return run_flag;
}
