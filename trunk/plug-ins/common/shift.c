/* Shift --- image filter plug-in for GIMP
 * Copyright (C) 1997 Brian Degenhardt and Federico Mena Quintero
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
 *
 * Please direct all comments, questions, bug reports  etc to Brian Degenhardt
 * bdegenha@ucsd.edu
 *
 * You can contact Federico Mena Quintero at quartic@polloux.fciencias.unam.mx
 * You can contact the original GIMP authors at gimp@xcf.berkeley.edu
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Some useful macros */

#define PLUG_IN_PROC      "plug-in-shift"
#define PLUG_IN_BINARY    "shift"
#define SPIN_BUTTON_WIDTH  8
#define TILE_CACHE_SIZE   16
#define HORIZONTAL         0
#define VERTICAL           1

typedef struct
{
  gint  shift_amount;
  gint  orientation;
} ShiftValues;


/* Declare local functions.
 */
static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static void      shift                 (GimpDrawable *drawable,
                                        GimpPreview  *preview);

static gboolean  shift_dialog          (gint32        image_ID,
                                        GimpDrawable *drawable);
static void      shift_amount_callback (GtkWidget    *widget,
                                        gpointer      data);


/***** Local vars *****/

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static ShiftValues shvals =
{
  5,          /* shift amount */
  HORIZONTAL  /* orientation  */
};


/***** Functions *****/

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive"     },
    { GIMP_PDB_IMAGE,    "image",        "Input image (unused)"             },
    { GIMP_PDB_DRAWABLE, "drawable",     "Input drawable"                   },
    { GIMP_PDB_INT32,    "shift-amount", "shift amount (0 <= shift_amount_x <= 200)" },
    { GIMP_PDB_INT32,    "orientation",  "vertical, horizontal orientation" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Shift each row of pixels by a random amount"),
                          "Shifts the pixels of the specified drawable. "
                          "Each row will be displaced a random value of pixels.",
                          "Spencer Kimball and Peter Mattis, ported by Brian "
                          "Degenhardt and Federico Mena Quintero",
                          "Brian Degenhardt",
                          "1997",
                          N_("_Shift..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Distorts");
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
  gint32             image_ID;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;
  image_ID = param[1].data.d_int32;

  INIT_I18N ();

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  set the tile cache size  */
  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &shvals);

      /*  First acquire information with a dialog  */
      if (! shift_dialog (image_ID, drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          shvals.shift_amount = param[3].data.d_int32;
          shvals.orientation = (param[4].data.d_int32) ? HORIZONTAL : VERTICAL;

          if (shvals.shift_amount < 0 || shvals.shift_amount > 200)
            status = GIMP_PDB_CALLING_ERROR;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &shvals);
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
          gimp_progress_init (_("Shifting"));

          /*  run the shift effect  */
          shift (drawable, NULL);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &shvals, sizeof (ShiftValues));
        }
      else
        {
          /* gimp_message ("shift: cannot operate on indexed color images"); */
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
shift (GimpDrawable *drawable,
       GimpPreview  *preview)
{
  GimpPixelRgn      dest_rgn;
  gpointer          pr;
  GimpPixelFetcher *pft;
  gint              width, height;
  gint              bytes;
  guchar           *destline;
  guchar           *dest;
  gint              x1, y1, x2, y2;
  gint              x, y;
  gint              progress, max_progress;
  gint              i, n = 0;
  gint             *offsets;
  GRand            *gr;

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
      width  = x2 - x1;
      height = y2 - y1;
    }

  bytes  = drawable->bpp;

  progress     = 0;
  max_progress = width * height;

  /* Shift the image.  It's a pretty simple algorithm.  If horizontal
     is selected, then every row is shifted a random number of pixels
     in the range of -shift_amount/2 to shift_amount/2.  The effect is
     just reproduced with columns if vertical is selected.
   */

  n = (shvals.orientation == HORIZONTAL) ? height : width;

  offsets = g_new (gint, n);
  gr = g_rand_new ();

  for (i = 0; i < n; i++)
    offsets[i] = g_rand_int_range (gr,
                                   - (shvals.shift_amount + 1) / 2.0,
                                   + (shvals.shift_amount + 1) / 2.0);

  g_rand_free (gr);

  pft = gimp_pixel_fetcher_new (drawable, FALSE);
  gimp_pixel_fetcher_set_edge_mode (pft, GIMP_PIXEL_FETCHER_EDGE_WRAP);

  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       x1, y1, width, height, (preview == NULL), TRUE);

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      destline = dest_rgn.data;

      switch (shvals.orientation)
        {
        case HORIZONTAL:
          for (y = dest_rgn.y; y < dest_rgn.y + dest_rgn.h; y++)
            {
              dest = destline;

              for (x = dest_rgn.x; x < dest_rgn.x + dest_rgn.w; x++)
                {
                  gimp_pixel_fetcher_get_pixel (pft,
                                                x + offsets[y - y1], y, dest);
                  dest += bytes;
                }

              destline += dest_rgn.rowstride;
            }
          break;

        case VERTICAL:
          for (x = dest_rgn.x; x < dest_rgn.x + dest_rgn.w; x++)
            {
              dest = destline;

              for (y = dest_rgn.y; y < dest_rgn.y + dest_rgn.h; y++)
                {
                  gimp_pixel_fetcher_get_pixel (pft,
                                                x, y + offsets[x - x1], dest);
                  dest += dest_rgn.rowstride;
                }

              destline += bytes;
            }
          break;
        }

      if (preview)
        {
          gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                             &dest_rgn);
        }
      else
        {
          progress += dest_rgn.w * dest_rgn.h;
          gimp_progress_update ((double) progress / (double) max_progress);
        }
    }

  gimp_pixel_fetcher_destroy (pft);
  g_free (offsets);

  if (! preview)
    {
      /*  update the region  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);
    }
}

static gboolean
shift_dialog (gint32        image_ID,
              GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *frame;
  GtkWidget *size_entry;
  GtkWidget *vertical;
  GtkWidget *horizontal;
  GimpUnit   unit;
  gdouble    xres;
  gdouble    yres;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Shift"), PLUG_IN_BINARY,
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

  preview = gimp_drawable_preview_new (drawable, NULL);
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), preview);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (shift), drawable);

  frame = gimp_int_radio_group_new (FALSE, NULL,
                                    G_CALLBACK (gimp_radio_button_update),
                                    &shvals.orientation, shvals.orientation,

                                    _("Shift _horizontally"),
                                    HORIZONTAL, &horizontal,

                                    _("Shift _vertically"),
                                    VERTICAL,   &vertical,

                                    NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_signal_connect_swapped (horizontal, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (vertical, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image_ID, &xres, &yres);
  unit = gimp_image_get_unit (image_ID);

  size_entry = gimp_size_entry_new (1, unit, "%a", TRUE, FALSE, FALSE,
                                    SPIN_BUTTON_WIDTH,
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (size_entry), GIMP_UNIT_PIXEL);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (size_entry), 0, xres, TRUE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (size_entry), 0,
                                         1.0, 200.0);
  gtk_table_set_col_spacing (GTK_TABLE (size_entry), 0, 4);
  gtk_table_set_col_spacing (GTK_TABLE (size_entry), 2, 12);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (size_entry), 0,
                              (gdouble) shvals.shift_amount);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (size_entry),
                                _("Shift _amount:"), 1, 0, 0.0);

  g_signal_connect (size_entry, "value-changed",
                    G_CALLBACK (shift_amount_callback),
                    preview);
  gtk_box_pack_start (GTK_BOX (main_vbox), size_entry, FALSE, FALSE, 0);
  gtk_widget_show (size_entry);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
shift_amount_callback (GtkWidget *widget,
                       gpointer   data)
{
  GimpPreview *preview = GIMP_PREVIEW (data);

  shvals.shift_amount = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget),
                                                    0);
  gimp_preview_invalidate (preview);
}
