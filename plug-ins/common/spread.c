/* Spread --- image filter plug-in for GIMP
 * Copyright (C) 1997 Brian Degenhardt and Federico Mena Quintero
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

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC    "plug-in-spread"
#define PLUG_IN_BINARY  "spread"
#define TILE_CACHE_SIZE 16

typedef struct
{
  gdouble  spread_amount_x;
  gdouble  spread_amount_y;
} SpreadValues;

/* Declare local functions.
 */
static void      query                 (void);
static void      run                   (const gchar      *name,
                                        gint              nparams,
                                        const GimpParam  *param,
                                        gint             *nreturn_vals,
                                        GimpParam       **return_vals);

static void      spread                (GimpDrawable     *drawable);

static void      spread_preview_update (GimpPreview      *preview,
                                        GtkWidget        *size);
static gboolean  spread_dialog         (gint32            image_ID,
                                        GimpDrawable     *drawable);

/***** Local vars *****/

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static SpreadValues spvals =
{
  5,   /*  horizontal spread amount  */
  5    /*  vertical spread amount    */
};

/***** Functions *****/

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_FLOAT,    "spread-amount-x",
      "Horizontal spread amount (0 <= spread_amount_x <= 200)" },
    { GIMP_PDB_FLOAT,    "spread-amount-y",
      "Vertical spread amount (0 <= spread_amount_y <= 200)"   }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Move pixels around randomly"),
                          "Spreads the pixels of the specified drawable.  "
                          "Pixels are randomly moved to another location whose "
                          "distance varies from the original by the horizontal "
                          "and vertical spread amounts ",
                          "Spencer Kimball and Peter Mattis, ported by Brian "
                          "Degenhardt and Federico Mena Quintero",
                          "Federico Mena Quintero and Brian Degenhardt",
                          "1997",
                          N_("Sp_read..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Noise");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  gint32             image_ID;
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  /*  Get the specified image and drawable  */
  image_ID = param[1].data.d_image;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  set the tile cache size  */
  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &spvals);

      /*  First acquire information with a dialog  */
      if (! spread_dialog (image_ID, drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          spvals.spread_amount_x= param[3].data.d_float;
          spvals.spread_amount_y = param[4].data.d_float;
        }

      if ((status == GIMP_PDB_SUCCESS) &&
          (spvals.spread_amount_x < 0 || spvals.spread_amount_x > 200) &&
          (spvals.spread_amount_y < 0 || spvals.spread_amount_y > 200))
        status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &spvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
          gimp_drawable_is_gray (drawable->drawable_id))
        {
          gimp_progress_init (_("Spreading"));

          /*  run the spread effect  */
          spread (drawable);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &spvals, sizeof (SpreadValues));
        }
      else
        {
          /* gimp_message ("spread: cannot operate on indexed color images"); */
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

typedef struct
{
  GimpPixelFetcher *pft;
  GRand            *gr;
  gint              x_amount;
  gint              y_amount;
  gint              width;
  gint              height;
} SpreadParam_t;

/* Spread the image.  This is done by going through every pixel
   in the source image and swapping it with some other random
   pixel.  The random pixel is located within an ellipse that is
   as high as the spread_amount_y parameter and as wide as the
   spread_amount_x parameter.  This is done by randomly selecting
   an angle and then multiplying the sine of the angle to a random
   number whose range is between -spread_amount_x/2 and spread_amount_x/2.
   The y coordinate is found by multiplying the cosine of the angle
   to the random value generated from spread_amount_y.  The reason
   that the spread is done this way is to make the end product more
   random looking.  To see a result of this, compare spreading a
   square with gimp 0.54 to spreading a square with this filter.
   The corners are less sharp with this algorithm.
*/

static void
spread_func (gint      x,
             gint      y,
             guchar   *dest,
             gint      bpp,
             gpointer  data)
{
  SpreadParam_t *param = (SpreadParam_t*) data;
  gdouble        angle;
  gint           xdist, ydist;
  gint           xi, yi;

  /* get random angle, x distance, and y distance */
  xdist = (param->x_amount > 0
           ? g_rand_int_range (param->gr, -param->x_amount, param->x_amount)
           : 0);
  ydist = (param->y_amount > 0
           ? g_rand_int_range (param->gr, -param->y_amount, param->y_amount)
           : 0);
  angle = g_rand_double_range (param->gr, -G_PI, G_PI);

  xi = x + floor (sin (angle) * xdist);
  yi = y + floor (cos (angle) * ydist);

  /* Only displace the pixel if it's within the bounds of the image. */
  if (xi >= 0 && xi < param->width && yi >= 0 && yi < param->height)
    {
      gimp_pixel_fetcher_get_pixel (param->pft, xi, yi, dest);
    }
  else /* Else just copy it */
    {
      gimp_pixel_fetcher_get_pixel (param->pft, x, y, dest);
    }
}

static void
spread (GimpDrawable *drawable)
{
  GimpRgnIterator *iter;
  SpreadParam_t    param;

  param.pft      = gimp_pixel_fetcher_new (drawable, FALSE);
  param.gr       = g_rand_new ();
  param.x_amount = (spvals.spread_amount_x + 1) / 2;
  param.y_amount = (spvals.spread_amount_y + 1) / 2;
  param.width    = drawable->width;
  param.height   = drawable->height;

  iter = gimp_rgn_iterator_new (drawable, 0);
  gimp_rgn_iterator_dest (iter, spread_func, &param);
  gimp_rgn_iterator_free (iter);

  g_rand_free (param.gr);
}

static void
spread_preview_update (GimpPreview *preview,
                       GtkWidget   *size)
{
  GimpDrawable   *drawable;
  SpreadParam_t   param;
  gint            x, y, bpp;
  guchar         *buffer, *dest;
  gint            x_off, y_off;
  gint            width, height;

  drawable =
    gimp_drawable_preview_get_drawable (GIMP_DRAWABLE_PREVIEW (preview));

  param.pft      = gimp_pixel_fetcher_new (drawable, FALSE);
  param.gr       = g_rand_new ();
  param.x_amount = (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size),
                                                0) + 1) / 2;
  param.y_amount = (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size),
                                                1) + 1) / 2;
  param.width    = drawable->width;
  param.height   = drawable->height;

  gimp_preview_get_size (preview, &width, &height);

  bpp = drawable->bpp;
  dest = buffer = g_new (guchar, width * height * bpp);

  gimp_preview_get_position (preview, &x_off, &y_off);

  for (y = 0 ; y < height ; y++)
    for (x = 0 ; x < width ; x++)
      {
        spread_func (x + x_off, y + y_off, dest, bpp, &param);
        dest += bpp;
      }

  gimp_preview_draw_buffer (preview, buffer, width * bpp);

  g_free (buffer);
  g_rand_free (param.gr);
}

static gboolean
spread_dialog (gint32        image_ID,
               GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *frame;
  GtkWidget *size;
  GimpUnit   unit;
  gdouble    xres;
  gdouble    yres;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Spread"), PLUG_IN_BINARY,
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

  preview = gimp_drawable_preview_new (drawable, NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  frame = gimp_frame_new (_("Spread Amount"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image_ID, &xres, &yres);
  unit = gimp_image_get_unit (image_ID);

  /* sizeentries */
  size = gimp_coordinates_new (unit, "%a", TRUE, FALSE, -1,
                               GIMP_SIZE_ENTRY_UPDATE_SIZE,

                               spvals.spread_amount_x == spvals.spread_amount_y,
                               FALSE,

                               _("_Horizontal:"), spvals.spread_amount_x, xres,
                               0, MAX (drawable->width, drawable->height),
                               0, 0,

                               _("_Vertical:"), spvals.spread_amount_y, yres,
                               0, MAX (drawable->width, drawable->height),
                               0, 0);
  gtk_container_add (GTK_CONTAINER (frame), size);
  gtk_widget_show (size);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (spread_preview_update),
                    size);
  g_signal_connect_swapped (size, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  spread_preview_update (GIMP_PREVIEW (preview), size);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      spvals.spread_amount_x =
        gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size), 0);
      spvals.spread_amount_y =
        gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size), 1);
    }

  gtk_widget_destroy (dialog);

  return run;
}
