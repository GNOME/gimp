/****************************************************************************
 * This is a plugin for GIMP v 1.0 or later.  Documentation is
 * available at http://www.rru.com/~meo/gimp/ .
 *
 * Copyright (C) 1997-8 Miles O'Neal  <meo@rru.com>  http://www.rru.com/~meo/
 * GUI based on GTK code from:
 *    alienmap (Copyright (C) 1996, 1997 Daniel Cotting)
 *    plasma   (Copyright (C) 1996 Stephen Norris),
 *    oilify   (Copyright (C) 1996 Torsten Martinsen),
 *    ripple   (Copyright (C) 1997 Brian Degenhardt) and
 *    whirl    (Copyright (C) 1997 Federico Mena Quintero).
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
 *
 ****************************************************************************/

/****************************************************************************
 * Randomize:
 *
 * randomize version 1.7 (1 May 1998, MEO)
 *
 * Please send any patches or suggestions to the author: meo@rru.com .
 *
 * This plug-in adds a user-defined amount of randomization to an
 * image.  Variations include:
 *
 *  - hurling (spewing random colors)
 *  - picking a nearby pixel at random
 *  - slurring (a crude form of melting)
 *
 * In any case, for each pixel in the selection or image,
 * whether to change the pixel is decided by picking a
 * random number, weighted by the user's "randomization" percentage.
 * If the random number is in range, the pixel is modified.  Picking
 * one selects the new pixel value at random from the current and
 * adjacent pixels.  Hurling assigns a random value to the pixel.
 * Slurring sort of melts downwards; if a pixel is to be slurred,
 * there is an 80% chance the pixel above be used; otherwise, one
 * of the pixels adjacent to the one above is used (even odds as
 * to which it will be).
 *
 * Picking, hurling and slurring work with any image type.
 *
 * This plug-in's effectiveness varies a lot with the type
 * and clarity of the image being "randomized".
 *
 * Hurling more than 75% or so onto an existing image will
 * make the image nearly unrecognizable.  By 90% hurl, most
 * images are indistinguishable from random noise.
 *
 * The repeat count is especially useful with slurring.
 *
 * TODO List
 *
 *  - add a real melt function
 ****************************************************************************/

/****************************************************************************
 *
 * 3 Jan 2008 -- weskaggs
 *
 * Added previews and ability to control the intensity at each point
 * using a "map drawable"
 *
 ****************************************************************************/

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/*********************************
 *
 *  PLUGIN-SPECIFIC CONSTANTS
 *
 ********************************/

#define PLUG_IN_BINARY "randomize"

/*
 *  progress meter update frequency
 */
#define PROG_UPDATE_TIME ((row % 12) == 0)

gchar *PLUG_IN_PROC[] =
{
  "plug-in-randomize-hurl",
  "plug-in-randomize-pick",
  "plug-in-randomize-slur",
  "plug-in-variable-hurl",
  "plug-in-variable-pick",
  "plug-in-variable-slur",
};

gchar *RNDM_NAME[] =
{
  N_("Random Hurl"),
  N_("Random Pick"),
  N_("Random Slur"),
  N_("Random Hurl"),
  N_("Random Pick"),
  N_("Random Slur"),
};

#define RNDM_HURL      1
#define RNDM_PICK      2
#define RNDM_SLUR      3
#define VARIABLE_HURL  4
#define VARIABLE_PICK  5
#define VARIABLE_SLUR  6

#define SEED_DEFAULT  10

#define SCALE_WIDTH  100

gint rndm_type = RNDM_HURL;  /* hurl, pick, etc. */

/*********************************
 *
 *  PLUGIN-SPECIFIC STRUCTURES AND DATA
 *
 ********************************/

typedef struct
{
  gdouble  rndm_pct;     /* likelihood of randomization (as %age)         */
  gdouble  rndm_rcount;  /* repeat count                                  */
  gboolean randomize;    /* Whether to use a random seed                  */
  guint    seed;         /* seed value for g_rand_set_seed() function     */
  gint32   drawable_id;  /* drawable that we are working on               */
  gboolean use_map;      /* use a mask to scale likelihoods at each pixel */
  gint32   map_id;       /* drawable used to scale likelihood             */
  gboolean map_is_rgb;   /* whether map drawable is rgb or grayscale      */
} RandomizeVals;

static RandomizeVals pivals =
{
  50.0,
  1.0,
  FALSE,
  SEED_DEFAULT,
  -1,
  FALSE,
  -1,
  FALSE
};

static GRand     *gr;   /* The GRand object which generates the
                         * random numbers */

static GtkWidget *map_drawable_combo;

/*********************************
 *
 *  LOCAL FUNCTIONS
 *
 ********************************/

static void query (void);
static void run   (const gchar      *name,
		   gint              nparams,
		   const GimpParam  *param,
		   gint             *nreturn_vals,
		   GimpParam       **return_vals);

static void randomize                    (GimpDrawable *drawable,
                                          GimpPreview  *preview);

static inline void randomize_prepare_row (GimpPixelRgn *pixel_rgn,
					  guchar       *data,
					  gint          x,
					  gint          y,
					  gint          w);

static gboolean    randomize_dialog      (GimpDrawable *drawable);

static gboolean    dialog_constrain      (gint32        image_id,
                                          gint32        drawable_id,
                                          gpointer      data);

static void        dialog_usemap_callback (GtkToggleButton *toggle,
                                           GimpPreview     *preview);

static void        dialog_setmap_callback (GimpIntComboBox *combo,
                                           GimpPreview     *preview);

/************************************ Guts ***********************************/

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
    { GIMP_PDB_INT32,    "run_mode",    "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",       "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",    "Input drawable" },
    { GIMP_PDB_FLOAT,    "rndm_pct",    "Randomization percentage (1.0 - 100.0)" },
    { GIMP_PDB_FLOAT,    "rndm_rcount", "Repeat count (1.0 - 100.0)" },
    { GIMP_PDB_INT32,    "randomize",   "Use random seed (TRUE, FALSE)" },
    { GIMP_PDB_INT32,    "seed",        "Seed value (used only if randomize is FALSE)" }
  };

  static const GimpParamDef args_with_map[] =
  {
    { GIMP_PDB_INT32,    "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Input drawable" },
    { GIMP_PDB_FLOAT,    "rndm_pct",     "Randomization percentage (1.0 - 100.0)" },
    { GIMP_PDB_FLOAT,    "rndm_rcount",  "Repeat count (1.0 - 100.0)" },
    { GIMP_PDB_INT32,    "randomize",    "Use random seed (TRUE, FALSE)" },
    { GIMP_PDB_INT32,    "seed",         "Seed value (used only if randomize is FALSE)" },
    { GIMP_PDB_DRAWABLE, "map_drawable", "Drawable used as intensity mask" }
  };

  const gchar *hurl_blurb =
    N_("Completely randomize a fraction of pixels");
  const gchar *pick_blurb =
    N_("Randomly interchange some pixels with neighbors");
  const gchar *slur_blurb =
    N_("Randomly slide some pixels downward (similar to melting)");

  const gchar *hurl_help =
    "This plug-in ``hurls'' randomly-valued pixels onto the selection or "
    "image.  You may select the percentage of pixels to modify and the number "
    "of times to repeat the process.";
  const gchar *pick_help =
    "This plug-in replaces a pixel with a random adjacent pixel.  You may "
    "select the percentage of pixels to modify and the number of times to "
    "repeat the process.";
  const gchar *slur_help =
    "This plug-in slurs (melts like a bunch of icicles) an image.  You may "
    "select the percentage of pixels to modify and the number of times to "
    "repeat the process.";

  const gchar *author = "Miles O'Neal  <meo@rru.com>";
  const gchar *copyrights = "Miles O'Neal, Spencer Kimball, Peter Mattis, "
    "Torsten Martinsen, Brian Degenhardt, Federico Mena Quintero, Stephen "
    "Norris, Daniel Cotting";
  const gchar *copyright_date = "1995-1998";

  gimp_install_procedure ("plug-in-randomize-hurl",
			  hurl_blurb,
			  hurl_help,
			  author,
			  copyrights,
			  copyright_date,
			  N_("_Hurl..."),
			  "RGB*, GRAY*, INDEXED*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);

  gimp_install_procedure ("plug-in-randomize-pick",
			  pick_blurb,
			  pick_help,
			  author,
			  copyrights,
			  copyright_date,
			  N_("_Pick..."),
			  "RGB*, GRAY*, INDEXED*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);

  gimp_install_procedure ("plug-in-randomize-slur",
			  slur_blurb,
			  slur_help,
			  author,
			  copyrights,
			  copyright_date,
			  N_("_Slur..."),
			  "RGB*, GRAY*, INDEXED*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);

  gimp_install_procedure ("plug-in-variable-hurl",
			  hurl_blurb,
			  hurl_help,
			  author,
			  copyrights,
			  copyright_date,
			  N_("_Hurl..."),
			  "RGB*, GRAY*, INDEXED*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args_with_map), 0,
			  args_with_map, NULL);

  gimp_install_procedure ("plug-in-variable-pick",
			  pick_blurb,
			  pick_help,
			  author,
			  copyrights,
			  copyright_date,
			  N_("_Pick..."),
			  "RGB*, GRAY*, INDEXED*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args_with_map), 0,
			  args_with_map, NULL);

  gimp_install_procedure ("plug-in-variable-slur",
			  slur_blurb,
			  slur_help,
			  author,
			  copyrights,
			  copyright_date,
			  N_("_Slur..."),
			  "RGB*, GRAY*, INDEXED*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args_with_map), 0,
			  args_with_map, NULL);

  gimp_plugin_menu_register ("plug-in-variable-hurl", "<Image>/Filters/Noise");
  gimp_plugin_menu_register ("plug-in-variable-pick", "<Image>/Filters/Blur");
  gimp_plugin_menu_register ("plug-in-variable-slur", "<Image>/Filters/Noise");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status     = GIMP_PDB_SUCCESS;
  static GimpParam   values[1];

  INIT_I18N ();

  /*
   *  Get the specified drawable, do standard initialization.
   */
  if (strcmp (name, "plug-in-randomize-hurl") == 0)
    rndm_type = RNDM_HURL;
  else if (strcmp (name, "plug-in-randomize-pick") == 0)
    rndm_type = RNDM_PICK;
  else if (strcmp (name, "plug-in-randomize-slur") == 0)
    rndm_type = RNDM_SLUR;
  else if (strcmp (name, "plug-in-variable-hurl") == 0)
    rndm_type = VARIABLE_HURL;
  else if (strcmp (name, "plug-in-variable-pick") == 0)
    rndm_type = VARIABLE_PICK;
  else if (strcmp (name, "plug-in-variable-slur") == 0)
    rndm_type = VARIABLE_SLUR;

  run_mode = param[0].data.d_int32;

  pivals.drawable_id = param[2].data.d_drawable;

  drawable = gimp_drawable_get (pivals.drawable_id);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /* if we are running a "variable" version, we have 8 args */
  if (nparams == 8)
    pivals.use_map = TRUE;

  gr = g_rand_new ();
  /*
   *  Make sure the drawable type is appropriate.
   */
  if (gimp_drawable_is_rgb (drawable->drawable_id)  ||
      gimp_drawable_is_gray (drawable->drawable_id) ||
      gimp_drawable_is_indexed (drawable->drawable_id))
    {
      gimp_tile_cache_ntiles (4 * drawable->ntile_cols);

      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  gimp_get_data (PLUG_IN_PROC[rndm_type - 1], &pivals);

	  if (! randomize_dialog (drawable)) /* return on Cancel */
	    return;
	  break;
	case GIMP_RUN_NONINTERACTIVE:
          pivals.rndm_pct    = (gdouble) param[3].data.d_float;
          pivals.rndm_rcount = (gdouble) param[4].data.d_float;
          pivals.randomize   = (gboolean) param[5].data.d_int32;
          pivals.seed        = (gint) param[6].data.d_int32;

          /* if we are running a "variable" version, we have 8 args */
          if (nparams == 8)
            {
              pivals.map_id = param[7].data.d_drawable;
              pivals.use_map = TRUE;
              pivals.map_is_rgb = gimp_drawable_is_rgb (pivals.map_id);
            }

          if (pivals.randomize)
            pivals.seed = g_random_int ();

          if ((rndm_type != RNDM_PICK &&
               rndm_type != RNDM_SLUR &&
               rndm_type != RNDM_HURL) ||
		  (pivals.rndm_pct < 1.0 || pivals.rndm_pct > 100.0) ||
              (pivals.rndm_rcount < 1.0 || pivals.rndm_rcount > 100.0))
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
	  break;
	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_get_data (PLUG_IN_PROC[rndm_type - 1], &pivals);

          if (pivals.randomize)
            pivals.seed = g_random_int ();
	  break;
	default:
	  break;
        }

      if (status == GIMP_PDB_SUCCESS)
	{
          gimp_progress_init_printf ("%s", gettext (RNDM_NAME[rndm_type - 1]));

          g_rand_set_seed (gr, pivals.seed);

	  randomize (drawable, NULL);

	  if (run_mode != GIMP_RUN_NONINTERACTIVE)
	    {
	      gimp_displays_flush ();
            }

	  if (run_mode == GIMP_RUN_INTERACTIVE)
	    {
	      gimp_set_data (PLUG_IN_PROC[rndm_type - 1], &pivals,
                             sizeof (RandomizeVals));
            }
        }
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  g_rand_free (gr);
  values[0].data.d_status = status;
  gimp_drawable_detach(drawable);
}

/*********************************
 *
 *  randomize_prepare_row()
 *
 *  Get a row of pixels.  If the requested row
 *  is off the edge, clone the edge row.
 *
 ********************************/

static inline void
randomize_prepare_row (GimpPixelRgn *pixel_rgn,
		       guchar    *data,
		       int        x,
		       int        y,
		       int        w)
{
  gint b;

  if (y <= 0)
    {
      gimp_pixel_rgn_get_row(pixel_rgn, data, x, (y + 1), w);
    }
  else if (y >= pixel_rgn->h)
    {
      gimp_pixel_rgn_get_row(pixel_rgn, data, x, (y - 1), w);
    }
  else
    {
      gimp_pixel_rgn_get_row(pixel_rgn, data, x, y, w);
    }
  /*
   *  Fill in edge pixels
   */
  for (b = 0; b < pixel_rgn->bpp; b++)
    {
      data[-(gint)pixel_rgn->bpp + b] = data[b];
      data[w * pixel_rgn->bpp + b] = data[(w - 1) * pixel_rgn->bpp + b];
    }
}

/*********************************
 *
 *  randomize()
 *
 *  Actually mess with the image.
 *
 ********************************/

static void
randomize (GimpDrawable *drawable,
           GimpPreview  *preview)
{
  GimpDrawable *map_drawable      = NULL;
  GimpPixelRgn  srcPR, destPR;
  GimpPixelRgn  destPR2, mapPR;
  GimpPixelRgn *sp, *dp, *tp, *mp;
  gint          width, height;
  gint          bytes;
  gint		map_bytes         = 0;
  guchar       *dest, *d;
  guchar       *prev_row, *pr;
  guchar       *cur_row, *cr;
  guchar       *next_row, *nr;
  guchar       *map_row           = NULL;
  guchar       *mr                = NULL;
  guchar       *tmp;
  gint          row, col;
  gint          x1, y1, x2, y2;
  gint          cnt;
  gint          has_alpha, ind;
  gint          i, j, k;

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);
      x2 = x1 + width;
      y2 = y1 + height;
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
      width     = (x2 - x1);
      height    = (y2 - y1);
    }

  bytes = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  if (pivals.use_map)
    {
      map_drawable = gimp_drawable_get (pivals.map_id);
      pivals.map_is_rgb = gimp_drawable_is_rgb (pivals.map_id);
      if (! map_drawable)
        return;
      map_bytes = map_drawable->bpp;
    }

  /*
   *  allocate row buffers
   */
  prev_row = g_new (guchar, (x2 - x1 + 2) * bytes);
  cur_row = g_new (guchar, (x2 - x1 + 2) * bytes);
  next_row = g_new (guchar, (x2 - x1 + 2) * bytes);
  dest = g_new (guchar, (x2 - x1) * bytes);
  if (pivals.use_map)
    map_row  = g_new (guchar, (x2 - x1 + 2) * map_bytes);

  /*
   *  initialize the pixel regions
   */
  gimp_pixel_rgn_init (&srcPR,   drawable, x1, y1, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR,  drawable, x1, y1, width, height, TRUE,  TRUE);
  gimp_pixel_rgn_init (&destPR2, drawable, x1, y1, width, height, TRUE,  TRUE);
  if (pivals.use_map)
    gimp_pixel_rgn_init (&mapPR,   map_drawable, x1, y1, width, height,
                         FALSE, FALSE);


  sp = &srcPR;
  dp = &destPR;
  tp = NULL;
  if (pivals.use_map)
    mp = &mapPR;

  pr = prev_row + bytes;
  cr = cur_row + bytes;
  nr = next_row + bytes;
  if (pivals.use_map)
    mr = map_row + map_bytes;

  for (cnt = 1; cnt <= pivals.rndm_rcount; cnt++)
    {
      /*
       *  prepare the first row and previous row
       */
      randomize_prepare_row (sp, pr, x1, y1 - 1, (x2 - x1));
      randomize_prepare_row (dp, cr, x1, y1, (x2 - x1));
      /*
       *  loop through the rows, applying the selected convolution
       */
      for (row = y1; row < y2; row++)
	{
	  /*  prepare the next row  */
	  randomize_prepare_row (sp, nr, x1, row + 1, (x2 - x1));

          if (pivals.use_map)
            randomize_prepare_row (mp, mr, x1, row, (x2 - x1));

	  d = dest;
	  ind = 0;
	  for (col = 0; col < (x2 - x1); col++)
	    {
              gint pct = pivals.rndm_pct;

              if (pivals.use_map)
                {
		  gint value;
                  gint c      = col * map_bytes;

		  if (pivals.map_is_rgb)
		    value = ((gint) mr[c] + (gint) mr[c + 1] + (gint) mr[c + 2]) / 3;
		  else
		    value = mr[c];

		  pct *= value / 255.0;
		}

	      if (g_rand_int_range (gr, 0, 100) < pct)
		{
		  switch (rndm_type)
		    {
		      /*
		       *  HURL
		       *      Just assign a random value.
		       */
		    case RNDM_HURL:
                    case VARIABLE_HURL:
                      for (j = 0; j < bytes; j++)
                        *d++ = g_rand_int_range (gr, 0, 256);
                      break;
		      /*
		       *  PICK
		       *      pick at random from a neighboring pixel.
		       */
		    case RNDM_PICK:
                    case VARIABLE_PICK:
                      k = g_rand_int_range (gr, 0, 9);
                      for (j = 0; j < bytes; j++)
                        {
                          i = col * bytes + j;

                          switch (k)
                            {
                            case 0:
                              *d++ = (gint) pr[i - bytes];
                              break;
                            case 1:
                              *d++ = (gint) pr[i];
                              break;
                            case 2:
                              *d++ = (gint) pr[i + bytes];
                              break;
                            case 3:
                              *d++ = (gint) cr[i - bytes];
                              break;
                            case 4:
                              *d++ = (gint) cr[i];
                              break;
                            case 5:
                              *d++ = (gint) cr[i + bytes];
                              break;
                            case 6:
                              *d++ = (gint) nr[i - bytes];
                              break;
                            case 7:
                              *d++ = (gint) nr[i];
                              break;
                            case 8:
                              *d++ = (gint) nr[i + bytes];
                              break;
                            }
                        }
                      break;
                      /*
		       *  SLUR
		       *      80% chance it's from directly above,
		       *      10% from above left,
		       *      10% from above right.
		       */
		    case RNDM_SLUR:
                    case VARIABLE_SLUR:
                      k = g_rand_int_range (gr, 0, 10);
                      for (j = 0; j < bytes; j++)
                        {
                          i = col*bytes + j;

                          switch (k )
                            {
                            case 0:
                              *d++ = (gint) pr[i - bytes];
                              break;
                            case 9:
                              *d++ = (gint) pr[i + bytes];
                              break;
                            default:
                              *d++ = (gint) pr[i];
                              break;
                            }
                        }
                      break;
                    }
                  /*
                   *  Otherwise, this pixel was not selected for randomization,
		   *  so use the current value.
		   */
                }
	      else
		{
                  for (j = 0; j < bytes; j++)
                    *d++ = (gint) cr[col * bytes + j];
                }
            }
	  /*
	   *  Save the modified row, shuffle the row pointers, and every
	   *  so often, update the progress meter.
	   */
	  gimp_pixel_rgn_set_row (dp, dest, x1, row, (x2 - x1));

	  tmp = pr;
	  pr = cr;
	  cr = nr;
	  nr = tmp;

          if (! preview && PROG_UPDATE_TIME)
            {
              gdouble base = (gdouble) cnt / pivals.rndm_rcount;
              gdouble inc  = (gdouble) row / ((y2 - y1) * pivals.rndm_rcount);

              gimp_progress_update (base + inc);
            }
        }

      /*
       *  if we have more cycles to perform, swap the src and dest Pixel Regions
       */
      if (cnt < pivals.rndm_rcount)
	{
	  if (tp != NULL)
	    {
	      tp = dp;
	      dp = sp;
	      sp = tp;
            }
	  else
	    {
	      tp = &srcPR;
	      sp = &destPR;
	      dp = &destPR2;
            }
        }
    }

  if (! preview)
    gimp_progress_update (1.0);

  /*
   *  update the randomized region
   */
  if (preview)
    {
      gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                         dp);
    }
  else
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
    }

  /*
   *  clean up after ourselves.
   */
  g_free (prev_row);
  g_free (cur_row);
  g_free (next_row);
  g_free (dest);
  if (pivals.use_map)
    g_free (map_row);
}

/*********************************
 *
 *  GUI ROUTINES
 *
 ********************************/


/*********************************
 *
 *  randomize_dialog() - set up the plug-in's dialog box
 *
 ********************************/

static gboolean
randomize_dialog (GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *table;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkObject *adj;
  gboolean   run;
  GtkWidget *combo;
  GtkWidget *toggle;
  GtkWidget *hbox;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  /*
   * sanity check
   */
  pivals.use_map = TRUE;
  if (pivals.map_id == -1)
    pivals.map_id = pivals.drawable_id;

  dlg = gimp_dialog_new (gettext (RNDM_NAME[rndm_type - 1]), PLUG_IN_BINARY,
                         NULL, 0,
			 gimp_standard_help_func, PLUG_IN_PROC[rndm_type - 1],

			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,

			 NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new (drawable, NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (randomize),
                            drawable);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, TRUE, TRUE, 0);
  gtk_widget_show(table);

  /*
   *  Randomization percentage label & scale (1 to 100)
   */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("R_andomization (%):"), SCALE_WIDTH, 0,
			      pivals.rndm_pct, 1.0, 100.0, 1.0, 10.0, 0,
			      TRUE, 0, 0,
			      _("Percentage of pixels to be filtered"), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pivals.rndm_pct);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*
   *  Repeat count label & scale (1 to 100).  Don't show this for Hurl,
   *  since repeating is equivalent to changing the percentage.
   */
  if (rndm_type != VARIABLE_HURL)
    {
      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                                  _("R_epeat:"), SCALE_WIDTH, 0,
                                  pivals.rndm_rcount, 1.0, 100.0, 1.0, 10.0, 0,
                                  TRUE, 0, 0,
                                  _("Number of times to apply filter"), NULL);
      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (gimp_double_adjustment_update),
                        &pivals.rndm_rcount);
      g_signal_connect_swapped (adj, "value-changed",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);
    }

  /*
   * drawable menu for blur intensity map
   */
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  toggle = gtk_check_button_new_with_label (_("Use intensity map:"));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 5);
  gtk_widget_show (toggle);
  gimp_help_set_help_data
    (toggle, _("Image that determines the intensity of blurring at each location"),
     PLUG_IN_PROC[rndm_type - 1]);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), pivals.use_map);
  g_signal_connect (toggle, "toggled",
                    (GCallback) dialog_usemap_callback, preview);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  combo = gimp_drawable_combo_box_new (dialog_constrain,
                                       GINT_TO_POINTER (pivals.drawable_id));
  map_drawable_combo = combo;
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              pivals.map_id,
                              (GCallback) dialog_setmap_callback,
                              preview);
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 10);
  gtk_widget_show (combo);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}


/* The intensity map can be rgb or grayscale, must have the same shape as the base image */
static gboolean
dialog_constrain (gint32   image_id,
                  gint32   drawable_id,
                  gpointer data)
{
  gint32 src_drawable_id = GPOINTER_TO_INT (data);

  if (drawable_id == -1) /* checking an image, not a drawable */
    return FALSE;

  if(gimp_drawable_width (drawable_id) == gimp_drawable_width (src_drawable_id)
     && gimp_drawable_height (drawable_id) == gimp_drawable_height (src_drawable_id)
     && (gimp_drawable_is_rgb (drawable_id) || gimp_drawable_is_gray (drawable_id)) )
    return TRUE;
  else
    return FALSE;
}

/*
 * called if the user toggles the "use intensity map" button.
 */
static void
dialog_usemap_callback (GtkToggleButton *toggle,
                        GimpPreview     *preview)
{
  gint active = gtk_toggle_button_get_active (toggle);

  if (active != pivals.use_map)
    {
      pivals.use_map = active;

      if (active)
        gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (map_drawable_combo),
                                       &pivals.map_id);

      gtk_widget_set_sensitive (map_drawable_combo, active);

      gimp_preview_invalidate (preview);
    }
}

/*
 * called if the user changes the map drawable.
 */
static void
dialog_setmap_callback (GimpIntComboBox *combo,
                        GimpPreview     *preview)
{
  gint id;

  if (! gimp_int_combo_box_get_active (combo, &id))
    {
      id = pivals.drawable_id;
      gimp_int_combo_box_set_active (combo, id);
    }

  pivals.map_id = id;
  pivals.map_is_rgb = gimp_drawable_is_rgb (id);

  gimp_preview_invalidate (preview);
}

