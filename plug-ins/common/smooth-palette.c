/*
 * smooth palette - derive smooth palette from image
 * Copyright (C) 1997  Scott Draves <spot@cs.cmu.edu>
 *
 * GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-smooth-palette"
#define PLUG_IN_BINARY "smooth-palette"
#define PLUG_IN_ROLE   "gimp-smooth-palette"


/* Declare local functions. */
static void      query          (void);
static void      run            (const gchar      *name,
                                 gint              nparams,
                                 const GimpParam  *param,
                                 gint             *nreturn_vals,
                                 GimpParam       **return_vals);

static gboolean  dialog         (gint32            drawable_id);

static gint32    smooth_palette (gint32            drawable_id,
                                 gint32           *layer_id);


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
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",   "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",      "Input image (unused)"         },
    { GIMP_PDB_DRAWABLE, "drawable",   "Input drawable"               },
    { GIMP_PDB_INT32,    "width",      "Width"                        },
    { GIMP_PDB_INT32,    "height",     "Height"                       },
    { GIMP_PDB_INT32,    "ntries",     "Search Depth"                 },
    { GIMP_PDB_INT32,    "show-image", "Show Image?"                  }
  };

  static const GimpParamDef return_vals[] =
  {
    { GIMP_PDB_IMAGE, "new-image", "Output image" },
    { GIMP_PDB_LAYER, "new-layer", "Output layer" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Derive a smooth color palette from the image"),
                          "help!",
                          "Scott Draves",
                          "Scott Draves",
                          "1997",
                          N_("Smoo_th Palette..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), G_N_ELEMENTS (return_vals),
                          args, return_vals);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Colors/Info");
}

static struct
{
  gint width;
  gint height;
  gint ntries;
  gint try_size;
  gint show_image;
} config =
{
  256,
  64,
  50,
  10000,
  1
};

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[3];
  GimpRunMode        run_mode;
  gint32             drawable_id;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 3;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  values[1].type          = GIMP_PDB_IMAGE;
  values[2].type          = GIMP_PDB_LAYER;

  drawable_id = param[2].data.d_drawable;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &config);
      if (! dialog (drawable_id))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 7)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          config.width      = param[3].data.d_int32;
          config.height     = param[4].data.d_int32;
          config.ntries     = param[5].data.d_int32;
          config.show_image = param[6].data.d_int32 ? TRUE : FALSE;
        }

      if (status == GIMP_PDB_SUCCESS &&
          ((config.width <= 0) || (config.height <= 0) || config.ntries <= 0))
        status = GIMP_PDB_CALLING_ERROR;

      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &config);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (gimp_drawable_is_rgb (drawable_id))
        {
          gimp_progress_init (_("Deriving smooth palette"));

          gegl_init (NULL, NULL);
          values[1].data.d_image = smooth_palette (drawable_id,
                                                   &values[2].data.d_layer);
          gegl_exit ();

          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &config, sizeof (config));

          if (config.show_image)
            gimp_display_new (values[1].data.d_image);
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;
}

static gfloat
pix_diff (gfloat  *pal,
          guint    bpp,
          gint    i,
          gint    j)
{
  gfloat r = 0.f;
  guint k;

  for (k = 0; k < bpp; k++)
    {
      gfloat p1 = pal[j * bpp + k];
      gfloat p2 = pal[i * bpp + k];
      r += (p1 - p2) * (p1 - p2);
    }

  return r;
}

static void
pix_swap (gfloat *pal,
          guint   bpp,
          gint    i,
          gint    j)
{
  guint k;

  for (k = 0; k < bpp; k++)
    {
      gfloat t = pal[j * bpp + k];
      pal[j * bpp + k] = pal[i * bpp + k];
      pal[i * bpp + k] = t;
    }
}

static gint32
smooth_palette (gint32  drawable_id,
                gint32 *layer_id)
{
  gint32        new_image_id;
  gint          psize, i, j;
  guint         bpp;
  gint          sel_x1, sel_y1;
  gint          width, height;
  GeglBuffer   *buffer;
  GeglSampler  *sampler;
  gfloat       *pal;
  GRand        *gr;

  const Babl *format = babl_format ("RGB float");

  new_image_id = gimp_image_new_with_precision (config.width,
                                                config.height,
                                                GIMP_RGB,
                                                GIMP_PRECISION_FLOAT_LINEAR);

  gimp_image_undo_disable (new_image_id);

  *layer_id = gimp_layer_new (new_image_id, _("Background"),
                              config.width, config.height,
                              gimp_drawable_type (drawable_id),
                              100,
                              gimp_image_get_default_new_layer_mode (new_image_id));

  gimp_image_insert_layer (new_image_id, *layer_id, -1, 0);

  if (! gimp_drawable_mask_intersect (drawable_id,
                                      &sel_x1, &sel_y1, &width, &height))
    return new_image_id;

  gr = g_rand_new ();

  psize = config.width;

  buffer = gimp_drawable_get_buffer (drawable_id);

  sampler = gegl_buffer_sampler_new (buffer, format, GEGL_SAMPLER_NEAREST);

  bpp = babl_format_get_n_components (gegl_buffer_get_format (buffer));

  pal = g_new (gfloat, psize * bpp);

  /* get initial palette */

  for (i = 0; i < psize; i++)
    {
      gint x = sel_x1 + g_rand_int_range (gr, 0, width);
      gint y = sel_y1 + g_rand_int_range (gr, 0, height);

      gegl_sampler_get (sampler,
                        (gdouble) x, (gdouble) y, NULL, pal + i * bpp,
                        GEGL_ABYSS_NONE);
    }

  g_object_unref (sampler);
  g_object_unref (buffer);

  /* reorder */
  if (1)
    {
      gfloat  *pal_best;
      gfloat  *original;
      gdouble  len_best = 0;
      gint     try;

      pal_best = g_memdup (pal, bpp * psize);
      original = g_memdup (pal, bpp * psize);

      for (try = 0; try < config.ntries; try++)
        {
          gdouble len;

          if (!(try%5))
            gimp_progress_update (try / (double) config.ntries);
          memcpy (pal, original, bpp * psize);

          /* scramble */
          for (i = 1; i < psize; i++)
            pix_swap (pal, bpp, i, g_rand_int_range (gr, 0, psize));

          /* measure */
          len = 0.0;
          for (i = 1; i < psize; i++)
            len += pix_diff (pal, bpp, i, i-1);

          /* improve */
          for (i = 0; i < config.try_size; i++)
            {
              gint  i0 = 1 + g_rand_int_range (gr, 0, psize-2);
              gint  i1 = 1 + g_rand_int_range (gr, 0, psize-2);
              gfloat as_is, swapd;

              if (1 == (i0 - i1))
                {
                  as_is = (pix_diff (pal, bpp, i1 - 1, i1) +
                           pix_diff (pal, bpp, i0, i0 + 1));
                  swapd = (pix_diff (pal, bpp, i1 - 1, i0) +
                           pix_diff (pal, bpp, i1, i0 + 1));
                }
              else if (1 == (i1 - i0))
                {
                  as_is = (pix_diff (pal, bpp, i0 - 1, i0) +
                           pix_diff (pal, bpp, i1, i1 + 1));
                  swapd = (pix_diff (pal, bpp, i0 - 1, i1) +
                           pix_diff (pal, bpp, i0, i1 + 1));
                }
              else
                {
                  as_is = (pix_diff (pal, bpp, i0, i0 + 1) +
                           pix_diff (pal, bpp, i0, i0 - 1) +
                           pix_diff (pal, bpp, i1, i1 + 1) +
                           pix_diff (pal, bpp, i1, i1 - 1));
                  swapd = (pix_diff (pal, bpp, i1, i0 + 1) +
                           pix_diff (pal, bpp, i1, i0 - 1) +
                           pix_diff (pal, bpp, i0, i1 + 1) +
                           pix_diff (pal, bpp, i0, i1 - 1));
                }
              if (swapd < as_is)
                {
                  pix_swap (pal, bpp, i0, i1);
                  len += swapd - as_is;
                }
            }
          /* best? */
          if (0 == try || len < len_best)
            {
              memcpy (pal_best, pal, bpp * psize);
              len_best = len;
            }
        }

      gimp_progress_update (1.0);
      memcpy (pal, pal_best, bpp * psize);
      g_free (pal_best);
      g_free (original);

      /* clean */
      for (i = 1; i < 4 * psize; i++)
        {
          gfloat as_is, swapd;
          gint i0 = 1 + g_rand_int_range (gr, 0, psize - 2);
          gint i1 = i0 + 1;

          as_is = (pix_diff (pal, bpp, i0 - 1, i0) +
                   pix_diff (pal, bpp, i1, i1 + 1));
          swapd = (pix_diff (pal, bpp, i0 - 1, i1) +
                   pix_diff (pal, bpp, i0, i1 + 1));

          if (swapd < as_is)
            {
              pix_swap (pal, bpp, i0, i1);
              len_best += swapd - as_is;
            }
        }
    }

  /* store smooth palette */

  buffer = gimp_drawable_get_buffer (*layer_id);

  for (j = 0; j < config.height; j++)
    {
      GeglRectangle row = {0, j, config.width, 1};
      gegl_buffer_set (buffer, &row, 0, format, pal, GEGL_AUTO_ROWSTRIDE);
    }

  gegl_buffer_flush (buffer);

  gimp_drawable_update (*layer_id, 0, 0,
                        config.width, config.height);
  gimp_image_undo_enable (new_image_id);

  g_object_unref (buffer);
  g_free (pal);
  g_rand_free (gr);

  return new_image_id;
}

static gboolean
dialog (gint32 drawable_id)
{
  GtkWidget     *dlg;
  GtkWidget     *spinbutton;
  GtkAdjustment *adj;
  GtkWidget     *sizeentry;
  guint32        image_id;
  GimpUnit       unit;
  gdouble        xres, yres;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dlg = gimp_dialog_new (_("Smooth Palette"), PLUG_IN_ROLE,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  image_id = gimp_item_get_image (drawable_id);
  unit = gimp_image_get_unit (image_id);
  gimp_image_get_resolution (image_id, &xres, &yres);

  sizeentry = gimp_coordinates_new (unit, "%a", TRUE, FALSE, 6,
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                    FALSE, FALSE,

                                    _("_Width:"),
                                    config.width, xres,
                                    2, GIMP_MAX_IMAGE_SIZE,
                                    2, GIMP_MAX_IMAGE_SIZE,

                                    _("_Height:"),
                                    config.height, yres,
                                    1, GIMP_MAX_IMAGE_SIZE,
                                    1, GIMP_MAX_IMAGE_SIZE);
  gtk_container_set_border_width (GTK_CONTAINER (sizeentry), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      sizeentry, FALSE, FALSE, 0);
  gtk_widget_show (sizeentry);

  adj = GTK_ADJUSTMENT (gtk_adjustment_new (config.ntries,
                                            1, 1024, 1, 10, 0));
  spinbutton = gtk_spin_button_new (adj, 1, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  gimp_grid_attach_aligned (GTK_GRID (sizeentry), 0, 2,
                            _("_Search depth:"), 0.0, 0.5,
                            spinbutton, 1);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &config.ntries);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  if (run)
    {
      config.width  = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (sizeentry),
                                                  0);
      config.height = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (sizeentry),
                                                  1);
    }

  gtk_widget_destroy (dlg);

  return run;
}
