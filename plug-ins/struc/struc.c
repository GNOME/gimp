/*
 * This is the Struc plug-in for the GIMP 0.99
 * Version 1.01
 *
 * Copyright (C) 1997 Karl-Johan Andersson (t96kja@student.tdb.uu.se)
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

/* Don't ask me why its called "Struc". The first .c-file
 * just happend to be called struc.c
 * 
 * Some code for the dialog was taken from Motion Blur plug-in for 
 * GIMP 0.99 by Daniel Skarda (0rfelyus@atrey.karlin.mff.cuni.cz) 
 * 
 * Please send any comments or suggestions to me,
 * Karl-Johan Andersson (t96kja@student.tdb.uu.se)
 * 
 */ 

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "struc.h"

/* --- Typedefs --- */
enum
{
  TOP_RIGHT,
  TOP_LEFT,
  BOTTOM_LEFT,
  BOTTOM_RIGHT
};

typedef struct
{
  gint direction;
  gint depth;
} StrucValues;

typedef struct
{
  gint run;
} StructInterface;

/* --- Declare local functions --- */
static void query (void);
static void run   (gchar     *name,
		   gint       nparams,
		   GParam    *param,
		   gint      *nreturn_vals,
		   GParam   **return_vals);

static gint struc_dialog      (void);
static void struc_ok_callback (GtkWidget *widget,
			       gpointer   data);

static void strucpi           (GDrawable *drawable);

/* --- Variables --- */

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};
static StrucValues svals =
{
  0,     /* direction*/
  4      /* depth */
};
static StructInterface s_int =
{
  FALSE     /* run */
};


/* --- Functions --- */

MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT32, "direction", "Light direction (0 - 3)" },
    { PARAM_INT32, "depth", "Texture depth (1 - 50)" },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0; 

  INIT_I18N();

  gimp_install_procedure ("plug_in_apply_canvas",
			  _("Adds a canvas texture map to the picture"),
			  _("This function applies a canvas texture map to the drawable."),
			  "Karl-Johan Andersson", /* Author */
			  "Karl-Johan Andersson", /* Copyright */
			  "1997",
			  N_("<Image>/Filters/Artistic/Apply Canvas..."),
			  "RGB*, GRAY*",
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
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  
  if (run_mode == RUN_INTERACTIVE)
    {
      INIT_I18N_UI();
    }
  else
    {
      INIT_I18N();
    }

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  
  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_struc", &svals);
      
      /*  First acquire information with a dialog  */
      if (! struc_dialog ())
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;
      
    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  svals.direction = (gint) param[3].data.d_int32;
	  svals.depth = (gint) param[4].data.d_int32;
	}
      if(svals.direction<0 || svals.direction>4) 
	status = STATUS_CALLING_ERROR;
      if(svals.depth<1 || svals.depth>50) 
	status = STATUS_CALLING_ERROR;
      break;
      
    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_struc", &svals);
      break;
      
    default:
      break;
    }
  
  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->id) || gimp_drawable_is_gray (drawable->id))
	{
	  gimp_progress_init ("struc");
	  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
	  
	  strucpi (drawable);
	      
	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush (); 
	  /*  Store data  */
	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_struc", &svals, sizeof (StrucValues));
	}
      else
	{
	  /* gimp_message ("struc: cannot operate on indexed color images"); */
	  status = STATUS_EXECUTION_ERROR;
	}
    }
  
  values[0].data.d_status = status;
  
  gimp_drawable_detach (drawable);
}

static gint
struc_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkObject *adj;
  gchar	**argv;	
  gint    argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("struc");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gimp_dialog_new (_("Apply Canvas"), "struc",
			 gimp_plugin_help_func, "filters/struc.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), struc_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* Parameter settings */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  frame =
    gimp_radio_group_new2 (TRUE, _("Direction"),
			   gimp_radio_button_update,
			   &svals.direction, (gpointer) svals.direction,

			   _("Top-Right"),    (gpointer) TOP_RIGHT, NULL,
			   _("Top-Left"),     (gpointer) TOP_LEFT, NULL,
			   _("Bottom-Left"),  (gpointer) BOTTOM_LEFT, NULL,
			   _("Bottom-Right"), (gpointer) BOTTOM_RIGHT, NULL,

			   NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Depth:"), 100, 0,
			      svals.depth, 1, 50, 1, 5, 0,
			      NULL, NULL);
  gtk_signal_connect (adj, "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &svals.depth);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return s_int.run;
}

static void
struc_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  s_int.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

/* Filter function */
static void
strucpi (GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  guchar *dest, *d;
  guchar *cur_row;
  gint row, col, rrow, rcol;
  gint x1, y1, x2, y2, varde;
  gint xm, ym, offs;
  gfloat mult;
      
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
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;
  
  /*  allocate row buffers  */
  cur_row  = (guchar *) malloc ((x2 - x1) * bytes);
  dest = (guchar *) malloc ((x2 - x1) * bytes);

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  mult=(gfloat)svals.depth*0.25;
  switch(svals.direction) 
    {
    case 0:
      xm=1;
      ym=128;
      offs=0;
      break;
    case 1:
      xm=-1;
      ym=128;
      offs=127;
      break;
    case 2:
      xm=128;
      ym=1;
      offs=0;
      break;
    case 3:
      xm=128;
      ym=-1;
      offs=128;
      break;
    default:
      xm=1;
      ym=128;
      offs=0;
      break;
    }

  /*  Loop through the rows */
  rrow = 0; rcol = 0;
  for (row = y1; row < y2; row++)
    {
      gimp_pixel_rgn_get_row (&srcPR, cur_row, x1, row, (x2-x1));
      d = dest;
      rcol = 0;
      switch (bytes)
	{
	case 1:  /* Grayscale */
	case 2:  /* Grayscale alpha */
	  for (col = 0; col < (x2 - x1) * bytes; col+=bytes)
	    {
	      varde = cur_row[col] + mult*sdata[rcol*xm+rrow*ym+offs];
	      if (varde > 255 ) varde = 255;
	      if (varde < 0) varde = 0;
	      *d++ = (guchar)varde;
	      if (bytes == 2) 
		*d++ = cur_row[col+1];
	      rcol++;
	      if (rcol == 128) rcol = 0;
	    }
	  break;
	case 3:  /* RGB */
	case 4:  /* RGB alpha */
	  for (col = 0; col < (x2 - x1) * bytes; col+=bytes)
	    {
	      varde = cur_row[col] + mult*sdata[rcol*xm+rrow*ym+offs];
	      if (varde > 255 ) varde = 255;
	      if (varde < 0) varde = 0;
	      *d++ = (guchar)varde;
	      varde = cur_row[col+1] + mult*sdata[rcol*xm+rrow*ym+offs];
	      if (varde > 255 ) varde = 255;
	      if (varde < 0) varde = 0;
	      *d++ = (guchar)varde;
	      varde = cur_row[col+2] + mult*sdata[rcol*xm+rrow*ym+offs];
	      if (varde > 255 ) varde = 255;
	      if (varde < 0) varde = 0;
	      *d++ = (guchar)varde;
	      if (bytes == 4)
		*d++ = cur_row[col+3];
	      rcol++;
	      if (rcol == 128) rcol = 0;
	    }
	  break;
	}
      /*  store the dest  */
      gimp_pixel_rgn_set_row (&destPR, dest, x1, row, (x2 - x1));

      rrow++;
      if (rrow == 128) rrow = 0;

      if ((row % 5) == 0)
	gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  /*  update the textured region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

  free (cur_row);
  free (dest);
}
