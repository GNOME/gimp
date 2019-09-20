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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-smooth-palette"
#define PLUG_IN_BINARY "smooth-palette"
#define PLUG_IN_ROLE   "gimp-smooth-palette"


typedef struct _Palette      Palette;
typedef struct _PaletteClass PaletteClass;

struct _Palette
{
  GimpPlugIn parent_instance;
};

struct _PaletteClass
{
  GimpPlugInClass parent_class;
};


#define PALETTE_TYPE  (palette_get_type ())
#define PALETTE (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PALETTE_TYPE, Palette))

GType                   palette_get_type         (void) G_GNUC_CONST;

static GList          * palette_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * palette_create_procedure (GimpPlugIn           *plug_in,
                                                  const gchar          *name);

static GimpValueArray * palette_run              (GimpProcedure        *procedure,
                                                  GimpRunMode           run_mode,
                                                  GimpImage            *image,
                                                  GimpDrawable         *drawable,
                                                  const GimpValueArray *args,
                                                  gpointer              run_data);

static gboolean         dialog                   (GimpDrawable         *drawable);

static GimpImage      * smooth_palette           (GimpDrawable         *drawable,
                                                  GimpLayer           **layer);


G_DEFINE_TYPE (Palette, palette, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PALETTE_TYPE)


static struct
{
  gint     width;
  gint     height;
  gint     ntries;
  gint     try_size;
  gboolean show_image;
} config =
{
  256,
  64,
  50,
  10000,
  TRUE
};


static void
palette_class_init (PaletteClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = palette_query_procedures;
  plug_in_class->create_procedure = palette_create_procedure;
}

static void
palette_init (Palette *palette)
{
}

static GList *
palette_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
palette_create_procedure (GimpPlugIn  *plug_in,
                          const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            palette_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*");

      gimp_procedure_set_menu_label (procedure, N_("Smoo_th Palette..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Colors/Info");

      gimp_procedure_set_documentation (procedure,
                                        N_("Derive a smooth color palette "
                                           "from the image"),
                                        "help!",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Scott Draves",
                                      "Scott Draves",
                                      "1997");

      GIMP_PROC_ARG_INT (procedure, "width",
                         "Widtg",
                         "Widtg",
                         2, GIMP_MAX_IMAGE_SIZE, 256,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "height",
                         "Height",
                         "Height",
                         2, GIMP_MAX_IMAGE_SIZE, 64,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "n-tries",
                         "N tries",
                         "Search septh",
                         1, 1024, 50,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "show-image",
                             "Show image",
                             "Show image",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_VAL_IMAGE (procedure, "new-image",
                           "New image",
                           "Output image",
                           FALSE,
                           G_PARAM_READWRITE);

      GIMP_PROC_VAL_LAYER (procedure, "new-layer",
                           "New layer",
                           "Output layer",
                           FALSE,
                           G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
palette_run (GimpProcedure        *procedure,
             GimpRunMode           run_mode,
             GimpImage            *image,
             GimpDrawable         *drawable,
             const GimpValueArray *args,
             gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *new_image;
  GimpLayer      *new_layer;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &config);

      if (! dialog (drawable))
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CANCEL,
                                                   NULL);
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      config.width      = GIMP_VALUES_GET_INT     (args, 0);
      config.height     = GIMP_VALUES_GET_INT     (args, 1);
      config.ntries     = GIMP_VALUES_GET_INT     (args, 2);
      config.show_image = GIMP_VALUES_GET_BOOLEAN (args, 3);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &config);
      break;
    }

  if (gimp_drawable_is_rgb (drawable))
    {
      gimp_progress_init (_("Deriving smooth palette"));

      new_image = smooth_palette (drawable, &new_layer);

      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &config, sizeof (config));

      if (config.show_image)
        gimp_display_new (new_image);
    }
  else
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, new_image);
  GIMP_VALUES_SET_LAYER (return_vals, 2, new_layer);

  return return_vals;
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

static GimpImage *
smooth_palette (GimpDrawable  *drawable,
                GimpLayer    **layer)
{
  GimpImage    *new_image;
  gint          psize, i, j;
  guint         bpp;
  gint          sel_x1, sel_y1;
  gint          width, height;
  GeglBuffer   *buffer;
  GeglSampler  *sampler;
  gfloat       *pal;
  GRand        *gr;

  const Babl *format = babl_format ("RGB float");

  new_image = gimp_image_new_with_precision (config.width,
                                             config.height,
                                             GIMP_RGB,
                                             GIMP_PRECISION_FLOAT_LINEAR);

  gimp_image_undo_disable (new_image);

  *layer = gimp_layer_new (new_image, _("Background"),
                           config.width, config.height,
                           gimp_drawable_type (drawable),
                           100,
                           gimp_image_get_default_new_layer_mode (new_image));

  gimp_image_insert_layer (new_image, *layer, NULL, 0);

  if (! gimp_drawable_mask_intersect (drawable,
                                      &sel_x1, &sel_y1, &width, &height))
    return new_image;

  gr = g_rand_new ();

  psize = config.width;

  buffer = gimp_drawable_get_buffer (drawable);

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

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (*layer));

  for (j = 0; j < config.height; j++)
    {
      GeglRectangle row = {0, j, config.width, 1};
      gegl_buffer_set (buffer, &row, 0, format, pal, GEGL_AUTO_ROWSTRIDE);
    }

  gegl_buffer_flush (buffer);

  gimp_drawable_update (GIMP_DRAWABLE (*layer), 0, 0,
                        config.width, config.height);
  gimp_image_undo_enable (new_image);

  g_object_unref (buffer);
  g_free (pal);
  g_rand_free (gr);

  return new_image;
}

static gboolean
dialog (GimpDrawable *drawable)
{
  GtkWidget     *dlg;
  GtkWidget     *spinbutton;
  GtkAdjustment *adj;
  GtkWidget     *sizeentry;
  GimpImage     *image;
  GimpUnit       unit;
  gdouble        xres, yres;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY);

  dlg = gimp_dialog_new (_("Smooth Palette"), PLUG_IN_ROLE,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  image = gimp_item_get_image (GIMP_ITEM (drawable));
  unit = gimp_image_get_unit (image);
  gimp_image_get_resolution (image, &xres, &yres);

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

  adj = gtk_adjustment_new (config.ntries, 1, 1024, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adj, 1, 0);
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
