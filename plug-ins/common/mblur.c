/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Motion Blur plug-in for GIMP 0.99
 * Copyright (C) 1997 Daniel Skarda (0rfelyus@atrey.karlin.mff.cuni.cz)
 * 
 * This plug-in is port of Motion Blur plug-in for GIMP 0.54 by Thorsten Martinsen
 * 	Copyright (C) 1996 Torsten Martinsen <torsten@danbbs.dk>
 * 	Bresenham algorithm stuff hacked from HP2xx written by Heinz W. Werntges
 * 	Changes for version 1.11/1.12 Copyright (C) 1996 Federico Mena Quintero
 * 	quartic@polloux.fciencias.unam.mx
 *
 * I also used some code from Whirl and Pinch plug-in by Federico Mena Quintero
 * 	(federico@nuclecu.unam.mx)
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

/* Version 1.2
 *
 * 	Everything is new - no changes
 *
 * TODO:
 *     Bilinear interpolation from original mblur for 0.54
 *     Speed all things up
 *		? better caching scheme
 *		- while bluring along long trajektory do not averrage all 
 * 		  pixels but averrage only few samples
 *     Function for weight of samples along trajectory
 *     Preview
 *     Support paths in GiMP 1.1 :-)
 *     Smash all bugs :-)
 */

#include "config.h"

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_NAME 	"plug_in_mblur"
#define PLUG_IN_VERSION	"Sep 1997, 1.2"

#define     MBLUR_LINEAR 0
#define     MBLUR_RADIAL 1
#define     MBLUR_ZOOM	 2

#define	    MBLUR_MAX    MBLUR_ZOOM

typedef struct
{
  gint32  mblur_type;
  gint32  length;
  gint32  angle;
} mblur_vals_t;

typedef struct
{
  gint       col, row;
  gint       img_width, img_height, img_bpp, img_has_alpha;
  gint       tile_width, tile_height;
  guchar     bg_color[4];
  GDrawable *drawable;
  GTile     *tile;
} pixel_fetcher_t;

/***** Prototypes *****/

static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GParam  *param,
		   gint    *nreturn_vals,
		   GParam **return_vals);

static pixel_fetcher_t *pixel_fetcher_new          (GDrawable *drawable);
static void             pixel_fetcher_set_bg_color (pixel_fetcher_t *pf,
						    guchar r, guchar g,
						    guchar b, guchar a);
static void             pixel_fetcher_get_pixel    (pixel_fetcher_t *pf,
						    int x, int y, guchar *pixel);
static void             pixel_fetcher_destroy       (pixel_fetcher_t *pf);

static void		mblur        (void);
static void 		mblur_linear (void);
static void 	        mblur_radial (void);
static void 	        mblur_zoom   (void);

static void       dialog_ok_callback   (GtkWidget *, gpointer);

static gboolean   mblur_dialog         (void);

/***** Variables *****/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

static mblur_vals_t mbvals =
{
  MBLUR_LINEAR,	/* mblur_type */
  5,		/* length */
  45		/* radius */
};

static gboolean mb_run = FALSE;

static GDrawable *drawable;

static gint img_width, img_height, img_bpp, img_has_alpha;
static gint sel_x1, sel_y1, sel_x2, sel_y2;
static gint sel_width, sel_height;

static double cen_x, cen_y;

/***** Functions *****/

MAIN()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32,    "run_mode",  "Interactive, non-interactive" },
    { PARAM_IMAGE,    "image",     "Input image" },
    { PARAM_DRAWABLE, "drawable",  "Input drawable" },
    { PARAM_INT32,     "type",      "Type of motion blur (0 - linear, 1 - radial, 2 - zoom)" },
    { PARAM_INT32,     "length",    "Length" },
    { PARAM_INT32,     "angle",     "Angle" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure (PLUG_IN_NAME,
			  "Motion blur of image",
			  "This plug-in simulates the effect seen when "
			  "photographing a moving object at a slow shutter "
			  "speed. Done by adding multiple displaced copies.",
			  "Torsten Martinsen, Federico Mena Quintero and Daniel Skarda",
			  "Torsten Martinsen, Federico Mena Quintero and Daniel Skarda",       
			  PLUG_IN_VERSION,
			  N_("<Image>/Filters/Blur/Motion Blur..."),
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

  GRunModeType run_mode;
  GStatusType  status;

  status   = STATUS_SUCCESS;
  run_mode = param[0].data.d_int32;

  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  /* Get the active drawable info */

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  img_width     = gimp_drawable_width (drawable->id);
  img_height    = gimp_drawable_height (drawable->id);
  img_bpp       = gimp_drawable_bpp (drawable->id);
  img_has_alpha = gimp_drawable_has_alpha (drawable->id);

  gimp_drawable_mask_bounds (drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  /* Calculate scaling parameters */

  sel_width  = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  cen_x = (double) (sel_x1 + sel_x2 - 1) / 2.0;
  cen_y = (double) (sel_y1 + sel_y2 - 1) / 2.0;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();

      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_NAME, &mbvals);

      /* Get information from the dialog */
      if (!mblur_dialog())
	return;
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();

      /* Make sure all the arguments are present */
      if (nparams != 6)
	status = STATUS_CALLING_ERROR;

      if (status == STATUS_SUCCESS)
	{
	  mbvals.mblur_type = param[3].data.d_int32;
	  mbvals.length     = param[4].data.d_int32;
	  mbvals.angle      = param[5].data.d_int32;
	}

    if ((mbvals.mblur_type < 0) && (mbvals.mblur_type > MBLUR_ZOOM))
      status= STATUS_CALLING_ERROR;
    break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();

      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_NAME, &mbvals);
      break;

    default:
      break;
    }

  /* Blur the image */

  if ((status == STATUS_SUCCESS) &&
      (gimp_drawable_is_rgb(drawable->id) ||
       gimp_drawable_is_gray(drawable->id)))
    {
      /* Set the tile cache size */
      gimp_tile_cache_ntiles (2 * (drawable->width +
				   gimp_tile_width () - 1) / gimp_tile_width ());

      /* Run! */
      mblur ();

      /* If run mode is interactive, flush displays */
      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      /* Store data */
      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data (PLUG_IN_NAME, &mbvals, sizeof(mblur_vals_t));
    }
  else if (status == STATUS_SUCCESS)
    status = STATUS_EXECUTION_ERROR;

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void 
mblur_linear (void)
{
  GPixelRgn	dest_rgn;
  pixel_fetcher_t *pft;
  gpointer	pr;

  guchar	*dest, *d;
  guchar	pixel[4], bg_color[4];
  gint32	sum[4];

  gint          progress, max_progress;
  gint		c;

  int 		x, y, i, xx, yy, n;
  int 		dx, dy, px, py, swapdir, err, e, s1, s2;

  gimp_pixel_rgn_init (&dest_rgn, drawable,
		       sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);
  
  pft = pixel_fetcher_new (drawable);

  gimp_palette_get_background (&bg_color[0], &bg_color[1], &bg_color[2]);
  pixel_fetcher_set_bg_color (pft, bg_color[0], bg_color[1], bg_color[2],
			      (img_has_alpha ? 0 : 255));

  progress     = 0;
  max_progress = sel_width * sel_height;

  n = mbvals.length;
  px = n * cos (mbvals.angle / 180.0 * G_PI);
  py = n * sin (mbvals.angle / 180.0 * G_PI);

  /*
   * Initialization for Bresenham algorithm:
   * dx = abs(x2-x1), s1 = sign(x2-x1)
   * dy = abs(y2-y1), s2 = sign(y2-y1)
   */
  if ((dx = px) != 0)
    {
      if (dx < 0)
	{
	  dx = -dx;
	  s1 = -1;
	}
      else
	s1 = 1;
    }
  else
    s1 = 0;

  if ((dy = py) != 0)
    {
      if (dy < 0)
	{
	  dy = -dy;
	  s2 = -1;
	}
      else
	s2 = 1;
    }
  else
    s2 = 0;

  if (dy > dx)
    {
      swapdir = dx;
      dx = dy;
      dy = swapdir;
      swapdir = 1;
    }
  else
    swapdir = 0;

  dy *= 2;
  err = dy - dx;	/* Initial error term	*/
  dx *= 2;

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      dest = dest_rgn.data;

      for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++)
	{
	  d = dest;

	  for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
	    {
	      xx = x; yy = y; e = err;
	      for (c= 0; c < img_bpp; c++)
		sum[c]= 0;

	      for (i = 0; i < n; )
		{
		  pixel_fetcher_get_pixel (pft, xx, yy, pixel);
		  for (c= 0; c < img_bpp; c++)
		    sum[c]+= pixel[c];
		  i++;

		  while (e >= 0)
		    {
		      if (swapdir)
			xx += s1;
		      else
			yy += s2;
		      e -= dx;
		    }
		  if (swapdir)
		    yy += s2;
		  else
		    xx += s1;
		  e += dy;
		  if ((xx < sel_x1) || (xx >= sel_x2) ||
		      (yy < sel_y1) || (yy >= sel_y2))
		    break;
		}

	      if ( i==0 )
		{
		  pixel_fetcher_get_pixel (pft, xx, yy, d); 
		}
	      else
		{
		  for (c=0; c < img_bpp; c++)
		    d[c]= sum[c] / i;
		}
	      d+= dest_rgn.bpp;
	    }
	  dest += dest_rgn.rowstride;
	}
      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / max_progress);
    }
  pixel_fetcher_destroy (pft);
}

static void
mblur_radial (void)
{
  GPixelRgn	dest_rgn;
  pixel_fetcher_t *pft;
  gpointer	pr;

  guchar	*dest, *d;
  guchar	pixel[4], bg_color[4];
  gint32	sum[4];

  gint          progress, max_progress, c;

  int 		x, y, i, n, xr, yr;
  int 		count, R, r, w, h, step;
  float 	angle, theta, * ct, * st, offset, xx, yy;

  /* initialize */

  xx = 0.0;
  yy = 0.0;

  gimp_pixel_rgn_init (&dest_rgn, drawable,
		       sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);
  
  pft = pixel_fetcher_new (drawable);

  gimp_palette_get_background (&bg_color[0], &bg_color[1], &bg_color[2]);
  pixel_fetcher_set_bg_color (pft, bg_color[0], bg_color[1], bg_color[2],
			      (img_has_alpha ? 0 : 255));

  progress     = 0;
  max_progress = sel_width * sel_height;

  angle = ((float) mbvals.angle) / 180.0 * G_PI;
  w = MAX (img_width-cen_x, cen_x);
  h = MAX (img_height-cen_y, cen_y);
  R = sqrt (w * w + h * h);
  n = 4 * angle * sqrt (R) + 2;
  theta = angle / ((float) (n - 1));

  if (((ct = g_new (float, n)) == NULL) ||
      ((st = g_new (float, n)) == NULL))
    return;
  offset = theta * (n - 1) / 2;
  for (i = 0; i < n; ++i)
    {
      ct[i] = cos (theta * i - offset);
      st[i] = sin (theta * i - offset);
    }

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      dest = dest_rgn.data;

      for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++)
	{
	  d = dest;

	  for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
	    {
	      xr = x-cen_x;
	      yr = y-cen_y;
	      r = sqrt (xr * xr + yr * yr);
	      if (r == 0)
		step = 1;
	      else if ((step = R / r) == 0)
		step = 1;
	      else if (step > n-1)
		step = n-1;

	      for (c = 0; c < img_bpp; c++)
		sum[c] = 0;

	      for (i = 0, count = 0; i < n; i += step)
		{
		  xx = cen_x + xr * ct[i] - yr * st[i];
		  yy = cen_y + xr * st[i] + yr * ct[i];
		  if ((yy < sel_y1) || (yy >= sel_y2) ||
		      (xx < sel_x1) || (xx >= sel_x2))
		    continue;

		  ++count;
		  pixel_fetcher_get_pixel (pft, xx, yy, pixel);
		  for (c = 0; c < img_bpp; c++) 
		    sum[c] += pixel[c];
		}

	      if (count == 0)
		{
		  pixel_fetcher_get_pixel (pft, xx, yy, d); 
		}
	      else
		{
		  for (c = 0; c < img_bpp; c++)
		    d[c]= sum[c] / count;
		}
	      d += dest_rgn.bpp;
	    }
	  dest += dest_rgn.rowstride;
	}
      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / max_progress);
    }

  pixel_fetcher_destroy (pft);

  g_free (ct);
  g_free (st);
}

static void
mblur_zoom (void)
{
  GPixelRgn	dest_rgn;
  pixel_fetcher_t *pft;
  gpointer	pr;

  guchar	*dest, *d;
  guchar	pixel[4], bg_color[4];
  gint32	sum[4];

  gint          progress, max_progress;
  int 		x, y, i, xx, yy, n, c;
  float 	f;
    
  /* initialize */

  xx = 0.0;
  yy = 0.0;

  gimp_pixel_rgn_init (&dest_rgn, drawable,
		       sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);

  pft = pixel_fetcher_new (drawable);

  gimp_palette_get_background (&bg_color[0], &bg_color[1], &bg_color[2]);
  pixel_fetcher_set_bg_color (pft, bg_color[0], bg_color[1], bg_color[2],
			      (img_has_alpha ? 0 : 255));

  progress     = 0;
  max_progress = sel_width * sel_height;

  n = mbvals.length;
  f = 0.02;

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr)) 
    {
      dest = dest_rgn.data;

      for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++)
	{
	  d = dest;

	  for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
	    {
	      for (c = 0; c < img_bpp; c++)
		sum[c] = 0;

	      for (i = 0; i < n; ++i)
		{
		  xx = cen_x + (x-cen_x) * (1.0 + f * i);
		  yy = cen_y + (y-cen_y) * (1.0 + f * i);

		  if ((yy < sel_y1) || (yy >= sel_y2) ||
		      (xx < sel_x1) || (xx >= sel_x2))
		    break;

		  pixel_fetcher_get_pixel (pft, xx, yy, pixel);
		  for (c = 0; c < img_bpp; c++)
		    sum[c] += pixel[c];	  
		}

	      if (i == 0)
		{
		  pixel_fetcher_get_pixel (pft, xx, yy, d); 
		}
	      else
		{
		  for (c = 0; c < img_bpp; c++)
		    d[c] = sum[c] / i;
		}
	      d += dest_rgn.bpp;
	    }
	  dest += dest_rgn.rowstride;
	}
      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / max_progress);
    }
  pixel_fetcher_destroy (pft);
}

static void
mblur (void)
{
  gimp_progress_init (_("Blurring..."));

  switch (mbvals.mblur_type)
    {
    case MBLUR_LINEAR:
      mblur_linear ();
      break;
    case MBLUR_RADIAL:
      mblur_radial ();
      break;
    case MBLUR_ZOOM:
      mblur_zoom ();
      break;
    default:
      break;
    }

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, sel_x1, sel_y1, sel_width, sel_height);
}

/*****************************************
 * pixel_fetcher from whirlpinch plug-in 
 ****************************************/

static pixel_fetcher_t *
pixel_fetcher_new (GDrawable *drawable)
{
  pixel_fetcher_t *pf;

  pf = g_new (pixel_fetcher_t, 1);

  pf->col           = -1;
  pf->row           = -1;
  pf->img_width     = gimp_drawable_width (drawable->id);
  pf->img_height    = gimp_drawable_height (drawable->id);
  pf->img_bpp       = gimp_drawable_bpp (drawable->id);
  pf->img_has_alpha = gimp_drawable_has_alpha (drawable->id);
  pf->tile_width    = gimp_tile_width ();
  pf->tile_height   = gimp_tile_height ();
  pf->bg_color[0]   = 0;
  pf->bg_color[1]   = 0;
  pf->bg_color[2]   = 0;
  pf->bg_color[3]   = 0;

  pf->drawable    = drawable;
  pf->tile        = NULL;

  return pf;
}

static void
pixel_fetcher_set_bg_color (pixel_fetcher_t *pf,
			    guchar           r,
			    guchar           g,
			    guchar           b,
			    guchar           a)
{
  pf->bg_color[0] = r;
  pf->bg_color[1] = g;
  pf->bg_color[2] = b;

  if (pf->img_has_alpha)
    pf->bg_color[pf->img_bpp - 1] = a;
}

static void
pixel_fetcher_get_pixel (pixel_fetcher_t *pf,
			 int              x,
			 int              y,
			 guchar          *pixel)
{
  gint    col, row;
  gint    coloff, rowoff;
  guchar *p;
  int     i;

  if ((x < sel_x1) || (x >= sel_x2) ||
      (y < sel_y1) || (y >= sel_y2))
    {
      for (i = 0; i < pf->img_bpp; i++)
	pixel[i] = pf->bg_color[i];

      return;
    }

  col    = x / pf->tile_width;
  coloff = x % pf->tile_width;
  row    = y / pf->tile_height;
  rowoff = y % pf->tile_height;

  if ((col != pf->col) ||
      (row != pf->row) ||
      (pf->tile == NULL))
    {
      if (pf->tile != NULL)
	gimp_tile_unref (pf->tile, FALSE);

      pf->tile = gimp_drawable_get_tile (pf->drawable, FALSE, row, col);
      gimp_tile_ref (pf->tile);

      pf->col = col;
      pf->row = row;
    }

  p = pf->tile->data + pf->img_bpp * (pf->tile->ewidth * rowoff + coloff);

  for (i = pf->img_bpp; i; i--)
    *pixel++ = *p++;
}

static void
pixel_fetcher_destroy (pixel_fetcher_t *pf)
{
  if (pf->tile != NULL)
    gimp_tile_unref (pf->tile, FALSE);

  g_free (pf);
}

/****************************************
 *                 UI
 ****************************************/

static gboolean
mblur_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkObject *adjustment;

  gimp_ui_init ("mblur", FALSE);

  dialog = gimp_dialog_new (_("Motion Blur"), "mblur",
			    gimp_plugin_help_func, "filters/mblur.html",
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

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  frame =
    gimp_radio_group_new2 (TRUE, _("Blur Type"),
			   gimp_radio_button_update,
			   &mbvals.mblur_type, (gpointer) mbvals.mblur_type,

			   _("Linear"), (gpointer) MBLUR_LINEAR, NULL,
			   _("Radial"), (gpointer) MBLUR_RADIAL, NULL,
			   _("Zoom"),   (gpointer) MBLUR_ZOOM, NULL,

			   NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame = gtk_frame_new (_("Blur Parameters"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adjustment = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
				     _("Length:"), 150, 0,
				     mbvals.length, 0.0, 256.0, 1.0, 8.0, 0,
				     TRUE, 0, 0,
				     NULL, NULL);
  gtk_signal_connect (adjustment, "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &mbvals.length);

  adjustment = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
				     _("Angle:"), 150, 0,
				     mbvals.angle, 0.0, 360.0, 1.0, 15.0, 0,
				     TRUE, 0, 0,
				     NULL, NULL);
  gtk_signal_connect (adjustment, "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &mbvals.angle);

  gtk_widget_show (dialog);

  gtk_main ();
  gdk_flush ();

  return mb_run;
}

static void
dialog_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  mb_run= TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
