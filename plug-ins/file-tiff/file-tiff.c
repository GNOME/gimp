/* tiff loading for GIMP
 *  -Peter Mattis
 *
 * The TIFF loading code has been completely revamped by Nick Lamb
 * njl195@zepler.org.uk -- 18 May 1998
 * And it now gains support for tiles (and doubtless a zillion bugs)
 * njl195@zepler.org.uk -- 12 June 1999
 * LZW patent fuss continues :(
 * njl195@zepler.org.uk -- 20 April 2000
 * The code for this filter is based on "tifftopnm" and "pnmtotiff",
 *  2 programs that are a part of the netpbm package.
 * khk@khk.net -- 13 May 2000
 * Added support for ICCPROFILE tiff tag. If this tag is present in a
 * TIFF file, then a parasite is created and vice versa.
 * peter@kirchgessner.net -- 29 Oct 2002
 * Progress bar only when run interactive
 * Added support for layer offsets - pablo.dangelo@web.de -- 7 Jan 2004
 * Honor EXTRASAMPLES tag while loading images with alphachannel
 * pablo.dangelo@web.de -- 16 Jan 2004
 */

/*
 * tifftopnm.c - converts a Tagged Image File to a portable anymap
 *
 * Derived by Jef Poskanzer from tif2ras.c, which is:
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * Author: Patrick J. Naughton
 * naughton@wind.sun.com
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 */

#include "config.h"

#include <tiffio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "file-tiff-load.h"
#include "file-tiff-save.h"

#include "libgimp/stdplugins-intl.h"


#define SAVE_PROC      "file-tiff-save"
#define SAVE2_PROC     "file-tiff-save2"
#define PLUG_IN_BINARY "file-tiff"


typedef struct _Tiff      Tiff;
typedef struct _TiffClass TiffClass;

struct _Tiff
{
  GimpPlugIn      parent_instance;
};

struct _TiffClass
{
  GimpPlugInClass parent_class;
};


#define TIFF_TYPE  (tiff_get_type ())
#define TIFF (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TIFF_TYPE, Tiff))

GType                   tiff_get_type         (void) G_GNUC_CONST;

static GList          * tiff_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * tiff_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * tiff_load             (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GFile                *file,
                                               const GimpValueArray *args,
                                               gpointer              run_data);
static GimpValueArray * tiff_save             (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               gint                  n_drawables,
                                               GimpDrawable        **drawables,
                                               GFile                *file,
                                               const GimpValueArray *args,
                                               gpointer              run_data);

static gboolean         image_is_monochrome  (GimpImage            *image);
static gboolean         image_is_multi_layer (GimpImage            *image);


G_DEFINE_TYPE (Tiff, tiff, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (TIFF_TYPE)


static TiffSaveVals tsvals =
{
  COMPRESSION_NONE,    /*  compression         */
  TRUE,                /*  alpha handling      */
  FALSE,               /*  save transp. pixels */
  FALSE,               /*  save exif           */
  FALSE,               /*  save xmp            */
  FALSE,               /*  save iptc           */
  TRUE,                /*  save thumbnail      */
  TRUE,                /*  save profile        */
  TRUE                 /*  save layer          */
};

static gchar *image_comment = NULL;


static void
tiff_class_init (TiffClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = tiff_query_procedures;
  plug_in_class->create_procedure = tiff_create_procedure;
}

static void
tiff_init (Tiff *tiff)
{
}

static GList *
tiff_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static GimpProcedure *
tiff_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           tiff_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, N_("TIFF image"));

      gimp_procedure_set_documentation (procedure,
                                        "loads files of the tiff file format",
                                        "FIXME: write help for tiff_load",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball, Peter Mattis & Nick Lamb",
                                      "Nick Lamb <njl195@zepler.org.uk>",
                                      "1995-1996,1998-2003");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/tiff");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "tif,tiff");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,II*\\0,0,string,MM\\0*");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           tiff_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("TIFF image"));

      gimp_procedure_set_documentation (procedure,
                                        "Saves files in the tiff file format",
                                        "Saves files in the Tagged Image File "
                                        "Format. The value for the saved "
                                        "comment is taken from the "
                                        "'gimp-comment' parasite",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis",
                                      "Spencer Kimball & Peter Mattis",
                                      "1995-1996,2000-2003");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/tiff");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "tif,tiff");

      GIMP_PROC_ARG_INT (procedure, "compression",
                         "Compression",
                         "Compression type: { NONE (0), LZW (1), PACKBITS (2), "
                         "DEFLATE (3), JPEG (4), CCITT G3 Fax (5), "
                         "CCITT G4 Fax (6) }",
                         0, 6, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "save-transp-pixels",
                             "Save transp pixels",
                             "Keep the color data masked by an alpha channel "
                             "intact (do not store premultiplied components)",
                             TRUE,
                             G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
tiff_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray    *return_vals;
  GimpPDBStatusType  status;
  GimpImage         *image             = NULL;
  gboolean           resolution_loaded = FALSE;
  gboolean           profile_loaded    = FALSE;
  GimpMetadata      *metadata;
  GError            *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    gimp_ui_init (PLUG_IN_BINARY);

  status = load_image (file, run_mode, &image,
                       &resolution_loaded,
                       &profile_loaded,
                       &error);

  if (!image)
    return gimp_procedure_new_return_values (procedure, status, error);

  metadata = gimp_image_metadata_load_prepare (image,
                                               "image/tiff",
                                               file, NULL);

  if (metadata)
    {
      GimpMetadataLoadFlags flags = GIMP_METADATA_LOAD_ALL;

      if (resolution_loaded)
        flags &= ~GIMP_METADATA_LOAD_RESOLUTION;

      if (profile_loaded)
        flags &= ~GIMP_METADATA_LOAD_COLORSPACE;

      gimp_image_metadata_load_finish (image, "image/tiff",
                                       metadata, flags,
                                       run_mode == GIMP_RUN_INTERACTIVE);

      g_object_unref (metadata);
    }

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
tiff_save (GimpProcedure        *procedure,
           GimpRunMode           run_mode,
           GimpImage            *image,
           gint                  n_drawables,
           GimpDrawable        **drawables,
           GFile                *file,
           const GimpValueArray *args,
           gpointer              run_data)
{
  GimpPDBStatusType      status = GIMP_PDB_SUCCESS;
  GimpMetadata          *metadata;
  GimpMetadataSaveFlags  metadata_flags;
  GimpParasite          *parasite;
  GimpImage             *orig_image = image;
  GimpExportReturn       export     = GIMP_EXPORT_CANCEL;
  GError                *error      = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);
      break;
    default:
      break;
    }

  /* Override the defaults with preferences. */
  metadata = gimp_image_metadata_save_prepare (orig_image,
                                               "image/tiff",
                                               &metadata_flags);
  tsvals.save_exif      = (metadata_flags & GIMP_METADATA_SAVE_EXIF) != 0;
  tsvals.save_xmp       = (metadata_flags & GIMP_METADATA_SAVE_XMP) != 0;
  tsvals.save_iptc      = (metadata_flags & GIMP_METADATA_SAVE_IPTC) != 0;
  tsvals.save_thumbnail = (metadata_flags & GIMP_METADATA_SAVE_THUMBNAIL) != 0;
  tsvals.save_profile   = (metadata_flags & GIMP_METADATA_SAVE_COLOR_PROFILE) != 0;

  parasite = gimp_image_get_parasite (orig_image, "gimp-comment");
  if (parasite)
    {
      image_comment = g_strndup (gimp_parasite_data (parasite),
                                 gimp_parasite_data_size (parasite));
      gimp_parasite_free (parasite);
    }

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (SAVE_PROC, &tsvals);

      parasite = gimp_image_get_parasite (orig_image, "tiff-save-options");
      if (parasite)
        {
          const TiffSaveVals *pvals = gimp_parasite_data (parasite);

          if (pvals->compression == COMPRESSION_DEFLATE)
            tsvals.compression = COMPRESSION_ADOBE_DEFLATE;
          else
            tsvals.compression = pvals->compression;

          tsvals.save_transp_pixels = pvals->save_transp_pixels;
        }
      gimp_parasite_free (parasite);

      /*  First acquire information with a dialog  */
      if (! save_dialog (&tsvals,
                         SAVE_PROC,
                         n_drawables == 1 ? gimp_drawable_has_alpha (drawables[0]) : TRUE,
                         image_is_monochrome (image),
                         gimp_image_base_type (image) == GIMP_INDEXED,
                         image_is_multi_layer (image),
                         &image_comment))
        {
          return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL,
                                                   NULL);
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      switch (GIMP_VALUES_GET_INT (args, 0))
        {
        case 0: tsvals.compression = COMPRESSION_NONE;          break;
        case 1: tsvals.compression = COMPRESSION_LZW;           break;
        case 2: tsvals.compression = COMPRESSION_PACKBITS;      break;
        case 3: tsvals.compression = COMPRESSION_ADOBE_DEFLATE; break;
        case 4: tsvals.compression = COMPRESSION_JPEG;          break;
        case 5: tsvals.compression = COMPRESSION_CCITTFAX3;     break;
        case 6: tsvals.compression = COMPRESSION_CCITTFAX4;     break;
        }

      tsvals.save_transp_pixels = GIMP_VALUES_GET_BOOLEAN (args, 1);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (SAVE_PROC, &tsvals);

      parasite = gimp_image_get_parasite (orig_image, "tiff-save-options");
      if (parasite)
        {
          const TiffSaveVals *pvals = gimp_parasite_data (parasite);

          tsvals.compression        = pvals->compression;
          tsvals.save_transp_pixels = pvals->save_transp_pixels;
        }
      gimp_parasite_free (parasite);
      break;

    default:
      break;
    }

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      {
        GimpExportCapabilities capabilities;

        if (tsvals.compression == COMPRESSION_CCITTFAX3 ||
            tsvals.compression == COMPRESSION_CCITTFAX4)
          /* G3/G4 are fax compressions. They only support monochrome
           * images without alpha support.
           */
          capabilities = GIMP_EXPORT_CAN_HANDLE_INDEXED;
        else
          capabilities = GIMP_EXPORT_CAN_HANDLE_RGB     |
            GIMP_EXPORT_CAN_HANDLE_GRAY    |
            GIMP_EXPORT_CAN_HANDLE_INDEXED |
            GIMP_EXPORT_CAN_HANDLE_ALPHA;

        if (tsvals.save_layers && image_is_multi_layer (image))
          capabilities |= GIMP_EXPORT_CAN_HANDLE_LAYERS;

        export = gimp_export_image (&image, &n_drawables, &drawables, "TIFF", capabilities);

        if (export == GIMP_EXPORT_CANCEL)
          return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL,
                                                   NULL);
      }
      break;
    default:
      break;
    }

  if (n_drawables != 1 && tsvals.save_layers)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("\"Save layers\" option not set while trying to export multiple layers."));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      gint saved_bpp;

      if (save_image (file, &tsvals, image, orig_image, image_comment,
                      &saved_bpp, metadata, metadata_flags, &error))
        {
          /*  Store mvals data  */
          gimp_set_data (SAVE_PROC, &tsvals, sizeof (TiffSaveVals));
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    {
      gimp_image_delete (image);
      g_free (drawables);
    }

  if (metadata)
    g_object_unref (metadata);

  return gimp_procedure_new_return_values (procedure, status, error);
}

static gboolean
image_is_monochrome (GimpImage *image)
{
  guchar   *colors;
  gint      num_colors;
  gboolean  monochrome = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  colors = gimp_image_get_colormap (image, &num_colors);

  if (colors)
    {
      if (num_colors == 2 || num_colors == 1)
        {
          const guchar  bw_map[] = { 0, 0, 0, 255, 255, 255 };
          const guchar  wb_map[] = { 255, 255, 255, 0, 0, 0 };

          if (memcmp (colors, bw_map, 3 * num_colors) == 0 ||
              memcmp (colors, wb_map, 3 * num_colors) == 0)
            {
              monochrome = TRUE;
            }
        }

      g_free (colors);
    }

  return monochrome;
}

static gboolean
image_is_multi_layer (GimpImage *image)
{
  gint32 n_layers;

  g_free (gimp_image_get_layers (image, &n_layers));

  return (n_layers > 1);
}
