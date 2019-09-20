/* GIMP - The GNU Image Manipulation Program
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC    "plug-in-destripe"
#define PLUG_IN_BINARY  "destripe"
#define PLUG_IN_ROLE    "gimp-destripe"
#define PLUG_IN_VERSION "0.2"
#define SCALE_WIDTH     140
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
  GimpPlugIn parent_instance;
};

struct _DestripeClass
{
  GimpPlugInClass parent_class;
};


#define DESTRIPE_TYPE  (destripe_get_type ())
#define DESTRIPE (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DESTRIPE_TYPE, Destripe))

GType                   destripe_get_type         (void) G_GNUC_CONST;

static GList          * destripe_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * destripe_create_procedure (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * destripe_run              (GimpProcedure        *procedure,
                                                   GimpRunMode           run_mode,
                                                   GimpImage            *image,
                                                   GimpDrawable         *drawable,
                                                   const GimpValueArray *args,
                                                   gpointer              run_data);

static void             destripe                  (GimpDrawable         *drawable,
                                                   GimpPreview          *preview);
static void             destripe_preview          (GimpDrawable         *drawable,
                                                   GimpPreview          *preview);

static gboolean         destripe_dialog           (GimpDrawable         *drawable);


G_DEFINE_TYPE (Destripe, destripe, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (DESTRIPE_TYPE)


static DestripeValues vals =
{
  FALSE, /* histogram     */
  36,    /* average width */
  TRUE   /* preview */
};


static void
destripe_class_init (DestripeClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = destripe_query_procedures;
  plug_in_class->create_procedure = destripe_create_procedure;
}

static void
destripe_init (Destripe *destripe)
{
}

static GList *
destripe_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
destripe_create_procedure (GimpPlugIn  *plug_in,
                               const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            destripe_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");

      gimp_procedure_set_menu_label (procedure, N_("Des_tripe..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Colors/Tone Mapping");

      gimp_procedure_set_documentation (procedure,
                                        N_("Remove vertical stripe artifacts "
                                           "from the image"),
                                        "This plug-in tries to remove vertical "
                                        "stripes from an image.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Marc Lehmann <pcg@goof.com>",
                                      "Marc Lehmann <pcg@goof.com>",
                                      PLUG_IN_VERSION);

      GIMP_PROC_ARG_INT (procedure, "avg-width",
                         "Avg width",
                         "Averaging filter width",
                         2, MAX_AVG, 36,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
destripe_run (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *image,
              GimpDrawable         *drawable,
              const GimpValueArray *args,
              gpointer              run_data)
{
  INIT_I18N ();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &vals);

      if (! destripe_dialog (drawable))
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CANCEL,
                                                   NULL);
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      vals.avg_width = GIMP_VALUES_GET_INT (args, 0);
      break;

    case GIMP_RUN_WITH_LAST_VALS :
      gimp_get_data (PLUG_IN_PROC, &vals);
      break;
    };

  if (gimp_drawable_is_rgb  (drawable) ||
      gimp_drawable_is_gray (drawable))
    {
      destripe (drawable, NULL);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &vals, sizeof (vals));
    }
  else
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static void
destripe (GimpDrawable *drawable,
          GimpPreview *preview)
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
  gint        tile_width = gimp_tile_width ();
  gint        i, x, y, ox, cols;

  progress     = 0.0;
  progress_inc = 0.0;

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);
    }
  else
    {
      gimp_progress_init (_("Destriping"));

      if (! gimp_drawable_mask_intersect (drawable,
                                          &x1, &y1, &width, &height))
        {
          return;
        }

      progress = 0;
      progress_inc = 0.5 * tile_width / width;
    }

  x2 = x1 + width;

  if (gimp_drawable_is_rgb (drawable))
    {
      if (gimp_drawable_has_alpha (drawable))
        format = babl_format ("R'G'B'A u8");
      else
        format = babl_format ("R'G'B' u8");
    }
  else
    {
      if (gimp_drawable_has_alpha (drawable))
        format = babl_format ("Y'A u8");
      else
        format = babl_format ("Y' u8");
    }

  bpp = babl_format_get_bytes_per_pixel (format);

  /*
   * Setup for filter...
   */

  src_buffer  = gimp_drawable_get_buffer (drawable);
  dest_buffer = gimp_drawable_get_shadow_buffer (drawable);

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
        gimp_progress_update (progress += progress_inc);
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
        gimp_progress_update (progress += progress_inc);

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
        gimp_progress_update (progress += progress_inc);
    }

  g_free (src_rows);

  g_object_unref (src_buffer);

  if (preview)
    {
      guchar *buffer = g_new (guchar, width * height * bpp);

      gegl_buffer_get (dest_buffer, GEGL_RECTANGLE (x1, y1, width, height), 1.0,
                       format, buffer,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      gimp_preview_draw_buffer (GIMP_PREVIEW (preview),
                                buffer, width * bpp);

      g_free (buffer);
      g_object_unref (dest_buffer);
    }
  else
    {
      g_object_unref (dest_buffer);

      gimp_progress_update (1.0);

      gimp_drawable_merge_shadow (drawable, TRUE);
      gimp_drawable_update (drawable,
                            x1, y1, width, height);
    }

  g_free (hist);
  g_free (corr);
}

static void
destripe_preview (GimpDrawable *drawable,
                  GimpPreview *preview)
{
  destripe (drawable, preview);
}


static gboolean
destripe_dialog (GimpDrawable *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *preview;
  GtkWidget     *grid;
  GtkWidget     *button;
  GtkAdjustment *adj;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_dialog_new (_("Destripe"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new_from_drawable (drawable);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (destripe_preview),
                            drawable);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 1,
                              _("_Width:"), SCALE_WIDTH, 0,
                              vals.avg_width, 2, MAX_AVG, 1, 10, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &vals.avg_width);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  button = gtk_check_button_new_with_mnemonic (_("Create _histogram"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), vals.histogram);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &vals.histogram);
  g_signal_connect_swapped (button, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
