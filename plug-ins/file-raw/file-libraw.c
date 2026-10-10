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

#include "config.h"

#include <glib/gstdio.h>
#include <libraw.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "file-raw-formats.h"
#include "file-raw-utils.h"

#define PLUG_IN_BINARY    "file-libraw"
#define LOAD_THUMB_PROC   "file-libraw-load-thumb"

typedef struct _FileLibRaw      FileLibRaw;
typedef struct _FileLibRawClass FileLibRawClass;

struct _FileLibRaw
{
  GimpPlugIn      parent_instance;
};

struct _FileLibRawClass
{
  GimpPlugInClass parent_class;
};


#define FILELIBRAW_TYPE  (file_libraw_get_type ())
#define FILELIBRAW(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), FILELIBRAW_TYPE, FileLibRaw))

GType                   file_libraw_get_type         (void);

static GList          * file_libraw_init_procedures  (GimpPlugIn            *plug_in);
static GimpProcedure  * file_libraw_create_procedure (GimpPlugIn            *plug_in,
                                                      const gchar           *name);

static GimpValueArray * file_libraw_load             (GimpProcedure         *procedure,
                                                      GimpRunMode            run_mode,
                                                      GFile                 *file,
                                                      GimpMetadata          *metadata,
                                                      GimpMetadataLoadFlags *flags,
                                                      GimpProcedureConfig   *config,
                                                      gpointer               run_data);
static GimpValueArray * file_libraw_load_thumb       (GimpProcedure         *procedure,
                                                      GFile                 *file,
                                                      gint                   size,
                                                      GimpProcedureConfig   *config,
                                                      gpointer               run_data);

static GimpImage      * load_image                   (GFile                 *file,
                                                      libraw_data_t         *raw_info,
                                                      GimpRunMode            run_mode,
                                                      GimpProcedureConfig   *config,
                                                      GError               **error);
static GimpImage      * load_thumbnail_image         (GFile                 *file,
                                                      gint                   thumb_size,
                                                      GError               **error);


static gboolean         load_dialog                  (GimpProcedure         *procedure,
                                                      GObject               *config,
                                                      libraw_data_t         *raw_info);


G_DEFINE_TYPE (FileLibRaw, file_libraw, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (FILELIBRAW_TYPE)
DEFINE_STD_SET_I18N


static void
file_libraw_class_init (FileLibRawClass *klass)
{
  GimpPlugInClass *plug_in_class  = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->init_procedures  = file_libraw_init_procedures;
  plug_in_class->create_procedure = file_libraw_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
file_libraw_init (FileLibRaw *file_libraw)
{
}

static GList *
file_libraw_init_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));

  for (gint i = 0; i < G_N_ELEMENTS (file_formats); i++)
    {
      const FileFormat *format = &file_formats[i];
      gchar            *load_proc;

      load_proc = g_strdup_printf (format->load_proc_format, "libraw");

      list = g_list_append (list, load_proc);
    }

  return list;
}

static GimpProcedure *
file_libraw_create_procedure (GimpPlugIn  *plug_in,
                              const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = gimp_thumbnail_procedure_new (plug_in, name,
                                                GIMP_PDB_PROC_TYPE_PLUGIN,
                                                file_libraw_load_thumb,
                                                NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        _("Load thumbnail from a raw image"),
                                        NULL, name);
      gimp_procedure_set_attribution (procedure,
                                      "Alx Sa",
                                      "Alx Sa",
                                      "2026");
    }
  else
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
        {
          const FileFormat *format = &file_formats[i];
          gchar            *load_proc;
          gchar            *load_blurb;
          gchar            *load_help;

          load_proc = g_strdup_printf (format->load_proc_format, "libraw");

          if (strcmp (name, load_proc))
            {
              g_free (load_proc);
              continue;
            }

          load_blurb = g_strdup_printf (format->load_blurb_format,
                                        "libraw");
          load_help  = g_strdup_printf (format->load_help_format,
                                        "libraw");

          procedure = gimp_load_procedure_new (plug_in, name,
                                               GIMP_PDB_PROC_TYPE_PLUGIN,
                                               file_libraw_load,
                                               (gpointer) format, NULL);

          gimp_procedure_set_documentation (procedure,
                                            load_blurb, load_help, name);
          gimp_procedure_set_attribution (procedure,
                                          "Alx Sa",
                                          "Alx Sa",
                                          "2026");

          gimp_procedure_add_boolean_argument (procedure, "enable-interpolation",
                                               _("Enable interpolation"),
                                               _("Enable denoising and demosaicing steps"),
                                               TRUE,
                                               G_PARAM_READWRITE);

          /* The list of denoising and demosaicing algorithms are
           * hardcoded in libraw, and there doesn't even seem to be any
           * enum values in the C API.
           */

          gimp_procedure_add_choice_argument (procedure, "noise-reduction",
                                              _("Noise reduction"),
                                              _("Controls FBDD (Fake Before Demosaicing Denoising) noise reduction before demosaic"),
                                              gimp_choice_new_with_values ("disabled",   0, _("Disabled"),             NULL,
                                                                           "light-fbdd", 1, _("Light FBDD reduction"), NULL,
                                                                           "full-fbdd",  2, _("Full FBDD reduction"),  NULL,
                                                                           NULL),
                                              "disabled", G_PARAM_READWRITE);

          gimp_procedure_add_choice_argument (procedure, "demosaicing",
                                              _("Demosaicing"),
                                              _("Interpolation method to reconstruct a matrix of colored pixels"
                                                " from raw sensor data"),
                                              gimp_choice_new_with_values ("linear",       0,  _("Linear"),       NULL,
                                                                           "vng",          1,  _("VNG"),          NULL,
                                                                           "ppg",          2,  _("PPG"),          NULL,
                                                                           "ahd",          3,  _("AHD"),          NULL,
                                                                           "dcb",          4,  _("DCB"),          NULL,
                                                                           "dht",          11, _("DHT"),          NULL,
                                                                           "modified-ahd", 12, _("Modified AHD"), NULL,
                                                                           NULL),
                                              "ahd", G_PARAM_READWRITE);

          gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                               format->file_type);
          gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                              format->mime_type);
          gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                              format->extensions);
          gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                          format->magic);

          gimp_load_procedure_set_handles_raw (GIMP_LOAD_PROCEDURE (procedure),
                                               TRUE);
          gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                    LOAD_THUMB_PROC);

          g_free (load_proc);
          g_free (load_blurb);
          g_free (load_help);

          break;
        }
    }

  return procedure;
}

static GimpValueArray *
file_libraw_load (GimpProcedure          *procedure,
                  GimpRunMode             run_mode,
                  GFile                  *file,
                  GimpMetadata           *metadata,
                  GimpMetadataLoadFlags  *flags,
                  GimpProcedureConfig    *config,
                  gpointer                run_data)
{
  GimpValueArray    *return_vals;
  GimpImage         *image;
  libraw_data_t     *raw_info;
  GError            *error  = NULL;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  raw_info = libraw_init (LIBRAW_OPTIONS_NO_DATAERR_CALLBACK);
  if (raw_info == NULL)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("Error initializing LibRaw."));
      status = GIMP_PDB_EXECUTION_ERROR;
    }
  else if (libraw_open_file (raw_info, g_file_peek_path (file)) != LIBRAW_SUCCESS)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("Error reading data. Image may be corrupt."));
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS && run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_ui_init (PLUG_IN_BINARY);
      if (! load_dialog (procedure, G_OBJECT (config), raw_info))
        status = GIMP_PDB_CANCEL;
    }

  if (status == GIMP_PDB_SUCCESS &&
      (image = load_image (file, raw_info,
                           run_mode, config, &error)) == NULL)
    status = GIMP_PDB_EXECUTION_ERROR;

  return_vals = gimp_procedure_new_return_values (procedure, status, error);
  if (status == GIMP_PDB_SUCCESS)
    GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  if (raw_info != NULL)
    libraw_close (raw_info);

  return return_vals;
}

static GimpValueArray *
file_libraw_load_thumb (GimpProcedure        *procedure,
                        GFile                *file,
                        gint                  size,
                        GimpProcedureConfig  *config,
                        gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  image = load_thumbnail_image (file, size, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);
  GIMP_VALUES_SET_INT   (return_vals, 2, 0);
  GIMP_VALUES_SET_INT   (return_vals, 3, 0);
  GIMP_VALUES_SET_ENUM  (return_vals, 4, GIMP_RGB_IMAGE);
  GIMP_VALUES_SET_INT   (return_vals, 5, 1);

  gimp_value_array_truncate (return_vals, 6);

  return return_vals;
}

static GimpImage *
load_image (GFile                *file,
            libraw_data_t        *raw_info,
            GimpRunMode           run_mode,
            GimpProcedureConfig  *config,
            GError              **error)
{
  GimpImage                *image       = NULL;
  libraw_processed_image_t *image_data;
  gint                      raw_error   = 0;
  gint                      demosaicing = 3;
  gint                      denoising   = 0;
  gboolean                  enable_interpolation = TRUE;

  raw_error = libraw_unpack (raw_info);
  if (raw_error != LIBRAW_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error reading data. Image may be corrupt."));
      return NULL;
    }

  /* Just disable the full interpolation step altogether. */
  raw_info->params.no_interpolation = enable_interpolation ? 0 : 1;

  denoising = gimp_procedure_config_get_choice_id (config, "noise-reduction");
  libraw_set_fbdd_noiserd (raw_info, denoising);

  demosaicing = gimp_procedure_config_get_choice_id (config, "demosaicing");
  libraw_set_demosaic (raw_info, demosaicing);

  /* Ensure image is imported as 16 bpc */
  raw_info->params.output_bps    = 16;
  /* Try to use camera white balance */
  raw_info->params.use_camera_wb = 1;

  raw_error = libraw_dcraw_process (raw_info);
  if (raw_error != LIBRAW_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error reading data. Image may be corrupt."));
      return NULL;
    }

  image_data = libraw_dcraw_make_mem_image (raw_info, &raw_error);
  if (raw_error == LIBRAW_SUCCESS)
    {
      if (image_data->type == LIBRAW_IMAGE_BITMAP)
        {
          GimpImageBaseType  image_type = GIMP_RGB;
          GimpImageType      layer_type = GIMP_RGB_IMAGE;
          GimpPrecision      precision;
          GimpLayer         *layer;
          GimpColorProfile  *profile = NULL;
          GeglBuffer        *buffer;

          switch (image_data->colors)
            {
            case 1:
            case 2:
              image_type = GIMP_GRAY;
              layer_type = (image_data->colors == 1) ? GIMP_GRAY_IMAGE :
                                                       GIMP_GRAYA_IMAGE;
              break;

            case 3:
            case 4:
              image_type = GIMP_RGB;
              layer_type = (image_data->colors == 3) ? GIMP_RGB_IMAGE :
                                                       GIMP_RGBA_IMAGE;
              break;

            default:
              if (image_data != NULL)
                libraw_dcraw_clear_mem (image_data);

              g_set_error (error, G_FILE_ERROR, 0,
                           _("Error reading data. Image may be corrupt."));
              return NULL;
            }

          if (image_data->bits == 8)
            {
              precision = GIMP_PRECISION_U8_NON_LINEAR;
            }
          else if (image_data->bits == 16)
            {
              precision = GIMP_PRECISION_U16_NON_LINEAR;
            }
          else
            {
              if (image_data != NULL)
                libraw_dcraw_clear_mem (image_data);

              g_set_error (error, G_FILE_ERROR, 0,
                           _("Error reading data. Image may be corrupt."));
              return NULL;
            }

          if (raw_info->color.profile != NULL)
            {
              profile =
                gimp_color_profile_new_from_icc_profile (raw_info->color.profile,
                                                         raw_info->color.profile_length,
                                                         NULL);
            }

          image = gimp_image_new_with_precision (image_data->width,
                                                 image_data->height, image_type,
                                                 precision);

          if (profile)
            {
              gimp_image_set_color_profile (image, profile);
              g_object_unref (profile);
            }

          layer = gimp_layer_new (image, _("Background"), image_data->width,
                                  image_data->height, layer_type, 100,
                                  gimp_image_get_default_new_layer_mode (image));
          gimp_image_insert_layer (image, layer, NULL, 0);

          buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, image_data->width,
                                                   image_data->height),
                           0, NULL, image_data->data, GEGL_AUTO_ROWSTRIDE);

          g_object_unref (buffer);
        }
    }

  if (image_data != NULL)
    libraw_dcraw_clear_mem (image_data);

  return image;
}

static GimpImage *
load_thumbnail_image (GFile   *file,
                      gint     thumb_size,
                      GError **error)
{
  libraw_data_t            *raw_info;
  libraw_processed_image_t *thumbnail;
  guint                     flags     = LIBRAW_OPTIONS_NO_DATAERR_CALLBACK;
  gint                      raw_error = 0;
  GimpImage                *image     = NULL;

  raw_info = libraw_init (flags);

  if (raw_info == NULL)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error reading data. Image may be corrupt."));
      libraw_close (raw_info);
      return NULL;
    }

  raw_error = libraw_open_file (raw_info, g_file_peek_path (file));
  if (raw_error != LIBRAW_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error reading data. Image may be corrupt."));
      libraw_close (raw_info);
      return NULL;
    }

  raw_error = libraw_unpack_thumb (raw_info);
  if (raw_error != LIBRAW_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error reading data. Image may be corrupt."));
      libraw_close (raw_info);
      return NULL;
    }

  /* Try to use camera white balance */
  raw_info->params.use_camera_wb = 1;

  thumbnail = libraw_dcraw_make_mem_thumb (raw_info, &raw_error);
  if (raw_error == LIBRAW_SUCCESS)
    {
      if (thumbnail->type == LIBRAW_IMAGE_JPEG)
        {
          GFile          *temp_file      = NULL;
          FILE           *fp;
          GimpValueArray *return_vals    = NULL;
          GimpProcedure  *procedure;

          temp_file = gimp_temp_file ("jpeg");
          fp        = g_fopen (g_file_peek_path (temp_file), "wb");

          if (! fp)
            {
              g_set_error (error, G_FILE_ERROR, 0,
                           _("Error reading data. Image may be corrupt."));
              g_file_delete (temp_file, NULL, NULL);
              g_object_unref (temp_file);
              return NULL;
            }

          fwrite (thumbnail->data, sizeof (guchar), thumbnail->data_size, fp);
          fclose (fp);

          procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (),
                                                   "file-jpeg-load");
          return_vals = gimp_procedure_run (procedure,
                                            "run-mode", GIMP_RUN_NONINTERACTIVE,
                                            "file",     temp_file,
                                            NULL);

         if (return_vals)
           image =
             g_value_get_object (gimp_value_array_index (return_vals, 1));

          g_file_delete (temp_file, NULL, NULL);
          g_object_unref (temp_file);
          g_clear_pointer (&return_vals, gimp_value_array_unref);
        }
      else if (thumbnail->type == LIBRAW_IMAGE_BITMAP)
        {
          GimpLayer  *layer;
          GeglBuffer *buffer;

          image = gimp_image_new_with_precision (thumbnail->width,
                                                 thumbnail->height, GIMP_RGB,
                                                 GIMP_PRECISION_U8_NON_LINEAR);

          layer = gimp_layer_new (image, _("Background"), thumbnail->width,
                                  thumbnail->height, GIMP_RGB_IMAGE, 100,
                                  gimp_image_get_default_new_layer_mode (image));
          gimp_image_insert_layer (image, layer, NULL, 0);

          buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, thumbnail->width,
                                                   thumbnail->height), 0,
                           NULL, thumbnail->data, GEGL_AUTO_ROWSTRIDE);

          g_object_unref (buffer);
        }
    }

  if (thumbnail != NULL)
    libraw_dcraw_clear_mem (thumbnail);

  libraw_close (raw_info);

  return image;
}

static gboolean
load_dialog (GimpProcedure *procedure,
             GObject       *config,
             libraw_data_t *raw_info)
{
  GtkWidget *dialog;
  gchar     *title;
  gboolean   run;

  /* TRANSLATORS: the "%s" will be the name of a RAW format, e.g. "Raw Canon". */
  title = g_strdup_printf (_("Develop: %s"),
                           gimp_file_procedure_get_format_name (GIMP_FILE_PROCEDURE (procedure)));
  dialog = gimp_procedure_dialog_new (procedure, GIMP_PROCEDURE_CONFIG (config), title);
  gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog), "noise-reduction",
                                    GTK_TYPE_COMBO_BOX);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "interpolation-step",
                                  "noise-reduction", "demosaicing", NULL);
  gimp_procedure_dialog_fill_expander (GIMP_PROCEDURE_DIALOG (dialog), "interpolation-expander",
                                       "enable-interpolation", FALSE, "interpolation-step");
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "interpolation-expander", NULL);

  /* The whole logic under comes from reading LibRaw::dcraw_process()
   * code. This is unfortunately not cleanly documented and there are no
   * programmatic ways to actually verify if some settings should be
   * disabled or not. This means that this code could become wrong if
   * upstream implementation changes so we have to keep it in sync. :-(
   */
  if (raw_info->idata.filters == 0)
    {
      gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                           "interpolation-expander", FALSE, NULL, NULL, FALSE);
    }
  else if (raw_info->idata.filters <= 1000 || raw_info->idata.colors != 3)
    {
      /* Note: there is actually another case where noise reduction should
       * be disabled, based on a computation of "real_colors" but I wanted
       * to avoid duplicating a big piece of code including some hardcoded
       * 16*16 integer matrix. So let's accept that in some edge case,
       * this setting will be available while not doing anything.
       */
      gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                           "noise-reduction", FALSE, NULL, NULL, FALSE);
      if (raw_info->idata.filters <= 1000)
        {
          GParamSpec *pspec;
          GimpChoice *choice;

          pspec  = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "demosaicing");
          choice = gimp_param_spec_choice_get_choice (pspec);
          gimp_choice_set_sensitive (choice, "ppg", FALSE);
        }
    }

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);
  g_free (title);

  return run;
}
