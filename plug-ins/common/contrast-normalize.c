/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Normalize 1.00 --- image filter plug-in
 *
 * Copyright (C) 1997 Adam D. Moss (adam@foxbox.org)
 * Very largely based on Quartic's "Contrast Autostretch"
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


/* This plugin performs almost the same operation as the 'contrast
 * autostretch' plugin, except that it won't allow the color channels
 * to normalize independently.  This is actually what most people probably
 * want instead of contrast-autostretch; use c-a only if you wish to remove
 * an undesirable color-tint from a source image which is supposed to
 * contain pure-white and pure-black.
 */

#include "config.h"

#include <stdlib.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC "plug-in-normalize"


/* Declare local functions.
 */
static void   query             (void);
static void   run               (const gchar      *name,
                                 gint              nparams,
                                 const GimpParam  *param,
                                 gint             *nreturn_vals,
                                 GimpParam       **return_vals);

static void   normalize         (GimpDrawable     *drawable);
static void   indexed_normalize (gint32            image_ID);


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
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image"    },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Stretch brightness values to cover the full range"),
                          "This plug-in performs almost the same operation as "
                          "the 'contrast autostretch' plug-in, except that it "
                          "won't allow the color channels to normalize "
                          "independently.  This is actually what most people "
                          "probably want instead of contrast-autostretch; use "
                          "c-a only if you wish to remove an undesirable "
                          "color-tint from a source image which is supposed to "
                          "contain pure-white and pure-black.",
                          "Adam D. Moss, Federico Mena Quintero",
                          "Adam D. Moss, Federico Mena Quintero",
                          "1997",
                          N_("_Normalize"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  gint32             image_ID;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  image_ID = param[1].data.d_image;

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id))
    {
      gimp_progress_init (_("Normalizing"));
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

      normalize (drawable);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }
  else if (gimp_drawable_is_indexed (drawable->drawable_id))
    {
      indexed_normalize (image_ID);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


static void
indexed_normalize (gint32 image_ID)  /* a.d.m. */
{
  guchar *cmap;
  gint ncols,i;
  gint hi=0,lo=255;

  cmap = gimp_image_get_colormap (image_ID, &ncols);

  if (cmap==NULL)
    {
      g_printerr ("normalize: cmap was NULL!  Quitting...\n");
      return;
    }

  for (i=0;i<ncols;i++)
    {
      if (cmap[i*3 +0] > hi) hi=cmap[i*3 +0];
      if (cmap[i*3 +1] > hi) hi=cmap[i*3 +1];
      if (cmap[i*3 +2] > hi) hi=cmap[i*3 +2];
      if (cmap[i*3 +0] < lo) lo=cmap[i*3 +0];
      if (cmap[i*3 +1] < lo) lo=cmap[i*3 +1];
      if (cmap[i*3 +2] < lo) lo=cmap[i*3 +2];
    }

  if (hi!=lo)
    for (i=0;i<ncols;i++)
      {
        cmap[i*3 +0] = (255 * (cmap[i*3 +0] - lo)) / (hi-lo);
        cmap[i*3 +1] = (255 * (cmap[i*3 +1] - lo)) / (hi-lo);
        cmap[i*3 +2] = (255 * (cmap[i*3 +2] - lo)) / (hi-lo);
      }

  gimp_image_set_colormap (image_ID, cmap, ncols);
}

typedef struct
{
  guchar  lut[256];
  gdouble min;
  gdouble max;
  gint alpha;
  gboolean has_alpha;
} NormalizeParam_t;

static void
find_min_max (const guchar *src,
              gint          bpp,
              gpointer      data)
{
  NormalizeParam_t *param = (NormalizeParam_t*) data;
  gint              b;

  for (b = 0; b < param->alpha; b++)
    {
      if (!param->has_alpha || src[param->alpha])
        {
          if (src[b] < param->min)
            param->min = src[b];
          if (src[b] > param->max)
            param->max = src[b];
        }
    }
}

static void
normalize_func (const guchar *src,
                guchar       *dest,
                gint          bpp,
                gpointer      data)
{
  NormalizeParam_t *param = (NormalizeParam_t*) data;
  gint              b;

  for (b = 0; b < param->alpha; b++)
    dest[b] = param->lut[src[b]];

  if (param->has_alpha)
    dest[param->alpha] = src[param->alpha];
}

static void
normalize (GimpDrawable *drawable)
{
  NormalizeParam_t param;
  gint x;
  guchar  range;

  param.min = 255;
  param.max = 0;
  param.has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  param.alpha = (param.has_alpha) ? drawable->bpp - 1 : drawable->bpp;

  gimp_rgn_iterate1 (drawable, 0 /* unused */, find_min_max, &param);

  /* Calculate LUT */

  range = param.max - param.min;

  if (range != 0)
    for (x = param.min; x <= param.max; x++)
      param.lut[x] = 255 * (x - param.min) / range;
  else
    param.lut[(gint)param.min] = param.min;

  gimp_rgn_iterate2 (drawable, 0 /* unused */, normalize_func, &param);
}
