/* Deinterlace 1.00 - image processing plug-in for GIMP
 *
 * Copyright (C) 1997 Andrew Kieschnick (andrewk@mail.utexas.edu)
 *
 * Original deinterlace for GIMP 0.54 API by Federico Mena Quintero
 *
 * Copyright (C) 1996 Federico Mena Quintero
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
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-deinterlace"
#define PLUG_IN_BINARY "deinterlace"


enum
{
  ODD_FIELDS,
  EVEN_FIELDS
};

typedef struct
{
  gint     evenness;
  gboolean preview;
} DeinterlaceValues;


/* Declare local functions.
 */
static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);


static void      deinterlace        (GimpDrawable *drawable,
                                     GimpPreview  *preview);

static gboolean  deinterlace_dialog (GimpDrawable *drawable);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static DeinterlaceValues devals =
{
  EVEN_FIELDS,  /* evenness */
  TRUE          /* preview */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,     "run-mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,     "image",    "Input image (unused)"         },
    { GIMP_PDB_DRAWABLE,  "drawable", "Input drawable"               },
    { GIMP_PDB_INT32,     "evenodd",  "0 = keep odd, 1 = keep even"  }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Fix images where every other row is missing"),
                          "Deinterlace is useful for processing images from "
                          "video capture cards. When only the odd or even "
                          "fields get captured, deinterlace can be used to "
                          "interpolate between the existing fields to correct "
                          "this.",
                          "Andrew Kieschnick",
                          "Andrew Kieschnick",
                          "1997",
                          N_("_Deinterlace..."),
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
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &devals);
      if (! deinterlace_dialog (drawable))
        status = GIMP_PDB_EXECUTION_ERROR;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 4)
        status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS)
        devals.evenness = param[3].data.d_int32;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &devals);
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
          gimp_progress_init (_("Deinterlace"));
          gimp_tile_cache_ntiles (2 * (drawable->width /
                                       gimp_tile_width () + 1));
          deinterlace (drawable, NULL);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &devals, sizeof (DeinterlaceValues));
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
deinterlace (GimpDrawable *drawable,
             GimpPreview  *preview)
{
  GimpPixelRgn  srcPR, destPR;
  gboolean      has_alpha;
  guchar       *dest;
  guchar       *dest_buffer = NULL;
  guchar       *upper;
  guchar       *lower;
  gint          row, col;
  gint          x, y;
  gint          width, height;
  gint          bytes;

  bytes = drawable->bpp;

  if (preview)
    {
      gimp_preview_get_position (preview, &x, &y);
      gimp_preview_get_size (preview, &width, &height);

      dest_buffer = g_new (guchar, width * height * bytes);
      dest = dest_buffer;
    }
  else
    {
      gint x2, y2;

      gimp_drawable_mask_bounds (drawable->drawable_id, &x, &y, &x2, &y2);

      width  = x2 - x;
      height = y2 - y;

      dest = g_new (guchar, width * bytes);

      gimp_pixel_rgn_init (&destPR, drawable, x, y, width, height, TRUE, TRUE);
    }

  gimp_pixel_rgn_init (&srcPR, drawable,
                       x, MAX (y - 1, 0),
                       width, MIN (height + 1, drawable->height),
                       FALSE, FALSE);

  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  /*  allocate row buffers  */
  upper = g_new (guchar, width * bytes);
  lower = g_new (guchar, width * bytes);

  /*  loop through the rows, performing our magic  */
  for (row = y; row < y + height; row++)
    {
      gimp_pixel_rgn_get_row (&srcPR, dest, x, row, width);

      if (row % 2 != devals.evenness)
        {
          if (row > 0)
            gimp_pixel_rgn_get_row (&srcPR, upper, x, row - 1, width);
          else
            gimp_pixel_rgn_get_row (&srcPR, upper, x, devals.evenness, width);

          if (row + 1 < drawable->height)
            gimp_pixel_rgn_get_row (&srcPR, lower, x, row + 1, width);
          else
            gimp_pixel_rgn_get_row (&srcPR, lower, x, row - 1 + devals.evenness,
                                    width);

          if (has_alpha)
            {
              const guchar *upix = upper;
              const guchar *lpix = lower;
              guchar       *dpix = dest;

              for (col = 0; col < width; col++)
                {
                  guint ualpha = upix[bytes - 1];
                  guint lalpha = lpix[bytes - 1];
                  guint alpha  = ualpha + lalpha;

                  if ((dpix[bytes - 1] = (alpha >> 1)))
                    {
                      gint b;

                      for (b = 0; b < bytes - 1; b++)
                        dpix[b] = (upix[b] * ualpha + lpix[b] * lalpha) / alpha;
                    }

                  upix += bytes;
                  lpix += bytes;
                  dpix += bytes;
                }
            }
          else
            {
              for (col = 0; col < width * bytes; col++)
                dest[col] = ((guint) upper[col] + (guint) lower[col]) / 2;
            }
        }

      if (preview)
        {
          dest += width * bytes;
        }
      else
        {
          gimp_pixel_rgn_set_row (&destPR, dest, x, row, width);

          if ((row % 20) == 0)
            gimp_progress_update ((double) row / (double) (height));
        }
    }

  if (preview)
    {
      gimp_preview_draw_buffer (preview, dest_buffer, width * bytes);
      dest = dest_buffer;
    }
  else
    {
      /*  update the deinterlaced region  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x, y, width, height);
    }

  g_free (lower);
  g_free (upper);
  g_free (dest);
}

static gboolean
deinterlace_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *frame;
  GtkWidget *odd;
  GtkWidget *even;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Deinterlace"), PLUG_IN_BINARY,
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

  preview = gimp_drawable_preview_new (drawable, &devals.preview);
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), preview);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (deinterlace),
                            drawable);

  frame = gimp_int_radio_group_new (FALSE, NULL,
                                    G_CALLBACK (gimp_radio_button_update),
                                    &devals.evenness, devals.evenness,

                                    _("Keep o_dd fields"),  ODD_FIELDS,  &odd,
                                    _("Keep _even fields"), EVEN_FIELDS, &even,

                                    NULL);

  g_signal_connect_swapped (odd, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (even, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
