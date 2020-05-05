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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <libgimp/stdplugins-intl.h>

#include "ddswrite.h"
#include "dds.h"
#include "dxt.h"
#include "mipmap.h"
#include "endian_rw.h"
#include "imath.h"
#include "color.h"


static gboolean   write_image (FILE          *fp,
                               GimpImage     *image,
                               GimpDrawable  *drawable,
                               GObject       *config);
static gboolean   save_dialog (GimpImage     *image,
                               GimpDrawable  *drawable,
                               GimpProcedure *procedure,
                               GObject       *config);


static const char *cubemap_face_names[4][6] =
{
  {
    "positive x", "negative x",
    "positive y", "negative y",
    "positive z", "negative z"
  },
  {
    "pos x", "neg x",
    "pos y", "neg y",
    "pos z", "neg z",
  },
  {
    "+x", "-x",
    "+y", "-y",
    "+z", "-z"
  },
  {
    "right", "left",
    "top", "bottom",
    "back", "front"
  }
};

static GimpLayer *cubemap_faces[6];
static gboolean   is_cubemap            = FALSE;
static gboolean   is_volume             = FALSE;
static gboolean   is_array              = FALSE;
static gboolean   is_mipmap_chain_valid = FALSE;

static GtkWidget *compress_opt;
static GtkWidget *format_opt;
static GtkWidget *mipmap_opt;
static GtkWidget *mipmap_filter_opt;
static GtkWidget *mipmap_wrap_opt;
static GtkWidget *transparent_spin;
static GtkWidget *srgb_check;
static GtkWidget *gamma_check;
static GtkWidget *gamma_spin;
static GtkWidget *pm_check;
static GtkWidget *alpha_coverage_check;
static GtkWidget *alpha_test_threshold_spin;

static struct
{
  gint         format;
  DXGI_FORMAT  dxgi_format;
  gint         bpp;
  gint         alpha;
  guint        rmask;
  guint        gmask;
  guint        bmask;
  guint        amask;
} format_info[] =
{
  { DDS_FORMAT_RGB8,    DXGI_FORMAT_UNKNOWN,           3, 0, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000},
  { DDS_FORMAT_RGBA8,   DXGI_FORMAT_B8G8R8A8_UNORM,    4, 1, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000},
  { DDS_FORMAT_BGR8,    DXGI_FORMAT_UNKNOWN,           3, 0, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000},
  { DDS_FORMAT_ABGR8,   DXGI_FORMAT_R8G8B8A8_UNORM,    4, 1, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000},
  { DDS_FORMAT_R5G6B5,  DXGI_FORMAT_B5G6R5_UNORM,      2, 0, 0x0000f800, 0x000007e0, 0x0000001f, 0x00000000},
  { DDS_FORMAT_RGBA4,   DXGI_FORMAT_B4G4R4A4_UNORM,    2, 1, 0x00000f00, 0x000000f0, 0x0000000f, 0x0000f000},
  { DDS_FORMAT_RGB5A1,  DXGI_FORMAT_B5G5R5A1_UNORM,    2, 1, 0x00007c00, 0x000003e0, 0x0000001f, 0x00008000},
  { DDS_FORMAT_RGB10A2, DXGI_FORMAT_R10G10B10A2_UNORM, 4, 1, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000},
  { DDS_FORMAT_R3G3B2,  DXGI_FORMAT_UNKNOWN,           1, 0, 0x000000e0, 0x0000001c, 0x00000003, 0x00000000},
  { DDS_FORMAT_A8,      DXGI_FORMAT_A8_UNORM,          1, 0, 0x00000000, 0x00000000, 0x00000000, 0x000000ff},
  { DDS_FORMAT_L8,      DXGI_FORMAT_R8_UNORM,          1, 0, 0x000000ff, 0x000000ff, 0x000000ff, 0x00000000},
  { DDS_FORMAT_L8A8,    DXGI_FORMAT_UNKNOWN,           2, 1, 0x000000ff, 0x000000ff, 0x000000ff, 0x0000ff00},
  { DDS_FORMAT_AEXP,    DXGI_FORMAT_B8G8R8A8_UNORM,    4, 1, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000},
  { DDS_FORMAT_YCOCG,   DXGI_FORMAT_B8G8R8A8_UNORM,    4, 1, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000}
};


static gboolean
check_mipmaps (GimpImage *image,
               gint       savetype)
{
  GList         *layers;
  GList         *list;
  gint           num_layers;
  gint           i, j;
  gint           w, h;
  gint           mipw, miph;
  gint           num_mipmaps;
  gint           num_surfaces = 0;
  gint           min_surfaces = 1;
  gint           max_surfaces = 1;
  gboolean       valid        = TRUE;
  GimpImageType  type;

  /* not handling volume maps for the moment... */
  if (savetype == DDS_SAVE_VOLUMEMAP)
    return 0;

  if (savetype == DDS_SAVE_CUBEMAP)
    {
      min_surfaces = 6;
      max_surfaces = 6;
    }
  else if (savetype == DDS_SAVE_ARRAY)
    {
      min_surfaces = 2;
      max_surfaces = INT_MAX;
    }

  layers = gimp_image_list_layers (image);
  num_layers = g_list_length (layers);

  w = gimp_image_width (image);
  h = gimp_image_height (image);

  num_mipmaps = get_num_mipmaps (w, h);

  type = gimp_drawable_type (layers->data);

  for (list = layers; list; list = g_list_next (list))
    {
      if (type != gimp_drawable_type (list->data))
        return 0;

      if ((gimp_drawable_width  (list->data) == w) &&
          (gimp_drawable_height (list->data) == h))
        ++num_surfaces;
    }

  if ((num_surfaces < min_surfaces) ||
      (num_surfaces > max_surfaces) ||
      (num_layers != (num_surfaces * num_mipmaps)))
    return 0;

  for (i = 0; valid && i < num_layers; i += num_mipmaps)
    {
      GimpDrawable *drawable = g_list_nth_data (layers, i);

      if ((gimp_drawable_width  (drawable) != w) ||
          (gimp_drawable_height (drawable) != h))
        {
          valid = FALSE;
          break;
        }

      for (j = 1; j < num_mipmaps; ++j)
        {
          drawable = g_list_nth_data (layers, i + j);

          mipw = w >> j;
          miph = h >> j;
          if (mipw < 1) mipw = 1;
          if (miph < 1) miph = 1;
          if ((gimp_drawable_width  (drawable) != mipw) ||
              (gimp_drawable_height (drawable) != miph))
            {
              valid = FALSE;
              break;
            }
        }
    }

  return valid;
}

static gboolean
check_cubemap (GimpImage *image)
{
  GList         *layers;
  GList         *list;
  gint           num_layers;
  gboolean       cubemap = TRUE;
  gint           i, j, k;
  gint           w, h;
  gchar         *layer_name;
  GimpImageType  type;

  layers = gimp_image_list_layers (image);
  num_layers = g_list_length (layers);

  if (num_layers < 6)
    return FALSE;

  /* check for a valid cubemap with mipmap layers */
  if (num_layers > 6)
    {
      /* check that mipmap layers are in order for a cubemap */
      if (! check_mipmaps (image, DDS_SAVE_CUBEMAP))
        return FALSE;

      /* invalidate cubemap faces */
      for (i = 0; i < 6; ++i)
        cubemap_faces[i] = NULL;

      /* find the mipmap level 0 layers */
      w = gimp_image_width (image);
      h = gimp_image_height (image);

      for (i = 0, list = layers;
           i < num_layers;
           ++i, list = g_list_next (list))
        {
          GimpDrawable *drawable = list->data;

          if ((gimp_drawable_width  (drawable) != w) ||
              (gimp_drawable_height (drawable) != h))
            continue;

          layer_name = (char *) gimp_item_get_name (GIMP_ITEM (drawable));
          for (j = 0; j < 6; ++j)
            {
              for (k = 0; k < 4; ++k)
                {
                  if (strstr (layer_name, cubemap_face_names[k][j]))
                    {
                      if (cubemap_faces[j] == NULL)
                        {
                          cubemap_faces[j] = GIMP_LAYER (drawable);
                          break;
                        }
                    }
                }
            }
        }

      /* check for 6 valid faces */
      for (i = 0; i < 6; ++i)
        {
          if (cubemap_faces[i] == NULL)
            {
              cubemap = FALSE;
              break;
            }
        }

      /* make sure they are all the same type */
      if (cubemap)
        {
          type = gimp_drawable_type (GIMP_DRAWABLE (cubemap_faces[0]));
          for (i = 1; i < 6 && cubemap; ++i)
            {
              if (gimp_drawable_type (GIMP_DRAWABLE (cubemap_faces[i])) != type)
                cubemap = FALSE;
            }
        }
    }

  if (num_layers == 6)
    {
      /* invalidate cubemap faces */
      for (i = 0; i < 6; ++i)
        cubemap_faces[i] = NULL;

      for (i = 0, list = layers;
           i < 6;
           ++i, list = g_list_next (layers))
        {
          GimpLayer *layer = list->data;

          layer_name = (gchar *) gimp_item_get_name (GIMP_ITEM (layer));

          for (j = 0; j < 6; ++j)
            {
              for (k = 0; k < 4; ++k)
                {
                  if (strstr (layer_name, cubemap_face_names[k][j]))
                    {
                      if (cubemap_faces[j] == NULL)
                        {
                          cubemap_faces[j] = layer;
                          break;
                        }
                    }
                }
            }
        }

      /* check for 6 valid faces */
      for (i = 0; i < 6; ++i)
        {
          if (cubemap_faces[i] == NULL)
            {
              cubemap = FALSE;
              break;
            }
        }

      /* make sure they are all the same size */
      if (cubemap)
        {
          w = gimp_drawable_width (GIMP_DRAWABLE (cubemap_faces[0]));
          h = gimp_drawable_height (GIMP_DRAWABLE (cubemap_faces[0]));

          for (i = 1; i < 6 && cubemap; ++i)
            {
              if ((gimp_drawable_width  (GIMP_DRAWABLE (cubemap_faces[i])) != w) ||
                  (gimp_drawable_height (GIMP_DRAWABLE (cubemap_faces[i])) != h))
                cubemap = FALSE;
            }
        }

      /* make sure they are all the same type */
      if (cubemap)
        {
          type = gimp_drawable_type (GIMP_DRAWABLE (cubemap_faces[0]));
          for (i = 1; i < 6 && cubemap; ++i)
            {
              if (gimp_drawable_type (GIMP_DRAWABLE (cubemap_faces[i])) != type)
                cubemap = FALSE;
            }
        }
    }

  return cubemap;
}

static gboolean
check_volume (GimpImage *image)
{
  GList         *layers;
  GList         *list;
  gint           num_layers;
  gboolean       volume = FALSE;
  gint           i;
  gint           w, h;
  GimpImageType  type;

  layers = gimp_image_list_layers (image);
  num_layers = g_list_length (layers);

  if (num_layers > 1)
    {
      volume = TRUE;

      /* make sure all layers are the same size */
      w = gimp_drawable_width  (layers->data);
      h = gimp_drawable_height (layers->data);

      for (i = 1, list = layers->next;
           i < num_layers && volume;
           ++i, list = g_list_next (list))
        {
          if ((gimp_drawable_width  (list->data) != w) ||
              (gimp_drawable_height (list->data) != h))
            volume = FALSE;
        }

      if (volume)
        {
          /* make sure all layers are the same type */
          type = gimp_drawable_type (layers->data);

          for (i = 1, list = layers->next;
               i < num_layers && volume;
               ++i, list = g_list_next (list))
            {
              if (gimp_drawable_type (list->data) != type)
                volume = FALSE;
            }
        }
    }

  return volume;
}

static gboolean
check_array (GimpImage *image)
{
  GList         *layers;
  gint           num_layers;
  gboolean       array = FALSE;
  gint           i;
  gint           w, h;
  GimpImageType  type;

  if (check_mipmaps (image, DDS_SAVE_ARRAY))
    return 1;

  layers = gimp_image_list_layers (image);
  num_layers = g_list_length (layers);

  if (num_layers > 1)
    {
      GList *list;

      array = TRUE;

      /* make sure all layers are the same size */
      w = gimp_drawable_width  (layers->data);
      h = gimp_drawable_height (layers->data);

      for (i = 1, list = g_list_next (layers);
           i < num_layers && array;
           ++i, list = g_list_next (list))
        {
          if ((gimp_drawable_width  (list->data)  != w) ||
              (gimp_drawable_height (list->data) != h))
            array = FALSE;
        }

      if (array)
        {
          /* make sure all layers are the same type */
          type = gimp_drawable_type (layers->data);

          for (i = 1, list = g_list_next (layers);
               i < num_layers;
               ++i, list = g_list_next (list))
            {
              if (gimp_drawable_type (list->data) != type)
                {
                  array = FALSE;
                  break;
                }
            }
        }
    }

  g_list_free (layers);

  return array;
}

static int
get_array_size (GimpImage *image)
{
  GList *layers;
  GList *list;
  gint   num_layers;
  gint   i;
  gint   w, h;
  gint   elements = 0;

  layers = gimp_image_list_layers (image);
  num_layers = g_list_length (layers);

  w = gimp_image_width (image);
  h = gimp_image_height (image);

  for (i = 0, list = layers;
       i < num_layers;
       ++i, list = g_list_next (list))
    {
      if ((gimp_drawable_width  (list->data) == w) &&
          (gimp_drawable_height (list->data) == h))
        {
          elements++;
        }
    }

  g_list_free (layers);

  return elements;
}

GimpPDBStatusType
write_dds (GFile         *file,
           GimpImage     *image,
           GimpDrawable  *drawable,
           gboolean       interactive,
           GimpProcedure *procedure,
           GObject       *config)
{
  gchar *filename;
  FILE  *fp;
  gint   rc = 0;
  gint   compression;
  gint   mipmaps;
  gint   savetype;

  g_object_get (config,
                "compression-format", &compression,
                "mipmaps",            &mipmaps,
                "save-type",          &savetype,
                NULL);

  is_mipmap_chain_valid = check_mipmaps (image, savetype);

  is_cubemap = check_cubemap (image);
  is_volume  = check_volume (image);
  is_array   = check_array (image);

  if (interactive)
    {
      if (! is_mipmap_chain_valid &&
          mipmaps == DDS_MIPMAP_EXISTING)
        mipmaps = DDS_MIPMAP_NONE;

      if (! save_dialog (image, drawable, procedure, config))
        return GIMP_PDB_CANCEL;
    }
  else
    {
      if (savetype == DDS_SAVE_CUBEMAP && ! is_cubemap)
        {
          g_message ("DDS: Cannot save image as cube map");
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if (savetype == DDS_SAVE_VOLUMEMAP && ! is_volume)
        {
          g_message ("DDS: Cannot save image as volume map");
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if (savetype == DDS_SAVE_VOLUMEMAP &&
          compression != DDS_COMPRESS_NONE)
        {
          g_message ("DDS: Cannot save volume map with compression");
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if (mipmaps == DDS_MIPMAP_EXISTING &&
          ! is_mipmap_chain_valid)
        {
          g_message ("DDS: Cannot save with existing mipmaps as the mipmap chain is incomplete");
          return GIMP_PDB_EXECUTION_ERROR;
        }
    }

  filename = g_file_get_path (file);
  fp = g_fopen (filename, "wb");
  g_free (filename);

  if (! fp)
    {
      g_message ("Error opening %s", filename);
      return GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_progress_init_printf ("Saving %s:", gimp_file_get_utf8_name (file));

  rc = write_image (fp, image, drawable, config);

  fclose (fp);

  return rc ? GIMP_PDB_SUCCESS : GIMP_PDB_EXECUTION_ERROR;
}

static void
swap_rb (unsigned char *pixels,
         unsigned int   n,
         int            bpp)
{
  unsigned int  i;
  unsigned char t;

  for (i = 0; i < n; ++i)
    {
      t = pixels[bpp * i + 0];
      pixels[bpp * i + 0] = pixels[bpp * i + 2];
      pixels[bpp * i + 2] = t;
    }
}

static void
alpha_exp (unsigned char *dst,
           int            r,
           int            g,
           int            b,
           int            a)
{
  float ar, ag, ab, aa;

  ar = (float)r / 255.0f;
  ag = (float)g / 255.0f;
  ab = (float)b / 255.0f;

  aa = MAX (ar, MAX (ag, ab));

  if (aa < 1e-04f)
    {
      dst[0] = b;
      dst[1] = g;
      dst[2] = r;
      dst[3] = 255;
      return;
    }

  ar /= aa;
  ag /= aa;
  ab /= aa;

  r = (int)floorf (255.0f * ar + 0.5f);
  g = (int)floorf (255.0f * ag + 0.5f);
  b = (int)floorf (255.0f * ab + 0.5f);
  a = (int)floorf (255.0f * aa + 0.5f);

  dst[0] = MAX (0, MIN (255, b));
  dst[1] = MAX (0, MIN (255, g));
  dst[2] = MAX (0, MIN (255, r));
  dst[3] = MAX (0, MIN (255, a));
}

static void
convert_pixels (unsigned char *dst,
                unsigned char *src,
                int            format,
                int            w,
                int            h,
                int            d,
                int            bpp,
                unsigned char *palette,
                int            mipmaps)
{
  unsigned int  i, num_pixels;
  unsigned char r, g, b, a;

  if (d > 0)
    num_pixels = get_volume_mipmapped_size (w, h, d, 1, 0, mipmaps, DDS_COMPRESS_NONE);
  else
    num_pixels = get_mipmapped_size (w, h, 1, 0, mipmaps, DDS_COMPRESS_NONE);

  for (i = 0; i < num_pixels; ++i)
    {
      if (bpp == 1)
        {
          if (palette)
            {
              r = palette[3 * src[i] + 0];
              g = palette[3 * src[i] + 1];
              b = palette[3 * src[i] + 2];
            }
          else
            r = g = b = src[i];

          if (format == DDS_FORMAT_A8)
            a = src[i];
          else
            a = 255;
        }
      else if (bpp == 2)
        {
          r = g = b = src[2 * i];
          a = src[2 * i + 1];
        }
      else if (bpp == 3)
        {
          b = src[3 * i + 0];
          g = src[3 * i + 1];
          r = src[3 * i + 2];
          a = 255;
        }
      else
        {
          b = src[4 * i + 0];
          g = src[4 * i + 1];
          r = src[4 * i + 2];
          a = src[4 * i + 3];
        }

      switch (format)
        {
        case DDS_FORMAT_RGB8:
          dst[3 * i + 0] = b;
          dst[3 * i + 1] = g;
          dst[3 * i + 2] = r;
          break;
        case DDS_FORMAT_RGBA8:
          dst[4 * i + 0] = b;
          dst[4 * i + 1] = g;
          dst[4 * i + 2] = r;
          dst[4 * i + 3] = a;
          break;
        case DDS_FORMAT_BGR8:
          dst[3 * i + 0] = r;
          dst[3 * i + 1] = g;
          dst[3 * i + 2] = b;
          break;
        case DDS_FORMAT_ABGR8:
          dst[4 * i + 0] = r;
          dst[4 * i + 1] = g;
          dst[4 * i + 2] = b;
          dst[4 * i + 3] = a;
          break;
        case DDS_FORMAT_R5G6B5:
          PUTL16(&dst[2 * i], pack_r5g6b5(r, g, b));
          break;
        case DDS_FORMAT_RGBA4:
          PUTL16(&dst[2 * i], pack_rgba4(r, g, b, a));
          break;
        case DDS_FORMAT_RGB5A1:
          PUTL16(&dst[2 * i], pack_rgb5a1(r, g, b, a));
          break;
        case DDS_FORMAT_RGB10A2:
          PUTL32(&dst[4 * i], pack_rgb10a2(r, g, b, a));
          break;
        case DDS_FORMAT_R3G3B2:
          dst[i] = pack_r3g3b2(r, g, b);
          break;
        case DDS_FORMAT_A8:
          dst[i] = a;
          break;
        case DDS_FORMAT_L8:
          dst[i] = rgb_to_luminance (r, g, b);
          break;
        case DDS_FORMAT_L8A8:
          dst[2 * i + 0] = rgb_to_luminance (r, g, b);
          dst[2 * i + 1] = a;
          break;
        case DDS_FORMAT_YCOCG:
          dst[4 * i] = a;
          RGB_to_YCoCg (&dst[4 * i], r, g, b);
          break;
        case DDS_FORMAT_AEXP:
          alpha_exp (&dst[4 * i], r, g, b, a);
          break;
        default:
          break;
        }
    }
}

static void
get_mipmap_chain (unsigned char *dst,
                  int            w,
                  int            h,
                  int            bpp,
                  GimpImage     *image,
                  GimpDrawable  *drawable)
{
  GList      *layers;
  GList      *list;
  gint        num_layers;
  GeglBuffer *buffer;
  const Babl *format;
  gint        i;
  gint        idx = 0;
  gint        offset;
  gint        mipw, miph;

  if (bpp == 1)
    format = babl_format ("Y' u8");
  else if (bpp == 2)
    format = babl_format ("Y'A u8");
  else if (bpp == 3)
    format = babl_format ("R'G'B' u8");
  else
    format = babl_format ("R'G'B'A u8");

  layers = gimp_image_list_layers (image);
  num_layers = g_list_length (layers);

  for (i = 0, list = layers;
       i < num_layers;
       ++i, list = g_list_next (list))
    {
      if (list->data == drawable)
        {
          idx = i;
          break;
        }
    }

  if (i == num_layers)
    return;

  offset = 0;

  while (get_next_mipmap_dimensions (&mipw, &miph, w, h))
    {
      buffer = gimp_drawable_get_buffer (g_list_nth_data (layers, ++idx));

      if ((gegl_buffer_get_width (buffer)  != mipw) ||
          (gegl_buffer_get_height (buffer) != miph))
        {
          g_object_unref (buffer);
          return;
        }

      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, mipw, miph), 1.0, format,
                       dst + offset, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      g_object_unref (buffer);

      /* we need BGRX or BGRA */
      if (bpp >= 3)
        swap_rb (dst + offset, mipw * miph, bpp);

      offset += (mipw * miph * bpp);
      w = mipw;
      h = miph;
    }
}

static void
write_layer (FILE         *fp,
             GimpImage    *image,
             GimpDrawable *drawable,
             GObject      *config,
             int           w,
             int           h,
             int           bpp,
             int           fmtbpp,
             int           num_mipmaps)
{
  GeglBuffer        *buffer;
  const Babl        *format;
  GimpImageBaseType  basetype;
  GimpImageType      type;
  guchar            *src;
  guchar            *dst;
  guchar            *fmtdst;
  guchar            *tmp;
  guchar            *palette = NULL;
  gint               i, c;
  gint               x, y;
  gint               size;
  gint               fmtsize;
  gint               offset;
  gint               colors;
  gint               compression;
  gint               mipmaps;
  gint               pixel_format;
  gboolean           perceptual_metric;
  gint               flags = 0;

  g_object_get (config,
                "compression-format", &compression,
                "mipmaps",            &mipmaps,
                "format",             &pixel_format,
                "perceptual-metric",  &perceptual_metric,
                NULL);

  basetype = gimp_image_base_type (image);
  type = gimp_drawable_type (drawable);

  buffer = gimp_drawable_get_buffer (drawable);

  src = g_malloc (w * h * bpp);

  if (bpp == 1)
    format = babl_format ("Y' u8");
  else if (bpp == 2)
    format = babl_format ("Y'A u8");
  else if (bpp == 3)
    format = babl_format ("R'G'B' u8");
  else
    format = babl_format ("R'G'B'A u8");

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, w, h), 1.0, format, src,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (basetype == GIMP_INDEXED)
    {
      palette = gimp_image_get_colormap (image, &colors);

      if (type == GIMP_INDEXEDA_IMAGE)
        {
          tmp = g_malloc (w * h);
          for (i = 0; i < w * h; ++i)
            tmp[i] = src[2 * i];
          g_free (src);
          src = tmp;
          bpp = 1;
        }
    }

  /* we want and assume BGRA ordered pixels for bpp >= 3 from here and
     onwards */
  if (bpp >= 3)
    swap_rb (src, w * h, bpp);

  if (compression == DDS_COMPRESS_BC3N)
    {
      if (bpp != 4)
        {
          fmtsize = w * h * 4;
          fmtdst = g_malloc (fmtsize);
          convert_pixels (fmtdst, src, DDS_FORMAT_RGBA8, w, h, 0, bpp,
                          palette, 1);
          g_free (src);
          src = fmtdst;
          bpp = 4;
        }

      for (y = 0; y < h; ++y)
        {
          for (x = 0; x < w; ++x)
            {
              /* set alpha to red (x) */
              src[y * (w * 4) + (x * 4) + 3] =
                src[y * (w * 4) + (x * 4) + 2];
              /* set red to 1 */
              src[y * (w * 4) + (x * 4) + 2] = 255;
            }
        }
    }

  /* RXGB (Doom3) */
  if (compression == DDS_COMPRESS_RXGB)
    {
      if (bpp != 4)
        {
          fmtsize = w * h * 4;
          fmtdst = g_malloc (fmtsize);
          convert_pixels (fmtdst, src, DDS_FORMAT_RGBA8, w, h, 0, bpp,
                          palette, 1);
          g_free (src);
          src = fmtdst;
          bpp = 4;
        }

      for (y = 0; y < h; ++y)
        {
          for (x = 0; x < w; ++x)
            {
              /* swap red and alpha */
              c = src[y * (w * 4) + (x * 4) + 3];
              src[y * (w * 4) + (x * 4) + 3] =
                src[y * (w * 4) + (x * 4) + 2];
              src[y * (w * 4) + (x * 4) + 2] = c;
            }
        }
    }

  if (compression == DDS_COMPRESS_YCOCG ||
      compression == DDS_COMPRESS_YCOCGS) /* convert to YCoCG */
    {
      fmtsize = w * h * 4;
      fmtdst = g_malloc (fmtsize);
      convert_pixels (fmtdst, src, DDS_FORMAT_YCOCG, w, h, 0, bpp,
                      palette, 1);
      g_free (src);
      src = fmtdst;
      bpp = 4;
    }

  if (compression == DDS_COMPRESS_AEXP)
    {
      fmtsize = w * h * 4;
      fmtdst = g_malloc (fmtsize);
      convert_pixels (fmtdst, src, DDS_FORMAT_AEXP, w, h, 0, bpp,
                      palette, 1);
      g_free (src);
      src = fmtdst;
      bpp = 4;
    }

  if (compression == DDS_COMPRESS_NONE)
    {
      if (num_mipmaps > 1)
        {
          /* pre-convert indexed images to RGB for better quality mipmaps
             if a pixel format conversion is requested */
          if (pixel_format > DDS_FORMAT_DEFAULT && basetype == GIMP_INDEXED)
            {
              fmtsize = get_mipmapped_size (w, h, 3, 0, num_mipmaps, DDS_COMPRESS_NONE);
              fmtdst = g_malloc (fmtsize);
              convert_pixels (fmtdst, src, DDS_FORMAT_RGB8, w, h, 0, bpp,
                              palette, 1);
              g_free (src);
              src = fmtdst;
              bpp = 3;
              palette = NULL;
            }

          size = get_mipmapped_size (w, h, bpp, 0, num_mipmaps,
                                     DDS_COMPRESS_NONE);
          dst = g_malloc (size);
          if (mipmaps == DDS_MIPMAP_GENERATE)
            {
              gint     mipmap_filter;
              gint     mipmap_wrap;
              gboolean gamma_correct;
              gboolean srgb;
              gdouble  gamma;
              gboolean preserve_alpha_coverage;
              gdouble  alpha_test_threshold;

              g_object_get (config,
                            "mipmap-filter",           &mipmap_filter,
                            "mipmap-wrap",             &mipmap_wrap,
                            "gamma-correct",           &gamma_correct,
                            "srgb",                    &srgb,
                            "gamma",                   &gamma,
                            "preserve-alpha-coverage", &preserve_alpha_coverage,
                            "alpha-test-threshold",    &alpha_test_threshold,
                            NULL);

              generate_mipmaps (dst, src, w, h, bpp, palette != NULL,
                                num_mipmaps,
                                mipmap_filter,
                                mipmap_wrap,
                                gamma_correct + srgb,
                                gamma,
                                preserve_alpha_coverage,
                                alpha_test_threshold);
            }
          else
            {
              memcpy (dst, src, w * h * bpp);
              get_mipmap_chain (dst + (w * h * bpp), w, h, bpp, image, drawable);
            }

          if (pixel_format > DDS_FORMAT_DEFAULT)
            {
              fmtsize = get_mipmapped_size (w, h, fmtbpp, 0, num_mipmaps,
                                            DDS_COMPRESS_NONE);
              fmtdst = g_malloc (fmtsize);

              convert_pixels (fmtdst, dst, pixel_format, w, h, 0, bpp,
                              palette, num_mipmaps);

              g_free (dst);
              dst = fmtdst;
              bpp = fmtbpp;
            }

          offset = 0;

          for (i = 0; i < num_mipmaps; ++i)
            {
              size = get_mipmapped_size (w, h, bpp, i, 1, DDS_COMPRESS_NONE);
              fwrite (dst + offset, 1, size, fp);
              offset += size;
            }

          g_free (dst);
        }
      else
        {
          if (pixel_format > DDS_FORMAT_DEFAULT)
            {
              fmtdst = g_malloc (h * w * fmtbpp);
              convert_pixels (fmtdst, src, pixel_format, w, h, 0, bpp,
                              palette, 1);
              g_free (src);
              src = fmtdst;
              bpp = fmtbpp;
            }

          fwrite (src, 1, h * w * bpp, fp);
        }
    }
  else
    {
      size = get_mipmapped_size (w, h, bpp, 0, num_mipmaps, compression);

      dst = g_malloc (size);

      if (basetype == GIMP_INDEXED)
        {
          fmtsize = get_mipmapped_size (w, h, 3, 0, num_mipmaps,
                                        DDS_COMPRESS_NONE);
          fmtdst = g_malloc (fmtsize);
          convert_pixels (fmtdst, src, DDS_FORMAT_RGB8, w, h, 0, bpp,
                          palette, num_mipmaps);
          g_free (src);
          src = fmtdst;
          bpp = 3;
        }

      if (num_mipmaps > 1)
        {
          fmtsize = get_mipmapped_size (w, h, bpp, 0, num_mipmaps,
                                        DDS_COMPRESS_NONE);
          fmtdst = g_malloc (fmtsize);
          if (mipmaps == DDS_MIPMAP_GENERATE)
            {
              gint     mipmap_filter;
              gint     mipmap_wrap;
              gboolean gamma_correct;
              gboolean srgb;
              gdouble  gamma;
              gboolean preserve_alpha_coverage;
              gdouble  alpha_test_threshold;

              g_object_get (config,
                            "mipmap-filter",           &mipmap_filter,
                            "mipmap-wrap",             &mipmap_wrap,
                            "gamma-correct",           &gamma_correct,
                            "srgb",                    &srgb,
                            "gamma",                   &gamma,
                            "preserve-alpha-coverage", &preserve_alpha_coverage,
                            "alpha-test-threshold",    &alpha_test_threshold,
                            NULL);

              generate_mipmaps (fmtdst, src, w, h, bpp, 0, num_mipmaps,
                                mipmap_filter,
                                mipmap_wrap,
                                gamma_correct + srgb,
                                gamma,
                                preserve_alpha_coverage,
                                alpha_test_threshold);
            }
          else
            {
              memcpy (fmtdst, src, w * h * bpp);
              get_mipmap_chain (fmtdst + (w * h * bpp), w, h, bpp, image, drawable);
            }

          g_free (src);
          src = fmtdst;
        }

      flags = 0;
      if (perceptual_metric)
        flags |= DXT_PERCEPTUAL;

      dxt_compress (dst, src, compression, w, h, bpp, num_mipmaps, flags);

      fwrite (dst, 1, size, fp);

      g_free (dst);
    }

  g_free (src);

  g_object_unref (buffer);
}

static void
write_volume_mipmaps (FILE      *fp,
                      GimpImage *image,
                      GObject   *config,
                      GList     *layers,
                      int        w,
                      int        h,
                      int        d,
                      int        bpp,
                      int        fmtbpp,
                      int        num_mipmaps)
{
  GList             *list;
  gint               i;
  gint               size;
  gint               offset;
  gint               colors;
  guchar            *src;
  guchar            *dst;
  guchar            *tmp;
  guchar            *fmtdst;
  guchar            *palette = 0;
  GeglBuffer        *buffer;
  const Babl        *format;
  GimpImageBaseType  type;
  gint               compression;
  gint               pixel_format;
  gint               mipmap_filter;
  gint               mipmap_wrap;
  gboolean           gamma_correct;
  gboolean           srgb;
  gdouble            gamma;

  g_object_get (config,
                "compression-format", &compression,
                "format",             &pixel_format,
                "mipmap-filter",      &mipmap_filter,
                "mipmap-wrap",        &mipmap_wrap,
                "gamma-correct",      &gamma_correct,
                "srgb",               &srgb,
                "gamma",              &gamma,
                NULL);

  type = gimp_image_base_type (image);

  if (compression != DDS_COMPRESS_NONE)
    return;

  src = g_malloc (w * h * bpp * d);

  if (bpp == 1)
    format = babl_format ("Y' u8");
  else if (bpp == 2)
    format = babl_format ("Y'A u8");
  else if (bpp == 3)
    format = babl_format ("R'G'B' u8");
  else
    format = babl_format ("R'G'B'A u8");

  if (gimp_image_base_type (image) == GIMP_INDEXED)
    palette = gimp_image_get_colormap (image, &colors);

  offset = 0;
  for (i = 0, list = layers;
       i < d;
       ++i, list = g_list_next (list))
    {
      buffer = gimp_drawable_get_buffer (list->data);
      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, w, h), 1.0, format,
                       src + offset, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      offset += (w * h * bpp);
      g_object_unref (buffer);
    }

  if (gimp_drawable_type (layers->data) == GIMP_INDEXEDA_IMAGE)
    {
      tmp = g_malloc (w * h * d);
      for (i = 0; i < w * h * d; ++i)
        tmp[i] = src[2 * i];
      g_free (src);
      src = tmp;
      bpp = 1;
    }

  /* we want and assume BGRA ordered pixels for bpp >= 3 from here and
     onwards */
  if (bpp >= 3)
    swap_rb (src, w * h * d, bpp);

  /* pre-convert indexed images to RGB for better mipmaps if a
     pixel format conversion is requested */
  if (pixel_format > DDS_FORMAT_DEFAULT && type == GIMP_INDEXED)
    {
      size = get_volume_mipmapped_size (w, h, d, 3, 0, num_mipmaps,
                                        DDS_COMPRESS_NONE);
      dst = g_malloc (size);
      convert_pixels (dst, src, DDS_FORMAT_RGB8, w, h, d, bpp, palette, 1);
      g_free (src);
      src = dst;
      bpp = 3;
      palette = NULL;
    }

  size = get_volume_mipmapped_size (w, h, d, bpp, 0, num_mipmaps,
                                    compression);

  dst = g_malloc (size);

  offset = get_volume_mipmapped_size (w, h, d, bpp, 0, 1,
                                      compression);

  generate_volume_mipmaps (dst, src, w, h, d, bpp,
                           palette != NULL, num_mipmaps,
                           mipmap_filter,
                           mipmap_wrap,
                           gamma_correct + srgb,
                           gamma);

  if (pixel_format > DDS_FORMAT_DEFAULT)
    {
      size = get_volume_mipmapped_size (w, h, d, fmtbpp, 0, num_mipmaps,
                                        compression);
      offset = get_volume_mipmapped_size (w, h, d, fmtbpp, 0, 1,
                                          compression);
      fmtdst = g_malloc (size);

      convert_pixels (fmtdst, dst, pixel_format, w, h, d, bpp,
                      palette, num_mipmaps);
      g_free (dst);
      dst = fmtdst;
    }

  fwrite (dst + offset, 1, size, fp);

  g_free (src);
  g_free (dst);
}

static gboolean
write_image (FILE         *fp,
             GimpImage    *image,
             GimpDrawable *drawable,
             GObject      *config)
{
  GimpImageType      drawable_type;
  GimpImageBaseType  basetype;
  gint               i, w, h;
  gint               bpp = 0;
  gint               fmtbpp = 0;
  gint               has_alpha = 0;
  gint               num_mipmaps;
  guchar             hdr[DDS_HEADERSIZE];
  guchar             hdr10[DDS_HEADERSIZE_DX10];
  guint              flags = 0, pflags = 0, caps = 0, caps2 = 0, size = 0;
  guint              rmask = 0, gmask = 0, bmask = 0, amask = 0;
  guint              fourcc = 0;
  DXGI_FORMAT        dxgi_format = DXGI_FORMAT_UNKNOWN;
  gint32             num_layers;
  GList             *layers;
  GList             *list;
  guchar            *cmap;
  gint               colors;
  guchar             zero[4] = {0, 0, 0, 0};
  gint               is_dx10 = 0;
  gint               array_size = 1;
  gint               compression;
  gint               mipmaps;
  gint               savetype;
  gint               pixel_format;
  gint               transindex;

  g_object_get (config,
                "compression-format", &compression,
                "mipmaps",            &mipmaps,
                "save-type",          &savetype,
                "format",             &pixel_format,
                "transparent-index",  &transindex,
                NULL);

  layers = gimp_image_list_layers (image);
  num_layers = g_list_length (layers);

  if (mipmaps == DDS_MIPMAP_EXISTING)
    drawable = layers->data;

  if (savetype == DDS_SAVE_SELECTED_LAYER)
    {
      w = gimp_drawable_width (drawable);
      h = gimp_drawable_height (drawable);
    }
  else
    {
      w = gimp_image_width (image);
      h = gimp_image_height (image);
    }

  basetype = gimp_image_base_type (image);
  drawable_type = gimp_drawable_type (drawable);

  switch (drawable_type)
    {
    case GIMP_RGB_IMAGE:      bpp = 3; break;
    case GIMP_RGBA_IMAGE:     bpp = 4; break;
    case GIMP_GRAY_IMAGE:     bpp = 1; break;
    case GIMP_GRAYA_IMAGE:    bpp = 2; break;
    case GIMP_INDEXED_IMAGE:  bpp = 1; break;
    case GIMP_INDEXEDA_IMAGE: bpp = 2; break;
    default:
                              break;
    }

  if (pixel_format > DDS_FORMAT_DEFAULT)
    {
      for (i = 0; ; ++i)
        {
          if (format_info[i].format == pixel_format)
            {
              fmtbpp = format_info[i].bpp;
              has_alpha = format_info[i].alpha;
              rmask = format_info[i].rmask;
              gmask = format_info[i].gmask;
              bmask = format_info[i].bmask;
              amask = format_info[i].amask;
              dxgi_format = format_info[i].dxgi_format;
              break;
            }
        }
    }
  else if (bpp == 1)
    {
      if (basetype == GIMP_INDEXED)
        {
          fmtbpp = 1;
          has_alpha = 0;
          rmask = bmask = gmask = amask = 0;
        }
      else
        {
          fmtbpp = 1;
          has_alpha = 0;
          rmask = 0x000000ff;
          gmask = bmask = amask = 0;
          dxgi_format = DXGI_FORMAT_R8_UNORM;
        }
    }
  else if (bpp == 2)
    {
      if (basetype == GIMP_INDEXED)
        {
          fmtbpp = 1;
          has_alpha = 0;
          rmask = gmask = bmask = amask = 0;
        }
      else
        {
          fmtbpp = 2;
          has_alpha = 1;
          rmask = 0x000000ff;
          gmask = 0x000000ff;
          bmask = 0x000000ff;
          amask = 0x0000ff00;
        }
    }
  else if (bpp == 3)
    {
      fmtbpp = 3;
      rmask = 0x00ff0000;
      gmask = 0x0000ff00;
      bmask = 0x000000ff;
      amask = 0x00000000;
    }
  else
    {
      fmtbpp = 4;
      has_alpha = 1;
      rmask = 0x00ff0000;
      gmask = 0x0000ff00;
      bmask = 0x000000ff;
      amask = 0xff000000;
      dxgi_format = DXGI_FORMAT_B8G8R8A8_UNORM;
    }

  memset (hdr, 0, DDS_HEADERSIZE);

  PUTL32(hdr,       FOURCC ('D','D','S',' '));
  PUTL32(hdr + 4,   124);
  PUTL32(hdr + 12,  h);
  PUTL32(hdr + 16,  w);
  PUTL32(hdr + 76,  32);

  if (compression == DDS_COMPRESS_NONE)
    {
      PUTL32(hdr + 88,  fmtbpp << 3);
      PUTL32(hdr + 92,  rmask);
      PUTL32(hdr + 96,  gmask);
      PUTL32(hdr + 100, bmask);
      PUTL32(hdr + 104, amask);
    }

  /*
     put some information in the reserved area to identify the origin
     of the image
     */
  PUTL32(hdr + 32, FOURCC ('G','I','M','P'));
  PUTL32(hdr + 36, FOURCC ('-','D','D','S'));
  PUTL32(hdr + 40, DDS_PLUGIN_VERSION);

  flags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;

  caps = DDSCAPS_TEXTURE;
  if (mipmaps)
    {
      flags |= DDSD_MIPMAPCOUNT;
      caps |= (DDSCAPS_COMPLEX | DDSCAPS_MIPMAP);
      num_mipmaps = get_num_mipmaps (w, h);
    }
  else
    {
      num_mipmaps = 1;
    }

  if ((savetype == DDS_SAVE_CUBEMAP) && is_cubemap)
    {
      caps |= DDSCAPS_COMPLEX;
      caps2 |= (DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALL_FACES);
    }
  else if ((savetype == DDS_SAVE_VOLUMEMAP) && is_volume)
    {
      PUTL32(hdr + 24, num_layers); /* depth */
      flags |= DDSD_DEPTH;
      caps |= DDSCAPS_COMPLEX;
      caps2 |= DDSCAPS2_VOLUME;
    }

  PUTL32(hdr + 28,  num_mipmaps);
  PUTL32(hdr + 108, caps);
  PUTL32(hdr + 112, caps2);

  if (compression == DDS_COMPRESS_NONE)
    {
      flags |= DDSD_PITCH;

      if (pixel_format > DDS_FORMAT_DEFAULT)
        {
          if (pixel_format == DDS_FORMAT_A8)
            pflags |= DDPF_ALPHA;
          else
            {
              if (((fmtbpp == 1) || (pixel_format == DDS_FORMAT_L8A8)) &&
                  (pixel_format != DDS_FORMAT_R3G3B2))
                pflags |= DDPF_LUMINANCE;
              else
                pflags |= DDPF_RGB;
            }
        }
      else
        {
          if (bpp == 1)
            {
              if (basetype == GIMP_INDEXED)
                pflags |= DDPF_PALETTEINDEXED8;
              else
                pflags |= DDPF_LUMINANCE;
            }
          else if ((bpp == 2) && (basetype == GIMP_INDEXED))
            {
              pflags |= DDPF_PALETTEINDEXED8;
            }
          else
            {
              pflags |= DDPF_RGB;
            }
        }

      if (has_alpha)
        pflags |= DDPF_ALPHAPIXELS;

      PUTL32 (hdr + 8,  flags);
      PUTL32 (hdr + 20, w * fmtbpp); /* pitch */
      PUTL32 (hdr + 80, pflags);

      /*
       * write extra fourcc info - this is special to GIMP DDS. When the image
       * is read by the plugin, we can detect the added information to decode
       * the pixels
       */
      if (pixel_format == DDS_FORMAT_AEXP)
        {
          PUTL32 (hdr + 44, FOURCC ('A','E','X','P'));
        }
      else if (pixel_format == DDS_FORMAT_YCOCG)
        {
          PUTL32 (hdr + 44, FOURCC ('Y','C','G','1'));
        }
    }
  else
    {
      flags |= DDSD_LINEARSIZE;
      pflags = DDPF_FOURCC;

      switch (compression)
        {
        case DDS_COMPRESS_BC1:
          fourcc = FOURCC ('D','X','T','1');
          dxgi_format = DXGI_FORMAT_BC1_UNORM;
          break;

        case DDS_COMPRESS_BC2:
          fourcc = FOURCC ('D','X','T','3');
          dxgi_format = DXGI_FORMAT_BC2_UNORM;
          break;

        case DDS_COMPRESS_BC3:
        case DDS_COMPRESS_BC3N:
        case DDS_COMPRESS_YCOCG:
        case DDS_COMPRESS_YCOCGS:
        case DDS_COMPRESS_AEXP:
          fourcc = FOURCC ('D','X','T','5');
          dxgi_format = DXGI_FORMAT_BC3_UNORM;
          break;

        case DDS_COMPRESS_RXGB:
          fourcc = FOURCC ('R','X','G','B');
          dxgi_format = DXGI_FORMAT_BC3_UNORM;
          break;

        case DDS_COMPRESS_BC4:
          fourcc = FOURCC ('A','T','I','1');
          dxgi_format = DXGI_FORMAT_BC4_UNORM;
          //is_dx10 = 1;
          break;

        case DDS_COMPRESS_BC5:
          fourcc = FOURCC ('A','T','I','2');
          dxgi_format = DXGI_FORMAT_BC5_UNORM;
          //is_dx10 = 1;
          break;
        }

      if ((compression == DDS_COMPRESS_BC3N) ||
          (compression == DDS_COMPRESS_RXGB))
        {
          pflags |= DDPF_NORMAL;
        }

      PUTL32 (hdr + 8,  flags);
      PUTL32 (hdr + 80, pflags);
      PUTL32 (hdr + 84, fourcc);

      size = ((w + 3) >> 2) * ((h + 3) >> 2);
      if ((compression == DDS_COMPRESS_BC1) ||
          (compression == DDS_COMPRESS_BC4))
        size *= 8;
      else
        size *= 16;

      PUTL32 (hdr + 20, size); /* linear size */

      /*
       * write extra fourcc info - this is special to GIMP DDS. When the image
       * is read by the plugin, we can detect the added information to decode
       * the pixels
       */
      if (compression == DDS_COMPRESS_AEXP)
        {
          PUTL32 (hdr + 44, FOURCC ('A','E','X','P'));
        }
      else if (compression == DDS_COMPRESS_YCOCG)
        {
          PUTL32 (hdr + 44, FOURCC ('Y','C','G','1'));
        }
      else if (compression == DDS_COMPRESS_YCOCGS)
        {
          PUTL32 (hdr + 44, FOURCC ('Y','C','G','2'));
        }
    }

  /* texture arrays require a DX10 header */
  if (savetype == DDS_SAVE_ARRAY)
    is_dx10 = 1;

  if (is_dx10)
    {
      array_size = ((savetype == DDS_SAVE_SELECTED_LAYER) ?
                    1 : get_array_size (image));

      PUTL32 (hdr10 +  0, dxgi_format);
      PUTL32 (hdr10 +  4, D3D10_RESOURCE_DIMENSION_TEXTURE2D);
      PUTL32 (hdr10 +  8, 0);
      PUTL32 (hdr10 + 12, array_size);
      PUTL32 (hdr10 + 16, 0);

      /* update main header accordingly */
      PUTL32 (hdr + 80, pflags | DDPF_FOURCC);
      PUTL32 (hdr + 84, FOURCC ('D','X','1','0'));
    }

  fwrite (hdr, DDS_HEADERSIZE, 1, fp);

  if (is_dx10)
    fwrite (hdr10, DDS_HEADERSIZE_DX10, 1, fp);

  /* write palette for indexed images */
  if ((basetype == GIMP_INDEXED) &&
      (pixel_format == DDS_FORMAT_DEFAULT) &&
      (compression == DDS_COMPRESS_NONE))
    {
      cmap = gimp_image_get_colormap (image, &colors);

      for (i = 0; i < colors; ++i)
        {
          fwrite (&cmap[3 * i], 1, 3, fp);
          if (i == transindex)
            fputc (0, fp);
          else
            fputc (255, fp);
        }

      for (; i < 256; ++i)
        fwrite (zero, 1, 4, fp);
    }

  if (savetype == DDS_SAVE_CUBEMAP)
    {
      for (i = 0; i < 6; ++i)
        {
          write_layer (fp, image, GIMP_DRAWABLE (cubemap_faces[i]), config,
                       w, h, bpp, fmtbpp,
                       num_mipmaps);
          gimp_progress_update ((float)(i + 1) / 6.0);
        }
    }
  else if (savetype == DDS_SAVE_VOLUMEMAP)
    {
      for (i = 0, list = layers;
           i < num_layers;
           ++i, list = g_list_next (layers))
        {
          write_layer (fp, image, list->data, config,
                       w, h, bpp, fmtbpp, 1);
          gimp_progress_update ((float)i / (float)num_layers);
        }

      if (num_mipmaps > 1)
        write_volume_mipmaps (fp, image, config, layers, w, h, num_layers,
                              bpp, fmtbpp, num_mipmaps);
    }
  else if (savetype == DDS_SAVE_ARRAY)
    {
      for (i = 0, list = layers;
           i < num_layers;
           ++i, list = g_list_next (layers))
        {
          if ((gimp_drawable_width  (list->data) == w) &&
              (gimp_drawable_height (list->data) == h))
            {
              write_layer (fp, image, list->data, config,
                           w, h, bpp, fmtbpp, num_mipmaps);
            }

          gimp_progress_update ((float)i / (float)num_layers);
        }
    }
  else
    {
      write_layer (fp, image, drawable, config,
                   w, h, bpp, fmtbpp, num_mipmaps);
    }

  gimp_progress_update (1.0);

  return TRUE;
}

static gboolean
combo_sensitivity_func (gint     value,
                        gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (data));

  if (gimp_int_store_lookup_by_value (model, value, &iter))
    {
      gpointer insensitive;

      gtk_tree_model_get (model, &iter,
                          GIMP_INT_STORE_USER_DATA, &insensitive,
                          -1);

      return ! GPOINTER_TO_INT (insensitive);
    }

  return TRUE;
}

static void
combo_set_item_sensitive (GtkWidget *widget,
                          gint       value,
                          gboolean   sensitive)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));

  if (gimp_int_store_lookup_by_value (model, value, &iter))
    {
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          GIMP_INT_STORE_USER_DATA,
                          ! GINT_TO_POINTER (sensitive),
                          -1);
    }
}

static void
config_notify (GObject          *config,
               const GParamSpec *pspec,
               GimpImage        *image)
{
  if (! strcmp (pspec->name, "compression-format"))
    {
      gint compression;

      g_object_get (config,
                    "compression-format", &compression,
                    NULL);

      if (format_opt)
        gtk_widget_set_sensitive (format_opt,
                                  compression == DDS_COMPRESS_NONE);

      if (pm_check)
        gtk_widget_set_sensitive (pm_check,
                                  compression != DDS_COMPRESS_NONE);
    }
  else if (! strcmp (pspec->name, "save-type"))
    {
      gint savetype;

      g_object_get (config,
                    "save-type", &savetype,
                    NULL);

      switch (savetype)
        {
        case DDS_SAVE_SELECTED_LAYER:
        case DDS_SAVE_CUBEMAP:
        case DDS_SAVE_ARRAY:
          gtk_widget_set_sensitive (compress_opt, TRUE);
          break;

        case DDS_SAVE_VOLUMEMAP:
          g_object_set (config,
                        "compression-format", DDS_COMPRESS_NONE,
                        NULL);
          gtk_widget_set_sensitive (compress_opt, FALSE);
          break;
        }

      if (mipmap_opt)
        combo_set_item_sensitive (mipmap_opt, DDS_MIPMAP_EXISTING,
                                  check_mipmaps (image, savetype));
    }
  else if (! strcmp (pspec->name, "mipmaps"))
    {
      gint     mipmaps;
      gboolean gamma_correct;
      gboolean srgb;
      gboolean preserve_alpha_coverage;

      g_object_get (config,
                    "mipmaps",                 &mipmaps,
                    "gamma-correct",           &gamma_correct,
                    "srgb",                    &srgb,
                    "preserve-alpha-coverage", &preserve_alpha_coverage,
                    NULL);

      if (mipmap_filter_opt)
        gtk_widget_set_sensitive (mipmap_filter_opt,
                                  mipmaps == DDS_MIPMAP_GENERATE);

      if (mipmap_wrap_opt)
        gtk_widget_set_sensitive (mipmap_wrap_opt,
                                  mipmaps == DDS_MIPMAP_GENERATE);

      if (gamma_check)
        gtk_widget_set_sensitive (gamma_check,
                                  mipmaps == DDS_MIPMAP_GENERATE);

      if (srgb_check)
        gtk_widget_set_sensitive (srgb_check,
                                  (mipmaps == DDS_MIPMAP_GENERATE) &&
                                  gamma_correct);

      if (gamma_spin)
        gtk_widget_set_sensitive (gamma_spin,
                                  (mipmaps == DDS_MIPMAP_GENERATE) &&
                                  gamma_correct &&
                                  ! srgb);

      if (alpha_coverage_check)
        gtk_widget_set_sensitive (alpha_coverage_check,
                                  mipmaps == DDS_MIPMAP_GENERATE);

      if (alpha_test_threshold_spin)
        gtk_widget_set_sensitive (alpha_test_threshold_spin,
                                  (mipmaps == DDS_MIPMAP_GENERATE) &&
                                  preserve_alpha_coverage);
    }
  else if (! strcmp (pspec->name, "transparent-color"))
    {
      GimpImageBaseType base_type;
      gboolean          transparent_color;

      base_type = gimp_image_base_type (image);

      g_object_get (config,
                    "transparent-color", &transparent_color,
                    NULL);

      if (transparent_spin)
        gtk_widget_set_sensitive (transparent_spin,
                                  transparent_color &&
                                  base_type == GIMP_INDEXED);
    }
  else if (! strcmp (pspec->name, "gamma-correct"))
    {
      gboolean gamma_correct;
      gboolean srgb;

      g_object_get (config,
                    "gamma-correct", &gamma_correct,
                    "srgb",          &srgb,
                    NULL);

      if (srgb_check)
        gtk_widget_set_sensitive (srgb_check, gamma_correct);

      if (gamma_spin)
        gtk_widget_set_sensitive (gamma_spin, gamma_correct && ! srgb);
    }
  else if (! strcmp (pspec->name, "srgb"))
    {
      gboolean gamma_correct;
      gboolean srgb;

      g_object_get (config,
                    "gamma-correct", &gamma_correct,
                    "srgb",          &srgb,
                    NULL);

      if (gamma_spin)
        gtk_widget_set_sensitive (gamma_spin, gamma_correct && ! srgb);
    }
  else if (! strcmp (pspec->name, "preserve-alpha-coverage"))
    {
      gboolean preserve_alpha_coverage;

      g_object_get (config,
                    "preserve-alpha-coverage", &preserve_alpha_coverage,
                    NULL);

      if (alpha_test_threshold_spin)
        gtk_widget_set_sensitive (alpha_test_threshold_spin,
                                  preserve_alpha_coverage);
    }
}

static gboolean
save_dialog (GimpImage     *image,
             GimpDrawable  *drawable,
             GimpProcedure *procedure,
             GObject       *config)
{
  GtkWidget    *dialog;
  GtkWidget    *vbox;
  GtkWidget    *hbox;
  GtkWidget    *grid;
  GtkListStore *store;
  GtkWidget    *opt;
  GtkWidget    *check;
  GtkWidget    *frame;
  gboolean      run;

  if (is_cubemap || is_volume || is_array)
    g_object_set (config,
                  "save-type", DDS_SAVE_SELECTED_LAYER,
                  NULL);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Export Image as DDS"));

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  store = gimp_int_store_new ("None",                  DDS_COMPRESS_NONE,
                              "BC1 / DXT1",            DDS_COMPRESS_BC1,
                              "BC2 / DXT3",            DDS_COMPRESS_BC2,
                              "BC3 / DXT5",            DDS_COMPRESS_BC3,
                              "BC3nm / DXT5nm",        DDS_COMPRESS_BC3N,
                              "BC4 / ATI1 (3Dc+)",     DDS_COMPRESS_BC4,
                              "BC5 / ATI2 (3Dc)",      DDS_COMPRESS_BC5,
                              "RXGB (DXT5)",           DDS_COMPRESS_RXGB,
                              "Alpha Exponent (DXT5)", DDS_COMPRESS_AEXP,
                              "YCoCg (DXT5)",          DDS_COMPRESS_YCOCG,
                              "YCoCg scaled (DXT5)",   DDS_COMPRESS_YCOCGS,
                              NULL);
  compress_opt = gimp_prop_int_combo_box_new (config, "compression-format",
                                              GIMP_INT_STORE (store));
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Compression:"),
                            0.0, 0.5,
                            compress_opt, 1);

  pm_check = gimp_prop_check_button_new (config, "perceptual-metric",
                                         _("Use _perceptual error metric"));
  gtk_grid_attach (GTK_GRID (grid), pm_check, 1, 1, 1, 1);

  store = gimp_int_store_new ("Default", DDS_FORMAT_DEFAULT,
                              "RGB8",    DDS_FORMAT_RGB8,
                              "RGBA8",   DDS_FORMAT_RGBA8,
                              "BGR8",    DDS_FORMAT_BGR8,
                              "ABGR8",   DDS_FORMAT_ABGR8,
                              "R5G6B5",  DDS_FORMAT_R5G6B5,
                              "RGBA4",   DDS_FORMAT_RGBA4,
                              "RGB5A1",  DDS_FORMAT_RGB5A1,
                              "RGB10A2", DDS_FORMAT_RGB10A2,
                              "R3G3B2",  DDS_FORMAT_R3G3B2,
                              "A8",      DDS_FORMAT_A8,
                              "L8",      DDS_FORMAT_L8,
                              "L8A8",    DDS_FORMAT_L8A8,
                              "AExp",    DDS_FORMAT_AEXP,
                              "YCoCg",   DDS_FORMAT_YCOCG,
                              NULL);
  format_opt = gimp_prop_int_combo_box_new (config, "format",
                                            GIMP_INT_STORE (store));
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("_Format:"),
                            0.0, 0.5,
                            format_opt, 1);

  store = gimp_int_store_new ("Image / Selected layer", DDS_SAVE_SELECTED_LAYER,
                              "As cube map",            DDS_SAVE_CUBEMAP,
                              "As volume map",          DDS_SAVE_VOLUMEMAP,
                              "As texture array",       DDS_SAVE_ARRAY,
                              NULL);
  opt = gimp_prop_int_combo_box_new (config, "save-type",
                                     GIMP_INT_STORE (store));
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                            _("_Save:"),
                            0.0, 0.5,
                            opt, 1);

  gimp_int_combo_box_set_sensitivity (GIMP_INT_COMBO_BOX (opt),
                                      combo_sensitivity_func,
                                      opt, NULL);

  combo_set_item_sensitive (opt, DDS_SAVE_CUBEMAP,   is_cubemap);
  combo_set_item_sensitive (opt, DDS_SAVE_VOLUMEMAP, is_volume);
  combo_set_item_sensitive (opt, DDS_SAVE_ARRAY,     is_array);

  store = gimp_int_store_new ("No mipmaps",           DDS_MIPMAP_NONE,
                              "Generate mipmaps",     DDS_MIPMAP_GENERATE,
                              "Use existing mipmaps", DDS_MIPMAP_EXISTING,
                              NULL);
  mipmap_opt = gimp_prop_int_combo_box_new (config, "mipmaps",
                                            GIMP_INT_STORE (store));
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 4,
                            _("_Mipmaps:"),
                            0.0, 0.5,
                            mipmap_opt, 1);

  gimp_int_combo_box_set_sensitivity (GIMP_INT_COMBO_BOX (mipmap_opt),
                                      combo_sensitivity_func,
                                      mipmap_opt, NULL);

  combo_set_item_sensitive (mipmap_opt, DDS_MIPMAP_EXISTING,
                            ! (is_volume || is_cubemap) &&
                            is_mipmap_chain_valid);


  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  check = gimp_prop_check_button_new (config, "transparent-color",
                                      _("Transparent index:"));
  gtk_box_pack_start (GTK_BOX (hbox), check, FALSE, FALSE, 0);
  gtk_widget_show (check);

  transparent_spin = gimp_prop_spin_button_new (config, "transparent-index",
                                                1, 8, 0);
  gtk_box_pack_start (GTK_BOX (hbox), transparent_spin, TRUE, TRUE, 0);

  frame = gimp_frame_new (_("Mipmap Options"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 8);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  store = gimp_int_store_new ("Default",   DDS_MIPMAP_FILTER_DEFAULT,
                              "Nearest",   DDS_MIPMAP_FILTER_NEAREST,
                              "Box",       DDS_MIPMAP_FILTER_BOX,
                              "Triangle",  DDS_MIPMAP_FILTER_TRIANGLE,
                              "Quadratic", DDS_MIPMAP_FILTER_QUADRATIC,
                              "B-Spline",  DDS_MIPMAP_FILTER_BSPLINE,
                              "Mitchell",  DDS_MIPMAP_FILTER_MITCHELL,
                              "Lanczos",   DDS_MIPMAP_FILTER_LANCZOS,
                              "Kaiser",    DDS_MIPMAP_FILTER_KAISER,
                              NULL);
  mipmap_filter_opt = gimp_prop_int_combo_box_new (config, "mipmap-filter",
                                                   GIMP_INT_STORE (store));
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("F_ilter:"),
                            0.0, 0.5,
                            mipmap_filter_opt, 1);

  store = gimp_int_store_new ("Default", DDS_MIPMAP_WRAP_DEFAULT,
                              "Mirror",  DDS_MIPMAP_WRAP_MIRROR,
                              "Repeat",  DDS_MIPMAP_WRAP_REPEAT,
                              "Clamp",   DDS_MIPMAP_WRAP_CLAMP,
                              NULL);
  mipmap_wrap_opt = gimp_prop_int_combo_box_new (config, "mipmap-wrap",
                                                 GIMP_INT_STORE (store));
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("_Wrap mode:"),
                            0.0, 0.5,
                            mipmap_wrap_opt, 1);

  gamma_check = gimp_prop_check_button_new (config, "gamma-correct",
                                            _("Appl_y gamma correction"));
  gtk_grid_attach (GTK_GRID (grid), gamma_check, 1, 2, 1, 1);

  srgb_check = gimp_prop_check_button_new (config, "srgb",
                                           _("Use s_RGB colorspace"));
  gtk_grid_attach (GTK_GRID (grid), srgb_check, 1, 3, 1, 1);

  gamma_spin = gimp_prop_spin_button_new (config, "gamma",
                                          0.1, 0.5, 1);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 4,
                            _("_Gamma:"), 0.0, 0.5,
                            gamma_spin, 1);

  alpha_coverage_check =
    gimp_prop_check_button_new (config, "preserve-alpha-coverage",
                                _("Preserve alpha _test coverage"));
  gtk_grid_attach (GTK_GRID (grid), alpha_coverage_check, 1, 5, 1, 1);

  alpha_test_threshold_spin =
    gimp_prop_spin_button_new (config, "alpha-test-threshold",
                               0.01, 0.1, 2);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 6,
                            _("_Alpha test threshold:"), 0.0, 0.5,
                            alpha_test_threshold_spin, 1);

  config_notify (config,
                 g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                               "compression-format"),
                 image);

  config_notify (config,
                 g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                               "mipmaps"),
                 image);

  config_notify (config,
                 g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                               "save-type"),
                 image);

  config_notify (config,
                 g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                               "transparent-color"),
                 image);

  g_signal_connect (config, "notify",
                    G_CALLBACK (config_notify),
                    image);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  g_signal_handlers_disconnect_by_func (config,
                                        config_notify,
                                        image);

  gtk_widget_destroy (dialog);

  return run;
}
