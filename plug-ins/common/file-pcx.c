/*
 * pcx.c GIMP plug-in for loading & exporting PCX files
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

/* This code is based in parts on code by Francisco Bustamante, but the
   largest portion of the code has been rewritten and is now maintained
   occasionally by Nick Lamb njl195@zepler.org.uk */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-pcx-load"
#define SAVE_PROC      "file-pcx-save"
#define PLUG_IN_BINARY "file-pcx"
#define PLUG_IN_ROLE   "gimp-file-pcx"


/* Declare local functions.  */

static void   query            (void);
static void   run              (const gchar      *name,
                                gint              nparams,
                                const GimpParam  *param,
                                gint             *nreturn_vals,
                                GimpParam       **return_vals);

static gint32 load_image       (const gchar      *filename,
                                GError          **error);

static void   load_1           (FILE             *fp,
                                gint              width,
                                gint              height,
                                guchar           *buf,
                                guint16           bytes);
static void   load_4           (FILE             *fp,
                                gint              width,
                                gint              height,
                                guchar           *buf,
                                guint16           bytes);
static void   load_sub_8       (FILE             *fp,
                                gint              width,
                                gint              height,
                                gint              bpp,
                                gint              plane,
                                guchar           *buf,
                                guint16           bytes);
static void   load_8           (FILE             *fp,
                                gint              width,
                                gint              height,
                                guchar           *buf,
                                guint16           bytes);
static void   load_24          (FILE             *fp,
                                gint              width,
                                gint              height,
                                guchar           *buf,
                                guint16           bytes);
static void   readline         (FILE             *fp,
                                guchar           *buf,
                                gint              bytes);

static gint   save_image       (const gchar      *filename,
                                gint32            image,
                                gint32            layer,
                                GError          **error);
static void   save_less_than_8 (FILE             *fp,
                                gint              width,
                                gint              height,
                                const gint        bpp,
                                const guchar     *buf,
                                gboolean          padding);
static void   save_8           (FILE             *fp,
                                gint              width,
                                gint              height,
                                const guchar     *buf,
                                gboolean          padding);
static void   save_24          (FILE             *fp,
                                gint              width,
                                gint              height,
                                const guchar     *buf,
                                gboolean          padding);
static void   writeline        (FILE             *fp,
                                const guchar     *buf,
                                gint              bytes);

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
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
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
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to export"           },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to export the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name entered"             }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads files in Zsoft PCX file format",
                          "FIXME: write help for pcx_load",
                          "Francisco Bustamante & Nick Lamb",
                          "Nick Lamb <njl195@zepler.org.uk>",
                          "January 1997",
                          N_("ZSoft PCX image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-pcx");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "pcx,pcc",
                                    "",
                                    "0&,byte,10,2&,byte,1,3&,byte,>0,3,byte,<9");

  gimp_install_procedure (SAVE_PROC,
                          "Exports files in ZSoft PCX file format",
                          "FIXME: write help for pcx_save",
                          "Francisco Bustamante & Nick Lamb",
                          "Nick Lamb <njl195@zepler.org.uk>",
                          "January 1997",
                          N_("ZSoft PCX image"),
                          "INDEXED, RGB, GRAY",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/x-pcx");
  gimp_register_save_handler (SAVE_PROC, "pcx,pcc", "");
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
  gint32             image_ID;
  gint32             drawable_ID;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
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
      image_ID = load_image (param[1].data.d_string, &error);

      if (image_ID != -1)
        {
          *nreturn_vals = 2;
          values[1].type = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          export = gimp_export_image (&image_ID, &drawable_ID, "PCX",
                                      GIMP_EXPORT_CAN_HANDLE_RGB  |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY |
                                      GIMP_EXPORT_CAN_HANDLE_INDEXED);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }
          break;
        default:
          break;
        }

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          break;

        case GIMP_RUN_NONINTERACTIVE:
          if (nparams != 5)
            status = GIMP_PDB_CALLING_ERROR;
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          if (! save_image (param[3].data.d_string, image_ID, drawable_ID,
                            &error))
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
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

static struct
{
  guint8  manufacturer;
  guint8  version;
  guint8  compression;
  guint8  bpp;
  guint16 x1, y1;
  guint16 x2, y2;
  guint16 hdpi;
  guint16 vdpi;
  guint8  colormap[48];
  guint8  reserved;
  guint8  planes;
  guint16 bytesperline;
  guint16 color;
  guint8  filler[58];
} pcx_header;

static struct {
  size_t   size;
  gpointer address;
} const pcx_header_buf_xlate[] = {
  { 1,  &pcx_header.manufacturer },
  { 1,  &pcx_header.version      },
  { 1,  &pcx_header.compression  },
  { 1,  &pcx_header.bpp          },
  { 2,  &pcx_header.x1           },
  { 2,  &pcx_header.y1           },
  { 2,  &pcx_header.x2           },
  { 2,  &pcx_header.y2           },
  { 2,  &pcx_header.hdpi         },
  { 2,  &pcx_header.vdpi         },
  { 48, &pcx_header.colormap     },
  { 1,  &pcx_header.reserved     },
  { 1,  &pcx_header.planes       },
  { 2,  &pcx_header.bytesperline },
  { 2,  &pcx_header.color        },
  { 58, &pcx_header.filler       },
  { 0,  NULL }
};

static void
pcx_header_from_buffer (guint8 *buf)
{
  gint i;
  gint buf_offset = 0;

  for (i = 0; pcx_header_buf_xlate[i].size != 0; i++)
    {
      memmove (pcx_header_buf_xlate[i].address, buf + buf_offset,
               pcx_header_buf_xlate[i].size);
      buf_offset += pcx_header_buf_xlate[i].size;
    }
}

static void
pcx_header_to_buffer (guint8 *buf)
{
  gint i;
  gint buf_offset = 0;

  for (i = 0; pcx_header_buf_xlate[i].size != 0; i++)
    {
      memmove (buf + buf_offset, pcx_header_buf_xlate[i].address,
               pcx_header_buf_xlate[i].size);
      buf_offset += pcx_header_buf_xlate[i].size;
    }
}

static gint32
load_image (const gchar  *filename,
            GError      **error)
{
  FILE         *fd;
  GeglBuffer   *buffer;
  guint16       offset_x, offset_y, bytesperline;
  gint32        width, height;
  guint16       resolution_x, resolution_y;
  gint32        image, layer;
  guchar       *dest, cmap[768];
  guint8        header_buf[128];

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  fd = g_fopen (filename, "rb");

  if (! fd)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  if (fread (header_buf, 128, 1, fd) == 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read header from '%s'"),
                   gimp_filename_to_utf8 (filename));
      fclose (fd);
      return -1;
    }

  pcx_header_from_buffer (header_buf);

  if (pcx_header.manufacturer != 10)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a PCX file"),
                   gimp_filename_to_utf8 (filename));
      fclose (fd);
      return -1;
    }

  offset_x     = GUINT16_FROM_LE (pcx_header.x1);
  offset_y     = GUINT16_FROM_LE (pcx_header.y1);
  width        = GUINT16_FROM_LE (pcx_header.x2) - offset_x + 1;
  height       = GUINT16_FROM_LE (pcx_header.y2) - offset_y + 1;
  bytesperline = GUINT16_FROM_LE (pcx_header.bytesperline);
  resolution_x = GUINT16_FROM_LE (pcx_header.hdpi);
  resolution_y = GUINT16_FROM_LE (pcx_header.vdpi);

  if ((width <= 0) || (width > GIMP_MAX_IMAGE_SIZE))
    {
      g_message (_("Unsupported or invalid image width: %d"), width);
      fclose (fd);
      return -1;
    }
  if ((height <= 0) || (height > GIMP_MAX_IMAGE_SIZE))
    {
      g_message (_("Unsupported or invalid image height: %d"), height);
      fclose (fd);
      return -1;
    }
  if (bytesperline < ((width * pcx_header.bpp + 7) / 8))
    {
      g_message (_("Invalid number of bytes per line in PCX header"));
      fclose (fd);
      return -1;
    }
  if ((resolution_x < 1) || (resolution_x > GIMP_MAX_RESOLUTION) ||
      (resolution_y < 1) || (resolution_y > GIMP_MAX_RESOLUTION))
    {
      g_message (_("Resolution out of bounds in XCX header, using 72x72"));
      resolution_x = 72;
      resolution_y = 72;
    }

  /* Shield against potential buffer overflows in load_*() functions. */
  if (G_MAXSIZE / width / height < 3)
    {
      g_message (_("Image dimensions too large: width %d x height %d"), width, height);
      fclose (fd);
      return -1;
    }

  if (pcx_header.planes == 3 && pcx_header.bpp == 8)
    {
      image= gimp_image_new (width, height, GIMP_RGB);
      layer= gimp_layer_new (image, _("Background"), width, height,
                             GIMP_RGB_IMAGE,
                             100,
                             gimp_image_get_default_new_layer_mode (image));
    }
  else
    {
      image= gimp_image_new (width, height, GIMP_INDEXED);
      layer= gimp_layer_new (image, _("Background"), width, height,
                             GIMP_INDEXED_IMAGE,
                             100,
                             gimp_image_get_default_new_layer_mode (image));
    }

  gimp_image_set_filename (image, filename);
  gimp_image_set_resolution (image, resolution_x, resolution_y);

  gimp_image_insert_layer (image, layer, -1, 0);
  gimp_layer_set_offsets (layer, offset_x, offset_y);

  buffer = gimp_drawable_get_buffer (layer);

  if (pcx_header.planes == 1 && pcx_header.bpp == 1)
    {
      const guint8 *colormap = pcx_header.colormap;
      dest = g_new (guchar, ((gsize) width) * height);
      load_1 (fd, width, height, dest, bytesperline);
      /* Monochrome does not mean necessarily B&W. Therefore we still
       * want to check the header palette, even for just 2 colors.
       * Hopefully the header palette will always be filled with
       * meaningful colors and the creator software did not just assume
       * B&W by being monochrome.
       * Until now test samples showed that even when B&W the header
       * palette was correctly filled with these 2 colors and we didn't
       * find counter-examples.
       * See bug 159947, comment 21 and 23.
       */
      /* ... Actually, there *are* files out there with a zeroed 1-bit palette,
       * which are supposed to be displayed as B&W (see issue #2997.)  These
       * files *might* be in the wrong (who knows...) but the fact is that
       * other software, including older versions of GIMP, do display them
       * "correctly", so let's follow suit: if the two palette colors are
       * equal, use a B&W palette instead.
       */
      if (! memcmp (colormap, colormap + 3, 3))
        {
          static const guint8 bw_colormap[6] = {  0,   0,   0,
                                                255, 255, 255};
          colormap = bw_colormap;
        }
      gimp_image_set_colormap (image, colormap, 2);
    }
  else if (pcx_header.bpp == 1 && pcx_header.planes == 2)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 1, 2, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 4);
    }
  else if (pcx_header.bpp == 2 && pcx_header.planes == 1)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 2, 1, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 4);
    }
  else if (pcx_header.bpp == 1 && pcx_header.planes == 3)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 1, 3, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 8);
    }
  else if (pcx_header.bpp == 1 && pcx_header.planes == 4)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_4 (fd, width, height, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 16);
    }
  else if (pcx_header.bpp == 4 && pcx_header.planes == 1)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 4, 1, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 16);
    }
  else if (pcx_header.bpp == 8 && pcx_header.planes == 1)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_8 (fd, width, height, dest, bytesperline);
      fseek (fd, -768L, SEEK_END);
      fread (cmap, 768, 1, fd);
      gimp_image_set_colormap (image, cmap, 256);
    }
  else if (pcx_header.bpp == 8 && pcx_header.planes == 3)
    {
      dest = g_new (guchar, ((gsize) width) * height * 3);
      load_24 (fd, width, height, dest, bytesperline);
    }
  else
    {
      g_message (_("Unusual PCX flavour, giving up"));
      fclose (fd);
      return -1;
    }

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   NULL, dest, GEGL_AUTO_ROWSTRIDE);

  fclose (fd);
  g_free (dest);
  g_object_unref (buffer);

  gimp_progress_update (1.0);

  return image;
}

static void
load_8 (FILE    *fp,
        gint     width,
        gint     height,
        guchar  *buf,
        guint16  bytes)
{
  gint    row;
  guchar *line = g_new (guchar, bytes);

  for (row = 0; row < height; buf += width, ++row)
    {
      readline (fp, line, bytes);
      memcpy (buf, line, width);
      gimp_progress_update ((double) row / (double) height);
    }

  g_free (line);
}

static void
load_24 (FILE    *fp,
         gint     width,
         gint     height,
         guchar  *buf,
         guint16  bytes)
{
  gint    x, y, c;
  guchar *line = g_new (guchar, bytes);

  for (y = 0; y < height; buf += width * 3, ++y)
    {
      for (c = 0; c < 3; ++c)
        {
          readline (fp, line, bytes);
          for (x = 0; x < width; ++x)
            {
              buf[x * 3 + c] = line[x];
            }
        }
      gimp_progress_update ((double) y / (double) height);
    }

  g_free (line);
}

static void
load_1 (FILE    *fp,
        gint     width,
        gint     height,
        guchar  *buf,
        guint16  bytes)
{
  gint    x, y;
  guchar *line = g_new (guchar, bytes);

  for (y = 0; y < height; buf += width, ++y)
    {
      readline (fp, line, bytes);
      for (x = 0; x < width; ++x)
        {
          if (line[x / 8] & (128 >> (x % 8)))
            buf[x] = 1;
          else
            buf[x] = 0;
        }
      gimp_progress_update ((double) y / (double) height);
    }

  g_free (line);
}

static void
load_4 (FILE    *fp,
        gint     width,
        gint     height,
        guchar  *buf,
        guint16  bytes)
{
  gint    x, y, c;
  guchar *line = g_new (guchar, bytes);

  for (y = 0; y < height; buf += width, ++y)
    {
      for (x = 0; x < width; ++x)
        buf[x] = 0;
      for (c = 0; c < 4; ++c)
        {
          readline(fp, line, bytes);
          for (x = 0; x < width; ++x)
            {
              if (line[x / 8] & (128 >> (x % 8)))
                buf[x] += (1 << c);
            }
        }
      gimp_progress_update ((double) y / (double) height);
    }

  g_free (line);
}

static void
load_sub_8 (FILE    *fp,
            gint     width,
            gint     height,
            gint     bpp,
            gint     plane,
            guchar  *buf,
            guint16  bytes)
{
  gint    x, y, c, b;
  guchar *line = g_new (guchar, bytes);
  gint    real_bpp = bpp - 1;
  gint    current_bit = 0;

  for (y = 0; y < height; buf += width, ++y)
    {
      for (x = 0; x < width; ++x)
        buf[x] = 0;
      for (c = 0; c < plane; ++c)
        {
          readline (fp, line, bytes);
          for (x = 0; x < width; ++x)
            {
              for (b = 0; b < bpp; b++)
                {
                  current_bit = bpp * x + b;
                  if (line[current_bit / 8] & (128 >> (current_bit % 8)))
                    buf[x] += (1 << (real_bpp - b + c));
                }
            }
        }
      gimp_progress_update ((double) y / (double) height);
    }

  g_free (line);
}

static void
readline (FILE   *fp,
          guchar *buf,
          gint    bytes)
{
  static guchar count = 0, value = 0;

  if (pcx_header.compression)
    {
      while (bytes--)
        {
          if (count == 0)
            {
              value = fgetc (fp);
              if (value < 0xc0)
                {
                  count = 1;
                }
              else
                {
                  count = value - 0xc0;
                  value = fgetc (fp);
                }
            }
          count--;
          *(buf++) = value;
        }
    }
  else
    {
      fread (buf, bytes, 1, fp);
    }
}

static gint
save_image (const gchar  *filename,
            gint32        image,
            gint32        layer,
            GError      **error)
{
  FILE          *fp;
  GeglBuffer    *buffer;
  const Babl    *format;
  GimpImageType  drawable_type;
  guchar        *cmap= NULL;
  guchar        *pixels;
  gint           offset_x, offset_y;
  guint          width, height;
  gdouble        resolution_x, resolution_y;
  gint           colors, i;
  guint8         header_buf[128];
  gboolean       padding = FALSE;

  drawable_type = gimp_drawable_type (layer);
  gimp_drawable_offsets (layer, &offset_x, &offset_y);

  buffer = gimp_drawable_get_buffer (layer);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_filename_to_utf8 (filename));

  pcx_header.manufacturer = 0x0a;
  pcx_header.version = 5;
  pcx_header.compression = 1;

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE:
      cmap                          = gimp_image_get_colormap (image, &colors);
      if (colors > 16)
        {
          pcx_header.bpp            = 8;
          pcx_header.planes         = 1;
          pcx_header.bytesperline   = width;
        }
      else if (colors > 2)
        {
          pcx_header.bpp            = 4;
          pcx_header.planes         = 1;
          pcx_header.bytesperline   = (width + 1) / 2;
        }
      else
       {
          pcx_header.bpp            = 1;
          pcx_header.planes         = 1;
          pcx_header.bytesperline   = (width + 7) / 8;
        }
      pcx_header.color        = GUINT16_TO_LE (1);
      format                  = NULL;

      /* Some references explain that 2bpp/1plane and 4bpp/1plane files
       * would use the palette at EOF (not the one from the header) if
       * we are in version 5 of PCX. Other sources affirm that even in
       * version 5, EOF palette must be used only when there are more
       * than 16 colors. We go with this second assumption.
       * See bug 159947, comment 21 and 23.
       */
      if (colors <= 16)
        {
          for (i = 0; i < (colors * 3); i++)
            {
              pcx_header.colormap[i] = cmap[i];
            }
        }

      break;

    case GIMP_RGB_IMAGE:
      pcx_header.bpp          = 8;
      pcx_header.planes       = 3;
      pcx_header.color        = GUINT16_TO_LE (1);
      pcx_header.bytesperline = width;
      format                  = babl_format ("R'G'B' u8");
      break;

    case GIMP_GRAY_IMAGE:
      pcx_header.bpp          = 8;
      pcx_header.planes       = 1;
      pcx_header.color        = GUINT16_TO_LE (2);
      pcx_header.bytesperline = width;
      format                  = babl_format ("Y' u8");
      break;

    default:
      g_message (_("Cannot export images with alpha channel."));
      return FALSE;
  }

  /* Bytes per Line must be an even number, according to spec */
  if (pcx_header.bytesperline % 2 != 0)
    {
      pcx_header.bytesperline++;
      padding = TRUE;
    }
  pcx_header.bytesperline = GUINT16_TO_LE (pcx_header.bytesperline);

  pixels = (guchar *) g_malloc (width * height * pcx_header.planes);

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, width, height), 1.0,
                   format, pixels,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if ((offset_x < 0) || (offset_x > (1<<16)))
    {
      g_message (_("Invalid X offset: %d"), offset_x);
      return FALSE;
    }

  if ((offset_y < 0) || (offset_y > (1<<16)))
    {
      g_message (_("Invalid Y offset: %d"), offset_y);
      return FALSE;
    }

  if (offset_x + width - 1 > (1<<16))
    {
      g_message (_("Right border out of bounds (must be < %d): %d"), (1<<16),
                 offset_x + width - 1);
      return FALSE;
    }

  if (offset_y + height - 1 > (1<<16))
    {
      g_message (_("Bottom border out of bounds (must be < %d): %d"), (1<<16),
                 offset_y + height - 1);
      return FALSE;
    }

  if ((fp = g_fopen (filename, "wb")) == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  pcx_header.x1 = GUINT16_TO_LE ((guint16)offset_x);
  pcx_header.y1 = GUINT16_TO_LE ((guint16)offset_y);
  pcx_header.x2 = GUINT16_TO_LE ((guint16)(offset_x + width - 1));
  pcx_header.y2 = GUINT16_TO_LE ((guint16)(offset_y + height - 1));

  gimp_image_get_resolution (image, &resolution_x, &resolution_y);

  pcx_header.hdpi = GUINT16_TO_LE (RINT (MAX (resolution_x, 1.0)));
  pcx_header.vdpi = GUINT16_TO_LE (RINT (MAX (resolution_y, 1.0)));
  pcx_header.reserved = 0;

  pcx_header_to_buffer (header_buf);

  fwrite (header_buf, 128, 1, fp);

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE:
      if (colors > 16)
        {
          save_8 (fp, width, height, pixels, padding);
          fputc (0x0c, fp);
          fwrite (cmap, colors, 3, fp);
          for (i = colors; i < 256; i++)
            {
              fputc (0, fp);
              fputc (0, fp);
              fputc (0, fp);
            }
        }
      else /* Covers 1 and 4 bpp */
        {
          save_less_than_8 (fp, width, height, pcx_header.bpp, pixels, padding);
        }
      break;

    case GIMP_RGB_IMAGE:
      save_24 (fp, width, height, pixels, padding);
      break;

    case GIMP_GRAY_IMAGE:
      save_8 (fp, width, height, pixels, padding);
      fputc (0x0c, fp);
      for (i = 0; i < 256; i++)
        {
          fputc ((guchar) i, fp);
          fputc ((guchar) i, fp);
          fputc ((guchar) i, fp);
        }
      break;

    default:
      return FALSE;
    }

  g_object_unref (buffer);
  g_free (pixels);

  if (fclose (fp) != 0)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Writing to file '%s' failed: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  return TRUE;
}

static void
save_less_than_8 (FILE         *fp,
                  gint          width,
                  gint          height,
                  const gint    bpp,
                  const guchar *buf,
                  gboolean      padding)
{
  const gint  bit_limit     = (8 - bpp);
  const gint  buf_size      = width * height;
  const gint  line_end      = width - 1;
  gint        j             = bit_limit;
  gint        count         = 0;
  guchar      byte_to_write = 0x00;
  guchar     *line;
  gint        x;

  line = (guchar *) g_malloc (((width + 7) / 8) * bpp);

  for (x = 0; x < buf_size; x++)
    {
      byte_to_write |= (buf[x] << j);
      j -= bpp;

      if (j < 0 || (x % width == line_end))
        {
          line[count] = byte_to_write;
          count++;
          byte_to_write = 0x00;
          j = bit_limit;

          if ((x % width == line_end))
            {
              writeline (fp, line, count);
              count = 0;
              if (padding)
                fputc ('\0', fp);
              gimp_progress_update ((double) x / (double) buf_size);
            }
        }
    }
  g_free (line);
}

static void
save_8 (FILE         *fp,
        gint          width,
        gint          height,
        const guchar *buf,
        gboolean      padding)
{
  int row;

  for (row = 0; row < height; ++row)
    {
      writeline (fp, buf, width);
      buf += width;
      if (padding)
        fputc ('\0', fp);
      gimp_progress_update ((double) row / (double) height);
    }
}

static void
save_24 (FILE         *fp,
         gint          width,
         gint          height,
         const guchar *buf,
         gboolean      padding)
{
  int     x, y, c;
  guchar *line;

  line = (guchar *) g_malloc (width);

  for (y = 0; y < height; ++y)
    {
      for (c = 0; c < 3; ++c)
        {
          for (x = 0; x < width; ++x)
            {
              line[x] = buf[(3*x) + c];
            }
          writeline (fp, line, width);
          if (padding)
            fputc ('\0', fp);
        }
      buf += width * 3;
      gimp_progress_update ((double) y / (double) height);
    }
  g_free (line);
}

static void
writeline (FILE         *fp,
           const guchar *buf,
           gint          bytes)
{
  const guchar *finish = buf + bytes;
  guchar        value;
  guchar        count;

  while (buf < finish)
    {
      value = *(buf++);
      count = 1;

      while (buf < finish && count < 63 && *buf == value)
        {
          count++; buf++;
        }

      if (value < 0xc0 && count == 1)
        {
          fputc (value, fp);
        }
      else
        {
          fputc (0xc0 + count, fp);
          fputc (value, fp);
        }
    }
}
