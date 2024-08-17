/* Raw data image loader (and exporter) plugin 3.4
 *
 * by tim copperfield [timecop@japan.co.jp]
 * http://www.ne.jp/asahi/linux/timecop
 *
 * Updated for Gimp 2.1 by pg@futureware.at and mitch@gimp.org
 *
 * This plugin is not based on any other plugin.
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

#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-raw-load"
#define LOAD_HGT_PROC  "file-hgt-load"
#define EXPORT_PROC    "file-raw-export"
#define PLUG_IN_BINARY "file-raw-data"
#define PLUG_IN_ROLE   "gimp-file-raw-data"
#define PREVIEW_SIZE   350


#define GIMP_PLUGIN_HGT_LOAD_ERROR gimp_plugin_hgt_load_error_quark ()

typedef enum
{
  GIMP_PLUGIN_HGT_LOAD_ARGUMENT_ERROR
} GimpPluginHGTError;

static GQuark
gimp_plugin_hgt_load_error_quark (void)
{
  return g_quark_from_static_string ("gimp-plugin-hgt-load-error-quark");
}


typedef enum
{
  HGT_SRTM_AUTO_DETECT,
  HGT_SRTM_1,
  HGT_SRTM_3,
} HgtSampleSpacing;

typedef enum
{
  RAW_PLANAR_CONTIGUOUS,   /* Contiguous/chunky format RGBRGB */
  RAW_PLANAR_SEPARATE,     /* Planar format RRGGBB            */
} RawPlanarConfiguration;

typedef enum
{
  RAW_ENCODING_UNSIGNED, /* Unsigned integer */
  RAW_ENCODING_SIGNED,   /* Signed integer   */
  RAW_ENCODING_FLOAT,    /* Floating point   */
} RawEncoding;

typedef enum
{
  RAW_LITTLE_ENDIAN, /* Little Endian */
  RAW_BIG_ENDIAN,    /* Big Endian    */
} RawEndianness;

typedef enum
{
  /* RGB Images */
  RAW_RGB_8BPP,
  RAW_RGB_16BPP,
  RAW_RGB_32BPP,

  /* RGB Image with an Alpha channel */
  RAW_RGBA_8BPP,
  RAW_RGBA_16BPP,
  RAW_RGBA_32BPP,

  RAW_RGB565,       /* RGB Image 16bit, 5,6,5 bits per channel */
  RAW_BGR565,       /* RGB Image 16bit, 5,6,5 bits per channel, red and blue swapped */

  /* Grayscale Images */
  RAW_GRAY_1BPP,
  RAW_GRAY_2BPP,
  RAW_GRAY_4BPP,

  RAW_GRAY_8BPP,
  RAW_GRAY_16BPP,
  RAW_GRAY_32BPP,

  RAW_GRAYA_8BPP,
  RAW_GRAYA_16BPP,
  RAW_GRAYA_32BPP,

  RAW_INDEXED,      /* Indexed image */
  RAW_INDEXEDA,     /* Indexed image with an Alpha channel */
} RawType;

typedef enum
{
  RAW_PALETTE_RGB,  /* standard RGB */
  RAW_PALETTE_BGR   /* Windows BGRX */
} RawPaletteType;

typedef struct
{
  FILE         *fp;        /* pointer to the already open file */
  GeglBuffer   *buffer;    /* gimp drawable buffer             */
  GimpImage    *image;     /* gimp image                       */
  guchar        cmap[768]; /* color map for indexed images     */
} RawGimpData;


typedef struct _Raw      Raw;
typedef struct _RawClass RawClass;

struct _Raw
{
  GimpPlugIn      parent_instance;
};

struct _RawClass
{
  GimpPlugInClass parent_class;
};


#define RAW_TYPE  (raw_get_type ())
#define RAW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RAW_TYPE, Raw))

GType                   raw_get_type         (void) G_GNUC_CONST;

static GList          * raw_query_procedures (GimpPlugIn               *plug_in);
static GimpProcedure  * raw_create_procedure (GimpPlugIn               *plug_in,
                                              const gchar              *name);

static GimpValueArray * raw_load             (GimpProcedure            *procedure,
                                              GimpRunMode               run_mode,
                                              GFile                    *file,
                                              GimpMetadata             *metadata,
                                              GimpMetadataLoadFlags    *flags,
                                              GimpProcedureConfig      *config,
                                              gpointer                  run_data);
static GimpValueArray * raw_export           (GimpProcedure            *procedure,
                                              GimpRunMode               run_mode,
                                              GimpImage                *image,
                                              GFile                    *file,
                                              GimpExportOptions        *options,
                                              GimpMetadata             *metadata,
                                              GimpProcedureConfig      *config,
                                              gpointer                  run_data);

/* prototypes for the new load functions */
static gboolean         raw_load_standard    (RawGimpData              *data,
                                              gint                      width,
                                              gint                      height,
                                              gint                      bpp,
                                              gint                      offset,
                                              RawType                   type,
                                              gboolean                  is_big_endian,
                                              gboolean                  is_signed,
                                              gboolean                  is_float);
static gboolean         raw_load_planar      (RawGimpData              *data,
                                              gint                      width,
                                              gint                      height,
                                              gint                      bpp,
                                              gint                      offset,
                                              RawType                   type,
                                              gboolean                  is_big_endian,
                                              gboolean                  is_signed,
                                              gboolean                  is_float);
static gboolean         raw_load_gray        (RawGimpData              *data,
                                              gint                      width,
                                              gint                      height,
                                              gint                      offset,
                                              gint                      bpp,
                                              gint                      bitspp);
static gboolean         raw_load_rgb565      (RawGimpData              *data,
                                              gint                      width,
                                              gint                      height,
                                              gint                      offset,
                                              RawType                   type,
                                              RawEndianness             endianness);
static gboolean         raw_load_palette     (RawGimpData              *data,
                                              gint                      palette_offset,
                                              RawPaletteType            palette_type,
                                              GFile                    *palette_file);

/* support functions */
static goffset          get_file_info        (GFile                    *file);
static void             raw_read_row         (FILE                     *fp,
                                              guchar                   *buf,
                                              gint32                    offset,
                                              gint32                    size);
static int              mmap_read            (gint                      fd,
                                              gpointer                  buf,
                                              gint32                    len,
                                              gint32                    pos,
                                              gint                      rowstride);
static void             rgb_565_to_888       (guint16                  *in,
                                              guchar                   *out,
                                              gint32                    num_pixels,
                                              RawType                   type,
                                              RawEndianness             endianness);

static GimpImage      * load_image           (GFile                    *file,
                                              GimpProcedureConfig      *config,
                                              GError                  **error);
static gboolean         export_image         (GFile                    *file,
                                              GimpImage                *image,
                                              GimpDrawable             *drawable,
                                              GimpProcedureConfig      *config,
                                              GError                  **error);

static void            get_bpp               (GimpProcedureConfig      *config,
                                              gint                     *bpp,
                                              gint                     *bitspp);
static gboolean        detect_sample_spacing (GimpProcedureConfig      *config,
                                              GFile                    *file,
                                              GError                  **error);
static void           get_load_config_values (GimpProcedureConfig      *config,
                                              gint32                   *file_offset,
                                              gint32                   *image_width,
                                              gint32                   *image_height,
                                              RawType                  *image_type,
                                              RawEncoding              *encoding,
                                              RawEndianness            *endianness,
                                              RawPlanarConfiguration   *planar_configuration,
                                              gint32                   *palette_offset,
                                              RawPaletteType           *palette_type,
                                              GFile                   **palette_file);

/* gui functions */
static void             halfp2singles        (uint32_t                 *xp,
                                              const uint16_t           *hp,
                                              int                       numel);

static void             preview_update       (GimpPreviewArea          *preview,
                                              gboolean                  preview_cmap_update);
static void             preview_update_size  (GimpPreviewArea          *preview);
static void             load_config_notify   (GimpProcedureConfig      *config,
                                              GParamSpec               *pspec,
                                              GimpPreviewArea          *preview);
static void             preview_allocate     (GimpPreviewArea          *preview,
                                              GtkAllocation            *allocation,
                                              gpointer                  user_data);
static gboolean         load_dialog          (GFile                    *file,
                                              GimpProcedure            *procedure,
                                              GObject                  *config,
                                              gboolean                  is_hgt);
static gboolean         save_dialog          (GimpImage                *image,
                                              GimpProcedure            *procedure,
                                              gboolean                  has_alpha,
                                              GObject                  *config);

G_DEFINE_TYPE (Raw, raw, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (RAW_TYPE)
DEFINE_STD_SET_I18N


static gint       preview_fd          = -1;
static guchar     preview_cmap[1024];


static void
raw_class_init (RawClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = raw_query_procedures;
  plug_in_class->create_procedure = raw_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
raw_init (Raw *raw)
{
}

static GList *
raw_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (LOAD_HGT_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static GimpProcedure *
raw_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           raw_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("Raw image data"));

      gimp_procedure_set_documentation (procedure,
                                        _("Load raw images, specifying image "
                                          "information"),
                                        _("Load raw images, specifying image "
                                          "information"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "timecop, pg@futureware.at",
                                      "timecop, pg@futureware.at",
                                      "Aug 2004");

      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "data");

      /* Properties for image data. */

      gimp_procedure_add_int_argument (procedure, "width",
                                       _("_Width"),
                                       _("Image width in number of pixels"),
                                       1, GIMP_MAX_IMAGE_SIZE, PREVIEW_SIZE,
                                       G_PARAM_READWRITE);
      gimp_procedure_add_int_argument (procedure, "height",
                                       _("_Height"),
                                       _("Image height in number of pixels"),
                                       1, GIMP_MAX_IMAGE_SIZE, PREVIEW_SIZE,
                                       G_PARAM_READWRITE);
      gimp_procedure_add_int_argument (procedure, "offset",
                                       _("O_ffset"),
                                       _("Offset to beginning of image in raw data"),
                                       0, GIMP_MAX_IMAGE_SIZE, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "pixel-format",
                                          _("Pi_xel format"),
                                          _("The layout of pixel data, such as components and their order"),
                                          gimp_choice_new_with_values ("rgb-8bpp",              RAW_RGB_8BPP,    _("RGB 8-bit"),                NULL,
                                                                       "rgb-16bpp",             RAW_RGB_16BPP,   _("RGB 16-bit"),               NULL,
                                                                       "rgb-32bpp",             RAW_RGB_32BPP,   _("RGB 32-bit"),               NULL,
                                                                       "rgba-8bpp",             RAW_RGBA_8BPP,   _("RGBA 8-bit"),               NULL,
                                                                       "rgba-16bpp",            RAW_RGBA_16BPP,  _("RGBA 16-bit"),              NULL,
                                                                       "rgba-32bpp",            RAW_RGBA_32BPP,  _("RGBA 32-bit"),              NULL,
                                                                       "rgb565",                RAW_RGB565,      _("RGB565"),                   NULL,
                                                                       "bgr565",                RAW_BGR565,      _("BGR565"),                   NULL,
                                                                       "grayscale-1bpp",        RAW_GRAY_1BPP,   _("B&W 1 bit"),                NULL,
                                                                       "grayscale-2bpp",        RAW_GRAY_2BPP,   _("Grayscale 2-bit"),          NULL,
                                                                       "grayscale-4bpp",        RAW_GRAY_4BPP,   _("Grayscale 4-bit"),          NULL,
                                                                       "grayscale-8bpp",        RAW_GRAY_8BPP,   _("Grayscale 8-bit"),          NULL,
                                                                       "grayscale-16bpp",       RAW_GRAY_16BPP,  _("Grayscale 16-bit"),         NULL,
                                                                       "grayscale-32bpp",       RAW_GRAY_32BPP,  _("Grayscale 32-bit"),         NULL,
                                                                       "grayscale-alpha-8bpp",  RAW_GRAYA_8BPP,  _("Grayscale-Alpha 8-bit"),    NULL,
                                                                       "grayscale-alpha-16bpp", RAW_GRAYA_16BPP, _("Grayscale-Alpha 16-bit"),   NULL,
                                                                       "grayscale-alpha-32bpp", RAW_GRAYA_32BPP, _("Grayscale-Alpha 32-bit"),   NULL,
                                                                       "indexed",               RAW_INDEXED,     _("Indexed"),                  NULL,
                                                                       "indexed-alpha",         RAW_INDEXEDA,    _("Indexed Alpha"),            NULL,
                                                                       NULL),
                                          "rgb-8bpp", G_PARAM_READWRITE);
      gimp_procedure_add_choice_argument (procedure, "data-type",
                                          _("_Data type"),
                                          _("Data type used to represent pixel values"),
                                          gimp_choice_new_with_values ("unsigned", RAW_ENCODING_UNSIGNED,   _("Unsigned Integer"),  NULL,
                                                                       "signed",   RAW_ENCODING_SIGNED,     _("Signed Integer"),    NULL,
                                                                       "float",    RAW_ENCODING_FLOAT,      _("Floating Point"),    NULL,
                                                                       NULL),
                                          "unsigned", G_PARAM_READWRITE);
      gimp_procedure_add_choice_argument (procedure, "endianness",
                                          _("_Endianness"),
                                          _("Order of sequences of bytes"),
                                          gimp_choice_new_with_values ("little-endian", RAW_LITTLE_ENDIAN, _("Little Endian"),    NULL,
                                                                       "big-endian",    RAW_BIG_ENDIAN,    _("Big Endian"),       NULL,
                                                                       NULL),
                                          "little-endian", G_PARAM_READWRITE);
      gimp_procedure_add_choice_argument (procedure, "planar-configuration",
                                          _("Planar confi_guration"),
                                          _("How color pixel data are stored"),
                                          gimp_choice_new_with_values ("contiguous", RAW_PLANAR_CONTIGUOUS, _("Contiguous"),    NULL,
                                                                       "planar",     RAW_PLANAR_SEPARATE,   _("Planar"),        NULL,
                                                                       NULL),
                                          "contiguous", G_PARAM_READWRITE);

      /* Properties for palette data. */

      gimp_procedure_add_int_argument (procedure, "palette-offset",
                                       _("Palette Offse_t"),
                                       _("Offset to beginning of data in the palette file"),
                                       0, GIMP_MAX_IMAGE_SIZE, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "palette-type",
                                          _("Palette's la_yout"),
                                          _("The layout for the palette's color channels"),
                                          gimp_choice_new_with_values ("rgb", RAW_PALETTE_RGB, _("R, G, B (normal)"),        NULL,
                                                                       "bgr", RAW_PALETTE_BGR, _("B, G, R, X (BMP style)"),  NULL,
                                                                       NULL),
                                          "rgb", G_PARAM_READWRITE);
      gimp_procedure_add_file_argument (procedure, "palette-file",
                                        _("_Palette File"),
                                        _("The file containing palette data"),
                                        G_PARAM_READWRITE);
    }
  else if (! strcmp (name, LOAD_HGT_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            raw_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure,
                                     _("Digital Elevation Model data"));

      gimp_procedure_set_documentation (procedure,
                                        _("Load HGT data as images"),
                                        _("Load Digital Elevation Model data "
                                          "in HGT format from the Shuttle Radar "
                                          "Topography Mission as images. Though "
                                          "the output image will be RGB, all "
                                          "colors are grayscale by default and "
                                          "the contrast will be quite low on "
                                          "most earth relief. Therefore you "
                                          "will likely want to remap elevation "
                                          "to colors as a second step, for "
                                          "instance with the \"Gradient Map\" "
                                          "plug-in."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      NULL, NULL,
                                      "2017-12-09");

      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "hgt");

      gimp_procedure_add_choice_argument (procedure, "sample-spacing",
                                          _("Sa_mple spacing"),
                                          _("The sample spacing of the data."),
                                          gimp_choice_new_with_values ("auto-detect", HGT_SRTM_AUTO_DETECT, _("Auto-Detect"),            NULL,
                                                                       "srtm-1",      HGT_SRTM_1,           _("SRTM-1 (1 arc second)"),  NULL,
                                                                       "srtm-3",      HGT_SRTM_3,           _("SRTM-3 (3 arc seconds)"),  NULL,
                                                                       NULL),
                                          "auto-detect", G_PARAM_READWRITE);

      /* Properties for palette data. */

      gimp_procedure_add_int_argument (procedure, "palette-offset",
                                       _("Palette Offse_t"),
                                       _("Offset to beginning of data in the palette file"),
                                       0, GIMP_MAX_IMAGE_SIZE, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "palette-type",
                                          _("Palette's la_yout"),
                                          _("The layout for the palette's color channels"),
                                          gimp_choice_new_with_values ("rgb", RAW_PALETTE_RGB, _("R, G, B (normal)"),        NULL,
                                                                       "bgr", RAW_PALETTE_BGR, _("B, G, R, X (BMP style)"),  NULL,
                                                                       NULL),
                                          "rgb", G_PARAM_READWRITE);
      gimp_procedure_add_file_argument (procedure, "palette-file",
                                        _("_Palette File"),
                                        _("The file containing palette data"),
                                        G_PARAM_READWRITE);
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, raw_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "INDEXED, GRAY, RGB, RGBA");

      gimp_procedure_set_menu_label (procedure, _("Raw image data"));

      gimp_procedure_set_documentation (procedure,
                                        _("Dump images to disk in raw format"),
                                        _("Dump images to disk in raw format"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Björn Kautler, Bjoern@Kautler.net",
                                      "Björn Kautler, Bjoern@Kautler.net",
                                      "April 2014");

      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "data,raw");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB     |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                              GIMP_EXPORT_CAN_HANDLE_ALPHA,
                                              NULL, NULL, NULL);

      gimp_procedure_add_choice_argument (procedure, "planar-configuration",
                                          _("Planar configuration"),
                                          _("How color pixel data are stored"),
                                          gimp_choice_new_with_values ("contiguous", RAW_PLANAR_CONTIGUOUS, _("Contiguous"),    NULL,
                                                                       "planar",     RAW_PLANAR_SEPARATE,   _("Planar"),        NULL,
                                                                       NULL),
                                          "contiguous", G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "palette-type",
                                          _("Palette's la_yout"),
                                          _("The layout for the palette's color channels"),
                                          gimp_choice_new_with_values ("rgb", RAW_PALETTE_RGB, _("R, G, B (normal)"),        NULL,
                                                                       "bgr", RAW_PALETTE_BGR, _("B, G, R, X (BMP style)"),  NULL,
                                                                       NULL),
                                          "rgb", G_PARAM_READWRITE);
    }

  gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                       _("Raw Data"));

  return procedure;
}

static GimpValueArray *
raw_load (GimpProcedure         *procedure,
          GimpRunMode            run_mode,
          GFile                 *file,
          GimpMetadata          *metadata,
          GimpMetadataLoadFlags *flags,
          GimpProcedureConfig   *config,
          gpointer               run_data)
{
  GimpValueArray    *return_vals;
  gboolean           is_hgt;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpImage         *image  = NULL;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  is_hgt = (! strcmp (gimp_procedure_get_name (procedure), LOAD_HGT_PROC));

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      preview_fd = g_open (g_file_peek_path (file), O_RDONLY, 0);
      if (preview_fd < 0)
        {
          g_set_error (&error,
                       G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Could not open '%s' for reading: %s"),
                       gimp_file_get_utf8_name (file),
                       g_strerror (errno));

          status = GIMP_PDB_EXECUTION_ERROR;
        }
      else
        {
          /* As a special exception, if the file looks like an HGT format
           * from extension, yet it doesn't have the right size, we will
           * degrade a bit the experience by adding sample spacing choice.
           */
          gboolean show_dialog = (! is_hgt || ! detect_sample_spacing (config, file, &error));

          if (error != NULL)
            status = GIMP_PDB_EXECUTION_ERROR;
          else if (show_dialog && ! load_dialog (file, procedure, G_OBJECT (config), is_hgt))
            status = GIMP_PDB_CANCEL;

          close (preview_fd);
        }
    }
  else if (is_hgt) /* HGT file in non-interactive mode. */
    {
      HgtSampleSpacing sample_spacing = HGT_SRTM_AUTO_DETECT;

      sample_spacing = gimp_procedure_config_get_choice_id (config, "sample-spacing");

      if (sample_spacing < HGT_SRTM_AUTO_DETECT ||
          sample_spacing > HGT_SRTM_3)
        {
          g_set_error (&error,
                       GIMP_PLUGIN_HGT_LOAD_ERROR,
                       GIMP_PLUGIN_HGT_LOAD_ARGUMENT_ERROR,
                       _("%d is not a valid sample spacing. "
                         "Valid values are: 0 (auto-detect), 1 and 3."),
                       sample_spacing);

          status = GIMP_PDB_CALLING_ERROR;
        }
      else if (sample_spacing == HGT_SRTM_AUTO_DETECT &&
               ! detect_sample_spacing (config, file, &error))
        {
          if (error == NULL)
            /* Auto-detection occurred and was not successful. */
            g_set_error (&error,
                         G_FILE_ERROR, G_FILE_ERROR_INVAL,
                         _("Auto-detection of sample spacing failed. "
                           "\"%s\" does not appear to be a valid HGT file "
                           "or its variant is not supported yet. "
                           "Supported HGT files are: SRTM-1 and SRTM-3. "
                           "If you know the variant, run with argument 1 or 3."),
                         gimp_file_get_utf8_name (file));

          status = GIMP_PDB_CALLING_ERROR;
        }
    }
  else
    {
      /* we only run interactively due to the nature of this plugin.
       * things like generate preview etc like to call us non-
       * interactively.  here we stop that.
       */
      status = GIMP_PDB_CALLING_ERROR;
    }

  /* we are okay, and the user clicked OK in the load dialog */
  if (status == GIMP_PDB_SUCCESS)
    image = load_image (file, config, &error);

  if (status != GIMP_PDB_SUCCESS && error)
    {
      g_printerr ("Loading \"%s\" failed with error: %s",
                  gimp_file_get_utf8_name (file),
                  error->message);
    }

  if (! image)
    return gimp_procedure_new_return_values (procedure, status, error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
raw_export (GimpProcedure        *procedure,
            GimpRunMode           run_mode,
            GimpImage            *image,
            GFile                *file,
            GimpExportOptions    *options,
            GimpMetadata         *metadata,
            GimpProcedureConfig  *config,
            gpointer              run_data)
{
  GimpPDBStatusType       status = GIMP_PDB_SUCCESS;
  GimpExportReturn        export = GIMP_EXPORT_IGNORE;
  GList                  *drawables;
  RawPlanarConfiguration  planar_conf;
  GError                 *error  = NULL;

  gegl_init (NULL, NULL);

  planar_conf = gimp_procedure_config_get_choice_id (config, "planar-configuration");

  if ((planar_conf != RAW_PLANAR_CONTIGUOUS) && (planar_conf != RAW_PLANAR_SEPARATE))
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               NULL);
    }

  export = gimp_export_options_get_image (options, &image);
  drawables = gimp_image_list_layers (image);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! save_dialog (image, procedure,
                         gimp_drawable_has_alpha (drawables->data),
                         G_OBJECT (config)))
        status = GIMP_PDB_CANCEL;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! export_image (file, image, drawables->data, config, &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
}

/* get file size from a filen */
static goffset
get_file_info (GFile *file)
{
  GFileInfo *info;
  goffset    size = 0;

  info = g_file_query_info (file,
                            G_FILE_ATTRIBUTE_STANDARD_SIZE,
                            G_FILE_QUERY_INFO_NONE,
                            NULL, NULL);

  if (info)
    {
      size = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_STANDARD_SIZE);

      g_object_unref (info);
    }

  return size;
}

/* new image handle functions */
static void
raw_read_row (FILE   *fp,
              guchar *buf,
              gint32  offset,
              gint32  size)
{
  size_t bread;

  fseek (fp, offset, SEEK_SET);

  memset (buf, 0xFF, size);
  bread = fread (buf, 1, size, fp);
  if (bread < size)
    {
      g_printerr ("fread failed: read %u instead of %u bytes\n", (guint) bread, (guint) size);
    }
}

/* similar to the above function but memset is done differently. has nothing
 * to do with mmap, by the way
 */
static gint
mmap_read (gint    fd,
           void   *buf,
           gint32  len,
           gint32  pos,
           gint    rowstride)
{
  lseek (fd, pos, SEEK_SET);
  if (! read (fd, buf, len))
    memset (buf, 0xFF, rowstride);
  return 0;
}

/* This handles simple cases where each component has the same size, is
 * in the same order as in the GEGL buffer and is one or several full
 * bytes.
 */
static gboolean
raw_load_standard (RawGimpData *data,
                   gint         width,
                   gint         height,
                   gint         bpp,
                   gint         offset,
                   RawType      type,
                   gboolean     is_big_endian,
                   gboolean     is_signed,
                   gboolean     is_float)
{
  GeglBufferIterator *iter;
  guchar             *in = NULL;

  gint                input_stride;
  gint                n_components;
  gint                bpc;

  input_stride = width * bpp;
  n_components = babl_format_get_n_components (gegl_buffer_get_format (data->buffer));
  bpc          = bpp / n_components;

  g_return_val_if_fail (bpc * n_components == bpp, FALSE);

  iter = gegl_buffer_iterator_new (data->buffer, GEGL_RECTANGLE (0, 0, width, height),
                                   0, NULL, GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      const GeglRectangle *roi = &iter->items[0].roi;
      guchar              *out = iter->items[0].data;
      gint                 line;
      gint                 in_size;

      in_size  = roi->width * bpp;
      in       = g_realloc (in, in_size);
      for (line = 0; line < roi->height; line++)
        {
          raw_read_row (data->fp, in,
                        offset + ((roi->y + line) * input_stride) + roi->x * bpp,
                        in_size);
          for (gint x = 0; x < roi->width; x++)
            {
              for (gint c = 0; c < n_components; c++)
                {
                  gint pixel_val;
                  gint pos = x * n_components + c;

                  if (bpc == 1)
                    {
                      pixel_val = (gint) in[pos];
                    }
                  else if (bpc == 2)
                    {
                      if (is_float)
                        {
                          pixel_val = ((guint16 *) in)[pos];
                        }
                      else if (is_big_endian)
                        {
                          if (is_signed)
                            pixel_val = GINT16_FROM_BE (((guint16 *) in)[pos]) - G_MININT16;
                          else
                            pixel_val = GUINT16_FROM_BE (((guint16 *) in)[pos]);
                        }
                      else
                        {
                          if (is_signed)
                            pixel_val = GINT16_FROM_LE (((guint16 *) in)[pos]) - G_MININT16;
                          else
                            pixel_val = GUINT16_FROM_LE (((guint16 *) in)[pos]);
                        }
                    }
                  else /* if (bpc == 4) */
                    {
                      g_return_val_if_fail (bpc == 4, FALSE);

                      if (is_float)
                        {
                          pixel_val = ((guint32 *) in)[pos];
                        }
                      else if (is_big_endian)
                        {
                          if (is_signed)
                            pixel_val = GINT32_FROM_BE (((guint32 *) in)[pos]) - G_MININT32;
                          else
                            pixel_val = GUINT32_FROM_BE (((guint32 *) in)[pos]);
                        }
                      else
                        {
                          if (is_signed)
                            pixel_val = GINT32_FROM_LE (((guint32 *) in)[pos]) - G_MININT32;
                          else
                            pixel_val = GUINT32_FROM_LE (((guint32 *) in)[pos]);
                        }
                    }

                  if (is_float)
                    {
                      gchar  *out2;
                      gchar  *in2;
                      gint32  int_val_32 = pixel_val;
                      gint16  int_val_16 = pixel_val;

                      if (bpc == 4)
                        {
                          out2 = (gchar *) ((guint32 *) out + pos);
                          in2  = (gchar *) (&int_val_32);
                        }
                      else /* if (bpc == 2) */
                        {
                          out2 = (gchar *) ((guint16 *) out + pos);
                          in2  = (gchar *) (&int_val_16);
                        }

                      /* Avoiding any type conversion by tricking the
                       * type system to just copy data as-is at every
                       * step. While we could have done differently for
                       * float (4-bytes), this is even more necessary
                       * for half float (2-bytes) as we don't even have
                       * a type to use in glib for this.
                       */
                      for (gint b = 0; b < bpc; b++)
                        out2[b] = in2[b];
                    }
                  else if (bpc == 4)
                    {
                      ((guint32*) out)[pos] = (guint32) pixel_val;
                    }
                  else if (bpc == 2)
                    {
                      ((guint16*) out)[pos] = (guint16) pixel_val;
                    }
                  else
                    {
                      out[pos] = (gchar) pixel_val;
                    }
                }
            }
          out += in_size;
        }
    }

  g_free (in);

  return TRUE;
}

static gboolean
raw_load_planar (RawGimpData *data,
                 gint         width,
                 gint         height,
                 gint         bpp,
                 gint         offset,
                 RawType      type,
                 gboolean     is_big_endian,
                 gboolean     is_signed,
                 gboolean     is_float)
{
  GeglBufferIterator *iter;
  guchar             *in = NULL;

  gint                input_stride;
  gint                n_components;
  gint                bpc;

  n_components = babl_format_get_n_components (gegl_buffer_get_format (data->buffer));
  bpc          = bpp / n_components;
  input_stride = width * bpc;

  g_return_val_if_fail (bpc * n_components == bpp, FALSE);

  iter = gegl_buffer_iterator_new (data->buffer, GEGL_RECTANGLE (0, 0, width, height),
                                   0, NULL, GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      const GeglRectangle *roi = &iter->items[0].roi;
      guchar              *out = iter->items[0].data;
      gint                 line;
      gint                 in_size;

      in_size  = roi->width * bpc;
      in       = g_realloc (in, in_size);

      for (gint c = 0; c < n_components; c++)
        {
          for (line = 0; line < roi->height; line++)
            {
              raw_read_row (data->fp, in,
                            offset +
                            width * height  * bpc * c +
                            ((roi->y + line) * input_stride) + roi->x * bpc,
                            in_size);

              for (gint x = 0; x < roi->width; x++)
                {
                  gint pixel_val;
                  gint pos = line * roi->width * n_components + x * n_components + c;

                  if (bpc == 1)
                    {
                      pixel_val = (gint) in[x];
                    }
                  else if (bpc == 2)
                    {
                      if (is_float)
                        {
                          pixel_val = ((guint16 *) in)[x];
                        }
                      else if (is_big_endian)
                        {
                          if (is_signed)
                            pixel_val = GINT16_FROM_BE (((guint16 *) in)[x]) - G_MININT16;
                          else
                            pixel_val = GUINT16_FROM_BE (((guint16 *) in)[x]);
                        }
                      else
                        {
                          if (is_signed)
                            pixel_val = GINT16_FROM_LE (((guint16 *) in)[x]) - G_MININT16;
                          else
                            pixel_val = GUINT16_FROM_LE (((guint16 *) in)[x]);
                        }
                    }
                  else /* if (bpc == 4) */
                    {
                      g_return_val_if_fail (bpc == 4, FALSE);

                      if (is_float)
                        {
                          pixel_val = ((guint32 *) in)[x];
                        }
                      else if (is_big_endian)
                        {
                          if (is_signed)
                            pixel_val = GINT32_FROM_BE (((guint32 *) in)[x]) - G_MININT32;
                          else
                            pixel_val = GUINT32_FROM_BE (((guint32 *) in)[x]);
                        }
                      else
                        {
                          if (is_signed)
                            pixel_val = GINT32_FROM_LE (((guint32 *) in)[x]) - G_MININT32;
                          else
                            pixel_val = GUINT32_FROM_LE (((guint32 *) in)[x]);
                        }
                    }

                  if (is_float)
                    {
                      gchar  *out2;
                      gchar  *in2;
                      gint32  int_val_32 = pixel_val;
                      gint16  int_val_16 = pixel_val;

                      if (bpc == 4)
                        {
                          out2 = (gchar *) ((guint32 *) out + pos);
                          in2  = (gchar *) (&int_val_32);
                        }
                      else /* if (bpc == 2) */
                        {
                          out2 = (gchar *) ((guint16 *) out + pos);
                          in2  = (gchar *) (&int_val_16);
                        }

                      /* Avoiding any type conversion by tricking the
                       * type system to just copy data as-is at every
                       * step. While we could have done differently for
                       * float (4-bytes), this is even more necessary
                       * for half float (2-bytes) as we don't even have
                       * a type to use in glib for this.
                       */
                      for (gint b = 0; b < bpc; b++)
                        out2[b] = in2[b];
                    }
                  else if (bpc == 4)
                    {
                      ((guint32*) out)[pos] = (guint32) pixel_val;
                    }
                  else if (bpc == 2)
                    {
                      ((guint16*) out)[pos] = (guint16) pixel_val;
                    }
                  else
                    {
                      out[pos] = (guchar) pixel_val;
                    }
                }
            }
        }
    }

  g_free (in);

  return TRUE;
}

/* this handles black and white, gray with 1, 2, 4, and 8 _bits_ per
 * pixel images - hopefully lots of binaries too
 */
static gboolean
raw_load_gray (RawGimpData *data,
               gint         width,
               gint         height,
               gint         offset,
               gint         bpp,
               gint         bitspp)
{
  guchar *in_raw = NULL;
  guchar *out_raw = NULL;
  gint    in_size;
  gint    out_size;
  guchar  pixel_mask_hi;
  guchar  pixel_mask_lo;
  guint   x;
  gint    i;

  in_size  = width * height / (8 / bitspp);
  out_size = width * height * 3;

  in_raw = g_try_malloc (in_size);
  if (! in_raw)
    return FALSE;

  out_raw = g_try_malloc (out_size);
  if (! out_raw)
    return FALSE;
  memset (out_raw, 0, out_size);

  /* calculate a pixel_mask_hi
     0x80 for 1 bitspp
     0xc0 for 2 bitspp
     0xf0 for 4 bitspp
     0xff for 8 bitspp
     and a pixel_mask_lo
     0x01 for 1 bitspp
     0x03 for 2 bitspp
     0x0f for 4 bitspp
     0xff for 8 bitspp
   */
  pixel_mask_hi = 0x80;
  pixel_mask_lo = 0x01;
  for (i = 1; i < bitspp; i++)
    {
      pixel_mask_hi |= pixel_mask_hi >> 1;
      pixel_mask_lo |= pixel_mask_lo << 1;
    }

  raw_read_row (data->fp, in_raw, offset, in_size);

  x = 0; /* walks though all output pixels */
  for (i = 0; i < in_size; i++)
    {
      guchar bit;

      for (bit = 0; bit < 8 / bitspp; bit++)
        {
          guchar pixel_val;

          pixel_val = in_raw[i] & (pixel_mask_hi >> (bit * bitspp));
          pixel_val >>= 8 - bitspp - bit * bitspp;
          pixel_val *= 0xff / pixel_mask_lo;

          out_raw[3 * x + 0] = pixel_val;
          out_raw[3 * x + 1] = pixel_val;
          out_raw[3 * x + 2] = pixel_val;

          x++;
        }
    }

  gegl_buffer_set (data->buffer, GEGL_RECTANGLE (0, 0, width, height),
                   0, NULL, out_raw, GEGL_AUTO_ROWSTRIDE);

  g_free (in_raw);
  g_free (out_raw);

  return TRUE;
}

/* this handles RGB565 images */
static gboolean
raw_load_rgb565 (RawGimpData   *data,
                 gint           width,
                 gint           height,
                 gint           offset,
                 RawType        type,
                 RawEndianness  endianness)
{
  GeglBufferIterator *iter;
  guint16            *in = NULL;

  iter = gegl_buffer_iterator_new (data->buffer, GEGL_RECTANGLE (0, 0, width, height),
                                   0, NULL, GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      const GeglRectangle *roi = &iter->items[0].roi;
      guchar              *out = iter->items[0].data;
      gint                 line;

      in = g_realloc (in, roi->width * 2);
      for (line = 0; line < roi->height; line++)
        {
          raw_read_row (data->fp, (guchar *) in,
                        offset + ((roi->y + line) * width * 2) + roi->x * 2,
                        roi->width * 2);
          rgb_565_to_888 (in, out + line * roi->width * 3, roi->width, type, endianness);
        }
    }

  g_free (in);

  return TRUE;
}

/* this converts a 2bpp buffer to a 3bpp buffer in is a buffer of
 * 16bit pixels, out is a buffer of 24bit pixels
 */
static void
rgb_565_to_888 (guint16       *in,
                guchar        *out,
                gint32         num_pixels,
                RawType        type,
                RawEndianness  endianness)
{
  guint32 i, j;
  guint16 input;
  gboolean swap_endian;

  if (G_BYTE_ORDER == G_LITTLE_ENDIAN || G_BYTE_ORDER == G_PDP_ENDIAN)
    {
      swap_endian = (endianness == RAW_BIG_ENDIAN);
    }
  else if (G_BYTE_ORDER == G_BIG_ENDIAN)
    {
      swap_endian = (endianness == RAW_LITTLE_ENDIAN);
    }

  switch (type)
    {
    case RAW_RGB565:
      for (i = 0, j = 0; i < num_pixels; i++)
        {
          input = in[i];

          if (swap_endian)
            input = GUINT16_SWAP_LE_BE (input);

          out[j++] = ((((input >> 11) & 0x1f) * 0x21) >> 2);
          out[j++] = ((((input >>  5) & 0x3f) * 0x41) >> 4);
          out[j++] = ((((input >>  0) & 0x1f) * 0x21) >> 2);
        }
      break;

    case RAW_BGR565:
      for (i = 0, j = 0; i < num_pixels; i++)
        {
          input = in[i];

          if (swap_endian)
            input = GUINT16_SWAP_LE_BE (input);

          out[j++] = ((((input >>  0) & 0x1f) * 0x21) >> 2);
          out[j++] = ((((input >>  5) & 0x3f) * 0x41) >> 4);
          out[j++] = ((((input >> 11) & 0x1f) * 0x21) >> 2);
        }
      break;

    default:
      /*This conversion function does not handle the passed in
       * planar-configuration*/
      g_assert_not_reached ();
    }
}

static gboolean
raw_load_palette (RawGimpData    *data,
                  gint            palette_offset,
                  RawPaletteType  palette_type,
                  GFile          *palette_file)
{
  guchar temp[1024];
  gint   fd, i, j;

  if (palette_file)
    {
      fd = g_open (g_file_peek_path (palette_file), O_RDONLY, 0);

      if (! fd)
        return FALSE;

      lseek (fd, palette_offset, SEEK_SET);

      switch (palette_type)
        {
        case RAW_PALETTE_RGB:
          read (fd, data->cmap, 768);
          break;

        case RAW_PALETTE_BGR:
          read (fd, temp, 1024);
          for (i = 0, j = 0; i < 256; i++)
            {
              data->cmap[j++] = temp[i * 4 + 2];
              data->cmap[j++] = temp[i * 4 + 1];
              data->cmap[j++] = temp[i * 4 + 0];
            }
          break;
        }

      close (fd);
    }
  else
    {
      /* make a fake grayscale color map */
      for (i = 0, j = 0; i < 256; i++)
        {
          data->cmap[j++] = i;
          data->cmap[j++] = i;
          data->cmap[j++] = i;
        }
    }

  gimp_image_set_colormap (data->image, data->cmap, 256);

  return TRUE;
}

/* end new image handle functions */

static gboolean
export_image (GFile                *file,
              GimpImage            *image,
              GimpDrawable         *drawable,
              GimpProcedureConfig  *config,
              GError              **error)
{
  GeglBuffer             *buffer;
  const Babl             *format = NULL;
  guchar                 *cmap   = NULL;  /* colormap for indexed images */
  guchar                 *buf;
  guchar                 *components[4] = { 0, };
  gint                    n_components;
  gint32                  width, height, bpp;
  gint                    bpc;
  FILE                   *fp;
  gint                    i, j, c;
  gint                    palsize = 0;
  RawPlanarConfiguration  planar_conf;    /* Planar Configuration (CONTIGUOUS, PLANAR) */
  RawPaletteType          palette_type;   /* type of palette (RGB/BGR)   */
  gboolean                ret = FALSE;

  planar_conf  = gimp_procedure_config_get_choice_id (config, "planar-configuration");
  palette_type = gimp_procedure_config_get_choice_id (config, "palette-type");

  buffer = gimp_drawable_get_buffer (drawable);

  format = gimp_drawable_get_format (drawable);

  n_components = babl_format_get_n_components (format);
  bpp          = babl_format_get_bytes_per_pixel (format);
  bpc          = bpp / n_components;

  g_return_val_if_fail (bpc * n_components == bpp, FALSE);

  if (gimp_drawable_is_indexed (drawable))
    cmap = gimp_image_get_colormap (image, NULL, &palsize);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  buf = g_new (guchar, width * height * bpp);

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, width, height), 1.0,
                   format, buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_object_unref (buffer);

  fp = g_fopen (g_file_peek_path (file), "wb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  ret = TRUE;

  switch (planar_conf)
    {
    case RAW_PLANAR_CONTIGUOUS:
      if (! fwrite (buf, width * height * bpp, 1, fp))
        {
          fclose (fp);
          return FALSE;
        }

      fclose (fp);

      if (cmap)
        {
          /* we have colormap, too.write it into filename+pal */
          gchar *newfile = g_strconcat (g_file_peek_path (file), ".pal", NULL);
          gchar *temp;

          fp = g_fopen (newfile, "wb");

          if (! fp)
            {
              g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Could not open '%s' for writing: %s"),
                           gimp_filename_to_utf8 (newfile), g_strerror (errno));
              return FALSE;
            }

          switch (palette_type)
            {
            case RAW_PALETTE_RGB:
              if (!fwrite (cmap, palsize * 3, 1, fp))
                ret = FALSE;
              fclose (fp);
              break;

            case RAW_PALETTE_BGR:
              temp = g_malloc0 (palsize * 4);
              for (i = 0, j = 0; i < palsize * 3; i += 3)
                {
                  temp[j++] = cmap[i + 2];
                  temp[j++] = cmap[i + 1];
                  temp[j++] = cmap[i + 0];
                  temp[j++] = 0;
                }
              if (!fwrite (temp, palsize * 4, 1, fp))
                ret = FALSE;
              fclose (fp);
              g_free (temp);
              break;
            }
        }
      break;

    case RAW_PLANAR_SEPARATE:
      for (c = 0; c < n_components; c++)
        components[c] = g_new (guchar, width * height * bpc);

      for (i = 0; i < width * height; i++)
        {
          for (c = 0; c < n_components; c++)
            {
              if (bpc == 1)
                components[c][i] = buf[i * n_components + c];
              else if (bpc == 2)
                ((guint16 **) components)[c][i] = ((guint16 *) buf)[i * n_components + c];
              else /* if (bpc == 4) */
                ((guint32 **) components)[c][i] = ((guint32 *) buf)[i * n_components + c];
            }
        }

      ret = TRUE;
      for (c = 0; c < n_components; c++)
        {
          if (! fwrite (components[c], width * height * bpc, 1, fp))
            ret = FALSE;

          g_free (components[c]);
        }

      fclose (fp);
      break;

    default:
      fclose (fp);
      break;
    }

  return ret;
}

static void
get_bpp (GimpProcedureConfig *config,
         gint                *bpp,
         gint                *bitspp)
{
  GimpProcedure *procedure;

  procedure = gimp_procedure_config_get_procedure (config);

  *bitspp = 8;
  if (g_strcmp0 (gimp_procedure_get_name (procedure), LOAD_HGT_PROC) == 0)
    {
      *bpp = 2;
    }
  else
    {
      RawType image_type;

      image_type = gimp_procedure_config_get_choice_id (config, "pixel-format");

      switch (image_type)
        {
        case RAW_RGB_8BPP:
          *bpp = 3;
          break;

        case RAW_RGB_16BPP:
          *bpp = 6;
          break;

        case RAW_RGB_32BPP:
          *bpp = 12;
          break;

        case RAW_RGB565:
        case RAW_BGR565:
          *bpp = 2;
          break;

        case RAW_RGBA_8BPP:
          *bpp = 4;
          break;

        case RAW_RGBA_16BPP:
          *bpp = 8;
          break;

        case RAW_RGBA_32BPP:
          *bpp = 16;
          break;

        case RAW_GRAY_1BPP:
          *bpp    = 1;
          *bitspp = 1;
          break;
        case RAW_GRAY_2BPP:
          *bpp    = 1;
          *bitspp = 2;
          break;
        case RAW_GRAY_4BPP:
          *bpp    = 1;
          *bitspp = 4;
          break;
        case RAW_GRAY_8BPP:
          *bpp   = 1;
          break;

        case RAW_INDEXED:
          *bpp   = 1;
          break;

        case RAW_INDEXEDA:
          *bpp   = 2;
          break;

        case RAW_GRAY_16BPP:
          *bpp   = 2;
          break;

        case RAW_GRAY_32BPP:
          *bpp   = 4;
          break;

        case RAW_GRAYA_8BPP:
          *bpp   = 2;
          break;

        case RAW_GRAYA_16BPP:
          *bpp   = 4;
          break;

        case RAW_GRAYA_32BPP:
          *bpp   = 8;
          break;
        }
    }
}

static gboolean
detect_sample_spacing (GimpProcedureConfig  *config,
                       GFile                *file,
                       GError              **error)
{
  HgtSampleSpacing  sample_spacing = HGT_SRTM_AUTO_DETECT;
  FILE             *fp;
  glong             pos;

  fp = g_fopen (g_file_peek_path (file), "rb");
  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for size verification: %s"),
                   gimp_file_get_utf8_name (file),
                   g_strerror (errno));
    }
  else
    {
      fseek (fp, 0, SEEK_END);
      pos = ftell (fp);

      /* HGT files have always the same size, either 1201*1201
       * or 3601*3601 of 16-bit values.
       */
      if (pos == 1201*1201*2)
        {
          sample_spacing = HGT_SRTM_3;
          g_object_set (config,
                        "sample-spacing", "srtm-3",
                        NULL);
        }
      else if (pos == 3601*3601*2)
        {
          sample_spacing = HGT_SRTM_1;
          g_object_set (config,
                        "sample-spacing", "srtm-1",
                        NULL);
        }

      fclose (fp);
    }

  return (sample_spacing != HGT_SRTM_AUTO_DETECT);
}

static void
get_load_config_values (GimpProcedureConfig     *config,
                        gint32                  *file_offset,
                        gint32                  *image_width,
                        gint32                  *image_height,
                        RawType                 *image_type,
                        RawEncoding             *encoding,
                        RawEndianness           *endianness,
                        RawPlanarConfiguration  *planar_configuration,
                        gint32                  *palette_offset,
                        RawPaletteType          *palette_type,
                        GFile                  **palette_file)
{
  GimpProcedure *procedure;

  procedure = gimp_procedure_config_get_procedure (config);

  if (g_strcmp0 (gimp_procedure_get_name (procedure), LOAD_HGT_PROC) == 0)
    {
      gint sample_spacing;

      sample_spacing = gimp_procedure_config_get_choice_id (config, "sample-spacing");

      if (sample_spacing == HGT_SRTM_3)
        *image_width = *image_height = 1201;
      else
        *image_width = *image_height = 3601;

      *file_offset          = 0;
      *image_type           = RAW_GRAY_16BPP;
      *encoding             = RAW_ENCODING_SIGNED;
      *endianness           = RAW_BIG_ENDIAN;
      *planar_configuration = RAW_PLANAR_CONTIGUOUS;
      *palette_offset       = 0;
      *palette_type         = RAW_PALETTE_RGB;
      *palette_file         = NULL;
    }
  else
    {
      g_object_get (config,
                    "offset",               file_offset,
                    "width",                image_width,
                    "height",               image_height,
                    "palette-offset",       palette_offset,
                    "palette-file",         palette_file,
                    NULL);
      *image_type   = gimp_procedure_config_get_choice_id (config, "pixel-format");
      *encoding     = gimp_procedure_config_get_choice_id (config, "data-type");
      *endianness   = gimp_procedure_config_get_choice_id (config, "endianness");
      *palette_type = gimp_procedure_config_get_choice_id (config, "palette-type");

      *planar_configuration =
        gimp_procedure_config_get_choice_id (config, "planar-configuration");
    }
}

static GimpImage *
load_image (GFile                *file,
            GimpProcedureConfig  *config,
            GError              **error)
{
  RawGimpData       *data;
  GimpLayer         *layer     = NULL;
  GimpImageType      ltype     = GIMP_RGB_IMAGE;
  GimpImageBaseType  itype     = GIMP_RGB;
  GimpPrecision      precision = GIMP_PRECISION_U8_NON_LINEAR;
  RawType            pixel_format;
  RawEncoding        encoding;
  RawEndianness      endianness;
  RawPlanarConfiguration planar_configuration;
  goffset            size;
  gint               width;
  gint               height;
  gint               offset;
  gint               bpp    = 0;
  gint               bitspp = 8;

  gint               palette_offset;
  RawPaletteType     palette_type;
  GFile             *palette_file;

  data = g_new0 (RawGimpData, 1);

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  data->fp = g_fopen (g_file_peek_path (file), "rb");

  if (! data->fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  get_load_config_values (config, &offset, &width, &height,
                          &pixel_format, &encoding, &endianness, &planar_configuration,
                          &palette_offset, &palette_type, &palette_file);

  size = get_file_info (file);

  switch (pixel_format)
    {
    case RAW_RGB_8BPP:        /* standard RGB */
      bpp = 3;

    case RAW_RGB_16BPP:
      if (bpp == 0)
        {
          bpp = 6;
          if (encoding == RAW_ENCODING_FLOAT)
            precision = GIMP_PRECISION_HALF_NON_LINEAR;
          else
            precision = GIMP_PRECISION_U16_NON_LINEAR;
        }

    case RAW_RGB_32BPP:
      if (bpp == 0)
        {
          bpp = 12;
          if (encoding == RAW_ENCODING_FLOAT)
            precision = GIMP_PRECISION_FLOAT_NON_LINEAR;
          else
            precision = GIMP_PRECISION_U32_NON_LINEAR;
        }

      ltype = GIMP_RGB_IMAGE;
      itype = GIMP_RGB;
      break;

    case RAW_RGB565:       /* RGB565 */
    case RAW_BGR565:       /* BGR565 */
      bpp   = 2;
      ltype = GIMP_RGB_IMAGE;
      itype = GIMP_RGB;
      break;

    case RAW_RGBA_8BPP:       /* RGB + alpha */
      bpp   = 4;
      ltype = GIMP_RGBA_IMAGE;
      itype = GIMP_RGB;
      break;

    case RAW_RGBA_16BPP:
      bpp   = 8;
      ltype = GIMP_RGBA_IMAGE;
      itype = GIMP_RGB;
      if (encoding == RAW_ENCODING_FLOAT)
        precision = GIMP_PRECISION_HALF_NON_LINEAR;
      else
        precision = GIMP_PRECISION_U16_NON_LINEAR;
      break;

    case RAW_RGBA_32BPP:
      bpp   = 16;
      ltype = GIMP_RGBA_IMAGE;
      itype = GIMP_RGB;
      if (encoding == RAW_ENCODING_FLOAT)
        precision = GIMP_PRECISION_FLOAT_NON_LINEAR;
      else
        precision = GIMP_PRECISION_U32_NON_LINEAR;
      break;

    case RAW_GRAY_1BPP:
      bpp    = 1;
      bitspp = 1;
      ltype  = GIMP_RGB_IMAGE;
      itype  = GIMP_RGB;
      break;
    case RAW_GRAY_2BPP:
      bpp    = 1;
      bitspp = 2;
      ltype  = GIMP_RGB_IMAGE;
      itype  = GIMP_RGB;
      break;
    case RAW_GRAY_4BPP:
      bpp    = 1;
      bitspp = 4;
      ltype  = GIMP_RGB_IMAGE;
      itype  = GIMP_RGB;
      break;
    case RAW_GRAY_8BPP:
      bpp   = 1;
      ltype = GIMP_GRAY_IMAGE;
      itype = GIMP_GRAY;
      break;

    case RAW_INDEXED:         /* Indexed */
      bpp   = 1;
      ltype = GIMP_INDEXED_IMAGE;
      itype = GIMP_INDEXED;
      break;

    case RAW_INDEXEDA:        /* Indexed + alpha */
      bpp   = 2;
      ltype = GIMP_INDEXEDA_IMAGE;
      itype = GIMP_INDEXED;
      break;

    case RAW_GRAY_16BPP:
      bpp       = 2;
      ltype     = GIMP_GRAY_IMAGE;
      itype     = GIMP_GRAY;
      if (encoding == RAW_ENCODING_FLOAT)
        precision = GIMP_PRECISION_HALF_NON_LINEAR;
      else
        precision = GIMP_PRECISION_U16_NON_LINEAR;
      break;

    case RAW_GRAY_32BPP:
      bpp       = 4;
      ltype     = GIMP_GRAY_IMAGE;
      itype     = GIMP_GRAY;
      if (encoding == RAW_ENCODING_FLOAT)
        precision = GIMP_PRECISION_FLOAT_NON_LINEAR;
      else
        precision = GIMP_PRECISION_U32_NON_LINEAR;
      break;

    case RAW_GRAYA_8BPP:
      bpp       = 2;
      ltype     = GIMP_GRAYA_IMAGE;
      itype     = GIMP_GRAY;
      precision = GIMP_PRECISION_U8_NON_LINEAR;
      break;

    case RAW_GRAYA_16BPP:
      bpp       = 4;
      ltype     = GIMP_GRAYA_IMAGE;
      itype     = GIMP_GRAY;
      if (encoding == RAW_ENCODING_FLOAT)
        precision = GIMP_PRECISION_HALF_NON_LINEAR;
      else
        precision = GIMP_PRECISION_U16_NON_LINEAR;
      break;

    case RAW_GRAYA_32BPP:
      bpp       = 8;
      ltype     = GIMP_GRAYA_IMAGE;
      itype     = GIMP_GRAY;
      if (encoding == RAW_ENCODING_FLOAT)
        precision = GIMP_PRECISION_FLOAT_NON_LINEAR;
      else
        precision = GIMP_PRECISION_U32_NON_LINEAR;
      break;
    }

  /* make sure we don't load image bigger than file size */
  if (height > (size / width / bpp * 8 / bitspp))
    height = size / width / bpp * 8 / bitspp;

  data->image = gimp_image_new_with_precision (width, height, itype, precision);

  layer = gimp_layer_new (data->image, _("Background"),
                          width, height, ltype, 100,
                          gimp_image_get_default_new_layer_mode (data->image));
  gimp_image_insert_layer (data->image, layer, NULL, 0);

  data->buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  switch (pixel_format)
    {
    case RAW_RGB_8BPP:
    case RAW_RGB_16BPP:
    case RAW_RGB_32BPP:

    case RAW_RGBA_8BPP:
    case RAW_RGBA_16BPP:
    case RAW_RGBA_32BPP:

    case RAW_GRAY_8BPP:
    case RAW_GRAY_16BPP:
    case RAW_GRAY_32BPP:

    case RAW_GRAYA_8BPP:
    case RAW_GRAYA_16BPP:
    case RAW_GRAYA_32BPP:
      if (planar_configuration == RAW_PLANAR_CONTIGUOUS)
        raw_load_standard (data, width, height, bpp, offset, pixel_format,
                           endianness == RAW_BIG_ENDIAN,
                           encoding == RAW_ENCODING_SIGNED,
                           encoding == RAW_ENCODING_FLOAT);
      else
        raw_load_planar (data, width, height, bpp, offset, pixel_format,
                         endianness == RAW_BIG_ENDIAN,
                         encoding == RAW_ENCODING_SIGNED,
                         encoding == RAW_ENCODING_FLOAT);
      break;

    case RAW_RGB565:
    case RAW_BGR565:
      raw_load_rgb565 (data, width, height, offset, pixel_format, endianness);
      break;

    case RAW_GRAY_1BPP:
      raw_load_gray (data, width, height, offset, bpp, bitspp);
      break;
    case RAW_GRAY_2BPP:
      raw_load_gray (data, width, height, offset, bpp, bitspp);
      break;
    case RAW_GRAY_4BPP:
      raw_load_gray (data, width, height, offset, bpp, bitspp);
      break;

    case RAW_INDEXED:
    case RAW_INDEXEDA:
      raw_load_palette (data, palette_offset, palette_type, palette_file);
      raw_load_standard (data, width, height, bpp, offset, pixel_format,
                         endianness == RAW_BIG_ENDIAN,
                         encoding == RAW_ENCODING_SIGNED,
                         encoding == RAW_ENCODING_FLOAT);
      break;
    }

  fclose (data->fp);

  g_object_unref (data->buffer);
  g_clear_object (&palette_file);

  return data->image;
}


/* misc GUI stuff */

/* Taken straight from babl repository in file `babl/base/type-half.c`.
 */
static void
halfp2singles (uint32_t       *xp,
               const uint16_t *hp,
               int             numel)
{

  uint16_t h, hs, he, hm;
  uint32_t xs, xe, xm;
  int32_t xes;
  int e;



  if( xp == NULL || hp == NULL ) // Nothing to convert (e.g., imag part of pure real)
    return;

  while( numel-- ) {
      h = *hp++;
      if( (h & 0x7FFFu) == 0 ) {  // Signed zero
          *xp++ = ((uint32_t) h) << 16;  // Return the signed zero
      } else { // Not zero
          hs = h & 0x8000u;  // Pick off sign bit
          he = h & 0x7C00u;  // Pick off exponent bits
          hm = h & 0x03FFu;  // Pick off mantissa bits
          if( he == 0 ) {  // Denormal will convert to normalized
              e = -1; // The following loop figures out how much extra to adjust the exponent
              do {
                  e++;
                  hm <<= 1;
              } while( (hm & 0x0400u) == 0 ); // Shift until leading bit overflows into exponent bit
              xs = ((uint32_t) hs) << 16; // Sign bit
              xes = ((int32_t) (he >> 10)) - 15 + 127 - e; // Exponent unbias the halfp, then bias the single
              xe = (uint32_t) (xes << 23); // Exponent
              xm = ((uint32_t) (hm & 0x03FFu)) << 13; // Mantissa
              *xp++ = (xs | xe | xm); // Combine sign bit, exponent bits, and mantissa bits
          } else if( he == 0x7C00u ) {  // Inf or NaN (all the exponent bits are set)
              if( hm == 0 ) { // If mantissa is zero ...
                  *xp++ = (((uint32_t) hs) << 16) | ((uint32_t) 0x7F800000u); // Signed Inf
              } else {
                  *xp++ = (uint32_t) 0xFFC00000u; // NaN, only 1st mantissa bit set
              }
          } else { // Normalized number
              xs = ((uint32_t) hs) << 16; // Sign bit
              xes = ((int32_t) (he >> 10)) - 15 + 127; // Exponent unbias the halfp, then bias the single
              xe = (uint32_t) (xes << 23); // Exponent
              xm = ((uint32_t) hm) << 13; // Mantissa
              *xp++ = (xs | xe | xm); // Combine sign bit, exponent bits, and mantissa bits
          }
      }
  }
  return;
}

static void
preview_update (GimpPreviewArea *preview,
                gboolean         preview_cmap_update)
{
  GimpImageType        preview_type = GIMP_RGB_IMAGE;
  gint                 preview_width;
  gint                 preview_height;
  gint32               pos;
  gint                 x, y;
  gint                 bitspp       = 0;
  gint                 bpc          = 0;
  gint                 bpp          = 0;
  gint                 n_components = 0;
  gboolean             is_big_endian;
  gboolean             is_signed;
  gboolean             is_float;

  GimpProcedureConfig *config;
  RawType              pixel_format;
  RawEncoding          encoding;
  RawEndianness        endianness;
  RawPlanarConfiguration planar_configuration;
  gint                 width;
  gint                 height;
  gint                 offset;

  GFile               *palette_file;
  gint                 palette_offset;
  RawPaletteType       palette_type;

  gimp_preview_area_get_size (preview, &preview_width, &preview_height);

  config = g_object_get_data (G_OBJECT (preview), "procedure-config");

  get_load_config_values (config, &offset, &width, &height,
                          &pixel_format, &encoding, &endianness, &planar_configuration,
                          &palette_offset, &palette_type, &palette_file);
  width  = MIN (width,  preview_width);
  height = MIN (height, preview_height);

  gimp_preview_area_fill (preview,
                          0, 0, preview_width, preview_height,
                          255, 255, 255);

  is_big_endian = (endianness == RAW_BIG_ENDIAN);
  is_signed     = (encoding   == RAW_ENCODING_SIGNED);
  is_float      = (encoding   == RAW_ENCODING_FLOAT);

  switch (pixel_format)
    {
    case RAW_RGBA_8BPP:
      bpc = 1;
      bpp = 4;
    case RAW_RGBA_16BPP:
      if (bpc == 0)
        {
          bpc = 2;
          bpp = 8;
        }
    case RAW_RGBA_32BPP:
      if (bpc == 0)
        {
          bpc = 4;
          bpp = 16;
        }
      n_components = 4;
      preview_type = GIMP_RGBA_IMAGE;

    case RAW_RGB_8BPP:
      if (bpc == 0)
        {
          bpc = 1;
          bpp = 3;
        }
    case RAW_RGB_16BPP:
      if (bpc == 0)
        {
          bpc = 2;
          bpp = 6;
        }
    case RAW_RGB_32BPP:
      if (bpc == 0)
        {
          bpc = 4;
          bpp = 12;
        }
      if (n_components == 0)
        {
          n_components = 3;
          preview_type = GIMP_RGB_IMAGE;
        }

    case RAW_GRAYA_8BPP:
      if (bpc == 0)
        {
          bpc = 1;
          bpp = 2;
        }
    case RAW_GRAYA_16BPP:
      if (bpc == 0)
        {
          bpc = 2;
          bpp = 4;
        }
    case RAW_GRAYA_32BPP:
      if (bpc == 0)
        {
          bpc = 4;
          bpp = 8;
        }
      if (n_components == 0)
        {
          n_components = 2;
          preview_type = GIMP_GRAYA_IMAGE;
        }

    case RAW_GRAY_16BPP:
      if (bpc == 0)
        {
          bpc = 2;
          bpp = 2;
        }

    case RAW_GRAY_32BPP:
      if (bpc == 0)
        {
          bpc = 4;
          bpp = 4;
        }
      if (n_components == 0)
        {
          n_components = 1;
          preview_type = GIMP_GRAY_IMAGE;
        }

      if (planar_configuration == RAW_PLANAR_CONTIGUOUS)
        {
          guchar *in;
          guchar *row;
          gint    input_stride;
          gint    input_offset;

          input_stride = width * bpp;

          in  = g_new (guchar, input_stride);
          row = g_malloc (width * n_components);

          for (y = 0; y < height; y++)
            {
              input_offset = offset + (y * input_stride);
              mmap_read (preview_fd, (guchar*) in, input_stride, input_offset, input_stride);

              for (gint x = 0; x < width; x++)
                {
                  for (gint c = 0; c < n_components; c++)
                    {
                      guint  pixel_val = 0;
                      gfloat float_val = 0.0;
                      gint   pos       = x * n_components + c;

                      if (bpc == 1)
                        {
                          pixel_val = (guint) in[pos];
                        }
                      else if (bpc == 2)
                        {
                          if (is_float)
                            {
                              guint16 int16_val;

                              int16_val = ((guint16 *) in)[pos];
                              halfp2singles ((uint32_t *) &float_val, &int16_val, 1);
                            }
                          else if (is_big_endian)
                            {
                              if (is_signed)
                                pixel_val = GINT16_FROM_BE (((guint16 *) in)[pos]) - G_MININT16;
                              else
                                pixel_val = GUINT16_FROM_BE (((guint16 *) in)[pos]);
                            }
                          else
                            {
                              if (is_signed)
                                pixel_val = GINT16_FROM_LE (((guint16 *) in)[pos]) - G_MININT16;
                              else
                                pixel_val = GUINT16_FROM_LE (((guint16 *) in)[pos]);
                            }
                        }
                      else /* if (bpc == 4) */
                        {
                          g_return_if_fail (bpc == 4);

                          if (is_float)
                            {
                              float_val = ((gfloat *) in)[pos];
                            }
                          else if (is_big_endian)
                            {
                              if (is_signed)
                                pixel_val = GINT32_FROM_BE (((guint32 *) in)[pos]) - G_MININT32;
                              else
                                pixel_val = GUINT32_FROM_BE (((guint32 *) in)[pos]);
                            }
                          else
                            {
                              if (is_signed)
                                pixel_val = GINT32_FROM_LE (((guint32 *) in)[pos]) - G_MININT32;
                              else
                                pixel_val = GUINT32_FROM_LE (((guint32 *) in)[pos]);
                            }
                        }

                      if (is_float)
                        row[pos] = float_val * 255;
                      else
                        row[pos] = pixel_val / pow (2, (bpc - 1) * 8);
                    }
                }

              gimp_preview_area_draw (preview, 0, y, width, 1,
                                      preview_type, row, width * n_components);
            }

          g_free (in);
          g_free (row);
        }
      else
        {
          guchar *in;
          guchar *row;
          gint    input_stride;
          gint    input_offset;

          input_stride = width * bpc;

          in  = g_new (guchar, input_stride);
          row = g_malloc (width * n_components);

          for (y = 0; y < height; y++)
            {
              for (gint c = 0; c < n_components; c++)
                {
                  input_offset = offset + c * width * height * bpc + (y * input_stride);
                  mmap_read (preview_fd, (guchar*) in, input_stride, input_offset, input_stride);

                  for (gint x = 0; x < width; x++)
                    {
                      guint  pixel_val = 0;
                      gfloat float_val = 0.0;
                      gint   pos       = x * n_components + c;

                      if (bpc == 1)
                        {
                          pixel_val = (guint) in[x];
                        }
                      else if (bpc == 2)
                        {
                          if (is_float)
                            {
                              guint16 int16_val;

                              int16_val = ((guint16 *) in)[x];
                              halfp2singles ((uint32_t *) &float_val, &int16_val, 1);
                            }
                          else if (is_big_endian)
                            {
                              if (is_signed)
                                pixel_val = GINT16_FROM_BE (((guint16 *) in)[x]) - G_MININT16;
                              else
                                pixel_val = GUINT16_FROM_BE (((guint16 *) in)[x]);
                            }
                          else
                            {
                              if (is_signed)
                                pixel_val = GINT16_FROM_LE (((guint16 *) in)[x]) - G_MININT16;
                              else
                                pixel_val = GUINT16_FROM_LE (((guint16 *) in)[x]);
                            }
                        }
                      else /* if (bpc == 4) */
                        {
                          g_return_if_fail (bpc == 4);

                          if (is_float)
                            {
                              float_val = ((gfloat *) in)[x];
                            }
                          else if (is_big_endian)
                            {
                              if (is_signed)
                                pixel_val = GINT32_FROM_BE (((guint32 *) in)[x]) - G_MININT32;
                              else
                                pixel_val = GUINT32_FROM_BE (((guint32 *) in)[x]);
                            }
                          else
                            {
                              if (is_signed)
                                pixel_val = GINT32_FROM_LE (((guint32 *) in)[x]) - G_MININT32;
                              else
                                pixel_val = GUINT32_FROM_LE (((guint32 *) in)[x]);
                            }
                        }

                      if (is_float)
                        row[pos] = float_val * 255;
                      else
                        row[pos] = pixel_val / pow (2, (bpc - 1) * 8);
                    }
                }

              gimp_preview_area_draw (preview, 0, y, width, 1,
                                      preview_type, row, width * n_components);
            }

          g_free (in);
          g_free (row);
        }
      break;

    case RAW_RGB565:
    case RAW_BGR565:
      /* RGB565 image, big/little endian */
      {
        guint16 *in  = g_malloc0 (width * 2);
        guchar  *row = g_malloc0 (width * 3);

        for (y = 0; y < height; y++)
          {
            pos = offset + width * y * 2;
            mmap_read (preview_fd, in, width * 2, pos, width * 2);
            rgb_565_to_888 (in, row, width, pixel_format, endianness);

            gimp_preview_area_draw (preview, 0, y, width, 1,
                                    GIMP_RGB_IMAGE, row, width * 3);
          }

        g_free (row);
        g_free (in);
      }
      break;

    case RAW_GRAY_1BPP:
      if (! bitspp) bitspp = 1;
    case RAW_GRAY_2BPP:
      if (! bitspp) bitspp = 2;
    case RAW_GRAY_4BPP:
      if (! bitspp) bitspp = 4;
    case RAW_GRAY_8BPP:
      if (! bitspp) bitspp = 8;
      {
        guint   in_size  = height * width / (8 / bitspp);
        guint   out_size = height * width * 3;
        guchar *in_raw  = g_malloc0 (in_size);
        guchar *out_raw = g_malloc0 (out_size);
        guchar  pixel_mask_hi;
        guchar  pixel_mask_lo;
        gint    i;

        /* calculate a pixel_mask_hi
           0x80 for 1 bitspp
           0xc0 for 2 bitspp
           0xf0 for 4 bitspp
           0xff for 8 bitspp
           and a pixel_mask_lo
           0x01 for 1 bitspp
           0x03 for 2 bitspp
           0x0f for 4 bitspp
           0xff for 8 bitspp
         */
        pixel_mask_hi = 0x80;
        pixel_mask_lo = 0x01;

        for (i = 1; i < bitspp; i++)
          {
            pixel_mask_hi |= pixel_mask_hi >> 1;
            pixel_mask_lo |= pixel_mask_lo << 1;
          }

        mmap_read (preview_fd, in_raw, in_size, offset, in_size);

        x = 0; /* walks though all output pixels */
        for (i = 0; i < in_size; i++)
          {
            guchar bit;

            for (bit = 0; bit < 8 / bitspp; bit++)
              {
                guchar pixel_val;

                pixel_val = in_raw[i] & (pixel_mask_hi >> (bit * bitspp));
                pixel_val >>= 8 - bitspp - bit * bitspp;
                pixel_val *= 0xff / pixel_mask_lo;

                out_raw[3 * x + 0] = pixel_val;
                out_raw[3 * x + 1] = pixel_val;
                out_raw[3 * x + 2] = pixel_val;

                x++;
              }
          }

        gimp_preview_area_draw (preview, 0, 0, width, height,
                                GIMP_RGB_IMAGE, out_raw, width * 3);
        g_free (in_raw);
        g_free (out_raw);
      }
      break;

    case RAW_INDEXED:
    case RAW_INDEXEDA:
      /* indexed image */
      {
        gboolean  alpha = (pixel_format == RAW_INDEXEDA);
        guchar   *index = g_malloc0 (width * (alpha ? 2 : 1));
        guchar   *row   = g_malloc0 (width * (alpha ? 4 : 3));

        if (preview_cmap_update)
          {
            if (palette_file)
              {
                gint fd;

                fd = g_open (g_file_peek_path (palette_file), O_RDONLY, 0);

                lseek (fd, palette_offset, SEEK_SET);
                read (fd, preview_cmap,
                      (palette_type == RAW_PALETTE_RGB) ? 768 : 1024);
                close (fd);
              }
            else
              {
                /* make fake palette, maybe overwrite it later */
                for (y = 0, x = 0; y < 256; y++)
                  {
                    preview_cmap[x++] = y;
                    preview_cmap[x++] = y;

                    if (palette_type == RAW_PALETTE_RGB)
                      {
                        preview_cmap[x++] = y;
                      }
                    else
                      {
                        preview_cmap[x++] = y;
                        preview_cmap[x++] = 0;
                      }
                  }
              }
          }

        for (y = 0; y < height; y++)
          {
            guchar *p = row;

            if (alpha)
              {
                pos = offset + width * 2 * y;
                mmap_read (preview_fd, index, width * 2, pos, width);

                for (x = 0; x < width; x++)
                  {
                    switch (palette_type)
                      {
                      case RAW_PALETTE_RGB:
                        *p++ = preview_cmap[index[2 * x] * 3 + 0];
                        *p++ = preview_cmap[index[2 * x] * 3 + 1];
                        *p++ = preview_cmap[index[2 * x] * 3 + 2];
                        *p++ = index[2 * x + 1];
                        break;
                      case RAW_PALETTE_BGR:
                        *p++ = preview_cmap[index[2 * x] * 4 + 2];
                        *p++ = preview_cmap[index[2 * x] * 4 + 1];
                        *p++ = preview_cmap[index[2 * x] * 4 + 0];
                        *p++ = index[2 * x + 1];
                        break;
                      }
                  }

                gimp_preview_area_draw (preview, 0, y, width, 1,
                                        GIMP_RGBA_IMAGE, row, width * 4);
              }
            else
              {
                pos = offset + width * y;
                mmap_read (preview_fd, index, width, pos, width);

                for (x = 0; x < width; x++)
                  {
                    switch (palette_type)
                      {
                      case RAW_PALETTE_RGB:
                        *p++ = preview_cmap[index[x] * 3 + 0];
                        *p++ = preview_cmap[index[x] * 3 + 1];
                        *p++ = preview_cmap[index[x] * 3 + 2];
                        break;
                      case RAW_PALETTE_BGR:
                        *p++ = preview_cmap[index[x] * 4 + 2];
                        *p++ = preview_cmap[index[x] * 4 + 1];
                        *p++ = preview_cmap[index[x] * 4 + 0];
                        break;
                      }
                  }

                gimp_preview_area_draw (preview, 0, y, width, 1,
                                        GIMP_RGB_IMAGE, row, width * 3);
              }
          }

        g_free (row);
        g_free (index);
      }
      break;
    }

  g_clear_object (&palette_file);
}

static void
preview_update_size (GimpPreviewArea *preview)
{
  GObject *config;
  gint     width;
  gint     height;

  config = g_object_get_data (G_OBJECT (preview), "procedure-config");

  if (g_object_class_find_property (G_OBJECT_GET_CLASS (config), "width") != NULL)
    {
      g_object_get (config,
                    "width",  &width,
                    "height", &height,
                    NULL);
    }
  else
    {
      gint sample_spacing;

      sample_spacing = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                            "sample-spacing");

      if (sample_spacing == HGT_SRTM_3)
        width = height = 1201;
      else
        width = height = 3601;

    }
  gtk_widget_set_size_request (GTK_WIDGET (preview), width, height);
}

static void
load_config_notify (GimpProcedureConfig  *config,
                    GParamSpec           *pspec,
                    GimpPreviewArea      *preview)
{
  gboolean preview_cmap_update = FALSE;
  gboolean width_update        = FALSE;
  gboolean height_update       = FALSE;
  gboolean offset_update       = FALSE;

  if (g_str_has_prefix (pspec->name, "palette-"))
    preview_cmap_update = TRUE;

  if ((width_update  = (g_strcmp0 (pspec->name, "width") == 0))  ||
      (height_update = (g_strcmp0 (pspec->name, "height") == 0)) ||
      (offset_update = (g_strcmp0 (pspec->name, "offset") == 0)))
    {
      GFile   *file;
      goffset  file_size;
      gint     width;
      gint     height;
      gint     offset;
      gint     bpp    = 1;
      gint     bitspp = 8;
      goffset  max_pixels;

      get_bpp (config, &bpp, &bitspp);
      g_object_get (config,
                    "width",  &width,
                    "height", &height,
                    "offset", &offset,
                    NULL);

      file = g_object_get_data (G_OBJECT (preview), "procedure-file");
      file_size = get_file_info (file);

      if (bitspp >= 8)
        {
          max_pixels = (file_size - offset) / (bpp * bitspp / 8);
        }
      else
        {
          if (bpp != 1)
            g_printerr ("Unexpected value of bpp: %d, should be 1!", bpp);
          max_pixels = ((file_size - offset) * 8) / bitspp;
        }

      if ((goffset) width * height > max_pixels)
        {
          g_signal_handlers_block_by_func (config,
                                           G_CALLBACK (load_config_notify),
                                           preview);
          if (width_update && width >= max_pixels)
            {
              g_object_set (config,
                            "width",  (gint) max_pixels,
                            "height", 1,
                            NULL);
            }
          else if (height_update && height >= max_pixels)
            {
              g_object_set (config,
                            "width",  1,
                            "height", (gint) max_pixels,
                            NULL);
            }
          else if (width_update || offset_update)
            {
              height = MAX (max_pixels / width, 1);
              g_object_set (config,
                            "height", height,
                            NULL);
            }
          else /* height_update */
            {
              width = MAX (max_pixels / height, 1);
              g_object_set (config,
                            "width", width,
                            NULL);
            }
          g_signal_handlers_unblock_by_func (config,
                                             G_CALLBACK (load_config_notify),
                                             preview);
        }
    }
  preview_update (preview, preview_cmap_update);
}

static void
preview_allocate (GimpPreviewArea   *preview,
                  GtkAllocation     *allocation,
                  gpointer           user_data)
{
  preview_update (preview, FALSE);
}

static gboolean
load_dialog (GFile         *file,
             GimpProcedure *procedure,
             GObject       *config,
             gboolean       is_hgt)
{
  GtkWidget *dialog;
  GtkWidget *preview;
  GtkWidget *sw;
  GtkWidget *viewport;
  GtkWidget *frame;
  gboolean   run;
  gint       sample_spacing;
  gint       width  = 0;
  gint       height = 0;

  if (is_hgt)
    sample_spacing = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                          "sample-spacing");
  else
    g_object_get (config,
                  "width",  &width,
                  "height", &height,
                  NULL);

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (GIMP_PROCEDURE (procedure),
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Load Image from Raw Data"));

  /* Preview frame. */

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (sw, PREVIEW_SIZE, PREVIEW_SIZE);
  gtk_widget_show (sw);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (sw), viewport);
  gtk_widget_show (viewport);

  preview = gimp_preview_area_new ();
  if (is_hgt)
    gtk_widget_set_size_request (preview,
                                 sample_spacing == HGT_SRTM_3 ? 1201 : 3601,
                                 sample_spacing == HGT_SRTM_3 ? 1201 : 3601);
  else
    gtk_widget_set_size_request (preview, width, height);

  gtk_container_add (GTK_CONTAINER (viewport), preview);
  gtk_widget_show (preview);

  g_object_set_data (G_OBJECT (preview), "procedure-config",
                     config);
  g_object_set_data (G_OBJECT (preview), "procedure-file",
                     file);

  g_signal_connect_after (preview, "size-allocate",
                          G_CALLBACK (preview_allocate),
                          NULL);

  frame = gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                            "preview-frame", NULL, FALSE,
                                            NULL);
  gtk_container_add (GTK_CONTAINER (frame), sw);

  /* Image frame. */

  if (is_hgt && sample_spacing == HGT_SRTM_AUTO_DETECT)
    {
      /* When auto-detection of the HGT variant failed, let's just
       * default to SRTM-3 and show a dropdown list.
       */
      g_object_set (config,
                    "sample-spacing", "srtm-3",
                    NULL);

      /* 2 types of HGT files are possible: SRTM-1 and SRTM-3.
       * From the documentation: https://dds.cr.usgs.gov/srtm/version1/Documentation/SRTM_Topo.txt
       * "SRTM-1 data are sampled at one arc-second of latitude and longitude and
       *  each file contains 3601 lines and 3601 samples.
       * [...]
       * SRTM-3 data are sampled at three arc-seconds and contain 1201 lines and
       * 1201 samples with similar overlapping rows and columns."
       */
    }

  if (is_hgt)
    {
      gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                      "image-box", "sample-spacing", NULL);
    }
  else
    {
      GtkWidget *entry;
      goffset    file_size;

      file_size = get_file_info (file);

      entry = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                     "offset", 1.0);
      gimp_scale_entry_set_bounds (GIMP_SCALE_ENTRY (entry), 0, file_size, FALSE);
      entry = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                     "width", 1.0);
      gimp_scale_entry_set_bounds (GIMP_SCALE_ENTRY (entry), 1, file_size, FALSE);
      entry = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                     "height", 1.0);
      gimp_scale_entry_set_bounds (GIMP_SCALE_ENTRY (entry), 1, file_size, FALSE);

      gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                      "image-box",
                                      "pixel-format",
                                      "data-type", "endianness",
                                      "planar-configuration",
                                      "offset", "width", "height",
                                      NULL);
    }

  frame = gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                            "image-frame", NULL, FALSE,
                                            "image-box");
  if (is_hgt)
    {
      if (sample_spacing == HGT_SRTM_3)
        gtk_frame_set_label (GTK_FRAME (frame),
                             /* Translators: Digital Elevation Model (DEM) is a technical term
                              * used for 3D surface modeling or relief maps; so it must be
                              * translated by the proper technical term in your language.
                              */
                             _("Digital Elevation Model data (1 arc-second)"));
      else if (sample_spacing == HGT_SRTM_1)
        gtk_frame_set_label (GTK_FRAME (frame),
                             _("Digital Elevation Model data (3 arc-seconds)"));
      else
        gtk_frame_set_label (GTK_FRAME (frame),
                             _("Digital Elevation Model data"));
    }
  else
    {
      gtk_frame_set_label (GTK_FRAME (frame), _("Image"));
    }

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "palette-box",
                                  "palette-offset",
                                  "palette-type",
                                  "palette-file",
                                  NULL);
  frame = gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                            "palette-frame", NULL, FALSE,
                                            "palette-box");
  gtk_frame_set_label (GTK_FRAME (frame), _("Palette"));

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "preview-frame",
                              "image-frame",
                              "palette-frame",
                              NULL);

  g_signal_connect_swapped (config, "notify::width",
                            G_CALLBACK (preview_update_size),
                            preview);
  g_signal_connect_swapped (config, "notify::height",
                            G_CALLBACK (preview_update_size),
                            preview);
  g_signal_connect_after (config, "notify",
                          G_CALLBACK (load_config_notify),
                          preview);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static gboolean
save_dialog (GimpImage     *image,
             GimpProcedure *procedure,
             gboolean       has_alpha,
             GObject       *config)
{
  GtkWidget    *dialog;
  const gchar  *contiguous_sample = NULL;
  const gchar  *planar_sample     = NULL;
  gchar        *contiguous_label;
  gchar        *planar_label;
  gboolean      run;

  gimp_ui_init (PLUG_IN_BINARY);

  switch (gimp_image_get_base_type (image))
    {
    case GIMP_RGB:
    case GIMP_INDEXED:
      if (has_alpha)
        {
          contiguous_sample = "RGBA,RGBA,RGBA";
          planar_sample     = "RRR,GGG,BBB,AAA";
        }
      else
        {
          contiguous_sample = "RGB,RGB,RGB";
          planar_sample     = "RRR,GGG,BBB";
        }
      break;
    case GIMP_GRAY:
      if (has_alpha)
        {
          contiguous_sample = "YA,YA,YA";
          planar_sample     = "YYY,AAA";
        }
      break;
    default:
      break;
    }

  if (contiguous_sample)
    /* TRANSLATORS: %s is a sample to describe the planar configuration
     * (e.g. RGB,RGB,RGB vs RRR,GGG,BBB).
     */
    contiguous_label = g_strdup_printf (_("_Contiguous (%s)"), contiguous_sample);
  else
    contiguous_label = g_strdup (_("_Contiguous"));

  if (planar_sample)
    /* TRANSLATORS: %s is a sample to describe the planar configuration
     * (e.g. RGB,RGB,RGB vs RRR,GGG,BBB).
     */
    planar_label = g_strdup_printf (_("_Planar (%s)"), planar_sample);
  else
    planar_label = g_strdup (_("_Planar"));

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  /* Image type combo */
  /* No need to give a choice for 1-channel cases where both contiguous
   * and planar are the same.
   */
  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "planar-configuration",
                                       contiguous_sample != NULL,
                                       NULL, NULL, FALSE);

  /* Palette type combo */

  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "palette-type",
                                       gimp_image_get_base_type (image) == GIMP_INDEXED,
                                       NULL, NULL, FALSE);
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), NULL);
  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  g_free (contiguous_label);
  g_free (planar_label);

  return run;
}
