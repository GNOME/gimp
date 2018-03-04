/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-jp2.c -- JPEG 2000 file format plug-in
 * Copyright (C) 2009 Aurimas Juška <aurimas.juska@gmail.com>
 * Copyright (C) 2004 Florian Traverse <florian.traverse@cpe.fr>
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

/*
 * Portions of this plug-in code (color conversion, etc.) were imported
 * from the OpenJPEG project covered under the following GNU GPL
 * compatible license:
 *
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2014, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux
 * Copyright (c) 2003-2014, Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include <openjpeg.h>


#define LOAD_PROC      "file-jp2-load"
#define PLUG_IN_BINARY "file-jp2-load"


static void     query             (void);
static void     run               (const gchar      *name,
                                   gint              nparams,
                                   const GimpParam  *param,
                                   gint             *nreturn_vals,
                                   GimpParam       **return_vals);
static gint32   load_image        (const gchar      *filename,
                                   GError          **error);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query proc */
  run,   /* run_proc */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load." },
    { GIMP_PDB_STRING, "raw-filename", "The name entered" },
  };

  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,  "image",        "Output image" }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads JPEG 2000 images.",
                          "The JPEG 2000 image loader.",
                          "Aurimas Juška",
                          "Aurimas Juška, Florian Traverse",
                          "2009",
                          N_("JPEG 2000 image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_magic_load_handler (LOAD_PROC,
                                    "jp2,jpc,jpx,j2k,jpf",
                                    "",
                                    "4,string,jP,0,string,\xff\x4f\xff\x51\x00");

  gimp_register_file_handler_mime (LOAD_PROC, "image/jp2");
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
  gint               image_ID;
  GError            *error = NULL;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      gboolean interactive;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);
          interactive = TRUE;
          break;
        default:
          interactive = FALSE;
          break;
        }

      image_ID = load_image (param[1].data.d_string, &error);

      if (image_ID != -1)
        {
          GFile        *file = g_file_new_for_path (param[1].data.d_string);
          GimpMetadata *metadata;

          metadata = gimp_image_metadata_load_prepare (image_ID, "image/jp2",
                                                       file, NULL);

          if (metadata)
            {
              GimpMetadataLoadFlags flags = GIMP_METADATA_LOAD_ALL;

              gimp_image_metadata_load_finish (image_ID, "image/jp2",
                                               metadata, flags,
                                               interactive);

              g_object_unref (metadata);
            }

          g_object_unref (file);

          *nreturn_vals = 2;
          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type           = GIMP_PDB_STRING;
      values[1].data.d_string  = error->message;
    }

  values[0].data.d_status = status;
}

static void
sycc_to_rgb (int  offset,
             int  upb,
             int  y,
             int  cb,
             int  cr,
             int *out_r,
             int *out_g,
             int *out_b)
{
  int r, g, b;

  cb -= offset;
  cr -= offset;
  r = y + (int) (1.402 * (float) cr);

  if (r < 0)
    r = 0;
  else if (r > upb)
    r = upb;
  *out_r = r;

  g = y - (int) (0.344 * (float) cb + 0.714 * (float) cr);
  if (g < 0)
    g = 0;
  else if (g > upb)
    g = upb;
  *out_g = g;

  b = y + (int) (1.772 * (float) cb);
  if (b < 0)
    b = 0;
  else if (b > upb)
    b = upb;
  *out_b = b;
}

static gboolean
sycc420_to_rgb (opj_image_t *img)
{
  int *d0, *d1, *d2, *r, *g, *b, *nr, *ng, *nb;
  const int *y, *cb, *cr, *ny;
  size_t maxw, maxh, max, offx, loopmaxw, offy, loopmaxh;
  int offset, upb;
  size_t i;

  upb = (int) img->comps[0].prec;
  offset = 1 << (upb - 1);
  upb = (1 << upb) - 1;

  maxw = (size_t) img->comps[0].w;
  maxh = (size_t) img->comps[0].h;
  max = maxw * maxh;

  y = img->comps[0].data;
  cb = img->comps[1].data;
  cr = img->comps[2].data;

  d0 = r = (int *) malloc (sizeof (int) * max);
  d1 = g = (int *) malloc (sizeof (int) * max);
  d2 = b = (int *) malloc (sizeof (int) * max);

  if (r == NULL || g == NULL || b == NULL)
    {
      g_warning ("malloc() failed in sycc420_to_rgb()");
      goto out;
    }

  /* if img->x0 is odd, then first column shall use Cb/Cr = 0 */
  offx = img->x0 & 1U;
  loopmaxw = maxw - offx;
  /* if img->y0 is odd, then first line shall use Cb/Cr = 0 */
  offy = img->y0 & 1U;
  loopmaxh = maxh - offy;

  if (offy > 0U)
    {
      size_t j;

      for (j = 0; j < maxw; ++j)
        {
          sycc_to_rgb (offset, upb, *y, 0, 0, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;
        }
    }

  for (i = 0U; i < (loopmaxh & ~(size_t) 1U); i += 2U)
    {
      size_t j;

      ny = y + maxw;
      nr = r + maxw;
      ng = g + maxw;
      nb = b + maxw;

      if (offx > 0U)
        {
          sycc_to_rgb (offset, upb, *y, 0, 0, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;

          sycc_to_rgb (offset, upb, *ny, *cb, *cr, nr, ng, nb);
          ++ny;
          ++nr;
          ++ng;
          ++nb;
        }

      for (j = 0; j < (loopmaxw & ~(size_t) 1U); j += 2U)
        {
          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;

          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;

          sycc_to_rgb (offset, upb, *ny, *cb, *cr, nr, ng, nb);
          ++ny;
          ++nr;
          ++ng;
          ++nb;

          sycc_to_rgb (offset, upb, *ny, *cb, *cr, nr, ng, nb);
          ++ny;
          ++nr;
          ++ng;
          ++nb;

          ++cb;
          ++cr;
        }

      if (j < loopmaxw)
        {
          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;

          sycc_to_rgb (offset, upb, *ny, *cb, *cr, nr, ng, nb);
          ++ny;
          ++nr;
          ++ng;
          ++nb;

          ++cb;
          ++cr;
        }

      y += maxw;
      r += maxw;
      g += maxw;
      b += maxw;
    }

  if (i < loopmaxh)
    {
      size_t j;

      for (j = 0U; j < (maxw & ~(size_t) 1U); j += 2U)
        {
          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;

          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;

          ++cb;
          ++cr;
        }

      if (j < maxw)
        {
          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
        }
    }

  free (img->comps[0].data);
  img->comps[0].data = d0;
  free (img->comps[1].data);
  img->comps[1].data = d1;
  free (img->comps[2].data);
  img->comps[2].data = d2;

  img->comps[1].w = img->comps[2].w = img->comps[0].w;
  img->comps[1].h = img->comps[2].h = img->comps[0].h;
  img->comps[1].dx = img->comps[2].dx = img->comps[0].dx;
  img->comps[1].dy = img->comps[2].dy = img->comps[0].dy;
  img->color_space = OPJ_CLRSPC_SRGB;

  return TRUE;

 out:
  free (r);
  free (g);
  free (b);
  return FALSE;
}

static gboolean
sycc422_to_rgb (opj_image_t *img)
{
  int *d0, *d1, *d2, *r, *g, *b;
  const int *y, *cb, *cr;
  size_t maxw, maxh, max, offx, loopmaxw;
  int offset, upb;
  size_t i;

  upb = (int) img->comps[0].prec;
  offset = 1 <<(upb - 1);
  upb = (1 << upb) - 1;

  maxw = (size_t) img->comps[0].w;
  maxh = (size_t) img->comps[0].h;
  max = maxw * maxh;

  y = img->comps[0].data;
  cb = img->comps[1].data;
  cr = img->comps[2].data;

  d0 = r = (int *) malloc (sizeof (int) * max);
  d1 = g = (int *) malloc (sizeof (int) * max);
  d2 = b = (int *) malloc (sizeof (int) * max);

  if (r == NULL || g == NULL || b == NULL)
    {
      g_warning ("malloc() failed in sycc422_to_rgb()");
      goto out;
    }

  /* if img->x0 is odd, then first column shall use Cb/Cr = 0 */
  offx = img->x0 & 1U;
  loopmaxw = maxw - offx;

  for (i = 0U; i < maxh; ++i)
    {
      size_t j;

      if (offx > 0U)
        {
          sycc_to_rgb (offset, upb, *y, 0, 0, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;
        }

      for (j = 0U; j < (loopmaxw & ~(size_t) 1U); j += 2U)
        {
          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;

          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;
          ++cb;
          ++cr;
        }

      if (j < loopmaxw)
        {
          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;
          ++cb;
          ++cr;
        }
    }

  free (img->comps[0].data);
  img->comps[0].data = d0;
  free (img->comps[1].data);
  img->comps[1].data = d1;
  free (img->comps[2].data);
  img->comps[2].data = d2;

  img->comps[1].w = img->comps[2].w = img->comps[0].w;
  img->comps[1].h = img->comps[2].h = img->comps[0].h;
  img->comps[1].dx = img->comps[2].dx = img->comps[0].dx;
  img->comps[1].dy = img->comps[2].dy = img->comps[0].dy;
  img->color_space = OPJ_CLRSPC_SRGB;

  return (TRUE);

 out:
  free (r);
  free (g);
  free (b);
  return (FALSE);
}

static gboolean
sycc444_to_rgb (opj_image_t *img)
{
  int *d0, *d1, *d2, *r, *g, *b;
  const int *y, *cb, *cr;
  size_t maxw, maxh, max, i;
  int offset, upb;

  upb = (int) img->comps[0].prec;
  offset = 1 << (upb - 1);
  upb = (1 << upb) - 1;

  maxw = (size_t) img->comps[0].w;
  maxh = (size_t) img->comps[0].h;
  max = maxw * maxh;

  y = img->comps[0].data;
  cb = img->comps[1].data;
  cr = img->comps[2].data;

  d0 = r = (int *) malloc(sizeof (int) * max);
  d1 = g = (int *) malloc(sizeof (int) * max);
  d2 = b = (int *) malloc(sizeof (int) * max);

  if (r == NULL || g == NULL || b == NULL)
    {
      g_warning ("malloc() failed in sycc444_to_rgb()");
      goto out;
    }

  for (i = 0U; i < max; ++i)
    {
      sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
      ++y;
      ++cb;
      ++cr;
      ++r;
      ++g;
      ++b;
    }

  free (img->comps[0].data);
  img->comps[0].data = d0;
  free (img->comps[1].data);
  img->comps[1].data = d1;
  free (img->comps[2].data);
  img->comps[2].data = d2;

  img->color_space = OPJ_CLRSPC_SRGB;
  return TRUE;

 out:
  free (r);
  free (g);
  free (b);
  return FALSE;
}

static gboolean
color_sycc_to_rgb (opj_image_t *img)
{
  if (img->numcomps < 3)
    {
      img->color_space = OPJ_CLRSPC_GRAY;
      return TRUE;
    }
  else if ((img->comps[0].dx == 1) &&
           (img->comps[1].dx == 2) &&
           (img->comps[2].dx == 2) &&
           (img->comps[0].dy == 1) &&
           (img->comps[1].dy == 2) &&
           (img->comps[2].dy == 2))
    {
      /* horizontal and vertical sub-sample */
      return sycc420_to_rgb (img);
    }
  else if ((img->comps[0].dx == 1) &&
           (img->comps[1].dx == 2) &&
           (img->comps[2].dx == 2) &&
           (img->comps[0].dy == 1) &&
           (img->comps[1].dy == 1) &&
           (img->comps[2].dy == 1))
    {
      /* horizontal sub-sample only */
      return sycc422_to_rgb (img);
    }
  else if ((img->comps[0].dx == 1) &&
           (img->comps[1].dx == 1) &&
           (img->comps[2].dx == 1) &&
           (img->comps[0].dy == 1) &&
           (img->comps[1].dy == 1) &&
           (img->comps[2].dy == 1))
    {
      /* no sub-sample */
      return sycc444_to_rgb (img);
    }
  else
    {
      g_warning ("Cannot convert in color_sycc_to_rgb()");
      return FALSE;
    }
}

static gboolean
color_cmyk_to_rgb (opj_image_t *image)
{
  float C, M, Y, K;
  float sC, sM, sY, sK;
  unsigned int w, h, max, i;

  w = image->comps[0].w;
  h = image->comps[0].h;

  if ((image->numcomps < 4) ||
      (image->comps[0].dx != image->comps[1].dx) ||
      (image->comps[0].dx != image->comps[2].dx) ||
      (image->comps[0].dx != image->comps[3].dx) ||
      (image->comps[0].dy != image->comps[1].dy) ||
      (image->comps[0].dy != image->comps[2].dy) ||
      (image->comps[0].dy != image->comps[3].dy))
    {
      g_warning ("Cannot convert in color_cmyk_to_rgb()");
      return FALSE;
    }

  max = w * h;

  sC = 1.0f / (float) ((1 << image->comps[0].prec) - 1);
  sM = 1.0f / (float) ((1 << image->comps[1].prec) - 1);
  sY = 1.0f / (float) ((1 << image->comps[2].prec) - 1);
  sK = 1.0f / (float) ((1 << image->comps[3].prec) - 1);

  for (i = 0; i < max; ++i)
    {
      /* CMYK values from 0 to 1 */
      C = (float) (image->comps[0].data[i]) * sC;
      M = (float) (image->comps[1].data[i]) * sM;
      Y = (float) (image->comps[2].data[i]) * sY;
      K = (float) (image->comps[3].data[i]) * sK;

      /* Invert all CMYK values */
      C = 1.0f - C;
      M = 1.0f - M;
      Y = 1.0f - Y;
      K = 1.0f - K;

      /* CMYK -> RGB : RGB results from 0 to 255 */
      image->comps[0].data[i] = (int) (255.0f * C * K); /* R */
      image->comps[1].data[i] = (int) (255.0f * M * K); /* G */
      image->comps[2].data[i] = (int) (255.0f * Y * K); /* B */
    }

  free (image->comps[3].data);
  image->comps[3].data = NULL;
  image->comps[0].prec = 8;
  image->comps[1].prec = 8;
  image->comps[2].prec = 8;
  image->numcomps -= 1;
  image->color_space = OPJ_CLRSPC_SRGB;

  for (i = 3; i < image->numcomps; ++i)
    {
      memcpy(&(image->comps[i]), &(image->comps[i + 1]),
             sizeof (image->comps[i]));
    }

  return TRUE;
}

/*
 * This code has been adopted from sjpx_openjpeg.c of ghostscript
 */
static gboolean
color_esycc_to_rgb (opj_image_t *image)
{
  int y, cb, cr, sign1, sign2, val;
  unsigned int w, h, max, i;
  int flip_value;
  int max_value;

  flip_value = (1 << (image->comps[0].prec - 1));
  max_value = (1 << image->comps[0].prec) - 1;

  if ((image->numcomps < 3) ||
      (image->comps[0].dx != image->comps[1].dx) ||
      (image->comps[0].dx != image->comps[2].dx) ||
      (image->comps[0].dy != image->comps[1].dy) ||
      (image->comps[0].dy != image->comps[2].dy))
    {
      g_warning ("Cannot convert in color_esycc_to_rgb()");
      return FALSE;
    }

  w = image->comps[0].w;
  h = image->comps[0].h;

  sign1 = (int)image->comps[1].sgnd;
  sign2 = (int)image->comps[2].sgnd;

  max = w * h;

  for (i = 0; i < max; ++i)
    {

      y = image->comps[0].data[i];
      cb = image->comps[1].data[i];
      cr = image->comps[2].data[i];

      if (! sign1)
        cb -= flip_value;

      if (! sign2)
        cr -= flip_value;

      val = (int) ((float) y - (float) 0.0000368 *
                   (float) cb + (float) 1.40199 * (float) cr + (float) 0.5);

      if (val > max_value)
        val = max_value;
      else if (val < 0)
        val = 0;
      image->comps[0].data[i] = val;

      val = (int) ((float) 1.0003 * (float) y - (float) 0.344125 *
                   (float) cb - (float) 0.7141128 * (float) cr + (float) 0.5);

      if (val > max_value)
        val = max_value;
      else if(val < 0)
        val = 0;
      image->comps[1].data[i] = val;

      val = (int) ((float) 0.999823 * (float) y + (float) 1.77204 *
                   (float) cb - (float) 0.000008 * (float) cr + (float) 0.5);

      if (val > max_value)
        val = max_value;
      else if (val < 0)
        val = 0;
      image->comps[2].data[i] = val;
    }

  image->color_space = OPJ_CLRSPC_SRGB;
  return TRUE;
}

static gint32
load_image (const gchar  *filename,
            GError      **error)
{
  opj_stream_t      *stream;
  opj_codec_t       *codec;
  opj_dparameters_t  parameters;
  opj_image_t       *image;
  gint32             image_ID;
  gint32             layer_ID;
  GimpImageType      image_type;
  GimpImageBaseType  base_type;
  gint               width;
  gint               height;
  gint               num_components;
  gint               colorspace_family;
  GeglBuffer        *buffer;
  gint               i, j, k;
  guchar            *pixels;
  gint               components[4];

  stream = NULL;
  codec = NULL;
  image = NULL;
  image_ID = -1;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  stream = opj_stream_create_default_file_stream (filename, OPJ_TRUE);
  if (! stream)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not open '%s' for reading"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  codec = opj_create_decompress (OPJ_CODEC_JP2);

  opj_set_default_decoder_parameters (&parameters);
  if (opj_setup_decoder (codec, &parameters) != OPJ_TRUE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Couldn't set parameters on decoder for '%s'."),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  if (opj_read_header (stream, codec, &image) != OPJ_TRUE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Couldn't read JP2 header from '%s'."),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  if (opj_decode (codec, stream, image) != OPJ_TRUE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Couldn't decode JP2 image in '%s'."),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  if (opj_end_decompress (codec, stream) != OPJ_TRUE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Couldn't decode JP2 image in '%s'."),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  if ((image->color_space != OPJ_CLRSPC_SYCC) &&
      (image->numcomps == 3) &&
      (image->comps[0].dx == image->comps[0].dy) &&
      (image->comps[1].dx != 1))
    {
      image->color_space = OPJ_CLRSPC_SYCC;
    }
  else if (image->numcomps <= 2)
    {
      image->color_space = OPJ_CLRSPC_GRAY;
    }

  if (image->color_space == OPJ_CLRSPC_SYCC)
    {
      if (! color_sycc_to_rgb (image))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Couldn't decode JP2 image in '%s'."),
                       gimp_filename_to_utf8 (filename));
          goto out;
        }
    }
  else if ((image->color_space == OPJ_CLRSPC_CMYK))
    {
      if (! color_cmyk_to_rgb (image))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Couldn't decode JP2 image in '%s'."),
                       gimp_filename_to_utf8 (filename));
          goto out;
        }
    }
  else if (image->color_space == OPJ_CLRSPC_EYCC)
    {
      if (! color_esycc_to_rgb (image))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Couldn't decode JP2 image in '%s'."),
                       gimp_filename_to_utf8 (filename));
          goto out;
        }
    }

  if(image->color_space == OPJ_CLRSPC_GRAY)
    {
      base_type = GIMP_GRAY;
      image_type = GIMP_GRAY_IMAGE;
    }
  else
    {
      base_type = GIMP_RGB;
      image_type = GIMP_RGB_IMAGE;
    }

  num_components = image->numcomps;

  if(num_components == 2)
    {
      image_type = GIMP_GRAYA_IMAGE;
    }
  else if(num_components == 4)
    {
      image_type = GIMP_RGBA_IMAGE;
    }

  /* FIXME */
/*  base_type = GIMP_GRAY;
  base_type = GIMP_RGB;
  image_type = GIMP_GRAYA_IMAGE;
  image_type = GIMP_GRAY_IMAGE;
  image_type = GIMP_RGBA_IMAGE;
  image_type = GIMP_RGB_IMAGE;
*/
  width = image->comps[0].w;
  height = image->comps[0].h;

  image_ID = gimp_image_new (width, height, base_type);
  gimp_image_set_filename (image_ID, filename);

  layer_ID = gimp_layer_new (image_ID,
                             _("Background"),
                             width, height,
                             image_type,
                             100,
                             gimp_image_get_default_new_layer_mode (image_ID));
  gimp_image_insert_layer (image_ID, layer_ID, -1, 0);

  buffer = gimp_drawable_get_buffer (layer_ID);

  pixels = malloc (width * num_components);

  for (i = 0; i < height; i++)
    {
      for( j = 0; j < num_components; j++)
        {
          const int channel_prec = 8;

          OPJ_UINT32 component_prec = image->comps[j].prec;

          if(component_prec >= channel_prec)
            {
              int shift = component_prec - channel_prec;

              for( k = 0; k < width; k++)
                {
                  pixels[k * num_components + j] = image->comps[j].data[i * width + k] >> shift; 
                }
            }
          else
            {
              int mult = 1 << (channel_prec - component_prec);

              for( k = 0; k < width; k++)
                {
                  pixels[k* num_components + j] = image->comps[j].data[i * width + k ] * mult; 
                }

            }
        }

        gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i, width, 1), 0,
                       NULL, pixels, GEGL_AUTO_ROWSTRIDE);
    }

#if 0
  matrix = jas_matrix_create (1, width);

  for (i = 0; i < height; i++)
    {
      for (j = 0; j < num_components; j++)
        {
          const int channel_prec = 8;

          jas_image_readcmpt (image, components[j], 0, i, width, 1, matrix);

          if (jas_image_cmptprec (image, components[j]) >= channel_prec)
            {
              int shift = MAX (jas_image_cmptprec (image, components[j]) - channel_prec, 0);

              for (k = 0; k < width; k++)
                {
                  pixels[k * num_components + j] = jas_matrix_get (matrix, 0, k) >> shift;
                }
            }
          else
            {
              int mul = 1 << (channel_prec - jas_image_cmptprec (image, components[j]));

              for (k = 0; k < width; k++)
                {
                  pixels[k * num_components + j] = jas_matrix_get (matrix, 0, k) * mul;
                }

            }
        }

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i, width, 1), 0,
                       NULL, pixels, GEGL_AUTO_ROWSTRIDE);
    }
#endif

  free (pixels);

  g_object_unref (buffer);

  if (image->icc_profile_buf)
    {
      if (image->icc_profile_len)
        {
          GimpColorProfile *profile;

          profile = gimp_color_profile_new_from_icc_profile
            (image->icc_profile_buf, image->icc_profile_len, error);
          if (! profile)
            goto out;

          gimp_image_set_color_profile (image_ID, profile);
          g_object_unref (profile);
        }
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Couldn't decode CIELAB JP2 image in '%s'."),
                       gimp_filename_to_utf8 (filename));
          goto out;
        }

      free (image->icc_profile_buf);
      image->icc_profile_buf = NULL;
      image->icc_profile_len = 0;
    }

  gimp_progress_update (1.0);

 out:
  if (image)
    opj_image_destroy (image);
  if (codec)
    opj_destroy_codec (codec);
  if (stream)
    opj_stream_destroy (stream);

  return image_ID;
}
