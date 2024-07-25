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
#define PALETTE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PALETTE_TYPE, Palette))

GType                   palette_get_type         (void) G_GNUC_CONST;

static GList          * palette_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * palette_create_procedure (GimpPlugIn           *plug_in,
                                                  const gchar          *name);

static GimpValueArray * palette_run              (GimpProcedure        *procedure,
                                                  GimpRunMode           run_mode,
                                                  GimpImage            *image,
                                                  gint                  n_drawables,
                                                  GimpDrawable        **drawables,
                                                  GimpProcedureConfig  *config,
                                                  gpointer              run_data);

static gboolean         dialog                   (GimpProcedure        *procedure,
                                                  GimpProcedureConfig  *config,
                                                  GimpDrawable         *drawable);

static GimpImage      * smooth_palette           (GimpProcedureConfig  *config,
                                                  GimpDrawable         *drawable,
                                                  GimpLayer           **layer);


G_DEFINE_TYPE (Palette, palette, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PALETTE_TYPE)
DEFINE_STD_SET_I18N

#define TRY_SIZE 10000

static void
palette_class_init (PaletteClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = palette_query_procedures;
  plug_in_class->create_procedure = palette_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
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
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("Smoo_th Palette..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Colors/Info");

      gimp_procedure_set_documentation (procedure,
                                        _("Derive a smooth color palette "
                                          "from the image"),
                                        "help!",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Scott Draves",
                                      "Scott Draves",
                                      "1997");

      gimp_procedure_add_int_argument (procedure, "width",
                                       _("_Width"),
                                       _("Width"),
                                       2, GIMP_MAX_IMAGE_SIZE, 256,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "height",
                                       _("_Height"),
                                       _("Height"),
                                       2, GIMP_MAX_IMAGE_SIZE, 64,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "n-tries",
                                       _("Search _depth"),
                                       _("Search depth"),
                                       1, 1024, 50,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "show-image",
                                           _("Show image"),
                                           _("Show image"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_image_return_value (procedure, "new-image",
                                             _("New image"),
                                             _("Output image"),
                                             FALSE,
                                             G_PARAM_READWRITE);

      gimp_procedure_add_layer_return_value (procedure, "new-layer",
                                             _("New layer"),
                                             _("Output layer"),
                                             FALSE,
                                             G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
palette_run (GimpProcedure        *procedure,
             GimpRunMode           run_mode,
             GimpImage            *image,
             gint                  n_drawables,
             GimpDrawable        **drawables,
             GimpProcedureConfig  *config,
             gpointer              run_data)
{
  GimpImage    *new_image;
  GimpLayer    *new_layer;
  GimpDrawable *drawable;
  gboolean      show_image;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  if (run_mode == GIMP_RUN_INTERACTIVE && ! dialog (procedure, config, drawable))
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CANCEL,
                                               NULL);
    }

  if (gimp_drawable_is_rgb (drawable))
    {
      gimp_progress_init (_("Deriving smooth palette"));

      new_image = smooth_palette (config, drawable, &new_layer);

      g_object_get (config, "show-image", &show_image, NULL);
      if (show_image)
        gimp_display_new (new_image);
    }
  else
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
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
smooth_palette (GimpProcedureConfig  *config,
                GimpDrawable         *drawable,
                GimpLayer           **layer)
{
  GimpImage    *new_image;
  gint          psize, i, j;
  guint         bpp;
  gint          sel_x1, sel_y1;
  gint          width, height;
  gint          config_width;
  gint          config_height;
  gint          config_n_tries;
  GeglBuffer   *buffer;
  GeglSampler  *sampler;
  gfloat       *pal;
  GRand        *gr;

  const Babl *format = babl_format ("RGB float");

  g_object_get (config,
                "width",   &config_width,
                "height",  &config_height,
                "n-tries", &config_n_tries,
                NULL);

  new_image = gimp_image_new_with_precision (config_width,
                                             config_height,
                                             GIMP_RGB,
                                             GIMP_PRECISION_FLOAT_LINEAR);

  gimp_image_undo_disable (new_image);

  *layer = gimp_layer_new (new_image, _("Background"),
                           config_width, config_height,
                           gimp_drawable_type (drawable),
                           100,
                           gimp_image_get_default_new_layer_mode (new_image));

  gimp_image_insert_layer (new_image, *layer, NULL, 0);

  if (! gimp_drawable_mask_intersect (drawable,
                                      &sel_x1, &sel_y1, &width, &height))
    return new_image;

  gr = g_rand_new ();

  psize = config_width;

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

      pal_best = g_memdup2 (pal, bpp * psize);
      original = g_memdup2 (pal, bpp * psize);

      for (try = 0; try < config_n_tries; try++)
        {
          gdouble len;

          if (! (try % 5))
            gimp_progress_update (try / (double) config_n_tries);
          memcpy (pal, original, bpp * psize);

          /* scramble */
          for (i = 1; i < psize; i++)
            pix_swap (pal, bpp, i, g_rand_int_range (gr, 0, psize));

          /* measure */
          len = 0.0;
          for (i = 1; i < psize; i++)
            len += pix_diff (pal, bpp, i, i-1);

          /* improve */
          for (i = 0; i < TRY_SIZE; i++)
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

  for (j = 0; j < config_height; j++)
    {
      GeglRectangle row = {0, j, config_width, 1};
      gegl_buffer_set (buffer, &row, 0, format, pal, GEGL_AUTO_ROWSTRIDE);
    }

  gegl_buffer_flush (buffer);

  gimp_drawable_update (GIMP_DRAWABLE (*layer), 0, 0,
                        config_width, config_height);
  gimp_image_undo_enable (new_image);

  g_object_unref (buffer);
  g_free (pal);
  g_rand_free (gr);

  return new_image;
}

static gboolean
dialog (GimpProcedure       *procedure,
        GimpProcedureConfig *config,
        GimpDrawable        *drawable)
{
  GtkWidget     *dlg;
  GtkWidget     *sizeentry;
  GimpImage     *image;
  GimpUnit      *unit;
  gdouble        xres, yres;
  gint           width;
  gint           height;
  gboolean       run;

  g_object_get (config,
                "width",  &width,
                "height", &height,
                NULL);

  gimp_ui_init (PLUG_IN_BINARY);

  dlg = gimp_procedure_dialog_new (procedure,
                                   GIMP_PROCEDURE_CONFIG (config),
                                   _("Smooth Palette"));

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
                                    width, xres,
                                    2, GIMP_MAX_IMAGE_SIZE,
                                    2, GIMP_MAX_IMAGE_SIZE,

                                    _("_Height:"),
                                    height, yres,
                                    1, GIMP_MAX_IMAGE_SIZE,
                                    1, GIMP_MAX_IMAGE_SIZE);
  gtk_container_set_border_width (GTK_CONTAINER (sizeentry), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      sizeentry, FALSE, FALSE, 0);
  gtk_widget_show (sizeentry);

  /* We don't want to have the "Show Image" option in the GUI */
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dlg), "n-tries", NULL);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  if (run)
    {
      width  = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (sizeentry), 0);
      height = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (sizeentry), 1);

      g_object_set (config,
                    "width",  width,
                    "height", height,
                    NULL);
    }

  gtk_widget_destroy (dlg);

  return run;
}
