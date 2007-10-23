/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Bump map plug-in --- emboss an image by using another image as a bump map
 * Copyright (C) 1997 Federico Mena Quintero <federico@nuclecu.unam.mx>
 * Copyright (C) 1997-2000 Jens Lautenbacher <jtl@gimp.org>
 * Copyright (C) 2000 Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* This plug-in uses the algorithm described by John Schlag, "Fast
 * Embossing Effects on Raster Image Data" in Graphics Gems IV (ISBN
 * 0-12-336155-9).  It takes a grayscale image to be applied as a
 * bump-map to another image, producing a nice embossing effect.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/***** Magic numbers *****/

#define PLUG_IN_PROC       "plug-in-bump-map"
#define PLUG_IN_TILED_PROC "plug-in-bump-map-tiled"
#define PLUG_IN_BINARY     "bumpmap"
#define PLUG_IN_VERSION    "April 2000, 3.0-pre1-ac2"

#define SCALE_WIDTH       100

/***** Types *****/

enum
{
  LINEAR = 0,
  SPHERICAL,
  SINUSOIDAL
};

enum
{
  DRAG_NONE = 0,
  DRAG_BUMPMAP
};

typedef struct
{
  gint32   bumpmap_id;
  gdouble  azimuth;
  gdouble  elevation;
  gint     depth;
  gint     xofs;
  gint     yofs;
  gint     waterlevel;
  gint     ambient;
  gboolean compensate;
  gboolean invert;
  gint     type;
  gboolean tiled;
} bumpmap_vals_t;

typedef struct
{
  gint    lx, ly;       /* X and Y components of light vector */
  gint    nz2, nzlz;    /* nz^2, nz*lz */
  gint    background;   /* Shade for vertical normals */
  gdouble compensation; /* Background compensation */
  guchar  lut[256];     /* Look-up table for modes */
} bumpmap_params_t;

typedef struct
{
  gint               mouse_x;
  gint               mouse_y;
  gint               drag_mode;

  GtkObject         *offset_adj_x;
  GtkObject         *offset_adj_y;

  guchar           **src_rows;
  guchar           **bm_rows;

  GimpDrawable      *bm_drawable;
  gint               bm_width;
  gint               bm_height;
  gint               bm_bpp;
  gboolean           bm_has_alpha;

  GimpPixelRgn       src_rgn;
  GimpPixelRgn       bm_rgn;

  bumpmap_params_t   params;
} bumpmap_interface_t;


/***** Prototypes *****/

static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

static void bumpmap             (void);
static void bumpmap_init_params (bumpmap_params_t *params);
static void bumpmap_row         (const guchar     *src_row,
                                 guchar           *dest_row,
                                 gint              width,
                                 gint              bpp,
                                 gboolean          has_alpha,
                                 const guchar     *bm_row1,
                                 const guchar     *bm_row2,
                                 const guchar     *bm_row3,
                                 gint              bm_width,
                                 gint              bm_xofs,
                                 gboolean          tiled,
                                 gboolean          row_in_bumpmap,
                                 bumpmap_params_t *params);
static void bumpmap_convert_row (guchar           *row,
                                 gint              width,
                                 gint              bpp,
                                 gboolean          has_alpha,
                                 const guchar     *lut);

static gboolean bumpmap_dialog             (void);
static void     dialog_new_bumpmap         (gboolean       init_offsets);
static void     dialog_update_preview      (GimpPreview   *preview);
static gint     dialog_preview_events      (GtkWidget     *area,
                                            GdkEvent      *event,
                                            GimpPreview   *preview);
static void     dialog_get_rows            (GimpPixelRgn  *pr,
                                            guchar       **rows,
                                            gint           x,
                                            gint           y,
                                            gint           width,
                                            gint           height);
static void     dialog_fill_src_rows       (gint           start,
                                            gint           how_many,
                                            gint           yofs);
static void     dialog_fill_bumpmap_rows   (gint           start,
                                            gint           how_many,
                                            gint           yofs);
static gint     dialog_constrain           (gint32         image_id,
                                            gint32         drawable_id,
                                            gpointer       data);
static void     dialog_bumpmap_callback    (GtkWidget     *widget,
                                            GimpPreview   *preview);
static void     dialog_maptype_callback    (GtkWidget     *widget,
                                            GimpPreview   *preview);


/***** Variables *****/

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

static bumpmap_vals_t bmvals =
{
  -1,     /* bumpmap_id */
  135.0,  /* azimuth */
  45.0,   /* elevation */
  3,      /* depth */
  0,      /* xofs */
  0,      /* yofs */
  0,      /* waterlevel */
  0,      /* ambient */
  TRUE,   /* compensate */
  FALSE,  /* invert */
  LINEAR, /* type */
  FALSE   /* tiled */
};

static bumpmap_interface_t bmint =
{
  0,         /* mouse_x */
  0,         /* mouse_y */
  DRAG_NONE, /* drag_mode */
  NULL,      /* offset_adj_x */
  NULL,      /* offset_adj_y */
  NULL,      /* src_rows */
  NULL,      /* bm_rows */
  NULL,      /* bm_drawable */
  0,         /* bm_width */
  0,         /* bm_height */
  0,         /* bm_bpp */
  FALSE,     /* bm_has_alpha */
  { 0, },    /* src_rgn */
  { 0, },    /* bm_rgn */
  { 0, }     /* params */
};

static GimpDrawable *drawable = NULL;

static gint       sel_x1, sel_y1;
static gint       sel_x2, sel_y2;
static gint       sel_width, sel_height;
static gint       img_bpp;
static gboolean   img_has_alpha;

/***** Macros *****/

#define MOD(x, y) \
  ((x) < 0 ? ((y) - 1 - ((y) - 1 - (x)) % (y)) : (x) % (y))

/***** Functions *****/

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",   "Interactive, non-interactive"   },
    { GIMP_PDB_IMAGE,    "image",      "Input image"                    },
    { GIMP_PDB_DRAWABLE, "drawable",   "Input drawable"                 },
    { GIMP_PDB_DRAWABLE, "bumpmap",    "Bump map drawable"              },
    { GIMP_PDB_FLOAT,    "azimuth",    "Azimuth"                        },
    { GIMP_PDB_FLOAT,    "elevation",  "Elevation"                      },
    { GIMP_PDB_INT32,    "depth",      "Depth"                          },
    { GIMP_PDB_INT32,    "xofs",       "X offset"                       },
    { GIMP_PDB_INT32,    "yofs",       "Y offset"                       },
    { GIMP_PDB_INT32,    "waterlevel", "Level that full transparency "
                                       "should represent"               },
    { GIMP_PDB_INT32,    "ambient",    "Ambient lighting factor"        },
    { GIMP_PDB_INT32,    "compensate", "Compensate for darkening"       },
    { GIMP_PDB_INT32,    "invert",     "Invert bumpmap"                 },
    { GIMP_PDB_INT32,    "type",       "Type of map (LINEAR (0), "
                                       "SPHERICAL (1), SINUSOIDAL (2))" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Create an embossing effect using a bump map"),
                          "This plug-in uses the algorithm described by John "
                          "Schlag, \"Fast Embossing Effects on Raster Image "
                          "Data\" in Graphics GEMS IV (ISBN 0-12-336155-9). "
                          "It takes a drawable to be applied as a bump "
                          "map to another image and produces a nice embossing "
                          "effect.",
                          "Federico Mena Quintero, Jens Lautenbacher & Sven Neumann",
                          "Federico Mena Quintero, Jens Lautenbacher & Sven Neumann",
                          PLUG_IN_VERSION,
                          N_("_Bump Map..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Map");

  gimp_install_procedure (PLUG_IN_TILED_PROC,
                          "Create an embossing effect using a tiled image "
                          "as a bump map",
                          "This plug-in uses the algorithm described by John "
                          "Schlag, \"Fast Embossing Effects on Raster Image "
                          "Data\" in Graphics GEMS IV (ISBN 0-12-336155-9). "
                          "It takes a drawable to be tiled and applied as a "
                          "bump map to another image and produces a nice "
                          "embossing effect.",
                          "Federico Mena Quintero, Jens Lautenbacher & Sven Neumann",
                          "Federico Mena Quintero, Jens Lautenbacher & Sven Neumann",
                          PLUG_IN_VERSION,
                          NULL,
                          "RGB*, GRAY*",
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
  static GimpParam values[1];

  GimpRunMode run_mode;
  GimpPDBStatusType  status;

  INIT_I18N ();

  status   = GIMP_PDB_SUCCESS;
  run_mode = param[0].data.d_int32;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  /* Get drawable information */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  gimp_drawable_mask_bounds (drawable->drawable_id,
                             &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  sel_width     = sel_x2 - sel_x1;
  sel_height    = sel_y2 - sel_y1;
  img_bpp       = gimp_drawable_bpp (drawable->drawable_id);
  img_has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  /* See how we will run */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data (name, &bmvals);

      /* Get information from the dialog */
      if (!bumpmap_dialog ())
        return;

      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* Make sure all the arguments are present */
      if (nparams != 14)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          bmvals.bumpmap_id = param[3].data.d_drawable;
          bmvals.azimuth    = param[4].data.d_float;
          bmvals.elevation  = param[5].data.d_float;
          bmvals.depth      = param[6].data.d_int32;
          bmvals.depth      = param[6].data.d_int32;
          bmvals.xofs       = param[7].data.d_int32;
          bmvals.yofs       = param[8].data.d_int32;
          bmvals.waterlevel = param[9].data.d_int32;
          bmvals.ambient    = param[10].data.d_int32;
          bmvals.compensate = param[11].data.d_int32;
          bmvals.invert     = param[12].data.d_int32;
          bmvals.type       = param[13].data.d_int32;
          bmvals.tiled      = strcmp (name, PLUG_IN_TILED_PROC) == 0;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data (name, &bmvals);
      break;

    default:
      break;
    }

  /* Bumpmap the image */

  if (status == GIMP_PDB_SUCCESS)
    {
      if ((gimp_drawable_is_rgb(drawable->drawable_id) ||
           gimp_drawable_is_gray(drawable->drawable_id)))
        {
          /* Run! */
          bumpmap ();

          /* If run mode is interactive, flush displays */
          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /* Store data */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (name, &bmvals, sizeof (bumpmap_vals_t));
        }
    }
  else
    status = GIMP_PDB_EXECUTION_ERROR;

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
bumpmap (void)
{
  bumpmap_params_t  params;
  GimpDrawable     *bm_drawable;
  GimpPixelRgn      src_rgn, dest_rgn, bm_rgn;
  gint              bm_width, bm_height, bm_bpp, bm_has_alpha;
  gint              yofs1, yofs2, yofs3;
  gboolean          row_in_bumpmap;
  guchar           *bm_row1, *bm_row2, *bm_row3, *bm_tmprow;
  guchar           *src_row, *dest_row;
  gint              y;
  gint              progress;
  gint              drawable_tiles_per_row, bm_tiles_per_row;

  gimp_progress_init (_("Bump-mapping"));

  /* Get the bumpmap drawable */
  if (bmvals.bumpmap_id != -1)
    bm_drawable = gimp_drawable_get (bmvals.bumpmap_id);
  else
    bm_drawable = drawable;

  if (!bm_drawable)
    return;

  /* Get image information */
  bm_width     = gimp_drawable_width (bm_drawable->drawable_id);
  bm_height    = gimp_drawable_height (bm_drawable->drawable_id);
  bm_bpp       = gimp_drawable_bpp (bm_drawable->drawable_id);
  bm_has_alpha = gimp_drawable_has_alpha (bm_drawable->drawable_id);

  /* Set the tile cache size */
  /* Compute number of tiles needed for one row of the drawable */
  drawable_tiles_per_row =
    1
    + (sel_x2 + gimp_tile_width () - 1) / gimp_tile_width ()
    - sel_x1 / gimp_tile_width ();
  /* Compute number of tiles needed for one row of the bitmap */
  bm_tiles_per_row = (bm_width + gimp_tile_width () - 1) / gimp_tile_width ();
  /* Cache one row of source, destination and bitmap */
  gimp_tile_cache_ntiles (bm_tiles_per_row + 2 * drawable_tiles_per_row);

  /* Initialize offsets */

  if (bmvals.tiled)
    {
      yofs2 = MOD (bmvals.yofs + sel_y1, bm_height);
      yofs1 = MOD (yofs2 - 1, bm_height);
      yofs3 = MOD (yofs2 + 1, bm_height);
    }
  else
    {
      yofs2 = CLAMP (bmvals.yofs + sel_y1, 0, bm_height - 1);
      yofs1 = yofs2;
      yofs3 = CLAMP (yofs2 + 1, 0, bm_height - 1);
    }

  /* Initialize row buffers */
  bm_row1 = g_new (guchar, bm_width * bm_bpp);
  bm_row2 = g_new (guchar, bm_width * bm_bpp);
  bm_row3 = g_new (guchar, bm_width * bm_bpp);

  src_row  = g_new (guchar, sel_width * img_bpp);
  dest_row = g_new (guchar, sel_width * img_bpp);

  /* Initialize pixel regions */
  gimp_pixel_rgn_init (&src_rgn, drawable,
                       sel_x1, sel_y1, sel_width, sel_height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);
  gimp_pixel_rgn_init (&bm_rgn, bm_drawable,
                       0, 0, bm_width, bm_height, FALSE, FALSE);

  /* Bumpmap */

  bumpmap_init_params (&params);

  gimp_pixel_rgn_get_row (&bm_rgn, bm_row1, 0, yofs1, bm_width);
  gimp_pixel_rgn_get_row (&bm_rgn, bm_row2, 0, yofs2, bm_width);
  gimp_pixel_rgn_get_row (&bm_rgn, bm_row3, 0, yofs3, bm_width);

  bumpmap_convert_row (bm_row1, bm_width, bm_bpp, bm_has_alpha, params.lut);
  bumpmap_convert_row (bm_row2, bm_width, bm_bpp, bm_has_alpha, params.lut);
  bumpmap_convert_row (bm_row3, bm_width, bm_bpp, bm_has_alpha, params.lut);

  for (y = sel_y1, progress = 0; y < sel_y2; y++, progress++)
    {
      row_in_bumpmap = (y >= - bmvals.yofs && y < - bmvals.yofs + bm_height);

      gimp_pixel_rgn_get_row (&src_rgn, src_row, sel_x1, y, sel_width);

      bumpmap_row (src_row, dest_row, sel_width, img_bpp, img_has_alpha,
                   bm_row1, bm_row2, bm_row3, bm_width, bmvals.xofs,
                   bmvals.tiled,
                   row_in_bumpmap,
                   &params);

      gimp_pixel_rgn_set_row (&dest_rgn, dest_row, sel_x1, y, sel_width);

      /* Next line */

      if (bmvals.tiled || row_in_bumpmap)
        {
          bm_tmprow = bm_row1;
          bm_row1   = bm_row2;
          bm_row2   = bm_row3;
          bm_row3   = bm_tmprow;

          if (++yofs2 == bm_height)
            yofs2 = 0;

          if (bmvals.tiled)
            yofs3 = MOD (yofs2 + 1, bm_height);
          else
            yofs3 = CLAMP (yofs2 + 1, 0, bm_height - 1);

          gimp_pixel_rgn_get_row (&bm_rgn, bm_row3, 0, yofs3, bm_width);
          bumpmap_convert_row (bm_row3, bm_width, bm_bpp, bm_has_alpha,
                               params.lut);
        }

      if ((progress % 16) == 0)
        gimp_progress_update ((gdouble) progress / sel_height);
    }

  /* Done */

  g_free (bm_row1);
  g_free (bm_row2);
  g_free (bm_row3);
  g_free (src_row);
  g_free (dest_row);

  if (bm_drawable != drawable)
    gimp_drawable_detach (bm_drawable);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id,
                        sel_x1, sel_y1, sel_width, sel_height);
}

static void
bumpmap_init_params (bumpmap_params_t *params)
{
  /* Convert to radians */
  const gdouble azimuth   = G_PI * bmvals.azimuth / 180.0;
  const gdouble elevation = G_PI * bmvals.elevation / 180.0;

  gint lz, nz;
  gint i;

  /* Calculate the light vector */
  params->lx = cos (azimuth) * cos (elevation) * 255.0;
  params->ly = sin (azimuth) * cos (elevation) * 255.0;
  lz         = sin (elevation) * 255.0;

  /* Calculate constant Z component of surface normal */
  nz           = (6 * 255) / bmvals.depth;
  params->nz2  = nz * nz;
  params->nzlz = nz * lz;

  /* Optimize for vertical normals */
  params->background = lz;

  /* Calculate darkness compensation factor */
  params->compensation = sin(elevation);

  /* Create look-up table for map type */
  for (i = 0; i < 256; i++)
    {
      gdouble n;

      switch (bmvals.type)
        {
        case SPHERICAL:
          n = i / 255.0 - 1.0;
          params->lut[i] = (int) (255.0 * sqrt(1.0 - n * n) + 0.5);
          break;

        case SINUSOIDAL:
          n = i / 255.0;
          params->lut[i] = (int) (255.0 *
                                  (sin((-G_PI / 2.0) + G_PI * n) + 1.0) /
                                  2.0 + 0.5);
          break;

        case LINEAR:
        default:
          params->lut[i] = i;
        }

      if (bmvals.invert)
        params->lut[i] = 255 - params->lut[i];
    }
}

static void
bumpmap_row (const guchar     *src,
             guchar           *dest,
             gint              width,
             gint              bpp,
             gboolean          has_alpha,
             const guchar     *bm_row1,
             const guchar     *bm_row2,
             const guchar     *bm_row3,
             gint              bm_width,
             gint              bm_xofs,
             gboolean          tiled,
             gboolean          row_in_bumpmap,
             bumpmap_params_t *params)
{
  gint xofs1, xofs2;
  gint x, k;
  gint pbpp;
  gint result;
  gint tmp;

  if (has_alpha)
    pbpp = bpp - 1;
  else
    pbpp = bpp;

  tmp = bm_xofs + sel_x1;
  xofs2 = MOD (tmp, bm_width);

  for (x = 0; x < width; x++)
    {
      gint xofs3;
      gint shade;
      gint nx, ny;

      /* Calculate surface normal from bump map */

      if (tiled || (row_in_bumpmap &&
                    x >= - tmp && x < - tmp + bm_width))
        {
          if (tiled)
            {
              xofs1 = MOD (xofs2 - 1, bm_width);
              xofs3 = MOD (xofs2 + 1, bm_width);
            }
          else
            {
              xofs1 = CLAMP (xofs2 - 1, 0, bm_width - 1);
              xofs3 = CLAMP (xofs2 + 1, 0, bm_width - 1);
            }

          nx = (bm_row1[xofs1] + bm_row2[xofs1] + bm_row3[xofs1] -
                bm_row1[xofs3] - bm_row2[xofs3] - bm_row3[xofs3]);
          ny = (bm_row3[xofs1] + bm_row3[xofs2] + bm_row3[xofs3] -
                bm_row1[xofs1] - bm_row1[xofs2] - bm_row1[xofs3]);
        }
      else
        {
          nx = ny = 0;
        }

      /* Shade */

      if ((nx == 0) && (ny == 0))
        {
          shade = params->background;
        }
      else
        {
          gint ndotl = nx * params->lx + ny * params->ly + params->nzlz;

          if (ndotl < 0)
            {
              shade = params->compensation * bmvals.ambient;
            }
          else
            {
              shade = ndotl / sqrt (nx * nx + ny * ny + params->nz2);

              shade = shade + MAX(0, (255 * params->compensation - shade)) *
                bmvals.ambient / 255;
            }
        }

      /* Paint */

      if (bmvals.compensate)
        {
          for (k = pbpp; k; k--)
            {
              result  = (*src++ * shade) / (params->compensation * 255);
              *dest++ = MIN(255, result);
            }
        }
      else
        {
          for (k = pbpp; k; k--)
            *dest++ = *src++ * shade / 255;
        }

      if (has_alpha)
        *dest++ = *src++;

      /* Next pixel */

      if (++xofs2 == bm_width)
        xofs2 = 0;
    }
}

static void
bumpmap_convert_row (guchar       *row,
                     gint          width,
                     gint          bpp,
                     gboolean      has_alpha,
                     const guchar *lut)
{
  guchar *p = row;

  has_alpha = has_alpha ? 1 : 0;

  if (bpp >= 3)
    for (; width; width--)
      {
        if (has_alpha)
          *p++ = lut[(gint) (bmvals.waterlevel +
                             (((gint) (GIMP_RGB_LUMINANCE (row[0],
                                                           row[1],
                                                           row[2]) + 0.5) -
                               bmvals.waterlevel) * row[3]) / 255.0)];
        else
          *p++ = lut[(gint) (GIMP_RGB_LUMINANCE (row[0],
                                                 row[1],
                                                 row[2]) + 0.5)];

        row += 3 + has_alpha;
      }
  else
    for (; width; width--)
      {
        if (has_alpha)
          *p++ = lut[bmvals.waterlevel +
                    ((row[0] - bmvals.waterlevel) * row[1]) / 255];
        else
          *p++ = lut[*row];

        row += 1 + has_alpha;
      }
}

static gboolean
bumpmap_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *paned;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *preview;
  GtkWidget *table;
  GtkWidget *combo;
  GtkWidget *button;
  GtkObject *adj;
  gboolean   run;
  gint       row = 0;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Bump Map"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  paned = gtk_hpaned_new ();
  gtk_container_set_border_width (GTK_CONTAINER (paned), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), paned);
  gtk_widget_show (paned);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_paned_pack1 (GTK_PANED (paned), hbox, TRUE, FALSE);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_box_pack_end (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  preview = gimp_drawable_preview_new (drawable, NULL);
  gtk_container_add (GTK_CONTAINER (hbox), preview);
  gtk_widget_show (preview);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (dialog_update_preview),
                    NULL);
  g_signal_connect (GIMP_PREVIEW (preview)->area, "event",
                    G_CALLBACK (dialog_preview_events), preview);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_paned_pack2 (GTK_PANED (paned), hbox, FALSE, FALSE);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (hbox), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (12, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Bump map menu */
  combo = gimp_drawable_combo_box_new (dialog_constrain, NULL);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), bmvals.bumpmap_id,
                              G_CALLBACK (dialog_bumpmap_callback),
                              preview);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("_Bump map:"), 0.0, 0.5, combo, 2, FALSE);

  /* Map type menu */
  combo = gimp_int_combo_box_new (_("Linear"),     LINEAR,
                                  _("Spherical"),  SPHERICAL,
                                  _("Sinusoidal"), SINUSOIDAL,
                                  NULL);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), bmvals.type,
                              G_CALLBACK (dialog_maptype_callback),
                              preview);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, row,
                             _("_Map type:"), 0.0, 0.5, combo, 2, FALSE);

  gtk_table_set_row_spacing (GTK_TABLE (table), row++, 12);

  /* Compensate darkening */
  button = gtk_check_button_new_with_mnemonic (_("Co_mpensate for darkening"));
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 3, row, row + 1);
  gtk_widget_show (button);
  row++;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), bmvals.compensate);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &bmvals.compensate);
  g_signal_connect_swapped (button, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* Invert bumpmap */
  button = gtk_check_button_new_with_mnemonic (_("I_nvert bumpmap"));
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 3, row, row + 1);
  gtk_widget_show (button);
  row++;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), bmvals.invert);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &bmvals.invert);
  g_signal_connect_swapped (button, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* Tile bumpmap */
  button = gtk_check_button_new_with_mnemonic (_("_Tile bumpmap"));
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 3, row, row + 1);
  gtk_widget_show (button);

  gtk_table_set_row_spacing (GTK_TABLE (table), row++, 12);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), bmvals.tiled);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &bmvals.tiled);
  g_signal_connect_swapped (button, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_Azimuth:"), SCALE_WIDTH, 6,
                              bmvals.azimuth, 0.0, 360.0, 1.0, 15.0, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &bmvals.azimuth);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_Elevation:"), SCALE_WIDTH, 6,
                              bmvals.elevation, 0.5, 90.0, 1.0, 5.0, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &bmvals.elevation);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row,
                              _("_Depth:"), SCALE_WIDTH, 6,
                              bmvals.depth, 1.0, 65.0, 1.0, 5.0, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &bmvals.depth);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  gtk_table_set_row_spacing (GTK_TABLE (table), row++, 12);

  bmint.offset_adj_x = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                          _("_X offset:"), SCALE_WIDTH, 6,
                          bmvals.xofs, -1000.0, 1001.0, 1.0, 10.0, 0,
                          TRUE, 0, 0,
                          _("The offset can be adjusted by dragging the "
                            "preview using the middle mouse button."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &bmvals.xofs);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  bmint.offset_adj_y = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, row,
                          _("_Y offset:"), SCALE_WIDTH, 6,
                          bmvals.yofs, -1000.0, 1001.0, 1.0, 10.0, 0,
                          TRUE, 0, 0,
                          _("The offset can be adjusted by dragging the "
                            "preview using the middle mouse button."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &bmvals.yofs);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  gtk_table_set_row_spacing (GTK_TABLE (table), row++, 12);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_Waterlevel:"), SCALE_WIDTH, 6,
                              bmvals.waterlevel, 0.0, 255.0, 1.0, 8.0, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &bmvals.waterlevel);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("A_mbient:"), SCALE_WIDTH, 6,
                              bmvals.ambient, 0.0, 255.0, 1.0, 8.0, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &bmvals.ambient);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* Initialise drawable
   * (don't initialise offsets if bumpmap_id is already known)
   */
  if (bmvals.bumpmap_id == -1)
    dialog_new_bumpmap (TRUE);
  else
    dialog_new_bumpmap (FALSE);


  /* Done */

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  if (bmint.bm_drawable != drawable)
    gimp_drawable_detach (bmint.bm_drawable);

  return run;
}

static gint
dialog_preview_events (GtkWidget   *area,
                       GdkEvent    *event,
                       GimpPreview *preview)
{
  gint            x, y;
  gint            dx, dy;
  GdkEventButton *bevent;

  gtk_widget_get_pointer (area, &x, &y);

  bevent = (GdkEventButton *) event;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      switch (bevent->button)
        {
        case 2:
          bmint.drag_mode = DRAG_BUMPMAP;
          break;

        default:
          return FALSE;
        }

      bmint.mouse_x = x;
      bmint.mouse_y = y;

      gtk_grab_add (area);

      return TRUE;
      break;

    case GDK_BUTTON_RELEASE:
      if (bmint.drag_mode != DRAG_NONE)
        {
          gtk_grab_remove (area);
          bmint.drag_mode = DRAG_NONE;
          gimp_preview_invalidate (preview);

          return TRUE;
        }

      break;

    case GDK_MOTION_NOTIFY:
      dx = x - bmint.mouse_x;
      dy = y - bmint.mouse_y;

      bmint.mouse_x = x;
      bmint.mouse_y = y;

      if ((dx == 0) && (dy == 0))
        break;

      switch (bmint.drag_mode)
        {
        case DRAG_BUMPMAP:
          bmvals.xofs = CLAMP (bmvals.xofs - dx, -1000, 1000);
          g_signal_handlers_block_by_func (bmint.offset_adj_x,
                                           gimp_int_adjustment_update,
                                           &bmvals.xofs);
          gtk_adjustment_set_value (GTK_ADJUSTMENT (bmint.offset_adj_x),
                                    bmvals.xofs);
          g_signal_handlers_unblock_by_func (bmint.offset_adj_x,
                                             gimp_int_adjustment_update,
                                             &bmvals.xofs);

          bmvals.yofs = CLAMP (bmvals.yofs - dy, -1000, 1000);
          g_signal_handlers_block_by_func (bmint.offset_adj_y,
                                           gimp_int_adjustment_update,
                                           &bmvals.yofs);
          gtk_adjustment_set_value (GTK_ADJUSTMENT (bmint.offset_adj_y),
                                    bmvals.yofs);
          g_signal_handlers_unblock_by_func (bmint.offset_adj_y,
                                             gimp_int_adjustment_update,
                                             &bmvals.yofs);
          break;

        default:
          return FALSE;
        }

      gimp_preview_invalidate (preview);

      return TRUE;
      break;

    default:
      break;
    }

  return FALSE;
}

static void
dialog_new_bumpmap (gboolean init_offsets)
{
  /* Get drawable */
  if (bmint.bm_drawable && (bmint.bm_drawable != drawable))
    gimp_drawable_detach (bmint.bm_drawable);

  if (bmvals.bumpmap_id != -1)
    bmint.bm_drawable = gimp_drawable_get (bmvals.bumpmap_id);
  else
    bmint.bm_drawable = drawable;

  if (!bmint.bm_drawable)
    return;

  /* Get sizes */
  bmint.bm_width     = gimp_drawable_width (bmint.bm_drawable->drawable_id);
  bmint.bm_height    = gimp_drawable_height (bmint.bm_drawable->drawable_id);
  bmint.bm_bpp       = gimp_drawable_bpp (bmint.bm_drawable->drawable_id);
  bmint.bm_has_alpha = gimp_drawable_has_alpha (bmint.bm_drawable->drawable_id);

  if (init_offsets)
    {
      GtkAdjustment  *adj;
      gint            bump_offset_x;
      gint            bump_offset_y;
      gint            draw_offset_y;
      gint            draw_offset_x;

      gimp_drawable_offsets (bmint.bm_drawable->drawable_id,
                             &bump_offset_x, &bump_offset_y);
      gimp_drawable_offsets (drawable->drawable_id,
                             &draw_offset_x, &draw_offset_y);

      bmvals.xofs = draw_offset_x - bump_offset_x;
      bmvals.yofs = draw_offset_y - bump_offset_y;

      adj = (GtkAdjustment *) bmint.offset_adj_x;
      if (adj)
        {
          adj->value = bmvals.xofs;
          g_signal_handlers_block_by_func (adj,
                                           gimp_int_adjustment_update,
                                           &bmvals.xofs);
          gtk_adjustment_value_changed (adj);
          g_signal_handlers_unblock_by_func (adj,
                                             gimp_int_adjustment_update,
                                             &bmvals.xofs);
        }

      adj = (GtkAdjustment *) bmint.offset_adj_y;
      if (adj)
        {
          adj->value = bmvals.yofs;
          g_signal_handlers_block_by_func (adj,
                                           gimp_int_adjustment_update,
                                           &bmvals.yofs);
          gtk_adjustment_value_changed (adj);
          g_signal_handlers_unblock_by_func (adj,
                                             gimp_int_adjustment_update,
                                             &bmvals.yofs);
        }
    }

  /* Initialize pixel region */
  gimp_pixel_rgn_init (&bmint.bm_rgn, bmint.bm_drawable,
                       0, 0, bmint.bm_width, bmint.bm_height, FALSE, FALSE);
}

static void
dialog_update_preview (GimpPreview *preview)
{
  guchar *dest_row;
  gint    y;
  gint    x1, y1;
  gint    width, height;
  gint    bytes;

  gimp_preview_get_position (preview, &x1, &y1);
  gimp_preview_get_size (preview, &width, &height);
  bytes = drawable->bpp;

  /* Initialize source rows */
  gimp_pixel_rgn_init (&bmint.src_rgn, drawable,
                       sel_x1, sel_y1, sel_width, sel_height, FALSE, FALSE);

  bmint.src_rows = g_new (guchar *, height);

  for (y = 0; y < height; y++)
    bmint.src_rows[y]  = g_new (guchar, sel_width * 4);

  dialog_fill_src_rows (0, height, y1);

  /* Initialize bumpmap rows */
  bmint.bm_rows = g_new (guchar *, height + 2);

  for (y = 0; y < height + 2; y++)
    bmint.bm_rows[y] = g_new (guchar, bmint.bm_width * bmint.bm_bpp);

  bumpmap_init_params (&bmint.params);

  dialog_fill_bumpmap_rows (0, height, bmvals.yofs + y1);

  dest_row = g_new (guchar, width * height * 4);

  /* Bumpmap */

  for (y = 0; y < height; y++)
    {
      gint isfirst = ((y == - bmvals.yofs - y1)
                      && ! bmvals.tiled) ? 1 : 0;
      gint islast = (y == (- bmvals.yofs - y1
                           + bmint.bm_height - 1) && ! bmvals.tiled) ? 1 : 0;
      bumpmap_row (bmint.src_rows[y] + 4 * x1,
                   dest_row + 4 * width * y,
                   width, 4, TRUE,
                   bmint.bm_rows[y + isfirst],
                   bmint.bm_rows[y + 1],
                   bmint.bm_rows[y + 2 - islast],
                   bmint.bm_width, bmvals.xofs + x1,
                   bmvals.tiled,
                   y >= - bmvals.yofs - y1 &&
                   y < (- bmvals.yofs - y1 + bmint.bm_height),
                   &bmint.params);

    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview->area),
                          0, 0, width, height,
                          GIMP_RGBA_IMAGE,
                          dest_row,
                          4 * width);

  g_free (dest_row);

  for (y = 0; y < height + 2; y++)
    g_free (bmint.bm_rows[y]);
  g_free (bmint.bm_rows);

  for (y = 0; y < height; y++)
    g_free (bmint.src_rows[y]);
  g_free (bmint.src_rows);
}

static void
dialog_get_rows (GimpPixelRgn  *pr,
                 guchar       **rows,
                 gint           x,
                 gint           y,
                 gint           width,
                 gint           height)
{
  /* This is shamelessly ripped off from gimp_pixel_rgn_get_rect().
   * Its function is exactly the same, but it can fetch an image
   * rectangle to a sparse buffer which is defined as separate
   * rows instead of one big linear region.
   */

  gint xstart, ystart;
  gint xend, yend;
  gint xboundary;
  gint yboundary;
  gint xstep, ystep;
  gint b, bpp;
  gint tx, ty;
  gint tile_width  = gimp_tile_width();
  gint tile_height = gimp_tile_height();

  bpp = pr->bpp;

  xstart = x;
  ystart = y;
  xend   = x + width;
  yend   = y + height;
  ystep  = 0; /* Shut up -Wall */

  while (y < yend)
    {
      x = xstart;

      while (x < xend)
        {
          GimpTile *tile;

          tile = gimp_drawable_get_tile2 (pr->drawable, pr->shadow, x, y);
          gimp_tile_ref (tile);

          xstep     = tile->ewidth - (x % tile_width);
          ystep     = tile->eheight - (y % tile_height);
          xboundary = x + xstep;
          yboundary = y + ystep;
          xboundary = MIN (xboundary, xend);
          yboundary = MIN (yboundary, yend);

          for (ty = y; ty < yboundary; ty++)
            {
              const guchar *src;
              guchar       *dest;

              src = tile->data + tile->bpp * (tile->ewidth *
                                              (ty % tile_height) +
                                               (x % tile_width));
              dest = rows[ty - ystart] + bpp * (x - xstart);

              for (tx = x; tx < xboundary; tx++)
                for (b = bpp; b; b--)
                  *dest++ = *src++;
            }

          gimp_tile_unref (tile, FALSE);

          x += xstep;
        }

      y += ystep;
    }
}

static void
dialog_fill_src_rows (gint start,
                      gint how_many,
                      gint yofs)
{
  gint x;
  gint y;

  dialog_get_rows (&bmint.src_rgn,
                   bmint.src_rows + start,
                   0/*sel_x1*/,
                   yofs,
                   sel_width,
                   how_many);

  /* Convert to RGBA.  We move backwards! */

  for (y = start; y < (start + how_many); y++)
    {
      const guchar *sp = bmint.src_rows[y] + img_bpp * sel_width - 1;
      guchar       *p  = bmint.src_rows[y] + 4 * sel_width - 1;

      for (x = 0; x < sel_width; x++)
        {
          if (img_has_alpha)
            *p-- = *sp--;
          else
            *p-- = 255;

          if (img_bpp < 3)
            {
              *p-- = *sp;
              *p-- = *sp;
              *p-- = *sp--;
            }
          else
            {
              *p-- = *sp--;
              *p-- = *sp--;
              *p-- = *sp--;
            }
        }
    }
}

static void
dialog_fill_bumpmap_rows (gint start,
                          gint how_many,
                          gint yofs)
{
  gint buf_row_ofs;
  gint remaining;
  gint this_pass;

  /* Adapt to offset of selection */
  yofs = MOD (yofs + sel_y1, bmint.bm_height);

  buf_row_ofs = start;
  remaining   = how_many;

  while (remaining > 0)
    {
      this_pass = MIN (remaining, bmint.bm_height - yofs);

      dialog_get_rows (&bmint.bm_rgn,
                       bmint.bm_rows + buf_row_ofs,
                       0,
                       yofs,
                       bmint.bm_width,
                       this_pass);

      yofs         = (yofs + this_pass) % bmint.bm_height;
      remaining   -= this_pass;
      buf_row_ofs += this_pass;
    }

  /* Convert rows */

  for (; how_many; how_many--)
    {
      bumpmap_convert_row (bmint.bm_rows[start],
                           bmint.bm_width,
                           bmint.bm_bpp,
                           bmint.bm_has_alpha,
                           bmint.params.lut);

      start++;
    }
}

static gboolean
dialog_constrain (gint32   image_id,
                  gint32   drawable_id,
                  gpointer data)
{
  return (gimp_drawable_is_rgb (drawable_id) ||
          gimp_drawable_is_gray (drawable_id));
}

static void
dialog_bumpmap_callback (GtkWidget   *widget,
                         GimpPreview *preview)
{
  gint32  drawable_id;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &drawable_id);

  if (bmvals.bumpmap_id != drawable_id)
    {
      bmvals.bumpmap_id = drawable_id;
      dialog_new_bumpmap (TRUE);
      gimp_preview_invalidate (preview);
    }
}

static void
dialog_maptype_callback (GtkWidget   *widget,
                         GimpPreview *preview)
{
  gint32  maptype;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &maptype);

  if (bmvals.type != maptype)
    {
      bmvals.type = maptype;
      bumpmap_init_params (&bmint.params);
      gimp_preview_invalidate (preview);
    }
}
