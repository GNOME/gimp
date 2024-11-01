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

#include "file-tiff.h"
#include "file-tiff-io.h"
#include "file-tiff-load.h"
#include "file-tiff-export.h"

#include "libgimp/stdplugins-intl.h"


#define EXPORT_PROC    "file-tiff-export"
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
#define TIFF(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TIFF_TYPE, Tiff))

GType                           tiff_get_type         (void) G_GNUC_CONST;

static GList                  * tiff_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure          * tiff_create_procedure (GimpPlugIn            *plug_in,
                                                       const gchar           *name);

static GimpValueArray         * tiff_load             (GimpProcedure         *procedure,
                                                       GimpRunMode            run_mode,
                                                       GFile                 *file,
                                                       GimpMetadata          *metadata,
                                                       GimpMetadataLoadFlags *flags,
                                                       GimpProcedureConfig   *config,
                                                       gpointer               run_data);
static GimpValueArray         * tiff_export           (GimpProcedure         *procedure,
                                                       GimpRunMode            run_mode,
                                                       GimpImage             *image,
                                                       GFile                 *file,
                                                       GimpExportOptions     *options,
                                                       GimpMetadata          *metadata,
                                                       GimpProcedureConfig   *config,
                                                       gpointer               run_data);
static GimpPDBStatusType        tiff_export_rec       (GimpProcedure         *procedure,
                                                       GimpRunMode            run_mode,
                                                       GimpImage             *orig_image,
                                                       GFile                 *file,
                                                       GimpProcedureConfig   *config,
                                                       GimpExportOptions     *options,
                                                       GimpMetadata          *metadata,
                                                       gboolean               retried,
                                                       GError               **error);
static GimpExportCapabilities   export_edit_options   (GimpProcedure         *procedure,
                                                       GimpProcedureConfig   *config,
                                                       GimpExportOptions     *options,
                                                       gpointer               create_data);

static gboolean                 image_is_monochrome  (GimpImage            *image);
static gboolean                 image_is_multi_layer (GimpImage            *image);


G_DEFINE_TYPE (Tiff, tiff, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (TIFF_TYPE)
DEFINE_STD_SET_I18N


static void
tiff_class_init (TiffClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = tiff_query_procedures;
  plug_in_class->create_procedure = tiff_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
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
  list = g_list_append (list, g_strdup (EXPORT_PROC));

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

      gimp_procedure_set_menu_label (procedure, _("TIFF or BigTIFF image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Loads files of the TIFF and BigTIFF file formats"),
                                        _("Loads files of the Tag Image File Format (TIFF) and "
                                          "its 64-bit offsets variant (BigTIFF)"),
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

      /* TODO: the 2 below AUX arguments should likely be real arguments, but I
       * just wanted to get rid of gimp_get_data/gimp_set_data() usage at first
       * and didn't dig much into proper and full usage. Since it's always
       * possible to add arguments, but not to remove them, I prefer to make
       * them AUX for now and leave it as further exercise to decide whether it
       * should be part of the PDB API.
       */
      gimp_procedure_add_enum_aux_argument (procedure, "target",
                                            "Open _pages as", NULL,
                                            GIMP_TYPE_PAGE_SELECTOR_TARGET,
                                            GIMP_PAGE_SELECTOR_TARGET_LAYERS,
                                            G_PARAM_READWRITE);

      gimp_procedure_add_boolean_aux_argument (procedure, "keep-empty-space",
                                               _("_Keep empty space around imported layers"),
                                               NULL, TRUE, GIMP_PARAM_READWRITE);
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             TRUE, tiff_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("TIFF or BigTIFF image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Exports files in the TIFF or BigTIFF file formats"),
                                        _("Exports files in the Tag Image File Format (TIFF) or "
                                          "its 64-bit offsets variant (BigTIFF) able to support "
                                          "much bigger file sizes"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis",
                                      "Spencer Kimball & Peter Mattis",
                                      "1995-1996,2000-2003");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);
      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("TIFF"));
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/tiff");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "tif,tiff");

      /* TIFF capabilities are so dependent on export settings, we can't assign
       * defaults until we know how the user wants to export it */
      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              0, export_edit_options, NULL, NULL);

      gimp_procedure_add_boolean_argument (procedure, "bigtiff",
                                           _("Export in _BigTIFF variant file format"),
                                           _("The BigTIFF variant file format uses 64-bit offsets, "
                                             "hence supporting over 4GiB files and bigger"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "compression",
                                          _("Co_mpression"),
                                          _("Compression type"),
                                          gimp_choice_new_with_values ("none",          GIMP_COMPRESSION_NONE,          _("None"),              NULL,
                                                                       "lzw",           GIMP_COMPRESSION_LZW,           _("LZW"),               NULL,
                                                                       "packbits",      GIMP_COMPRESSION_PACKBITS,      _("Pack Bits"),         NULL,
                                                                       "adobe_deflate", GIMP_COMPRESSION_ADOBE_DEFLATE, _("Deflate"),           NULL,
                                                                       "jpeg",          GIMP_COMPRESSION_JPEG,          _("JPEG"),              NULL,
                                                                       "ccittfax3",     GIMP_COMPRESSION_CCITTFAX3,     _("CCITT Group 3 fax"), NULL,
                                                                       "ccittfax4",     GIMP_COMPRESSION_CCITTFAX4,     _("CCITT Group 4 fax"), NULL,
                                                                       NULL),
                                          "none", G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "save-transparent-pixels",
                                           _("Save color _values from transparent pixels"),
                                           _("Keep the color data masked by an alpha channel "
                                             "intact (do not store premultiplied components)"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "cmyk",
                                           _("Export as CMY_K"),
                                           _("Create a CMYK TIFF image using the soft-proofing color profile"),
                                           FALSE,
                                           G_PARAM_READWRITE);

     gimp_procedure_add_boolean_aux_argument (procedure, "save-layers",
                                               _("Save La_yers"),
                                               _("Save Layers"),
                                               TRUE,
                                               G_PARAM_READWRITE);

     gimp_procedure_add_boolean_aux_argument (procedure, "crop-layers",
                                               _("Crop L_ayers"),
                                               _("Crop Layers"),
                                               TRUE,
                                               G_PARAM_READWRITE);

     gimp_procedure_add_boolean_aux_argument (procedure, "save-geotiff",
                                               _("Save _GeoTIFF data"),
                                               _("Save GeoTIFF data"),
                                               TRUE,
                                               G_PARAM_READWRITE);

      gimp_export_procedure_set_support_exif      (GIMP_EXPORT_PROCEDURE (procedure), TRUE);
      gimp_export_procedure_set_support_iptc      (GIMP_EXPORT_PROCEDURE (procedure), TRUE);
      gimp_export_procedure_set_support_xmp       (GIMP_EXPORT_PROCEDURE (procedure), TRUE);
#ifdef TIFFTAG_ICCPROFILE
      gimp_export_procedure_set_support_profile   (GIMP_EXPORT_PROCEDURE (procedure), TRUE);
#endif
      gimp_export_procedure_set_support_thumbnail (GIMP_EXPORT_PROCEDURE (procedure), TRUE);
      gimp_export_procedure_set_support_comment   (GIMP_EXPORT_PROCEDURE (procedure), TRUE);
    }

  return procedure;
}

static GimpValueArray *
tiff_load (GimpProcedure         *procedure,
           GimpRunMode            run_mode,
           GFile                 *file,
           GimpMetadata          *metadata,
           GimpMetadataLoadFlags *flags,
           GimpProcedureConfig   *config,
           gpointer               run_data)
{
  GimpValueArray    *return_vals;
  GimpPDBStatusType  status;
  GimpImage         *image              = NULL;
  gboolean           resolution_loaded  = FALSE;
  gboolean           profile_loaded     = FALSE;
  gboolean           ps_metadata_loaded = FALSE;
  GError            *error = NULL;

  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    gimp_ui_init (PLUG_IN_BINARY);

  status = load_image (procedure,
                       file, run_mode, &image,
                       &resolution_loaded,
                       &profile_loaded,
                       &ps_metadata_loaded,
                       config, &error);

  if (!image)
    return gimp_procedure_new_return_values (procedure, status, error);

  if (resolution_loaded)
    *flags &= ~GIMP_METADATA_LOAD_RESOLUTION;

  if (profile_loaded)
    *flags &= ~GIMP_METADATA_LOAD_COLORSPACE;

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
tiff_export (GimpProcedure        *procedure,
             GimpRunMode           run_mode,
             GimpImage            *image,
             GFile                *file,
             GimpExportOptions    *options,
             GimpMetadata         *metadata,
             GimpProcedureConfig  *config,
             gpointer              run_data)
{
  GError            *error   = NULL;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

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

  status = tiff_export_rec (procedure, run_mode, image, file,
                            config, options, metadata, FALSE, &error);

  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpPDBStatusType
tiff_export_rec (GimpProcedure        *procedure,
                 GimpRunMode           run_mode,
                 GimpImage            *orig_image,
                 GFile                *file,
                 GimpProcedureConfig  *config,
                 GimpExportOptions    *options,
                 GimpMetadata         *metadata,
                 gboolean              retried,
                 GError              **error)
{
  GimpImage         *image       = orig_image;
  GList             *drawables   = gimp_image_list_layers (image);
  gint               n_drawables = g_list_length (drawables);
  GimpPDBStatusType  status      = GIMP_PDB_SUCCESS;
  GimpExportReturn   export      = GIMP_EXPORT_IGNORE;
  gboolean           bigtiff     = FALSE;

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! save_dialog (orig_image, procedure, G_OBJECT (config),
                         n_drawables == 1 ? gimp_drawable_has_alpha (drawables->data) : TRUE,
                         image_is_monochrome (orig_image),
                         gimp_image_get_base_type (orig_image) == GIMP_INDEXED,
                         image_is_multi_layer (orig_image),
                         retried))
        {
          return GIMP_PDB_CANCEL;
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      g_object_get (config, "bigtiff", &bigtiff, NULL);

      export = gimp_export_options_get_image (options, &image);
    }
  drawables = gimp_image_list_layers (image);

#if 0
  /* FIXME */
  if (n_drawables != 1 && tsvals.save_layers)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("\"Save layers\" option not set while trying to export multiple layers."));

      status = GIMP_PDB_CALLING_ERROR;
    }
#endif

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! export_image (file, image, orig_image, G_OBJECT (config),
                          metadata, error))
        status = GIMP_PDB_EXECUTION_ERROR;
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  if (status == GIMP_PDB_EXECUTION_ERROR &&
      run_mode == GIMP_RUN_INTERACTIVE   &&
      ! retried && ! bigtiff && tiff_got_file_size_error ())
    {
      /* Retrying but just once, when the save failed because we exceeded
       * TIFF max size, to propose BigTIFF instead. */
      tiff_reset_file_size_error ();
      g_clear_error (error);

      return tiff_export_rec (procedure, run_mode, orig_image, file,
                              config, options, metadata, TRUE, error);
    }

  return status;
}

static GimpExportCapabilities
export_edit_options (GimpProcedure        *procedure,
                     GimpProcedureConfig  *config,
                     GimpExportOptions    *options,
                     gpointer              create_data)
{
  GimpExportCapabilities capabilities;
  GimpCompression        compression;
  gboolean               save_layers;
  gboolean               crop_layers;

  g_object_get (config,
                "save-layers", &save_layers,
                "crop-layers", &crop_layers,
                NULL);
  compression = gimp_procedure_config_get_choice_id (config, "compression");

  if (compression == GIMP_COMPRESSION_CCITTFAX3 ||
      compression == GIMP_COMPRESSION_CCITTFAX4)
    {
      /* G3/G4 are fax compressions. They only support
       * monochrome images without alpha support.
       */
      capabilities = GIMP_EXPORT_CAN_HANDLE_INDEXED;
    }
  else
    {
      capabilities = (GIMP_EXPORT_CAN_HANDLE_RGB     |
                      GIMP_EXPORT_CAN_HANDLE_GRAY    |
                      GIMP_EXPORT_CAN_HANDLE_INDEXED |
                      GIMP_EXPORT_CAN_HANDLE_ALPHA);
    }

  if (save_layers)
    {
      capabilities |= GIMP_EXPORT_CAN_HANDLE_LAYERS;

      if (crop_layers)
        capabilities |= GIMP_EXPORT_NEEDS_CROP;
    }

  return capabilities;
}

static gboolean
image_is_monochrome (GimpImage *image)
{
  GimpPalette *palette;
  gboolean     monochrome = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  palette = gimp_image_get_palette (image);
  if (palette)
    {
      gint num_colors;

      num_colors = gimp_palette_get_color_count (palette);

      if (num_colors == 2 || num_colors == 1)
        {
          guchar       *colors;
          const guchar  bw_map[] = { 0, 0, 0, 255, 255, 255 };
          const guchar  wb_map[] = { 255, 255, 255, 0, 0, 0 };

          colors = gimp_palette_get_colormap (palette, babl_format ("R'G'B' u8"), &num_colors, NULL);

          if (memcmp (colors, bw_map, 3 * num_colors) == 0 ||
              memcmp (colors, wb_map, 3 * num_colors) == 0)
            {
              monochrome = TRUE;
            }

          g_free (colors);
        }
    }

  return monochrome;
}

static gboolean
image_is_multi_layer (GimpImage *image)
{
  GimpLayer **layers;
  gint32      n_layers;

  layers   = gimp_image_get_layers (image);
  n_layers = gimp_core_object_array_get_length ((GObject **) layers);
  g_free (layers);

  return (n_layers > 1);
}

gint
gimp_compression_to_tiff_compression (GimpCompression compression)
{
  switch (compression)
    {
    case GIMP_COMPRESSION_NONE:          return COMPRESSION_NONE;
    case GIMP_COMPRESSION_LZW:           return COMPRESSION_LZW;
    case GIMP_COMPRESSION_PACKBITS:      return COMPRESSION_PACKBITS;
    case GIMP_COMPRESSION_ADOBE_DEFLATE: return COMPRESSION_ADOBE_DEFLATE;
    case GIMP_COMPRESSION_JPEG:          return COMPRESSION_JPEG;
    case GIMP_COMPRESSION_CCITTFAX3:     return COMPRESSION_CCITTFAX3;
    case GIMP_COMPRESSION_CCITTFAX4:     return COMPRESSION_CCITTFAX4;
    }

  return COMPRESSION_NONE;
}

GimpCompression
tiff_compression_to_gimp_compression (gint compression)
{
  switch (compression)
    {
    case COMPRESSION_NONE:          return GIMP_COMPRESSION_NONE;
    case COMPRESSION_LZW:           return GIMP_COMPRESSION_LZW;
    case COMPRESSION_PACKBITS:      return GIMP_COMPRESSION_PACKBITS;
    case COMPRESSION_DEFLATE:       return GIMP_COMPRESSION_ADOBE_DEFLATE;
    case COMPRESSION_ADOBE_DEFLATE: return GIMP_COMPRESSION_ADOBE_DEFLATE;
    case COMPRESSION_OJPEG:         return GIMP_COMPRESSION_JPEG;
    case COMPRESSION_JPEG:          return GIMP_COMPRESSION_JPEG;
    case COMPRESSION_CCITTFAX3:     return GIMP_COMPRESSION_CCITTFAX3;
    case COMPRESSION_CCITTFAX4:     return GIMP_COMPRESSION_CCITTFAX4;
    }

  return GIMP_COMPRESSION_NONE;
}
