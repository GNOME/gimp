/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Whirl and Pinch plug-in --- two common distortions in one place
 * Copyright (C) 1997 Federico Mena Quintero
 * federico@nuclecu.unam.mx
 * Copyright (C) 1997 Scott Goehring
 * scott@poverty.bloomington.in.us
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


/* Version 2.09:
 *
 * - Another cool patch from Scott.  The radius is now in [0.0, 2.0],
 * with 1.0 being the default as usual.  In addition to a minimal
 * speed-up (one multiplication is eliminated in the calculation
 * code), it is easier to think of nice round values instead of
 * sqrt(2) :-)
 *
 * - Modified the way out-of-range pixels are handled.  This time the
 * plug-in handles `outside' pixels better; it paints them with the
 * current background color (for images without transparency) or with
 * a completely transparent background color (for images with
 * transparency).
 */


/* Version 2.08:
 *
 * This is the first version of this plug-in.  It is called 2.08
 * because it came out of merging the old Whirl 2.08 and Pinch 2.08
 * plug-ins.  */

#include "config.h"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimp/gimplimits.h>
#include "libgimp/stdplugins-intl.h"

#define PLUG_IN_NAME    "plug_in_whirl_pinch"
#define PLUG_IN_VERSION "May 1997, 2.09"

/***** Magic numbers *****/

#define PREVIEW_SIZE 128
#define SCALE_WIDTH  200
#define ENTRY_WIDTH  60

/***** Types *****/

typedef struct
{
  gdouble whirl;
  gdouble pinch;
  gdouble radius;
} whirl_pinch_vals_t;

typedef struct
{
  GtkWidget *preview;
  guchar    *check_row_0;
  guchar    *check_row_1;
  guchar    *image;
  guchar    *dimage;

  gint run;
} whirl_pinch_interface_t;

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

static void   whirl_pinch (void);
static int    calc_undistorted_coords (double wx, double wy,
				       double whirl, double pinch,
				       double *x, double *y);
static guchar bilinear (double x, double y, guchar *values);

static pixel_fetcher_t *pixel_fetcher_new (GDrawable *drawable);
static void             pixel_fetcher_set_bg_color (pixel_fetcher_t *pf,
						    guchar r, guchar g,
						    guchar b, guchar a);
static void             pixel_fetcher_get_pixel (pixel_fetcher_t *pf, int x,
						 int y, guchar *pixel);
static void             pixel_fetcher_destroy (pixel_fetcher_t *pf);

static void build_preview_source_image (void);

static gint whirl_pinch_dialog    (void);
static void dialog_update_preview (void);
static void dialog_scale_update   (GtkAdjustment *adjustment, gdouble *value);
static void dialog_ok_callback    (GtkWidget *widget, gpointer data);


/***** Variables *****/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc */
  NULL,   /* quit_proc */
  query,  /* query_proc */
  run     /* run_proc */
};

static whirl_pinch_vals_t wpvals =
{
  90.0, /* whirl */
  0.0,  /* pinch */
  1.0   /* radius */
};

static whirl_pinch_interface_t wpint =
{
  NULL,  /* preview */
  NULL,  /* check_row_0 */
  NULL,  /* check_row_1 */
  NULL,  /* image */
  NULL,  /* dimage */
  FALSE  /* run */
};

static GDrawable *drawable;

static gint img_width, img_height, img_bpp, img_has_alpha;
static gint sel_x1, sel_y1, sel_x2, sel_y2;
static gint sel_width, sel_height;
static gint preview_width, preview_height;

static double cen_x, cen_y;
static double scale_x, scale_y;
static double radius, radius2;


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
    { PARAM_FLOAT,    "whirl",     "Whirl angle (degrees)" },
    { PARAM_FLOAT,    "pinch",     "Pinch amount" },
    { PARAM_FLOAT,    "radius",    "Radius (1.0 is the largest circle that fits in the image, "
      "and 2.0 goes all the way to the corners)" }
  }; /* args */

  static GParamDef *return_vals  = NULL;
  static int        nargs        = sizeof(args) / sizeof(args[0]);
  static int        nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure (PLUG_IN_NAME,
			  _("Distort an image by whirling and pinching"),
			  _("Distorts the image by whirling and pinching, which are two common center-based, circular distortions.  Whirling is like projecting the image onto the surface of water in a toilet and flushing.  Pinching is similar to projecting the image onto an elastic surface and pressing or pulling on the center of the surface."),
			  "Federico Mena Quintero and Scott Goehring",
			  "Federico Mena Quintero and Scott Goehring",
			  PLUG_IN_VERSION,
			  N_("<Image>/Filters/Distorts/Whirl and Pinch..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs,
			  nreturn_vals,
			  args,
			  return_vals);
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
  double       xhsiz, yhsiz;
  int          pwidth, pheight;

  status   = STATUS_SUCCESS;
  run_mode = param[0].data.d_int32;

  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  /* Get the active drawable info */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  img_width     = gimp_drawable_width(drawable->id);
  img_height    = gimp_drawable_height(drawable->id);
  img_bpp       = gimp_drawable_bpp(drawable->id);
  img_has_alpha = gimp_drawable_has_alpha(drawable->id);

  gimp_drawable_mask_bounds(drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  /* Calculate scaling parameters */

  sel_width  = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  cen_x = (double) (sel_x1 + sel_x2 - 1) / 2.0;
  cen_y = (double) (sel_y1 + sel_y2 - 1) / 2.0;

  xhsiz = (double) (sel_width - 1) / 2.0;
  yhsiz = (double) (sel_height - 1) / 2.0;

  if (xhsiz < yhsiz)
    {
      scale_x = yhsiz / xhsiz;
      scale_y = 1.0;
    }
  else if (xhsiz > yhsiz)
    {
      scale_x = 1.0;
      scale_y = xhsiz / yhsiz;
    }
  else
    {
      scale_x = 1.0;
      scale_y = 1.0;
    }

  radius = MAX(xhsiz, yhsiz);

  /* Calculate preview size */

  if (sel_width > sel_height)
    {
      pwidth  = MIN(sel_width, PREVIEW_SIZE);
      pheight = sel_height * pwidth / sel_width;
    }
  else
    {
      pheight = MIN(sel_height, PREVIEW_SIZE);
      pwidth  = sel_width * pheight / sel_height;
    }

  preview_width  = MAX(pwidth, 2); /* Min size is 2 */
  preview_height = MAX(pheight, 2);

  /* See how we will run */

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_NAME, &wpvals);

      /* Get information from the dialog */
      if (!whirl_pinch_dialog ())
	return;

      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /* Make sure all the arguments are present */
      if (nparams != 6)
	status = STATUS_CALLING_ERROR;

      if (status == STATUS_SUCCESS)
	{
	  wpvals.whirl  = param[3].data.d_float;
	  wpvals.pinch  = param[4].data.d_float;
	  wpvals.radius = param[5].data.d_float;
	}

      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_NAME, &wpvals);
      break;

    default:
      break;
    }

  /* Distort the image */
  if ((status == STATUS_SUCCESS) &&
      (gimp_drawable_is_rgb (drawable->id) ||
       gimp_drawable_is_gray (drawable->id)))
    {
      /* Set the tile cache size */
      gimp_tile_cache_ntiles (2 * (drawable->width + gimp_tile_width () - 1) /
			      gimp_tile_width ());

      /* Run! */
      whirl_pinch ();

      /* If run mode is interactive, flush displays */
      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      /* Store data */

      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data (PLUG_IN_NAME, &wpvals, sizeof (whirl_pinch_vals_t));
    }
  else if (status == STATUS_SUCCESS)
    status = STATUS_EXECUTION_ERROR;

  values[0].data.d_status = status;

  gimp_drawable_detach(drawable);
}

static void
whirl_pinch (void)
{
  GPixelRgn        dest_rgn;
  gint             progress, max_progress;
  guchar          *top_row, *bot_row;
  guchar          *top_p, *bot_p;
  gint             row, col;
  guchar           pixel[4][4];
  guchar           values[4];
  double           whirl;
  double           cx, cy;
  int              ix, iy;
  int              i;
  guchar           bg_color[4];
  pixel_fetcher_t *pft, *pfb;

#if 0
  g_printf ("Waiting... (pid %d)\n", getpid ());
  kill (getpid (), SIGSTOP);
#endif
  /* Initialize rows */
  top_row = g_malloc (img_bpp * sel_width);
  bot_row = g_malloc (img_bpp * sel_width);

  /* Initialize pixel region */
  gimp_pixel_rgn_init (&dest_rgn, drawable,
		       sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);

  pft = pixel_fetcher_new (drawable);
  pfb = pixel_fetcher_new (drawable);

  gimp_palette_get_background (&bg_color[0], &bg_color[1], &bg_color[2]);
  pixel_fetcher_set_bg_color (pft,
			      bg_color[0],
			      bg_color[1],
			      bg_color[2],
			      (img_has_alpha ? 0 : 255));
  pixel_fetcher_set_bg_color (pfb,
			      bg_color[0],
			      bg_color[1],
			      bg_color[2],
			      (img_has_alpha ? 0 : 255));

  progress     = 0;
  max_progress = sel_width * sel_height;

  gimp_progress_init ( _("Whirling and pinching..."));

  whirl   = wpvals.whirl * G_PI / 180;
  radius2 = radius * radius * wpvals.radius;

  for (row = sel_y1; row <= ((sel_y1 + sel_y2) / 2); row++)
    {
      top_p = top_row;
      bot_p = bot_row + img_bpp * (sel_width - 1);

      for (col = sel_x1; col < sel_x2; col++)
	{
	  if (calc_undistorted_coords (col, row, whirl, wpvals.pinch, &cx, &cy))
	    {
	      /* We are inside the distortion area */

	      /* Top */

	      if (cx >= 0.0)
		ix = (int) cx;
	      else
		ix = -((int) -cx + 1);

	      if (cy >= 0.0)
		iy = (int) cy;
	      else
		iy = -((int) -cy + 1);

	      pixel_fetcher_get_pixel (pft, ix,     iy,     pixel[0]);
	      pixel_fetcher_get_pixel (pft, ix + 1, iy,     pixel[1]);
	      pixel_fetcher_get_pixel (pft, ix,     iy + 1, pixel[2]);
	      pixel_fetcher_get_pixel (pft, ix + 1, iy + 1, pixel[3]);

	      for (i = 0; i < img_bpp; i++)
		{
		  values[0] = pixel[0][i];
		  values[1] = pixel[1][i];
		  values[2] = pixel[2][i];
		  values[3] = pixel[3][i];

		  *top_p++ = bilinear (cx, cy, values);
		}

	      /* Bottom */

	      cx = cen_x + (cen_x - cx);
	      cy = cen_y + (cen_y - cy);

	      if (cx >= 0.0)
		ix = (int) cx;
	      else
		ix = -((int) -cx + 1);

	      if (cy >= 0.0)
		iy = (int) cy;
	      else
		iy = -((int) -cy + 1);

	      pixel_fetcher_get_pixel (pfb, ix,     iy,     pixel[0]);
	      pixel_fetcher_get_pixel (pfb, ix + 1, iy,     pixel[1]);
	      pixel_fetcher_get_pixel (pfb, ix,     iy + 1, pixel[2]);
	      pixel_fetcher_get_pixel (pfb, ix + 1, iy + 1, pixel[3]);

	      for (i = 0; i < img_bpp; i++)
		{
		  values[0] = pixel[0][i];
		  values[1] = pixel[1][i];
		  values[2] = pixel[2][i];
		  values[3] = pixel[3][i];

		  *bot_p++ = bilinear (cx, cy, values);
		}

	      bot_p -= 2 * img_bpp; /* We move backwards! */
	    }
	  else
	    {
	      /*  We are outside the distortion area;
	       *  just copy the source pixels
	       */

	      /* Top */

	      pixel_fetcher_get_pixel (pft, col, row, pixel[0]);

	      for (i = 0; i < img_bpp; i++)
		*top_p++ = pixel[0][i];

	      /* Bottom */

	      pixel_fetcher_get_pixel (pfb,
				       (sel_x2 - 1) - (col - sel_x1),
				       (sel_y2 - 1) - (row - sel_y1),
				       pixel[0]);

	      for (i = 0; i < img_bpp; i++)
		*bot_p++ = pixel[0][i];

	      bot_p -= 2 * img_bpp; /* We move backwards! */
	    }
	}

      /* Paint rows to image */

      gimp_pixel_rgn_set_row (&dest_rgn, top_row, sel_x1, row, sel_width);
      gimp_pixel_rgn_set_row (&dest_rgn, bot_row,
			      sel_x1, (sel_y2 - 1) - (row - sel_y1), sel_width);

      /* Update progress */

      progress += sel_width * 2;
      gimp_progress_update ((double) progress / max_progress);
    }

  pixel_fetcher_destroy (pft);
  pixel_fetcher_destroy (pfb);

  g_free (top_row);
  g_free (bot_row);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, sel_x1, sel_y1, sel_width, sel_height);
}

static int
calc_undistorted_coords (double  wx,
			 double  wy,
			 double  whirl,
			 double  pinch,
			 double *x,
			 double *y)
{
  double dx, dy;
  double d, factor;
  double dist;
  double ang, sina, cosa;
  int    inside;

  /* Distances to center, scaled */

  dx = (wx - cen_x) * scale_x;
  dy = (wy - cen_y) * scale_y;

  /* Distance^2 to center of *circle* (scaled ellipse) */

  d = dx * dx + dy * dy;

  /*  If we are inside circle, then distort.
   *  Else, just return the same position
   */

  inside = (d < radius2);

  if (inside)
    {
      dist = sqrt(d / wpvals.radius) / radius;

      /* Pinch */

      factor = pow (sin (G_PI_2 * dist), -pinch);

      dx *= factor;
      dy *= factor;

      /* Whirl */

      factor = 1.0 - dist;

      ang = whirl * factor * factor;

      sina = sin (ang);
      cosa = cos (ang);

      *x = (cosa * dx - sina * dy) / scale_x + cen_x;
      *y = (sina * dx + cosa * dy) / scale_y + cen_y;
    }
  else
    {
      *x = wx;
      *y = wy;
    }

  return inside;
}

static guchar
bilinear (double  x,
	  double  y,
	  guchar *values)
{
  double m0, m1;

  x = fmod (x, 1.0);
  y = fmod (y, 1.0);

  if (x < 0.0)
    x += 1.0;

  if (y < 0.0)
    y += 1.0;

  m0 = (double) values[0] + x * ((double) values[1] - values[0]);
  m1 = (double) values[2] + x * ((double) values[3] - values[2]);

  return (guchar) (m0 + y * (m1 - m0));
}

static pixel_fetcher_t *
pixel_fetcher_new (GDrawable *drawable)
{
  pixel_fetcher_t *pf;

  pf = g_malloc (sizeof (pixel_fetcher_t));

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

static void
build_preview_source_image (void)
{
  double           left, right, bottom, top;
  double           px, py;
  double           dx, dy;
  int              x, y;
  guchar          *p;
  guchar           pixel[4];
  pixel_fetcher_t *pf;

  wpint.check_row_0 = g_new (guchar, preview_width);
  wpint.check_row_1 = g_new (guchar, preview_width);
  wpint.image       = g_new (guchar, preview_width * preview_height * 4);
  wpint.dimage      = g_new (guchar, preview_width * preview_height * 3);

  left   = sel_x1;
  right  = sel_x2 - 1;
  bottom = sel_y2 - 1;
  top    = sel_y1;

  dx = (right - left) / (preview_width - 1);
  dy = (bottom - top) / (preview_height - 1);

  py = top;

  pf = pixel_fetcher_new (drawable);

  p = wpint.image;

  for (y = 0; y < preview_height; y++)
    {
      px = left;

      for (x = 0; x < preview_width; x++)
	{
	  /* Checks */

	  if ((x / GIMP_CHECK_SIZE) & 1)
	    {
	      wpint.check_row_0[x] = GIMP_CHECK_DARK * 255;
	      wpint.check_row_1[x] = GIMP_CHECK_LIGHT * 255;
	    }
	  else
	    {
	      wpint.check_row_0[x] = GIMP_CHECK_LIGHT * 255;
	      wpint.check_row_1[x] = GIMP_CHECK_DARK * 255;
	    }

	  /* Thumbnail image */

	  pixel_fetcher_get_pixel (pf, (int) px, (int) py, pixel);

	  if (img_bpp < 3)
	    {
	      if (img_has_alpha)
		pixel[3] = pixel[1];
	      else
		pixel[3] = 255;

	      pixel[1] = pixel[0];
	      pixel[2] = pixel[0];
	    }
	  else
	    if (!img_has_alpha)
	      pixel[3] = 255;

	  *p++ = pixel[0];
	  *p++ = pixel[1];
	  *p++ = pixel[2];
	  *p++ = pixel[3];

	  px += dx;
	}

      py += dy;
    }

  pixel_fetcher_destroy (pf);
}

static gint
whirl_pinch_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *abox;
  GtkWidget *pframe;
  GtkWidget *table;
  GtkObject *adj;
  gint        argc;
  gchar     **argv;
  guchar     *color_cube;

#if 0
  g_print ("Waiting... (pid %d)\n", getpid ());
  kill (getpid (), SIGSTOP);
#endif

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("whirlpinch");

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

  build_preview_source_image ();

  dialog = gimp_dialog_new ( _("Whirl and Pinch"), "whirlpinch",
			    gimp_plugin_help_func, "filters/whirlpinch.html",
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
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox), main_vbox,
		      FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  /* Preview */
  frame = gtk_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);

  pframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (pframe), 4);
  gtk_container_add (GTK_CONTAINER (abox), pframe);
  gtk_widget_show (pframe);

  /* Preview */
  wpint.preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (wpint.preview), preview_width, preview_height);
  gtk_container_add (GTK_CONTAINER (pframe), wpint.preview);
  gtk_widget_show (wpint.preview);

  /* Controls */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Whirl Angle:"), SCALE_WIDTH, 0,
			      wpvals.whirl, -360.0, 360.0, 1.0, 15.0, 2,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (dialog_scale_update),
		      &wpvals.whirl);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Pinch Amount:"), SCALE_WIDTH, 0,
			      wpvals.pinch, -1.0, 1.0, 0.01, 0.1, 3,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (dialog_scale_update),
		      &wpvals.pinch);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Radius:"), SCALE_WIDTH, 0,
			      wpvals.radius, 0.0, 2.0, 0.01, 0.1, 3,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (dialog_scale_update),
		      &wpvals.radius);

  /* Done */

  gtk_widget_show (dialog);
  dialog_update_preview ();

  gtk_main ();
  gdk_flush ();

  g_free (wpint.check_row_0);
  g_free (wpint.check_row_1);
  g_free (wpint.image);
  g_free (wpint.dimage);

  return wpint.run;
}

static void
dialog_update_preview (void)
{
  double  left, right, bottom, top;
  double  dx, dy;
  double  px, py;
  double  cx, cy;
  int     ix, iy;
  int     x, y;
  double  whirl;
  double  scale_x, scale_y;
  guchar *p_ul, *p_lr, *i, *p;
  guchar *check_ul, *check_lr;
  int     check;
  guchar  outside[4];

  gimp_palette_get_background(&outside[0], &outside[1], &outside[2]);
  outside[3] = (img_has_alpha ? 0 : 255);

  if (img_bpp < 3)
    {
      outside[1] = outside[0];
      outside[2] = outside[0];
    }

  left   = sel_x1;
  right  = sel_x2 - 1;
  bottom = sel_y2 - 1;
  top    = sel_y1;

  dx = (right - left) / (preview_width - 1);
  dy = (bottom - top) / (preview_height - 1);

  whirl   = wpvals.whirl * G_PI / 180.0;
  radius2 = radius * radius * wpvals.radius;

  scale_x = (double) preview_width / (right - left + 1);
  scale_y = (double) preview_height / (bottom - top + 1);

  py = top;

  p_ul = wpint.dimage;
  p_lr = wpint.dimage + 3 * (preview_width * preview_height - 1);

  for (y = 0; y <= (preview_height / 2); y++)
    {
      px = left;

      if ((y / GIMP_CHECK_SIZE) & 1)
	check_ul = wpint.check_row_0;
      else
	check_ul = wpint.check_row_1;

      if (((preview_height - y - 1) / GIMP_CHECK_SIZE) & 1)
	check_lr = wpint.check_row_0;
      else
	check_lr = wpint.check_row_1;

      for (x = 0; x < preview_width; x++)
	{
	  calc_undistorted_coords (px, py, whirl, wpvals.pinch, &cx, &cy);

	  cx = (cx - left) * scale_x;
	  cy = (cy - top) * scale_y;

	  /* Upper left mirror */

	  ix = (int) (cx + 0.5);
	  iy = (int) (cy + 0.5);

	  check = check_ul[x];

	  if ((ix >= 0) && (ix < preview_width) &&
	      (iy >= 0) && (iy < preview_height))
	    i = wpint.image + 4 * (preview_width * iy + ix);
	  else
	    i = outside;

	  p_ul[0] = check + ((i[0] - check) * i[3]) / 255;
	  p_ul[1] = check + ((i[1] - check) * i[3]) / 255;
	  p_ul[2] = check + ((i[2] - check) * i[3]) / 255;

	  p_ul += 3;

	  /* Lower right mirror */

	  ix = preview_width - ix - 1;
	  iy = preview_height - iy - 1;

	  check = check_lr[preview_width - x - 1];

	  if ((ix >= 0) && (ix < preview_width) &&
	      (iy >= 0) && (iy < preview_height))
	    i = wpint.image + 4 * (preview_width * iy + ix);
	  else
	    i = outside;

	  p_lr[0] = check + ((i[0] - check) * i[3]) / 255;
	  p_lr[1] = check + ((i[1] - check) * i[3]) / 255;
	  p_lr[2] = check + ((i[2] - check) * i[3]) / 255;

	  p_lr -= 3;

	  px += dx;
	}

      py += dy;
    }

  p = wpint.dimage;

  for (y = 0; y < preview_height; y++)
    {
      gtk_preview_draw_row (GTK_PREVIEW (wpint.preview), p, 0, y, preview_width);

      p += preview_width * 3;
    }

  gtk_widget_draw (wpint.preview, NULL);
  gdk_flush ();
}

static void
dialog_scale_update (GtkAdjustment *adjustment,
		     gdouble       *value)
{
  gimp_double_adjustment_update (adjustment, value);

  dialog_update_preview ();
}

static void
dialog_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  wpint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
