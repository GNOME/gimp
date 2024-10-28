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
#define MAX_AVG         100


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
#define DESTRIPE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DESTRIPE_TYPE, Destripe))

GType                   destripe_get_type         (void) G_GNUC_CONST;

static GList          * destripe_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * destripe_create_procedure (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * destripe_run              (GimpProcedure        *procedure,
                                                   GimpRunMode           run_mode,
                                                   GimpImage            *image,
                                                   GimpDrawable        **drawables,
                                                   GimpProcedureConfig  *config,
                                                   gpointer              run_data);

static void             destripe                  (GObject              *config,
                                                   GimpDrawable         *drawable,
                                                   GimpPreview          *preview);
static void             destripe_preview          (GtkWidget            *widget,
                                                   GObject              *config);

static gboolean         destripe_dialog           (GimpProcedure        *procedure,
                                                   GObject              *config,
                                                   GimpDrawable         *drawable);


G_DEFINE_TYPE (Destripe, destripe, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (DESTRIPE_TYPE)
DEFINE_STD_SET_I18N


static void
destripe_class_init (DestripeClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = destripe_query_procedures;
  plug_in_class->create_procedure = destripe_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
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
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("Des_tripe..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Colors/Tone Mapping");

      gimp_procedure_set_documentation (procedure,
                                        _("Remove vertical stripe artifacts "
                                          "from the image"),
                                        _("This plug-in tries to remove vertical "
                                          "stripes from an image."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Marc Lehmann <pcg@goof.com>",
                                      "Marc Lehmann <pcg@goof.com>",
                                      PLUG_IN_VERSION);

      gimp_procedure_add_int_argument (procedure, "avg-width",
                                       _("_Width"),
                                       _("Averaging filter width"),
                                       2, MAX_AVG, 36,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure,
                                           "create-histogram",
                                           _("Create _histogram"),
                                           _("Output a histogram"),
                                           FALSE,
                                           G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
destripe_run (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *image,
              GimpDrawable        **drawables,
              GimpProcedureConfig  *config,
              gpointer              run_data)
{
  GimpDrawable *drawable;

  gegl_init (NULL, NULL);

  if (gimp_core_object_array_get_length ((GObject **) drawables) != 1)
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

  if (run_mode == GIMP_RUN_INTERACTIVE && ! destripe_dialog (procedure, G_OBJECT (config), drawable))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  if (gimp_drawable_is_rgb (drawable)|| gimp_drawable_is_gray (drawable))
    destripe (G_OBJECT (config), drawable, NULL);
  else
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             NULL);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static void
destripe (GObject      *config,
          GimpDrawable *drawable,
          GimpPreview  *preview)
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
  gint        avg_width;
  gboolean    histogram;

  g_object_get (config,
                "avg-width",        &avg_width,
                "create-histogram", &histogram,
                NULL);

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
    gint extend = (avg_width / 2) * bpp;

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

          if (histogram)
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
destripe_preview (GtkWidget *widget,
                  GObject   *config)
{
  GimpPreview  *preview  = GIMP_PREVIEW (widget);
  GimpDrawable *drawable = g_object_get_data (config, "drawable");

  destripe (config, drawable, preview);
}


static gboolean
destripe_dialog (GimpProcedure *procedure,
                 GObject       *config,
                 GimpDrawable  *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *preview;
  GtkWidget     *scale;
  GtkWidget     *button;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Destripe"));

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  preview = gimp_procedure_dialog_get_drawable_preview (GIMP_PROCEDURE_DIALOG (dialog),
                                                        "preview", drawable);

  scale = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "avg-width", 1.0);
  gtk_widget_set_margin_bottom (scale, 12);

  button = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                             "create-histogram",
                                             GTK_TYPE_CHECK_BUTTON);
  gtk_widget_set_margin_bottom (button, 12);

  g_object_set_data (config, "drawable", drawable);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (destripe_preview),
                    config);

  g_signal_connect_swapped (config, "notify",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "preview", "avg-width", "create-histogram",
                              NULL);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
