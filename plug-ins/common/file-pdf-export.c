/* GIMP - The GNU Image Manipulation Program
 *
 * file-pdf-export.c - PDF file exporter, based on the cairo PDF surface
 *
 * Copyright (C) 2010 Barak Itkin <lightningismyname@gmail.com>
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

/* The PDF export plugin has 3 main procedures:
 * 1. file-pdf-export
 *    This is the main procedure. It has 3 options for optimizations of
 *    the pdf file, and it can show a gui. This procedure works on a single
 *    image.
 * 2. file-pdf-export-defaults
 *    This procedures is the one that will be invoked by gimp's file-save,
 *    when the pdf extension is chosen. If it's in RUN_INTERACTIVE, it will
 *    pop a user interface with more options, like file-pdf-export. If it's in
 *    RUN_NONINTERACTIVE, it will simply use the default values. Note that on
 *    RUN_WITH_LAST_VALS there will be no gui, however the values will be the
 *    ones that were used in the last interactive run (or the defaults if none
 *    are available.
 * 3. file-pdf-export-multi
 *    This procedures is more advanced, and it allows the creation of multiple
 *    paged pdf files. It will be located in File/Create/Multiple page PDF...
 *
 * It was suggested that file-pdf-export-multi will be removed from the UI as it
 * does not match the product vision (GIMP isn't a program for editing multiple
 * paged documents).
 */

/* Known Issues (except for the coding style issues):
 * 1. Grayscale layers are inverted (although layer masks which are not grayscale,
 * are not inverted)
 * 2. Exporting some fonts doesn't work since gimp_text_layer_get_font Returns a
 * font which is sometimes incompatible with pango_font_description_from_string
 * (gimp_text_layer_get_font sometimes returns suffixes such as "semi-expanded" to
 * the font's name although the GIMP's font selection dialog shows the don'ts name
 * normally - This should be checked again in GIMP 2.7)
 * 3. Indexed layers can't be optimized yet (Since gimp_histogram won't work on
 * indexed layers)
 * 4. Rendering the pango layout requires multiplying the size in PANGO_SCALE. This
 * means I'll need to do some hacking on the markup returned from GIMP.
 * 5. When accessing the contents of layer groups is supported, we should do use it
 * (since this plugin should preserve layers).
 *
 * Also, there are 2 things which we should warn the user about:
 * 1. Cairo does not support bitmap masks for text.
 * 2. Currently layer modes are ignored. We do support layers, including
 * transparency and opacity, but layer modes are not supported.
 */

/* Changelog
 *
 * April 29, 2009 | Barak Itkin <lightningismyname@gmail.com>
 *   First version of the plugin. This is only a proof of concept and not a full
 *   working plugin.
 *
 * May 6, 2009 Barak | Itkin <lightningismyname@gmail.com>
 *   Added new features and several bugfixes:
 *   - Added handling for image resolutions
 *   - fixed the behavior of getting font sizes
 *   - Added various optimizations (solid rectangles instead of bitmaps, ignoring
 *     invisible layers, etc.) as a macro flag.
 *   - Added handling for layer masks, use CAIRO_FORMAT_A8 for grayscale drawables.
 *   - Indexed layers are now supported
 *
 * August 17, 2009 | Barak Itkin <lightningismyname@gmail.com>
 *   Most of the plugin was rewritten from scratch and it now has several new
 *   features:
 *   - Got rid of the optimization macros in the code. The gui now supports
 *     selecting which optimizations to apply.
 *   - Added a procedure to allow the creation of multiple paged PDF's
 *   - Registered the plugin on "<Image>/File/Create/PDF"
 *
 * August 21, 2009 | Barak Itkin <lightningismyname@gmail.com>
 *   Fixed a typo that prevented the plugin from compiling...
 *   A migration to the new GIMP 2.8 api, which includes:
 *   - Now using gimp_export_dialog_new
 *   - Using gimp_text_layer_get_hint_style (2.8) instead of the deprecated
 *     gimp_text_layer_get_hinting (2.6).
 *
 * August 24, 2010 | Barak Itkin <lightningismyname@gmail.com>
 *   More migrations to the new GIMP 2.8 api:
 *   - Now using the GimpItem api
 *   - Using gimp_text_layer_get_markup where possible
 *   - Fixed some compiler warnings
 *   Also merged the header and c file into one file, Updated some of the comments
 *   and documentation, and moved this into the main source repository.
 */

#include "config.h"

#include <errno.h>

#include <glib/gstdio.h>
#include <cairo-pdf.h>
#include <pango/pangocairo.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define EXPORT_PROC             "file-pdf-export"
#define EXPORT_MULTI_PROC       "file-pdf-export-multi"
#define PLUG_IN_BINARY          "file-pdf-export"
#define PLUG_IN_ROLE            "gimp-file-pdf-export"

#define DATA_IMAGE_LIST         "file-pdf-data-multi-page"

/* Gimp will crash before you reach this limitation :D */
#define MAX_PAGE_COUNT           350
#define MAX_FILE_NAME_LENGTH     350

#define THUMB_WIDTH              90
#define THUMB_HEIGHT             120

#define GIMP_PLUGIN_PDF_EXPORT_ERROR gimp_plugin_pdf_export_error_quark ()

typedef enum
{
  GIMP_PLUGIN_PDF_EXPORT_ERROR_FAILED
} GimpPluginPDFError;

GQuark gimp_plugin_pdf_export_error_quark (void);

typedef struct
{
  GimpImage *images[MAX_PAGE_COUNT];
  guint32    image_count;
  gchar      file_name[MAX_FILE_NAME_LENGTH];
} PdfMultiPage;

enum
{
  THUMB,
  PAGE_NUMBER,
  IMAGE_NAME,
  IMAGE
};

typedef struct
{
  GdkPixbuf *thumb;
  gint32     page_number;
  gchar     *image_name;
} Page;


typedef struct _Pdf      Pdf;
typedef struct _PdfClass PdfClass;

struct _Pdf
{
  GimpPlugIn      parent_instance;
};

struct _PdfClass
{
  GimpPlugInClass parent_class;
};


#define PDF_TYPE  (pdf_get_type ())
#define PDF(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PDF_TYPE, Pdf))

GType                   pdf_get_type             (void) G_GNUC_CONST;

static GList          * pdf_query_procedures     (GimpPlugIn           *plug_in);
static GimpProcedure  * pdf_create_procedure     (GimpPlugIn           *plug_in,
                                                  const gchar          *name);

static GimpValueArray * pdf_export               (GimpProcedure        *procedure,
                                                  GimpRunMode           run_mode,
                                                  GimpImage            *image,
                                                  GFile                *file,
                                                  GimpExportOptions    *options,
                                                  GimpMetadata         *metadata,
                                                  GimpProcedureConfig  *config,
                                                  gpointer              run_data);
static GimpValueArray * pdf_export_multi         (GimpProcedure        *procedure,
                                                  GimpProcedureConfig  *config,
                                                  gpointer              run_data);

static GimpPDBStatusType pdf_export_image        (GimpProcedure        *procedure,
                                                  GimpProcedureConfig  *config,
                                                  GimpExportOptions    *options,
                                                  gboolean              single_image,
                                                  gboolean              show_progress,
                                                  GError              **error);

static void              export_edit_options     (GimpProcedure        *procedure,
                                                  GimpProcedureConfig  *config,
                                                  GimpExportOptions    *options,
                                                  gpointer              create_data);

static void             init_image_list_defaults (GimpImage            *image);

static void             validate_image_list      (void);

static gboolean         gui_single               (GimpProcedure        *procedure,
                                                  GimpProcedureConfig  *config,
                                                  GimpImage            *image);
static gboolean         gui_multi                (GimpProcedure        *procedure,
                                                  GimpProcedureConfig  *config);

static void             choose_file_call         (GtkWidget            *browse_button,
                                                  gpointer              file_entry);

static gboolean         get_image_list           (void);

static GtkTreeModel   * create_model             (void);

static void             add_image_call           (GtkWidget            *widget,
                                                  gpointer              img_combo);
static void             del_image_call           (GtkWidget            *widget,
                                                  gpointer              icon_view);
static void             remove_call              (GtkTreeModel         *tree_model,
                                                  GtkTreePath          *path,
                                                  gpointer              user_data);
static void             recount_pages            (void);

static cairo_surface_t *get_cairo_surface        (GimpDrawable         *drawable,
                                                  gboolean              as_mask,
                                                  GError              **error);

static GeglColor *      get_layer_color          (GimpLayer            *layer,
                                                  gboolean             *single);

static void             drawText                 (GimpLayer            *layer,
                                                  gdouble               opacity,
                                                  cairo_t              *cr,
                                                  gdouble               x_res,
                                                  gdouble               y_res);

static gboolean         draw_layer               (GimpLayer           **layers,
                                                  gint                  n_layers,
                                                  GimpProcedureConfig  *config,
                                                  gboolean              single_image,
                                                  gint                  j,
                                                  cairo_t              *cr,
                                                  gdouble               x_res,
                                                  gdouble               y_res,
                                                  const gchar          *name,
                                                  gboolean              show_progress,
                                                  gdouble               progress_start,
                                                  gdouble               progress_end,
                                                  gint                  layer_level,
                                                  GError              **error);


G_DEFINE_TYPE (Pdf, pdf, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PDF_TYPE)
DEFINE_STD_SET_I18N


static gboolean     dnd_remove = TRUE;
static PdfMultiPage multi_page;

static GtkTreeModel *model;
static GtkWidget    *file_choose;
static gchar        *file_name;


G_DEFINE_QUARK (gimp-plugin-pdf-export-error-quark, gimp_plugin_pdf_export_error)

static void
pdf_class_init (PdfClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = pdf_query_procedures;
  plug_in_class->create_procedure = pdf_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
pdf_init (Pdf *pdf)
{
}

static GList *
pdf_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (EXPORT_PROC));
  list = g_list_append (list, g_strdup (EXPORT_MULTI_PROC));

  return list;
}

static GimpProcedure *
pdf_create_procedure (GimpPlugIn  *plug_in,
                        const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             TRUE, pdf_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("Portable Document Format"));

      gimp_procedure_set_documentation (procedure,
                                        _("Save files in PDF format"),
                                        _("Saves files in Adobe's Portable "
                                          "Document Format. PDF is designed to "
                                          "be easily processed by a variety of "
                                          "different platforms, and is a "
                                          "distant cousin of PostScript."),
                                          name);
      gimp_procedure_set_attribution (procedure,
                                      "Barak Itkin, Lionel N., Jehan",
                                      "Copyright Barak Itkin, Lionel N., Jehan",
                                      "August 2009, 2017");

      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("PDF"));
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "application/pdf");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "pdf");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB    |
                                              GIMP_EXPORT_CAN_HANDLE_ALPHA  |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY   |
                                              GIMP_EXPORT_CAN_HANDLE_LAYERS |
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED,
                                              export_edit_options, NULL, NULL);

      gimp_procedure_add_boolean_argument (procedure, "vectorize",
                                           _("Convert _bitmaps to vector graphics where possible"),
                                           _("Convert bitmaps to vector graphics where possible"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "ignore-hidden",
                                           _("O_mit hidden layers and layers with zero opacity"),
                                           _("Non-visible layers will not be exported"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "apply-masks",
                                           _("_Apply layer masks"),
                                           _("Apply layer masks before saving (Keeping the mask "
                                             "will not change the output, only the PDF structure)"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "layers-as-pages",
                                           _("La_yers as pages"),
                                           _("Layers as pages (bottom layers first)."),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "reverse-order",
                                           _("Re_verse order"),
                                           _("Reverse the pages order (top layers first)."),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "root-layers-only",
                                           _("Roo_t layers only"),
                                           _("Only the root layers are considered pages"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "convert-text-layers",
                                           _("Convert te_xt layers to image"),
                                           _("Convert text layers to raster graphics"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "fill-background-color",
                                           _("_Fill transparent areas with background color"),
                                           _("Fill transparent areas with background color if "
                                             "layer has an alpha channel"),
                                           TRUE,
                                           G_PARAM_READWRITE);
    }
  else if (! strcmp (name, EXPORT_MULTI_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      pdf_export_multi, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("_Create multipage PDF..."));
#if 0
      gimp_procedure_add_menu_path (procedure, "<Image>/File/Create/PDF");
#endif

      gimp_procedure_set_documentation (procedure,
                                        _("Save files in PDF format"),
                                        _("Saves files in Adobe's Portable "
                                          "Document Format. PDF is designed to "
                                          "be easily processed by a variety of "
                                          "different platforms, and is a "
                                          "distant cousin of PostScript."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Barak Itkin",
                                      "Copyright Barak Itkin",
                                      "August 2009");

      gimp_procedure_add_enum_argument (procedure, "run-mode",
                                        "Run mode",
                                        "The run mode",
                                        GIMP_TYPE_RUN_MODE,
                                        GIMP_RUN_INTERACTIVE,
                                        G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "count",
                                       _("Count"),
                                       _("The number of images entered (This will be the "
                                         "number of pages)."),
                                       1, MAX_PAGE_COUNT, 1,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int32_array_argument (procedure, "images",
                                               "Images",
                                               "Input image for each page (An image can "
                                               "appear more than once)",
                                               G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "vectorize",
                                           _("Convert _bitmaps to vector graphics where possible"),
                                           _("Convert bitmaps to vector graphics where possible"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "ignore-hidden",
                                           _("O_mit hidden layers and layers with zero opacity"),
                                           _("Non-visible layers will not be exported"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "apply-masks",
                                           _("_Apply layer masks"),
                                           _("Apply layer masks before saving (Keeping the mask "
                                             "will not change the output, only the PDF structure)"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "fill-background-color",
                                           _("_Fill transparent areas with background color"),
                                           _("Fill transparent areas with background color if "
                                             "layer has an alpha channel"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_string_argument (procedure, "uri",
                                          _("Save to"),
                                          _("The URI of the file to save to"),
                                          NULL,
                                          GIMP_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
pdf_export (GimpProcedure        *procedure,
            GimpRunMode           run_mode,
            GimpImage            *image,
            GFile                *file,
            GimpExportOptions    *options,
            GimpMetadata         *metadata,
            GimpProcedureConfig  *config,
            gpointer              run_data)
{
  GError            *error       = NULL;
  GimpPDBStatusType  status      = GIMP_PDB_SUCCESS;

  gegl_init (NULL, NULL);

  /* Initializing all the settings */
  multi_page.image_count = 0;

  file_name = g_file_get_path (file);

  init_image_list_defaults (image);

  validate_image_list ();

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! gui_single (procedure, config, image))
        status = GIMP_PDB_CANCEL;
    }

  if (status == GIMP_PDB_SUCCESS)
    status = pdf_export_image (procedure, config, options, TRUE,
                               (run_mode != GIMP_RUN_NONINTERACTIVE),
                               &error);

  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpValueArray *
pdf_export_multi (GimpProcedure        *procedure,
                  GimpProcedureConfig  *config,
                  gpointer              run_data)
{
  GError            *error     = NULL;
  GimpPDBStatusType  status    = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  GimpExportOptions *options   = NULL;
  gchar             *uri;
  const gint32      *image_ids = NULL;
  GimpImage         *image     = NULL;
  GFile             *file      = NULL;

  gegl_init (NULL, NULL);

  g_object_get (config,
                "run-mode", &run_mode,
                "uri",      &uri,
                "count",    &multi_page.image_count,
                "images",   &image_ids,
                NULL);

  options = g_object_new (GIMP_TYPE_EXPORT_OPTIONS,
                          "capabilities",
                          GIMP_EXPORT_CAN_HANDLE_RGB    |
                          GIMP_EXPORT_CAN_HANDLE_ALPHA  |
                          GIMP_EXPORT_CAN_HANDLE_GRAY   |
                          GIMP_EXPORT_CAN_HANDLE_LAYERS |
                          GIMP_EXPORT_CAN_HANDLE_INDEXED,
                          NULL);

  if (uri != NULL)
    {
      file = g_file_new_for_uri (uri);
      g_free (uri);
    }

  if (file != NULL)
    file_name = g_file_get_path (file);

  if (image_ids)
    for (gint i = 0; i < multi_page.image_count; i++)
      multi_page.images[i] = gimp_image_get_by_id (image_ids[i]);
  else
    init_image_list_defaults (image);

  validate_image_list ();

  /* Starting the executions */
  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! gui_multi (procedure, config))
        status = GIMP_PDB_CANCEL;
    }

  if (status == GIMP_PDB_SUCCESS)
    status = pdf_export_image (procedure, config, options, FALSE,
                               (run_mode != GIMP_RUN_NONINTERACTIVE),
                               &error);

  return gimp_procedure_new_return_values (procedure, status, error);
}

static cairo_status_t
write_func (void                *fp,
            const unsigned char *data,
            unsigned int         size)
{
  return fwrite (data, 1, size, fp) == size ? CAIRO_STATUS_SUCCESS
                                            : CAIRO_STATUS_WRITE_ERROR;
}

static GList *
get_missing_fonts (GList *layers)
{
  GList *missing_fonts = NULL;
  GList *iter;

  for (iter = layers; iter; iter = iter->next)
    {
      GimpLayer *layer = iter->data;

      if (gimp_item_is_group (GIMP_ITEM (layer)))
        {
          GList *child_missing_fonts;
          GList *iter2;

          child_missing_fonts = get_missing_fonts (gimp_item_list_children (GIMP_ITEM (layer)));
          for (iter2 = child_missing_fonts; iter2; iter2 = iter2->next)
            {
              gchar *missing = iter2->data;

              if (g_list_find_custom (missing_fonts, missing, (GCompareFunc) g_strcmp0) == NULL)
                missing_fonts = g_list_prepend (missing_fonts, missing);
              else
                g_free (missing);
            }
          g_list_free (child_missing_fonts);
        }
      else if (gimp_item_is_text_layer (GIMP_ITEM (layer)))
        {
          GimpFont             *gimp_font;
          PangoFontDescription *font_description;
          PangoFontDescription *font_description2;
          PangoFontMap         *fontmap;
          PangoFont            *font;
          PangoContext         *context;

          fontmap = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);
          context = pango_font_map_create_context (fontmap);

          gimp_font        = gimp_text_layer_get_font (GIMP_TEXT_LAYER (layer));
          font_description = gimp_font_get_pango_font_description (gimp_font);

          font = pango_font_map_load_font (fontmap, context, font_description);
          font_description2 = pango_font_describe (font);
          if (g_strcmp0 (pango_font_description_get_family (font_description),
                         pango_font_description_get_family (font_description2)) != 0)
            {
              const gchar *missing = pango_font_description_get_family (font_description);

              if (g_list_find_custom (missing_fonts, missing, (GCompareFunc) g_strcmp0) == NULL)
                missing_fonts = g_list_prepend (missing_fonts,
                                                g_strdup (missing));
            }

          g_object_unref (font);
          pango_font_description_free (font_description);
          pango_font_description_free (font_description2);
          g_object_unref (context);
          g_object_unref (fontmap);
        }
    }

  g_list_free (layers);

  return missing_fonts;
}

static GimpPDBStatusType
pdf_export_image (GimpProcedure        *procedure,
                  GimpProcedureConfig  *config,
                  GimpExportOptions    *options,
                  gboolean              single_image,
                  gboolean              show_progress,
                  GError              **error)
{
  cairo_surface_t *pdf_file;
  cairo_t         *cr;
  FILE            *fp;
  gint             i;
  gboolean         layers_as_pages = FALSE;
  gboolean         fill_background_color;

  g_object_get (config,
                "fill-background-color", &fill_background_color,
                NULL);

  if (single_image)
    g_object_get (config, "layers-as-pages", &layers_as_pages, NULL);

  fp = g_fopen (file_name, "wb");
  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (file_name), g_strerror (errno));

      return GIMP_PDB_EXECUTION_ERROR;
    }

  pdf_file = cairo_pdf_surface_create_for_stream (write_func, fp, 1, 1);

  if (cairo_surface_status (pdf_file) != CAIRO_STATUS_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("An error occurred while creating the PDF file:\n"
                     "%s\n"
                     "Make sure you entered a valid filename and that the "
                     "selected location isn't read only!"),
                   cairo_status_to_string (cairo_surface_status (pdf_file)));

      return GIMP_PDB_EXECUTION_ERROR;
    }

  cr = cairo_create (pdf_file);

  for (i = 0; i < multi_page.image_count; i++)
    {
      GimpImage     *image = multi_page.images[i];
      GimpLayer    **layers;
      gint32         n_layers;
      gdouble        x_res, y_res;
      gdouble        x_scale, y_scale;
      GimpItem     **drawables;
      gint           n_drawables;
      gint           j;

      drawables = gimp_image_get_selected_drawables (image, &n_drawables);
      if (n_drawables == 0)
        {
          g_free (drawables);
          continue;
        }

      /* Save the state of the surface before any changes, so that
       * settings from one page won't affect all the others
       */
      cairo_save (cr);

      if (! (gimp_export_options_get_image (options,
                                            &image) == GIMP_EXPORT_EXPORT))
        {
          /* gimp_drawable_histogram() only works within the bounds of
           * the selection, which is a problem (see issue #2431).
           * Instead of saving the selection, unselecting to later
           * reselect, let's just always work on a duplicate of the
           * image.
           */
          image = gimp_image_duplicate (image);
        }

      gimp_selection_none (image);

      gimp_image_get_resolution (image, &x_res, &y_res);
      x_scale = 72.0 / x_res;
      y_scale = 72.0 / y_res;

      cairo_pdf_surface_set_size (pdf_file,
                                  gimp_image_get_width  (image) * x_scale,
                                  gimp_image_get_height (image) * y_scale);

      /* This way we set how many pixels are there in every inch.
       * It's very important for PangoCairo
       */
      cairo_surface_set_fallback_resolution (pdf_file, x_res, y_res);

      /* Cairo has a concept of user-space vs device-space units.
       * From what I understand, by default the user-space unit is the
       * typographical "point". Since we work mostly with pixels, not
       * points, the following call simply scales the transformation
       * matrix from points to pixels, relatively to the image
       * resolution, knowing that 1 typographical point == 1/72 inch.
       */
      cairo_scale (cr, x_scale, y_scale);

      layers = gimp_image_get_layers (image, &n_layers);

      /* Fill image with background color if transparent and
       * user chose that option.
       */
      if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layers[n_layers - 1])) &&
          fill_background_color)
        {
          GeglColor *color;
          double     rgb[3];

          cairo_rectangle (cr, 0.0, 0.0,
                           gimp_image_get_width  (image),
                           gimp_image_get_height (image));
          color = gimp_context_get_background ();
          gegl_color_get_pixel (color, babl_format_with_space ("R'G'B'A double", NULL), rgb);
          cairo_set_source_rgb (cr, rgb[0], rgb[1], rgb[2]);
          cairo_fill (cr);

          g_object_unref (color);
        }

      /* Now, we should loop over the layers of each image */
      for (j = 0; j < n_layers; j++)
        {
          if (! draw_layer (layers, n_layers, config,
                            single_image, j, cr, x_res, y_res,
                            gimp_procedure_get_name (procedure),
                            show_progress,
                            /* Progression is showed per image, and would restart at 0
                             * if you open several images.
                             */
                            (gdouble) j / n_layers,
                            (gdouble) (j + 1) / n_layers,
                            0, error))
            {
              /* free the resources */
              g_free (layers);
              cairo_surface_destroy (pdf_file);
              cairo_destroy (cr);
              fclose (fp);

              return GIMP_PDB_EXECUTION_ERROR;
            }
        }
      if (show_progress)
        gimp_progress_update (1.0);

      g_free (layers);

      /* We are done with this image - Show it!
       * Unless that's a multi-page to avoid blank page at the end
       */

      if (! layers_as_pages)
        cairo_show_page (cr);

      cairo_restore (cr);

      gimp_image_delete (image);
    }

  /* We are done with all the images - time to free the resources */
  cairo_surface_destroy (pdf_file);
  cairo_destroy (cr);

  fclose (fp);

  return GIMP_PDB_SUCCESS;
}

static void
export_edit_options (GimpProcedure        *procedure,
                     GimpProcedureConfig  *config,
                     GimpExportOptions    *options,
                     gpointer              create_data)
{
  GimpExportCapabilities capabilities;
  gboolean               apply_masks;

  g_object_get (G_OBJECT (config),
                "apply-masks", &apply_masks,
                NULL);

  capabilities = (GIMP_EXPORT_CAN_HANDLE_RGB    |
                  GIMP_EXPORT_CAN_HANDLE_ALPHA  |
                  GIMP_EXPORT_CAN_HANDLE_GRAY   |
                  GIMP_EXPORT_CAN_HANDLE_LAYERS |
                  GIMP_EXPORT_CAN_HANDLE_INDEXED);


  if (! apply_masks)
    capabilities |= GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS;

  g_object_set (G_OBJECT (options),
                "capabilities", capabilities,
                NULL);
}

/******************************************************/
/* Beginning of parameter handling functions          */
/******************************************************/

/* A function that initializes the image list to default values */
static void
init_image_list_defaults (GimpImage *image)
{
  if (image)
    {
      multi_page.images[0]   = image;
      multi_page.image_count = 1;
    }
  else
    {
      multi_page.image_count = 0;
    }
}

/* A function that removes images that are no longer valid from the
 * image list
 */
static void
validate_image_list (void)
{
  gint32  valid = 0;
  guint32 i     = 0;

  for (i = 0 ; i < MAX_PAGE_COUNT && i < multi_page.image_count ; i++)
    {
      if (gimp_image_is_valid (multi_page.images[i]))
        {
          multi_page.images[valid] = multi_page.images[i];
          valid++;
        }
    }

  multi_page.image_count = valid;
}


/******************************************************/
/* Beginning of GUI functions                         */
/******************************************************/

/* The main GUI function for saving single-paged PDFs */

static gboolean
gui_single (GimpProcedure       *procedure,
            GimpProcedureConfig *config,
            GimpImage           *image)
{
  GtkWidget  *window;
  GtkWidget  *widget;
  GimpLayer **layers;
  GList      *missing_fonts;
  GList      *dialog_props = NULL;
  gboolean    run;
  gint32      n_layers;

  gimp_ui_init (PLUG_IN_BINARY);

  window = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (window),
                                  "pages-box",
                                  "reverse-order", "root-layers-only", NULL);
  /* XXX the "layers-as-pages" checkbox label used to be changing,
   * showing "top layers first" or "bottom layers first" depending on
   * the value of "reverse-order". Should we want this? Or do it
   * differently, i.e. maybe with an additional label showing the order?
   */
  widget = gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (window),
                                            "pages-frame", "layers-as-pages", FALSE,
                                            "pages-box");
  /* Enable "layers-as-pages" if more than one layer, or there's a single
   * layer group has more than one layer */
  layers = gimp_image_get_layers (multi_page.images[0], &n_layers);
  if (n_layers == 1 && gimp_item_is_group (GIMP_ITEM (layers[0])))
    g_free (gimp_item_get_children (GIMP_ITEM (layers[0]), &n_layers));
  g_free (layers);

  gtk_widget_set_sensitive (widget, n_layers > 1);

  /* Warning for missing fonts (non-embeddable with rasterization
   * possible).
   */
  missing_fonts = get_missing_fonts (gimp_image_list_layers (multi_page.images[0]));
  if (missing_fonts != NULL)
    {
      GList *iter;
      gchar *font_list = NULL;
      gchar *text;

      for (iter = missing_fonts; iter; iter = iter->next)
        {
          gchar *fontname = iter->data;

          if (font_list == NULL)
            {
              font_list = g_strdup (fontname);
            }
          else
            {
              gchar *tmp = font_list;

              font_list = g_strjoin (", ", tmp, fontname, NULL);
              g_free (tmp);
            }
        }

      text = g_strdup_printf (_("The following fonts cannot be found: %s.\n"
                                "It is recommended to convert your text layers to image "
                                "or to install the missing fonts before exporting, "
                                "otherwise your design may not look right."),
                              font_list);
      /* TODO: we used to have a GtkImage showing a GIMP_ICON_WILBER_EEK
       * icon in GTK_ICON_SIZE_BUTTON size, next to the label, to make
       * the warning more obvious.
       */
      widget = gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (window),
                                                "missing-fonts-label",
                                                text, FALSE, FALSE);
      gtk_label_set_line_wrap (GTK_LABEL (widget), TRUE);
      gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (window),
                                        "convert-text-layers-frame",
                                        "convert-text-layers", TRUE,
                                        "missing-fonts-label");

      dialog_props = g_list_prepend (dialog_props, "convert-text-layers-frame");

      g_list_free_full (missing_fonts, g_free);
      g_free (text);
      g_free (font_list);
    }
  else
    {
      dialog_props = g_list_prepend (dialog_props, "convert-text-layers");
    }

  dialog_props = g_list_prepend (dialog_props, "fill-background-color");
  dialog_props = g_list_prepend (dialog_props, "ignore-hidden");
  dialog_props = g_list_prepend (dialog_props, "vectorize");
  dialog_props = g_list_prepend (dialog_props, "apply-masks");
  dialog_props = g_list_prepend (dialog_props, "pages-frame");
  gimp_procedure_dialog_fill_list (GIMP_PROCEDURE_DIALOG (window),
                                   dialog_props);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (window));

  gtk_widget_destroy (window);

  return run;
}

/* The main GUI function for saving multi-paged PDFs */

static gboolean
gui_multi (GimpProcedure       *procedure,
           GimpProcedureConfig *config)
{
  GtkWidget   *window;
  GtkWidget   *vbox;
  GtkWidget   *file_label;
  GtkWidget   *file_entry;
  GtkWidget   *file_browse;
  GtkWidget   *file_hbox;
  GtkWidget   *scroll;
  GtkWidget   *page_view;
  GtkWidget   *h_but_box;
  GtkWidget   *del;
  GtkWidget   *h_box;
  GtkWidget   *img_combo;
  GtkWidget   *add_image;
  gboolean     run;
  const gchar *temp;

  gimp_ui_init (PLUG_IN_BINARY);

  window = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Export Image as Multi-Page PDF"));
  gimp_procedure_dialog_set_ok_label (GIMP_PROCEDURE_DIALOG (window), _("_Export"));

  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (window),
                                         "multi-pdf-options-vbox",
                                         "apply-masks", "vectorize",
                                         "fill-background-color",
                                         "ignore-hidden", NULL);

  file_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  file_label = gtk_label_new (_("Save to:"));
  file_entry = gtk_entry_new ();
  if (file_name != NULL)
    gtk_entry_set_text (GTK_ENTRY (file_entry), file_name);
  file_browse = gtk_button_new_with_label (_("Browse..."));
  file_choose = gtk_file_chooser_dialog_new (_("Multipage PDF export"),
                                             GTK_WINDOW (window),
                                             GTK_FILE_CHOOSER_ACTION_SAVE,
                                             _("_Save"),   GTK_RESPONSE_OK,
                                             _("_Cancel"), GTK_RESPONSE_CANCEL,
                                             NULL);

  gtk_box_pack_start (GTK_BOX (file_hbox), file_label, FALSE, FALSE, 0);
  gtk_widget_set_visible (file_label, TRUE);
  gtk_box_pack_start (GTK_BOX (file_hbox), file_entry, TRUE, TRUE, 0);
  gtk_widget_set_visible (file_entry, TRUE);
  gtk_box_pack_start (GTK_BOX (file_hbox), file_browse, FALSE, FALSE, 0);
  gtk_widget_set_visible (file_browse, TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), file_hbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (file_hbox, TRUE);
  gtk_box_reorder_child (GTK_BOX (vbox), file_hbox, 0);

  page_view = gtk_icon_view_new ();
  model = create_model ();
  gtk_icon_view_set_model (GTK_ICON_VIEW (page_view), model);
  gtk_icon_view_set_reorderable (GTK_ICON_VIEW (page_view), TRUE);
  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (page_view),
                                    GTK_SELECTION_MULTIPLE);

  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (page_view), THUMB);
  gtk_icon_view_set_text_column (GTK_ICON_VIEW (page_view), PAGE_NUMBER);
  gtk_icon_view_set_tooltip_column (GTK_ICON_VIEW (page_view), IMAGE_NAME);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scroll, -1, 300);
  gtk_widget_set_visible (scroll, TRUE);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_container_add (GTK_CONTAINER (scroll), page_view);
  gtk_widget_set_visible (page_view, TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);
  gtk_box_reorder_child (GTK_BOX (vbox), scroll, 1);

  h_but_box = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (h_but_box), GTK_BUTTONBOX_START);

  del = gtk_button_new_with_label (_("Remove the selected pages"));
  gtk_box_pack_start (GTK_BOX (h_but_box), del, TRUE, TRUE, 0);
  gtk_widget_set_visible (del, TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), h_but_box, FALSE, FALSE, 0);
  gtk_widget_set_visible (h_but_box, TRUE);
  gtk_box_reorder_child (GTK_BOX (vbox), h_but_box, 2);

  h_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);

  img_combo = gimp_image_combo_box_new (NULL, NULL, NULL);
  gtk_box_pack_start (GTK_BOX (h_box), img_combo, FALSE, FALSE, 0);
  gtk_widget_set_visible (img_combo, TRUE);

  add_image = gtk_button_new_with_label (_("Add this image"));
  gtk_box_pack_start (GTK_BOX (h_box), add_image, FALSE, FALSE, 0);
  gtk_widget_set_visible (add_image, TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), h_box, FALSE, FALSE, 0);
  gtk_widget_set_visible (h_box, TRUE);
  gtk_box_reorder_child (GTK_BOX (vbox), h_box, 3);

  g_signal_connect (G_OBJECT (file_browse), "clicked",
                    G_CALLBACK (choose_file_call),
                    file_entry);

  g_signal_connect (G_OBJECT (add_image), "clicked",
                    G_CALLBACK (add_image_call),
                    img_combo);

  g_signal_connect (G_OBJECT (del), "clicked",
                    G_CALLBACK (del_image_call),
                    page_view);

  g_signal_connect (G_OBJECT (model), "row-deleted",
                    G_CALLBACK (remove_call),
                    NULL);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (window),
                              "multi-pdf-options-vbox",
                              NULL);
  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (window));

  if (run)
    {
      run &= get_image_list ();

      temp = gtk_entry_get_text (GTK_ENTRY (file_entry));
      if (temp != NULL)
        {
          g_stpcpy (file_name, temp);
          g_object_set (config, "uri", temp, NULL);
        }
    }

  gtk_widget_destroy (window);

  return run;
}

/* A function that is called when the button for browsing for file
 * locations was clicked
 */
static void
choose_file_call (GtkWidget *browse_button,
                  gpointer   file_entry)
{
  GFile *file = g_file_new_for_path (gtk_entry_get_text (GTK_ENTRY (file_entry)));

  gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (file_choose),
                            g_file_get_uri (file));

  if (gtk_dialog_run (GTK_DIALOG (file_choose)) == GTK_RESPONSE_OK)
    {
      file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (file_choose));
      gtk_entry_set_text (GTK_ENTRY (file_entry), g_file_peek_path (file));
    }

  file_name = g_file_get_path (file);
  gtk_widget_set_visible (file_choose, FALSE);
}

/* A function to create the basic GtkTreeModel for the icon view */
static GtkTreeModel*
create_model (void)
{
  GtkListStore *model;
  guint32       i;

  /* validate_image_list was called earlier, so all the images
   * up to multi_page.image_count are valid
   */
  model = gtk_list_store_new (4,
                              GDK_TYPE_PIXBUF,  /* THUMB */
                              G_TYPE_STRING,    /* PAGE_NUMBER */
                              G_TYPE_STRING,    /* IMAGE_NAME */
                              GIMP_TYPE_IMAGE); /* IMAGE */

  for (i = 0 ; i < multi_page.image_count && i < MAX_PAGE_COUNT ; i++)
    {
      GtkTreeIter  iter;
      GimpImage   *image = multi_page.images[i];
      GdkPixbuf   *pixbuf;

      pixbuf = gimp_image_get_thumbnail (image, THUMB_WIDTH, THUMB_HEIGHT,
                                         GIMP_PIXBUF_SMALL_CHECKS);

      gtk_list_store_append (model, &iter);
      gtk_list_store_set (model, &iter,
                          THUMB,       pixbuf,
                          PAGE_NUMBER, g_strdup_printf (_("Page %d"), i + 1),
                          IMAGE_NAME,  gimp_image_get_name (image),
                          IMAGE,       image,
                          -1);

      g_object_unref (pixbuf);
    }

  return GTK_TREE_MODEL (model);
}

/* A function that puts the images from the model inside the images
 * (pages) array
 */
static gboolean
get_image_list (void)
{
  GtkTreeIter iter;
  gboolean    valid;

  multi_page.image_count = 0;

  for (valid = gtk_tree_model_get_iter_first (model, &iter);
       valid;
       valid = gtk_tree_model_iter_next (model, &iter))
    {
      GimpImage *image;

      gtk_tree_model_get (model, &iter,
                          IMAGE, &image,
                          -1);

      multi_page.images[multi_page.image_count] = image;
      multi_page.image_count++;

      g_object_unref (image);
    }

  if (multi_page.image_count == 0)
    {
      g_message (_("Error! In order to save the file, at least one image "
                   "should be added!"));
      return FALSE;
    }

  return TRUE;
}

/* A function that is called when the button for adding an image was
 * clicked
 */
static void
add_image_call (GtkWidget *widget,
                gpointer   img_combo)
{
  GtkListStore *store;
  GtkTreeIter   iter;
  gint32        image_id;
  GimpImage    *image;
  GdkPixbuf    *pixbuf;

  dnd_remove = FALSE;

  gimp_int_combo_box_get_active (img_combo, &image_id);
  image = gimp_image_get_by_id (image_id);

  store = GTK_LIST_STORE (model);

  pixbuf = gimp_image_get_thumbnail (image, THUMB_WIDTH, THUMB_HEIGHT,
                                     GIMP_PIXBUF_SMALL_CHECKS);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      PAGE_NUMBER, g_strdup_printf (_("Page %d"),
                                                    multi_page.image_count+1),
                      THUMB,       pixbuf,
                      IMAGE_NAME,  gimp_image_get_name (image),
                      IMAGE,       image,
                      -1);

  g_object_unref (pixbuf);

  multi_page.image_count++;

  dnd_remove = TRUE;
}

/* A function that is called when the button for deleting the selected
 * images was clicked
 */
static void
del_image_call (GtkWidget *widget,
                gpointer   icon_view)
{
  GList                *list;
  GtkTreeRowReference **items;
  GtkTreePath          *item_path;
  GtkTreeIter           item;
  gpointer              temp;
  guint32               len;

  dnd_remove = FALSE;

  list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (icon_view));

  len = g_list_length (list);
  if (len > 0)
    {
      gint i;

      items = g_newa (GtkTreeRowReference*, len);

      for (i = 0; i < len; i++)
        {
          temp = g_list_nth_data (list, i);
          items[i] = gtk_tree_row_reference_new (model, temp);
          gtk_tree_path_free (temp);
        }
      g_list_free (list);

      for (i = 0; i < len; i++)
        {
          item_path = gtk_tree_row_reference_get_path (items[i]);

          gtk_tree_model_get_iter (model, &item, item_path);
          gtk_list_store_remove (GTK_LIST_STORE (model), &item);

          gtk_tree_path_free (item_path);

          gtk_tree_row_reference_free (items[i]);
          multi_page.image_count--;
        }
    }

  dnd_remove = TRUE;

  recount_pages ();
}

/* A function that is called on rows-deleted signal. It will call the
 * function to relabel the pages
 */
static void
remove_call (GtkTreeModel *tree_model,
             GtkTreePath  *path,
             gpointer      user_data)
{

  if (dnd_remove)
    /* The gtk documentation says that we should not free the indices array */
    recount_pages ();
}

/* A function to relabel the pages in the icon view, when their order
 * was changed
 */
static void
recount_pages (void)
{
  GtkListStore *store;
  GtkTreeIter   iter;
  gboolean      valid;
  gint32        i = 0;

  store = GTK_LIST_STORE (model);

  for (valid = gtk_tree_model_get_iter_first (model, &iter);
       valid;
       valid = gtk_tree_model_iter_next (model, &iter))
    {
      gtk_list_store_set (store, &iter,
                          PAGE_NUMBER, g_strdup_printf (_("Page %d"), i + 1),
                          -1);
      i++;
    }
}


/******************************************************/
/* Beginning of the actual PDF functions              */
/******************************************************/

static cairo_surface_t *
get_cairo_surface (GimpDrawable  *drawable,
                   gboolean       as_mask,
                   GError       **error)
{
  GeglBuffer      *src_buffer;
  GeglBuffer      *dest_buffer;
  cairo_surface_t *surface;
  cairo_status_t   status;
  cairo_format_t   format;
  gint             width;
  gint             height;

  src_buffer = gimp_drawable_get_buffer (drawable);

  width  = gegl_buffer_get_width  (src_buffer);
  height = gegl_buffer_get_height (src_buffer);

  if (as_mask)
    format = CAIRO_FORMAT_A8;
  else if (gimp_drawable_has_alpha (drawable))
    format = CAIRO_FORMAT_ARGB32;
  else
    format = CAIRO_FORMAT_RGB24;

  surface = cairo_image_surface_create (format, width, height);

  status = cairo_surface_status (surface);
  if (status != CAIRO_STATUS_SUCCESS)
    {
      switch (status)
        {
        case CAIRO_STATUS_INVALID_SIZE:
          g_set_error_literal (error,
                               GIMP_PLUGIN_PDF_EXPORT_ERROR,
                               GIMP_PLUGIN_PDF_EXPORT_ERROR_FAILED,
                               _("Cannot handle the size (either width or height) of the image."));
          break;
        default:
          g_set_error (error,
                       GIMP_PLUGIN_PDF_EXPORT_ERROR,
                       GIMP_PLUGIN_PDF_EXPORT_ERROR_FAILED,
                       "Cairo error: %s",
                       cairo_status_to_string (status));
          break;
        }

      return NULL;
    }

  dest_buffer = gimp_cairo_surface_create_buffer (surface, NULL);
  if (as_mask)
    {
      /* src_buffer represents a mask in "Y u8", "Y u16", etc. formats.
       * Yet cairo_mask*() functions only care about the alpha channel of
       * the surface. Hence I change the format of dest_buffer so that the
       * Y channel of src_buffer actually refers to the A channel of
       * dest_buffer/surface in Cairo.
       */
      gegl_buffer_set_format (dest_buffer, babl_format ("Y u8"));
    }

  gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE, dest_buffer, NULL);

  cairo_surface_mark_dirty (surface);

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  return surface;
}

/* A function to check if a drawable is single colored This allows to
 * convert bitmaps to vector where possible
 */
static GeglColor *
get_layer_color (GimpLayer *layer,
                 gboolean  *single)
{
  GeglColor *col = gegl_color_new (NULL);
  gdouble red, green, blue, alpha;
  gdouble dev, devSum;
  gdouble median, pixels, count, percentile;

  devSum = 0;
  red = 0;
  green = 0;
  blue = 0;
  alpha = 0;
  dev = 0;

  if (gimp_drawable_is_indexed (GIMP_DRAWABLE (layer)))
    {
      /* FIXME: We can't do a proper histogram on indexed layers! */
      *single = FALSE;
      gegl_color_set_rgba (col, 0.0, 0.0, 0.0, 0.0);
      return col;
    }

  if (gimp_drawable_get_bpp (GIMP_DRAWABLE (layer)) >= 3)
    {
      /* Are we in RGB mode? */

      gimp_drawable_histogram (GIMP_DRAWABLE (layer),
                               GIMP_HISTOGRAM_RED, 0.0, 1.0,
                               &red, &dev, &median, &pixels, &count, &percentile);
      devSum += dev;

      gimp_drawable_histogram (GIMP_DRAWABLE (layer),
                               GIMP_HISTOGRAM_GREEN, 0.0, 1.0,
                               &green, &dev, &median, &pixels, &count, &percentile);
      devSum += dev;

      gimp_drawable_histogram (GIMP_DRAWABLE (layer),
                               GIMP_HISTOGRAM_BLUE, 0.0, 1.0,
                               &blue, &dev, &median, &pixels, &count, &percentile);
      devSum += dev;
    }
  else
    {
      /* We are in Grayscale mode (or Indexed) */

      gimp_drawable_histogram (GIMP_DRAWABLE (layer),
                               GIMP_HISTOGRAM_VALUE, 0.0, 1.0,
                               &red, &dev, &median, &pixels, &count, &percentile);
      devSum += dev;
      green = red;
      blue = red;
    }

  if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    gimp_drawable_histogram (GIMP_DRAWABLE (layer),
                             GIMP_HISTOGRAM_ALPHA, 0.0, 1.0,
                             &alpha, &dev, &median, &pixels, &count, &percentile);
  else
    alpha = 255;

  devSum += dev;
  *single = devSum == 0;

  gegl_color_set_rgba (col, red, green, blue, alpha);
  return col;
}

/* A function that uses Pango to render the text to our cairo surface,
 * in the same way it was the user saw it inside gimp.
 * Needs some work on choosing the font name better, and on hinting
 * (freetype and pango differences)
 */
static void
drawText (GimpLayer *layer,
          gdouble    opacity,
          cairo_t   *cr,
          gdouble    x_res,
          gdouble    y_res)
{
  GimpImageType         type   = gimp_drawable_type (GIMP_DRAWABLE (layer));
  gchar                *text   = gimp_text_layer_get_text (GIMP_TEXT_LAYER (layer));
  gchar                *markup = gimp_text_layer_get_markup (GIMP_TEXT_LAYER (layer));
  gchar                *language;
  cairo_font_options_t *options;
  gint                  x;
  gint                  y;
  GeglColor            *color;
  gdouble               rgb[3];
  GimpUnit             *unit;
  gdouble               size;
  GimpTextHintStyle     hinting;
  GimpTextJustification j;
  gboolean              justify;
  PangoAlignment        align;
  GimpTextDirection     dir;
  PangoLayout          *layout;
  PangoContext         *context;
  GimpFont             *font;
  PangoFontDescription *font_description;
  gdouble               indent;
  gdouble               line_spacing;
  gdouble               letter_spacing;
  PangoAttribute       *letter_spacing_at;
  PangoAttrList        *attr_list = pango_attr_list_new ();
  PangoFontMap         *fontmap;

  cairo_save (cr);

  options = cairo_font_options_create ();
  attr_list = pango_attr_list_new ();
  cairo_get_font_options (cr, options);

  /* Position */
  gimp_drawable_get_offsets (GIMP_DRAWABLE (layer), &x, &y);
  cairo_translate (cr, x, y);

  /* Color */
  /* When dealing with a gray/indexed image, the viewed color of the text layer
   * can be different than the one kept in the memory */
  if (type == GIMP_RGBA_IMAGE)
    color = gimp_text_layer_get_color (GIMP_TEXT_LAYER (layer));
  else
    gimp_image_pick_color (gimp_item_get_image (GIMP_ITEM (layer)), 1,
                           (const GimpItem**) &layer, x, y, FALSE, FALSE, 0,
                           &color);

  /* TODO: this export plug-in is not space-aware yet, so we draw everything as
   * sRGB for the time being.
   */
  gegl_color_get_pixel (color, babl_format_with_space ("R'G'B' double", NULL), rgb);
  cairo_set_source_rgba (cr, rgb[0], rgb[1], rgb[2], opacity);
  g_object_unref (color);

  /* Hinting */
  hinting = gimp_text_layer_get_hint_style (GIMP_TEXT_LAYER (layer));
  switch (hinting)
    {
    case GIMP_TEXT_HINT_STYLE_NONE:
      cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_NONE);
      break;

    case GIMP_TEXT_HINT_STYLE_SLIGHT:
      cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_SLIGHT);
      break;

    case GIMP_TEXT_HINT_STYLE_MEDIUM:
      cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_MEDIUM);
      break;

    case GIMP_TEXT_HINT_STYLE_FULL:
      cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_FULL);
      break;
    }

  /* Antialiasing */
  if (gimp_text_layer_get_antialias (GIMP_TEXT_LAYER (layer)))
    cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_DEFAULT);
  else
    cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_NONE);

  /* We are done with cairo's settings. It's time to create the
   * context
   */
  fontmap = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);

  /* Font */
  font = gimp_text_layer_get_font (GIMP_TEXT_LAYER (layer));
  font_description = gimp_font_get_pango_font_description (font);

  /* This function breaks rendering with some fonts if it's called before
   * gimp_font_get_pango_font_description(). I'm still unsure why yet it
   * probably means there is a bug somewhere we must fix. Until then, let's make
   * sure we keep this order. XXX
   */
  pango_cairo_font_map_set_resolution (PANGO_CAIRO_FONT_MAP (fontmap), y_res);

  context = pango_font_map_create_context (fontmap);
  g_object_unref (fontmap);

  pango_cairo_context_set_font_options (context, options);

  /* Language */
  language = gimp_text_layer_get_language (GIMP_TEXT_LAYER (layer));
  if (language)
    pango_context_set_language (context,
                                pango_language_from_string (language));

  /* Text Direction */
  dir = gimp_text_layer_get_base_direction (GIMP_TEXT_LAYER (layer));

  switch (dir)
    {
    case GIMP_TEXT_DIRECTION_LTR:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_NATURAL);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_SOUTH);
      break;

    case GIMP_TEXT_DIRECTION_RTL:
      pango_context_set_base_dir (context, PANGO_DIRECTION_RTL);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_NATURAL);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_SOUTH);
      break;

    case GIMP_TEXT_DIRECTION_TTB_RTL:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_LINE);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_EAST);
      break;

    case GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_STRONG);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_EAST);
      break;

    case GIMP_TEXT_DIRECTION_TTB_LTR:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_LINE);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_WEST);
      break;

    case GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_STRONG);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_WEST);
      break;
    }

  /* We are done with the context's settings. It's time to create the
   * layout
   */
  layout = pango_layout_new (context);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);

  /* Font Size */
  size = gimp_text_layer_get_font_size (GIMP_TEXT_LAYER (layer), &unit);
  size = gimp_units_to_pixels (size, unit, y_res);
  pango_font_description_set_absolute_size (font_description, size * PANGO_SCALE);

  pango_layout_set_font_description (layout, font_description);

  /* Width */
  if (! PANGO_GRAVITY_IS_VERTICAL (pango_context_get_base_gravity (context)))
    pango_layout_set_width (layout,
                            gimp_drawable_get_width (GIMP_DRAWABLE (layer)) *
                            PANGO_SCALE);
  else
    pango_layout_set_width (layout,
                            gimp_drawable_get_height (GIMP_DRAWABLE (layer)) *
                            PANGO_SCALE);

  /* Justification, and Alignment */
  justify = FALSE;
  j = gimp_text_layer_get_justification (GIMP_TEXT_LAYER (layer));
  align = PANGO_ALIGN_LEFT;
  switch (j)
    {
    case GIMP_TEXT_JUSTIFY_LEFT:
      align = PANGO_ALIGN_LEFT;
      break;
    case GIMP_TEXT_JUSTIFY_RIGHT:
      align = PANGO_ALIGN_RIGHT;
      break;
    case GIMP_TEXT_JUSTIFY_CENTER:
      align = PANGO_ALIGN_CENTER;
      break;
    case GIMP_TEXT_JUSTIFY_FILL:
      align = PANGO_ALIGN_LEFT;
      justify = TRUE;
      break;
    }
  pango_layout_set_alignment (layout, align);
  pango_layout_set_justify (layout, justify);

  /* Indentation */
  indent = gimp_text_layer_get_indent (GIMP_TEXT_LAYER (layer));
  pango_layout_set_indent (layout, (int)(PANGO_SCALE * indent));

  /* Line Spacing */
  line_spacing = gimp_text_layer_get_line_spacing (GIMP_TEXT_LAYER (layer));
  pango_layout_set_spacing (layout, (int)(PANGO_SCALE * line_spacing));

  /* Letter Spacing */
  letter_spacing = gimp_text_layer_get_letter_spacing (GIMP_TEXT_LAYER (layer));
  letter_spacing_at = pango_attr_letter_spacing_new ((int)(PANGO_SCALE * letter_spacing));
  pango_attr_list_insert (attr_list, letter_spacing_at);


  pango_layout_set_attributes (layout, attr_list);

  /* Use the pango markup of the text layer */

  if (markup != NULL && markup[0] != '\0')
    pango_layout_set_markup (layout, markup, -1);
  else /* If we can't find a markup, then it has just text */
    pango_layout_set_text (layout, text, -1);

  if (dir == GIMP_TEXT_DIRECTION_TTB_RTL ||
      dir == GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT)
    {
      cairo_translate (cr, gimp_drawable_get_width (GIMP_DRAWABLE (layer)), 0);
      cairo_rotate (cr, G_PI_2);
    }

  if (dir == GIMP_TEXT_DIRECTION_TTB_LTR ||
      dir == GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT)
    {
      cairo_translate (cr, 0, gimp_drawable_get_height (GIMP_DRAWABLE (layer)));
      cairo_rotate (cr, -G_PI_2);
    }

  pango_cairo_show_layout (cr, layout);

  g_free (text);
  g_free (language);

  g_object_unref (layout);
  pango_font_description_free (font_description);
  g_object_unref (context);
  pango_attr_list_unref (attr_list);

  cairo_font_options_destroy (options);

  cairo_restore (cr);
}

static gboolean
draw_layer (GimpLayer           **layers,
            gint                  n_layers,
            GimpProcedureConfig  *config,
            gboolean              single_image,
            gint                  j,
            cairo_t              *cr,
            gdouble               x_res,
            gdouble               y_res,
            const gchar          *name,
            gboolean              show_progress,
            gdouble               progress_start,
            gdouble               progress_end,
            gint                  layer_level,
            GError              **error)
{
  GimpLayer *layer;
  gdouble    opacity;
  gboolean   vectorize;
  gboolean   ignore_hidden;
  gboolean   layers_as_pages = FALSE;
  gboolean   reverse_order   = FALSE;
  gboolean   root_layers_only;
  gboolean   convert_text;

  g_object_get (config,
                "vectorize",           &vectorize,
                "ignore-hidden",       &ignore_hidden,
                NULL);

  if (single_image)
    g_object_get (config,
                  "layers-as-pages",     &layers_as_pages,
                  "reverse-order",       &reverse_order,
                  "root-layers-only",    &root_layers_only,
                  "convert-text-layers", &convert_text,
                  NULL);

  if (reverse_order && layers_as_pages)
    layer = layers [j];
  else
    layer = layers [n_layers - j - 1];

  opacity = gimp_layer_get_opacity (layer) / 100.0;
  if ((! gimp_item_get_visible (GIMP_ITEM (layer)) || opacity == 0.0) &&
      ignore_hidden)
    return TRUE;

  if (gimp_item_is_group (GIMP_ITEM (layer)))
    {
      GimpItem **children;
      gint       children_num;
      gint       i;

      children = gimp_item_get_children (GIMP_ITEM (layer), &children_num);
      for (i = 0; i < children_num; i++)
        {
          if (! draw_layer ((GimpLayer **) children, children_num,
                            config, single_image, i,
                            cr, x_res, y_res, name,
                            show_progress,
                            progress_start + i * (progress_end - progress_start) / children_num,
                            progress_end,
                            layer_level + 1, error))
            {
              g_free (children);
              return FALSE;
            }
        }
      g_free (children);

      if (root_layers_only && layers_as_pages &&
          children_num > 0 && layer_level == 0)
        cairo_show_page (cr);
    }
  else
    {
      cairo_surface_t *mask_image = NULL;
      GimpLayerMask   *mask       = NULL;
      gint             x, y;

      if (show_progress)
        gimp_progress_update (progress_start);

      mask = gimp_layer_get_mask (layer);
      if (mask)
        {
          mask_image = get_cairo_surface (GIMP_DRAWABLE (mask), TRUE,
                                          error);

          if (*error)
            return FALSE;
        }

      gimp_drawable_get_offsets (GIMP_DRAWABLE (layer), &x, &y);

      if (! gimp_item_is_text_layer (GIMP_ITEM (layer)) || convert_text)
        {
          /* For raster layers */

          GeglColor *layer_color;
          gboolean    single_color = FALSE;

          layer_color = get_layer_color (layer, &single_color);

          cairo_rectangle (cr, x, y,
                           gimp_drawable_get_width  (GIMP_DRAWABLE (layer)),
                           gimp_drawable_get_height (GIMP_DRAWABLE (layer)));

          if (vectorize && single_color)
            {
              gimp_cairo_set_source_color (cr, layer_color, NULL, FALSE, NULL);
              if (mask)
                cairo_mask_surface (cr, mask_image, x, y);
              else
                cairo_fill (cr);
            }
          else
            {
              cairo_surface_t *layer_image;

              layer_image = get_cairo_surface (GIMP_DRAWABLE (layer), FALSE,
                                               error);

              if (*error)
                return FALSE;

              cairo_clip (cr);

              cairo_set_source_surface (cr, layer_image, x, y);
              cairo_push_group (cr);
              cairo_paint_with_alpha (cr, opacity);
              cairo_pop_group_to_source (cr);

              if (mask)
                cairo_mask_surface (cr, mask_image, x, y);
              else
                cairo_paint (cr);

              cairo_reset_clip (cr);

              cairo_surface_destroy (layer_image);
            }

          g_object_unref (layer_color);
        }
      else
        {
          /* For text layers */
          drawText (layer, opacity, cr, x_res, y_res);
        }

      /* draw new page if "layers as pages" option is checked */
      if (layers_as_pages && (! root_layers_only || layer_level == 0))
        cairo_show_page (cr);

      /* We are done with the layer - time to free some resources */
      if (mask)
        cairo_surface_destroy (mask_image);
    }

  return TRUE;
}
