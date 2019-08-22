/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Portable Network Graphics (PNG) plug-in
 *
 *   Copyright 1997-1998 Michael Sweet (mike@easysw.com) and
 *   Daniel Skarda (0rfelyus@atrey.karlin.mff.cuni.cz).
 *   and 1999-2000 Nick Lamb (njl195@zepler.org.uk)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Contents:
 *
 *   main()                      - Main entry - just call gimp_main()...
 *   query()                     - Respond to a plug-in query...
 *   run()                       - Run the plug-in...
 *   load_image()                - Load a PNG image into a new image window.
 *   offsets_dialog()            - Asks the user about offsets when loading.
 *   respin_cmap()               - Re-order a Gimp colormap for PNG tRNS
 *   save_image()                - Export the specified image to a PNG file.
 *   save_compression_callback() - Update the image compression level.
 *   save_interlace_update()     - Update the interlacing option.
 *   save_dialog()               - Pop up the export dialog.
 *
 * Revision History:
 *
 *   see ChangeLog
 */

#include "config.h"

#include <stdlib.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <png.h>                /* PNG library definitions */

#include "libgimp/stdplugins-intl.h"


/*
 * Constants...
 */

#define LOAD_PROC              "file-png-load"
#define SAVE_PROC              "file-png-save"
#define PLUG_IN_BINARY         "file-png"
#define PLUG_IN_ROLE           "gimp-file-png"

#define PLUG_IN_VERSION        "1.3.4 - 03 September 2002"
#define SCALE_WIDTH            125

#define DEFAULT_GAMMA          2.20

#define PNG_DEFAULTS_PARASITE  "png-save-defaults"

/*
 * Structures...
 */

typedef enum _PngExportformat {
  PNG_FORMAT_AUTO = 0,
  PNG_FORMAT_RGB8,
  PNG_FORMAT_GRAY8,
  PNG_FORMAT_RGBA8,
  PNG_FORMAT_GRAYA8,
  PNG_FORMAT_RGB16,
  PNG_FORMAT_GRAY16,
  PNG_FORMAT_RGBA16,
  PNG_FORMAT_GRAYA16
} PngExportFormat;

typedef struct
{
  gboolean  interlaced;
  gboolean  bkgd;
  gboolean  gama;
  gboolean  offs;
  gboolean  phys;
  gboolean  time;
  gboolean  comment;
  gboolean  save_transp_pixels;
  gint      compression_level;
  gboolean  save_exif;
  gboolean  save_xmp;
  gboolean  save_iptc;
  gboolean  save_thumbnail;
  gboolean  save_profile;
  PngExportFormat export_format;
}
PngSaveVals;

typedef struct
{
  gboolean       run;

  GtkWidget     *interlaced;
  GtkWidget     *bkgd;
  GtkWidget     *gama;
  GtkWidget     *offs;
  GtkWidget     *phys;
  GtkWidget     *time;
  GtkWidget     *comment;
  GtkWidget     *pixelformat;
  GtkWidget     *save_transp_pixels;
  GtkAdjustment *compression_level;
  GtkWidget     *save_exif;
  GtkWidget     *save_xmp;
  GtkWidget     *save_iptc;
  GtkWidget     *save_thumbnail;
  GtkWidget     *save_profile;
}
PngSaveGui;

/* These are not saved or restored. */
typedef struct
{
  gboolean   has_trns;
  png_bytep  trans;
  int        num_trans;
  gboolean   has_plte;
  png_colorp palette;
  int        num_palette;
}
PngGlobals;


typedef struct _Png      Png;
typedef struct _PngClass PngClass;

struct _Png
{
  GimpPlugIn      parent_instance;
};

struct _PngClass
{
  GimpPlugInClass parent_class;
};


#define PNG_TYPE  (png_get_type ())
#define PNG (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PNG_TYPE, Png))

GType                   png_get_type         (void) G_GNUC_CONST;

static GList          * png_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * png_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * png_load             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static GimpValueArray * png_save             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              GimpDrawable         *drawable,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);

static GimpImage * load_image              (const gchar      *filename,
                                            gboolean          interactive,
                                            gboolean         *resolution_loaded,
                                            gboolean         *profile_loaded,
                                            GError          **error);
static gboolean  save_image                (const gchar      *filename,
                                            GimpImage        *image,
                                            GimpDrawable     *drawable,
                                            GimpImage        *orig_image,
                                            gboolean         *profile_saved,
                                            GError          **error);

static int       respin_cmap               (png_structp       pp,
                                            png_infop         info,
                                            guchar           *remap,
                                            GimpImage        *image,
                                            GimpDrawable     *drawable);

static gboolean  save_dialog               (GimpImage        *image,
                                            gboolean          alpha);

static void      save_dialog_response      (GtkWidget        *widget,
                                            gint              response_id,
                                            gpointer          data);

static gboolean  offsets_dialog            (gint              offset_x,
                                            gint              offset_y);

static gboolean  ia_has_transparent_pixels (GeglBuffer       *buffer);

static gint      find_unused_ia_color      (GeglBuffer       *buffer,
                                            gint             *colors);

static void      load_parasite             (void);
static void      save_parasite             (void);
static void      load_gui_defaults         (PngSaveGui       *pg);


G_DEFINE_TYPE (Png, png, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PNG_TYPE)


static const PngSaveVals defaults =
{
  FALSE,
  TRUE,
  FALSE,
  FALSE,
  TRUE,
  TRUE,
  TRUE,
  TRUE,
  9,
  FALSE,               /* save exif       */
  FALSE,               /* save xmp        */
  FALSE,               /* save iptc        */
  TRUE,                /* save thumbnail  */
  PNG_FORMAT_AUTO
};

static PngSaveVals pngvals;
static PngGlobals  pngg;


static void
png_class_init (PngClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = png_query_procedures;
  plug_in_class->create_procedure = png_create_procedure;
}

static void
png_init (Png *png)
{
}

static GList *
png_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static GimpProcedure *
png_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name, GIMP_PLUGIN,
                                           png_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, N_("PNG image"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads files in PNG file format",
                                        "This plug-in loads Portable Network "
                                        "Graphics (PNG) files.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Michael Sweet <mike@easysw.com>, "
                                      "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>",
                                      "Michael Sweet <mike@easysw.com>, "
                                      "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>, "
                                      "Nick Lamb <njl195@zepler.org.uk>",
                                      PLUG_IN_VERSION);

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/png");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "png");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,\211PNG\r\n\032\n");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name, GIMP_PLUGIN,
                                           png_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("PNG image"));

      gimp_procedure_set_documentation (procedure,
                                        "Exports files in PNG file format",
                                        "This plug-in exports Portable Network "
                                        "Graphics (PNG) files.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Michael Sweet <mike@easysw.com>, "
                                      "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>",
                                      "Michael Sweet <mike@easysw.com>, "
                                      "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>, "
                                      "Nick Lamb <njl195@zepler.org.uk>",
                                      PLUG_IN_VERSION);

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/png");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "png");

      GIMP_PROC_ARG_BOOLEAN (procedure, "interlace",
                             "Interlace",
                             "Use Adam7 interlacing?",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "compression",
                         "Compression",
                         "Deflate Compression factor (0..9)",
                         0, 9, 9,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "bkgd",
                             "bKGD",
                             "Write bKGD chunk?",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "gama",
                             "gAMA",
                             "Write gAMA chunk?",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "offs",
                             "oFFs",
                             "Write oFFs chunk?",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "phys",
                             "pHYs",
                             "Write pHYs chunk?",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "time",
                             "tIME",
                             "Write tIME chunk?",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "comment",
                             "Comment",
                             "Write comment?",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "save-trans",
                             "Save trans",
                             "Preserve color of transparent pixels?",
                             TRUE,
                             G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
png_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray *return_vals;
  gboolean        interactive;
  gboolean        resolution_loaded = FALSE;
  gboolean        profile_loaded    = FALSE;
  GimpImage      *image;
  GimpMetadata   *metadata;
  GError         *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY, FALSE);
      interactive = TRUE;
      break;
    default:
      interactive = FALSE;
      break;
    }

  image = load_image (g_file_get_path (file),
                      interactive,
                      &resolution_loaded,
                      &profile_loaded,
                      &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  metadata = gimp_image_metadata_load_prepare (image, "image/png",
                                               file, NULL);

  if (metadata)
    {
      GimpMetadataLoadFlags flags = GIMP_METADATA_LOAD_ALL;

      if (resolution_loaded)
        flags &= ~GIMP_METADATA_LOAD_RESOLUTION;

      if (profile_loaded)
        flags &= ~GIMP_METADATA_LOAD_COLORSPACE;

      gimp_image_metadata_load_finish (image, "image/png",
                                       metadata, flags,
                                       interactive);

      g_object_unref (metadata);
    }

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
png_save (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          GimpDrawable         *drawable,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpPDBStatusType      status = GIMP_PDB_SUCCESS;
  GimpMetadata          *metadata;
  GimpMetadataSaveFlags  metadata_flags;
  GimpImage             *orig_image;
  GimpExportReturn       export = GIMP_EXPORT_CANCEL;
  gboolean               alpha;
  GError                *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  orig_image = image;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY, FALSE);

      export = gimp_export_image (&image, &drawable, "PNG",
                                  GIMP_EXPORT_CAN_HANDLE_RGB     |
                                  GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                  GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA);

      if (export == GIMP_EXPORT_CANCEL)
        return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  /* Initialize with hardcoded defaults */
  pngvals = defaults;

  /* Override the defaults with preferences. */
  metadata = gimp_image_metadata_save_prepare (orig_image,
                                               "image/png",
                                               &metadata_flags);
  pngvals.save_exif      = (metadata_flags & GIMP_METADATA_SAVE_EXIF) != 0;
  pngvals.save_xmp       = (metadata_flags & GIMP_METADATA_SAVE_XMP) != 0;
  pngvals.save_iptc      = (metadata_flags & GIMP_METADATA_SAVE_IPTC) != 0;
  pngvals.save_thumbnail = (metadata_flags & GIMP_METADATA_SAVE_THUMBNAIL) != 0;
  pngvals.save_profile   = (metadata_flags & GIMP_METADATA_SAVE_COLOR_PROFILE) != 0;

  /* Override preferences from PNG export defaults (if saved). */
  /* FIXME this is currently broken for NONINTERACTICE until we have
   * new default value handling, if simply gets overwritten below
   */
  load_parasite ();

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Finally possibly retrieve data from previous run. */
      gimp_get_data (SAVE_PROC, &pngvals);

      alpha = gimp_drawable_has_alpha (drawable);

      /* If the image has no transparency, then there is usually
       * no need to save a bKGD chunk.  For more information, see:
       * http://bugzilla.gnome.org/show_bug.cgi?id=92395
       */
      if (! alpha)
        pngvals.bkgd = FALSE;

      /* Then acquire information with a dialog...
       */
      if (! save_dialog (orig_image, alpha))
        status = GIMP_PDB_CANCEL;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      pngvals.interlaced         = GIMP_VALUES_GET_BOOLEAN (args, 0);
      pngvals.compression_level  = GIMP_VALUES_GET_INT     (args, 1);
      pngvals.bkgd               = GIMP_VALUES_GET_BOOLEAN (args, 2);
      pngvals.gama               = GIMP_VALUES_GET_BOOLEAN (args, 3);
      pngvals.offs               = GIMP_VALUES_GET_BOOLEAN (args, 4);
      pngvals.phys               = GIMP_VALUES_GET_BOOLEAN (args, 5);
      pngvals.time               = GIMP_VALUES_GET_BOOLEAN (args, 6);
      pngvals.comment            = GIMP_VALUES_GET_BOOLEAN (args, 7);
      pngvals.save_transp_pixels = GIMP_VALUES_GET_BOOLEAN (args, 8);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* possibly retrieve data */
      gimp_get_data (SAVE_PROC, &pngvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      gboolean profile_saved = FALSE;

      if (save_image (g_file_get_path (file),
                      image, drawable, orig_image,
                      &profile_saved, &error))
        {
          if (metadata)
            {
              gimp_metadata_set_bits_per_sample (metadata, 8);

              if (pngvals.save_exif)
                metadata_flags |= GIMP_METADATA_SAVE_EXIF;
              else
                metadata_flags &= ~GIMP_METADATA_SAVE_EXIF;

              if (pngvals.save_xmp)
                metadata_flags |= GIMP_METADATA_SAVE_XMP;
              else
                metadata_flags &= ~GIMP_METADATA_SAVE_XMP;

              if (pngvals.save_iptc)
                metadata_flags |= GIMP_METADATA_SAVE_IPTC;
              else
                metadata_flags &= ~GIMP_METADATA_SAVE_IPTC;

              if (pngvals.save_thumbnail)
                metadata_flags |= GIMP_METADATA_SAVE_THUMBNAIL;
              else
                metadata_flags &= ~GIMP_METADATA_SAVE_THUMBNAIL;

              /* check if the profile was actually saved, not only if
               * we wanted to save it
               */
              if (profile_saved)
                metadata_flags |= GIMP_METADATA_SAVE_COLOR_PROFILE;
              else
                metadata_flags &= ~GIMP_METADATA_SAVE_COLOR_PROFILE;

              gimp_image_metadata_save_finish (orig_image,
                                               "image/png",
                                               metadata, metadata_flags,
                                               file, NULL);
            }

          gimp_set_data (SAVE_PROC, &pngvals, sizeof (pngvals));
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  if (metadata)
    g_object_unref (metadata);

  return gimp_procedure_new_return_values (procedure, status, error);
}

struct read_error_data
{
  guchar       *pixel;           /* Pixel data */
  GeglBuffer   *buffer;          /* GEGL buffer for layer */
  const Babl   *file_format;
  guint32       width;           /* png_infop->width */
  guint32       height;          /* png_infop->height */
  gint          bpp;             /* Bytes per pixel */
  gint          tile_height;     /* Height of tile in GIMP */
  gint          begin;           /* Beginning tile row */
  gint          end;             /* Ending tile row */
  gint          num;             /* Number of rows to load */
};

static void
on_read_error (png_structp     png_ptr,
               png_const_charp error_msg)
{
  struct read_error_data *error_data = png_get_error_ptr (png_ptr);
  gint                    begin;
  gint                    end;
  gint                    num;

  g_printerr (_("Error loading PNG file: %s\n"), error_msg);

  /* Flush the current half-read row of tiles */

  gegl_buffer_set (error_data->buffer,
                   GEGL_RECTANGLE (0, error_data->begin,
                                   error_data->width,
                                   error_data->num),
                   0,
                   error_data->file_format,
                   error_data->pixel,
                   GEGL_AUTO_ROWSTRIDE);

  begin = error_data->begin + error_data->tile_height;

  if (begin < error_data->height)
    {
      end = MIN (error_data->end + error_data->tile_height, error_data->height);
      num = end - begin;

      gegl_buffer_clear (error_data->buffer,
                         GEGL_RECTANGLE (0, begin, error_data->width, num));
    }

  g_object_unref (error_data->buffer);
  longjmp (png_jmpbuf (png_ptr), 1);
}

static int
get_bit_depth_for_palette (int num_palette)
{
  if (num_palette <= 2)
    return 1;
  else if (num_palette <= 4)
    return 2;
  else if (num_palette <= 16)
    return 4;
  else
    return 8;
}

static GimpColorProfile *
load_color_profile (png_structp   pp,
                    png_infop     info,
                    gchar       **profile_name)
{
  GimpColorProfile *profile = NULL;

#if defined(PNG_iCCP_SUPPORTED)
  png_uint_32       proflen;
  png_charp         profname;
  png_bytep         prof;
  int               profcomp;

  if (png_get_iCCP (pp, info, &profname, &profcomp, &prof, &proflen))
    {
      profile = gimp_color_profile_new_from_icc_profile ((guint8 *) prof,
                                                         proflen, NULL);
      if (profile && profname)
        {
          *profile_name = g_convert (profname, strlen (profname),
                                     "ISO-8859-1", "UTF-8", NULL, NULL, NULL);
        }
    }
#endif

  return profile;
}

/*
 * 'load_image()' - Load a PNG image into a new image window.
 */
static GimpImage *
load_image (const gchar  *filename,
            gboolean      interactive,
            gboolean     *resolution_loaded,
            gboolean     *profile_loaded,
            GError      **error)
{
  gint              i;                    /* Looping var */
  gint              trns;                 /* Transparency present */
  gint              bpp;                  /* Bytes per pixel */
  gint              width;                /* image width */
  gint              height;               /* image height */
  gint              num_passes;           /* Number of interlace passes in file */
  gint              pass;                 /* Current pass in file */
  gint              tile_height;          /* Height of tile in GIMP */
  gint              begin;                /* Beginning tile row */
  gint              end;                  /* Ending tile row */
  gint              num;                  /* Number of rows to load */
  GimpImageBaseType image_type;           /* Type of image */
  GimpPrecision     image_precision;      /* Precision of image */
  GimpImageType     layer_type;           /* Type of drawable/layer */
  GimpColorProfile *profile      = NULL;  /* Color profile */
  gchar            *profile_name = NULL;  /* Profile's name */
  gboolean          linear       = FALSE; /* Linear RGB */
  FILE             *fp;                   /* File pointer */
  volatile GimpImage *image      = NULL;  /* Image -- protected for setjmp() */
  GimpLayer        *layer;                /* Layer */
  GeglBuffer       *buffer;               /* GEGL buffer for layer */
  const Babl       *file_format;          /* BABL format for layer */
  png_structp       pp;                   /* PNG read pointer */
  png_infop         info;                 /* PNG info pointers */
  guchar          **pixels;               /* Pixel rows */
  guchar           *pixel;                /* Pixel data */
  guchar            alpha[256];           /* Index -> Alpha */
  png_textp         text;
  gint              num_texts;
  struct read_error_data error_data;

  pp = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (! pp)
    {
      /* this could happen if the compile time and run-time libpng
         versions do not match. */

      g_set_error (error, 0, 0,
                   _("Error creating PNG read struct while loading '%s'."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  info = png_create_info_struct (pp);
  if (! info)
    {
      g_set_error (error, 0, 0,
                   _("Error while reading '%s'. Could not create PNG header info structure."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  if (setjmp (png_jmpbuf (pp)))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error while reading '%s'. File corrupted?"),
                   gimp_filename_to_utf8 (filename));
      return (GimpImage *) image;
    }

#ifdef PNG_BENIGN_ERRORS_SUPPORTED
  /* Change some libpng errors to warnings (e.g. bug 721135) */
  png_set_benign_errors (pp, TRUE);

  /* bug 765850 */
  png_set_option (pp, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);
#endif

  /*
   * Open the file and initialize the PNG read "engine"...
   */

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  fp = g_fopen (filename, "rb");

  if (fp == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  png_init_io (pp, fp);
  png_set_compression_buffer_size (pp, 512);

  /*
   * Get the image info
   */

  png_read_info (pp, info);

  if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    png_set_swap (pp);

  /*
   * Get the iCCP (color profile) chunk, if any, and figure if it's
   * a linear RGB profile
   */
  profile = load_color_profile (pp, info, &profile_name);

  if (profile)
    {
      *profile_loaded = TRUE;

      linear = gimp_color_profile_is_linear (profile);
    }

  /*
   * Get image precision and color model
   */

  if (png_get_bit_depth (pp, info) == 16)
    {
      if (linear)
        image_precision = GIMP_PRECISION_U16_LINEAR;
      else
        image_precision = GIMP_PRECISION_U16_NON_LINEAR;
    }
  else
    {
      if (linear)
        image_precision = GIMP_PRECISION_U8_LINEAR;
      else
        image_precision = GIMP_PRECISION_U8_NON_LINEAR;
    }

  if (png_get_bit_depth (pp, info) < 8)
    {
      if (png_get_color_type (pp, info) == PNG_COLOR_TYPE_GRAY)
        png_set_expand (pp);

      if (png_get_color_type (pp, info) == PNG_COLOR_TYPE_PALETTE)
        png_set_packing (pp);
    }

  /*
   * Expand G+tRNS to GA, RGB+tRNS to RGBA
   */

  if (png_get_color_type (pp, info) != PNG_COLOR_TYPE_PALETTE &&
      png_get_valid (pp, info, PNG_INFO_tRNS))
    png_set_expand (pp);

  /*
   * Turn on interlace handling... libpng returns just 1 (ie single pass)
   * if the image is not interlaced
   */

  num_passes = png_set_interlace_handling (pp);

  /*
   * Special handling for INDEXED + tRNS (transparency palette)
   */

  if (png_get_valid (pp, info, PNG_INFO_tRNS) &&
      png_get_color_type (pp, info) == PNG_COLOR_TYPE_PALETTE)
    {
      guchar *alpha_ptr;

      png_get_tRNS (pp, info, &alpha_ptr, &num, NULL);

      /* Copy the existing alpha values from the tRNS chunk */
      for (i = 0; i < num; ++i)
        alpha[i] = alpha_ptr[i];

      /* And set any others to fully opaque (255)  */
      for (i = num; i < 256; ++i)
        alpha[i] = 255;

      trns = 1;
    }
  else
    {
      trns = 0;
    }

  /*
   * Update the info structures after the transformations take effect
   */

  png_read_update_info (pp, info);

  switch (png_get_color_type (pp, info))
    {
    case PNG_COLOR_TYPE_RGB:
      image_type = GIMP_RGB;
      layer_type = GIMP_RGB_IMAGE;
      break;

    case PNG_COLOR_TYPE_RGB_ALPHA:
      image_type = GIMP_RGB;
      layer_type = GIMP_RGBA_IMAGE;
      break;

    case PNG_COLOR_TYPE_GRAY:
      image_type = GIMP_GRAY;
      layer_type = GIMP_GRAY_IMAGE;
      break;

    case PNG_COLOR_TYPE_GRAY_ALPHA:
      image_type = GIMP_GRAY;
      layer_type = GIMP_GRAYA_IMAGE;
      break;

    case PNG_COLOR_TYPE_PALETTE:
      image_type = GIMP_INDEXED;
      layer_type = GIMP_INDEXED_IMAGE;
      break;

    default:
      g_set_error (error, 0, 0,
                   _("Unknown color model in PNG file '%s'."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  width = png_get_image_width (pp, info);
  height = png_get_image_height (pp, info);

  image = gimp_image_new_with_precision (width, height,
                                         image_type, image_precision);
  if (! image)
    {
      g_set_error (error, 0, 0,
                   _("Could not create new image for '%s': %s"),
                   gimp_filename_to_utf8 (filename),
                   gimp_pdb_get_last_error (gimp_get_pdb ()));
      return NULL;
    }

  /*
   * Attach the color profile, if any
   */

  if (profile)
    {
      gimp_image_set_color_profile ((GimpImage *) image, profile);
      g_object_unref (profile);

      if (profile_name)
        {
          GimpParasite *parasite;

          parasite = gimp_parasite_new ("icc-profile-name",
                                        GIMP_PARASITE_PERSISTENT |
                                        GIMP_PARASITE_UNDOABLE,
                                        strlen (profile_name),
                                        profile_name);
          gimp_image_attach_parasite ((GimpImage *) image, parasite);
          gimp_parasite_free (parasite);

          g_free (profile_name);
        }
    }

  /*
   * Create the "background" layer to hold the image...
   */

  layer = gimp_layer_new ((GimpImage *) image, _("Background"), width, height,
                          layer_type,
                          100,
                          gimp_image_get_default_new_layer_mode ((GimpImage *) image));
  gimp_image_insert_layer ((GimpImage *) image, layer, NULL, 0);

  file_format = gimp_drawable_get_format (GIMP_DRAWABLE (layer));

  /*
   * Find out everything we can about the image resolution
   * This is only practical with the new 1.0 APIs, I'm afraid
   * due to a bug in libpng-1.0.6, see png-implement for details
   */

  if (png_get_valid (pp, info, PNG_INFO_gAMA))
    {
      GimpParasite *parasite;
      gchar         buf[G_ASCII_DTOSTR_BUF_SIZE];
      gdouble       gamma;

      png_get_gAMA (pp, info, &gamma);

      g_ascii_dtostr (buf, sizeof (buf), gamma);

      parasite = gimp_parasite_new ("gamma",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (buf) + 1, buf);
      gimp_image_attach_parasite ((GimpImage *) image, parasite);
      gimp_parasite_free (parasite);
    }

  if (png_get_valid (pp, info, PNG_INFO_oFFs))
    {
      gint offset_x = png_get_x_offset_pixels (pp, info);
      gint offset_y = png_get_y_offset_pixels (pp, info);

      if (! interactive)
        {
          gimp_layer_set_offsets (layer, offset_x, offset_y);
        }
      else if (offsets_dialog (offset_x, offset_y))
        {
          gimp_layer_set_offsets (layer, offset_x, offset_y);

          if (abs (offset_x) > width ||
              abs (offset_y) > height)
            {
              g_message (_("The PNG file specifies an offset that caused "
                           "the layer to be positioned outside the image."));
            }
        }
    }

  if (png_get_valid (pp, info, PNG_INFO_pHYs))
    {
      png_uint_32  xres;
      png_uint_32  yres;
      gint         unit_type;

      if (png_get_pHYs (pp, info,
                        &xres, &yres, &unit_type) && xres > 0 && yres > 0)
        {
          switch (unit_type)
            {
            case PNG_RESOLUTION_UNKNOWN:
              {
                gdouble image_xres, image_yres;

                gimp_image_get_resolution ((GimpImage *) image, &image_xres, &image_yres);

                if (xres > yres)
                  image_xres = image_yres * (gdouble) xres / (gdouble) yres;
                else
                  image_yres = image_xres * (gdouble) yres / (gdouble) xres;

                gimp_image_set_resolution ((GimpImage *) image, image_xres, image_yres);

                *resolution_loaded = TRUE;
              }
              break;

            case PNG_RESOLUTION_METER:
              gimp_image_set_resolution ((GimpImage *) image,
                                         (gdouble) xres * 0.0254,
                                         (gdouble) yres * 0.0254);
              gimp_image_set_unit ((GimpImage *) image, GIMP_UNIT_MM);

              *resolution_loaded = TRUE;
              break;

            default:
              break;
            }
        }

    }

  gimp_image_set_filename ((GimpImage *) image, filename);

  /*
   * Load the colormap as necessary...
   */

  if (png_get_color_type (pp, info) & PNG_COLOR_MASK_PALETTE)
    {
      png_colorp palette;
      int num_palette;

      png_get_PLTE (pp, info, &palette, &num_palette);
      gimp_image_set_colormap ((GimpImage *) image, (guchar *) palette,
                               num_palette);
    }

  bpp = babl_format_get_bytes_per_pixel (file_format);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  /*
   * Temporary buffer...
   */

  tile_height = gimp_tile_height ();
  pixel = g_new0 (guchar, tile_height * width * bpp);
  pixels = g_new (guchar *, tile_height);

  for (i = 0; i < tile_height; i++)
    pixels[i] = pixel + width * bpp * i;

  /* Install our own error handler to handle incomplete PNG files better */
  error_data.buffer      = buffer;
  error_data.pixel       = pixel;
  error_data.file_format = file_format;
  error_data.tile_height = tile_height;
  error_data.width       = width;
  error_data.height      = height;
  error_data.bpp         = bpp;

  png_set_error_fn (pp, &error_data, on_read_error, NULL);

  for (pass = 0; pass < num_passes; pass++)
    {
      /*
       * This works if you are only reading one row at a time...
       */

      for (begin = 0; begin < height; begin += tile_height)
        {
          end = MIN (begin + tile_height, height);
          num = end - begin;

          if (pass != 0)        /* to handle interlaced PiNGs */
            gegl_buffer_get (buffer,
                             GEGL_RECTANGLE (0, begin, width, num),
                             1.0,
                             file_format,
                             pixel,
                             GEGL_AUTO_ROWSTRIDE,
                             GEGL_ABYSS_NONE);

          error_data.begin = begin;
          error_data.end   = end;
          error_data.num   = num;

          png_read_rows (pp, pixels, NULL, num);

          gegl_buffer_set (buffer,
                           GEGL_RECTANGLE (0, begin, width, num),
                           0,
                           file_format,
                           pixel,
                           GEGL_AUTO_ROWSTRIDE);

          gimp_progress_update
            (((gdouble) pass +
              (gdouble) end / (gdouble) height) /
             (gdouble) num_passes);
        }
    }

  png_read_end (pp, info);

  /* Switch back to default error handler */
  png_set_error_fn (pp, NULL, NULL, NULL);

  if (png_get_text (pp, info, &text, &num_texts))
    {
      gchar *comment = NULL;

      for (i = 0; i < num_texts && !comment; i++, text++)
        {
          if (text->key == NULL || strcmp (text->key, "Comment"))
            continue;

          if (text->text_length > 0)   /*  tEXt  */
            {
              comment = g_convert (text->text, -1,
                                   "UTF-8", "ISO-8859-1",
                                   NULL, NULL, NULL);
            }
          else if (g_utf8_validate (text->text, -1, NULL))
            {                          /*  iTXt  */
              comment = g_strdup (text->text);
            }
        }

      if (comment && *comment)
        {
          GimpParasite *parasite;

          parasite = gimp_parasite_new ("gimp-comment",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (comment) + 1, comment);
          gimp_image_attach_parasite ((GimpImage *) image, parasite);
          gimp_parasite_free (parasite);
        }

      g_free (comment);
    }

  /*
   * Done with the file...
   */

  png_destroy_read_struct (&pp, &info, NULL);

  g_free (pixel);
  g_free (pixels);
  g_object_unref (buffer);
  free (pp);
  free (info);

  fclose (fp);

  if (trns)
    {
      GeglBufferIterator *iter;
      gint                n_components;

      gimp_layer_add_alpha (layer);
      buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
      file_format = gegl_buffer_get_format (buffer);

      iter = gegl_buffer_iterator_new (buffer, NULL, 0, file_format,
                                       GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);
      n_components = babl_format_get_n_components (file_format);
      g_warn_if_fail (n_components == 2);

      while (gegl_buffer_iterator_next (iter))
        {
          guchar *data   = iter->items[0].data;
          gint    length = iter->length;

          while (length--)
            {
              data[1] = alpha[data[0]];

              data += n_components;
            }
        }

      g_object_unref (buffer);
    }

  return (GimpImage *) image;
}

/*
 * 'offsets_dialog ()' - Asks the user about offsets when loading.
 */
static gboolean
offsets_dialog (gint offset_x,
                gint offset_y)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *image;
  GtkWidget *label;
  gchar     *message;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Apply PNG Offset"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, LOAD_PROC,

                            _("Ignore PNG offset"),         GTK_RESPONSE_NO,
                            _("Apply PNG offset to layer"), GTK_RESPONSE_YES,

                            NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_YES,
                                           GTK_RESPONSE_NO,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_icon_name (GIMP_ICON_DIALOG_QUESTION,
                                        GTK_ICON_SIZE_DIALOG);
  gtk_widget_set_valign (image, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  message = g_strdup_printf (_("The PNG image you are importing specifies an "
                               "offset of %d, %d. Do you want to apply "
                               "this offset to the layer?"),
                             offset_x, offset_y);
  label = gtk_label_new (message);
  gtk_label_set_yalign (GTK_LABEL (label), 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_YES);

  gtk_widget_destroy (dialog);

  return run;
}

/*
 * 'save_image ()' - Export the specified image to a PNG file.
 */

static gboolean
save_image (const gchar  *filename,
            GimpImage    *image,
            GimpDrawable *drawable,
            GimpImage    *orig_image,
            gboolean     *profile_saved,
            GError      **error)
{
  gint              i, k;             /* Looping vars */
  gint              bpp = 0;          /* Bytes per pixel */
  gint              type;             /* Type of drawable/layer */
  gint              num_passes;       /* Number of interlace passes in file */
  gint              pass;             /* Current pass in file */
  gint              tile_height;      /* Height of tile in GIMP */
  gint              width;            /* image width */
  gint              height;           /* image height */
  gint              begin;            /* Beginning tile row */
  gint              end;              /* Ending tile row */
  gint              num;              /* Number of rows to load */
  FILE             *fp;               /* File pointer */
  GimpColorProfile *profile = NULL;   /* Color profile */
  gboolean          out_linear;       /* Save linear RGB */
  GeglBuffer       *buffer;           /* GEGL buffer for layer */
  const Babl       *file_format = NULL; /* BABL format of file */
  const gchar      *encoding;
  const Babl       *space;
  png_structp       pp;               /* PNG read pointer */
  png_infop         info;             /* PNG info pointer */
  gint              offx, offy;       /* Drawable offsets from origin */
  guchar          **pixels;           /* Pixel rows */
  guchar           *fixed;            /* Fixed-up pixel data */
  guchar           *pixel;            /* Pixel data */
  gdouble           xres, yres;       /* GIMP resolution (dpi) */
  png_color_16      background;       /* Background color */
  png_time          mod_time;         /* Modification time (ie NOW) */
  time_t            cutime;           /* Time since epoch */
  struct tm        *gmt;              /* GMT broken down */
  gint              color_type;       /* PNG color type */
  gint              bit_depth;        /* Default to bit depth 16 */

  guchar            remap[256];       /* Re-mapping for the palette */

  png_textp         text = NULL;

  out_linear = FALSE;
  space      = gimp_drawable_get_format (drawable);
#if defined(PNG_iCCP_SUPPORTED)
  /* If no profile is written: export as sRGB.
   * If manually assigned profile written: follow its TRC.
   * If default profile written:
   *   - when export as auto or 16-bit: follow the storage TRC.
   *   - when export from 8-bit storage: follow the storage TRC.
   *   - when converting high bit depth to 8-bit: export as sRGB.
   */
  if (pngvals.save_profile)
    {
      profile = gimp_image_get_color_profile (orig_image);

      if (profile                                     ||
          pngvals.export_format == PNG_FORMAT_AUTO    ||
          pngvals.export_format == PNG_FORMAT_RGB16   ||
          pngvals.export_format == PNG_FORMAT_RGBA16  ||
          pngvals.export_format == PNG_FORMAT_GRAY16  ||
          pngvals.export_format == PNG_FORMAT_GRAYA16 ||
          gimp_image_get_precision (image) == GIMP_PRECISION_U8_LINEAR     ||
          gimp_image_get_precision (image) == GIMP_PRECISION_U8_NON_LINEAR ||
          gimp_image_get_precision (image) == GIMP_PRECISION_U8_PERCEPTUAL)
        {
          if (! profile)
            profile = gimp_image_get_effective_color_profile (orig_image);
          out_linear = (gimp_color_profile_is_linear (profile));
        }
      else
        {
          /* When converting higher bit depth work image into 8-bit,
           * with no manually assigned profile, make sure the result is
           * sRGB.
           */
          profile = gimp_image_get_effective_color_profile (orig_image);

          if (gimp_color_profile_is_linear (profile))
            {
              GimpColorProfile *saved_profile;

              saved_profile = gimp_color_profile_new_srgb_trc_from_color_profile (profile);
              g_object_unref (profile);
              profile = saved_profile;
            }
        }

      space = gimp_color_profile_get_space (profile,
                                            GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                            error);
      if (error && *error)
        {
          /* XXX: the profile space should normally be the same one as
           * the drawable's so let's continue with it. We were mostly
           * getting the profile space to be complete. Still let's
           * display the error to standard error channel because if the
           * space could not be extracted, there is a problem somewhere!
           */
          g_printerr ("%s: error getting the profile space: %s",
                     G_STRFUNC, (*error)->message);
          g_clear_error (error);
          space = gimp_drawable_get_format (drawable);
        }
    }
#endif

  /* We save as 8-bit PNG only if:
   * (1) Work image is 8-bit linear with linear profile to be saved.
   * (2) Work image is 8-bit non-linear or perceptual with or without
   * profile.
   */
  bit_depth = 16;
  switch (gimp_image_get_precision (image))
    {
    case GIMP_PRECISION_U8_LINEAR:
      if (out_linear)
        bit_depth = 8;
      break;

    case GIMP_PRECISION_U8_NON_LINEAR:
    case GIMP_PRECISION_U8_PERCEPTUAL:
      if (! out_linear)
        bit_depth = 8;
      break;

    default:
      break;
    }

  pp = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!pp)
    {
      /* this could happen if the compile time and run-time libpng
       * versions do not match.
       */
      g_set_error (error, 0, 0,
                   _("Error creating PNG write struct while exporting '%s'."),
                   gimp_filename_to_utf8 (filename));
      return FALSE;
    }

  info = png_create_info_struct (pp);
  if (! info)
    {
      g_set_error (error, 0, 0,
                   _("Error while exporting '%s'. Could not create PNG header info structure."),
                   gimp_filename_to_utf8 (filename));
      return FALSE;
    }

  if (setjmp (png_jmpbuf (pp)))
    {
      g_set_error (error, 0, 0,
                   _("Error while exporting '%s'. Could not export image."),
                   gimp_filename_to_utf8 (filename));
      return FALSE;
    }

#ifdef PNG_BENIGN_ERRORS_SUPPORTED
  /* Change some libpng errors to warnings (e.g. bug 721135) */
  png_set_benign_errors (pp, TRUE);

  /* bug 765850 */
  png_set_option (pp, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);
#endif

  /*
   * Open the file and initialize the PNG write "engine"...
   */

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_filename_to_utf8 (filename));

  fp = g_fopen (filename, "wb");
  if (fp == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  png_init_io (pp, fp);

  /*
   * Get the buffer for the current image...
   */

  buffer = gimp_drawable_get_buffer (drawable);
  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);
  type   = gimp_drawable_type (drawable);

  /*
   * Initialise remap[]
   */
  for (i = 0; i < 256; i++)
    remap[i] = i;

  if (pngvals.export_format == PNG_FORMAT_AUTO)
    {
    /*
     * Set color type and remember bytes per pixel count
     */

    switch (type)
      {
      case GIMP_RGB_IMAGE:
        color_type = PNG_COLOR_TYPE_RGB;
        if (bit_depth == 8)
          {
            if (out_linear)
              encoding = "RGB u8";
            else
              encoding = "R'G'B' u8";
          }
        else
          {
            if (out_linear)
              encoding = "RGB u16";
            else
              encoding = "R'G'B' u16";
          }
        break;

      case GIMP_RGBA_IMAGE:
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        if (bit_depth == 8)
          {
            if (out_linear)
              encoding = "RGBA u8";
            else
              encoding = "R'G'B'A u8";
          }
        else
          {
            if (out_linear)
              encoding = "RGBA u16";
            else
              encoding = "R'G'B'A u16";
          }
        break;

      case GIMP_GRAY_IMAGE:
        color_type = PNG_COLOR_TYPE_GRAY;
        if (bit_depth == 8)
          {
            if (out_linear)
              encoding = "Y u8";
            else
              encoding = "Y' u8";
          }
        else
          {
            if (out_linear)
              encoding = "Y u16";
            else
              encoding = "Y' u16";
          }
        break;

      case GIMP_GRAYA_IMAGE:
        color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
        if (bit_depth == 8)
          {
            if (out_linear)
              encoding = "YA u8";
            else
              encoding = "Y'A u8";
          }
        else
          {
            if (out_linear)
              encoding = "YA u16";
            else
              encoding = "Y'A u16";
          }
        break;

      case GIMP_INDEXED_IMAGE:
        color_type = PNG_COLOR_TYPE_PALETTE;
        file_format = gimp_drawable_get_format (drawable);
        pngg.has_plte = TRUE;
        pngg.palette = (png_colorp) gimp_image_get_colormap (image,
                                                             &pngg.num_palette);
        bit_depth = get_bit_depth_for_palette (pngg.num_palette);
        break;

      case GIMP_INDEXEDA_IMAGE:
        color_type = PNG_COLOR_TYPE_PALETTE;
        file_format = gimp_drawable_get_format (drawable);
        /* fix up transparency */
        bit_depth = respin_cmap (pp, info, remap, image, drawable);
        break;

      default:
        g_set_error (error, 0, 0, "Image type can't be exported as PNG");
        return FALSE;
      }
    }
  else
    {
      switch (pngvals.export_format)
        {
        case PNG_FORMAT_RGB8:
          color_type = PNG_COLOR_TYPE_RGB;
          if (out_linear)
            encoding = "RGB u8";
          else
            encoding = "R'G'B' u8";
          bit_depth = 8;
          break;
        case PNG_FORMAT_GRAY8:
          color_type = PNG_COLOR_TYPE_GRAY;
          if (out_linear)
            encoding = "Y u8";
          else
            encoding = "Y' u8";
          bit_depth = 8;
          break;
        case PNG_FORMAT_RGBA8:
          color_type = PNG_COLOR_TYPE_RGB_ALPHA;
          if (out_linear)
            encoding = "RGBA u8";
          else
            encoding = "R'G'B'A u8";
          bit_depth = 8;
          break;
        case PNG_FORMAT_GRAYA8:
          color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
          if (out_linear)
            encoding = "YA u8";
          else
            encoding = "Y'A u8";
          bit_depth = 8;
          break;
        case PNG_FORMAT_RGB16:
          color_type = PNG_COLOR_TYPE_RGB;
          if (out_linear)
            encoding = "RGB u16";
          else
            encoding = "R'G'B' u16";
          bit_depth = 16;
          break;
        case PNG_FORMAT_GRAY16:
          color_type = PNG_COLOR_TYPE_GRAY;
          if (out_linear)
            encoding = "Y u16";
          else
            encoding = "Y' u16";
          bit_depth = 16;
          break;
        case PNG_FORMAT_RGBA16:
          color_type = PNG_COLOR_TYPE_RGB_ALPHA;
          if (out_linear)
            encoding = "RGBA u16";
          else
            encoding = "R'G'B'A u16";
          bit_depth = 16;
          break;
        case PNG_FORMAT_GRAYA16:
          color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
          if (out_linear)
            encoding = "YA u16";
          else
            encoding = "Y'A u16";
          bit_depth = 16;
          break;
        case PNG_FORMAT_AUTO:
          g_return_val_if_reached (FALSE);
      }
    }

  if (! file_format)
    file_format = babl_format_with_space (encoding, space);

  bpp = babl_format_get_bytes_per_pixel (file_format);

  /* Note: png_set_IHDR() must be called before any other png_set_*()
     functions. */
  png_set_IHDR (pp, info, width, height, bit_depth, color_type,
                pngvals.interlaced ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_BASE,
                PNG_FILTER_TYPE_BASE);

  if (pngg.has_trns)
    png_set_tRNS (pp, info, pngg.trans, pngg.num_trans, NULL);

  if (pngg.has_plte)
    png_set_PLTE (pp, info, pngg.palette, pngg.num_palette);

  /* Set the compression level */

  png_set_compression_level (pp, pngvals.compression_level);

  /* All this stuff is optional extras, if the user is aiming for smallest
     possible file size she can turn them all off */

  if (pngvals.bkgd)
    {
      GimpRGB color;
      guchar  red, green, blue;

      gimp_context_get_background (&color);
      gimp_rgb_get_uchar (&color, &red, &green, &blue);

      background.index = 0;
      background.red = red;
      background.green = green;
      background.blue = blue;
      background.gray = gimp_rgb_luminance_uchar (&color);
      png_set_bKGD (pp, info, &background);
    }

  if (pngvals.gama)
    {
      GimpParasite *parasite;
      gdouble       gamma = 1.0 / DEFAULT_GAMMA;

      parasite = gimp_image_get_parasite (orig_image, "gamma");
      if (parasite)
        {
          gamma = g_ascii_strtod (gimp_parasite_data (parasite), NULL);
          gimp_parasite_free (parasite);
        }

      png_set_gAMA (pp, info, gamma);
    }

  if (pngvals.offs)
    {
      gimp_drawable_offsets (drawable, &offx, &offy);
      if (offx != 0 || offy != 0)
        png_set_oFFs (pp, info, offx, offy, PNG_OFFSET_PIXEL);
    }

  if (pngvals.phys)
    {
      gimp_image_get_resolution (orig_image, &xres, &yres);
      png_set_pHYs (pp, info, RINT (xres / 0.0254), RINT (yres / 0.0254),
                    PNG_RESOLUTION_METER);
    }

  if (pngvals.time)
    {
      cutime = time (NULL);     /* time right NOW */
      gmt = gmtime (&cutime);

      mod_time.year = gmt->tm_year + 1900;
      mod_time.month = gmt->tm_mon + 1;
      mod_time.day = gmt->tm_mday;
      mod_time.hour = gmt->tm_hour;
      mod_time.minute = gmt->tm_min;
      mod_time.second = gmt->tm_sec;
      png_set_tIME (pp, info, &mod_time);
    }

#if defined(PNG_iCCP_SUPPORTED)
  if (pngvals.save_profile)
    {
      GimpParasite *parasite;
      gchar        *profile_name = NULL;
      const guint8 *icc_data;
      gsize         icc_length;

      icc_data = gimp_color_profile_get_icc_profile (profile, &icc_length);

      parasite = gimp_image_get_parasite (orig_image,
                                          "icc-profile-name");
      if (parasite)
        profile_name = g_convert (gimp_parasite_data (parasite),
                                  gimp_parasite_data_size (parasite),
                                  "UTF-8", "ISO-8859-1", NULL, NULL, NULL);

      png_set_iCCP (pp,
                    info,
                    profile_name ? profile_name : "ICC profile",
                    0,
                    icc_data,
                    icc_length);

      g_free (profile_name);

      *profile_saved = TRUE;
      g_object_unref (profile);
    }
#endif

#ifdef PNG_zTXt_SUPPORTED
/* Small texts are not worth compressing and will be even bigger if compressed.
   Empirical length limit of a text being worth compressing. */
#define COMPRESSION_WORTHY_LENGTH 200
#endif

  if (pngvals.comment)
    {
      GimpParasite *parasite;
      gsize         text_length = 0;

      parasite = gimp_image_get_parasite (orig_image, "gimp-comment");
      if (parasite)
        {
          gchar *comment = g_strndup (gimp_parasite_data (parasite),
                                      gimp_parasite_data_size (parasite));

          gimp_parasite_free (parasite);

          if (comment && strlen (comment) > 0)
            {
              text = g_new0 (png_text, 1);

              text[0].key = "Comment";

#ifdef PNG_iTXt_SUPPORTED

              text[0].text = g_convert (comment, -1,
                                        "ISO-8859-1",
                                        "UTF-8",
                                        NULL,
                                        &text_length,
                                        NULL);

              if (text[0].text == NULL || strlen (text[0].text) == 0)
                {
                  /* We can't convert to ISO-8859-1 without loss.
                     Save the comment as iTXt (UTF-8). */
                  g_free (text[0].text);

                  text[0].text        = g_strdup (comment);
                  text[0].itxt_length = strlen (text[0].text);

#ifdef PNG_zTXt_SUPPORTED
                  text[0].compression = strlen (text[0].text) > COMPRESSION_WORTHY_LENGTH ?
                                        PNG_ITXT_COMPRESSION_zTXt : PNG_ITXT_COMPRESSION_NONE;
#else
                  text[0].compression = PNG_ITXT_COMPRESSION_NONE;
#endif /* PNG_zTXt_SUPPORTED */
                }
              else
                  /* The comment is ISO-8859-1 compatible, so we use tEXt
                     even if there is iTXt support for compatibility to more
                     png reading programs. */
#endif /* PNG_iTXt_SUPPORTED */
                {
#ifndef PNG_iTXt_SUPPORTED
                  /* No iTXt support, so we are forced to use tEXt (ISO-8859-1).
                     A broken comment is better than no comment at all, so the
                     conversion does not fail on unknown character.
                     They are simply ignored. */
                  text[0].text = g_convert_with_fallback (comment, -1,
                                                          "ISO-8859-1",
                                                          "UTF-8",
                                                          "",
                                                          NULL,
                                                          &text_length,
                                                          NULL);
#endif

#ifdef PNG_zTXt_SUPPORTED
                  text[0].compression = strlen (text[0].text) > COMPRESSION_WORTHY_LENGTH ?
                                        PNG_TEXT_COMPRESSION_zTXt : PNG_TEXT_COMPRESSION_NONE;
#else
                  text[0].compression = PNG_TEXT_COMPRESSION_NONE;
#endif /* PNG_zTXt_SUPPORTED */

                  text[0].text_length = text_length;
                 }

              if (! text[0].text || strlen (text[0].text) == 0)
                {
                  g_free (text[0].text);
                  g_free (text);
                  text = NULL;
                }

              g_free (comment);
            }
        }
    }

#ifdef PNG_zTXt_SUPPORTED
#undef COMPRESSION_WORTHY_LENGTH
#endif

  if (text)
    png_set_text (pp, info, text, 1);

  png_write_info (pp, info);
  if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    png_set_swap (pp);

  /*
   * Turn on interlace handling...
   */

  if (pngvals.interlaced)
    num_passes = png_set_interlace_handling (pp);
  else
    num_passes = 1;

  /*
   * Convert unpacked pixels to packed if necessary
   */

  if (color_type == PNG_COLOR_TYPE_PALETTE &&
      bit_depth < 8)
    png_set_packing (pp);

  /*
   * Allocate memory for "tile_height" rows and export the image...
   */

  tile_height = gimp_tile_height ();
  pixel = g_new (guchar, tile_height * width * bpp);
  pixels = g_new (guchar *, tile_height);

  for (i = 0; i < tile_height; i++)
    pixels[i] = pixel + width * bpp * i;

  for (pass = 0; pass < num_passes; pass++)
    {
      /* This works if you are only writing one row at a time... */
      for (begin = 0, end = tile_height;
           begin < height; begin += tile_height, end += tile_height)
        {
          if (end > height)
            end = height;

          num = end - begin;

          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (0, begin, width, num),
                           1.0,
                           file_format,
                           pixel,
                           GEGL_AUTO_ROWSTRIDE,
                           GEGL_ABYSS_NONE);

          /* If we are with a RGBA image and have to pre-multiply the
             alpha channel */
          if (bpp == 4 && ! pngvals.save_transp_pixels)
            {
              for (i = 0; i < num; ++i)
                {
                  fixed = pixels[i];
                  for (k = 0; k < width; ++k)
                    {
                      if (!fixed[3])
                        fixed[0] = fixed[1] = fixed[2] = 0;
                      fixed += bpp;
                    }
                }
            }

          if (bpp == 8 && ! pngvals.save_transp_pixels)
            {
              for (i = 0; i < num; ++i)
                {
                  fixed = pixels[i];
                  for (k = 0; k < width; ++k)
                    {
                      if (!fixed[6] && !fixed[7])
                        fixed[0] = fixed[1] = fixed[2] =
                            fixed[3] = fixed[4] = fixed[5] = 0;
                      fixed += bpp;
                    }
                }
            }

          /* If we're dealing with a paletted image with
           * transparency set, write out the remapped palette */
          if (png_get_valid (pp, info, PNG_INFO_tRNS))
            {
              guchar inverse_remap[256];

              for (i = 0; i < 256; i++)
                inverse_remap[ remap[i] ] = i;

              for (i = 0; i < num; ++i)
                {
                  fixed = pixels[i];
                  for (k = 0; k < width; ++k)
                    {
                      fixed[k] = (fixed[k*2+1] > 127) ?
                                 inverse_remap[ fixed[k*2] ] :
                                 0;
                    }
                }
            }

          /* Otherwise if we have a paletted image and transparency
           * couldn't be set, we ignore the alpha channel */
          else if (png_get_valid (pp, info, PNG_INFO_PLTE) &&
                   bpp == 2)
            {
              for (i = 0; i < num; ++i)
                {
                  fixed = pixels[i];
                  for (k = 0; k < width; ++k)
                    {
                      fixed[k] = fixed[k * 2];
                    }
                }
            }

          png_write_rows (pp, pixels, num);

          gimp_progress_update (((double) pass + (double) end /
                                 (double) height) /
                                (double) num_passes);
        }
    }

  gimp_progress_update (1.0);

  png_write_end (pp, info);
  png_destroy_write_struct (&pp, &info);

  g_free (pixel);
  g_free (pixels);

  /*
   * Done with the file...
   */

  if (text)
    {
      g_free (text[0].text);
      g_free (text);
    }

  free (pp);
  free (info);

  fclose (fp);

  return TRUE;
}

static gboolean
ia_has_transparent_pixels (GeglBuffer *buffer)
{
  GeglBufferIterator *iter;
  const Babl         *format;
  gint                n_components;

  format = gegl_buffer_get_format (buffer);
  iter = gegl_buffer_iterator_new (buffer, NULL, 0, format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);
  n_components = babl_format_get_n_components (format);
  g_return_val_if_fail (n_components == 2, FALSE);

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *data   = iter->items[0].data;
      gint          length = iter->length;

      while (length--)
        {
          if (data[1] <= 127)
            {
              gegl_buffer_iterator_stop (iter);
              return TRUE;
            }

          data += n_components;
        }
    }

  return FALSE;
}

/* Try to find a color in the palette which isn't actually
 * used in the image, so that we can use it as the transparency
 * index. Taken from gif.c */
static gint
find_unused_ia_color (GeglBuffer *buffer,
                      gint       *colors)
{
  GeglBufferIterator *iter;
  const Babl         *format;
  gint                n_components;
  gboolean            ix_used[256];
  gboolean            trans_used = FALSE;
  gint                i;

  for (i = 0; i < *colors; i++)
    ix_used[i] = FALSE;

  format = gegl_buffer_get_format (buffer);
  iter = gegl_buffer_iterator_new (buffer, NULL, 0, format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);
  n_components = babl_format_get_n_components (format);
  g_return_val_if_fail (n_components == 2, FALSE);

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *data   = iter->items[0].data;
      gint          length = iter->length;

      while (length--)
        {
          if (data[1] > 127)
            ix_used[data[0]] = TRUE;
          else
            trans_used = TRUE;

          data += n_components;
        }
    }

  /* If there is no transparency, ignore alpha. */
  if (trans_used == FALSE)
    return -1;

  /* If there is still some room at the end of the palette, increment
   * the number of colors in the image and assign a transparent pixel
   * there. */
  if ((*colors) < 256)
    {
      (*colors)++;

      return (*colors) - 1;
    }

  for (i = 0; i < *colors; i++)
    {
      if (ix_used[i] == FALSE)
        return i;
    }

  return -1;
}


static int
respin_cmap (png_structp   pp,
             png_infop     info,
             guchar       *remap,
             GimpImage    *image,
             GimpDrawable *drawable)
{
  static guchar trans[] = { 0 };
  GeglBuffer *buffer;

  gint          colors;
  guchar       *before;

  before = gimp_image_get_colormap (image, &colors);
  buffer = gimp_drawable_get_buffer (drawable);

  /*
   * Make sure there is something in the colormap.
   */
  if (colors == 0)
    {
      before = g_newa (guchar, 3);
      memset (before, 0, sizeof (guchar) * 3);

      colors = 1;
    }

  /* Try to find an entry which isn't actually used in the
     image, for a transparency index. */

  if (ia_has_transparent_pixels (buffer))
    {
      gint transparent = find_unused_ia_color (buffer, &colors);

      if (transparent != -1)        /* we have a winner for a transparent
                                     * index - do like gif2png and swap
                                     * index 0 and index transparent */
        {
          static png_color palette[256];
          gint      i;

          /* Set tRNS chunk values for writing later. */
          pngg.has_trns = TRUE;
          pngg.trans = trans;
          pngg.num_trans = 1;

          /* Transform all pixels with a value = transparent to
           * 0 and vice versa to compensate for re-ordering in palette
           * due to png_set_tRNS() */

          remap[0] = transparent;
          for (i = 1; i <= transparent; i++)
            remap[i] = i - 1;

          /* Copy from index 0 to index transparent - 1 to index 1 to
           * transparent of after, then from transparent+1 to colors-1
           * unchanged, and finally from index transparent to index 0. */

          for (i = 0; i < colors; i++)
            {
              palette[i].red = before[3 * remap[i]];
              palette[i].green = before[3 * remap[i] + 1];
              palette[i].blue = before[3 * remap[i] + 2];
            }

          /* Set PLTE chunk values for writing later. */
          pngg.has_plte = TRUE;
          pngg.palette = palette;
          pngg.num_palette = colors;
        }
      else
        {
          /* Inform the user that we couldn't losslessly save the
           * transparency & just use the full palette */
          g_message (_("Couldn't losslessly save transparency, "
                       "saving opacity instead."));

          /* Set PLTE chunk values for writing later. */
          pngg.has_plte = TRUE;
          pngg.palette = (png_colorp) before;
          pngg.num_palette = colors;
        }
    }
  else
    {
      /* Set PLTE chunk values for writing later. */
      pngg.has_plte = TRUE;
      pngg.palette = (png_colorp) before;
      pngg.num_palette = colors;
    }

  g_object_unref (buffer);

  return get_bit_depth_for_palette (colors);
}

static GtkWidget *
toggle_button_init (GtkBuilder  *builder,
                    const gchar *name,
                    gboolean     initial_value,
                    gboolean    *value_pointer)
{
  GtkWidget *toggle = NULL;

  toggle = GTK_WIDGET (gtk_builder_get_object (builder, name));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), initial_value);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    value_pointer);

  return toggle;
}

static void pixformat_changed (GtkWidget *widget,
                               void      *foo)
{
  PngExportFormat *ep = foo;
  *ep = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
}

static gboolean
save_dialog (GimpImage *image,
             gboolean   alpha)
{
  PngSaveGui    pg;
  GtkWidget    *dialog;
  GtkBuilder   *builder;
  gchar        *ui_file;
  GimpParasite *parasite;
  GError       *error = NULL;

  /* Dialog init */
  dialog = gimp_export_dialog_new (_("PNG"), PLUG_IN_BINARY, SAVE_PROC);
  g_signal_connect (dialog, "response",
                    G_CALLBACK (save_dialog_response),
                    &pg);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  /* GtkBuilder init */
  builder = gtk_builder_new ();
  ui_file = g_build_filename (gimp_data_directory (),
                              "ui/plug-ins/plug-in-file-png.ui",
                              NULL);
  if (! gtk_builder_add_from_file (builder, ui_file, &error))
    {
      gchar *display_name = g_filename_display_name (ui_file);

      g_printerr (_("Error loading UI file '%s': %s"),
                  display_name, error ? error->message : _("Unknown error"));

      g_free (display_name);
    }

  g_free (ui_file);

  /* Grid */
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      GTK_WIDGET (gtk_builder_get_object (builder, "grid")),
                      FALSE, FALSE, 0);

  /* Toggles */
  pg.interlaced = toggle_button_init (builder, "interlace",
                                      pngvals.interlaced,
                                      &pngvals.interlaced);
  pg.bkgd = toggle_button_init (builder, "save-background-color",
                                pngvals.bkgd,
                                &pngvals.bkgd);
  pg.gama = toggle_button_init (builder, "save-gamma",
                                pngvals.gama,
                                &pngvals.gama);
  pg.offs = toggle_button_init (builder, "save-layer-offset",
                                pngvals.offs,
                                &pngvals.offs);
  pg.phys = toggle_button_init (builder, "save-resolution",
                                pngvals.phys,
                                &pngvals.phys);
  pg.time = toggle_button_init (builder, "save-creation-time",
                                pngvals.time,
                                &pngvals.time);
  pg.save_exif = toggle_button_init (builder, "save-exif",
                                     pngvals.save_exif,
                                     &pngvals.save_exif);
  pg.save_xmp = toggle_button_init (builder, "save-xmp",
                                    pngvals.save_xmp,
                                    &pngvals.save_xmp);
  pg.save_iptc = toggle_button_init (builder, "save-iptc",
                                     pngvals.save_iptc,
                                     &pngvals.save_iptc);
  pg.save_thumbnail = toggle_button_init (builder, "save-thumbnail",
                                          pngvals.save_thumbnail,
                                          &pngvals.save_thumbnail);
  pg.save_profile = toggle_button_init (builder, "save-color-profile",
                                        pngvals.save_profile,
                                        &pngvals.save_profile);

#if !defined(PNG_iCCP_SUPPORTED)
  gtk_widget_hide (pg.save_profile);
#endif

  /* Comment toggle */
  parasite = gimp_image_get_parasite (image, "gimp-comment");
  pg.comment =
    toggle_button_init (builder, "save-comment",
                        pngvals.comment && parasite != NULL,
                        &pngvals.comment);
  gtk_widget_set_sensitive (pg.comment, parasite != NULL);
  gimp_parasite_free (parasite);

  /* Transparent pixels toggle */
  pg.save_transp_pixels =
    toggle_button_init (builder,
                        "save-transparent-pixels",
                        alpha && pngvals.save_transp_pixels,
                        &pngvals.save_transp_pixels);
  gtk_widget_set_sensitive (pg.save_transp_pixels, alpha);

  /* Compression level scale */
  pg.compression_level =
    GTK_ADJUSTMENT (gtk_builder_get_object (builder, "compression-level"));

  gtk_adjustment_set_value (pg.compression_level, pngvals.compression_level);

  g_signal_connect (pg.compression_level, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &pngvals.compression_level);

  /* Compression level scale */
  pg.pixelformat =
    GTK_WIDGET (gtk_builder_get_object (builder, "pixelformat-combo"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (pg.pixelformat), pngvals.export_format);
  g_signal_connect (pg.pixelformat, "changed",
                    G_CALLBACK (pixformat_changed),
                    &pngvals.export_format);

#if 0
  gtk_adjustment_set_value (pg.compression_level, pngvals.compression_level);
  g_signal_connect (pg.compression_level, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &pngvals.compression_level);
#endif

  /* Load/save defaults buttons */
  g_signal_connect_swapped (gtk_builder_get_object (builder, "load-defaults"),
                            "clicked",
                            G_CALLBACK (load_gui_defaults),
                            &pg);

  g_signal_connect_swapped (gtk_builder_get_object (builder, "save-defaults"),
                            "clicked",
                            G_CALLBACK (save_parasite),
                            &pg);

  /* Show dialog and run */
  gtk_widget_show (dialog);

  pg.run = FALSE;

  gtk_main ();

  return pg.run;
}

static void
save_dialog_response (GtkWidget *widget,
                      gint       response_id,
                      gpointer   data)
{
  PngSaveGui *pg = data;

  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      pg->run = TRUE;

    default:
      gtk_widget_destroy (widget);
      break;
    }
}

static void
load_parasite (void)
{
  GimpParasite *parasite;

  parasite = gimp_get_parasite (PNG_DEFAULTS_PARASITE);

  if (parasite)
    {
      gchar        *def_str;
      PngSaveVals   tmpvals = defaults;
      gint          num_fields;

      def_str = g_strndup (gimp_parasite_data (parasite),
                           gimp_parasite_data_size (parasite));

      gimp_parasite_free (parasite);

      num_fields = sscanf (def_str, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                           &tmpvals.interlaced,
                           &tmpvals.bkgd,
                           &tmpvals.gama,
                           &tmpvals.offs,
                           &tmpvals.phys,
                           &tmpvals.time,
                           &tmpvals.comment,
                           &tmpvals.save_transp_pixels,
                           &tmpvals.compression_level,
                           &tmpvals.save_exif,
                           &tmpvals.save_xmp,
                           &tmpvals.save_iptc,
                           &tmpvals.save_thumbnail,
                           &tmpvals.save_profile);

      g_free (def_str);

      if (num_fields == 9 || num_fields == 13 || num_fields == 14)
        pngvals = tmpvals;
    }
}

static void
save_parasite (void)
{
  GimpParasite *parasite;
  gchar        *def_str;

  def_str = g_strdup_printf ("%d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                             pngvals.interlaced,
                             pngvals.bkgd,
                             pngvals.gama,
                             pngvals.offs,
                             pngvals.phys,
                             pngvals.time,
                             pngvals.comment,
                             pngvals.save_transp_pixels,
                             pngvals.compression_level,
                             pngvals.save_exif,
                             pngvals.save_xmp,
                             pngvals.save_iptc,
                             pngvals.save_thumbnail,
                             pngvals.save_profile);

  parasite = gimp_parasite_new (PNG_DEFAULTS_PARASITE,
                                GIMP_PARASITE_PERSISTENT,
                                strlen (def_str), def_str);

  gimp_attach_parasite (parasite);

  gimp_parasite_free (parasite);
  g_free (def_str);
}

static void
load_gui_defaults (PngSaveGui *pg)
{
  /* initialize with hardcoded defaults */
  pngvals = defaults;
  /* Override with parasite. */
  load_parasite ();

#define SET_ACTIVE(field) \
  if (gtk_widget_is_sensitive (pg->field)) \
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pg->field), pngvals.field)

  SET_ACTIVE (interlaced);
  SET_ACTIVE (bkgd);
  SET_ACTIVE (gama);
  SET_ACTIVE (offs);
  SET_ACTIVE (phys);
  SET_ACTIVE (time);
  SET_ACTIVE (comment);
  SET_ACTIVE (save_transp_pixels);
  SET_ACTIVE (save_exif);
  SET_ACTIVE (save_xmp);
  SET_ACTIVE (save_iptc);
  SET_ACTIVE (save_thumbnail);
  SET_ACTIVE (save_profile);

#undef SET_ACTIVE

  gtk_adjustment_set_value (pg->compression_level,
                            pngvals.compression_level);
}
