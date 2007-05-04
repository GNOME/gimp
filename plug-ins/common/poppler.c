/* GIMP - The GNU Image Manipulation Program
 *
 * poppler.c - PDF file loader
 *
 * Copyright (C) 2005 Nathan Summers
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
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <poppler.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC       "file-pdf-load"
#define LOAD_THUMB_PROC "file-pdf-load-thumb"
#define PLUG_IN_BINARY  "poppler"

#define THUMBNAIL_SIZE  128


/* Structs for the load dialog */
typedef struct
{
  GimpPageSelectorTarget target;
  gdouble                resolution;
} PdfLoadVals;

static PdfLoadVals loadvals =
{
  GIMP_PAGE_SELECTOR_TARGET_LAYERS,
  100.00  /* 100 dpi   */
};

typedef struct
{
  gint  n_pages;
  gint *pages;
} PdfSelectedPages;

/* Declare local functions */
static void              query             (void);
static void              run               (const gchar            *name,
                                            gint                    nparams,
                                            const GimpParam        *param,
                                            gint                   *nreturn_vals,
                                            GimpParam             **return_vals);

static gint32            load_image        (PopplerDocument        *doc,
                                            const gchar            *filename,
                                            GimpRunMode             run_mode,
                                            GimpPageSelectorTarget  target,
                                            guint32                 resolution,
                                            PdfSelectedPages       *pages);

static gboolean          load_dialog       (PopplerDocument        *doc,
                                            PdfSelectedPages       *pages);

static PopplerDocument * open_document     (const gchar            *filename);

static GdkPixbuf *       get_thumbnail     (PopplerDocument        *doc,
                                            gint                    page,
                                            gint                    preferred_size);

static gint32            layer_from_pixbuf (gint32                  image,
                                            const gchar            *layer_name,
                                            gint                    position,
                                            GdkPixbuf              *buf,
                                            gdouble                 progress_start,
                                            gdouble                 progress_scale);

/**
 ** the following was formerly part of
 ** gimpresolutionentry.h and gimpresolutionentry.c,
 ** moved here because this is the only thing that uses
 ** it, and it is undesirable to maintain all that api.
 ** Most unused functions have been removed.
 **/
#define GIMP_TYPE_RESOLUTION_ENTRY            (gimp_resolution_entry_get_type ())
#define GIMP_RESOLUTION_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_RESOLUTION_ENTRY, GimpResolutionEntry))
#define GIMP_RESOLUTION_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_RESOLUTION_ENTRY, GimpResolutionEntryClass))
#define GIMP_IS_RESOLUTION_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_RESOLUTION_ENTRY))
#define GIMP_IS_RESOLUTION_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_RESOLUTION_ENTRY))
#define GIMP_RESOLUTION_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_RESOLUTION_ENTRY, GimpResolutionEntryClass))


typedef struct _GimpResolutionEntryClass  GimpResolutionEntryClass;

typedef struct _GimpResolutionEntryField  GimpResolutionEntryField;

struct _GimpResolutionEntryField
{
  GimpResolutionEntry      *gre;
  GimpResolutionEntryField *corresponding;

  gboolean       size;

  GtkWidget     *label;

  guint          changed_signal;

  GtkObject     *adjustment;
  GtkWidget     *spinbutton;

  gdouble        phy_size;

  gdouble        value;
  gdouble        min_value;
  gdouble        max_value;

  gint           stop_recursion;
};


struct _GimpResolutionEntry
{
  GtkTable                  parent_instance;

  GimpUnit                  size_unit;
  GimpUnit                  unit;

  GtkWidget                *unitmenu;
  GtkWidget                *chainbutton;

  GimpResolutionEntryField  width;
  GimpResolutionEntryField  height;
  GimpResolutionEntryField  x;
  GimpResolutionEntryField  y;

};

struct _GimpResolutionEntryClass
{
  GtkTableClass  parent_class;

  void (* value_changed)  (GimpResolutionEntry *gse);
  void (* refval_changed) (GimpResolutionEntry *gse);
  void (* unit_changed)   (GimpResolutionEntry *gse);
};


GType       gimp_resolution_entry_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_resolution_entry_new          (const gchar   *width_label,
                                                gdouble        width,
                                                const gchar   *height_label,
                                                gdouble        height,
                                                GimpUnit       size_unit,
                                                const gchar   *res_label,
                                                gdouble        initial_res,
                                                GimpUnit       initial_unit);

GtkWidget * gimp_resolution_entry_attach_label (GimpResolutionEntry *gre,
                                                const gchar         *text,
                                                gint                 row,
                                                gint                 column,
                                                gfloat               alignment);

gdouble     gimp_resolution_entry_get_x_in_dpi (GimpResolutionEntry *gre);

gdouble     gimp_resolution_entry_get_y_in_dpi (GimpResolutionEntry *gre);


/* signal callback convenience functions */
void        gimp_resolution_entry_update_x_in_dpi (GimpResolutionEntry *gre,
                                                   gpointer             data);

void        gimp_resolution_entry_update_y_in_dpi (GimpResolutionEntry *gre,
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

static void   gimp_resolution_entry_class_init      (GimpResolutionEntryClass *class);
static void   gimp_resolution_entry_init            (GimpResolutionEntry      *gre);

static void   gimp_resolution_entry_update_value    (GimpResolutionEntryField *gref,
                                                     gdouble              value);
static void   gimp_resolution_entry_value_callback  (GtkWidget           *widget,
                                                     gpointer             data);
static void   gimp_resolution_entry_update_unit     (GimpResolutionEntry *gre,
                                                     GimpUnit             unit);
static void   gimp_resolution_entry_unit_callback   (GtkWidget           *widget,
                                                     GimpResolutionEntry *gre);

static void   gimp_resolution_entry_field_init (GimpResolutionEntry      *gre,
                                                GimpResolutionEntryField *gref,
                                                GimpResolutionEntryField *corresponding,
                                                guint                     changed_signal,
                                                gdouble                   initial_val,
                                                GimpUnit                  initial_unit,
                                                gboolean                  size,
                                                gint                      spinbutton_width);

static void   gimp_resolution_entry_format_label (GimpResolutionEntry *gre,
                                                  GtkWidget           *label,
                                                  gdouble              size);

/**
 ** end of gimpresolutionentry stuff
 ** the actual code can be found at the end of this file
 **/
const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,     "run-mode",     "Interactive, non-interactive"     },
    { GIMP_PDB_STRING,    "filename",     "The name of the file to load"     },
    { GIMP_PDB_STRING,    "raw-filename", "The name entered"                 },
    { GIMP_PDB_INT32,     "resolution",   "Resolution to rasterize to (dpi)" },
    { GIMP_PDB_INT32,     "n-pages",      "Number of pages to load (0 for all)" },
    { GIMP_PDB_INT32ARRAY,"pages",        "The pages to load"                }
  };

  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,     "image",        "Output image" }
  };

  static const GimpParamDef thumb_args[] =
  {
    { GIMP_PDB_STRING,    "filename",     "The name of the file to load"  },
    { GIMP_PDB_INT32,     "thumb-size",   "Preferred thumbnail size"      }
  };

  static const GimpParamDef thumb_return_vals[] =
  {
    { GIMP_PDB_IMAGE,  "image",        "Thumbnail image"               },
    { GIMP_PDB_INT32,  "image-width",  "Width of full-sized image"     },
    { GIMP_PDB_INT32,  "image-height", "Height of full-sized image"    }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Load file in PDF format.",
                          "Load file in PDF format. "
                          "PDF is a portable document format created by Adobe. "
                          "It is designed to be easily processed by a variety "
                          "of different platforms, and is a distant cousin of "
                          "postscript. ",
                          "Nathan Summers",
                          "Nathan Summers",
                          "2005",
                          N_("Portable Document Format"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "application/pdf");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "pdf",
                                    "",
                                    "0, string,%PDF-");

  gimp_install_procedure (LOAD_THUMB_PROC,
                          "Loads a preview from a PDF file.",
                          "Loads a small preview of the first page of the PDF "
                          "format file. Uses the embedded thumbnail if "
                          "present.",
                          "Nathan Summers",
                          "Nathan Summers",
                          "2005",
                          NULL,
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (thumb_args),
                          G_N_ELEMENTS (thumb_return_vals),
                          thumb_args, thumb_return_vals);

  gimp_register_thumbnail_loader (LOAD_PROC, LOAD_THUMB_PROC);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;
  GimpPDBStatusType status   = GIMP_PDB_SUCCESS;
  gint32            image_ID = -1;
  PopplerDocument  *doc      = NULL;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (! g_thread_supported ())
    g_thread_init (NULL);

  if (strcmp (name, LOAD_PROC) == 0)
    {
      PdfSelectedPages *pages = g_new (PdfSelectedPages, 1);

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /* Possibly retrieve last settings */
          gimp_get_data (LOAD_PROC, &loadvals);

          doc = open_document (param[1].data.d_string);

          if (!doc)
            {
              status = GIMP_PDB_EXECUTION_ERROR;
              break;
            }

          if (load_dialog (doc, pages))
            gimp_set_data (LOAD_PROC, &loadvals, sizeof(loadvals));
          else
            status = GIMP_PDB_CANCEL;
          break;

	case GIMP_RUN_WITH_LAST_VALS:
        case GIMP_RUN_NONINTERACTIVE:
          /* FIXME: implement non-interactive mode */
          status = GIMP_PDB_EXECUTION_ERROR;
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          image_ID = load_image (doc, param[1].data.d_string,
                                 run_mode,
                                 loadvals.target,
                                 loadvals.resolution,
                                 pages);

          if (image_ID != -1)
            {
              *nreturn_vals = 2;
              values[1].type         = GIMP_PDB_IMAGE;
              values[1].data.d_image = image_ID;
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }

      if (doc)
        g_object_unref (doc);

      g_free (pages->pages);
      g_free (pages);
    }
  else if (strcmp (name, LOAD_THUMB_PROC) == 0)
    {
      if (nparams < 2)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          gdouble      width  = 0;
          gdouble      height = 0;
          gdouble      scale;
          gint32       image  = -1;
          GdkPixbuf   *pixbuf = NULL;

          /* Possibly retrieve last settings */
          gimp_get_data (LOAD_PROC, &loadvals);

          doc = open_document (param[0].data.d_string);

          if (doc)
            {
              PopplerPage *page = poppler_document_get_page (doc, 0);

              if (page)
                {
                  poppler_page_get_size (page, &width, &height);

                  g_object_unref (page);
                }

              pixbuf = get_thumbnail (doc, 0, param[1].data.d_int32);
              g_object_unref (doc);
            }

          if (pixbuf)
            {
              image = gimp_image_new (gdk_pixbuf_get_width  (pixbuf),
                                      gdk_pixbuf_get_height (pixbuf),
                                      GIMP_RGB);

              gimp_image_undo_disable (image);

              layer_from_pixbuf (image, "thumbnail", 0, pixbuf, 0.0, 1.0);
              g_object_unref (pixbuf);

              gimp_image_undo_enable (image);
              gimp_image_clean_all (image);
            }

          scale = loadvals.resolution / gimp_unit_get_factor (GIMP_UNIT_POINT);

          width  *= scale;
          height *= scale;

          if (image != -1)
            {
	      *nreturn_vals = 4;

              values[1].type         = GIMP_PDB_IMAGE;
	      values[1].data.d_image = image;
	      values[2].type         = GIMP_PDB_INT32;
	      values[2].data.d_int32 = width;
	      values[3].type         = GIMP_PDB_INT32;
	      values[3].data.d_int32 = height;
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }

    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static PopplerDocument*
open_document (const gchar *filename)
{
  PopplerDocument *doc;
  GError          *err = NULL;
  gchar           *uri;

  uri = g_filename_to_uri (filename, NULL, &err);

  if (err)
    {
      g_warning ("Could not convert '%s' to an URI: %s",
                 gimp_filename_to_utf8 (filename),
                 err->message);

      return NULL;
    }

  doc = poppler_document_new_from_file (uri, NULL, &err);

  g_free (uri);

  if (err)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename),
                 err->message);

      return NULL;
    }

  return doc;
}

static gint32
layer_from_pixbuf (gint32        image,
                   const gchar  *layer_name,
                   gint          position,
                   GdkPixbuf    *pixbuf,
                   gdouble       progress_start,
                   gdouble       progress_scale)
{
  gint32 layer = gimp_layer_new_from_pixbuf (image, layer_name, pixbuf,
                                             100.0, GIMP_NORMAL_MODE,
                                             progress_start,
                                             progress_start + progress_scale);

  gimp_image_add_layer (image, layer, position);

  return layer;
}

static gint32
load_image (PopplerDocument        *doc,
            const gchar            *filename,
            GimpRunMode             run_mode,
            GimpPageSelectorTarget  target,
            guint32                 resolution,
            PdfSelectedPages       *pages)
{
  gint32   image_ID = 0;
  gint32  *images   = NULL;
  gint     i;
  gdouble  scale;
  gdouble  doc_progress = 0;

  if (target == GIMP_PAGE_SELECTOR_TARGET_IMAGES)
    images = g_new0 (gint32, pages->n_pages);

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  scale = resolution / gimp_unit_get_factor (GIMP_UNIT_POINT);

  /* read the file */

  for (i = 0; i < pages->n_pages; i++)
    {
      PopplerPage *page;
      gchar       *page_label;
      gdouble      page_width;
      gdouble      page_height;

      GdkPixbuf   *buf;
      gint         width;
      gint         height;

      page = poppler_document_get_page (doc, pages->pages[i]);

      poppler_page_get_size (page, &page_width, &page_height);
      width  = page_width  * scale;
      height = page_height * scale;

      g_object_get (G_OBJECT (page), "label", &page_label, NULL);

      if (! image_ID)
        {
          gchar *name;

          image_ID = gimp_image_new (width, height, GIMP_RGB);
          gimp_image_undo_disable (image_ID);

          if (target == GIMP_PAGE_SELECTOR_TARGET_IMAGES)
            name = g_strdup_printf (_("%s-%s"), filename, page_label);
          else
            name = g_strdup_printf (_("%s-pages"), filename);

          gimp_image_set_filename (image_ID, name);
          g_free (name);

          gimp_image_set_resolution (image_ID, resolution, resolution);
        }

      buf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);

      poppler_page_render_to_pixbuf (page, 0, 0, width, height, scale, 0, buf);

      layer_from_pixbuf (image_ID, page_label, i, buf,
                         doc_progress, 1.0 / pages->n_pages);

      g_free (page_label);
      g_object_unref (buf);

      doc_progress = (double) (i + 1) / pages->n_pages;
      gimp_progress_update (doc_progress);

      if (target == GIMP_PAGE_SELECTOR_TARGET_IMAGES)
        {
          images[i] = image_ID;

          gimp_image_undo_enable (image_ID);
          gimp_image_clean_all (image_ID);

          image_ID = 0;
        }
   }

  if (image_ID)
    {
      gimp_image_undo_enable (image_ID);
      gimp_image_clean_all (image_ID);
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

      image_ID = images[0];

      g_free (images);
    }

  return image_ID;
}

static GdkPixbuf *
get_thumbnail (PopplerDocument *doc,
               gint             page_num,
               gint             preferred_size)
{
  PopplerPage *page;
  GdkPixbuf   *pixbuf;

  page = poppler_document_get_page (doc, page_num);

  if (! page)
    return NULL;

  pixbuf = poppler_page_get_thumbnail (page);

  if (! pixbuf)
    {
      gdouble width;
      gdouble height;
      gdouble scale;

      poppler_page_get_size (page, &width, &height);

      scale = (gdouble) preferred_size / MAX (width, height);

      width  *= scale;
      height *= scale;

      pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
                               width, height);

      poppler_page_render_to_pixbuf (page,
                                     0, 0, width, height, scale, 0, pixbuf);
    }

  g_object_unref (page);

  return pixbuf;
}

typedef struct
{
  PopplerDocument  *document;
  GimpPageSelector *selector;
  gboolean          stop_thumbnailing;
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
      idle_data->pixbuf = get_thumbnail (thread_data->document, i,
                                         THUMBNAIL_SIZE);

      g_idle_add (idle_set_thumbnail, idle_data);

      if (thread_data->stop_thumbnailing)
        break;
    }

  return NULL;
}

static gboolean
load_dialog (PopplerDocument  *doc,
             PdfSelectedPages *pages)
{
  GtkWidget  *dialog;
  GtkWidget  *vbox;
  GtkWidget  *title;
  GtkWidget  *selector;
  GtkWidget  *resolution;
  GtkWidget  *hbox;

  ThreadData  thread_data;
  GThread    *thread;

  gint        i;
  gint        n_pages;

  gdouble     width;
  gdouble     height;

  gboolean    run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Import from PDF"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, LOAD_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            _("_Import"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  /* Title */
  title = gimp_prop_label_new (G_OBJECT (doc), "title");
  gtk_box_pack_start (GTK_BOX (vbox), title, FALSE, FALSE, 0);
  gtk_widget_show (title);

  /* Page Selector */
  selector = gimp_page_selector_new ();
  gtk_widget_set_size_request (selector, 380, 360);
  gtk_box_pack_start (GTK_BOX (vbox), selector, TRUE, TRUE, 0);
  gtk_widget_show (selector);

  n_pages = poppler_document_get_n_pages (doc);
  gimp_page_selector_set_n_pages (GIMP_PAGE_SELECTOR (selector), n_pages);
  gimp_page_selector_set_target (GIMP_PAGE_SELECTOR (selector),
                                 loadvals.target);

  for (i = 0; i < n_pages; i++)
    {
      PopplerPage     *page;
      gchar           *label;

      page = poppler_document_get_page (doc, i);
      g_object_get (G_OBJECT (page), "label", &label, NULL);

      gimp_page_selector_set_page_label (GIMP_PAGE_SELECTOR (selector), i,
                                         label);

      if (i == 0)
        poppler_page_get_size (page, &width, &height);

      g_object_unref (page);
      g_free (label);
    }

  g_signal_connect_swapped (selector, "activate",
                            G_CALLBACK (gtk_window_activate_default),
                            dialog);

  thread_data.document          = doc;
  thread_data.selector          = GIMP_PAGE_SELECTOR (selector);
  thread_data.stop_thumbnailing = FALSE;

  thread = g_thread_create (thumbnail_thread, &thread_data, TRUE, NULL);

  /* Resolution */

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  resolution = gimp_resolution_entry_new (_("_Width (pixels):"), width,
                                          _("_Height (pixels):"), height,
                                          GIMP_UNIT_POINT,
                                          _("_Resolution:"),
                                          loadvals.resolution, GIMP_UNIT_INCH);

  gtk_box_pack_start (GTK_BOX (hbox), resolution, FALSE, FALSE, 0);
  gtk_widget_show (resolution);

  g_signal_connect (resolution, "x-changed",
                    G_CALLBACK (gimp_resolution_entry_update_x_in_dpi),
                    &loadvals.resolution);

  /* Setup done; display the dialog */
  gtk_widget_show (dialog);

  /* run the dialog */
  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  loadvals.target =
    gimp_page_selector_get_target (GIMP_PAGE_SELECTOR (selector));

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
  thread_data.stop_thumbnailing = TRUE;
  g_thread_join (thread);

  return run;
}


/**
 ** code for GimpResolutionEntry widget, formerly in libgimpwidgets
 **/

static guint gimp_resolution_entry_signals[LAST_SIGNAL] = { 0 };

static GtkTableClass *parent_class = NULL;


GType
gimp_resolution_entry_get_type (void)
{
  static GType gre_type = 0;

  if (! gre_type)
    {
      const GTypeInfo gre_info =
      {
        sizeof (GimpResolutionEntryClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_resolution_entry_class_init,
        NULL,                /* class_finalize */
        NULL,                /* class_data     */
        sizeof (GimpResolutionEntry),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_resolution_entry_init,
      };

      gre_type = g_type_register_static (GTK_TYPE_TABLE,
                                         "GimpResolutionEntry",
                                         &gre_info, 0);
    }

  return gre_type;
}

static void
gimp_resolution_entry_class_init (GimpResolutionEntryClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  gimp_resolution_entry_signals[HEIGHT_CHANGED] =
    g_signal_new ("height-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpResolutionEntryClass, value_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_resolution_entry_signals[WIDTH_CHANGED] =
    g_signal_new ("width-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpResolutionEntryClass, value_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_resolution_entry_signals[X_CHANGED] =
    g_signal_new ("x-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpResolutionEntryClass, value_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_resolution_entry_signals[Y_CHANGED] =
    g_signal_new ("y-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpResolutionEntryClass, refval_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_resolution_entry_signals[UNIT_CHANGED] =
    g_signal_new ("unit-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpResolutionEntryClass, unit_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  klass->value_changed  = NULL;
  klass->refval_changed = NULL;
  klass->unit_changed   = NULL;
}

static void
gimp_resolution_entry_init (GimpResolutionEntry *gre)
{
  gre->unitmenu = NULL;
  gre->unit     = GIMP_UNIT_INCH;

  gtk_table_set_col_spacings (GTK_TABLE (gre), 4);
  gtk_table_set_row_spacings (GTK_TABLE (gre), 2);
}

static void
gimp_resolution_entry_field_init (GimpResolutionEntry      *gre,
                                  GimpResolutionEntryField *gref,
                                  GimpResolutionEntryField *corresponding,
                                  guint                     changed_signal,
                                  gdouble                   initial_val,
                                  GimpUnit                  initial_unit,
                                  gboolean                  size,
                                  gint                      spinbutton_width)
{
  gint digits;

  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  gref->gre               = gre;
  gref->corresponding     = corresponding;
  gref->changed_signal    = gimp_resolution_entry_signals[changed_signal];

  if (size)
    {
      gref->value         = initial_val /
                            gimp_unit_get_factor (initial_unit) *
                            corresponding->value *
                            gimp_unit_get_factor (gre->unit);

      gref->phy_size      = initial_val /
                            gimp_unit_get_factor (initial_unit);
    }
  else
    {
      gref->value         = initial_val;
    }

  gref->min_value         = GIMP_MIN_RESOLUTION;
  gref->max_value         = GIMP_MAX_RESOLUTION;
  gref->adjustment        = NULL;

  gref->stop_recursion    = 0;

  gref->size              = size;

  if (size)
    {
      gref->label = g_object_new (GTK_TYPE_LABEL,
                                  "xalign", 0.0,
                                  "yalign", 0.5,
                                  NULL);
      gimp_label_set_attributes (GTK_LABEL (gref->label),
                                 PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                                 -1);

      gimp_resolution_entry_format_label (gre, gref->label, gref->phy_size);
    }

  digits = size ? 0 : MIN (gimp_unit_get_digits (initial_unit), 5) + 1;

  gref->spinbutton = gimp_spin_button_new (&gref->adjustment,
                                            gref->value,
                                            gref->min_value,
                                            gref->max_value,
                                            1.0, 10.0, 0.0,
                                            1.0,
                                            digits);


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
 * gimp_resolution_entry_new:
 * @width_label:       Optional label for the width control.
 * @width:             Width of the item, specified in terms of @size_unit.
 * @height_label:      Optional label for the height control.
 * @height:            Height of the item, specified in terms of @size_unit.
 * @size_unit:         Unit used to specify the width and height.
 * @res_label:         Optional label for the resolution entry.
 * @initial_res:       The initial resolution.
 * @initial_unit:      The initial unit.
 *
 * Creates a new #GimpResolutionEntry widget.
 *
 * The #GimpResolutionEntry is derived from #GtkTable and will have
 * an empty border of one cell width on each side plus an empty column left
 * of the #GimpUnitMenu to allow the caller to add labels or other widgets.
 *
 * A #GimpChainButton is displayed if independent is set to %TRUE.
 *
 * Returns: A pointer to the new #GimpResolutionEntry widget.
 **/
GtkWidget *
gimp_resolution_entry_new (const gchar *width_label,
                           gdouble      width,
                           const gchar *height_label,
                           gdouble      height,
                           GimpUnit     size_unit,
                           const gchar *res_label,
                           gdouble      initial_res,
                           GimpUnit     initial_unit)
{
  GimpResolutionEntry *gre;

  gre = g_object_new (GIMP_TYPE_RESOLUTION_ENTRY, NULL);

  gre->unit = initial_unit;

  gtk_table_resize (GTK_TABLE (gre), 4, 4);

  gimp_resolution_entry_field_init (gre, &gre->x,
                                    &gre->width,
                                    X_CHANGED,
                                    initial_res, initial_unit,
                                    FALSE, 0);

  gtk_table_attach_defaults (GTK_TABLE (gre), gre->x.spinbutton,
                             1, 2,
                             3, 4);

  g_signal_connect (gre->x.adjustment, "value-changed",
                    G_CALLBACK (gimp_resolution_entry_value_callback),
                    &gre->x);

  gtk_widget_show (gre->x.spinbutton);

  gre->unitmenu = gimp_unit_menu_new (_("pixels/%s"), initial_unit,
                                      FALSE, FALSE,
                                      TRUE);
  gtk_table_attach (GTK_TABLE (gre), gre->unitmenu,
                    3, 4, 3, 4,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  g_signal_connect (gre->unitmenu, "unit-changed",
                    G_CALLBACK (gimp_resolution_entry_unit_callback),
                    gre);
  gtk_widget_show (gre->unitmenu);

  gimp_resolution_entry_field_init (gre, &gre->width,
                                    &gre->x,
                                    WIDTH_CHANGED,
                                    width, size_unit,
                                    TRUE, 0);

  gtk_table_attach_defaults (GTK_TABLE (gre), gre->width.spinbutton,
                             1, 2,
                             1, 2);

  gtk_table_attach_defaults (GTK_TABLE (gre), gre->width.label,
                             3, 4,
                             1, 2);

  g_signal_connect (gre->width.adjustment, "value-changed",
                    G_CALLBACK (gimp_resolution_entry_value_callback),
                    &gre->width);

  gtk_widget_show (gre->width.spinbutton);
  gtk_widget_show (gre->width.label);

  gimp_resolution_entry_field_init (gre, &gre->height, &gre->x,
                                    HEIGHT_CHANGED,
                                    height, size_unit,
                                    TRUE, 0);

  gtk_table_attach_defaults (GTK_TABLE (gre), gre->height.spinbutton,
                             1, 2, 2, 3);

  gtk_table_attach_defaults (GTK_TABLE (gre), gre->height.label,
                             3, 4, 2, 3);

  g_signal_connect (gre->height.adjustment, "value-changed",
                    G_CALLBACK (gimp_resolution_entry_value_callback),
                    &gre->height);

  gtk_widget_show (gre->height.spinbutton);
  gtk_widget_show (gre->height.label);

  if (width_label)
    gimp_resolution_entry_attach_label (gre, width_label,  1, 0, 0.0);

  if (height_label)
    gimp_resolution_entry_attach_label (gre, height_label, 2, 0, 0.0);

  if (res_label)
    gimp_resolution_entry_attach_label (gre, res_label,    3, 0, 0.0);

  return GTK_WIDGET (gre);
}

/**
 * gimp_resolution_entry_attach_label:
 * @gre:       The #GimpResolutionEntry you want to add a label to.
 * @text:      The text of the label.
 * @row:       The row where the label will be attached.
 * @column:    The column where the label will be attached.
 * @alignment: The horizontal alignment of the label.
 *
 * Attaches a #GtkLabel to the #GimpResolutionEntry (which is a #GtkTable).
 *
 * Returns: A pointer to the new #GtkLabel widget.
 **/
GtkWidget *
gimp_resolution_entry_attach_label (GimpResolutionEntry *gre,
                                    const gchar         *text,
                                    gint                 row,
                                    gint                 column,
                                    gfloat               alignment)
{
  GtkWidget *label;

  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), NULL);
  g_return_val_if_fail (text != NULL, NULL);

  label = gtk_label_new_with_mnemonic (text);

  if (column == 0)
    {
      GtkTableChild *child;
      GList         *list;

      for (list = GTK_TABLE (gre)->children; list; list = g_list_next (list))
        {
          child = list->data;

          if (child->left_attach == 1 && child->top_attach == row)
            {
              gtk_label_set_mnemonic_widget (GTK_LABEL (label),
                                             child->widget);
              break;
            }
        }
    }

  gtk_misc_set_alignment (GTK_MISC (label), alignment, 0.5);

  gtk_table_attach (GTK_TABLE (gre), label, column, column+1, row, row+1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  return label;
}

/**
 * gimp_resolution_entry_get_x_in_dpi;
 * @gre:   The #GimpResolutionEntry you want to know the resolution of.
 *
 * Returns the X resolution of the #GimpResolutionEntry in pixels per inch.
 **/
gdouble
gimp_resolution_entry_get_x_in_dpi (GimpResolutionEntry *gre)
{
  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), 0);

  return gre->x.value / gimp_unit_get_factor (gre->unit);
}

/**
 * gimp_resolution_entry_get_y_in_dpi;
 * @gre:   The #GimpResolutionEntry you want to know the resolution of.
 *
 * Returns the Y resolution of the #GimpResolutionEntry in pixels per inch.
 **/
gdouble
gimp_resolution_entry_get_y_in_dpi (GimpResolutionEntry *gre)
{
  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), 0);

  return gre->y.value / gimp_unit_get_factor (gre->unit);
}


static void
gimp_resolution_entry_update_value (GimpResolutionEntryField *gref,
                                    gdouble                   value)
{
  if (gref->stop_recursion > 0)
    return;

  gref->value = value;

  gref->stop_recursion++;

  if (gref->size)
    gimp_resolution_entry_update_value (gref->corresponding,
                                        gref->value /
                                          gref->phy_size /
                                          gimp_unit_get_factor (gref->gre->unit));
  else
    {
      gdouble factor = gimp_unit_get_factor (gref->gre->unit);

      gimp_resolution_entry_update_value (&gref->gre->width,
                                          gref->value *
                                          gref->gre->width.phy_size *
                                          factor);

      gimp_resolution_entry_update_value (&gref->gre->height,
                                          gref->value *
                                          gref->gre->height.phy_size *
                                          factor);
    }

  gtk_adjustment_set_value (GTK_ADJUSTMENT (gref->adjustment), value);

  gref->stop_recursion--;

  g_signal_emit (gref->gre, gref->changed_signal, 0);
}

static void
gimp_resolution_entry_value_callback (GtkWidget *widget,
                                      gpointer   data)
{
  GimpResolutionEntryField *gref = (GimpResolutionEntryField *) data;
  gdouble                   new_value;

  new_value = GTK_ADJUSTMENT (widget)->value;

  if (gref->value != new_value)
    gimp_resolution_entry_update_value (gref, new_value);
}

static void
gimp_resolution_entry_update_unit (GimpResolutionEntry *gre,
                                   GimpUnit             unit)
{
  GimpUnit  old_unit;
  gint      digits;
  gdouble   factor;

  old_unit  = gre->unit;
  gre->unit = unit;

  digits = (gimp_unit_get_digits (GIMP_UNIT_INCH) -
            gimp_unit_get_digits (unit));

  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gre->x.spinbutton),
                              MAX (3 + digits, 3));

  factor = gimp_unit_get_factor (old_unit) / gimp_unit_get_factor (unit);

  gre->x.min_value *= factor;
  gre->x.max_value *= factor;
  gre->x.value     *= factor;

  gtk_adjustment_set_value (GTK_ADJUSTMENT (gre->x.adjustment),
                            gre->x.value);

  gimp_resolution_entry_format_label (gre,
                                      gre->width.label, gre->width.phy_size);
  gimp_resolution_entry_format_label (gre,
                                      gre->height.label, gre->height.phy_size);

  g_signal_emit (gre, gimp_resolution_entry_signals[UNIT_CHANGED], 0);
}

static void
gimp_resolution_entry_unit_callback (GtkWidget           *widget,
                                     GimpResolutionEntry *gre)
{
  GimpUnit new_unit;

  new_unit = gimp_unit_menu_get_unit (GIMP_UNIT_MENU (widget));

  if (gre->unit != new_unit)
    gimp_resolution_entry_update_unit (gre, new_unit);
}

/**
 * gimp_resolution_entry_update_x_in_dpi:
 * @gre: the #GimpResolutionEntry
 * @data: a pointer to a gdouble
 *
 * Convenience function to set a double to the X resolution, suitable
 * for use as a signal callback.
 */
void
gimp_resolution_entry_update_x_in_dpi (GimpResolutionEntry *gre,
                                       gpointer             data)
{
  gdouble *val;

  g_return_if_fail (gre  != NULL);
  g_return_if_fail (data != NULL);
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  val = (gdouble *) data;

  *val = gimp_resolution_entry_get_x_in_dpi (gre);
}

/**
 * gimp_resolution_entry_update_y_in_dpi:
 * @gre: the #GimpResolutionEntry
 * @data: a pointer to a gdouble
 *
 * Convenience function to set a double to the Y resolution, suitable
 * for use as a signal callback.
 */
void
gimp_resolution_entry_update_y_in_dpi (GimpResolutionEntry *gre,
                                       gpointer             data)
{
  gdouble *val;

  g_return_if_fail (gre  != NULL);
  g_return_if_fail (data != NULL);
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  val = (gdouble *) data;

  *val = gimp_resolution_entry_get_y_in_dpi (gre);
}

static void
gimp_resolution_entry_format_label (GimpResolutionEntry *gre,
                                    GtkWidget           *label,
                                    gdouble              size)
{
  gchar *format = g_strdup_printf ("%%.%df %%s",
                                   gimp_unit_get_digits (gre->unit));
  gchar *text = g_strdup_printf (format,
                                 size * gimp_unit_get_factor (gre->unit),
                                 gimp_unit_get_plural (gre->unit));
  g_free (format);

  gtk_label_set_text (GTK_LABEL (label), text);
  g_free (text);
}
