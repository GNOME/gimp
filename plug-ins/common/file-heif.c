/*
 * GIMP HEIF loader / write plugin.
 * Copyright (c) 2018 struktur AG, Dirk Farin <farin@struktur.de>
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

#include <libheif/heif.h>
#include <lcms2.h>
#include <gexiv2/gexiv2.h>
#include <sys/time.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC        "file-heif-load"
#define LOAD_PROC_AV1    "file-heif-av1-load"
#define LOAD_PROC_HEJ2   "file-heif-hej2-load"
#define LOAD_PROC_AVCI   "file-heif-avci-load"
#define EXPORT_PROC      "file-heif-export"
#define EXPORT_PROC_AV1  "file-heif-av1-export"
#define EXPORT_PROC_HEJ2 "file-heif-hej2-export"
#define PLUG_IN_BINARY   "file-heif"

typedef enum _HeifpluginEncoderSpeed
{
  HEIFPLUGIN_ENCODER_SPEED_SLOW = 0,
  HEIFPLUGIN_ENCODER_SPEED_BALANCED = 1,
  HEIFPLUGIN_ENCODER_SPEED_FASTER = 2
} HeifpluginEncoderSpeed;

typedef enum _HeifpluginExportFormat
{
  HEIFPLUGIN_EXPORT_FORMAT_RGB = 0,
  HEIFPLUGIN_EXPORT_FORMAT_YUV444 = 1,
  HEIFPLUGIN_EXPORT_FORMAT_YUV422 = 2,
  HEIFPLUGIN_EXPORT_FORMAT_YUV420 = 3
} HeifpluginExportFormat;

typedef struct _GimpHeif      GimpHeif;
typedef struct _GimpHeifClass GimpHeifClass;

struct _GimpHeif
{
  GimpPlugIn      parent_instance;
};

struct _GimpHeifClass
{
  GimpPlugInClass parent_class;
};


#define GIMP_HEIF_TYPE  (gimp_heif_get_type ())
#define GIMP_HEIF(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_HEIF_TYPE, GimpHeif))

GType                   gimp_heif_get_type    (void) G_GNUC_CONST;

static GList          * heif_init_procedures  (GimpPlugIn                   *plug_in);
static GimpProcedure  * heif_create_procedure (GimpPlugIn                   *plug_in,
                                               const gchar                  *name);

static GimpValueArray * heif_load             (GimpProcedure                *procedure,
                                               GimpRunMode                   run_mode,
                                               GFile                        *file,
                                               GimpMetadata                 *metadata,
                                               GimpMetadataLoadFlags        *flags,
                                               GimpProcedureConfig          *config,
                                               gpointer                      run_data);
static GimpValueArray * heif_export           (GimpProcedure                *procedure,
                                               GimpRunMode                   run_mode,
                                               GimpImage                    *image,
                                               GFile                        *file,
                                               GimpExportOptions            *options,
                                               GimpMetadata                 *metadata,
                                               GimpProcedureConfig          *config,
                                               gpointer                      run_data);

static GimpValueArray * heif_av1_export       (GimpProcedure                *procedure,
                                               GimpRunMode                   run_mode,
                                               GimpImage                    *image,
                                               GFile                        *file,
                                               GimpExportOptions            *options,
                                               GimpMetadata                 *metadata,
                                               GimpProcedureConfig          *config,
                                               gpointer                      run_data);

#if LIBHEIF_HAVE_VERSION(1,19,8)
static GimpValueArray * heif_jpeg2000_export   (GimpProcedure                *procedure,
                                                GimpRunMode                   run_mode,
                                                GimpImage                    *image,
                                                GFile                        *file,
                                                GimpExportOptions            *options,
                                                GimpMetadata                 *metadata,
                                                GimpProcedureConfig          *config,
                                                gpointer                      run_data);
#endif

static GimpImage      * load_image            (GFile                        *file,
                                               GimpMetadata                 *metadata,
                                               GimpMetadataLoadFlags        *flags,
                                               gboolean                      interactive,
                                               GimpPDBStatusType            *status,
                                               GError                      **error);
static gboolean         export_image          (GFile                        *file,
                                               GimpImage                    *image,
                                               GimpDrawable                 *drawable,
                                               GObject                      *config,
                                               GError                      **error,
                                               enum heif_compression_format  compression,
                                               GimpMetadata                 *metadata);

static gboolean         load_dialog           (struct heif_context          *heif,
                                               uint32_t                     *selected_image);
static gboolean         save_dialog           (GimpProcedure                *procedure,
                                               GObject                      *config,
                                               GimpImage                    *image);


G_DEFINE_TYPE (GimpHeif, gimp_heif, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GIMP_HEIF_TYPE)
DEFINE_STD_SET_I18N


static void
gimp_heif_class_init (GimpHeifClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->init_procedures  = heif_init_procedures;
  plug_in_class->create_procedure = heif_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
gimp_heif_init (GimpHeif *heif)
{
}

static GList *
heif_init_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  heif_init (NULL);

  if (heif_have_decoder_for_format (heif_compression_HEVC))
    {
      list = g_list_append (list, g_strdup (LOAD_PROC));
    }

  if (heif_have_encoder_for_format (heif_compression_HEVC))
    {
      list = g_list_append (list, g_strdup (EXPORT_PROC));
    }

  if (heif_have_decoder_for_format (heif_compression_AV1))
    {
      list = g_list_append (list, g_strdup (LOAD_PROC_AV1));
    }

  if (heif_have_encoder_for_format (heif_compression_AV1))
    {
      list = g_list_append (list, g_strdup (EXPORT_PROC_AV1));
    }

#if LIBHEIF_HAVE_VERSION(1,17,0)
  if (heif_have_decoder_for_format (heif_compression_JPEG2000))
    {
      list = g_list_append (list, g_strdup (LOAD_PROC_HEJ2));
    }
#endif

#if LIBHEIF_HAVE_VERSION(1,19,8)
  if (heif_have_encoder_for_format (heif_compression_JPEG2000))
    {
      list = g_list_append (list, g_strdup (EXPORT_PROC_HEJ2));
    }
#endif

#if LIBHEIF_HAVE_VERSION(1,19,6)
  if (heif_have_decoder_for_format (heif_compression_AVC))
    {
      list = g_list_append (list, g_strdup (LOAD_PROC_AVCI));
    }
#endif

  heif_deinit ();

  return list;
}

static GimpProcedure *
heif_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           heif_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("HEIF/HEIC"));

      gimp_procedure_set_documentation (procedure,
                                        _("Loads HEIF images"),
                                        _("Load image stored in HEIF format (High "
                                          "Efficiency Image File Format). Typical "
                                          "suffices for HEIF files are .heif, "
                                          ".heic."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Dirk Farin <farin@struktur.de>",
                                      "Dirk Farin <farin@struktur.de>",
                                      "2018");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/heif");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "heif,heic");

      /* HEIF is an ISOBMFF format whose "brand" (the value after "ftyp")
       * can be of various values.
       * See also: https://gitlab.gnome.org/GNOME/gimp/issues/2209
       */
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "4,string,ftypheic,4,string,ftypheix,"
                                      "4,string,ftyphevc,4,string,ftypheim,"
                                      "4,string,ftypheis,4,string,ftyphevm,"
                                      "4,string,ftyphevs,4,string,ftypmif1,"
                                      "4,string,ftypmsf1");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, heif_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*");

      gimp_procedure_set_menu_label (procedure, _("HEIF/HEIC"));
      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           "HEIF");

      gimp_procedure_set_documentation (procedure,
                                        _("Exports HEIF images"),
                                        _("Save image in HEIF format (High "
                                          "Efficiency Image File Format)."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Dirk Farin <farin@struktur.de>",
                                      "Dirk Farin <farin@struktur.de>",
                                      "2018");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/heif");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "heif,heic");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB |
                                              GIMP_EXPORT_CAN_HANDLE_ALPHA,
                                              NULL, NULL, NULL);

      gimp_procedure_add_int_argument (procedure, "quality",
                                       _("_Quality"),
                                       _("Quality factor (0 = worst, 100 = best)"),
                                       0, 100, 50,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "lossless",
                                           _("L_ossless"),
                                           _("Use lossless compression"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_aux_argument (procedure, "include-color-profile",
                                               _("Save color prof_ile"),
                                               _("Save the image's color profile"),
                                               gimp_export_color_profile (),
                                               G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "save-bit-depth",
                                       _("_Bit depth"),
                                       _("Bit depth of exported image"),
                                       8, 12, 8,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "pixel-format",
                                          _("_Pixel format"),
                                          _("Format of color sub-sampling"),
                                          gimp_choice_new_with_values ("rgb",    HEIFPLUGIN_EXPORT_FORMAT_RGB,    _("RGB"),    NULL,
                                                                       "yuv444", HEIFPLUGIN_EXPORT_FORMAT_YUV444, _("YUV444"), NULL,
                                                                       "yuv420", HEIFPLUGIN_EXPORT_FORMAT_YUV420, _("YUV420"), NULL,
                                                                       NULL),
                                          "yuv420",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "encoder-speed",
                                          _("Enco_der speed"),
                                          _("Tradeoff between speed and compression"),
                                          gimp_choice_new_with_values ("slow",     HEIFPLUGIN_ENCODER_SPEED_SLOW,     _("Slow"),     NULL,
                                                                       "balanced", HEIFPLUGIN_ENCODER_SPEED_BALANCED, _("Balanced"), NULL,
                                                                       "fast",     HEIFPLUGIN_ENCODER_SPEED_FASTER,   _("Fast"),     NULL,
                                                                       NULL),
                                          "balanced",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "include-exif",
                                           _("Save Exi_f"),
                                           _("Toggle saving Exif data"),
                                           gimp_export_exif (),
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "include-xmp",
                                           _("Save _XMP"),
                                           _("Toggle saving XMP data"),
                                           gimp_export_xmp (),
                                           G_PARAM_READWRITE);
    }
  else if (! strcmp (name, LOAD_PROC_AV1))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           heif_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("HEIF/AVIF"));

      gimp_procedure_set_documentation (procedure,
                                        _("Loads AVIF images"),
                                        _("Load image stored in AV1 Image File Format (AVIF)"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Daniel Novomesky <dnovomesky@gmail.com>",
                                      "Daniel Novomesky <dnovomesky@gmail.com>",
                                      "2020");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/avif");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "avif");

      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "4,string,ftypmif1,4,string,ftypavif");

      gimp_file_procedure_set_priority (GIMP_FILE_PROCEDURE (procedure), 100);
    }
  else if (! strcmp (name, EXPORT_PROC_AV1))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, heif_av1_export, NULL,
                                             NULL);

      gimp_procedure_set_image_types (procedure, "RGB*");

      gimp_procedure_set_menu_label (procedure, _("HEIF/AVIF"));
      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           "AVIF");

      gimp_procedure_set_documentation (procedure,
                                        _("Exports AVIF images"),
                                        _("Save image in AV1 Image File Format (AVIF)"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Daniel Novomesky <dnovomesky@gmail.com>",
                                      "Daniel Novomesky <dnovomesky@gmail.com>",
                                      "2020");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/avif");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "avif");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB |
                                              GIMP_EXPORT_CAN_HANDLE_ALPHA,
                                              NULL, NULL, NULL);

      gimp_file_procedure_set_priority (GIMP_FILE_PROCEDURE (procedure), 100);

      gimp_procedure_add_int_argument (procedure, "quality",
                                       _("_Quality"),
                                       _("Quality factor (0 = worst, 100 = best)"),
                                       0, 100, 50,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "lossless",
                                           _("L_ossless"),
                                           _("Use lossless compression"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_aux_argument (procedure, "include-color-profile",
                                               _("Save color prof_ile"),
                                               _("Save the image's color profile"),
                                               gimp_export_color_profile (),
                                               G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "save-bit-depth",
                                       _("_Bit depth"),
                                       _("Bit depth of exported image"),
                                       8, 12, 8,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "pixel-format",
                                          _("_Pixel format"),
                                          _("Format of color sub-sampling"),
                                          gimp_choice_new_with_values ("rgb",    HEIFPLUGIN_EXPORT_FORMAT_RGB,    _("RGB"),    NULL,
                                                                       "yuv444", HEIFPLUGIN_EXPORT_FORMAT_YUV444, _("YUV444"), NULL,
                                                                       "yuv420", HEIFPLUGIN_EXPORT_FORMAT_YUV420, _("YUV420"), NULL,
                                                                       NULL),
                                          "yuv420",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "encoder-speed",
                                          _("Enco_der speed"),
                                          _("Tradeoff between speed and compression"),
                                          gimp_choice_new_with_values ("slow",     HEIFPLUGIN_ENCODER_SPEED_SLOW,     _("Slow"),     NULL,
                                                                       "balanced", HEIFPLUGIN_ENCODER_SPEED_BALANCED, _("Balanced"), NULL,
                                                                       "fast",     HEIFPLUGIN_ENCODER_SPEED_FASTER,   _("Fast"),     NULL,
                                                                       NULL),
                                          "balanced",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "include-exif",
                                           _("Save Exi_f"),
                                           _("Toggle saving Exif data"),
                                           gimp_export_exif (),
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "include-xmp",
                                           _("Save _XMP"),
                                           _("Toggle saving XMP data"),
                                           gimp_export_xmp (),
                                           G_PARAM_READWRITE);
    }
#if LIBHEIF_HAVE_VERSION(1,17,0)
  else if (! strcmp (name, LOAD_PROC_HEJ2))
    {
      procedure = gimp_load_procedure_new (plug_in, name, GIMP_PDB_PROC_TYPE_PLUGIN,
                                           heif_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("JPEG 2000 encapsulated in HEIF"));

      gimp_procedure_set_documentation (procedure,
                                        _("Loads HEJ2 images"),
                                        _("Load JPEG 2000 image encapsulated in HEIF (HEJ2)"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Daniel Novomesky <dnovomesky@gmail.com>",
                                      "Daniel Novomesky <dnovomesky@gmail.com>",
                                      "2023");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure), TRUE);
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/hej2k");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "hej2");

      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "4,string,ftypj2ki");
    }
#endif
#if LIBHEIF_HAVE_VERSION(1,19,6)
  else if (! strcmp (name, LOAD_PROC_AVCI))
    {
      procedure = gimp_load_procedure_new (plug_in, name, GIMP_PDB_PROC_TYPE_PLUGIN,
                                           heif_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("AVC-encoded image encapsulated in HEIF"));

      gimp_procedure_set_documentation (procedure,
                                        _("Loads AVCI images"),
                                        _("Load AVCI image - H.264 image encapsulated in HEIF"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Daniel Novomesky <dnovomesky@gmail.com>",
                                      "Daniel Novomesky <dnovomesky@gmail.com>",
                                      "2025");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure), TRUE);
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/avci");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "avci");

      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "4,string,ftypavci");
    }
#endif
#if LIBHEIF_HAVE_VERSION(1,19,8)
  else if (! strcmp (name, EXPORT_PROC_HEJ2))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, heif_jpeg2000_export, NULL,
                                             NULL);

      gimp_procedure_set_image_types (procedure, "RGB*");

      gimp_procedure_set_menu_label (procedure, "HEIF/HEJ2");
      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           "HEJ2");

      gimp_procedure_set_documentation (procedure,
                                        _("Exports HEJ2 images"),
                                        _("Save JPEG 2000 image encapsulated in HEIF (HEJ2)"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Daniel Novomesky <dnovomesky@gmail.com>",
                                      "Daniel Novomesky <dnovomesky@gmail.com>",
                                      "2025");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/hej2k");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "hej2");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB |
                                              GIMP_EXPORT_CAN_HANDLE_ALPHA,
                                              NULL, NULL, NULL);

      gimp_procedure_add_int_argument (procedure, "quality",
                                       _("_Quality"),
                                       _("Quality factor (0 = worst, 100 = best)"),
                                       0, 100, 50,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "lossless",
                                           _("L_ossless"),
                                           _("Use lossless compression"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_aux_argument (procedure, "include-color-profile",
                                               _("Save color prof_ile"),
                                               _("Save the image's color profile"),
                                               gimp_export_color_profile (),
                                               G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "save-bit-depth",
                                       _("_Bit depth"),
                                       _("Bit depth of exported image"),
                                       8, 12, 8,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "pixel-format",
                                          _("_Pixel format"),
                                          _("Format of color sub-sampling"),
                                          gimp_choice_new_with_values ("yuv444", HEIFPLUGIN_EXPORT_FORMAT_YUV444, _("YUV444"), NULL,
                                                                       "yuv420", HEIFPLUGIN_EXPORT_FORMAT_YUV420, _("YUV420"), NULL,
                                                                       NULL),
                                          "yuv420",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "include-exif",
                                           _("Save Exi_f"),
                                           _("Toggle saving Exif data"),
                                           gimp_export_exif (),
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "include-xmp",
                                           _("Save _XMP"),
                                           _("Toggle saving XMP data"),
                                           gimp_export_xmp (),
                                           G_PARAM_READWRITE);
    }
#endif
  return procedure;
}

static GimpValueArray *
heif_load (GimpProcedure         *procedure,
           GimpRunMode            run_mode,
           GFile                 *file,
           GimpMetadata          *metadata,
           GimpMetadataLoadFlags *flags,
           GimpProcedureConfig   *config,
           gpointer               run_data)
{
  GimpValueArray    *return_vals;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpImage         *image;
  gboolean           interactive;
  GError            *error = NULL;

  gegl_init (NULL, NULL);

  interactive = (run_mode == GIMP_RUN_INTERACTIVE);

  if (interactive)
    gimp_ui_init (PLUG_IN_BINARY);

  heif_init (NULL);

  image = load_image (file, metadata, flags, interactive, &status, &error);

  heif_deinit ();

  if (! image)
    return gimp_procedure_new_return_values (procedure, status, error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
heif_export (GimpProcedure        *procedure,
             GimpRunMode           run_mode,
             GimpImage            *image,
             GFile                *file,
             GimpExportOptions    *options,
             GimpMetadata         *metadata_unused,
             GimpProcedureConfig  *config,
             gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_IGNORE;
  GList             *drawables;
  GimpMetadata      *metadata;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_ui_init (PLUG_IN_BINARY);

      if (! save_dialog (procedure, G_OBJECT (config), image))
        status = GIMP_PDB_CANCEL;
    }

  export = gimp_export_options_get_image (options, &image);
  drawables = gimp_image_list_layers (image);

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpMetadataSaveFlags metadata_flags;

      metadata = gimp_image_metadata_save_prepare (image, "image/heif", &metadata_flags);

      heif_init (NULL);

      if (! export_image (file, image, drawables->data, G_OBJECT (config),
                          &error, heif_compression_HEVC, metadata))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }

      heif_deinit ();

      if (metadata)
        {
          g_object_unref (metadata);
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpValueArray *
heif_av1_export (GimpProcedure        *procedure,
                 GimpRunMode           run_mode,
                 GimpImage            *image,
                 GFile                *file,
                 GimpExportOptions    *options,
                 GimpMetadata         *metadata_unused,
                 GimpProcedureConfig  *config,
                 gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_IGNORE;
  GList             *drawables;

  GimpMetadata      *metadata;
  GError            *error       = NULL;

  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_ui_init (PLUG_IN_BINARY);

      if (! save_dialog (procedure, G_OBJECT (config), image))
        status = GIMP_PDB_CANCEL;
    }

  export = gimp_export_options_get_image (options, &image);
  drawables = gimp_image_list_layers (image);

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpMetadataSaveFlags metadata_flags;

      metadata = gimp_image_metadata_save_prepare (image, "image/avif", &metadata_flags);

      heif_init (NULL);

      if (! export_image (file, image, drawables->data, G_OBJECT (config),
                          &error, heif_compression_AV1, metadata))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }

      heif_deinit ();

      if (metadata)
        {
          g_object_unref (metadata);
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
}

#if LIBHEIF_HAVE_VERSION(1,19,8)
static GimpValueArray *
heif_jpeg2000_export (GimpProcedure        *procedure,
                      GimpRunMode           run_mode,
                      GimpImage            *image,
                      GFile                *file,
                      GimpExportOptions    *options,
                      GimpMetadata         *metadata_unused,
                      GimpProcedureConfig  *config,
                      gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_IGNORE;
  GList             *drawables;

  GimpMetadata      *metadata;
  GError            *error       = NULL;

  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_ui_init (PLUG_IN_BINARY);

      if (! save_dialog (procedure, G_OBJECT (config), image))
        status = GIMP_PDB_CANCEL;
    }

  export = gimp_export_options_get_image (options, &image);
  drawables = gimp_image_list_layers (image);

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpMetadataSaveFlags metadata_flags;

      metadata = gimp_image_metadata_save_prepare (image, "image/hej2k", &metadata_flags);

      heif_init (NULL);

      if (! export_image (file, image, drawables->data, G_OBJECT (config),
                          &error, heif_compression_JPEG2000, metadata))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }

      heif_deinit ();

      if (metadata)
        {
          g_object_unref (metadata);
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
}
#endif

static goffset
get_file_size (GFile   *file,
               GError **error)
{
  GFileInfo *info;
  goffset    size = 1;

  info = g_file_query_info (file,
                            G_FILE_ATTRIBUTE_STANDARD_SIZE,
                            G_FILE_QUERY_INFO_NONE,
                            NULL, error);
  if (info)
    {
      size = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_STANDARD_SIZE);

      g_object_unref (info);
    }

  return size;
}

static void
heifplugin_color_profile_set_tag (cmsHPROFILE      profile,
                                  cmsTagSignature  sig,
                                  const gchar     *tag)
{
  cmsMLU *mlu;

  mlu = cmsMLUalloc (NULL, 1);
  cmsMLUsetASCII (mlu, "en", "US", tag);
  cmsWriteTag (profile, sig, mlu);
  cmsMLUfree (mlu);
}

static GimpColorProfile *
nclx_to_gimp_profile (const struct heif_color_profile_nclx *nclx)
{
  const gchar *primaries_name = "";
  const gchar *trc_name = "";
  cmsHPROFILE profile = NULL;
  cmsCIExyY whitepoint;
  cmsCIExyYTRIPLE primaries;
  cmsToneCurve *curve[3];

  cmsFloat64Number srgb_parameters[5] =
  { 2.4, 1.0 / 1.055,  0.055 / 1.055, 1.0 / 12.92, 0.04045 };

  cmsFloat64Number rec709_parameters[5] =
  { 2.2, 1.0 / 1.099,  0.099 / 1.099, 1.0 / 4.5, 0.081 };

  if (nclx == NULL)
    {
      return NULL;
    }

  if (nclx->color_primaries == heif_color_primaries_unspecified)
    {
      return NULL;
    }

  if (nclx->color_primaries == heif_color_primaries_ITU_R_BT_709_5)
    {
      if (nclx->transfer_characteristics == heif_transfer_characteristic_IEC_61966_2_1)
        {
          return gimp_color_profile_new_rgb_srgb();
        }

      if (nclx->transfer_characteristics == heif_transfer_characteristic_linear)
        {
          return gimp_color_profile_new_rgb_srgb_linear();
        }
    }

  whitepoint.x = nclx->color_primary_white_x;
  whitepoint.y = nclx->color_primary_white_y;
  whitepoint.Y = 1.0f;

  primaries.Red.x = nclx->color_primary_red_x;
  primaries.Red.y = nclx->color_primary_red_y;
  primaries.Red.Y = 1.0f;

  primaries.Green.x = nclx->color_primary_green_x;
  primaries.Green.y = nclx->color_primary_green_y;
  primaries.Green.Y = 1.0f;

  primaries.Blue.x = nclx->color_primary_blue_x;
  primaries.Blue.y = nclx->color_primary_blue_y;
  primaries.Blue.Y = 1.0f;

  switch (nclx->color_primaries)
    {
    case heif_color_primaries_ITU_R_BT_709_5:
      primaries_name = "BT.709";
      break;
    case   heif_color_primaries_ITU_R_BT_470_6_System_M:
      primaries_name = "BT.470-6 System M";
      break;
    case heif_color_primaries_ITU_R_BT_470_6_System_B_G:
      primaries_name = "BT.470-6 System BG";
      break;
    case heif_color_primaries_ITU_R_BT_601_6:
      primaries_name = "BT.601";
      break;
    case heif_color_primaries_SMPTE_240M:
      primaries_name = "SMPTE 240M";
      break;
    case 8:
      primaries_name = "Generic film";
      break;
    case 9:
      primaries_name = "BT.2020";
      break;
    case 10:
      primaries_name = "XYZ";
      break;
    case 11:
      primaries_name = "SMPTE RP 431-2";
      break;
    case 12:
      primaries_name = "SMPTE EG 432-1 (DCI P3)";
      break;
    case 22:
      primaries_name = "EBU Tech. 3213-E";
      break;
    default:
      g_warning ("%s: Unsupported color_primaries value %d.",
                 G_STRFUNC, nclx->color_primaries);
      return NULL;
      break;
    }

  switch (nclx->transfer_characteristics)
    {
    case heif_transfer_characteristic_ITU_R_BT_709_5:
      curve[0] = curve[1] = curve[2] = cmsBuildParametricToneCurve (NULL, 4,
                                       rec709_parameters);
      profile = cmsCreateRGBProfile (&whitepoint, &primaries, curve);
      cmsFreeToneCurve (curve[0]);
      trc_name = "Rec709 RGB";
      break;
    case heif_transfer_characteristic_ITU_R_BT_470_6_System_M:
      curve[0] = curve[1] = curve[2] = cmsBuildGamma (NULL, 2.2f);
      profile = cmsCreateRGBProfile (&whitepoint, &primaries, curve);
      cmsFreeToneCurve (curve[0]);
      trc_name = "Gamma2.2 RGB";
      break;
    case heif_transfer_characteristic_ITU_R_BT_470_6_System_B_G:
      curve[0] = curve[1] = curve[2] = cmsBuildGamma (NULL, 2.8f);
      profile = cmsCreateRGBProfile (&whitepoint, &primaries, curve);
      cmsFreeToneCurve (curve[0]);
      trc_name = "Gamma2.8 RGB";
      break;
    case heif_transfer_characteristic_linear:
      curve[0] = curve[1] = curve[2] = cmsBuildGamma (NULL, 1.0f);
      profile = cmsCreateRGBProfile (&whitepoint, &primaries, curve);
      cmsFreeToneCurve (curve[0]);
      trc_name = "linear RGB";
      break;
    case heif_transfer_characteristic_IEC_61966_2_1:
      /* same as default */
    default:
      curve[0] = curve[1] = curve[2] = cmsBuildParametricToneCurve (NULL, 4,
                                       srgb_parameters);
      profile = cmsCreateRGBProfile (&whitepoint, &primaries, curve);
      cmsFreeToneCurve (curve[0]);
      trc_name = "sRGB-TRC RGB";
      break;
    }

  if (profile)
    {
      GimpColorProfile *new_profile;
      gchar *description = g_strdup_printf ("%s %s", primaries_name, trc_name);

      heifplugin_color_profile_set_tag (profile, cmsSigProfileDescriptionTag,
                                        description);
      heifplugin_color_profile_set_tag (profile, cmsSigDeviceMfgDescTag,
                                        "GIMP");
      heifplugin_color_profile_set_tag (profile, cmsSigDeviceModelDescTag,
                                        description);
      heifplugin_color_profile_set_tag (profile, cmsSigCopyrightTag,
                                        "Public Domain");

      new_profile = gimp_color_profile_new_from_lcms_profile (profile, NULL);

      cmsCloseProfile (profile);
      g_free (description);
      return new_profile;
    }

  return NULL;
}

GimpImage *
load_image (GFile                 *file,
            GimpMetadata          *metadata,
            GimpMetadataLoadFlags *flags,
            gboolean               interactive,
            GimpPDBStatusType     *status,
            GError               **error)
{
  GInputStream             *input;
  goffset                   file_size;
  guchar                   *file_buffer;
  gsize                     bytes_read;
  struct heif_context      *ctx;
  struct heif_error         err;
  struct heif_image_handle *handle  = NULL;
  struct heif_image        *img     = NULL;
  GimpColorProfile         *profile = NULL;
  gint                      n_images;
  heif_item_id              primary;
  heif_item_id              selected_image;
  gboolean                  has_alpha;
  gint                      width;
  gint                      height;
  GimpImage                *image;
  GimpLayer                *layer;
  GeglBuffer               *buffer;
  const Babl               *format;
  const guint8             *data;
  gint                      stride;
  gint                      bit_depth = 8;
  enum heif_chroma          chroma    = heif_chroma_interleaved_RGB;
  GimpPrecision             precision;
  gboolean                  load_linear;
  const char               *encoding;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  *status = GIMP_PDB_EXECUTION_ERROR;

  file_size = get_file_size (file, error);
  if (file_size <= 0)
    return NULL;

  input = G_INPUT_STREAM (g_file_read (file, NULL, error));
  if (! input)
    return NULL;

  file_buffer = g_malloc (file_size);

  if (! g_input_stream_read_all (input, file_buffer, file_size,
                                 &bytes_read, NULL, error) &&
      bytes_read == 0)
    {
      g_free (file_buffer);
      g_object_unref (input);
      return NULL;
    }

  gimp_progress_update (0.25);

  ctx = heif_context_alloc ();
  if (!ctx)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "cannot allocate heif_context");
      g_free (file_buffer);
      g_object_unref (input);
      return NULL;
    }

  err = heif_context_read_from_memory (ctx, file_buffer, file_size, NULL);
  if (err.code)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Loading HEIF image failed: %s"),
                   err.message);
      heif_context_free (ctx);
      g_free (file_buffer);
      g_object_unref (input);

      return NULL;
    }

  g_free (file_buffer);
  g_object_unref (input);

  gimp_progress_update (0.5);

  /* analyze image content
   * Is there more than one image? Which image is the primary image?
   */

  n_images = heif_context_get_number_of_top_level_images (ctx);
  if (n_images == 0)
    {
      g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Loading HEIF image failed: "
                             "Input file contains no readable images"));
      heif_context_free (ctx);

      return NULL;
    }

  err = heif_context_get_primary_image_ID (ctx, &primary);
  if (err.code)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Loading HEIF image failed: %s"),
                   err.message);
      heif_context_free (ctx);

      return NULL;
    }

  /* if primary image is no top level image or not present (invalid
   * file), just take the first image
   */

  if (! heif_context_is_top_level_image_ID (ctx, primary))
    {
      gint n = heif_context_get_list_of_top_level_image_IDs (ctx, &primary, 1);
      g_assert (n == 1);
    }

  selected_image = primary;

  /* if there are several images in the file and we are running
   * interactive, let the user choose a picture
   */

  if (interactive && n_images > 1)
    {
      if (! load_dialog (ctx, &selected_image))
        {
          heif_context_free (ctx);

          *status = GIMP_PDB_CANCEL;

          return NULL;
        }
    }

  /* load the picture */

  err = heif_context_get_image_handle (ctx, selected_image, &handle);
  if (err.code)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Loading HEIF image failed: %s"),
                   err.message);
      heif_context_free (ctx);

      return NULL;
    }

  bit_depth = heif_image_handle_get_luma_bits_per_pixel (handle);
  if (bit_depth < 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Input image has undefined bit-depth");
      heif_image_handle_release (handle);
      heif_context_free (ctx);

      return NULL;
    }

  has_alpha = heif_image_handle_has_alpha_channel (handle);

  if (bit_depth == 8)
    {
      if (has_alpha)
        {
          chroma = heif_chroma_interleaved_RGBA;
        }
      else
        {
          chroma = heif_chroma_interleaved_RGB;
        }
    }
  else /* high bit depth */
    {
#if ( G_BYTE_ORDER == G_LITTLE_ENDIAN )
      if (has_alpha)
        {
          chroma = heif_chroma_interleaved_RRGGBBAA_LE;
        }
      else
        {
          chroma = heif_chroma_interleaved_RRGGBB_LE;
        }
#else
      if (has_alpha)
        {
          chroma = heif_chroma_interleaved_RRGGBBAA_BE;
        }
      else
        {
          chroma = heif_chroma_interleaved_RRGGBB_BE;
        }
#endif
    }

  err = heif_decode_image (handle,
                           &img,
                           heif_colorspace_RGB,
                           chroma,
                           NULL);
  if (err.code)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Loading HEIF image failed: %s"),
                   err.message);
      heif_image_handle_release (handle);
      heif_context_free (ctx);

      return NULL;
    }

  switch (heif_image_handle_get_color_profile_type (handle))
    {
    case heif_color_profile_type_not_present:
      break;
    case heif_color_profile_type_rICC:
    case heif_color_profile_type_prof:
      /* I am unsure, but it looks like both these types represent an
       * ICC color profile. XXX
       */
      {
        void   *profile_data;
        size_t  profile_size;

        profile_size = heif_image_handle_get_raw_color_profile_size (handle);
        profile_data = g_malloc0 (profile_size);
        err = heif_image_handle_get_raw_color_profile (handle, profile_data);

        if (err.code)
          g_warning ("%s: ICC profile loading failed and discarded.",
                     G_STRFUNC);
        else
          profile = gimp_color_profile_new_from_icc_profile ((guint8 *) profile_data,
                                                             profile_size, NULL);

        g_free (profile_data);
      }
      break;
    case heif_color_profile_type_nclx:
      {
        struct heif_color_profile_nclx *nclx = NULL;

        err = heif_image_handle_get_nclx_color_profile (handle, &nclx);
        if (err.code)
          {
            g_warning ("%s: NCLX profile loading failed and discarded.",
                       G_STRFUNC);
          }
        else
          {
            profile = nclx_to_gimp_profile (nclx);
            heif_nclx_color_profile_free (nclx);
          }
      }
      break;
    default:
      g_warning ("%s: unknown color profile type has been discarded.",
                 G_STRFUNC);
      break;
    }

  gimp_progress_update (0.75);

  width  = heif_image_get_width  (img, heif_channel_interleaved);
  height = heif_image_get_height (img, heif_channel_interleaved);

  /* create GIMP image and copy HEIF image into the GIMP image
   * (converting it to RGB)
   */

  if (profile)
    {
      load_linear = gimp_color_profile_is_linear (profile);
    }
  else
    {
      load_linear = FALSE;
    }

  if (load_linear)
    {
      if (bit_depth == 8)
        {
          precision = GIMP_PRECISION_U8_LINEAR;
          encoding = has_alpha ? "RGBA u8" : "RGB u8";
        }
      else
        {
          precision = GIMP_PRECISION_U16_LINEAR;
          encoding = has_alpha ? "RGBA u16" : "RGB u16";
        }
    }
  else /* non-linear profiles */
    {
      if (bit_depth == 8)
        {
          precision = GIMP_PRECISION_U8_NON_LINEAR;
          encoding = has_alpha ? "R'G'B'A u8" : "R'G'B' u8";
        }
      else
        {
          precision = GIMP_PRECISION_U16_NON_LINEAR;
          encoding = has_alpha ? "R'G'B'A u16" : "R'G'B' u16";
        }
    }

  image = gimp_image_new_with_precision (width, height, GIMP_RGB, precision);

  if (profile)
    {
      if (gimp_color_profile_is_rgb (profile))
        {
          gimp_image_set_color_profile (image, profile);
        }
      else if (gimp_color_profile_is_gray (profile))
        {
          g_warning ("Gray ICC profile was not applied to the imported image.");
        }
      else
        {
          g_warning ("ICC profile was not applied to the imported image.");
        }
    }

  layer = gimp_layer_new (image,
                          _("image content"),
                          width, height,
                          has_alpha ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE,
                          100.0,
                          gimp_image_get_default_new_layer_mode (image));

  gimp_image_insert_layer (image, layer, NULL, 0);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  data = heif_image_get_plane_readonly (img, heif_channel_interleaved,
                                        &stride);

  format = babl_format_with_space (encoding,
                                   gegl_buffer_get_format (buffer));

  if (bit_depth == 8)
    {
      gegl_buffer_set (buffer,
                       GEGL_RECTANGLE (0, 0, width, height),
                       0, format, data, stride);
    }
  else /* high bit depth */
    {
      uint16_t       *data16;
      const uint16_t *src16;
      uint16_t       *dest16;
      gint            x, y, rowentries;
      int             tmp_pixelval;

      if (has_alpha)
        {
          rowentries = width * 4;
        }
      else /* no alpha */
        {
          rowentries = width * 3;
        }

      data16 = g_malloc_n (height, rowentries * 2);
      dest16 = data16;

      switch (bit_depth)
        {
        case 10:
          for (y = 0; y < height; y++)
            {
              src16 = (const uint16_t *) (y * stride + data);
              for (x = 0; x < rowentries; x++)
                {
                  tmp_pixelval = (int) ( ( (float) (0x03ff & (*src16)) / 1023.0f) * 65535.0f + 0.5f);
                  *dest16 = CLAMP (tmp_pixelval, 0, 65535);
                  dest16++;
                  src16++;
                }
            }
          break;
        case 12:
          for (y = 0; y < height; y++)
            {
              src16 = (const uint16_t *) (y * stride + data);
              for (x = 0; x < rowentries; x++)
                {
                  tmp_pixelval = (int) ( ( (float) (0x0fff & (*src16))  / 4095.0f) * 65535.0f + 0.5f);
                  *dest16 = CLAMP (tmp_pixelval, 0, 65535);
                  dest16++;
                  src16++;
                }
            }
          break;
        default:
          for (y = 0; y < height; y++)
            {
              src16 = (const uint16_t *) (y * stride + data);
              for (x = 0; x < rowentries; x++)
                {
                  *dest16 = *src16;
                  dest16++;
                  src16++;
                }
            }
          break;
        }

      gegl_buffer_set (buffer,
                       GEGL_RECTANGLE (0, 0, width, height),
                       0, format, data16, GEGL_AUTO_ROWSTRIDE);

      g_free (data16);
    }

  g_object_unref (buffer);

  if (metadata)
    {
      size_t       exif_data_size = 0;
      uint8_t     *exif_data      = NULL;
      size_t       xmp_data_size  = 0;
      uint8_t     *xmp_data       = NULL;
      gint         n_metadata;
      heif_item_id metadata_id;

      n_metadata = heif_image_handle_get_list_of_metadata_block_IDs (handle, "Exif",
                                                                     &metadata_id, 1);
      if (n_metadata > 0)
        {
          exif_data_size = heif_image_handle_get_metadata_size (handle, metadata_id);

          exif_data = g_alloca (exif_data_size);

          err = heif_image_handle_get_metadata (handle, metadata_id, exif_data);
          if (err.code != 0)
            {
              exif_data      = NULL;
              exif_data_size = 0;
            }
        }

      n_metadata = heif_image_handle_get_list_of_metadata_block_IDs (handle, "mime",
                                                                     &metadata_id, 1);
      if (n_metadata > 0)
        {
          if (g_strcmp0 (heif_image_handle_get_metadata_content_type (handle, metadata_id), "application/rdf+xml")
              == 0)
            {
              xmp_data_size = heif_image_handle_get_metadata_size (handle, metadata_id);

              xmp_data = g_alloca (xmp_data_size);

              err = heif_image_handle_get_metadata (handle, metadata_id, xmp_data);
              if (err.code != 0)
                {
                  xmp_data      = NULL;
                  xmp_data_size = 0;
                }
            }
        }

      if (exif_data || xmp_data)
        {
          gexiv2_metadata_clear (GEXIV2_METADATA (metadata));

          if (exif_data)
            {
              const guint8    tiffHeaderBE[4] = { 'M', 'M', 0, 42 };
              const guint8    tiffHeaderLE[4] = { 'I', 'I', 42, 0 };
              GExiv2Metadata *exif_metadata   = GEXIV2_METADATA (metadata);
              const guint8   *tiffheader      = exif_data;
              glong           new_exif_size   = exif_data_size;

              while (new_exif_size >= 4) /*Searching for TIFF Header*/
                {
                  if (tiffheader[0] == tiffHeaderBE[0] && tiffheader[1] == tiffHeaderBE[1] &&
                      tiffheader[2] == tiffHeaderBE[2] && tiffheader[3] == tiffHeaderBE[3])
                    {
                      break;
                    }
                  if (tiffheader[0] == tiffHeaderLE[0] && tiffheader[1] == tiffHeaderLE[1] &&
                      tiffheader[2] == tiffHeaderLE[2] && tiffheader[3] == tiffHeaderLE[3])
                    {
                      break;
                    }
                  new_exif_size--;
                  tiffheader++;
                }

              if (new_exif_size > 4) /* TIFF header + some data found*/
                {
                  if (! gexiv2_metadata_open_buf (exif_metadata, tiffheader, new_exif_size, error))
                    {
                      g_printerr ("%s: Failed to set EXIF metadata: %s\n", G_STRFUNC, (*error)->message);
                      g_clear_error (error);
                    }
                }
              else
                {
                  g_printerr ("%s: EXIF metadata not set\n", G_STRFUNC);
                }
            }

          if (xmp_data)
            {
              if (! gimp_metadata_set_from_xmp (metadata, xmp_data, xmp_data_size, error))
                {
                  g_printerr ("%s: Failed to set XMP metadata: %s\n", G_STRFUNC, (*error)->message);
                  g_clear_error (error);
                }
            }
        }

      gexiv2_metadata_try_set_orientation (GEXIV2_METADATA (metadata),
                                           GEXIV2_ORIENTATION_NORMAL, NULL);
      gexiv2_metadata_try_set_metadata_pixel_width (GEXIV2_METADATA (metadata), width, NULL);
      gexiv2_metadata_try_set_metadata_pixel_height (GEXIV2_METADATA (metadata),
                                                     height, NULL);

      *flags = GIMP_METADATA_LOAD_COMMENT | GIMP_METADATA_LOAD_RESOLUTION;
    }

  if (profile)
    g_object_unref (profile);

  heif_image_handle_release (handle);
  heif_context_free (ctx);
  heif_image_release (img);

  gimp_progress_update (1.0);

  if (image)
    {
      *status = GIMP_PDB_SUCCESS;
    }
  return image;
}

static struct heif_error
write_callback (struct heif_context *ctx,
                const void          *data,
                size_t               size,
                void                *userdata)
{
  GOutputStream     *output = userdata;
  GError            *error  = NULL;
  struct heif_error  heif_error;

  heif_error.code    = heif_error_Ok;
  heif_error.subcode = heif_suberror_Unspecified;
  heif_error.message = "";

  if (! g_output_stream_write_all (output, data, size, NULL, NULL, &error))
    {
      heif_error.code    = 99; /* hmm */
      heif_error.message = error->message;
    }

  return heif_error;
}

static gboolean
export_image (GFile                        *file,
              GimpImage                    *image,
              GimpDrawable                 *drawable,
              GObject                      *config,
              GError                      **error,
              enum heif_compression_format  compression,
              GimpMetadata                 *metadata)
{
  struct heif_image                    *h_image = NULL;
  struct heif_context                  *context = heif_context_alloc ();
  struct heif_encoder                  *encoder = NULL;
  struct heif_encoding_options         *encoder_options = NULL;
  const struct heif_encoder_descriptor *encoder_descriptor;
  const char                           *encoder_name;
  struct heif_image_handle             *handle  = NULL;
  struct heif_writer                    writer;
  struct heif_error                     err;
  GOutputStream                        *output;
  GeglBuffer                           *buffer;
  const gchar                          *encoding;
  const Babl                           *format;
  const Babl                           *space   = NULL;
  guint8                               *data;
  gint                                  stride;
  gint                                  width;
  gint                                  height;
  gboolean                              has_alpha;
  gboolean                              out_linear = FALSE;
  gboolean                              lossless;
  gint                                  quality;
  gboolean                              save_profile;
  gint                                  save_bit_depth = 8;

  HeifpluginExportFormat    pixel_format = HEIFPLUGIN_EXPORT_FORMAT_YUV420;
  const char               *parameter_value;
  struct heif_color_profile_nclx nclx_profile;
  gboolean                  save_exif = FALSE;
  gboolean                  save_xmp = FALSE;

  if (!context)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "cannot allocate heif_context");
      return FALSE;
    }

  g_object_get (config,
                "lossless",           &lossless,
                "quality",            &quality,
                "save-bit-depth",     &save_bit_depth,
                "include-color-profile", &save_profile,
                "include-exif", &save_exif,
                "include-xmp", &save_xmp,
                NULL);
  pixel_format =
    gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                         "pixel-format");

  if (compression == heif_compression_HEVC)
    {
      if (heif_context_get_encoder_descriptors (context,
                                                heif_compression_HEVC,
                                                NULL,
                                                &encoder_descriptor, 1) == 1)
        {
          encoder_name = heif_encoder_descriptor_get_id_name (encoder_descriptor);
        }
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       "Unable to find suitable HEIF encoder");
          heif_context_free (context);
          return FALSE;
        }
    }
  else if (compression == heif_compression_AV1 &&
           heif_context_get_encoder_descriptors (context,
                                                 heif_compression_AV1,
                                                 "aom", /* we prefer aom rather than rav1e */
                                                 &encoder_descriptor, 1) == 1)
    {
      encoder_name = heif_encoder_descriptor_get_id_name (encoder_descriptor);
    }
  else /* non-AOM AV1 or JPEG 2000 */
    {
      if (heif_context_get_encoder_descriptors (context,
                                                compression,
                                                NULL,
                                                &encoder_descriptor, 1) == 1)
        {
          encoder_name = heif_encoder_descriptor_get_id_name (encoder_descriptor);
        }
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       "Unable to find suitable encoder");
          heif_context_free (context);
          return FALSE;
        }
    }

  gimp_progress_init_printf (_("Exporting '%s' using %s encoder"),
                             gimp_file_get_utf8_name (file), encoder_name);

  width   = gimp_drawable_get_width  (drawable);
  height  = gimp_drawable_get_height (drawable);

  has_alpha = gimp_drawable_has_alpha (drawable);

  switch (save_bit_depth)
    {
    case 8:
      err = heif_image_create (width, height,
                               heif_colorspace_RGB,
                               has_alpha ?
                               heif_chroma_interleaved_RGBA :
                               heif_chroma_interleaved_RGB,
                               &h_image);
      break;
    case 10:
    case 12:
#if ( G_BYTE_ORDER == G_LITTLE_ENDIAN )
      err = heif_image_create (width, height,
                               heif_colorspace_RGB,
                               has_alpha ?
                               heif_chroma_interleaved_RRGGBBAA_LE :
                               heif_chroma_interleaved_RRGGBB_LE,
                               &h_image);
#else
      err = heif_image_create (width, height,
                               heif_colorspace_RGB,
                               has_alpha ?
                               heif_chroma_interleaved_RRGGBBAA_BE :
                               heif_chroma_interleaved_RRGGBB_BE,
                               &h_image);
#endif
      break;
    default:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Unsupported bit depth: %d",
                   save_bit_depth);
      heif_context_free (context);
      return FALSE;
      break;
    }

  if (err.code != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Encoding HEIF image failed: %s"),
                   err.message);
      heif_context_free (context);
      return FALSE;
    }

  if (save_profile)
    {
      GimpColorProfile *profile = NULL;
      const guint8     *icc_data;
      gsize             icc_length;

      profile = gimp_image_get_color_profile (image);
      if (profile && gimp_color_profile_is_linear (profile))
        out_linear = TRUE;

      if (! profile)
        {
          profile = gimp_image_get_effective_color_profile (image);

          if (gimp_color_profile_is_linear (profile))
            {
              if (gimp_image_get_precision (image) != GIMP_PRECISION_U8_LINEAR)
                {
                  /* If stored data was linear, let's convert the profile. */
                  GimpColorProfile *saved_profile;

                  saved_profile = gimp_color_profile_new_srgb_trc_from_color_profile (profile);
                  g_clear_object (&profile);
                  profile = saved_profile;
                }
              else
                {
                  /* Keep linear profile as-is for 8-bit linear image. */
                  out_linear = TRUE;
                }
            }
        }

      if (pixel_format == HEIFPLUGIN_EXPORT_FORMAT_RGB)
        {
          nclx_profile.version = 1;
          nclx_profile.color_primaries = heif_color_primaries_unspecified;

          if (out_linear)
            {
              nclx_profile.transfer_characteristics = heif_transfer_characteristic_linear;
            }
          else
            {
              nclx_profile.transfer_characteristics = heif_transfer_characteristic_unspecified;
            }

          nclx_profile.matrix_coefficients = heif_matrix_coefficients_RGB_GBR;
          nclx_profile.full_range_flag = 1;

          heif_image_set_nclx_color_profile (h_image, &nclx_profile);
        }

      icc_data = gimp_color_profile_get_icc_profile (profile, &icc_length);
      heif_image_set_raw_color_profile (h_image, "prof", icc_data, icc_length);
      space = gimp_color_profile_get_space (profile,
                                            GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                            error);
      if (error && *error)
        {
          /* Don't make this a hard failure yet output the error. */
          g_printerr ("%s: error getting the profile space: %s",
                      G_STRFUNC, (*error)->message);
          g_clear_error (error);
        }

      g_object_unref (profile);
    }
  else
    {
      /* We save as sRGB */

      nclx_profile.version = 1;
      nclx_profile.color_primaries = heif_color_primaries_ITU_R_BT_709_5;
      nclx_profile.transfer_characteristics = heif_transfer_characteristic_IEC_61966_2_1;
      nclx_profile.matrix_coefficients = heif_matrix_coefficients_ITU_R_BT_601_6;
      nclx_profile.full_range_flag = 1;

      if (pixel_format == HEIFPLUGIN_EXPORT_FORMAT_RGB)
        {
          nclx_profile.matrix_coefficients = heif_matrix_coefficients_RGB_GBR;
        }

      heif_image_set_nclx_color_profile (h_image, &nclx_profile);

      space = babl_space ("sRGB");
      out_linear = FALSE;
    }

  if (! space)
    space = gimp_drawable_get_format (drawable);


  if (save_bit_depth > 8)
    {
      uint16_t       *data16;
      const uint16_t *src16;
      uint16_t       *dest16;
      gint            x, y, rowentries;
      int             tmp_pixelval;

      if (has_alpha)
        {
          rowentries = width * 4;

          if (out_linear)
            encoding = "RGBA u16";
          else
            encoding = "R'G'B'A u16";
        }
      else /* no alpha */
        {
          rowentries = width * 3;

          if (out_linear)
            encoding = "RGB u16";
          else
            encoding = "R'G'B' u16";
        }

      data16 = g_malloc_n (height, rowentries * 2);
      src16 = data16;

      format = babl_format_with_space (encoding, space);

      buffer = gimp_drawable_get_buffer (drawable);

      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (0, 0, width, height),
                       1.0, format, data16, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      g_object_unref (buffer);

      heif_image_add_plane (h_image, heif_channel_interleaved,
                            width, height, save_bit_depth);

      data = heif_image_get_plane (h_image, heif_channel_interleaved, &stride);

      switch (save_bit_depth)
        {
        case 10:
          for (y = 0; y < height; y++)
            {
              dest16 = (uint16_t *) (y * stride + data);
              for (x = 0; x < rowentries; x++)
                {
                  tmp_pixelval = (int) ( ( (float) (*src16) / 65535.0f) * 1023.0f + 0.5f);
                  *dest16 = CLAMP (tmp_pixelval, 0, 1023);
                  dest16++;
                  src16++;
                }
            }
          break;
        case 12:
          for (y = 0; y < height; y++)
            {
              dest16 = (uint16_t *) (y * stride + data);
              for (x = 0; x < rowentries; x++)
                {
                  tmp_pixelval = (int) ( ( (float) (*src16) / 65535.0f) * 4095.0f + 0.5f);
                  *dest16 = CLAMP (tmp_pixelval, 0, 4095);
                  dest16++;
                  src16++;
                }
            }
          break;
        default:
          for (y = 0; y < height; y++)
            {
              dest16 = (uint16_t *) (y * stride + data);
              for (x = 0; x < rowentries; x++)
                {
                  *dest16 = *src16;
                  dest16++;
                  src16++;
                }
            }
          break;
        }
      g_free (data16);
    }
  else /* save_bit_depth == 8 */
    {
      heif_image_add_plane (h_image, heif_channel_interleaved,
                            width, height, 8);

      data = heif_image_get_plane (h_image, heif_channel_interleaved, &stride);

      buffer = gimp_drawable_get_buffer (drawable);

      if (has_alpha)
        {
          if (out_linear)
            encoding = "RGBA u8";
          else
            encoding = "R'G'B'A u8";
        }
      else
        {
          if (out_linear)
            encoding = "RGB u8";
          else
            encoding = "R'G'B' u8";
        }
      format = babl_format_with_space (encoding, space);

      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (0, 0, width, height),
                       1.0, format, data, stride, GEGL_ABYSS_NONE);

      g_object_unref (buffer);
    }

  gimp_progress_update (0.33);

  /*  encode to HEIF file  */
  err = heif_context_get_encoder (context,
                                  encoder_descriptor,
                                  &encoder);

  if (err.code != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Unable to get an encoder instance");
      heif_image_release (h_image);
      heif_context_free (context);
      return FALSE;
    }

  /* workaround for a bug in libheif when heif_encoder_set_lossless is not working
     (known problem with encoding via rav1e) */
  if (lossless)
    {
      quality = 100;
    }

  heif_encoder_set_lossy_quality (encoder, quality);
  heif_encoder_set_lossless (encoder, lossless);
  /* heif_encoder_set_logging_level (encoder, logging_level); */


  if (lossless && pixel_format != HEIFPLUGIN_EXPORT_FORMAT_RGB)
    {
      /* disable subsampling for lossless */
      pixel_format = HEIFPLUGIN_EXPORT_FORMAT_YUV444;
    }

  switch (pixel_format)
    {
    case HEIFPLUGIN_EXPORT_FORMAT_RGB:
      /* same as HEIFPLUGIN_EXPORT_FORMAT_YUV444 */
    case HEIFPLUGIN_EXPORT_FORMAT_YUV444:
      parameter_value = "444";
      break;
    case HEIFPLUGIN_EXPORT_FORMAT_YUV422:
      parameter_value = "422";
      break;
    default: /* HEIFPLUGIN_EXPORT_FORMAT_YUV420 */
      parameter_value = "420";
      break;
    }

  err = heif_encoder_set_parameter_string (encoder, "chroma", parameter_value);
  if (err.code != 0)
    {
      g_printerr ("Failed to set chroma %s for %s encoder: %s", parameter_value, encoder_name, err.message);
    }

  if (compression == heif_compression_HEVC)
    {
      HeifpluginEncoderSpeed  encoder_speed_hevc =
        gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                             "encoder-speed");

      switch (encoder_speed_hevc)
        {
        case HEIFPLUGIN_ENCODER_SPEED_SLOW:
          parameter_value = "veryslow";
          break;
        case HEIFPLUGIN_ENCODER_SPEED_FASTER:
          parameter_value = "faster";
          break;
        default: /*  HEIFPLUGIN_ENCODER_SPEED_BALANCED */
          parameter_value = "medium";
          break;
        }

      err = heif_encoder_set_parameter_string (encoder, "preset", parameter_value);
      if (err.code != 0)
        {
          g_printerr ("Failed to set preset %s for %s encoder: %s", parameter_value, encoder_name, err.message);
        }

    }
  else if (compression == heif_compression_AV1)
    {
      HeifpluginEncoderSpeed  encoder_speed_av1 =
        gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                             "encoder-speed");

      int parameter_number;

      parameter_number = gimp_get_num_processors();
      parameter_number = CLAMP(parameter_number, 1, 16);

      err = heif_encoder_set_parameter_integer (encoder, "threads", parameter_number);
      if (err.code != 0)
        {
          g_printerr ("Failed to set threads=%d for %s encoder: %s", parameter_number, encoder_name, err.message);
        }


      if (g_strcmp0 (encoder_name, "aom") == 0) /* AOMedia AV1 encoder */
        {
          switch (encoder_speed_av1)
            {
            case HEIFPLUGIN_ENCODER_SPEED_SLOW:
              parameter_number = 1;
              break;
            case HEIFPLUGIN_ENCODER_SPEED_FASTER:
              parameter_number = 6;
              err = heif_encoder_set_parameter_boolean (encoder, "realtime", 1);
              if (err.code != 0)
                {
                  g_printerr ("Failed to set realtime=1 for %s encoder: %s",  encoder_name, err.message);
                }
              break;
            default: /*  HEIFPLUGIN_ENCODER_SPEED_BALANCED */
              parameter_number = 5;
              break;
            }

          err = heif_encoder_set_parameter_integer (encoder, "speed", parameter_number);
          if (err.code != 0)
            {
              g_printerr ("Failed to set speed=%d for %s encoder: %s", parameter_number, encoder_name, err.message);
            }

        }
      else if (g_strcmp0 (encoder_name, "rav1e") == 0) /* Rav1e encoder */
        {
          switch (encoder_speed_av1)
            {
            case HEIFPLUGIN_ENCODER_SPEED_SLOW:
              parameter_number = 6;
              break;
            case HEIFPLUGIN_ENCODER_SPEED_FASTER:
              parameter_number = 10;
              break;
            default: /*  HEIFPLUGIN_ENCODER_SPEED_BALANCED */
              parameter_number = 8;
              break;
            }

          err = heif_encoder_set_parameter_integer (encoder, "speed", parameter_number);
          if (err.code != 0)
            {
              g_printerr ("Failed to set speed=%d for %s encoder: %s", parameter_number, encoder_name, err.message);
            }

        }
      else
        {
          g_printerr ("Parameters not set, unsupported AV1 encoder: %s", encoder_name);
        }
    }

  if (pixel_format == HEIFPLUGIN_EXPORT_FORMAT_RGB)
    {
      encoder_options = heif_encoding_options_alloc ();
      encoder_options->save_two_colr_boxes_when_ICC_and_nclx_available = 1;
    }

  err = heif_context_encode_image (context,
                                   h_image,
                                   encoder,
                                   encoder_options,
                                   &handle);

  if (encoder_options)
    {
      heif_encoding_options_free (encoder_options);
    }

  if (err.code != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Encoding HEIF image failed: %s"),
                   err.message);
      heif_encoder_release (encoder);
      heif_image_release (h_image);
      heif_context_free (context);
      return FALSE;
    }

  if (metadata && (save_exif || save_xmp))
    {
      GimpMetadata         *filtered_metadata;
      GimpMetadataSaveFlags metadata_flags = 0;

      if (save_exif)
        {
          metadata_flags |= GIMP_METADATA_SAVE_EXIF;
        }

      if (save_xmp)
        {
          metadata_flags |= GIMP_METADATA_SAVE_XMP;
        }

      filtered_metadata = gimp_image_metadata_save_filter (image, "image/heif", metadata, metadata_flags, NULL, error);
      if(! filtered_metadata)
        {
          if (error && *error)
            {
              g_printerr ("%s: error filtering metadata: %s",
                          G_STRFUNC, (*error)->message);
              g_clear_error (error);
            }
        }
      else
        {
          GExiv2Metadata *filtered_g2metadata = GEXIV2_METADATA (filtered_metadata);

          /*  EXIF metadata  */
          if (save_exif && gexiv2_metadata_has_exif (filtered_g2metadata))
            {
              GBytes *raw_exif_data;

              raw_exif_data = gexiv2_metadata_get_exif_data (filtered_g2metadata, GEXIV2_BYTE_ORDER_LITTLE, error);
              if (raw_exif_data)
                {
                  gsize exif_size = 0;
                  gconstpointer exif_buffer = g_bytes_get_data (raw_exif_data, &exif_size);

                  if (exif_size >= 4)
                    {
                      err = heif_context_add_exif_metadata (context, handle,
                                                            exif_buffer, exif_size);
                      if (err.code != 0)
                        {
                          g_printerr ("Failed to save EXIF metadata: %s", err.message);
                        }
                    }
                  g_bytes_unref (raw_exif_data);
                }
              else
                {
                  if (error && *error)
                    {
                      g_printerr ("%s: error preparing EXIF metadata: %s",
                                  G_STRFUNC, (*error)->message);
                      g_clear_error (error);
                    }
                }
            }

          /*  XMP metadata  */
          if (save_xmp && gexiv2_metadata_has_xmp (filtered_g2metadata))
            {
              gchar *xmp_packet;

              xmp_packet = gexiv2_metadata_try_generate_xmp_packet (filtered_g2metadata, GEXIV2_USE_COMPACT_FORMAT | GEXIV2_OMIT_ALL_FORMATTING, 0, NULL);
              if (xmp_packet)
                {
                  int xmp_size = strlen (xmp_packet);
                  if (xmp_size > 0)
                    {
                      heif_context_add_XMP_metadata (context, handle,
                                                     xmp_packet, xmp_size);
                    }
                  g_free (xmp_packet);
                }
            }

          g_object_unref (filtered_metadata);
        }
    }

  heif_image_handle_release (handle);

  gimp_progress_update (0.66);

  writer.writer_api_version = 1;
  writer.write              = write_callback;

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (! output)
    {
      heif_encoder_release (encoder);
      heif_image_release (h_image);
      heif_context_free (context);
      return FALSE;
    }

  err = heif_context_write (context, &writer, output);

  if (err.code != 0)
    {
      GCancellable *cancellable = g_cancellable_new ();

      g_cancellable_cancel (cancellable);
      g_output_stream_close (output, cancellable, NULL);
      g_object_unref (cancellable);

      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Writing HEIF image failed: %s"),
                   err.message);

      heif_encoder_release (encoder);
      heif_image_release (h_image);
      heif_context_free (context);
      return FALSE;
    }

  g_object_unref (output);

  heif_encoder_release (encoder);
  heif_image_release (h_image);
  heif_context_free (context);

  gimp_progress_update (1.0);

  return TRUE;
}


/*  the load dialog  */

#define MAX_THUMBNAIL_SIZE 320

typedef struct _HeifImage HeifImage;

struct _HeifImage
{
  uint32_t           ID;
  gchar              caption[100];
  struct heif_image *thumbnail;
  gint               width;
  gint               height;
};

static gboolean
load_thumbnails (struct heif_context *heif,
                 HeifImage           *images)
{
  guint32 *IDs;
  gint     n_images;
  gint     i;

  n_images = heif_context_get_number_of_top_level_images (heif);

  /* get list of all (top level) image IDs */

  IDs = g_alloca (n_images * sizeof (guint32));

  heif_context_get_list_of_top_level_image_IDs (heif, IDs, n_images);


  /* Load a thumbnail for each image. */

  for (i = 0; i < n_images; i++)
    {
      struct heif_image_handle *handle = NULL;
      struct heif_error         err;
      gint                      width;
      gint                      height;
      struct heif_image_handle *thumbnail_handle = NULL;
      heif_item_id              thumbnail_ID;
      gint                      n_thumbnails;
      struct heif_image        *thumbnail_img = NULL;
      gint                      thumbnail_width;
      gint                      thumbnail_height;

      images[i].ID         = IDs[i];
      images[i].caption[0] = 0;
      images[i].thumbnail  = NULL;

      /* get image handle */

      err = heif_context_get_image_handle (heif, IDs[i], &handle);
      if (err.code)
        {
          gimp_message (err.message);
          continue;
        }

      /* generate image caption */

      width  = heif_image_handle_get_width  (handle);
      height = heif_image_handle_get_height (handle);

      if (heif_image_handle_is_primary_image (handle))
        {
          g_snprintf (images[i].caption, sizeof (images[i].caption),
                      "%dx%d (%s)", width, height, _("primary"));
        }
      else
        {
          g_snprintf (images[i].caption, sizeof (images[i].caption),
                      "%dx%d", width, height);
        }

      /* get handle to thumbnail image
       *
       * if there is no thumbnail image, just the the image itself
       * (will be scaled down later)
       */

      n_thumbnails = heif_image_handle_get_list_of_thumbnail_IDs (handle,
                                                                  &thumbnail_ID,
                                                                  1);

      if (n_thumbnails > 0)
        {
          err = heif_image_handle_get_thumbnail (handle, thumbnail_ID,
                                                 &thumbnail_handle);
          if (err.code)
            {
              gimp_message (err.message);
              continue;
            }
        }
      else
        {
          err = heif_context_get_image_handle (heif, IDs[i], &thumbnail_handle);
          if (err.code)
            {
              gimp_message (err.message);
              continue;
            }
        }

      /* decode the thumbnail image */

      err = heif_decode_image (thumbnail_handle,
                               &thumbnail_img,
                               heif_colorspace_RGB,
                               heif_chroma_interleaved_RGB,
                               NULL);
      if (err.code)
        {
          gimp_message (err.message);
          continue;
        }

      /* if thumbnail image size exceeds the maximum, scale it down */

      thumbnail_width  = heif_image_handle_get_width  (thumbnail_handle);
      thumbnail_height = heif_image_handle_get_height (thumbnail_handle);

      if (thumbnail_width  > MAX_THUMBNAIL_SIZE ||
          thumbnail_height > MAX_THUMBNAIL_SIZE)
        {
          /* compute scaling factor to fit into a max sized box */

          gfloat factor_h = thumbnail_width  / (gfloat) MAX_THUMBNAIL_SIZE;
          gfloat factor_v = thumbnail_height / (gfloat) MAX_THUMBNAIL_SIZE;
          gint   new_width, new_height;
          struct heif_image *scaled_img = NULL;

          if (factor_v > factor_h)
            {
              new_height = MAX_THUMBNAIL_SIZE;
              new_width  = thumbnail_width / factor_v;
            }
          else
            {
              new_height = thumbnail_height / factor_h;
              new_width  = MAX_THUMBNAIL_SIZE;
            }

          /* scale the image */

          err = heif_image_scale_image (thumbnail_img,
                                        &scaled_img,
                                        new_width, new_height,
                                        NULL);
          if (err.code)
            {
              gimp_message (err.message);
              continue;
            }

          /* release the old image and only keep the scaled down version */

          heif_image_release (thumbnail_img);
          thumbnail_img = scaled_img;

          thumbnail_width  = new_width;
          thumbnail_height = new_height;
        }

      heif_image_handle_release (thumbnail_handle);
      heif_image_handle_release (handle);

      /* remember the HEIF thumbnail image (we need it for the GdkPixbuf) */

      images[i].thumbnail = thumbnail_img;

      images[i].width  = thumbnail_width;
      images[i].height = thumbnail_height;
    }

  return TRUE;
}

static void
load_dialog_item_activated (GtkIconView *icon_view,
                            GtkTreePath *path,
                            GtkDialog   *dialog)
{
  gtk_dialog_response (dialog, GTK_RESPONSE_OK);
}

static gboolean
load_dialog (struct heif_context *heif,
             uint32_t            *selected_image)
{
  GtkWidget       *dialog;
  GtkWidget       *main_vbox;
  GtkWidget       *frame;
  HeifImage       *heif_images;
  GtkListStore    *list_store;
  GtkTreeIter      iter;
  GtkWidget       *scrolled_window;
  GtkWidget       *icon_view;
  GtkCellRenderer *renderer;
  gint             n_images;
  gint             i;
  gint             selected_idx = -1;
  gboolean         run          = FALSE;

  n_images = heif_context_get_number_of_top_level_images (heif);

  heif_images = g_alloca (n_images * sizeof (HeifImage));

  if (! load_thumbnails (heif, heif_images))
    return FALSE;

  dialog = gimp_dialog_new (_("Load HEIF Image"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, LOAD_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);

  frame = gimp_frame_new (_("Select Image"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* prepare list store with all thumbnails and caption */

  list_store = gtk_list_store_new (2, G_TYPE_STRING, GDK_TYPE_PIXBUF);

  for (i = 0; i < n_images; i++)
    {
      GdkPixbuf    *pixbuf;
      const guint8 *data;
      gint          stride;

      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter, 0, heif_images[i].caption, -1);

      data = heif_image_get_plane_readonly (heif_images[i].thumbnail,
                                            heif_channel_interleaved,
                                            &stride);

      pixbuf = gdk_pixbuf_new_from_data (data,
                                         GDK_COLORSPACE_RGB,
                                         FALSE,
                                         8,
                                         heif_images[i].width,
                                         heif_images[i].height,
                                         stride,
                                         NULL,
                                         NULL);

      gtk_list_store_set (list_store, &iter, 1, pixbuf, -1);
    }

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_widget_set_size_request (scrolled_window,
                               2   * MAX_THUMBNAIL_SIZE,
                               1.5 * MAX_THUMBNAIL_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), scrolled_window);
  gtk_widget_show (scrolled_window);

  icon_view = gtk_icon_view_new_with_model (GTK_TREE_MODEL (list_store));
  gtk_container_add (GTK_CONTAINER (scrolled_window), icon_view);
  gtk_widget_show (icon_view);

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon_view), renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view), renderer,
                                  "pixbuf", 1,
                                  NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon_view), renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view), renderer,
                                  "text", 0,
                                  NULL);
  g_object_set (renderer,
                "alignment", PANGO_ALIGN_CENTER,
                "wrap-mode", PANGO_WRAP_WORD_CHAR,
                "xalign",    0.5,
                "yalign",    0.0,
                NULL);

  g_signal_connect (icon_view, "item-activated",
                    G_CALLBACK (load_dialog_item_activated),
                    dialog);

  /* pre-select the primary image */

  for (i = 0; i < n_images; i++)
    {
      if (heif_images[i].ID == *selected_image)
        {
          selected_idx = i;
          break;
        }
    }

  if (selected_idx != -1)
    {
      GtkTreePath *path = gtk_tree_path_new_from_indices (selected_idx, -1);

      gtk_icon_view_select_path (GTK_ICON_VIEW (icon_view), path);
      gtk_tree_path_free (path);
    }

  gtk_widget_show (main_vbox);
  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      GList *selected_items =
        gtk_icon_view_get_selected_items (GTK_ICON_VIEW (icon_view));

      if (selected_items)
        {
          GtkTreePath *path    = selected_items->data;
          gint        *indices = gtk_tree_path_get_indices (path);

          *selected_image = heif_images[indices[0]].ID;

          g_list_free_full (selected_items,
                            (GDestroyNotify) gtk_tree_path_free);
        }
    }

  gtk_widget_destroy (dialog);

  /* release thumbnail images */

  for (i = 0 ; i < n_images; i++)
    heif_image_release (heif_images[i].thumbnail);

  return run;
}


/*  the save dialog  */

gboolean
save_dialog (GimpProcedure *procedure,
             GObject       *config,
             GimpImage     *image)
{
  GtkWidget    *dialog;
  GtkWidget    *quality_scale;
  GtkListStore *store_bitdepths;
  gboolean      run;

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                    "lossless", GTK_TYPE_CHECK_BUTTON);

  quality_scale = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                                    "quality",
                                                    GIMP_TYPE_SCALE_ENTRY);

  g_object_bind_property (config,        "lossless",
                          quality_scale, "sensitive",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_INVERT_BOOLEAN);

  store_bitdepths = gimp_int_store_new (_("8 bit/channel"),   8,
                                        _("10 bit/channel"), 10,
                                        _("12 bit/channel"), 12,
                                        NULL);

  gimp_procedure_dialog_get_int_combo (GIMP_PROCEDURE_DIALOG (dialog),
                                       "save-bit-depth", GIMP_INT_STORE (store_bitdepths));

  if (g_strcmp0 (gimp_procedure_get_name (procedure), EXPORT_PROC_HEJ2) == 0)
    {
      /* HEJ2 doesn't use encoder-speed */
      gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                  "lossless", "quality",
                                  "pixel-format",
                                  "save-bit-depth",
                                  "include-color-profile",
                                  "include-exif", "include-xmp", NULL);
    }
  else
    {
      gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                  "lossless", "quality",
                                  "pixel-format",
                                  "save-bit-depth", "encoder-speed",
                                  "include-color-profile",
                                  "include-exif", "include-xmp", NULL);
    }

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
