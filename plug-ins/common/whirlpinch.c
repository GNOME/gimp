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

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_NAME    "plug_in_whirl_pinch"
#define PLUG_IN_VERSION "May 1997, 2.09"
#define HELP_ID         "plug-in-whirl-pinch"

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
} whirl_pinch_interface_t;

/***** Prototypes *****/

static void query (void);
static void run   (const gchar      *name,
		   gint              nparams,
		   const GimpParam  *param,
		   gint             *nreturn_vals,
		   GimpParam       **return_vals);

static void   whirl_pinch (void);
static int    calc_undistorted_coords (double wx, double wy,
				       double whirl, double pinch,
				       double *x, double *y);
static void build_preview_source_image (void);

static gint whirl_pinch_dialog    (void);
static void dialog_update_preview (void);
static void dialog_scale_update   (GtkAdjustment *adjustment, gdouble *value);


/***** Variables *****/

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,   /* quit_proc  */
  query,  /* query_proc */
  run     /* run_proc   */
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
  NULL   /* dimage */
};

static GimpDrawable *drawable;

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
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",  "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",     "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",  "Input drawable" },
    { GIMP_PDB_FLOAT,    "whirl",     "Whirl angle (degrees)" },
    { GIMP_PDB_FLOAT,    "pinch",     "Pinch amount" },
    { GIMP_PDB_FLOAT,    "radius",    "Radius (1.0 is the largest circle that fits in the image, "
      "and 2.0 goes all the way to the corners)" }
  };

  gimp_install_procedure (PLUG_IN_NAME,
			  "Distort an image by whirling and pinching",
			  "Distorts the image by whirling and pinching, which "
			  "are two common center-based, circular distortions.  "
			  "Whirling is like projecting the image onto the "
			  "surface of water in a toilet and flushing.  "
			  "Pinching is similar to projecting the image onto "
			  "an elastic surface and pressing or pulling on the "
			  "center of the surface.",
			  "Federico Mena Quintero and Scott Goehring",
			  "Federico Mena Quintero and Scott Goehring",
			  PLUG_IN_VERSION,
			  N_("<Image>/Filters/Distorts/W_hirl and Pinch..."),
			  "RGB*, GRAY*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam values[1];

  GimpRunMode run_mode;
  GimpPDBStatusType  status;
  double       xhsiz, yhsiz;
  int          pwidth, pheight;

  status   = GIMP_PDB_SUCCESS;
  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /* Get the active drawable info */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  img_width     = gimp_drawable_width (drawable->drawable_id);
  img_height    = gimp_drawable_height (drawable->drawable_id);
  img_bpp       = gimp_drawable_bpp (drawable->drawable_id);
  img_has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  gimp_drawable_mask_bounds (drawable->drawable_id,
			     &sel_x1, &sel_y1, &sel_x2, &sel_y2);

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
      pwidth  = MIN (sel_width, PREVIEW_SIZE);
      pheight = sel_height * pwidth / sel_width;
    }
  else
    {
      pheight = MIN (sel_height, PREVIEW_SIZE);
      pwidth  = sel_width * pheight / sel_height;
    }

  preview_width  = MAX (pwidth, 2); /* Min size is 2 */
  preview_height = MAX (pheight, 2);

  /* See how we will run */

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_NAME, &wpvals);

      /* Get information from the dialog */
      if (!whirl_pinch_dialog ())
	return;

      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* Make sure all the arguments are present */
      if (nparams != 6)
	{
	  status = GIMP_PDB_CALLING_ERROR;
	}
      else
	{
	  wpvals.whirl  = param[3].data.d_float;
	  wpvals.pinch  = param[4].data.d_float;
	  wpvals.radius = param[5].data.d_float;
	}

      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_NAME, &wpvals);
      break;

    default:
      break;
    }

  /* Distort the image */
  if ((status == GIMP_PDB_SUCCESS) &&
      (gimp_drawable_is_rgb (drawable->drawable_id) ||
       gimp_drawable_is_gray (drawable->drawable_id)))
    {
      /* Set the tile cache size */
      gimp_tile_cache_ntiles (2 * (drawable->width + gimp_tile_width () - 1) /
			      gimp_tile_width ());

      /* Run! */
      whirl_pinch ();

      /* If run mode is interactive, flush displays */
      if (run_mode != GIMP_RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      /* Store data */

      if (run_mode == GIMP_RUN_INTERACTIVE)
	gimp_set_data (PLUG_IN_NAME, &wpvals, sizeof (whirl_pinch_vals_t));
    }
  else if (status == GIMP_PDB_SUCCESS)
    status = GIMP_PDB_EXECUTION_ERROR;

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
whirl_pinch (void)
{
  GimpPixelRgn      dest_rgn;
  gint              progress, max_progress;
  guchar           *top_row, *bot_row;
  guchar           *top_p, *bot_p;
  gint              row, col;
  guchar          **pixel;
  gdouble           whirl;
  gdouble           cx, cy;
  gint              ix, iy;
  gint              i;
  GimpPixelFetcher *pft, *pfb;
  GimpRGB           background;

  /* Initialize rows */
  top_row = g_malloc (img_bpp * sel_width);
  bot_row = g_malloc (img_bpp * sel_width);
  pixel = g_new (guchar *, 4);
  for (i = 0; i < 4; i++)
    pixel[i] = g_new (guchar, 4);

  /* Initialize pixel region */
  gimp_pixel_rgn_init (&dest_rgn, drawable,
		       sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);

  pft = gimp_pixel_fetcher_new (drawable, FALSE);
  pfb = gimp_pixel_fetcher_new (drawable, FALSE);

  gimp_palette_get_background (&background);
  gimp_pixel_fetcher_set_bg_color (pft, &background);
  gimp_pixel_fetcher_set_bg_color (pfb, &background);

  progress     = 0;
  max_progress = sel_width * sel_height;

  gimp_progress_init (_("Whirling and Pinching..."));

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

	      gimp_pixel_fetcher_get_pixel (pft, ix,     iy,     pixel[0]);
	      gimp_pixel_fetcher_get_pixel (pft, ix + 1, iy,     pixel[1]);
	      gimp_pixel_fetcher_get_pixel (pft, ix,     iy + 1, pixel[2]);
	      gimp_pixel_fetcher_get_pixel (pft, ix + 1, iy + 1, pixel[3]);

              gimp_bilinear_pixels_8 (top_p, cx, cy, img_bpp, img_has_alpha,
                                      pixel);
              top_p += img_bpp;
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

	      gimp_pixel_fetcher_get_pixel (pfb, ix,     iy,     pixel[0]);
	      gimp_pixel_fetcher_get_pixel (pfb, ix + 1, iy,     pixel[1]);
	      gimp_pixel_fetcher_get_pixel (pfb, ix,     iy + 1, pixel[2]);
	      gimp_pixel_fetcher_get_pixel (pfb, ix + 1, iy + 1, pixel[3]);

              gimp_bilinear_pixels_8 (bot_p, cx, cy, img_bpp, img_has_alpha,
                                      pixel);
              bot_p -= img_bpp;
	    }
	  else
	    {
	      /*  We are outside the distortion area;
	       *  just copy the source pixels
	       */

	      /* Top */

	      gimp_pixel_fetcher_get_pixel (pft, col, row, pixel[0]);

	      for (i = 0; i < img_bpp; i++)
		*top_p++ = pixel[0][i];

	      /* Bottom */

	      gimp_pixel_fetcher_get_pixel (pfb,
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

  gimp_pixel_fetcher_destroy (pft);
  gimp_pixel_fetcher_destroy (pfb);

  for (i = 0; i < 4; i++)
    g_free (pixel[i]);
  g_free (pixel);
  g_free (top_row);
  g_free (bot_row);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id,
			sel_x1, sel_y1, sel_width, sel_height);
}

static gint
calc_undistorted_coords (gdouble  wx,
			 gdouble  wy,
			 gdouble  whirl,
			 gdouble  pinch,
			 gdouble *x,
			 gdouble *y)
{
  gdouble dx, dy;
  gdouble d, factor;
  gdouble dist;
  gdouble ang, sina, cosa;
  gint    inside;

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

static void
build_preview_source_image (void)
{
  gdouble          left, right, bottom, top;
  gdouble          px, py;
  gdouble          dx, dy;
  gint             x, y;
  guchar          *p;
  guchar           pixel[4];
  GimpPixelFetcher *pf;

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

  pf = gimp_pixel_fetcher_new (drawable, FALSE);

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

	  gimp_pixel_fetcher_get_pixel (pf, (int) px, (int) py, pixel);

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

  gimp_pixel_fetcher_destroy (pf);
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
  gboolean   run;

  gimp_ui_init ("whirlpinch", TRUE);

  build_preview_source_image ();

  dialog = gimp_dialog_new (_("Whirl and Pinch"), "whirlpinch",
                            NULL, 0,
			    gimp_standard_help_func, HELP_ID,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

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
  gtk_preview_size (GTK_PREVIEW (wpint.preview),
		    preview_width, preview_height);
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
			      _("_Whirl Angle:"), SCALE_WIDTH, 7,
			      wpvals.whirl, -360.0, 360.0, 1.0, 15.0, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (dialog_scale_update),
                    &wpvals.whirl);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("_Pinch Amount:"), SCALE_WIDTH, 7,
			      wpvals.pinch, -1.0, 1.0, 0.01, 0.1, 3,
			      TRUE, 0, 0,
			      NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (dialog_scale_update),
                    &wpvals.pinch);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("_Radius:"), SCALE_WIDTH, 7,
			      wpvals.radius, 0.0, 2.0, 0.01, 0.1, 3,
			      TRUE, 0, 0,
			      NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (dialog_scale_update),
                    &wpvals.radius);

  /* Done */

  gtk_widget_show (dialog);
  dialog_update_preview ();

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  g_free (wpint.check_row_0);
  g_free (wpint.check_row_1);
  g_free (wpint.image);
  g_free (wpint.dimage);

  return run;
}

static void
dialog_update_preview (void)
{
  gdouble  left, right, bottom, top;
  gdouble  dx, dy;
  gdouble  px, py;
  gdouble  cx, cy;
  gint     ix, iy;
  gint     x, y;
  gdouble  whirl;
  gdouble  scale_x, scale_y;
  guchar  *p_ul, *p_lr, *i, *p;
  guchar  *check_ul, *check_lr;
  gint     check;
  guchar   outside[4];
  GimpRGB  background;

  gimp_palette_get_background (&background);

  switch (img_bpp)
    {
    case 1:
      outside[0] = outside[1] = outside [2] = gimp_rgb_intensity_uchar (&background);
      outside[3] = 255;
      break;

    case 2:
      outside[0] = outside[1] = outside [2] = gimp_rgb_intensity_uchar (&background);
      outside[3] = 0;
      break;

    case 3:
      gimp_rgb_get_uchar (&background,
			  &outside[0], &outside[1], &outside[2]);
      outside[3] = 255;
      break;

    case 4:
      gimp_rgb_get_uchar (&background,
			  &outside[0], &outside[1], &outside[2]);
      outside[3] = 0;
      break;
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

  gtk_widget_queue_draw (wpint.preview);
}

static void
dialog_scale_update (GtkAdjustment *adjustment,
		     gdouble       *value)
{
  gimp_double_adjustment_update (adjustment, value);

  dialog_update_preview ();
}
