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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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


#define LOAD_JP2_PROC      "file-jp2-load"
#define LOAD_J2K_PROC      "file-j2k-load"
#define PLUG_IN_BINARY     "file-jp2-load"
#define PLUG_IN_ROLE       "gimp-file-jp2-load"


typedef struct _Jp2      Jp2;
typedef struct _Jp2Class Jp2Class;

struct _Jp2
{
  GimpPlugIn      parent_instance;
};

struct _Jp2Class
{
  GimpPlugInClass parent_class;
};


#define JP2_TYPE  (jp2_get_type ())
#define JP2 (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), JP2_TYPE, Jp2))

GType                   jp2_get_type         (void) G_GNUC_CONST;

static GList          * jp2_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * jp2_create_procedure (GimpPlugIn            *plug_in,
                                              const gchar           *name);

static GimpValueArray * jp2_load             (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GFile                 *file,
                                              GimpMetadata          *metadata,
                                              GimpMetadataLoadFlags *flags,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);

static GimpImage      * load_image           (GimpProcedure         *procedure,
                                              GObject               *config,
                                              GFile                 *file,
                                              OPJ_CODEC_FORMAT       format,
                                              OPJ_COLOR_SPACE        color_space,
                                              gboolean               interactive,
                                              gboolean              *profile_loaded,
                                              GError               **error);

static OPJ_COLOR_SPACE  open_dialog          (GimpProcedure         *procedure,
                                              GObject               *config,
                                              GFile                 *file,
                                              OPJ_CODEC_FORMAT       format,
                                              gint                   num_components,
                                              GError               **error);


G_DEFINE_TYPE (Jp2, jp2, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (JP2_TYPE)
DEFINE_STD_SET_I18N


static void
jp2_class_init (Jp2Class *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = jp2_query_procedures;
  plug_in_class->create_procedure = jp2_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
jp2_init (Jp2 *jp2)
{
}

static GList *
jp2_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_JP2_PROC));
  list = g_list_append (list, g_strdup (LOAD_J2K_PROC));

  return list;
}

static GimpProcedure *
jp2_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_JP2_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           jp2_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("JPEG 2000 image"));
      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("JPEG 2000"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads JPEG 2000 images.",
                                        "The JPEG 2000 image loader.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Aurimas Juška",
                                      "Aurimas Juška, Florian Traverse",
                                      "2009");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/jp2");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "jp2");

      /* XXX: more complete magic number would be:
       * "0,string,\x00\x00\x00\x0C\x6A\x50\x20\x20\x0D\x0A\x87\x0A"
       * But the '\0' character makes problem in a 0-terminated string
       * obviously, as well as some other space characters, it would
       * seem. The below smaller version seems ok and not interfering
       * with other formats.
       */
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "3,string,\x0CjP");
    }
  else if (! strcmp (name, LOAD_J2K_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           jp2_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("JPEG 2000 codestream"));
      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("JPEG 2000"));

      gimp_procedure_set_documentation (procedure,
                                        _("Loads JPEG 2000 codestream."),
                                        _("Loads JPEG 2000 codestream. "
                                          "If the color space is set to "
                                          "UNKNOWN (0), we will try to guess, "
                                          "which is only possible for few "
                                          "spaces (such as grayscale). Most "
                                          "such calls will fail. You are rather "
                                          "expected to know the color space of "
                                          "your data."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Jehan",
                                      "Jehan",
                                      "2009");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-jp2-codestream");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "j2k,j2c,jpc");

      gimp_procedure_add_choice_argument (procedure, "colorspace",
                                          _("Color s_pace"),
                                          _("Color space"),
                                          gimp_choice_new_with_values ("srgb",      OPJ_CLRSPC_SRGB,     _("sRGB"),     NULL,
                                                                       "grayscale", OPJ_CLRSPC_GRAY,     _("Grayscale"), NULL,
                                                                       "ycbcr",     OPJ_CLRSPC_SYCC,     _("YCbCr"),     NULL,
                                                                       "xvycc",     OPJ_CLRSPC_EYCC,     _("xvYCC"),     NULL,
                                                                       "cmyk",      OPJ_CLRSPC_CMYK,     _("CMYK"),      NULL,
                                                                       "unknown",   OPJ_CLRSPC_UNKNOWN,  _("Unknown"),   NULL,
                                                                       NULL),
                                          "unknown",
                                          G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
jp2_load (GimpProcedure         *procedure,
          GimpRunMode            run_mode,
          GFile                 *file,
          GimpMetadata          *metadata,
          GimpMetadataLoadFlags *flags,
          GimpProcedureConfig   *config,
          gpointer               run_data)
{
  GimpValueArray    *return_vals;
  GimpImage         *image          = NULL;
  OPJ_COLOR_SPACE    color_space    = OPJ_CLRSPC_UNKNOWN;
  gboolean           interactive;
  gboolean           profile_loaded = FALSE;
  GimpPDBStatusType  status         = GIMP_PDB_SUCCESS;
  GError            *error          = NULL;

  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_ui_init (PLUG_IN_BINARY);
      interactive = TRUE;
      break;

    default:
      if (! strcmp (gimp_procedure_get_name (procedure), LOAD_J2K_PROC))
        color_space =
          gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                               "colorspace");
      interactive = FALSE;
      break;
    }

  if (! strcmp (gimp_procedure_get_name (procedure), LOAD_JP2_PROC))
    {
      image = load_image (procedure, G_OBJECT (config), file, OPJ_CODEC_JP2,
                          color_space, interactive, &profile_loaded,
                          &error);
    }
  else
    {
      image = load_image (procedure, G_OBJECT (config), file, OPJ_CODEC_J2K,
                          color_space, interactive, &profile_loaded,
                          &error);
    }

  if (! image)
    {
      status = error ? GIMP_PDB_EXECUTION_ERROR : GIMP_PDB_CANCEL;

      return gimp_procedure_new_return_values (procedure, status, error);
    }

  if (profile_loaded)
    *flags &= ~GIMP_METADATA_LOAD_COLORSPACE;

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
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

/*
 * get_valid_precision() converts given precision to standard precision
 * of gimp i.e. 8, 16, 32
 * e.g 12-bit to 16-bit , 24-bit to 32-bit
*/
static gint
get_valid_precision (gint precision_actual)
{
  if (precision_actual <= 8)
    return 8;
  else if (precision_actual <= 16)
    return 16;
  else
    return 32;
}

static GimpPrecision
get_image_precision (gint     precision,
                     gboolean linear)
{
  switch (precision)
    {
      case 32:
        if (linear)
          return GIMP_PRECISION_U32_LINEAR;
        return GIMP_PRECISION_U32_NON_LINEAR;
      case 16:
        if (linear)
          return GIMP_PRECISION_U16_LINEAR;
        return GIMP_PRECISION_U16_NON_LINEAR;
      default:
         if (linear)
          return GIMP_PRECISION_U8_LINEAR;
        return GIMP_PRECISION_U8_NON_LINEAR;
    }
}

static OPJ_COLOR_SPACE
open_dialog (GimpProcedure    *procedure,
             GObject          *config,
             GFile            *file,
             OPJ_CODEC_FORMAT  format,
             gint              num_components,
             GError          **error)
{
  const gchar         *title;
  GtkWidget           *dialog;
  gboolean             run;
  GimpParamSpecChoice *cspec;
  OPJ_COLOR_SPACE      color_space = OPJ_CLRSPC_SRGB;

  if (format == OPJ_CODEC_J2K)
    /* Not having color information is expected. */
    title = "Opening JPEG 2000 codestream";
  else
    /* Unexpected, but let's be a bit flexible and ask. */
    title = "JPEG 2000 image with no color space";

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _(title));

  cspec =
    GIMP_PARAM_SPEC_CHOICE (g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                          "colorspace"));


  if (num_components == 3)
    {
      /* Can be RGB, YUV and YCC. */
      gimp_choice_set_sensitive (cspec->choice, "grayscale", FALSE);
      gimp_choice_set_sensitive (cspec->choice, "cmyk", FALSE);
    }
  else if (num_components == 4)
    {
      /* Can be RGB, YUV and YCC with alpha or CMYK. */
      gimp_choice_set_sensitive (cspec->choice, "grayscale", FALSE);
    }
  else
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported JPEG 2000%s '%s' with %d components."),
                   (format == OPJ_CODEC_J2K) ? " codestream" : "",
                   gimp_file_get_utf8_name (file), num_components);

      g_object_set (config, "colorspace", "unknown", NULL);
    }

  if (num_components == 3 || num_components == 4)
    {
      /* By default, RGB is active. */
      gimp_choice_set_sensitive (cspec->choice, "unknown", FALSE);
      g_object_set (config, "colorspace", "srgb", NULL);

      gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                  NULL);

      gtk_widget_set_visible (dialog, TRUE);

      run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

      if (! run)
        /* Do not set an error here. The import was simply canceled.
         * No error occurred. */
        color_space = OPJ_CLRSPC_UNKNOWN;
      else
        color_space =
          gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                               "colorspace");
    }

  gtk_widget_destroy (dialog);

  return color_space;
}

static GimpImage *
load_image (GimpProcedure     *procedure,
            GObject           *config,
            GFile             *file,
            OPJ_CODEC_FORMAT   format,
            OPJ_COLOR_SPACE    color_space,
            gboolean           interactive,
            gboolean          *profile_loaded,
            GError           **error)
{
  opj_stream_t        *stream     = NULL;
  opj_codec_t         *codec      = NULL;
  opj_dparameters_t    parameters;
  opj_image_t         *image      = NULL;
  GimpColorProfile    *profile    = NULL;
  GimpImage           *gimp_image = NULL;
  GimpLayer           *layer;
  GimpImageType        image_type;
  GimpImageBaseType    base_type;
  gint                 width;
  gint                 height;
  gint                 num_components;
  GeglBuffer          *buffer;
  gint                 i, j, k, it;
  guchar              *pixels;
  const Babl          *file_format;
  gint                 bpp;
  GimpPrecision        image_precision;
  gint                 precision_actual, precision_scaled;
  gint                 temp;
  gboolean             linear = FALSE;
  unsigned char       *c      = NULL;
  gint                 n_threads;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  stream = opj_stream_create_default_file_stream (g_file_peek_path (file), OPJ_TRUE);

  if (! stream)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not open '%s' for reading"),
                   gimp_file_get_utf8_name (file));
      goto out;
    }

  codec = opj_create_decompress (format);
  if (! codec)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Failed to initialize decoder for '%s', out of memory?"),
                   gimp_file_get_utf8_name (file));
      goto out;
    }

  n_threads = gimp_get_num_processors ();
  if (n_threads >= 2 && ! opj_codec_set_threads (codec, n_threads))
    {
      g_warning ("Couldn't set number of threads on decoder for '%s'.",
                 gimp_file_get_utf8_name (file));
    }

  opj_set_default_decoder_parameters (&parameters);
  if (opj_setup_decoder (codec, &parameters) != OPJ_TRUE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Couldn't set parameters on decoder for '%s'."),
                   gimp_file_get_utf8_name (file));
      goto out;
    }

  if (opj_read_header (stream, codec, &image) != OPJ_TRUE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Couldn't read JP2 header from '%s'."),
                   gimp_file_get_utf8_name (file));
      goto out;
    }

  if (opj_decode (codec, stream, image) != OPJ_TRUE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Couldn't decode JP2 image in '%s'."),
                   gimp_file_get_utf8_name (file));
      goto out;
    }

  if (opj_end_decompress (codec, stream) != OPJ_TRUE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Couldn't decompress JP2 image in '%s'."),
                   gimp_file_get_utf8_name (file));
      goto out;
    }

  if (image->icc_profile_buf)
    {
      if (image->icc_profile_len)
        {
          profile = gimp_color_profile_new_from_icc_profile (image->icc_profile_buf,
                                                             image->icc_profile_len,
                                                             error);
          if (! profile)
            goto out;

          *profile_loaded = TRUE;

          if (image->color_space == OPJ_CLRSPC_UNSPECIFIED ||
              image->color_space == OPJ_CLRSPC_UNKNOWN)
            {
              if (gimp_color_profile_is_rgb (profile))
                image->color_space = OPJ_CLRSPC_SRGB;
              else if (gimp_color_profile_is_gray (profile))
                image->color_space = OPJ_CLRSPC_GRAY;
              else if (gimp_color_profile_is_cmyk (profile))
                image->color_space = OPJ_CLRSPC_CMYK;
            }
        }
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Couldn't decode CIELAB JP2 image in '%s'."),
                       gimp_file_get_utf8_name (file));
          goto out;
        }

      free (image->icc_profile_buf);
      image->icc_profile_buf = NULL;
      image->icc_profile_len = 0;
    }

  num_components = image->numcomps;

  if ((image->color_space == OPJ_CLRSPC_UNSPECIFIED ||
       image->color_space == OPJ_CLRSPC_UNKNOWN) && ! interactive)
      image->color_space = color_space;

  if (image->color_space == OPJ_CLRSPC_UNSPECIFIED ||
      image->color_space == OPJ_CLRSPC_UNKNOWN)
    {
      /* Sometimes the color space is not set at this point, which
       * sucks. This happens always with codestream images (.j2c or
       * .j2k) which are meant to be embedded by other files.
       *
       * It might also happen with JP2 in case the header does not have
       * color space and the ICC profile is absent (though this may mean
       * that the JP2 is broken, but let's be flexible and allow manual
       * fallback).
       * Assuming RGB/RGBA space is bogus since this format can handle
       * so much more. Therefore we instead pop-up a dialog asking one
       * to specify the color space in interactive mode.
       */
      if (num_components == 1 || num_components == 2)
        {
          /* Only possibility is gray. */
          image->color_space = OPJ_CLRSPC_GRAY;
        }
      else if (num_components == 5)
        {
          /* Can only be CMYK with Alpha. */
          image->color_space = OPJ_CLRSPC_CMYK;
        }
      else if (interactive)
        {
          image->color_space = open_dialog (procedure, config, file, format,
                                            num_components, error);

          if (image->color_space == OPJ_CLRSPC_UNKNOWN)
            goto out;
        }
      else /* ! interactive */
        {
          /* API call where color space was set to UNKNOWN. We don't
           * want to guess or assume anything. It is much better to just
           * fail. It is the responsibility of the developer to know its
           * data when loading it in a script.
           */
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Unknown color space in JP2 codestream '%s'."),
                       gimp_file_get_utf8_name (file));
          goto out;
        }
    }

  if (image->color_space == OPJ_CLRSPC_SYCC)
    {
      if (! color_sycc_to_rgb (image))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Couldn't convert YCbCr JP2 image '%s' to RGB."),
                       gimp_file_get_utf8_name (file));
          goto out;
        }
    }
  else if (image->color_space == OPJ_CLRSPC_CMYK)
    {
      if (! color_cmyk_to_rgb (image))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Couldn't convert CMYK JP2 image in '%s' to RGB."),
                       gimp_file_get_utf8_name (file));
          goto out;
        }
    }
  else if (image->color_space == OPJ_CLRSPC_EYCC)
    {
      if (! color_esycc_to_rgb (image))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Couldn't convert xvYCC JP2 image in '%s' to RGB."),
                       gimp_file_get_utf8_name (file));
          goto out;
        }
    }

  /* At this point, the image should be converted to Gray or RGB. */
  if (image->color_space == OPJ_CLRSPC_GRAY)
    {
      base_type  = GIMP_GRAY;
      image_type = GIMP_GRAY_IMAGE;

      if (num_components == 2)
        image_type = GIMP_GRAYA_IMAGE;
    }
  else if (image->color_space == OPJ_CLRSPC_SRGB)
    {
      base_type  = GIMP_RGB;
      image_type = GIMP_RGB_IMAGE;

      if (num_components == 4)
        image_type = GIMP_RGBA_IMAGE;
    }
  else
    {
      /* If not gray or RGB, this is an image we cannot handle. */
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported color space in JP2 image '%s'."),
                   gimp_file_get_utf8_name (file));
      goto out;
    }

  width = image->comps[0].w;
  height = image->comps[0].h;

  if (profile)
    linear = gimp_color_profile_is_linear (profile);

  precision_actual = image->comps[0].prec;

  precision_scaled = get_valid_precision (precision_actual);
  image_precision = get_image_precision (precision_scaled, linear);

  gimp_image = gimp_image_new_with_precision (width, height,
                                              base_type, image_precision);

  if (profile)
    gimp_image_set_color_profile (gimp_image, profile);

  layer = gimp_layer_new (gimp_image,
                          _("Background"),
                          width, height,
                          image_type,
                          100,
                          gimp_image_get_default_new_layer_mode (gimp_image));
  gimp_image_insert_layer (gimp_image, layer, NULL, 0);

  file_format = gimp_drawable_get_format (GIMP_DRAWABLE (layer));
  bpp = babl_format_get_bytes_per_pixel (file_format);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
  pixels = g_new0 (guchar, width * bpp);

  for (i = 0; i < height; i++)
    {
      for (j = 0; j < num_components; j++)
        {
          int shift = precision_scaled - precision_actual;

          for (k = 0; k < width; k++)
            {
              if (shift >= 0)
                temp = image->comps[j].data[i * width + k] << shift;
              else /* precision_actual > 32 */
                temp = image->comps[j].data[i * width + k] >> (- shift);

              c = (unsigned char *) &temp;
              for (it = 0; it < (precision_scaled / 8); it++)
                {
                  pixels[k * bpp + j * (precision_scaled / 8) + it] =  c[it];
                }
            }
        }

        gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i, width, 1), 0,
                         file_format, pixels, GEGL_AUTO_ROWSTRIDE);
    }

  g_free (pixels);

  g_object_unref (buffer);
  gimp_progress_update (1.0);

 out:
  if (profile)
    g_object_unref (profile);
  if (image)
    opj_image_destroy (image);
  if (codec)
    opj_destroy_codec (codec);
  if (stream)
    opj_stream_destroy (stream);

  return gimp_image;
}
