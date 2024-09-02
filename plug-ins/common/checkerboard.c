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


typedef struct _Checkerboard      Checkerboard;
typedef struct _CheckerboardClass CheckerboardClass;

struct _Checkerboard
{
  GimpPlugIn parent_instance;
};

struct _CheckerboardClass
{
  GimpPlugInClass parent_class;
};


#define CHECKERBOARD_TYPE  (checkerboard_get_type ())
#define CHECKERBOARD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHECKERBOARD_TYPE, Checkerboard))

GType                   checkerboard_get_type         (void) G_GNUC_CONST;

static GList          * checkerboard_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * checkerboard_create_procedure (GimpPlugIn           *plug_in,
                                                       const gchar          *name);

static GimpValueArray * checkerboard_run              (GimpProcedure        *procedure,
                                                       GimpRunMode           run_mode,
                                                       GimpImage            *image,
                                                       gint                  n_drawables,
                                                       GimpDrawable        **drawables,
                                                       GimpProcedureConfig  *config,
                                                       gpointer              run_data);

static void             do_checkerboard_pattern       (GObject       *config,
                                                       GimpDrawable  *drawable,
                                                       GimpPreview   *preview);
static void             do_checkerboard_preview       (GtkWidget     *widget,
                                                       GObject       *config);
static gint             inblock                       (gint           pos,
                                                       gint           size);

static gboolean         checkerboard_dialog           (GimpProcedure *procedure,
                                                       GObject       *config,
                                                       GimpImage     *image,
                                                       GimpDrawable  *drawable);


G_DEFINE_TYPE (Checkerboard, checkerboard, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (CHECKERBOARD_TYPE)
DEFINE_STD_SET_I18N


static void
checkerboard_class_init (CheckerboardClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = checkerboard_query_procedures;
  plug_in_class->create_procedure = checkerboard_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
checkerboard_init (Checkerboard *checkerboard)
{
}

static GList *
checkerboard_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
checkerboard_create_procedure (GimpPlugIn  *plug_in,
                               const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            checkerboard_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure,
                                     _("_Checkerboard (legacy)..."));
      gimp_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Render/Pattern");

      gimp_procedure_set_documentation (procedure,
                                        _("Create a checkerboard pattern"),
                                        "More here later",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Brent Burton & the Edward Blevins",
                                      "Brent Burton & the Edward Blevins",
                                      "1997");

      gimp_procedure_add_boolean_argument (procedure, "psychobilly",
                                           _("_Psychobilly"),
                                           _("Render a psychobilly checkerboard"),
                                           FALSE,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "check-size",
                                       _("_Size"),
                                       _("Size of the checks"),
                                       1, GIMP_MAX_IMAGE_SIZE, 10,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_unit_aux_argument (procedure, "check-size-unit",
                                            _("Check size unit of measure"),
                                            _("Check size unit of measure"),
                                            TRUE, TRUE, gimp_unit_pixel (),
                                            GIMP_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
checkerboard_run (GimpProcedure        *procedure,
                  GimpRunMode           run_mode,
                  GimpImage            *image,
                  gint                  n_drawables,
                  GimpDrawable        **drawables,
                  GimpProcedureConfig  *config,
                  gpointer              run_data)
{
  GimpDrawable *drawable;

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

  if (run_mode == GIMP_RUN_INTERACTIVE &&
      ! checkerboard_dialog (procedure, G_OBJECT (config), image, drawable))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  if (gimp_drawable_is_rgb  (drawable) ||
      gimp_drawable_is_gray (drawable))
    {
      do_checkerboard_pattern (G_OBJECT (config), drawable, NULL);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }
  else
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
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
                   gint      size,
                   gboolean  mode,
                   gpointer  data)
{
  CheckerboardParam_t *param = (CheckerboardParam_t*) data;

  gint val, xp, yp;
  gint b;

  if (mode)
    {
      /* Psychobilly Mode */
      val = (inblock (x, size) != inblock (y, size));
    }
  else
    {
      /* Normal, regular checkerboard mode.
       * Determine base factor (even or odd) of block
       * this x/y position is in.
       */
      xp = x / size;
      yp = y / size;

      /* if both even or odd, color sqr */
      val = ( (xp & 1) != (yp & 1) );
    }

  for (b = 0; b < bpp; b++)
    dest[b] = val ? param->fg[b] : param->bg[b];
}

static void
do_checkerboard_pattern (GObject      *config,
                         GimpDrawable *drawable,
                         GimpPreview  *preview)
{
  CheckerboardParam_t  param;
  GeglColor           *color;
  guchar               fg[4]     = {0, 0, 0, 0};
  guchar               bg[4]     = {0, 0, 0, 0};
  guchar               fg_lum[1] = {0};
  guchar               bg_lum[1] = {0};
  const Babl          *format;
  gint                 bpp;
  gboolean             mode = FALSE;
  gint                 size = 10;

  if (config)
    g_object_get (config,
                  "check-size",  &size,
                  "psychobilly", &mode,
                  NULL);

  color = gimp_context_get_background ();
  gegl_color_get_pixel (color, babl_format_with_space ("R'G'B'A u8", NULL), bg);
  gegl_color_get_pixel (color, babl_format_with_space ("Y' u8", NULL), bg_lum);
  g_object_unref (color);
  color = gimp_context_get_foreground ();
  gegl_color_get_pixel (color, babl_format_with_space ("R'G'B'A u8", NULL), fg);
  gegl_color_get_pixel (color, babl_format_with_space ("Y' u8", NULL), fg_lum);
  g_object_unref (color);

  if (gimp_drawable_is_gray (drawable))
    {
      param.bg[0] = bg_lum[0];
      param.bg[1] = bg[3];

      param.fg[0] = fg_lum[0];
      param.fg[1] = fg[3];

      if (gimp_drawable_has_alpha (drawable))
        format = babl_format ("Y'A u8");
      else
        format = babl_format ("Y' u8");
    }
  else
    {
      for (gint i = 0; i < 4; i++)
        {
          param.bg[i] = bg[i];
          param.fg[i] = fg[i];
        }

      if (gimp_drawable_has_alpha (drawable))
        format = babl_format ("R'G'B'A u8");
      else
        format = babl_format ("R'G'B' u8");
    }

  bpp = babl_format_get_bytes_per_pixel (format);

  if (size < 1)
    {
      /* make size 1 to prevent division by zero */
      size = 1;
    }

  if (preview)
    {
      gint    x1, y1;
      gint    width, height;
      gint    i;
      guchar *buffer;

      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);
      bpp = gimp_drawable_get_bpp (drawable);
      buffer = g_new (guchar, width * height * bpp);

      for (i = 0; i < width * height; i++)
        {
          checkerboard_func (x1 + i % width,
                             y1 + i / width,
                             buffer + i * bpp,
                             bpp, size, mode,
                             &param);
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

      if (! gimp_drawable_mask_intersect (drawable, &x, &y, &w, &h))
        return;

      progress_total = w * h;

      gimp_progress_init (_("Checkerboard"));

      buffer = gimp_drawable_get_shadow_buffer (drawable);

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
                                     bpp, size, mode,
                                     &param);
                }

              d += roi.width * bpp;
            }

          progress_done += roi.width * roi.height;
          gimp_progress_update ((gdouble) progress_done /
                                (gdouble) progress_total);
        }

      g_object_unref (buffer);

      gimp_progress_update (1.0);

      gimp_drawable_merge_shadow (drawable, TRUE);
      gimp_drawable_update (drawable, x, y, w, h);
    }
}

static void
do_checkerboard_preview (GtkWidget *widget,
                         GObject   *config)
{
  GimpPreview  *preview  = GIMP_PREVIEW (widget);
  GimpDrawable *drawable = g_object_get_data (config, "drawable");

  do_checkerboard_pattern (config, drawable, preview);
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
checkerboard_dialog (GimpProcedure *procedure,
                     GObject       *config,
                     GimpImage     *image,
                     GimpDrawable  *drawable)
{
  GtkWidget *dialog;
  GtkWidget *preview;
  GtkWidget *toggle;
  GtkWidget *size_entry;
  gint       size, width, height;
  gdouble    xres;
  gdouble    yres;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Checkerboard"));

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image, &xres, &yres);

  width  = gimp_drawable_get_width (drawable);
  height = gimp_drawable_get_height (drawable);
  size   = MIN (width, height);

  size_entry =
    gimp_procedure_dialog_get_size_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                          "check-size", TRUE,
                                          "check-size-unit", "%a",
                                          GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                          xres);
  gtk_widget_set_margin_bottom (size_entry, 12);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (size_entry), 0, 0.0, size);

  /*  set upper and lower limits (in pixels)  */
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (size_entry), 0,
                                         1.0, size);

  toggle = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                             "psychobilly",
                                             GTK_TYPE_CHECK_BUTTON);
  gtk_widget_set_margin_bottom (toggle, 12);

  preview = gimp_procedure_dialog_get_drawable_preview (GIMP_PROCEDURE_DIALOG (dialog),
                                                        "preview", drawable);
  g_object_set_data (config, "drawable", drawable);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (do_checkerboard_preview),
                    config);

  g_signal_connect_swapped (config, "notify",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "preview", "check-size", "psychobilly",
                              NULL);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
