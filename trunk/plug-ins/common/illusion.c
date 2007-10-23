/*
 * illusion.c  -- This is a plug-in for GIMP 1.0
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC    "plug-in-illusion"
#define PLUG_IN_BINARY  "illusion"
#define PLUG_IN_VERSION "v0.8 (May 14 2000)"


static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparam,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static void      illusion         (GimpDrawable *drawable);
static void      illusion_preview (GimpPreview  *preview,
                                   GimpDrawable *drawable);
static gboolean  illusion_dialog  (GimpDrawable *drawable);

typedef struct
{
  gint32   division;
  gboolean type1;
  gboolean type2;
} IllValues;

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

static IllValues parameters =
{
  8,     /* division */
  TRUE,  /* type 1 */
  FALSE  /* type 2 */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",  "interactive / non-interactive"    },
    { GIMP_PDB_IMAGE,    "image",     "input image"                      },
    { GIMP_PDB_DRAWABLE, "drawable",  "input drawable"                   },
    { GIMP_PDB_INT32,    "division",  "the number of divisions"          },
    { GIMP_PDB_INT32,    "type",      "illusion type (0=type1, 1=type2)" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Superimpose many altered copies of the image"),
                          "produce illusion",
                          "Hirotsuna Mizuno <s1041150@u-aizu.ac.jp>",
                          "Hirotsuna Mizuno",
                          PLUG_IN_VERSION,
                          N_("_Illusion..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Map");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *params,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   returnv[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  GimpDrawable      *drawable;

  INIT_I18N ();

  /* get the drawable info */
  run_mode = params[0].data.d_int32;
  drawable = gimp_drawable_get (params[2].data.d_drawable);

  *nreturn_vals = 1;
  *return_vals  = returnv;

  /* switch the run mode */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &parameters);
      if (! illusion_dialog (drawable))
        return;
      gimp_set_data (PLUG_IN_PROC, &parameters, sizeof (IllValues));
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
              parameters.type1 = TRUE;
              parameters.type2 = FALSE;
            }
          else
            {
              parameters.type1 = FALSE;
              parameters.type2 = TRUE;
            }
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &parameters);
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
          gimp_drawable_is_gray (drawable->drawable_id))
        {
          gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width() + 1));
          gimp_progress_init (_("Illusion"));
          illusion (drawable);
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
illusion_func (gint          x,
               gint          y,
               const guchar *src,
               guchar       *dest,
               gint          bpp,
               gpointer      data)
{
  IllusionParam_t *param = (IllusionParam_t*) data;
  gint             xx, yy, b;
  gdouble          radius, cx, cy, angle;
  guchar           pixel[4];

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

  gimp_pixel_fetcher_get_pixel (param->pft, xx, yy, pixel);

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
illusion (GimpDrawable *drawable)
{
  IllusionParam_t  param;
  GimpRgnIterator *iter;
  gint             width, height;
  gint             x1, y1, x2, y2;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
  width  = x2 - x1;
  height = y2 - y1;

  param.pft = gimp_pixel_fetcher_new (drawable, FALSE);
  gimp_pixel_fetcher_set_edge_mode (param.pft, GIMP_PIXEL_FETCHER_EDGE_SMEAR);

  param.has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  param.center_x = (x1 + x2) / 2.0;
  param.center_y = (y1 + y2) / 2.0;
  param.scale = sqrt (width * width + height * height) / 2;
  param.offset = (gint) (param.scale / 2);

  iter = gimp_rgn_iterator_new (drawable, 0);
  gimp_rgn_iterator_src_dest (iter, illusion_func, &param);
  gimp_rgn_iterator_free (iter);

  gimp_pixel_fetcher_destroy (param.pft);
}

static void
illusion_preview (GimpPreview  *preview,
                  GimpDrawable *drawable)

{
  gint                  x, y;
  gint                  sx, sy;
  gint                  preview_width, preview_height;
  guchar               *src;
  guchar               *dest;
  guchar               *src_pixel;
  guchar               *dest_pixel;
  gint                  bpp;
  IllusionParam_t       param;
  gint                  width, height;
  gint                  x1, y1, x2, y2;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
  width  = x2 - x1;
  height = y2 - y1;

  param.pft = gimp_pixel_fetcher_new (drawable, FALSE);
  gimp_pixel_fetcher_set_edge_mode (param.pft, GIMP_PIXEL_FETCHER_EDGE_SMEAR);

  param.has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  param.center_x = (x1 + x2) / 2.0;
  param.center_y = (y1 + y2) / 2.0;
  param.scale = sqrt (width * width + height * height) / 2;
  param.offset = (gint) (param.scale / 2);

  src = gimp_zoom_preview_get_source (GIMP_ZOOM_PREVIEW (preview),
                                      &preview_width, &preview_height, &bpp);
  dest = g_malloc (preview_width * preview_height * bpp);

  src_pixel = src;
  dest_pixel = dest;

  for (y = 0; y < preview_height; y++)
    {
      for (x = 0; x < preview_width; x++)
        {
          gimp_preview_untransform (preview, x, y, &sx, &sy);

          illusion_func (sx, sy,
                         src_pixel, dest_pixel,
                         bpp,
                         (gpointer) &param);

          src_pixel += bpp;
          dest_pixel += bpp;
        }
    }

  gimp_pixel_fetcher_destroy (param.pft);

  gimp_preview_draw_buffer (preview, dest, preview_width * bpp);
  g_free (dest);
  g_free (src);
}

static gboolean
illusion_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *radio;
  GSList    *group = NULL;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Illusion"), PLUG_IN_BINARY,
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

  preview = gimp_zoom_preview_new (drawable);
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), preview);
  gtk_widget_show (preview);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (illusion_preview),
                    drawable);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&adj, parameters.division,
                                     -32, 64, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("_Divisions:"), 0.0, 0.5,
                             spinbutton, 1, TRUE);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &parameters.division);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  radio = gtk_radio_button_new_with_mnemonic (group, _("Mode _1"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
  gtk_table_attach (GTK_TABLE (table), radio, 0, 2, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (radio);

  g_signal_connect (radio, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &parameters.type1);
  g_signal_connect_swapped (radio, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), parameters.type1);

  radio = gtk_radio_button_new_with_mnemonic (group, _("Mode _2"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
  gtk_table_attach (GTK_TABLE (table), radio, 0, 2, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (radio);

  g_signal_connect (radio, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &parameters.type2);
  g_signal_connect_swapped (radio, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), parameters.type2);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
