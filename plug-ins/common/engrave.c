/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Eiichi Takamori
 * Copyright (C) 1996, 1997 Torsten Martinsen
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
 * This plug-in creates a black-and-white 'engraved' version of an image.
 * Much of the code is stolen from the Pixelize plug-in.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/* Some useful macros */

#define SCALE_WIDTH     125
#define TILE_CACHE_SIZE  16

#define INTENSITY(r,g,b) (r * 0.30 + g * 0.59 + b * 0.11)

typedef struct
{
  gint height;
  gint limit;
} EngraveValues;

typedef struct
{
  gint run;
} EngraveInterface;

static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GParam  *param,
		   gint    *nreturn_vals,
		   GParam **return_vals);

static gint engrave_dialog      (void);
static void engrave_ok_callback (GtkWidget *widget,
				 gpointer   data);

static void engrave       (GDrawable *drawable);
static void engrave_large (GDrawable *drawable,
			   gint       height,
			   gint       limit);
static void engrave_small (GDrawable *drawable,
			   gint       height,
			   gint       limit,
			   gint       tile_width);
static void engrave_sub   (gint       height,
			   gint       limit,
			   gint       bpp,
			   gint       color_n);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static EngraveValues pvals =
{
  10
};

static EngraveInterface pint =
{
  FALSE			/* run */
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
    { PARAM_INT32, "height", "Resolution in pixels" },
    { PARAM_INT32, "limit", "If true, limit line width" }
  };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof(args) / sizeof(args[0]);
  static gint nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_engrave",
			  _("Engrave the contents of the specified drawable"),
			  _("Creates a black-and-white 'engraved' version of an image as seen in old illustrations"),
			  "Spencer Kimball & Peter Mattis, Eiichi Takamori, Torsten Martinsen",
			  "Spencer Kimball & Peter Mattis, Eiichi Takamori, Torsten Martinsen",
			  "1995,1996,1997",
			  N_("<Image>/Filters/Distorts/Engrave..."),
			  "RGBA, GRAYA",
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

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get(param[2].data.d_drawable);

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      /*  Possibly retrieve data  */
      gimp_get_data("plug_in_engrave", &pvals);

      /*  First acquire information with a dialog  */
      if (!engrave_dialog ())
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  pvals.height = param[3].data.d_int32;
	  pvals.limit  = (param[4].data.d_int32) ? TRUE : FALSE;
	}
      if ((status == STATUS_SUCCESS) &&
	  pvals.height < 0)
	status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_engrave", &pvals);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      gimp_progress_init (_("Engraving..."));
      gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

      engrave (drawable);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_engrave", &pvals, sizeof (EngraveValues));
    }
  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static gint
engrave_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *toggle;
  GtkObject *adj;
  gchar **argv;
  gint argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("engrave");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gimp_dialog_new (_("Engrave"), "engrave",
			 gimp_plugin_help_func, "filters/engrave.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), engrave_ok_callback,
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

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  toggle = gtk_check_button_new_with_label (_("Limit Line Width"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 3, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &pvals.limit);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), pvals.limit);
  gtk_widget_show (toggle);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Height:"), SCALE_WIDTH, 0,
			      pvals.height, 2.0, 16.0, 1.0, 4.0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &pvals.height);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return pint.run;
}

/*  Engrave interface functions  */

static void
engrave_ok_callback (GtkWidget *widget,
		     gpointer   data)
{
  pint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
engrave (GDrawable *drawable)
{
  gint tile_width;
  gint height;
  gint limit;

  tile_width = gimp_tile_width();
  height = (gint) pvals.height;
  limit = (gint) pvals.limit;
  if (height >= tile_width)
    engrave_large (drawable, height, limit);
  else
    engrave_small (drawable, height, limit, tile_width);
}

static void
engrave_large (GDrawable *drawable,
	       gint       height,
	       gint       limit)
{
  GPixelRgn src_rgn, dest_rgn;
  guchar *src_row, *dest_row;
  guchar *src, *dest;
  gulong *average;
  gint row, col, b, bpp;
  gint x, y, y_step, inten, v;
  gulong count;
  gint x1, y1, x2, y2;
  gint progress, max_progress;
  gpointer pr;

  gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);

  if (gimp_drawable_is_rgb(drawable->id))
    bpp = 3;
  else
    bpp = 1;
  average = g_new(gulong, bpp);

  /* Initialize progress */
  progress = 0;
  max_progress = 2 * (x2 - x1) * (y2 - y1);

  for (y = y1; y < y2; y += height - (y % height))
    {
      for (x = x1; x < x2; ++x)
	{
	  y_step = height - (y % height);
	  y_step = MIN(y_step, x2 - x);

	  gimp_pixel_rgn_init(&src_rgn, drawable, x, y, 1, y_step, FALSE, FALSE);
	  for (b = 0; b < bpp; b++)
	    average[b] = 0;
	  count = 0;

	  for (pr = gimp_pixel_rgns_register(1, &src_rgn);
	       pr != NULL;
	       pr = gimp_pixel_rgns_process(pr))
	    {
	      src_row = src_rgn.data;
	      for (row = 0; row < src_rgn.h; row++)
		{
		  src = src_row;
		  for (col = 0; col < src_rgn.w; col++)
		    {
		      for (b = 0; b < bpp; b++)
			average[b] += src[b];
		      src += src_rgn.bpp;
		      count += 1;
		    }
		  src_row += src_rgn.rowstride;
		}
	      /* Update progress */
	      progress += src_rgn.w * src_rgn.h;
	      gimp_progress_update((double) progress / (double) max_progress);
	    }

	  if (count > 0)
	    for (b = 0; b < bpp; b++)
	      average[b] = (guchar) (average[b] / count);

	  if (bpp < 3)
	    inten = average[0]/254.0*height;
	  else
	    inten = INTENSITY(average[0],
			      average[1],
			      average[2])/254.0*height;

	  gimp_pixel_rgn_init(&dest_rgn, drawable, x, y, 1, y_step, TRUE, TRUE);
	  for (pr = gimp_pixel_rgns_register(1, &dest_rgn);
	       pr != NULL;
	       pr = gimp_pixel_rgns_process(pr))
	    {
	      dest_row = dest_rgn.data;
	      for (row = 0; row < dest_rgn.h; row++)
		{
		  dest = dest_row;
		  v = inten > row ? 255 : 0;
		  if (limit)
		    {
		      if (row == 0)
			v = 255;
		      else if (row == height-1)
			v = 0;
		    }
		  for (b = 0; b < bpp; b++)
		    dest[b] = v;
		  dest_row += dest_rgn.rowstride;
		}
	      /* Update progress */
	      progress += dest_rgn.w * dest_rgn.h;
	      gimp_progress_update((double) progress / (double) max_progress);
	    }
	}
    }

  g_free(average);

  /*  update the engraved region  */
  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

typedef struct
{
  gint    x, y, h;
  gint    width;
  guchar *data;
} PixelArea;

PixelArea area;

static void
engrave_small (GDrawable *drawable,
	       gint       height,
	       gint       limit,
	       gint       tile_width)
{
  GPixelRgn src_rgn, dest_rgn;
  gint bpp, color_n;
  gint x1, y1, x2, y2;
  gint progress, max_progress;

  /*
    For speed efficiency, operates on PixelAreas, whose each width and
    height are less than tile size.

    If both ends of area cannot be divided by height ( as
    x1%height != 0 etc.), operates on the remainder pixels.
  */

  gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);
  gimp_pixel_rgn_init(&src_rgn, drawable,
		      x1, y1, x2 - x1, y2 - y1, FALSE, FALSE);
  gimp_pixel_rgn_init(&dest_rgn, drawable,
		      x1, y1, x2 - x1, y2 - y1, TRUE, TRUE);

  /* Initialize progress */
  progress = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  bpp = drawable->bpp;
  if (gimp_drawable_is_rgb(drawable->id))
    color_n = 3;
  else
    color_n = 1;

  area.width = (tile_width / height) * height;
  area.data = g_new(guchar, (glong) bpp * area.width * area.width);

  for (area.y = y1; area.y < y2;
       area.y += area.width - (area.y % area.width))
    {
      area.h = area.width - (area.y % area.width);
      area.h = MIN(area.h, y2 - area.y);
      for (area.x = x1; area.x < x2; ++area.x)
	{
	  gimp_pixel_rgn_get_rect(&src_rgn, area.data, area.x, area.y, 1, area.h);

	  engrave_sub(height, limit, bpp, color_n);

	  gimp_pixel_rgn_set_rect(&dest_rgn, area.data, area.x, area.y, 1, area.h);

	  /* Update progress */
	  progress += area.h;
	  gimp_progress_update((double) progress / (double) max_progress);
	}
    }

  g_free(area.data);

  /*  update the engraved region  */
  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

static void
engrave_sub (gint height,
	     gint limit,
	     gint bpp,
	     gint color_n)
{
  glong average[3];		/* color_n <= 3 */
  gint y, h, inten, v;
  guchar *buf_row, *buf;
  gint row;
  gint rowstride;
  gint count;
  gint i;

  /*
    Since there's so many nested FOR's,
    put a few of them here...
  */

  rowstride = bpp;

  for (y = area.y; y < area.y + area.h; y += height - (y % height))
    {
      h = height - (y % height);
      h = MIN(h, area.y + area.h - y);

      for (i = 0; i < color_n; i++)
	average[i] = 0;
      count = 0;

      /* Read */
      buf_row = area.data + (y - area.y) * rowstride;

      for (row = 0; row < h; row++)
	{
	  buf = buf_row;
	  for (i = 0; i < color_n; i++)
	    average[i] += buf[i];
	  count++;
	  buf_row += rowstride;
	}

      /* Average */
      if (count > 0) 
	for (i = 0; i < color_n; i++)
	  average[i] /= count;

      if (bpp < 3)
	inten = average[0]/254.0*height;
      else
	inten = INTENSITY(average[0],
			  average[1],
			  average[2])/254.0*height;

      /* Write */
      buf_row = area.data + (y - area.y) * rowstride;

      for (row = 0; row < h; row++)
	{
	  buf = buf_row;
	  v = inten > row ? 255 : 0;
	  if (limit)
	    {
	      if (row == 0)
		v = 255;
	      else if (row == height-1)
		v = 0;
	    }
	  for (i = 0; i < color_n; i++)
	    buf[i] = v;
	  buf_row += rowstride;
	}
    }
}
