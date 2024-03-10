/* GIMP - The GNU Image Manipulation Program
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

/* SVG loader plug-in
 * (C) Copyright 2003  Dom Lachowicz <cinamod@hotmail.com>
 *
 * Largely rewritten in September 2003 by Sven Neumann <sven@gimp.org>
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <librsvg/rsvg.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC               "file-svg-load"
#define LOAD_THUMB_PROC         "file-svg-load-thumb"
#define PLUG_IN_BINARY          "file-svg"
#define PLUG_IN_ROLE            "gimp-file-svg"
#define SVG_VERSION             "2.5.0"
#define SVG_DEFAULT_RESOLUTION  90.0
#define SVG_DEFAULT_SIZE        500
#define SVG_PREVIEW_SIZE        128


typedef struct
{
  gdouble    resolution;
  gint       width;
  gint       height;
} SvgLoadVals;


typedef struct _Svg      Svg;
typedef struct _SvgClass SvgClass;

struct _Svg
{
  GimpPlugIn      parent_instance;
};

struct _SvgClass
{
  GimpPlugInClass parent_class;
};


#define SVG_TYPE  (svg_get_type ())
#define SVG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SVG_TYPE, Svg))

GType                   svg_get_type         (void) G_GNUC_CONST;

static GList          * svg_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * svg_create_procedure (GimpPlugIn            *plug_in,
                                              const gchar           *name);

static GimpValueArray * svg_load             (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GFile                 *file,
                                              GimpMetadata          *metadata,
                                              GimpMetadataLoadFlags *flags,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);
static GimpValueArray * svg_load_thumb       (GimpProcedure         *procedure,
                                              GFile                 *file,
                                              gint                   size,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);

static GimpImage         * load_image        (GFile                 *file,
                                              gint                   width,
                                              gint                   height,
                                              gdouble                resolution,
                                              RsvgHandleFlags        rsvg_flags,
                                              GError               **error);
static GdkPixbuf         * load_rsvg_pixbuf  (GFile                 *file,
                                              gint                   width,
                                              gint                   height,
                                              gdouble                resolution,
                                              RsvgHandleFlags        rsvg_flags,
                                              gboolean              *allow_retry,
                                              GError               **error);
static gboolean            load_rsvg_size    (GFile                 *file,
                                              SvgLoadVals           *vals,
                                              RsvgHandleFlags        rsvg_flags,
                                              GError               **error);
static GimpPDBStatusType   load_dialog       (GFile                 *file,
                                              GimpProcedure         *procedure,
                                              GimpProcedureConfig   *config,
                                              RsvgHandleFlags       *rsvg_flags,
                                              GError               **error);

static void              svg_destroy_surface (guchar                *pixels,
                                              cairo_surface_t       *surface);



G_DEFINE_TYPE (Svg, svg, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (SVG_TYPE)
DEFINE_STD_SET_I18N


static void
svg_class_init (SvgClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = svg_query_procedures;
  plug_in_class->create_procedure = svg_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
svg_init (Svg *svg)
{
}

static GList *
svg_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static GimpProcedure *
svg_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           svg_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("SVG image"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads files in the SVG file format",
                                        "Renders SVG files to raster graphics "
                                        "using librsvg.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Dom Lachowicz, Sven Neumann",
                                      "Dom Lachowicz <cinamod@hotmail.com>",
                                      SVG_VERSION);

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/svg+xml");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "svg");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,<?xml,0,string,<svg");

      gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);

      GIMP_PROC_ARG_DOUBLE (procedure, "resolution",
                            _("Resolu_tion"),
                            _("Resolution to use for rendering the SVG"),
                            GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION, 300,
                            GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "width",
                         _("_Width"),
                         _("Width (in pixels) to load the SVG in. "
                         "(0 for original width, a negative width to "
                         "specify a maximum width)"),
                         -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE, 0,
                         GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "height",
                         _("_Height"),
                         _("Height (in pixels) to load the SVG in. "
                         "(0 for original height, a negative height to "
                         "specify a maximum height)"),
                         -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE, 0,
                         GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_CHOICE (procedure, "paths",
                            _("_Paths"),
                            _("Whether and how to import paths so that they can be used with the path tool"),
                            gimp_choice_new_with_values ("no-import",     0, _("Don't import paths"),        NULL,
                                                         "import",        0, _("Import paths individually"), NULL,
                                                         "import-merged", 0, _("Merge imported paths"),      NULL,
                                                         NULL),
                            "no-import", G_PARAM_READWRITE);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = gimp_thumbnail_procedure_new (plug_in, name,
                                                GIMP_PDB_PROC_TYPE_PLUGIN,
                                                svg_load_thumb, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Generates a thumbnail of an SVG image",
                                        "Renders a thumbnail of an SVG file "
                                        "using librsvg.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Dom Lachowicz, Sven Neumann",
                                      "Dom Lachowicz <cinamod@hotmail.com>",
                                      SVG_VERSION);
    }

  return procedure;
}

static GimpValueArray *
svg_load (GimpProcedure         *procedure,
          GimpRunMode            run_mode,
          GFile                 *file,
          GimpMetadata          *metadata,
          GimpMetadataLoadFlags *flags,
          GimpProcedureConfig   *config,
          gpointer               run_data)
{
  GimpValueArray    *return_vals;
  GimpImage         *image;
  GError            *error      = NULL;
  RsvgHandleFlags    rsvg_flags = RSVG_HANDLE_FLAGS_NONE;
  GimpPDBStatusType  status;
  gchar             *import_paths;
  gint               width;
  gint               height;
  gdouble            resolution;

  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      status = load_dialog (file, procedure, config, &rsvg_flags, &error);
      if (status != GIMP_PDB_SUCCESS)
        return gimp_procedure_new_return_values (procedure, status, error);
    }

  g_object_get (config,
                "width",      &width,
                "height",     &height,
                "resolution", &resolution,
                NULL);
  image = load_image (file, width, height, resolution, rsvg_flags, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  g_object_get (config, "paths", &import_paths, NULL);
  if (g_strcmp0 (import_paths, "no-import") != 0)
    {
      GimpVectors **vectors;
      gint          num_vectors;

      gimp_vectors_import_from_file (image, file,
                                     g_strcmp0 (import_paths, "import-merged") == 0,
                                     TRUE, &num_vectors, &vectors);
      if (num_vectors > 0)
        g_free (vectors);
    }
  g_free (import_paths);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
svg_load_thumb (GimpProcedure       *procedure,
                GFile               *file,
                gint                 size,
                GimpProcedureConfig *config,
                gpointer             run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  gint            width        = 0;
  gint            height       = 0;
  gint            thumb_width  = 0;
  gint            thumb_height = 0;
  GError         *error        = NULL;
  SvgLoadVals     vals         =
  {
    SVG_DEFAULT_RESOLUTION,
    SVG_PREVIEW_SIZE,
    SVG_PREVIEW_SIZE,
  };

  gegl_init (NULL, NULL);

  if (load_rsvg_size (file, &vals, RSVG_HANDLE_FLAGS_NONE, NULL))
    {
      width  = vals.width;
      height = vals.height;
      if (width > height)
        {
          thumb_width  = size;
          thumb_height = size * height / width;
        }
      else
        {
          thumb_width  = size * width / height;
          thumb_height = size;
        }
    }
  else
    {
      thumb_width  = size;
      thumb_height = size;
    }

  image = load_image (file, thumb_width, thumb_height, SVG_DEFAULT_RESOLUTION, RSVG_HANDLE_FLAGS_NONE, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);
  GIMP_VALUES_SET_INT   (return_vals, 2, width);
  GIMP_VALUES_SET_INT   (return_vals, 3, height);

  gimp_value_array_truncate (return_vals, 4);

  return return_vals;
}

static GimpImage *
load_image (GFile            *file,
            gint              width,
            gint              height,
            gdouble           resolution,
            RsvgHandleFlags   rsvg_flags,
            GError          **load_error)
{
  GimpImage *image;
  GimpLayer *layer;
  GdkPixbuf *pixbuf;
  GError    *error = NULL;

  pixbuf = load_rsvg_pixbuf (file, width, height, resolution, rsvg_flags, NULL, &error);

  if (! pixbuf)
    {
      /*  Do not rely on librsvg setting GError on failure!  */
      g_set_error (load_error,
                   error ? error->domain : 0, error ? error->code : 0,
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file),
                   error ? error->message : _("Unknown reason"));
      g_clear_error (&error);

      return NULL;
    }

  gimp_progress_init (_("Rendering SVG"));

  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  image = gimp_image_new (width, height, GIMP_RGB);
  gimp_image_undo_disable (image);

  gimp_image_set_resolution (image, resolution, resolution);

  layer = gimp_layer_new_from_pixbuf (image, _("Rendered SVG"), pixbuf,
                                      100,
                                      gimp_image_get_default_new_layer_mode (image),
                                      0.0, 1.0);
  gimp_image_insert_layer (image, layer, NULL, 0);

  gimp_image_undo_enable (image);

  return image;
}

#if ! LIBRSVG_CHECK_VERSION(2, 46, 0)
/* XXX Why we keep old deprecated implementation next to newer librsvg API is
 * because librsvg uses Rust since version 2.41.0. There are 2 raised problems:
 * 1. Rust is still not available or well tested on every architecture/OS out
 *    there yet so unless we decide to have SVG support as optional again, it
 *    would make GIMP non-available on these.
 * 2. There are some technical-ideological position against Rust for bundling
 *    and linking dependencies statically.
 * While the second point may or may not be a problem we want to take into
 * account (I guess that it mostly depends on the amount of additional
 * maintenance work it would imply), the first is definitely enough of a reason
 * to keep an old version requirement.
 * See also report #6821.
 */
static void
load_set_size_callback (gint     *width,
                        gint     *height,
                        gpointer  data)
{
  SvgLoadVals *vals = data;

  if (*width < 1 || *height < 1)
    {
      *width  = SVG_DEFAULT_SIZE;
      *height = SVG_DEFAULT_SIZE;
    }

  if (!vals->width || !vals->height)
    return;

  /*  either both arguments negative or none  */
  if ((vals->width * vals->height) < 0)
    return;

  if (vals->width > 0)
    {
      *width  = vals->width;
      *height = vals->height;
    }
  else
    {
      gdouble w      = *width;
      gdouble h      = *height;
      gdouble aspect = (gdouble) vals->width / (gdouble) vals->height;

      if (aspect > (w / h))
        {
          *height = abs (vals->height);
          *width  = (gdouble) abs (vals->width) * (w / h) + 0.5;
        }
      else
        {
          *width  = abs (vals->width);
          *height = (gdouble) abs (vals->height) / (w / h) + 0.5;
        }

      vals->width  = *width;
      vals->height = *height;
    }
}
#endif /* ! LIBRSVG_CHECK_VERSION(2, 46, 0) */

/*  This function renders a pixbuf from an SVG file according to vals.  */
static GdkPixbuf *
load_rsvg_pixbuf (GFile            *file,
                  gint              width,
                  gint              height,
                  gdouble           resolution,
                  RsvgHandleFlags   rsvg_flags,
                  gboolean         *allow_retry,
                  GError          **error)
{
  GdkPixbuf  *pixbuf  = NULL;
  RsvgHandle *handle;

#if LIBRSVG_CHECK_VERSION(2, 46, 0)
  cairo_surface_t *surface  = NULL;
  cairo_t         *cr       = NULL;
  RsvgRectangle    viewport = { 0, };
  guchar          *src;
  gint             y;
#else
  SvgLoadVals      vals;
#endif

  handle = rsvg_handle_new_from_gfile_sync (file, rsvg_flags, NULL, error);

  if (! handle)
    {
      /* The "huge data" error will be seen from the handle creation
       * already. No need to retry when the error occurs later.
       */
      if (allow_retry)
        *allow_retry = TRUE;
      return NULL;
    }

  rsvg_handle_set_dpi (handle, resolution);
#if LIBRSVG_CHECK_VERSION(2, 46, 0)
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr      = cairo_create (surface);

  viewport.width  = width;
  viewport.height = height;

  rsvg_handle_render_document (handle, cr, &viewport, NULL);
  pixbuf = gdk_pixbuf_new_from_data (cairo_image_surface_get_data (surface),
                                     GDK_COLORSPACE_RGB, TRUE, 8, width, height,
                                     cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, width),
                                     (GdkPixbufDestroyNotify) svg_destroy_surface, surface);
  cairo_destroy (cr);

  /* un-premultiply the data */
  src = gdk_pixbuf_get_pixels (pixbuf);

  for (y = 0; y < height; y++)
    {
      guchar *s = src;
      gint    w = width;

      while (w--)
        {
          GIMP_CAIRO_ARGB32_GET_PIXEL (s, s[0], s[1], s[2], s[3]);
          s += 4;
        }

      src += gdk_pixbuf_get_rowstride (pixbuf);
    }
#else
  vals.resolution = resolution;
  vals.width      = width;
  vals.height     = height;
  rsvg_handle_set_size_callback (handle, load_set_size_callback, vals, NULL);
  pixbuf = rsvg_handle_get_pixbuf (handle);
#endif

  g_object_unref (handle);

  return pixbuf;
}

static GtkWidget *size_label = NULL;

/*  This function retrieves the pixel size from an SVG file.  */
static gboolean
load_rsvg_size (GFile            *file,
                SvgLoadVals      *vals,
                RsvgHandleFlags   rsvg_flags,
                GError          **error)
{
  RsvgHandle        *handle;
  gboolean           has_size;
  gdouble            width, height;
#if ! LIBRSVG_CHECK_VERSION(2, 52, 0)
  RsvgDimensionData  dim;
#endif

  handle = rsvg_handle_new_from_gfile_sync (file, rsvg_flags, NULL, error);

  if (! handle)
    return FALSE;

  rsvg_handle_set_dpi (handle, vals->resolution);

#if LIBRSVG_CHECK_VERSION(2, 52, 0)
  rsvg_handle_get_intrinsic_size_in_pixels (handle, &width, &height);
#else
  rsvg_handle_get_dimensions (handle, &dim);
  width  = (gdouble) dim.width;
  height = (gdouble) dim.height;
#endif

  if (width > 0.0 && height > 0.0)
    {
      vals->width  = ceil (width);
      vals->height = ceil (height);
      has_size = TRUE;
    }
  else
    {
      vals->width  = SVG_DEFAULT_SIZE;
      vals->height = SVG_DEFAULT_SIZE;
      has_size = FALSE;
    }

  if (size_label)
    {
      if (has_size)
        {
          gchar *text = g_strdup_printf (_("%d × %d"),
                                         vals->width, vals->height);
          gtk_label_set_text (GTK_LABEL (size_label), text);
          g_free (text);
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (size_label),
                              _("SVG file does not\nspecify a size!"));
        }
    }

  g_object_unref (handle);

  if (vals->width  < 1)  vals->width  = 1;
  if (vals->height < 1)  vals->height = 1;

  return TRUE;
}


/*  User interface  */

static GimpSizeEntry *size       = NULL;
static GtkAdjustment *xadj       = NULL;
static GtkAdjustment *yadj       = NULL;
static GtkWidget     *constrain  = NULL;
static gdouble        ratio_x    = 1.0;
static gdouble        ratio_y    = 1.0;
static gint           svg_width  = 0;
static gint           svg_height = 0;

static void  load_dialog_set_ratio (gdouble x,
                                    gdouble y);


static void
load_dialog_size_callback (GtkWidget *widget,
                           gpointer   data)
{
  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (constrain)))
    {
      gdouble x = gimp_size_entry_get_refval (size, 0) / (gdouble) svg_width;
      gdouble y = gimp_size_entry_get_refval (size, 1) / (gdouble) svg_height;

      if (x != ratio_x)
        {
          load_dialog_set_ratio (x, x);
        }
      else if (y != ratio_y)
        {
          load_dialog_set_ratio (y, y);
        }
    }
}

static void
load_dialog_ratio_callback (GtkAdjustment *adj,
                            gpointer       data)
{
  gdouble x = gtk_adjustment_get_value (xadj);
  gdouble y = gtk_adjustment_get_value (yadj);

  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (constrain)))
    {
      if (x != ratio_x)
        y = x;
      else
        x = y;
    }

  load_dialog_set_ratio (x, y);
}

static void
load_dialog_set_ratio (gdouble x,
                       gdouble y)
{
  ratio_x = x;
  ratio_y = y;

  g_signal_handlers_block_by_func (size, load_dialog_size_callback, NULL);

  gimp_size_entry_set_refval (size, 0, svg_width  * x);
  gimp_size_entry_set_refval (size, 1, svg_height * y);

  g_signal_handlers_unblock_by_func (size, load_dialog_size_callback, NULL);

  g_signal_handlers_block_by_func (xadj, load_dialog_ratio_callback, NULL);
  g_signal_handlers_block_by_func (yadj, load_dialog_ratio_callback, NULL);

  gtk_adjustment_set_value (xadj, x);
  gtk_adjustment_set_value (yadj, y);

  g_signal_handlers_unblock_by_func (xadj, load_dialog_ratio_callback, NULL);
  g_signal_handlers_unblock_by_func (yadj, load_dialog_ratio_callback, NULL);
}

static GimpPDBStatusType
load_dialog (GFile                *file,
             GimpProcedure        *procedure,
             GimpProcedureConfig  *config,
             RsvgHandleFlags      *rsvg_flags,
             GError              **load_error)
{
  GtkWidget     *dialog;
  GtkWidget     *frame;
  GtkWidget     *main_hbox;
  GtkWidget     *vbox;
  GtkWidget     *image;
  GdkPixbuf     *preview;
  GtkWidget     *label;
  gboolean       run;
  GError        *error = NULL;
  gchar         *text;
  gboolean       allow_retry = FALSE;
  SvgLoadVals    vals  =
  {
    SVG_DEFAULT_RESOLUTION,
    SVG_PREVIEW_SIZE,
    SVG_PREVIEW_SIZE,
  };

  preview = load_rsvg_pixbuf (file, SVG_PREVIEW_SIZE, SVG_PREVIEW_SIZE, SVG_DEFAULT_RESOLUTION,
                              *rsvg_flags, &allow_retry, &error);

  gimp_ui_init (PLUG_IN_BINARY);

  if (! preview && *rsvg_flags == RSVG_HANDLE_FLAGS_NONE && allow_retry)
    {
      /* We need to ask explicitly before using the "unlimited" size
       * option (XML_PARSE_HUGE in libxml) because it is considered
       * unsafe, possibly consumming too much memory with malicious XML
       * files.
       */
      dialog = gimp_dialog_new (_("Disable safety size limits?"),
                                PLUG_IN_ROLE,
                                NULL, 0,
                                gimp_standard_help_func, LOAD_PROC,

                                _("_No"),  GTK_RESPONSE_NO,
                                _("_Yes"), GTK_RESPONSE_YES,

                                NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_NO);
      gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
      gimp_window_set_transient (GTK_WINDOW (dialog));

      main_hbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
      gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                          main_hbox, TRUE, TRUE, 0);
      gtk_widget_show (main_hbox);

      /* Unfortunately the error returned by librsvg is unclear. While
       * libxml explicitly returns a "parser error : internal error:
       * Huge input lookup", librsvg does not seem to relay this error.
       * It sends a further parsing error, false positive provoked by
       * the huge input error.
       * If we were able to single out the huge data error, we could
       * just directly return from the plug-in as a failure run in other
       * cases. Instead of this, we need to ask each and every time, in
       * case it might be the huge data error.
       */
      label = gtk_label_new (_("A parsing error occurred.\n"
                               "Disabling safety limits may help. "
                               "Malicious SVG files may use this to consume too much memory."));
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_label_set_line_wrap_mode (GTK_LABEL (label), PANGO_WRAP_WORD);
      gtk_label_set_max_width_chars (GTK_LABEL (label), 80);
      gtk_box_pack_start (GTK_BOX (main_hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      label = gtk_label_new (NULL);
      text = g_strdup_printf ("<b>%s</b>",
                              _("For security reasons, this should only be used for trusted input!"));
      gtk_label_set_markup (GTK_LABEL (label), text);
      g_free (text);
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
      gtk_box_pack_start (GTK_BOX (main_hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("Retry without limits preventing to parse huge data?"));
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
      gtk_box_pack_start (GTK_BOX (main_hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      gtk_widget_show (dialog);

      run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_YES);
      gtk_widget_destroy (dialog);

      if (run)
        {
          *rsvg_flags = RSVG_HANDLE_FLAG_UNLIMITED;
          g_clear_error (&error);
          preview = load_rsvg_pixbuf (file, SVG_PREVIEW_SIZE, SVG_PREVIEW_SIZE, SVG_DEFAULT_RESOLUTION,
                                      *rsvg_flags, NULL, &error);
        }
    }

  if (! preview)
    {
      /*  Do not rely on librsvg setting GError on failure!  */
      g_set_error (load_error,
                   error ? error->domain : 0, error ? error->code : 0,
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file),
                   error ? error->message : _("Unknown reason"));
      g_clear_error (&error);

      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* Scalable Vector Graphics is SVG, should perhaps not be translated */
  dialog = gimp_procedure_dialog_new (GIMP_PROCEDURE (procedure),
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Render Scalable Vector Graphics"));

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "vbox-arguments",
                                  "width", "height", "resolution", "paths",
                                  NULL);
  main_hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog), "main-hbox",
                                              "vbox-arguments", NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (main_hbox), GTK_ORIENTATION_HORIZONTAL);

  /*  The SVG preview  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  image = gtk_image_new_from_pixbuf (preview);
  gtk_container_add (GTK_CONTAINER (frame), image);
  gtk_widget_show (image);

  size_label = gtk_label_new (NULL);
  gtk_label_set_justify (GTK_LABEL (size_label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), size_label, TRUE, TRUE, 4);
  gtk_widget_show (size_label);

  /*  query the initial size after the size label is created  */
  g_object_get (config, "resolution", &vals.resolution, NULL);

  load_rsvg_size (file, &vals, *rsvg_flags, NULL);
  g_object_set (config,
                "width",  vals.width,
                "height", vals.height,
                NULL);

  /* Complete the dialog. */
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), "main-hbox", NULL);
  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run ? GIMP_PDB_SUCCESS : GIMP_PDB_CANCEL;
}

static void
svg_destroy_surface (guchar          *pixels,
                     cairo_surface_t *surface)
{
  cairo_surface_destroy (surface);
}
