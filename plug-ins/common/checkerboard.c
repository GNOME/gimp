/*
 * This is a plug-in for LIGMA.
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define PLUG_IN_PROC      "plug-in-checkerboard"
#define PLUG_IN_BINARY    "checkerboard"
#define PLUG_IN_ROLE      "ligma-checkerboard"
#define SPIN_BUTTON_WIDTH 8


/* Variables set in dialog box */
typedef struct data
{
  gboolean mode;
  gint     size;
} CheckVals;


typedef struct _Checkerboard      Checkerboard;
typedef struct _CheckerboardClass CheckerboardClass;

struct _Checkerboard
{
  LigmaPlugIn parent_instance;
};

struct _CheckerboardClass
{
  LigmaPlugInClass parent_class;
};


#define CHECKERBOARD_TYPE  (checkerboard_get_type ())
#define CHECKERBOARD (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHECKERBOARD_TYPE, Checkerboard))

GType                   checkerboard_get_type         (void) G_GNUC_CONST;

static GList          * checkerboard_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * checkerboard_create_procedure (LigmaPlugIn           *plug_in,
                                                       const gchar          *name);

static LigmaValueArray * checkerboard_run              (LigmaProcedure        *procedure,
                                                       LigmaRunMode           run_mode,
                                                       LigmaImage            *image,
                                                       gint                  n_drawables,
                                                       LigmaDrawable        **drawables,
                                                       const LigmaValueArray *args,
                                                       gpointer              run_data);

static void             do_checkerboard_pattern       (LigmaDrawable *drawable,
                                                       LigmaPreview  *preview);
static void             do_checkerboard_preview       (LigmaDrawable *drawable,
                                                       LigmaPreview  *preview);
static gint             inblock                       (gint          pos,
                                                       gint          size);

static gboolean         checkerboard_dialog           (LigmaImage    *image,
                                                       LigmaDrawable *drawable);
static void             check_size_update_callback    (GtkWidget    *widget);


G_DEFINE_TYPE (Checkerboard, checkerboard, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (CHECKERBOARD_TYPE)
DEFINE_STD_SET_I18N


static CheckVals cvals =
{
  FALSE,     /* mode */
  10         /* size */
};


static void
checkerboard_class_init (CheckerboardClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = checkerboard_query_procedures;
  plug_in_class->create_procedure = checkerboard_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
checkerboard_init (Checkerboard *checkerboard)
{
}

static GList *
checkerboard_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
checkerboard_create_procedure (LigmaPlugIn  *plug_in,
                               const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            checkerboard_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "RGB*, GRAY*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_menu_label (procedure,
                                     N_("_Checkerboard (legacy)..."));
      ligma_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Render/Pattern");

      ligma_procedure_set_documentation (procedure,
                                        N_("Create a checkerboard pattern"),
                                        "More here later",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Brent Burton & the Edward Blevins",
                                      "Brent Burton & the Edward Blevins",
                                      "1997");

      LIGMA_PROC_ARG_BOOLEAN (procedure, "psychobily",
                             "Psychobily",
                             "Render a psychobiliy checkerboard",
                             FALSE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "check-size",
                         "Check size",
                         "Size of the checks",
                         1, LIGMA_MAX_IMAGE_SIZE, 10,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
checkerboard_run (LigmaProcedure        *procedure,
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
      ligma_get_data (PLUG_IN_PROC, &cvals);

      if (! checkerboard_dialog (image, drawable))
        {
          return ligma_procedure_new_return_values (procedure,
                                                   LIGMA_PDB_CANCEL,
                                                   NULL);
        }
      break;

    case LIGMA_RUN_NONINTERACTIVE:
      cvals.mode = LIGMA_VALUES_GET_BOOLEAN (args, 0);
      cvals.size = LIGMA_VALUES_GET_INT     (args, 1);
      break;

    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_get_data (PLUG_IN_PROC, &cvals);
      break;

    default:
      break;
    }

  if (ligma_drawable_is_rgb  (drawable) ||
      ligma_drawable_is_gray (drawable))
    {
      do_checkerboard_pattern (drawable, NULL);

      if (run_mode != LIGMA_RUN_NONINTERACTIVE)
        ligma_displays_flush ();

      if (run_mode == LIGMA_RUN_INTERACTIVE)
        ligma_set_data (PLUG_IN_PROC, &cvals, sizeof (CheckVals));
    }
  else
    {
      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
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
do_checkerboard_pattern (LigmaDrawable *drawable,
                         LigmaPreview  *preview)
{
  CheckerboardParam_t  param;
  LigmaRGB              fg, bg;
  const Babl          *format;
  gint                 bpp;

  ligma_context_get_background (&bg);
  ligma_context_get_foreground (&fg);

  if (ligma_drawable_is_gray (drawable))
    {
      param.bg[0] = ligma_rgb_luminance_uchar (&bg);
      ligma_rgba_get_uchar (&bg, NULL, NULL, NULL, param.bg + 1);

      param.fg[0] = ligma_rgb_luminance_uchar (&fg);
      ligma_rgba_get_uchar (&fg, NULL, NULL, NULL, param.fg + 3);

      if (ligma_drawable_has_alpha (drawable))
        format = babl_format ("R'G'B'A u8");
      else
        format = babl_format ("R'G'B' u8");
    }
  else
    {
      ligma_rgba_get_uchar (&bg,
                           param.bg, param.bg + 1, param.bg + 2, param.bg + 1);

      ligma_rgba_get_uchar (&fg,
                           param.fg, param.fg + 1, param.fg + 2, param.fg + 3);

      if (ligma_drawable_has_alpha (drawable))
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

      ligma_preview_get_position (preview, &x1, &y1);
      ligma_preview_get_size (preview, &width, &height);
      bpp = ligma_drawable_get_bpp (drawable);
      buffer = g_new (guchar, width * height * bpp);

      for (i = 0; i < width * height; i++)
        {
          checkerboard_func (x1 + i % width,
                             y1 + i / width,
                             buffer + i * bpp,
                             bpp, &param);
        }

      ligma_preview_draw_buffer (preview, buffer, width * bpp);
      g_free (buffer);
    }
  else
    {
      GeglBuffer         *buffer;
      GeglBufferIterator *iter;
      gint                x, y, w, h;
      gint                progress_total;
      gint                progress_done = 0;

      if (! ligma_drawable_mask_intersect (drawable, &x, &y, &w, &h))
        return;

      progress_total = w * h;

      ligma_progress_init (_("Checkerboard"));

      buffer = ligma_drawable_get_shadow_buffer (drawable);

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
          ligma_progress_update ((gdouble) progress_done /
                                (gdouble) progress_total);
        }

      g_object_unref (buffer);

      ligma_progress_update (1.0);

      ligma_drawable_merge_shadow (drawable, TRUE);
      ligma_drawable_update (drawable, x, y, w, h);
    }
}

static void
do_checkerboard_preview (LigmaDrawable *drawable,
                         LigmaPreview  *preview)
{
  do_checkerboard_pattern (drawable, preview);
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
checkerboard_dialog (LigmaImage    *image,
                     LigmaDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *preview;
  GtkWidget *toggle;
  GtkWidget *size_entry;
  gint       size, width, height;
  LigmaUnit   unit;
  gdouble    xres;
  gdouble    yres;
  gboolean   run;

  ligma_ui_init (PLUG_IN_BINARY);

  dialog = ligma_dialog_new (_("Checkerboard"), PLUG_IN_ROLE,
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

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  preview = ligma_drawable_preview_new_from_drawable (drawable);
  gtk_box_pack_start (GTK_BOX (vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (do_checkerboard_preview),
                            drawable);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  Get the image resolution and unit  */
  ligma_image_get_resolution (image, &xres, &yres);
  unit = ligma_image_get_unit (image);

  width  = ligma_drawable_get_width (drawable);
  height = ligma_drawable_get_height (drawable);
  size   = MIN (width, height);

  size_entry = ligma_size_entry_new (1, unit, "%a",
                                    TRUE, TRUE, FALSE, SPIN_BUTTON_WIDTH,
                                    LIGMA_SIZE_ENTRY_UPDATE_SIZE);
  gtk_box_pack_start (GTK_BOX (hbox), size_entry, FALSE, FALSE, 0);
  gtk_widget_show (size_entry);

  /*  set the unit back to pixels, since most times we will want pixels */
  ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (size_entry), LIGMA_UNIT_PIXEL);

  /*  set the resolution to the image resolution  */
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (size_entry), 0, xres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (size_entry), 0, 0.0, size);

  /*  set upper and lower limits (in pixels)  */
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (size_entry), 0,
                                         1.0, size);

  /*  initialize the values  */
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (size_entry), 0, cvals.size);

  /*  attach labels  */
  ligma_size_entry_attach_label (LIGMA_SIZE_ENTRY (size_entry),
                                _("_Size:"), 1, 0, 0.0);

  g_signal_connect (size_entry, "value-changed",
                    G_CALLBACK (check_size_update_callback),
                    &cvals.size);
  g_signal_connect_swapped (size_entry, "value-changed",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);

  toggle = gtk_check_button_new_with_mnemonic (_("_Psychobilly"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cvals.mode);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &cvals.mode);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
check_size_update_callback (GtkWidget *widget)
{
  cvals.size = ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (widget), 0);
}
