/* Raw data image loader (and saver) plugin 3.4
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
#define SAVE_PROC      "file-raw-save"
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
  RAW_RGB,          /* RGB Image */
  RAW_RGBA,         /* RGB Image with an Alpha channel */
  RAW_RGB565_BE,    /* RGB Image 16bit, 5,6,5 bits per channel, big-endian */
  RAW_RGB565_LE,    /* RGB Image 16bit, 5,6,5 bits per channel, little-endian */
  RAW_BGR565_BE,    /* RGB Image 16bit, 5,6,5 bits per channel, big-endian, red and blue swapped */
  RAW_BGR565_LE,    /* RGB Image 16bit, 5,6,5 bits per channel, little-endian, red and blue swapped */
  RAW_PLANAR,       /* Planar RGB */
  RAW_GRAY_1BPP,
  RAW_GRAY_2BPP,
  RAW_GRAY_4BPP,
  RAW_GRAY_8BPP,
  RAW_INDEXED,      /* Indexed image */
  RAW_INDEXEDA,     /* Indexed image with an Alpha channel */

  RAW_GRAY_16BPP_BE,
  RAW_GRAY_16BPP_LE,
  RAW_GRAY_16BPP_SBE,
  RAW_GRAY_16BPP_SLE,

  RAW_GRAY_32BPP_BE,
  RAW_GRAY_32BPP_LE,
  RAW_GRAY_32BPP_SBE,
  RAW_GRAY_32BPP_SLE,
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
#define RAW (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RAW_TYPE, Raw))

GType                   raw_get_type         (void) G_GNUC_CONST;

static GList          * raw_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * raw_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * raw_load             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static GimpValueArray * raw_save             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              gint                  n_drawables,
                                              GimpDrawable        **drawables,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);

/* prototypes for the new load functions */
static gboolean         raw_load_standard    (RawGimpData          *data,
                                              gint                  width,
                                              gint                  height,
                                              gint                  offset,
                                              gint                  bpp);
static gboolean         raw_load_gray_8_plus (RawGimpData          *data,
                                              gint                  width,
                                              gint                  height,
                                              gint                  bpp,
                                              gint                  offset,
                                              RawType               type);
static gboolean         raw_load_gray        (RawGimpData          *data,
                                              gint                  width,
                                              gint                  height,
                                              gint                  offset,
                                              gint                  bpp,
                                              gint                  bitspp);
static gboolean         raw_load_rgb565      (RawGimpData          *data,
                                              gint                  width,
                                              gint                  height,
                                              gint                  offset,
                                              RawType               type);
static gboolean         raw_load_planar      (RawGimpData          *data,
                                              gint                  width,
                                              gint                  height,
                                              gint                  offset);
static gboolean         raw_load_palette     (RawGimpData          *data,
                                              gint                  palette_offset,
                                              RawPaletteType        palette_type,
                                              GFile                *palette_file);

/* support functions */
static goffset          get_file_info        (GFile                *file);
static void             raw_read_row         (FILE                 *fp,
                                              guchar               *buf,
                                              gint32                offset,
                                              gint32                size);
static int              mmap_read            (gint                  fd,
                                              gpointer              buf,
                                              gint32                len,
                                              gint32                pos,
                                              gint                  rowstride);
static void             rgb_565_to_888       (guint16              *in,
                                              guchar               *out,
                                              gint32                num_pixels,
                                              RawType               type);

static GimpImage      * load_image           (GFile                *file,
                                              GimpProcedureConfig  *config,
                                              GError              **error);
static gboolean         save_image           (GFile                *file,
                                              GimpImage            *image,
                                              GimpDrawable         *drawable,
                                              GimpProcedureConfig  *config,
                                              GError              **error);

static void            get_bpp               (GimpProcedureConfig     *config,
                                              gint                    *bpp,
                                              gint                    *bitspp);
static gboolean        detect_sample_spacing (GimpProcedureConfig     *config,
                                              GFile                   *file,
                                              GError                 **error);
static void           get_load_config_values (GimpProcedureConfig     *config,
                                              gint32                  *file_offset,
                                              gint32                  *image_width,
                                              gint32                  *image_height,
                                              RawType                 *image_type,
                                              gint32                  *palette_offset,
                                              RawPaletteType          *palette_type,
                                              GFile                  **palette_file);

/* gui functions */
static void             preview_update       (GimpPreviewArea         *preview,
                                              gboolean                 preview_cmap_update);
static void             preview_update_size  (GimpPreviewArea         *preview);
static void             load_config_notify   (GimpProcedureConfig     *config,
                                              GParamSpec              *pspec,
                                              GimpPreviewArea         *preview);
static void             preview_allocate     (GimpPreviewArea         *preview,
                                              GtkAllocation           *allocation,
                                              gpointer                 user_data);
static gboolean         load_dialog          (GFile                   *file,
                                              GimpProcedure           *procedure,
                                              GObject                 *config,
                                              gboolean                 is_hgt);
static gboolean         save_dialog          (GimpImage               *image,
                                              GimpProcedure           *procedure,
                                              gboolean                 has_alpha,
                                              GObject                 *config);

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
  list = g_list_append (list, g_strdup (SAVE_PROC));

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

      gimp_procedure_set_menu_label (procedure, N_("Raw image data"));

      gimp_procedure_set_documentation (procedure,
                                        "Load raw images, specifying image "
                                        "information",
                                        "Load raw images, specifying image "
                                        "information",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "timecop, pg@futureware.at",
                                      "timecop, pg@futureware.at",
                                      "Aug 2004");

      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "data");

      /* Properties for image data. */

      GIMP_PROC_ARG_INT (procedure, "width",
                         "_Width",
                         "Image width in number of pixels",
                         1, GIMP_MAX_IMAGE_SIZE, PREVIEW_SIZE,
                         G_PARAM_READWRITE);
      GIMP_PROC_ARG_INT (procedure, "height",
                         "_Height",
                         "Image height in number of pixels",
                         1, GIMP_MAX_IMAGE_SIZE, PREVIEW_SIZE,
                         G_PARAM_READWRITE);
      GIMP_PROC_ARG_INT (procedure, "offset",
                         "O_ffset",
                         "Offset to beginning of image in raw data",
                         0, GIMP_MAX_IMAGE_SIZE, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "pixel-format",
                         "Pixel _format",
                         "How color pixel data are stored { RAW_PLANAR_CONTIGUOUS (0), RAW_PLANAR_SEPARATE (1) }",
                         RAW_RGB, RAW_GRAY_32BPP_SLE, RAW_RGB,
                         G_PARAM_READWRITE);

      /* Properties for palette data. */

      GIMP_PROC_ARG_INT (procedure, "palette-offset",
                         "Pallette Offse_t",
                         "Offset to beginning of data in the palette file",
                         0, GIMP_MAX_IMAGE_SIZE, 0,
                         G_PARAM_READWRITE);
      GIMP_PROC_ARG_INT (procedure, "palette-type",
                         "Palette's la_yout",
                         "The layout for the palette's color channels"
                         "{ RAW_PALETTE_RGB (0), RAW_PALETTE_BGR (1) }",
                         RAW_PALETTE_RGB, RAW_PALETTE_BGR, RAW_PALETTE_RGB,
                         G_PARAM_READWRITE);
      GIMP_PROC_ARG_FILE (procedure, "palette-file",
                          "_Palette File",
                          "The file containing palette data",
                          G_PARAM_READWRITE);
    }
  else if (! strcmp (name, LOAD_HGT_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           raw_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure,
                                     N_("Digital Elevation Model data"));

      gimp_procedure_set_documentation (procedure,
                                        "Load HGT data as images",
                                        "Load Digital Elevation Model data "
                                        "in HGT format from the Shuttle Radar "
                                        "Topography Mission as images. Though "
                                        "the output image will be RGB, all "
                                        "colors are grayscale by default and "
                                        "the contrast will be quite low on "
                                        "most earth relief. Therefore You "
                                        "will likely want to remap elevation "
                                        "to colors as a second step, for "
                                        "instance with the \"Gradient Map\" "
                                        "plug-in.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      NULL, NULL,
                                      "2017-12-09");

      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "hgt");

      GIMP_PROC_ARG_INT (procedure, "sample-spacing",
                         "_Sample spacing",
                         "The sample spacing of the data. "
                         "(0: auto-detect, 1: SRTM-1, 2: SRTM-3 data)",
                         HGT_SRTM_AUTO_DETECT, HGT_SRTM_3, HGT_SRTM_AUTO_DETECT,
                         G_PARAM_READWRITE);
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           raw_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "INDEXED, GRAY, RGB, RGBA");

      gimp_procedure_set_menu_label (procedure, N_("Raw image data"));

      gimp_procedure_set_documentation (procedure,
                                        "Dump images to disk in raw format",
                                        "Dump images to disk in raw format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Björn Kautler, Bjoern@Kautler.net",
                                      "Björn Kautler, Bjoern@Kautler.net",
                                      "April 2014");

      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "data,raw");

      GIMP_PROC_ARG_INT (procedure, "planar-configuration",
                         "Planar configuration",
                         "How color pixel data are stored { RAW_PLANAR_CONTIGUOUS (0), RAW_PLANAR_SEPARATE (1) }",
                         RAW_PLANAR_CONTIGUOUS, RAW_PLANAR_SEPARATE, RAW_PLANAR_CONTIGUOUS,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "palette-type",
                         "Palette's layout",
                         "The layout for the palette's color channels"
                         "{ RAW_PALETTE_RGB (0), RAW_PALETTE_BGR (1) }",
                         RAW_PALETTE_RGB, RAW_PALETTE_BGR, RAW_PALETTE_RGB,
                         G_PARAM_READWRITE);
    }

  gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                       _("Raw Data"));

  return procedure;
}

static GimpValueArray *
raw_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray      *return_vals;
  GimpProcedureConfig *config;
  gboolean             is_hgt;
  GimpPDBStatusType    status = GIMP_PDB_SUCCESS;
  GimpImage           *image  = NULL;
  GError              *error  = NULL;

  gegl_init (NULL, NULL);

  config = gimp_procedure_create_config (procedure);
  gimp_procedure_config_begin_run (config, image, run_mode, args);

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

      g_object_get (config,
                    "sample-spacing", &sample_spacing,
                    NULL);

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

  gimp_procedure_config_end_run (config, status);
  g_object_unref (config);

  if (! image)
    return gimp_procedure_new_return_values (procedure, status, error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
raw_save (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          gint                  n_drawables,
          GimpDrawable        **drawables,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpProcedureConfig    *config;
  GimpPDBStatusType       status = GIMP_PDB_SUCCESS;
  GimpExportReturn        export = GIMP_EXPORT_CANCEL;
  RawPlanarConfiguration  planar_conf;
  GError                 *error  = NULL;

  gegl_init (NULL, NULL);

  config = gimp_procedure_create_config (procedure);
  gimp_procedure_config_begin_export (config, image, run_mode, args, NULL);

  g_object_get (config,
                "planar-configuration", &planar_conf,
                NULL);

  if ((planar_conf != RAW_PLANAR_CONTIGUOUS) && (planar_conf != RAW_PLANAR_SEPARATE))
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               NULL);
    }

  export = gimp_export_image (&image, &n_drawables, &drawables, "RAW",
                              GIMP_EXPORT_CAN_HANDLE_RGB     |
                              GIMP_EXPORT_CAN_HANDLE_GRAY    |
                              GIMP_EXPORT_CAN_HANDLE_INDEXED |
                              GIMP_EXPORT_CAN_HANDLE_ALPHA);

  if (export == GIMP_EXPORT_CANCEL)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("RAW export does not support multiple layers."));

      if (export == GIMP_EXPORT_EXPORT)
        {
          gimp_image_delete (image);
          g_free (drawables);
        }

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! save_dialog (image, procedure,
                         gimp_drawable_has_alpha (drawables[0]),
                         G_OBJECT (config)))
        status = GIMP_PDB_CANCEL;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! save_image (file, image, drawables[0], config, &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  gimp_procedure_config_end_export (config, image, file, status);
  g_object_unref (config);

  if (export == GIMP_EXPORT_EXPORT)
    {
      gimp_image_delete (image);
      g_free (drawables);
    }

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
      size = g_file_info_get_size (info);

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

/* this handles 1, 2, 3, 4 bpp "standard" images */
static gboolean
raw_load_standard (RawGimpData *data,
                   gint         width,
                   gint         height,
                   gint         offset,
                   gint         bpp)
{
  guchar *row = NULL;

  row = g_try_malloc (width * height * bpp);
  if (! row)
    return FALSE;

  raw_read_row (data->fp, row, offset, width * height * bpp);

  gegl_buffer_set (data->buffer, GEGL_RECTANGLE (0, 0, width, height),
                   0, NULL, row, GEGL_AUTO_ROWSTRIDE);

  g_free (row);

  return TRUE;
}

static gboolean
raw_load_gray_8_plus (RawGimpData *data,
                      gint         width,
                      gint         height,
                      gint         bpp,
                      gint         offset,
                      RawType      type)
{
  GeglBufferIterator *iter;
  guchar             *in = NULL;
  gint                input_stride;

  input_stride = width * bpp;

  iter = gegl_buffer_iterator_new (data->buffer, GEGL_RECTANGLE (0, 0, width, height),
                                   0, NULL, GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      const GeglRectangle *roi = &iter->items[0].roi;
      guchar              *out = iter->items[0].data;
      gint                 line;
      gint                 in_size;

      in_size  = roi->width * bpp;
      in = g_realloc (in, in_size);
      for (line = 0; line < roi->height; line++)
        {
          raw_read_row (data->fp, in,
                        offset + ((roi->y + line) * input_stride) + roi->x * bpp,
                        in_size);
          for (gint x = 0; x < roi->width; x++)
            {
              gint pixel_val;

              if (type == RAW_GRAY_8BPP)
                pixel_val = (gint) in[x];
              else if (type == RAW_GRAY_16BPP_BE)
                pixel_val = GUINT16_FROM_BE (((guint16 *) in)[x]);
              else if (type == RAW_GRAY_16BPP_LE)
                pixel_val = GUINT16_FROM_LE (((guint16 *) in)[x]);
              else if (type == RAW_GRAY_16BPP_SBE)
                pixel_val = GINT16_FROM_BE (((guint16 *) in)[x]) - G_MININT16;
              else if (type == RAW_GRAY_16BPP_SLE)
                pixel_val = GINT16_FROM_LE (((guint16 *) in)[x]) - G_MININT16;
              else if (type == RAW_GRAY_32BPP_BE)
                pixel_val = GUINT32_FROM_BE (((guint32 *) in)[x]);
              else if (type == RAW_GRAY_32BPP_LE)
                pixel_val = GUINT32_FROM_LE (((guint32 *) in)[x]);
              else if (type == RAW_GRAY_32BPP_SBE)
                pixel_val = GINT32_FROM_BE (((guint32 *) in)[x]) - G_MININT32;
              else /* if (type == RAW_GRAY_32BPP_SLE) */
                pixel_val = GINT32_FROM_LE (((guint32 *) in)[x]) - G_MININT32;

              if (bpp == 4)
                ((guint32*) out)[x] = (guint32) pixel_val;
              else if (bpp == 2)
                ((guint16*) out)[x] = (guint16) pixel_val;
              else
                out[x] = (gchar) pixel_val;
            }
          out += in_size;
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
raw_load_rgb565 (RawGimpData *data,
                 gint         width,
                 gint         height,
                 gint         offset,
                 RawType      type)
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
          rgb_565_to_888 (in, out + line * roi->width * 3, roi->width, type);
        }
    }

  g_free (in);

  return TRUE;
}

/* this converts a 2bpp buffer to a 3bpp buffer in is a buffer of
 * 16bit pixels, out is a buffer of 24bit pixels
 */
static void
rgb_565_to_888 (guint16 *in,
                guchar  *out,
                gint32   num_pixels,
                RawType  type)
{
  guint32 i, j;
  guint16 input;
  gboolean swap_endian;

  if (G_BYTE_ORDER == G_LITTLE_ENDIAN || G_BYTE_ORDER == G_PDP_ENDIAN)
    {
      swap_endian = (type == RAW_RGB565_BE || type == RAW_BGR565_BE);
    }
  else if (G_BYTE_ORDER == G_BIG_ENDIAN)
    {
      swap_endian = (type == RAW_RGB565_LE || type == RAW_BGR565_LE);
    }

  switch (type)
    {
    case RAW_RGB565_LE:
    case RAW_RGB565_BE:
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

    case RAW_BGR565_BE:
    case RAW_BGR565_LE:
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

/* this handles 3 bpp "planar" images */
static gboolean
raw_load_planar (RawGimpData *data,
                 gint         width,
                 gint         height,
                 gint         offset)
{
  gint32  r_offset, g_offset, b_offset, i, j, k;
  guchar *r_row, *b_row, *g_row, *row;
  gint    bpp = 3; /* adding support for alpha channel should be easy */

  /* red, green, blue rows temporary data */
  r_row = g_malloc (width);
  g_row = g_malloc (width);
  b_row = g_malloc (width);

  /* row for the pixel region, after combining RGB together */
  row = g_malloc (width * bpp);

  r_offset = offset;
  g_offset = r_offset + width * height;
  b_offset = g_offset + width * height;

  for (i = 0; i < height; i++)
    {
      /* Read R, G, B rows */
      raw_read_row (data->fp, r_row, r_offset + (width * i), width);
      raw_read_row (data->fp, g_row, g_offset + (width * i), width);
      raw_read_row (data->fp, b_row, b_offset + (width * i), width);

      /* Combine separate R, G and B rows into RGB triples */
      for (j = 0, k = 0; j < width; j++)
        {
          row[k++] = r_row[j];
          row[k++] = g_row[j];
          row[k++] = b_row[j];
        }

      gegl_buffer_set (data->buffer, GEGL_RECTANGLE (0, i, width, 1),
                       0, NULL, row, GEGL_AUTO_ROWSTRIDE);

      gimp_progress_update ((gfloat) i / (gfloat) height);
    }

  gimp_progress_update (1.0);

  g_free (row);
  g_free (r_row);
  g_free (g_row);
  g_free (b_row);

  return TRUE;
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
save_image (GFile                *file,
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
  FILE                   *fp;
  gint                    i, j, c;
  gint                    palsize = 0;
  RawPlanarConfiguration  planar_conf;    /* Planar Configuration (CONTIGUOUS, PLANAR) */
  RawPaletteType          palette_type;   /* type of palette (RGB/BGR)   */
  gboolean                ret = FALSE;

  g_object_get (config,
                "planar-configuration", &planar_conf,
                "palette-type",         &palette_type,
                NULL);

  buffer = gimp_drawable_get_buffer (drawable);

  format = gimp_drawable_get_format (drawable);

  n_components = babl_format_get_n_components (format);
  bpp          = babl_format_get_bytes_per_pixel (format);

  if (gimp_drawable_is_indexed (drawable))
    cmap = gimp_image_get_colormap (image, &palsize);

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
        components[c] = g_new (guchar, width * height);

      for (i = 0, j = 0; i < width * height * bpp; i += bpp, j++)
        {
          for (c = 0; c < n_components; c++)
            components[c][j] = buf[i + c];
        }

      ret = TRUE;
      for (c = 0; c < n_components; c++)
        {
          if (! fwrite (components[c], width * height, 1, fp))
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

      g_object_get (config,
                    "pixel-format", &image_type,
                    NULL);
      switch (image_type)
        {
        case RAW_RGB:
        case RAW_PLANAR:
          *bpp = 3;
          break;

        case RAW_RGB565_BE:
        case RAW_RGB565_LE:
        case RAW_BGR565_BE:
        case RAW_BGR565_LE:
          *bpp = 2;
          break;

        case RAW_RGBA:
          *bpp = 4;
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

        case RAW_GRAY_16BPP_BE:
        case RAW_GRAY_16BPP_LE:
        case RAW_GRAY_16BPP_SBE:
        case RAW_GRAY_16BPP_SLE:
          *bpp   = 2;
          break;

        case RAW_GRAY_32BPP_BE:
        case RAW_GRAY_32BPP_LE:
        case RAW_GRAY_32BPP_SBE:
        case RAW_GRAY_32BPP_SLE:
          *bpp   = 4;
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
        sample_spacing = HGT_SRTM_3;
      else if (pos == 3601*3601*2)
        sample_spacing = HGT_SRTM_1;

      g_object_set (config,
                    "sample-spacing", sample_spacing,
                    NULL);

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
                        gint32                  *palette_offset,
                        RawPaletteType          *palette_type,
                        GFile                  **palette_file)
{
  GimpProcedure *procedure;

  procedure = gimp_procedure_config_get_procedure (config);

  if (g_strcmp0 (gimp_procedure_get_name (procedure), LOAD_HGT_PROC) == 0)
    {
      gint sample_spacing;

      g_object_get (config,
                    "sample-spacing", &sample_spacing,
                    NULL);

      if (sample_spacing == HGT_SRTM_3)
        *image_width = *image_height = 1201;
      else
        *image_width = *image_height = 3601;

      *file_offset    = 0;
      *image_type     = RAW_GRAY_16BPP_SBE;
      *palette_offset = 0;
      *palette_type   = RAW_PALETTE_RGB;
      *palette_file   = NULL;
    }
  else
    {
      g_object_get (config,
                    "offset",         file_offset,
                    "width",          image_width,
                    "height",         image_height,
                    "pixel-format",   image_type,
                    "palette-offset", palette_offset,
                    "palette-type",   palette_type,
                    "palette-file",   palette_file,
                    NULL);
    }
}

static GimpImage *
load_image (GFile                *file,
            GimpProcedureConfig  *config,
            GError              **error)
{
  RawGimpData       *data;
  GimpLayer         *layer = NULL;
  GimpImageType      ltype = GIMP_RGB_IMAGE;
  GimpImageBaseType  itype = GIMP_RGB;
  RawType            pixel_format;
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

  get_load_config_values (config, &offset, &width, &height, &pixel_format,
                          &palette_offset, &palette_type, &palette_file);

  size = get_file_info (file);

  switch (pixel_format)
    {
    case RAW_RGB:             /* standard RGB */
    case RAW_PLANAR:          /* planar RGB */
      bpp   = 3;
      ltype = GIMP_RGB_IMAGE;
      itype = GIMP_RGB;
      break;

    case RAW_RGB565_BE:       /* RGB565 big endian */
    case RAW_RGB565_LE:       /* RGB565 little endian */
    case RAW_BGR565_BE:       /* RGB565 big endian */
    case RAW_BGR565_LE:       /* RGB565 little endian */
      bpp   = 2;
      ltype = GIMP_RGB_IMAGE;
      itype = GIMP_RGB;
      break;

    case RAW_RGBA:            /* RGB + alpha */
      bpp   = 4;
      ltype = GIMP_RGBA_IMAGE;
      itype = GIMP_RGB;
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

    case RAW_GRAY_16BPP_BE:
    case RAW_GRAY_16BPP_LE:
    case RAW_GRAY_16BPP_SBE:
    case RAW_GRAY_16BPP_SLE:
      bpp   = 2;
      ltype = GIMP_GRAY_IMAGE;
      itype = GIMP_GRAY;
      break;

    case RAW_GRAY_32BPP_BE:
    case RAW_GRAY_32BPP_LE:
    case RAW_GRAY_32BPP_SBE:
    case RAW_GRAY_32BPP_SLE:
      bpp   = 4;
      ltype = GIMP_GRAY_IMAGE;
      itype = GIMP_GRAY;
      break;
    }

  /* make sure we don't load image bigger than file size */
  if (height > (size / width / bpp * 8 / bitspp))
    height = size / width / bpp * 8 / bitspp;

  if (pixel_format >= RAW_GRAY_32BPP_BE)
    data->image = gimp_image_new_with_precision (width, height, itype,
                                                 GIMP_PRECISION_U32_NON_LINEAR);
  else if (pixel_format >= RAW_GRAY_16BPP_BE)
    data->image = gimp_image_new_with_precision (width, height, itype,
                                                 GIMP_PRECISION_U16_NON_LINEAR);
  else
    data->image = gimp_image_new (width, height, itype);
  gimp_image_set_file (data->image, file);
  layer = gimp_layer_new (data->image, _("Background"),
                          width, height, ltype, 100,
                          gimp_image_get_default_new_layer_mode (data->image));
  gimp_image_insert_layer (data->image, layer, NULL, 0);

  data->buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  switch (pixel_format)
    {
    case RAW_RGB:
    case RAW_RGBA:
      raw_load_standard (data, width, height, offset, bpp);
      break;

    case RAW_RGB565_BE:
    case RAW_RGB565_LE:
    case RAW_BGR565_BE:
    case RAW_BGR565_LE:
      raw_load_rgb565 (data, width, height, offset, pixel_format);
      break;

    case RAW_PLANAR:
      raw_load_planar (data, width, height, offset);
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
      raw_load_standard (data, width, height, offset, bpp);
      break;

    case RAW_GRAY_8BPP:
    case RAW_GRAY_16BPP_BE:
    case RAW_GRAY_16BPP_LE:
    case RAW_GRAY_16BPP_SBE:
    case RAW_GRAY_16BPP_SLE:
    case RAW_GRAY_32BPP_BE:
    case RAW_GRAY_32BPP_LE:
    case RAW_GRAY_32BPP_SBE:
    case RAW_GRAY_32BPP_SLE:
      raw_load_gray_8_plus (data, width, height, bpp, offset, pixel_format);
      break;
    }

  fclose (data->fp);

  g_object_unref (data->buffer);
  g_clear_object (&palette_file);

  return data->image;
}


/* misc GUI stuff */

static void
preview_update (GimpPreviewArea *preview,
                gboolean         preview_cmap_update)
{
  gint                 preview_width;
  gint                 preview_height;
  gint32               pos;
  gint                 x, y;
  gint                 bitspp = 0;

  GimpProcedureConfig *config;
  RawType              pixel_format;
  gint                 width;
  gint                 height;
  gint                 offset;

  GFile               *palette_file;
  gint                 palette_offset;
  RawPaletteType       palette_type;

  printf("START %s\n", G_STRFUNC);
  gimp_preview_area_get_size (preview, &preview_width, &preview_height);

  config = g_object_get_data (G_OBJECT (preview), "procedure-config");

  get_load_config_values (config, &offset, &width, &height, &pixel_format,
                          &palette_offset, &palette_type, &palette_file);
  width  = MIN (width,  preview_width);
  height = MIN (height, preview_height);

  gimp_preview_area_fill (preview,
                          0, 0, preview_width, preview_height,
                          255, 255, 255);

  switch (pixel_format)
    {
    case RAW_RGB:
      /* standard RGB image */
      {
        guchar *row = g_malloc0 (width * 3);

        for (y = 0; y < height; y++)
          {
            pos = offset + width * y * 3;
            mmap_read (preview_fd, row, width * 3, pos, width * 3);

            gimp_preview_area_draw (preview, 0, y, width, 1,
                                    GIMP_RGB_IMAGE, row, width * 3);
          }

        g_free (row);
      }
      break;

    case RAW_RGBA:
      /* RGB + alpha image */
      {
        guchar *row = g_malloc0 (width * 4);

        for (y = 0; y < height; y++)
          {
            pos = offset + width * y * 4;
            mmap_read (preview_fd, row, width * 4, pos, width * 4);

            gimp_preview_area_draw (preview, 0, y, width, 1,
                                    GIMP_RGBA_IMAGE, row, width * 4);
          }

        g_free (row);
      }
      break;

    case RAW_RGB565_BE:
    case RAW_RGB565_LE:
    case RAW_BGR565_BE:
    case RAW_BGR565_LE:
      /* RGB565 image, big/little endian */
      {
        guint16 *in  = g_malloc0 (width * 2);
        guchar  *row = g_malloc0 (width * 3);

        for (y = 0; y < height; y++)
          {
            pos = offset + width * y * 2;
            mmap_read (preview_fd, in, width * 2, pos, width * 2);
            rgb_565_to_888 (in, row, width, pixel_format);

            gimp_preview_area_draw (preview, 0, y, width, 1,
                                    GIMP_RGB_IMAGE, row, width * 3);
          }

        g_free (row);
        g_free (in);
      }
      break;

     case RAW_PLANAR:
      {
        guchar *r_row = g_malloc0 (width);
        guchar *g_row = g_malloc0 (width);
        guchar *b_row = g_malloc0 (width);
        guchar *row   = g_malloc0 (width * 3);

        for (y = 0; y < height; y++)
          {
            gint j, k;

            pos = (offset + (y * width));
            mmap_read (preview_fd, r_row, width, pos, width);

            pos = (offset + (width * (height + y)));
            mmap_read (preview_fd, g_row, width, pos, width);

            pos = (offset + (width * (height * 2 + y)));
            mmap_read (preview_fd, b_row, width, pos, width);

            for (j = 0, k = 0; j < width; j++)
              {
                row[k++] = r_row[j];
                row[k++] = g_row[j];
                row[k++] = b_row[j];
              }

            gimp_preview_area_draw (preview, 0, y, width, 1,
                                    GIMP_RGB_IMAGE, row, width * 3);
          }

        g_free (b_row);
        g_free (g_row);
        g_free (r_row);
        g_free (row);
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

    case RAW_GRAY_16BPP_BE:
    case RAW_GRAY_16BPP_LE:
    case RAW_GRAY_16BPP_SBE:
    case RAW_GRAY_16BPP_SLE:
      bitspp = 16;
    case RAW_GRAY_32BPP_BE:
    case RAW_GRAY_32BPP_LE:
    case RAW_GRAY_32BPP_SBE:
    case RAW_GRAY_32BPP_SLE:
      if (bitspp == 0)
        bitspp = 32;

        {
        guchar *r_row = g_new (guchar, width * bitspp / 8);
        guchar *row   = g_malloc (width);

        for (y = 0; y < height; y++)
          {
            gint j;

            pos = (offset + (y * width * 2));
            mmap_read (preview_fd, (guchar*) r_row, (bitspp / 8) * width, pos, width);

            for (j = 0; j < width; j++)
              {
                gint pixel_val;

                if (pixel_format == RAW_GRAY_16BPP_BE)
                  pixel_val = GUINT16_FROM_BE (((guint16 *) r_row)[j]);
                else if (pixel_format == RAW_GRAY_16BPP_LE)
                  pixel_val = GUINT16_FROM_LE (((guint16 *) r_row)[j]);
                else if (pixel_format == RAW_GRAY_16BPP_SBE)
                  pixel_val = GINT16_FROM_BE (((guint16 *) r_row)[j]) - G_MININT16;
                else if (pixel_format == RAW_GRAY_16BPP_SLE)
                  pixel_val = GINT16_FROM_LE (((guint16 *) r_row)[j]) - G_MININT16;
                else if (pixel_format == RAW_GRAY_32BPP_BE)
                  pixel_val = GUINT32_FROM_BE (((guint32 *) r_row)[j]);
                else if (pixel_format == RAW_GRAY_32BPP_LE)
                  pixel_val = GUINT32_FROM_LE (((guint32 *) r_row)[j]);
                else if (pixel_format == RAW_GRAY_32BPP_SBE)
                  pixel_val = GINT32_FROM_BE (((guint32 *) r_row)[j]) - G_MININT32;
                else /* if (pixel_format == RAW_GRAY_32BPP_SLE)*/
                  pixel_val = GINT32_FROM_LE (((guint32 *) r_row)[j]) - G_MININT32;

                row[j] = pixel_val / pow (2, bitspp - 8);
              }

            gimp_preview_area_draw (preview, 0, y, width, 1,
                                    GIMP_GRAY_IMAGE, row, width * 3);
          }

        g_free (r_row);
        g_free (row);
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

      g_object_get (config,
                    "sample-spacing", &sample_spacing,
                    NULL);

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

      max_pixels = (file_size - offset) / (bpp * bitspp / 8);
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

  GtkListStore *store;

  if (is_hgt)
    g_object_get (config,
                  "sample-spacing", &sample_spacing,
                  NULL);
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
                    "sample-spacing", HGT_SRTM_3,
                    NULL);

      /* 2 types of HGT files are possible: SRTM-1 and SRTM-3.
       * From the documentation: https://dds.cr.usgs.gov/srtm/version1/Documentation/SRTM_Topo.txt
       * "SRTM-1 data are sampled at one arc-second of latitude and longitude and
       *  each file contains 3601 lines and 3601 samples.
       * [...]
       * SRTM-3 data are sampled at three arc-seconds and contain 1201 lines and
       * 1201 samples with similar overlapping rows and columns."
       */
      store = gimp_int_store_new (_("SRTM-1 (1 arc-second)"),  HGT_SRTM_1,
                                  _("SRTM-3 (3 arc-seconds)"), HGT_SRTM_3,
                                  NULL);
      gimp_procedure_dialog_get_int_combo (GIMP_PROCEDURE_DIALOG (dialog),
                                           "sample-spacing",
                                           GIMP_INT_STORE (store));
    }
  else if (! is_hgt)
    {
      /* Generic case for any data. Let's leave choice to select the
       * right type of raw data.
       */
      store = gimp_int_store_new (_("RGB"),                                RAW_RGB,
                                  _("RGB Alpha"),                          RAW_RGBA,
                                  _("RGB565 Big Endian"),                  RAW_RGB565_BE,
                                  _("RGB565 Little Endian"),               RAW_RGB565_LE,
                                  _("BGR565 Big Endian"),                  RAW_BGR565_BE,
                                  _("BGR565 Little Endian"),               RAW_BGR565_LE,
                                  _("Planar RGB"),                         RAW_PLANAR,
                                  _("B&W 1 bit"),                          RAW_GRAY_1BPP,
                                  _("Gray 2 bit"),                         RAW_GRAY_2BPP,
                                  _("Gray 4 bit"),                         RAW_GRAY_4BPP,
                                  _("Gray 8 bit"),                         RAW_GRAY_8BPP,
                                  _("Indexed"),                            RAW_INDEXED,
                                  _("Indexed Alpha"),                      RAW_INDEXEDA,
                                  _("Gray unsigned 16 bit Big Endian"),    RAW_GRAY_16BPP_BE,
                                  _("Gray unsigned 16 bit Little Endian"), RAW_GRAY_16BPP_LE,
                                  _("Gray 16 bit Big Endian"),             RAW_GRAY_16BPP_SBE,
                                  _("Gray 16 bit Little Endian"),          RAW_GRAY_16BPP_SLE,
                                  _("Gray unsigned 32 bit Big Endian"),    RAW_GRAY_32BPP_BE,
                                  _("Gray unsigned 32 bit Little Endian"), RAW_GRAY_32BPP_LE,
                                  _("Gray 32 bit Big Endian"),             RAW_GRAY_32BPP_SBE,
                                  _("Gray 32 bit Little Endian"),          RAW_GRAY_32BPP_SLE,
                                  NULL);
      gimp_procedure_dialog_get_int_combo (GIMP_PROCEDURE_DIALOG (dialog),
                                           "pixel-format",
                                           GIMP_INT_STORE (store));
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

  store = gimp_int_store_new (_("R, G, B (normal)"),       RAW_PALETTE_RGB,
                              _("B, G, R, X (BMP style)"), RAW_PALETTE_BGR,
                              NULL);
  gimp_procedure_dialog_get_int_combo (GIMP_PROCEDURE_DIALOG (dialog),
                                       "palette-type",
                                       GIMP_INT_STORE (store));

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
  GtkListStore *store;
  const gchar  *contiguous_sample = NULL;
  const gchar  *planar_sample = NULL;
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

  dialog = gimp_save_procedure_dialog_new (GIMP_SAVE_PROCEDURE (procedure),
                                           GIMP_PROCEDURE_CONFIG (config),
                                           image);

  /* Image type combo */
  store = gimp_int_store_new (contiguous_label, RAW_PLANAR_CONTIGUOUS,
                              planar_label,     RAW_PLANAR_SEPARATE,
                              NULL);
  gimp_procedure_dialog_get_int_radio (GIMP_PROCEDURE_DIALOG (dialog),
                                       "planar-configuration", GIMP_INT_STORE (store));

  /* No need to give a choice for 1-channel cases where both contiguous
   * and planar are the same.
   */
  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "planar-configuration",
                                       contiguous_sample != NULL,
                                       NULL, NULL, FALSE);

  /* Palette type combo */
  store = gimp_int_store_new (_("_R, G, B (normal)"),       RAW_PALETTE_RGB,
                              _("_B, G, R, X (BMP style)"), RAW_PALETTE_BGR,
                              NULL);
  gimp_procedure_dialog_get_int_radio (GIMP_PROCEDURE_DIALOG (dialog),
                                       "palette-type", GIMP_INT_STORE (store));

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
