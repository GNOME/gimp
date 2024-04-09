/*
 * DDS GIMP plugin
 *
 * Copyright (C) 2004-2012 Shawn Kirst <skirst@gmail.com>,
 * with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor
 * Boston, MA 02110-1301, USA.
 */

/*
 ** !!! COPYRIGHT NOTICE !!!
 **
 ** The following is based on code (C) 2003 Arne Reuter <homepage@arnereuter.de>
 ** URL: http://www.dr-reuter.de/arne/dds.html
 **
 */

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <libgimp/stdplugins-intl.h>

#include "dds.h"
#include "ddsread.h"
#include "dxt.h"
#include "endian_rw.h"
#include "formats.h"
#include "imath.h"
#include "misc.h"


/*
 * Struct containing all info needed to parse the file.
 * This can be thought of as a version-agnostic header,
 * holding all relevant data from the two headers
 * plus some GIMP-specific information.
 */
typedef struct
{
  fmt_read_info_t       read_info;
  gchar                 fourcc[4];
  gchar                 gimp_fourcc[4];
  guint                 flags;
  guint                 fmt_flags;
  guint                 bpp;
  guint                 gimp_bpp;
  DXGI_FORMAT           dxgi_format;
  D3DFORMAT             d3d9_format;
  DDS_COMPRESSION_TYPE  comp_format;
  guint                 width;
  guint                 height;
  gint                  tile_height;
  gsize                 linear_size;
  gsize                 pitch;
  guint                 mipmaps;
  guint                 volume_slices;
  guint                 array_items;
  guint                 cubemap_faces;
  guint                 gimp_version;
  guchar               *palette;
} dds_load_info_t;


static gboolean      read_header          (dds_header_t         *hdr,
                                           FILE                 *fp);
static gboolean      read_header_dx10     (dds_header_dx10_t    *hdr,
                                           FILE                 *fp);
static gboolean      validate_header      (dds_header_t         *hdr,
                                           GError              **error);
static gboolean      validate_dx10_header (dds_header_dx10_t    *dx10hdr,
                                           dds_load_info_t      *load_info,
                                           GError              **error);
static gboolean      load_layer           (FILE                 *fp,
                                           dds_load_info_t      *load_info,
                                           GimpImage            *image,
                                           guint                 level,
                                           gchar                *prefix,
                                           guint                *layer_index,
                                           guchar               *pixels,
                                           guchar               *buf,
                                           GError              **error);
static gboolean      load_mipmaps         (FILE                 *fp,
                                           dds_load_info_t      *load_info,
                                           GimpImage            *image,
                                           gchar                *prefix,
                                           guint                *layer_index,
                                           guchar               *pixels,
                                           guchar               *buf,
                                           gboolean              read_mipmaps,
                                           GError              **error);
static gboolean      load_face            (FILE                 *fp,
                                           dds_load_info_t      *load_info,
                                           GimpImage            *image,
                                           gchar                *prefix,
                                           guint                *layer_index,
                                           guchar               *pixels,
                                           guchar               *buf,
                                           gboolean              read_mipmaps,
                                           GError              **error);
static gboolean      load_dialog          (GimpProcedure        *procedure,
                                           GimpProcedureConfig  *config);


/* Read DDS file */
GimpPDBStatusType
read_dds (GFile                *file,
          GimpImage           **ret_image,
          gboolean              interactive,
          GimpProcedure        *procedure,
          GimpProcedureConfig  *config,
          GError              **error)
{
  GimpImage         *image       = NULL;
  guint              layer_index = 0;
  guchar            *buf, *pixels;
  FILE              *fp;
  gsize              file_size;
  dds_header_t       hdr;
  dds_header_dx10_t  dx10hdr;
  dds_load_info_t    load_info;
  GList             *layers;
  GimpImageBaseType  type;
  GimpPrecision      precision = GIMP_PRECISION_U8_NON_LINEAR;
  gboolean           read_mipmaps;
  gboolean           flip_import;
  gint               i, j;

  if (interactive)
    {
      gimp_ui_init ("dds");

      if (! load_dialog (procedure, config))
        return GIMP_PDB_CANCEL;
    }

  g_object_get (config,
                "load-mipmaps", &read_mipmaps,
                "flip-image",   &flip_import,
                NULL);

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* Get total file size to compare against header info later */
  fseek (fp, 0L, SEEK_END);
  file_size = ftell (fp);
  fseek (fp, 0L, SEEK_SET);

  gimp_progress_init_printf (_("Loading: %s"), gimp_file_get_utf8_name (file));

  /* Read standard header */
  memset (&hdr, 0, sizeof (dds_header_t));
  read_header (&hdr, fp);

  /* Check that header is actually valid */
  if (! validate_header (&hdr, error))
    {
      fclose (fp);
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* Initialize load_info with data from header */
  memset (&load_info, 0, sizeof (dds_load_info_t));
  PUTL32 (load_info.fourcc, GETL32 (hdr.pixelfmt.fourcc));
  load_info.flags         = hdr.flags;
  load_info.fmt_flags     = hdr.pixelfmt.flags;
  load_info.width         = hdr.width;
  load_info.height        = hdr.height;
  load_info.gimp_version  = hdr.reserved.gimp_dds_special.version;
  PUTL32 (load_info.gimp_fourcc, hdr.reserved.gimp_dds_special.extra_fourcc);

  /* Get D3DFORMAT directly from FourCC if present there,
   * otherwise find it based on provided bpp, masks, and flags */
  if ((load_info.fmt_flags & DDPF_FOURCC) && (load_info.fourcc[1] == 0))
    load_info.d3d9_format = GETL32 (load_info.fourcc);
  else
    load_info.d3d9_format = get_d3d9format (hdr.pixelfmt.bpp,
                                            hdr.pixelfmt.rmask,
                                            hdr.pixelfmt.gmask,
                                            hdr.pixelfmt.bmask,
                                            hdr.pixelfmt.amask,
                                            hdr.pixelfmt.flags);

  /* Read DX10 header if present */
  memset (&dx10hdr, 0, sizeof (dds_header_dx10_t));
  if (GETL32 (load_info.fourcc) == FOURCC ('D','X','1','0'))
    {
      read_header_dx10 (&dx10hdr, fp);

      /* Check that DX10 header is actually valid */
      if (! validate_dx10_header (&dx10hdr, &load_info, error))
        {
          fclose (fp);
          return GIMP_PDB_EXECUTION_ERROR;
        }
    }

  /* If format search was successful, get info needed to parse the file */
  if (load_info.d3d9_format || load_info.dxgi_format)
    {
      load_info.read_info = get_format_read_info (load_info.d3d9_format,
                                                  load_info.dxgi_format);

      if ((! hdr.pixelfmt.bpp) && load_info.d3d9_format)
        hdr.pixelfmt.bpp = get_bpp_d3d9 (load_info.d3d9_format);
      else if (load_info.dxgi_format)
        hdr.pixelfmt.bpp = get_bpp_dxgi (load_info.dxgi_format);

      /* Unset the FourCC flag as D3D formats will be handled as uncompressed */
      if ((load_info.fmt_flags & DDPF_FOURCC) && load_info.d3d9_format)
        load_info.fmt_flags &= ~DDPF_FOURCC;
    }

  /* Exit if uncompressed format could not be determined by any method */
  if ((! (load_info.fmt_flags & DDPF_FOURCC)) &&
      (! (load_info.d3d9_format || load_info.dxgi_format)))
    {
      fclose (fp);
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported DDS pixel format:\n"
                     "bpp: %d, Rmask: %x, Gmask: %x, Bmask: %x, Amask: %x, flags: %u"),
                   hdr.pixelfmt.bpp,
                   hdr.pixelfmt.rmask, hdr.pixelfmt.gmask,
                   hdr.pixelfmt.bmask, hdr.pixelfmt.amask,
                   hdr.pixelfmt.flags);
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* If compressed, determine the format used */
  if (load_info.fmt_flags & DDPF_FOURCC)
    {
      if (GETL32 (load_info.fourcc) == FOURCC ('D','X','1','0'))
        {
          /* Compression type from DXGI format */
          switch (dx10hdr.dxgiFormat)
            {
            case DXGI_FORMAT_BC1_TYPELESS:
            case DXGI_FORMAT_BC1_UNORM:
            case DXGI_FORMAT_BC1_UNORM_SRGB:
              load_info.comp_format = DDS_COMPRESS_BC1;
              break;
            case DXGI_FORMAT_BC2_TYPELESS:
            case DXGI_FORMAT_BC2_UNORM:
            case DXGI_FORMAT_BC2_UNORM_SRGB:
              load_info.comp_format = DDS_COMPRESS_BC2;
              break;
            case DXGI_FORMAT_BC3_TYPELESS:
            case DXGI_FORMAT_BC3_UNORM:
            case DXGI_FORMAT_BC3_UNORM_SRGB:
              load_info.comp_format = DDS_COMPRESS_BC3;
              break;
            case DXGI_FORMAT_BC4_TYPELESS:
            case DXGI_FORMAT_BC4_UNORM:
            case DXGI_FORMAT_BC4_SNORM:
              load_info.comp_format = DDS_COMPRESS_BC4;
              break;
            case DXGI_FORMAT_BC5_TYPELESS:
            case DXGI_FORMAT_BC5_UNORM:
            case DXGI_FORMAT_BC5_SNORM:
              load_info.comp_format = DDS_COMPRESS_BC5;
              break;
            default:
              load_info.comp_format = DDS_COMPRESS_MAX;
              break;
            }
        }
      else
        {
          /* Compression type from FourCC */
          switch (GETL32 (load_info.fourcc))
            {
            case FOURCC ('D','X','T','1'):
              load_info.comp_format = DDS_COMPRESS_BC1;
              break;
            case FOURCC ('D','X','T','2'):
            case FOURCC ('D','X','T','3'):
              load_info.comp_format = DDS_COMPRESS_BC2;
              break;
            case FOURCC ('D','X','T','4'):
            case FOURCC ('D','X','T','5'):
            case FOURCC ('R','X','G','B'):
              load_info.comp_format = DDS_COMPRESS_BC3;
              break;
            case FOURCC ('A','T','I','1'):
            case FOURCC ('B','C','4','U'):
            case FOURCC ('B','C','4','S'):
              load_info.comp_format = DDS_COMPRESS_BC4;
              break;
            case FOURCC ('A','T','I','2'):
            case FOURCC ('B','C','5','U'):
            case FOURCC ('B','C','5','S'):
              load_info.comp_format = DDS_COMPRESS_BC5;
              break;
            default:
              load_info.comp_format = DDS_COMPRESS_MAX;
              break;
            }
        }
    }

  /* Determine resource type (cubemap, volume, array) and number of mipmaps.
   * Filling in these variables conditionally here simplifies some checks later */
  if (load_info.dxgi_format)
    {
      if (dx10hdr.resourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE3D)
        load_info.volume_slices = hdr.depth;

      if ((dx10hdr.resourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE2D) &&
          (dx10hdr.miscFlag & D3D10_RESOURCE_MISC_TEXTURECUBE))
        load_info.cubemap_faces = DDSCAPS2_CUBEMAP_ALL_FACES;

      load_info.array_items = dx10hdr.arraySize;
    }
  else
    {
      /* This and the mipmap check below were originally AND, not OR,
       * but some images out there only have one of these two flags,
       * so for compatibility's sake we take the more lenient route */
      if ((hdr.caps.caps2 & DDSCAPS2_VOLUME) ||
          (load_info.flags & DDSD_DEPTH))
        load_info.volume_slices = hdr.depth;

      load_info.cubemap_faces = hdr.caps.caps2 & DDSCAPS2_CUBEMAP_ALL_FACES;
    }
  if ((hdr.caps.caps1 & DDSCAPS_MIPMAP) ||
      (load_info.flags & DDSD_MIPMAPCOUNT))
    load_info.mipmaps = hdr.num_mipmaps;

  /* Historically many DDS exporters haven't set pitch/linearsize and the corresponding flags,
   * or set them incorrectly, so it's more reliable to always compute these manually.
   * See: https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide
   */
  if (load_info.fmt_flags & DDPF_FOURCC)
    {
      if (hdr.flags & DDSD_PITCH)
        {
          g_printerr ("Warning: DDSD_PITCH is incorrectly set for DDPF_FOURCC! (recovered)\n");
          load_info.flags &= ~DDSD_PITCH;
        }
      if (! (hdr.flags & DDSD_LINEARSIZE))
        {
          g_printerr ("Warning: DDSD_LINEARSIZE is incorrectly not set for DDPF_FOURCC! (recovered)\n");
          load_info.flags |= DDSD_LINEARSIZE;
        }

      load_info.pitch = MAX (1, (hdr.width + 3) >> 2);

      if (load_info.comp_format == DDS_COMPRESS_BC1 ||
          load_info.comp_format == DDS_COMPRESS_BC4)
        {
          load_info.pitch *= 8;
        }
      else
        {
          load_info.pitch *= 16;
        }

      load_info.linear_size = MAX (1, (hdr.height + 3) >> 2) * load_info.pitch;

      if (load_info.linear_size != hdr.pitch_or_linsize)
        {
          g_printerr ("Unexpected linear size (%u) set to %u\n",
                      hdr.pitch_or_linsize, (guint32) load_info.linear_size);
        }
    }
  else
    {
      if (! (hdr.flags & DDSD_PITCH))
        {
          g_printerr ("Warning: DDSD_PITCH is incorrectly not set for an uncompressed texture! (recovered)\n");
          load_info.flags |= DDSD_PITCH;
        }
      if ((hdr.flags & DDSD_LINEARSIZE))
        {
          g_printerr ("Warning: DDSD_LINEARSIZE is incorrectly set for an uncompressed texture! (recovered)\n");
          load_info.flags &= ~DDSD_LINEARSIZE;
        }

      load_info.pitch = (hdr.width * hdr.pixelfmt.bpp + 7) >> 3;

      if (load_info.pitch != hdr.pitch_or_linsize)
        {
          g_printerr ("Unexpected pitch (%u) set to %u\n",
                      hdr.pitch_or_linsize, (guint32) load_info.pitch);
        }

      load_info.linear_size = load_info.pitch * hdr.height;
    }

  /* Determine bytes-per-pixel and GIMP type needed */
  if (load_info.fmt_flags & DDPF_FOURCC)
    {
      /* Compressed */
      switch (load_info.comp_format)
        {
        case DDS_COMPRESS_BC4:
          load_info.bpp = load_info.gimp_bpp = 1;  /* Gray */
          type = GIMP_GRAY;
          break;
        case DDS_COMPRESS_BC5:
          load_info.bpp = load_info.gimp_bpp = 3;  /* RGB */
          type = GIMP_RGB;
          break;
        default:
          load_info.bpp = load_info.gimp_bpp = 4;  /* RGBA */
          type = GIMP_RGB;
          break;
        }

      precision = GIMP_PRECISION_U8_NON_LINEAR;
    }
  else
    {
      /* Uncompressed */
      load_info.bpp = hdr.pixelfmt.bpp >> 3;
      type = load_info.read_info.gimp_type;

      /* Set up GIMP bytes-per-pixel */
      if (load_info.read_info.gimp_type == GIMP_INDEXED)
        {
          load_info.gimp_bpp = 1;

          if (load_info.read_info.use_alpha)
            load_info.gimp_bpp += 1;
        }
      else
        {
          if (load_info.read_info.gimp_type == GIMP_RGB)
            load_info.gimp_bpp = 3;
          else  /* load_info.read_info.gimp_type == GIMP_GRAY */
            load_info.gimp_bpp = 1;

          if (load_info.read_info.use_alpha)
            load_info.gimp_bpp += 1;

          if (load_info.read_info.output_bit_depth == 16)
            load_info.gimp_bpp *= 2;
          else if (load_info.read_info.output_bit_depth == 32)
            load_info.gimp_bpp *= 4;
        }

      /* Set up canvas precision */
      if (load_info.read_info.output_bit_depth == 8)
        {
          precision = GIMP_PRECISION_U8_NON_LINEAR;
        }
      else if (load_info.read_info.output_bit_depth == 16)
        {
          if (load_info.read_info.is_float)
            precision = GIMP_PRECISION_HALF_LINEAR;
          else
            precision = GIMP_PRECISION_U16_NON_LINEAR;
        }
      else if (load_info.read_info.output_bit_depth == 32)
        {
          if (load_info.read_info.is_float)
            precision = GIMP_PRECISION_FLOAT_LINEAR;
          else
            precision = GIMP_PRECISION_U32_NON_LINEAR;
        }
    }

  /* Verify header information is accurate to avoid allocating more memory than is actually needed */
  if (load_info.bpp < 1 ||
      (load_info.linear_size > (file_size - sizeof (hdr))))
    {
      fclose (fp);
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Invalid or corrupted DDS header."));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* Generate GIMP image with set precision */
  image = gimp_image_new_with_precision (load_info.width,
                                         load_info.height,
                                         type, precision);

  if (! image)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOMEM,
                   _("Could not allocate a new image."));
      fclose (fp);
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* Read palette for indexed DDS */
  if (load_info.fmt_flags & DDPF_PALETTEINDEXED8)
    {
      load_info.palette = g_malloc (256 * 4);
      if (fread (load_info.palette, 1, 1024, fp) != 1024)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error reading palette."));
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }
      for (i = j = 0; i < 768; i += 3, j += 4)
        {
          load_info.palette[i + 0] = load_info.palette[j + 0];
          load_info.palette[i + 1] = load_info.palette[j + 1];
          load_info.palette[i + 2] = load_info.palette[j + 2];
        }
      gimp_image_set_colormap (image, load_info.palette, 256);
    }

  load_info.tile_height = gimp_tile_height ();

  pixels = g_new (guchar, load_info.tile_height * load_info.width * load_info.gimp_bpp);
  buf = g_malloc (load_info.linear_size);

  if (load_info.cubemap_faces)  /* Cubemap texture */
    {
      if ((load_info.cubemap_faces & DDSCAPS2_CUBEMAP_POSITIVEX) &&
          ! load_face (fp, &load_info, image, "(positive x)",
                       &layer_index, pixels, buf, read_mipmaps, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if ((load_info.cubemap_faces & DDSCAPS2_CUBEMAP_NEGATIVEX) &&
          ! load_face (fp, &load_info, image, "(negative x)",
                       &layer_index, pixels, buf, read_mipmaps, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if ((load_info.cubemap_faces & DDSCAPS2_CUBEMAP_POSITIVEY) &&
          ! load_face (fp, &load_info, image, "(positive y)",
                       &layer_index, pixels, buf, read_mipmaps, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if ((load_info.cubemap_faces & DDSCAPS2_CUBEMAP_NEGATIVEY) &&
          ! load_face (fp, &load_info, image, "(negative y)",
                       &layer_index, pixels, buf, read_mipmaps, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if ((load_info.cubemap_faces & DDSCAPS2_CUBEMAP_POSITIVEZ) &&
          ! load_face (fp, &load_info, image, "(positive z)",
                       &layer_index, pixels, buf, read_mipmaps, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if ((load_info.cubemap_faces & DDSCAPS2_CUBEMAP_NEGATIVEZ) &&
          ! load_face (fp, &load_info, image, "(negative z)",
                       &layer_index, pixels, buf, read_mipmaps, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (load_info.volume_slices > 0)  /* Volume texture */
    {
      guint  i, level;
      gchar *plane;

      for (i = 0; i < load_info.volume_slices; ++i)
        {
          plane = g_strdup_printf ("(z = %d)", i);

          if (! load_layer (fp, &load_info, image, 0, plane,
                            &layer_index, pixels, buf, error))
            {
              g_free (plane);
              fclose (fp);
              gimp_image_delete (image);
              return GIMP_PDB_EXECUTION_ERROR;
            }

          g_free (plane);
        }

      if (read_mipmaps)
        {
          for (level = 1; level < load_info.mipmaps; ++level)
            {
              int n = load_info.volume_slices >> level;

              if (n < 1)
                n = 1;

              for (i = 0; i < n; ++i)
                {
                  plane = g_strdup_printf ("(z = %d)", i);

                  if (! load_layer (fp, &load_info, image, level, plane,
                                    &layer_index, pixels, buf, error))
                    {
                      g_free (plane);
                      fclose (fp);
                      gimp_image_delete (image);
                      return GIMP_PDB_EXECUTION_ERROR;
                    }

                  g_free (plane);
                }
            }
        }
    }
  else if (load_info.array_items > 1)  /* Texture Array */
    {
      guint  i;
      gchar *elem;

      for (i = 0; i < load_info.array_items; ++i)
        {
          elem = g_strdup_printf ("(array element %d)", i);

          if (! load_layer (fp, &load_info, image, 0, elem, &layer_index,
                            pixels, buf, error))
            {
              fclose (fp);
              gimp_image_delete (image);
              return GIMP_PDB_EXECUTION_ERROR;
            }

          if (! load_mipmaps (fp, &load_info, image, elem, &layer_index,
                              pixels, buf, read_mipmaps, error))
            {
              fclose (fp);
              gimp_image_delete (image);
              return GIMP_PDB_EXECUTION_ERROR;
            }

          g_free (elem);
        }
    }
  else  /* Standard 2D texture */
    {
      if (! load_layer (fp, &load_info, image, 0, "", &layer_index,
                        pixels, buf, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if (! load_mipmaps (fp, &load_info, image, "", &layer_index,
                          pixels, buf, read_mipmaps, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }
    }

  gimp_progress_update (1.0);

  if (load_info.fmt_flags & DDPF_PALETTEINDEXED8)
    g_free (load_info.palette);

  g_free (buf);
  g_free (pixels);
  fclose (fp);

  layers = gimp_image_list_layers (image);

  if (! layers)
    {
      /* XXX This error should never happen, and probably it should be a
       * CRITICAL/g_return_if_fail(). Yet let's just set it to the
       * GError until we better handle the debug dialog for plug-ins. A
       * pop-up with this message will be easier to track. No need to
       * localize it though.
       */
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Oops! NULL image read! Please report this!");
      return GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_image_take_selected_layers (image, layers);

  if (flip_import)
    gimp_image_flip (image, GIMP_ORIENTATION_VERTICAL);

  *ret_image = image;

  return GIMP_PDB_SUCCESS;
}

/*
 * Read data from standard header
 */
static gboolean
read_header (dds_header_t *hdr,
             FILE         *fp)
{
  guchar buf[DDS_HEADERSIZE];

  if (fread (buf, 1, DDS_HEADERSIZE, fp) != DDS_HEADERSIZE)
    return FALSE;

  hdr->magic              = GETL32 (buf);

  hdr->size               = GETL32 (buf + 4);
  hdr->flags              = GETL32 (buf + 8);
  hdr->height             = GETL32 (buf + 12);
  hdr->width              = GETL32 (buf + 16);
  hdr->pitch_or_linsize   = GETL32 (buf + 20);
  hdr->depth              = GETL32 (buf + 24);
  hdr->num_mipmaps        = GETL32 (buf + 28);

  hdr->pixelfmt.size      = GETL32 (buf + 76);
  hdr->pixelfmt.flags     = GETL32 (buf + 80);
  hdr->pixelfmt.fourcc[0] = buf[84];
  hdr->pixelfmt.fourcc[1] = buf[85];
  hdr->pixelfmt.fourcc[2] = buf[86];
  hdr->pixelfmt.fourcc[3] = buf[87];
  hdr->pixelfmt.bpp       = GETL32 (buf + 88);
  hdr->pixelfmt.rmask     = GETL32 (buf + 92);
  hdr->pixelfmt.gmask     = GETL32 (buf + 96);
  hdr->pixelfmt.bmask     = GETL32 (buf + 100);
  hdr->pixelfmt.amask     = GETL32 (buf + 104);

  hdr->caps.caps1         = GETL32 (buf + 108);
  hdr->caps.caps2         = GETL32 (buf + 112);

  /* GIMP-DDS special info */
  if (GETL32 (buf + 32) == FOURCC ('G','I','M','P') &&
      GETL32 (buf + 36) == FOURCC ('-','D','D','S'))
    {
      hdr->reserved.gimp_dds_special.magic1       = GETL32 (buf + 32);
      hdr->reserved.gimp_dds_special.magic2       = GETL32 (buf + 36);
      hdr->reserved.gimp_dds_special.version      = GETL32 (buf + 40);
      hdr->reserved.gimp_dds_special.extra_fourcc = GETL32 (buf + 44);
    }

  return TRUE;
}

/*
 * Read data from DX10 header
 */
static gboolean
read_header_dx10 (dds_header_dx10_t *dx10hdr,
                  FILE              *fp)
{
  gchar buf[DDS_HEADERSIZE_DX10];

  if (fread (buf, 1, DDS_HEADERSIZE_DX10, fp) != DDS_HEADERSIZE_DX10)
    return FALSE;

  dx10hdr->dxgiFormat        = GETL32 (buf);
  dx10hdr->resourceDimension = GETL32 (buf + 4);
  dx10hdr->miscFlag          = GETL32 (buf + 8);
  dx10hdr->arraySize         = GETL32 (buf + 12);
  dx10hdr->reserved          = GETL32 (buf + 16);

  return TRUE;
}

/*
 * Check data from standard header for validity
 * Invalid header data is corrected where possible
 */
static gboolean
validate_header (dds_header_t  *hdr,
                 GError       **error)
{
  guint fourcc;

  /* Check  ~ m a g i c ~ */
  if (hdr->magic != FOURCC ('D','D','S',' '))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                   _("Invalid DDS format magic number."));
      return FALSE;
    }

  /* Check pixel format flags
   * If none are set, try to recover based on what information is available */
  fourcc = GETL32 (hdr->pixelfmt.fourcc);
  if (! (hdr->pixelfmt.flags & DDPF_RGB)           &&
      ! (hdr->pixelfmt.flags & DDPF_ALPHA)         &&
      ! (hdr->pixelfmt.flags & DDPF_BUMPDUDV)      &&
      ! (hdr->pixelfmt.flags & DDPF_BUMPLUMINANCE) &&
      ! (hdr->pixelfmt.flags & DDPF_ZBUFFER)       &&
      ! (hdr->pixelfmt.flags & DDPF_FOURCC)        &&
      ! (hdr->pixelfmt.flags & DDPF_LUMINANCE)     &&
      ! (hdr->pixelfmt.flags & DDPF_PALETTEINDEXED8))
    {
      g_message (_("File lacks expected pixel format flags! "
                   "Image may not be decoded correctly."));
      switch (fourcc)
        {
        case FOURCC ('D','X','T','1'):
        case FOURCC ('D','X','T','2'):
        case FOURCC ('D','X','T','3'):
        case FOURCC ('D','X','T','4'):
        case FOURCC ('D','X','T','5'):
        case FOURCC ('R','X','G','B'):
        case FOURCC ('A','T','I','1'):
        case FOURCC ('B','C','4','U'):
        case FOURCC ('B','C','4','S'):
        case FOURCC ('A','T','I','2'):
        case FOURCC ('B','C','5','U'):
        case FOURCC ('B','C','5','S'):
          hdr->pixelfmt.flags |= DDPF_FOURCC;
          break;
        default:
          switch (hdr->pixelfmt.bpp)
            {
            case 8:
              if (hdr->pixelfmt.flags & DDPF_ALPHAPIXELS)
                hdr->pixelfmt.flags |= DDPF_ALPHA;
              else
                hdr->pixelfmt.flags |= DDPF_LUMINANCE;
              break;
            case 16:
            case 24:
            case 32:
            case 64:
              hdr->pixelfmt.flags |= DDPF_RGB;
              break;
            default:
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Invalid pixel format."));
              return FALSE;
            }
          break;
        }
    }

  /* Check all supported FourCC codes */
  if ((hdr->pixelfmt.flags & DDPF_FOURCC) &&
      fourcc != FOURCC ('D','X','T','1')  &&
      fourcc != FOURCC ('D','X','T','2')  &&
      fourcc != FOURCC ('D','X','T','3')  &&
      fourcc != FOURCC ('D','X','T','4')  &&
      fourcc != FOURCC ('D','X','T','5')  &&
      fourcc != FOURCC ('R','X','G','B')  &&
      fourcc != FOURCC ('A','T','I','1')  &&
      fourcc != FOURCC ('B','C','4','U')  &&
      fourcc != FOURCC ('B','C','4','S')  &&
      fourcc != FOURCC ('A','T','I','2')  &&
      fourcc != FOURCC ('B','C','5','U')  &&
      fourcc != FOURCC ('B','C','5','S')  &&
      fourcc != FOURCC ('D','X','1','0')  &&
      hdr->pixelfmt.fourcc[1] != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported format (FourCC: %c%c%c%c, hex: %08x)"),
                   hdr->pixelfmt.fourcc[0],
                   hdr->pixelfmt.fourcc[1] != 0 ? hdr->pixelfmt.fourcc[1] : ' ',
                   hdr->pixelfmt.fourcc[2] != 0 ? hdr->pixelfmt.fourcc[2] : ' ',
                   hdr->pixelfmt.fourcc[3] != 0 ? hdr->pixelfmt.fourcc[3] : ' ',
                   GETL32 (hdr->pixelfmt.fourcc));
      return FALSE;
    }

  /* Check bits-per-pixel */
  if (hdr->pixelfmt.flags & DDPF_RGB)
    {
      if ((hdr->pixelfmt.bpp !=  8) &&
          (hdr->pixelfmt.bpp != 16) &&
          (hdr->pixelfmt.bpp != 24) &&
          (hdr->pixelfmt.bpp != 32) &&
          (hdr->pixelfmt.bpp != 48) &&
          (hdr->pixelfmt.bpp != 64) &&
          (hdr->pixelfmt.bpp != 96) &&
          (hdr->pixelfmt.bpp != 128))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Invalid bpp value for RGB data: %d"),
                       hdr->pixelfmt.bpp);
          return FALSE;
        }
    }
  else if (hdr->pixelfmt.flags & DDPF_LUMINANCE)
    {
      if ((hdr->pixelfmt.bpp !=  8) &&
          (hdr->pixelfmt.bpp != 16))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Invalid bpp value for luminance data: %d"),
                       hdr->pixelfmt.bpp);
          return FALSE;
        }
    }

  return TRUE;
}

/*
 * Check data from DX10 header for validity
 */
static gboolean
validate_dx10_header (dds_header_dx10_t  *dx10hdr,
                      dds_load_info_t    *load_info,
                      GError            **error)
{
  if ((dx10hdr->resourceDimension != D3D10_RESOURCE_DIMENSION_TEXTURE1D) &&
      (dx10hdr->resourceDimension != D3D10_RESOURCE_DIMENSION_TEXTURE2D) &&
      (dx10hdr->resourceDimension != D3D10_RESOURCE_DIMENSION_TEXTURE3D))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Invalid DX10 header"));
      return FALSE;
    }

  switch (dx10hdr->dxgiFormat)
    {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
      /* Return early for supported compressed formats */
      return TRUE;
    default:
      /* Unset FourCC flag for uncompressed formats */
      load_info->fmt_flags &= ~DDPF_FOURCC;
      break;
    }

  if (! dxgiformat_supported (dx10hdr->dxgiFormat & 0xFF))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported DXGI Format: %u"),
                   dx10hdr->dxgiFormat & 0xFF);
      return FALSE;
    }

  load_info->dxgi_format = dx10hdr->dxgiFormat & 0xFF;

  return TRUE;
}

static const Babl *
premultiplied_variant (const Babl* format)
{
  if (format == babl_format ("R'G'B'A u8"))
    return babl_format ("R'aG'aB'aA u8");
  else
    g_printerr ("Add format %s to premultiplied_variant () %s: %d\n",
                babl_get_name (format), __FILE__, __LINE__);
  return format;
}

static gboolean
load_layer (FILE             *fp,
            dds_load_info_t  *load_info,
            GimpImage        *image,
            guint             level,
            gchar            *prefix,
            guint            *layer_index,
            guchar           *pixels,
            guchar           *buf,
            GError          **error)
{
  GeglBuffer    *buffer;
  const Babl    *bablfmt  = NULL;
  gchar         *babl_str = "";
  GimpImageType  type     = GIMP_RGBA_IMAGE;
  guint          width    = load_info->width  >> level;
  guint          height   = load_info->height >> level;
  guint          size     = width * height * load_info->bpp;
  gchar         *layer_name;
  GimpLayer     *layer;
  guint          layerw;
  gsize          file_size;
  gsize          current_position;
  gint           x, y, n;

  current_position = ftell (fp);
  fseek (fp, 0L, SEEK_END);
  file_size = ftell (fp);
  fseek (fp, current_position, SEEK_SET);

  if (width  < 1) width  = 1;
  if (height < 1) height = 1;

  /* Setup image type and Babl format */
  if (load_info->fmt_flags & DDPF_FOURCC)  /* Compressed */
    {
      /* Set Babl format */
      switch (load_info->comp_format)
        {
        case DDS_COMPRESS_BC4:
          type = GIMP_GRAY_IMAGE;
          babl_str = "Y'";
          break;
        case DDS_COMPRESS_BC5:
          type = GIMP_RGB_IMAGE;
          babl_str = "R'G'B'";
          break;
        default:
          type = GIMP_RGBA_IMAGE;
          babl_str = "R'G'B'A";
          break;
        }

      /* Set Babl precision */
      if ((GETL32 (load_info->fourcc) == FOURCC ('D','X','1','0')) &&
          (load_info->dxgi_format >= DXGI_FORMAT_BC6H_TYPELESS)    &&
          (load_info->dxgi_format <= DXGI_FORMAT_BC6H_SF16))
        {
          babl_str = g_strdup_printf ("%s %s", babl_str, "half");
        }
      else
        {
          babl_str = g_strdup_printf ("%s %s", babl_str, "u8");
        }
    }
  else  /* Uncompressed */
    {
      /* Set Babl format */
      if (load_info->read_info.gimp_type == GIMP_INDEXED)
        {
          if (load_info->read_info.use_alpha)
            type = GIMP_INDEXEDA_IMAGE;
          else
            type = GIMP_INDEXED_IMAGE;
        }
      else if (load_info->read_info.gimp_type == GIMP_RGB)
        {
          if (load_info->read_info.use_alpha)
            {
              type = GIMP_RGBA_IMAGE;
              babl_str = "R'G'B'A";
            }
          else
            {
              type = GIMP_RGB_IMAGE;
              babl_str = "R'G'B'";
            }
        }
      else  /* load_info->read_info.gimp_type == GIMP_GRAY */
        {
          if (load_info->read_info.use_alpha)
            {
              type = GIMP_GRAYA_IMAGE;
              babl_str = "Y'A";
            }
          else
            {
              type = GIMP_GRAY_IMAGE;
              babl_str = "Y'";
            }
        }

      /* Set Babl precision */
      if (load_info->read_info.is_float)
        {
          /* Floating-point */
          if (load_info->read_info.output_bit_depth == 16)
            babl_str = g_strdup_printf ("%s %s", babl_str, "half");
          else  /* load_info->read_info.output_bit_depth == 32 */
            babl_str = g_strdup_printf ("%s %s", babl_str, "float");
        }
      else
        {
          /* Integer */
          if (load_info->read_info.output_bit_depth == 32)
            babl_str = g_strdup_printf ("%s %s", babl_str, "u32");
          else if (load_info->read_info.output_bit_depth == 16)
            babl_str = g_strdup_printf ("%s %s", babl_str, "u16");
          else  /* load_info->read_info.output_bit_depth == 8 */
            babl_str = g_strdup_printf ("%s %s", babl_str, "u8");
        }
    }

  if (! (load_info->read_info.gimp_type == GIMP_INDEXED))
    bablfmt = babl_format (babl_str);

  g_free (babl_str);

  layer_name = (level) ? g_strdup_printf ("mipmap %d %s", level, prefix) :
                         g_strdup_printf ("main surface %s", prefix);

  layer = gimp_layer_new (image, layer_name, width, height, type, 100,
                          gimp_image_get_default_new_layer_mode (image));
  g_free (layer_name);

  gimp_image_insert_layer (image, layer, NULL, *layer_index);

  if (type == GIMP_INDEXED_IMAGE || type == GIMP_INDEXEDA_IMAGE)
    bablfmt = gimp_drawable_get_format (GIMP_DRAWABLE (layer));

  if ((*layer_index)++)
    gimp_item_set_visible (GIMP_ITEM (layer), FALSE);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  layerw = gegl_buffer_get_width (buffer);

  if (load_info->fmt_flags & DDPF_FOURCC)
    {
      size = ((width + 3) >> 2) * ((height + 3) >> 2);

      /* Let Babl handle premultiplied format conversion */
      if ((GETL32 (load_info->fourcc) == FOURCC ('D','X','T','2')) ||
          (GETL32 (load_info->fourcc) == FOURCC ('D','X','T','4')))
        bablfmt = premultiplied_variant (bablfmt);

      if ((load_info->comp_format == DDS_COMPRESS_BC1) ||
          (load_info->comp_format == DDS_COMPRESS_BC4))
        size *= 8;
      else
        size *= 16;
    }

  if (size > (file_size - current_position) ||
      size > load_info->linear_size)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Requested data exceeds size of file.\n"));
      return FALSE;
    }

  if ((load_info->flags & DDSD_LINEARSIZE) &&
      ! fread (buf, size, 1, fp))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unexpected EOF.\n"));
      return FALSE;
    }

  if (! (load_info->fmt_flags & DDPF_FOURCC))  /* Read uncompressed pixel data */
    {
      guint   rowstride   = width * load_info->bpp;
      guint32 sign_add[4] = { 0, 0, 0, 0 };
      guint   idx_r = 0, idx_b = 2;

      /* Prior plug-in versions (3.9.91 and earlier) wrote the R and G channels reversed for RGB10A2. */
      if ((load_info->gimp_version > 0)       &&
          (load_info->gimp_version <= 199003) &&
          (load_info->d3d9_format == D3DFMT_A2R10G10B10))
        {
          g_printerr ("Switching incorrect red and green channels in RGB10A2 DDS "
                      "written by an older version of GIMP's DDS plug-in.\n");
          idx_r = 2;
          idx_b = 0;
        }

      /* Set up offset to apply to signed integer formats
       * Per-channel to accommodate for mixed formats  */
      if (load_info->read_info.is_signed &&
          (! load_info->read_info.is_float))
        {
          if (load_info->read_info.output_bit_depth == 8)
            {
              sign_add[0] = 128;
              sign_add[1] = 128;
              if (! (load_info->d3d9_format == D3DFMT_L6V5U5 ||
                     load_info->d3d9_format == D3DFMT_X8L8V8U8))
                sign_add[2] = 128;
              sign_add[3] = 128;
            }
          else if (load_info->read_info.output_bit_depth == 16)
            {
              sign_add[0] = 32768;
              sign_add[1] = 32768;
              sign_add[2] = 32768;
              if (! (load_info->d3d9_format == D3DFMT_A2W10V10U10 ||
                     load_info->dxgi_format == DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM))
                sign_add[3] = 32768;
            }
          else  /* load_info->read_info.output_bit_depth == 32 */
            {
              sign_add[0] = 2147483648;
              sign_add[1] = 2147483648;
              sign_add[2] = 2147483648;
              sign_add[3] = 2147483648;
            }
        }

      if ((load_info->flags & DDSD_PITCH) && (rowstride > load_info->pitch))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Requested data exceeds size of file.\n"));
          return FALSE;
        }

      for (y = 0, n = 0; y < height; ++y, ++n)
        {
          if (n >= load_info->tile_height)
            {
              gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y - n, layerw, n), 0,
                               bablfmt, pixels, GEGL_AUTO_ROWSTRIDE);
              n = 0;
              gimp_progress_update ((gdouble) y / (gdouble) load_info->height);
            }

          if (load_info->flags & DDSD_PITCH)
            {
              current_position = ftell (fp);
              if (rowstride > (file_size - current_position))
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Requested data exceeds size of file.\n"));
                  return FALSE;
                }
              if (! fread (buf, rowstride, 1, fp))
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Unexpected EOF.\n"));
                  return FALSE;
                }
            }

          for (x = 0; x < layerw; ++x)
            {
              guint   pos       = (n * layerw + x) * load_info->gimp_bpp;
              guint   buf_reads = 0;
              guchar  read_buf;
              guint32 ch_registers[4];

              memset (ch_registers, 0, sizeof (ch_registers));

              /* Format-agnostic bit-reader, driven by the 'format_read_info' table.
               * Reads one bit at a time from source bytes into per-channel registers.
               * While somewhat simplistic, reading bit-by-bit allows us to handle channels
               * that cross byte boundaries trivially, and without the need for look-ahead.
               */
              read_buf = *buf;
              for (gint reg = 0; reg < 4; reg++)
                {
                  const guchar  ch        = load_info->read_info.channel_order[reg];
                  const guchar  ch_bits   = load_info->read_info.channel_bits[reg];
                  const guint32 write_bit = 1 << (ch_bits - 1);

                  if (! ch_bits) continue;

                  /* Note: bits are written to the registers in the opposite order they're read in */
                  for (gint bit = 0; bit < ch_bits; bit++)
                    {
                      ch_registers[ch] >>= 1;
                      ch_registers[ch] |= read_buf & 1 ? write_bit : 0;
                      read_buf >>= 1;

                      buf_reads++;
                      if (buf_reads == 8)
                        {
                          /* Roll-over to next byte */
                          buf++;
                          read_buf = *buf;
                          buf_reads = 0;
                        }
                    }

                  /* Most DXGI small-float formats have 5 exponent bits, so can be interpreted as 16-bit floats with a simple shift.
                   * Integers meanwhile must be properly requantized to the output range */
                  if (load_info->read_info.is_float)
                    {
                      guint shift = load_info->read_info.output_bit_depth - ch_bits;

                      if (load_info->dxgi_format == DXGI_FORMAT_R9G9B9E5_SHAREDEXP     ||
                          load_info->dxgi_format == DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT ||
                          load_info->dxgi_format == DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT)
                        /* Skip shifting for float formats that require special handling */
                        shift = 0;
                      else if (! load_info->read_info.is_signed)
                        /* Don't shift into sign bit for unsigned floats, eg. R11G11B10 */
                        shift -= 1;

                      ch_registers[ch] = ch_registers[ch] << shift;
                    }
                  else
                    {
                      ch_registers[ch] = requantize_component (ch_registers[ch], ch_bits,
                                                               load_info->read_info.output_bit_depth);
                    }
                }

              /* Special cases for formats requiring extra decoding */
              if (load_info->dxgi_format == DXGI_FORMAT_R9G9B9E5_SHAREDEXP)
                float_from_9e5 (ch_registers);
              else if (load_info->dxgi_format == DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT)
                float_from_7e3a2 (ch_registers);
              else if (load_info->dxgi_format == DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT)
                float_from_6e4a2 (ch_registers);
              else if (load_info->d3d9_format == D3DFMT_CxV8U8)
                reconstruct_z (ch_registers);

              /* Clear alpha to all 1s instead of all 0s */
              if (! load_info->read_info.use_alpha)
                ch_registers[3] = G_MAXUINT32;

              /* Output converted values to canvas pixels */
              if (load_info->read_info.gimp_type == GIMP_RGB)
                {
                  if (load_info->read_info.output_bit_depth == 8)
                    {
                      guchar *pixel8 = (guchar *) &pixels[pos];
                      pixel8[0] = ch_registers[0] + sign_add[0];
                      pixel8[1] = ch_registers[1] + sign_add[1];
                      pixel8[2] = ch_registers[2] + sign_add[2];

                      if (load_info->read_info.use_alpha)
                        pixel8[3] = ch_registers[3] + sign_add[3];
                    }
                  else if (load_info->read_info.output_bit_depth == 16)
                    {
                      /* Variable indices for R and B to accommodate RGB10A2 fixup */
                      guint16 *pixel16 = (guint16 *) &pixels[pos];
                      pixel16[0] = ch_registers[idx_r] + sign_add[0];
                      pixel16[1] = ch_registers[1]     + sign_add[1];
                      pixel16[2] = ch_registers[idx_b] + sign_add[2];

                      if (load_info->read_info.use_alpha)
                        pixel16[3] = ch_registers[3] + sign_add[3];
                    }
                  else  /* load_info->read_info.output_bit_depth == 32 */
                    {
                      guint32 *pixel32 = (guint32 *) &pixels[pos];
                      pixel32[0] = (guint64) ch_registers[0] + sign_add[0];
                      pixel32[1] = (guint64) ch_registers[1] + sign_add[1];
                      pixel32[2] = (guint64) ch_registers[2] + sign_add[2];

                      if (load_info->read_info.use_alpha)
                        pixel32[3] = (guint64) ch_registers[3] + sign_add[3];
                    }
                }
              else if (load_info->read_info.gimp_type == GIMP_GRAY)
                {
                  if (load_info->read_info.output_bit_depth == 8)
                    {
                      guchar *pixel8 = (guchar *) &pixels[pos];
                      pixel8[0] = ch_registers[0] + sign_add[0];

                      if (load_info->read_info.use_alpha)
                        pixel8[1] = ch_registers[3] + sign_add[3];
                    }
                  else if (load_info->read_info.output_bit_depth == 16)
                    {
                      guint16 *pixel16 = (guint16 *) &pixels[pos];
                      pixel16[0] = ch_registers[0] + sign_add[0];

                      if (load_info->read_info.use_alpha)
                        pixel16[1] = ch_registers[3] + sign_add[3];
                    }
                  else  /* load_info->read_info.output_bit_depth == 32 */
                    {
                      guint32 *pixel32 = (guint32 *) &pixels[pos];
                      pixel32[0] = (guint64) ch_registers[0] + sign_add[0];

                      if (load_info->read_info.use_alpha)
                        pixel32[1] = (guint64) ch_registers[3] + sign_add[3];
                    }
                }
              else  /* load_info->read_info.gimp_type == GIMP_INDEXED */
                {
                  pixels[pos] = ch_registers[0] & 0xFF;
                  if (load_info->read_info.use_alpha)
                    pixels[pos + 1] = ch_registers[3] & 0xFF;
                }
            }
        }

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y - n, layerw, n), 0,
                       bablfmt, pixels, GEGL_AUTO_ROWSTRIDE);
    }
  else  /* Read compressed pixel data */
    {
      guchar *dst;

      dst = g_malloc (width * height * load_info->gimp_bpp);
      memset (dst, 0, width * height * load_info->gimp_bpp);

      /* Initialize alpha to all 1s instead of all 0s */
      if (load_info->gimp_bpp == 4)
        {
          for (y = 0; y < height; ++y)
            {
              for (x = 0; x < width; ++x)
                {
                  dst[y * (width * 4) + (x * 4) + 3] = 255;
                }
            }
        }

      dxt_decompress (dst, buf, load_info->comp_format, size, width, height,
                      load_info->gimp_bpp, load_info->fmt_flags & DDPF_NORMAL);

      /* Prior plug-in versions (before 3.9.90) wrote the R and G channels reversed for BC5. */
      if ((load_info->gimp_version > 0)       &&
          (load_info->gimp_version <= 199002) &&
          (load_info->comp_format == DDS_COMPRESS_BC5))
        {
          g_printerr ("Switching incorrect red and green channels in BC5 DDS "
                      "written by an older version of GIMP's DDS plug-in.\n");

          for (y = 0; y < height; ++y)
            {
              for (x = 0; x < width; ++x)
                {
                  guchar tmpG;
                  guint  pix_width = width * load_info->gimp_bpp;
                  guint  x_width   = x * load_info->gimp_bpp;

                  tmpG = dst[y * pix_width + x_width];
                  dst[y * pix_width + x_width] = dst[y * pix_width + x_width + 1];
                  dst[y * pix_width + x_width + 1] = tmpG;
                }
            }
        }

      for (y = 0, n = 0; y < height; ++y, ++n)
        {
          if (n >= load_info->tile_height)
            {
              gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y - n, layerw, n), 0,
                               bablfmt, pixels, GEGL_AUTO_ROWSTRIDE);
              n = 0;
              gimp_progress_update ((gdouble) y / (gdouble) load_info->height);
            }

          memcpy (pixels + n * layerw * load_info->gimp_bpp,
                  dst + y * layerw * load_info->gimp_bpp,
                  width * load_info->gimp_bpp);
        }

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y - n, layerw, n), 0,
                       bablfmt, pixels, GEGL_AUTO_ROWSTRIDE);

      g_free (dst);
    }

  gegl_buffer_flush (buffer);

  g_object_unref (buffer);

  /* Decode files with GIMP-specific encodings */
  if (load_info->gimp_version > 0)
    {
      switch (GETL32 (load_info->gimp_fourcc))
        {
        case FOURCC ('A','E','X','P'):
          decode_alpha_exponent (GIMP_DRAWABLE (layer));
          break;
        case FOURCC ('Y','C','G','1'):
          decode_ycocg (GIMP_DRAWABLE (layer));
          break;
        case FOURCC ('Y','C','G','2'):
          decode_ycocg_scaled (GIMP_DRAWABLE (layer));
          break;
        default:
          break;
        }
    }

  return TRUE;
}

static gboolean
load_mipmaps (FILE             *fp,
              dds_load_info_t  *load_info,
              GimpImage        *image,
              gchar            *prefix,
              guint            *layer_index,
              guchar           *pixels,
              guchar           *buf,
              gboolean          read_mipmaps,
              GError          **error)
{
  guint level;

  if (read_mipmaps)
    {
      for (level = 1; level < load_info->mipmaps; ++level)
        {
          if (! load_layer (fp, load_info, image, level, prefix, layer_index,
                            pixels, buf, error))
            return FALSE;
        }
    }
  else
    {
      /* Skip past mipmaps, as simply not reading them leaves us in the wrong pos for subsequent layers */
      for (level = 1; level < load_info->mipmaps; ++level)
        {
          guint width  = MAX (1, load_info->width  >> level);
          guint height = MAX (1, load_info->height >> level);
          guint size   = load_info->linear_size >> (2 * level);

          if (load_info->fmt_flags & DDPF_FOURCC)
            {
              size = ((width + 3) >> 2) * ((height + 3) >> 2);
              if ((load_info->comp_format == DDS_COMPRESS_BC1) ||
                  (load_info->comp_format == DDS_COMPRESS_BC4))
                size *= 8;
              else
                size *= 16;
            }

          fseek (fp, size, SEEK_CUR);
        }
    }

  return TRUE;
}

static gboolean
load_face (FILE             *fp,
           dds_load_info_t  *load_info,
           GimpImage        *image,
           gchar            *prefix,
           guint            *layer_index,
           guchar           *pixels,
           guchar           *buf,
           gboolean          read_mipmaps,
           GError          **error)
{
  if (! load_layer (fp, load_info, image, 0, prefix,
                    layer_index, pixels, buf, error))
    return FALSE;

  return load_mipmaps (fp, load_info, image, prefix, layer_index,
                       pixels, buf, read_mipmaps, error);
}

static gboolean
load_dialog (GimpProcedure       *procedure,
             GimpProcedureConfig *config)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  gboolean   run;

  dialog = gimp_procedure_dialog_new (procedure,
                                      config,
                                      _("Open DDS"));

  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "dds-read-box",
                                         "load-mipmaps",
                                         "flip-image",
                                         NULL);
  gtk_box_set_spacing (GTK_BOX (vbox), 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "dds-read-box", NULL);
  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
