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

/* WMF loading file filter for GIMP
 * -Dom Lachowicz <cinamod@hotmail.com> 2003
 * -Francis James Franklin <fjf@alinameridon.com>
 */

#include "config.h"

#include <libwmf/api.h>
#include <libwmf/gd.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimpmath/gimpmath.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC               "file-wmf-load"
#define LOAD_THUMB_PROC         "file-wmf-load-thumb"
#define PLUG_IN_BINARY          "file-wmf"
#define PLUG_IN_ROLE            "gimp-file-wmf"

#define WMF_DEFAULT_RESOLUTION  300.0
#define WMF_DEFAULT_SIZE        500.0
#define WMF_PREVIEW_SIZE        128


typedef struct
{
  gdouble    resolution;
  gint       width;
  gint       height;
} WmfLoadVals;


typedef struct _Wmf      Wmf;
typedef struct _WmfClass WmfClass;

struct _Wmf
{
  GimpPlugIn      parent_instance;
};

struct _WmfClass
{
  GimpPlugInClass parent_class;
};

typedef struct
{
  guchar *pixels;
  gint    width;
  gint    height;
} WmfPixbuf;


#define WMF_TYPE  (wmf_get_type ())
#define WMF(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WMF_TYPE, Wmf))

GType                   wmf_get_type         (void) G_GNUC_CONST;

static GList          * wmf_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * wmf_create_procedure (GimpPlugIn            *plug_in,
                                              const gchar           *name);

static gboolean        wmf_extract           (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GFile                *file,
                                              GimpMetadata         *metadata,
                                              GimpProcedureConfig  *config,
                                              GimpVectorLoadData   *extracted_dimensions,
                                              gpointer             *data_for_run,
                                              GDestroyNotify       *data_for_run_destroy,
                                              gpointer              extract_data,
                                              GError              **error);
static GimpValueArray * wmf_load             (GimpProcedure         *procedure,
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
static GimpValueArray * wmf_load_thumb       (GimpProcedure         *procedure,
                                              GFile                 *file,
                                              gint                   size,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);

static GimpImage      * load_image           (GFile                 *file,
                                              gint                   requested_width,
                                              gint                   requested_height,
                                              gdouble                resolution,
                                              GError               **error);
static gboolean         load_wmf_size        (GFile                 *file,
                                              float                 *width,
                                              float                 *height,
                                              gboolean              *guessed);
static gboolean         load_dialog          (GFile                 *file,
                                              GimpProcedure         *procedure,
                                              GimpProcedureConfig   *config);
static WmfPixbuf      * wmf_get_pixbuf       (GFile                 *file,
                                              gint                  *width,
                                              gint                  *height);
static guchar         * wmf_load_file        (GFile                 *file,
                                              gint                   requested_width,
                                              gint                   requested_height,
                                              gdouble                resolution,
                                              guint                 *width,
                                              guint                 *height,
                                              GError               **error);


G_DEFINE_TYPE (Wmf, wmf, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (WMF_TYPE)
DEFINE_STD_SET_I18N


static void
wmf_class_init (WmfClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = wmf_query_procedures;
  plug_in_class->create_procedure = wmf_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
wmf_init (Wmf *wmf)
{
}

static GList *
wmf_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static GimpProcedure *
wmf_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_vector_load_procedure_new (plug_in, name,
                                                  GIMP_PDB_PROC_TYPE_PLUGIN,
                                                  wmf_extract, NULL, NULL,
                                                  wmf_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("Microsoft WMF file"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads files in the WMF file format",
                                        "Loads files in the WMF file format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Dom Lachowicz <cinamod@hotmail.com>",
                                      "Dom Lachowicz <cinamod@hotmail.com>",
                                      "(c) 2003 - Version 0.3.0");

      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("WMF"));
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-wmf");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "wmf,apm");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,\\327\\315\\306\\232,0,string,\\1\\0\\11\\0");

      gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = gimp_thumbnail_procedure_new (plug_in, name,
                                                GIMP_PDB_PROC_TYPE_PLUGIN,
                                                wmf_load_thumb, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Loads a small preview from a WMF image",
                                        "",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Dom Lachowicz <cinamod@hotmail.com>",
                                      "Dom Lachowicz <cinamod@hotmail.com>",
                                      "(c) 2003 - Version 0.3.0");
    }

  return procedure;
}

static gboolean
wmf_extract (GimpProcedure        *procedure,
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
  float    width   = 0;
  float    height  = 0;
  gboolean guessed = FALSE;

  if (! load_wmf_size (file, &width, &height, &guessed))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not open '%s' for reading AA"),
                   gimp_file_get_utf8_name (file));
      return FALSE;
    }

  extracted_dimensions->width         = width;
  extracted_dimensions->width_unit    = GIMP_UNIT_PERCENT;
  extracted_dimensions->exact_width   = guessed ? FALSE : TRUE;
  extracted_dimensions->height        = height;
  extracted_dimensions->height_unit   = GIMP_UNIT_PERCENT;
  extracted_dimensions->exact_height  = guessed ? FALSE : TRUE;
  extracted_dimensions->correct_ratio = guessed ? FALSE : TRUE;

  return TRUE;
}

static GimpValueArray *
wmf_load (GimpProcedure         *procedure,
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
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;
  gdouble         resolution;

  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE && ! load_dialog (file, procedure, config))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  g_object_get (config,
                "pixel-density", &resolution,
                "width",         &width,
                "height",        &height,
                NULL);
  image = load_image (file, width, height, resolution, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
wmf_load_thumb (GimpProcedure       *procedure,
                GFile               *file,
                gint                 size,
                GimpProcedureConfig *config,
                gpointer             run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  float           width  = 0;
  float           height = 0;
  GError         *error  = NULL;

  gegl_init (NULL, NULL);

  if (load_wmf_size (file, &width, &height, NULL) &&
      width  > 0 && height > 0)
    {
      if (width > height)
        {
          height *= (size / width);
          width   = (float) size;
        }
      else
        {
          width *= size / height;
          height = (float) size;
        }
    }
  else
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  image = load_image (file, width, height, WMF_DEFAULT_RESOLUTION, &error);

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


static GtkWidget *size_label = NULL;


/*  This function retrieves the pixel size from a WMF file. */
static gboolean
load_wmf_size (GFile       *file,
               float       *width,
               float       *height,
               gboolean    *guessed)
{
  GMappedFile    *mapped;
  /* the bits we need to decode the WMF via libwmf2's GD layer  */
  wmf_error_t     err;
  gulong          flags;
  wmf_gd_t       *ddata = NULL;
  wmfAPI         *API   = NULL;
  wmfAPI_Options  api_options;
  wmfD_Rect       bbox;
  gboolean        success = TRUE;
  char*           wmffontdirs[2] = { NULL, NULL };

  mapped = g_mapped_file_new (g_file_peek_path (file), FALSE, NULL);

  if (! mapped)
    return FALSE;

  flags = WMF_OPT_IGNORE_NONFATAL | WMF_OPT_FUNCTION;
  api_options.function = wmf_gd_function;

#ifdef ENABLE_RELOCATABLE_RESOURCES
  wmffontdirs[0] = g_build_filename (gimp_installation_directory (),
                                     "share/libwmf/fonts", NULL);
  flags |= WMF_OPT_FONTDIRS;
  api_options.fontdirs = wmffontdirs;
#endif

  err = wmf_api_create (&API, flags, &api_options);
  if (wmffontdirs[0])
    g_free (wmffontdirs[0]);
  if (err != wmf_E_None)
    success = FALSE;

  ddata = WMF_GD_GetData (API);
  ddata->type = wmf_gd_image;

  err = wmf_mem_open (API,
                      (guchar *) g_mapped_file_get_contents (mapped),
                      g_mapped_file_get_length (mapped));
  if (err != wmf_E_None)
    success = FALSE;

  err = wmf_scan (API, 0, &bbox);
  if (err != wmf_E_None)
    success = FALSE;

  err = wmf_size (API, width, height);
  if (err != wmf_E_None || *width <= 0.0f || *height <= 0.0f)
    success = FALSE;

  wmf_mem_close (API);
  g_mapped_file_unref (mapped);

  if (*width <= 0.0f || *height <= 0.0f)
    {
      *width  = WMF_DEFAULT_SIZE;
      *height = WMF_DEFAULT_SIZE;

      if (guessed)
        *guessed = TRUE;

      if (size_label)
        gtk_label_set_text (GTK_LABEL (size_label),
                            _("WMF file does not\nspecify a size!"));
    }
  else
    {
      if (size_label)
        {
          gchar *text = g_strdup_printf (_("%.4f × %.4f"), *width, *height);

          gtk_label_set_text (GTK_LABEL (size_label), text);
          g_free (text);
        }
    }

  return success;
}


/*  User interface  */

static void
wmf_preview_callback (GtkWidget     *widget,
                      GtkAllocation *allocation,
                      WmfPixbuf     *pixbuf)
{
  gimp_preview_area_draw (GIMP_PREVIEW_AREA (widget),
                          0, 0, pixbuf->width, pixbuf->height,
                          GIMP_RGBA_IMAGE,
                          pixbuf->pixels, pixbuf->width * 4);
}

static gboolean
load_dialog (GFile               *file,
             GimpProcedure       *procedure,
             GimpProcedureConfig *config)
{
  GtkWidget     *dialog;
  GtkWidget     *content_area;
  GtkWidget     *vbox;
  GtkWidget     *frame;
  GtkWidget     *image;
  WmfPixbuf     *pixbuf;
  gboolean       run = FALSE;
  WmfLoadVals    vals;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_vector_load_procedure_dialog_new (GIMP_VECTOR_LOAD_PROCEDURE (procedure),
                                                  GIMP_PROCEDURE_CONFIG (config),
                                                  NULL);

  /* TODO: in the old version, there used to be X and Y ratio fields which could
   * be constrained/linked (or not) and would sync back and forth with the
   * width/height fields. Are these needed? If so, should we try to automatize
   * such UI when editing dimensions with options in GimpResolutionEntry?
   */

  content_area  = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  /*  The WMF preview  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (content_area), vbox, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (content_area), vbox, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vals.width  = - WMF_PREVIEW_SIZE;
  vals.height = - WMF_PREVIEW_SIZE;
  pixbuf = wmf_get_pixbuf (file, &vals.width, &vals.height);
  image = gimp_preview_area_new ();
  gtk_widget_set_size_request (image, vals.width, vals.height);
  gtk_container_add (GTK_CONTAINER (frame), image);
  gtk_widget_show (image);

  g_signal_connect (image, "size-allocate",
                    G_CALLBACK (wmf_preview_callback),
                    pixbuf);

  size_label = gtk_label_new (NULL);
  gtk_label_set_justify (GTK_LABEL (size_label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), size_label, TRUE, TRUE, 4);
  gtk_widget_show (size_label);

  /* Run the dialog. */
  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);
  g_free (pixbuf->pixels);
  g_free (pixbuf);

  return run;
}

static guchar *
pixbuf_gd_convert (const gint *gd_pixels,
                   gint        width,
                   gint        height)
{
  const gint *gd_ptr = gd_pixels;
  guchar     *pixels;
  guchar     *px_ptr;
  gint        i, j;

  pixels = (guchar *) g_try_malloc (width * height * sizeof (guchar) * 4);
  if (! pixels)
    return NULL;

  px_ptr = pixels;

  for (i = 0; i < height; i++)
    for (j = 0; j < width; j++)
      {
        guchar r, g, b, a;
        guint  pixel = (guint) (*gd_ptr++);

        b = (guchar) (pixel & 0xff);
        pixel >>= 8;
        g = (guchar) (pixel & 0xff);
        pixel >>= 8;
        r = (guchar) (pixel & 0xff);
        pixel >>= 7;
        a = (guchar) (pixel & 0xfe);
        a ^= 0xff;

        *px_ptr++ = r;
        *px_ptr++ = g;
        *px_ptr++ = b;
        *px_ptr++ = a;
      }

  return pixels;
}

static WmfPixbuf *
wmf_get_pixbuf (GFile *file,
                gint  *width,
                gint  *height)
{
  WmfPixbuf      *pixbuf  = NULL;
  GMappedFile    *mapped;
  guchar         *pixels  = NULL;

  /* the bits we need to decode the WMF via libwmf2's GD layer  */
  wmf_error_t     err;
  gulong          flags;
  wmf_gd_t       *ddata = NULL;
  wmfAPI         *API   = NULL;
  wmfAPI_Options  api_options;
  guint           file_width;
  guint           file_height;
  wmfD_Rect       bbox;
  gint           *gd_pixels = NULL;
  char*           wmffontdirs[2] = { NULL, NULL };

  mapped = g_mapped_file_new (g_file_peek_path (file), FALSE, NULL);

  if (! mapped)
    return NULL;

  flags = WMF_OPT_IGNORE_NONFATAL | WMF_OPT_FUNCTION;
  api_options.function = wmf_gd_function;

#ifdef ENABLE_RELOCATABLE_RESOURCES
  wmffontdirs[0] = g_build_filename (gimp_installation_directory (),
                                     "share/libwmf/fonts", NULL);
  flags |= WMF_OPT_FONTDIRS;
  api_options.fontdirs = wmffontdirs;
#endif

  err = wmf_api_create (&API, flags, &api_options);
  if (wmffontdirs[0])
    g_free (wmffontdirs[0]);
  if (err != wmf_E_None)
    goto _wmf_error;

  ddata = WMF_GD_GetData (API);
  ddata->type = wmf_gd_image;

  err = wmf_mem_open (API,
                      (guchar *) g_mapped_file_get_contents (mapped),
                      g_mapped_file_get_length (mapped));
  if (err != wmf_E_None)
    goto _wmf_error;

  err = wmf_scan (API, 0, &bbox);
  if (err != wmf_E_None)
    goto _wmf_error;

  err = wmf_display_size (API, &file_width, &file_height,
                          WMF_DEFAULT_RESOLUTION, WMF_DEFAULT_RESOLUTION);
  if (err != wmf_E_None || file_width <= 0 || file_height <= 0)
    goto _wmf_error;

  if (!*width || !*height)
    goto _wmf_error;

  /*  either both arguments negative or none  */
  if ((*width * *height) < 0)
    goto _wmf_error;

  ddata->bbox   = bbox;

  if (*width > 0)
    {
      ddata->width  = *width;
      ddata->height = *height;
    }
  else
    {
      gdouble w      = file_width;
      gdouble h      = file_height;
      gdouble aspect = ((gdouble) *width) / (gdouble) *height;

      if (aspect > (w / h))
        {
          ddata->height = abs (*height);
          ddata->width  = (gdouble) abs (*width) * (w / h) + 0.5;
        }
      else
        {
          ddata->width  = abs (*width);
          ddata->height = (gdouble) abs (*height) / (w / h) + 0.5;
        }
    }

  err = wmf_play (API, 0, &bbox);
  if (err != wmf_E_None)
    goto _wmf_error;

  if (ddata->gd_image != NULL)
    gd_pixels = wmf_gd_image_pixels (ddata->gd_image);
  if (gd_pixels == NULL)
    goto _wmf_error;

  pixels = pixbuf_gd_convert (gd_pixels, ddata->width, ddata->height);
  if (pixels == NULL)
    goto _wmf_error;

  *width = ddata->width;
  *height = ddata->height;

 _wmf_error:
  if (API)
    {
      wmf_mem_close (API);
      wmf_api_destroy (API);
    }

  g_mapped_file_unref (mapped);

  if (pixels)
    {
      pixbuf = g_new0 (WmfPixbuf, 1);
      pixbuf->pixels = pixels;
      pixbuf->width  = *width;
      pixbuf->height = *height;
    }

  return pixbuf;
}

static guchar *
wmf_load_file (GFile   *file,
               gint     requested_width,
               gint     requested_height,
               gdouble  resolution,
               guint   *width,
               guint   *height,
               GError **error)
{
  GMappedFile    *mapped;
  guchar         *pixels   = NULL;

  /* the bits we need to decode the WMF via libwmf2's GD layer  */
  wmf_error_t     err;
  gulong          flags;
  wmf_gd_t       *ddata = NULL;
  wmfAPI         *API   = NULL;
  wmfAPI_Options  api_options;
  wmfD_Rect       bbox;
  gint           *gd_pixels = NULL;
  char*           wmffontdirs[2] = { NULL, NULL };

  *width = *height = -1;

  mapped = g_mapped_file_new (g_file_peek_path (file), FALSE, NULL);

  if (! mapped)
    return NULL;

  flags = WMF_OPT_IGNORE_NONFATAL | WMF_OPT_FUNCTION;
  api_options.function = wmf_gd_function;

#ifdef ENABLE_RELOCATABLE_RESOURCES
  wmffontdirs[0] = g_build_filename (gimp_installation_directory (),
                                     "share/libwmf/fonts/", NULL);
  flags |= WMF_OPT_FONTDIRS;
  api_options.fontdirs = wmffontdirs;
#endif

  err = wmf_api_create (&API, flags, &api_options);
  if (wmffontdirs[0])
    g_free (wmffontdirs[0]);
  if (err != wmf_E_None)
    goto _wmf_error;

  ddata = WMF_GD_GetData (API);
  ddata->type = wmf_gd_image;

  err = wmf_mem_open (API,
                      (guchar *) g_mapped_file_get_contents (mapped),
                      g_mapped_file_get_length (mapped));
  if (err != wmf_E_None)
    goto _wmf_error;

  err = wmf_scan (API, 0, &bbox);
  if (err != wmf_E_None)
    goto _wmf_error;

  err = wmf_display_size (API,
                          width, height,
                          resolution, resolution);
  if (err != wmf_E_None || *width <= 0 || *height <= 0)
    goto _wmf_error;

  if (requested_width > 0 && requested_height > 0)
    {
      *width  = requested_width;
      *height = requested_height;
    }

  ddata->bbox   = bbox;
  ddata->width  = *width;
  ddata->height = *height;

  err = wmf_play (API, 0, &bbox);
  if (err != wmf_E_None)
    goto _wmf_error;

  if (ddata->gd_image != NULL)
    gd_pixels = wmf_gd_image_pixels (ddata->gd_image);
  if (gd_pixels == NULL)
    goto _wmf_error;

  pixels = pixbuf_gd_convert (gd_pixels, *width, *height);
  if (pixels == NULL)
    goto _wmf_error;

 _wmf_error:
  if (API)
    {
      wmf_mem_close (API);
      wmf_api_destroy (API);
    }

  g_mapped_file_unref (mapped);

  /* FIXME: improve error message */
  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
               _("Could not open '%s' for reading"),
               gimp_file_get_utf8_name (file));

  return pixels;
}

/*
 * 'load_image()' - Load a WMF image into a new image window.
 */
static GimpImage *
load_image (GFile    *file,
            gint      requested_width,
            gint      requested_height,
            gdouble   resolution,
            GError  **error)
{
  GimpImage   *image;
  GimpLayer   *layer;
  GeglBuffer  *buffer;
  guchar      *pixels;
  guint        width, height;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  pixels = wmf_load_file (file, requested_width, requested_height, resolution,
                          &width, &height, error);

  if (! pixels)
    return NULL;

  image = gimp_image_new (width, height, GIMP_RGB);
  gimp_image_set_resolution (image, resolution, resolution);

  layer = gimp_layer_new (image,
                          _("Rendered WMF"),
                          width, height,
                          GIMP_RGBA_IMAGE,
                          100,
                          gimp_image_get_default_new_layer_mode (image));

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   babl_format ("R'G'B'A u8"),
                   pixels, GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);

  g_free (pixels);

  gimp_image_insert_layer (image, layer, NULL, 0);

  gimp_progress_update (1.0);

  return image;
}
