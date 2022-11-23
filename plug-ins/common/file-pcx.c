/*
 * pcx.c LIGMA plug-in for loading & exporting PCX files
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define LOAD_PROC      "file-pcx-load"
#define SAVE_PROC      "file-pcx-save"
#define PLUG_IN_BINARY "file-pcx"
#define PLUG_IN_ROLE   "ligma-file-pcx"


typedef struct _Pcx      Pcx;
typedef struct _PcxClass PcxClass;

struct _Pcx
{
  LigmaPlugIn      parent_instance;
};

struct _PcxClass
{
  LigmaPlugInClass parent_class;
};


#define PCX_TYPE  (pcx_get_type ())
#define PCX (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PCX_TYPE, Pcx))

GType                   pcx_get_type         (void) G_GNUC_CONST;

static GList          * pcx_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * pcx_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * pcx_load             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * pcx_save             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);

static LigmaImage      * load_image           (GFile                *file,
                                              GError              **error);

static void             load_1               (FILE                 *fp,
                                              gint                  width,
                                              gint                  height,
                                              guchar               *buf,
                                              guint16               bytes);
static void             load_4               (FILE                 *fp,
                                              gint                  width,
                                              gint                  height,
                                              guchar               *buf,
                                              guint16               bytes);
static void             load_sub_8           (FILE                 *fp,
                                              gint                  width,
                                              gint                  height,
                                              gint                  bpp,
                                              gint                  plane,
                                              guchar               *buf,
                                              guint16               bytes);
static void             load_8               (FILE                 *fp,
                                              gint                  width,
                                              gint                  height,
                                              guchar               *buf,
                                              guint16               bytes);
static void             load_24              (FILE                 *fp,
                                              gint                  width,
                                              gint                  height,
                                              guchar               *buf,
                                              guint16               bytes);
static void             readline             (FILE                 *fp,
                                              guchar               *buf,
                                              gint                  bytes);

static gboolean         save_image           (GFile                *file,
                                              LigmaImage            *image,
                                              LigmaDrawable         *drawable,
                                              GError              **error);
static void             save_less_than_8     (FILE                 *fp,
                                              gint                  width,
                                              gint                  height,
                                              const gint            bpp,
                                              const guchar         *buf,
                                              gboolean              padding);
static void             save_8               (FILE                 *fp,
                                              gint                  width,
                                              gint                  height,
                                              const guchar         *buf,
                                              gboolean              padding);
static void             save_24              (FILE                 *fp,
                                              gint                  width,
                                              gint                  height,
                                              const guchar         *buf,
                                              gboolean              padding);
static void             writeline            (FILE                 *fp,
                                              const guchar         *buf,
                                              gint                  bytes);


G_DEFINE_TYPE (Pcx, pcx, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (PCX_TYPE)
DEFINE_STD_SET_I18N


static void
pcx_class_init (PcxClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = pcx_query_procedures;
  plug_in_class->create_procedure = pcx_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
pcx_init (Pcx *pcx)
{
}

static GList *
pcx_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static LigmaProcedure *
pcx_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           pcx_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("ZSoft PCX image"));

      ligma_procedure_set_documentation (procedure,
                                        "Loads files in Zsoft PCX file format",
                                        "FIXME: write help for pcx_load",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Francisco Bustamante & Nick Lamb",
                                      "Nick Lamb <njl195@zepler.org.uk>",
                                      "January 1997");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-pcx");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "pcx,pcc");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0&,byte,10,2&,byte,1,3&,byte,>0,3,byte,<9");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           pcx_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "INDEXED, RGB, GRAY");

      ligma_procedure_set_menu_label (procedure, _("ZSoft PCX image"));

      ligma_procedure_set_documentation (procedure,
                                        "Exports files in ZSoft PCX file format",
                                        "FIXME: write help for pcx_save",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Francisco Bustamante & Nick Lamb",
                                      "Nick Lamb <njl195@zepler.org.uk>",
                                      "January 1997");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-pcx");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "pcx,pcc");
    }

  return procedure;
}

static LigmaValueArray *
pcx_load (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaValueArray *
pcx_save (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          LigmaImage            *image,
          gint                  n_drawables,
          LigmaDrawable        **drawables,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaPDBStatusType  status = LIGMA_PDB_SUCCESS;
  LigmaExportReturn   export = LIGMA_EXPORT_CANCEL;
  GError            *error = NULL;

  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);

      export = ligma_export_image (&image, &n_drawables, &drawables, "PCX",
                                  LIGMA_EXPORT_CAN_HANDLE_RGB  |
                                  LIGMA_EXPORT_CAN_HANDLE_GRAY |
                                  LIGMA_EXPORT_CAN_HANDLE_INDEXED);

      if (export == LIGMA_EXPORT_CANCEL)
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("PCX format does not support multiple layers."));

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }

  if (! save_image (file,
                    image, drawables[0],
                    &error))
    {
      status = LIGMA_PDB_EXECUTION_ERROR;
    }

  if (export == LIGMA_EXPORT_EXPORT)
    {
      ligma_image_delete (image);
      g_free (drawables);
    }

  return ligma_procedure_new_return_values (procedure, status, error);
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

static LigmaImage *
load_image (GFile   *file,
            GError **error)
{
  FILE         *fd;
  GeglBuffer   *buffer;
  guint16       offset_x, offset_y, bytesperline;
  gint32        width, height;
  guint16       resolution_x, resolution_y;
  LigmaImage    *image;
  LigmaLayer    *layer;
  guchar       *dest, cmap[768];
  guint8        header_buf[128];

  ligma_progress_init_printf (_("Opening '%s'"),
                             ligma_file_get_utf8_name (file));

  fd = g_fopen (g_file_peek_path (file), "rb");

  if (! fd)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   ligma_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  if (fread (header_buf, 128, 1, fd) == 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read header from '%s'"),
                   ligma_file_get_utf8_name (file));
      fclose (fd);
      return NULL;
    }

  pcx_header_from_buffer (header_buf);

  if (pcx_header.manufacturer != 10)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a PCX file"),
                   ligma_file_get_utf8_name (file));
      fclose (fd);
      return NULL;
    }

  offset_x     = GUINT16_FROM_LE (pcx_header.x1);
  offset_y     = GUINT16_FROM_LE (pcx_header.y1);
  width        = GUINT16_FROM_LE (pcx_header.x2) - offset_x + 1;
  height       = GUINT16_FROM_LE (pcx_header.y2) - offset_y + 1;
  bytesperline = GUINT16_FROM_LE (pcx_header.bytesperline);
  resolution_x = GUINT16_FROM_LE (pcx_header.hdpi);
  resolution_y = GUINT16_FROM_LE (pcx_header.vdpi);

  if ((width <= 0) || (width > LIGMA_MAX_IMAGE_SIZE))
    {
      g_message (_("Unsupported or invalid image width: %d"), width);
      fclose (fd);
      return NULL;
    }
  if ((height <= 0) || (height > LIGMA_MAX_IMAGE_SIZE))
    {
      g_message (_("Unsupported or invalid image height: %d"), height);
      fclose (fd);
      return NULL;
    }
  if (bytesperline < ((width * pcx_header.bpp + 7) / 8))
    {
      g_message (_("Invalid number of bytes per line in PCX header"));
      fclose (fd);
      return NULL;
    }
  if ((resolution_x < 1) || (resolution_x > LIGMA_MAX_RESOLUTION) ||
      (resolution_y < 1) || (resolution_y > LIGMA_MAX_RESOLUTION))
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
      return NULL;
    }

  if (pcx_header.planes == 3 && pcx_header.bpp == 8)
    {
      image = ligma_image_new (width, height, LIGMA_RGB);
      layer = ligma_layer_new (image, _("Background"), width, height,
                              LIGMA_RGB_IMAGE,
                              100,
                              ligma_image_get_default_new_layer_mode (image));
    }
  else
    {
      image = ligma_image_new (width, height, LIGMA_INDEXED);
      layer = ligma_layer_new (image, _("Background"), width, height,
                              LIGMA_INDEXED_IMAGE,
                              100,
                              ligma_image_get_default_new_layer_mode (image));
    }

  ligma_image_set_file (image, file);
  ligma_image_set_resolution (image, resolution_x, resolution_y);

  ligma_image_insert_layer (image, layer, NULL, 0);
  ligma_layer_set_offsets (layer, offset_x, offset_y);

  buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

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
       * other software, including older versions of LIGMA, do display them
       * "correctly", so let's follow suit: if the two palette colors are
       * equal, use a B&W palette instead.
       */
      if (! memcmp (colormap, colormap + 3, 3))
        {
          static const guint8 bw_colormap[6] = {  0,   0,   0,
                                                255, 255, 255};
          colormap = bw_colormap;
        }
      ligma_image_set_colormap (image, colormap, 2);
    }
  else if (pcx_header.bpp == 1 && pcx_header.planes == 2)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 1, 2, dest, bytesperline);
      ligma_image_set_colormap (image, pcx_header.colormap, 4);
    }
  else if (pcx_header.bpp == 2 && pcx_header.planes == 1)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 2, 1, dest, bytesperline);
      ligma_image_set_colormap (image, pcx_header.colormap, 4);
    }
  else if (pcx_header.bpp == 1 && pcx_header.planes == 3)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 1, 3, dest, bytesperline);
      ligma_image_set_colormap (image, pcx_header.colormap, 8);
    }
  else if (pcx_header.bpp == 1 && pcx_header.planes == 4)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_4 (fd, width, height, dest, bytesperline);
      ligma_image_set_colormap (image, pcx_header.colormap, 16);
    }
  else if (pcx_header.bpp == 4 && pcx_header.planes == 1)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 4, 1, dest, bytesperline);
      ligma_image_set_colormap (image, pcx_header.colormap, 16);
    }
  else if (pcx_header.bpp == 8 && pcx_header.planes == 1)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_8 (fd, width, height, dest, bytesperline);
      fseek (fd, -768L, SEEK_END);
      fread (cmap, 768, 1, fd);
      ligma_image_set_colormap (image, cmap, 256);
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
      return NULL;
    }

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   NULL, dest, GEGL_AUTO_ROWSTRIDE);

  fclose (fd);
  g_free (dest);
  g_object_unref (buffer);

  ligma_progress_update (1.0);

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
      ligma_progress_update ((double) row / (double) height);
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
      ligma_progress_update ((double) y / (double) height);
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
      ligma_progress_update ((double) y / (double) height);
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
      ligma_progress_update ((double) y / (double) height);
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
      ligma_progress_update ((double) y / (double) height);
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

static gboolean
save_image (GFile         *file,
            LigmaImage     *image,
            LigmaDrawable  *drawable,
            GError       **error)
{
  FILE          *fp;
  GeglBuffer    *buffer;
  const Babl    *format;
  LigmaImageType  drawable_type;
  guchar        *cmap= NULL;
  guchar        *pixels;
  gint           offset_x, offset_y;
  guint          width, height;
  gdouble        resolution_x, resolution_y;
  gint           colors, i;
  guint8         header_buf[128];
  gboolean       padding = FALSE;

  drawable_type = ligma_drawable_type (drawable);
  ligma_drawable_get_offsets (drawable, &offset_x, &offset_y);

  buffer = ligma_drawable_get_buffer (drawable);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  ligma_progress_init_printf (_("Exporting '%s'"),
                             ligma_file_get_utf8_name (file));

  pcx_header.manufacturer = 0x0a;
  pcx_header.version      = 5;
  pcx_header.compression  = 1;

  switch (drawable_type)
    {
    case LIGMA_INDEXED_IMAGE:
      cmap                          = ligma_image_get_colormap (image, &colors);
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

    case LIGMA_RGB_IMAGE:
      pcx_header.bpp          = 8;
      pcx_header.planes       = 3;
      pcx_header.color        = GUINT16_TO_LE (1);
      pcx_header.bytesperline = width;
      format                  = babl_format ("R'G'B' u8");
      break;

    case LIGMA_GRAY_IMAGE:
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

  fp = g_fopen (g_file_peek_path (file), "wb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   ligma_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  pcx_header.x1 = GUINT16_TO_LE ((guint16)offset_x);
  pcx_header.y1 = GUINT16_TO_LE ((guint16)offset_y);
  pcx_header.x2 = GUINT16_TO_LE ((guint16)(offset_x + width - 1));
  pcx_header.y2 = GUINT16_TO_LE ((guint16)(offset_y + height - 1));

  ligma_image_get_resolution (image, &resolution_x, &resolution_y);

  pcx_header.hdpi = GUINT16_TO_LE (RINT (MAX (resolution_x, 1.0)));
  pcx_header.vdpi = GUINT16_TO_LE (RINT (MAX (resolution_y, 1.0)));
  pcx_header.reserved = 0;

  pcx_header_to_buffer (header_buf);

  fwrite (header_buf, 128, 1, fp);

  switch (drawable_type)
    {
    case LIGMA_INDEXED_IMAGE:
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

    case LIGMA_RGB_IMAGE:
      save_24 (fp, width, height, pixels, padding);
      break;

    case LIGMA_GRAY_IMAGE:
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
                   ligma_file_get_utf8_name (file), g_strerror (errno));
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
              ligma_progress_update ((double) x / (double) buf_size);
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
      ligma_progress_update ((double) row / (double) height);
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
      ligma_progress_update ((double) y / (double) height);
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
