/*   GIMP - The GNU Image Manipulation Program
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   PowerVR texture plug-in
 *
 *   Copyright (C) 2025 Alex S.
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
 */

/* Assistance with additional PVR formats provided by VincentNL and his PyPVR tool,
 * at https://github.com/VincentNLOBJ/PyPVR
 *
 * MIT License
 *
 * Copyright (c) 2024 VincentNL
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-pvr-load"
#define PLUG_IN_BINARY "file-pvr"
#define PLUG_IN_ROLE   "gimp-file-pvr"


typedef enum
{
  MODE_ARGB1555 = 0,
  MODE_RGB565   = 1,
  MODE_ARGB4444 = 2,
  MODE_YUV422   = 3,
  MODE_BUMPMAP  = 4,
  MODE_RGB555   = 5,
  MODE_ARGB8888 = 6
} PvrPixelMode;

typedef enum
{
  MODE_TWIDDLE               = 1,
  MODE_TWIDDLE_MIPMAP        = 2,
  MODE_COMPRESSED            = 3,
  MODE_COMPRESSED_MIPMAP     = 4,
  MODE_CLUT4                 = 5,
  MODE_CLUT4_MIPMAP          = 6,
  MODE_CLUT8                 = 7,
  MODE_CLUT8_MIPMAP          = 8,
  MODE_RECTANGLE             = 9,
  MODE_STRIDE                = 11,
  MODE_TWIDDLED_RECTANGLE    = 13,
  MODE_SMALL_VQ              = 16,
  MODE_SMALL_VQ_MIPMAP       = 17,
  MODE_TWIDDLED_ALIAS_MIPMAP = 18,
} PvrTextureMode;


typedef struct _Pvr      Pvr;
typedef struct _PvrClass PvrClass;

struct _Pvr
{
  GimpPlugIn      parent_instance;
};

struct _PvrClass
{
  GimpPlugInClass parent_class;
};


#define PVR_TYPE  (pvr_get_type ())
#define PVR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PVR_TYPE, Pvr))

GType                   pvr_get_type         (void) G_GNUC_CONST;


static GList          * pvr_query_procedures  (GimpPlugIn            *plug_in);
static GimpProcedure  * pvr_create_procedure  (GimpPlugIn            *plug_in,
                                               const gchar           *name);

static GimpValueArray * pvr_load              (GimpProcedure         *procedure,
                                               GimpRunMode            run_mode,
                                               GFile                 *file,
                                               GimpMetadata          *metadata,
                                               GimpMetadataLoadFlags *flags,
                                               GimpProcedureConfig   *config,
                                               gpointer               run_data);

static GimpImage      * load_image            (GFile                 *file,
                                               GObject               *config,
                                               GimpRunMode            run_mode,
                                               FILE                  *fp,
                                               GError               **error);

static void             pvr_create_twiddle    (gint                  *twiddle);

static gboolean         pvr_decode_color      (gint                   pixel_mode,
                                               gint                   color,
                                               guchar                *pixels,
                                               gint                   index);

static gboolean         pvr_decode_rect       (GimpLayer             *layer,
                                               gint                   pixel_mode,
                                               gushort                width,
                                               gushort                height,
                                               gint                   n_components,
                                               FILE                  *fp,
                                               GError               **error);

static gboolean         pvr_decode_twiddle    (GimpLayer             *layer,
                                               gint                   pixel_mode,
                                               gushort                width,
                                               gushort                height,
                                               gint                   n_components,
                                               gint                   mipmap_offset,
                                               guchar                *data,
                                               GError               **error);

static gboolean         pvr_decode_compressed (GimpLayer             *layer,
                                               gint                   pixel_mode,
                                               gushort                width,
                                               gushort                height,
                                               gint                   n_components,
                                               gint                   mipmap_offset,
                                               guchar                *code_data,
                                               guchar                *data,
                                               GError               **error);

G_DEFINE_TYPE (Pvr, pvr, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PVR_TYPE)
DEFINE_STD_SET_I18N


static void
pvr_class_init (PvrClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = pvr_query_procedures;
  plug_in_class->create_procedure = pvr_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
pvr_init (Pvr *pvr)
{
}

static GList *
pvr_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static GimpProcedure *
pvr_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           pvr_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("PVR Image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Load file in the PVR file format"),
                                        _("Load file in the PowerVR texture "
                                          "file format"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Per Hedbor",
                                      "Per Hedbor",
                                      "2000");

      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "pvr");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,GBIX,0,string,PVRT");
    }

  return procedure;
}

static GimpValueArray *
pvr_load (GimpProcedure         *procedure,
          GimpRunMode            run_mode,
          GFile                 *file,
          GimpMetadata          *metadata,
          GimpMetadataLoadFlags *flags,
          GimpProcedureConfig   *config,
          gpointer               run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  FILE           *fp;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (&error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      fclose (fp);
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }

  image = load_image (file, G_OBJECT (config), run_mode, fp, &error);
  fclose (fp);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpImage *
load_image (GFile        *file,
            GObject      *config,
            GimpRunMode   run_mode,
            FILE         *fp,
            GError      **error)
{
  GimpImage  *image       = NULL;
  GimpLayer  *layer       = NULL;
  gboolean    has_alpha   = FALSE;
  gboolean    has_mipmaps = FALSE;
  gsize       file_size;
  gsize       file_start;
  gchar       magic[5];
  guint32     header_file_size;
  guchar      pixel_mode;
  guchar      data_format;
  gushort     width;
  gushort     height;

  /* Get file size for error handling. */
  file_start = ftell (fp);
  fseek (fp, 0, SEEK_END);
  file_size = ftell (fp) - file_start;
  fseek (fp, file_start, SEEK_SET);

  if (fread (magic, sizeof (gchar), 4, fp) != 4)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read header from '%s'"),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  magic[4] = '\0';
  /* Skip global header if it exists */
  if (g_strcmp0 (magic, "GBIX") == 0)
    {
      gint global_header_size = 0;

      if (fread (&global_header_size, sizeof (gint), 1, fp) != 1)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Could not read GBIX header from '%s'"),
                       gimp_file_get_utf8_name (file));
          return NULL;
        }

      fseek (fp, global_header_size, SEEK_CUR);

      if (fread (magic, sizeof (gchar), 4, fp) != 4)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Could not read GBIX header from '%s'"),
                       gimp_file_get_utf8_name (file));
          return NULL;
        }

      file_size -= 16;
    }
  else if (g_strcmp0 (magic, "PVRT") != 0)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Unsupported PVR format: %s"), magic);
      return NULL;
    }

  /* Read rest of PVR header */
  if (fread (&header_file_size, sizeof (guint32), 1, fp) != 1 ||
      fread (&pixel_mode, sizeof (guchar), 1, fp) != 1        ||
      fread (&data_format, sizeof (guchar), 1, fp) != 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read header from '%s'"),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  /* Skip two empty bytes */
  fseek (fp, 2, SEEK_CUR);

  if (fread (&width, sizeof (gushort), 1, fp) != 1 ||
      fread (&height, sizeof (gushort), 1, fp) != 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read header from '%s'"),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  file_size        -= 16;
  header_file_size -= 8;

  /* If header file size is wrong, attempt to load correctly */
  if (header_file_size > (gint) file_size)
    header_file_size = (gint) file_size;

  if (pixel_mode == MODE_ARGB1555 || pixel_mode == MODE_ARGB4444)
    has_alpha = TRUE;

  if (data_format == MODE_TWIDDLE_MIPMAP    ||
      data_format == MODE_COMPRESSED_MIPMAP ||
      data_format == MODE_SMALL_VQ_MIPMAP   ||
      data_format == MODE_TWIDDLED_ALIAS_MIPMAP)
    has_mipmaps = TRUE;

  image = gimp_image_new (width, height, GIMP_RGB);

  /* Load layer(s) based on PVR texture mode */
  if (data_format == MODE_TWIDDLE            ||
      data_format == MODE_TWIDDLED_RECTANGLE ||
      data_format == MODE_TWIDDLE_MIPMAP     ||
      data_format == MODE_TWIDDLED_ALIAS_MIPMAP)
    {
      gint    mipmap_width  = 1;
      gint    mipmap_height = 1;
      gint    mipmap_offset = 1;
      gint    n_components  = has_alpha ? 4 : 3;
      guchar *data;

      data = g_malloc0 (header_file_size);
      /* Read rest of data */
      if (fread (data, sizeof (guchar),
                 header_file_size, fp) != header_file_size)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Incomplete PVR header data."));
          return NULL;
        }

      /* If we don't have mipmaps, set layer to image size */
      if (! has_mipmaps)
        {
          mipmap_width  = width;
          mipmap_height = height;
          mipmap_offset = 0;
        }

      /* This format has a larger offset for some reason */
      if (data_format == MODE_TWIDDLED_ALIAS_MIPMAP)
        mipmap_offset *= 3;

      for (gint m = 1; m <= width; m <<= 1)
        {
          gchar *layer_name = NULL;

          if (has_mipmaps)
            layer_name = g_strdup_printf ("Mipmap (%dx%d)",
                                          mipmap_width, mipmap_height);

          layer = gimp_layer_new (image, layer_name, mipmap_width, mipmap_height,
                                  (has_alpha ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE),
                                  100,
                                  gimp_image_get_default_new_layer_mode (image));
          gimp_image_insert_layer (image, layer, NULL, 0);
          g_free (layer_name);

          if (! pvr_decode_twiddle (layer, pixel_mode, mipmap_width,
                                    mipmap_height, n_components,
                                    mipmap_offset * 2, data, error))
            {
              g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Unable to decode twiddled PVR texture"));
              return NULL;
            }

          mipmap_offset += m * m;
          mipmap_width  *= 2;
          mipmap_height *= 2;

          /* Stop loading textures without mipmaps after first one */
          if (! has_mipmaps)
            break;
        }
      g_free (data);
    }
  else if (data_format == MODE_RECTANGLE)
    {
      gint n_components  = has_alpha ? 4 : 3;

      layer = gimp_layer_new (image, NULL, width, height,
                              (has_alpha ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE),
                              100, gimp_image_get_default_new_layer_mode (image));
      gimp_image_insert_layer (image, layer, NULL, 0);

      if (! pvr_decode_rect (layer, pixel_mode, width, height, n_components,
                             fp, error))
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Unable to decode PVR texture"));
          return NULL;
        }
    }
  else if (data_format == MODE_COMPRESSED        ||
           data_format == MODE_SMALL_VQ          ||
           data_format == MODE_COMPRESSED_MIPMAP ||
           data_format == MODE_SMALL_VQ_MIPMAP)
    {
      gint   n_components   = has_alpha ? 4 : 3;
      gint   code_data_size = 256 * 2 * 4;
      gint   mipmap_width   = (gint) pow (2, (3));
      gint   mipmap_height  = (gint) pow (2, (3));
      gint   mipmap_offset  = 6;
      guchar code_data[code_data_size];
      gsize  data_start;

      /* Small VQ format can have a smaller codebook size */
      if (data_format == MODE_SMALL_VQ ||
          data_format == MODE_SMALL_VQ_MIPMAP)
        {
          if (width == 32)
            code_data_size = 64 * 2 * 4;
          else if (width <= 16)
            code_data_size = 16 * 2 * 4;
        }

      /* Pixels are stored in the codebook in 2x2 format */
      if (fread (code_data, sizeof (guchar),
                 code_data_size, fp) != code_data_size)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Incomplete PVR codebook data."));
          return NULL;
        }

      /* If we don't have mipmaps, set layer to image size */
      if (! has_mipmaps)
        {
          mipmap_width  = width;
          mipmap_height = height;
          mipmap_offset = 0;
        }

      header_file_size -= code_data_size;

      data_start = ftell (fp);
      for (gint i = 0; i < 8; i++)
        {
          guchar  data[header_file_size - mipmap_offset];
          gint    temp_size   = header_file_size;
          gint    mipmap_size = 0x10 << (i * 2);
          gchar  *layer_name  = NULL;;

          if (has_mipmaps)
            layer_name = g_strdup_printf ("Mipmap (%dx%d)",
                                          mipmap_width, mipmap_height);

          layer = gimp_layer_new (image, layer_name, mipmap_width, mipmap_height,
                                  (has_alpha ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE),
                                  100,
                                  gimp_image_get_default_new_layer_mode (image));
          gimp_image_insert_layer (image, layer, NULL, 0);
          g_free (layer_name);

          fseek (fp, data_start, SEEK_SET);
          temp_size -= mipmap_offset;
          fseek (fp, mipmap_offset, SEEK_CUR);

          /* Read rest of data */
          if (fread (data, sizeof (guchar), temp_size, fp) != temp_size)
            {
              g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Incomplete PVR data."));
              return NULL;
            }

          if (! pvr_decode_compressed (layer, pixel_mode, width, height,
                                       n_components, mipmap_offset, code_data,
                                       data, error))
            {
              g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Unable to decode compressed PVR texture"));
              return NULL;
            }

          if (mipmap_width == width || ! has_mipmaps)
            break;

          mipmap_width  = (gint) pow (2, (3 + i + 1));
          mipmap_height = (gint) pow (2, (3 + i + 1));

          mipmap_offset += mipmap_size;
        }
    }
  else
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Unsupported PVR texture format: %d"), data_format);
      return NULL;
    }

  return image;
}

static void
pvr_create_twiddle (gint *twiddle)
{
  for (gint i = 0; i < 1024; i++)
    {
      twiddle[i] = i & 1;

      for (gint j = 1; j < 10; j++)
        twiddle[i] |= (i & ((gint) pow (2, j))) << j;
    }
}

static gboolean
pvr_decode_color (gint    pixel_mode,
                  gint    color,
                  guchar *pixels,
                  gint    index)
{
  switch (pixel_mode)
    {
    case MODE_ARGB1555:
      pixels[index] = ((color & 0x7c00) >> 7) | ((color & 0x7000) >> 12);
      pixels[index + 1] = ((color & 0x03e0) >> 2) | ((color & 0x0380) >> 7);
      pixels[index + 2] = ((color & 0x001f) << 3) | ((color & 0x001c) >> 2);

      if (((color >> 15) & 1) != 0)
        pixels[index + 3] = 255;
      else
        pixels[index + 3] = 0;
      break;

    case MODE_RGB565:
      pixels[index] = ((color & 0xf800) >> 8) | ((color & 0xe000) >> 13);
      pixels[index + 1] = ((color & 0x07e0)>>3) | ((color & 0x0600) >> 9);
      pixels[index + 2] = ((color & 0x001f)<<3) | ((color & 0x001c) >> 2);
      break;

    case MODE_ARGB4444:
	  pixels[index] = ((color & 0x0f00) >> 4)|((color & 0x0f00) >> 8);
	  pixels[index + 1] = (color & 0x00f0) | ((color & 0x00f0) >> 4);
	  pixels[index + 2] = ((color & 0x000f) << 4)|(color & 0x000f);

      pixels[index + 3] = ((color & 0xf000) >> 4) | ((color & 0xf000) >> 8);
      break;

    case MODE_RGB555:
      pixels[index] = ((color & 0x7c00) >> 7) | ((color & 0x7000) >> 12);
      pixels[index + 1] = ((color & 0x03e0) >> 2) | ((color & 0x0380) >> 7);
      pixels[index + 2] = ((color & 0x001f) << 3) | ((color & 0x001c) >> 2);
      break;

    case MODE_BUMPMAP:
      {
        gdouble xyz[3];
        gdouble radius = color & 0xFF;
        gdouble arc    = color >> 8;

        radius = gimp_deg_to_rad ((radius / 255) * 360);
        arc    = gimp_deg_to_rad ((arc / 255) * 90);

        xyz[0] = cos (radius) * cos (arc);
        xyz[1] = sin (radius) * cos (arc);
        xyz[2] = sin (arc);

        for (gint i = 0; i < 3; i++)
          pixels[index + i] = (guchar) ((0.5f * xyz[i] + 0.5f) * 255);
        pixels[3] = 255;
      }
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

static gboolean
pvr_decode_rect (GimpLayer  *layer,
                 gint        pixel_mode,
                 gushort     width,
                 gushort     height,
                 gint        n_components,
                 FILE       *fp,
                 GError    **error)
{
  GeglBuffer *buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
  gint        count  = width * height * 2;
  guchar      pixels[width * height * n_components];
  guchar      data[count];

  if (fread (data, sizeof (guchar), count, fp) != count)
    return FALSE;

  for (gint index = 0; index < count; index += 2)
    {
      guint rgb = data[index] | (data[index + 1] << 8);
      gint  i   = (index / 2) * n_components;

      if (! pvr_decode_color (pixel_mode, rgb, pixels, i))
        {
          g_object_unref (buffer);
          return FALSE;
        }
    }
  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   NULL, pixels, GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);

  return TRUE;
}

static gboolean
pvr_decode_twiddle (GimpLayer  *layer,
                    gint        pixel_mode,
                    gushort     width,
                    gushort     height,
                    gint        n_components,
                    gint        mipmap_offset,
                    guchar     *data,
                    GError    **error)
{
  gint        twiddle[1024];
  GeglBuffer *buffer   = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
  guint       end      = 0;
  gint        distance = 0;
  gint        stride   = 0;
  gint        offset   = 0;
  guchar     *pixels;

  pixels = g_malloc0 (width * height * n_components);

  /* Initialize twiddle look up table */
  pvr_create_twiddle (twiddle);

  if (height < width)
    {
      end      = width;
      distance = height;
      stride   = width - height;
    }
  else
    {
      end      = height;
      distance = width;
    }

  for (gint index = 0; index < end; index += distance)
    {
      gint base = 2 * index * distance; /* For non-square textures */

      if (height < width)
        offset = index * n_components;
      else
        offset = (width * index) * n_components;

      for (gint y = 0; y < distance; y++)
        {
          for (gint x = 0; x < distance; x++)
            {
              guint   p;
              gint    twiddle_index = (((twiddle[x] << 1) | twiddle[y]) << 1);
              gsize   offset2       = twiddle_index + base;

              offset2 += mipmap_offset;

              p = data[offset2] | (data[offset2 + 1] << 8);
              if (! pvr_decode_color (pixel_mode, p, pixels, offset))
                {
                  g_object_unref (buffer);
                  return FALSE;
                }
              offset += n_components;
            }
          offset += (n_components * stride);
        }
    }

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   NULL, pixels, GEGL_AUTO_ROWSTRIDE);
  g_object_unref (buffer);
  g_free (pixels);

  return TRUE;
}

static gboolean
pvr_decode_compressed (GimpLayer  *layer,
                       gint        pixel_mode,
                       gushort     width,
                       gushort     height,
                       gint        n_components,
                       gint        mipmap_offset,
                       guchar     *code_data,
                       guchar     *data,
                       GError    **error)
{
  gint        code_data_size = 256 * 2 * 4;
  guchar      codebook[256 * 4 * n_components];
  gint        twiddle[1024];
  GeglBuffer *buffer;
  guchar      pixels[width * height * n_components];

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  /* Initialize twiddle look up table */
  pvr_create_twiddle (twiddle);

  for (gint i = 0; i < code_data_size; i += 2)
    {
      guint rgb  = code_data[i] | (code_data[i + 1] << 8);
      gint index = (i / 2) * n_components;

      if (! pvr_decode_color (pixel_mode, rgb, codebook, index))
        {
          g_object_unref (buffer);
          return FALSE;
        }
    }

  /* Twiddled values are indexes */
  for (gint y = 0; y < (width / 2); y++)
    {
      gint offset = ((y * 2) * width * n_components);

      for (gint x = 0; x < (width / 2); x++)
        {
          gint twiddle_index = (((twiddle[x] << 1) | twiddle[y]));
          gint p;
          gint code_offset;

          p = data[twiddle_index];

          /* Account for 4 pixels per offset */
          code_offset = p * n_components * 4;

          for (gint i = 0; i < n_components; i++)
            pixels[offset + i] = codebook[code_offset + i];

          for (gint i = 0; i < n_components; i++)
            pixels[offset + i + (width * n_components)] =
              codebook[code_offset + i + n_components];

          for (gint i = 0; i < n_components; i++)
            pixels[offset + n_components + i] =
              codebook[code_offset + i + (n_components * 2)];

          for (gint i = 0; i < n_components; i++)
            pixels[offset + n_components + i + (width * n_components)] =
              codebook[code_offset + i + (n_components * 3)];

          offset += (n_components * 2);
        }
    }

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   NULL, pixels, GEGL_AUTO_ROWSTRIDE);
  g_object_unref (buffer);

  return TRUE;
}
