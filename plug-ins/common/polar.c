/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Polarize plug-in --- maps a rectangle to a circle or vice-versa
 * Copyright (C) 1997 Daniel Dunbar
 * Email: ddunbar@diads.com
 * WWW:   http://millennium.diads.com/gimp/
 * Copyright (C) 1997 Federico Mena Quintero
 * federico@nuclecu.unam.mx
 * Copyright (C) 1996 Marc Bless
 * E-mail: bless@ai-lab.fh-furtwangen.de
 * WWW:    www.ai-lab.fh-furtwangen.de/~bless
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


/* Version 1.0:
 * This is the follow-up release.  It contains a few minor changes, the
 * most major being that the first time I released the wrong version of
 * the code, and this time the changes have been fixed.  I also added
 * tooltips to the dialog.
 *
 * Feel free to email me if you have any comments or suggestions on this
 * plugin.
 *               --Daniel Dunbar
 *                 ddunbar@diads.com
 */


/* Version .5:
 * This is the first version publicly released, it will probably be the
 * last also unless i can think of some features i want to add.
 *
 * This plug-in was created with loads of help from quartic (Frederico
 * Mena Quintero), and would surely not have come about without it.
 *
 * The polar algorithms is copied from Marc Bless' polar plug-in for
 * .54, many thanks to him also.
 *
 * If you can think of a neat addition to this plug-in, or any other
 * info about it, please email me at ddunbar@diads.com.
 *                                     - Daniel Dunbar
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define WITHIN(a, b, c) ((((a) <= (b)) && ((b) <= (c))) ? 1 : 0)


/***** Magic numbers *****/

#define PLUG_IN_PROC    "plug-in-polar-coords"
#define PLUG_IN_BINARY  "polar"
#define PLUG_IN_VERSION "July 1997, 0.5"

#define SCALE_WIDTH     200
#define ENTRY_WIDTH      60

/***** Types *****/

typedef struct
{
  gdouble  circle;
  gdouble  angle;
  gboolean backwards;
  gboolean inverse;
  gboolean polrec;
} polarize_vals_t;

/***** Prototypes *****/

static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

static void     polarize                (GimpDrawable *drawable);
static gboolean calc_undistorted_coords (double        wx,
                                         double        wy,
                                         double       *x,
                                         double       *y);

static gboolean polarize_dialog         (GimpDrawable *drawable);
static void     dialog_update_preview   (GimpDrawable *drawable,
                                         GimpPreview  *preview);

/***** Variables *****/

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

static polarize_vals_t pcvals =
{
  100.0, /* circle */
  0.0,   /* angle */
  FALSE, /* backwards */
  TRUE,  /* inverse */
  TRUE   /* polar to rectangular? */
};

static gint img_width, img_height, img_has_alpha;
static gint sel_x1, sel_y1, sel_x2, sel_y2;
static gint sel_width, sel_height;

static double cen_x, cen_y;
static double scale_x, scale_y;

static guchar back_color[4];

/***** Functions *****/

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",  "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",     "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable",  "Input drawable"               },
    { GIMP_PDB_FLOAT,    "circle",    "Circle depth in %"            },
    { GIMP_PDB_FLOAT,    "angle",     "Offset angle"                 },
    { GIMP_PDB_INT32,    "backwards", "Map backwards?"               },
    { GIMP_PDB_INT32,    "inverse",   "Map from top?"                },
    { GIMP_PDB_INT32,    "polrec",    "Polar to rectangular?"        }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Convert image to or from polar coordinates"),
                          "Remaps and image from rectangular coordinates "
                          "to polar coordinates "
                          "or vice versa",
                          "Daniel Dunbar and Federico Mena Quintero",
                          "Daniel Dunbar and Federico Mena Quintero",
                          PLUG_IN_VERSION,
                          N_("P_olar Coordinates..."),
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
  static GimpParam values[1];

  GimpDrawable      *drawable;
  GimpPDBStatusType  status;
  GimpRunMode        run_mode;
  gdouble            xhsiz, yhsiz;
  GimpRGB            background;

  status   = GIMP_PDB_SUCCESS;
  run_mode = param[0].data.d_int32;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  INIT_I18N ();

  /* Get the active drawable info */

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  img_width     = gimp_drawable_width (drawable->drawable_id);
  img_height    = gimp_drawable_height (drawable->drawable_id);
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

  /* Get background color */
  gimp_context_get_background (&background);
  gimp_rgb_set_alpha (&background, 0.0);
  gimp_drawable_get_color_uchar (drawable->drawable_id, &background,
                                 back_color);

  /* See how we will run */

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_PROC, &pcvals);

      /* Get information from the dialog */
      if (! polarize_dialog (drawable))
        return;

      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* Make sure all the arguments are present */
      if (nparams != 8)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          pcvals.circle  = param[3].data.d_float;
          pcvals.angle  = param[4].data.d_float;
          pcvals.backwards  = param[5].data.d_int32;
          pcvals.inverse  = param[6].data.d_int32;
          pcvals.polrec  = param[7].data.d_int32;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_PROC, &pcvals);
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
      gimp_tile_cache_ntiles (2 * (drawable->width + gimp_tile_width() - 1) /
                              gimp_tile_width ());

      /* Run! */
      polarize (drawable);

      /* If run mode is interactive, flush displays */
      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /* Store data */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &pcvals, sizeof (polarize_vals_t));
    }
  else if (status == GIMP_PDB_SUCCESS)
    status = GIMP_PDB_EXECUTION_ERROR;

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
polarize_func (gint x,
               gint y,
               guchar *dest,
               gint bpp,
               gpointer data)
{
  double     cx, cy;

  if (calc_undistorted_coords (x, y, &cx, &cy))
    {
      guchar     pixel1[4], pixel2[4], pixel3[4], pixel4[4];
      guchar     *values[4];
      GimpPixelFetcher *pft = (GimpPixelFetcher*) data;

      values[0] = pixel1;
      values[1] = pixel2;
      values[2] = pixel3;
      values[3] = pixel4;

      gimp_pixel_fetcher_get_pixel (pft, cx, cy, pixel1);
      gimp_pixel_fetcher_get_pixel (pft, cx + 1, cy, pixel2);
      gimp_pixel_fetcher_get_pixel (pft, cx, cy + 1, pixel3);
      gimp_pixel_fetcher_get_pixel (pft, cx + 1, cy + 1, pixel4);

      gimp_bilinear_pixels_8 (dest, cx, cy, bpp, img_has_alpha, values);
    }
  else
    {
      gint b;
      for (b = 0; b < bpp; b++)
        {
          dest[b] = back_color[b];
        }
    }
}

static void
polarize (GimpDrawable *drawable)
{
  GimpRgnIterator  *iter;
  GimpPixelFetcher *pft;
  GimpRGB           background;

  pft = gimp_pixel_fetcher_new (drawable, FALSE);

  gimp_context_get_background (&background);
  gimp_rgb_set_alpha (&background, 0.0);
  gimp_pixel_fetcher_set_bg_color (pft, &background);
  gimp_pixel_fetcher_set_edge_mode (pft, GIMP_PIXEL_FETCHER_EDGE_SMEAR);

  gimp_progress_init (_("Polar coordinates"));

  iter = gimp_rgn_iterator_new (drawable, 0);
  gimp_rgn_iterator_dest (iter, polarize_func, pft);
  gimp_rgn_iterator_free (iter);

  gimp_pixel_fetcher_destroy (pft);
}

static gboolean
calc_undistorted_coords (gdouble  wx,
                         gdouble  wy,
                         gdouble *x,
                         gdouble *y)
{
  gboolean inside;
  gdouble  phi, phi2;
  gdouble  xx, xm, ym, yy;
  gint     xdiff, ydiff;
  gdouble  r;
  gdouble  m;
  gdouble  xmax, ymax, rmax;
  gdouble  x_calc, y_calc;
  gdouble  xi, yi;
  gdouble  circle, angl, t, angle;
  gint     x1, x2, y1, y2;

  /* initialize */

  phi = 0.0;
  r   = 0.0;

  x1     = 0;
  y1     = 0;
  x2     = img_width;
  y2     = img_height;
  xdiff  = x2 - x1;
  ydiff  = y2 - y1;
  xm     = xdiff / 2.0;
  ym     = ydiff / 2.0;
  circle = pcvals.circle;
  angle  = pcvals.angle;
  angl   = (gdouble) angle / 180.0 * G_PI;

  if (pcvals.polrec)
    {
      if (wx >= cen_x)
        {
          if (wy > cen_y)
            {
              phi = G_PI - atan (((double)(wx - cen_x))/
                                 ((double)(wy - cen_y)));
            }
          else if (wy < cen_y)
            {
              phi = atan (((double)(wx - cen_x))/((double)(cen_y - wy)));
            }
          else
            {
              phi = G_PI / 2;
            }
        }
      else if (wx < cen_x)
        {
          if (wy < cen_y)
            {
              phi = 2 * G_PI - atan (((double)(cen_x -wx)) /
                                     ((double)(cen_y - wy)));
            }
          else if (wy > cen_y)
            {
              phi = G_PI + atan (((double)(cen_x - wx))/
                                 ((double)(wy - cen_y)));
            }
          else
            {
              phi = 1.5 * G_PI;
            }
        }

      r   = sqrt (SQR (wx - cen_x) + SQR (wy - cen_y));

      if (wx != cen_x)
        {
          m = fabs (((double)(wy - cen_y)) / ((double)(wx - cen_x)));
        }
      else
        {
          m = 0;
        }

      if (m <= ((double)(y2 - y1) / (double)(x2 - x1)))
        {
          if (wx == cen_x)
            {
              xmax = 0;
              ymax = cen_y - y1;
            }
          else
            {
              xmax = cen_x - x1;
              ymax = m * xmax;
            }
        }
      else
        {
          ymax = cen_y - y1;
          xmax = ymax / m;
        }

      rmax = sqrt ( (double)(SQR (xmax) + SQR (ymax)) );

      t = ((cen_y - y1) < (cen_x - x1)) ? (cen_y - y1) : (cen_x - x1);
      rmax = (rmax - t) / 100 * (100 - circle) + t;

      phi = fmod (phi + angl, 2*G_PI);

      if (pcvals.backwards)
        x_calc = x2 - 1 - (x2 - x1 - 1)/(2*G_PI) * phi;
      else
        x_calc = (x2 - x1 - 1)/(2*G_PI) * phi + x1;

      if (pcvals.inverse)
        y_calc = (y2 - y1)/rmax   * r   + y1;
      else
        y_calc = y2 - (y2 - y1)/rmax * r;
    }
  else
    {
      if (pcvals.backwards)
        phi = (2 * G_PI) * (x2 - wx) / xdiff;
      else
        phi = (2 * G_PI) * (wx - x1) / xdiff;

      phi = fmod (phi + angl, 2 * G_PI);

      if (phi >= 1.5 * G_PI)
        phi2 = 2 * G_PI - phi;
      else if (phi >= G_PI)
        phi2 = phi - G_PI;
      else if (phi >= 0.5 * G_PI)
        phi2 = G_PI - phi;
      else
        phi2 = phi;

      xx = tan (phi2);
      if (xx != 0)
        m = (double) 1.0 / xx;
      else
        m = 0;

      if (m <= ((double)(ydiff) / (double)(xdiff)))
        {
          if (phi2 == 0)
            {
              xmax = 0;
              ymax = ym - y1;
            }
          else
            {
              xmax = xm - x1;
              ymax = m * xmax;
            }
        }
      else
        {
          ymax = ym - y1;
          xmax = ymax / m;
        }

      rmax = sqrt ((double)(SQR (xmax) + SQR (ymax)));

      t = ((ym - y1) < (xm - x1)) ? (ym - y1) : (xm - x1);

      rmax = (rmax - t) / 100.0 * (100 - circle) + t;

      if (pcvals.inverse)
        r = rmax * (double)((wy - y1) / (double)(ydiff));
      else
        r = rmax * (double)((y2 - wy) / (double)(ydiff));

      xx = r * sin (phi2);
      yy = r * cos (phi2);

      if (phi >= 1.5 * G_PI)
        {
          x_calc = (double)xm - xx;
          y_calc = (double)ym - yy;
        }
      else if (phi >= G_PI)
        {
          x_calc = (double)xm - xx;
          y_calc = (double)ym + yy;
        }
      else if (phi >= 0.5 * G_PI)
        {
          x_calc = (double)xm + xx;
          y_calc = (double)ym + yy;
        }
      else
        {
          x_calc = (double)xm + xx;
          y_calc = (double)ym - yy;
        }
    }

  xi = (int) (x_calc + 0.5);
  yi = (int) (y_calc + 0.5);

  inside = (WITHIN (0, xi, img_width - 1) && WITHIN (0, yi, img_height - 1));
  if (inside)
    {
      *x = x_calc;
      *y = y_calc;
    }
  return inside;
}

static gboolean
polarize_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *table;
  GtkWidget *hbox;
  GtkWidget *toggle;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Polar Coordinates"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  /* Preview */
  preview = gimp_zoom_preview_new (drawable);
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), preview);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (dialog_update_preview),
                            drawable);

  /* Controls */

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("Circle _depth in percent:"), SCALE_WIDTH, 0,
                              pcvals.circle, 0.0, 100.0, 1.0, 10.0, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pcvals.circle);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("Offset _angle:"), SCALE_WIDTH, 0,
                              pcvals.angle, 0.0, 359.0, 1.0, 15.0, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pcvals.angle);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* togglebuttons for backwards, top, polar->rectangular */
  hbox = gtk_hbox_new (TRUE, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  toggle = gtk_check_button_new_with_mnemonic (_("_Map backwards"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), pcvals.backwards);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
                           _("If checked the mapping will begin at the right "
                             "side, as opposed to beginning at the left."),
                           NULL);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &pcvals.backwards);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  toggle = gtk_check_button_new_with_mnemonic (_("Map from _top"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), pcvals.inverse);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
                           _("If unchecked the mapping will put the bottom "
                             "row in the middle and the top row on the "
                             "outside.  If checked it will be the opposite."),
                           NULL);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &pcvals.inverse);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  toggle = gtk_check_button_new_with_mnemonic (_("To _polar"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), pcvals.polrec);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
                           _("If unchecked the image will be circularly "
                             "mapped onto a rectangle.  If checked the image "
                             "will be mapped onto a circle."),
                           NULL);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &pcvals.polrec);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (hbox);

  /* Done */

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
dialog_update_preview (GimpDrawable *drawable,
                       GimpPreview  *preview)
{
  gdouble  left, right, bottom, top;
  gdouble  dx, dy;
  gdouble  px, py;
  gdouble  cx = 0.0, cy = 0.0;
  gint     ix, iy;
  gint     x, y;
  gint     width, height;
  gint     bpp;
  gdouble  scale_x, scale_y;
  guchar  *p_ul, *i;
  GimpRGB  background;
  guchar   outside[4];
  guchar  *buffer, *preview_cache;
  guchar   k;

  gimp_context_get_background (&background);
  gimp_rgb_set_alpha (&background, 0.0);
  gimp_drawable_get_color_uchar (drawable->drawable_id, &background, outside);

  left   = sel_x1;
  right  = sel_x2 - 1;
  bottom = sel_y2 - 1;
  top    = sel_y1;

  preview_cache = gimp_zoom_preview_get_source (GIMP_ZOOM_PREVIEW (preview),
                                                &width, &height, &bpp);
  dx = (right - left) / (width - 1);
  dy = (bottom - top) / (height - 1);

  scale_x = (double) width / (right - left + 1);
  scale_y = (double) height / (bottom - top + 1);

  py = top;

  buffer = g_new (guchar, bpp * width * height);
  p_ul = buffer;

  for (y = 0; y < height; y++)
    {
      px = left;

      for (x = 0; x < width; x++)
        {
          calc_undistorted_coords (px, py, &cx, &cy);

          cx = (cx - left) * scale_x;
          cy = (cy - top) * scale_y;

          ix = (int) (cx + 0.5);
          iy = (int) (cy + 0.5);

          if ((ix >= 0) && (ix < width) &&
              (iy >= 0) && (iy < height))
            i = preview_cache + bpp * (width * iy + ix);
          else
            i = outside;

          for (k = 0; k < bpp; k ++)
            p_ul[k] = i[k];

          p_ul += bpp;

          px += dx;
        }

      py += dy;
    }

  gimp_preview_draw_buffer (preview, buffer, bpp * width);

  g_free (buffer);
  g_free (preview_cache);
}
