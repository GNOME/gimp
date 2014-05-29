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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#define SAVE_PROC      "file-raw-save"
#define PLUG_IN_BINARY "file-raw-data"
#define PLUG_IN_ROLE   "gimp-file-raw-data"
#define PREVIEW_SIZE   350


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
  RAW_INDEXEDA      /* Indexed image with an Alpha channel */
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
  gint32        image_id;  /* gimp image id                    */
  guchar        cmap[768]; /* color map for indexed images     */
} RawGimpData;


static void              query               (void);
static void              run                 (const gchar      *name,
                                              gint              nparams,
                                              const GimpParam  *param,
                                              gint             *nreturn_vals,
                                              GimpParam       **return_vals);

/* prototypes for the new load functions */
static gboolean          raw_load_standard   (RawGimpData      *data,
                                              gint              bpp);
static gboolean          raw_load_gray       (RawGimpData      *data,
                                              gint              bpp,
                                              gint              bitspp);
static gboolean          raw_load_rgb565     (RawGimpData      *data,
                                              RawType           type);
static gboolean          raw_load_planar     (RawGimpData      *data);
static gboolean          raw_load_palette    (RawGimpData      *data,
                                              const gchar      *palette_filename);

/* support functions */
static gint32            get_file_info       (const gchar      *filename);
static void              raw_read_row        (FILE             *fp,
                                              guchar           *buf,
                                              gint32            offset,
                                              gint32            size);
static int               mmap_read           (gint              fd,
                                              gpointer          buf,
                                              gint32            len,
                                              gint32            pos,
                                              gint              rowstride);
static void              rgb_565_to_888      (guint16          *in,
                                              guchar           *out,
                                              gint32            num_pixels,
                                              RawType           type);

static gint32            load_image          (const gchar      *filename,
                                              GError          **error);
static GimpPDBStatusType save_image          (const gchar      *filename,
                                              gint32            image_id,
                                              gint32            drawable_id,
                                              GError          **error);

/* gui functions */
static void              preview_update_size (GimpPreviewArea  *preview);
static void              preview_update      (GimpPreviewArea  *preview);
static void              palette_update      (GimpPreviewArea  *preview);
static gboolean          load_dialog         (const gchar      *filename);
static gboolean          save_dialog         (const gchar      *filename,
                                              gint32            image_id,
                                              gint32            drawable_id);
static void              palette_callback    (GtkFileChooser   *button,
                                              GimpPreviewArea  *preview);


static RawConfig *runtime             = NULL;
static gchar     *palfile             = NULL;
static gint       preview_fd          = -1;
static guchar     preview_cmap[1024];
static gboolean   preview_cmap_update = TRUE;


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,   /* quit_proc  */
  query,  /* query_proc */
  run,    /* run_proc   */
};

MAIN()

static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0) }"                  },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name entered"             }
  };

  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save"             },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name entered"             }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Load raw images, specifying image information",
                          "Load raw images, specifying image information",
                          "timecop, pg@futureware.at",
                          "timecop, pg@futureware.at",
                          "Aug 2004",
                          N_("Raw image data"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_load_handler (LOAD_PROC, "data", "");

  gimp_install_procedure (SAVE_PROC,
                          "Dump images to disk in raw format",
                          "Dump images to disk in raw format",
                          "timecop, pg@futureware.at",
                          "timecop, pg@futureware.at",
                          "Aug 2004",
                          N_("Raw image data"),
                          "INDEXED, GRAY, RGB, RGBA",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_save_handler (SAVE_PROC, "data", "");
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
  gint32             image_id;
  gint32             drawable_id;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  /* allocate config structure and fill with defaults */
  runtime = g_new0 (RawConfig, 1);

  runtime->file_offset    = 0;
  runtime->image_width    = PREVIEW_SIZE;
  runtime->image_height   = PREVIEW_SIZE;
  runtime->palette_offset = 0;
  runtime->palette_type   = RAW_PALETTE_RGB;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          gimp_get_data (LOAD_PROC, runtime);

          preview_fd = g_open (param[1].data.d_string, O_RDONLY, 0);

          if (preview_fd < 0)
            {
              g_set_error (&error,
                           G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Could not open '%s' for reading: %s"),
                           gimp_filename_to_utf8 (param[1].data.d_string),
                           g_strerror (errno));

              status = GIMP_PDB_EXECUTION_ERROR;
            }
          else
            {
              if (! load_dialog (param[1].data.d_string))
                status = GIMP_PDB_CANCEL;

              close (preview_fd);
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
          image_id = load_image (param[1].data.d_string, &error);

          if (image_id != -1)
            {
              gimp_set_data (LOAD_PROC, runtime, sizeof (RawConfig));

              *nreturn_vals = 2;
              values[1].type         = GIMP_PDB_IMAGE;
              values[1].data.d_image = image_id;
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }
    }
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      image_id    = param[1].data.d_int32;
      drawable_id = param[2].data.d_int32;

      /* export the image */
      export = gimp_export_image (&image_id, &drawable_id, "RAW",
                                  GIMP_EXPORT_CAN_HANDLE_RGB     |
                                  GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                  GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA);

      if (export == GIMP_EXPORT_CANCEL)
        {
          *nreturn_vals = 1;
          values[0].data.d_status = GIMP_PDB_CANCEL;
          return;
        }

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          gimp_get_data (SAVE_PROC, runtime);

          if (nparams != 5)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else if (! save_dialog (param[3].data.d_string,
                                  image_id, drawable_id))
            {
              status = GIMP_PDB_CANCEL;
            }
        }
      else
        {
          /* we want to make sure we always do save-as for raw images
           * to avoid confusion and data loss
           */
          status = GIMP_PDB_CALLING_ERROR;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          status = save_image (param[3].data.d_string, image_id, drawable_id,
                               &error);
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_id);
    }

  g_free (runtime);

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}


/* get file size from a filename */
static gint32
get_file_info (const gchar *filename)
{
  struct stat status;

  g_stat (filename, &status);

  return status.st_size;
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
                  const gchar *palette_file)
{
  guchar temp[1024];
  gint   fd, i, j;

  if (palette_file)
    {
      fd = g_open (palette_file, O_RDONLY, 0);

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

  gimp_image_set_colormap (data->image_id, data->cmap, 256);

  return TRUE;
}

/* end new image handle functions */

static GimpPDBStatusType
save_image (const gchar  *filename,
            gint32        image_id,
            gint32        drawable_id,
            GError      **error)
{
  GeglBuffer       *buffer;
  const Babl       *format = NULL;
  guchar           *cmap   = NULL;  /* colormap for indexed images */
  guchar           *buf;
  guchar           *red, *green, *blue, *alpha = NULL;
  gint32            width, height, bpp = 0;
  gboolean          have_alpha = 0;
  FILE             *fp;
  gint              i, j = 0;
  gint              palsize = 0;
  GimpPDBStatusType ret = GIMP_PDB_EXECUTION_ERROR;

  /* get info about the current image */
  buffer = gimp_drawable_get_buffer (drawable_id);

  switch (gimp_drawable_type (drawable_id))
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
      format = gimp_drawable_get_format (drawable_id);
      break;
    }

  bpp        = babl_format_get_bytes_per_pixel (format);
  have_alpha = babl_format_has_alpha (format);

  if (gimp_drawable_is_indexed (drawable_id))
    cmap = gimp_image_get_colormap (image_id, &palsize);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  buf = g_new (guchar, width * height * bpp);

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, width, height), 1.0,
                   format, buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_object_unref (buffer);

  fp = g_fopen (filename, "wb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  ret = GIMP_PDB_SUCCESS;

  switch (runtime->image_type)
    {
    case RAW_RGB:
      if (! fwrite (buf, width * height * bpp, 1, fp))
        {
          return GIMP_PDB_EXECUTION_ERROR;
        }

      fclose (fp);

      if (cmap)
        {
          /* we have a colormap, too.write it into filename+pal */
          gchar *newfile = g_strconcat (filename, ".pal", NULL);
          gchar *temp;

          fp = g_fopen (newfile, "wb");

          if (! fp)
            {
              g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Could not open '%s' for writing: %s"),
                           gimp_filename_to_utf8 (newfile), g_strerror (errno));
              return GIMP_PDB_EXECUTION_ERROR;
            }

          switch (runtime->palette_type)
            {
            case RAW_PALETTE_RGB:
              if (!fwrite (cmap, palsize * 3, 1, fp))
                ret = GIMP_PDB_EXECUTION_ERROR;
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
                ret = GIMP_PDB_EXECUTION_ERROR;
              fclose (fp);
              g_free (temp);
              break;
            }
        }
      break;

    case RAW_PLANAR:
      red   = g_new (guchar, width * height);
      green = g_new (guchar, width * height);
      blue  = g_new (guchar, width * height);
      if (have_alpha)
        alpha = g_new (guchar, width * height);

      for (i = 0; i < width * height * bpp; i += bpp)
        {
          red[j]   = buf[i + 0];
          green[j] = buf[i + 1];
          blue[j]  = buf[i + 2];
          if (have_alpha)
            alpha[j] = buf[i + 3];
          j++;
        }

      ret = GIMP_PDB_SUCCESS;
      if (!fwrite (red, width * height, 1, fp))
        ret = GIMP_PDB_EXECUTION_ERROR;
      if (!fwrite (green, width * height, 1, fp))
        ret = GIMP_PDB_EXECUTION_ERROR;
      if (!fwrite (blue, width * height, 1, fp))
        ret = GIMP_PDB_EXECUTION_ERROR;
      if (have_alpha)
        {
          if (!fwrite (alpha, width * height, 1, fp))
            ret = GIMP_PDB_EXECUTION_ERROR;
        }
      g_free (red);
      g_free (green);
      g_free (blue);
      if (have_alpha)
        g_free (alpha);
      fclose (fp);
      break;

    default:
      break;
    }

  return ret;
}

static gint32
load_image (const gchar  *filename,
            GError      **error)
{
  RawGimpData       *data;
  gint32             layer_id = -1;
  GimpImageType      ltype    = GIMP_RGB;
  GimpImageBaseType  itype    = GIMP_RGB_IMAGE;
  gint32             size;
  gint               bpp = 0;
  gint               bitspp = 8;

  data = g_new0 (RawGimpData, 1);

  data->fp = g_fopen (filename, "rb");
  if (! data->fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  size = get_file_info (filename);

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
    }

  /* make sure we don't load image bigger than file size */
  if (runtime->image_height > (size / runtime->image_width / bpp * 8 / bitspp))
    runtime->image_height = size / runtime->image_width / bpp * 8 / bitspp;

  data->image_id = gimp_image_new (runtime->image_width,
                                   runtime->image_height,
                                   itype);
  gimp_image_set_filename(data->image_id, filename);
  layer_id = gimp_layer_new (data->image_id, _("Background"),
                             runtime->image_width, runtime->image_height, ltype,
                             100, GIMP_NORMAL_MODE);
  gimp_image_insert_layer (data->image_id, layer_id, -1, 0);

  data->buffer = gimp_drawable_get_buffer (layer_id);

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
    }

  fclose (data->fp);

  g_object_unref (data->buffer);

  return data->image_id;
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
  gint     width;
  gint     height;
  gint32   pos;
  gint     x, y;
  gint     bitspp = 0;

  width  = MIN (runtime->image_width,  preview->width);
  height = MIN (runtime->image_height, preview->height);

  gimp_preview_area_fill (preview,
                          0, 0, preview->width, preview->height,
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
                gint fd;

                fd = g_open (palfile, O_RDONLY, 0);
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
    }
}

static void
palette_update (GimpPreviewArea *preview)
{
  preview_cmap_update = TRUE;

  preview_update (preview);
}

static gboolean
load_dialog (const gchar *filename)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *sw;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *combo;
  GtkWidget *button;
  GtkObject *adj;
  gint32     file_size;
  gboolean   run;

  file_size = get_file_info (filename);

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Load Image from Raw Data"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, LOAD_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OPEN,   GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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

  preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (preview,
                               runtime->image_width, runtime->image_height);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (sw), preview);
  gtk_widget_show (preview);

  g_signal_connect_after (preview, "size-allocate",
                          G_CALLBACK (preview_update),
                          NULL);

  frame = gimp_frame_new (_("Image"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

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
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                 runtime->image_type);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Image _Type:"), 0.0, 0.5,
                             combo, 2, FALSE);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &runtime->image_type);
  g_signal_connect_swapped (combo, "changed",
                            G_CALLBACK (preview_update),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
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

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
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

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
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


  frame = gimp_frame_new (_("Palette"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  combo = gimp_int_combo_box_new (_("R, G, B (normal)"),       RAW_PALETTE_RGB,
                                  _("B, G, R, X (BMP style)"), RAW_PALETTE_BGR,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                 runtime->palette_type);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("_Palette Type:"), 0.0, 0.5,
                             combo, 2, FALSE);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &runtime->palette_type);
  g_signal_connect_swapped (combo, "changed",
                            G_CALLBACK (palette_update),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
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
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (button), palfile);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Pal_ette File:"), 0.0, 0.5,
                             button, 2, FALSE);

  g_signal_connect (button, "selection-changed",
                    G_CALLBACK (palette_callback),
                    preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static gboolean
save_dialog (const gchar *filename,
             gint32       image_id,
             gint32       drawable_id)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_export_dialog_new (_("Raw Image"), PLUG_IN_BINARY, SAVE_PROC);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      main_vbox, FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  frame = gimp_int_radio_group_new (TRUE, _("RGB Save Type"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &runtime->image_type,
                                    runtime->image_type,
                                    _("Standard (R,G,B)"),     RAW_RGB,    NULL,
                                    _("Planar (RRR,GGG,BBB)"), RAW_PLANAR, NULL,
                                    NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame = gimp_int_radio_group_new (TRUE, _("Indexed Palette Type"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &runtime->palette_type,
                                    runtime->palette_type,
                                    _("R, G, B (normal)"),
                                    RAW_PALETTE_RGB, NULL,
                                    _("B, G, R, X (BMP style)"),
                                    RAW_PALETTE_BGR, NULL,
                                    NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
palette_callback (GtkFileChooser  *button,
                  GimpPreviewArea *preview)
{
  if (palfile)
    g_free (palfile);

  palfile = gtk_file_chooser_get_filename (button);

  palette_update (preview);
}
