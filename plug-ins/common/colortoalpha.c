/*
 * Color To Alpha plug-in v1.0 by Seth Burgess, sjburges@gimp.org 1999/05/14
 *  with algorithm by clahey
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

/* TODO: 
 *  Add fg/bg buttons! 
 */
#include <stdio.h>
#include <stdlib.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#define PRV_WIDTH 40
#define PRV_HEIGHT 20

typedef struct {
  guchar color[3];
} C2AValues;

typedef struct {
  gint run;
} C2AInterface;

typedef struct {
  GtkWidget	*preview;
} C2APreview;

typedef struct
{
  GtkWidget *activate;   /* The button that activates the color sel. dialog */
  GtkWidget *colselect;  /* The colour selection dialog itself */
  GtkWidget *preview;    /* The colour preview */
  unsigned char color[3];/* The selected colour */
} CSEL;


/* Declare local functions.
 */
static void      query  (void);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

static void      toalpha            (GDrawable  *drawable);

static void      toalpha_render_row (const guchar *src_row,
				     guchar *dest_row,
				     gint row_width,
				     const gint bytes);
/* UI stuff */
static gint 	colortoalpha_dialog (GDrawable *drawable);
static void 	C2A_color_button_callback (GtkWidget *widget, gpointer data);
static void 	C2A_close_callback (GtkWidget *widget, gpointer data);
static void 	C2A_ok_callback (GtkWidget *widget, gpointer data);
static void 	C2A_bg_button_callback (GtkWidget *widget, gpointer data);
static void 	C2A_fg_button_callback (GtkWidget *widget, gpointer data);

static void     color_preview_show(GtkWidget *widget, guchar *color);
static void     color_select_ok_callback(GtkWidget *widget, gpointer data);
static void     color_select_cancel_callback(GtkWidget *widget, gpointer data);
static GRunModeType run_mode;

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static C2AValues pvals = 
{ 
  {255, 255, 255} /* white default */
};

static C2AInterface pint = 
{
  FALSE
};

static C2APreview ppreview = 
{
  NULL
};

MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_COLOR, "color", "Color to remove"},
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_colortoalpha",
			  "Convert the color in an image to alpha",
			  "This replaces as much of a given color as possible in each pixel with a corresponding amount of alpha, then readjusts the color accordingly.",
			  "Seth Burgess",
			  "Seth Burgess <sjburges@gimp.org>",
			  "7th Aug 1999",
			  "<Image>/Filters/Colors/Color To Alpha",
			  "RGBA",
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

  switch (run_mode)
  {
  case RUN_INTERACTIVE:
   gimp_get_data ("plug_in_colortoalpha", &pvals);
   if (! colortoalpha_dialog (drawable ))
    { 
      gimp_drawable_detach (drawable);
      return;
    }
  break;
 
  case RUN_NONINTERACTIVE:
    if (nparams != 3)
      status = STATUS_CALLING_ERROR;
    if (status == STATUS_SUCCESS)
    {
     pvals.color[0] = param[3].data.d_color.red;
     pvals.color[1] = param[3].data.d_color.green;
     pvals.color[2] = param[3].data.d_color.blue;
    }
    break;

  case RUN_WITH_LAST_VALS:
    gimp_get_data ("plug_in_colortoalpha", &pvals);
    break;
  default:
    break;
  }  

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is indexed or RGB color  */
      if (gimp_drawable_color (drawable->id) && 
          gimp_drawable_has_alpha(drawable->id))
	{
          if (run_mode != RUN_NONINTERACTIVE)
	    gimp_progress_init ("Removing color...");
             
	  toalpha (drawable);
	  gimp_displays_flush ();
	}
    }
  if (run_mode == RUN_INTERACTIVE)
    gimp_set_data ("plug_in_colortoalpha", &pvals, sizeof (pvals));

  values[0].data.d_status = status;
  gimp_drawable_detach (drawable);
}

void
colortoalpha(float *a1,
	     float *a2,
	     float *a3,
	     float *a4,
	     float c1,
	     float c2,
	     float c3)
{

  float alpha1, alpha2, alpha3;

  if ( *a1 > c1 )
    alpha1 = (*a1 - c1)/(255.0-c1);
  else if ( *a1 < c1 )
    alpha1 = (c1 - *a1)/(c1);
  else alpha1 = 0.0;

  if ( *a2 > c2 )
    alpha2 = (*a2 - c2)/(255.0-c2);
  else if ( *a2 < c2 )
    alpha2 = (c2 - *a2)/(c2);
  else alpha2 = 0.0;

  if ( *a3 > c3 )
    alpha3 = (*a3 - c3)/(255.0-c3);
  else if ( *a3 < c3 )
    alpha3 = (c3 - *a3)/(c3);
  else alpha3 = 0.0;

  if ( alpha1 > alpha2 )
    if ( alpha1 > alpha3 )
      {
	*a4 = alpha1;
      }
    else
      {
	*a4 = alpha3;
      }
  else
    if ( alpha2 > alpha3 )
      {
	*a4 = alpha2;
      }
    else
      {
	*a4 = alpha3;
      }
  *a4 *= 255.0;
  if ( *a4 < 1.0 )
    return;
  *a1 = 255.0 * (*a1-c1)/ *a4 + c1;
  *a2 = 255.0 * (*a2-c2)/ *a4 + c2;
  *a3 = 255.0 * (*a3-c3)/ *a4 + c3;

}
/*
<clahey> so if a1 > c1, a2 > c2, and a3 > c2 and a1 - c1 > a2-c2, a3-c3, then a1 = b1 * alpha + c1 * (1-alpha) So, maximizing alpha without taking b1 above 1 gives a1 = alpha + c1(1-alpha) and therefore alpha = (a1-c1)/(1-c1).
<AmJur2d> eek!  math!
> AmJur2d runs and hides behind a library carrel
<sjburges> clahey: btw, the ordering of that a2, a3 in the white->alpha didn't matter
<clahey> sjburges: You mean that it could be either a1, a2, a3 or a1, a3, a2?
<sjburges> yeah
<sjburges> because neither one uses the other
<clahey> sjburges: That's exactly as it should be.  They are both just getting reduced to the same amount, limited by the the darkest color.
<clahey> Then a2 = b2 * alpha + c2 * ( 1- alpha).  Solving for b2 gives b2 = (a1-c2)/alpha + c2.
<sjburges> yeah
<jlb> xachbot, url holy wars
<jlb> xachbot, url wars
<clahey> That gives us are formula for if the background is darker than the foreground? Yep.
<clahey> Next if a1 < c1, a2 < c2, a3 < c3, and c1-a1 > c2-a2, c3-a3, and by our desired result a1 = b1 * alpha + c1 * (1-alpha), we maximize alpha without taking b1 negative gives alpha = 1-a1/c1.
<clahey> And then again, b2 = (a2-c2)/alpha + c2 by the same formula.  (Actually, I think we can use that formula for all cases, though it may possibly introduce rounding error.
<clahey> sjburges: I like the idea of using floats to avoid rounding error.  Good call.
<clahey> It's cool to be able to replace all the black in an image with another color.  It'
*/

static void
toalpha_render_row (const guchar *src_data,
		    guchar *dest_data,
		    gint col,               /* row width in pixels */
		    const gint bytes)
{
  while (col--)
    {
      float v1, v2, v3, v4;

      v1 = (float)src_data[col*bytes   ];
      v2 = (float)src_data[col*bytes +1];
      v3 = (float)src_data[col*bytes +2];
      v4 = (float)src_data[col*bytes +3];
      /* For brighter than the background the rule is to send the
	 farthest above the background as the first address.
	 However, since v1 < COLOR_RED, for example, all of these
	 are negative so we have to invert the operator to reduce
	 the amount of typing to fix the problem.  :) */
      colortoalpha(&v1, &v2, &v3, &v4, 
                   (float)pvals.color[0],
                   (float)pvals.color[1],
                   (float)pvals.color[2]);

      dest_data[col*bytes   ] = (int)v1;
      dest_data[col*bytes +1] = (int)v2;
      dest_data[col*bytes +2] = (int)v3;
      dest_data[col*bytes +3] = (int)v4;
    }
}

static void
toalpha_render_region (const GPixelRgn srcPR,
		       const GPixelRgn destPR)
{
  gint row;
  guchar* src_ptr  = srcPR.data;
  guchar* dest_ptr = destPR.data;
  
  for (row = 0; row < srcPR.h ; row++)
    {
      if (srcPR.bpp!=4) {
	gimp_message("Not the proper bpp! \n");
	exit(1);
	} 
      toalpha_render_row (src_ptr, dest_ptr,
			  srcPR.w,
			  srcPR.bpp) ;

      src_ptr  += srcPR.rowstride;
      dest_ptr += destPR.rowstride;
    }
}

static void
toalpha (GDrawable *drawable)
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
      toalpha_render_region (srcPR, destPR);

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

static gint 
colortoalpha_dialog (GDrawable * drawable)
{
  GtkWidget *button;
  GtkWidget *dlg;
  GtkWidget *preview;
  GtkWidget *table;
  GtkWidget *label;

  guchar *color_cube;
  gchar **argv;
  gint argc;
  
  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("colortoalpha");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gdk_set_use_xshm (gimp_use_xshm ());
  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
                              color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Color To Alpha");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      (GtkSignalFunc) C2A_close_callback,
                      NULL);
  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) C2A_ok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             (GtkSignalFunc) gtk_widget_destroy,
                             GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  table = gtk_table_new(2,6,FALSE);
  button =   gtk_button_new ();
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                     (GtkSignalFunc) C2A_color_button_callback,
                     NULL);
  gtk_widget_set_usize(button, PRV_WIDTH+9, PRV_HEIGHT+9);
  preview = gtk_preview_new(GTK_PREVIEW_COLOR);
  ppreview.preview = preview; /* save in a global structure */

  gtk_preview_size (GTK_PREVIEW (preview), PRV_WIDTH, PRV_HEIGHT);
  gtk_container_add (GTK_CONTAINER (button), preview);
  gtk_widget_show(preview);
  color_preview_show(preview, pvals.color);
  gtk_widget_show(button);
 
  gtk_table_attach( GTK_TABLE(table), button, 2, 4, 0, 1, 
                                 GTK_EXPAND, GTK_SHRINK, 4, 4) ; 
  label = gtk_label_new("From:");
  gtk_widget_show(label);
  gtk_table_attach_defaults ( GTK_TABLE(table), label, 1, 2, 0, 1);
  
  label = gtk_label_new("to alpha");
  gtk_widget_show(label);
  gtk_table_attach_defaults ( GTK_TABLE(table), label, 4, 5, 0, 1);
  gtk_widget_show(table);
  gtk_box_pack_start( GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);

  button = gtk_button_new_with_label("Foreground");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                     (GtkSignalFunc) C2A_fg_button_callback,
                     NULL);
  gtk_widget_show(button);
  gtk_table_attach( GTK_TABLE(table), button, 1, 3, 1, 2, 
                                 GTK_EXPAND, GTK_SHRINK, 4, 0) ; 
  
  button = gtk_button_new_with_label("Background");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                     (GtkSignalFunc) C2A_bg_button_callback,
                     NULL);
  gtk_widget_show(button);
  gtk_table_attach( GTK_TABLE(table), button, 3, 5, 1, 2, 
                                 GTK_EXPAND, GTK_SHRINK, 4, 0) ; 
   
  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush();

  return pint.run;
}

static void
color_preview_show (GtkWidget *widget,
                    unsigned char *rgb)

{guchar *buf, *bp;
 int j, width, height;

 width = PRV_WIDTH;
 height = PRV_HEIGHT;

 bp = buf = g_malloc (width*3);
 if (buf == NULL) return;

 for (j = 0; j < width; j++)
 {
   *(bp++) = rgb[0];
   *(bp++) = rgb[1];
   *(bp++) = rgb[2];
 }
 for (j = 0; j < height; j++)
   gtk_preview_draw_row (GTK_PREVIEW (widget), buf, 0, j, width);

 gtk_widget_draw (widget, NULL);

 g_free (buf);
}



static void
C2A_close_callback(GtkWidget *widget,
                   gpointer data)
{
  gtk_main_quit ();
}
static void
C2A_ok_callback (GtkWidget *widget, 
                 gpointer data)
{
  pint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void 
C2A_color_button_callback (GtkWidget *widget, 
                           gpointer data)
{
  GtkColorSelectionDialog *csd;
  GtkWidget *dialog;
  gdouble color[3];
  gint i;
  for (i=0;i<3;i++)
    color[i] = (gdouble) pvals.color[i] / 255.0;
  
  dialog = gtk_color_selection_dialog_new("Color To Alpha Color Picker");
  csd = GTK_COLOR_SELECTION_DIALOG (dialog);

  gtk_widget_destroy (csd->help_button);
  
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
                      (GtkSignalFunc) color_select_cancel_callback, dialog);
  gtk_signal_connect (GTK_OBJECT (csd->ok_button), "clicked",
                     (GtkSignalFunc) color_select_ok_callback, dialog);
  gtk_signal_connect (GTK_OBJECT (csd->cancel_button), "clicked",
                     (GtkSignalFunc) color_select_cancel_callback, dialog);

  gtk_color_selection_set_color (GTK_COLOR_SELECTION (csd->colorsel), color);

  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_widget_show (dialog);
}

static void
color_select_ok_callback (GtkWidget *widget,
                          gpointer data)

{gdouble color[3];
 GtkWidget *dialog;
 gint j;

 dialog = (GtkWidget *) data;
 gtk_color_selection_get_color (
   GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel),
   color);

 for (j = 0; j < 3; j++)
   pvals.color[j] = (unsigned char)(color[j]*255.0);

 color_preview_show (ppreview.preview, pvals.color);

 gtk_widget_destroy (dialog);
}

static void
color_select_cancel_callback (GtkWidget *widget,
                              gpointer data)
{
 GtkWidget *dialog;

 dialog = (GtkWidget *)data;
 gtk_widget_destroy (dialog);
}

static void
C2A_fg_button_callback (GtkWidget *widget,
                          gpointer data)

{
 guchar color[3];
 int i;
 gimp_palette_get_foreground(color, color+1, color+2);

 for (i = 0; i < 3; i++)
   pvals.color[i] = color[i];

 color_preview_show (ppreview.preview, pvals.color);
}

static void
C2A_bg_button_callback (GtkWidget *widget,
                          gpointer data)

{
 guchar color[3];
 int i;
 gimp_palette_get_background(color, color+1, color+2);

 for (i = 0; i < 3; i++)
   pvals.color[i] = color[i];

 color_preview_show (ppreview.preview, pvals.color);
}
