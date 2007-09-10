/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Lens plug-in - adjust for lens distortion
 * Copyright (C) 2001-2005 David Hodson hodsond@acm.org
 * Many thanks for Lars Clausen for the original inspiration,
 *   useful discussion, optimisation and improvements.
 * Framework borrowed from many similar plugins...
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC     "plug-in-lens-distortion"
#define PLUG_IN_BINARY   "lens"

#define RESPONSE_RESET   1

#define LENS_MAX_PIXEL_DEPTH        4


typedef struct
{
  gdouble  centre_x;
  gdouble  centre_y;
  gdouble  square_a;
  gdouble  quad_a;
  gdouble  scale_a;
  gdouble  brighten;
} LensValues;

typedef struct
{
  gdouble  normallise_radius_sq;
  gdouble  centre_x;
  gdouble  centre_y;
  gdouble  mult_sq;
  gdouble  mult_qd;
  gdouble  rescale;
  gdouble  brighten;
} LensCalcValues;


/* Declare local functions. */

static void     query (void);
static void     run   (const gchar      *name,
                       gint              nparams,
                       const GimpParam  *param,
                       gint             *nreturn_vals,
                       GimpParam       **return_vals);

static void     lens_distort    (GimpDrawable *drawable);
static gboolean lens_dialog     (GimpDrawable *drawable);


static LensValues         vals = { 0.0, 0.0, 0.0, 0.0 };
static LensCalcValues     calc_vals;

static gint               drawable_width, drawable_height;
static guchar             background_color[4];


const GimpPlugInInfo PLUG_IN_INFO =
  {
    NULL,  /* init_proc  */
    NULL,  /* quit_proc  */
    query, /* query_proc */
    run,   /* run_proc   */
  };


MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
    {
      { GIMP_PDB_INT32,      "run-mode",    "Interactive, non-interactive" },
      { GIMP_PDB_IMAGE,      "image",       "Input image (unused)" },
      { GIMP_PDB_DRAWABLE,   "drawable",    "Input drawable" },
      { GIMP_PDB_FLOAT,      "offset-x",    "Effect centre offset in X" },
      { GIMP_PDB_FLOAT,      "offset-y",    "Effect centre offset in Y" },
      { GIMP_PDB_FLOAT,      "main-adjust",
        "Amount of second-order distortion" },
      { GIMP_PDB_FLOAT,      "edge-adjust",
        "Amount of fourth-order distortion" },
      { GIMP_PDB_FLOAT,      "rescale",     "Rescale overall image size" },
      { GIMP_PDB_FLOAT,      "brighten",    "Adjust brightness in corners" }
    };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Corrects lens distortion"),
                          "Corrects barrel or pincushion lens distortion.",
                          "David Hodson, Aurimas Juška",
                          "David Hodson",
                          "Version 1.0.10",
                          N_("Lens Distortion..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Distorts");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  gint32             image_ID;
  GimpRGB            background;
  GimpPDBStatusType  status   = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;

  run_mode = param[0].data.d_int32;
  image_ID = param[1].data.d_int32;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  INIT_I18N ();

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  drawable_width = drawable->width;
  drawable_height = drawable->height;

  /* Get background color */
  gimp_context_get_background (&background);
  gimp_rgb_set_alpha (&background, 0.0);
  gimp_drawable_get_color_uchar (drawable->drawable_id, &background,
                                 background_color);

  /* Set the tile cache size */
  gimp_tile_cache_ntiles (2 * MAX (drawable->ntile_rows, drawable->ntile_cols));

  *nreturn_vals = 1;
  *return_vals = values;

  switch (run_mode) {
  case GIMP_RUN_INTERACTIVE:
    gimp_get_data (PLUG_IN_PROC, &vals);
    if (! lens_dialog (drawable))
      return;
    break;

  case GIMP_RUN_NONINTERACTIVE:
    if (nparams != 9)
      status = GIMP_PDB_CALLING_ERROR;

    if (status == GIMP_PDB_SUCCESS)
      {
        vals.centre_x = param[3].data.d_float;
        vals.centre_y = param[4].data.d_float;
        vals.square_a = param[5].data.d_float;
        vals.quad_a = param[6].data.d_float;
        vals.scale_a = param[7].data.d_float;
        vals.brighten = param[8].data.d_float;
      }

    break;

  case GIMP_RUN_WITH_LAST_VALS:
    gimp_get_data (PLUG_IN_PROC, &vals);
    break;

  default:
    break;
  }

  if ( status == GIMP_PDB_SUCCESS )
    {
      lens_distort (drawable);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &vals, sizeof (LensValues));

      gimp_drawable_detach (drawable);
    }

  values[0].data.d_status = status;
}

static void
lens_get_source_coords (gdouble  i,
                        gdouble  j,
                        gdouble *x,
                        gdouble *y,
                        gdouble *mag)
{
  gdouble radius_sq;

  gdouble off_x;
  gdouble off_y;

  gdouble radius_mult;

  off_x = i - calc_vals.centre_x;
  off_y = j - calc_vals.centre_y;
  radius_sq = (off_x * off_x) + (off_y * off_y);

  radius_sq *= calc_vals.normallise_radius_sq;

  radius_mult = radius_sq * calc_vals.mult_sq + radius_sq * radius_sq *
    calc_vals.mult_qd;
  *mag = radius_mult;
  radius_mult = calc_vals.rescale * (1.0 + radius_mult);

  *x = calc_vals.centre_x + radius_mult * off_x;
  *y = calc_vals.centre_y + radius_mult * off_y;
}

static void
lens_setup_calc (gint width, gint height)
{
  calc_vals.normallise_radius_sq =
    4.0 / (width * width + height * height);

  calc_vals.centre_x = width * (100.0 + vals.centre_x) / 200.0;
  calc_vals.centre_y = height * (100.0 + vals.centre_y) / 200.0;
  calc_vals.mult_sq = vals.square_a / 200.0;
  calc_vals.mult_qd = vals.quad_a / 200.0;
  calc_vals.rescale = pow(2.0, - vals.scale_a / 100.0);
  calc_vals.brighten = - vals.brighten / 10.0;
}

/*
 * Catmull-Rom cubic interpolation
 *
 * equally spaced points p0, p1, p2, p3
 * interpolate 0 <= u < 1 between p1 and p2
 *
 * (1 u u^2 u^3) (  0.0  1.0  0.0  0.0 ) (p0)
 *               ( -0.5  0.0  0.5  0.0 ) (p1)
 *               (  1.0 -2.5  2.0 -0.5 ) (p2)
 *               ( -0.5  1.5 -1.5  0.5 ) (p3)
 *
 */

static void
lens_cubic_interpolate (const guchar *src,
                        gint          row_stride,
                        gint          src_depth,
                        guchar       *dst,
                        gint          dst_depth,
                        gdouble       dx,
                        gdouble       dy,
                        gdouble       brighten)
{
  gfloat um1, u, up1, up2;
  gfloat vm1, v, vp1, vp2;
  gint   c;
  gfloat verts[4 * LENS_MAX_PIXEL_DEPTH];

  um1 = ((-0.5 * dx + 1.0) * dx - 0.5) * dx;
  u = (1.5 * dx - 2.5) * dx * dx + 1.0;
  up1 = ((-1.5 * dx + 2.0) * dx + 0.5) * dx;
  up2 = (0.5 * dx - 0.5) * dx * dx;

  vm1 = ((-0.5 * dy + 1.0) * dy - 0.5) * dy;
  v = (1.5 * dy - 2.5) * dy * dy + 1.0;
  vp1 = ((-1.5 * dy + 2.0) * dy + 0.5) * dy;
  vp2 = (0.5 * dy - 0.5) * dy * dy;

  /* Note: if dst_depth < src_depth, we calculate unneeded pixels here */
  /* later - select or create index array */
  for (c = 0; c < 4 * src_depth; ++c)
    {
      verts[c] = vm1 * src[c] + v * src[c+row_stride] +
        vp1 * src[c+row_stride*2] + vp2 * src[c+row_stride*3];
    }

  for (c = 0; c < dst_depth; ++c)
    {
      gfloat result;

      result = um1 * verts[c] + u * verts[c+src_depth] +
        up1 * verts[c+src_depth*2] + up2 * verts[c+src_depth*3];

      result *= brighten;

      dst[c] = CLAMP (result, 0, 255);
    }
}

static void
lens_distort_func (gint              ix,
                   gint              iy,
                   guchar           *dest,
                   gint              bpp,
                   GimpPixelFetcher *pft)
{
  gdouble  src_x, src_y, mag;
  gdouble  brighten;
  guchar   pixel_buffer[16 * LENS_MAX_PIXEL_DEPTH];
  guchar  *pixel;
  gdouble  dx, dy;
  gint     x_int, y_int;
  gint     x, y;

  lens_get_source_coords (ix, iy, &src_x, &src_y, &mag);

  brighten = 1.0 + mag * calc_vals.brighten;
  x_int = floor (src_x);
  dx = src_x - x_int;

  y_int = floor (src_y);
  dy = src_y - y_int;

  pixel = pixel_buffer;
  for (y = y_int - 1; y <= y_int + 2; y++)
    {
      for (x = x_int -1; x <= x_int + 2; x++)
        {
          if (x >= 0  && y >= 0 &&
              x < drawable_width &&  y < drawable_height)
            {
              gimp_pixel_fetcher_get_pixel (pft, x, y, pixel);
            }
          else
            {
              gint i;

              for (i = 0; i < bpp; i++)
                pixel[i] = background_color[i];
            }

          pixel += bpp;
        }
    }

  lens_cubic_interpolate (pixel_buffer, bpp * 4, bpp,
                          dest, bpp, dx, dy, brighten);
}

static void
lens_distort (GimpDrawable *drawable)
{
  GimpRgnIterator  *iter;
  GimpPixelFetcher *pft;
  GimpRGB           background;

  lens_setup_calc (drawable->width, drawable->height);

  pft = gimp_pixel_fetcher_new (drawable, FALSE);

  gimp_context_get_background (&background);
  gimp_rgb_set_alpha (&background, 0.0);
  gimp_pixel_fetcher_set_bg_color (pft, &background);
  gimp_pixel_fetcher_set_edge_mode (pft, GIMP_PIXEL_FETCHER_EDGE_BACKGROUND);

  gimp_progress_init (_("Lens distortion"));

  iter = gimp_rgn_iterator_new (drawable, 0);
  gimp_rgn_iterator_dest (iter, (GimpRgnFuncDest) lens_distort_func, pft);
  gimp_rgn_iterator_free (iter);

  gimp_pixel_fetcher_destroy (pft);
}

static void
lens_distort_preview (GimpDrawable *drawable,
                      GimpPreview  *preview)
{
  guchar               *dest;
  guchar               *pixel;
  gint                  width, height, bpp;
  gint                  x, y;
  GimpPixelFetcher     *pft;
  GimpRGB               background;

  pft = gimp_pixel_fetcher_new (drawable, FALSE);

  gimp_context_get_background (&background);
  gimp_rgb_set_alpha (&background, 0.0);
  gimp_pixel_fetcher_set_bg_color (pft, &background);
  gimp_pixel_fetcher_set_edge_mode (pft, GIMP_PIXEL_FETCHER_EDGE_BACKGROUND);

  lens_setup_calc (drawable->width, drawable->height);

  dest = gimp_zoom_preview_get_source (GIMP_ZOOM_PREVIEW (preview),
                                       &width, &height, &bpp);
  pixel = dest;

  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
        {
          gint sx, sy;

          gimp_preview_untransform (preview, x, y, &sx, &sy);

          lens_distort_func (sx, sy, pixel, bpp, pft);

          pixel += bpp;
        }
    }

  gimp_pixel_fetcher_destroy (pft);

  gimp_preview_draw_buffer (preview, dest, width * bpp);
  g_free (dest);
}

/* UI callback functions */

static GSList *adjustments = NULL;

static void
lens_dialog_reset (void)
{
  GSList *list;

  for (list = adjustments; list; list = list->next)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (list->data), 0.0);
}

static void
lens_response (GtkWidget *widget,
               gint       response_id,
               gboolean  *run)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      lens_dialog_reset ();
      break;

    case GTK_RESPONSE_OK:
      *run = TRUE;
      /* fallthrough */

    default:
      gtk_widget_destroy (GTK_WIDGET (widget));
      break;
    }
}

static gboolean
lens_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *table;
  GtkWidget *preview;
  GtkObject *adj;
  gint       row = 0;
  gboolean   run = FALSE;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Lens Distortion"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GIMP_STOCK_RESET, RESPONSE_RESET,
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_zoom_preview_new (drawable);
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), preview);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (lens_distort_preview),
                            drawable);

  table = gtk_table_new (6, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_Main:"), 120, 6,
                              vals.square_a, -100.0, 100.0, 0.1, 1.0, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  adjustments = g_slist_append (adjustments, adj);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &vals.square_a);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_Edge:"), 120, 6,
                              vals.quad_a, -100.0, 100.0, 0.1, 1.0, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  adjustments = g_slist_append (adjustments, adj);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &vals.quad_a);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_Zoom:"), 120, 6,
                              vals.scale_a, -100.0, 100.0, 0.1, 1.0, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  adjustments = g_slist_append (adjustments, adj);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &vals.scale_a);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_Brighten:"), 120, 6,
                              vals.brighten, -100.0, 100.0, 0.1, 1.0, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  adjustments = g_slist_append (adjustments, adj);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &vals.brighten);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_X shift:"), 120, 6,
                              vals.centre_x, -100.0, 100.0, 0.1, 1.0, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  adjustments = g_slist_append (adjustments, adj);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &vals.centre_x);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_Y shift:"), 120, 6,
                              vals.centre_y, -100.0, 100.0, 0.1, 1.0, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  adjustments = g_slist_append (adjustments, adj);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &vals.centre_y);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (lens_response),
                    &run);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  gtk_widget_show (dialog);

  gtk_main ();

  g_slist_free (adjustments);
  adjustments = NULL;

  return run;
}
