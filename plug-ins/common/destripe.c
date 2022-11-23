/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Destripe filter
 *
 * Copyright 1997 Marc Lehmann, heavily modified from a filter by
 * Michael Sweet.
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
 *
 */

#include "config.h"

#include <string.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define PLUG_IN_PROC    "plug-in-destripe"
#define PLUG_IN_BINARY  "destripe"
#define PLUG_IN_ROLE    "ligma-destripe"
#define PLUG_IN_VERSION "0.2"
#define MAX_AVG         100


typedef struct
{
  gboolean histogram;
  gint     avg_width;
  gboolean preview;
} DestripeValues;


typedef struct _Destripe      Destripe;
typedef struct _DestripeClass DestripeClass;

struct _Destripe
{
  LigmaPlugIn parent_instance;
};

struct _DestripeClass
{
  LigmaPlugInClass parent_class;
};


#define DESTRIPE_TYPE  (destripe_get_type ())
#define DESTRIPE (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DESTRIPE_TYPE, Destripe))

GType                   destripe_get_type         (void) G_GNUC_CONST;

static GList          * destripe_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * destripe_create_procedure (LigmaPlugIn           *plug_in,
                                                   const gchar          *name);

static LigmaValueArray * destripe_run              (LigmaProcedure        *procedure,
                                                   LigmaRunMode           run_mode,
                                                   LigmaImage            *image,
                                                   gint                  n_drawables,
                                                   LigmaDrawable        **drawables,
                                                   const LigmaValueArray *args,
                                                   gpointer              run_data);

static void             destripe                  (LigmaDrawable         *drawable,
                                                   LigmaPreview          *preview);
static void             destripe_preview          (LigmaDrawable         *drawable,
                                                   LigmaPreview          *preview);

static gboolean         destripe_dialog           (LigmaDrawable         *drawable);
static void       destripe_scale_entry_update_int (LigmaLabelSpin        *entry,
                                                   gint                 *value);


G_DEFINE_TYPE (Destripe, destripe, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (DESTRIPE_TYPE)
DEFINE_STD_SET_I18N


static DestripeValues vals =
{
  FALSE, /* histogram     */
  36,    /* average width */
  TRUE   /* preview */
};


static void
destripe_class_init (DestripeClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = destripe_query_procedures;
  plug_in_class->create_procedure = destripe_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
destripe_init (Destripe *destripe)
{
}

static GList *
destripe_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
destripe_create_procedure (LigmaPlugIn  *plug_in,
                               const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            destripe_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "RGB*, GRAY*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_menu_label (procedure, _("Des_tripe..."));
      ligma_procedure_add_menu_path (procedure, "<Image>/Colors/Tone Mapping");

      ligma_procedure_set_documentation (procedure,
                                        _("Remove vertical stripe artifacts "
                                          "from the image"),
                                        "This plug-in tries to remove vertical "
                                        "stripes from an image.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Marc Lehmann <pcg@goof.com>",
                                      "Marc Lehmann <pcg@goof.com>",
                                      PLUG_IN_VERSION);

      LIGMA_PROC_ARG_INT (procedure, "avg-width",
                         "Avg width",
                         "Averaging filter width",
                         2, MAX_AVG, 36,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
destripe_run (LigmaProcedure        *procedure,
              LigmaRunMode           run_mode,
              LigmaImage            *image,
              gint                  n_drawables,
              LigmaDrawable        **drawables,
              const LigmaValueArray *args,
              gpointer              run_data)
{
  LigmaDrawable *drawable;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, LIGMA_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
      ligma_get_data (PLUG_IN_PROC, &vals);

      if (! destripe_dialog (drawable))
        {
          return ligma_procedure_new_return_values (procedure,
                                                   LIGMA_PDB_CANCEL,
                                                   NULL);
        }
      break;

    case LIGMA_RUN_NONINTERACTIVE:
      vals.avg_width = LIGMA_VALUES_GET_INT (args, 0);
      break;

    case LIGMA_RUN_WITH_LAST_VALS :
      ligma_get_data (PLUG_IN_PROC, &vals);
      break;
    };

  if (ligma_drawable_is_rgb  (drawable) ||
      ligma_drawable_is_gray (drawable))
    {
      destripe (drawable, NULL);

      if (run_mode != LIGMA_RUN_NONINTERACTIVE)
        ligma_displays_flush ();

      if (run_mode == LIGMA_RUN_INTERACTIVE)
        ligma_set_data (PLUG_IN_PROC, &vals, sizeof (vals));
    }
  else
    {
      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}

static void
destripe (LigmaDrawable *drawable,
          LigmaPreview *preview)
{
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *format;
  guchar     *src_rows;       /* image data */
  gdouble     progress, progress_inc;
  gint        x1, x2, y1;
  gint        width, height;
  gint        bpp;
  glong      *hist, *corr;        /* "histogram" data */
  gint        tile_width = ligma_tile_width ();
  gint        i, x, y, ox, cols;

  progress     = 0.0;
  progress_inc = 0.0;

  if (preview)
    {
      ligma_preview_get_position (preview, &x1, &y1);
      ligma_preview_get_size (preview, &width, &height);
    }
  else
    {
      ligma_progress_init (_("Destriping"));

      if (! ligma_drawable_mask_intersect (drawable,
                                          &x1, &y1, &width, &height))
        {
          return;
        }

      progress = 0;
      progress_inc = 0.5 * tile_width / width;
    }

  x2 = x1 + width;

  if (ligma_drawable_is_rgb (drawable))
    {
      if (ligma_drawable_has_alpha (drawable))
        format = babl_format ("R'G'B'A u8");
      else
        format = babl_format ("R'G'B' u8");
    }
  else
    {
      if (ligma_drawable_has_alpha (drawable))
        format = babl_format ("Y'A u8");
      else
        format = babl_format ("Y' u8");
    }

  bpp = babl_format_get_bytes_per_pixel (format);

  /*
   * Setup for filter...
   */

  src_buffer  = ligma_drawable_get_buffer (drawable);
  dest_buffer = ligma_drawable_get_shadow_buffer (drawable);

  hist = g_new (long, width * bpp);
  corr = g_new (long, width * bpp);
  src_rows = g_new (guchar, tile_width * height * bpp);

  memset (hist, 0, width * bpp * sizeof (long));

  /*
   * collect "histogram" data.
   */

  for (ox = x1; ox < x2; ox += tile_width)
    {
      guchar *rows = src_rows;

      cols = x2 - ox;
      if (cols > tile_width)
        cols = tile_width;

      gegl_buffer_get (src_buffer, GEGL_RECTANGLE (ox, y1, cols, height), 1.0,
                       format, rows,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      for (y = 0; y < height; y++)
        {
          long   *h       = hist + (ox - x1) * bpp;
          guchar *row_end = rows + cols * bpp;

          while (rows < row_end)
            *h++ += *rows++;
        }

      if (! preview)
        ligma_progress_update (progress += progress_inc);
    }

  /*
   * average out histogram
   */

  {
    gint extend = (vals.avg_width / 2) * bpp;

    for (i = 0; i < MIN (3, bpp); i++)
      {
        long *h   = hist - extend + i;
        long *c   = corr - extend + i;
        long  sum = 0;
        gint  cnt = 0;

        for (x = -extend; x < width * bpp; x += bpp)
          {
            if (x + extend < width * bpp)
              {
                sum += h[ extend]; cnt++;
              }

            if (x - extend >= 0)
              {
                sum -= h[-extend]; cnt--;
              }

            if (x >= 0)
              {
                if (*h)
                  *c = ((sum / cnt - *h) << 10) / *h;
                else
                  *c = G_MAXINT;
              }

            h += bpp;
            c += bpp;
          }
      }
  }

  /*
   * remove stripes.
   */

  for (ox = x1; ox < x2; ox += tile_width)
    {
      guchar *rows = src_rows;

      cols = x2 - ox;
      if (cols > tile_width)
        cols = tile_width;

      gegl_buffer_get (src_buffer, GEGL_RECTANGLE (ox, y1, cols, height), 1.0,
                       format, rows,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (! preview)
        ligma_progress_update (progress += progress_inc);

      for (y = 0; y < height; y++)
        {
          long   *c = corr + (ox - x1) * bpp;
          guchar *row_end = rows + cols * bpp;

          if (vals.histogram)
            {
              while (rows < row_end)
                {
                  *rows = MIN (255, MAX (0, 128 + (*rows * *c >> 10)));
                  c++; rows++;
                }
            }
          else
            {
              while (rows < row_end)
                {
                  *rows = MIN (255, MAX (0, *rows + (*rows * *c >> 10) ));
                  c++; rows++;
                }
            }
        }

      gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (ox, y1, cols, height), 0,
                       format, src_rows,
                       GEGL_AUTO_ROWSTRIDE);

      if (! preview)
        ligma_progress_update (progress += progress_inc);
    }

  g_free (src_rows);

  g_object_unref (src_buffer);

  if (preview)
    {
      guchar *buffer = g_new (guchar, width * height * bpp);

      gegl_buffer_get (dest_buffer, GEGL_RECTANGLE (x1, y1, width, height), 1.0,
                       format, buffer,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      ligma_preview_draw_buffer (LIGMA_PREVIEW (preview),
                                buffer, width * bpp);

      g_free (buffer);
      g_object_unref (dest_buffer);
    }
  else
    {
      g_object_unref (dest_buffer);

      ligma_progress_update (1.0);

      ligma_drawable_merge_shadow (drawable, TRUE);
      ligma_drawable_update (drawable,
                            x1, y1, width, height);
    }

  g_free (hist);
  g_free (corr);
}

static void
destripe_preview (LigmaDrawable *drawable,
                  LigmaPreview *preview)
{
  destripe (drawable, preview);
}


static gboolean
destripe_dialog (LigmaDrawable *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *preview;
  GtkWidget     *scale;
  GtkWidget     *button;
  gboolean       run;

  ligma_ui_init (PLUG_IN_BINARY);

  dialog = ligma_dialog_new (_("Destripe"), PLUG_IN_ROLE,
                            NULL, 0,
                            ligma_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  ligma_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = ligma_drawable_preview_new_from_drawable (drawable);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (destripe_preview),
                            drawable);

  scale = ligma_scale_entry_new (_("_Width:"), vals.avg_width, 2, MAX_AVG, 0);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (destripe_scale_entry_update_int),
                    &vals.avg_width);
  g_signal_connect_swapped (scale, "value-changed",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);
  gtk_box_pack_start (GTK_BOX (main_vbox), scale, FALSE, FALSE, 6);
  gtk_widget_show (scale);

  button = gtk_check_button_new_with_mnemonic (_("Create _histogram"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), vals.histogram);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &vals.histogram);
  g_signal_connect_swapped (button, "toggled",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
destripe_scale_entry_update_int (LigmaLabelSpin *entry,
                                 gint          *value)
{
  *value = (gint) ligma_label_spin_get_value (entry);
}
