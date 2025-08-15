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
 *   APNG code referenced from APNG Disassembler by Max Stepin at
 *   http://apngdis.sourceforge.net
 */

#include "config.h"

#include <stdlib.h>
#include <errno.h>

#include <glib/gstdio.h>
#include "lcms2.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <png.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC       "file-png-load"
#define LOAD_APNG_PROC  "file-apng-load"
#define EXPORT_PROC     "file-png-export"
#define PLUG_IN_BINARY  "file-png"
#define PLUG_IN_ROLE    "gimp-file-png"

#define PLUG_IN_VERSION "1.3.4 - 03 September 2002"
#define SCALE_WIDTH     125

#define DEFAULT_GAMMA   2.20

/* APNG */
#define id_IHDR 0x52444849
#define id_tRNS 0x534E5274
#define id_acTL 0x4C546361
#define id_fcTL 0x4C546366
#define id_IDAT 0x54414449
#define id_fdAT 0x54416466
#define id_IEND 0x444E4549

typedef enum _PngExportformat
{
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

static GSList *safe_to_copy_chunks;

typedef struct
{
  guchar *data;
  guint   size;
} APNGChunk;

typedef struct {
  guchar  *pixels;
  guint    width;
  guint    height;
  guint    bpp;
  guint    offset_x;
  guint    offset_y;
  guint16  delay_num;
  guint16  delay_den;

  guint    image_width;
} APNGFrame;

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


#define PNG_TYPE (png_get_type ())
#define PNG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PNG_TYPE, Png))

GType                   png_get_type         (void) G_GNUC_CONST;

static GList          * png_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * png_create_procedure (GimpPlugIn            *plug_in,
                                              const gchar           *name);

static GimpValueArray * png_load             (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GFile                 *file,
                                              GimpMetadata          *metadata,
                                              GimpMetadataLoadFlags *flags,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);
static GimpValueArray * apng_load            (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GFile                 *file,
                                              GimpMetadata          *metadata,
                                              GimpMetadataLoadFlags *flags,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);
static GimpValueArray * png_export           (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GimpImage             *image,
                                              GFile                 *file,
                                              GimpExportOptions     *options,
                                              GimpMetadata          *metadata,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);

static GimpImage * load_image                (GFile                 *file,
                                              gboolean               report_progress,
                                              gboolean              *resolution_loaded,
                                              gboolean              *profile_loaded,
                                              gboolean              *is_apng,
                                              GError               **error);
static gboolean    load_apng_image           (GFile                 *file,
                                              GimpImage             *image,
                                              gboolean               report_progress,
                                              GError               **error);
static gboolean    export_image              (GFile                 *file,
                                              GimpImage             *image,
                                              GimpDrawable          *drawable,
                                              GimpImage             *orig_image,
                                              GObject               *config,
                                              gint                  *bits_per_sample,
                                              gboolean               report_progress,
                                              GError               **error);

static int         respin_cmap               (png_structp            pp,
                                              png_infop              info,
                                              guchar                *remap,
                                              GimpImage             *image,
                                              GimpDrawable          *drawable);

static gboolean    export_dialog             (GimpImage             *image,
                                              GimpProcedure         *procedure,
                                              GObject               *config,
                                              gboolean               alpha);

static gboolean    offsets_dialog            (gint                   offset_x,
                                              gint                   offset_y);

static gboolean    ia_has_transparent_pixels (GeglBuffer            *buffer);

static gint        find_unused_ia_color      (GeglBuffer            *buffer,
                                              gint                  *colors);

static void        add_alpha_to_indexed      (GimpLayer             *layer,
                                              guchar                *alpha);

static gboolean    restart_apng_processing   (png_structp           *png_ptr,
                                              png_infop             *info_ptr,
                                              void                  *frame_ptr,
                                              gboolean               fctl_read,
                                              APNGChunk             *ihdr_chunk,
                                              GList                 *apng_chunks);
static gboolean     process_apng_data        (png_structp            png_ptr,
                                              png_infop              info_ptr,
                                              guchar                *data,
                                              guint                  size);
static gboolean     end_apng_processing      (png_structp            png_ptr,
                                              png_infop              info_ptr);
static void         read_apng_header         (png_structp            png_ptr,
                                              png_infop              info_ptr);
static void         read_apng_row            (png_structp            png_ptr,
                                              png_bytep              new_row,
                                              png_uint_32            row_num,
                                              gint                   pass);
static inline guint read_apng_chunk          (FILE                  *f,
                                              APNGChunk             *chunk);
static void         create_apng_layer        (GimpImage             *image,
                                              APNGFrame             *apng_frame,
                                              guchar                *prior_pixels,
                                              GimpImageType          image_type,
                                              guchar                *alpha,
                                              gboolean               is_indexed_alpha,
                                              gint                   dispose_op,
                                              gint                   blend_op);

static gint        read_unknown_chunk        (png_structp            png_ptr,
                                              png_unknown_chunkp     chunk);


G_DEFINE_TYPE (Png, png, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PNG_TYPE)
DEFINE_STD_SET_I18N


static void
png_class_init (PngClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = png_query_procedures;
  plug_in_class->create_procedure = png_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
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
  list = g_list_append (list, g_strdup (LOAD_APNG_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static GimpProcedure *
png_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           png_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("PNG image"));

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
  else if (! strcmp (name, LOAD_APNG_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           apng_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("APNG image"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads files in APNG file format",
                                        "This plug-in loads Animated Portable Network "
                                        "Graphics (APNG) files.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Alx Sa",
                                      "Alx Sa",
                                      "2025");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/apng");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "apng");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             TRUE, png_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("PNG image"));

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

      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("PNG"));
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/png");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "png");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB     |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                              GIMP_EXPORT_CAN_HANDLE_ALPHA,
                                              NULL, NULL, NULL);

      gimp_procedure_add_boolean_argument (procedure, "interlaced",
                                           _("_Interlacing (Adam7)"),
                                           _("Use Adam7 interlacing"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "compression",
                                       _("Co_mpression level"),
                                       _("Deflate Compression factor (0..9)"),
                                       0, 9, 9,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "bkgd",
                                           _("Save _background color"),
                                           _("Write bKGD chunk (PNG metadata)"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "offs",
                                           _("Save layer o_ffset"),
                                           _("Write oFFs chunk (PNG metadata)"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "phys",
                                           _("Save resol_ution"),
                                           _("Write pHYs chunk (PNG metadata)"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "time",
                                           _("Save creation _time"),
                                           _("Write tIME chunk (PNG metadata)"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "save-transparent",
                                           _("Save color _values from transparent pixels"),
                                           _("Preserve color of completely transparent pixels"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "optimize-palette",
                                           _("_Optimize for smallest possible palette size"),
                                           _("When checked, save as 1, 2, 4, or 8-bit depending"
                                             " on number of colors used. When unchecked, always"
                                             " save as 8-bit"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "format",
                                          _("_Pixel format"),
                                          _("PNG export format"),
                                          gimp_choice_new_with_values ("auto",    PNG_FORMAT_AUTO,    _("Automatic"),    NULL,
                                                                       "rgb8",    PNG_FORMAT_RGB8,    _("8 bpc RGB"),    NULL,
                                                                       "gray8",   PNG_FORMAT_GRAY8,   _("8 bpc GRAY"),   NULL,
                                                                       "rgba8",   PNG_FORMAT_RGBA8,   _("8 bpc RGBA"),   NULL,
                                                                       "graya8",  PNG_FORMAT_GRAYA8,  _("8 bpc GRAYA"),  NULL,
                                                                       "rgb16",   PNG_FORMAT_RGB16,   _("16 bpc RGB"),   NULL,
                                                                       "gray16",  PNG_FORMAT_GRAY16,  _("16 bpc GRAY"),  NULL,
                                                                       "rgba16",  PNG_FORMAT_RGBA16,  _("16 bpc RGBA"),  NULL,
                                                                       "graya16", PNG_FORMAT_GRAYA16, _("16 bpc GRAYA"), NULL,
                                                                       NULL),
                                          "auto", G_PARAM_READWRITE);

      gimp_export_procedure_set_support_exif      (GIMP_EXPORT_PROCEDURE (procedure), TRUE);
      gimp_export_procedure_set_support_iptc      (GIMP_EXPORT_PROCEDURE (procedure), TRUE);
      gimp_export_procedure_set_support_xmp       (GIMP_EXPORT_PROCEDURE (procedure), TRUE);
#if defined(PNG_iCCP_SUPPORTED)
      gimp_export_procedure_set_support_profile   (GIMP_EXPORT_PROCEDURE (procedure), TRUE);
#endif
      gimp_export_procedure_set_support_thumbnail (GIMP_EXPORT_PROCEDURE (procedure), TRUE);
      gimp_export_procedure_set_support_comment   (GIMP_EXPORT_PROCEDURE (procedure), TRUE);
    }

  return procedure;
}

static GimpValueArray *
png_load (GimpProcedure         *procedure,
          GimpRunMode            run_mode,
          GFile                 *file,
          GimpMetadata          *metadata,
          GimpMetadataLoadFlags *flags,
          GimpProcedureConfig   *config,
          gpointer               run_data)
{
  GimpValueArray *return_vals;
  gboolean        report_progress   = FALSE;
  gboolean        resolution_loaded = FALSE;
  gboolean        profile_loaded    = FALSE;
  gboolean        is_apng           = FALSE;
  GimpImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    {
      gimp_ui_init (PLUG_IN_BINARY);
      report_progress = TRUE;
    }

  image = load_image (file,
                      report_progress,
                      &resolution_loaded,
                      &profile_loaded,
                      &is_apng,
                      &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  /* Some APNGs have .png extensions */
  if (is_apng)
    load_apng_image (file, image, report_progress, &error);

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
apng_load (GimpProcedure         *procedure,
           GimpRunMode            run_mode,
           GFile                 *file,
           GimpMetadata          *metadata,
           GimpMetadataLoadFlags *flags,
           GimpProcedureConfig   *config,
           gpointer               run_data)
{
  GimpValueArray *return_vals;
  gboolean        report_progress   = FALSE;
  gboolean        resolution_loaded = FALSE;
  gboolean        profile_loaded    = FALSE;
  gboolean        is_apng           = TRUE;
  GimpImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    {
      gimp_ui_init (PLUG_IN_BINARY);
      report_progress = TRUE;
    }

  /* Load PNG first to handle set-up, then we'll add frames later */
  image = load_image (file,
                      report_progress,
                      &resolution_loaded,
                      &profile_loaded,
                      &is_apng,
                      &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  load_apng_image (file, image, report_progress, &error);

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
png_export (GimpProcedure        *procedure,
            GimpRunMode           run_mode,
            GimpImage            *image,
            GFile                *file,
            GimpExportOptions    *options,
            GimpMetadata         *metadata,
            GimpProcedureConfig  *config,
            gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_IGNORE;
  GList             *drawables;
  GimpImage         *orig_image;
  gboolean           alpha;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  orig_image = image;

  export = gimp_export_options_get_image (options, &image);
  drawables = gimp_image_list_layers (image);
  alpha = gimp_drawable_has_alpha (drawables->data);

  /* If the image has no transparency, then there is usually no need
   * to save a bKGD chunk. For more information, see:
   * http://bugzilla.gnome.org/show_bug.cgi?id=92395
   */
  if (! alpha)
    g_object_set (config,
                  "bkgd", FALSE,
                  NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_ui_init (PLUG_IN_BINARY);

      if (! export_dialog (orig_image, procedure, G_OBJECT (config), alpha))
        status = GIMP_PDB_CANCEL;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      gint bits_per_sample;

      if (export_image (file, image, drawables->data, orig_image, G_OBJECT (config),
                        &bits_per_sample, run_mode != GIMP_RUN_NONINTERACTIVE,
                        &error))
        {
          if (metadata)
            gimp_metadata_set_bits_per_sample (metadata, bits_per_sample);
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
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

/* Copied from src/cmsvirt.c in Little-CMS. */
static cmsToneCurve *
Build_sRGBGamma (cmsContext ContextID)
{
    cmsFloat64Number Parameters[5];

    Parameters[0] = 2.4;
    Parameters[1] = 1. / 1.055;
    Parameters[2] = 0.055 / 1.055;
    Parameters[3] = 1. / 12.92;
    Parameters[4] = 0.04045;

    return cmsBuildParametricToneCurve (ContextID, 4, Parameters);
}

/*
 * 'load_image()' - Load a PNG image into a new image window.
 */
static GimpImage *
load_image (GFile        *file,
            gboolean      report_progress,
            gboolean     *resolution_loaded,
            gboolean     *profile_loaded,
            gboolean     *is_apng,
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
  FILE             *fp;                   /* File pointer */
  volatile GimpImage *image      = NULL;  /* Image -- protected for setjmp() */
  GimpLayer        *layer;                /* Layer */
  GeglBuffer       *buffer;               /* GEGL buffer for layer */
  const Babl       *file_format;          /* BABL format for layer */
  png_structp       pp;                   /* PNG read pointer */
  png_infop         info;                 /* PNG info pointers */
  png_voidp         user_chunkp;          /* PNG unknown chunk pointer */
  guchar          **pixels;               /* Pixel rows */
  guchar           *pixel;                /* Pixel data */
  guchar            alpha[256];           /* Index -> Alpha */
  png_textp         text;
  gint              num_texts;
  struct read_error_data error_data;

  safe_to_copy_chunks = NULL;

  pp = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (! pp)
    {
      /* this could happen if the compile time and run-time libpng
         versions do not match. */

      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error creating PNG read struct while loading '%s'."),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  info = png_create_info_struct (pp);
  if (! info)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error while reading '%s'. Could not create PNG header info structure."),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  if (setjmp (png_jmpbuf (pp)))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error while reading '%s'. File corrupted?"),
                   gimp_file_get_utf8_name (file));
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

  if (report_progress)
    gimp_progress_init_printf (_("Opening '%s'"),
                               gimp_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (fp == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  png_init_io (pp, fp);
  png_set_compression_buffer_size (pp, 512);

  /* Set up callback to save "safe to copy" chunks */
  png_set_keep_unknown_chunks (pp, PNG_HANDLE_CHUNK_IF_SAFE, NULL, 0);
  user_chunkp = png_get_user_chunk_ptr (pp);
  png_set_read_user_chunk_fn (pp, user_chunkp, read_unknown_chunk);

  /*
   * Get the image info
   */

  png_read_info (pp, info);

  if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    png_set_swap (pp);

  /*
   * Get the iCCP (color profile) chunk, if any.
   */
  profile = load_color_profile (pp, info, &profile_name);

  if (! profile && ! png_get_valid (pp, info, PNG_INFO_sRGB) &&
      (png_get_valid (pp, info, PNG_INFO_gAMA) ||
       png_get_valid (pp, info, PNG_INFO_cHRM)))
    {
      /* This is kind of a special case for PNG. If an image has no
       * profile, and the sRGB chunk is not set, and either gAMA or cHRM
       * (or ideally both) are set, then we generate a profile from
       * these data on import. See #3265.
       */
      cmsToneCurve    *gamma_curve[3];
      cmsCIExyY        whitepoint;
      cmsCIExyYTRIPLE  primaries;
      cmsHPROFILE      cms_profile = NULL;
      gdouble          gamma = 1.0 / DEFAULT_GAMMA;

      if (png_get_valid (pp, info, PNG_INFO_gAMA) &&
          png_get_gAMA (pp, info, &gamma) == PNG_INFO_gAMA)
        {
          gamma_curve[0] = gamma_curve[1] = gamma_curve[2] = cmsBuildGamma (NULL, 1.0 / gamma);
        }
      else
        {
          /* Use the sRGB gamma curve. */
          gamma_curve[0] = gamma_curve[1] = gamma_curve[2] = Build_sRGBGamma (NULL);
        }

      if (png_get_valid (pp, info, PNG_INFO_cHRM) &&
          png_get_cHRM (pp, info, &whitepoint.x, &whitepoint.y,
                        &primaries.Red.x,   &primaries.Red.y,
                        &primaries.Green.x, &primaries.Green.y,
                        &primaries.Blue.x,  &primaries.Blue.y) == PNG_INFO_cHRM)
        {
          whitepoint.Y = primaries.Red.Y = primaries.Green.Y = primaries.Blue.Y = 1.0;
        }
      else
        {
          /* Rec709 primaries and D65 whitepoint as copied from
           * cmsCreate_sRGBProfileTHR() in Little-CMS.
           */
          cmsCIExyY       d65_whitepoint   = { 0.3127, 0.3290, 1.0 };
          cmsCIExyYTRIPLE rec709_primaries =
            {
                {0.6400, 0.3300, 1.0},
                {0.3000, 0.6000, 1.0},
                {0.1500, 0.0600, 1.0}
            };

          memcpy (&whitepoint, &d65_whitepoint, sizeof whitepoint);
          memcpy (&primaries, &rec709_primaries, sizeof primaries);
        }

      if (png_get_color_type (pp, info) == PNG_COLOR_TYPE_GRAY ||
          png_get_color_type (pp, info) == PNG_COLOR_TYPE_GRAY_ALPHA)
        cms_profile = cmsCreateGrayProfile (&whitepoint, gamma_curve[0]);
      else /* RGB, RGB with Alpha and Indexed. */
        cms_profile = cmsCreateRGBProfile (&whitepoint, &primaries, gamma_curve);

      cmsFreeToneCurve (gamma_curve[0]);
      g_warn_if_fail (cms_profile != NULL);

      if (cms_profile != NULL)
        {
          /* Customize the profile description to show it is generated
           * from PNG metadata.
           */
          gchar      *profile_desc;
          cmsMLU     *description_mlu;
          cmsContext  context_id = cmsGetProfileContextID (cms_profile);

          /* Note that I am not trying to localize these strings on purpose
           * because cmsMLUsetASCII() expects ASCII. Maybe we should move to
           * using cmsMLUsetWide() if we want the generated profile
           * descriptions to be localized. XXX
           */
          if ((png_get_valid (pp, info, PNG_INFO_gAMA) && png_get_valid (pp, info, PNG_INFO_cHRM)))
            profile_desc = g_strdup_printf ("Generated %s profile from PNG's gAMA (gamma %.4f) and cHRM chunks",
                                            (png_get_color_type (pp, info) == PNG_COLOR_TYPE_GRAY ||
                                             png_get_color_type (pp, info) == PNG_COLOR_TYPE_GRAY_ALPHA) ?
                                            "grayscale" : "RGB", 1.0 / gamma);
          else if (png_get_valid (pp, info, PNG_INFO_gAMA))
            profile_desc = g_strdup_printf ("Generated %s profile from PNG's gAMA chunk (gamma %.4f)",
                                            (png_get_color_type (pp, info) == PNG_COLOR_TYPE_GRAY ||
                                             png_get_color_type (pp, info) == PNG_COLOR_TYPE_GRAY_ALPHA) ?
                                            "grayscale" : "RGB", 1.0 / gamma);
          else
            profile_desc = g_strdup_printf ("Generated %s profile from PNG's cHRM chunk",
                                            (png_get_color_type (pp, info) == PNG_COLOR_TYPE_GRAY ||
                                             png_get_color_type (pp, info) == PNG_COLOR_TYPE_GRAY_ALPHA) ?
                                            "grayscale" : "RGB");

          description_mlu  = cmsMLUalloc (context_id, 1);

          cmsMLUsetASCII (description_mlu,  "en", "US", profile_desc);
          cmsWriteTag (cms_profile, cmsSigProfileDescriptionTag, description_mlu);

          profile = gimp_color_profile_new_from_lcms_profile (cms_profile, NULL);

          g_free (profile_desc);
          cmsMLUfree (description_mlu);
          cmsCloseProfile (cms_profile);
        }
    }

  if (profile)
    *profile_loaded = TRUE;

  /*
   * Get image precision and color model.
   * Note that we always import PNG as non-linear. The data might be
   * actually linear because of a linear profile, or because of a gAMA
   * chunk with 1.0 value (which we convert to a profile above). But
   * then we'll just set the right profile and that's it. Other than
   * this, PNG doesn't have (that I can see in the spec) any kind of
   * flag saying that data is linear, bypassing the profile's TRC so
   * there is basically no reason to explicitly set a linear precision.
   */

  if (png_get_bit_depth (pp, info) == 16)
    image_precision = GIMP_PRECISION_U16_NON_LINEAR;
  else
    image_precision = GIMP_PRECISION_U8_NON_LINEAR;

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
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Unknown color model in PNG file '%s'."),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  width = png_get_image_width (pp, info);
  height = png_get_image_height (pp, info);

  image = gimp_image_new_with_precision (width, height,
                                         image_type, image_precision);
  if (! image)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Could not create new image for '%s': %s"),
                   gimp_file_get_utf8_name (file),
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

  if (png_get_valid (pp, info, PNG_INFO_oFFs))
    {
      gint offset_x = png_get_x_offset_pixels (pp, info);
      gint offset_y = png_get_y_offset_pixels (pp, info);

      if (offset_x != 0 ||
          offset_y != 0)
        {
          if (! report_progress)
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
              gimp_image_set_unit ((GimpImage *) image, gimp_unit_mm ());

              *resolution_loaded = TRUE;
              break;

            default:
              break;
            }
        }

    }

  /*
   * Load the colormap as necessary...
   */

  if (png_get_color_type (pp, info) & PNG_COLOR_MASK_PALETTE)
    {
      png_colorp palette;
      int num_palette;

      png_get_PLTE (pp, info, &palette, &num_palette);
      gimp_palette_set_colormap (gimp_image_get_palette ((GimpImage *) image), babl_format ("R'G'B' u8"),
                                 (guchar *) palette, num_palette * 3);
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

          if (report_progress)
            gimp_progress_update (((gdouble) pass +
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
    add_alpha_to_indexed (layer, alpha);

  /* If any safe-to-copy chunks were saved,
   * store them in the image as parasite */
  if (safe_to_copy_chunks)
    {
      GSList *iter;

      for (iter = safe_to_copy_chunks; iter; iter = iter->next)
        {
          GimpParasite *parasite = iter->data;

          if (strcmp (gimp_parasite_get_name (parasite), "png/fcTL") == 0)
            *is_apng = TRUE;
          else
            gimp_image_attach_parasite ((GimpImage *) image, parasite);
          gimp_parasite_free (parasite);
        }

      g_slist_free (safe_to_copy_chunks);
    }

  return (GimpImage *) image;
}

static gboolean
load_apng_image (GFile      *file,
                 GimpImage  *image,
                 gboolean    report_progress,
                 GError    **error)
{
  GimpLayer     **layers;
  GimpImageType   image_type;
  gint            bpp;
  guchar          sig[8];
  guchar          trns[256];
  FILE           *fp;
  png_structp     pp;
  png_infop       info;
  /* APNG specific data */
  gboolean        is_indexed_alpha = FALSE;
  gboolean        is_animated      = FALSE;
  gboolean        idat_read        = FALSE;
  gint            frame_start      = 0;
  guint           sequence_num     = 0;
  guint           frame_no;
  guint           playback_no;
  guint           frame_data_size  = 0;

  layers     = gimp_image_get_layers (image);
  image_type = gimp_drawable_type (GIMP_DRAWABLE (layers[0]));
  bpp        = gimp_drawable_get_bpp (GIMP_DRAWABLE (layers[0]));
  g_free (layers);

  if (image_type == GIMP_INDEXEDA_IMAGE)
    {
      image_type       = GIMP_INDEXED_IMAGE;
      bpp              = 1;
      is_indexed_alpha = TRUE;

      for (gint i = 0; i < 256; i++)
        trns[i] = 255;
    }

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (fp == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  if (report_progress)
    gimp_progress_init_printf (_("Importing APNG frames from '%s'"),
                               gimp_file_get_utf8_name (file));

  if (fread (sig, 1, 8, fp)   == 8 &&
      png_sig_cmp (sig, 0, 8) == 0)
    {
      guint      png_id       = 0;
      APNGChunk  ihdr_chunk;
      APNGChunk  chunk;
      APNGFrame  apng_frame;
      guchar    *prior_pixels;
      guchar     dispose_op   = 0;
      guchar     blend_op     = 0;
      GList     *other_chunks = NULL;

      png_id = read_apng_chunk (fp, &ihdr_chunk);
      if (! png_id)
        {
          fclose (fp);
          if (ihdr_chunk.data)
            g_free (ihdr_chunk.data);

          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Could not read APNG frames. "
                         "They will be discarded."));

          return FALSE;
        }

      /* Set initial frame information */
      if (png_id == id_IHDR)
        {
          gdouble image_width  = gimp_image_get_width (image);
          gdouble image_height = gimp_image_get_height (image);

          frame_data_size = image_width * image_height * bpp;

          apng_frame.bpp         = bpp;
          apng_frame.image_width = image_width;
          apng_frame.pixels      = g_malloc0 (frame_data_size);
          prior_pixels           = g_malloc0 (frame_data_size);

          /* In case we have IDAT before fcTL chunk, set the frame defaults */
          apng_frame.width     = image_width;
          apng_frame.height    = image_height;
          apng_frame.offset_x  = 0;
          apng_frame.offset_y  = 0;
          apng_frame.delay_num = 0;
          apng_frame.delay_den = 100;

          dispose_op = 0;
          blend_op   = 0;
        }
      else
        {
          fclose (fp);
          if (ihdr_chunk.data)
            g_free (ihdr_chunk.data);

          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Could not read APNG frames. "
                         "They will be discarded."));

          return FALSE;
        }

      /* libpng will only read one image chunk for PNG, and will warn that
       * there is extra compressed data instead of reading the rest. So,
       * each time we want to read a new frame, we'll recreate the PNG
       * reader and trick it into thinking it is reading a single frame. */
      if (restart_apng_processing (&pp, &info, &apng_frame, FALSE, &ihdr_chunk,
                                   NULL))
        {
          while (! feof (fp))
            {
              png_id = read_apng_chunk (fp, &chunk);
              if (! png_id)
                {
                  fclose (fp);
                  g_free (apng_frame.pixels);
                  g_free (prior_pixels);

                  g_set_error (error, G_FILE_ERROR,
                               g_file_error_from_errno (errno),
                               _("Could not read APNG frames. "
                                 "They will be discarded."));

                  return FALSE;
                }

              if (png_id == id_acTL)
                {
                  GimpParasite *parasite;
                  gchar        *str;

                  is_animated = TRUE;
                  frame_no    = png_get_uint_32 (chunk.data + 8);
                  playback_no = png_get_uint_32 (chunk.data + 12);

                  str = g_strdup_printf ("%d %d", frame_no, playback_no);
                  parasite = gimp_parasite_new ("apng-image-data",
                                                GIMP_PARASITE_PERSISTENT,
                                                strlen (str) + 1, (gpointer) str);
                  g_free (str);
                  gimp_image_attach_parasite (image, parasite);
                  gimp_parasite_free (parasite);

                }
              else if (png_id == id_fcTL && (! idat_read || is_animated))
                {
                  /* Check if we've gotten past the first frame (standard PNG),
                   * and are reading in the remaining APNG frames */
                  if (idat_read)
                    {
                      /* Skipping the first frame because we've already loaded
                       * it with load_png () */
                      if (end_apng_processing (pp, info) && sequence_num > frame_start)
                        {
                          create_apng_layer (image, &apng_frame, prior_pixels,
                                             image_type, trns, is_indexed_alpha,
                                             dispose_op, blend_op);
                          memcpy (prior_pixels, apng_frame.pixels, frame_data_size);
                        }
                    }
                  else
                    {
                      frame_start++;
                    }

                  apng_frame.width     = png_get_uint_32 (chunk.data + 12);
                  apng_frame.height    = png_get_uint_32 (chunk.data + 16);
                  apng_frame.offset_x  = png_get_uint_32 (chunk.data + 20);
                  apng_frame.offset_y  = png_get_uint_32 (chunk.data + 24);
                  apng_frame.delay_num = png_get_uint_16 (chunk.data + 28);
                  apng_frame.delay_den = png_get_uint_16 (chunk.data + 30);

                  dispose_op = chunk.data[32];
                  blend_op   = chunk.data[33];

                  /* Trick libpng into thinking we're loading the next frame
                   * at the start of a new PNG */
                  if (idat_read)
                    {
                      memcpy (ihdr_chunk.data + 8, chunk.data + 12, 8);

                      if (! restart_apng_processing (&pp, &info, &apng_frame,
                                                     idat_read, &ihdr_chunk,
                                                     other_chunks))
                        break;
                    }
                  sequence_num++;
                }
              else if (png_id == id_fdAT && is_animated)
                {
                  /* Remove sequence number and rename chunk so that
                   * libpng thinks this is a regular IDAT chunk */
                  png_save_uint_32 (chunk.data + 4, chunk.size - 16);
                  memcpy (chunk.data + 8, "IDAT", 4);

                  if (! process_apng_data (pp, info, chunk.data + 4,
                                           chunk.size - 4))
                    break;
                }
              else if (png_id == id_IDAT)
                {
                  idat_read = TRUE;

                  if (! is_animated)
                    {
                      fclose (fp);
                      g_free (apng_frame.pixels);
                      g_free (prior_pixels);

                      g_set_error (error, G_FILE_ERROR,
                                   g_file_error_from_errno (errno),
                                   _("Invalid APNG: Image data appeared "
                                     "before actTL chunk."));

                      return FALSE;
                    }

                  if (! process_apng_data (pp, info, chunk.data, chunk.size))
                    break;
                }
              else if (png_id == id_tRNS)
                {
                  for (gint i = 0; i < chunk.size; i++)
                    trns[i] = chunk.data[i + 8];
                }
              else if (png_id == id_IEND)
                {
                  if (idat_read && end_apng_processing (pp, info))
                    {
                      create_apng_layer (image, &apng_frame, prior_pixels,
                                         image_type, trns, is_indexed_alpha,
                                         dispose_op, blend_op);
                      memcpy (prior_pixels, apng_frame.pixels, frame_data_size);
                    }

                  break;
                }
              else if (! idat_read)
                {
                  APNGChunk *other_chunk = g_malloc (sizeof (APNGChunk));

                  other_chunk->data = g_malloc0 (chunk.size);

                  memcpy (other_chunk->data, chunk.data, chunk.size);
                  other_chunk->size = chunk.size;

                  if (! process_apng_data (pp, info, chunk.data, chunk.size))
                    break;

                  other_chunks = g_list_append (other_chunks, other_chunk);
                }

              g_free (chunk.data);
            }
        }

      /* Clear out lingering chunks like palette */
      if (other_chunks)
        {
          GList *iter;

          for (iter = other_chunks; iter; iter = iter->next)
            {
              APNGChunk *chunk = iter->data;

              g_free (chunk->data);
            }
          g_list_free (other_chunks);
        }
      if (prior_pixels)
        g_free (prior_pixels);
    }

  fclose (fp);

  if (report_progress)
    gimp_progress_update (1.0);

  return TRUE;
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

  gimp_ui_init (PLUG_IN_BINARY);

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
 * 'export_image ()' - Export the specified image to a PNG file.
 */

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

static PngGlobals pngg;

static gboolean
export_image (GFile        *file,
              GimpImage    *image,
              GimpDrawable *drawable,
              GimpImage    *orig_image,
              GObject      *config,
              gint         *bits_per_sample,
              gboolean      report_progress,
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
  gchar           **parasites;        /* Safe-to-copy chunks */
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
  png_time          mod_time;         /* Modification time (ie NOW) */
  time_t            cutime;           /* Time since epoch */
  struct tm        *gmt;              /* GMT broken down */
  gint              color_type;       /* PNG color type */
  gint              bit_depth;        /* Default to bit depth 16 */

  guchar            remap[256];       /* Re-mapping for the palette */

  png_textp         text = NULL;

  gboolean        save_interlaced;
  gboolean        save_bkgd;
  gboolean        save_offs;
  gboolean        save_phys;
  gboolean        save_time;
  gboolean        save_comment;
  gchar          *comment;
  gboolean        save_transp_pixels;
  gboolean        optimize_palette;
  gint            compression_level;
  PngExportFormat export_format;
  gboolean        save_profile;

#if !defined(PNG_iCCP_SUPPORTED)
  g_object_set (config,
                "include-color-profile", FALSE,
                NULL);
#endif

  g_object_get (config,
                "interlaced",            &save_interlaced,
                "bkgd",                  &save_bkgd,
                "offs",                  &save_offs,
                "phys",                  &save_phys,
                "time",                  &save_time,
                "include-comment",       &save_comment,
                "gimp-comment",          &comment,
                "save-transparent",      &save_transp_pixels,
                "optimize-palette",      &optimize_palette,
                "compression",           &compression_level,
                "include-color-profile", &save_profile,
                NULL);

  export_format = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config), "format");

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
  if (save_profile)
    {
      profile = gimp_image_get_color_profile (orig_image);

      if (profile                             ||
          export_format == PNG_FORMAT_AUTO    ||
          export_format == PNG_FORMAT_RGB16   ||
          export_format == PNG_FORMAT_RGBA16  ||
          export_format == PNG_FORMAT_GRAY16  ||
          export_format == PNG_FORMAT_GRAYA16 ||
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
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error creating PNG write struct while exporting '%s'."),
                   gimp_file_get_utf8_name (file));
      return FALSE;
    }

  info = png_create_info_struct (pp);
  if (! info)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error while exporting '%s'. Could not create PNG header info structure."),
                   gimp_file_get_utf8_name (file));
      return FALSE;
    }

  if (setjmp (png_jmpbuf (pp)))
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error while exporting '%s'. Could not export image."),
                   gimp_file_get_utf8_name (file));
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

  if (report_progress)
    gimp_progress_init_printf (_("Exporting '%s'"),
                               gimp_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file), "wb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
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

  if (export_format == PNG_FORMAT_AUTO)
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
        pngg.palette = (png_colorp) gimp_palette_get_colormap (gimp_image_get_palette (image),
                                                               babl_format ("R'G'B' u8"),
                                                               &pngg.num_palette, NULL);
        if (optimize_palette)
          bit_depth = get_bit_depth_for_palette (pngg.num_palette);
        break;

      case GIMP_INDEXEDA_IMAGE:
        color_type = PNG_COLOR_TYPE_PALETTE;
        file_format = gimp_drawable_get_format (drawable);
        /* fix up transparency */
        if (optimize_palette)
          bit_depth = respin_cmap (pp, info, remap, image, drawable);
        else
          respin_cmap (pp, info, remap, image, drawable);
        break;

      default:
        g_set_error (error, G_FILE_ERROR, 0, "Image type can't be exported as PNG");
        return FALSE;
      }
    }
  else
    {
      switch (export_format)
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
                save_interlaced ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_BASE,
                PNG_FILTER_TYPE_BASE);

  if (pngg.has_trns)
    png_set_tRNS (pp, info, pngg.trans, pngg.num_trans, NULL);

  if (pngg.has_plte)
    png_set_PLTE (pp, info, pngg.palette, pngg.num_palette);

  /* Set the compression level */

  png_set_compression_level (pp, compression_level);

  /* All this stuff is optional extras, if the user is aiming for smallest
     possible file size she can turn them all off */

  if (save_bkgd)
    {
      GeglColor    *color;
      png_color_16  background;       /* Background color */

      background.index = 0;
      color = gimp_context_get_background ();
      if (bit_depth < 16)
        {
          /* Per PNG spec 1.2: "(If the image bit depth is less than 16, the
           * least significant bits are used and the others are 0.)"
           * And png_set_bKGD() doesn't handle the conversion for us, if we try
           * to set a u16 background, it outputs the following warning:
           * > libpng warning: Ignoring attempt to write 16-bit bKGD chunk when bit_depth is 8
           */
          guint8 rgb[3];
          guint8 gray;

          gegl_color_get_pixel (color, babl_format_with_space ("R'G'B' u8", space), rgb);
          gegl_color_get_pixel (color, babl_format_with_space ("Y' u8", space), &gray);

          background.red   = rgb[0];
          background.green = rgb[1];
          background.blue  = rgb[2];
          background.gray  = gray;
        }
      else
        {
          guint16 rgb[3];
          guint16 gray;

          gegl_color_get_pixel (color, babl_format_with_space ("R'G'B' u16", space), rgb);
          gegl_color_get_pixel (color, babl_format_with_space ("Y' u16", space), &gray);

          background.red   = rgb[0];
          background.green = rgb[1];
          background.blue  = rgb[2];
          background.gray  = gray;
        }

      png_set_bKGD (pp, info, &background);
      g_object_unref (color);
    }

  if (save_offs)
    {
      gimp_drawable_get_offsets (drawable, &offx, &offy);
      if (offx != 0 || offy != 0)
        png_set_oFFs (pp, info, offx, offy, PNG_OFFSET_PIXEL);
    }

  if (save_phys)
    {
      gimp_image_get_resolution (orig_image, &xres, &yres);
      png_set_pHYs (pp, info, RINT (xres / 0.0254), RINT (yres / 0.0254),
                    PNG_RESOLUTION_METER);
    }

  if (save_time)
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
  if (save_profile)
    {
      GimpParasite *parasite;
      gchar        *profile_name = NULL;
      const guint8 *icc_data;
      gsize         icc_length;

      icc_data = gimp_color_profile_get_icc_profile (profile, &icc_length);

      parasite = gimp_image_get_parasite (orig_image,
                                          "icc-profile-name");
      if (parasite)
        {
          gchar   *parasite_data;
          guint32  parasite_size;

          parasite_data = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
          profile_name = g_convert (parasite_data, parasite_size,
                                    "UTF-8", "ISO-8859-1", NULL, NULL, NULL);
        }

      png_set_iCCP (pp,
                    info,
                    profile_name ? profile_name : "ICC profile",
                    0,
                    icc_data,
                    icc_length);

      g_free (profile_name);

      g_object_unref (profile);
    }
  else
#endif
    {
      /* Be more specific by writing into the file that the image is in
       * sRGB color space.
       */
      GimpColorConfig *config = gimp_get_color_configuration ();
      int              srgb_intent;

      switch (gimp_color_config_get_display_intent (config))
        {
        case GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL:
          srgb_intent = PNG_sRGB_INTENT_PERCEPTUAL;
          break;
        case GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC:
          srgb_intent = PNG_sRGB_INTENT_RELATIVE;
          break;
        case GIMP_COLOR_RENDERING_INTENT_SATURATION:
          srgb_intent = PNG_sRGB_INTENT_SATURATION;
          break;
        case GIMP_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC:
          srgb_intent = PNG_sRGB_INTENT_ABSOLUTE;
          break;
        }
      png_set_sRGB_gAMA_and_cHRM (pp, info, srgb_intent);

      g_object_unref (config);
    }

#ifdef PNG_zTXt_SUPPORTED
/* Small texts are not worth compressing and will be even bigger if compressed.
   Empirical length limit of a text being worth compressing. */
#define COMPRESSION_WORTHY_LENGTH 200
#endif

  if (save_comment && comment && strlen (comment))
    {
      gsize text_length = 0;

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
           * Save the comment as iTXt (UTF-8).
           */
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
        /* The comment is ISO-8859-1 compatible, so we use tEXt even
         * if there is iTXt support for compatibility to more png
         * reading programs.
         */
#endif /* PNG_iTXt_SUPPORTED */
        {
#ifndef PNG_iTXt_SUPPORTED
          /* No iTXt support, so we are forced to use tEXt
           * (ISO-8859-1).  A broken comment is better than no comment
           * at all, so the conversion does not fail on unknown
           * character.  They are simply ignored.
           */
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
    }

  g_free (comment);

#ifdef PNG_zTXt_SUPPORTED
#undef COMPRESSION_WORTHY_LENGTH
#endif

  if (text)
    png_set_text (pp, info, text, 1);

  png_write_info (pp, info);
  if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    png_set_swap (pp);

  /* Write any safe-to-copy chunks saved from import */
  parasites = gimp_image_get_parasite_list (image);

  if (parasites)
    {
      gint count;

      count = g_strv_length (parasites);
      for (gint i = 0; i < count; i++)
        {
          if (strncmp (parasites[i], "png", 3) == 0)
            {
              GimpParasite *parasite;

              parasite = gimp_image_get_parasite (image, parasites[i]);

              if (parasite)
                {
                  gchar  buf[1024];
                  gchar *chunk_name;

                  g_strlcpy (buf, parasites[i], sizeof (buf));
                  chunk_name = strchr (buf, '/');
                  chunk_name++;

                  if (chunk_name)
                    {
                      png_byte      name[4];
                      const guint8 *data;
                      guint32       len;

                      for (gint j = 0; j < 4; j++)
                        name[j] = chunk_name[j];

                      data = (const guint8 *) gimp_parasite_get_data (parasite, &len);

                      png_write_chunk (pp, name, data, len);
                    }
                  gimp_parasite_free (parasite);
                }
            }
        }
    }
  g_strfreev (parasites);

  /*
   * Turn on interlace handling...
   */

  if (save_interlaced)
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
          if (bpp == 4 && ! save_transp_pixels)
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

          if (bpp == 8 && ! save_transp_pixels)
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
          if (pngg.has_trns)
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

          if (report_progress)
            gimp_progress_update (((double) pass + (double) end /
                                   (double) height) /
                                  (double) num_passes);
        }
    }

  if (report_progress)
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

  *bits_per_sample = bit_depth;

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

/* Try to find a color in the palette which isn't actually used in the
 * image, so that we can use it as the transparency index. Taken from
 * gif.c
 */
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
   * there.
   */
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

static void
add_alpha_to_indexed (GimpLayer *layer,
                      guchar    *alpha)
{
  GeglBufferIterator *iter;
  GeglBuffer         *buffer;
  const Babl         *format;
  gint                n_components;

  gimp_layer_add_alpha (layer);
  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
  format = gegl_buffer_get_format (buffer);

  iter = gegl_buffer_iterator_new (buffer, NULL, 0, format,
                                   GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);
  n_components = babl_format_get_n_components (format);
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

static int
respin_cmap (png_structp   pp,
             png_infop     info,
             guchar       *remap,
             GimpImage    *image,
             GimpDrawable *drawable)
{
  static guchar trans[] = { 0 };
  GeglBuffer   *buffer;

  gint          colors;
  guchar       *before;

  before = gimp_palette_get_colormap (gimp_image_get_palette (image), babl_format ("R'G'B' u8"), &colors, NULL);
  buffer = gimp_drawable_get_buffer (drawable);

  /* Make sure there is something in the colormap.
   */
  if (colors == 0)
    {
      before = g_newa (guchar, 3);
      memset (before, 0, sizeof (guchar) * 3);

      colors = 1;
    }

  /* Try to find an entry which isn't actually used in the image, for
   * a transparency index.
   */

  if (ia_has_transparent_pixels (buffer))
    {
      gint transparent = find_unused_ia_color (buffer, &colors);

      if (transparent != -1)        /* we have a winner for a transparent
                                     * index - do like gif2png and swap
                                     * index 0 and index transparent
                                     */
        {
          static png_color palette[256];
          gint      i;

          /* Set tRNS chunk values for writing later. */
          pngg.has_trns = TRUE;
          pngg.trans = trans;
          pngg.num_trans = 1;

          /* Transform all pixels with a value = transparent to 0 and
           * vice versa to compensate for re-ordering in palette due
           * to png_set_tRNS()
           */

          remap[0] = transparent;
          for (i = 1; i <= transparent; i++)
            remap[i] = i - 1;

          /* Copy from index 0 to index transparent - 1 to index 1 to
           * transparent of after, then from transparent+1 to colors-1
           * unchanged, and finally from index transparent to index 0.
           */

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
           * transparency & just use the full palette
           */
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

static gint
read_unknown_chunk (png_structp        png_ptr,
                    png_unknown_chunkp chunk)
{
  /* Chunks with a lowercase letter in the 4th byte
   * are safe to copy */
  if (g_ascii_islower (chunk->name[3]) ||
      strcmp ((gchar *) chunk->name, "fcTL") == 0)
    {
      GimpParasite *parasite;
      gchar         pname[255];

      g_snprintf (pname, sizeof (pname), "png/%s", chunk->name);

      if ((parasite = gimp_parasite_new (pname,
                                         GIMP_PARASITE_PERSISTENT,
                                         chunk->size, chunk->data)))
        safe_to_copy_chunks = g_slist_prepend (safe_to_copy_chunks, parasite);
    }

  return 0;
}

/* APNG specific functions */

static gboolean
restart_apng_processing (png_structp *png_ptr,
                         png_infop   *info_ptr,
                         void        *frame_ptr,
                         gboolean     idat_read,
                         APNGChunk   *ihdr_chunk,
                         GList       *apng_chunks)
{
  guchar  header[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
  GList  *iter;

  *png_ptr  = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  *info_ptr = png_create_info_struct (*png_ptr);
  if (! *png_ptr || ! *info_ptr)
    return FALSE;

  if (setjmp (png_jmpbuf (*png_ptr)))
    {
      png_destroy_read_struct (png_ptr, info_ptr, 0);
      return FALSE;
    }

#ifdef PNG_BENIGN_ERRORS_SUPPORTED
  png_set_benign_errors (*png_ptr, TRUE);
  png_set_option (*png_ptr, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);
#endif

  png_set_crc_action (*png_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);
  png_set_progressive_read_fn (*png_ptr, frame_ptr, read_apng_header,
                               read_apng_row, NULL);

  png_process_data (*png_ptr, *info_ptr, header, 8);
  png_process_data (*png_ptr, *info_ptr, ihdr_chunk->data, ihdr_chunk->size);

  /* Load any other required chunks (like palette) before next image chunk */
  if (idat_read)
    for (iter = apng_chunks; iter; iter = iter->next)
      {
        APNGChunk *apng_chunk = iter->data;

        png_process_data (*png_ptr, *info_ptr, apng_chunk->data,
                          apng_chunk->size);
      }

  return TRUE;
}

static gboolean
process_apng_data (png_structp  png_ptr,
                   png_infop    info_ptr,
                   guchar      *data,
                   guint        size)
{
  if (! png_ptr || ! info_ptr)
    return FALSE;

  if (setjmp (png_jmpbuf (png_ptr)))
    {
      png_destroy_read_struct (&png_ptr, &info_ptr, 0);
      return FALSE;
    }

  png_process_data (png_ptr, info_ptr, data, size);
  return TRUE;
}

static gboolean
end_apng_processing (png_structp png_ptr,
                     png_infop   info_ptr)
{
  unsigned char footer[12] = {0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96, 130};

  if (! png_ptr || ! info_ptr)
    return FALSE;

  if (setjmp (png_jmpbuf (png_ptr)))
    {
      png_destroy_read_struct(&png_ptr, &info_ptr, 0);
      return FALSE;
    }

  png_process_data (png_ptr, info_ptr, footer, 12);
  png_destroy_read_struct (&png_ptr, &info_ptr, 0);

  return TRUE;
}

static void
read_apng_header (png_structp png_ptr,
                  png_infop   info_ptr)
{
  (void) png_set_interlace_handling (png_ptr);

  png_read_update_info (png_ptr, info_ptr);
}

static void
read_apng_row (png_structp png_ptr,
               png_bytep   new_row,
               png_uint_32 row_num,
               gint        pass)
{
  APNGFrame *apng_frame = (APNGFrame *) png_get_progressive_ptr (png_ptr);
  gsize      row_offset;

  row_offset = row_num * apng_frame->bpp * apng_frame->width;
  png_progressive_combine_row (png_ptr,
                               apng_frame->pixels + row_offset,
                               new_row);
}

static inline guint
read_apng_chunk (FILE      *f,
                 APNGChunk *chunk)
{
  guchar len[4];

  chunk->size = 0;
  chunk->data = NULL;

  if (fread (&len, 4, 1, f) == 1)
    {
      chunk->size = png_get_uint_32 (len);

      if (chunk->size > PNG_USER_CHUNK_MALLOC_MAX)
        return 0;

      chunk->size += 12;

      chunk->data = g_malloc (chunk->size);
      memcpy (chunk->data, len, 4);

      if (fread (chunk->data + 4, chunk->size - 4, 1, f) == 1)
        return *(guint *) (chunk->data + 4);
    }

  return 0;
}

static void
create_apng_layer (GimpImage     *image,
                   APNGFrame     *apng_frame,
                   guchar        *prior_pixels,
                   GimpImageType  image_type,
                   guchar        *alpha,
                   gboolean       is_indexed_alpha,
                   gint           dispose_op,
                   gint           blend_op)
{
  GimpLayer    *layer;
  GeglBuffer   *buffer;
  GimpParasite *parasite;
  gchar        *str;

  layer = gimp_layer_new (image, NULL,
                          apng_frame->width, apng_frame->height,
                          image_type, 100,
                          gimp_image_get_default_new_layer_mode (image));
  gimp_layer_set_offsets (layer, apng_frame->offset_x, apng_frame->offset_y);
  gimp_image_insert_layer (image, layer, NULL, 0);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  gegl_buffer_set (buffer,
                   GEGL_RECTANGLE (0, 0, apng_frame->width, apng_frame->height),
                   0, NULL, apng_frame->pixels,
                   GEGL_AUTO_ROWSTRIDE);
  g_object_unref (buffer);

  if (is_indexed_alpha)
    add_alpha_to_indexed (layer, alpha);

  str = g_strdup_printf ("%d %d %d %d",
                         apng_frame->delay_num,
                         apng_frame->delay_den,
                         dispose_op,
                         blend_op);
  parasite = gimp_parasite_new ("apng-frame-data",
                                GIMP_PARASITE_PERSISTENT,
                                strlen (str) + 1,
                                (gpointer) str);
  g_free (str);
  gimp_item_attach_parasite (GIMP_ITEM (layer), parasite);
  gimp_parasite_free (parasite);
}

static gboolean
export_dialog (GimpImage     *image,
               GimpProcedure *procedure,
               GObject       *config,
               gboolean       alpha)
{
  GtkWidget *dialog;
  gboolean   run;
  gboolean   indexed;

  indexed = (gimp_image_get_base_type (image) == GIMP_INDEXED);

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                    "compression", GIMP_TYPE_SPIN_SCALE);

  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "save-transparent",
                                       alpha, NULL, NULL, FALSE);

  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "optimize-palette",
                                       indexed, NULL, NULL, FALSE);

  gimp_export_procedure_dialog_add_metadata (GIMP_EXPORT_PROCEDURE_DIALOG (dialog), "bkgd");
  gimp_export_procedure_dialog_add_metadata (GIMP_EXPORT_PROCEDURE_DIALOG (dialog), "offs");
  gimp_export_procedure_dialog_add_metadata (GIMP_EXPORT_PROCEDURE_DIALOG (dialog), "phys");
  gimp_export_procedure_dialog_add_metadata (GIMP_EXPORT_PROCEDURE_DIALOG (dialog), "time");
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "format", "compression",
                              "interlaced", "save-transparent",
                              "optimize-palette",
                              NULL);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
