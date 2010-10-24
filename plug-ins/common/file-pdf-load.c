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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
  100.00,  /* 100 dpi   */
  TRUE
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

static GimpPDBStatusType load_dialog       (PopplerDocument        *doc,
                                            PdfSelectedPages       *pages);

static PopplerDocument * open_document     (const gchar            *filename,
                                            GError                **error);

static cairo_surface_t * get_thumb_surface (PopplerDocument        *doc,
                                            gint                    page,
                                            gint                    preferred_size);

static GdkPixbuf *       get_thumb_pixbuf  (PopplerDocument        *doc,
                                            gint                    page,
                                            gint                    preferred_size);

static gint32            layer_from_surface (gint32                  image,
                                             const gchar            *layer_name,
                                             gint                    position,
                                             cairo_surface_t        *surface,
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


typedef struct _GimpResolutionEntry       GimpResolutionEntry;
typedef struct _GimpResolutionEntryClass  GimpResolutionEntryClass;

typedef struct _GimpResolutionEntryField  GimpResolutionEntryField;

struct _GimpResolutionEntryField
{
  GimpResolutionEntry      *gre;
  GimpResolutionEntryField *corresponding;

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
    { GIMP_PDB_INT32,     "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"     },
    { GIMP_PDB_STRING,    "filename",     "The name of the file to load"     },
    { GIMP_PDB_STRING,    "raw-filename", "The name entered"                 }
    /* XXX: Nice to have API at some point, but needs work
    { GIMP_PDB_INT32,     "resolution",   "Resolution to rasterize to (dpi)" },
    { GIMP_PDB_INT32,     "antialiasing", "Use anti-aliasing" },
    { GIMP_PDB_INT32,     "n-pages",      "Number of pages to load (0 for all)" },
    { GIMP_PDB_INT32ARRAY,"pages",        "The pages to load"                }
    */
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
    { GIMP_PDB_INT32,  "image-height", "Height of full-sized image"    },
    { GIMP_PDB_INT32,  "image-type",   "Image type"                    },
    { GIMP_PDB_INT32,  "num-layers",   "Number of pages"               }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Load file in PDF format",
                          "Loads files in Adobe's Portable Document Format. "
                          "PDF is designed to be easily processed by a variety "
                          "of different platforms, and is a distant cousin of "
                          "PostScript.",
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
  static GimpParam  values[6];
  GimpRunMode       run_mode;
  GimpPDBStatusType status   = GIMP_PDB_SUCCESS;
  gint32            image_ID = -1;
  PopplerDocument  *doc      = NULL;
  GError           *error    = NULL;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      PdfSelectedPages pages = { 0, NULL };

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /* Possibly retrieve last settings */
          gimp_get_data (LOAD_PROC, &loadvals);

          doc = open_document (param[1].data.d_string, &error);

          if (!doc)
            {
              status = GIMP_PDB_EXECUTION_ERROR;
              break;
            }

          status = load_dialog (doc, &pages);
          if (status == GIMP_PDB_SUCCESS)
            gimp_set_data (LOAD_PROC, &loadvals, sizeof(loadvals));
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /* FIXME: implement last vals mode */
          status = GIMP_PDB_EXECUTION_ERROR;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          doc = open_document (param[1].data.d_string, &error);

          if (doc)
            {
              PopplerPage *test_page = poppler_document_get_page (doc, 0);

              if (test_page)
                {
                  pages.n_pages = 1;
                  pages.pages = g_new (gint, 1);
                  pages.pages[0] = 0;

                  g_object_unref (test_page);
                }
              else
                {
                  status = GIMP_PDB_EXECUTION_ERROR;
                  g_object_unref (doc);
                }
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          image_ID = load_image (doc, param[1].data.d_string,
                                 run_mode,
                                 loadvals.target,
                                 loadvals.resolution,
                                 loadvals.antialias,
                                 &pages);

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

      g_free (pages.pages);
    }
  else if (strcmp (name, LOAD_THUMB_PROC) == 0)
    {
      if (nparams < 2)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          gdouble          width     = 0;
          gdouble          height    = 0;
          gdouble          scale;
          gint32           image     = -1;
          gint             num_pages = 0;
          cairo_surface_t *surface   = NULL;

          /* Possibly retrieve last settings */
          gimp_get_data (LOAD_PROC, &loadvals);

          doc = open_document (param[0].data.d_string, &error);

          if (doc)
            {
              PopplerPage *page = poppler_document_get_page (doc, 0);

              if (page)
                {
                  poppler_page_get_size (page, &width, &height);

                  g_object_unref (page);
                }

              num_pages = poppler_document_get_n_pages (doc);

              surface = get_thumb_surface (doc, 0, param[1].data.d_int32);

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

          scale = loadvals.resolution / gimp_unit_get_factor (GIMP_UNIT_POINT);

          width  *= scale;
          height *= scale;

          if (image != -1)
            {
              *nreturn_vals = 6;

              values[1].type         = GIMP_PDB_IMAGE;
              values[1].data.d_image = image;
              values[2].type         = GIMP_PDB_INT32;
              values[2].data.d_int32 = width;
              values[3].type         = GIMP_PDB_INT32;
              values[3].data.d_int32 = height;
              values[4].type         = GIMP_PDB_INT32;
              values[4].data.d_int32 = GIMP_RGB_IMAGE;
              values[5].type         = GIMP_PDB_INT32;
              values[5].data.d_int32 = num_pages;
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

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}

static PopplerDocument*
open_document (const gchar  *filename,
               GError      **load_error)
{
  PopplerDocument *doc;
  GMappedFile     *mapped_file;
  GError          *error = NULL;

  mapped_file = g_mapped_file_new (filename, FALSE, &error);

  if (! mapped_file)
    {
      g_set_error (load_error, 0, 0,
                   _("Could not load '%s': %s"),
                   gimp_filename_to_utf8 (filename), error->message);
      g_error_free (error);
      return NULL;
    }

  doc = poppler_document_new_from_data (g_mapped_file_get_contents (mapped_file),
                                        g_mapped_file_get_length (mapped_file),
                                        NULL,
                                        &error);

  /* We can't g_mapped_file_unref(mapped_file) as apparently doc has
   * references to data in there. No big deal, this is just a
   * short-lived plug-in.
   */

  if (! doc)
    {
      g_set_error (load_error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not load '%s': %s"),
                   gimp_filename_to_utf8 (filename),
                   error->message);
      g_error_free (error);
      return NULL;
    }

  return doc;
}

/* FIXME: Remove this someday when we depend fully on GTK+ >= 3 */

#if (!GTK_CHECK_VERSION (3, 0, 0))

static cairo_format_t
gdk_cairo_format_for_content (cairo_content_t content)
{
  switch (content)
    {
    case CAIRO_CONTENT_COLOR:
      return CAIRO_FORMAT_RGB24;
    case CAIRO_CONTENT_ALPHA:
      return CAIRO_FORMAT_A8;
    case CAIRO_CONTENT_COLOR_ALPHA:
    default:
      return CAIRO_FORMAT_ARGB32;
    }
}

static cairo_surface_t *
gdk_cairo_surface_coerce_to_image (cairo_surface_t *surface,
                                   cairo_content_t  content,
                                   int              src_x,
                                   int              src_y,
                                   int              width,
                                   int              height)
{
  cairo_surface_t *copy;
  cairo_t *cr;

  copy = cairo_image_surface_create (gdk_cairo_format_for_content (content),
                                     width,
                                     height);

  cr = cairo_create (copy);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface (cr, surface, -src_x, -src_y);
  cairo_paint (cr);
  cairo_destroy (cr);

  return copy;
}

static void
convert_alpha (guchar *dest_data,
               int     dest_stride,
               guchar *src_data,
               int     src_stride,
               int     src_x,
               int     src_y,
               int     width,
               int     height)
{
  int x, y;

  src_data += src_stride * src_y + src_x * 4;

  for (y = 0; y < height; y++) {
    guint32 *src = (guint32 *) src_data;

    for (x = 0; x < width; x++) {
      guint alpha = src[x] >> 24;

      if (alpha == 0)
        {
          dest_data[x * 4 + 0] = 0;
          dest_data[x * 4 + 1] = 0;
          dest_data[x * 4 + 2] = 0;
        }
      else
        {
          dest_data[x * 4 + 0] = (((src[x] & 0xff0000) >> 16) * 255 + alpha / 2) / alpha;
          dest_data[x * 4 + 1] = (((src[x] & 0x00ff00) >>  8) * 255 + alpha / 2) / alpha;
          dest_data[x * 4 + 2] = (((src[x] & 0x0000ff) >>  0) * 255 + alpha / 2) / alpha;
        }
      dest_data[x * 4 + 3] = alpha;
    }

    src_data += src_stride;
    dest_data += dest_stride;
  }
}

static void
convert_no_alpha (guchar *dest_data,
                  int     dest_stride,
                  guchar *src_data,
                  int     src_stride,
                  int     src_x,
                  int     src_y,
                  int     width,
                  int     height)
{
  int x, y;

  src_data += src_stride * src_y + src_x * 4;

  for (y = 0; y < height; y++) {
    guint32 *src = (guint32 *) src_data;

    for (x = 0; x < width; x++) {
      dest_data[x * 3 + 0] = src[x] >> 16;
      dest_data[x * 3 + 1] = src[x] >>  8;
      dest_data[x * 3 + 2] = src[x];
    }

    src_data += src_stride;
    dest_data += dest_stride;
  }
}

/**
 * gdk_pixbuf_get_from_surface:
 * @surface: surface to copy from
 * @src_x: Source X coordinate within @surface
 * @src_y: Source Y coordinate within @surface
 * @width: Width in pixels of region to get
 * @height: Height in pixels of region to get
 *
 * Transfers image data from a #cairo_surface_t and converts it to an RGB(A)
 * representation inside a #GdkPixbuf. This allows you to efficiently read
 * individual pixels from cairo surfaces. For #GdkWindows, use
 * gdk_pixbuf_get_from_window() instead.
 *
 * This function will create an RGB pixbuf with 8 bits per channel.
 * The pixbuf will contain an alpha channel if the @surface contains one.
 *
 * Return value: (transfer full): A newly-created pixbuf with a reference
 *     count of 1, or %NULL on error
 */
static GdkPixbuf *
gdk_pixbuf_get_from_surface  (cairo_surface_t *surface,
                              gint             src_x,
                              gint             src_y,
                              gint             width,
                              gint             height)
{
  cairo_content_t content;
  GdkPixbuf *dest;

  /* General sanity checks */
  g_return_val_if_fail (surface != NULL, NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  content = cairo_surface_get_content (surface) | CAIRO_CONTENT_COLOR;
  dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                         !!(content & CAIRO_CONTENT_ALPHA),
                         8,
                         width, height);

  surface = gdk_cairo_surface_coerce_to_image (surface, content,
                                               src_x, src_y,
                                               width, height);
  cairo_surface_flush (surface);
  if (cairo_surface_status (surface) || dest == NULL)
    {
      cairo_surface_destroy (surface);
      return NULL;
    }

  if (gdk_pixbuf_get_has_alpha (dest))
    convert_alpha (gdk_pixbuf_get_pixels (dest),
                   gdk_pixbuf_get_rowstride (dest),
                   cairo_image_surface_get_data (surface),
                   cairo_image_surface_get_stride (surface),
                   0, 0,
                   width, height);
  else
    convert_no_alpha (gdk_pixbuf_get_pixels (dest),
                      gdk_pixbuf_get_rowstride (dest),
                      cairo_image_surface_get_data (surface),
                      cairo_image_surface_get_stride (surface),
                      0, 0,
                      width, height);

  cairo_surface_destroy (surface);
  return dest;
}

#endif

static gint32
layer_from_surface (gint32           image,
                    const gchar     *layer_name,
                    gint             position,
                    cairo_surface_t *surface,
                    gdouble          progress_start,
                    gdouble          progress_scale)
{
  gint32 layer = gimp_layer_new_from_surface (image, layer_name, surface,
                                              progress_start,
                                              progress_start + progress_scale);

  gimp_image_insert_layer (image, layer, -1, position);

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

  for (i = 0; i < pages->n_pages; i++)
    {
      PopplerPage *page;
      gchar       *page_label;
      gdouble      page_width;
      gdouble      page_height;

      cairo_surface_t *surface;
      gint             width;
      gint             height;

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

      surface = render_page_to_surface (page, width, height, scale, antialias);

      layer_from_surface (image_ID, page_label, i, surface,
                          doc_progress, 1.0 / pages->n_pages);

      g_free (page_label);
      cairo_surface_destroy (surface);

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
  gimp_progress_update (1.0);

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
      idle_data->pixbuf = get_thumb_pixbuf (thread_data->document, i,
                                            THUMBNAIL_SIZE);

      g_idle_add (idle_set_thumbnail, idle_data);

      if (thread_data->stop_thumbnailing)
        break;
    }

  return NULL;
}

static GimpPDBStatusType
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

  ThreadData  thread_data;
  GThread    *thread;

  gint        i;
  gint        n_pages;

  gdouble     width;
  gdouble     height;

  gboolean    run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Import from PDF"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, LOAD_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_Import"), GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /* Title */
  title = gimp_prop_label_new (G_OBJECT (doc), "title");
  gtk_label_set_ellipsize (GTK_LABEL (title), PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (vbox), title, FALSE, FALSE, 0);
  gtk_widget_show (title);

  /* Page Selector */
  selector = gimp_page_selector_new ();
  gtk_widget_set_size_request (selector, 380, 360);
  gtk_box_pack_start (GTK_BOX (vbox), selector, TRUE, TRUE, 0);
  gtk_widget_show (selector);

  n_pages = poppler_document_get_n_pages (doc);

  if (n_pages <= 0)
    {
      g_message (_("Error getting number of pages from the given PDF file."));
      return GIMP_PDB_EXECUTION_ERROR;
    }

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

  thread = g_thread_new ("thumbnailer", thumbnail_thread, &thread_data);

  /* Resolution */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
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

  /* Antialiasing*/
  antialias = gtk_check_button_new_with_mnemonic (_("Use _Anti-aliasing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (antialias), loadvals.antialias);
  gtk_box_pack_start (GTK_BOX (vbox), antialias, FALSE, FALSE, 0);

  g_signal_connect (antialias, "toggled",
                    G_CALLBACK (gimp_toggle_button_update), &loadvals.antialias);
  gtk_widget_show (antialias);

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

  return run ? GIMP_PDB_SUCCESS : GIMP_PDB_CANCEL;
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

  gref->adjustment = (GtkAdjustment *)
    gtk_adjustment_new (gref->value,
                        gref->min_value,
                        gref->max_value,
                        1.0, 10.0, 0.0);
  gref->spinbutton = gtk_spin_button_new (gref->adjustment,
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
  GtkTreeModel        *model;

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

  gre->unitmenu = gimp_unit_combo_box_new ();
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (gre->unitmenu));
  gimp_unit_store_set_has_pixels (GIMP_UNIT_STORE (model), FALSE);
  gimp_unit_store_set_has_percent (GIMP_UNIT_STORE (model), FALSE);
  g_object_set (model,
                "short-format", _("pixels/%a"),
                "long-format",  _("pixels/%a"),
                NULL);
  gimp_unit_combo_box_set_active (GIMP_UNIT_COMBO_BOX (gre->unitmenu),
                                  initial_unit);

  gtk_table_attach (GTK_TABLE (gre), gre->unitmenu,
                    3, 4, 3, 4,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  g_signal_connect (gre->unitmenu, "changed",
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

  /* dots_in_one_unit * units_in_one_inch -> dpi */
  return gre->x.value * gimp_unit_get_factor (gre->unit);
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

  return gre->y.value * gimp_unit_get_factor (gre->unit);
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

  gtk_adjustment_set_value (gref->adjustment, value);

  gref->stop_recursion--;

  g_signal_emit (gref->gre, gref->changed_signal, 0);
}

static void
gimp_resolution_entry_value_callback (GtkWidget *widget,
                                      gpointer   data)
{
  GimpResolutionEntryField *gref = (GimpResolutionEntryField *) data;
  gdouble                   new_value;

  new_value = gtk_adjustment_get_value (GTK_ADJUSTMENT (widget));

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

  gtk_adjustment_set_value (gre->x.adjustment, gre->x.value);

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

  new_unit = gimp_unit_combo_box_get_active (GIMP_UNIT_COMBO_BOX (widget));

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
