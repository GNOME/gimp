/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * newsprint plug-in
 * Copyright (C) 1997-1998 Austin Donnelly <austin@gimp.org>
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
 * version 0.60
 *
 * This plug-in puts an image through a screen at a particular angle
 * and lines per inch, to arrive at a newspaper-style effect.
 *
 * Austin Donnelly <austin@greenend.org.uk>
 * http://www.cl.cam.ac.uk/~and1000/newsprint/
 *
 * Richard Mortier <rmm1002@cam.ac.uk> did the spot_round() function
 * with correct tonal balance.
 *
 * Tim Harris <tim.harris@acm.org> provided valuable feedback on
 * pre-press issues.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define VERSION "v0.60"

/* Some useful macros */
#ifdef DEBUG
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x)
#endif

/*#define TIMINGS*/

#define PLUG_IN_PROC       "plug-in-newsprint"
#define PLUG_IN_BINARY     "newsprint"
#define PLUG_IN_ROLE       "gimp-newsprint"

#define TILE_CACHE_SIZE     16
#define SCALE_WIDTH        125
#define DEF_OVERSAMPLE       1 /* default for how much to oversample by */
#define SPOT_PREVIEW_SZ     16
#define PREVIEW_SIZE        (2 * SPOT_PREVIEW_SZ + 1)
#define PREVIEW_OVERSAMPLE   3


#define ISNEG(x)        (((x) < 0)? 1 : 0)
#define DEG2RAD(d)      ((d) * G_PI / 180)
#define CLAMPED_ADD(a, b) (((a)+(b) > 0xff)? 0xff : (a) + (b))


/* Bartlett window supersampling weight function.  See table 4.1, page
 * 123 of Alan Watt and Mark Watt, Advanced Animation and Rendering
 * Techniques, theory and practice. Addison-Wesley, 1992. ISBN
 * 0-201-54412-1 */
#define BARTLETT(x,y)   (((oversample/2)+1-ABS(x)) * ((oversample/2)+1-ABS(y)))
#define WGT(x,y)        wgt[((y+oversample/2)*oversample) + x+oversample/2]

/* colorspaces we can separate to: */
#define CS_GREY         0
#define CS_RGB          1
#define CS_CMYK         2
#define CS_LUMINANCE    3
#define NUM_CS          4
#define VALID_CS(x)     ((x) >= 0 && (x) <= NUM_CS-1)

/* Spot function related types and definitions */

typedef gdouble spotfn_t (gdouble x, gdouble y);

/* forward declaration of the functions themselves */
static spotfn_t spot_round;
static spotfn_t spot_line;
static spotfn_t spot_diamond;
static spotfn_t spot_PSsquare;
static spotfn_t spot_PSdiamond;

typedef struct
{
  const gchar *name;        /* function's name */
  spotfn_t    *fn;          /* function itself */
  guchar      *thresh;      /* cached threshold matrix */
  gdouble      prev_lvl[3]; /* intensities to preview */
  guchar      *prev_thresh; /* preview-sized threshold matrix */
  gint         balanced;    /* TRUE if spot fn is already balanced */
} spot_info_t;


/* This is all the info needed per spot function.  Functions are referred to
 * by their index into this array. */
static spot_info_t spotfn_list[] =
{
#define SPOTFN_DOT 0
  {
    N_("Round"),
    spot_round,
    NULL,
    { .22, .50, .90 },
    NULL,
    FALSE
  },

  {
    N_("Line"),
    spot_line,
    NULL,
    { .15, .50, .80 },
    NULL,
    FALSE
  },

  {
    N_("Diamond"),
    spot_diamond,
    NULL,
    { .15, .50, .80 },
    NULL,
    TRUE
  },

  { N_("PS Square (Euclidean Dot)"),
    spot_PSsquare,
    NULL,
    { .15, .50, .90 },
    NULL,
    FALSE
  },

  {
    N_("PS Diamond"),
    spot_PSdiamond,
    NULL,
    { .15, .50, .90 },
    NULL,
    FALSE
  },

    /* NULL-name terminates */
  { NULL,
    NULL,
    NULL,
    { 0.0, 0.0, 0.0 },
    NULL,
    FALSE
  }
};

#define NUM_SPOTFN      (G_N_ELEMENTS (spotfn_list))
#define VALID_SPOTFN(x) ((x) >= 0 && (x) < NUM_SPOTFN)
#define THRESH(x,y)     (thresh[(y)*width + (x)])
#define THRESHn(n,x,y)  ((thresh[n])[(y)*width + (x)])


/* Arguments to filter */

/* Some of these are here merely to save them across calls.  They are
 * marked as "UI use".  Everything else is a valid arg. */
typedef struct
{
  /* resolution section: */
  gint    cell_width;

  /* screening section: */
  gint    colorspace;  /* 0: RGB, 1: CMYK, 2: Luminance */
  gint    k_pullout;    /* percentage of black to pull out */

  /* grey screen (only used if grayscale drawable) */
  gdouble gry_ang;
  gint    gry_spotfn;

  /* red / cyan screen */
  gdouble red_ang;
  gint    red_spotfn;

  /* green / magenta screen */
  gdouble grn_ang;
  gint    grn_spotfn;

  /* blue / yellow screen */
  gdouble blu_ang;
  gint    blu_spotfn;

  /* anti-alias section */
  gint    oversample;   /* 1 == no anti-aliasing, else small odd int */
} NewsprintValues;

/* bits of state used by the UI, but not visible from the PDB */
typedef struct
{
  gdouble  input_spi;     /* input samples per inch */
  gdouble  output_lpi;    /* desired output lines per inch */
  gboolean lock_channels; /* changes to one channel affect all */
} NewsprintUIValues;


/* state for the preview widgets */
typedef struct
{
  GtkWidget *widget;    /* preview widget itself */
  GtkWidget *label;     /* the label below it */
} preview_st;

/* state for the channel notebook pages */
typedef struct _channel_st channel_st;

struct _channel_st
{
  GtkWidget     *vbox;             /* vbox of this channel */
  gint          *spotfn_num;       /* which spotfn the menu is controlling */
  preview_st     prev[3];          /* state for 3 preview widgets */
  GtkAdjustment *angle_adj;        /* angle adjustment */
  GtkWidget     *combo;            /* popup for spot function */
  GtkWidget     *menuitem[NUM_SPOTFN]; /* menuitems for each spot function */
  GtkWidget     *ch_menuitem;      /* menuitem for the channel selector */
  gint           ch_menu_num;      /* this channel's position in the selector */
  channel_st    *next;             /* circular list of channels in locked group */
};


/* State associated with the configuration dialog and passed to its
 * callback functions */
typedef struct
{
  GtkWidget     *pull_table;
  GtkAdjustment *pull;            /* black pullout percentage */
  GtkAdjustment *input_spi;
  GtkAdjustment *output_lpi;
  GtkAdjustment *cellsize;
  GtkWidget     *vbox;            /* container for screen info */

  /* Notebook for the channels (one per colorspace) */
  GtkWidget     *channel_notebook[NUM_CS];

  /* room for up to 4 channels per colourspace */
  channel_st    *chst[NUM_CS][4];
} NewsprintDialog_st;


/***** Local vars *****/

/* defaults */
/* fixed copy so we can reset them */
static const NewsprintValues factory_defaults =
{
  /* resolution stuff */
  10,          /* cell width */

  /* screen setup (default is the classic rosette pattern) */
  CS_RGB,      /* use RGB, not CMYK or Luminance */
  100, /* max pullout */

  /* grey/black */
  45.0,
  SPOTFN_DOT,

  /* red/cyan */
  15.0,
  SPOTFN_DOT,

  /* green/magenta */
  75.0,
  SPOTFN_DOT,

  /* blue/yellow */
  0.0,
  SPOTFN_DOT,

  /* anti-alias control */
  DEF_OVERSAMPLE
};

static const NewsprintUIValues factory_defaults_ui =
{
  72,    /* input spi     */
  7.2,   /* output lpi    */
  FALSE  /* lock channels */
};

/* Mutable copy for normal use.  Initialised in run(). */
static NewsprintValues   pvals;
static NewsprintUIValues pvals_ui;


/* channel templates */
typedef struct
{
  const gchar   *name;
  /* pointers to the variables this channel updates */
  gdouble       *angle;
  gint          *spotfn;
  /* factory defaults for the angle and spot function (as pointers so
   * the silly compiler can see they're really constants) */
  const gdouble *factory_angle;
  const gint    *factory_spotfn;
} chan_tmpl;

static const chan_tmpl grey_tmpl[] =
{
  {
    N_("_Grey"),
    &pvals.gry_ang,
    &pvals.gry_spotfn,
    &factory_defaults.gry_ang,
    &factory_defaults.gry_spotfn
  },

  { NULL, NULL, NULL, NULL, NULL }
};

static const chan_tmpl rgb_tmpl[] =
{
  {
    N_("R_ed"),
    &pvals.red_ang,
    &pvals.red_spotfn,
    &factory_defaults.red_ang,
    &factory_defaults.red_spotfn
  },

  {
    N_("_Green"),
    &pvals.grn_ang,
    &pvals.grn_spotfn,
    &factory_defaults.grn_ang,
    &factory_defaults.grn_spotfn
  },

  {
    N_("_Blue"),
    &pvals.blu_ang,
    &pvals.blu_spotfn,
    &factory_defaults.blu_ang,
    &factory_defaults.blu_spotfn
  },

  { NULL, NULL, NULL, NULL, NULL }
};

static const chan_tmpl cmyk_tmpl[] =
{
  {
    N_("C_yan"),
    &pvals.red_ang,
    &pvals.red_spotfn,
    &factory_defaults.red_ang,
    &factory_defaults.red_spotfn
  },

  {
    N_("Magen_ta"),
    &pvals.grn_ang,
    &pvals.grn_spotfn,
    &factory_defaults.grn_ang,
    &factory_defaults.grn_spotfn
  },

  {
    N_("_Yellow"),
    &pvals.blu_ang,
    &pvals.blu_spotfn,
    &factory_defaults.blu_ang,
    &factory_defaults.blu_spotfn
  },

  {
    N_("_Black"),
    &pvals.gry_ang,
    &pvals.gry_spotfn,
    &factory_defaults.gry_ang,
    &factory_defaults.gry_spotfn
  },

  { NULL, NULL, NULL, NULL, NULL }
};

static const chan_tmpl luminance_tmpl[] =
{
  {
    N_("Luminance"),
    &pvals.gry_ang,
    &pvals.gry_spotfn,
    &factory_defaults.gry_ang,
    &factory_defaults.gry_spotfn
  },

  { NULL, NULL, NULL, NULL, NULL }
};

/* cspace_chan_tmpl is indexed by colorspace, and gives an array of
 * channel templates for that colorspace */
static const chan_tmpl *cspace_chan_tmpl[] =
{
  grey_tmpl,
  rgb_tmpl,
  cmyk_tmpl,
  luminance_tmpl
};

#define NCHANS(x) ((sizeof(x) / sizeof(chan_tmpl)) - 1)

/* cspace_nchans gives a quick way of finding the number of channels
 * in a colorspace.  Alternatively, if you're walking the channel
 * template, you can use the NULL entry at the end to stop. */
static const gint cspace_nchans[] =
{
  NCHANS (grey_tmpl),
  NCHANS (rgb_tmpl),
  NCHANS (cmyk_tmpl),
  NCHANS (luminance_tmpl)
};


/* Declare local functions.  */
static void     query   (void);
static void     run     (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static gboolean newsprint_dialog            (GimpDrawable  *drawable);
static void     newsprint_cspace_update     (GtkWidget     *widget,
                                             gpointer       data);

static void     newsprint_menu_callback     (GtkWidget     *widget,
                                             gpointer       data);
static void     angle_callback              (GtkAdjustment *adjustment,
                                             gpointer       data);
static void     lpi_callback                (GtkAdjustment *adjustment,
                                             gpointer       data);
static void     spi_callback                (GtkAdjustment *adjustment,
                                             gpointer       data);
static void     cellsize_callback           (GtkAdjustment *adjustment,
                                             gpointer       data);
static void     newsprint_defaults_callback (GtkWidget     *widget,
                                             gpointer       data);

static void     newsprint                   (GimpDrawable  *drawable,
                                             GimpPreview   *preview);
static guchar * spot2thresh                 (gint           type,
                                             gint           width);

static void     preview_update              (channel_st    *st);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};


/***** Functions *****/

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[]=
  {
    { GIMP_PDB_INT32,    "run-mode",   "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",      "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",   "Input drawable" },

    { GIMP_PDB_INT32,    "cell-width", "Screen cell width in pixels" },

    { GIMP_PDB_INT32,    "colorspace", "Separate to { GRAYSCALE (0), RGB (1), CMYK (2), LUMINANCE (3) }" },
    { GIMP_PDB_INT32,    "k-pullout",  "Percentage of black to pullout (CMYK only)" },

    { GIMP_PDB_FLOAT,    "gry-ang",    "Grey/black screen angle (degrees)" },
    { GIMP_PDB_INT32,    "gry-spotfn", "Grey/black spot function { DOTS (0), LINES (1), DIAMONDS (2), EUCLIDIAN-DOT (3), PS-DIAMONDS (4) }" },
    { GIMP_PDB_FLOAT,    "red-ang",    "Red/cyan screen angle (degrees)" },
    { GIMP_PDB_INT32,    "red-spotfn", "Red/cyan spot function (values as gry-spotfn)" },
    { GIMP_PDB_FLOAT,    "grn-ang",    "Green/magenta screen angle (degrees)" },
    { GIMP_PDB_INT32,    "grn-spotfn", "Green/magenta spot function (values as gry-spotfn)" },
    { GIMP_PDB_FLOAT,    "blu-ang",    "Blue/yellow screen angle (degrees)" },
    { GIMP_PDB_INT32,    "blu-spotfn", "Blue/yellow spot function (values as gry-spotfn)" },

    { GIMP_PDB_INT32,    "oversample", "how many times to oversample spot fn" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Halftone the image to give newspaper-like effect"),
                          "Halftone the image, trading off resolution to "
                          "represent colors or grey levels using the process "
                          "described both in the PostScript language "
                          "definition, and also by Robert Ulichney, \"Digital "
                          "halftoning\", MIT Press, 1987.",
                          "Austin Donnelly",
                          "Austin Donnelly",
                          "1998 (" VERSION ")",
                          N_("Newsprin_t..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Distorts");
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
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /* basic defaults */
  pvals    = factory_defaults;
  pvals_ui = factory_defaults_ui;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &pvals);
      gimp_get_data (PLUG_IN_PROC "-ui", &pvals_ui);

      /*  First acquire information with a dialog  */
      if (! newsprint_dialog (drawable))
        {
          gimp_drawable_detach (drawable);
          return;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 15)
        {
          status = GIMP_PDB_CALLING_ERROR;
          break;
        }

      pvals.cell_width  = param[3].data.d_int32;
      pvals.colorspace = param[4].data.d_int32;
      pvals.k_pullout   = param[5].data.d_int32;
      pvals.gry_ang     = param[6].data.d_float;
      pvals.gry_spotfn  = param[7].data.d_int32;
      pvals.red_ang     = param[8].data.d_float;
      pvals.red_spotfn  = param[9].data.d_int32;
      pvals.grn_ang     = param[10].data.d_float;
      pvals.grn_spotfn  = param[11].data.d_int32;
      pvals.blu_ang     = param[12].data.d_float;
      pvals.blu_spotfn  = param[13].data.d_int32;
      pvals.oversample  = param[14].data.d_int32;

      /* check values are within permitted ranges */
      if (!VALID_SPOTFN (pvals.gry_spotfn) ||
          !VALID_SPOTFN (pvals.red_spotfn) ||
          !VALID_SPOTFN (pvals.grn_spotfn) ||
          !VALID_SPOTFN (pvals.blu_spotfn) ||
          !VALID_CS (pvals.colorspace) ||
          pvals.k_pullout < 0 || pvals.k_pullout > 100)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &pvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
          gimp_drawable_is_gray (drawable->drawable_id))
        {
          gimp_progress_init (_("Newsprint"));

          /*  set the tile cache size  */
          gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

          /*  run the newsprint effect  */
          newsprint (drawable, NULL);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            {
              gimp_set_data (PLUG_IN_PROC,
                             &pvals, sizeof (NewsprintValues));
              gimp_set_data (PLUG_IN_PROC "-ui",
                             &pvals_ui, sizeof (NewsprintUIValues));
            }
        }
      else
        {
          /*gimp_message ("newsprint: cannot operate on indexed images");*/
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


/* create new menu state, and the preview widgets for it */
static channel_st *
new_preview (gint *sfn)
{
  channel_st *st;
  GtkWidget  *preview;
  GtkWidget  *label;
  gint        i;

  st = g_new (channel_st, 1);

  st->spotfn_num = sfn;

  /* make the preview widgets */
  for (i = 0; i < 3; i++)
    {
      preview = gimp_preview_area_new ();
      gtk_widget_set_size_request (preview,
                                   PREVIEW_SIZE, PREVIEW_SIZE);
      gtk_widget_show (preview);
      g_signal_connect_swapped (preview, "size-allocate",
                                G_CALLBACK (preview_update), st);

      label = gtk_label_new ("");
      gtk_widget_show (label);

      st->prev[i].widget = preview;
      st->prev[i].label  = label;
      /* st->prev[i].value changed by preview_update */
    }

  return st;
}


/* the popup menu "st" has changed, so the previews associated with it
 * need re-calculation */
static void
preview_update (channel_st *st)
{
  gint        sfn = *(st->spotfn_num);
  preview_st *prev;
  gint        i;
  gint        x;
  gint        y;
  gint        width = SPOT_PREVIEW_SZ * PREVIEW_OVERSAMPLE;
  gint        oversample = PREVIEW_OVERSAMPLE;
  guchar     *thresh;
  gchar       pct[12];
  guchar      value;
  guchar      rgb[3 * PREVIEW_SIZE * PREVIEW_SIZE ];
  /* If we don't yet have a preview threshold matrix for this spot
   * function, generate one now. */
  if (!spotfn_list[sfn].prev_thresh)
    {
      spotfn_list[sfn].prev_thresh =
        spot2thresh (sfn, SPOT_PREVIEW_SZ * PREVIEW_OVERSAMPLE);
    }

  thresh = spotfn_list[sfn].prev_thresh;

  for (i = 0; i < 3; i++)
    {
      prev = &st->prev[i];

      value = spotfn_list[sfn].prev_lvl[i] * 0xff;

      for (y = 0; y < PREVIEW_SIZE ; y++)
        {
          for (x = 0; x < PREVIEW_SIZE ; x++)
            {
              guint32 sum = 0;
              gint sx, sy;
              gint tx, ty;
              gint rx, ry;

              rx = x * PREVIEW_OVERSAMPLE;
              ry = y * PREVIEW_OVERSAMPLE;

              for (sy = -oversample/2; sy <= oversample/2; sy++)
                for (sx = -oversample/2; sx <= oversample/2; sx++)
                  {
                    tx = rx+sx;
                    ty = ry+sy;
                    while (tx < 0)  tx += width;
                    while (ty < 0)  ty += width;
                    while (tx >= width)  tx -= width;
                    while (ty >= width)  ty -= width;

                    if (value > THRESH (tx, ty))
                      sum += 0xff * BARTLETT (sx, sy);
                  }
              sum /= BARTLETT (0, 0) * BARTLETT (0, 0);

              /* blue lines on cell boundaries */
              if ((x % SPOT_PREVIEW_SZ) == 0 ||
                  (y % SPOT_PREVIEW_SZ) == 0)
                {
                  rgb[(y*PREVIEW_SIZE+x)*3+0] = 0;
                  rgb[(y*PREVIEW_SIZE+x)*3+1] = 0;
                  rgb[(y*PREVIEW_SIZE+x)*3+2] = 0xff;
                }
              else
                {
                  rgb[(y*PREVIEW_SIZE+x)*3+0] = sum;
                  rgb[(y*PREVIEW_SIZE+x)*3+1] = sum;
                  rgb[(y*PREVIEW_SIZE+x)*3+2] = sum;
                }
            }
        }

      /* redraw preview widget */
      gimp_preview_area_draw (GIMP_PREVIEW_AREA (prev->widget),
                              0, 0, PREVIEW_SIZE, PREVIEW_SIZE,
                              GIMP_RGB_IMAGE,
                              rgb,
                              PREVIEW_SIZE * 3);

      g_snprintf (pct, sizeof (pct), "%2d%%",
                  (int) RINT (spotfn_list[sfn].prev_lvl[i] * 100));
      gtk_label_set_text (GTK_LABEL(prev->label), pct);
    }
}


static void
newsprint_menu_callback (GtkWidget *widget,
                         gpointer   data)
{
  channel_st      *st = data;
  gint             value;
  static gboolean  in_progress = FALSE;

  /* We shouldn't need recursion protection, but if lock_channels is
   * set, and gimp_int_combo_box_set_active ever generates a "changed"
   * signal, then we'll get back here.
   */
  if (in_progress)
    {
      printf ("newsprint_menu_callback: unexpected recursion: can't happen\n");
      return;
    }

  in_progress = TRUE;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value);

  *(st->spotfn_num) = value;

  preview_update (st);

  /* ripple the change to the other popups if the channels are
   * locked together. */
  if (pvals_ui.lock_channels)
    {
      channel_st *c = st->next;
      gint        old_value;

      while (c != st)
        {
          gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (c->combo), value);

          old_value = *(c->spotfn_num);
          *(c->spotfn_num) = value;
          if (old_value != value)
            preview_update (c);
          c = c->next;
        }
    }

  in_progress = FALSE;
}


static void
angle_callback (GtkAdjustment *adjustment,
                gpointer       data)
{
  channel_st *st = data;
  channel_st *c;
  gdouble    *angle_ptr;
  static gint in_progress = FALSE;

  angle_ptr = g_object_get_data (G_OBJECT (adjustment), "angle");

  gimp_double_adjustment_update (adjustment, angle_ptr);

  if (pvals_ui.lock_channels && !in_progress)
    {
      in_progress = TRUE;

      c = st->next;

      while (c != st)
        {
          gtk_adjustment_set_value (c->angle_adj, *angle_ptr);
          c = c->next;
        }

      in_progress = FALSE;
    }
}

static void
lpi_callback (GtkAdjustment *adjustment,
              gpointer       data)
{
  NewsprintDialog_st *st = data;

  gimp_double_adjustment_update (adjustment, &pvals_ui.output_lpi);

  g_signal_handlers_block_by_func (st->cellsize,
                                   cellsize_callback,
                                   data);

  gtk_adjustment_set_value (st->cellsize,
                            pvals_ui.input_spi / pvals_ui.output_lpi);

  g_signal_handlers_unblock_by_func (st->cellsize,
                                     cellsize_callback,
                                     data);
}

static void
spi_callback (GtkAdjustment *adjustment,
              gpointer       data)
{
  NewsprintDialog_st *st = data;

  gimp_double_adjustment_update (adjustment, &pvals_ui.input_spi);

  g_signal_handlers_block_by_func (st->output_lpi,
                                   lpi_callback,
                                   data);

  gtk_adjustment_set_value (st->output_lpi,
                            pvals_ui.input_spi / pvals.cell_width);

  g_signal_handlers_unblock_by_func (st->output_lpi,
                                     lpi_callback,
                                     data);
}

static void
cellsize_callback (GtkAdjustment *adjustment,
                   gpointer       data)
{
  NewsprintDialog_st *st = data;

  gimp_int_adjustment_update (adjustment, &pvals.cell_width);

  g_signal_handlers_block_by_func (st->output_lpi,
                                   lpi_callback,
                                   data);

  gtk_adjustment_set_value (st->output_lpi,
                            pvals_ui.input_spi / pvals.cell_width);

  g_signal_handlers_unblock_by_func (st->output_lpi,
                                     lpi_callback,
                                     data);
}

static void
newsprint_defaults_callback (GtkWidget *widget,
                             gpointer   data)
{
  NewsprintDialog_st  *st = data;
  gint                 saved_lock;
  gint                 cspace;
  channel_st         **chst;
  const chan_tmpl     *ct;
  gint                 spotfn;
  gint                 c;

  /* temporarily turn off channel lock */
  saved_lock = pvals_ui.lock_channels;
  pvals_ui.lock_channels = FALSE;

  /* for each colorspace, reset its channel info */
  for (cspace = 0; cspace < NUM_CS; cspace++)
    {
      chst = st->chst[cspace];
      ct = cspace_chan_tmpl[cspace];

      /* skip this colorspace if we haven't used it yet */
      if (!chst[0])
        continue;

      c = 0;
      while (ct->name)
        {
          gtk_adjustment_set_value (chst[c]->angle_adj,
                                    *(ct->factory_angle));

          /* change the popup menu and also activate the menuitem in
           * question, in order to run the handler that re-computes
           * the preview area */
          spotfn = *(ct->factory_spotfn);
          gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (chst[c]->combo),
                                         spotfn);

          c++;
          ct++;
        }
    }

  pvals_ui.lock_channels = saved_lock;
}

/* Create (but don't yet show) a new vbox for a channel 'widget'.
 * Return the channel state, so caller can find the vbox and place it
 * in the notebook. */
static channel_st *
new_channel (const chan_tmpl *ct, GtkWidget *preview)
{
  GtkSizeGroup *group;
  GtkWidget    *table;
  GtkWidget    *hbox;
  GtkWidget    *hbox2;
  GtkWidget    *abox;
  GtkWidget    *label;
  spot_info_t  *sf;
  channel_st   *chst;
  gint          i;

  /* create the channel state record */
  chst = new_preview (ct->spotfn);

  chst->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (chst->vbox), 12);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (chst->vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* angle slider */
  chst->angle_adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                          _("_Angle:"), SCALE_WIDTH, 0,
                                          *ct->angle,
                                          -90, 90, 1, 15, 1,
                                          TRUE, 0, 0,
                                          NULL, NULL);
  g_object_set_data (G_OBJECT (chst->angle_adj), "angle", ct->angle);

  gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (chst->angle_adj));
  g_object_unref (group);

  g_signal_connect (chst->angle_adj, "value-changed",
                    G_CALLBACK (angle_callback),
                    chst);
  g_signal_connect_swapped (chst->angle_adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* spot function popup */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (chst->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  abox = gtk_alignment_new (0.5, 0.0, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_add (GTK_CONTAINER (abox), hbox2);
  gtk_widget_show (hbox2);

  label = gtk_label_new_with_mnemonic (_("_Spot function:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_size_group_add_widget (group, label);

  chst->combo = g_object_new (GIMP_TYPE_INT_COMBO_BOX, NULL);

  for (sf = spotfn_list, i = 0; sf->name; sf++, i++)
    gimp_int_combo_box_append (GIMP_INT_COMBO_BOX (chst->combo),
                               GIMP_INT_STORE_VALUE, i,
                               GIMP_INT_STORE_LABEL, gettext (sf->name),
                               -1);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (chst->combo),
                                 *ct->spotfn);

  g_signal_connect (chst->combo, "changed",
                    G_CALLBACK (newsprint_menu_callback),
                    chst);
  g_signal_connect_swapped (chst->combo, "changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_box_pack_start (GTK_BOX (hbox2), chst->combo, FALSE, FALSE, 0);
  gtk_widget_show (chst->combo);

  /* spot function previews */
  {
    GtkWidget *sub;

    sub = gtk_table_new (2, 3, FALSE);
    gtk_table_set_col_spacings (GTK_TABLE (sub), 6);
    gtk_table_set_row_spacings (GTK_TABLE (sub), 1);
    gtk_box_pack_start (GTK_BOX (hbox), sub, FALSE, FALSE, 0);

    for (i = 0; i < 3; i++)
      {
        gtk_table_attach (GTK_TABLE (sub), chst->prev[i].widget,
                          i, i+1, 0, 1,
                          GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);

        gtk_table_attach (GTK_TABLE (sub), chst->prev[i].label,
                          i, i+1, 1, 2,
                          GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
      }

    gtk_widget_show (sub);
  }

  /* fire the update once to make sure we start with something
   * in the preview windows */
  preview_update (chst);

  gtk_widget_show (table);

  /* create the menuitem used to select this channel for editing */
  chst->ch_menuitem = gtk_menu_item_new_with_label (gettext (ct->name));
  /* signal attachment and showing left to caller */

  /* deliberately don't show the chst->frame, leave that up to
   * the caller */

  return chst;
}


/* Make all the channels needed for "colorspace", and fill in
 * the respective channel state fields in "st". */
static void
gen_channels (NewsprintDialog_st *st,
              gint                colorspace,
              GtkWidget          *preview)
{
  const chan_tmpl  *ct;
  channel_st      **chst;
  channel_st       *base = NULL;
  gint              i;

  chst = st->chst[colorspace];
  ct   = cspace_chan_tmpl[colorspace];
  i    = 0;

  st->channel_notebook[colorspace] = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (st->vbox), st->channel_notebook[colorspace],
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (st->vbox),
                         st->channel_notebook[colorspace], 3);
  gtk_widget_show (st->channel_notebook[colorspace]);

  while (ct->name)
    {
      chst[i] = new_channel (ct, preview);

      if (i)
        chst[i-1]->next = chst[i];
      else
        base = chst[i];

      gtk_notebook_append_page (GTK_NOTEBOOK (st->channel_notebook[colorspace]),
                                chst[i]->vbox,
                                gtk_label_new_with_mnemonic (gettext (ct->name)));
      gtk_widget_show (chst[i]->vbox);

      i++;
      ct++;
    }

  /* make the list circular */
  chst[i-1]->next = base;
}


static gboolean
newsprint_dialog (GimpDrawable *drawable)
{
  /* widgets we need from callbacks stored here */
  NewsprintDialog_st st;
  GtkWidget     *dialog;
  GtkWidget     *paned;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *preview;
  GtkWidget     *frame;
  GtkWidget     *table;
  GtkAdjustment *adj;
  GSList        *group = NULL;
  gint           bpp;
  gint           i;
  gdouble        xres, yres;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  /* flag values to say we haven't filled these channel
   * states in yet */
  for(i=0; i<NUM_CS; i++)
    st.chst[i][0] = NULL;

  /* need to know the bpp, so we can tell if we're doing
   * RGB/CMYK or grey style of dialog box */
  bpp = gimp_drawable_bpp (drawable->drawable_id);
  if (gimp_drawable_has_alpha (drawable->drawable_id))
    bpp--;

  /* force grayscale if it's the only thing we can do */
  if (bpp == 1)
    {
      pvals.colorspace = CS_GREY;
    }
  else
    {
      if (pvals.colorspace == CS_GREY)
        pvals.colorspace = CS_RGB;
    }

  dialog = gimp_dialog_new (_("Newsprint"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  paned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_container_set_border_width (GTK_CONTAINER (paned), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      paned, TRUE, TRUE, 0);
  gtk_widget_show (paned);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_paned_pack1 (GTK_PANED (paned), hbox, TRUE, FALSE);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_end (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  preview = gimp_drawable_preview_new_from_drawable_id (drawable->drawable_id);
  gtk_box_pack_start (GTK_BOX (hbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (newsprint),
                            drawable);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_paned_pack2 (GTK_PANED (paned), hbox, FALSE, FALSE);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /* resolution settings  */
  frame = gimp_frame_new (_("Resolution"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  gimp_image_get_resolution (gimp_item_get_image (drawable->drawable_id),
                             &xres, &yres);
  /* XXX hack: should really note both resolutions, and use
   * rectangular cells, not square cells.  But I'm being lazy,
   * and the majority of the world works with xres == yres */
  pvals_ui.input_spi = xres;

  st.input_spi =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                          _("_Input SPI:"), SCALE_WIDTH, 7,
                          pvals_ui.input_spi,
                          1.0, 1200.0, 1.0, 10.0, 0,
                          FALSE, GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION,
                          NULL, NULL);
  g_signal_connect (st.input_spi, "value-changed",
                    G_CALLBACK (spi_callback),
                    &st);
  g_signal_connect_swapped (st.input_spi, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  st.output_lpi =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                          _("O_utput LPI:"), SCALE_WIDTH, 7,
                          pvals_ui.output_lpi,
                          1.0, 1200.0, 1.0, 10.0, 1,
                          FALSE, GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION,
                          NULL, NULL);
  g_signal_connect (st.output_lpi, "value-changed",
                    G_CALLBACK (lpi_callback),
                    &st);
  g_signal_connect_swapped (st.output_lpi, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  st.cellsize = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                                      _("C_ell size:"), SCALE_WIDTH, 7,
                                      pvals.cell_width,
                                      3.0, 100.0, 1.0, 5.0, 0,
                                      FALSE, 3.0, GIMP_MAX_IMAGE_SIZE,
                                      NULL, NULL);
  g_signal_connect (st.cellsize, "value-changed",
                    G_CALLBACK (cellsize_callback),
                    &st);
  g_signal_connect_swapped (st.cellsize, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* screen settings */
  frame = gimp_frame_new (_("Screen"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  st.vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_add (GTK_CONTAINER (frame), st.vbox);

  /* optional portion begins */
  if (bpp != 1)
    {
      GtkWidget *hbox;
      GtkWidget *label;
      GtkWidget *button;
      GtkWidget *toggle;

      st.pull_table = gtk_table_new (1, 3, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (st.pull_table), 6);

      /* black pullout */
      st.pull = gimp_scale_entry_new (GTK_TABLE (st.pull_table), 0, 0,
                                      _("B_lack pullout (%):"), SCALE_WIDTH, 0,
                                      pvals.k_pullout,
                                      0, 100, 1, 10, 0,
                                      TRUE, 0, 0,
                                      NULL, NULL);
      gtk_widget_set_sensitive (st.pull_table, (pvals.colorspace == CS_CMYK));
      gtk_widget_show (st.pull_table);

      g_signal_connect (st.pull, "value-changed",
                        G_CALLBACK (gimp_int_adjustment_update),
                        &pvals.k_pullout);
      g_signal_connect_swapped (st.pull, "value-changed",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);

      /* RGB / CMYK / Luminance select */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_start (GTK_BOX (st.vbox), hbox, FALSE, FALSE, 0);

      /*  pack the scaleentry table  */
      gtk_box_pack_start (GTK_BOX (st.vbox), st.pull_table, FALSE, FALSE, 0);

      label = gtk_label_new (_("Separate to:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show(label);

      toggle = gtk_radio_button_new_with_mnemonic(group, _("_RGB"));
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
      gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    (pvals.colorspace == CS_RGB));
      gtk_widget_show (toggle);

      g_object_set_data (G_OBJECT (toggle), "dialog", &st);
      g_object_set_data (G_OBJECT (toggle), "preview", preview);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (newsprint_cspace_update),
                        GINT_TO_POINTER (CS_RGB));
      g_signal_connect_swapped (toggle, "toggled",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);

      toggle = gtk_radio_button_new_with_mnemonic (group, _("C_MYK"));
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
      gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    (pvals.colorspace == CS_CMYK));
      gtk_widget_show (toggle);

      g_object_set_data (G_OBJECT (toggle), "dialog", &st);
      g_object_set_data (G_OBJECT (toggle), "preview", preview);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (newsprint_cspace_update),
                        GINT_TO_POINTER (CS_CMYK));
      g_signal_connect_swapped (toggle, "toggled",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);

      toggle = gtk_radio_button_new_with_mnemonic (group, _("I_ntensity"));
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
      gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    (pvals.colorspace == CS_LUMINANCE));
      gtk_widget_show (toggle);

      g_object_set_data (G_OBJECT (toggle), "dialog", &st);
      g_object_set_data (G_OBJECT (toggle), "preview", preview);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (newsprint_cspace_update),
                        GINT_TO_POINTER (CS_LUMINANCE));
      g_signal_connect_swapped (toggle, "toggled",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);

      gtk_widget_show (hbox);

      /* channel lock & factory defaults button */
      hbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
      gtk_box_set_spacing (GTK_BOX (hbox), 6);
      gtk_box_pack_start (GTK_BOX (st.vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      toggle = gtk_check_button_new_with_mnemonic (_("_Lock channels"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    pvals_ui.lock_channels);
      gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &pvals_ui.lock_channels);
      g_signal_connect_swapped (toggle, "toggled",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);

      button = gtk_button_new_with_mnemonic (_("_Factory Defaults"));
      gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (newsprint_defaults_callback),
                        &st);
      g_signal_connect_swapped (button, "clicked",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);
    }

  /*  Make the channels appropriate for this colorspace and
   *  currently selected defaults.  They may have already been
   *  created as a result of callbacks to cspace_update from
   *  gtk_toggle_button_set_active().
   */
  if (!st.chst[pvals.colorspace][0])
    {
      gen_channels (&st, pvals.colorspace, preview);
    }

  gtk_widget_show (st.vbox);
  gtk_widget_show (frame);

  /* anti-alias control */
  frame = gimp_frame_new (_("Antialiasing"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("O_versample:"), SCALE_WIDTH, 0,
                              pvals.oversample,
                              1.0, 15.0, 1.0, 5.0, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &pvals.oversample);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  preview_update(st.chst[pvals.colorspace][0]);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

/*  Newsprint interface functions  */

static void
newsprint_cspace_update (GtkWidget *widget,
                         gpointer   data)
{
  NewsprintDialog_st *st;
  gint                new_cs = GPOINTER_TO_INT (data);
  gint                old_cs = pvals.colorspace;
  GtkWidget          *preview;

  st = g_object_get_data (G_OBJECT (widget), "dialog");
  preview = g_object_get_data (G_OBJECT (widget), "preview");

  if (!st)
    printf ("newsprint: cspace_update: no state, can't happen!\n");

  if (st)
    {
      gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

      /* the CMYK widget looks after the black pullout widget */
      if (new_cs == CS_CMYK)
        {
          gtk_widget_set_sensitive (st->pull_table, active);
        }

      /* if we're not activate, then there's nothing more to do */
      if (! active)
        return;

      pvals.colorspace = new_cs;

      /* make sure we have the necessary channels for the new
       * colorspace */
      if (!st->chst[new_cs][0])
        gen_channels (st, new_cs, preview);

      /* hide the old channels (if any) */
      if (st->channel_notebook[old_cs])
        {
          gtk_widget_hide (st->channel_notebook[old_cs]);
        }

      /* show the new channels */
      gtk_widget_show (st->channel_notebook[new_cs]);

      gtk_notebook_set_current_page (GTK_NOTEBOOK (st->channel_notebook[new_cs]), 0);
      preview_update (st->chst[new_cs][0]);
    }
}


/*
 * Newsprint Effect
 */

/*************************************************************/
/* Spot functions */


/* Spot functions define the order in which pixels should be whitened
 * as a cell lightened in color.  They are defined over the entire
 * cell, and are called over each pixel in the cell.  The cell
 * co-ordinate space ranges from -1.0 .. +1.0 inclusive, in both x- and
 * y-axes.
 *
 * This means the spot function f(x, y) must be defined for:
 *     -1 <= x <= +1, where x is a real number,   and
 *     -1 <= y <= +1, where y is a real number.
 *
 * The function f's range is -1.0 .. +1.0 inclusive, but it is
 * permissible for f to return values outside this range: the nearest
 * valid value will be used instead.  NOTE: this is in contrast with
 * PostScript spot functions, where it is a RangeError for the spot
 * function to go outside these limits.
 *
 * An initially black cell is filled from lowest spot function value
 * to highest.  The actual values returned do not matter - it is their
 * relative orderings that count.  This means that spot functions do
 * not need to be tonally balanced.  A tonally balanced spot function
 * is one which for all slices though the function (eg say at z), the
 * area of the slice = 4z.  In particular, almost all PostScript spot
 * functions are _not_ tonally balanced.
 */


/* The classic growing dot spot function.  This one isn't tonally
 * balanced.  It can be made so, but it's _really_ ugly.  Thanks to
 * Richard Mortier for this one:
 *
 * #define a(r) \
 *     ((r<=1)? G_PI * (r*r) : \
 *      4 * sqrt(r*r - 1)  +  G_PI*r*r  -  4*r*r*acos(1/r))
 *
 *   radius = sqrt(x*x + y*y);
 *
 * return a(radius) / 4; */
static gdouble
spot_round (gdouble x,
            gdouble y)
{
  return 1 - (x * x + y * y);
}

/* Another commonly seen spot function is the v-shaped wedge. Tonally
 * balanced. */
static gdouble
spot_line (gdouble x,
           gdouble y)
{
  return ABS (y);
}

/* Square/Diamond dot that never becomes round.  Tonally balanced. */
static gdouble
spot_diamond (gdouble x,
              gdouble y)
{
  gdouble xy = ABS (x) + ABS (y);
  /* spot only valid for 0 <= xy <= 2 */
  return ((xy <= 1) ?
          2 * xy * xy :
          2 * xy * xy - 4 * (xy - 1) * (xy - 1)) / 4;
}

/* The following two functions are implementations of algorithms
 * described in "PostScript Screening: Adobe Accurate Screens"
 * (Adobe Press, 1992) by Peter Fink.
 */

/* Square (or Euclidean) dot.  Also very common. */
static gdouble
spot_PSsquare (gdouble x,
               gdouble y)
{
  gdouble ax = ABS (x);
  gdouble ay = ABS (y);

  return ((ax + ay) > 1 ?
          ((ay - 1) * (ay - 1) + (ax - 1) * (ax - 1)) - 1 :
          1 - (ay * ay + ax * ax));
}

/* Diamond spot function */
static gdouble
spot_PSdiamond (gdouble x,
                gdouble y)
{
  gdouble ax = ABS (x);
  gdouble ay = ABS (y);

  return ((ax + ay) <= 0.75 ? 1 - (ax * ax + ay * ay) :     /* dot */
          ((ax + ay) <= 1.23 ?  1 - ((ay * 0.76) + ax) :    /* to diamond (0.76 distort) */
           ((ay - 1) * (ay - 1) + (ax - 1) * (ax - 1)) -1)); /* back to dot */
}


/*************************************************************/
/* Spot function to threshold matrix conversion */


/* Each call of the spot function results in one of these */
typedef struct
{
  gint    index;  /* (y * width) + x */
  gdouble value;  /* return value of the spot function */
} order_t;

/* qsort(3) compare function */
static gint
order_cmp (const void *va,
           const void *vb)
{
  const order_t *a = va;
  const order_t *b = vb;

  return (a->value < b->value) ? -1 : ((a->value > b->value)? + 1 : 0);
}

/* Convert spot function "type" to a threshold matrix of size "width"
 * times "width".  Returns newly allocated threshold matrix.  The
 * reason for qsort()ing the results rather than just using the spot
 * function's value directly as the threshold value is that we want to
 * ensure that the threshold matrix is tonally balanced - that is, for
 * a threshold value of x%, x% of the values in the matrix are < x%.
 *
 * Actually, it turns out that qsort()ing a function which is already
 * balanced can quite significantly detract from the quality of the
 * final result.  This is particularly noticeable with the line or
 * diamond spot functions at 45 degrees.  This is because if the spot
 * function has multiple locations with the same value, qsort may use
 * them in any order.  Often, there is quite clearly an optimal order
 * however.  By marking functions as pre-balanced, this random
 * shuffling is avoided.  WARNING: a non-balanced spot function marked
 * as pre-balanced is bad: you'll end up with dark areas becoming too
 * dark or too light, and vice versa for light areas.  This is most
 * easily checked by halftoning an area, then bluring it back - you
 * should get the same color back again.  The only way of getting a
 * correctly balanced function is by getting a formula for the spot's
 * area as a function of x and y - this can be fairly tough (ie
 * possiblly an integral in two dimensions that must be solved
 * analytically).
 *
 * The threshold matrix is used to compare against image values.  If
 * the image value is greater than the threshold value, then the
 * output pixel is illuminated.  This means that a threshold matrix
 * entry of 0 never causes output pixels to be illuminated.  */
static guchar *
spot2thresh (gint type,
             gint width)
{
  gdouble   sx, sy;
  gdouble   val;
  spotfn_t *spotfn;
  guchar   *thresh;
  order_t  *order;
  gint      x, y;
  gint      i;
  gint      wid2 = width * width;
  gint      balanced = spotfn_list[type].balanced;

  thresh = g_new (guchar, wid2);
  spotfn = spotfn_list[type].fn;

  order = g_new (order_t, wid2);

  i = 0;
  for (y = 0; y < width; y++)
    {
      for (x = 0; x < width; x++)
        {
          /* scale x & y to -1 ... +1 inclusive */
          sx = (((gdouble)x) / (width-1) - 0.5) * 2;
          sy = (((gdouble)y) / (width-1) - 0.5) * 2;
          val = spotfn(sx, sy);
          val = CLAMP (val, -1, 1);  /* interval is inclusive */

          order[i].index = i;
          order[i].value = val;
          i++;
        }
    }

  if (!balanced)
    {
      /* now sort array of (point, value) pairs in ascending order */
      qsort (order, wid2, sizeof (order_t), order_cmp);
    }

  /* compile threshold matrix in order from darkest to lightest */
  for (i = 0; i < wid2; i++)
    {
      /* thresh[] contains values from 0 .. 254.  The reason for not
       * including 255 is so that an image value of 255 remains
       * unmolested.  It would be bad to filter a completely white
       * image and end up with black speckles.  */
      if (balanced)
        thresh[order[i].index] = order[i].value * 0xfe;
      else
        thresh[order[i].index] = i * 0xff / wid2;
    }

  g_free (order);

  /* TODO: this is where the code to apply a transfer or dot gain
   * function to the threshold matrix would go. */

  return thresh;
}


/**************************************************************/
/* Main loop */


/* This function operates on the image, striding across it in tiles. */
static void
newsprint (GimpDrawable *drawable,
           GimpPreview  *preview)
{
  GimpPixelRgn  src_rgn, dest_rgn;
  guchar       *src_row, *dest_row;
  guchar       *src, *dest;
  guchar       *thresh[4] = { NULL, NULL, NULL, NULL };
  gdouble       r;
  gdouble       theta;
  gdouble       rot[4];
  gint          bpp, color_bpp;
  gint          has_alpha;
  gint          b;
  gint          tile_width;
  gint          tile_height;
  gint          width;
  gint          row, col;
  gint          x, y, x_step, y_step;
  gint          x1, y1, x2, y2;
  gint          preview_width, preview_height;
  gint          rx, ry;
  gint          progress, max_progress;
  gint          oversample;
  gint          colorspace;
  gpointer      pr;
  gint          w002;
  guchar       *preview_buffer = NULL;

#ifdef TIMINGS
  GTimer    *timer = g_timer_new ();
#endif

  width = pvals.cell_width;

  if (width < 0)
    width = -width;
  if (width < 1)
    width = 1;

  oversample = pvals.oversample;

  width *= oversample;

  tile_width = gimp_tile_width ();
  tile_height = gimp_tile_height ();

  bpp        = gimp_drawable_bpp (drawable->drawable_id);

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &preview_width, &preview_height);
      x2 = x1 + preview_width;
      y2 = y1 + preview_height;
      preview_buffer = g_new (guchar, preview_width * preview_height * bpp);
    }
  else
    {
      gint w, h;

      if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                          &x1, &y1, &w, &h))
	return;

      x2 = x1 + w;
      y2 = y1 + h;
    }

  has_alpha  = gimp_drawable_has_alpha (drawable->drawable_id);
  color_bpp = has_alpha ? bpp-1 : bpp;
  colorspace= pvals.colorspace;
  if (color_bpp == 1)
    {
      colorspace = CS_GREY;
    }
  else
    {
      if (colorspace == CS_GREY)
        colorspace = CS_RGB;
    }

  /* Bartlett window matrix optimisation */
  w002 = BARTLETT (0, 0) * BARTLETT (0, 0);
#if 0
  /* It turns out to be slightly slower to cache a pre-computed
   * bartlett matrix!   I put it down to d-cache pollution *shrug* */
  wgt = g_new (gint, oversample * oversample);
  for (y = -oversample / 2; y <= oversample / 2; y++)
    for (x = -oversample / 2; x<=oversample / 2; x++)
      WGT (x, y) = BARTLETT (x, y);
#endif /* 0 */

#define ASRT(_x)                                                \
do {                                                            \
    if (!VALID_SPOTFN(_x))                                      \
    {                                                           \
        printf("newsprint: %d is not a valid spot type\n", _x); \
        _x = SPOTFN_DOT;                                        \
    }                                                           \
} while(0)

  /* calculate the RGB / CMYK rotations and threshold matrices */
  if (color_bpp == 1 || colorspace == CS_LUMINANCE)
    {
      rot[0]    = DEG2RAD (pvals.gry_ang);
      thresh[0] = spot2thresh (pvals.gry_spotfn, width);
    }
  else
    {
      gint rf = pvals.red_spotfn;
      gint gf = pvals.grn_spotfn;
      gint bf = pvals.blu_spotfn;

      rot[0] = DEG2RAD (pvals.red_ang);
      rot[1] = DEG2RAD (pvals.grn_ang);
      rot[2] = DEG2RAD (pvals.blu_ang);

      /* always need at least one threshold matrix */
      ASRT (rf);
      spotfn_list[rf].thresh = spot2thresh (rf, width);
      thresh[0] = spotfn_list[rf].thresh;

      ASRT (gf);
      spotfn_list[gf].thresh = spot2thresh (gf, width);
      thresh[1] = spotfn_list[gf].thresh;

      ASRT (bf);
      spotfn_list[bf].thresh = spot2thresh (bf, width);
      thresh[2] = spotfn_list[bf].thresh;

      if (colorspace == CS_CMYK)
        {
          rot[3] = DEG2RAD (pvals.gry_ang);
          gf = pvals.gry_spotfn;
          ASRT (gf);
          spotfn_list[gf].thresh = spot2thresh (gf, width);
          thresh[3] = spotfn_list[gf].thresh;
        }
    }

  /* Initialize progress */
  progress     = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  for (y = y1; y < y2; y += tile_height - (y % tile_height))
    {
      for (x = x1; x < x2; x += tile_width - (x % tile_width))
        {
          /* snap to tile boundary */
          x_step = tile_width - (x % tile_width);
          y_step = tile_height - (y % tile_height);
          /* don't step off the end of the image */
          x_step = MIN (x_step, x2 - x);
          y_step = MIN (y_step, y2 - y);

          /* set up the source and dest regions */
          gimp_pixel_rgn_init (&src_rgn, drawable, x, y, x_step, y_step,
                               FALSE/*dirty*/, FALSE/*shadow*/);

          gimp_pixel_rgn_init (&dest_rgn, drawable, x, y, x_step, y_step,
                               TRUE/*dirty*/, TRUE/*shadow*/);

          /* page in the image, one tile at a time */
          for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
               pr != NULL;
               pr = gimp_pixel_rgns_process (pr))
            {
              src_row  = src_rgn.data;
              if (preview)
                dest_row = preview_buffer +
                   ((src_rgn.y - y1) * preview_width + src_rgn.x - x1) * bpp;
              else
                dest_row = dest_rgn.data;

              for (row = 0; row < src_rgn.h; row++)
                {
                  src  = src_row;
                  dest = dest_row;
                  for (col = 0; col < src_rgn.w; col++)
                    {
                      guchar data[4];

                      rx = (x + col) * oversample;
                      ry = (y + row) * oversample;

                      /* convert rx and ry to polar (r, theta) */
                      r = sqrt (((double)rx)*rx + ((double)ry)*ry);
                      theta = atan2 (((gdouble)ry), ((gdouble)rx));

                      for (b = 0; b < color_bpp; b++)
                        data[b] = src[b];

                      /* do color space conversion */
                      switch (colorspace)
                        {
                        case CS_CMYK:
                          {
                            gint r,g,b,k;

                            r = data[0];
                            g = data[1];
                            b = data[2];
                            k = pvals.k_pullout;

                            gimp_rgb_to_cmyk_int (&r, &g, &b, &k);

                            data[0] = r;
                            data[1] = g;
                            data[2] = b;
                            data[3] = k;
                          }
                          break;

                        case CS_LUMINANCE:
                          data[3] = data[0]; /* save orig for later */
                          data[0] = GIMP_RGB_LUMINANCE (data[0],
                                                        data[1],
                                                        data[2]) + 0.5;
                          break;

                        default:
                          break;
                        }

                      for (b = 0; b < cspace_nchans[colorspace]; b++)
                        {
                          rx = RINT (r * cos (theta + rot[b]));
                          ry = RINT (r * sin (theta + rot[b]));

                          /* Make sure rx and ry are positive and within
                           * the range 0 .. width-1 (incl).  Can't use %
                           * operator, since its definition on negative
                           * numbers is not helpful.  Can't use ABS(),
                           * since that would cause reflection about the
                           * x- and y-axes.  Relies on integer division
                           * rounding towards zero. */
                          rx -= ((rx - ISNEG (rx) * (width-1)) / width) * width;
                          ry -= ((ry - ISNEG (ry) * (width-1)) / width) * width;

                          {
                            guint32 sum = 0;
                            gint sx, sy;
                            gint tx, ty;
                            for (sy = -oversample/2; sy <= oversample/2; sy++)
                              for (sx = -oversample/2; sx <= oversample/2; sx++)
                                {
                                  tx = rx+sx;
                                  ty = ry+sy;
                                  while (tx < 0)  tx += width;
                                  while (ty < 0)  ty += width;
                                  while (tx >= width)  tx -= width;
                                  while (ty >= width)  ty -= width;
                                  if (data[b] > THRESHn(b, tx, ty))
                                    sum += 0xff * BARTLETT(sx, sy);
                                }
                            sum /= w002;
                            data[b] = sum;
                          }
                        }
                      if (has_alpha)
                        dest[color_bpp] = src[color_bpp];

                      /* re-pack the colors into RGB */
                      switch (colorspace)
                        {
                        case CS_CMYK:
                          data[0] = CLAMPED_ADD (data[0], data[3]);
                          data[1] = CLAMPED_ADD (data[1], data[3]);
                          data[2] = CLAMPED_ADD (data[2], data[3]);
                          data[0] = 0xff - data[0];
                          data[1] = 0xff - data[1];
                          data[2] = 0xff - data[2];
                          break;

                        case CS_LUMINANCE:
                          if (has_alpha)
                            {
                              dest[color_bpp] = data[0];
                              data[0] = 0xff;
                            }
                          data[1] = data[1] * data[0] / 0xff;
                          data[2] = data[2] * data[0] / 0xff;
                          data[0] = data[3] * data[0] / 0xff;
                          break;

                        default:
                          /* no other special cases */
                          break;
                        }

                      for (b = 0; b < color_bpp; b++)
                        dest[b] = data[b];

                      src  += src_rgn.bpp;
                      dest += dest_rgn.bpp;
                    }
                  src_row  += src_rgn.rowstride;
                  if (preview)
                    dest_row += preview_width * bpp;
                  else
                    dest_row += dest_rgn.rowstride;
                }

              /* Update progress */
              progress += src_rgn.w * src_rgn.h;
              if (!preview)
                gimp_progress_update ((double) progress / (double) max_progress);
            }
        }
    }

#ifdef TIMINGS
  g_printerr ("%f seconds\n", g_timer_elapsed (timer));
  g_timer_destroy (timer);
#endif

  /*
   * Note: the tresh array should *NOT* be freed.
   * Its values will be reused anyway so this is NOT a memory leak.
   * Well it is, but only the first time, so it doesn't matter.
   */

  if (preview)
    {
      gimp_preview_draw_buffer (preview, preview_buffer, preview_width * bpp);

      g_free (preview_buffer);
    }
  else
    {
      gimp_progress_update (1.0);
      /* update the affected region */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
    }
}
