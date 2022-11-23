/* LIGMA - The GNU Image Manipulation Program
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#undef GTK_DISABLE_SINGLE_INCLUDES
#include <poppler.h>
#define GTK_DISABLE_SINGLE_INCLUDES

#include "libligma/stdplugins-intl.h"


/**
 ** the following was formerly part of
 ** ligmaresolutionentry.h and ligmaresolutionentry.c,
 ** moved here because this is the only thing that uses
 ** it, and it is undesirable to maintain all that api.
 ** Most unused functions have been removed.
 **/
#define LIGMA_TYPE_RESOLUTION_ENTRY            (ligma_resolution_entry_get_type ())
#define LIGMA_RESOLUTION_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_RESOLUTION_ENTRY, LigmaResolutionEntry))
#define LIGMA_RESOLUTION_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_RESOLUTION_ENTRY, LigmaResolutionEntryClass))
#define LIGMA_IS_RESOLUTION_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_RESOLUTION_ENTRY))
#define LIGMA_IS_RESOLUTION_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_RESOLUTION_ENTRY))
#define LIGMA_RESOLUTION_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_RESOLUTION_ENTRY, LigmaResolutionEntryClass))


typedef struct _LigmaResolutionEntry       LigmaResolutionEntry;
typedef struct _LigmaResolutionEntryClass  LigmaResolutionEntryClass;

typedef struct _LigmaResolutionEntryField  LigmaResolutionEntryField;

struct _LigmaResolutionEntryField
{
  LigmaResolutionEntry      *gre;
  LigmaResolutionEntryField *corresponding;

  gboolean       size;

  GtkWidget     *label;

  guint          changed_signal;

  GtkAdjustment *adjustment;
  GtkWidget     *spinbutton;

  gdouble        phy_size;

  gdouble        value;
  gdouble        min_value;
  gdouble        max_value;

  gint           stop_recursion;
};


struct _LigmaResolutionEntry
{
  GtkGrid                   parent_instance;

  LigmaUnit                  size_unit;
  LigmaUnit                  unit;

  GtkWidget                *unitmenu;
  GtkWidget                *chainbutton;

  LigmaResolutionEntryField  width;
  LigmaResolutionEntryField  height;
  LigmaResolutionEntryField  x;
  LigmaResolutionEntryField  y;

};

struct _LigmaResolutionEntryClass
{
  GtkGridClass   parent_class;

  void (* value_changed)  (LigmaResolutionEntry *gse);
  void (* refval_changed) (LigmaResolutionEntry *gse);
  void (* unit_changed)   (LigmaResolutionEntry *gse);
};


GType       ligma_resolution_entry_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_resolution_entry_new          (const gchar   *width_label,
                                                gdouble        width,
                                                const gchar   *height_label,
                                                gdouble        height,
                                                LigmaUnit       size_unit,
                                                const gchar   *res_label,
                                                gdouble        initial_res,
                                                LigmaUnit       initial_unit);

GtkWidget * ligma_resolution_entry_attach_label (LigmaResolutionEntry *gre,
                                                const gchar         *text,
                                                gint                 row,
                                                gint                 column,
                                                gfloat               alignment);

gdouble     ligma_resolution_entry_get_x_in_dpi (LigmaResolutionEntry *gre);

gdouble     ligma_resolution_entry_get_y_in_dpi (LigmaResolutionEntry *gre);


/* signal callback convenience functions */
void        ligma_resolution_entry_update_x_in_dpi (LigmaResolutionEntry *gre,
                                                   gpointer             data);

void        ligma_resolution_entry_update_y_in_dpi (LigmaResolutionEntry *gre,
                                                   gpointer             data);


enum
{
  WIDTH_CHANGED,
  HEIGHT_CHANGED,
  X_CHANGED,
  Y_CHANGED,
  UNIT_CHANGED,
  LAST_SIGNAL
};

static void   ligma_resolution_entry_class_init      (LigmaResolutionEntryClass *class);
static void   ligma_resolution_entry_init            (LigmaResolutionEntry      *gre);

static void   ligma_resolution_entry_update_value    (LigmaResolutionEntryField *gref,
                                                     gdouble              value);
static void   ligma_resolution_entry_value_callback  (GtkAdjustment       *adjustment,
                                                     gpointer             data);
static void   ligma_resolution_entry_update_unit     (LigmaResolutionEntry *gre,
                                                     LigmaUnit             unit);
static void   ligma_resolution_entry_unit_callback   (GtkWidget           *widget,
                                                     LigmaResolutionEntry *gre);

static void   ligma_resolution_entry_field_init (LigmaResolutionEntry      *gre,
                                                LigmaResolutionEntryField *gref,
                                                LigmaResolutionEntryField *corresponding,
                                                guint                     changed_signal,
                                                gdouble                   initial_val,
                                                LigmaUnit                  initial_unit,
                                                gboolean                  size,
                                                gint                      spinbutton_width);

static void   ligma_resolution_entry_format_label (LigmaResolutionEntry *gre,
                                                  GtkWidget           *label,
                                                  gdouble              size);

/**
 ** end of ligmaresolutionentry stuff
 ** the actual code can be found at the end of this file
 **/


#define LOAD_PROC       "file-pdf-load"
#define LOAD_THUMB_PROC "file-pdf-load-thumb"
#define PLUG_IN_BINARY  "file-pdf-load"
#define PLUG_IN_ROLE    "ligma-file-pdf-load"

#define THUMBNAIL_SIZE  128

#define LIGMA_PLUGIN_PDF_LOAD_ERROR ligma_plugin_pdf_load_error_quark ()
static GQuark
ligma_plugin_pdf_load_error_quark (void)
{
  return g_quark_from_static_string ("ligma-plugin-pdf-load-error-quark");
}

/* Structs for the load dialog */
typedef struct
{
  LigmaPageSelectorTarget  target;
  gdouble                 resolution;
  gboolean                antialias;
  gboolean                reverse_order;
  gchar                  *PDF_password;
} PdfLoadVals;

static PdfLoadVals loadvals =
{
  LIGMA_PAGE_SELECTOR_TARGET_LAYERS,
  100.00,  /* resolution in dpi   */
  TRUE,    /* antialias */
  FALSE,   /* reverse_order */
  NULL     /* pdf_password */
};

typedef struct
{
  gint  n_pages;
  gint *pages;
} PdfSelectedPages;


typedef struct _Pdf      Pdf;
typedef struct _PdfClass PdfClass;

struct _Pdf
{
  LigmaPlugIn      parent_instance;
};

struct _PdfClass
{
  LigmaPlugInClass parent_class;
};


#define PDF_TYPE  (pdf_get_type ())
#define PDF (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PDF_TYPE, Pdf))

GType                   pdf_get_type         (void) G_GNUC_CONST;

static GList          * pdf_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * pdf_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * pdf_load             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * pdf_load_thumb       (LigmaProcedure        *procedure,
                                              GFile                *file,
                                              gint                  size,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);

static LigmaImage       * load_image          (PopplerDocument      *doc,
                                              GFile                *file,
                                              LigmaRunMode           run_mode,
                                              LigmaPageSelectorTarget target,
                                              gdouble               resolution,
                                              gboolean              antialias,
                                              gboolean              reverse_order,
                                              PdfSelectedPages     *pages);

static LigmaPDBStatusType load_dialog         (PopplerDocument      *doc,
                                              PdfSelectedPages     *pages);

static PopplerDocument * open_document       (GFile                *file,
                                              const gchar          *PDF_password,
                                              LigmaRunMode           run_mode,
                                              GError              **error);

static cairo_surface_t * get_thumb_surface   (PopplerDocument      *doc,
                                              gint                  page,
                                              gint                  preferred_size);

static GdkPixbuf       * get_thumb_pixbuf    (PopplerDocument      *doc,
                                              gint                  page,
                                              gint                  preferred_size);

static LigmaLayer       * layer_from_surface  (LigmaImage            *image,
                                              const gchar          *layer_name,
                                              gint                  position,
                                              cairo_surface_t      *surface,
                                              gdouble               progress_start,
                                              gdouble               progress_scale);


G_DEFINE_TYPE (Pdf, pdf, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (PDF_TYPE)
DEFINE_STD_SET_I18N


static void
pdf_class_init (PdfClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = pdf_query_procedures;
  plug_in_class->create_procedure = pdf_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
pdf_init (Pdf *pdf)
{
}

static GList *
pdf_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static LigmaProcedure *
pdf_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           pdf_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("Portable Document Format"));

      ligma_procedure_set_documentation (procedure,
                                        "Load file in PDF format",
                                        "Loads files in Adobe's Portable "
                                        "Document Format. PDF is designed to "
                                        "be easily processed by a variety "
                                        "of different platforms, and is a "
                                        "distant cousin of PostScript.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Nathan Summers, Lionel N.",
                                      "Nathan Summers, Lionel N.",
                                      "2005, 2017");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "application/pdf");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "pdf");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0, string,%PDF-");

      ligma_load_procedure_set_thumbnail_loader (LIGMA_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);

      LIGMA_PROC_ARG_STRING (procedure, "pdf-password",
                            "PDF password",
                            "The password to decrypt the encrypted PDF file",
                            NULL,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "reverse-order",
                             "Load in reverse order",
                             "Load PDF pages in reverse order",
                             FALSE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "n-pages",
                         "N pages",
                         "Number of pages to load (0 for all)",
                         0, G_MAXINT, 0,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT32_ARRAY (procedure, "pages",
                                 "Pages",
                                 "The pages to load in the expected order",
                                 G_PARAM_READWRITE);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = ligma_thumbnail_procedure_new (plug_in, name,
                                                LIGMA_PDB_PROC_TYPE_PLUGIN,
                                                pdf_load_thumb, NULL, NULL);

      ligma_procedure_set_documentation (procedure,
                                        "Loads a preview from a PDF file.",
                                        "Loads a small preview of the first "
                                        "page of the PDF format file. Uses "
                                        "the embedded thumbnail if present.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Nathan Summers",
                                      "Nathan Summers",
                                      "2005");
    }

  return procedure;
}

static LigmaValueArray *
pdf_load (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaValueArray    *return_vals;
  LigmaPDBStatusType  status   = LIGMA_PDB_SUCCESS;
  LigmaImage         *image    = NULL;
  PopplerDocument   *doc      = NULL;
  PdfSelectedPages   pages    = { 0, NULL };
  GError            *error    = NULL;

  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
      ligma_get_data (LOAD_PROC, &loadvals);
      ligma_ui_init (PLUG_IN_BINARY);
      doc = open_document (file,
                           loadvals.PDF_password,
                           run_mode, &error);

      if (! doc)
        {
          status = LIGMA_PDB_EXECUTION_ERROR;
          break;
        }

      status = load_dialog (doc, &pages);
      if (status == LIGMA_PDB_SUCCESS)
        {
          ligma_set_data (LOAD_PROC, &loadvals, sizeof(loadvals));
        }
      break;

    case LIGMA_RUN_WITH_LAST_VALS:
      /* FIXME: implement last vals mode */
      status = LIGMA_PDB_EXECUTION_ERROR;
      break;

    case LIGMA_RUN_NONINTERACTIVE:
      doc = open_document (file,
                           LIGMA_VALUES_GET_STRING (args, 0),
                           run_mode, &error);

      loadvals.reverse_order = LIGMA_VALUES_GET_BOOLEAN (args, 1);

      if (doc)
        {
          PopplerPage *test_page = poppler_document_get_page (doc, 0);

          if (test_page)
            {
              gint i;
              gint doc_n_pages;

              doc_n_pages = poppler_document_get_n_pages (doc);
              /* The number of imported pages may be bigger than
               * the number of pages from the original document.
               * Indeed it is possible to duplicate some pages
               * by setting the same number several times in the
               * "pages" argument.
               * Not ceiling this value is *not* an error.
               */
              pages.n_pages = LIGMA_VALUES_GET_INT (args, 2);
              if (pages.n_pages <= 0)
                {
                  pages.n_pages = doc_n_pages;
                  pages.pages = g_new (gint, pages.n_pages);
                  for (i = 0; i < pages.n_pages; i++)
                    pages.pages[i] = i;
                }
              else
                {
                  const gint32 *p = LIGMA_VALUES_GET_INT32_ARRAY (args, 3);

                  pages.pages = g_new (gint, pages.n_pages);

                  for (i = 0; i < pages.n_pages; i++)
                    {
                      if (p[i] >= doc_n_pages)
                        {
                          status = LIGMA_PDB_EXECUTION_ERROR;
                          g_set_error (&error, LIGMA_PLUGIN_PDF_LOAD_ERROR, 0,
                                       /* TRANSLATORS: first argument is file name,
                                        * second is out-of-range page number,
                                        * third is number of pages.
                                        * Specify order as in English if needed.
                                        */
                                       ngettext ("PDF document '%1$s' has %3$d page. Page %2$d is out of range.",
                                                 "PDF document '%1$s' has %3$d pages. Page %2$d is out of range.",
                                                 doc_n_pages),
                                       ligma_file_get_utf8_name (file),
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
      else
        {
          status = LIGMA_PDB_EXECUTION_ERROR;
          g_object_unref (doc);
        }
      break;
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      image = load_image (doc,
                          file,
                          run_mode,
                          loadvals.target,
                          loadvals.resolution,
                          loadvals.antialias,
                          loadvals.reverse_order,
                          &pages);
    }

  if (doc)
    g_object_unref (doc);

  g_free (pages.pages);

  if (! image)
    return ligma_procedure_new_return_values (procedure, status, error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaValueArray *
pdf_load_thumb (LigmaProcedure        *procedure,
                GFile                *file,
                gint                  size,
                const LigmaValueArray *args,
                gpointer              run_data)
{
  LigmaValueArray  *return_vals;
  gdouble          width  = 0;
  gdouble          height = 0;
  gdouble          scale;
  LigmaImage       *image     = NULL;
  gint             num_pages = 0;
  PopplerDocument *doc       = NULL;
  cairo_surface_t *surface   = NULL;
  GError          *error     = NULL;

  gegl_init (NULL, NULL);

  doc = open_document (file,
                       NULL,
                       LIGMA_RUN_NONINTERACTIVE,
                       &error);

  if (doc)
    {
      PopplerPage *page = poppler_document_get_page (doc, 0);

      if (page)
        {
          poppler_page_get_size (page, &width, &height);

          g_object_unref (page);
        }

      num_pages = poppler_document_get_n_pages (doc);

      surface = get_thumb_surface (doc, 0, size);

      g_object_unref (doc);
    }

  if (surface)
    {
      image = ligma_image_new (cairo_image_surface_get_width (surface),
                              cairo_image_surface_get_height (surface),
                              LIGMA_RGB);

      ligma_image_undo_disable (image);

      layer_from_surface (image, "thumbnail", 0, surface, 0.0, 1.0);
      cairo_surface_destroy (surface);

      ligma_image_undo_enable (image);
      ligma_image_clean_all (image);
    }

  scale = loadvals.resolution / ligma_unit_get_factor (LIGMA_UNIT_POINT);

  width  *= scale;
  height *= scale;

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);
  LIGMA_VALUES_SET_INT   (return_vals, 2, width);
  LIGMA_VALUES_SET_INT   (return_vals, 3, height);
  LIGMA_VALUES_SET_ENUM  (return_vals, 4, LIGMA_RGB_IMAGE);
  LIGMA_VALUES_SET_INT   (return_vals, 5, num_pages);

  return return_vals;
}

static PopplerDocument *
open_document (GFile        *file,
               const gchar  *PDF_password,
               LigmaRunMode   run_mode,
               GError      **load_error)
{
  PopplerDocument *doc;
  GError          *error = NULL;

  doc = poppler_document_new_from_gfile (file, PDF_password, NULL, &error);

  if (run_mode == LIGMA_RUN_INTERACTIVE)
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

          dialog = ligma_dialog_new (_("Encrypted PDF"), PLUG_IN_ROLE,
                                    NULL, 0,
                                    NULL, NULL,
                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("_OK"), GTK_RESPONSE_OK,
                                    NULL);
          ligma_window_set_transient (GTK_WINDOW (dialog));
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

          run = ligma_dialog_run (LIGMA_DIALOG (dialog));
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
                   ligma_file_get_utf8_name (file),
                   error->message);
      g_error_free (error);
      return NULL;
    }

  return doc;
}

static LigmaLayer *
layer_from_surface (LigmaImage       *image,
                    const gchar     *layer_name,
                    gint             position,
                    cairo_surface_t *surface,
                    gdouble          progress_start,
                    gdouble          progress_scale)
{
  LigmaLayer *layer;

  layer = ligma_layer_new_from_surface (image, layer_name, surface,
                                       progress_start,
                                       progress_start + progress_scale);
  ligma_image_insert_layer (image, layer, NULL, position);

  return layer;
}

static cairo_surface_t *
render_page_to_surface (PopplerPage *page,
                        int          width,
                        int          height,
                        double       scale,
                        gboolean     antialias)
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

  cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OVER);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);

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

static LigmaImage *
load_image (PopplerDocument        *doc,
            GFile                  *file,
            LigmaRunMode             run_mode,
            LigmaPageSelectorTarget  target,
            gdouble                 resolution,
            gboolean                antialias,
            gboolean                reverse_order,
            PdfSelectedPages       *pages)
{
  LigmaImage  *image = NULL;
  LigmaImage **images   = NULL;
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

  if (target == LIGMA_PAGE_SELECTOR_TARGET_IMAGES)
    images = g_new0 (LigmaImage *, pages->n_pages);

  ligma_progress_init_printf (_("Opening '%s'"),
                             ligma_file_get_utf8_name (file));

  scale = resolution / ligma_unit_get_factor (LIGMA_UNIT_POINT);

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
      width  = page_width  * scale;
      height = page_height * scale;

      g_object_get (G_OBJECT (page), "label", &page_label, NULL);

      if (! image)
        {
          GFile *new_file;
          gchar *uri;
          gchar *new_uri;

          image = ligma_image_new (width, height, LIGMA_RGB);
          ligma_image_undo_disable (image);

          uri = g_file_get_uri (file);

          if (target == LIGMA_PAGE_SELECTOR_TARGET_IMAGES)
            new_uri = g_strdup_printf (_("%s-%s"), uri, page_label);
          else
            new_uri = g_strdup_printf (_("%s-pages"), uri);

          g_free (uri);

          new_file = g_file_new_for_uri (new_uri);

          g_free (new_uri);

          ligma_image_set_file (image, new_file);

          g_object_unref (new_file);

          ligma_image_set_resolution (image, resolution, resolution);
        }

      surface = render_page_to_surface (page, width, height, scale, antialias);

      layer_from_surface (image, page_label, 0, surface,
                          doc_progress, 1.0 / pages->n_pages);

      g_free (page_label);
      cairo_surface_destroy (surface);

      doc_progress = (double) (i + 1) / pages->n_pages;
      ligma_progress_update (doc_progress);

      if (target == LIGMA_PAGE_SELECTOR_TARGET_IMAGES)
        {
          images[i] = image;

          ligma_image_undo_enable (image);
          ligma_image_clean_all (image);

          image = 0;
        }
    }
  ligma_progress_update (1.0);

  if (image)
    {
      ligma_image_undo_enable (image);
      ligma_image_clean_all (image);
    }

  if (target == LIGMA_PAGE_SELECTOR_TARGET_IMAGES)
    {
      if (run_mode != LIGMA_RUN_NONINTERACTIVE)
        {
          /* Display images in reverse order.  The last will be
           * displayed by LIGMA itself
           */
          for (i = pages->n_pages - 1; i > 0; i--)
            ligma_display_new (images[i]);
        }

      image = images[0];

      g_free (images);
    }

  return image;
}

static cairo_surface_t *
get_thumb_surface (PopplerDocument *doc,
                   gint             page_num,
                   gint             preferred_size)
{
  PopplerPage *page;
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

      surface = render_page_to_surface (page, width, height, scale, TRUE);
    }

  g_object_unref (page);

  return surface;
}

static GdkPixbuf *
get_thumb_pixbuf (PopplerDocument *doc,
                  gint             page_num,
                  gint             preferred_size)
{
  cairo_surface_t *surface;
  GdkPixbuf *pixbuf;

  surface = get_thumb_surface (doc, page_num, preferred_size);
  pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0,
                                        cairo_image_surface_get_width (surface),
                                        cairo_image_surface_get_height (surface));
  cairo_surface_destroy (surface);

  return pixbuf;
}

typedef struct
{
  PopplerDocument  *document;
  LigmaPageSelector *selector;
  gboolean          stop_thumbnailing;
} ThreadData;

typedef struct
{
  LigmaPageSelector *selector;
  gint              page_no;
  GdkPixbuf        *pixbuf;
} IdleData;

static gboolean
idle_set_thumbnail (gpointer data)
{
  IdleData *idle_data = data;

  ligma_page_selector_set_page_thumbnail (idle_data->selector,
                                         idle_data->page_no,
                                         idle_data->pixbuf);
  g_object_unref (idle_data->pixbuf);
  g_free (idle_data);

  return FALSE;
}

static gpointer
thumbnail_thread (gpointer data)
{
  ThreadData  *thread_data = data;
  gint         n_pages;
  gint         i;

  n_pages = poppler_document_get_n_pages (thread_data->document);

  for (i = 0; i < n_pages; i++)
    {
      IdleData *idle_data = g_new0 (IdleData, 1);

      idle_data->selector = thread_data->selector;
      idle_data->page_no  = i;

      /* FIXME get preferred size from somewhere? */
      idle_data->pixbuf = get_thumb_pixbuf (thread_data->document, i,
                                            THUMBNAIL_SIZE);

      g_idle_add (idle_set_thumbnail, idle_data);

      if (thread_data->stop_thumbnailing)
        break;
    }

  return NULL;
}

static LigmaPDBStatusType
load_dialog (PopplerDocument  *doc,
             PdfSelectedPages *pages)
{
  GtkWidget  *dialog;
  GtkWidget  *vbox;
  GtkWidget  *title;
  GtkWidget  *selector;
  GtkWidget  *resolution;
  GtkWidget  *antialias;
  GtkWidget  *hbox;
  GtkWidget  *reverse_order;
  GtkWidget  *separator;

  ThreadData  thread_data;
  GThread    *thread;

  gint        i;
  gint        n_pages;

  gdouble     width;
  gdouble     height;

  gboolean    run;

  dialog = ligma_dialog_new (_("Import from PDF"), PLUG_IN_ROLE,
                            NULL, 0,
                            ligma_standard_help_func, LOAD_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_Import"), GTK_RESPONSE_OK,

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

  /* Title */
  title = ligma_prop_label_new (G_OBJECT (doc), "title");
  gtk_label_set_ellipsize (GTK_LABEL (title), PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (vbox), title, FALSE, FALSE, 0);

  /* Page Selector */
  selector = ligma_page_selector_new ();
  gtk_widget_set_size_request (selector, 380, 360);
  gtk_box_pack_start (GTK_BOX (vbox), selector, TRUE, TRUE, 0);
  gtk_widget_show (selector);

  n_pages = poppler_document_get_n_pages (doc);

  if (n_pages <= 0)
    {
      g_message (_("Error getting number of pages from the given PDF file."));
      return LIGMA_PDB_EXECUTION_ERROR;
    }

  ligma_page_selector_set_n_pages (LIGMA_PAGE_SELECTOR (selector), n_pages);
  ligma_page_selector_set_target (LIGMA_PAGE_SELECTOR (selector),
                                 loadvals.target);

  for (i = 0; i < n_pages; i++)
    {
      PopplerPage     *page;
      gchar           *label;

      page = poppler_document_get_page (doc, i);
      g_object_get (G_OBJECT (page), "label", &label, NULL);

      ligma_page_selector_set_page_label (LIGMA_PAGE_SELECTOR (selector), i,
                                         label);

      if (i == 0)
        poppler_page_get_size (page, &width, &height);

      g_object_unref (page);
      g_free (label);
    }
  /* Since selecting none will be equivalent to selecting all, this is
   * only useful as a feedback for the default behavior of selecting all
   * pages. */
  ligma_page_selector_select_all (LIGMA_PAGE_SELECTOR (selector));

  g_signal_connect_swapped (selector, "activate",
                            G_CALLBACK (gtk_window_activate_default),
                            dialog);

  thread_data.document          = doc;
  thread_data.selector          = LIGMA_PAGE_SELECTOR (selector);
  thread_data.stop_thumbnailing = FALSE;

  thread = g_thread_new ("thumbnailer", thumbnail_thread, &thread_data);

  /* "Load in reverse order" toggle button */
  reverse_order = gtk_check_button_new_with_mnemonic (_("Load in reverse order"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (reverse_order), loadvals.reverse_order);
  gtk_box_pack_start (GTK_BOX (vbox), reverse_order, FALSE, FALSE, 0);

  g_signal_connect (reverse_order, "toggled",
                    G_CALLBACK (ligma_toggle_button_update), &loadvals.reverse_order);
  gtk_widget_show (reverse_order);

  /* Separator */
  separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 0);
  gtk_widget_show (separator);

  /* Resolution */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  resolution = ligma_resolution_entry_new (_("_Width (pixels):"), width,
                                          _("_Height (pixels):"), height,
                                          LIGMA_UNIT_POINT,
                                          _("_Resolution:"),
                                          loadvals.resolution, LIGMA_UNIT_INCH);

  gtk_box_pack_start (GTK_BOX (hbox), resolution, FALSE, FALSE, 0);
  gtk_widget_show (resolution);

  g_signal_connect (resolution, "x-changed",
                    G_CALLBACK (ligma_resolution_entry_update_x_in_dpi),
                    &loadvals.resolution);

  /* Antialiasing*/
  antialias = gtk_check_button_new_with_mnemonic (_("Use _Anti-aliasing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (antialias), loadvals.antialias);
  gtk_box_pack_start (GTK_BOX (vbox), antialias, FALSE, FALSE, 0);

  g_signal_connect (antialias, "toggled",
                    G_CALLBACK (ligma_toggle_button_update), &loadvals.antialias);
  gtk_widget_show (antialias);

  /* Setup done; display the dialog */
  gtk_widget_show (dialog);

  /* run the dialog */
  run = (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK);

  loadvals.target =
    ligma_page_selector_get_target (LIGMA_PAGE_SELECTOR (selector));

  pages->pages =
    ligma_page_selector_get_selected_pages (LIGMA_PAGE_SELECTOR (selector),
                                           &pages->n_pages);

  /* select all if none selected */
  if (pages->n_pages == 0)
    {
      ligma_page_selector_select_all (LIGMA_PAGE_SELECTOR (selector));

      pages->pages =
        ligma_page_selector_get_selected_pages (LIGMA_PAGE_SELECTOR (selector),
                                               &pages->n_pages);
    }

  /* cleanup */
  thread_data.stop_thumbnailing = TRUE;
  g_thread_join (thread);

  return run ? LIGMA_PDB_SUCCESS : LIGMA_PDB_CANCEL;
}


/**
 ** code for LigmaResolutionEntry widget, formerly in libligmawidgets
 **/

static guint ligma_resolution_entry_signals[LAST_SIGNAL] = { 0 };

static GtkGridClass *parent_class = NULL;


GType
ligma_resolution_entry_get_type (void)
{
  static GType gre_type = 0;

  if (! gre_type)
    {
      const GTypeInfo gre_info =
      {
        sizeof (LigmaResolutionEntryClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) ligma_resolution_entry_class_init,
        NULL,                /* class_finalize */
        NULL,                /* class_data     */
        sizeof (LigmaResolutionEntry),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) ligma_resolution_entry_init,
      };

      gre_type = g_type_register_static (GTK_TYPE_GRID,
                                         "LigmaResolutionEntry",
                                         &gre_info, 0);
    }

  return gre_type;
}

static void
ligma_resolution_entry_class_init (LigmaResolutionEntryClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  ligma_resolution_entry_signals[HEIGHT_CHANGED] =
    g_signal_new ("height-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaResolutionEntryClass, value_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_resolution_entry_signals[WIDTH_CHANGED] =
    g_signal_new ("width-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaResolutionEntryClass, value_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_resolution_entry_signals[X_CHANGED] =
    g_signal_new ("x-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaResolutionEntryClass, value_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_resolution_entry_signals[Y_CHANGED] =
    g_signal_new ("y-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaResolutionEntryClass, refval_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_resolution_entry_signals[UNIT_CHANGED] =
    g_signal_new ("unit-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaResolutionEntryClass, unit_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  klass->value_changed  = NULL;
  klass->refval_changed = NULL;
  klass->unit_changed   = NULL;
}

static void
ligma_resolution_entry_init (LigmaResolutionEntry *gre)
{
  gre->unitmenu = NULL;
  gre->unit     = LIGMA_UNIT_INCH;

  gtk_grid_set_row_spacing (GTK_GRID (gre), 2);
  gtk_grid_set_column_spacing (GTK_GRID (gre), 4);
}

static void
ligma_resolution_entry_field_init (LigmaResolutionEntry      *gre,
                                  LigmaResolutionEntryField *gref,
                                  LigmaResolutionEntryField *corresponding,
                                  guint                     changed_signal,
                                  gdouble                   initial_val,
                                  LigmaUnit                  initial_unit,
                                  gboolean                  size,
                                  gint                      spinbutton_width)
{
  gint digits;

  g_return_if_fail (LIGMA_IS_RESOLUTION_ENTRY (gre));

  gref->gre               = gre;
  gref->corresponding     = corresponding;
  gref->changed_signal    = ligma_resolution_entry_signals[changed_signal];

  if (size)
    {
      gref->value         = initial_val /
                            ligma_unit_get_factor (initial_unit) *
                            corresponding->value *
                            ligma_unit_get_factor (gre->unit);

      gref->phy_size      = initial_val /
                            ligma_unit_get_factor (initial_unit);
    }
  else
    {
      gref->value         = initial_val;
    }

  gref->min_value         = LIGMA_MIN_RESOLUTION;
  gref->max_value         = LIGMA_MAX_RESOLUTION;
  gref->adjustment        = NULL;

  gref->stop_recursion    = 0;

  gref->size              = size;

  if (size)
    {
      gref->label = g_object_new (GTK_TYPE_LABEL,
                                  "xalign", 0.0,
                                  "yalign", 0.5,
                                  NULL);
      ligma_label_set_attributes (GTK_LABEL (gref->label),
                                 PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                                 -1);

      ligma_resolution_entry_format_label (gre, gref->label, gref->phy_size);
    }

  digits = size ? 0 : MIN (ligma_unit_get_digits (initial_unit), 5) + 1;

  gref->adjustment = gtk_adjustment_new (gref->value,
                                         gref->min_value,
                                         gref->max_value,
                                         1.0, 10.0, 0.0);
  gref->spinbutton = ligma_spin_button_new (gref->adjustment,
                                           1.0, digits);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (gref->spinbutton), TRUE);

  if (spinbutton_width > 0)
    {
      if (spinbutton_width < 17)
        gtk_entry_set_width_chars (GTK_ENTRY (gref->spinbutton),
                                   spinbutton_width);
      else
        gtk_widget_set_size_request (gref->spinbutton,
                                     spinbutton_width, -1);
    }
}

/**
 * ligma_resolution_entry_new:
 * @width_label:       Optional label for the width control.
 * @width:             Width of the item, specified in terms of @size_unit.
 * @height_label:      Optional label for the height control.
 * @height:            Height of the item, specified in terms of @size_unit.
 * @size_unit:         Unit used to specify the width and height.
 * @res_label:         Optional label for the resolution entry.
 * @initial_res:       The initial resolution.
 * @initial_unit:      The initial unit.
 *
 * Creates a new #LigmaResolutionEntry widget.
 *
 * The #LigmaResolutionEntry is derived from #GtkGrid and will have
 * an empty border of one cell width on each side plus an empty column left
 * of the #LigmaUnitMenu to allow the caller to add labels or other widgets.
 *
 * A #LigmaChainButton is displayed if independent is set to %TRUE.
 *
 * Returns: A pointer to the new #LigmaResolutionEntry widget.
 **/
GtkWidget *
ligma_resolution_entry_new (const gchar *width_label,
                           gdouble      width,
                           const gchar *height_label,
                           gdouble      height,
                           LigmaUnit     size_unit,
                           const gchar *res_label,
                           gdouble      initial_res,
                           LigmaUnit     initial_unit)
{
  LigmaResolutionEntry *gre;
  GtkTreeModel        *model;

  gre = g_object_new (LIGMA_TYPE_RESOLUTION_ENTRY, NULL);

  gre->unit = initial_unit;

  ligma_resolution_entry_field_init (gre, &gre->x,
                                    &gre->width,
                                    X_CHANGED,
                                    initial_res, initial_unit,
                                    FALSE, 0);

  gtk_grid_attach (GTK_GRID (gre), gre->x.spinbutton, 1, 3, 1, 1);

  g_signal_connect (gre->x.adjustment, "value-changed",
                    G_CALLBACK (ligma_resolution_entry_value_callback),
                    &gre->x);

  gtk_widget_show (gre->x.spinbutton);

  gre->unitmenu = ligma_unit_combo_box_new ();
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (gre->unitmenu));
  ligma_unit_store_set_has_pixels (LIGMA_UNIT_STORE (model), FALSE);
  ligma_unit_store_set_has_percent (LIGMA_UNIT_STORE (model), FALSE);
  g_object_set (model,
                "short-format", _("pixels/%a"),
                "long-format",  _("pixels/%a"),
                NULL);
  ligma_unit_combo_box_set_active (LIGMA_UNIT_COMBO_BOX (gre->unitmenu),
                                  initial_unit);

  gtk_grid_attach (GTK_GRID (gre), gre->unitmenu, 3, 3, 1, 1);
  g_signal_connect (gre->unitmenu, "changed",
                    G_CALLBACK (ligma_resolution_entry_unit_callback),
                    gre);
  gtk_widget_show (gre->unitmenu);

  ligma_resolution_entry_field_init (gre, &gre->width,
                                    &gre->x,
                                    WIDTH_CHANGED,
                                    width, size_unit,
                                    TRUE, 0);

  gtk_grid_attach (GTK_GRID (gre), gre->width.spinbutton, 1, 1, 1, 1);

  gtk_grid_attach (GTK_GRID (gre), gre->width.label, 3, 1, 1, 1);

  g_signal_connect (gre->width.adjustment, "value-changed",
                    G_CALLBACK (ligma_resolution_entry_value_callback),
                    &gre->width);

  gtk_widget_show (gre->width.spinbutton);
  gtk_widget_show (gre->width.label);

  ligma_resolution_entry_field_init (gre, &gre->height, &gre->x,
                                    HEIGHT_CHANGED,
                                    height, size_unit,
                                    TRUE, 0);

  gtk_grid_attach (GTK_GRID (gre), gre->height.spinbutton, 1, 2, 1, 1);

  gtk_grid_attach (GTK_GRID (gre), gre->height.label, 3, 2, 1, 1);

  g_signal_connect (gre->height.adjustment, "value-changed",
                    G_CALLBACK (ligma_resolution_entry_value_callback),
                    &gre->height);

  gtk_widget_show (gre->height.spinbutton);
  gtk_widget_show (gre->height.label);

  if (width_label)
    ligma_resolution_entry_attach_label (gre, width_label,  1, 0, 0.0);

  if (height_label)
    ligma_resolution_entry_attach_label (gre, height_label, 2, 0, 0.0);

  if (res_label)
    ligma_resolution_entry_attach_label (gre, res_label,    3, 0, 0.0);

  return GTK_WIDGET (gre);
}

/**
 * ligma_resolution_entry_attach_label:
 * @gre:       The #LigmaResolutionEntry you want to add a label to.
 * @text:      The text of the label.
 * @row:       The row where the label will be attached.
 * @column:    The column where the label will be attached.
 * @alignment: The horizontal alignment of the label.
 *
 * Attaches a #GtkLabel to the #LigmaResolutionEntry (which is a #GtkGrid).
 *
 * Returns: A pointer to the new #GtkLabel widget.
 **/
GtkWidget *
ligma_resolution_entry_attach_label (LigmaResolutionEntry *gre,
                                    const gchar         *text,
                                    gint                 row,
                                    gint                 column,
                                    gfloat               alignment)
{
  GtkWidget *label;

  g_return_val_if_fail (LIGMA_IS_RESOLUTION_ENTRY (gre), NULL);
  g_return_val_if_fail (text != NULL, NULL);

  label = gtk_label_new_with_mnemonic (text);

  if (column == 0)
    {
      GList *children;
      GList *list;

      children = gtk_container_get_children (GTK_CONTAINER (gre));

      for (list = children; list; list = g_list_next (list))
        {
          GtkWidget *child = list->data;
          gint       left_attach;
          gint       top_attach;

          gtk_container_child_get (GTK_CONTAINER (gre), child,
                                   "left-attach", &left_attach,
                                   "top-attach",  &top_attach,
                                   NULL);

          if (left_attach == 1 && top_attach == row)
            {
              gtk_label_set_mnemonic_widget (GTK_LABEL (label), child);
              break;
            }
        }

      g_list_free (children);
    }

  gtk_label_set_xalign (GTK_LABEL (label), alignment);

  gtk_grid_attach (GTK_GRID (gre), label, column, row, 1, 1);
  gtk_widget_show (label);

  return label;
}

/**
 * ligma_resolution_entry_get_x_in_dpi;
 * @gre:   The #LigmaResolutionEntry you want to know the resolution of.
 *
 * Returns the X resolution of the #LigmaResolutionEntry in pixels per inch.
 **/
gdouble
ligma_resolution_entry_get_x_in_dpi (LigmaResolutionEntry *gre)
{
  g_return_val_if_fail (LIGMA_IS_RESOLUTION_ENTRY (gre), 0);

  /* dots_in_one_unit * units_in_one_inch -> dpi */
  return gre->x.value * ligma_unit_get_factor (gre->unit);
}

/**
 * ligma_resolution_entry_get_y_in_dpi;
 * @gre:   The #LigmaResolutionEntry you want to know the resolution of.
 *
 * Returns the Y resolution of the #LigmaResolutionEntry in pixels per inch.
 **/
gdouble
ligma_resolution_entry_get_y_in_dpi (LigmaResolutionEntry *gre)
{
  g_return_val_if_fail (LIGMA_IS_RESOLUTION_ENTRY (gre), 0);

  return gre->y.value * ligma_unit_get_factor (gre->unit);
}


static void
ligma_resolution_entry_update_value (LigmaResolutionEntryField *gref,
                                    gdouble                   value)
{
  if (gref->stop_recursion > 0)
    return;

  gref->value = value;

  gref->stop_recursion++;

  if (gref->size)
    ligma_resolution_entry_update_value (gref->corresponding,
                                        gref->value /
                                          gref->phy_size /
                                          ligma_unit_get_factor (gref->gre->unit));
  else
    {
      gdouble factor = ligma_unit_get_factor (gref->gre->unit);

      ligma_resolution_entry_update_value (&gref->gre->width,
                                          gref->value *
                                          gref->gre->width.phy_size *
                                          factor);

      ligma_resolution_entry_update_value (&gref->gre->height,
                                          gref->value *
                                          gref->gre->height.phy_size *
                                          factor);
    }

  gtk_adjustment_set_value (gref->adjustment, value);

  gref->stop_recursion--;

  g_signal_emit (gref->gre, gref->changed_signal, 0);
}

static void
ligma_resolution_entry_value_callback (GtkAdjustment *adjustment,
                                      gpointer       data)
{
  LigmaResolutionEntryField *gref = (LigmaResolutionEntryField *) data;
  gdouble                   new_value;

  new_value = gtk_adjustment_get_value (adjustment);

  if (gref->value != new_value)
    ligma_resolution_entry_update_value (gref, new_value);
}

static void
ligma_resolution_entry_update_unit (LigmaResolutionEntry *gre,
                                   LigmaUnit             unit)
{
  LigmaUnit  old_unit;
  gint      digits;
  gdouble   factor;

  old_unit  = gre->unit;
  gre->unit = unit;

  digits = (ligma_unit_get_digits (LIGMA_UNIT_INCH) -
            ligma_unit_get_digits (unit));

  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gre->x.spinbutton),
                              MAX (3 + digits, 3));

  factor = ligma_unit_get_factor (old_unit) / ligma_unit_get_factor (unit);

  gre->x.min_value *= factor;
  gre->x.max_value *= factor;
  gre->x.value     *= factor;

  gtk_adjustment_set_value (gre->x.adjustment, gre->x.value);

  ligma_resolution_entry_format_label (gre,
                                      gre->width.label, gre->width.phy_size);
  ligma_resolution_entry_format_label (gre,
                                      gre->height.label, gre->height.phy_size);

  g_signal_emit (gre, ligma_resolution_entry_signals[UNIT_CHANGED], 0);
}

static void
ligma_resolution_entry_unit_callback (GtkWidget           *widget,
                                     LigmaResolutionEntry *gre)
{
  LigmaUnit new_unit;

  new_unit = ligma_unit_combo_box_get_active (LIGMA_UNIT_COMBO_BOX (widget));

  if (gre->unit != new_unit)
    ligma_resolution_entry_update_unit (gre, new_unit);
}

/**
 * ligma_resolution_entry_update_x_in_dpi:
 * @gre: the #LigmaResolutionEntry
 * @data: a pointer to a gdouble
 *
 * Convenience function to set a double to the X resolution, suitable
 * for use as a signal callback.
 */
void
ligma_resolution_entry_update_x_in_dpi (LigmaResolutionEntry *gre,
                                       gpointer             data)
{
  gdouble *val;

  g_return_if_fail (gre  != NULL);
  g_return_if_fail (data != NULL);
  g_return_if_fail (LIGMA_IS_RESOLUTION_ENTRY (gre));

  val = (gdouble *) data;

  *val = ligma_resolution_entry_get_x_in_dpi (gre);
}

/**
 * ligma_resolution_entry_update_y_in_dpi:
 * @gre: the #LigmaResolutionEntry
 * @data: a pointer to a gdouble
 *
 * Convenience function to set a double to the Y resolution, suitable
 * for use as a signal callback.
 */
void
ligma_resolution_entry_update_y_in_dpi (LigmaResolutionEntry *gre,
                                       gpointer             data)
{
  gdouble *val;

  g_return_if_fail (gre  != NULL);
  g_return_if_fail (data != NULL);
  g_return_if_fail (LIGMA_IS_RESOLUTION_ENTRY (gre));

  val = (gdouble *) data;

  *val = ligma_resolution_entry_get_y_in_dpi (gre);
}

static void
ligma_resolution_entry_format_label (LigmaResolutionEntry *gre,
                                    GtkWidget           *label,
                                    gdouble              size)
{
  gchar *format = g_strdup_printf ("%%.%df %%s",
                                   ligma_unit_get_digits (gre->unit));
  gchar *text = g_strdup_printf (format,
                                 size * ligma_unit_get_factor (gre->unit),
                                 ligma_unit_get_plural (gre->unit));
  g_free (format);

  gtk_label_set_text (GTK_LABEL (label), text);
  g_free (text);
}
