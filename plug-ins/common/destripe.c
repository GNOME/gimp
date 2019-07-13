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


/*
 * Constants...
 */

#define PLUG_IN_PROC    "plug-in-destripe"
#define PLUG_IN_BINARY  "destripe"
#define PLUG_IN_ROLE    "gimp-destripe"
#define PLUG_IN_VERSION "0.2"
#define SCALE_WIDTH     140
#define MAX_AVG         100


/*
 * Local functions...
 */

static void      query (void);
static void      run   (const gchar      *name,
                        gint              nparams,
                        const GimpParam  *param,
                        gint             *nreturn_vals,
                        GimpParam       **return_vals);

static void      destripe         (gint32        drawable_ID,
                                   GimpPreview  *preview);
static void      destripe_preview (gpointer      drawable_ID,
                                   GimpPreview  *preview);

static gboolean  destripe_dialog  (gint32        drawable_ID);

/*
 * Globals...
 */

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

typedef struct
{
  gboolean histogram;
  gint     avg_width;
  gboolean preview;
} DestripeValues;

static DestripeValues vals =
{
  FALSE, /* histogram     */
  36,    /* average width */
  TRUE   /* preview */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",  "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"          },
    { GIMP_PDB_IMAGE,    "image",     "Input image"                           },
    { GIMP_PDB_DRAWABLE, "drawable",  "Input drawable"                        },
    { GIMP_PDB_INT32,    "avg-width", "Averaging filter width (default = 36)" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Remove vertical stripe artifacts from the image"),
                          "This plug-in tries to remove vertical stripes from "
                          "an image.",
                          "Marc Lehmann <pcg@goof.com>",
                          "Marc Lehmann <pcg@goof.com>",
                          PLUG_IN_VERSION,
                          N_("Des_tripe..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Enhance");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];  /* Return values */
  GimpPDBStatusType  status;     /* Return status */
  GimpRunMode        run_mode;   /* Current run mode */
  gint32             drawable_ID;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  status   = GIMP_PDB_SUCCESS;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  run_mode    = param[0].data.d_int32;
  drawable_ID = param[2].data.d_drawable;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*
       * Possibly retrieve data...
       */
      gimp_get_data (PLUG_IN_PROC, &vals);

      /*
       * Get information from the dialog...
       */
      if (! destripe_dialog (drawable_ID))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*
       * Make sure all the arguments are present...
       */
      if (nparams != 4)
        status = GIMP_PDB_CALLING_ERROR;
      else
        vals.avg_width = param[3].data.d_int32;
      break;

    case GIMP_RUN_WITH_LAST_VALS :
      /*
       * Possibly retrieve data...
       */
      gimp_get_data (PLUG_IN_PROC, &vals);
      break;

    default :
      status = GIMP_PDB_CALLING_ERROR;
      break;
    };

  /*
   * Destripe the image...
   */

  if (status == GIMP_PDB_SUCCESS)
    {
      if ((gimp_drawable_is_rgb (drawable_ID) ||
           gimp_drawable_is_gray (drawable_ID)))
        {
          /*
           * Run!
           */
          destripe (drawable_ID, NULL);

          /*
           * If run mode is interactive, flush displays...
           */
          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*
           * Store data...
           */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &vals, sizeof (vals));
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    };

  /*
   * Reset the current run status...
   */
  values[0].data.d_status = status;
}

static void
destripe (gint32       drawable_ID,
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

      if (! gimp_drawable_mask_intersect (drawable_ID,
                                          &x1, &y1, &width, &height))
        {
          return;
        }

      progress = 0;
      progress_inc = 0.5 * tile_width / width;
    }

  x2 = x1 + width;

  if (gimp_drawable_is_rgb (drawable_ID))
    {
      if (gimp_drawable_has_alpha (drawable_ID))
        format = babl_format ("R'G'B'A u8");
      else
        format = babl_format ("R'G'B' u8");
    }
  else
    {
      if (gimp_drawable_has_alpha (drawable_ID))
        format = babl_format ("Y'A u8");
      else
        format = babl_format ("Y' u8");
    }

  bpp = babl_format_get_bytes_per_pixel (format);

  /*
   * Setup for filter...
   */

  src_buffer  = gimp_drawable_get_buffer (drawable_ID);
  dest_buffer = gimp_drawable_get_shadow_buffer (drawable_ID);

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

      gimp_drawable_merge_shadow (drawable_ID, TRUE);
      gimp_drawable_update (drawable_ID,
                            x1, y1, width, height);
    }

  g_free (hist);
  g_free (corr);
}

static void
destripe_preview (gpointer     drawable_ID,
                  GimpPreview *preview)
{
  destripe (GPOINTER_TO_INT (drawable_ID), preview);
}


static gboolean
destripe_dialog (gint32 drawable_ID)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *table;
  GtkWidget *button;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Destripe"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new_from_drawable_id (drawable_ID);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (destripe_preview),
                            GINT_TO_POINTER (drawable_ID));

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
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
