/*
 * This is a plug-in for GIMP.
 *
 * Copyright (C) 1997 Brent Burton & the Edward Blevins
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC      "plug-in-checkerboard"
#define PLUG_IN_BINARY    "checkerboard"
#define PLUG_IN_ROLE      "gimp-checkerboard"
#define SPIN_BUTTON_WIDTH 8


/* Variables set in dialog box */
typedef struct data
{
  gboolean mode;
  gint     size;
} CheckVals;


static void      query  (void);
static void      run    (const gchar       *name,
                         gint               nparams,
                         const GimpParam   *param,
                         gint              *nreturn_vals,
                         GimpParam        **return_vals);

static void      do_checkerboard_pattern    (gint32        drawable_ID,
                                             GimpPreview  *preview);
static void      do_checkerboard_preview    (gpointer      drawable_ID,
                                             GimpPreview  *preview);
static gint      inblock                    (gint          pos,
                                             gint          size);

static gboolean  checkerboard_dialog        (gint32        image_ID,
                                             gint32        drawable_ID);
static void      check_size_update_callback (GtkWidget    *widget);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static CheckVals cvals =
{
  FALSE,     /* mode */
  10         /* size */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",   "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",      "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",   "Input drawable"       },
    { GIMP_PDB_INT32,    "check-mode", "Check mode { REGULAR (0), PSYCHOBILY (1) }" },
    { GIMP_PDB_INT32,    "check-size", "Size of the checks"   }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Create a checkerboard pattern"),
                          "More here later",
                          "Brent Burton & the Edward Blevins",
                          "Brent Burton & the Edward Blevins",
                          "1997",
                          N_("_Checkerboard (legacy)..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Render/Pattern");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  gint32             image_ID;
  gint32             drawable_ID;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  run_mode    = param[0].data.d_int32;
  image_ID    = param[1].data.d_int32;
  drawable_ID = param[2].data.d_drawable;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &cvals);
      if (! checkerboard_dialog (image_ID, drawable_ID))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 5)
        status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS)
        {
          cvals.mode = param[3].data.d_int32;
          cvals.size = param[4].data.d_int32;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &cvals);
      break;

    default:
      break;
    }

  if (gimp_drawable_is_rgb (drawable_ID) ||
      gimp_drawable_is_gray (drawable_ID))
    {
      do_checkerboard_pattern (drawable_ID, NULL);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &cvals, sizeof (CheckVals));
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;
}

typedef struct
{
  guchar fg[4];
  guchar bg[4];
} CheckerboardParam_t;

static void
checkerboard_func (gint      x,
                   gint      y,
                   guchar   *dest,
                   gint      bpp,
                   gpointer  data)
{
  CheckerboardParam_t *param = (CheckerboardParam_t*) data;

  gint val, xp, yp;
  gint b;

  if (cvals.mode)
    {
      /* Psychobilly Mode */
      val = (inblock (x, cvals.size) != inblock (y, cvals.size));
    }
  else
    {
      /* Normal, regular checkerboard mode.
       * Determine base factor (even or odd) of block
       * this x/y position is in.
       */
      xp = x / cvals.size;
      yp = y / cvals.size;

      /* if both even or odd, color sqr */
      val = ( (xp & 1) != (yp & 1) );
    }

  for (b = 0; b < bpp; b++)
    dest[b] = val ? param->fg[b] : param->bg[b];
}

static void
do_checkerboard_pattern (gint32       drawable_ID,
                         GimpPreview *preview)
{
  CheckerboardParam_t  param;
  GimpRGB              fg, bg;
  const Babl          *format;
  gint                 bpp;

  gimp_context_get_background (&bg);
  gimp_context_get_foreground (&fg);

  if (gimp_drawable_is_gray (drawable_ID))
    {
      param.bg[0] = gimp_rgb_luminance_uchar (&bg);
      gimp_rgba_get_uchar (&bg, NULL, NULL, NULL, param.bg + 1);

      param.fg[0] = gimp_rgb_luminance_uchar (&fg);
      gimp_rgba_get_uchar (&fg, NULL, NULL, NULL, param.fg + 3);

      if (gimp_drawable_has_alpha (drawable_ID))
        format = babl_format ("R'G'B'A u8");
      else
        format = babl_format ("R'G'B' u8");
    }
  else
    {
      gimp_rgba_get_uchar (&bg,
                           param.bg, param.bg + 1, param.bg + 2, param.bg + 1);

      gimp_rgba_get_uchar (&fg,
                           param.fg, param.fg + 1, param.fg + 2, param.fg + 3);

      if (gimp_drawable_has_alpha (drawable_ID))
        format = babl_format ("Y'A u8");
      else
        format = babl_format ("Y' u8");
    }

  bpp = babl_format_get_bytes_per_pixel (format);

  if (cvals.size < 1)
    {
      /* make size 1 to prevent division by zero */
      cvals.size = 1;
    }

  if (preview)
    {
      gint    x1, y1;
      gint    width, height;
      gint    i;
      guchar *buffer;

      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);
      bpp = gimp_drawable_bpp (drawable_ID);
      buffer = g_new (guchar, width * height * bpp);

      for (i = 0; i < width * height; i++)
        {
          checkerboard_func (x1 + i % width,
                             y1 + i / width,
                             buffer + i * bpp,
                             bpp, &param);
        }

      gimp_preview_draw_buffer (preview, buffer, width * bpp);
      g_free (buffer);
    }
  else
    {
      GeglBuffer         *buffer;
      GeglBufferIterator *iter;
      gint                x, y, w, h;
      gint                progress_total;
      gint                progress_done = 0;

      if (! gimp_drawable_mask_intersect (drawable_ID, &x, &y, &w, &h))
        return;

      progress_total = w * h;

      gimp_progress_init (_("Checkerboard"));

      buffer = gimp_drawable_get_shadow_buffer (drawable_ID);

      iter = gegl_buffer_iterator_new (buffer,
                                       GEGL_RECTANGLE (x, y, w, h), 0,
                                       format,
                                       GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);

      while (gegl_buffer_iterator_next (iter))
        {
          GeglRectangle  roi  = iter->items[0].roi;
          guchar        *dest = iter->items[0].data;
          guchar        *d;
          gint           y1, x1;

          d = dest;

          for (y1 = 0; y1 < roi.height; y1++)
            {
              for (x1 = 0; x1 < roi.width; x1++)
                {
                  checkerboard_func (roi.x + x1,
                                     roi.y + y1,
                                     d + x1 * bpp,
                                     bpp, &param);
                }

              d += roi.width * bpp;
            }

          progress_done += roi.width * roi.height;
          gimp_progress_update ((gdouble) progress_done /
                                (gdouble) progress_total);
        }

      g_object_unref (buffer);

      gimp_progress_update (1.0);

      gimp_drawable_merge_shadow (drawable_ID, TRUE);
      gimp_drawable_update (drawable_ID, x, y, w, h);
    }
}

static void
do_checkerboard_preview (gpointer     drawable_ID,
                         GimpPreview *preview)
{
  do_checkerboard_pattern (GPOINTER_TO_INT (drawable_ID), preview);
}

static gint
inblock (gint pos,
         gint size)
{
  static gint *in  = NULL;       /* initialized first time */
  static gint  len = -1;

  /* avoid a FP exception */
  if (size == 1)
    size = 2;

  if (in && len != size * size)
    {
      g_free (in);
      in = NULL;
    }
  len = size * size;

  /* Initialize the array; since we'll be called thousands of
   * times with the same size value, precompute the array.
   */
  if (in == NULL)
    {
      gint cell = 1;    /* cell value */
      gint i, j, k;

      in = g_new (gint, len);

      /* i is absolute index into in[]
       * j is current number of blocks to fill in with a 1 or 0.
       * k is just counter for the j cells.
       */
      i = 0;
      for (j = 1; j <= size; j++)
        { /* first half */
          for (k = 0; k < j; k++)
            {
              in[i++] = cell;
            }
          cell = !cell;
        }
      for (j = size - 1; j >= 1; j--)
        { /* second half */
          for (k = 0; k < j; k++)
            {
              in[i++] = cell;
            }
          cell = !cell;
        }
    }

  /* place pos within 0..(len-1) grid and return the value. */
  return in[pos % (len - 1)];
}

static gboolean
checkerboard_dialog (gint32 image_ID,
                     gint32 drawable_ID)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *preview;
  GtkWidget *toggle;
  GtkWidget *size_entry;
  gint       size, width, height;
  GimpUnit   unit;
  gdouble    xres;
  gdouble    yres;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Checkerboard"), PLUG_IN_ROLE,
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

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  preview = gimp_drawable_preview_new_from_drawable_id (drawable_ID);
  gtk_box_pack_start (GTK_BOX (vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (do_checkerboard_preview),
                            GINT_TO_POINTER (drawable_ID));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image_ID, &xres, &yres);
  unit = gimp_image_get_unit (image_ID);

  width  = gimp_drawable_width (drawable_ID);
  height = gimp_drawable_height (drawable_ID);
  size   = MIN (width, height);

  size_entry = gimp_size_entry_new (1, unit, "%a",
                                    TRUE, TRUE, FALSE, SPIN_BUTTON_WIDTH,
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_table_set_col_spacing (GTK_TABLE (size_entry), 0, 4);
  gtk_table_set_col_spacing (GTK_TABLE (size_entry), 1, 4);
  gtk_box_pack_start (GTK_BOX (hbox), size_entry, FALSE, FALSE, 0);
  gtk_widget_show (size_entry);

  /*  set the unit back to pixels, since most times we will want pixels */
  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (size_entry), GIMP_UNIT_PIXEL);

  /*  set the resolution to the image resolution  */
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (size_entry), 0, xres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (size_entry), 0, 0.0, size);

  /*  set upper and lower limits (in pixels)  */
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (size_entry), 0,
                                         1.0, size);

  /*  initialize the values  */
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (size_entry), 0, cvals.size);

  /*  attach labels  */
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (size_entry),
                                _("_Size:"), 1, 0, 0.0);

  g_signal_connect (size_entry, "value-changed",
                    G_CALLBACK (check_size_update_callback),
                    &cvals.size);
  g_signal_connect_swapped (size_entry, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  toggle = gtk_check_button_new_with_mnemonic (_("_Psychobilly"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cvals.mode);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &cvals.mode);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
check_size_update_callback (GtkWidget *widget)
{
  cvals.size = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
}
