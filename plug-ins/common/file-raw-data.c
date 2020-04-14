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
} RawType;

typedef enum
{
  RAW_PALETTE_RGB,  /* standard RGB */
  RAW_PALETTE_BGR   /* Windows BGRX */
} RawPaletteType;

typedef struct
{
  gint32         file_offset;    /* offset to beginning of image in raw data */
  gint32         image_width;    /* width of the raw image                   */
  gint32         image_height;   /* height of the raw image                  */
  RawType        image_type;     /* type of image (RGB, INDEXED, etc)        */
  gint32         palette_offset; /* offset inside the palette file, if any   */
  RawPaletteType palette_type;   /* type of palette (RGB/BGR)                */
} RawConfig;

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
                                              gint                  bpp);
static gboolean         raw_load_gray        (RawGimpData          *data,
                                              gint                  bpp,
                                              gint                  bitspp);
static gboolean         raw_load_rgb565      (RawGimpData          *data,
                                              RawType               type);
static gboolean         raw_load_planar      (RawGimpData          *data);
static gboolean         raw_load_palette     (RawGimpData          *data,
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
                                              GError              **error);
static gboolean         save_image           (GFile                *file,
                                              GimpImage            *image,
                                              GimpDrawable         *drawable,
                                              GObject              *config,
                                              GError              **error);

/* gui functions */
static void             preview_update_size  (GimpPreviewArea      *preview);
static void             preview_update       (GimpPreviewArea      *preview);
static void             palette_update       (GimpPreviewArea      *preview);
static gboolean         load_dialog          (GFile                *file,
                                              gboolean              is_hgt);
static gboolean         save_dialog          (GimpImage            *image,
                                              GimpProcedure        *procedure,
                                              GObject              *config);
static void             palette_callback     (GtkFileChooser       *button,
                                              GimpPreviewArea      *preview);


G_DEFINE_TYPE (Raw, raw, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (RAW_TYPE)


static RawConfig *runtime             = NULL;
static GFile     *palfile             = NULL;
static gint       preview_fd          = -1;
static guchar     preview_cmap[1024];
static gboolean   preview_cmap_update = TRUE;


static void
raw_class_init (RawClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = raw_query_procedures;
  plug_in_class->create_procedure = raw_create_procedure;
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
                         "Sample spacing",
                         "The sample spacing of the data. "
                         "(0: auto-detect, 1: SRTM-1, 2: SRTM-3 data)",
                         0, 2, 0,
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

      GIMP_PROC_ARG_INT (procedure, "image-type",
                         "Image type",
                         "The image type { RAW_RGB (0), RAW_PLANAR (6) }",
                         RAW_RGB, RAW_PLANAR, RAW_RGB,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "palette-type",
                         "Palette type",
                         "The palette type "
                         "{ RAW_PALETTE_RGB (0), RAW_PALETTE_BGR (1) }",
                         RAW_PALETTE_RGB, RAW_PALETTE_BGR, RAW_PALETTE_RGB,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
raw_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray    *return_vals;
  gboolean           is_hgt;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpImage         *image  = NULL;
  GError            *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  is_hgt = (! strcmp (gimp_procedure_get_name (procedure), LOAD_HGT_PROC));

  /* allocate config structure and fill with defaults */
  runtime = g_new0 (RawConfig, 1);

  runtime->file_offset    = 0;
  runtime->palette_offset = 0;
  runtime->palette_type   = RAW_PALETTE_RGB;

  if (is_hgt)
    {
      FILE  *fp;
      glong  pos;
      gint   hgt_size;

      runtime->image_type = RAW_GRAY_16BPP_SBE;

      fp = g_fopen (g_file_get_path (file), "rb");
      if (! fp)
        {
          g_set_error (&error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Could not open '%s' for size verification: %s"),
                       gimp_file_get_utf8_name (file),
                       g_strerror (errno));

          status = GIMP_PDB_EXECUTION_ERROR;
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
              hgt_size = 1201;
            }
          else if (pos == 3601*3601*2)
            {
              hgt_size = 3601;
            }
          else
            {
              /* As a special exception, if the file looks like an HGT
               * format from extension, yet it doesn't have the right
               * size, we will degrade a bit the experience by adding
               * sample spacing choice.
               */
              hgt_size = 0;
            }

          runtime->image_width  = hgt_size;
          runtime->image_height = hgt_size;

          fclose (fp);
        }
    }
  else
    {
      runtime->image_width  = PREVIEW_SIZE;
      runtime->image_height = PREVIEW_SIZE;
      runtime->image_type   = RAW_RGB;
    }

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! is_hgt)
        gimp_get_data (LOAD_PROC, runtime);

      preview_fd = g_open (g_file_get_path (file), O_RDONLY, 0);
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
          if (! load_dialog (file, is_hgt))
            status = GIMP_PDB_CANCEL;

          close (preview_fd);
        }
    }
  else if (is_hgt) /* HGT file in non-interactive mode. */
    {
      gint32 sample_spacing = GIMP_VALUES_GET_INT (args, 0);

      if (sample_spacing != 0 &&
          sample_spacing != 1 &&
          sample_spacing != 3)
        {
          g_set_error (&error,
                       GIMP_PLUGIN_HGT_LOAD_ERROR,
                       GIMP_PLUGIN_HGT_LOAD_ARGUMENT_ERROR,
                       _("%d is not a valid sample spacing. "
                         "Valid values are: 0 (auto-detect), 1 and 3."),
                       sample_spacing);

          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          switch (sample_spacing)
            {
            case 0:
              /* Auto-detection already occurred. Let's just check if
               *it was successful.
               */
              if (runtime->image_width != 1201 &&
                  runtime->image_width != 3601)
                {
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
              break;

            case 1:
              runtime->image_width  = 3601;
              runtime->image_height = 3601;
              break;

            default: /* 3 */
              runtime->image_width  = 1201;
              runtime->image_height = 1201;
              break;
            }
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
    {
      image = load_image (file, &error);

      if (image)
        {
          if (! is_hgt)
            gimp_set_data (LOAD_PROC, runtime, sizeof (RawConfig));
        }
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      g_printerr ("Loading \"%s\" failed with error: %s",
                  gimp_file_get_utf8_name (file),
                  error->message);
    }

  g_free (runtime);

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
  GimpProcedureConfig *config;
  GimpPDBStatusType    status = GIMP_PDB_SUCCESS;
  GimpExportReturn     export = GIMP_EXPORT_CANCEL;
  RawType              image_type;
  GError              *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  config = gimp_procedure_create_config (procedure);
  gimp_procedure_config_begin_export (config, image, run_mode, args, NULL);

  g_object_get (config,
                "image-type", &image_type,
                NULL);

  if ((image_type != RAW_RGB) && (image_type != RAW_PLANAR))
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

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! save_dialog (image, procedure, G_OBJECT (config)))
        status = GIMP_PDB_CANCEL;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! save_image (file, image, drawables[0], G_OBJECT (config),
                        &error))
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
  fseek (fp, offset, SEEK_SET);

  if (! fread (buf, size, 1, fp))
    {
      g_printerr ("fread failed\n");
      memset (buf, 0xFF, size);
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
                   gint         bpp)
{
  guchar *row = NULL;

  row = g_try_malloc (runtime->image_width * runtime->image_height * bpp);
  if (! row)
    return FALSE;

  raw_read_row (data->fp, row, runtime->file_offset,
                runtime->image_width * runtime->image_height * bpp);

  gegl_buffer_set (data->buffer, GEGL_RECTANGLE (0, 0,
                                                 runtime->image_width,
                                                 runtime->image_height), 0,
                   NULL, row, GEGL_AUTO_ROWSTRIDE);

  g_free (row);

  return TRUE;
}

static gboolean
raw_load_gray16 (RawGimpData *data,
                 RawType      type)
{
  guint16 *in_raw = NULL;
  guint16 *out_raw = NULL;
  gsize    in_size;
  gsize    out_size;
  gsize    i;

  in_size  = runtime->image_width * runtime->image_height;
  out_size = runtime->image_width * runtime->image_height * 3 * sizeof (*out_raw);

  in_raw = g_try_malloc (in_size * sizeof (*in_raw));
  if (! in_raw)
    return FALSE;

  out_raw = g_try_malloc0 (out_size);
  if (! out_raw)
    {
      g_free (in_raw);
      return FALSE;
    }

  raw_read_row (data->fp, (guchar*) in_raw, runtime->file_offset,
                in_size * sizeof (*in_raw));

  for (i = 0; i < in_size; i++)
    {
      gint pixel_val;

      if (type == RAW_GRAY_16BPP_BE)
        pixel_val = GUINT16_FROM_BE (in_raw[i]);
      else if (type == RAW_GRAY_16BPP_LE)
        pixel_val = GUINT16_FROM_LE (in_raw[i]);
      else if (type == RAW_GRAY_16BPP_SBE)
        pixel_val = GINT16_FROM_BE (in_raw[i]) - G_MININT16;
      else /* if (type == RAW_GRAY_16BPP_SLE)*/
        pixel_val = GINT16_FROM_LE (in_raw[i]) - G_MININT16;

      out_raw[3 * i + 0] = pixel_val;
      out_raw[3 * i + 1] = pixel_val;
      out_raw[3 * i + 2] = pixel_val;
    }

  gegl_buffer_set (data->buffer, GEGL_RECTANGLE (0, 0,
                                                 runtime->image_width,
                                                 runtime->image_height), 0,
                   NULL, out_raw, GEGL_AUTO_ROWSTRIDE);

  g_free (in_raw);
  g_free (out_raw);

  return TRUE;
}

/* this handles black and white, gray with 1, 2, 4, and 8 _bits_ per
 * pixel images - hopefully lots of binaries too
 */
static gboolean
raw_load_gray (RawGimpData *data,
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

  in_size  = runtime->image_width * runtime->image_height / (8 / bitspp);
  out_size = runtime->image_width * runtime->image_height * 3;

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

  raw_read_row (data->fp, in_raw, runtime->file_offset,
                in_size);

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

  gegl_buffer_set (data->buffer, GEGL_RECTANGLE (0, 0,
                                                 runtime->image_width,
                                                 runtime->image_height), 0,
                   NULL, out_raw, GEGL_AUTO_ROWSTRIDE);

  g_free (in_raw);
  g_free (out_raw);

  return TRUE;
}

/* this handles RGB565 images */
static gboolean
raw_load_rgb565 (RawGimpData *data,
                 RawType      type)
{
  gint32   num_pixels = runtime->image_width * runtime->image_height;
  guint16 *in         = g_malloc (num_pixels * 2);
  guchar  *row        = g_malloc (num_pixels * 3);

  raw_read_row (data->fp, (guchar *)in, runtime->file_offset, num_pixels * 2);
  rgb_565_to_888 (in, row, num_pixels, type);

  gegl_buffer_set (data->buffer, GEGL_RECTANGLE (0, 0,
                                                 runtime->image_width,
                                                 runtime->image_height), 0,
                   NULL, row, GEGL_AUTO_ROWSTRIDE);

  g_free (in);
  g_free (row);

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
      /*This conversion function does not handle the passed in image-type*/
      g_assert_not_reached ();
    }
}

/* this handles 3 bpp "planar" images */
static gboolean
raw_load_planar (RawGimpData *data)
{
  gint32  r_offset, g_offset, b_offset, i, j, k;
  guchar *r_row, *b_row, *g_row, *row;
  gint    bpp = 3; /* adding support for alpha channel should be easy */

  /* red, green, blue rows temporary data */
  r_row = g_malloc (runtime->image_width);
  g_row = g_malloc (runtime->image_width);
  b_row = g_malloc (runtime->image_width);

  /* row for the pixel region, after combining RGB together */
  row = g_malloc (runtime->image_width * bpp);

  r_offset = runtime->file_offset;
  g_offset = r_offset + runtime->image_width * runtime->image_height;
  b_offset = g_offset + runtime->image_width * runtime->image_height;

  for (i = 0; i < runtime->image_height; i++)
    {
      /* Read R, G, B rows */
      raw_read_row (data->fp, r_row, r_offset + (runtime->image_width * i),
                    runtime->image_width);
      raw_read_row (data->fp, g_row, g_offset + (runtime->image_width * i),
                    runtime->image_width);
      raw_read_row (data->fp, b_row, b_offset + (runtime->image_width * i),
                    runtime->image_width);

      /* Combine separate R, G and B rows into RGB triples */
      for (j = 0, k = 0; j < runtime->image_width; j++)
        {
          row[k++] = r_row[j];
          row[k++] = g_row[j];
          row[k++] = b_row[j];
        }

      gegl_buffer_set (data->buffer,
                       GEGL_RECTANGLE (0, i, runtime->image_width, 1), 0,
                       NULL, row, GEGL_AUTO_ROWSTRIDE);

      gimp_progress_update ((gfloat) i / (gfloat) runtime->image_height);
    }

  gimp_progress_update (1.0);

  g_free (row);
  g_free (r_row);
  g_free (g_row);
  g_free (b_row);

  return TRUE;
}

static gboolean
raw_load_palette (RawGimpData *data,
                  GFile       *palette_file)
{
  guchar temp[1024];
  gint   fd, i, j;

  if (palette_file)
    {
      gchar *filename;

      filename = g_file_get_path (palette_file);
      fd = g_open (filename, O_RDONLY, 0);
      g_free (filename);

      if (! fd)
        return FALSE;

      lseek (fd, runtime->palette_offset, SEEK_SET);

      switch (runtime->palette_type)
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
save_image (GFile         *file,
            GimpImage     *image,
            GimpDrawable  *drawable,
            GObject       *config,
            GError       **error)
{
  GeglBuffer     *buffer;
  gchar          *filename;
  const Babl     *format = NULL;
  guchar         *cmap   = NULL;  /* colormap for indexed images */
  guchar         *buf;
  guchar         *components[4] = { 0, };
  gint            n_components;
  gint32          width, height, bpp;
  FILE           *fp;
  gint            i, j, c;
  gint            palsize = 0;
  RawType         image_type;     /* type of image (RGB, PLANAR) */
  RawPaletteType  palette_type;   /* type of palette (RGB/BGR)   */
  gboolean        ret = FALSE;

  g_object_get (config,
                "image-type",   &image_type,
                "palette-type", &palette_type,
                NULL);

  buffer = gimp_drawable_get_buffer (drawable);

  switch (gimp_drawable_type (drawable))
    {
    case GIMP_RGB_IMAGE:
      format = babl_format ("R'G'B' u8");
      break;

    case GIMP_RGBA_IMAGE:
      format = babl_format ("R'G'B'A u8");
      break;

    case GIMP_GRAY_IMAGE:
      format = babl_format ("Y' u8");
      break;

    case GIMP_GRAYA_IMAGE:
      format = babl_format ("Y'A u8");
      break;

    case GIMP_INDEXED_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
      format = gimp_drawable_get_format (drawable);
      break;
    }

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

  filename = g_file_get_path (file);
  fp = g_fopen (filename, "wb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      g_free (filename);
      return FALSE;
    }

  ret = TRUE;

  switch (image_type)
    {
    case RAW_RGB:
      if (! fwrite (buf, width * height * bpp, 1, fp))
        {
          fclose (fp);
          return FALSE;
        }

      fclose (fp);

      if (cmap)
        {
          /* we have colormap, too.write it into filename+pal */
          gchar *newfile = g_strconcat (filename, ".pal", NULL);
          gchar *temp;

          fp = g_fopen (newfile, "wb");

          if (! fp)
            {
              g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Could not open '%s' for writing: %s"),
                           gimp_filename_to_utf8 (newfile), g_strerror (errno));
              g_free (filename);
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

    case RAW_PLANAR:
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

  g_free (filename);

  return ret;
}

static GimpImage *
load_image (GFile   *file,
            GError **error)
{
  RawGimpData       *data;
  GimpLayer         *layer = NULL;
  gchar             *filename;
  GimpImageType      ltype = GIMP_RGB_IMAGE;
  GimpImageBaseType  itype = GIMP_RGB;
  goffset            size;
  gint               bpp    = 0;
  gint               bitspp = 8;

  data = g_new0 (RawGimpData, 1);

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  filename = g_file_get_path (file);
  data->fp = g_fopen (filename, "rb");
  g_free (filename);

  if (! data->fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  size = get_file_info (file);

  switch (runtime->image_type)
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
      ltype = GIMP_RGB_IMAGE;
      itype = GIMP_RGB;
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
      ltype = GIMP_RGB_IMAGE;
      itype = GIMP_RGB;
      break;
    }

  /* make sure we don't load image bigger than file size */
  if (runtime->image_height > (size / runtime->image_width / bpp * 8 / bitspp))
    runtime->image_height = size / runtime->image_width / bpp * 8 / bitspp;

  if (runtime->image_type >= RAW_GRAY_16BPP_BE)
    data->image = gimp_image_new_with_precision (runtime->image_width,
                                                 runtime->image_height,
                                                 itype,
                                                 GIMP_PRECISION_U16_NON_LINEAR);
  else
    data->image = gimp_image_new (runtime->image_width,
                                  runtime->image_height,
                                  itype);
  gimp_image_set_file (data->image, file);
  layer = gimp_layer_new (data->image, _("Background"),
                          runtime->image_width, runtime->image_height,
                          ltype,
                          100,
                          gimp_image_get_default_new_layer_mode (data->image));
  gimp_image_insert_layer (data->image, layer, NULL, 0);

  data->buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  switch (runtime->image_type)
    {
    case RAW_RGB:
    case RAW_RGBA:
      raw_load_standard (data, bpp);
      break;

    case RAW_RGB565_BE:
    case RAW_RGB565_LE:
    case RAW_BGR565_BE:
    case RAW_BGR565_LE:
      raw_load_rgb565 (data, runtime->image_type);
      break;

    case RAW_PLANAR:
      raw_load_planar (data);
      break;

    case RAW_GRAY_1BPP:
      raw_load_gray (data, bpp, bitspp);
      break;
    case RAW_GRAY_2BPP:
      raw_load_gray (data, bpp, bitspp);
      break;
    case RAW_GRAY_4BPP:
      raw_load_gray (data, bpp, bitspp);
      break;
    case RAW_GRAY_8BPP:
      raw_load_gray (data, bpp, bitspp);
      break;

    case RAW_INDEXED:
    case RAW_INDEXEDA:
      raw_load_palette (data, palfile);
      raw_load_standard (data, bpp);
      break;

    case RAW_GRAY_16BPP_BE:
    case RAW_GRAY_16BPP_LE:
    case RAW_GRAY_16BPP_SBE:
    case RAW_GRAY_16BPP_SLE:
      raw_load_gray16 (data, runtime->image_type);
      break;
    }

  fclose (data->fp);

  g_object_unref (data->buffer);

  return data->image;
}


/* misc GUI stuff */

static void
preview_update_size (GimpPreviewArea *preview)
{
  gtk_widget_set_size_request (GTK_WIDGET (preview),
                               runtime->image_width, runtime->image_height);
}

static void
preview_update (GimpPreviewArea *preview)
{
  gint     preview_width;
  gint     preview_height;
  gint     width;
  gint     height;
  gint32   pos;
  gint     x, y;
  gint     bitspp = 0;

  gimp_preview_area_get_size (preview, &preview_width, &preview_height);

  width  = MIN (runtime->image_width,  preview_width);
  height = MIN (runtime->image_height, preview_height);

  gimp_preview_area_fill (preview,
                          0, 0, preview_width, preview_height,
                          255, 255, 255);

  switch (runtime->image_type)
    {
    case RAW_RGB:
      /* standard RGB image */
      {
        guchar *row = g_malloc0 (width * 3);

        for (y = 0; y < height; y++)
          {
            pos = runtime->file_offset + runtime->image_width * y * 3;
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
            pos = runtime->file_offset + runtime->image_width * y * 4;
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
            pos = runtime->file_offset + runtime->image_width * y * 2;
            mmap_read (preview_fd, in, width * 2, pos, width * 2);
            rgb_565_to_888 (in, row, width, runtime->image_type);

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

            pos = (runtime->file_offset +
                   (y * runtime->image_width));
            mmap_read (preview_fd, r_row, width, pos, width);

            pos = (runtime->file_offset +
                   (runtime->image_width * (runtime->image_height + y)));
            mmap_read (preview_fd, g_row, width, pos, width);

            pos = (runtime->file_offset +
                   (runtime->image_width * (runtime->image_height * 2 + y)));
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

        mmap_read (preview_fd, in_raw, in_size,
                   runtime->file_offset,
                   in_size);

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
        gboolean  alpha = (runtime->image_type == RAW_INDEXEDA);
        guchar   *index = g_malloc0 (width * (alpha ? 2 : 1));
        guchar   *row   = g_malloc0 (width * (alpha ? 4 : 3));

        if (preview_cmap_update)
          {
            if (palfile)
              {
                gchar *filename;
                gint   fd;

                filename = g_file_get_path (palfile);
                fd = g_open (filename, O_RDONLY, 0);
                g_free (filename);

                lseek (fd, runtime->palette_offset, SEEK_SET);
                read (fd, preview_cmap,
                      (runtime->palette_type == RAW_PALETTE_RGB) ? 768 : 1024);
                close (fd);
              }
            else
              {
                /* make fake palette, maybe overwrite it later */
                for (y = 0, x = 0; y < 256; y++)
                  {
                    preview_cmap[x++] = y;
                    preview_cmap[x++] = y;

                    if (runtime->palette_type == RAW_PALETTE_RGB)
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

            preview_cmap_update = FALSE;
          }

        for (y = 0; y < height; y++)
          {
            guchar *p = row;

            if (alpha)
              {
                pos = runtime->file_offset + runtime->image_width * 2 * y;
                mmap_read (preview_fd, index, width * 2, pos, width);

                for (x = 0; x < width; x++)
                  {
                    switch (runtime->palette_type)
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
                pos = runtime->file_offset + runtime->image_width * y;
                mmap_read (preview_fd, index, width, pos, width);

                for (x = 0; x < width; x++)
                  {
                    switch (runtime->palette_type)
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
        {
        guint16 *r_row = g_new (guint16, width);
        guchar  *row   = g_malloc (width);

        for (y = 0; y < height; y++)
          {
            gint j;

            pos = (runtime->file_offset + (y * runtime->image_width * 2));
            mmap_read (preview_fd, (guchar*) r_row, 2 * width, pos, width);

            for (j = 0; j < width; j++)
              {
                gint pixel_val;

                if (runtime->image_type == RAW_GRAY_16BPP_BE)
                  pixel_val = GUINT16_FROM_BE (r_row[j]);
                else if (runtime->image_type == RAW_GRAY_16BPP_LE)
                  pixel_val = GUINT16_FROM_LE (r_row[j]);
                else if (runtime->image_type == RAW_GRAY_16BPP_SBE)
                  pixel_val = GINT16_FROM_BE (r_row[j]) - G_MININT16;
                else /* if (runtime->image_type == RAW_GRAY_16BPP_SLE)*/
                  pixel_val = GINT16_FROM_LE (r_row[j]) - G_MININT16;

                row[j] = pixel_val / 257;
              }

            gimp_preview_area_draw (preview, 0, y, width, 1,
                                    GIMP_GRAY_IMAGE, row, width * 3);
          }

        g_free (r_row);
        g_free (row);
        }
      break;
    }
}

static void
palette_update (GimpPreviewArea *preview)
{
  preview_cmap_update = TRUE;

  preview_update (preview);
}

static gboolean
load_dialog (GFile    *file,
             gboolean  is_hgt)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *preview;
  GtkWidget     *sw;
  GtkWidget     *viewport;
  GtkWidget     *frame;
  GtkWidget     *grid;
  GtkWidget     *combo;
  GtkWidget     *button;
  GtkAdjustment *adj;
  goffset        file_size;
  gboolean       run;

  file_size = get_file_info (file);

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_dialog_new (_("Load Image from Raw Data"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func,
                            is_hgt ? LOAD_HGT_PROC : LOAD_PROC,
                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_Open"),   GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (main_vbox), sw, TRUE, TRUE, 0);
  gtk_widget_set_size_request (sw, PREVIEW_SIZE, PREVIEW_SIZE);
  gtk_widget_show (sw);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (sw), viewport);
  gtk_widget_show (viewport);

  preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (preview,
                               runtime->image_width, runtime->image_height);
  gtk_container_add (GTK_CONTAINER (viewport), preview);
  gtk_widget_show (preview);

  g_signal_connect_after (preview, "size-allocate",
                          G_CALLBACK (preview_update),
                          NULL);

  if (is_hgt)
    {
      if (runtime->image_width == 1201)
        /* Translators: Digital Elevation Model (DEM) is a technical term
         * used for 3D surface modeling or relief maps; so it must be
         * translated by the proper technical term in your language.
         */
        frame = gimp_frame_new (_("Digital Elevation Model data (1 arc-second)"));
      else if (runtime->image_width == 3601)
        frame = gimp_frame_new (_("Digital Elevation Model data (3 arc-seconds)"));
      else
        frame = gimp_frame_new (_("Digital Elevation Model data"));
    }
  else
    {
      frame = gimp_frame_new (_("Image"));
    }
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  combo = NULL;
  if (is_hgt                       &&
      runtime->image_width != 1201 &&
      runtime->image_width != 3601)
    {
      /* When auto-detection of the HGT variant failed, let's just
       * default to SRTM-3 and show a dropdown list.
       */
      runtime->image_width  = 1201;
      runtime->image_height = 1201;

      /* 2 types of HGT files are possible: SRTM-1 and SRTM-3.
       * From the documentation: https://dds.cr.usgs.gov/srtm/version1/Documentation/SRTM_Topo.txt
       * "SRTM-1 data are sampled at one arc-second of latitude and longitude and
       *  each file contains 3601 lines and 3601 samples.
       * [...]
       * SRTM-3 data are sampled at three arc-seconds and contain 1201 lines and
       * 1201 samples with similar overlapping rows and columns."
       */
      combo = gimp_int_combo_box_new (_("SRTM-1 (1 arc-second)"),  3601,
                                      _("SRTM-3 (3 arc-seconds)"), 1201,
                                      NULL);
      gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                _("_Sample Spacing:"), 0.0, 0.5,
                                combo, 2);

      g_signal_connect (combo, "changed",
                        G_CALLBACK (gimp_int_combo_box_get_active),
                        &runtime->image_width);
      g_signal_connect (combo, "changed",
                        G_CALLBACK (gimp_int_combo_box_get_active),
                        &runtime->image_height);
      /* By default, SRTM-3 is active. */
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), 1201);
    }
  else if (! is_hgt)
    {
      /* Generic case for any data. Let's leave choice to select the
       * right type of raw data.
       */
      combo = gimp_int_combo_box_new (_("RGB"),                  RAW_RGB,
                                      _("RGB Alpha"),            RAW_RGBA,
                                      _("RGB565 Big Endian"),    RAW_RGB565_BE,
                                      _("RGB565 Little Endian"), RAW_RGB565_LE,
                                      _("BGR565 Big Endian"),    RAW_BGR565_BE,
                                      _("BGR565 Little Endian"), RAW_BGR565_LE,
                                      _("Planar RGB"),           RAW_PLANAR,
                                      _("B&W 1 bit"),            RAW_GRAY_1BPP,
                                      _("Gray 2 bit"),           RAW_GRAY_2BPP,
                                      _("Gray 4 bit"),           RAW_GRAY_4BPP,
                                      _("Gray 8 bit"),           RAW_GRAY_8BPP,
                                      _("Indexed"),              RAW_INDEXED,
                                      _("Indexed Alpha"),        RAW_INDEXEDA,
                                      _("Gray unsigned 16 bit Big Endian"),    RAW_GRAY_16BPP_BE,
                                      _("Gray unsigned 16 bit Little Endian"), RAW_GRAY_16BPP_LE,
                                      _("Gray 16 bit Big Endian"),             RAW_GRAY_16BPP_SBE,
                                      _("Gray 16 bit Little Endian"),          RAW_GRAY_16BPP_SLE,
                                      NULL);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                     runtime->image_type);
      gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                _("Image _Type:"), 0.0, 0.5,
                                combo, 2);

      g_signal_connect (combo, "changed",
                        G_CALLBACK (gimp_int_combo_box_get_active),
                        &runtime->image_type);
    }
  if (combo)
    g_signal_connect_swapped (combo, "changed",
                              G_CALLBACK (preview_update),
                              preview);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 1,
                              _("O_ffset:"), -1, 9,
                              runtime->file_offset, 0, file_size, 1, 1000, 0,
                              TRUE, 0.0, 0.0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &runtime->file_offset);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (preview_update),
                            preview);

  if (! is_hgt)
    {
      adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 2,
                                  _("_Width:"), -1, 9,
                                  runtime->image_width, 1, file_size, 1, 10, 0,
                                  TRUE, 0.0, 0.0,
                                  NULL, NULL);

      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (gimp_int_adjustment_update),
                        &runtime->image_width);
      g_signal_connect_swapped (adj, "value-changed",
                                G_CALLBACK (preview_update_size),
                                preview);
      g_signal_connect_swapped (adj, "value-changed",
                                G_CALLBACK (preview_update),
                                preview);

      adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 3,
                                  _("_Height:"), -1, 9,
                                  runtime->image_height, 1, file_size, 1, 10, 0,
                                  TRUE, 0.0, 0.0,
                                  NULL, NULL);

      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (gimp_int_adjustment_update),
                        &runtime->image_height);
      g_signal_connect_swapped (adj, "value-changed",
                                G_CALLBACK (preview_update_size),
                                preview);
      g_signal_connect_swapped (adj, "value-changed",
                                G_CALLBACK (preview_update),
                                preview);
    }


  frame = gimp_frame_new (_("Palette"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  combo = gimp_int_combo_box_new (_("R, G, B (normal)"),       RAW_PALETTE_RGB,
                                  _("B, G, R, X (BMP style)"), RAW_PALETTE_BGR,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                 runtime->palette_type);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Palette Type:"), 0.0, 0.5,
                            combo, 2);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &runtime->palette_type);
  g_signal_connect_swapped (combo, "changed",
                            G_CALLBACK (palette_update),
                            preview);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 1,
                              _("Off_set:"), -1, 0,
                              runtime->palette_offset, 0, 1 << 24, 1, 768, 0,
                              TRUE, 0.0, 0.0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &runtime->palette_offset);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (palette_update),
                            preview);

  button = gtk_file_chooser_button_new (_("Select Palette File"),
                                        GTK_FILE_CHOOSER_ACTION_OPEN);
  if (palfile)
    gtk_file_chooser_set_file (GTK_FILE_CHOOSER (button), palfile, NULL);

  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("Pal_ette File:"), 0.0, 0.5,
                            button, 2);

  g_signal_connect (button, "selection-changed",
                    G_CALLBACK (palette_callback),
                    preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static gboolean
save_dialog (GimpImage     *image,
             GimpProcedure *procedure,
             GObject       *config)
{
  GtkWidget    *dialog;
  GtkWidget    *main_vbox;
  GtkListStore *store;
  GtkWidget    *frame;
  gboolean      run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Export Image as Raw Data"));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /* Image type combo */
  store = gimp_int_store_new (_("_Standard (R,G,B)"),     RAW_RGB,
                              _("_Planar (RRR,GGG,BBB)"), RAW_PLANAR,
                              NULL);
  frame = gimp_prop_int_radio_frame_new (config, "image-type",
                                         _("Image Type"),
                                         GIMP_INT_STORE (store));
  g_object_unref (store);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);

  /* Palette type combo */
  store = gimp_int_store_new (_("_R, G, B (normal)"),       RAW_PALETTE_RGB,
                              _("_B, G, R, X (BMP style)"), RAW_PALETTE_BGR,
                              NULL);
  frame = gimp_prop_int_radio_frame_new (config, "palette-type",
                                         _("Palette Type"),
                                         GIMP_INT_STORE (store));
  g_object_unref (store);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static void
palette_callback (GtkFileChooser  *button,
                  GimpPreviewArea *preview)
{
  if (palfile)
    g_object_unref (palfile);

  palfile = gtk_file_chooser_get_file (button);

  palette_update (preview);
}
