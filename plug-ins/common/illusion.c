/*
 * illusion.c  -- This is a plug-in for the GIMP 1.0
 *
 * Copyright (C) 1997  Hirotsuna Mizuno
 *                     s1041150@u-aizu.ac.jp
 *
 * Preview and new mode added May 2000 by tim copperfield
 *                     timecop@japan.co.jp
 *                     http://www.ne.jp/asahi/linux/timecop
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_NAME    "plug_in_illusion"
#define PLUG_IN_VERSION "v0.8 (May 14 2000)"


static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparam,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static void      filter                  (GimpDrawable *drawable);
static void      filter_preview          (void);
static gboolean  dialog                  (GimpDrawable *drawable);

typedef struct
{
  gint32 division;
  gint   type1;
  gint   type2;
} IllValues;

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

static IllValues parameters =
{
  8, /* division */
  1, /* type 1 */
  0  /* type 2 */
};


static GimpOldPreview *preview;
static GimpRunMode     run_mode;


MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",  "interactive / non-interactive" },
    { GIMP_PDB_IMAGE,    "image",     "input image" },
    { GIMP_PDB_DRAWABLE, "drawable",  "input drawable" },
    { GIMP_PDB_INT32,    "division",  "the number of divisions" },
    { GIMP_PDB_INT32,    "type",      "illusion type (0=type1, 1=type2)" }
  };

  gimp_install_procedure (PLUG_IN_NAME,
                          "produce illusion",
                          "produce illusion",
                          "Hirotsuna Mizuno <s1041150@u-aizu.ac.jp>",
                          "Hirotsuna Mizuno",
                          PLUG_IN_VERSION,
                          N_("<Image>/Filters/Map/_Illusion..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *params,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpDrawable      *drawable;
  static GimpParam   returnv[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  INIT_I18N ();

  run_mode = params[0].data.d_int32;
  drawable = gimp_drawable_get (params[2].data.d_drawable);

  *nreturn_vals = 1;
  *return_vals  = returnv;

  /* get the drawable info */

  /* switch the run mode */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_NAME, &parameters);
      if (! dialog(drawable))
        return;
      gimp_set_data (PLUG_IN_NAME, &parameters, sizeof (IllValues));
      gimp_old_preview_free (preview);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 5)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          parameters.division = params[3].data.d_int32;
          if (params[4].data.d_int32 == 0)
            {
              parameters.type1 = 1;
              parameters.type2 = 0;
            }
          else
            {
              parameters.type1 = 0;
              parameters.type2 = 1;
            }
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_NAME, &parameters);
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
          gimp_drawable_is_gray (drawable->drawable_id))
        {
          gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width() + 1));
          gimp_progress_init (_("Illusion..."));
          filter (drawable);
          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  returnv[0].type          = GIMP_PDB_STATUS;
  returnv[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

typedef struct {
  GimpPixelFetcher *pft;
  gdouble           center_x;
  gdouble           center_y;
  gdouble           scale;
  gdouble           offset;
  gboolean          has_alpha;
} IllusionParam_t;

static void
illusion_func (gint x,
               gint y,
               const guchar *src,
               guchar *dest,
               gint bpp,
               gpointer data)
{
  IllusionParam_t *param = (IllusionParam_t*) data;
  gint      xx, yy, b;
  gdouble   radius, cx, cy, angle;
  guchar    pixel[4];

  cy = ((gdouble) y - param->center_y) / param->scale;
  cx = ((gdouble) x - param->center_x) / param->scale;

  angle = floor (atan2 (cy, cx) * parameters.division / G_PI_2)
    * G_PI_2 / parameters.division + (G_PI / parameters.division);
  radius = sqrt ((gdouble) (cx * cx + cy * cy));

  if (parameters.type1)
    {
      xx = x - param->offset * cos (angle);
      yy = y - param->offset * sin (angle);
    }
  else                          /* Type 2 */
    {
      xx = x - param->offset * sin (angle);
      yy = y - param->offset * cos (angle);
    }

  gimp_pixel_fetcher_get_pixel2 (param->pft, xx, yy, PIXEL_SMEAR, pixel);

  if (param->has_alpha)
    {
      guint alpha1 = src[bpp - 1];
      guint alpha2 = pixel[bpp - 1];
      guint alpha  = (1 - radius) * alpha1 + radius * alpha2;

      if ((dest[bpp - 1] = (alpha >> 1)))
        {
          for (b = 0; b < bpp - 1; b++)
            dest[b] = ((1 - radius) * src[b] * alpha1 +
                       radius * pixel[b] * alpha2) / alpha;
        }
    }
  else
    {
      for (b = 0; b < bpp; b++)
        dest[b] = (1 - radius) * src[b] + radius * pixel[b];
    }
}

static void
filter (GimpDrawable *drawable)
{
  IllusionParam_t param;
  GimpRgnIterator *iter;
  gint width, height;
  gint x1, y1, x2, y2;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
  width = x2 - x1;
  height = y2 - y1;

  param.pft = gimp_pixel_fetcher_new (drawable);
  param.has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  param.center_x = (x1 + x2) / 2.0;
  param.center_y = (y1 + y2) / 2.0;
  param.scale = sqrt (width * width + height * height) / 2;
  param.offset = (gint) (param.scale / 2);

  iter = gimp_rgn_iterator_new (drawable, run_mode);
  gimp_rgn_iterator_src_dest (iter, illusion_func, &param);
  gimp_rgn_iterator_free (iter);

  gimp_pixel_fetcher_destroy (param.pft);
}

static void
filter_preview (void)
{
  guchar  **pixels;
  guchar  **destpixels;

  gint      image_width;
  gint      image_height;
  gint      image_bpp;
  gdouble   center_x;
  gdouble   center_y;

  gint      x, y, b;
  gint      xx = 0;
  gint      yy = 0;
  gdouble   scale, radius, cx, cy, angle, offset;

  image_width  = preview->width;
  image_height = preview->height;
  image_bpp    = preview->bpp;
  center_x     = (gdouble)image_width  / 2;
  center_y     = (gdouble)image_height / 2;

  pixels     = g_new (guchar *, image_height);
  destpixels = g_new (guchar *, image_height);

  for (y = 0; y < image_height; y++)
    {
      pixels[y]     = g_new (guchar, preview->rowstride);
      destpixels[y] = g_new (guchar, preview->rowstride);
      memcpy (pixels[y],
              preview->cache + preview->rowstride * y,
              preview->rowstride);
    }

  scale  = sqrt (image_width * image_width + image_height * image_height) / 2;
  offset = (gint) (scale / 2);

  for (y = 0; y < image_height; y++)
    {
      cy = ((gdouble)y - center_y) / scale;
      for (x = 0; x < image_width; x++)
        {
          cx = ((gdouble)x - center_x) / scale;
          angle = floor (atan2 (cy, cx) * parameters.division / G_PI_2)
            * G_PI_2 / parameters.division + (G_PI / parameters.division);
          radius = sqrt ((gdouble) (cx * cx + cy * cy));

          if (parameters.type1)
            {
              xx = x - offset * cos (angle);
              yy = y - offset * sin (angle);
            }
          else                  /* Type 2 */
            {
              xx = x - offset * sin (angle);
              yy = y - offset * cos (angle);
            }

          xx = CLAMP (xx, 0, image_width - 1);
          yy = CLAMP (yy, 0, image_height - 1);

          if (image_bpp == 2 || image_bpp == 4)
            {
              gdouble alpha1 = pixels[y][x * image_bpp + image_bpp - 1];
              gdouble alpha2 = pixels[yy][xx * image_bpp + image_bpp - 1];
              gdouble alpha = (1 - radius) * alpha1 + radius * alpha2;

              for (b = 0; alpha > 0 && b < image_bpp - 1; b++)
                {
                  destpixels[y][x * image_bpp+b] =
                    ((1-radius) * alpha1 * pixels[y][x * image_bpp + b]
                     + radius * alpha2 * pixels[yy][xx * image_bpp + b])/alpha;
                }
              destpixels[y][x * image_bpp + image_bpp-1] = alpha;
            }
          else
            {
              for (b = 0; b < image_bpp; b++)
                destpixels[y][x*image_bpp+b] =
                  (1-radius) * pixels[y][x * image_bpp + b]
                  + radius * pixels[yy][xx * image_bpp + b];
            }
        }
      gimp_old_preview_do_row (preview, y, image_width, destpixels[y]);
    }

  for (y = 0; y < image_height; y++)
    g_free (pixels[y]);
  g_free (pixels);

  for (y = 0; y < image_height; y++)
    g_free (destpixels[y]);
  g_free (destpixels);

  gtk_widget_queue_draw (preview->widget);
}

static gboolean
dialog (GimpDrawable *mangle)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *radio;
  GSList    *group = NULL;
  gboolean   run;

  gimp_ui_init ("illusion", TRUE);

  dlg = gimp_dialog_new (_("Illusion"), "illusion",
                         NULL, 0,
                         gimp_standard_help_func, "filters/illusion.html",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = gimp_old_preview_new (mangle, TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview->frame, FALSE, FALSE, 0);
  filter_preview();
  gtk_widget_show (preview->widget);

  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&adj, parameters.division,
                                     -32, 64, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("_Division:"), 1.0, 0.5,
                             spinbutton, 1, TRUE);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &parameters.division);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (filter_preview),
                    NULL);

  radio = gtk_radio_button_new_with_mnemonic (group, _("Mode _1"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
  gtk_table_attach (GTK_TABLE (table), radio, 0, 2, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (radio);

  g_signal_connect (radio, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &parameters.type1);
  g_signal_connect (radio, "toggled",
                    G_CALLBACK (filter_preview),
                    NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), parameters.type1);

  radio = gtk_radio_button_new_with_mnemonic (group, _("Mode _2"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
  gtk_table_attach (GTK_TABLE (table), radio, 0, 2, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (radio);

  g_signal_connect (radio, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &parameters.type2);
  g_signal_connect (radio, "toggled",
                    G_CALLBACK (filter_preview),
                    NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), parameters.type2);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}
