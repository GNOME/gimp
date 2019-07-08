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

/* Declare local functions.
 */

static void  query (void);
static void  run   (const gchar      *name,
                    gint              nparams,
                    const GimpParam  *param,
                    gint             *nreturn_vals,
                    GimpParam       **return_vals);

gboolean         undo_touched;
gboolean         load_interactive;
gchar           *image_comment;
gint32           display_ID;
JpegSaveVals     jsvals;
gint32           orig_image_ID_global;
gint32           drawable_ID_global;
gint             orig_quality;
JpegSubsampling  orig_subsmp;
gint             num_quant_tables;


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
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to load" }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,   "image",         "Output image" }
  };

  static const GimpParamDef thumb_args[] =
  {
    { GIMP_PDB_STRING, "filename",     "The name of the file to load"  },
    { GIMP_PDB_INT32,  "thumb-size",   "Preferred thumbnail size"      }
  };
  static const GimpParamDef thumb_return_vals[] =
  {
    { GIMP_PDB_IMAGE,  "image",        "Thumbnail image"               },
    { GIMP_PDB_INT32,  "image-width",  "Width of full-sized image"     },
    { GIMP_PDB_INT32,  "image-height", "Height of full-sized image"    }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" },
    { GIMP_PDB_FLOAT,    "quality",      "Quality of saved image (0 <= quality <= 1)" },
    { GIMP_PDB_FLOAT,    "smoothing",    "Smoothing factor for saved image (0 <= smoothing <= 1)" },
    { GIMP_PDB_INT32,    "optimize",     "Use optimized tables during Huffman coding (0/1)" },
    { GIMP_PDB_INT32,    "progressive",  "Create progressive JPEG images (0/1)" },
    { GIMP_PDB_STRING,   "comment",      "Image comment" },
    { GIMP_PDB_INT32,    "subsmp",       "Sub-sampling type { 0, 1, 2, 3 } 0 == 4:2:0 (chroma quartered), 1 == 4:2:2 Horizontal (chroma halved), 2 == 4:4:4 (best quality), 3 == 4:2:2 Vertical (chroma halved)" },
    { GIMP_PDB_INT32,    "baseline",     "Force creation of a baseline JPEG (non-baseline JPEGs can't be read by all decoders) (0/1)" },
    { GIMP_PDB_INT32,    "restart",      "Interval of restart markers (in MCU rows, 0 = no restart markers)" },
    { GIMP_PDB_INT32,    "dct",          "DCT method to use { INTEGER (0), FIXED (1), FLOAT (2) }" }
  };

  gimp_install_procedure (LOAD_PROC,
                          "loads files in the JPEG file format",
                          "loads files in the JPEG file format",
                          "Spencer Kimball, Peter Mattis & others",
                          "Spencer Kimball & Peter Mattis",
                          "1995-2007",
                          N_("JPEG image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/jpeg");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "jpg,jpeg,jpe",
                                    "",
                                    "6,string,JFIF,6,string,Exif");

  gimp_install_procedure (LOAD_THUMB_PROC,
                          "Loads a thumbnail from a JPEG image",
                          "Loads a thumbnail from a JPEG image (only if it exists)",
                          "Mukund Sivaraman <muks@mukund.org>, Sven Neumann <sven@gimp.org>",
                          "Mukund Sivaraman <muks@mukund.org>, Sven Neumann <sven@gimp.org>",
                          "November 15, 2004",
                          NULL,
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (thumb_args),
                          G_N_ELEMENTS (thumb_return_vals),
                          thumb_args, thumb_return_vals);

  gimp_register_thumbnail_loader (LOAD_PROC, LOAD_THUMB_PROC);

  gimp_install_procedure (SAVE_PROC,
                          "saves files in the JPEG file format",
                          "saves files in the lossy, widely supported JPEG format",
                          "Spencer Kimball, Peter Mattis & others",
                          "Spencer Kimball & Peter Mattis",
                          "1995-2007",
                          N_("JPEG image"),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/jpeg");
  gimp_register_save_handler (SAVE_PROC, "jpg,jpeg,jpe", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[6];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint32             image_ID;
  gint32             drawable_ID;
  GimpParasite      *parasite;
  GError            *error  = NULL;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  preview_image_ID = -1;
  preview_layer_ID = -1;

  orig_quality = 0;
  orig_subsmp = JPEG_SUBSAMPLING_2x2_1x1_1x1;
  num_quant_tables = 0;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      gboolean resolution_loaded = FALSE;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);
          load_interactive = TRUE;
          break;
        default:
          load_interactive = FALSE;
          break;
        }

      image_ID = load_image (param[1].data.d_string, run_mode, FALSE,
                             &resolution_loaded, &error);

      if (image_ID != -1)
        {
          GFile        *file = g_file_new_for_path (param[1].data.d_string);
          GimpMetadata *metadata;

          metadata = gimp_image_metadata_load_prepare (image_ID, "image/jpeg",
                                                       file, NULL);

          if (metadata)
            {
              GimpMetadataLoadFlags flags = GIMP_METADATA_LOAD_ALL;

              if (resolution_loaded)
                flags &= ~GIMP_METADATA_LOAD_RESOLUTION;

              gimp_image_metadata_load_finish (image_ID, "image/jpeg",
                                               metadata, flags,
                                               load_interactive);

              g_object_unref (metadata);
            }

          g_object_unref (file);

          *nreturn_vals = 2;
          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, LOAD_THUMB_PROC) == 0)
    {
      if (nparams < 2)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          GFile        *file   = g_file_new_for_path (param[0].data.d_string);
          gint          width  = 0;
          gint          height = 0;
          GimpImageType type   = -1;

          image_ID = load_thumbnail_image (file, &width, &height, &type,
                                           &error);

          g_object_unref (file);

          if (image_ID != -1)
            {
              *nreturn_vals = 6;
              values[1].type         = GIMP_PDB_IMAGE;
              values[1].data.d_image = image_ID;
              values[2].type         = GIMP_PDB_INT32;
              values[2].data.d_int32 = width;
              values[3].type         = GIMP_PDB_INT32;
              values[3].data.d_int32 = height;
              values[4].type         = GIMP_PDB_INT32;
              values[4].data.d_int32 = type;
              values[5].type         = GIMP_PDB_INT32;
              values[5].data.d_int32 = 1; /* num_layers */
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }
    }
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      GimpMetadata          *metadata;
      GimpMetadataSaveFlags  metadata_flags;
      gint32                 orig_image_ID;
      GimpExportReturn       export = GIMP_EXPORT_CANCEL;

      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      orig_image_ID = image_ID;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          export = gimp_export_image (&image_ID, &drawable_ID, "JPEG",
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
                    gimp_image_set_filename (image_ID, tmp);
                    g_free (tmp);
                  }

                display_ID = -1;
              }
              break;

            case GIMP_EXPORT_IGNORE:
              break;

            case GIMP_EXPORT_CANCEL:
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
              break;
            }
          break;

        default:
          break;
        }

      /* Initialize with hardcoded defaults */
      load_defaults ();

      /* Override the defaults with preferences. */
      metadata = gimp_image_metadata_save_prepare (orig_image_ID,
                                                   "image/jpeg",
                                                   &metadata_flags);
      jsvals.save_exif      = (metadata_flags & GIMP_METADATA_SAVE_EXIF) != 0;
      jsvals.save_xmp       = (metadata_flags & GIMP_METADATA_SAVE_XMP) != 0;
      jsvals.save_iptc      = (metadata_flags & GIMP_METADATA_SAVE_IPTC) != 0;
      jsvals.save_thumbnail = (metadata_flags & GIMP_METADATA_SAVE_THUMBNAIL) != 0;
      jsvals.save_profile   = (metadata_flags & GIMP_METADATA_SAVE_COLOR_PROFILE) != 0;

      parasite = gimp_image_get_parasite (orig_image_ID, "gimp-comment");
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
          /*  Make sure all the arguments are there!  */
          /*  pw - added two more progressive and comment */
          /*  sg - added subsampling, preview, baseline, restarts and DCT */
          if (nparams != 14)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              /* Once the PDB gets default parameters, remove this hack */
              if (param[5].data.d_float >= 0.01)
                {
                  jsvals.quality     = 100.0 * param[5].data.d_float;
                  jsvals.smoothing   = param[6].data.d_float;
                  jsvals.optimize    = param[7].data.d_int32;
                  jsvals.progressive = param[8].data.d_int32;
                  jsvals.baseline    = param[11].data.d_int32;
                  jsvals.subsmp      = param[10].data.d_int32;
                  jsvals.restart     = param[12].data.d_int32;
                  jsvals.dct         = param[13].data.d_int32;

                  /* free up the default -- wasted some effort earlier */
                  g_free (image_comment);
                  image_comment = g_strdup (param[9].data.d_string);
                }

              jsvals.preview = FALSE;

              if (jsvals.quality < 0.0 || jsvals.quality > 100.0)
                status = GIMP_PDB_CALLING_ERROR;
              else if (jsvals.smoothing < 0.0 || jsvals.smoothing > 1.0)
                status = GIMP_PDB_CALLING_ERROR;
              else if (jsvals.subsmp < 0 || jsvals.subsmp > 3)
                status = GIMP_PDB_CALLING_ERROR;
              else if (jsvals.dct < 0 || jsvals.dct > 2)
                status = GIMP_PDB_CALLING_ERROR;
            }
          break;

        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          /* restore the values found when loading the file (if available) */
          jpeg_restore_original_settings (orig_image_ID,
                                          &orig_quality,
                                          &orig_subsmp,
                                          &num_quant_tables);

          /* load up the previously used values (if file was saved once) */
          parasite = gimp_image_get_parasite (orig_image_ID,
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
               * to prompt the user with a dialog now so that she has control
               * over the JPEG encoding parameters.
               */
              run_mode = GIMP_RUN_INTERACTIVE;

              /* If this image was loaded from a JPEG file and has not been
               * saved yet, try to use some of the settings from the
               * original file if they are better than the default values.
               */
              if (orig_quality > jsvals.quality)
                {
                  jsvals.quality = orig_quality;
                }

              /* Skip changing subsampling to original if we already have best
               * setting or if original have worst setting */
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
              gimp_image_undo_freeze (image_ID);

              undo_touched = TRUE;
            }

          /* prepare for the preview */
          preview_image_ID = image_ID;
          orig_image_ID_global = orig_image_ID;
          drawable_ID_global = drawable_ID;

          /*  First acquire information with a dialog  */
          status = (save_dialog () ? GIMP_PDB_SUCCESS : GIMP_PDB_CANCEL);

          if (undo_touched)
            {
              /* thaw undo saving and flush the displays to have them
               * reflect the current shortcuts */
              gimp_image_undo_thaw (image_ID);
              gimp_displays_flush ();
            }
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          if (! save_image (param[3].data.d_string,
                            image_ID, drawable_ID, orig_image_ID, FALSE,
                            &error))
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }

      if (export == GIMP_EXPORT_EXPORT)
        {
          /* If the image was exported, delete the new display. */
          /* This also deletes the image.
           */

          if (display_ID != -1)
            gimp_display_delete (display_ID);
          else
            gimp_image_delete (image_ID);
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          /* pw - now we need to change the defaults to be whatever
           * was used to save this image.  Dump the old parasites
           * and add new ones.
           */

          gimp_image_detach_parasite (orig_image_ID, "gimp-comment");
          if (image_comment && strlen (image_comment))
            {
              parasite = gimp_parasite_new ("gimp-comment",
                                            GIMP_PARASITE_PERSISTENT,
                                            strlen (image_comment) + 1,
                                            image_comment);
              gimp_image_attach_parasite (orig_image_ID, parasite);
              gimp_parasite_free (parasite);
            }

          parasite = gimp_parasite_new ("jpeg-save-options",
                                        0, sizeof (jsvals), &jsvals);
          gimp_image_attach_parasite (orig_image_ID, parasite);
          gimp_parasite_free (parasite);

          /* write metadata */

          if (metadata)
            {
              GFile *file;

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

              file = g_file_new_for_path (param[3].data.d_string);
              gimp_image_metadata_save_finish (orig_image_ID,
                                               "image/jpeg",
                                               metadata, metadata_flags,
                                               file, NULL);
              g_object_unref (file);
            }
        }

      if (metadata)
        g_object_unref (metadata);
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
