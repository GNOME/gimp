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

#include <glib/gstdio.h>
#include <librsvg/rsvg.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC               "file-svg-load"
#define EXPORT_PROC             "file-svg-export"
#define PLUG_IN_BINARY          "file-svg"
#define PLUG_IN_ROLE            "gimp-file-svg"
#define SVG_VERSION             "2.5.0"
#define SVG_DEFAULT_RESOLUTION  90.0
#define SVG_DEFAULT_SIZE        500
#define SVG_PREVIEW_SIZE        128

#define EPSILON 1e-6


typedef struct
{
  gdouble    resolution;
  gint       width;
  gint       height;
} SvgLoadVals;

typedef enum
{
  EXPORT_FORMAT_NONE,
  EXPORT_FORMAT_PNG,
  EXPORT_FORMAT_JPEG
} EXPORT_FORMAT;

typedef enum
{
  LAYER_ID_VECTOR,
  LAYER_ID_GROUP,
  LAYER_ID_TEXT,
  LAYER_ID_LINK,
  LAYER_ID_RASTER,
  LAYER_ID_MASK,
} LAYER_ID;

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

static gboolean        svg_extract           (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GFile                 *file,
                                              GimpMetadata          *metadata,
                                              GimpProcedureConfig   *config,
                                              GimpVectorLoadData    *extracted_dimensions,
                                              gpointer              *data_for_run,
                                              GDestroyNotify        *data_for_run_destroy,
                                              gpointer               extract_data,
                                              GError               **error);
static GimpValueArray * svg_load             (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GFile                 *file,
                                              gint                   width,
                                              gint                   height,
                                              GimpVectorLoadData     extracted_data,
                                              GimpMetadata          *metadata,
                                              GimpMetadataLoadFlags *flags,
                                              GimpProcedureConfig   *config,
                                              gpointer               data_from_extract,
                                              gpointer               run_data);
static GimpValueArray * svg_export           (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GimpImage             *image,
                                              GFile                 *file,
                                              GimpExportOptions     *options,
                                              GimpMetadata          *metadata,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);

static GimpImage         * load_image        (GFile                 *file,
                                              RsvgHandle            *handle,
                                              gint                   width,
                                              gint                   height,
                                              gdouble                resolution,
                                              GError               **error);
static gboolean            export_image      (GFile                 *file,
                                              GimpProcedureConfig   *config,
                                              GimpImage             *image,
                                              GError               **error);

static GdkPixbuf         * load_rsvg_pixbuf  (RsvgHandle            *handle,
                                              gint                   width,
                                              gint                   height,
                                              gdouble                resolution,
                                              GError               **error);
static GimpPDBStatusType   load_dialog       (GFile                 *file,
                                              RsvgHandle            *handle,
                                              GimpProcedure         *procedure,
                                              GimpProcedureConfig   *config,
                                              GimpVectorLoadData     extracted_data,
                                              GError               **error);
static gboolean            export_dialog     (GimpImage             *image,
                                              GimpProcedure         *procedure,
                                              GObject               *config);
static gboolean            has_invalid_links (GimpLayer            **layers,
                                              GimpGroupLayer        *group);

static gboolean       svg_extract_dimensions (RsvgHandle            *handle,
                                              GimpVectorLoadData    *extracted_dimensions);

/* Taken from /app/path/gimppath-export.c */
static GString     * svg_export_file         (GimpImage             *image,
                                              GimpProcedureConfig   *config,
                                              GError               **error);
static void          svg_export_image_size   (GimpImage             *image,
                                              GString               *str);
static void          svg_export_defs         (GimpItem             **layers,
                                              GimpGroupLayer        *group,
                                              GString               *str,
                                              GimpProcedureConfig   *config,
                                              GList                **exported_res,
                                              GError               **error);
static void          svg_export_def          (GimpVectorLayer       *layer,
                                              GString               *str,
                                              GimpProcedureConfig   *config,
                                              GList                **exported_res,
                                              GError               **error);
static gchar       * svg_get_def_string      (GimpVectorLayer       *layer,
                                              const gchar           *type,
                                              GimpProcedureConfig   *config,
                                              GList                **exported_res,
                                              GError               **error);
static void          svg_export_layers       (GimpItem             **layers,
                                              GimpGroupLayer        *group,
                                              gint                  *layer_ids,
                                              GString               *str,
                                              GimpProcedureConfig   *config,
                                              gchar                 *spacing,
                                              GError               **error);
static void          svg_export_group_header (GimpGroupLayer        *group,
                                              gint                   group_id,
                                              GString               *str,
                                              gchar                 *spacing);
static void          svg_export_path         (GimpVectorLayer       *layer,
                                              gint                   vector_id,
                                              GString               *str,
                                              gchar                 *spacing);
static gchar       * svg_export_path_data    (GimpPath              *paths);
static void          svg_export_text         (GimpTextLayer         *layer,
                                              gint                   text_id,
                                              GString               *str,
                                              gchar                 *spacing);
static void          svg_export_text_lines   (GimpTextLayer         *layer,
                                              GString               *str,
                                              gchar                 *text);
static void          svg_export_link_layer   (GimpLinkLayer         *layer,
                                              gint                   link_id,
                                              GString               *str,
                                              gchar                 *spacing);
static void          svg_export_raster       (GimpDrawable          *layer,
                                              gint                  *layer_ids,
                                              GString               *str,
                                              gint                   format_id,
                                              gchar                 *spacing,
                                              GError               **error);
static void          svg_export_layer_mask   (GimpLayerMask         *mask,
                                              gint                  *layer_ids,
                                              GString               *str,
                                              gchar                 *spacing,
                                              GError               **error);

static gchar       * svg_get_color_string    (GimpVectorLayer       *layer,
                                              const gchar           *type);
static gchar       * svg_get_hex_color       (GeglColor             *color);

#if LIBRSVG_CHECK_VERSION(2, 46, 0)
static GimpUnit      * svg_rsvg_to_gimp_unit (RsvgUnit               unit);
static void              svg_destroy_surface (guchar                *pixels,
                                              cairo_surface_t       *surface);
#endif



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

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static GimpProcedure *
svg_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_vector_load_procedure_new (plug_in, name,
                                                  GIMP_PDB_PROC_TYPE_PLUGIN,
                                                  svg_extract, NULL, NULL,
                                                  svg_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("SVG image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Loads files in SVG file format"),
                                        /* TODO: let's localize after we
                                         * add support for vector and/or
                                         * text and link layers:
                                         * _("Loads SVG (Scalable Vector Graphic) files")
                                         */
                                        "Renders SVG files to raster graphics "
                                        "using librsvg.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Dom Lachowicz, Sven Neumann",
                                      "Dom Lachowicz <cinamod@hotmail.com>",
                                      SVG_VERSION);

      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("SVG"));
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/svg+xml");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "svg");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,<?xml,0,string,<svg");

      gimp_procedure_add_choice_argument (procedure, "paths",
                                          _("_Paths"),
                                          _("Whether and how to import paths so that they can be used with the path tool"),
                                          gimp_choice_new_with_values ("no-import",     0, _("Don't import paths"),        NULL,
                                                                       "import",        1, _("Import paths individually"), NULL,
                                                                       "import-merged", 2, _("Merge imported paths"),      NULL,
                                                                       NULL),
                                          "no-import", G_PARAM_READWRITE);
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, svg_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("Scalable Vector Graphic"));

      gimp_procedure_set_documentation (procedure,
                                        _("Exports files in SVG file format"),
                                        _("Exports files in SVG file format "
                                          "(Scalable Vector Graphic)"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2025");

      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           "SVG");
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/svg+xml");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "svg");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB     |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                              GIMP_EXPORT_CAN_HANDLE_ALPHA   |
                                              GIMP_EXPORT_CAN_HANDLE_LAYERS  |
                                              GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS,
                                              NULL, NULL, NULL);

      gimp_procedure_add_string_argument (procedure, "title",
                                          _("_Title"),
                                          _("Optional <title> for SVG"),
                                          NULL,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "raster-export-format",
                                          _("Em_bed raster layers as"),
                                          NULL,
                                          gimp_choice_new_with_values ("png",  EXPORT_FORMAT_PNG,  _("PNG"),                         NULL,
                                                                       "jpeg", EXPORT_FORMAT_JPEG, _("JPEG"),                        NULL,
                                                                       "none", EXPORT_FORMAT_NONE, _("Do not export raster layers"), NULL,
                                                                       NULL),
                                          "png",
                                          G_PARAM_READWRITE);
    }

  return procedure;
}

static gboolean
svg_extract (GimpProcedure        *procedure,
             GimpRunMode           run_mode,
             GFile                *file,
             GimpMetadata         *metadata,
             GimpProcedureConfig  *config,
             GimpVectorLoadData   *extracted_dimensions,
             gpointer             *data_for_run,
             GDestroyNotify       *data_for_run_destroy,
             gpointer              extract_data,
             GError              **error)
{
  RsvgHandle *handle;

  g_return_val_if_fail (extracted_dimensions != NULL, FALSE);
  g_return_val_if_fail (data_for_run_destroy != NULL && *data_for_run_destroy == NULL, FALSE);
  g_return_val_if_fail (data_for_run != NULL && *data_for_run == NULL, FALSE);
  g_return_val_if_fail (error != NULL && *error == NULL, FALSE);

  handle = rsvg_handle_new_from_gfile_sync (file, RSVG_HANDLE_FLAGS_NONE, NULL, error);

  if (! handle)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          GtkWidget *dialog;
          GtkWidget *main_hbox;
          GtkWidget *label;
          gchar     *text;
          gboolean   retry;

          gimp_ui_init (PLUG_IN_BINARY);

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

          retry = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_YES);
          gtk_widget_destroy (dialog);

          if (retry)
            {
              g_clear_error (error);
              handle = rsvg_handle_new_from_gfile_sync (file, RSVG_HANDLE_FLAG_UNLIMITED, NULL, error);
            }
        }

      if (! handle)
        return FALSE;
    }

  if (! svg_extract_dimensions (handle, extracted_dimensions))
    {
      g_object_unref (handle);
      return FALSE;
    }

  *data_for_run = handle;
  *data_for_run_destroy = g_object_unref;

  return TRUE;
}

static gboolean
svg_extract_dimensions (RsvgHandle         *handle,
                        GimpVectorLoadData *extracted_dimensions)
{
#if LIBRSVG_CHECK_VERSION(2, 46, 0)
  gboolean           out_has_width;
  RsvgLength         out_width;
  gboolean           out_has_height;
  RsvgLength         out_height;
  gboolean           out_has_viewbox;
  RsvgRectangle      out_viewbox;
#endif
#if ! LIBRSVG_CHECK_VERSION(2, 52, 0)
  RsvgDimensionData  dim;
#endif

  /* SVG has no pixel density information */
  extracted_dimensions->exact_density = TRUE;
  extracted_dimensions->correct_ratio = TRUE;

#if LIBRSVG_CHECK_VERSION(2, 46, 0)
  rsvg_handle_get_intrinsic_dimensions (handle,
                                        &out_has_width,   &out_width,
                                        &out_has_height,  &out_height,
                                        &out_has_viewbox, &out_viewbox);

  if (out_has_width && out_has_height)
    {
      /* Starting in librsvg 2.54, rsvg_handle_get_intrinsic_dimensions ()
       * always returns TRUE for width/height. If width/height default to 100%,
       * we should check the viewport dimensions */
      gdouble viewbox_width  = 0;
      gdouble viewbox_height = 0;

      if (out_has_viewbox)
        {
          viewbox_width  = out_viewbox.width - out_viewbox.x;
          viewbox_height = out_viewbox.height - out_viewbox.y;
        }

      /* "width" and "height" present */
      if (svg_rsvg_to_gimp_unit (out_width.unit)  != NULL &&
          svg_rsvg_to_gimp_unit (out_height.unit) != NULL)
        {
          /* Best case scenario: we know all these units. */
          extracted_dimensions->width        = out_width.length;
          extracted_dimensions->width_unit   = svg_rsvg_to_gimp_unit (out_width.unit);
          extracted_dimensions->height       = out_height.length;
          extracted_dimensions->height_unit  = svg_rsvg_to_gimp_unit (out_height.unit);

          if ((out_width.length == 1.0 && out_width.unit == RSVG_UNIT_PERCENT) &&
              (out_height.length == 1.0 && out_height.unit == RSVG_UNIT_PERCENT))
            {
              extracted_dimensions->width        = viewbox_width;
              extracted_dimensions->height       = viewbox_height;
              extracted_dimensions->exact_width  = FALSE;
              extracted_dimensions->exact_height = FALSE;
            }
          else
            {
              extracted_dimensions->exact_width  = TRUE;
              extracted_dimensions->exact_height = TRUE;
            }
        }
      else
        {
          /* Ideally we let GIMP handle the transformation, but for cases where
           * we don't know how, we let rsvg make a computation to pixel for us.
           */
#if LIBRSVG_CHECK_VERSION(2, 52, 0)
          if (rsvg_handle_get_intrinsic_size_in_pixels (handle,
                                                        &extracted_dimensions->width,
                                                        &extracted_dimensions->height))
            {
              extracted_dimensions->width_unit   = gimp_unit_pixel ();
              extracted_dimensions->height_unit  = gimp_unit_pixel ();
              extracted_dimensions->exact_width  = FALSE;
              extracted_dimensions->exact_height = FALSE;
            }
          else
            {
              /* Honestly at this point, setting 300 PPI is random. */
              rsvg_handle_set_dpi (handle, 300.0);
              if (rsvg_handle_get_intrinsic_size_in_pixels (handle,
                                                            &extracted_dimensions->width,
                                                            &extracted_dimensions->height))
                {
                  extracted_dimensions->width_unit  = gimp_unit_percent ();
                  extracted_dimensions->height_unit = gimp_unit_percent ();
                }
              else if (out_width.unit == out_height.unit)
                {
                  /* Odd case which should likely never happen as we have width,
                   * height and a (fake) pixel density. Just in case though.
                   */
                  extracted_dimensions->width       = out_width.length;
                  extracted_dimensions->height      = out_height.length;
                  extracted_dimensions->width_unit  = gimp_unit_percent ();
                  extracted_dimensions->height_unit = gimp_unit_percent ();
                }
              else
                {
                  /* Should even less happen! */
                  return FALSE;
                }
            }
#else
          rsvg_handle_get_dimensions (handle, &dim);
          extracted_dimensions->width         = (gdouble) dim.width;
          extracted_dimensions->height        = (gdouble) dim.height;
          extracted_dimensions->exact_width   = FALSE;
          extracted_dimensions->exact_height  = FALSE;
          extracted_dimensions->correct_ratio = TRUE;
#endif
        }
    }
  else if (out_has_viewbox)
    {
      /* Only a viewbox, so dimensions have a ratio, but no units. */
      extracted_dimensions->width         = out_width.length;
      extracted_dimensions->width_unit    = gimp_unit_percent ();
      extracted_dimensions->exact_width   = FALSE;
      extracted_dimensions->height        = out_height.length;
      extracted_dimensions->height_unit   = gimp_unit_percent ();
      extracted_dimensions->exact_height  = FALSE;
      extracted_dimensions->correct_ratio = TRUE;
    }
  else
    {
      /* Neither width nor viewbox. Nothing can be determined. */
      return FALSE;
    }

#else
  rsvg_handle_get_dimensions (handle, &dim);
  extracted_dimensions->width         = (gdouble) dim.width;
  extracted_dimensions->height        = (gdouble) dim.height;
  extracted_dimensions->exact_width   = FALSE;
  extracted_dimensions->exact_height  = FALSE;
  extracted_dimensions->correct_ratio = TRUE;
#endif

  return TRUE;
}

static GimpValueArray *
svg_load (GimpProcedure         *procedure,
          GimpRunMode            run_mode,
          GFile                 *file,
          gint                   width,
          gint                   height,
          GimpVectorLoadData     extracted_data,
          GimpMetadata          *metadata,
          GimpMetadataLoadFlags *flags,
          GimpProcedureConfig   *config,
          gpointer               data_from_extract,
          gpointer               run_data)
{
  GimpValueArray    *return_vals;
  GimpImage         *image;
  GError            *error      = NULL;
  GimpPDBStatusType  status;
  gchar             *import_paths;
  gdouble            resolution;

  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      status = load_dialog (file, RSVG_HANDLE (data_from_extract),
                            procedure, config, extracted_data, &error);
      if (status != GIMP_PDB_SUCCESS)
        return gimp_procedure_new_return_values (procedure, status, error);
    }

  g_object_get (config,
                "width",         &width,
                "height",        &height,
                "pixel-density", &resolution,
                NULL);
  image = load_image (file, RSVG_HANDLE (data_from_extract),
                      width, height, resolution, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  g_object_get (config, "paths", &import_paths, NULL);
  if (g_strcmp0 (import_paths, "no-import") != 0)
    {
      GimpPath **paths = NULL;

      gimp_image_import_paths_from_file (image, file,
                                         g_strcmp0 (import_paths, "import-merged") == 0,
                                         TRUE, &paths);
      g_free (paths);
    }
  g_free (import_paths);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
svg_export (GimpProcedure       *procedure,
            GimpRunMode          run_mode,
            GimpImage           *image,
            GFile               *file,
            GimpExportOptions   *options,
            GimpMetadata        *metadata,
            GimpProcedureConfig *config,
            gpointer             run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_IGNORE;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_ui_init (PLUG_IN_BINARY);

      if (! export_dialog (image, procedure, G_OBJECT (config)))
        status = GIMP_PDB_CANCEL;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      export = gimp_export_options_get_image (options, &image);

      if (! export_image (file, config, image, &error))
        status = GIMP_PDB_EXECUTION_ERROR;

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image);
    }

  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpImage *
load_image (GFile            *file,
            RsvgHandle       *handle,
            gint              width,
            gint              height,
            gdouble           resolution,
            GError          **load_error)
{
  GimpImage *image;
  GimpLayer *layer;
  GdkPixbuf *pixbuf;
  GError    *error = NULL;

  pixbuf = load_rsvg_pixbuf (handle, width, height, resolution, &error);

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

static gboolean
export_image (GFile                *file,
              GimpProcedureConfig  *config,
              GimpImage            *image,
              GError              **error)
{
  GOutputStream *output;
  GString       *string;

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (! output)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Writing to file '%s' failed: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  string = svg_export_file (image, config, error);

  if (! g_output_stream_write_all (output, string->str, string->len,
                                   NULL, NULL, error))
    {
      GCancellable *cancellable = g_cancellable_new ();

      if (error)
        g_clear_error (error);

      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Writing to file '%s' failed: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      g_string_free (string, TRUE);

      /* Cancel the overwrite initiated by g_file_replace(). */
      g_cancellable_cancel (cancellable);
      g_output_stream_close (output, cancellable, NULL);
      g_object_unref (cancellable);
      g_object_unref (output);

      return FALSE;
    }

  g_string_free (string, TRUE);
  g_object_unref (output);

  return TRUE;
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
load_rsvg_pixbuf (RsvgHandle  *handle,
                  gint         width,
                  gint         height,
                  gdouble      resolution,
                  GError     **error)
{
  GdkPixbuf  *pixbuf  = NULL;

#if LIBRSVG_CHECK_VERSION(2, 46, 0)
  cairo_surface_t   *surface  = NULL;
  cairo_t           *cr       = NULL;
  RsvgRectangle      viewport = { 0, };
  guchar            *src;
  gint               y;
  /* Aspect-ratio breaking is only implemented with newer librsvg API. */
  GimpVectorLoadData dimensions;
  gdouble            x_scale       = 1.0;
  gdouble            y_scale       = 1.0;
  gdouble            aspect_width  = width;
  gdouble            aspect_height = height;
#else
  SvgLoadVals      vals;
#endif

  g_return_val_if_fail (handle, NULL);

  rsvg_handle_set_dpi (handle, resolution);
#if LIBRSVG_CHECK_VERSION(2, 46, 0)
  if (svg_extract_dimensions (handle, &dimensions))
    {
      if (fabs ((gdouble) dimensions.width / dimensions.height - (gdouble) width / height) < EPSILON)
        {
          rsvg_handle_set_dpi (handle, resolution);
        }
      else if ((gdouble) dimensions.width / dimensions.height > (gdouble) width / height)
        {
          aspect_height = (gdouble) width * dimensions.height / dimensions.width;
          /* XXX The X/Y resolution doesn't change a thing regarding
           * rendering unfortunately so we still have to scale when the
           * aspect ratio is not kept. I still set these, just in case.
           */
          rsvg_handle_set_dpi_x_y (handle, resolution, aspect_height / height * resolution);
          y_scale = height / aspect_height;
        }
      else
        {
          aspect_width = (gdouble) height * dimensions.width / dimensions.height;
          rsvg_handle_set_dpi_x_y (handle, aspect_width / width * resolution, resolution);
          x_scale = width / aspect_width;
        }
    }

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr      = cairo_create (surface);

  viewport.width  = (double) aspect_width;
  viewport.height = (double) aspect_height;

  if (x_scale != 1.0 || y_scale != 1.0)
    cairo_scale (cr, x_scale, y_scale);

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
  rsvg_handle_set_size_callback (handle, load_set_size_callback, (gpointer) &vals, NULL);
  pixbuf = rsvg_handle_get_pixbuf (handle);
#endif

  return pixbuf;
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
             RsvgHandle           *handle,
             GimpProcedure        *procedure,
             GimpProcedureConfig  *config,
             GimpVectorLoadData    extracted_data,
             GError              **load_error)
{
  GtkWidget *dialog;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_vector_load_procedure_dialog_new (GIMP_VECTOR_LOAD_PROCEDURE (procedure),
                                                  GIMP_PROCEDURE_CONFIG (config),
                                                  &extracted_data, file);
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), NULL);
  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run ? GIMP_PDB_SUCCESS : GIMP_PDB_CANCEL;
}

static gboolean
export_dialog (GimpImage     *image,
               GimpProcedure *procedure,
               GObject       *config)
{
  GtkWidget *dialog;
  GtkWidget *box;
  GtkWidget *hint = NULL;
  gboolean   run;
  gboolean   has_nonstandard_links = FALSE;

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  box = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                        "svg-box", "title",
                                        "raster-export-format", NULL);

  has_nonstandard_links = has_invalid_links (gimp_image_get_layers (image),
                                             NULL);
  if (has_nonstandard_links)
    {
      hint =
        g_object_new (GIMP_TYPE_HINT_BOX,
                      "icon-name", GIMP_ICON_DIALOG_WARNING,
                      "hint",      _("The SVG format only requires viewers to "
                                     "display image links for SVG, PNG, and "
                                     "JPEG images.\n"
                                     "Your image links external image files "
                                     "in other formats which may not show up "
                                     "in all viewers."),
                      NULL);
      gtk_widget_set_visible (hint, TRUE);
      gtk_widget_set_margin_bottom (hint, 12);

      gtk_box_pack_start (GTK_BOX (box), hint, FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (box), hint, 0);
    }

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), "svg-box", NULL);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static gboolean
has_invalid_links (GimpLayer      **layers,
                   GimpGroupLayer  *group)
{
  GimpItem **temp_layers = (GimpItem **) layers;
  gint32     n_layers;

  if (layers == NULL)
    temp_layers = gimp_item_get_children (GIMP_ITEM (group));

  n_layers = gimp_core_object_array_get_length ((GObject **) temp_layers);

  for (gint i = 0; i < n_layers; i++)
    {
      if (gimp_item_is_link_layer (temp_layers[i]))
        {
          gchar *mimetype =
            gimp_link_layer_get_mime_type (GIMP_LINK_LAYER (temp_layers[i]));

          /* The SVG specification only requires viewers to support
             SVG, PNG, and JPEG links. If the linked layer is not one of
             those types, we'll notify the user */
          if (g_strcmp0 (mimetype, "image/svg+xml") != 0 &&
              g_strcmp0 (mimetype, "image/png") != 0 &&
              g_strcmp0 (mimetype, "image/jpeg") != 0)
            return TRUE;

        }
      else if (gimp_item_is_group (GIMP_ITEM (temp_layers[i])))
        {
          if (has_invalid_links (NULL, GIMP_GROUP_LAYER (temp_layers[i])))
            return TRUE;
        }
    }

  return FALSE;
}

#if LIBRSVG_CHECK_VERSION(2, 46, 0)
static GimpUnit *
svg_rsvg_to_gimp_unit (RsvgUnit unit)
{
  switch (unit)
    {
    case RSVG_UNIT_PERCENT:
      return gimp_unit_percent ();
    case RSVG_UNIT_PX:
      return gimp_unit_pixel ();
    case RSVG_UNIT_IN:
      return gimp_unit_inch ();
    case RSVG_UNIT_MM:
      return gimp_unit_mm ();
    case RSVG_UNIT_PT:
      return gimp_unit_point ();
    case RSVG_UNIT_PC:
      return gimp_unit_pica ();
    case RSVG_UNIT_CM:
    case RSVG_UNIT_EM:
    case RSVG_UNIT_EX:
      /*case RSVG_UNIT_CH:*/
    default:
      return NULL;
    }
}

static void
svg_destroy_surface (guchar          *pixels,
                     cairo_surface_t *surface)
{
  cairo_surface_destroy (surface);
}
#endif

/* Taken from app/path/gimppath-export.c */
static GString *
svg_export_file (GimpImage            *image,
                 GimpProcedureConfig  *config,
                 GError              **error)
{
  GimpLayer **layers;
  gchar      *title        = NULL;
  GString    *str          = g_string_new (NULL);
  GList      *exported_res = NULL;
  gint        layer_ids[6] = { 0 };

  g_object_get (config, "title", &title, NULL);

  g_string_append_printf (str,
                          "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
                          "<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
                          "     xmlns:svg=\"http://www.w3.org/2000/svg\"\n"
                          "     version=\"1.1\"\n");

  g_string_append (str, "     ");
  svg_export_image_size (image, str);
  g_string_append_c (str, '\n');

  g_string_append_printf (str,
                          "     viewBox=\"0 0 %d %d\">\n",
                          gimp_image_get_width  (image),
                          gimp_image_get_height (image));

  /* Optional SVG title */
  if (title && strlen (title))
    g_string_append_printf (str,
                            "  <title>%s</title>\n",
                            title);

  layers = gimp_image_get_layers (image);
  g_string_append (str, "  <defs>\n");
  svg_export_defs ((GimpItem **) layers, NULL, str, config, &exported_res, error);
  g_list_free (exported_res);
  g_string_append (str, "  </defs>\n");
  svg_export_layers ((GimpItem **) layers, NULL, layer_ids, str, config, "", error);
  g_free (layers);

  g_string_append (str, "</svg>\n");

  g_free (title);

  return str;
}

static void
svg_export_defs (GimpItem             **layers,
                 GimpGroupLayer        *group,
                 GString               *str,
                 GimpProcedureConfig   *config,
                 GList                **exported_res,
                 GError               **error)
{
  GimpItem **items;
  gint32     n_layers;

  if (group)
    items = gimp_item_get_children (GIMP_ITEM (group));
  else
    items = layers;

  n_layers = gimp_core_object_array_get_length ((GObject **) items);

  for (gint i = n_layers - 1; i >= 0; i--)
    {
      if (gimp_item_get_visible (items[i]))
        {
          if (gimp_item_is_group (items[i]))
            svg_export_defs (NULL, GIMP_GROUP_LAYER (items[i]), str, config, exported_res, error);
          else if (GIMP_IS_VECTOR_LAYER (items[i]) &&
                   ! gimp_rasterizable_is_rasterized (GIMP_RASTERIZABLE (items[i])))
            svg_export_def (GIMP_VECTOR_LAYER (items[i]), str, config, exported_res, error);
        }
      gimp_progress_update ((gdouble) (n_layers - i) / n_layers);

      if (*error)
        break;
    }
  if (group)
    g_free (items);

  gimp_progress_update (1.0);
}

static void
svg_export_def (GimpVectorLayer      *layer,
                GString              *str,
                GimpProcedureConfig  *config,
                GList               **exported_res,
                GError              **error)
{
  gchar *fill_string;
  gchar *stroke_string;

  fill_string   = svg_get_def_string (layer, "fill", config, exported_res, error);
  stroke_string = svg_get_def_string (layer, "stroke", config, exported_res, error);

  g_string_append_printf (str,
                          "%s%s",
                          fill_string ? fill_string : "",
                          stroke_string ? stroke_string : "");

  g_free (fill_string);
  g_free (stroke_string);
}

static gchar *
svg_get_def_string (GimpVectorLayer      *layer,
                    const gchar          *type,
                    GimpProcedureConfig  *config,
                    GList               **exported_res,
                    GError              **error)
{
  GimpPattern *pattern = NULL;
  gboolean     enabled;

  if (g_strcmp0 ("fill", type) == 0)
    {
      enabled = gimp_vector_layer_get_enable_fill (layer);
      if (enabled)
        pattern = gimp_vector_layer_get_fill_pattern (layer);
    }
  else
    {
      enabled = gimp_vector_layer_get_enable_stroke (layer);
      if (enabled)
        pattern = gimp_vector_layer_get_stroke_pattern (layer);
    }

  if (enabled && pattern && ! g_list_find (*exported_res, pattern))
    {
      gchar          *def_string = NULL;
      GFile          *temp_file  = NULL;
      FILE           *temp_fp;
      gsize           temp_size;
      GimpProcedure  *procedure;
      GimpValueArray *return_vals;
      GimpImage      *temp_image;
      GimpLayer      *temp_layer;
      const gchar    *mimetype;
      gint            width;
      gint            height;
      gint            format_id;

      GeglBuffer     *src_buffer;
      GeglBuffer     *dst_buffer;
      gint            res_id;

      res_id     = gimp_resource_get_id (GIMP_RESOURCE (pattern));
      /* XXX Should we try to output in high-bit depth when possible? */
      src_buffer = gimp_pattern_get_buffer (pattern, 0, 0, babl_format ("R'G'B' u8"));
      width      = gegl_buffer_get_width (src_buffer);
      height     = gegl_buffer_get_height (src_buffer);
      temp_image = gimp_image_new (width, height, GIMP_RGB);
      temp_layer = gimp_layer_new (temp_image, NULL, width, height, GIMP_RGBA_IMAGE,
                                   100.0, GIMP_LAYER_MODE_NORMAL);
      gimp_image_insert_layer (temp_image, temp_layer, NULL, 0);
      dst_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (temp_layer));
      gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE, dst_buffer, NULL);

      g_object_unref (src_buffer);
      g_object_unref (dst_buffer);

      format_id = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                       "raster-export-format");
      if (format_id == EXPORT_FORMAT_PNG)
        {
          temp_file = gimp_temp_file ("png");
          mimetype  = "image/png";

          procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (), "file-png-export");
          return_vals = gimp_procedure_run (procedure,
                                            "run-mode",              GIMP_RUN_NONINTERACTIVE,
                                            "image",                 temp_image,
                                            "file",                  temp_file,
                                            "interlaced",            FALSE,
                                            "compression",           9,
                                            "bkgd",                  FALSE,
                                            "offs",                  FALSE,
                                            "phys",                  FALSE,
                                            "time",                  FALSE,
                                            "save-transparent",      FALSE,
                                            "optimize-palette",      FALSE,
                                            "include-color-profile", FALSE,
                                            NULL);
        }
      else
        {
          temp_file = gimp_temp_file ("jpeg");
          mimetype  = "image/jpeg";

          procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (), "file-jpeg-export");
          return_vals = gimp_procedure_run (procedure,
                                            "run-mode",              GIMP_RUN_NONINTERACTIVE,
                                            "image",                 temp_image,
                                            "file",                  temp_file,
                                            "quality",               0.9f,
                                            "cmyk",                  FALSE,
                                            "include-color-profile", FALSE,
                                            NULL);
        }
      gimp_image_delete (temp_image);

      if (GIMP_VALUES_GET_ENUM (return_vals, 0) != GIMP_PDB_SUCCESS)
        {
          if (! error)
            {
              if (G_VALUE_HOLDS_STRING (gimp_value_array_index (return_vals, 1)))
                g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                             "SVG: Unable to export pattern image data: %s",
                             GIMP_VALUES_GET_STRING (return_vals, 1));
              else
                g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                     "SVG: Unable to export pattern image data.");
            }

          g_object_unref (temp_file);

          return NULL;
        }

      temp_fp = g_fopen (g_file_peek_path (temp_file), "rb");
      fseek (temp_fp, 0L, SEEK_END);
      temp_size = ftell (temp_fp);
      fseek (temp_fp, 0L, SEEK_SET);

      if (temp_size > 0)
        {
          guchar *buf;
          gchar  *encoded;

          buf = g_malloc0 (temp_size);
          fread (buf, 1, temp_size, temp_fp);

          encoded = g_base64_encode ((const guchar *) buf, temp_size + 1);

          def_string = g_strdup_printf ("    <pattern id=\"Pattern%d\"\n"
                                        "             patternUnits=\"userSpaceOnUse\"\n"
                                        "             width=\"%d\"\n"
                                        "             height=\"%d\">\n"
                                        "      <image href=\"data:%s;base64,%s\" />\n"
                                        "    </pattern>\n",
                                        res_id, width, height, mimetype, encoded);

          g_free (encoded);
          g_free (buf);
        }

      fclose (temp_fp);
      g_file_delete (temp_file, NULL, NULL);
      g_object_unref (temp_file);

      *exported_res = g_list_prepend (*exported_res, pattern);

      return def_string;
    }

  return NULL;
}

static void
svg_export_layers (GimpItem             **layers,
                   GimpGroupLayer        *group,
                   gint                  *layer_ids,
                   GString               *str,
                   GimpProcedureConfig   *config,
                   gchar                 *spacing,
                   GError               **error)
{
  GimpItem **items;
  gint32     n_layers;
  gchar     *extra_spacing;
  gint       format_id;

  if (group)
    items = gimp_item_get_children (GIMP_ITEM (group));
  else
    items = layers;

  format_id = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                   "raster-export-format");

  extra_spacing = g_strdup_printf ("%s  ", spacing);

  n_layers = gimp_core_object_array_get_length ((GObject **) items);

  for (gint i = n_layers - 1; i >= 0; i--)
    {
      if (gimp_item_get_visible (items[i]))
        {
          if (gimp_item_is_group (items[i]))
            {
              svg_export_group_header (GIMP_GROUP_LAYER (items[i]),
                                       layer_ids[LAYER_ID_GROUP]++, str,
                                       extra_spacing);
              svg_export_layers (NULL, GIMP_GROUP_LAYER (items[i]), layer_ids,
                                 str, config, extra_spacing, error);
              g_string_append_printf (str, "%s</g>\n", extra_spacing);
            }
          else if (GIMP_IS_RASTERIZABLE (items[i]) &&
                   ! gimp_rasterizable_is_rasterized (GIMP_RASTERIZABLE (items[i])))
            {
              if (GIMP_IS_VECTOR_LAYER (items[i]))
                {
                  GimpVectorLayer *layer = GIMP_VECTOR_LAYER (items[i]);

                  svg_export_path (layer, layer_ids[LAYER_ID_VECTOR]++, str,
                                   extra_spacing);
                }
              else if (GIMP_IS_TEXT_LAYER (items[i]))
                {
                  GimpTextLayer *layer = GIMP_TEXT_LAYER (items[i]);

                  svg_export_text (layer, layer_ids[LAYER_ID_TEXT]++, str,
                                   extra_spacing);
                }
              else if (GIMP_IS_LINK_LAYER (items[i]))
                {
                  GimpLinkLayer *layer = GIMP_LINK_LAYER (items[i]);

                  svg_export_link_layer (layer, layer_ids[LAYER_ID_LINK]++, str,
                                         extra_spacing);
                }
            }
          else if (format_id != EXPORT_FORMAT_NONE)
            {
              svg_export_raster (GIMP_DRAWABLE (items[i]), layer_ids, str,
                                 format_id, extra_spacing, error);
            }
        }
      gimp_progress_update ((gdouble) (n_layers - i) / n_layers);
    }
  g_free (extra_spacing);
  if (group)
    g_free (items);

  gimp_progress_update (1.0);
}

static void
svg_export_group_header (GimpGroupLayer *group,
                         gint            group_id,
                         GString        *str,
                         gchar          *spacing)
{
  gchar   *name;
  gdouble  opacity;

  name    = g_strdup_printf ("group%d", group_id);
  opacity = gimp_layer_get_opacity (GIMP_LAYER (group)) / 100.0f;

  g_string_append_printf (str, "%s<g id=\"%s\" opacity=\"%f\">\n",
                          spacing, name, opacity);
  g_free (name);
}

static void
svg_export_image_size (GimpImage *image,
                       GString   *str)
{
  GimpUnit    *unit;
  const gchar *abbrev;
  gchar        wbuf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar        hbuf[G_ASCII_DTOSTR_BUF_SIZE];
  gdouble      xres;
  gdouble      yres;
  gdouble      w, h;

  gimp_image_get_resolution (image, &xres, &yres);

  w = (gdouble) gimp_image_get_width  (image) / xres;
  h = (gdouble) gimp_image_get_height (image) / yres;

  /*  FIXME: should probably use the display unit here  */
  unit = gimp_image_get_unit (image);
  switch (gimp_unit_get_id (unit))
    {
    case GIMP_UNIT_INCH:  abbrev = "in";  break;
    case GIMP_UNIT_MM:    abbrev = "mm";  break;
    case GIMP_UNIT_POINT: abbrev = "pt";  break;
    case GIMP_UNIT_PICA:  abbrev = "pc";  break;
    default:              abbrev = "cm";
      unit = gimp_unit_mm ();
      w /= 10.0;
      h /= 10.0;
      break;
    }

  g_ascii_formatd (wbuf, sizeof (wbuf), "%g", w * gimp_unit_get_factor (unit));
  g_ascii_formatd (hbuf, sizeof (hbuf), "%g", h * gimp_unit_get_factor (unit));

  g_string_append_printf (str,
                          "width=\"%s%s\" height=\"%s%s\"",
                          wbuf, abbrev, hbuf, abbrev);
}

static void
svg_export_path (GimpVectorLayer *layer,
                 gint             vector_id,
                 GString         *str,
                 gchar           *spacing)
{
  GimpPath    *path;
  gchar       *name;
  gchar       *data;
  gdouble      opacity;
  gchar       *fill_string;
  gchar       *stroke_string;
  gdouble      stroke_width;
  const gchar *stroke_capstyle;
  const gchar *stroke_joinstyle;
  gdouble      stroke_miter_limit;
  gdouble      stroke_dash_offset;
  gsize        num_dashes;
  gdouble     *stroke_dash_pattern;

  path = gimp_vector_layer_get_path (layer);
  if (! path)
    return;

  name = g_strdup_printf ("vector%d", vector_id);
  data = svg_export_path_data (path);

  opacity = gimp_layer_get_opacity (GIMP_LAYER (layer)) / 100.0f;

  /* Vector Layer Properties */
  fill_string         = svg_get_color_string (layer, "fill");
  stroke_string       = svg_get_color_string (layer, "stroke");
  stroke_width        = gimp_vector_layer_get_stroke_width (layer);
  stroke_miter_limit  = gimp_vector_layer_get_stroke_miter_limit (layer);
  stroke_dash_offset  = gimp_vector_layer_get_stroke_dash_offset (layer);

  gimp_vector_layer_get_stroke_dash_pattern (layer, &num_dashes,
                                             &stroke_dash_pattern);

  switch (gimp_vector_layer_get_stroke_cap_style (layer))
    {
    case GIMP_CAP_ROUND:
      stroke_capstyle = "round";
      break;

    case GIMP_CAP_SQUARE:
      stroke_capstyle = "square";
      break;

    default:
      stroke_capstyle = "butt";
      break;
    }

  switch (gimp_vector_layer_get_stroke_join_style (layer))
    {
    case GIMP_JOIN_ROUND:
      stroke_joinstyle = "round";
      break;

    case GIMP_JOIN_BEVEL:
      stroke_joinstyle = "bevel";
      break;

    default:
      stroke_joinstyle = "miter";
      break;
    }

  g_string_append_printf (str,
                          "%s<path id=\"%s\"\n"
                          "%s      %s\n"
                          "%s      %s\n"
                          "%s      opacity=\"%f\"\n"
                          "%s      stroke-width=\"%f\"\n"
                          "%s      stroke-linecap=\"%s\"\n"
                          "%s      stroke-linejoin=\"%s\"\n"
                          "%s      stroke-miterlimit=\"%f\"\n",
                          spacing, name,
                          spacing, fill_string,
                          spacing, stroke_string,
                          spacing, opacity,
                          spacing, stroke_width,
                          spacing, stroke_capstyle,
                          spacing, stroke_joinstyle,
                          spacing, stroke_miter_limit);

  if (num_dashes > 0)
    {
      for (gint i = 0; i < num_dashes; i++)
        stroke_dash_pattern[i] *= stroke_width;

      g_string_append_printf (str,
                              "%s      stroke-dashoffset=\"%f\"\n"
                              "%s      stroke-dasharray=\"%f",
                              spacing, stroke_dash_offset,
                              spacing, stroke_dash_pattern[0]);

      for (gint i = 0; i < num_dashes; i++)
        g_string_append_printf (str, " %f", stroke_dash_pattern[i]);

      g_string_append_printf (str, "\"\n");
    }

   g_string_append_printf (str,
                           "%s      d=\"%s\" />\n",
                           spacing, data);

  g_free (fill_string);
  g_free (stroke_string);
  g_free (name);
  g_free (data);
}


#define NEWLINE "\n           "

static gchar *
svg_export_path_data (GimpPath *path)
{
  GString  *str;
  gsize     num_strokes;
  gint     *strokes;
  gint      s;
  gchar     x_string[G_ASCII_DTOSTR_BUF_SIZE];
  gchar     y_string[G_ASCII_DTOSTR_BUF_SIZE];
  gboolean  closed = FALSE;

  str = g_string_new (NULL);

  strokes = gimp_path_get_strokes (path, &num_strokes);
  for (s = 0; s < num_strokes; s++)
    {
      GimpPathStrokeType  type;
      gdouble            *control_points;
      gsize               num_points;
      gsize               num_indexes;

      type = gimp_path_stroke_get_points (path, strokes[s], &num_points,
                                          &control_points, &closed);

      if (type == GIMP_PATH_STROKE_TYPE_BEZIER)
        {
          num_indexes = num_points / 2;
          if (num_indexes >= 3)
            {
              g_ascii_formatd (x_string, G_ASCII_DTOSTR_BUF_SIZE,
                               "%.2f", control_points[2]);
              g_ascii_formatd (y_string, G_ASCII_DTOSTR_BUF_SIZE,
                               "%.2f", control_points[3]);
              if (s > 0)
                g_string_append_printf (str, NEWLINE);
              g_string_append_printf (str, "M %s,%s", x_string, y_string);
            }

          if (num_indexes > 3)
            g_string_append_printf (str, NEWLINE "C");

          for (gint i = 2; i < (num_indexes + (closed ? 2 : - 1)); i++)
            {
              gint index = (i % num_indexes) * 2;

              if (i > 2 && i % 3 == 2)
                g_string_append_printf (str, NEWLINE " ");

              g_ascii_formatd (x_string, G_ASCII_DTOSTR_BUF_SIZE,
                               "%.2f", control_points[index]);
              g_ascii_formatd (y_string, G_ASCII_DTOSTR_BUF_SIZE,
                               "%.2f", control_points[index + 1]);
              g_string_append_printf (str, " %s,%s", x_string, y_string);
            }

          if (closed && num_points > 3)
            g_string_append_printf (str, " Z");
        }
    }

  return g_strchomp (g_string_free (str, FALSE));
}

static void
svg_export_text (GimpTextLayer *layer,
                 gint           text_id,
                 GString       *str,
                 gchar         *spacing)
{
  gchar                *name;
  gint                  x     = 0;
  gint                  y     = 0;
  gdouble               x_res = 0;
  gdouble               y_res = 0;
  gdouble               opacity;
  gchar                *hex_color;
  gdouble               font_size;
  GimpUnit             *unit;
  GimpFont             *gimp_font;
  PangoFontDescription *font_description;
  PangoFontDescription *font_description2;
  PangoFontMap         *fontmap;
  PangoFont            *font;
  PangoContext         *context;

  name = g_strdup_printf ("text%d", text_id);

  gimp_drawable_get_offsets (GIMP_DRAWABLE (layer), &x, &y);
  gimp_image_get_resolution (gimp_item_get_image (GIMP_ITEM (layer)),
                             &x_res, &y_res);

  opacity   = gimp_layer_get_opacity (GIMP_LAYER (layer)) / 100.0f;
  hex_color = svg_get_hex_color (gimp_text_layer_get_color (layer));

  g_string_append_printf (str,
                          "%s<text id=\"%s\"\n"
                          "%s      x=\"%d\"\n"
                          "%s      y=\"%d\"\n"
                          "%s      opacity=\"%f\"\n"
                          "%s      fill=\"%s\"\n",
                          spacing, name,
                          spacing, x,
                          spacing, y,
                          spacing, opacity,
                          spacing, hex_color);
  g_free (hex_color);
  g_free (name);

  /* Font style */
  font_size = gimp_text_layer_get_font_size (layer, &unit);
  font_size = gimp_units_to_pixels (font_size, unit, y_res);

  fontmap = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);
  context = pango_font_map_create_context (fontmap);

  gimp_font         = gimp_text_layer_get_font (layer);
  font_description  = gimp_font_get_pango_font_description (gimp_font);
  font              = pango_font_map_load_font (fontmap, context,
                                                font_description);
  font_description2 = pango_font_describe (font);

  g_string_append_printf (str,
                          "%s      style=\"font-size:%fpx; "
                          "font-family:%s; font-weight:%d; "
                          "letter-spacing:%fpx;\">",
                          spacing, font_size,
                          pango_font_description_get_family (font_description2),
                          pango_font_description_get_weight (font_description2),
                          gimp_text_layer_get_letter_spacing (layer));

  g_object_unref (font);
  pango_font_description_free (font_description);
  pango_font_description_free (font_description2);
  g_object_unref (context);
  g_object_unref (fontmap);

  /* Text */
  if (gimp_text_layer_get_markup (layer))
    {
      GString *markup;

      markup = g_string_new (gimp_text_layer_get_markup (layer));

      g_string_replace (markup, "<markup>", "", 1);
      g_string_replace (markup, "</markup>", "", 1);

      g_string_replace (markup, "<b>",
                        "<tspan style=\"font-weight: bold\">", 0);
      g_string_replace (markup, "</b>", "</tspan>", 0);

      g_string_replace (markup, "<i>",
                        "<tspan style=\"font-style: italic\">", 0);
      g_string_replace (markup, "</i>", "</tspan>", 0);

      g_string_replace (markup, "<u>",
                        "<tspan style=\"text-decoration: underline\">", 0);
      g_string_replace (markup, "</u>", "</tspan>", 0);

      g_string_replace (markup, "<s>",
                        "<tspan style=\"text-decoration: linethrough\">", 0);
      g_string_replace (markup, "</s>", "</tspan>", 0);

      g_string_replace (markup, "<span foreground=\"", "<tspan style=\"fill:", 0);
      g_string_replace (markup, "<span font=\"", "<tspan style=\"font-family:", 0);

      g_string_replace (markup, "<span ", "<tspan ", 0);
      g_string_replace (markup, "</span>", "</tspan>", 0);

      svg_export_text_lines (layer, str, markup->str);
      g_string_free (markup, TRUE);
    }
  else
    {
      svg_export_text_lines (layer, str, gimp_text_layer_get_text (layer));
    }

  g_string_append_printf (str, "</text>\n");
}

static void
svg_export_text_lines (GimpTextLayer *layer,
                       GString       *str,
                       gchar         *text)
{
  gint                  x = 0;
  gint                  y = 0;
  gdouble               x_res = 0;
  gdouble               y_res = 0;
  gdouble               font_size;
  GimpUnit             *unit;
  gdouble               line_spacing;
  gchar               **lines;
  guint                 count;

  gimp_drawable_get_offsets (GIMP_DRAWABLE (layer), &x, &y);
  gimp_image_get_resolution (gimp_item_get_image (GIMP_ITEM (layer)),
                             &x_res, &y_res);

  line_spacing = gimp_text_layer_get_line_spacing (layer);
  font_size    = gimp_text_layer_get_font_size (layer, &unit);
  font_size    = gimp_units_to_pixels (font_size, unit, y_res);

  lines = g_strsplit (text, "\n", -1);
  count = g_strv_length (lines);

  for (gint i = 0; i < count; i++)
    {
      g_string_append_printf (str, "<tspan x=\"%d\" y=\"%d\">"
                              "%s</tspan>",
                              x, y, lines[i]);
      y += font_size + line_spacing;
    }
  g_strfreev (lines);
}

static void
svg_export_link_layer (GimpLinkLayer *layer,
                       gint           link_id,
                       GString       *str,
                       gchar         *spacing)
{
  GFile   *file;
  gchar   *name;
  gchar   *path;
  gint     width;
  gint     height;
  gdouble  opacity;
  gint     x;
  gint     y;

  width   = gimp_drawable_get_width (GIMP_DRAWABLE (layer));
  height  = gimp_drawable_get_height (GIMP_DRAWABLE (layer));
  opacity = gimp_layer_get_opacity (GIMP_LAYER (layer)) / 100.0f;
  gimp_drawable_get_offsets (GIMP_DRAWABLE (layer), &x, &y);

  name = g_strdup_printf ("link%d", link_id);

  file = gimp_link_layer_get_file (layer);
  path = g_file_get_path (file);

  g_string_append_printf (str,
                          "%s<image id=\"%s\"\n"
                          "%s       x=\"%d\"\n"
                          "%s       y=\"%d\"\n"
                          "%s       width=\"%d\"\n"
                          "%s       height=\"%d\"\n"
                          "%s       opacity=\"%f\"\n"
                          "%s       href=\"%s\" />\n",
                          spacing, name,
                          spacing, x,
                          spacing, y,
                          spacing, width,
                          spacing, height,
                          spacing, opacity,
                          spacing, path);
  g_free (name);
  g_free (path);
}

static void
svg_export_raster (GimpDrawable  *layer,
                   gint          *layer_ids,
                   GString       *str,
                   gint           format_id,
                   gchar         *spacing,
                   GError       **error)
{
  GimpProcedure  *procedure;
  GimpValueArray *return_vals = NULL;
  GimpImage      *image;
  GimpImage      *temp_image;
  GimpLayer      *temp_layer;
  GFile          *temp_file = NULL;
  FILE           *temp_fp;
  gsize           temp_size;
  gint            raster_id;
  gint            width;
  gint            height;
  gdouble         opacity;
  gboolean        include_color_profile;
  gboolean        has_layer_mask;
  const gchar    *mimetype;

  has_layer_mask = FALSE;
  if (GIMP_IS_LAYER (layer) && gimp_layer_get_mask (GIMP_LAYER (layer)))
    {
      svg_export_layer_mask (gimp_layer_get_mask (GIMP_LAYER (layer)),
                             layer_ids, str, spacing, error);
      has_layer_mask = TRUE;
    }

  if (format_id == EXPORT_FORMAT_PNG)
    {
      temp_file = gimp_temp_file ("png");
      mimetype  = "image/png";
    }
  else
    {
      temp_file = gimp_temp_file ("jpeg");
      mimetype  = "image/jpeg";
    }
  raster_id = layer_ids[LAYER_ID_RASTER]++;

  width  = gimp_drawable_get_width (layer);
  height = gimp_drawable_get_height (layer);
  if (GIMP_IS_LAYER (layer))
    opacity = gimp_layer_get_opacity (GIMP_LAYER (layer)) / 100.0f;
  else
    opacity = gimp_channel_get_opacity (GIMP_CHANNEL (layer)) / 100.0f;

  image = gimp_item_get_image (GIMP_ITEM (layer));
  temp_image = gimp_image_new (width, height,
                               gimp_image_get_base_type (image));
  if (gimp_image_get_base_type (image) == GIMP_INDEXED)
    gimp_image_set_palette (temp_image,
                            gimp_image_get_palette (image));

  temp_layer = gimp_layer_new_from_drawable (layer, temp_image);
  gimp_image_insert_layer (temp_image, temp_layer, NULL, 0);
  gimp_layer_set_offsets (temp_layer, 0, 0);
  /* Set layer to full opacity and use opacity attribute instead */
  gimp_layer_set_opacity (temp_layer, 100.0f);

  include_color_profile = FALSE;
  if (gimp_image_get_color_profile (image))
    {
      GimpColorProfile *profile;

      profile =  gimp_image_get_color_profile (image);
      gimp_image_set_color_profile (temp_image, profile);
      include_color_profile = TRUE;
    }

  if (format_id == EXPORT_FORMAT_PNG)
    {
      procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (), "file-png-export");
      return_vals = gimp_procedure_run (procedure,
                                        "run-mode",              GIMP_RUN_NONINTERACTIVE,
                                        "image",                 temp_image,
                                        "file",                  temp_file,
                                        "interlaced",            FALSE,
                                        "compression",           9,
                                        "bkgd",                  FALSE,
                                        "offs",                  FALSE,
                                        "phys",                  FALSE,
                                        "time",                  FALSE,
                                        "save-transparent",      FALSE,
                                        "optimize-palette",      FALSE,
                                        "include-color-profile", include_color_profile,
                                        NULL);
    }
  else
    {
      procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (), "file-jpeg-export");
      return_vals = gimp_procedure_run (procedure,
                                        "run-mode",              GIMP_RUN_NONINTERACTIVE,
                                        "image",                 temp_image,
                                        "file",                  temp_file,
                                        "quality",               0.9f,
                                        "cmyk",                  FALSE,
                                        "include-color-profile", include_color_profile,
                                        NULL);
    }
  gimp_image_delete (temp_image);

  if (GIMP_VALUES_GET_ENUM (return_vals, 0) != GIMP_PDB_SUCCESS)
    {
      if (! error)
        g_set_error (error, G_FILE_ERROR, 0,
                     "SVG: Unable to export raster layer.");

      return;
    }

  temp_fp = g_fopen (g_file_peek_path (temp_file), "rb");
  fseek (temp_fp, 0L, SEEK_END);
  temp_size = ftell (temp_fp);
  fseek (temp_fp, 0L, SEEK_SET);

  if (temp_size > 0)
    {
      guchar *buf;
      gchar  *encoded;
      gchar  *name;
      gint    x;
      gint    y;

      buf = g_malloc0 (temp_size);
      fread (buf, 1, temp_size, temp_fp);

      encoded = g_base64_encode ((const guchar *) buf, temp_size + 1);

      name = g_strdup_printf ("raster%d", raster_id);
      gimp_drawable_get_offsets (GIMP_DRAWABLE (layer), &x, &y);

      g_string_append_printf (str,
                              "%s<image id=\"%s\"\n"
                              "%s       x=\"%d\"\n"
                              "%s       y=\"%d\"\n"
                              "%s       width=\"%d\"\n"
                              "%s       height=\"%d\"\n"
                              "%s       opacity=\"%f\"\n",
                              spacing, name,
                              spacing, x,
                              spacing, y,
                              spacing, width,
                              spacing, height,
                              spacing, opacity);

      if (has_layer_mask)
        g_string_append_printf (str,
                                "%s       mask=\"url(#mask%d)\"\n",
                                spacing, layer_ids[LAYER_ID_MASK]);

      g_string_append_printf (str,
                              "%s       href=\"data:%s;base64,%s\" />\n",
                              spacing, mimetype, encoded);
      g_free (encoded);
      g_free (name);
      g_free (buf);
    }
  fclose (temp_fp);

  g_file_delete (temp_file, NULL, NULL);
  g_object_unref (temp_file);
}

static void
svg_export_layer_mask (GimpLayerMask  *mask,
                       gint           *layer_ids,
                       GString        *str,
                       gchar          *spacing,
                       GError        **error)
{
  gchar *name;
  gchar *extra_spacing;
  gint   mask_id;
  gint   x = 0;
  gint   y = 0;

  mask_id = layer_ids[LAYER_ID_MASK]++;
  name    = g_strdup_printf ("mask%d", mask_id);
  gimp_drawable_get_offsets (GIMP_DRAWABLE (mask), &x, &y);

  g_string_append_printf (str,
                          "%s<mask id=\"%s\"\n"
                          "%s      x=\"%d\"\n"
                          "%s      y=\"%d\"\n"
                          "%s      mask-type=\"luminance\">\n",
                          spacing, name,
                          spacing, x,
                          spacing, y,
                          spacing);
  g_free (name);

  extra_spacing = g_strdup_printf ("%s  ", spacing);

  svg_export_raster (GIMP_DRAWABLE (mask), layer_ids, str, EXPORT_FORMAT_PNG,
                     extra_spacing, error);

  g_free (extra_spacing);
  g_string_append_printf (str, "</mask>\n");
}

static gchar *
svg_get_color_string (GimpVectorLayer *layer,
                      const gchar     *type)
{
  GeglColor   *color   = NULL;
  GimpPattern *pattern = NULL;
  gboolean     enabled = TRUE;

  if (g_strcmp0 ("fill", type) == 0)
    {
      color   = gimp_vector_layer_get_fill_color (layer);
      if (color == NULL)
        pattern = gimp_vector_layer_get_fill_pattern (layer);
      enabled = gimp_vector_layer_get_enable_fill (layer);
    }
  else
    {
      color   = gimp_vector_layer_get_stroke_color (layer);
      if (color == NULL)
        pattern = gimp_vector_layer_get_stroke_pattern (layer);
      enabled = gimp_vector_layer_get_enable_stroke (layer);
    }

  if (enabled)
    {
      if (color)
        {
          gchar *hex_color;
          gchar *color_string;

          hex_color = svg_get_hex_color (color);

          color_string = g_strdup_printf ("%s=\"%s\"",
                                          type, hex_color);
          g_free (hex_color);
          g_clear_object (&color);

          return color_string;
        }
      else if (pattern)
        {
          gchar *color_string;
          gint   res_id = gimp_resource_get_id (GIMP_RESOURCE (pattern));

          color_string = g_strdup_printf ("%s=\"url(#Pattern%d)\"",
                                          type, res_id);

          return color_string;
        }
    }

  g_clear_object (&color);

  return g_strdup_printf ("%s=\"none\"", type);
}

static gchar *
svg_get_hex_color (GeglColor *color)
{
  guchar  rgb[3] = { 0, 0, 0 };
  gchar  *hex_color = NULL;

  gegl_color_get_pixel (color, babl_format ("R'G'B' u8"), rgb);
  hex_color = g_strdup_printf ("#%02X%02X%02X", rgb[0], rgb[1], rgb[2]);

  return hex_color;
}
