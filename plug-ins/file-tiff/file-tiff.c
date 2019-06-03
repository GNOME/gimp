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

#include "file-tiff-io.h"
#include "file-tiff-load.h"
#include "file-tiff-save.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-tiff-load"
#define SAVE_PROC      "file-tiff-save"
#define SAVE2_PROC     "file-tiff-save2"
#define PLUG_IN_BINARY "file-tiff"


static void       query               (void);
static void       run                 (const gchar      *name,
                                       gint              nparams,
                                       const GimpParam  *param,
                                       gint             *nreturn_vals,
                                       GimpParam       **return_vals);

static gboolean   image_is_monochrome (gint32            image);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static TiffSaveVals tsvals =
{
  COMPRESSION_NONE,    /*  compression         */
  TRUE,                /*  alpha handling      */
  TRUE,                /*  save transp. pixels */
  FALSE,               /*  save exif           */
  FALSE,               /*  save xmp            */
  FALSE,               /*  save iptc           */
  TRUE,                /*  save thumbnail      */
  TRUE,                /*  save profile        */
  TRUE                 /*  save layer          */
};

static gchar *image_comment = NULL;


MAIN ()


static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name of the file to load" }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

#define COMMON_SAVE_ARGS \
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },\
    { GIMP_PDB_IMAGE,    "image",        "Input image" },\
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },\
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },\
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" },\
    { GIMP_PDB_INT32,    "compression",  "Compression type: { NONE (0), LZW (1), PACKBITS (2), DEFLATE (3), JPEG (4), CCITT G3 Fax (5), CCITT G4 Fax (6) }" }

  static const GimpParamDef save_args_old[] =
  {
    COMMON_SAVE_ARGS
  };

  static const GimpParamDef save_args[] =
  {
    COMMON_SAVE_ARGS,
    { GIMP_PDB_INT32, "save-transp-pixels", "Keep the color data masked by an alpha channel intact (do not store premultiplied components)" }
  };

  gimp_install_procedure (LOAD_PROC,
                          "loads files of the tiff file format",
                          "FIXME: write help for tiff_load",
                          "Spencer Kimball, Peter Mattis & Nick Lamb",
                          "Nick Lamb <njl195@zepler.org.uk>",
                          "1995-1996,1998-2003",
                          N_("TIFF image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/tiff");
  gimp_register_file_handler_uri (LOAD_PROC);
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "tif,tiff",
                                    "",
                                    "0,string,II*\\0,0,string,MM\\0*");

  gimp_install_procedure (SAVE_PROC,
                          "saves files in the tiff file format",
                          "Saves files in the Tagged Image File Format.  "
                          "The value for the saved comment is taken "
                          "from the 'gimp-comment' parasite.",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1995-1996,2000-2003",
                          N_("TIFF image"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args_old), 0,
                          save_args_old, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/tiff");
  gimp_register_file_handler_uri (SAVE_PROC);
  gimp_register_save_handler (SAVE_PROC, "tif,tiff", "");

  gimp_install_procedure (SAVE2_PROC,
                          "saves files in the tiff file format",
                          "Saves files in the Tagged Image File Format.  "
                          "The value for the saved comment is taken "
                          "from the 'gimp-comment' parasite.",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1995-1996,2000-2003",
                          N_("TIFF image"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GError            *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      GFile *file = g_file_new_for_uri (param[1].data.d_string);
      TIFF  *tif;

      tif = tiff_open (file, "r", &error);

      if (tif)
        {
          TiffSelectedPages pages;

          pages.target = GIMP_PAGE_SELECTOR_TARGET_LAYERS;

          gimp_get_data (LOAD_PROC "-target", &pages.target);

          pages.keep_empty_space = TRUE;

          gimp_get_data (LOAD_PROC "-keep-empty-space",
                         &pages.keep_empty_space);

          pages.n_pages = pages.o_pages = TIFFNumberOfDirectories (tif);

          if (pages.n_pages == 0)
            {
              g_set_error (&error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("TIFF '%s' does not contain any directories"),
                           gimp_file_get_utf8_name (file));

              status = GIMP_PDB_EXECUTION_ERROR;
            }
          else
            {
              gboolean run_it = FALSE;
              gint     i;

              if (run_mode != GIMP_RUN_INTERACTIVE)
                {
                  pages.pages = g_new (gint, pages.n_pages);

                  for (i = 0; i < pages.n_pages; i++)
                    pages.pages[i] = i;

                  run_it = TRUE;
                }
              else
                {
                  gimp_ui_init (PLUG_IN_BINARY, FALSE);
                }

              if (pages.n_pages == 1)
                {
                  pages.pages  = g_new0 (gint, pages.n_pages);
                  pages.target = GIMP_PAGE_SELECTOR_TARGET_LAYERS;

                  run_it = TRUE;
                }

              if ((! run_it) && (run_mode == GIMP_RUN_INTERACTIVE))
                run_it = load_dialog (tif, LOAD_PROC, &pages);

              if (run_it)
                {
                  gint32    image;
                  gboolean  resolution_loaded = FALSE;

                  gimp_set_data (LOAD_PROC "-target",
                                 &pages.target, sizeof (pages.target));

                  gimp_set_data (LOAD_PROC "-keep-empty-space",
                                 &pages.keep_empty_space,
                                 sizeof (pages.keep_empty_space));

                  image = load_image (file, tif, &pages,
                                      &resolution_loaded,
                                      &error);

                  g_free (pages.pages);

                  if (image > 0)
                    {
                      GimpMetadata *metadata;

                      metadata = gimp_image_metadata_load_prepare (image,
                                                                   "image/tiff",
                                                                   file, NULL);

                      if (metadata)
                        {
                          GimpMetadataLoadFlags flags = GIMP_METADATA_LOAD_ALL;

                          if (resolution_loaded)
                            flags &= ~GIMP_METADATA_LOAD_RESOLUTION;

                          gimp_image_metadata_load_finish (image, "image/tiff",
                                                           metadata, flags,
                                                           run_mode == GIMP_RUN_INTERACTIVE);

                          g_object_unref (metadata);
                        }

                      *nreturn_vals = 2;
                      values[1].type         = GIMP_PDB_IMAGE;
                      values[1].data.d_image = image;
                    }
                  else
                    {
                      status = GIMP_PDB_EXECUTION_ERROR;
                    }

                }
              else
                {
                  status = GIMP_PDB_CANCEL;
                }
            }

          TIFFClose (tif);
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }

      g_object_unref (file);
    }
  else if ((strcmp (name, SAVE_PROC)  == 0) ||
           (strcmp (name, SAVE2_PROC) == 0))
    {
      /* Plug-in is either file_tiff_save or file_tiff_save2 */

      GimpMetadata          *metadata;
      GimpMetadataSaveFlags  metadata_flags;
      GimpParasite          *parasite;
      gint32                 image      = param[1].data.d_int32;
      gint32                 drawable   = param[2].data.d_int32;
      gint32                 orig_image = image;
      GimpExportReturn       export     = GIMP_EXPORT_CANCEL;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          export = gimp_export_image (&image, &drawable, "TIFF",
                                      GIMP_EXPORT_CAN_HANDLE_RGB     |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                      GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA   |
                                      GIMP_EXPORT_CAN_HANDLE_LAYERS);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }
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
                             gimp_drawable_has_alpha (drawable),
                             image_is_monochrome (image),
                             gimp_image_base_type (image) == GIMP_INDEXED,
                             &image_comment))
            {
              status = GIMP_PDB_CANCEL;
            }
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams == 6 || nparams == 7)
            {
              switch (param[5].data.d_int32)
                {
                case 0: tsvals.compression = COMPRESSION_NONE;          break;
                case 1: tsvals.compression = COMPRESSION_LZW;           break;
                case 2: tsvals.compression = COMPRESSION_PACKBITS;      break;
                case 3: tsvals.compression = COMPRESSION_ADOBE_DEFLATE; break;
                case 4: tsvals.compression = COMPRESSION_JPEG;          break;
                case 5: tsvals.compression = COMPRESSION_CCITTFAX3;     break;
                case 6: tsvals.compression = COMPRESSION_CCITTFAX4;     break;
                default: status = GIMP_PDB_CALLING_ERROR; break;
                }

              if (nparams == 7)
                tsvals.save_transp_pixels = param[6].data.d_int32;
              else
                tsvals.save_transp_pixels = TRUE;
            }
          else
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
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

      if (status == GIMP_PDB_SUCCESS)
        {
          GFile *file;
          gint   saved_bpp;

          file = g_file_new_for_uri (param[3].data.d_string);

          /* saving with layers is not supporting blend modes, so people might
           * prefer to save a flat copy. */
          if (! tsvals.save_layers)
            {
              gint32 transp;

              if (export != GIMP_EXPORT_EXPORT)
                {
                  image    = gimp_image_duplicate (image);
                  drawable = gimp_image_get_active_layer (image);

                  export = GIMP_EXPORT_EXPORT;
                }

              /* borrowed from ./libgimp/gimpexport.c:export_merge()
               * this makes sure that the exported file size is correct. */
              transp = gimp_layer_new (image, "-",
                                       gimp_image_width (image),
                                       gimp_image_height (image),
                                       gimp_drawable_type (drawable) | 1,
                                       100.0, GIMP_LAYER_MODE_NORMAL);
              gimp_image_insert_layer (image, transp, -1, 1);
              gimp_selection_none (image);
              gimp_drawable_edit_clear (transp);

              gimp_image_merge_visible_layers (image, GIMP_CLIP_TO_IMAGE);
            }

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

          g_object_unref (file);
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image);

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

static gboolean
image_is_monochrome (gint32 image)
{
  guchar   *colors;
  gint      num_colors;
  gboolean  monochrome = FALSE;

  g_return_val_if_fail (image != -1, FALSE);

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
