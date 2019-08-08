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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <libgimp/stdplugins-intl.h>

#include "ddsplugin.h"
#include "dds.h"
#include "dxt.h"
#include "endian_rw.h"
#include "misc.h"
#include "imath.h"

typedef struct
{
  unsigned char  rshift, gshift, bshift, ashift;
  unsigned char  rbits, gbits, bbits, abits;
  unsigned int   rmask, gmask, bmask, amask;
  unsigned int   bpp, gimp_bpp;
  int            tile_height;
  unsigned char *palette;
} dds_load_info_t;

static int           read_header       (dds_header_t      *hdr,
                                        FILE              *fp);
static int           read_header_dx10  (dds_header_dx10_t *hdr,
                                        FILE              *fp);
static int           validate_header   (dds_header_t      *hdr);
static int           setup_dxgi_format (dds_header_t      *hdr,
                                        dds_header_dx10_t *dx10hdr);
static int           load_layer        (FILE              *fp,
                                        dds_header_t      *hdr,
                                        dds_load_info_t   *d,
                                        gint32             image,
                                        unsigned int       level,
                                        char              *prefix,
                                        unsigned int      *l,
                                        guchar            *pixels,
                                        unsigned char     *buf);
static int           load_mipmaps      (FILE              *fp,
                                        dds_header_t      *hdr,
                                        dds_load_info_t   *d,
                                        gint32             image,
                                        char              *prefix,
                                        unsigned int      *l,
                                        guchar            *pixels,
                                        unsigned char     *buf);
static int           load_face         (FILE              *fp,
                                        dds_header_t      *hdr,
                                        dds_load_info_t   *d,
                                        gint32             image,
                                        char              *prefix,
                                        unsigned int      *l,
                                        guchar            *pixels,
                                        unsigned char     *buf);
static unsigned char color_bits        (unsigned int       mask);
static unsigned char color_shift       (unsigned int       mask);
static int           load_dialog       (void);

static gboolean runme = FALSE;

GimpPDBStatusType
read_dds (gchar    *filename,
          gint32   *imageID,
          gboolean  interactive_dds)
{
  gint32 image = 0;
  unsigned char *buf;
  unsigned int l = 0;
  guchar *pixels;
  gchar *tmp;
  FILE *fp;
  dds_header_t hdr;
  dds_header_dx10_t dx10hdr;
  dds_load_info_t d;
  gint *layers, layer_count;
  GimpImageBaseType type;
  int i, j;

  if (interactive_dds)
    {
      if (!load_dialog ())
        return GIMP_PDB_CANCEL;
    }

  fp = g_fopen (filename, "rb");
  if (fp == 0)
    {
      g_message ("Error opening file.\n");
      return GIMP_PDB_EXECUTION_ERROR;
    }

  if (strrchr (filename, '/'))
    tmp = g_strdup_printf ("Loading %s:", strrchr (filename, '/') + 1);
  else
    tmp = g_strdup_printf ("Loading %s:", filename);
  gimp_progress_init (tmp);
  g_free (tmp);

  /* read header */
  read_header (&hdr, fp);

  memset (&dx10hdr, 0, sizeof (dds_header_dx10_t));

  /* read DX10 header if necessary */
  if (GETL32(hdr.pixelfmt.fourcc) == FOURCC ('D','X','1','0'))
    {
      read_header_dx10(&dx10hdr, fp);

      if (!setup_dxgi_format (&hdr, &dx10hdr))
        {
          fclose (fp);
          return GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (!validate_header (&hdr))
    {
      fclose (fp);
      g_message ("Invalid DDS header!\n");
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* a lot of DDS images out there don't have this for some reason -_- */
  if (hdr.pitch_or_linsize == 0)
    {
      if (hdr.pixelfmt.flags & DDPF_FOURCC) /* assume linear size */
        {
          hdr.pitch_or_linsize = ((hdr.width + 3) >> 2) * ((hdr.height + 3) >> 2);
          switch (GETL32(hdr.pixelfmt.fourcc))
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

  if (hdr.pixelfmt.flags & DDPF_FOURCC)
    {
      switch (GETL32(hdr.pixelfmt.fourcc))
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
          if (hdr.pixelfmt.amask == 0xf000) // RGBA4
            {
              d.gimp_bpp = 4;
              type = GIMP_RGB;
            }
          else if (hdr.pixelfmt.amask == 0xff00) //L8A8
            {
              d.gimp_bpp = 2;
              type = GIMP_GRAY;
            }
          else if (hdr.pixelfmt.bmask == 0x1f) //R5G6B5 or RGB5A1
            {
              if (hdr.pixelfmt.amask == 0x8000) // RGB5A1
                d.gimp_bpp = 4;
              else
                d.gimp_bpp = 3;

              type = GIMP_RGB;
            }
          else //L16
            {
              d.gimp_bpp = 1;
              type = GIMP_GRAY;
            }
        }
      else
        {
          if (hdr.pixelfmt.flags & DDPF_PALETTEINDEXED8)
            {
              type = GIMP_INDEXED;
              d.gimp_bpp = 1;
            }
          else if (hdr.pixelfmt.rmask == 0xe0) // R3G3B2
            {
              type = GIMP_RGB;
              d.gimp_bpp = 3;
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

  image = gimp_image_new (hdr.width, hdr.height, type);

  if (image == -1)
    {
      g_message ("Can't allocate new image.\n");
      fclose (fp);
      return GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_image_set_filename (image, filename);

  if (hdr.pixelfmt.flags & DDPF_PALETTEINDEXED8)
    {
      d.palette = g_malloc (256 * 4);
      if (fread (d.palette, 1, 1024, fp) != 1024)
        {
          g_message ("Error reading palette.\n");
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

  d.rshift = color_shift (hdr.pixelfmt.rmask);
  d.gshift = color_shift (hdr.pixelfmt.gmask);
  d.bshift = color_shift (hdr.pixelfmt.bmask);
  d.ashift = color_shift (hdr.pixelfmt.amask);
  d.rbits = color_bits (hdr.pixelfmt.rmask);
  d.gbits = color_bits (hdr.pixelfmt.gmask);
  d.bbits = color_bits (hdr.pixelfmt.bmask);
  d.abits = color_bits (hdr.pixelfmt.amask);
  d.rmask = (hdr.pixelfmt.rmask >> d.rshift) << (8 - d.rbits);
  d.gmask = (hdr.pixelfmt.gmask >> d.gshift) << (8 - d.gbits);
  d.bmask = (hdr.pixelfmt.bmask >> d.bshift) << (8 - d.bbits);
  d.amask = (hdr.pixelfmt.amask >> d.ashift) << (8 - d.abits);

  if (!(hdr.caps.caps2 & DDSCAPS2_CUBEMAP) &&
      !(hdr.caps.caps2 & DDSCAPS2_VOLUME) &&
      dx10hdr.arraySize == 0)
    {
      if (!load_layer (fp, &hdr, &d, image, 0, "", &l, pixels, buf))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }
      if (!load_mipmaps (fp, &hdr, &d, image, "", &l, pixels, buf))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (hdr.caps.caps2 & DDSCAPS2_CUBEMAP)
    {
      if ((hdr.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEX) &&
          !load_face (fp, &hdr, &d, image, "(positive x)", &l, pixels, buf))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }
      if ((hdr.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) &&
          !load_face (fp, &hdr, &d, image, "(negative x)", &l, pixels, buf))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }
      if ((hdr.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEY) &&
          !load_face (fp, &hdr, &d, image, "(positive y)", &l, pixels, buf))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }
      if ((hdr.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) &&
          !load_face (fp, &hdr, &d, image, "(negative y)", &l, pixels, buf))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }
      if ((hdr.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) &&
          !load_face (fp, &hdr, &d, image, "(positive z)", &l, pixels, buf))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }
      if ((hdr.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ) &&
          !load_face (fp, &hdr, &d, image, "(negative z)", &l, pixels, buf))
        {
          fclose (fp);
          gimp_image_delete (image);
          return GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if ((hdr.caps.caps2 & DDSCAPS2_VOLUME) &&
           (hdr.flags & DDSD_DEPTH))
    {
      unsigned int i, level;
      char *plane;
      for (i = 0; i < hdr.depth; ++i)
        {
          plane = g_strdup_printf ("(z = %d)", i);
          if (!load_layer (fp, &hdr, &d, image, 0, plane, &l, pixels, buf))
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
          (dds_read_vals.mipmaps != 0))
        {
          for (level = 1; level < hdr.num_mipmaps; ++level)
            {
              int n = hdr.depth >> level;
              if (n < 1) n = 1;
              for (i = 0; i < n; ++i)
                {
                  plane = g_strdup_printf ("(z = %d)", i);
                  if (!load_layer (fp, &hdr, &d, image, level, plane, &l, pixels, buf))
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
      unsigned int i;
      char *elem;

      for (i = 0; i < dx10hdr.arraySize; ++i)
        {
          elem = g_strdup_printf ("(array element %d)", i);
          if (!load_layer (fp, &hdr, &d, image, 0, elem, &l, pixels, buf))
            {
              fclose (fp);
              gimp_image_delete (image);
              return GIMP_PDB_EXECUTION_ERROR;
            }
          if (!load_mipmaps (fp, &hdr, &d, image, elem, &l, pixels, buf))
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

  layers = gimp_image_get_layers (image, &layer_count);

  if (layers == NULL || layer_count == 0)
    {
      g_message ("Oops!  NULL image read!  Please report this!");
      return GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_image_set_active_layer (image, layers[0]);
  g_free (layers);

  *imageID = image;

  return GIMP_PDB_SUCCESS;
}

static int
read_header (dds_header_t *hdr,
             FILE         *fp)
{
  unsigned char buf[DDS_HEADERSIZE];

  memset (hdr, 0, sizeof (dds_header_t));

  if (fread (buf, 1, DDS_HEADERSIZE, fp) != DDS_HEADERSIZE)
    return 0;

  hdr->magic = GETL32(buf);

  hdr->size = GETL32(buf + 4);
  hdr->flags = GETL32(buf + 8);
  hdr->height = GETL32(buf + 12);
  hdr->width = GETL32(buf + 16);
  hdr->pitch_or_linsize = GETL32(buf + 20);
  hdr->depth = GETL32(buf + 24);
  hdr->num_mipmaps = GETL32(buf + 28);

  hdr->pixelfmt.size = GETL32(buf + 76);
  hdr->pixelfmt.flags = GETL32(buf + 80);
  hdr->pixelfmt.fourcc[0] = buf[84];
  hdr->pixelfmt.fourcc[1] = buf[85];
  hdr->pixelfmt.fourcc[2] = buf[86];
  hdr->pixelfmt.fourcc[3] = buf[87];
  hdr->pixelfmt.bpp = GETL32(buf + 88);
  hdr->pixelfmt.rmask = GETL32(buf + 92);
  hdr->pixelfmt.gmask = GETL32(buf + 96);
  hdr->pixelfmt.bmask = GETL32(buf + 100);
  hdr->pixelfmt.amask = GETL32(buf + 104);

  hdr->caps.caps1 = GETL32(buf + 108);
  hdr->caps.caps2 = GETL32(buf + 112);

  /* GIMP-DDS special info */
  if (GETL32(buf + 32) == FOURCC ('G','I','M','P') &&
      GETL32(buf + 36) == FOURCC ('-','D','D','S'))
    {
      hdr->reserved.gimp_dds_special.magic1 = GETL32(buf + 32);
      hdr->reserved.gimp_dds_special.magic2 = GETL32(buf + 36);
      hdr->reserved.gimp_dds_special.version = GETL32(buf + 40);
      hdr->reserved.gimp_dds_special.extra_fourcc = GETL32(buf + 44);
    }

  return 1;
}

static int
read_header_dx10 (dds_header_dx10_t *hdr,
                  FILE              *fp)
{
  char buf[DDS_HEADERSIZE_DX10];

  memset (hdr, 0, sizeof (dds_header_dx10_t));

  if (fread (buf, 1, DDS_HEADERSIZE_DX10, fp) != DDS_HEADERSIZE_DX10)
    return 0;

  hdr->dxgiFormat = GETL32(buf);
  hdr->resourceDimension = GETL32(buf + 4);
  hdr->miscFlag = GETL32(buf + 8);
  hdr->arraySize = GETL32(buf + 12);
  hdr->reserved = GETL32(buf + 16);

  return 1;
}

static int
validate_header (dds_header_t *hdr)
{
  unsigned int fourcc;

  if (hdr->magic != FOURCC ('D','D','S',' '))
    {
      g_message ("Invalid DDS file.\n");
      return 0;
    }

  if ((hdr->flags & DDSD_PITCH) == (hdr->flags & DDSD_LINEARSIZE))
    {
      //g_message ("Warning: DDSD_PITCH or DDSD_LINEARSIZE is not set.\n");
      if (hdr->pixelfmt.flags & DDPF_FOURCC)
        hdr->flags |= DDSD_LINEARSIZE;
      else
        hdr->flags |= DDSD_PITCH;
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
      g_message ("Unsupported format (FOURCC: %c%c%c%c, hex: %08x).\n",
                 hdr->pixelfmt.fourcc[0],
                 hdr->pixelfmt.fourcc[1],
                 hdr->pixelfmt.fourcc[2],
                 hdr->pixelfmt.fourcc[3],
                 GETL32(hdr->pixelfmt.fourcc));
      return 0;
    }

  if (hdr->pixelfmt.flags & DDPF_RGB)
    {
      if ((hdr->pixelfmt.bpp !=  8) &&
          (hdr->pixelfmt.bpp != 16) &&
          (hdr->pixelfmt.bpp != 24) &&
          (hdr->pixelfmt.bpp != 32))
        {
          g_message ("Invalid BPP.\n");
          return 0;
        }
    }
  else if (hdr->pixelfmt.flags & DDPF_LUMINANCE)
    {
      if ((hdr->pixelfmt.bpp !=  8) &&
          (hdr->pixelfmt.bpp != 16))
        {
          g_message ("Invalid BPP.\n");
          return 0;
        }

      hdr->pixelfmt.flags |= DDPF_RGB;
    }
  else if (hdr->pixelfmt.flags & DDPF_PALETTEINDEXED8)
    {
      hdr->pixelfmt.flags |= DDPF_RGB;
    }

  if (!(hdr->pixelfmt.flags & DDPF_RGB) &&
      !(hdr->pixelfmt.flags & DDPF_ALPHA) &&
      !(hdr->pixelfmt.flags & DDPF_FOURCC) &&
      !(hdr->pixelfmt.flags & DDPF_LUMINANCE))
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
              hdr->pixelfmt.flags |= DDPF_RGB;
              break;
            default:
              g_message ("Invalid pixel format.");
              return 0;
            }
          break;
        }
    }

  return 1;
}

/*
 * This function will set the necessary flags and attributes in the standard
 * dds header using the information found in the DX10 header.
 */
static int
setup_dxgi_format (dds_header_t      *hdr,
                   dds_header_dx10_t *dx10hdr)
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
    return 0;

  // check for a compressed DXGI format
  if ((dx10hdr->dxgiFormat >= DXGI_FORMAT_BC1_TYPELESS) &&
      (dx10hdr->dxgiFormat <= DXGI_FORMAT_BC5_SNORM))
    {
      // set flag and replace FOURCC
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
          g_message ("Unsupported DXGI format (%d)", dx10hdr->dxgiFormat);
          return 0;
        }
    }

  return 1;
}


static const Babl*
premultiplied_variant (const Babl* format)
{
  if (format == babl_format ("R'G'B'A u8"))
    return babl_format ("R'aG'aB'aA u8");
  else
    g_printerr ("Add format %s to premultiplied_variant () %s: %d\n", babl_get_name (format), __FILE__, __LINE__);
  return format;
}

static int
load_layer (FILE            *fp,
            dds_header_t    *hdr,
            dds_load_info_t *d,
            gint32           image,
            unsigned int     level,
            char            *prefix,
            unsigned int    *l,
            guchar          *pixels,
            unsigned char   *buf)
{
  GeglBuffer    *buffer;
  const Babl    *bablfmt = NULL;
  GimpImageType  type = GIMP_RGBA_IMAGE;
  gchar         *layer_name;
  gint           x, y, z, n;
  gint32         layer;
  unsigned int   width = hdr->width >> level;
  unsigned int   height = hdr->height >> level;
  unsigned int   size = hdr->pitch_or_linsize >> (2 * level);
  unsigned int   layerw;
  int            format = DDS_COMPRESS_NONE;

  if (width < 1) width = 1;
  if (height < 1) height = 1;

  switch (d->bpp)
    {
    case 1:
      if (hdr->pixelfmt.flags & DDPF_PALETTEINDEXED8)
        {
          type = GIMP_INDEXED_IMAGE;
          bablfmt = babl_format ("R'G'B' u8");
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
      if (hdr->pixelfmt.amask == 0xf000) /* RGBA4 */
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
      else /* L16 */
        {
          type = GIMP_GRAY_IMAGE;
          bablfmt = babl_format ("Y' u8");
        }
      break;
    case 3: type = GIMP_RGB_IMAGE;  bablfmt = babl_format ("R'G'B' u8");  break;
    case 4: type = GIMP_RGBA_IMAGE; bablfmt = babl_format ("R'G'B'A u8"); break;
    }

  layer_name = (level) ? g_strdup_printf ("mipmap %d %s", level, prefix) :
    g_strdup_printf ("main surface %s", prefix);

  layer = gimp_layer_new (image, layer_name, width, height, type, 100,
                          GIMP_LAYER_MODE_NORMAL);
  g_free (layer_name);

  gimp_image_insert_layer (image, layer, 0, *l);

  if ((*l)++) gimp_item_set_visible (layer, FALSE);

  buffer = gimp_drawable_get_buffer (layer);

  layerw = gegl_buffer_get_width (buffer);

  if (hdr->pixelfmt.flags & DDPF_FOURCC)
    {
      unsigned int w = (width  + 3) >> 2;
      unsigned int h = (height + 3) >> 2;

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
      g_message ("Unexpected EOF.\n");
      return 0;
    }

  if ((hdr->pixelfmt.flags & DDPF_RGB) ||
      (hdr->pixelfmt.flags & DDPF_ALPHA))
    {
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

          if ((hdr->flags & DDSD_PITCH) &&
              !fread (buf, width * d->bpp, 1, fp))
            {
              g_message ("Unexpected EOF.\n");
              return 0;
            }

          if (!(hdr->flags & DDSD_LINEARSIZE)) z = 0;

          for (x = 0; x < layerw; ++x)
            {
              unsigned int pixel = buf[z];
              unsigned int pos = (n * layerw + x) * d->gimp_bpp;

              if (d->bpp > 1) pixel += ((unsigned int)buf[z + 1] <<  8);
              if (d->bpp > 2) pixel += ((unsigned int)buf[z + 2] << 16);
              if (d->bpp > 3) pixel += ((unsigned int)buf[z + 3] << 24);

              if (d->bpp >= 3)
                {
                  if (hdr->pixelfmt.amask == 0xc0000000) // handle RGB10A2
                    {
                      pixels[pos + 0] = (pixel >> d->bshift) >> 2;
                      pixels[pos + 1] = (pixel >> d->gshift) >> 2;
                      pixels[pos + 2] = (pixel >> d->rshift) >> 2;
                      if (hdr->pixelfmt.flags & DDPF_ALPHAPIXELS)
                        pixels[pos + 3] = (pixel >> d->ashift << (8 - d->abits) & d->amask) * 255 / d->amask;
                    }
                  else
                    {
                      pixels[pos] =
                        (pixel >> d->rshift << (8 - d->rbits) & d->rmask) * 255 / d->rmask;
                      pixels[pos + 1] =
                        (pixel >> d->gshift << (8 - d->gbits) & d->gmask) * 255 / d->gmask;
                      pixels[pos + 2] =
                        (pixel >> d->bshift << (8 - d->bbits) & d->bmask) * 255 / d->bmask;
                      if (hdr->pixelfmt.flags & DDPF_ALPHAPIXELS)
                        {
                          pixels[pos + 3] =
                            (pixel >> d->ashift << (8 - d->abits) & d->amask) * 255 / d->amask;
                        }
                    }
                }
              else if (d->bpp == 2)
                {
                  if (hdr->pixelfmt.amask == 0xf000) //RGBA4
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
                  else if (hdr->pixelfmt.amask == 0xff00) //L8A8
                    {
                      pixels[pos] =
                        (pixel >> d->rshift << (8 - d->rbits) & d->rmask) * 255 / d->rmask;
                      pixels[pos + 1] =
                        (pixel >> d->ashift << (8 - d->abits) & d->amask) * 255 / d->amask;
                    }
                  else if (hdr->pixelfmt.bmask == 0x1f) //R5G6B5 or RGB5A1
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
                  else //L16
                    pixels[pos] = (unsigned char)(255 * ((float)(pixel & 0xffff) / 65535.0f));
                }
              else
                {
                  if (hdr->pixelfmt.flags & DDPF_PALETTEINDEXED8)
                    {
                      pixels[pos] = pixel & 0xff;
                    }
                  else if (hdr->pixelfmt.rmask == 0xe0) // R3G3B2
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
                  else // LUMINANCE
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
      unsigned char *dst;

      if (!(hdr->flags & DDSD_LINEARSIZE))
        {
          g_message ("Image marked as compressed, but DDSD_LINEARSIZE is not set.\n");
          return 0;
        }

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
  if (dds_read_vals.decode_images &&
      hdr->reserved.gimp_dds_special.magic1 == FOURCC ('G','I','M','P') &&
      hdr->reserved.gimp_dds_special.magic2 == FOURCC ('-','D','D','S'))
    {
      switch (hdr->reserved.gimp_dds_special.extra_fourcc)
        {
        case FOURCC ('A','E','X','P'):
          decode_alpha_exp_image (layer, FALSE);
          break;
        case FOURCC ('Y','C','G','1'):
          decode_ycocg_image (layer, FALSE);
          break;
        case FOURCC ('Y','C','G','2'):
          decode_ycocg_scaled_image (layer, FALSE);
          break;
        default:
          break;
        }
    }

  return 1;
}

static int
load_mipmaps (FILE            *fp,
              dds_header_t    *hdr,
              dds_load_info_t *d,
              gint32           image,
              char            *prefix,
              unsigned int    *l,
              guchar          *pixels,
              unsigned char   *buf)
{
  unsigned int level;

  if ((hdr->flags & DDSD_MIPMAPCOUNT) &&
      (hdr->caps.caps1 & DDSCAPS_MIPMAP) &&
      (dds_read_vals.mipmaps != 0))
    {
      for (level = 1; level < hdr->num_mipmaps; ++level)
        {
          if (!load_layer (fp, hdr, d, image, level, prefix, l, pixels, buf))
            return 0;
        }
    }

  return 1;
}

static int
load_face (FILE            *fp,
           dds_header_t    *hdr,
           dds_load_info_t *d,
           gint32           image,
           char            *prefix,
           unsigned int    *l,
           guchar          *pixels,
           unsigned char   *buf)
{
  if (!load_layer (fp, hdr, d, image, 0, prefix, l, pixels, buf))
    return 0;

  return load_mipmaps (fp, hdr, d, image, prefix, l, pixels, buf);
}

static unsigned char
color_bits (unsigned int mask)
{
  unsigned char i = 0;

  while (mask)
    {
      if (mask & 1) ++i;
      mask >>= 1;
    }

  return i;
}

static unsigned char
color_shift (unsigned int mask)
{
  guchar i = 0;

  if (! mask)
    return 0;

  while (!((mask >> i) & 1))
    ++i;

  return i;
}

static void
load_dialog_response (GtkWidget *widget,
                      gint       response_id,
                      gpointer   data)
{
  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      runme = TRUE;
    default:
      gtk_widget_destroy (widget);
      break;
    }
}

static void
toggle_clicked (GtkWidget *widget,
                gpointer   data)
{
  int *flag = (int*) data;
  (*flag) = !(*flag);
}

static int
load_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *vbox;
  GtkWidget *check;

  dlg = gimp_dialog_new (_("Load DDS"), "dds", NULL, GTK_WIN_POS_MOUSE,
                         gimp_standard_help_func, LOAD_PROC,
                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,
                         NULL);

  g_signal_connect (dlg, "response",
                    G_CALLBACK (load_dialog_response),
                    0);
  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      vbox, 1, 1, 0);
  gtk_widget_show (vbox);

  check = gtk_check_button_new_with_mnemonic (_("_Load mipmaps"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), dds_read_vals.mipmaps);
  g_signal_connect (check, "clicked",
                    G_CALLBACK (toggle_clicked), &dds_read_vals.mipmaps);
  gtk_box_pack_start (GTK_BOX (vbox), check, 1, 1, 0);
  gtk_widget_show (check);

  check = gtk_check_button_new_with_mnemonic (_("_Automatically decode YCoCg/AExp images when detected"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), dds_read_vals.decode_images);
  g_signal_connect (check, "clicked",
                    G_CALLBACK (toggle_clicked), &dds_read_vals.decode_images);
  gtk_box_pack_start (GTK_BOX (vbox), check, 1, 1, 0);
  gtk_widget_show (check);

  gtk_widget_show (dlg);

  runme = FALSE;

  gtk_main ();

  return runme;
}
