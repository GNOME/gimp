/* GIMP - The GNU Image Manipulation Program
 *
 * file-pdf-save.c - PDF file exporter, based on the cairo PDF surface
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
 * 1. file-pdf-save
 *    This is the main procedure. It has 3 options for optimizations of
 *    the pdf file, and it can show a gui. This procedure works on a single
 *    image.
 * 2. file-pdf-save-defaults
 *    This procedures is the one that will be invoked by gimp's file-save,
 *    when the pdf extension is chosen. If it's in RUN_INTERACTIVE, it will
 *    pop a user interface with more options, like file-pdf-save. If it's in
 *    RUN_NONINTERACTIVE, it will simply use the default values. Note that on
 *    RUN_WITH_LAST_VALS there will be no gui, however the values will be the
 *    ones that were used in the last interactive run (or the defaults if none
 *    are available.
 * 3. file-pdf-save-multi
 *    This procedures is more advanced, and it allows the creation of multiple
 *    paged pdf files. It will be located in File/Create/Multiple page PDF...
 *
 * It was suggested that file-pdf-save-multi will be removed from the UI as it
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
 *   - fixed the behaviour of getting font sizes
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


#define SAVE_PROC               "file-pdf-save"
#define SAVE_MULTI_PROC         "file-pdf-save-multi"
#define PLUG_IN_BINARY          "file-pdf-save"
#define PLUG_IN_ROLE            "gimp-file-pdf-save"

#define DATA_OPTIMIZE           "file-pdf-data-optimize"
#define DATA_IMAGE_LIST         "file-pdf-data-multi-page"

/* Gimp will crash before you reach this limitation :D */
#define MAX_PAGE_COUNT           350
#define MAX_FILE_NAME_LENGTH     350

#define THUMB_WIDTH              90
#define THUMB_HEIGHT             120

#define GIMP_PLUGIN_PDF_SAVE_ERROR gimp_plugin_pdf_save_error_quark ()

typedef enum
{
  GIMP_PLUGIN_PDF_SAVE_ERROR_FAILED
} GimpPluginPDFError;

GQuark gimp_plugin_pdf_save_error_quark (void);

typedef enum
{
  SA_VECTORIZE,
  SA_IGNORE_HIDDEN,
  SA_APPLY_MASKS,
  SA_LAYERS_AS_PAGES,
  SA_REVERSE_ORDER,
  SA_CONVERT_TEXT
} SaveArgs;

typedef enum
{
  SMA_RUN_MODE,
  SMA_COUNT,
  SMA_IMAGES,
  SMA_VECTORIZE,
  SMA_IGNORE_HIDDEN,
  SMA_APPLY_MASKS,
  SMA_FILENAME
} SaveMultiArgs;

typedef struct
{
  gboolean vectorize;
  gboolean ignore_hidden;
  gboolean apply_masks;
  gboolean layers_as_pages;
  gboolean reverse_order;
  gboolean convert_text;
} PdfOptimize;

typedef struct
{
  GimpImage *images[MAX_PAGE_COUNT];
  guint32    image_count;
  gchar      file_name[MAX_FILE_NAME_LENGTH];
} PdfMultiPage;

typedef struct
{
  PdfOptimize  optimize;
  GArray      *images;
} PdfMultiVals;

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
#define PDF (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PDF_TYPE, Pdf))

GType                   pdf_get_type             (void) G_GNUC_CONST;

static GList          * pdf_query_procedures     (GimpPlugIn           *plug_in);
static GimpProcedure  * pdf_create_procedure     (GimpPlugIn           *plug_in,
                                                  const gchar          *name);

static GimpValueArray * pdf_save                 (GimpProcedure        *procedure,
                                                  GimpRunMode           run_mode,
                                                  GimpImage            *image,
                                                  gint                  n_drawables,
                                                  GimpDrawable        **drawables,
                                                  GFile                *file,
                                                  const GimpValueArray *args,
                                                  gpointer              run_data);
static GimpValueArray * pdf_save_multi           (GimpProcedure        *procedure,
                                                  const GimpValueArray *args,
                                                  gpointer              run_data);

static GimpValueArray * pdf_save_image           (GimpProcedure        *procedure,
                                                  gboolean              single_image,
                                                  gboolean              defaults_proc);

static void             init_image_list_defaults (GimpImage            *image);

static void             validate_image_list      (void);

static gboolean         gui_single               (void);
static gboolean         gui_multi                (void);

static void             reverse_order_toggled    (GtkToggleButton      *reverse_order,
                                                  GtkButton            *layers_as_pages);

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

static GimpRGB          get_layer_color          (GimpLayer            *layer,
                                                  gboolean             *single);

static void             drawText                 (GimpLayer            *layer,
                                                  gdouble               opacity,
                                                  cairo_t              *cr,
                                                  gdouble               x_res,
                                                  gdouble               y_res);

static gboolean         draw_layer               (GimpLayer           **layers,
                                                  gint                  n_layers,
                                                  gint                  j,
                                                  cairo_t              *cr,
                                                  gdouble               x_res,
                                                  gdouble               y_res,
                                                  const gchar          *name,
                                                  GError              **error);


G_DEFINE_TYPE (Pdf, pdf, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PDF_TYPE)


static gboolean     dnd_remove = TRUE;
static PdfMultiPage multi_page;

static PdfOptimize optimize =
{
  TRUE,  /* vectorize */
  TRUE,  /* ignore_hidden */
  TRUE,  /* apply_masks */
  FALSE, /* layers_as_pages */
  FALSE, /* reverse_order */
  FALSE  /* convert text layers to raster */
};

static GtkTreeModel *model;
static GtkWidget    *file_choose;
static gchar        *file_name;


G_DEFINE_QUARK (gimp-plugin-pdf-save-error-quark, gimp_plugin_pdf_save_error)

static void
pdf_class_init (PdfClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = pdf_query_procedures;
  plug_in_class->create_procedure = pdf_create_procedure;
}

static void
pdf_init (Pdf *pdf)
{
}

static GList *
pdf_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (SAVE_PROC));
  list = g_list_append (list, g_strdup (SAVE_MULTI_PROC));

  return list;
}

static GimpProcedure *
pdf_create_procedure (GimpPlugIn  *plug_in,
                        const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           pdf_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("Portable Document Format"));

      gimp_procedure_set_documentation (procedure,
                                        "Save files in PDF format",
                                        "Saves files in Adobe's Portable "
                                        "Document Format. PDF is designed to "
                                        "be easily processed by a variety of "
                                        "different platforms, and is a "
                                        "distant cousin of PostScript.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Barak Itkin, Lionel N., Jehan",
                                      "Copyright Barak Itkin, Lionel N., Jehan",
                                      "August 2009, 2017");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "application/pdf");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "pdf");

      GIMP_PROC_ARG_BOOLEAN (procedure, "vectorize",
                             "Vectorize",
                             "Convert bitmaps to vector graphics where possible.",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "ignore-hidden",
                             "Ignore hidden",
                             "Omit hidden layers and layers with zero opacity.",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "apply-masks",
                             "Apply masks",
                             "Apply layer masks before saving (Keeping them "
                             "will not change the output),",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "layers-as-pages",
                             "Layers as pages",
                             "Layers as pages (bottom layers first).",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "reverse-order",
                             "Reverse order",
                             "Reverse the pages order (top layers first).",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "convert-text-layers",
                             "Convert text layers to image",
                             "Convert text layers to raster graphics",
                             FALSE,
                             G_PARAM_READWRITE);
    }
  else if (! strcmp (name, SAVE_MULTI_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      pdf_save_multi, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("_Create multipage PDF..."));
#if 0
      gimp_procedure_add_menu_path (procedure, "<Image>/File/Create/PDF");
#endif

      gimp_procedure_set_documentation (procedure,
                                        "Save files in PDF format",
                                        "Saves files in Adobe's Portable "
                                        "Document Format. PDF is designed to "
                                        "be easily processed by a variety of "
                                        "different platforms, and is a "
                                        "distant cousin of PostScript.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Barak Itkin",
                                      "Copyright Barak Itkin",
                                      "August 2009");

      GIMP_PROC_ARG_ENUM (procedure, "run-mode",
                          "Run mode",
                          "The run mode",
                          GIMP_TYPE_RUN_MODE,
                          GIMP_RUN_INTERACTIVE,
                          G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "count",
                         "Count",
                         "The number of images entered (This will be the "
                         "number of pages).",
                         1, MAX_PAGE_COUNT, 1,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT32_ARRAY (procedure, "images",
                                 "Images",
                                 "Input image for each page (An image can "
                                 "appear more than once)",
                                 G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "vectorize",
                             "Vectorize",
                             "Convert bitmaps to vector graphics where possible.",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "ignore-hidden",
                             "Ignore hidden",
                             "Omit hidden layers and layers with zero opacity.",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "apply-masks",
                             "Apply masks",
                             "Apply layer masks before saving (Keeping them "
                             "will not change the output),",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING (procedure, "uri",
                            "URI",
                            "The URI of the file to save to",
                            NULL,
                            GIMP_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
pdf_save (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          gint                  n_drawables,
          GimpDrawable        **drawables,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  gboolean had_saved_list = FALSE;
  gboolean defaults = FALSE;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  /* Initializing all the settings */
  multi_page.image_count = 0;

  file_name = g_file_get_path (file);

  if (run_mode == GIMP_RUN_NONINTERACTIVE)
    {
      optimize.apply_masks     = GIMP_VALUES_GET_BOOLEAN (args, SA_APPLY_MASKS);
      optimize.vectorize       = GIMP_VALUES_GET_BOOLEAN (args, SA_VECTORIZE);
      optimize.ignore_hidden   = GIMP_VALUES_GET_BOOLEAN (args, SA_IGNORE_HIDDEN);
      optimize.layers_as_pages = GIMP_VALUES_GET_BOOLEAN (args, SA_LAYERS_AS_PAGES);
      optimize.reverse_order   = GIMP_VALUES_GET_BOOLEAN (args, SA_REVERSE_ORDER);
      optimize.convert_text    = GIMP_VALUES_GET_BOOLEAN (args, SA_CONVERT_TEXT);
    }
  else
    defaults = TRUE;

  switch (run_mode)
    {
    case GIMP_RUN_NONINTERACTIVE:
      init_image_list_defaults (image);
      break;

    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data (DATA_OPTIMIZE, &optimize);
      had_saved_list = gimp_get_data (DATA_IMAGE_LIST, &multi_page);

      if (had_saved_list && (file_name == NULL || strlen (file_name) == 0))
        {
          file_name = multi_page.file_name;
        }

      init_image_list_defaults (image);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      init_image_list_defaults (image);
      gimp_get_data (DATA_OPTIMIZE, &optimize);
      break;
    }

  validate_image_list ();

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! gui_single ())
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CANCEL,
                                                   NULL);
        }
    }

  return pdf_save_image (procedure, TRUE, defaults);
}

static GimpValueArray *
pdf_save_multi (GimpProcedure        *procedure,
                const GimpValueArray *args,
                gpointer              run_data)
{
  GimpRunMode  run_mode;
  GimpImage   *image = NULL;
  GFile       *file;
  gboolean     had_saved_list = FALSE;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = GIMP_VALUES_GET_ENUM (args, 0);

  file = g_file_new_for_uri (GIMP_VALUES_GET_STRING (args, SMA_FILENAME));
  file_name = g_file_get_path (file);

  /* Initializing all the settings */
  multi_page.image_count = 0;

  optimize.apply_masks   = GIMP_VALUES_GET_BOOLEAN (args, SMA_APPLY_MASKS);
  optimize.vectorize     = GIMP_VALUES_GET_BOOLEAN (args, SMA_VECTORIZE);
  optimize.ignore_hidden = GIMP_VALUES_GET_BOOLEAN (args, SMA_IGNORE_HIDDEN);

  switch (run_mode)
    {
      const gint32 *image_ids;
      gint          i;

    case GIMP_RUN_NONINTERACTIVE:
      multi_page.image_count = GIMP_VALUES_GET_INT (args, SMA_COUNT);
      image_ids = GIMP_VALUES_GET_INT32_ARRAY (args, SMA_IMAGES);
      if (image_ids)
        for (i = 0; i < multi_page.image_count; i++)
          multi_page.images[i] = gimp_image_get_by_id (image_ids[i]);
      break;

    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data (DATA_OPTIMIZE, &optimize);
      had_saved_list = gimp_get_data (DATA_IMAGE_LIST, &multi_page);

      if (had_saved_list && (file_name == NULL || strlen (file_name) == 0))
        {
          file_name = multi_page.file_name;
        }

      if (! had_saved_list)
        init_image_list_defaults (image);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      had_saved_list = gimp_get_data (DATA_IMAGE_LIST, &multi_page);
      if (had_saved_list)
        file_name = multi_page.file_name;

      gimp_get_data (DATA_OPTIMIZE, &optimize);
      break;
    }

  validate_image_list ();

  /* Starting the executions */
  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! gui_multi ())
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CANCEL,
                                                   NULL);
        }
    }

  return pdf_save_image (procedure, FALSE, FALSE);
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
          gchar                *font_family;
          PangoFontDescription *font_description;
          PangoFontDescription *font_description2;
          PangoFontMap         *fontmap;
          PangoFont            *font;
          PangoContext         *context;

          fontmap = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);
          context = pango_font_map_create_context (fontmap);

          font_family = gimp_text_layer_get_font (layer);
          font_description = pango_font_description_from_string (font_family);

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
          g_free (font_family);
          g_object_unref (context);
          g_object_unref (fontmap);
        }
    }

  g_list_free (layers);

  return missing_fonts;
}

static GimpValueArray *
pdf_save_image (GimpProcedure *procedure,
                gboolean       single_image,
                gboolean       defaults_proc)
{
  cairo_surface_t        *pdf_file;
  cairo_t                *cr;
  GimpExportCapabilities  capabilities;
  FILE                   *fp;
  gint                    i;
  GError                 *error = NULL;

  fp = g_fopen (file_name, "wb");
  if (! fp)
    {
      g_set_error (&error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (file_name), g_strerror (errno));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }

  pdf_file = cairo_pdf_surface_create_for_stream (write_func, fp, 1, 1);

  if (cairo_surface_status (pdf_file) != CAIRO_STATUS_SUCCESS)
    {
      g_message (_("An error occurred while creating the PDF file:\n"
                   "%s\n"
                   "Make sure you entered a valid filename and that the "
                   "selected location isn't read only!"),
                 cairo_status_to_string (cairo_surface_status (pdf_file)));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }

  cr = cairo_create (pdf_file);

  capabilities = (GIMP_EXPORT_CAN_HANDLE_RGB    |
                  GIMP_EXPORT_CAN_HANDLE_ALPHA  |
                  GIMP_EXPORT_CAN_HANDLE_GRAY   |
                  GIMP_EXPORT_CAN_HANDLE_LAYERS |
                  GIMP_EXPORT_CAN_HANDLE_INDEXED);
  /* This seems counter-intuitive, but not setting the mask capability
   * will apply any layer mask upon gimp_export_image().
   */
  if (! optimize.apply_masks)
    capabilities |= GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS;

  for (i = 0; i < multi_page.image_count; i++)
    {
      GimpImage     *image = multi_page.images[i];
      GimpLayer    **layers;
      gint32         n_layers;
      gdouble        x_res, y_res;
      gdouble        x_scale, y_scale;
      GimpDrawable **temp;
      GimpDrawable **temp_out;
      gint           temp_size = 1;
      gint           j;

      if (! gimp_image_get_active_drawable (image))
        continue;

      temp = g_new (GimpDrawable *, 1);
      temp[0] = gimp_image_get_active_drawable (image);
      temp_out = temp;

      /* Save the state of the surface before any changes, so that
       * settings from one page won't affect all the others
       */
      cairo_save (cr);

      if (! (gimp_export_image (&image, &temp_size, &temp, NULL,
                                capabilities) == GIMP_EXPORT_EXPORT))
        {
          /* gimp_drawable_histogram() only works within the bounds of
           * the selection, which is a problem (see issue #2431).
           * Instead of saving the selection, unselecting to later
           * reselect, let's just always work on a duplicate of the
           * image.
           */
          image = gimp_image_duplicate (image);
        }

      if (temp != temp_out)
        g_free (temp_out);
      g_free (temp);

      gimp_selection_none (image);

      gimp_image_get_resolution (image, &x_res, &y_res);
      x_scale = 72.0 / x_res;
      y_scale = 72.0 / y_res;

      cairo_pdf_surface_set_size (pdf_file,
                                  gimp_image_width  (image) * x_scale,
                                  gimp_image_height (image) * y_scale);

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

      /* Fill image with background color -
       * otherwise the output PDF will always show white for background,
       * and may display artifacts at transparency boundaries
       */
      if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layers[n_layers - 1])))
        {
          GimpRGB color;

          cairo_rectangle (cr, 0.0, 0.0,
                           gimp_image_width  (image),
                           gimp_image_height (image));
          gimp_context_get_background (&color);
          cairo_set_source_rgb (cr,
                                color.r,
                                color.g,
                                color.b);
          cairo_fill (cr);
        }

      /* Now, we should loop over the layers of each image */
      for (j = 0; j < n_layers; j++)
        {
          if (! draw_layer (layers, n_layers, j, cr, x_res, y_res,
                            gimp_procedure_get_name (procedure),
                            &error))
            {
              /* free the resources */
              g_free (layers);
              cairo_surface_destroy (pdf_file);
              cairo_destroy (cr);
              fclose (fp);

              return gimp_procedure_new_return_values (procedure,
                                                       GIMP_PDB_EXECUTION_ERROR,
                                                       error);
            }
        }

      g_free (layers);

      /* We are done with this image - Show it!
       * Unless that's a multi-page to avoid blank page at the end
       */

      if (! optimize.layers_as_pages)
        cairo_show_page (cr);

      cairo_restore (cr);

      gimp_image_delete (image);
    }

  /* We are done with all the images - time to free the resources */
  cairo_surface_destroy (pdf_file);
  cairo_destroy (cr);

  fclose (fp);

  /* Finally done, let's save the parameters */
  gimp_set_data (DATA_OPTIMIZE, &optimize, sizeof (optimize));

  if (! single_image)
    {
      g_strlcpy (multi_page.file_name, file_name, MAX_FILE_NAME_LENGTH);
      gimp_set_data (DATA_IMAGE_LIST, &multi_page, sizeof (multi_page));
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, error);
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
gui_single (void)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *vectorize_c;
  GtkWidget *ignore_hidden_c;
  GtkWidget *convert_text_c;
  GtkWidget *apply_c;
  GtkWidget *layers_as_pages_c;
  GtkWidget *reverse_order_c;
  GtkWidget *frame;
  gchar     *text;
  GList     *missing_fonts;
  gboolean   run;
  gint32     n_layers;

  gimp_ui_init (PLUG_IN_BINARY);

  window = gimp_export_dialog_new ("PDF", PLUG_IN_ROLE, SAVE_PROC);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (window)),
                      vbox, TRUE, TRUE, 0);

  gtk_container_set_border_width (GTK_CONTAINER (window), 12);

  convert_text_c = gtk_check_button_new_with_mnemonic (_("_Convert text layers to image"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (convert_text_c),
                                optimize.convert_text);
  missing_fonts = get_missing_fonts (gimp_image_list_layers (multi_page.images[0]));
  if (missing_fonts != NULL)
    {
      GtkWidget *label;
      GtkWidget *image;
      GtkWidget *warn_box;
      GList     *iter;
      gchar     *font_list = NULL;

      frame = gtk_frame_new (NULL);
      gtk_box_pack_end (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

      /* Checkbox is frame label. */
      gtk_frame_set_label_widget (GTK_FRAME (frame), convert_text_c);

      /* Show warning inside the frame. */
      warn_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
      gtk_container_add (GTK_CONTAINER (frame), warn_box);

      image = gtk_image_new_from_icon_name (GIMP_ICON_WILBER_EEK,
                                            GTK_ICON_SIZE_BUTTON);
      gtk_box_pack_start (GTK_BOX (warn_box), image, FALSE, FALSE, 0);
      gtk_widget_show (image);

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
      label = gtk_label_new (text);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      g_free (text);
      gtk_box_pack_start (GTK_BOX (warn_box), label, TRUE, TRUE, 0);

      g_list_free_full (missing_fonts, g_free);
    }
  else
    {
      gtk_box_pack_end (GTK_BOX (vbox), convert_text_c, TRUE, TRUE, 0);
    }

  ignore_hidden_c = gtk_check_button_new_with_mnemonic (_("_Omit hidden layers and layers with zero opacity"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ignore_hidden_c),
                                optimize.ignore_hidden);
  gtk_box_pack_end (GTK_BOX (vbox), ignore_hidden_c, TRUE, TRUE, 0);

  vectorize_c = gtk_check_button_new_with_mnemonic (_("Convert _bitmaps to vector graphics where possible"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (vectorize_c),
                                optimize.vectorize);
  gtk_box_pack_end (GTK_BOX (vbox), vectorize_c, TRUE, TRUE, 0);

  apply_c = gtk_check_button_new_with_mnemonic (_("_Apply layer masks before saving"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (apply_c),
                                optimize.apply_masks);
  gtk_box_pack_end (GTK_BOX (vbox), apply_c, TRUE, TRUE, 0);
  gimp_help_set_help_data (apply_c, _("Keeping the masks will not change the output"), NULL);

  /* Frame for multi-page from layers. */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_end (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

  text = g_strdup_printf (_("_Layers as pages (%s)"),
                          optimize.reverse_order ?
                          _("top layers first") : _("bottom layers first"));
  layers_as_pages_c = gtk_check_button_new_with_mnemonic (text);
  g_free (text);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (layers_as_pages_c),
                                optimize.layers_as_pages);
  gtk_frame_set_label_widget (GTK_FRAME (frame), layers_as_pages_c);
  g_free (gimp_image_get_layers (multi_page.images[0], &n_layers));

  reverse_order_c = gtk_check_button_new_with_mnemonic (_("_Reverse the pages order"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (reverse_order_c),
                                optimize.reverse_order);
  gtk_container_add (GTK_CONTAINER (frame), reverse_order_c);

  if (n_layers <= 1)
    {
      gtk_widget_set_sensitive (layers_as_pages_c, FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (layers_as_pages_c),
                                    FALSE);
    }

  g_object_bind_property (layers_as_pages_c, "active",
                          reverse_order_c,  "sensitive",
                          G_BINDING_SYNC_CREATE);
  g_signal_connect (G_OBJECT (reverse_order_c), "toggled",
                    G_CALLBACK (reverse_order_toggled),
                    layers_as_pages_c);

  gtk_widget_show_all (window);

  run = gtk_dialog_run (GTK_DIALOG (window)) == GTK_RESPONSE_OK;

  optimize.convert_text =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (convert_text_c));
  optimize.ignore_hidden =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ignore_hidden_c));
  optimize.vectorize =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (vectorize_c));
  optimize.apply_masks =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (apply_c));
  optimize.layers_as_pages =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (layers_as_pages_c));
  optimize.reverse_order =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (reverse_order_c));

  gtk_widget_destroy (window);

  return run;
}

/* The main GUI function for saving multi-paged PDFs */

static gboolean
gui_multi (void)
{
  GtkWidget   *window;
  GtkWidget   *vbox;
  GtkWidget   *file_label;
  GtkWidget   *file_entry;
  GtkWidget   *file_browse;
  GtkWidget   *file_hbox;
  GtkWidget   *vectorize_c;
  GtkWidget   *ignore_hidden_c;
  GtkWidget   *apply_c;
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

  window = gimp_export_dialog_new ("PDF", PLUG_IN_ROLE, SAVE_MULTI_PROC);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (window)),
                      vbox, TRUE, TRUE, 0);

  gtk_container_set_border_width (GTK_CONTAINER (window), 12);

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
  gtk_box_pack_start (GTK_BOX (file_hbox), file_entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (file_hbox), file_browse, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), file_hbox, TRUE, TRUE, 0);

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

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_container_add (GTK_CONTAINER (scroll), page_view);

  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);

  h_but_box = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (h_but_box), GTK_BUTTONBOX_START);

  del = gtk_button_new_with_label (_("Remove the selected pages"));
  gtk_box_pack_start (GTK_BOX (h_but_box), del, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), h_but_box, FALSE, FALSE, 0);

  h_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);

  img_combo = gimp_image_combo_box_new (NULL, NULL, NULL);
  gtk_box_pack_start (GTK_BOX (h_box), img_combo, FALSE, FALSE, 0);

  add_image = gtk_button_new_with_label (_("Add this image"));
  gtk_box_pack_start (GTK_BOX (h_box), add_image, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), h_box, FALSE, FALSE, 0);

  ignore_hidden_c = gtk_check_button_new_with_mnemonic (_("_Omit hidden layers and layers with zero opacity"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ignore_hidden_c),
                                optimize.ignore_hidden);
  gtk_box_pack_end (GTK_BOX (vbox), ignore_hidden_c, FALSE, FALSE, 0);

  vectorize_c = gtk_check_button_new_with_mnemonic (_("Convert _bitmaps to vector graphics where possible"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (vectorize_c),
                                optimize.vectorize);
  gtk_box_pack_end (GTK_BOX (vbox), vectorize_c, FALSE, FALSE, 0);

  apply_c = gtk_check_button_new_with_mnemonic (_("_Apply layer masks before saving"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (apply_c),
                                optimize.apply_masks);
  gtk_box_pack_end (GTK_BOX (vbox), apply_c, FALSE, FALSE, 0);
  gimp_help_set_help_data (apply_c, _("Keeping the masks will not change the output"), NULL);

  gtk_widget_show_all (window);

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

  run = gtk_dialog_run (GTK_DIALOG (window)) == GTK_RESPONSE_OK;

  run &= get_image_list ();

  temp = gtk_entry_get_text (GTK_ENTRY (file_entry));
  g_stpcpy (file_name, temp);

  optimize.ignore_hidden =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ignore_hidden_c));
  optimize.vectorize =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (vectorize_c));
  optimize.apply_masks =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (apply_c));

  gtk_widget_destroy (window);

  return run;
}

static void
reverse_order_toggled (GtkToggleButton *reverse_order,
                       GtkButton       *layers_as_pages)
{
  gchar *text;

  text = g_strdup_printf (_("Layers as pages (%s)"),
                          gtk_toggle_button_get_active (reverse_order) ?
                          _("top layers first") : _("bottom layers first"));
  gtk_button_set_label (layers_as_pages, text);
  g_free (text);
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
      gtk_entry_set_text (GTK_ENTRY (file_entry), g_file_get_path (file));
    }

  file_name = g_file_get_path (file);
  gtk_widget_hide (file_choose);
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
                               GIMP_PLUGIN_PDF_SAVE_ERROR,
                               GIMP_PLUGIN_PDF_SAVE_ERROR_FAILED,
                               _("Cannot handle the size (either width or height) of the image."));
          break;
        default:
          g_set_error (error,
                       GIMP_PLUGIN_PDF_SAVE_ERROR,
                       GIMP_PLUGIN_PDF_SAVE_ERROR_FAILED,
                       "Cairo error: %s",
                       cairo_status_to_string (status));
          break;
        }

      return NULL;
    }

  dest_buffer = gimp_cairo_surface_create_buffer (surface);
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
static GimpRGB
get_layer_color (GimpLayer *layer,
                 gboolean  *single)
{
  GimpRGB col;
  gdouble red, green, blue, alpha;
  gdouble dev, devSum;
  gdouble median, pixels, count, precentile;

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
      col. r = col.g = col.b = col.a = 0;
      return col;
    }

  if (gimp_drawable_bpp (GIMP_DRAWABLE (layer)) >= 3)
    {
      /* Are we in RGB mode? */

      gimp_drawable_histogram (GIMP_DRAWABLE (layer),
                               GIMP_HISTOGRAM_RED, 0.0, 1.0,
                               &red, &dev, &median, &pixels, &count, &precentile);
      devSum += dev;

      gimp_drawable_histogram (GIMP_DRAWABLE (layer),
                               GIMP_HISTOGRAM_GREEN, 0.0, 1.0,
                               &green, &dev, &median, &pixels, &count, &precentile);
      devSum += dev;

      gimp_drawable_histogram (GIMP_DRAWABLE (layer),
                               GIMP_HISTOGRAM_BLUE, 0.0, 1.0,
                               &blue, &dev, &median, &pixels, &count, &precentile);
      devSum += dev;
    }
  else
    {
      /* We are in Grayscale mode (or Indexed) */

      gimp_drawable_histogram (GIMP_DRAWABLE (layer),
                               GIMP_HISTOGRAM_VALUE, 0.0, 1.0,
                               &red, &dev, &median, &pixels, &count, &precentile);
      devSum += dev;
      green = red;
      blue = red;
    }

  if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    gimp_drawable_histogram (GIMP_DRAWABLE (layer),
                             GIMP_HISTOGRAM_ALPHA, 0.0, 1.0,
                             &alpha, &dev, &median, &pixels, &count, &precentile);
  else
    alpha = 255;

  devSum += dev;
  *single = devSum == 0;

  col.r = red   / 255;
  col.g = green / 255;
  col.b = blue  / 255;
  col.a = alpha / 255;

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
  gchar                *text   = gimp_text_layer_get_text (layer);
  gchar                *markup = gimp_text_layer_get_markup (layer);
  gchar                *font_family;
  gchar                *language;
  cairo_font_options_t *options;
  gint                  x;
  gint                  y;
  GimpRGB               color;
  GimpUnit              unit;
  gdouble               size;
  GimpTextHintStyle     hinting;
  GimpTextJustification j;
  gboolean              justify;
  PangoAlignment        align;
  GimpTextDirection     dir;
  PangoLayout          *layout;
  PangoContext         *context;
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
  gimp_drawable_offsets (GIMP_DRAWABLE (layer), &x, &y);
  cairo_translate (cr, x, y);

  /* Color */
  /* When dealing with a gray/indexed image, the viewed color of the text layer
   * can be different than the one kept in the memory */
  if (type == GIMP_RGBA_IMAGE)
    gimp_text_layer_get_color (layer, &color);
  else
    gimp_image_pick_color (gimp_item_get_image (GIMP_ITEM (layer)), 1,
                           (const GimpItem**) &layer, x, y, FALSE, FALSE, 0,
                           &color);

  cairo_set_source_rgba (cr, color.r, color.g, color.b, opacity);

  /* Hinting */
  hinting = gimp_text_layer_get_hint_style (layer);
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
  if (gimp_text_layer_get_antialias (layer))
    cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_DEFAULT);
  else
    cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_NONE);

  /* We are done with cairo's settings. It's time to create the
   * context
   */
  fontmap = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);

  pango_cairo_font_map_set_resolution (PANGO_CAIRO_FONT_MAP (fontmap), y_res);

  context = pango_font_map_create_context (fontmap);
  g_object_unref (fontmap);

  pango_cairo_context_set_font_options (context, options);

  /* Language */
  language = gimp_text_layer_get_language (layer);
  if (language)
    pango_context_set_language (context,
                                pango_language_from_string(language));

  /* Text Direction */
  dir = gimp_text_layer_get_base_direction (layer);

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

  /* Font */
  font_family = gimp_text_layer_get_font (layer);

  /* We need to find a way to convert GIMP's returned font name to a
   * normal Pango name... Hopefully GIMP 2.8 with Pango will fix it.
   */
  font_description = pango_font_description_from_string (font_family);

  /* Font Size */
  size = gimp_text_layer_get_font_size (layer, &unit);
  size = gimp_units_to_pixels (size, unit, y_res);
  pango_font_description_set_absolute_size (font_description, size * PANGO_SCALE);

  pango_layout_set_font_description (layout, font_description);

  /* Width */
  if (! PANGO_GRAVITY_IS_VERTICAL (pango_context_get_base_gravity (context)))
    pango_layout_set_width (layout,
                            gimp_drawable_width (GIMP_DRAWABLE (layer)) *
                            PANGO_SCALE);
  else
    pango_layout_set_width (layout,
                            gimp_drawable_height (GIMP_DRAWABLE (layer)) *
                            PANGO_SCALE);

  /* Justification, and Alignment */
  justify = FALSE;
  j = gimp_text_layer_get_justification (layer);
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
  indent = gimp_text_layer_get_indent (layer);
  pango_layout_set_indent (layout, (int)(PANGO_SCALE * indent));

  /* Line Spacing */
  line_spacing = gimp_text_layer_get_line_spacing (layer);
  pango_layout_set_spacing (layout, (int)(PANGO_SCALE * line_spacing));

  /* Letter Spacing */
  letter_spacing = gimp_text_layer_get_letter_spacing (layer);
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
      cairo_translate (cr, gimp_drawable_width (GIMP_DRAWABLE (layer)), 0);
      cairo_rotate (cr, G_PI_2);
    }

  if (dir == GIMP_TEXT_DIRECTION_TTB_LTR ||
      dir == GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT)
    {
      cairo_translate (cr, 0, gimp_drawable_height (GIMP_DRAWABLE (layer)));
      cairo_rotate (cr, -G_PI_2);
    }

  pango_cairo_show_layout (cr, layout);

  g_free (text);
  g_free (font_family);
  g_free (language);

  g_object_unref (layout);
  pango_font_description_free (font_description);
  g_object_unref (context);
  pango_attr_list_unref (attr_list);

  cairo_font_options_destroy (options);

  cairo_restore (cr);
}

static gboolean
draw_layer (GimpLayer   **layers,
            gint          n_layers,
            gint          j,
            cairo_t      *cr,
            gdouble       x_res,
            gdouble       y_res,
            const gchar  *name,
            GError      **error)
{
  GimpLayer *layer;
  gdouble    opacity;

  if (optimize.reverse_order && optimize.layers_as_pages)
    layer = layers [j];
  else
    layer = layers [n_layers - j - 1];

  opacity = gimp_layer_get_opacity (layer) / 100.0;
  if ((! gimp_item_get_visible (GIMP_ITEM (layer)) || opacity == 0.0) &&
      optimize.ignore_hidden)
    return TRUE;

  if (gimp_item_is_group (GIMP_ITEM (layer)))
    {
      GimpItem **children;
      gint       children_num;
      gint       i;

      children = gimp_item_get_children (GIMP_ITEM (layer), &children_num);
      for (i = 0; i < children_num; i++)
        {
          if (! draw_layer ((GimpLayer **) children, children_num, i,
                            cr, x_res, y_res, name, error))
            {
              g_free (children);
              return FALSE;
            }
        }
      g_free (children);
    }
  else
    {
      cairo_surface_t *mask_image = NULL;
      GimpLayerMask   *mask       = NULL;
      gint             x, y;

      mask = gimp_layer_get_mask (layer);
      if (mask)
        {
          mask_image = get_cairo_surface (GIMP_DRAWABLE (mask), TRUE,
                                          error);

          if (*error)
            return FALSE;
        }

      gimp_drawable_offsets (GIMP_DRAWABLE (layer), &x, &y);

      if (! gimp_item_is_text_layer (GIMP_ITEM (layer)) || optimize.convert_text)
        {
          /* For raster layers */

          GimpRGB  layer_color;
          gboolean single_color = FALSE;

          layer_color = get_layer_color (layer, &single_color);

          cairo_rectangle (cr, x, y,
                           gimp_drawable_width  (GIMP_DRAWABLE (layer)),
                           gimp_drawable_height (GIMP_DRAWABLE (layer)));

          if (optimize.vectorize && single_color)
            {
              cairo_set_source_rgba (cr,
                                     layer_color.r,
                                     layer_color.g,
                                     layer_color.b,
                                     layer_color.a * opacity);
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
        }
      else
        {
          /* For text layers */
          drawText (layer, opacity, cr, x_res, y_res);
        }

      /* draw new page if "layers as pages" option is checked */
      if (optimize.layers_as_pages)
        cairo_show_page (cr);

      /* We are done with the layer - time to free some resources */
      if (mask)
        cairo_surface_destroy (mask_image);
    }

  return TRUE;
}
