/*
 * This is a plug-in for the GIMP.
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
 *
 */

/*
 * Exchange one color with the other (settable threshold to convert from
 * one color-shade to another...might do wonders on certain images, or be
 * totally useless on others).
 * 
 * Author: robert@experimental.net
 * 
 * TODO:
 *		- locken van scales met elkaar
 */

/* 
 * 1999/03/17   Fixed RUN_NONINTERACTIVE and RUN_WITH_LAST_VALS. 
 *              There were uninitialized variables.
 *                                        --Sven <sven@gimp.org>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define	SCALE_WIDTH  128
#define PREVIEW_SIZE 128

/* datastructure to store parameters in */
typedef struct
{
  guchar  fromred, fromgreen, fromblue;
  guchar  tored, togreen, toblue;
  guchar  red_threshold, green_threshold, blue_threshold;
  gint32  image;
  gint32  drawable;
} myParams;

/* lets prototype */
static void	query (void);
static void	run   (gchar   *name,
		       gint     nparams,
		       GParam  *param,
		       gint    *nreturn_vals,
		       GParam **return_vals);

static void	exchange      (void);
static void	real_exchange (gint, gint, gint, gint, int);

static int	doDialog              (void);
static void	update_preview        (void);
static void	ok_callback           (GtkWidget *, gpointer);
static void	lock_callback         (GtkWidget *, gpointer);
static void	color_button_callback (GtkWidget *, gpointer);
static void	scale_callback        (GtkAdjustment *, gpointer);

/* some global variables */
GDrawable *drw;
gboolean   has_alpha;
myParams   xargs = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
gint       running = 0;
GPixelRgn  origregion;
GtkWidget *preview;
GtkWidget *from_colorbutton;
GtkWidget *to_colorbutton;
gint       sel_x1, sel_y1, sel_x2, sel_y2;
gint       prev_width, prev_height, sel_width, sel_height;
gint       lock_thres = 0;

/* lets declare what we want to do */
GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

/* run program */
MAIN()

/* tell GIMP who we are */
static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT8, "fromred", "Red value (from)" },
    { PARAM_INT8, "fromgreen", "Green value (from)" },
    { PARAM_INT8, "fromblue", "Blue value (from)" },
    { PARAM_INT8, "tored", "Red value (to)" },
    { PARAM_INT8, "togreen", "Green value (to)" },
    { PARAM_INT8, "toblue", "Blue value (to)" },
    { PARAM_INT8, "red_threshold", "Red threshold" },
    { PARAM_INT8, "green_threshold", "Green threshold" },
    { PARAM_INT8, "blue_threshold", "Blue threshold" },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof(args) / sizeof(args[0]);
  static int nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_exchange",
			  "Color Exchange",
			  "Exchange one color with another, optionally setting a threshold to convert from one shade to another",
			  "robert@experimental.net",
			  "robert@experimental.net",
			  "June 17th, 1997",
			  N_("<Image>/Filters/Colors/Map/Color Exchange..."),
			  "RGB*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

/* main function */
static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam	values[1];
  GRunModeType	runmode;
  GStatusType 	status = STATUS_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  runmode        = param[0].data.d_int32;
  xargs.image    = param[1].data.d_image;
  xargs.drawable = param[2].data.d_drawable;
  drw = gimp_drawable_get (xargs.drawable);

  switch (runmode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      /* retrieve stored arguments (if any) */
      gimp_get_data ("plug_in_exchange", &xargs);
      /* initialize using foreground color */
      gimp_palette_get_foreground (&xargs.fromred,
				   &xargs.fromgreen,
				   &xargs.fromblue);
      /* and initialize some other things */
      gimp_drawable_mask_bounds (drw->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);
      sel_width = sel_x2 - sel_x1;
      sel_height = sel_y2 - sel_y1;
      if (sel_width > PREVIEW_SIZE)
	prev_width = PREVIEW_SIZE;
      else
	prev_width = sel_width;
      if (sel_height > PREVIEW_SIZE)
	prev_height = PREVIEW_SIZE;
      else
	prev_height = sel_height;
      has_alpha = gimp_drawable_has_alpha (drw->id);
      if (!doDialog ())
	return;
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      gimp_get_data ("plug_in_exchange", &xargs);
      /* 
       * instead of recalling the last-set values,
       * run with the current foreground as 'from'
       * color, making ALT-F somewhat more useful.
       */
      gimp_palette_get_foreground (&xargs.fromred,
				   &xargs.fromgreen,
				   &xargs.fromblue);
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      if (nparams != 10)
	status = STATUS_EXECUTION_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  xargs.fromred         = param[3].data.d_int8;
	  xargs.fromgreen       = param[4].data.d_int8;
	  xargs.fromblue        = param[5].data.d_int8;
	  xargs.tored           = param[6].data.d_int8;
	  xargs.togreen         = param[7].data.d_int8;
	  xargs.toblue          = param[8].data.d_int8;
	  xargs.red_threshold   = param[9].data.d_int32;
	  xargs.green_threshold = param[10].data.d_int32;
	  xargs.blue_threshold  = param[11].data.d_int32;
	}
      break;
    default:	
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      if (gimp_drawable_is_rgb (drw->id))
	{
	  gimp_progress_init (_("Color Exchange..."));
	  gimp_tile_cache_ntiles (2 * (drw->width / gimp_tile_width () + 1));
	  exchange ();	
	  gimp_drawable_detach( drw);

	  /* store our settings */
	  if (runmode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_exchange", &xargs, sizeof (myParams));

	  /* and flush */
	  if (runmode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();
	}
      else
	status = STATUS_EXECUTION_ERROR;
    }
  values[0].data.d_status = status;
}

/* do the exchanging */
static void
exchange (void)
{
  /* do the real exchange */
  real_exchange (-1, -1, -1, -1, FALSE);
}

/* show our dialog */
static gint
doDialog (void)
{
  GtkWidget *dialog;
  GtkWidget *mainbox;
  GtkWidget *frame;
  GtkWidget *abox;
  GtkWidget *pframe;
  GtkWidget *table;
  GtkWidget *colorbutton;
  GtkObject *adj;
  guchar  *color_cube;
  gchar	 **argv;
  gint     argc;
  gint     framenumber;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("exchange");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  /* stuff for preview */
  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ()); 
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
			      color_cube[2], color_cube[3]); 
  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  /* load pixelregion */
  gimp_pixel_rgn_init (&origregion, drw,
		       0, 0, PREVIEW_SIZE, PREVIEW_SIZE, FALSE, FALSE);

  /* set up the dialog */
  dialog = gimp_dialog_new (_("Color Exchange"), "exchange",
			    gimp_plugin_help_func, "filters/exchange.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    _("OK"), ok_callback,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* do some boxes here */
  mainbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (mainbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), mainbox,
		      TRUE, TRUE, 0);

  frame = gtk_frame_new (_("Preview"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (mainbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);

  pframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (pframe), 4);
  gtk_container_add (GTK_CONTAINER (abox), pframe);
  gtk_widget_show (pframe);

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), prev_width, prev_height);
  gtk_container_add (GTK_CONTAINER (pframe), preview);
  update_preview ();
  gtk_widget_show (preview); 

  /* and our scales */
  for (framenumber = 0; framenumber < 2; framenumber++)
    {
      guint id;

      frame = gtk_frame_new (framenumber ? _("To Color") : _("From Color"));
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
      gtk_box_pack_start (GTK_BOX (mainbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      table = gtk_table_new (framenumber ? 3 : 9, 4, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 4);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);
      gtk_container_set_border_width (GTK_CONTAINER (table), 4);
      gtk_container_add (GTK_CONTAINER (frame), table);
      gtk_widget_show (table);

      colorbutton = gimp_color_button_new (framenumber ?
					   _("Color Exchange: To Color") :
					   _("Color Exchange: From Color"),
					   SCALE_WIDTH / 2, 16,
					   framenumber ?
					   &xargs.tored : &xargs.fromred,
					   3);
      gimp_table_attach_aligned (GTK_TABLE (table), 0,
				 NULL, 0.0, 0.0,
				 colorbutton, TRUE);
      gtk_signal_connect (GTK_OBJECT (colorbutton), "color_changed",
			  GTK_SIGNAL_FUNC (color_button_callback),
			  NULL);

      if (framenumber)
	to_colorbutton = colorbutton;
      else
	from_colorbutton = colorbutton;

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
				  _("Red:"), SCALE_WIDTH, 0,
				  framenumber ? xargs.tored : xargs.fromred,
				  0, 255, 1, 8, 0, NULL, NULL);
      gtk_object_set_user_data (GTK_OBJECT (adj), colorbutton);
      gtk_object_set_data (GTK_OBJECT (colorbutton), "red", adj);
      id = gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			       GTK_SIGNAL_FUNC (scale_callback),
			       framenumber ? &xargs.tored : &xargs.fromred);
      gtk_object_set_data (GTK_OBJECT (adj), "handler", (gpointer) id);

      if (! framenumber)
	{
	  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
				      _("Red Threshold:"), SCALE_WIDTH, 0,
				      xargs.red_threshold,
				      0, 255, 1, 8, 0, NULL, NULL);
	  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			      GTK_SIGNAL_FUNC (scale_callback),
			      &xargs.red_threshold);
	}

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, framenumber ? 2 : 3,
				  _("Green:"), SCALE_WIDTH, 0,
				  framenumber ? xargs.togreen : xargs.fromgreen,
				  0, 255, 1, 8, 0, NULL, NULL);
      gtk_object_set_user_data (GTK_OBJECT (adj), colorbutton);
      gtk_object_set_data (GTK_OBJECT (colorbutton), "green", adj);
      id = gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			       GTK_SIGNAL_FUNC (scale_callback),
			       framenumber ? &xargs.togreen : &xargs.fromgreen);
      gtk_object_set_data (GTK_OBJECT (adj), "handler", (gpointer) id);

      if (! framenumber)
	{
	  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 4,
				      _("Green Threshold:"), SCALE_WIDTH, 0,
				      xargs.green_threshold,
				      0, 255, 1, 8, 0, NULL, NULL);
	  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			      GTK_SIGNAL_FUNC (scale_callback),
			      &xargs.green_threshold);
	}

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, framenumber ? 3 : 5,
				  _("Blue:"), SCALE_WIDTH, 0,
				  framenumber ? xargs.toblue : xargs.fromblue,
				  0, 255, 1, 8, 0, NULL, NULL);
      gtk_object_set_user_data (GTK_OBJECT (adj), colorbutton);
      gtk_object_set_data (GTK_OBJECT (colorbutton), "blue", adj);
      id = gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			       GTK_SIGNAL_FUNC (scale_callback),
			       framenumber ? &xargs.toblue : &xargs.fromblue);
      gtk_object_set_data (GTK_OBJECT (adj), "handler", (gpointer) id);

      if (! framenumber)
	{
	  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 6,
				      _("Blue Threshold:"), SCALE_WIDTH, 0,
				      xargs.blue_threshold,
				      0, 255, 1, 8, 0, NULL, NULL);
	  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			      GTK_SIGNAL_FUNC (scale_callback),
			      &xargs.blue_threshold);
	}

      if (! framenumber)
	{
	  GtkWidget *button;

	  button = gtk_check_button_new_with_label (_("Lock Thresholds"));
	  gtk_table_attach (GTK_TABLE (table), button, 1, 3, 7, 8,
			    GTK_FILL, 0, 0, 0);
	  gtk_signal_connect (GTK_OBJECT (button), "clicked",
			      GTK_SIGNAL_FUNC (lock_callback),
			      dialog);
	  gtk_widget_show (button);
	}
    }

  /* show everything */
  gtk_widget_show (mainbox);
  gtk_widget_show (dialog);

  gtk_main ();
  gdk_flush ();

  return running;
}

static void
ok_callback (GtkWidget *widget,
	     gpointer   data)
{
  running = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
lock_callback (GtkWidget *widget,
	       gpointer   data)
{
  lock_thres = 1 - lock_thres;
}

static void
color_button_callback (GtkWidget *widget,
		       gpointer   data)
{
  GtkObject *red_adj;
  GtkObject *green_adj;
  GtkObject *blue_adj;

  guint red_handler;
  guint green_handler;
  guint blue_handler;

  red_adj   = (GtkObject *) gtk_object_get_data (GTK_OBJECT (widget), "red");
  green_adj = (GtkObject *) gtk_object_get_data (GTK_OBJECT (widget), "green");
  blue_adj  = (GtkObject *) gtk_object_get_data (GTK_OBJECT (widget), "blue");

  red_handler   = (guint) gtk_object_get_data (GTK_OBJECT (red_adj),
					       "handler");
  green_handler = (guint) gtk_object_get_data (GTK_OBJECT (green_adj),
					       "handler");
  blue_handler  = (guint) gtk_object_get_data (GTK_OBJECT (blue_adj),
					       "handler");

  gtk_signal_handler_block (GTK_OBJECT (red_adj),   red_handler);
  gtk_signal_handler_block (GTK_OBJECT (green_adj), green_handler);
  gtk_signal_handler_block (GTK_OBJECT (blue_adj),  blue_handler);

  if (widget == from_colorbutton)
    {
      gtk_adjustment_set_value (GTK_ADJUSTMENT (red_adj),   xargs.fromred);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (green_adj), xargs.fromgreen);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (blue_adj),  xargs.fromblue);
    }
  else
    {
      gtk_adjustment_set_value (GTK_ADJUSTMENT (red_adj),   xargs.tored);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (green_adj), xargs.togreen);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (blue_adj),  xargs.toblue);
    }

  gtk_signal_handler_unblock (GTK_OBJECT (red_adj),   red_handler);
  gtk_signal_handler_unblock (GTK_OBJECT (green_adj), green_handler);
  gtk_signal_handler_unblock (GTK_OBJECT (blue_adj),  blue_handler);

  update_preview ();
}

static void
scale_callback (GtkAdjustment *adj,
		gpointer       data)
{
  GtkWidget *colorbutton;
  guchar *val = data;

  *val = (guchar) adj->value;

  colorbutton = gtk_object_get_user_data (GTK_OBJECT (adj));

  if (colorbutton)
    gimp_color_button_update (GIMP_COLOR_BUTTON (colorbutton));

  update_preview ();
}

static void
update_preview (void)
{
  real_exchange (sel_x1, sel_y1,
		 sel_x1 + prev_width, sel_y1 + prev_height,
		 TRUE);

  gtk_widget_draw (preview, NULL);
  gdk_flush ();
}

static void
real_exchange (gint x1,
	       gint y1,
	       gint x2,
	       gint y2,
	       gint dopreview)
{
  GPixelRgn  srcPR, destPR;
  guchar    *src_row, *dest_row;
  gint       x, y, bpp = drw->bpp;
  gint       width, height;

  /* fill if necessary */
  if (x1 == -1 || y1 == -1 || x2 == -1 || y2 == -1)
    {
      x1 = sel_x1;
      y1 = sel_y1;
      x2 = sel_x2;
      y2 = sel_y2;
    }

  /* check for valid coordinates */
  width  = x2 - x1;
  height = y2 - y1;

  /* allocate memory */
  src_row  = g_new (guchar, drw->width * bpp);

  if (dopreview && has_alpha)
    dest_row = g_new (guchar, drw->width * (bpp - 1));
  else
    dest_row = g_new (guchar, drw->width * bpp);

  /* initialize the pixel regions */
  /*
    gimp_pixel_rgn_init (&srcPR, drw, x1, y1, width, height, FALSE, FALSE);
  */
  gimp_pixel_rgn_init (&srcPR, drw, 0, 0, drw->width, drw->height, FALSE, FALSE);

  if (! dopreview)
    gimp_pixel_rgn_init (&destPR, drw, 0, 0, width, height, TRUE, TRUE);

  for (y = y1; y < y2; y++)
    {
      gimp_pixel_rgn_get_row (&srcPR, src_row, 0, y, drw->width);
      for (x = x1; x < x2; x++)
	{
	  gint pixel_red, pixel_green, pixel_blue;
	  gint min_red, max_red, min_green, max_green, min_blue, max_blue;
	  gint new_red, new_green, new_blue;
	  gint idx, rest;

	  /* get boundary values */
	  min_red   = MAX (xargs.fromred - xargs.red_threshold, 0);
	  min_green = MAX (xargs.fromgreen - xargs.green_threshold, 0);
	  min_blue  = MAX (xargs.fromblue - xargs.blue_threshold, 0);

	  max_red   = MIN (xargs.fromred + xargs.red_threshold, 255);
	  max_green = MIN (xargs.fromgreen + xargs.green_threshold, 255);
	  max_blue  = MIN (xargs.fromblue + xargs.blue_threshold, 255);

	  /* get current pixel-values */
	  pixel_red   = src_row[x * bpp];
	  pixel_green = src_row[x * bpp + 1];
	  pixel_blue  = src_row[x * bpp + 2];

	  /* shift down for preview */
	  if (dopreview)
	    {
	      if (has_alpha)
		idx = (x - x1) * (bpp - 1);
	      else
		idx = (x - x1) * bpp;
	    }
	  else
	    idx = x * bpp;

	  /* want this pixel? */
	  if (pixel_red >= min_red &&
	      pixel_red <= max_red &&
	      pixel_green >= min_green &&
	      pixel_green <= max_green &&
	      pixel_blue >= min_blue &&
	      pixel_blue <= max_blue)
	    {
	      gint red_delta, green_delta, blue_delta;

	      red_delta   = pixel_red - xargs.fromred;
	      green_delta = pixel_green - xargs.fromgreen;
	      blue_delta  = pixel_blue - xargs.fromblue;
	      new_red   = (guchar) CLAMP (xargs.tored + red_delta, 0, 255);
	      new_green = (guchar) CLAMP (xargs.togreen + green_delta, 0, 255);
	      new_blue  = (guchar) CLAMP (xargs.toblue + blue_delta, 0, 255);
	    }
	  else
	    {
	      new_red   = pixel_red;
	      new_green = pixel_green;
	      new_blue  = pixel_blue;
	    }

	  /* fill buffer (cast it too) */
	  dest_row[idx + 0] = (guchar) new_red;
	  dest_row[idx + 1] = (guchar) new_green;
	  dest_row[idx + 2] = (guchar) new_blue;

	  /* copy rest (most likely alpha-channel) */
	  for (rest = 3; rest < bpp; rest++)
	    dest_row[idx + rest] = src_row[x * bpp + rest];
	}
      /* store the dest */
      if (dopreview)
	gtk_preview_draw_row (GTK_PREVIEW (preview), dest_row, 0, y - y1, width);
      else
	gimp_pixel_rgn_set_row (&destPR, dest_row, 0, y, drw->width);
      /* and tell the user what we're doing */
      if (! dopreview && (y % 10) == 0)
	gimp_progress_update ((double) y / (double) height);
    }
  g_free(src_row);
  g_free(dest_row);
  if (! dopreview)
    {
      /* update the processed region */
      gimp_drawable_flush (drw);
      gimp_drawable_merge_shadow (drw->id, TRUE);
      gimp_drawable_update (drw->id, x1, y1, width, height);
    }
}
