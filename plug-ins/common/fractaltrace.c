/*******************************************************************************

  fractaltrace.c  -- This is a plug-in for the GIMP 1.0

  Copyright (C) 1997  Hirotsuna Mizuno
                      s1041150@u-aizu.ac.jp

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*******************************************************************************/

#define PLUG_IN_NAME     "plug_in_fractal_trace"
#define PLUG_IN_VERSION  "v0.4 test version (Dec. 25 1997)"

/******************************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/******************************************************************************/

static void query  (void);
static void run    (gchar   *name,
		    gint     nparams,
		    GimpParam  *param,
		    gint    *nreturn_vals,
		    GimpParam **return_vals);

static void filter      (GimpDrawable *drawable);

static void pixels_init (GimpDrawable *drawable);
static void pixels_free (void);

static int  dialog_show         (void);
static void dialog_preview_draw (void);

/******************************************************************************/

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

MAIN ()

/******************************************************************************/

enum
{
  OUTSIDE_TYPE_WRAP,
  OUTSIDE_TYPE_TRANSPARENT,
  OUTSIDE_TYPE_BLACK,
  OUTSIDE_TYPE_WHITE
};

typedef struct
{
  gdouble x1;
  gdouble x2;
  gdouble y1;
  gdouble y2;
  gint32  depth;
  gint32  outside_type;
} parameter_t;

static parameter_t parameters =
{
  -1.0,
  +0.5,
  -1.0,
  +1.0,
  3,
  OUTSIDE_TYPE_WRAP
};

/******************************************************************************/

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",     "interactive / non-interactive"    },
    { GIMP_PDB_IMAGE,    "image",        "input image (not used)"           },
    { GIMP_PDB_DRAWABLE, "drawable",     "input drawable"                   },
    { GIMP_PDB_FLOAT,    "xmin",         "xmin fractal image delimiter"     },
    { GIMP_PDB_FLOAT,    "xmax",         "xmax fractal image delimiter"     },
    { GIMP_PDB_FLOAT,    "ymin",         "ymin fractal image delimiter"     },
    { GIMP_PDB_FLOAT,    "ymax",         "ymax fractal image delimiter"     },
    { GIMP_PDB_INT32,    "depth",        "trace depth"                      },
    { GIMP_PDB_INT32,    "outside_type", "outside type"
                                      "(0=WRAP/1=TRANS/2=BLACK/3=WHITE)" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure (PLUG_IN_NAME,
			  "transform image with the Mandelbrot Fractal",
			  "transform image with the Mandelbrot Fractal",
			  "Hirotsuna Mizuno <s1041150@u-aizu.ac.jp>",
			  "Copyright (C) 1997 Hirotsuna Mizuno",
			  PLUG_IN_VERSION,
			  N_("<Image>/Filters/Map/Fractal Trace..."),
			  "RGB*, GRAY*",
			  GIMP_PLUGIN,
			  nargs, 0,
			  args, NULL);
}

/******************************************************************************/

typedef struct
{
  gint     x1;
  gint     x2;
  gint     y1;
  gint     y2;
  gint     width;
  gint     height;
  gdouble  center_x;
  gdouble  center_y;
} selection_t;

typedef struct
{
  gint         width;
  gint         height;
  gint         bpp;
  gint         alpha;
} image_t;

static selection_t selection;
static image_t     image;
  
/******************************************************************************/

static void
run (gchar   *name,
     gint     argc,
     GimpParam  *args,
     gint    *retc,
     GimpParam **rets)
{
  GimpDrawable     *drawable;
  GimpRunModeType   run_mode;
  GimpPDBStatusType    status;
  static GimpParam  returns[1];

  run_mode = args[0].data.d_int32;
  status   = GIMP_PDB_SUCCESS;

  drawable     = gimp_drawable_get (args[2].data.d_drawable);
  image.width  = gimp_drawable_width( drawable->id);
  image.height = gimp_drawable_height (drawable->id);
  image.bpp    = gimp_drawable_bpp (drawable->id);
  image.alpha  = gimp_drawable_has_alpha (drawable->id);
  gimp_drawable_mask_bounds (drawable->id, &selection.x1, &selection.y1,
			     &selection.x2, &selection.y2);
  selection.width    = selection.x2 - selection.y1;
  selection.height   = selection.y2 - selection.y1;
  selection.center_x = selection.x1 + (gdouble) selection.width / 2.0;
  selection.center_y = selection.y1 + (gdouble) selection.height / 2.0;

  pixels_init (drawable);

  if (!gimp_drawable_is_rgb(drawable->id) && 
      !gimp_drawable_is_gray(drawable->id))
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  switch (run_mode)
    {
    case GIMP_RUN_WITH_LAST_VALS:
      INIT_I18N();
      gimp_get_data (PLUG_IN_NAME, &parameters);
      break;

    case GIMP_RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_get_data (PLUG_IN_NAME, &parameters);
      if (!dialog_show ())
	{
	  status = GIMP_PDB_EXECUTION_ERROR;
	  break;
	}
      gimp_set_data (PLUG_IN_NAME, &parameters, sizeof (parameter_t));
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (argc != 9)
	{
	  status = GIMP_PDB_CALLING_ERROR;
	}
      else
	{
	  parameters.x1           = args[3].data.d_float;
	  parameters.x2           = args[4].data.d_float;
	  parameters.y1           = args[5].data.d_float;
	  parameters.y2           = args[6].data.d_float;
	  parameters.depth        = args[7].data.d_int32;
	  parameters.outside_type = args[8].data.d_int32;
	}
      INIT_I18N();
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));
      filter (drawable);
      if (run_mode != GIMP_RUN_NONINTERACTIVE)
	gimp_displays_flush();
    }

  gimp_drawable_detach (drawable);

  pixels_free ();

  returns[0].type          = GIMP_PDB_STATUS;
  returns[0].data.d_status = status;
  *retc = 1;
  *rets = returns;
}

/******************************************************************************/

static guchar    **spixels;
static guchar    **dpixels;
static GimpPixelRgn   sPR;
static GimpPixelRgn   dPR;

typedef struct
{
  guchar r;
  guchar g;
  guchar b;
  guchar a;
} pixel_t;

static void
pixels_init (GimpDrawable *drawable)
{
  gint y;

  gimp_pixel_rgn_init (&sPR, drawable,
		       0, 0, image.width, image.height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dPR, drawable,
		       0, 0, image.width, image.height, TRUE, TRUE);

  spixels = g_new (guchar *, image.height);
  dpixels = g_new (guchar *, image.height);

  for (y = 0; y < image.height; y++)
    {
      spixels[y] = g_new (guchar, image.width * image.bpp);
      dpixels[y] = g_new (guchar, image.width * image.bpp);
      gimp_pixel_rgn_get_row (&sPR, spixels[y], 0, y, image.width);
    }
}

static void
pixels_free (void)
{
  gint y;

  for (y = 0; y < image.height; y++)
    {
      g_free (spixels[y]);
      g_free (dpixels[y]);
    }
  g_free (spixels);
  g_free (dpixels);
}

static void
pixels_get (gint     x,
	    gint     y,
	    pixel_t *pixel)
{
  if(x < 0) x = 0; else if (image.width  <= x) x = image.width  - 1;
  if(y < 0) y = 0; else if (image.height <= y) y = image.height - 1;

  switch (image.bpp)
    {
    case 1: /* GRAY */
      pixel->r = spixels[y][x*image.bpp];
      pixel->g = spixels[y][x*image.bpp];
      pixel->b = spixels[y][x*image.bpp];
      pixel->a = 255;
      break;
    case 2: /* GRAY+A */
      pixel->r = spixels[y][x*image.bpp];
      pixel->g = spixels[y][x*image.bpp];
      pixel->b = spixels[y][x*image.bpp];
      pixel->a = spixels[y][x*image.bpp+1];
      break;
    case 3: /* RGB */
      pixel->r = spixels[y][x*image.bpp];
      pixel->g = spixels[y][x*image.bpp+1];
      pixel->b = spixels[y][x*image.bpp+2];
      pixel->a = 255;
      break;
    case 4: /* RGB+A */
      pixel->r = spixels[y][x*image.bpp];
      pixel->g = spixels[y][x*image.bpp+1];
      pixel->b = spixels[y][x*image.bpp+2];
      pixel->a = spixels[y][x*image.bpp+3];
      break;
    }
}

static void
pixels_get_biliner (gdouble  x,
		    gdouble  y,
		    pixel_t *pixel)
{
  pixel_t A, B, C, D;
  gdouble a, b, c, d;
  gint    x1, y1, x2, y2;
  gdouble dx, dy;

  x1 = (gint) floor (x);
  x2 = x1 + 1;
  y1 = (gint) floor (y);
  y2 = y1 + 1;

  dx = x - (gdouble) x1;
  dy = y - (gdouble) y1;
  a  = (1.0 - dx) * (1.0 - dy);
  b  = dx * (1.0 - dy);
  c  = (1.0 - dx) * dy;
  d  = dx * dy;

  pixels_get (x1, y1, &A);
  pixels_get (x2, y1, &B);
  pixels_get (x1, y2, &C);
  pixels_get (x2, y2, &D);

  pixel->r = (guchar) (a * (gdouble) A.r + b * (gdouble) B.r +
		       c * (gdouble) C.r + d * (gdouble) D.r);
  pixel->g = (guchar) (a * (gdouble) A.g + b * (gdouble) B.g +
		       c * (gdouble) C.g + d * (gdouble) D.g);
  pixel->b = (guchar) (a * (gdouble) A.b + b * (gdouble) B.b +
		       c * (gdouble) C.b + d * (gdouble) D.b);
  pixel->a = (guchar) (a * (gdouble) A.a + b * (gdouble) B.a +
		       c * (gdouble) C.a + d * (gdouble) D.a);
}

static void
pixels_set (gint     x,
	    gint     y,
	    pixel_t *pixel)
{
  switch (image.bpp)
    {
    case 1: /* GRAY */
      dpixels[y][x*image.bpp]   = pixel->r;
      break;
    case 2: /* GRAY+A */
      dpixels[y][x*image.bpp]   = pixel->r;
      dpixels[y][x*image.bpp+1] = pixel->a;
      break;
    case 3: /* RGB */
      dpixels[y][x*image.bpp]   = pixel->r;
      dpixels[y][x*image.bpp+1] = pixel->g;
      dpixels[y][x*image.bpp+2] = pixel->b;
      break;
    case 4: /* RGB+A */
      dpixels[y][x*image.bpp]   = pixel->r;
      dpixels[y][x*image.bpp+1] = pixel->g;
      dpixels[y][x*image.bpp+2] = pixel->b;
      dpixels[y][x*image.bpp+3] = pixel->a;
      break;
    }
}

static void
pixels_store (void)
{
  gint y;

  for (y = selection.y1; y < selection.y2; y++)
    {
      gimp_pixel_rgn_set_row (&dPR, dpixels[y], 0, y, image.width);
    }
}

/******************************************************************************/

static void
mandelbrot (gdouble  x,
	    gdouble  y,
	    gdouble *u,
	    gdouble *v)
{
  gint    iter = 0;
  gdouble xx   = x;
  gdouble yy   = y;
  gdouble x2   = xx * xx;
  gdouble y2   = yy * yy;
  gdouble tmp;

  while (iter < parameters.depth)
    {
      tmp = x2 - y2 + x;
      yy  = 2 * xx * yy + y;
      xx  = tmp;
      x2  = xx * xx;
      y2  = yy * yy;
      iter++;
    }
  *u = xx;
  *v = yy;
}

/******************************************************************************/

static void
filter (GimpDrawable *drawable)
{
  gint    x, y;
  pixel_t pixel;
  gdouble scale_x, scale_y;
  gdouble cx, cy;
  gdouble px, py;

  gimp_progress_init (_("Fractal Trace"));

  scale_x = (parameters.x2 - parameters.x1) / selection.width;
  scale_y = (parameters.y2 - parameters.y1) / selection.height;

  for (y = selection.y1; y < selection.y2; y++)
    {
      cy = parameters.y1 + (y - selection.y1) * scale_y;
      for (x = selection.x1; x < selection.x2; x++)
	{
	  cx = parameters.x1 + (x - selection.x1) * scale_x;
	  mandelbrot (cx, cy, &px, &py);
	  px = (px - parameters.x1) / scale_x + selection.x1;
	  py = (py - parameters.y1) / scale_y + selection.y1;
	  if (0 <= px && px < image.width && 0 <= py && py < image.height)
	    {
	      pixels_get_biliner (px, py, &pixel);
	    }
	  else
	    {
	      switch (parameters.outside_type)
		{
		case OUTSIDE_TYPE_WRAP:
		  px = fmod (px, image.width);
		  py = fmod (py, image.height);
		  if( px < 0.0) px += image.width;
		  if (py < 0.0) py += image.height;
		  pixels_get_biliner (px, py, &pixel);
		  break;
		case OUTSIDE_TYPE_TRANSPARENT:
		  pixel.r = pixel.g = pixel.b = 0;
		  pixel.a = 0;
		  break;
		case OUTSIDE_TYPE_BLACK:
		  pixel.r = pixel.g = pixel.b = 0;
		  pixel.a = 255;
		  break;
		case OUTSIDE_TYPE_WHITE:
		  pixel.r = pixel.g = pixel.b = 255;
		  pixel.a = 255;
		  break;
		}
	    }
	  pixels_set (x, y, &pixel);
	}
      gimp_progress_update ((gdouble) (y-selection.y1) / selection.height);
    }

  pixels_store ();

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id,
			selection.x1, selection.y1,
			selection.width, selection.height);
}

/******************************************************************************/

#define PREVIEW_SIZE 200

typedef struct
{
  GtkWidget  *preview;
  guchar    **source;
  guchar    **pixels;
  gdouble     scale;
  gint        width;
  gint        height;
  gint        bpp;
} preview_t;

static preview_t preview;

static void
dialog_preview_setpixel (gint     x,
			 gint     y,
			 pixel_t *pixel)
{
  switch (preview.bpp)
    {
    case 1:
      preview.pixels[y][x*preview.bpp] = pixel->r;
      break;
    case 3:
      preview.pixels[y][x*preview.bpp]   = pixel->r;
      preview.pixels[y][x*preview.bpp+1] = pixel->g;
      preview.pixels[y][x*preview.bpp+2] = pixel->b;
      break;
    }
}

static void
dialog_preview_store (void)
{
  gint y;

  for (y = 0; y < preview.height; y++)
    {
      gtk_preview_draw_row (GTK_PREVIEW (preview.preview),
			    preview.pixels[y], 0, y, preview.width);
    }
  gtk_widget_draw (preview.preview, NULL);
  gdk_flush ();
}

static void
dialog_preview_init (void)
{
  pixel_t  pixel;
  gint     x, y;
  gdouble  cx, cy;

  if (image.width < image.height)
    preview.scale = (gdouble)selection.height / (gdouble)PREVIEW_SIZE;
  else
    preview.scale = (gdouble)selection.width / (gdouble)PREVIEW_SIZE;
  preview.width  = (gdouble)selection.width / preview.scale;
  preview.height = (gdouble)selection.height / preview.scale;

  if (image.bpp < 3)
    {
      preview.bpp = 1;
      preview.preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
    }
  else
    {
      preview.bpp = 3;
      preview.preview = gtk_preview_new (GTK_PREVIEW_COLOR);
    }
  gtk_preview_size (GTK_PREVIEW (preview.preview),
		    preview.width, preview.height);

  preview.source = g_new (guchar *, preview.height);
  preview.pixels = g_new (guchar *, preview.height);
  for (y = 0; y < preview.height; y++)
    {
      preview.source[y] = g_new (guchar, preview.width * preview.bpp);
      preview.pixels[y] = g_new (guchar, preview.width * preview.bpp);
    }

  for (y = 0; y < preview.height; y++)
    {
      cy = selection.y1 + (gdouble)y * preview.scale;
      for (x = 0; x < preview.width; x++)
	{
	  cx = selection.x1 + (gdouble)x * preview.scale;
	  pixels_get_biliner (cx, cy, &pixel);
	  dialog_preview_setpixel (x, y, &pixel);
	}
    }
  dialog_preview_store ();
}

static void
dialog_preview_draw (void)
{
  gint    x, y;
  pixel_t pixel;
  gdouble scale_x, scale_y;
  gdouble cx, cy;
  gdouble px, py;

  scale_x = (parameters.x2 - parameters.x1) / preview.width;
  scale_y = (parameters.y2 - parameters.y1) / preview.height;

  for (y = 0; y < preview.height; y++)
    {
      cy = parameters.y1 + y * scale_y;
      for (x = 0; x < preview.width; x++)
	{
	  cx = parameters.x1 + x * scale_x;
	  mandelbrot(cx, cy, &px, &py);
	  px = (px - parameters.x1) / scale_x * preview.scale + selection.x1;
	  py = (py - parameters.y1) / scale_y * preview.scale + selection.y1;
	  if (0 <= px && px < image.width && 0 <= py && py < image.height)
	    {
	      pixels_get_biliner (px, py, &pixel);
	    }
	  else
	    {
	      switch (parameters.outside_type)
		{
		case OUTSIDE_TYPE_WRAP:
		  px = fmod (px, image.width);
		  py = fmod (py, image.height);
		  if (px < 0.0) px += image.width;
		  if (py < 0.0) py += image.height;
		  pixels_get_biliner (px, py, &pixel);
		  break;
		case OUTSIDE_TYPE_TRANSPARENT:
		  pixel.r = pixel.g = pixel.b =
		    (((x % (GIMP_CHECK_SIZE * 2) < GIMP_CHECK_SIZE) ?
		      (y % (GIMP_CHECK_SIZE * 2) < GIMP_CHECK_SIZE) :
		      (y % (GIMP_CHECK_SIZE * 2) > GIMP_CHECK_SIZE)) ?
		     GIMP_CHECK_LIGHT : GIMP_CHECK_DARK) * 255;
		  break;
		case OUTSIDE_TYPE_BLACK:
		  pixel.r = pixel.g = pixel.b = 0;
		  break;
		case OUTSIDE_TYPE_WHITE:
		  pixel.r = pixel.g = pixel.b = 255;
		  break;
		}
	    }
	  dialog_preview_setpixel (x, y, &pixel);
	}
    }

  dialog_preview_store ();
}

/******************************************************************************/

static gboolean dialog_status = FALSE;

static void
dialog_int_adjustment_update (GtkAdjustment *adjustment,
			      gpointer       data)
{
  gimp_int_adjustment_update (adjustment, data);

  dialog_preview_draw ();
}

static void
dialog_double_adjustment_update (GtkAdjustment *adjustment,
				 gpointer       data)
{
  gimp_double_adjustment_update (adjustment, data);

  dialog_preview_draw ();
}

static void
dialog_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  dialog_status = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
dialog_outside_type_callback (GtkWidget *widget,
			      gpointer  *data)
{
  gimp_radio_button_update (widget, data);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    dialog_preview_draw ();
}

/******************************************************************************/

static gint
dialog_show (void)
{
  GtkWidget *dialog;
  GtkWidget *mainbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *abox;
  GtkWidget *pframe;
  GtkObject *adj;

  gimp_ui_init ("fractaltrace", TRUE);

  dialog = gimp_dialog_new (_("Fractal Trace"), "fractaltrace",
			    gimp_standard_help_func, "filters/fractaltrace.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    _("OK"), dialog_ok_callback,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  mainbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (mainbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), mainbox);
  gtk_widget_show (mainbox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (mainbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  Preview  */
  frame = gtk_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (abox), 4);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);

  pframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), pframe);
  gtk_widget_show (pframe);

  dialog_preview_init ();
  gtk_container_add (GTK_CONTAINER (pframe), preview.preview);
  gtk_widget_show (preview.preview);

  /*  Settings  */
  frame = gimp_radio_group_new2 (TRUE, _("Outside Type"),
				 dialog_outside_type_callback,
				 &parameters.outside_type,
				 (gpointer) parameters.outside_type,

				 _("Warp"),
				 (gpointer) OUTSIDE_TYPE_WRAP, NULL,
				 _("Transparent"),
				 (gpointer) OUTSIDE_TYPE_TRANSPARENT, NULL,
				 _("Black"),
				 (gpointer) OUTSIDE_TYPE_BLACK, NULL,
				 _("White"),
				 (gpointer) OUTSIDE_TYPE_WHITE, NULL,

				 NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame = gtk_frame_new (_("Mandelbrot Parameters"));
  gtk_box_pack_start (GTK_BOX (mainbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (5, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      "X1:", 0, 0,
			      parameters.x1, -50, 50, 0.1, 0.5, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (dialog_double_adjustment_update),
		      &parameters.x1);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      "X2:", 0, 0,
			      parameters.x2, -50, 50, 0.1, 0.5, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (dialog_double_adjustment_update),
		      &parameters.x2);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      "Y1:", 0, 0,
			      parameters.y1, -50, 50, 0.1, 0.5, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (dialog_double_adjustment_update),
		      &parameters.y1);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
			      "Y2:", 0, 0,
			      parameters.y2, -50, 50, 0.1, 0.5, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (dialog_double_adjustment_update),
		      &parameters.y2);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 4,
			      "Depth:", 0, 0,
			      parameters.depth, 1, 50, 1, 5, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (dialog_int_adjustment_update),
		      &parameters.depth);

  gtk_widget_show (dialog);
  dialog_preview_draw ();

  gtk_main ();
  gdk_flush ();

  return dialog_status;
}
