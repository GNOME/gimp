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

#include "ddsread.h"
#include "dds.h"
#include "dxt.h"
#include "endian_rw.h"
#include "misc.h"
#include "imath.h"


typedef struct
{
  guchar  rshift, gshift, bshift, ashift;
  guchar  rbits, gbits, bbits, abits;
  guint   rmask, gmask, bmask, amask;
  guint   bpp, gimp_bpp;
  guint   gimp_bps;         /* bytes per sample */
  gint    tile_height;
  guchar *palette;
} dds_load_info_t;


static gboolean      read_header       (dds_header_t       *hdr,
                                        FILE               *fp);
static gboolean      read_header_dx10  (dds_header_dx10_t  *hdr,
                                        FILE               *fp);
static gboolean      validate_header   (dds_header_t       *hdr,
                                        GError            **error);
static gboolean      setup_dxgi_format (dds_header_t       *hdr,
                                        dds_header_dx10_t  *dx10hdr,
                                        GError            **error);
static gboolean      load_layer        (FILE               *fp,
                                        dds_header_t       *hdr,
                                        dds_load_info_t    *d,
                                        GimpImage          *image,
                                        guint               level,
                                        gchar              *prefix,
                                        guint              *l,
                                        guchar             *pixels,
                                        guchar             *buf,
                                        gboolean            decode_images,
                                        GError            **error);
static gboolean      load_mipmaps      (FILE               *fp,
                                        dds_header_t       *hdr,
                                        dds_load_info_t    *d,
                                        GimpImage          *image,
                                        gchar              *prefix,
                                        guint              *l,
                                        guchar             *pixels,
                                        guchar             *buf,
                                        gboolean            read_mipmaps,
                                        gboolean            decode_images,
                                        GError            **error);
static gboolean      load_face         (FILE               *fp,
                                        dds_header_t       *hdr,
                                        dds_load_info_t    *d,
                                        GimpImage          *image,
                                        char               *prefix,
                                        guint              *l,
                                        guchar             *pixels,
                                        guchar             *buf,
                                        gboolean            read_mipmaps,
                                        gboolean            decode_images,
                                        GError            **error);
static guchar        color_bits        (guint               mask);
static guchar        color_shift       (guint               mask);
static gboolean      load_dialog       (GimpProcedure      *procedure,
                                        GObject            *config);


GimpPDBStatusType
read_dds (GFile          *file,
          GimpImage     **ret_image,
          gboolean        interactive,
          GimpProcedure  *procedure,
          GObject        *config,
          GError        **error)
{
  GimpImage         *image = NULL;
  guchar            *buf;
  guint              l = 0;
  guchar            *pixels;
  FILE              *fp;
  dds_header_t       hdr;
  dds_header_dx10_t  dx10hdr;
  dds_load_info_t    d;
  GList             *layers;
  GimpImageBaseType  type;
  GimpPrecision      precision;
  gboolean           read_mipmaps;
  gboolean           decode_images;
  gint               i, j;

  if (interactive)
    {
      gimp_ui_init ("dds");

      if (! load_dialog (procedure, config))
        return GIMP_PDB_CANCEL;
    }

  g_object_get (config,
                "load-mipmaps",  &read_mipmaps,
                "decode-images", &decode_images,
                NULL);

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_progress_init_printf ("Loading %s:", gimp_file_get_utf8_name (file));

  /* read header */
  read_header (&hdr, fp);

  memset (&dx10hdr, 0, sizeof (dds_header_dx10_t));

  /* read DX10 header if necessary */
  if (GETL32 (hdr.pixelfmt.fourcc) == FOURCC ('D','X','1','0'))
    {
      read_header_dx10 (&dx10hdr, fp);

      if (! setup_dxgi_format (&hdr, &dx10hdr, error))
        {
          fclose (fp);
          return GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (! validate_header (&hdr, error))
    {
      fclose (fp);
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* a lot of DDS images out there don't have this for some reason -_- */
  if (hdr.pitch_or_linsize == 0)
    {
      if (hdr.pixelfmt.flags & DDPF_FOURCC) /* assume linear size */
        {
          hdr.pitch_or_linsize = ((hdr.width + 3) >> 2) * ((hdr.height + 3) >> 2);
          switch (GETL32 (hdr.pixelfmt.fourcc))
            {
            case FOURCC ('D','X','T','1'):
            case FOURCC ('A','T','I','1'):
            case FOURCC ('B','C','4','U'):
            case FOURCC ('B','C','4','S'):
              hdr.pitch_or_linsize *= 8;
              break;
            default:
              hdr.pitch_or_linsize *= 16;
              break;
            }
        }
      else /* assume pitch */
        {
          hdr.pitch_or_linsize = hdr.height * hdr.width * (hdr.pixelfmt.bpp >> 3);
        }
    }

  if (hdr.pixelfmt.flags & DDPF_FOURCC)
    {
      /* fourcc is dXt* or rXgb */
      if (hdr.pixelfmt.fourcc[1] == 'X')
        hdr.pixelfmt.flags |= DDPF_ALPHAPIXELS;
    }

  d.rshift = color_shift (hdr.pixelfmt.rmask);
  d.gshift = color_shift (hdr.pixelfmt.gmask);
  d.bshift = color_shift (hdr.pixelfmt.bmask);
  d.ashift = color_shift (hdr.pixelfmt.amask);
  d.rbits  = color_bits (hdr.pixelfmt.rmask);
  d.gbits  = color_bits (hdr.pixelfmt.gmask);
  d.bbits  = color_bits (hdr.pixelfmt.bmask);
  d.abits  = color_bits (hdr.pixelfmt.amask);
  if (d.rbits <= 8)
    d.rmask  = (hdr.pixelfmt.rmask >> d.rshift) << (8 - d.rbits);
  else
    d.rmask  = (hdr.pixelfmt.rmask >> d.rshift) << (16 - d.rbits);
  if (d.gbits <= 8)
    d.gmask  = (hdr.pixelfmt.gmask >> d.gshift) << (8 - d.gbits);
  else
    d.gmask  = (hdr.pixelfmt.gmask >> d.gshift) << (16 - d.gbits);
  if (d.bbits <= 8)
    d.bmask  = (hdr.pixelfmt.bmask >> d.bshift) << (8 - d.bbits);
  else
    d.bmask  = (hdr.pixelfmt.bmask >> d.bshift) << (16 - d.bbits);
  if (d.abits <= 8)
    d.amask  = (hdr.pixelfmt.amask >> d.ashift) << (8 - d.abits);
  else
    d.amask  = (hdr.pixelfmt.amask >> d.ashift) << (16 - d.abits);

  d.gimp_bps = 1; /* Most formats will be converted to 1 byte per sample */
  if (hdr.pixelfmt.flags & DDPF_FOURCC)
    {
      switch (GETL32 (hdr.pixelfmt.fourcc))
        {
        case FOURCC ('A','T','I','1'):
        case FOURCC ('B','C','4','U'):
        case FOURCC ('B','C','4','S'):
          d.bpp = d.gimp_bpp = 1;
          type = GIMP_GRAY;
          break;
        case FOURCC ('A','T','I','2'):
        case FOURCC ('B','C','5','U'):
        case FOURCC ('B','C','5','S'):
          d.bpp = d.gimp_bpp = 3;
          type = GIMP_RGB;
          break;
        default:
          d.bpp = d.gimp_bpp = 4;
          type = GIMP_RGB;
          break;
        }
    }
  else
    {
      d.bpp = hdr.pixelfmt.bpp >> 3;

      if (d.bpp == 2)
        {
          if (hdr.pixelfmt.amask == 0xf000) /* RGBA4 */
            {
              d.gimp_bpp = 4;
              type = GIMP_RGB;
            }
          else if (hdr.pixelfmt.amask == 0xff00) /* L8A8 */
            {
              d.gimp_bpp = 2;
              type = GIMP_GRAY;
            }
          else if (hdr.pixelfmt.bmask == 0x1f) /* R5G6B5 or RGB5A1 */
            {
              if (hdr.pixelfmt.amask == 0x8000) /* RGB5A1 */
                d.gimp_bpp = 4;
              else
                d.gimp_bpp = 3;

              type = GIMP_RGB;
            }
          else if (hdr.pixelfmt.rmask == 0xffff || /* L16 */
                   hdr.pixelfmt.gmask == 0xffff ||
                   hdr.pixelfmt.bmask == 0xffff ||
                   hdr.pixelfmt.amask == 0xffff)
            {
              d.gimp_bpp = 2;
              d.gimp_bps = 2;
              type = GIMP_GRAY;
            }
          else
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           "Unsupported uncompressed dds format: "
                           "bpp: %d, Rmask: %x, Gmask: %x, Bmask: %x, Amask: %x",
                           hdr.pixelfmt.bpp,
                           hdr.pixelfmt.rmask, hdr.pixelfmt.gmask,
                           hdr.pixelfmt.bmask, hdr.pixelfmt.amask);
              return GIMP_PDB_EXECUTION_ERROR;
            }
        }
      else
        {
          if (hdr.pixelfmt.flags & DDPF_PALETTEINDEXED8)
            {
              type = GIMP_INDEXED;
              d.gimp_bpp = 1;
            }
          else if (hdr.pixelfmt.rmask == 0xe0) /* R3G3B2 */
            {
              type = GIMP_RGB;
              d.gimp_bpp = 3;
            }
          else if (d.bpp == 4)
            {
              type = GIMP_RGB;
              if (d.rbits > 8 || d.gbits > 8 || d.bbits > 8 || d.abits > 8)
                {
                  d.gimp_bps = 2;
                  d.gimp_bpp = 8;
                }
              else
                {
                  d.gimp_bpp = d.bpp;
                }
            }
          else
            {
              /* test alpha only image */
              if (d.bpp == 1 && (hdr.pixelfmt.flags & DDPF_ALPHA))
                {
                  d.gimp_bpp = 2;
                  type = GIMP_GRAY;
                }
              else
                {
                  d.gimp_bpp = d.bpp;
                  type = (d.bpp == 1) ? GIMP_GRAY : GIMP_RGB;
                }
            }
        }
    }

  if (d.gimp_bps == 2)
    {
      precision = GIMP_PRECISION_U16_NON_LINEAR;
    }
  else
    {
      precision = GIMP_PRECISION_U8_NON_LINEAR;
    }

  image = gimp_image_new_with_precision (hdr.width, hdr.height, type, precision);

  if (! image)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOMEM,
                   _("Could not allocate a new image."));
      fclose (fp);
      return GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_image_set_file (image, file);

  if (hdr.pixelfmt.flags & DDPF_PALETTEINDEXED8)
    {
      d.palette = g_malloc (256 * 4);
      if (fread (d.palette, 1, 1024, fp) != 1024)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error reading palette."));
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }
      for (i = j = 0; i < 768; i += 3, j += 4)
        {
          d.palette[i + 0] = d.palette[j + 0];
          d.palette[i + 1] = d.palette[j + 1];
          d.palette[i + 2] = d.palette[j + 2];
        }
      gimp_image_set_colormap (image, d.palette, 256);
    }

  d.tile_height = gimp_tile_height ();

  pixels = g_new (guchar, d.tile_height * hdr.width * d.gimp_bpp);
  buf = g_malloc (hdr.pitch_or_linsize);

  if (! (hdr.caps.caps2 & DDSCAPS2_CUBEMAP) &&
      ! (hdr.caps.caps2 & DDSCAPS2_VOLUME) &&
      dx10hdr.arraySize == 0)
    {
      if (! load_layer (fp, &hdr, &d, image, 0, "", &l, pixels, buf,
                        decode_images, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if (! load_mipmaps (fp, &hdr, &d, image, "", &l, pixels, buf,
                          read_mipmaps, decode_images, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (hdr.caps.caps2 & DDSCAPS2_CUBEMAP)
    {
      if ((hdr.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEX) &&
          ! load_face (fp, &hdr, &d, image, "(positive x)", &l, pixels, buf,
                       read_mipmaps, decode_images, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if ((hdr.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) &&
          ! load_face (fp, &hdr, &d, image, "(negative x)", &l, pixels, buf,
                       read_mipmaps, decode_images, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if ((hdr.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEY) &&
          ! load_face (fp, &hdr, &d, image, "(positive y)", &l, pixels, buf,
                       read_mipmaps, decode_images, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if ((hdr.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) &&
          ! load_face (fp, &hdr, &d, image, "(negative y)", &l, pixels, buf,
                       read_mipmaps, decode_images, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if ((hdr.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) &&
          ! load_face (fp, &hdr, &d, image, "(positive z)", &l, pixels, buf,
                       read_mipmaps, decode_images, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if ((hdr.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ) &&
          ! load_face (fp, &hdr, &d, image, "(negative z)", &l, pixels, buf,
                       read_mipmaps, decode_images, error))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if ((hdr.caps.caps2 & DDSCAPS2_VOLUME) &&
           (hdr.flags & DDSD_DEPTH))
    {
      guint  i, level;
      gchar *plane;

      for (i = 0; i < hdr.depth; ++i)
        {
          plane = g_strdup_printf ("(z = %d)", i);
          if (! load_layer (fp, &hdr, &d, image, 0, plane, &l, pixels, buf,
                            decode_images, error))
            {
              g_free (plane);
              fclose (fp);
              gimp_image_delete (image);
              return GIMP_PDB_EXECUTION_ERROR;
            }
          g_free (plane);
        }

      if ((hdr.flags & DDSD_MIPMAPCOUNT) &&
          (hdr.caps.caps1 & DDSCAPS_MIPMAP) &&
          read_mipmaps)
        {
          for (level = 1; level < hdr.num_mipmaps; ++level)
            {
              int n = hdr.depth >> level;

              if (n < 1)
                n = 1;

              for (i = 0; i < n; ++i)
                {
                  plane = g_strdup_printf ("(z = %d)", i);

                  if (! load_layer (fp, &hdr, &d, image, level, plane, &l,
                                    pixels, buf,
                                    decode_images, error))
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
  else if (dx10hdr.arraySize > 0)
    {
      guint  i;
      gchar *elem;

      for (i = 0; i < dx10hdr.arraySize; ++i)
        {
          elem = g_strdup_printf ("(array element %d)", i);

          if (! load_layer (fp, &hdr, &d, image, 0, elem, &l, pixels, buf,
                            decode_images, error))
            {
              fclose (fp);
              gimp_image_delete (image);
              return GIMP_PDB_EXECUTION_ERROR;
            }

          if (! load_mipmaps (fp, &hdr, &d, image, elem, &l, pixels, buf,
                              read_mipmaps, decode_images, error))
            {
              fclose (fp);
              gimp_image_delete (image);
              return GIMP_PDB_EXECUTION_ERROR;
            }

          g_free (elem);
        }
    }

  gimp_progress_update (1.0);

  if (hdr.pixelfmt.flags & DDPF_PALETTEINDEXED8)
    g_free (d.palette);

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

  *ret_image = image;

  return GIMP_PDB_SUCCESS;
}

static gboolean
read_header (dds_header_t *hdr,
             FILE         *fp)
{
  guchar buf[DDS_HEADERSIZE];

  memset (hdr, 0, sizeof (dds_header_t));

  if (fread (buf, 1, DDS_HEADERSIZE, fp) != DDS_HEADERSIZE)
    return FALSE;

  hdr->magic = GETL32(buf);

  hdr->size             = GETL32 (buf + 4);
  hdr->flags            = GETL32 (buf + 8);
  hdr->height           = GETL32 (buf + 12);
  hdr->width            = GETL32 (buf + 16);
  hdr->pitch_or_linsize = GETL32 (buf + 20);
  hdr->depth            = GETL32 (buf + 24);
  hdr->num_mipmaps      = GETL32 (buf + 28);

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

  hdr->caps.caps1 = GETL32 (buf + 108);
  hdr->caps.caps2 = GETL32 (buf + 112);

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

static gboolean
read_header_dx10 (dds_header_dx10_t *hdr,
                  FILE              *fp)
{
  gchar buf[DDS_HEADERSIZE_DX10];

  memset (hdr, 0, sizeof (dds_header_dx10_t));

  if (fread (buf, 1, DDS_HEADERSIZE_DX10, fp) != DDS_HEADERSIZE_DX10)
    return FALSE;

  hdr->dxgiFormat        = GETL32 (buf);
  hdr->resourceDimension = GETL32 (buf + 4);
  hdr->miscFlag          = GETL32 (buf + 8);
  hdr->arraySize         = GETL32 (buf + 12);
  hdr->reserved          = GETL32 (buf + 16);

  return TRUE;
}

static gboolean
validate_header (dds_header_t  *hdr,
                 GError       **error)
{
  guint fourcc;

  if (hdr->magic != FOURCC ('D','D','S',' '))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                   _("Invalid DDS format magic number."));
      return FALSE;
    }

  if (hdr->pixelfmt.flags & DDPF_FOURCC)
    {
      /* These format errors are recoverable as we recognize other codes
       * allowing us to decode the image without data loss.
       * Therefore let's not bother with GUI error messaging, but still
       * print out the warning to standard error. See #5357.
       */
      if (hdr->flags & DDSD_PITCH)
        {
          g_printerr ("Warning: DDSD_PITCH is incorrectly set for DDPF_FOURCC! (recovered)\n");
          hdr->flags &= DDSD_PITCH;
        }
      if (! (hdr->flags & DDSD_LINEARSIZE))
        {
          g_printerr ("Warning: DDSD_LINEARSIZE is incorrectly not set for DDPF_FOURCC! (recovered)\n");
          hdr->flags |= DDSD_LINEARSIZE;
        }
    }
  else
    {
      if (! (hdr->flags & DDSD_PITCH))
        {
          g_printerr ("Warning: DDSD_PITCH is incorrectly not set for an uncompressed texture! (recovered)\n");
          hdr->flags |= DDSD_PITCH;
        }
      if ((hdr->flags & DDSD_LINEARSIZE))
        {
          g_printerr ("Warning: DDSD_LINEARSIZE is incorrectly set for an uncompressed texture! (recovered)\n");
          hdr->flags &= DDSD_LINEARSIZE;
        }
    }

  /*
     if ((hdr->pixelfmt.flags & DDPF_FOURCC) ==
     (hdr->pixelfmt.flags & DDPF_RGB))
     {
     g_message ("Invalid pixel format.\n");
     return 0;
     }
     */
  fourcc = GETL32(hdr->pixelfmt.fourcc);

  if ((hdr->pixelfmt.flags & DDPF_FOURCC) &&
      fourcc != FOURCC ('D','X','T','1') &&
      fourcc != FOURCC ('D','X','T','2') &&
      fourcc != FOURCC ('D','X','T','3') &&
      fourcc != FOURCC ('D','X','T','4') &&
      fourcc != FOURCC ('D','X','T','5') &&
      fourcc != FOURCC ('R','X','G','B') &&
      fourcc != FOURCC ('A','T','I','1') &&
      fourcc != FOURCC ('B','C','4','U') &&
      fourcc != FOURCC ('B','C','4','S') &&
      fourcc != FOURCC ('A','T','I','2') &&
      fourcc != FOURCC ('B','C','5','U') &&
      fourcc != FOURCC ('B','C','5','S') &&
      fourcc != FOURCC ('D','X','1','0'))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Unsupported format (FOURCC: %c%c%c%c, hex: %08x).",
                   hdr->pixelfmt.fourcc[0],
                   hdr->pixelfmt.fourcc[1],
                   hdr->pixelfmt.fourcc[2],
                   hdr->pixelfmt.fourcc[3],
                   GETL32(hdr->pixelfmt.fourcc));
      return FALSE;
    }

  if (hdr->pixelfmt.flags & DDPF_RGB)
    {
      if ((hdr->pixelfmt.bpp !=  8) &&
          (hdr->pixelfmt.bpp != 16) &&
          (hdr->pixelfmt.bpp != 24) &&
          (hdr->pixelfmt.bpp != 32))
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

      hdr->pixelfmt.flags |= DDPF_RGB;
    }
  else if (hdr->pixelfmt.flags & DDPF_PALETTEINDEXED8)
    {
      hdr->pixelfmt.flags |= DDPF_RGB;
    }

  if (! (hdr->pixelfmt.flags & DDPF_RGB) &&
      ! (hdr->pixelfmt.flags & DDPF_ALPHA) &&
      ! (hdr->pixelfmt.flags & DDPF_FOURCC) &&
      ! (hdr->pixelfmt.flags & DDPF_LUMINANCE))
    {
      g_message ("Unknown pixel format!  Taking a guess, expect trouble!");
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

  return TRUE;
}

/*
 * This function will set the necessary flags and attributes in the standard
 * dds header using the information found in the DX10 header.
 */
static gboolean
setup_dxgi_format (dds_header_t       *hdr,
                   dds_header_dx10_t  *dx10hdr,
                   GError            **error)
{
  if ((dx10hdr->resourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE2D) &&
      (dx10hdr->miscFlag & D3D10_RESOURCE_MISC_TEXTURECUBE))
    {
      hdr->caps.caps2 |= DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALL_FACES;
    }
  else if (dx10hdr->resourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE3D)
    {
      hdr->flags |= DDSD_DEPTH;
      hdr->caps.caps2 |= DDSCAPS2_VOLUME;
    }

  if ((dx10hdr->resourceDimension != D3D10_RESOURCE_DIMENSION_TEXTURE1D) &&
      (dx10hdr->resourceDimension != D3D10_RESOURCE_DIMENSION_TEXTURE2D) &&
      (dx10hdr->resourceDimension != D3D10_RESOURCE_DIMENSION_TEXTURE3D))
    return FALSE;

  /* check for a compressed DXGI format */
  if ((dx10hdr->dxgiFormat >= DXGI_FORMAT_BC1_TYPELESS) &&
      (dx10hdr->dxgiFormat <= DXGI_FORMAT_BC5_SNORM))
    {
      /* set flag and replace FOURCC */
      hdr->pixelfmt.flags |= DDPF_FOURCC;

      switch (dx10hdr->dxgiFormat)
        {
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
          PUTL32(hdr->pixelfmt.fourcc, FOURCC ('D','X','T','1'));
          break;
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
          PUTL32(hdr->pixelfmt.fourcc, FOURCC ('D','X','T','3'));
          break;
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
          PUTL32(hdr->pixelfmt.fourcc, FOURCC ('D','X','T','5'));
          break;
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
          PUTL32(hdr->pixelfmt.fourcc, FOURCC ('A','T','I','1'));
          break;
        case DXGI_FORMAT_BC4_SNORM:
          PUTL32(hdr->pixelfmt.fourcc, FOURCC ('B','C','4','S'));
          break;
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
          PUTL32(hdr->pixelfmt.fourcc, FOURCC ('A','T','I','2'));
          break;
        case DXGI_FORMAT_BC5_SNORM:
          PUTL32(hdr->pixelfmt.fourcc, FOURCC ('B','C','5','S'));
          break;
        default:
          break;
        }
    }
  else
    {
      /* unset the FOURCC flag */
      hdr->pixelfmt.flags &= ~DDPF_FOURCC;

      switch (dx10hdr->dxgiFormat)
        {
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
          hdr->pixelfmt.bpp = 32;
          hdr->pixelfmt.flags |= DDPF_ALPHAPIXELS;
          hdr->pixelfmt.rmask = 0x00ff0000;
          hdr->pixelfmt.gmask = 0x0000ff00;
          hdr->pixelfmt.bmask = 0x000000ff;
          hdr->pixelfmt.amask = 0xff000000;
          break;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
          hdr->pixelfmt.bpp = 32;
          hdr->pixelfmt.flags |= DDPF_ALPHAPIXELS;
          hdr->pixelfmt.rmask = 0x00ff0000;
          hdr->pixelfmt.gmask = 0x0000ff00;
          hdr->pixelfmt.bmask = 0x000000ff;
          hdr->pixelfmt.amask = 0x00000000;
          break;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
          hdr->pixelfmt.bpp = 32;
          hdr->pixelfmt.flags |= DDPF_ALPHAPIXELS;
          hdr->pixelfmt.rmask = 0x000000ff;
          hdr->pixelfmt.gmask = 0x0000ff00;
          hdr->pixelfmt.bmask = 0x00ff0000;
          hdr->pixelfmt.amask = 0xff000000;
          break;
        case DXGI_FORMAT_B5G6R5_UNORM:
          hdr->pixelfmt.bpp = 16;
          hdr->pixelfmt.rmask = 0x0000f800;
          hdr->pixelfmt.gmask = 0x000007e0;
          hdr->pixelfmt.bmask = 0x0000001f;
          hdr->pixelfmt.amask = 0x00000000;
          break;
        case DXGI_FORMAT_B5G5R5A1_UNORM:
          hdr->pixelfmt.bpp = 16;
          hdr->pixelfmt.rmask = 0x00007c00;
          hdr->pixelfmt.gmask = 0x000003e0;
          hdr->pixelfmt.bmask = 0x0000001f;
          hdr->pixelfmt.amask = 0x00008000;
          break;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
          hdr->pixelfmt.bpp = 32;
          hdr->pixelfmt.flags |= DDPF_ALPHAPIXELS;
          hdr->pixelfmt.rmask = 0x000003ff;
          hdr->pixelfmt.gmask = 0x000ffc00;
          hdr->pixelfmt.bmask = 0x3ff00000;
          hdr->pixelfmt.amask = 0xc0000000;
          break;
        case DXGI_FORMAT_A8_UNORM:
          hdr->pixelfmt.bpp = 8;
          hdr->pixelfmt.flags |= DDPF_ALPHA | DDPF_ALPHAPIXELS;
          hdr->pixelfmt.rmask = hdr->pixelfmt.gmask = hdr->pixelfmt.bmask = 0;
          hdr->pixelfmt.amask = 0x000000ff;
          break;
        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
          hdr->pixelfmt.bpp = 8;
          hdr->pixelfmt.rmask = 0x000000ff;
          hdr->pixelfmt.gmask = hdr->pixelfmt.bmask = hdr->pixelfmt.amask = 0;
          break;
        case DXGI_FORMAT_B4G4R4A4_UNORM:
          hdr->pixelfmt.bpp = 16;
          hdr->pixelfmt.flags |= DDPF_ALPHAPIXELS;
          hdr->pixelfmt.rmask = 0x00000f00;
          hdr->pixelfmt.gmask = 0x000000f0;
          hdr->pixelfmt.bmask = 0x0000000f;
          hdr->pixelfmt.amask = 0x0000f000;
          break;
        case DXGI_FORMAT_UNKNOWN:
          g_message ("Unknown DXGI format.  Expect problems...");
          break;
        default:  /* unsupported DXGI format */
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Unsupported DXGI format (%d)"),
                       dx10hdr->dxgiFormat);
          return FALSE;
        }
    }

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
            dds_header_t     *hdr,
            dds_load_info_t  *d,
            GimpImage        *image,
            guint             level,
            char             *prefix,
            guint            *l,
            guchar           *pixels,
            guchar           *buf,
            gboolean          decode_images,
            GError          **error)
{
  GeglBuffer    *buffer;
  const Babl    *bablfmt = NULL;
  GimpImageType  type = GIMP_RGBA_IMAGE;
  gchar         *layer_name;
  gint           x, y, z, n;
  GimpLayer     *layer;
  guint          width = hdr->width >> level;
  guint          height = hdr->height >> level;
  guint          size = hdr->pitch_or_linsize >> (2 * level);
  guint          layerw;
  gint           format = DDS_COMPRESS_NONE;

  if (width < 1) width = 1;
  if (height < 1) height = 1;

  switch (d->bpp)
    {
    case 1:
      if (hdr->pixelfmt.flags & DDPF_PALETTEINDEXED8)
        {
          type = GIMP_INDEXED_IMAGE;
        }
      else if (hdr->pixelfmt.rmask == 0xe0)
        {
          type = GIMP_RGB_IMAGE;
          bablfmt = babl_format ("R'G'B' u8");
        }
      else if (hdr->pixelfmt.flags & DDPF_ALPHA)
        {
          type = GIMP_GRAYA_IMAGE;
          bablfmt = babl_format ("Y'A u8");
        }
      else
        {
          type = GIMP_GRAY_IMAGE;
          bablfmt = babl_format ("Y' u8");
        }
      break;
    case 2:
      if ((hdr->pixelfmt.flags & (DDPF_PALETTEINDEXED8 + DDPF_ALPHA)) ==
          DDPF_PALETTEINDEXED8 + DDPF_ALPHA)
        {
          type = GIMP_INDEXEDA_IMAGE;
        }
      else if (hdr->pixelfmt.amask == 0xf000) /* RGBA4 */
        {
          type = GIMP_RGBA_IMAGE;
          bablfmt = babl_format ("R'G'B'A u8");
        }
      else if (hdr->pixelfmt.amask == 0xff00) /* L8A8 */
        {
          type = GIMP_GRAYA_IMAGE;
          bablfmt = babl_format ("Y'A u8");
        }
      else if (hdr->pixelfmt.bmask == 0x1f) /* R5G6B5 or RGB5A1 */
        {
          type = (hdr->pixelfmt.amask == 0x8000) ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE;
          bablfmt = (hdr->pixelfmt.amask == 0x8000) ? babl_format ("R'G'B'A u8") : babl_format ("R'G'B' u8");
        }
      else if (hdr->pixelfmt.rmask == 0xffff || /* L16 */
               hdr->pixelfmt.gmask == 0xffff ||
               hdr->pixelfmt.bmask == 0xffff ||
               hdr->pixelfmt.amask == 0xffff)
        {
          type = GIMP_GRAY_IMAGE;
          bablfmt = babl_format ("Y' u16");
        }
      break;
    case 3: type = GIMP_RGB_IMAGE;  bablfmt = babl_format ("R'G'B' u8");  break;
    case 4:
    case 8:
      {
        type = GIMP_RGBA_IMAGE;
        if (d->gimp_bps == 1)
          bablfmt = babl_format ("R'G'B'A u8");
        else if (d->gimp_bps == 2)
          bablfmt = babl_format ("R'G'B'A u16");
      }
      break;
    }

  layer_name = (level) ? g_strdup_printf ("mipmap %d %s", level, prefix) :
    g_strdup_printf ("main surface %s", prefix);

  layer = gimp_layer_new (image, layer_name, width, height, type, 100,
                          gimp_image_get_default_new_layer_mode (image));
  g_free (layer_name);

  gimp_image_insert_layer (image, layer, NULL, *l);

  if (type == GIMP_INDEXED_IMAGE || type == GIMP_INDEXEDA_IMAGE)
    bablfmt = gimp_drawable_get_format (GIMP_DRAWABLE (layer));

  if ((*l)++) gimp_item_set_visible (GIMP_ITEM (layer), FALSE);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  layerw = gegl_buffer_get_width (buffer);

  if (hdr->pixelfmt.flags & DDPF_FOURCC)
    {
      guint w = (width  + 3) >> 2;
      guint h = (height + 3) >> 2;

      switch (GETL32(hdr->pixelfmt.fourcc))
        {
        case FOURCC ('D','X','T','1'): format = DDS_COMPRESS_BC1; break;
        case FOURCC ('D','X','T','2'): bablfmt = premultiplied_variant (bablfmt);
        case FOURCC ('D','X','T','3'): format = DDS_COMPRESS_BC2; break;
        case FOURCC ('D','X','T','4'): bablfmt = premultiplied_variant (bablfmt);
        case FOURCC ('D','X','T','5'): format = DDS_COMPRESS_BC3; break;
        case FOURCC ('R','X','G','B'): format = DDS_COMPRESS_BC3; break;
        case FOURCC ('A','T','I','1'):
        case FOURCC ('B','C','4','U'):
        case FOURCC ('B','C','4','S'): format = DDS_COMPRESS_BC4; break;
        case FOURCC ('A','T','I','2'):
        case FOURCC ('B','C','5','U'):
        case FOURCC ('B','C','5','S'): format = DDS_COMPRESS_BC5; break;
        }

      size = w * h;
      if ((format == DDS_COMPRESS_BC1) || (format == DDS_COMPRESS_BC4))
        size *= 8;
      else
        size *= 16;
    }

  if ((hdr->flags & DDSD_LINEARSIZE) &&
      !fread (buf, size, 1, fp))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unexpected EOF.\n"));
      return FALSE;
    }

  if ((hdr->pixelfmt.flags & DDPF_RGB) ||
      (hdr->pixelfmt.flags & DDPF_ALPHA))
    {
      guint ired  = 0;
      guint iblue = 2;

      if (hdr->reserved.gimp_dds_special.magic1 == FOURCC ('G','I','M','P') &&
          hdr->reserved.gimp_dds_special.version <= 199003 &&
          hdr->reserved.gimp_dds_special.version > 0 &&
          d->bpp >= 3 && hdr->pixelfmt.amask == 0xc0000000)
        {
          /* GIMP dds plug-in versions before or equal to 199003 (3.9.91) wrote
           * the red and green channels reversed for RGB10A2. We will fix that here.
           */
          g_printerr ("Switching incorrect red and green channels in RGB10A2 dds "
                      "written by an older version of GIMP's dds plug-in.\n");
          ired = 2;
          iblue = 0;
        }

      z = 0;
      for (y = 0, n = 0; y < height; ++y, ++n)
        {
          if (n >= d->tile_height)
            {
              gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y - n, layerw, n), 0,
                               bablfmt, pixels, GEGL_AUTO_ROWSTRIDE);
              n = 0;
              gimp_progress_update ((double) y / (double) hdr->height);
            }

          if ((hdr->flags & DDSD_PITCH) &&
              ! fread (buf, width * d->bpp, 1, fp))
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Unexpected EOF.\n"));
              return FALSE;
            }

          if (!(hdr->flags & DDSD_LINEARSIZE)) z = 0;

          for (x = 0; x < layerw; ++x)
            {
              guint pixel = buf[z];
              guint pos   = (n * layerw + x) * d->gimp_bpp;

              if (d->bpp > 1) pixel += ((guint) buf[z + 1] <<  8);
              if (d->bpp > 2) pixel += ((guint) buf[z + 2] << 16);
              if (d->bpp > 3) pixel += ((guint) buf[z + 3] << 24);

              if (d->bpp >= 3)
                {
                  if (hdr->pixelfmt.amask == 0xc0000000) /* handle RGB10A2 */
                    {
                      pixels[pos + ired]  = (pixel >> d->rshift) >> 2;
                      pixels[pos + 1]     = (pixel >> d->gshift) >> 2;
                      pixels[pos + iblue] = (pixel >> d->bshift) >> 2;
                      if (hdr->pixelfmt.flags & DDPF_ALPHAPIXELS)
                        pixels[pos + 3] = (pixel >> d->ashift << (8 - d->abits) & d->amask) * 255 / d->amask;
                    }
                  else if (d->rmask > 0 && d->gmask > 0 && d->bmask > 0)
                    {
                      pixels[pos] =
                        (pixel >> d->rshift << (8 - d->rbits) & d->rmask) * 255 / d->rmask;
                      pixels[pos + 1] =
                        (pixel >> d->gshift << (8 - d->gbits) & d->gmask) * 255 / d->gmask;
                      pixels[pos + 2] =
                        (pixel >> d->bshift << (8 - d->bbits) & d->bmask) * 255 / d->bmask;
                      if (hdr->pixelfmt.flags & DDPF_ALPHAPIXELS && d->bpp == 4)
                        {
                          if (d->amask > 0)
                            pixels[pos + 3] =
                              (pixel >> d->ashift << (8 - d->abits) & d->amask) * 255 / d->amask;
                          else
                            pixels[pos + 3] = 255;
                        }
                    }
                  else if (d->bpp == 4)
                    {
                      if (d->gimp_bps == 2)
                        {
                          guint16 *pixels16 = (guint16 *) &pixels[pos];

                          if (d->rbits == 16) /* red */
                            {
                              pixels16[0] = (guint16) (pixel >> d->rshift & d->rmask);
                            }
                          else
                            {
                              pixels16[0] = 0;
                            }
                          if (d->gbits == 16) /* green */
                            {
                              pixels16[1] = (guint16) (pixel >> d->gshift & d->gmask);
                            }
                          else
                            {
                              pixels16[1] = 0;
                            }
                          if (d->bbits == 16) /* blue */
                            {
                              pixels16[2] = (guint16) (pixel >> d->bshift & d->bmask);
                            }
                          else
                            {
                              pixels16[2] = 0;
                            }
                          if (d->abits == 16) /* alpha */
                            {
                              pixels16[3] = (guint16) (pixel >> d->ashift & d->amask);
                            }
                          else
                            {
                              pixels16[3] = 0xffff;
                            }
                        }
                    }
                }
              else if (d->bpp == 2)
                {
                  if (hdr->pixelfmt.amask == 0xf000) /* RGBA4 */
                    {
                      pixels[pos] =
                        (pixel >> d->rshift << (8 - d->rbits) & d->rmask) * 255 / d->rmask;
                      pixels[pos + 1] =
                        (pixel >> d->gshift << (8 - d->gbits) & d->gmask) * 255 / d->gmask;
                      pixels[pos + 2] =
                        (pixel >> d->bshift << (8 - d->bbits) & d->bmask) * 255 / d->bmask;
                      pixels[pos + 3] =
                        (pixel >> d->ashift << (8 - d->abits) & d->amask) * 255 / d->amask;
                    }
                  else if (hdr->pixelfmt.amask == 0xff00) /* L8A8 */
                    {
                      pixels[pos] =
                        (pixel >> d->rshift << (8 - d->rbits) & d->rmask) * 255 / d->rmask;
                      pixels[pos + 1] =
                        (pixel >> d->ashift << (8 - d->abits) & d->amask) * 255 / d->amask;
                    }
                  else if (hdr->pixelfmt.bmask == 0x1f) /* R5G6B5 or RGB5A1 */
                    {
                      pixels[pos] =
                        (pixel >> d->rshift << (8 - d->rbits) & d->rmask) * 255 / d->rmask;
                      pixels[pos + 1] =
                        (pixel >> d->gshift << (8 - d->gbits) & d->gmask) * 255 / d->gmask;
                      pixels[pos + 2] =
                        (pixel >> d->bshift << (8 - d->bbits) & d->bmask) * 255 / d->bmask;
                      if (hdr->pixelfmt.amask == 0x8000)
                        {
                          pixels[pos + 3] =
                            (pixel >> d->ashift << (8 - d->abits) & d->amask) * 255 / d->amask;
                        }
                    }
                  else if (hdr->pixelfmt.rmask == 0xffff || /* L16 */
                           hdr->pixelfmt.gmask == 0xffff ||
                           hdr->pixelfmt.bmask == 0xffff ||
                           hdr->pixelfmt.amask == 0xffff)
                    {
                      guint16 *pixels16 = (guint16 *) &pixels[pos];

                      *pixels16 = (guint16) (pixel & 0xffff);
                    }
                }
              else
                {
                  if (hdr->pixelfmt.flags & DDPF_PALETTEINDEXED8)
                    {
                      pixels[pos] = pixel & 0xff;
                    }
                  else if (hdr->pixelfmt.rmask == 0xe0) /* R3G3B2 */
                    {
                      pixels[pos] =
                        (pixel >> d->rshift << (8 - d->rbits) & d->rmask) * 255 / d->rmask;
                      pixels[pos + 1] =
                        (pixel >> d->gshift << (8 - d->gbits) & d->gmask) * 255 / d->gmask;
                      pixels[pos + 2] =
                        (pixel >> d->bshift << (8 - d->bbits) & d->bmask) * 255 / d->bmask;
                    }
                  else if (hdr->pixelfmt.flags & DDPF_ALPHA)
                    {
                      pixels[pos + 0] = 255;
                      pixels[pos + 1] = pixel & 0xff;
                    }
                  else /* LUMINANCE */
                    {
                      pixels[pos] = pixel & 0xff;
                    }
                }

              z += d->bpp;
            }
        }

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y - n, layerw, n), 0,
                       bablfmt, pixels, GEGL_AUTO_ROWSTRIDE);
    }
  else if (hdr->pixelfmt.flags & DDPF_FOURCC)
    {
      guchar *dst;

      dst = g_malloc (width * height * d->gimp_bpp);
      memset (dst, 0, width * height * d->gimp_bpp);

      if (d->gimp_bpp == 4)
        {
          for (y = 0; y < height; ++y)
            for (x = 0; x < width; ++x)
              dst[y * (width * 4) + (x * 4) + 3] = 255;
        }

      dxt_decompress (dst, buf, format, size, width, height, d->gimp_bpp,
                      hdr->pixelfmt.flags & DDPF_NORMAL);

      if (format == DDS_COMPRESS_BC5 &&
          hdr->reserved.gimp_dds_special.magic1 == FOURCC ('G','I','M','P') &&
          hdr->reserved.gimp_dds_special.version > 0 &&
          hdr->reserved.gimp_dds_special.version <= 199002)
        {
          /* GIMP dds plug-in versions before 199002 == 3.9.90 wrote
           * the red and green channels reversed. We will fix that here.
           */
          g_printerr ("Switching incorrect red and green channels in BC5 dds "
                      "written by an older version of GIMP's dds plug-in.\n");

          for (y = 0; y < height; ++y)
            for (x = 0; x < width; ++x)
              {
                guchar tmpG;
                guint  pix_width = width * d->gimp_bpp;
                guint  x_width   = x * d->gimp_bpp;

                tmpG = dst[y * pix_width + x_width];
                dst[y * pix_width + x_width] = dst[y * pix_width + x_width + 1];
                dst[y * pix_width + x_width + 1] = tmpG;
              }
        }

      z = 0;
      for (y = 0, n = 0; y < height; ++y, ++n)
        {
          if (n >= d->tile_height)
            {
              gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y - n, layerw, n), 0,
                               bablfmt, pixels, GEGL_AUTO_ROWSTRIDE);
              n = 0;
              gimp_progress_update ((double)y / (double)hdr->height);
            }

          memcpy (pixels + n * layerw * d->gimp_bpp,
                  dst + y * layerw * d->gimp_bpp,
                  width * d->gimp_bpp);
        }

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y - n, layerw, n), 0,
                       bablfmt, pixels, GEGL_AUTO_ROWSTRIDE);

      g_free (dst);
    }

  gegl_buffer_flush (buffer);

  g_object_unref (buffer);

  /* gimp dds specific.  decode encoded images */
  if (decode_images &&
      hdr->reserved.gimp_dds_special.magic1 == FOURCC ('G','I','M','P') &&
      hdr->reserved.gimp_dds_special.magic2 == FOURCC ('-','D','D','S'))
    {
      switch (hdr->reserved.gimp_dds_special.extra_fourcc)
        {
        case FOURCC ('A','E','X','P'):
          decode_alpha_exp_image (GIMP_DRAWABLE (layer), FALSE);
          break;
        case FOURCC ('Y','C','G','1'):
          decode_ycocg_image (GIMP_DRAWABLE (layer), FALSE);
          break;
        case FOURCC ('Y','C','G','2'):
          decode_ycocg_scaled_image (GIMP_DRAWABLE (layer), FALSE);
          break;
        default:
          break;
        }
    }

  return TRUE;
}

static gboolean
load_mipmaps (FILE             *fp,
              dds_header_t     *hdr,
              dds_load_info_t  *d,
              GimpImage        *image,
              char             *prefix,
              unsigned int     *l,
              guchar           *pixels,
              guchar           *buf,
              gboolean          read_mipmaps,
              gboolean          decode_images,
              GError          **error)
{
  guint level;

  if ((hdr->flags & DDSD_MIPMAPCOUNT) &&
      (hdr->caps.caps1 & DDSCAPS_MIPMAP) &&
      read_mipmaps)
    {
      for (level = 1; level < hdr->num_mipmaps; ++level)
        {
          if (! load_layer (fp, hdr, d, image, level, prefix, l, pixels, buf,
                            decode_images, error))
            return FALSE;
        }
    }

  return TRUE;
}

static gboolean
load_face (FILE             *fp,
           dds_header_t     *hdr,
           dds_load_info_t  *d,
           GimpImage        *image,
           gchar            *prefix,
           guint            *l,
           guchar           *pixels,
           guchar           *buf,
           gboolean          read_mipmaps,
           gboolean          decode_images,
           GError          **error)
{
  if (! load_layer (fp, hdr, d, image, 0, prefix, l, pixels, buf,
                    decode_images, error))
    return FALSE;

  return load_mipmaps (fp, hdr, d, image, prefix, l, pixels, buf,
                       read_mipmaps, decode_images, error);
}

static guchar
color_bits (guint mask)
{
  guchar i = 0;

  while (mask)
    {
      if (mask & 1) ++i;
      mask >>= 1;
    }

  return i;
}

static guchar
color_shift (guint mask)
{
  guchar i = 0;

  if (! mask)
    return 0;

  while (!((mask >> i) & 1))
    ++i;

  return i;
}

static gboolean
load_dialog (GimpProcedure *procedure,
             GObject       *config)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *check;
  gboolean   run;

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Open DDS"));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, 1, 1, 0);
  gtk_widget_show (vbox);

  check = gimp_prop_check_button_new (config, "load-mipmaps",
                                      _("_Load mipmaps"));
  gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);

  check = gimp_prop_check_button_new (config, "decode-images",
                                      _("_Automatically decode YCoCg/AExp "
                                        "images when detected"));
  gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
