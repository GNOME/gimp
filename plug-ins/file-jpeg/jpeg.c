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

#include <stdio.h>
#include <setjmp.h>
#include <string.h>

#include <jpeglib.h>
#include <jerror.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "jpeg.h"
#include "jpeg-settings.h"
#include "jpeg-load.h"
#include "jpeg-save.h"


typedef struct _Jpeg      Jpeg;
typedef struct _JpegClass JpegClass;

struct _Jpeg
{
  GimpPlugIn      parent_instance;
};

struct _JpegClass
{
  GimpPlugInClass parent_class;
};


#define JPEG_TYPE  (jpeg_get_type ())
#define JPEG (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), JPEG_TYPE, Jpeg))

GType                   jpeg_get_type         (void) G_GNUC_CONST;

static GList          * jpeg_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * jpeg_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * jpeg_load             (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GFile                *file,
                                               const GimpValueArray *args,
                                               gpointer              run_data);
static GimpValueArray * jpeg_load_thumb       (GimpProcedure        *procedure,
                                               GFile                *file,
                                               gint                  size,
                                               const GimpValueArray *args,
                                               gpointer              run_data);
static GimpValueArray * jpeg_save             (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               gint                  n_drawables,
                                               GimpDrawable        **drawables,
                                               GFile                *file,
                                               const GimpValueArray *args,
                                               gpointer              run_data);


G_DEFINE_TYPE (Jpeg, jpeg, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (JPEG_TYPE)


gboolean         undo_touched      = FALSE;
gchar           *image_comment     = NULL;
GimpDisplay     *display           = NULL;
JpegSaveVals     jsvals            = { 0, };
GimpImage       *orig_image_global = NULL;
GimpDrawable    *drawable_global   = NULL;
gint             orig_quality      = 0;
JpegSubsampling  orig_subsmp       = JPEG_SUBSAMPLING_2x2_1x1_1x1;;
gint             num_quant_tables  = 0;


static void
jpeg_class_init (JpegClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = jpeg_query_procedures;
  plug_in_class->create_procedure = jpeg_create_procedure;
}

static void
jpeg_init (Jpeg *jpeg)
{
}

static GList *
jpeg_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static GimpProcedure *
jpeg_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           jpeg_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, N_("JPEG image"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads files in the JPEG file format",
                                        "Loads files in the JPEG file format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball, Peter Mattis & others",
                                      "Spencer Kimball & Peter Mattis",
                                      "1995-2007");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/jpeg");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "jpg,jpeg,jpe");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "6,string,JFIF,6,string,Exif");

      gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = gimp_thumbnail_procedure_new (plug_in, name,
                                                GIMP_PDB_PROC_TYPE_PLUGIN,
                                                jpeg_load_thumb, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Loads a thumbnail from a JPEG image",
                                        "Loads a thumbnail from a JPEG image, "
                                        "if one exists",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Mukund Sivaraman <muks@mukund.org>, "
                                      "Sven Neumann <sven@gimp.org>",
                                      "Mukund Sivaraman <muks@mukund.org>, "
                                      "Sven Neumann <sven@gimp.org>",
                                      "November 15, 2004");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           jpeg_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");

      gimp_procedure_set_menu_label (procedure, N_("JPEG image"));

      gimp_procedure_set_documentation (procedure,
                                        "Saves files in the JPEG file format",
                                        "Saves files in the lossy, widely "
                                        "supported JPEG format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball, Peter Mattis & others",
                                      "Spencer Kimball & Peter Mattis",
                                      "1995-2007");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/jpeg");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "jpg,jpeg,jpe");

      /* See bugs #63610 and #61088 for a discussion about the quality
       * settings
       */
      GIMP_PROC_ARG_DOUBLE (procedure, "quality",
                            "Quality",
                            "Quality of saved image",
                            0.0, 1.0, 0.9,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "smoothing",
                            "Smoothing",
                            "Smoothing factor for saved image",
                            0.0, 1.0, 0.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "optimize",
                             "Optimize",
                             "Use optimized tables during Huffman coding",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "progressive",
                             "Progressive",
                             "Create progressive JPEG images",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING (procedure, "comment",
                            "Comment",
                            "Image comment",
                            gimp_get_default_comment (),
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "sub-sampling",
                         "Sub-sampling",
                         "Sub-sampling type { 0 == 4:2:0 (chroma quartered), "
                         "1 == 4:2:2 Horizontal (chroma halved), "
                         "2 == 4:4:4 (best quality), "
                         "3 == 4:2:2 Vertical (chroma halved)",
                         JPEG_SUBSAMPLING_2x2_1x1_1x1,
                         JPEG_SUBSAMPLING_1x2_1x1_1x1,
                         JPEG_SUBSAMPLING_1x1_1x1_1x1,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "baseline",
                             "Baseline",
                             "Force creation of a baseline JPEG "
                             "(non-baseline JPEGs can't be read by all decoders)",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "restart",
                         "Restart",
                         "Interval of restart markers "
                         "(in MCU rows, 0 = no restart markers)",
                         0, 64, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "dct",
                         "DCT",
                         "DCT method to use { INTEGER (0), FIXED (1), "
                         "FLOAT (2) }",
                         0, 2, 0,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
jpeg_load (GimpProcedure        *procedure,
           GimpRunMode           run_mode,
           GFile                *file,
           const GimpValueArray *args,
           gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  gboolean        interactive;
  gboolean        resolution_loaded = FALSE;
  GError         *error             = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  preview_image = NULL;
  preview_layer = NULL;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);
      interactive = TRUE;
      break;

    default:
      interactive = FALSE;
      break;
    }

  image = load_image (file, run_mode, FALSE,
                      &resolution_loaded, &error);

  if (image)
    {
      GimpMetadata *metadata;

      metadata = gimp_image_metadata_load_prepare (image, "image/jpeg",
                                                   file, NULL);

      if (metadata)
        {
          GimpMetadataLoadFlags flags = GIMP_METADATA_LOAD_ALL;

          if (resolution_loaded)
            flags &= ~GIMP_METADATA_LOAD_RESOLUTION;

          gimp_image_metadata_load_finish (image, "image/jpeg",
                                           metadata, flags,
                                           interactive);

          g_object_unref (metadata);
        }
    }

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
jpeg_load_thumb (GimpProcedure        *procedure,
                 GFile                *file,
                 gint                  size,
                 const GimpValueArray *args,
                 gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  gint            width  = 0;
  gint            height = 0;
  GimpImageType   type   = -1;
  GError         *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  preview_image = NULL;
  preview_layer = NULL;

  image = load_thumbnail_image (file, &width, &height, &type,
                                &error);


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
  GIMP_VALUES_SET_ENUM  (return_vals, 4, type);
  GIMP_VALUES_SET_INT   (return_vals, 5, 1); /* 1 layer */

  return return_vals;
}

static GimpValueArray *
jpeg_save (GimpProcedure        *procedure,
           GimpRunMode           run_mode,
           GimpImage            *image,
           gint                  n_drawables,
           GimpDrawable        **drawables,
           GFile                *file,
           const GimpValueArray *args,
           gpointer              run_data)
{
  GimpPDBStatusType      status = GIMP_PDB_SUCCESS;
  GimpParasite          *parasite;
  GimpMetadata          *metadata;
  GimpMetadataSaveFlags  metadata_flags;
  GimpImage             *orig_image;
  GimpExportReturn       export = GIMP_EXPORT_CANCEL;
  GError                *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  preview_image = NULL;
  preview_layer = NULL;

  orig_image = image;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);

      export = gimp_export_image (&image, &n_drawables, &drawables, "JPEG",
                                  GIMP_EXPORT_CAN_HANDLE_RGB |
                                  GIMP_EXPORT_CAN_HANDLE_GRAY);

      switch (export)
        {
        case GIMP_EXPORT_EXPORT:
          {
            gchar *tmp = g_filename_from_utf8 (_("Export Preview"), -1,
                                               NULL, NULL, NULL);
            if (tmp)
              {
                GFile *file = g_file_new_for_path (tmp);
                gimp_image_set_file (image, file);
                g_object_unref (file);
                g_free (tmp);
              }

            display = NULL;
          }
          break;

        case GIMP_EXPORT_IGNORE:
          break;

        case GIMP_EXPORT_CANCEL:
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CANCEL,
                                                   NULL);
          break;
        }
      break;

    default:
      break;
    }

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("JPEG format does not support multiple layers."));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }

  /* Initialize with hardcoded defaults */
  load_defaults ();

  /* Override the defaults with preferences. */
  metadata = gimp_image_metadata_save_prepare (orig_image,
                                               "image/jpeg",
                                               &metadata_flags);
  jsvals.save_exif      = (metadata_flags & GIMP_METADATA_SAVE_EXIF) != 0;
  jsvals.save_xmp       = (metadata_flags & GIMP_METADATA_SAVE_XMP) != 0;
  jsvals.save_iptc      = (metadata_flags & GIMP_METADATA_SAVE_IPTC) != 0;
  jsvals.save_thumbnail = (metadata_flags & GIMP_METADATA_SAVE_THUMBNAIL) != 0;
  jsvals.save_profile   = (metadata_flags & GIMP_METADATA_SAVE_COLOR_PROFILE) != 0;

  parasite = gimp_image_get_parasite (orig_image, "gimp-comment");
  if (parasite)
    {
      image_comment = g_strndup (gimp_parasite_data (parasite),
                                 gimp_parasite_data_size (parasite));
      gimp_parasite_free (parasite);
    }

  /* Override preferences from JPG export defaults (if saved). */
  load_parasite ();

  switch (run_mode)
    {
    case GIMP_RUN_NONINTERACTIVE:
      g_free (image_comment);

      jsvals.quality     = GIMP_VALUES_GET_DOUBLE  (args, 0) * 100.0;
      jsvals.smoothing   = GIMP_VALUES_GET_DOUBLE  (args, 1);
      jsvals.optimize    = GIMP_VALUES_GET_BOOLEAN (args, 2);
      jsvals.progressive = GIMP_VALUES_GET_BOOLEAN (args, 3);
      image_comment      = GIMP_VALUES_DUP_STRING  (args, 4);
      jsvals.subsmp      = GIMP_VALUES_GET_DOUBLE  (args, 5);
      jsvals.baseline    = GIMP_VALUES_GET_DOUBLE  (args, 6);
      jsvals.restart     = GIMP_VALUES_GET_DOUBLE  (args, 7);
      jsvals.dct         = GIMP_VALUES_GET_DOUBLE  (args, 8);
      jsvals.preview     = FALSE;
      break;

    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      /* restore the values found when loading the file (if available) */
      jpeg_restore_original_settings (orig_image,
                                      &orig_quality,
                                      &orig_subsmp,
                                      &num_quant_tables);

      /* load up the previously used values (if file was saved once) */
      parasite = gimp_image_get_parasite (orig_image,
                                          "jpeg-save-options");
      if (parasite)
        {
          const JpegSaveVals *save_vals = gimp_parasite_data (parasite);

          jsvals.quality          = save_vals->quality;
          jsvals.smoothing        = save_vals->smoothing;
          jsvals.optimize         = save_vals->optimize;
          jsvals.progressive      = save_vals->progressive;
          jsvals.baseline         = save_vals->baseline;
          jsvals.subsmp           = save_vals->subsmp;
          jsvals.restart          = save_vals->restart;
          jsvals.dct              = save_vals->dct;
          jsvals.preview          = save_vals->preview;
          jsvals.save_exif        = save_vals->save_exif;
          jsvals.save_thumbnail   = save_vals->save_thumbnail;
          jsvals.save_xmp         = save_vals->save_xmp;
          jsvals.save_iptc        = save_vals->save_iptc;
          jsvals.use_orig_quality = save_vals->use_orig_quality;

          gimp_parasite_free (parasite);
        }
      else
        {
          /* We are called with GIMP_RUN_WITH_LAST_VALS but this image
           * doesn't have a "jpeg-save-options" parasite. It's better
           * to prompt the user with a dialog now so that she has
           * control over the JPEG encoding parameters.
           */
          run_mode = GIMP_RUN_INTERACTIVE;

          /* If this image was loaded from a JPEG file and has not
           * been saved yet, try to use some of the settings from the
           * original file if they are better than the default values.
           */
          if (orig_quality > jsvals.quality)
            {
              jsvals.quality = orig_quality;
            }

          /* Skip changing subsampling to original if we already have
           * best setting or if original have worst setting
           */
          if (!(jsvals.subsmp == JPEG_SUBSAMPLING_1x1_1x1_1x1 ||
                orig_subsmp == JPEG_SUBSAMPLING_2x2_1x1_1x1))
            {
              jsvals.subsmp = orig_subsmp;
            }

          if (orig_quality == jsvals.quality &&
              orig_subsmp == jsvals.subsmp)
            {
              jsvals.use_orig_quality = TRUE;
            }
        }
      break;
    }

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (jsvals.preview)
        {
          /* we freeze undo saving so that we can avoid sucking up
           * tile cache with our unneeded preview steps. */
          gimp_image_undo_freeze (image);

          undo_touched = TRUE;
        }

      /* prepare for the preview */
      preview_image     = image;
      orig_image_global = orig_image;
      drawable_global   = drawables[0];

      /*  First acquire information with a dialog  */
      if (! save_dialog (drawables[0]))
        {
          status = GIMP_PDB_CANCEL;
        }

      if (undo_touched)
        {
          /* thaw undo saving and flush the displays to have them
           * reflect the current shortcuts
           */
          gimp_image_undo_thaw (image);
          gimp_displays_flush ();
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! save_image (file, image, drawables[0], orig_image, FALSE,
                        &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    {
      /* If the image was exported, delete the new display. This also
       * deletes the image.
       */
      if (display)
        gimp_display_delete (display);
      else
        gimp_image_delete (image);

      g_free (drawables);
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /* pw - now we need to change the defaults to be whatever was
       * used to save this image.  Dump the old parasites and add new
       * ones.
       */

      gimp_image_detach_parasite (orig_image, "gimp-comment");
      if (image_comment && strlen (image_comment))
        {
          parasite = gimp_parasite_new ("gimp-comment",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (image_comment) + 1,
                                        image_comment);
          gimp_image_attach_parasite (orig_image, parasite);
          gimp_parasite_free (parasite);
        }

      parasite = gimp_parasite_new ("jpeg-save-options",
                                    0, sizeof (jsvals), &jsvals);
      gimp_image_attach_parasite (orig_image, parasite);
      gimp_parasite_free (parasite);

      /* write metadata */

      if (metadata)
        {
          gimp_metadata_set_bits_per_sample (metadata, 8);

          if (jsvals.save_exif)
            metadata_flags |= GIMP_METADATA_SAVE_EXIF;
          else
            metadata_flags &= ~GIMP_METADATA_SAVE_EXIF;

          if (jsvals.save_xmp)
            metadata_flags |= GIMP_METADATA_SAVE_XMP;
          else
            metadata_flags &= ~GIMP_METADATA_SAVE_XMP;

          if (jsvals.save_iptc)
            metadata_flags |= GIMP_METADATA_SAVE_IPTC;
          else
            metadata_flags &= ~GIMP_METADATA_SAVE_IPTC;

          if (jsvals.save_thumbnail)
            metadata_flags |= GIMP_METADATA_SAVE_THUMBNAIL;
          else
            metadata_flags &= ~GIMP_METADATA_SAVE_THUMBNAIL;

          if (jsvals.save_profile)
            metadata_flags |= GIMP_METADATA_SAVE_COLOR_PROFILE;
          else
            metadata_flags &= ~GIMP_METADATA_SAVE_COLOR_PROFILE;

          gimp_image_metadata_save_finish (orig_image,
                                           "image/jpeg",
                                           metadata, metadata_flags,
                                           file, NULL);
        }
    }

  if (metadata)
    g_object_unref (metadata);

  return gimp_procedure_new_return_values (procedure, status, error);
}

/*
 * Here's the routine that will replace the standard error_exit method:
 */

void
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp (myerr->setjmp_buffer, 1);
}


void
my_output_message (j_common_ptr cinfo)
{
  gchar  buffer[JMSG_LENGTH_MAX + 1];

  (*cinfo->err->format_message)(cinfo, buffer);

  g_message ("%s", buffer);
}
