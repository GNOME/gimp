/* GIMP - The GNU Image Manipulation Program
 *
 * file-pdf-load.c - PDF file loader
 *
 * Copyright (C) 2005 Nathan Summers
 *
 * Some code in render_page_to_surface() borrowed from
 * poppler.git/glib/poppler-page.cc.
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

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#undef GTK_DISABLE_SINGLE_INCLUDES
#include <poppler.h>
#define GTK_DISABLE_SINGLE_INCLUDES

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC       "file-pdf-load"
#define LOAD_THUMB_PROC "file-pdf-load-thumb"
#define PLUG_IN_BINARY  "file-pdf-load"
#define PLUG_IN_ROLE    "gimp-file-pdf-load"

#define THUMBNAIL_SIZE  128

#define GIMP_PLUGIN_PDF_LOAD_ERROR gimp_plugin_pdf_load_error_quark ()
static GQuark
gimp_plugin_pdf_load_error_quark (void)
{
  return g_quark_from_static_string ("gimp-plugin-pdf-load-error-quark");
}

typedef struct
{
  gint  n_pages;
  gint *pages;
} PdfSelectedPages;


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

GType                   pdf_get_type         (void) G_GNUC_CONST;

static GList          * pdf_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * pdf_create_procedure (GimpPlugIn            *plug_in,
                                              const gchar           *name);

static gboolean         pdf_extract          (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GFile                *file,
                                              GimpMetadata         *metadata,
                                              GimpProcedureConfig  *config,
                                              GimpVectorLoadData   *extracted_dimensions,
                                              gpointer             *data_for_run,
                                              GDestroyNotify       *data_for_run_destroy,
                                              gpointer              extract_data,
                                              GError              **error);
static GimpValueArray * pdf_load             (GimpProcedure         *procedure,
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
static GimpValueArray * pdf_load_thumb       (GimpProcedure         *procedure,
                                              GFile                 *file,
                                              gint                   size,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);

static GimpImage       * load_image          (PopplerDocument       *doc,
                                              GFile                 *file,
                                              GimpVectorLoadData     extracted_data,
                                              GimpRunMode            run_mode,
                                              GimpPageSelectorTarget target,
                                              gint                   width,
                                              gint                   height,
                                              gdouble                resolution,
                                              gboolean               antialias,
                                              gboolean               white_background,
                                              gboolean               reverse_order,
                                              PdfSelectedPages      *pages);

static GimpPDBStatusType load_dialog         (PopplerDocument       *doc,
                                              PdfSelectedPages      *pages,
                                              GimpVectorLoadData     extracted_data,
                                              GimpProcedure         *procedure,
                                              GimpProcedureConfig   *config);

static PopplerDocument * open_document       (GFile                 *file,
                                              const gchar           *PDF_password,
                                              GimpRunMode            run_mode,
                                              gdouble               *width,
                                              gdouble               *height,
                                              GError               **error);

static cairo_surface_t * get_thumb_surface   (PopplerDocument       *doc,
                                              gint                   page,
                                              gint                   preferred_size,
                                              gboolean               white_background);

static GdkPixbuf       * get_thumb_pixbuf    (PopplerDocument       *doc,
                                              gint                   page,
                                              gint                   preferred_size,
                                              gboolean               white_background);

static GimpLayer       * layer_from_surface  (GimpImage             *image,
                                              const gchar           *layer_name,
                                              gint                   position,
                                              cairo_surface_t       *surface,
                                              gdouble                progress_start,
                                              gdouble                progress_scale);


G_DEFINE_TYPE (Pdf, pdf, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PDF_TYPE)
DEFINE_STD_SET_I18N


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

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static GimpProcedure *
pdf_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_vector_load_procedure_new (plug_in, name,
                                                  GIMP_PDB_PROC_TYPE_PLUGIN,
                                                  pdf_extract, NULL, NULL,
                                                  pdf_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("Portable Document Format"));

      gimp_procedure_set_documentation (procedure,
                                        "Load file in PDF format",
                                        "Loads files in Adobe's Portable "
                                        "Document Format. PDF is designed to "
                                        "be easily processed by a variety "
                                        "of different platforms, and is a "
                                        "distant cousin of PostScript.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Nathan Summers, Lionel N.",
                                      "Nathan Summers, Lionel N.",
                                      "2005, 2017");

      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("PDF"));
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "application/pdf");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "pdf");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0, string,%PDF-");

      gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);

      gimp_procedure_add_string_argument (procedure, "password",
                                          _("PDF password"),
                                          _("The password to decrypt the encrypted PDF file"),
                                          NULL,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "reverse-order",
                                           _("Load in re_verse order"),
                                           _("Load PDF pages in reverse order"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      /* FIXME: this should be a gimp_procedure_add_enum_argument () of type
       * GIMP_TYPE_PAGE_SELECTOR_TARGET but it won't work right now (see FIXME
       * comment in libgimp/gimpgpparams-body.c:116).

      gimp_procedure_add_enum_argument (procedure, "target",
                                        _("Open pages as"),
                                        _("Number of pages to load (0 for all)"),
                                        GIMP_TYPE_PAGE_SELECTOR_TARGET,
                                        GIMP_PAGE_SELECTOR_TARGET_LAYERS,
                                        G_PARAM_READWRITE);

       */
      gimp_procedure_add_int_aux_argument (procedure, "target",
                                           _("Open pages as"),
                                           _("Number of pages to load (0 for all)"),
                                           GIMP_PAGE_SELECTOR_TARGET_LAYERS, GIMP_PAGE_SELECTOR_TARGET_IMAGES,
                                           GIMP_PAGE_SELECTOR_TARGET_LAYERS,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "n-pages",
                                       _("N pages"),
                                       _("Number of pages to load (0 for all)"),
                                       0, G_MAXINT, 0,
                                       G_PARAM_READWRITE);

      /* FIXME: shouldn't the whole selector be considered as one argument
       * containing properties "target", "n-pages" and "pages" as a single
       * object?
       * Or actually should we store pages at all? While it makes sense to store
       * some settings generally, not sure that the list of page makes sense
       * from one PDF document loaded to another (different) one.
       */
      gimp_procedure_add_int32_array_argument (procedure, "pages",
                                               _("Pages"),
                                               _("The pages to load in the expected order"),
                                               G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "antialias",
                                           _("Use _Anti-aliasing"),
                                           _("Render texts with anti-aliasing"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "white-background",
                                           _("_Fill transparent areas with white"),
                                           _("Render all pages as opaque by filling the background in white"),
                                           TRUE,
                                           G_PARAM_READWRITE);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = gimp_thumbnail_procedure_new (plug_in, name,
                                                GIMP_PDB_PROC_TYPE_PLUGIN,
                                                pdf_load_thumb, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Loads a preview from a PDF file.",
                                        "Loads a small preview of the first "
                                        "page of the PDF format file. Uses "
                                        "the embedded thumbnail if present.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Nathan Summers",
                                      "Nathan Summers",
                                      "2005");
    }

  return procedure;
}

static gboolean
pdf_extract (GimpProcedure        *procedure,
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
  PopplerDocument     *doc      = NULL;
  gchar               *password = NULL;
  gdouble              width    = 0.0;
  gdouble              height   = 0.0;

  if (run_mode == GIMP_RUN_INTERACTIVE)
    gimp_ui_init (PLUG_IN_BINARY);

  g_object_get (config, "password", &password, NULL);
  doc = open_document (file, password, run_mode,
                       &width, &height, error);
  g_free (password);
  if (doc == NULL)
    return FALSE;

  extracted_dimensions->width         = width;
  extracted_dimensions->height        = height;
  extracted_dimensions->width_unit    = gimp_unit_point ();
  extracted_dimensions->height_unit   = gimp_unit_point ();
  extracted_dimensions->exact_width   = TRUE;
  extracted_dimensions->exact_height  = TRUE;
  extracted_dimensions->correct_ratio = TRUE;

  *data_for_run = doc;
  *data_for_run_destroy = g_object_unref;

  return TRUE;
}

static GimpValueArray *
pdf_load (GimpProcedure         *procedure,
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
  GimpValueArray      *return_vals;
  GimpPDBStatusType    status   = GIMP_PDB_SUCCESS;
  GimpImage           *image    = NULL;
  PopplerDocument     *doc      = POPPLER_DOCUMENT (data_from_extract);
  PdfSelectedPages     pages    = { 0, NULL };
  GError              *error    = NULL;

  gegl_init (NULL, NULL);

  if (doc == NULL)
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }
  else if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      status = load_dialog (doc, &pages, extracted_data, procedure, config);
    }
  else if (run_mode == GIMP_RUN_NONINTERACTIVE)
    {
      PopplerPage *test_page = poppler_document_get_page (doc, 0);

      if (test_page)
        {
          gint i;
          gint doc_n_pages;

          g_object_get (config, "n-pages", &pages.n_pages, NULL);
          doc_n_pages = poppler_document_get_n_pages (doc);
          /* The number of imported pages may be bigger than
           * the number of pages from the original document.
           * Indeed it is possible to duplicate some pages
           * by setting the same number several times in the
           * "pages" argument.
           * Not ceiling this value is *not* an error.
           */
          if (pages.n_pages <= 0)
            {
              pages.n_pages = doc_n_pages;
              pages.pages = g_new (gint, pages.n_pages);
              for (i = 0; i < pages.n_pages; i++)
                pages.pages[i] = i;
            }
          else
            {
              const gint32 *p;

              g_object_get (config, "pages", &p, NULL);

              pages.pages = g_new (gint, pages.n_pages);

              for (i = 0; i < pages.n_pages; i++)
                {
                  if (p[i] >= doc_n_pages)
                    {
                      status = GIMP_PDB_EXECUTION_ERROR;
                      g_set_error (&error, GIMP_PLUGIN_PDF_LOAD_ERROR, 0,
                                   /* TRANSLATORS: first argument is file name,
                                    * second is out-of-range page number,
                                    * third is number of pages.
                                    * Specify order as in English if needed.
                                    */
                                   ngettext ("PDF document '%1$s' has %3$d page. Page %2$d is out of range.",
                                             "PDF document '%1$s' has %3$d pages. Page %2$d is out of range.",
                                             doc_n_pages),
                                   gimp_file_get_utf8_name (file),
                                   p[i],
                                   doc_n_pages);
                      break;
                    }
                  else
                    {
                      pages.pages[i] = p[i];
                    }
                }
            }

          g_object_unref (test_page);
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpPageSelectorTarget target;
      gboolean               reverse_order;
      gdouble                resolution;
      gboolean               antialias;
      gboolean               white_background;

      g_object_get (config,
                    "target",           &target,
                    "reverse-order",    &reverse_order,
                    "width",            &width,
                    "height",           &height,
                    "pixel-density",    &resolution,
                    "antialias",        &antialias,
                    "white-background", &white_background,
                    NULL);
      image = load_image (doc,
                          file, extracted_data,
                          run_mode,
                          target,
                          width, height,
                          resolution,
                          antialias,
                          white_background,
                          reverse_order,
                          &pages);

      if (image == NULL)
        status = GIMP_PDB_EXECUTION_ERROR;
    }

  g_free (pages.pages);

  return_vals = gimp_procedure_new_return_values (procedure, status, error);
  if (status == GIMP_PDB_SUCCESS)
    GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
pdf_load_thumb (GimpProcedure       *procedure,
                GFile               *file,
                gint                 size,
                GimpProcedureConfig *config,
                gpointer             run_data)
{
  GimpValueArray  *return_vals;
  gdouble          width  = 0;
  gdouble          height = 0;
  gdouble          scale;
  GimpImage       *image     = NULL;
  gint             num_pages = 0;
  PopplerDocument *doc       = NULL;
  cairo_surface_t *surface   = NULL;
  GError          *error     = NULL;

  gegl_init (NULL, NULL);

  doc = open_document (file, NULL, GIMP_RUN_NONINTERACTIVE,
                       NULL, NULL, &error);

  if (doc)
    {
      PopplerPage *page = poppler_document_get_page (doc, 0);

      if (page)
        {
          poppler_page_get_size (page, &width, &height);

          g_object_unref (page);
        }

      num_pages = poppler_document_get_n_pages (doc);

      surface = get_thumb_surface (doc, 0, size, TRUE);

      g_object_unref (doc);
    }

  if (surface)
    {
      image = gimp_image_new (cairo_image_surface_get_width (surface),
                              cairo_image_surface_get_height (surface),
                              GIMP_RGB);

      gimp_image_undo_disable (image);

      layer_from_surface (image, "thumbnail", 0, surface, 0.0, 1.0);
      cairo_surface_destroy (surface);

      gimp_image_undo_enable (image);
      gimp_image_clean_all (image);
    }

  /* Thumbnail resolution: 100.0. */
  scale = 100.0 / gimp_unit_get_factor (gimp_unit_point ());

  width  *= scale;
  height *= scale;

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
  GIMP_VALUES_SET_ENUM  (return_vals, 4, GIMP_RGB_IMAGE);
  GIMP_VALUES_SET_INT   (return_vals, 5, num_pages);

  return return_vals;
}

static PopplerDocument *
open_document (GFile        *file,
               const gchar  *PDF_password,
               GimpRunMode   run_mode,
               gdouble      *width,
               gdouble      *height,
               GError      **load_error)
{
  PopplerDocument *doc;
  GError          *error = NULL;

  doc = poppler_document_new_from_gfile (file, PDF_password, NULL, &error);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      GtkWidget *label;

      label = gtk_label_new (_("PDF is password protected, please input the password:"));
      while (error                          &&
             error->domain == POPPLER_ERROR &&
             error->code == POPPLER_ERROR_ENCRYPTED)
        {
          GtkWidget *vbox;
          GtkWidget *dialog;
          GtkWidget *entry;
          gint       run;

          dialog = gimp_dialog_new (_("Encrypted PDF"), PLUG_IN_ROLE,
                                    NULL, 0,
                                    NULL, NULL,
                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("_OK"), GTK_RESPONSE_OK,
                                    NULL);
          gimp_window_set_transient (GTK_WINDOW (dialog));
          vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
          gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
          gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                              vbox, TRUE, TRUE, 0);
          entry = gtk_entry_new ();
          gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
          gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
          gtk_container_add (GTK_CONTAINER (vbox), label);
          gtk_container_add (GTK_CONTAINER (vbox), entry);

          gtk_widget_show_all (dialog);

          run = gimp_dialog_run (GIMP_DIALOG (dialog));
          if (run == GTK_RESPONSE_OK)
            {
              g_clear_error (&error);
              doc = poppler_document_new_from_gfile (file,
                                                     gtk_entry_get_text (GTK_ENTRY (entry)),
                                                     NULL, &error);
            }
          label = gtk_label_new (_("Wrong password! Please input the right one:"));
          gtk_widget_destroy (dialog);
          if (run == GTK_RESPONSE_CANCEL || run == GTK_RESPONSE_DELETE_EVENT)
            {
              break;
            }
        }

      gtk_widget_destroy (label);
    }

  /* We can't g_mapped_file_unref(mapped_file) as apparently doc has
   * references to data in there. No big deal, this is just a
   * short-lived plug-in.
   */
  if (! doc)
    {
      g_set_error (load_error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not load '%s': %s"),
                   gimp_file_get_utf8_name (file),
                   error->message);
      g_error_free (error);
      return NULL;
    }

  if (width && height)
    {
      gint n_pages;

      n_pages = poppler_document_get_n_pages (doc);
      for (gint i = 0; i < n_pages; i++)
        {
          PopplerPage *page;

          page = poppler_document_get_page (doc, i);
          poppler_page_get_size (page, width, height);
          g_object_unref (page);

          /* Only check the first page for the whole document size. */
          break;
        }
    }

  return doc;
}

static GimpLayer *
layer_from_surface (GimpImage       *image,
                    const gchar     *layer_name,
                    gint             position,
                    cairo_surface_t *surface,
                    gdouble          progress_start,
                    gdouble          progress_scale)
{
  GimpLayer *layer;

  layer = gimp_layer_new_from_surface (image, layer_name, surface,
                                       progress_start,
                                       progress_start + progress_scale);
  gimp_image_insert_layer (image, layer, NULL, position);

  return layer;
}

static cairo_surface_t *
render_page_to_surface (PopplerPage *page,
                        int          width,
                        int          height,
                        double       scale,
                        gboolean     antialias,
                        gboolean     white_background)
{
  cairo_surface_t *surface;
  cairo_t *cr;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create (surface);

  cairo_save (cr);
  cairo_translate (cr, 0.0, 0.0);

  if (scale != 1.0)
    cairo_scale (cr, scale, scale);

  if (! antialias)
    {
      cairo_font_options_t *options = cairo_font_options_create ();

      cairo_get_font_options (cr, options);
      cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_NONE);
      cairo_set_font_options (cr, options);
      cairo_font_options_destroy (options);

      cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    }

  poppler_page_render (page, cr);
  cairo_restore (cr);

  if (white_background)
    {
      cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OVER);
      cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
      cairo_paint (cr);
    }

  cairo_destroy (cr);

  return surface;
}

#if 0

/* This is currently unused, but we'll have it here in case the military
   wants it. */

static GdkPixbuf *
render_page_to_pixbuf (PopplerPage *page,
                       int          width,
                       int          height,
                       double       scale)
{
  GdkPixbuf *pixbuf;
  cairo_surface_t *surface;

  surface = render_page_to_surface (page, width, height, scale);
  pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0,
                                        cairo_image_surface_get_width (surface),
                                        cairo_image_surface_get_height (surface));
  cairo_surface_destroy (surface);

  return pixbuf;
}

#endif

static GimpImage *
load_image (PopplerDocument        *doc,
            GFile                  *file,
            GimpVectorLoadData      extracted_data,
            GimpRunMode             run_mode,
            GimpPageSelectorTarget  target,
            gint                    doc_width,
            gint                    doc_height,
            gdouble                 resolution,
            gboolean                antialias,
            gboolean                white_background,
            gboolean                reverse_order,
            PdfSelectedPages       *pages)
{
  GimpImage  *image = NULL;
  GimpImage **images   = NULL;
  gint        i;
  gdouble     scale;
  gdouble     doc_progress = 0;
  gint        base_index = 0;
  gint        sign = 1;

  if (reverse_order && pages->n_pages > 0)
    {
      base_index = pages->n_pages - 1;
      sign = -1;
    }

  if (target == GIMP_PAGE_SELECTOR_TARGET_IMAGES)
    images = g_new0 (GimpImage *, pages->n_pages);

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  scale = (gdouble) doc_width / extracted_data.width;

  /* read the file */

  for (i = 0; i < pages->n_pages; i++)
    {
      PopplerPage *page;
      gchar       *page_label;
      gdouble      page_width;
      gdouble      page_height;

      cairo_surface_t *surface;
      gint             width;
      gint             height;
      gint             page_index;

      page_index = base_index + sign * i;

      page = poppler_document_get_page (doc, pages->pages[page_index]);

      poppler_page_get_size (page, &page_width, &page_height);
      width  = (gint) ceil (page_width  * scale);
      height = (gint) ceil (page_height * scale);

      g_object_get (G_OBJECT (page), "label", &page_label, NULL);

      if (! image)
        {
          image = gimp_image_new (doc_width, doc_height, GIMP_RGB);
          gimp_image_undo_disable (image);

          gimp_image_set_resolution (image, resolution, resolution);
        }

      surface = render_page_to_surface (page, width, height, scale,
                                        antialias, white_background);

      layer_from_surface (image, page_label, 0, surface,
                          doc_progress, 1.0 / pages->n_pages);

      g_free (page_label);
      cairo_surface_destroy (surface);

      doc_progress = (double) (i + 1) / pages->n_pages;
      gimp_progress_update (doc_progress);

      if (target == GIMP_PAGE_SELECTOR_TARGET_IMAGES)
        {
          images[i] = image;

          gimp_image_undo_enable (image);
          gimp_image_clean_all (image);

          image = 0;
        }
    }
  gimp_progress_update (1.0);

  if (image)
    {
      gimp_image_undo_enable (image);
      gimp_image_clean_all (image);
    }

  if (target == GIMP_PAGE_SELECTOR_TARGET_IMAGES)
    {
      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        {
          /* Display images in reverse order.  The last will be
           * displayed by GIMP itself
           */
          for (i = pages->n_pages - 1; i > 0; i--)
            gimp_display_new (images[i]);
        }

      image = images[0];

      g_free (images);
    }

  return image;
}

static cairo_surface_t *
get_thumb_surface (PopplerDocument *doc,
                   gint             page_num,
                   gint             preferred_size,
                   gboolean         white_background)
{
  PopplerPage     *page;
  cairo_surface_t *surface;

  page = poppler_document_get_page (doc, page_num);

  if (! page)
    return NULL;

  surface = poppler_page_get_thumbnail (page);

  if (! surface)
    {
      gdouble width;
      gdouble height;
      gdouble scale;

      poppler_page_get_size (page, &width, &height);

      scale = (gdouble) preferred_size / MAX (width, height);

      width  *= scale;
      height *= scale;

      surface = render_page_to_surface (page, width, height, scale, TRUE, white_background);
    }

  g_object_unref (page);

  return surface;
}

static GdkPixbuf *
get_thumb_pixbuf (PopplerDocument *doc,
                  gint             page_num,
                  gint             preferred_size,
                  gboolean         white_background)
{
  cairo_surface_t *surface;
  GdkPixbuf       *pixbuf;

  surface = get_thumb_surface (doc, page_num, preferred_size, white_background);
  pixbuf  = gdk_pixbuf_get_from_surface (surface, 0, 0,
                                         cairo_image_surface_get_width (surface),
                                         cairo_image_surface_get_height (surface));
  cairo_surface_destroy (surface);

  return pixbuf;
}

typedef struct
{
  PopplerDocument  *document;
  GimpPageSelector *selector;
  gboolean          white_background;

  GMutex            mutex;
  GCond             render_thumb;
  gboolean          stop_thumbnailing;
  gboolean          render_thumbnails;
} ThreadData;

typedef struct
{
  GimpPageSelector *selector;
  gint              page_no;
  GdkPixbuf        *pixbuf;
} IdleData;

static gboolean
idle_set_thumbnail (gpointer data)
{
  IdleData *idle_data = data;

  gimp_page_selector_set_page_thumbnail (idle_data->selector,
                                         idle_data->page_no,
                                         idle_data->pixbuf);
  g_object_unref (idle_data->pixbuf);
  g_free (idle_data);

  return FALSE;
}

static gpointer
thumbnail_thread (gpointer data)
{
  ThreadData *thread_data = data;
  gboolean    first_loop  = TRUE;
  gint        n_pages;
  gint        i;

  n_pages = poppler_document_get_n_pages (thread_data->document);

  while (TRUE)
    {
      gboolean white_background;
      gboolean stop_thumbnailing;
      gboolean render_thumbnails;

      g_mutex_lock (&thread_data->mutex);
      if (first_loop)
        first_loop = FALSE;
      else
        g_cond_wait (&thread_data->render_thumb, &thread_data->mutex);

      stop_thumbnailing = thread_data->stop_thumbnailing;
      g_mutex_unlock (&thread_data->mutex);

      if (stop_thumbnailing)
        break;

      g_mutex_lock (&thread_data->mutex);
      render_thumbnails = thread_data->render_thumbnails;
      white_background  = thread_data->white_background;
      thread_data->render_thumbnails = FALSE;
      g_mutex_unlock (&thread_data->mutex);

      /* This handles "spurious wakeup", i.e. cases when g_cond_wait() returned
       * even though there was no call asking us to re-render the thumbnails.
       * See docs of g_cond_wait().
       */
      if (! render_thumbnails)
        continue;

      for (i = 0; i < n_pages; i++)
        {
          IdleData *idle_data = g_new0 (IdleData, 1);
          gboolean  white_background2;

          idle_data->selector = thread_data->selector;
          idle_data->page_no  = i;

          /* FIXME get preferred size from somewhere? */
          idle_data->pixbuf = get_thumb_pixbuf (thread_data->document, i,
                                                THUMBNAIL_SIZE,
                                                white_background);

          g_idle_add (idle_set_thumbnail, idle_data);

          g_mutex_lock (&thread_data->mutex);
          white_background2 = thread_data->white_background;
          stop_thumbnailing = thread_data->stop_thumbnailing;
          g_mutex_unlock (&thread_data->mutex);

          if (stop_thumbnailing || white_background2 != white_background)
            break;
        }

      if (stop_thumbnailing)
        break;
    }

  return NULL;
}

static void
white_background_toggled (GtkToggleButton *widget,
                          ThreadData      *thread_data)
{
  g_mutex_lock (&thread_data->mutex);
  thread_data->white_background  = gtk_toggle_button_get_active (widget);
  thread_data->render_thumbnails = TRUE;
  g_cond_signal (&thread_data->render_thumb);
  g_mutex_unlock (&thread_data->mutex);
}

static GimpPDBStatusType
load_dialog (PopplerDocument     *doc,
             PdfSelectedPages    *pages,
             GimpVectorLoadData   extracted_data,
             GimpProcedure       *procedure,
             GimpProcedureConfig *config)
{
  GtkWidget  *dialog;
  GtkWidget  *vbox;
  GtkWidget  *title;
  GtkWidget  *selector;
  GtkWidget  *white_bg;

  ThreadData  thread_data;
  GThread    *thread;

  gint        i;
  gint        n_pages;

  gboolean    run;

  GimpPageSelectorTarget  target;
  gboolean                white_background;

  n_pages = poppler_document_get_n_pages (doc);

  dialog = gimp_vector_load_procedure_dialog_new (GIMP_VECTOR_LOAD_PROCEDURE (procedure),
                                                  GIMP_PROCEDURE_CONFIG (config),
                                                  &extracted_data, NULL);

  g_object_get (config,
                "target",           &target,
                "white-background", &white_background,
                NULL);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /* Title */
  title = gimp_prop_label_new (G_OBJECT (doc), "title");
  gtk_label_set_ellipsize (GTK_LABEL (title), PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (vbox), title, FALSE, FALSE, 0);

  /* Page Selector */
  selector = gimp_page_selector_new ();
  gtk_widget_set_size_request (selector, 380, 360);
  gtk_box_pack_start (GTK_BOX (vbox), selector, TRUE, TRUE, 0);
  gtk_widget_show (selector);

  if (n_pages <= 0)
    {
      g_message (_("Error getting number of pages from the given PDF file."));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_page_selector_set_n_pages (GIMP_PAGE_SELECTOR (selector), n_pages);
  gimp_page_selector_set_target (GIMP_PAGE_SELECTOR (selector),
                                 target);

  for (i = 0; i < n_pages; i++)
    {
      PopplerPage *page;
      gchar       *label;

      page = poppler_document_get_page (doc, i);
      g_object_get (G_OBJECT (page), "label", &label, NULL);

      gimp_page_selector_set_page_label (GIMP_PAGE_SELECTOR (selector), i,
                                         label);

      g_object_unref (page);
      g_free (label);
    }
  /* Since selecting none will be equivalent to selecting all, this is
   * only useful as a feedback for the default behavior of selecting all
   * pages. */
  gimp_page_selector_select_all (GIMP_PAGE_SELECTOR (selector));

  g_signal_connect_swapped (selector, "activate",
                            G_CALLBACK (gtk_window_activate_default),
                            dialog);

  thread_data.document          = doc;
  thread_data.selector          = GIMP_PAGE_SELECTOR (selector);
  thread_data.render_thumbnails = TRUE;
  thread_data.stop_thumbnailing = FALSE;
  thread_data.white_background  = white_background;
  g_mutex_init (&thread_data.mutex);
  g_cond_init (&thread_data.render_thumb);

  thread = g_thread_new ("thumbnailer", thumbnail_thread, &thread_data);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "reverse-order",
                              NULL);

  white_bg = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                               "white-background", G_TYPE_NONE);
  g_signal_connect (white_bg, "toggled",
                    G_CALLBACK (white_background_toggled), &thread_data);
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "antialias",
                              "white-background",
                              NULL);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  target = gimp_page_selector_get_target (GIMP_PAGE_SELECTOR (selector));
  g_object_set (config,
                "target",     target,
                NULL);

  pages->pages =
    gimp_page_selector_get_selected_pages (GIMP_PAGE_SELECTOR (selector),
                                           &pages->n_pages);

  /* select all if none selected */
  if (pages->n_pages == 0)
    {
      gimp_page_selector_select_all (GIMP_PAGE_SELECTOR (selector));

      pages->pages =
        gimp_page_selector_get_selected_pages (GIMP_PAGE_SELECTOR (selector),
                                               &pages->n_pages);
    }

  /* cleanup */
  g_mutex_lock (&thread_data.mutex);
  thread_data.stop_thumbnailing = TRUE;
  g_cond_signal (&thread_data.render_thumb);
  g_mutex_unlock (&thread_data.mutex);
  g_thread_join (thread);

  g_mutex_clear (&thread_data.mutex);
  g_cond_clear (&thread_data.render_thumb);

  /* XXX Unsure why, I get some CRITICALs when destroying the dialog, unless I
   * unselect all first.
   */
  gimp_page_selector_unselect_all (GIMP_PAGE_SELECTOR (selector));
  gtk_widget_destroy (dialog);

  return run ? GIMP_PDB_SUCCESS : GIMP_PDB_CANCEL;
}
