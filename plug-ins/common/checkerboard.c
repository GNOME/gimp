/*
 * This is a plug-in for the GIMP.
 *
 * Copyright (C) 1997 Brent Burton & the Edward Blevins
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
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/* Variables set in dialog box */
typedef struct data
{
  gint mode;
  gint size;
} CheckVals;

typedef struct
{
  gint run;
} CheckInterface;

static CheckInterface cint =
{
  FALSE
};

static void      query  (void);
static void      run    (gchar    *name,
			 gint      nparams,
			 GParam   *param,
			 gint     *nreturn_vals,
			 GParam  **return_vals);

static void      check   (GDrawable *drawable);
static gint      inblock (gint       pos,
			  gint       size);

static gint      check_dialog      (void);
static void      check_ok_callback (GtkWidget *widget,
				    gpointer   data);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static CheckVals cvals =
{
   0,
   10
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
    { PARAM_INT32, "check_mode", "Regular or Physcobilly" },
    { PARAM_INT32, "check_size", "Size of the checks" }
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_checkerboard",
			  "Adds a checkerboard pattern to an image",
			  "More here later",
			  "Brent Burton & the Edward Blevins",
			  "Brent Burton & the Edward Blevins",
			  "1997",
			  N_("<Image>/Filters/Render/Pattern/Checkerboard..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run    (gchar    *name,
	gint      nparams,
	GParam   *param,
	gint     *nreturn_vals,
	GParam  **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  INIT_I18N_UI();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      gimp_get_data ("plug_in_checkerboard", &cvals);
      if (! check_dialog())
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case RUN_NONINTERACTIVE:
      if (nparams != 5)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  cvals.mode = param[3].data.d_int32;
	  cvals.size = param[4].data.d_int32;
	}
      break;

    case RUN_WITH_LAST_VALS:
      gimp_get_data ("plug_in_checkerboard", &cvals);
      break;

    default:
      break;
    }

  if (gimp_drawable_is_rgb (drawable->id) ||
      gimp_drawable_is_gray (drawable->id))
    {
      gimp_progress_init (_("Adding Checkerboard..."));

      check (drawable);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_checkerboard", &cvals, sizeof (CheckVals));
    }
  else
    {
      status = STATUS_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


static void
check (GDrawable *drawable)
{
  GPixelRgn dest_rgn;
  guchar *dest_row;
  guchar *dest;
  gint row, col;
  gint progress, max_progress;
  gint x1, y1, x2, y2, x, y;
  guchar fg[4],bg[4];
  gint bp;
  gpointer pr;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  gimp_pixel_rgn_init (&dest_rgn, drawable,
		       x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  progress = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  /* Get the foreground and background colors */

  switch ( gimp_drawable_type (drawable->id) )
    {
    case RGBA_IMAGE:
      fg[3] = 255;
      bg[3] = 255;
    case RGB_IMAGE:
      gimp_palette_get_foreground (&fg[0], &fg[1], &fg[2]);
      gimp_palette_get_background (&bg[0], &bg[1], &bg[2]);
      break;
    case GRAYA_IMAGE:
      fg[1] = 255;
      bg[1] = 255;
    case GRAY_IMAGE:
      fg[0] = 255;
      bg[0] = 0;
      break;
    default:
      break;
    }

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      y = dest_rgn.y;

      dest_row = dest_rgn.data;
      for ( row = 0; row < dest_rgn.h; row++)
	{
	  dest = dest_row;
	  x = dest_rgn.x;

	  for (col = 0; col < dest_rgn.w; col++)
	    {
	      gint val, xp, yp;

	      if (cvals.mode)
		{
		  /* Psychobilly Mode */
		  val =
		    (inblock (x, cvals.size) == inblock (y, cvals.size)) ? 0 : 1;
		}
	      else
		{
		  /* Normal, regular checkerboard mode.
		   * Determine base factor (even or odd) of block
		   * this x/y position is in.
		   */
		  xp = x/cvals.size;
		  yp = y/cvals.size;
		  /* if both even or odd, color sqr */
		  val = ( (xp&1) == (yp&1) ) ? 0 : 1;
		}
	      for (bp = 0; bp < dest_rgn.bpp; bp++)
		dest[bp] = val ? fg[bp] : bg[bp];
	      dest += dest_rgn.bpp;
	      x++;
	    }
	  dest_row += dest_rgn.rowstride;
	  y++;
	}
      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / (double) max_progress);
    }
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

static gint
inblock (gint pos, 
	 gint size)
{
  static gint *in = NULL;		/* initialized first time */
  gint i,j,k, len;

  /* avoid a FP exception */
  if (size == 1)
    size = 2;

  len = size*size;
  /*
   * Initialize the array; since we'll be called thousands of
   * times with the same size value, precompute the array.
   */
  if (in == NULL)
    {
      in = g_new (gint, len);
      if (in == NULL)
	{
	  return 0;
	}
      else
	{
	  int cell = 1;	/* cell value */
	  /*
	   * i is absolute index into in[]
	   * j is current number of blocks to fill in with a 1 or 0.
	   * k is just counter for the j cells.
	   */
	  i=0;
	  for (j=1; j<=size; j++ )
	    { /* first half */
	      for (k=0; k<j; k++ )
		{
		  in[i++] = cell;
		}
	      cell = !cell;
	    }
	  for ( j=size-1; j>=1; j--)
	    { /* second half */
	      for (k=0; k<j; k++ )
		{
		  in[i++] = cell;
		}
	      cell = !cell;
	    }
	}
    }

  /* place pos within 0..(len-1) grid and return the value. */
  return in[ pos % (len-1) ];
}

static gint
check_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *toggle;
  GtkWidget *table;
  GtkObject *size_data;
  gchar **argv;
  gint    argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("checkerboard");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gimp_dialog_new (_("Checkerboard"), "checkerboard",
			 gimp_plugin_help_func, "filters/checkerboard.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), check_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_label (_("Psychobilly"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &cvals.mode);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cvals.mode);
  gtk_widget_show (toggle);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  size_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
				    _("Check Size:"), 200, 0,
				    cvals.size, 1, 400, 1, 10, 0,
				    NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (size_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &cvals.size);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return cint.run;
}

static void
check_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  cint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
