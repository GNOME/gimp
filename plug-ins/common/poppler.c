/* The GIMP -- an image manipulation program
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
  gboolean               antialias;
} PdfLoadVals;

static PdfLoadVals loadvals =
{
  GIMP_PAGE_SELECTOR_TARGET_LAYERS,
  100.00, /* 100 dpi   */
  TRUE    /* antialias */
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
                                            gboolean                antialias,
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

GimpPlugInInfo PLUG_IN_INFO =
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
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,     "run-mode",     "Interactive, non-interactive"     },
    { GIMP_PDB_STRING,    "filename",     "The name of the file to load"     },
    { GIMP_PDB_STRING,    "raw-filename", "The name entered"                 },
    { GIMP_PDB_INT32,     "resolution",   "Resolution to rasterize to (dpi)" },
    { GIMP_PDB_INT32,     "antialias",    "Whether to antialias"             },
    { GIMP_PDB_INT32,     "n-pages",      "Number of pages to load (0 for all)" },
    { GIMP_PDB_INT32ARRAY,"page",         "The pages to load"                }
  };

  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,    "image",         "Output image" }
  };

  static GimpParamDef thumb_args[] =
  {
    { GIMP_PDB_STRING, "filename",     "The name of the file to load"  },
    { GIMP_PDB_INT32,  "thumb-size",   "Preferred thumbnail size"      }
  };

  static GimpParamDef thumb_return_vals[] =
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
                                 loadvals.antialias,
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
          gdouble      width    = 0;
          gdouble      height   = 0;
          gdouble      scale;
          gint32       image    = -1;
          GdkPixbuf   *buf      = NULL;

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

              buf = get_thumbnail (doc, 0, param[1].data.d_int32);
            }

          if (buf)
            {
              image = gimp_image_new (gdk_pixbuf_get_width  (buf),
                                      gdk_pixbuf_get_height (buf),
                                      GIMP_RGB);

              gimp_image_undo_disable (image);
              layer_from_pixbuf (image, "thumbnail", 0, buf, 0.0, 1.0);
              gimp_image_undo_enable (image);
              gimp_image_clean_all (image);
            }


          scale = loadvals.resolution /
                  gimp_unit_get_factor (GIMP_UNIT_POINT);

          width  *= scale;
          height *= scale;

          if (doc)
            g_object_unref (doc);

          if (buf)
            g_object_unref (buf);

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
                   GdkPixbuf    *buf,
                   gdouble       progress_start,
                   gdouble       progress_scale)
{
  gint32           layer;
  GimpPixelRgn     rgn;
  gpointer         pr;
  GimpDrawable    *drawable;

  gint             width;
  gint             height;
  gint             rowstride;
  gint             bpp;
  guchar          *pixels;

  gdouble          progress = 0;

  g_return_val_if_fail (buf != NULL, -1);

  width  = gdk_pixbuf_get_width  (buf);
  height = gdk_pixbuf_get_height (buf);

  rowstride = gdk_pixbuf_get_rowstride  (buf);
  bpp       = gdk_pixbuf_get_n_channels (buf);
  pixels    = gdk_pixbuf_get_pixels     (buf);

  layer = gimp_layer_new (image, layer_name,
                          width, height,
                          GIMP_RGB_IMAGE, 100.0, GIMP_NORMAL_MODE);

  gimp_image_add_layer (image, layer, position);

  drawable = gimp_drawable_get (layer);

  gimp_pixel_rgn_init (&rgn, drawable, 0, 0, width, height, TRUE, FALSE);

  for (pr = gimp_pixel_rgns_register (1, &rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      const guchar *src;
      guchar       *dest;
      gint          y;

      src  = pixels + rgn.y * rowstride + rgn.x * bpp;
      dest = rgn.data;

      for (y = 0; y < rgn.h; y++)
        {
          memcpy (dest, src, rgn.w * rgn.bpp);

          src  += rowstride;
          dest += rgn.rowstride;
        }

      progress += (double) (rgn.h * rgn.w) / (height * width);

      gimp_progress_update (progress_start + progress * progress_scale);
    }

  gimp_drawable_detach (drawable);

  return layer;
}

static gint32
load_image (PopplerDocument        *doc,
            const gchar            *filename,
            GimpRunMode             run_mode,
            GimpPageSelectorTarget  target,
            guint32                 resolution,
            gboolean                antialias,
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

#if 0
  poppler_set_antialias (antialias);
#endif

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

      poppler_page_render_to_pixbuf (page, 0, 0,
                                     width, height,
                                     scale,
#ifdef HAVE_POPPLER_0_4_1
                                     0,
#endif
                                     buf
#ifndef HAVE_POPPLER_0_4
                                     , 0, 0
#endif
                                     );

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
      double width, height, scale;

      poppler_page_get_size (page, &width, &height);

      scale = (double) preferred_size / MAX (width, height);

      width  *= scale;
      height *= scale;

      pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
                               width, height);

      poppler_page_render_to_pixbuf (page, 0, 0,
                                     width, height,
                                     scale,
#ifdef HAVE_POPPLER_0_4_1
                                     0,
#endif
                                     pixbuf
#ifndef HAVE_POPPLER_0_4
                                     ,0, 0
#endif
                                     );
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

/* user interface for size / resolution entry */

static GimpSizeEntry *size       = NULL;
static GtkObject     *xadj       = NULL;
static GtkObject     *yadj       = NULL;
static GtkWidget     *constrain  = NULL;
static gdouble        ratio_x    = 1.0;
static gdouble        ratio_y    = 1.0;
static gint           pdf_width  = 0;
static gint           pdf_height = 0;

static void  load_dialog_set_ratio (gdouble x,
                                    gdouble y);


static void
load_dialog_size_callback (GtkWidget *widget,
                           gpointer   data)
{
  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (constrain)))
    {
      gdouble x = gimp_size_entry_get_refval (size, 0) / (gdouble) pdf_width;
      gdouble y = gimp_size_entry_get_refval (size, 1) / (gdouble) pdf_height;

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
  gdouble x = gtk_adjustment_get_value (GTK_ADJUSTMENT (xadj));
  gdouble y = gtk_adjustment_get_value (GTK_ADJUSTMENT (yadj));

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
load_dialog_resolution_callback (GimpSizeEntry   *res,
                                 PopplerDocument *doc)
{
  PdfLoadVals  vals = { 0.0, 0, 0 };

  loadvals.resolution = vals.resolution = gimp_size_entry_get_refval (res, 0);

#if 0
  if (!load_rpdf_size (filename, &vals, NULL))
    return;

  pdf_width  = vals.width;
  pdf_height = vals.height;

#endif

  load_dialog_set_ratio (ratio_x, ratio_y);
}

static void
load_dialog_set_ratio (gdouble x,
                       gdouble y)
{
  ratio_x = x;
  ratio_y = y;

  g_signal_handlers_block_by_func (size, load_dialog_size_callback, NULL);

  gimp_size_entry_set_refval (size, 0, pdf_width  * x);
  gimp_size_entry_set_refval (size, 1, pdf_height * y);

  g_signal_handlers_unblock_by_func (size, load_dialog_size_callback, NULL);

  g_signal_handlers_block_by_func (xadj, load_dialog_ratio_callback, NULL);
  g_signal_handlers_block_by_func (yadj, load_dialog_ratio_callback, NULL);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (xadj), x);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (yadj), y);

  g_signal_handlers_unblock_by_func (xadj, load_dialog_ratio_callback, NULL);
  g_signal_handlers_unblock_by_func (yadj, load_dialog_ratio_callback, NULL);
}

static gboolean
load_dialog (PopplerDocument  *doc,
             PdfSelectedPages *pages)
{
  GtkWidget  *dialog;
  GtkWidget  *vbox;
  GtkWidget  *label;
  GtkWidget  *selector;
  GtkWidget  *toggle;
  GtkWidget  *hbox;
  GtkWidget  *table;
  GtkWidget  *table2;
  GtkWidget  *spinbutton;
  GtkWidget  *res;
  GtkObject  *adj;

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
  label = gimp_prop_label_new (G_OBJECT (doc), "title");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* Page Selector */
  selector = gimp_page_selector_new ();
  gtk_box_pack_start (GTK_BOX (vbox), selector, TRUE, TRUE, 0);
  n_pages = poppler_document_get_n_pages (doc);
  gimp_page_selector_set_n_pages (GIMP_PAGE_SELECTOR (selector), n_pages);

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

  gtk_widget_show (selector);

  thread_data.document          = doc;
  thread_data.selector          = GIMP_PAGE_SELECTOR (selector);
  thread_data.stop_thumbnailing = FALSE;

  thread = g_thread_create (thumbnail_thread, &thread_data, TRUE, NULL);

  /* Resolution */
#if 0
  pdf_width  = loadvals.width;
  pdf_height = loadvals.height;
#endif

  table = gtk_table_new (5, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  Width and Height  */
  label = gtk_label_new (_("Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  spinbutton = gimp_spin_button_new (&adj, 1, 1, 1, 1, 10, 1, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 1, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  size = GIMP_SIZE_ENTRY (gimp_size_entry_new (1, GIMP_UNIT_PIXEL, "%a",
                                               TRUE, FALSE, FALSE, 10,
                                               GIMP_SIZE_ENTRY_UPDATE_SIZE));
  gtk_table_set_col_spacing (GTK_TABLE (size), 1, 6);

  gimp_size_entry_add_field (size, GTK_SPIN_BUTTON (spinbutton), NULL);

  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (size), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (size));

  gimp_size_entry_set_refval_boundaries (size, 0,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (size, 1,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_refval (size, 0, pdf_width);
  gimp_size_entry_set_refval (size, 1, pdf_height);

  gimp_size_entry_set_resolution (size, 0, loadvals.resolution, FALSE);
  gimp_size_entry_set_resolution (size, 1, loadvals.resolution, FALSE);

  g_signal_connect (size, "value-changed",
		    G_CALLBACK (load_dialog_size_callback),
                    NULL);

  /*  Scale ratio  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 2, 4,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  table2 = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table2), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table2), 0, 4);
  gtk_box_pack_start (GTK_BOX (hbox), table2, FALSE, FALSE, 0);

  spinbutton =
    gimp_spin_button_new (&xadj,
                          ratio_x,
                          (gdouble) GIMP_MIN_IMAGE_SIZE / (gdouble) pdf_width,
                          (gdouble) GIMP_MAX_IMAGE_SIZE / (gdouble) pdf_width,
                          0.01, 0.1, 1,
                          0.01, 4);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);
  gtk_table_attach_defaults (GTK_TABLE (table2), spinbutton, 0, 1, 0, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (xadj, "value-changed",
		    G_CALLBACK (load_dialog_ratio_callback),
		    NULL);

  label = gtk_label_new_with_mnemonic (_("_X ratio:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  spinbutton =
    gimp_spin_button_new (&yadj,
                          ratio_y,
                          (gdouble) GIMP_MIN_IMAGE_SIZE / (gdouble) pdf_height,
                          (gdouble) GIMP_MAX_IMAGE_SIZE / (gdouble) pdf_height,
                          0.01, 0.1, 1,
                          0.01, 4);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);
  gtk_table_attach_defaults (GTK_TABLE (table2), spinbutton, 0, 1, 1, 2);
  gtk_widget_show (spinbutton);

  g_signal_connect (yadj, "value-changed",
		    G_CALLBACK (load_dialog_ratio_callback),
		    NULL);

  label = gtk_label_new_with_mnemonic (_("_Y ratio:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  the constrain ratio chainbutton  */
  constrain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (constrain), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table2), constrain, 1, 2, 0, 2);
  gtk_widget_show (constrain);

  gimp_help_set_help_data (GIMP_CHAIN_BUTTON (constrain)->button,
                           _("Constrain aspect ratio"), NULL);

  gtk_widget_show (table2);

  /*  Resolution   */
  label = gtk_label_new (_("Resolution:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  res = gimp_size_entry_new (1, GIMP_UNIT_INCH, _("pixels/%a"),
                             FALSE, FALSE, FALSE, 10,
                             GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
  gtk_table_set_col_spacing (GTK_TABLE (res), 1, 6);

  gtk_table_attach (GTK_TABLE (table), res, 1, 2, 4, 5,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (res);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (res), 0,
                                         5.0, GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (res), 0, loadvals.resolution);

  g_signal_connect (res, "value-changed",
                    G_CALLBACK (load_dialog_resolution_callback),
                    doc);

  /* Antialiasing */
  toggle = gtk_check_button_new_with_mnemonic (_("A_ntialiasing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), loadvals.antialias);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &loadvals.antialias);

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
