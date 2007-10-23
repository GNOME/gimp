/*
 * This is a plugin for GIMP.
 *
 * Copyright (C) 1997 Xavier Bouchoux
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
 */

/*
 * This plug-in produces sinus textures.
 *
 * Please send any patches or suggestions to me: Xavier.Bouchoux@ensimag.imag.fr.
 */

/* Version 0.99 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-sinus"
#define PLUG_IN_BINARY "sinus"


/*
 * This structure is used for persistent data.
 */

#define B_W        0L  /* colors setting */
#define USE_FG_BG  1L
#define USE_COLORS 2L

#define LINEAR     0L  /* colorization settings */
#define BILINEAR   1L
#define SINUS      2L

#define IDEAL      0L  /* Perturbation settings */
#define PERTURBED  1L

typedef struct
{
  gdouble   scalex;
  gdouble   scaley;
  gdouble   cmplx;
  gdouble   blend_power;
  guint32   seed;
  gint      tiling;
  glong     perturbation;
  glong     colorization;
  glong     colors;
  GimpRGB   col1;
  GimpRGB   col2;
  gboolean  random_seed;
} SinusVals;

static SinusVals svals =
{
  15.0,
  15.0,
  1.0,
  0.0,
  42,
  TRUE,
  PERTURBED,
  LINEAR,
  USE_COLORS,
  { 1.0, 1.0, 0.0, 1.0 },
  { 0.0, 0.0, 1.0, 1.0 },
  FALSE
};

typedef struct
{
  gint    height, width;
  gdouble c11, c12, c13, c21, c22, c23, c31, c32, c33;
  gdouble (*blend) (double );
  guchar  r, g, b, a;
  gint    dr, dg, db, da;
} params;


typedef struct
{
  gint     width;
  gint     height;
  gint     bpp;
  gdouble  scale;
  guchar  *bits;
} mwPreview;

static gboolean          drawable_is_grayscale = FALSE;
static mwPreview        *thePreview;
static GimpDrawable     *drawable;

/*  preview stuff -- to be removed as soon as we have a real libgimp preview  */

#define PREVIEW_SIZE 100

static gboolean do_preview = TRUE;

static GtkWidget        * mw_preview_new   (GtkWidget    *parent,
                                            mwPreview    *mwp);
static mwPreview * mw_preview_build_virgin (GimpDrawable *drw);

/* Declare functions */

static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);
static void sinus (void);

static gdouble linear   (gdouble v);
static gdouble bilinear (gdouble v);
static gdouble cosinus  (gdouble v);

static gint    sinus_dialog     (void);
static void    sinus_do_preview (GtkWidget *widget);

static void assign_block_4 (guchar *dest, gdouble grey, params *p);
static void assign_block_3 (guchar *dest, gdouble grey, params *p);
static void assign_block_2 (guchar *dest, gdouble grey, params *p);
static void assign_block_1 (guchar *dest, gdouble grey, params *p);
static void compute_block_x (guchar *dest_row,
                             guint rowstride,
                             gint x0, gint y0, gint w, gint h,
                             gint bpp,
                             void (*assign)(guchar *dest, gdouble grey,
                                            params *p),
                             params *p);

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
    { GIMP_PDB_INT32,    "run-mode",    "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",       "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",    "Input drawable" },

    { GIMP_PDB_FLOAT,    "xscale",      "Scale value for x axis" },
    { GIMP_PDB_FLOAT,    "yscale",      "Scale value dor y axis" },
    { GIMP_PDB_FLOAT,    "complex",     "Complexity factor" },
    { GIMP_PDB_INT32,    "seed",        "Seed value for random number generator" },
    { GIMP_PDB_INT32,    "tiling",      "If set, the pattern generated will tile" },
    { GIMP_PDB_INT32,    "perturb",     "If set, the pattern is a little more distorted..." },
    { GIMP_PDB_INT32,    "colors",      "where to take the colors (0= B&W,  1= fg/bg, 2= col1/col2)"},
    { GIMP_PDB_COLOR,    "col1",        "fist color (sometimes unused)" },
    { GIMP_PDB_COLOR,    "col2",        "second color (sometimes unused)" },
    { GIMP_PDB_FLOAT,    "alpha1",      "alpha for the first color (used if the drawable has an alpha chanel)" },
    { GIMP_PDB_FLOAT,    "alpha2",      "alpha for the second color (used if the drawable has an alpha chanel)" },
    { GIMP_PDB_INT32,    "blend",       "0= linear, 1= bilinear, 2= sinusoidal" },
    { GIMP_PDB_FLOAT,    "blend-power", "Power used to strech the blend" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Generate complex sinusoidal textures"),
                          "FIX ME: sinus help",
                          "Xavier Bouchoux",
                          "Xavier Bouchoux",
                          "1997",
                          N_("_Sinus..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Render/Pattern");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  INIT_I18N ();

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &svals);

      /* In order to prepare the dialog I need to know wether it's grayscale or not */
      drawable = gimp_drawable_get (param[2].data.d_drawable);
      thePreview = mw_preview_build_virgin(drawable);
      drawable_is_grayscale = gimp_drawable_is_gray (drawable->drawable_id);

      if (! sinus_dialog ())
        return;

      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 17)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          svals.scalex       = param[3].data.d_float;
          svals.scaley       = param[4].data.d_float;
          svals.cmplx        = param[5].data.d_float;
          svals.seed         = param[6].data.d_int32;
          svals.tiling       = param[7].data.d_int32;
          svals.perturbation = param[8].data.d_int32;
          svals.colors       = param[9].data.d_int32;
          svals.col1         = param[10].data.d_color;
          svals.col2         = param[11].data.d_color;
          gimp_rgb_set_alpha (&svals.col1, param[12].data.d_float);
          gimp_rgb_set_alpha (&svals.col2, param[13].data.d_float);
          svals.colorization = param[14].data.d_int32;
          svals.blend_power  = param[15].data.d_float;

          if (svals.random_seed)
            svals.seed = g_random_int ();
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &svals);

      if (svals.random_seed)
        svals.seed = g_random_int ();
      break;

    default:
      break;
    }

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  Make sure that the drawable is gray or RGB */
  if ((status == GIMP_PDB_SUCCESS) &&
      (gimp_drawable_is_rgb (drawable->drawable_id) ||
       gimp_drawable_is_gray (drawable->drawable_id)))
    {
      gimp_progress_init (_("Sinus: rendering"));
      gimp_tile_cache_ntiles (1);
      sinus ();

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &svals, sizeof (SinusVals));
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/*
 *  Main procedure
 */

static void
prepare_coef (params *p)
{
  GimpRGB color1;
  GimpRGB color2;
  gdouble scalex = svals.scalex;
  gdouble scaley = svals.scaley;
  GRand *gr;

  gr = g_rand_new ();

  g_rand_set_seed (gr, svals.seed);

  switch (svals.colorization)
    {
    case BILINEAR:
      p->blend = bilinear;
      break;
    case SINUS:
      p->blend = cosinus;
      break;
    case LINEAR:
    default:
      p->blend = linear;
    }

  if (svals.perturbation==IDEAL)
    {
      /* Presumably the 0 * g_rand_int ()s are to pop random
       * values off the prng, I don't see why though. */
      p->c11= 0 * g_rand_int (gr);
      p->c12= g_rand_double_range (gr, -1, 1) * scaley;
      p->c13= g_rand_double_range (gr, 0, 2 * G_PI);
      p->c21= 0 * g_rand_int (gr);
      p->c22= g_rand_double_range (gr, -1, 1)  * scaley;
      p->c23= g_rand_double_range (gr, 0, 2 * G_PI);
      p->c31= g_rand_double_range (gr, -1, 1) * scalex;
      p->c32= 0 * g_rand_int (gr);
      p->c33= g_rand_double_range (gr, 0, 2 * G_PI);
    }
  else
    {
      p->c11= g_rand_double_range (gr, -1, 1) * scalex;
      p->c12= g_rand_double_range (gr, -1, 1) * scaley;
      p->c13= g_rand_double_range (gr, 0, 2 * G_PI);
      p->c21= g_rand_double_range (gr, -1, 1) * scalex;
      p->c22= g_rand_double_range (gr, -1, 1) * scaley;
      p->c23= g_rand_double_range (gr, 0, 2 * G_PI);
      p->c31= g_rand_double_range (gr, -1, 1) * scalex;
      p->c32= g_rand_double_range (gr, -1, 1) * scaley;
      p->c33= g_rand_double_range (gr, 0, 2 * G_PI);
    }

  if (svals.tiling)
    {
      p->c11= ROUND (p->c11/(2*G_PI))*2*G_PI;
      p->c12= ROUND (p->c12/(2*G_PI))*2*G_PI;
      p->c21= ROUND (p->c21/(2*G_PI))*2*G_PI;
      p->c22= ROUND (p->c22/(2*G_PI))*2*G_PI;
      p->c31= ROUND (p->c31/(2*G_PI))*2*G_PI;
      p->c32= ROUND (p->c32/(2*G_PI))*2*G_PI;
    }

  color1 = svals.col1;
  color2 = svals.col2;

  if (drawable_is_grayscale)
    {
      gimp_rgb_set (&color1, 1.0, 1.0, 1.0);
      gimp_rgb_set (&color2, 0.0, 0.0, 0.0);
    }
  else
    {
      switch (svals.colors)
        {
        case USE_COLORS:
          break;
        case B_W:
          gimp_rgb_set (&color1, 1.0, 1.0, 1.0);
          gimp_rgb_set (&color2, 0.0, 0.0, 0.0);
          break;
        case USE_FG_BG:
          gimp_context_get_background (&color1);
          gimp_context_get_foreground (&color2);
          break;
        }
    }

  gimp_rgba_get_uchar (&color1, &p->r, &p->g, &p->b, &p->a);

  gimp_rgba_subtract (&color2, &color1);
  p->dr = color2.r * 255.0;
  p->dg = color2.g * 255.0;
  p->db = color2.b * 255.0;
  p->da = color2.a * 255.0;

  g_rand_free (gr);
}

static void
sinus (void)
{
  params  p;
  gint    bytes;
  GimpPixelRgn dest_rgn;
  gint     x1, y1, x2, y2;
  gpointer pr;
  gint progress, max_progress;

  prepare_coef(&p);

  gimp_drawable_mask_bounds(drawable->drawable_id, &x1, &y1, &x2, &y2);

  p.width = drawable->width;
  p.height = drawable->height;
  bytes = drawable->bpp;

  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       x1, y1, x2 - x1, y2 - y1, TRUE,TRUE);
  progress = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      switch (bytes)
        {
        case 4:
          compute_block_x (dest_rgn.data, dest_rgn.rowstride,
                           dest_rgn.x, dest_rgn.y, dest_rgn.w, dest_rgn.h,
                           4, assign_block_4, &p);
          break;
        case 3:
          compute_block_x (dest_rgn.data, dest_rgn.rowstride,
                           dest_rgn.x, dest_rgn.y, dest_rgn.w, dest_rgn.h,
                           3, assign_block_3, &p);
          break;
        case 2:
          compute_block_x (dest_rgn.data, dest_rgn.rowstride,
                           dest_rgn.x, dest_rgn.y, dest_rgn.w, dest_rgn.h,
                           2, assign_block_2, &p);
          break;
        case 1:
          compute_block_x (dest_rgn.data, dest_rgn.rowstride,
                           dest_rgn.x, dest_rgn.y, dest_rgn.w, dest_rgn.h,
                           1, assign_block_1, &p);
          break;
        }
      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / (double) max_progress);
    }

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, x2 - x1, y2 - y1);
}

static gdouble
linear (gdouble v)
{
  double a = v - (int) v;

  return (a < 0 ? 1.0 + a : a);
}

static gdouble
bilinear (gdouble v)
{
  double a = v - (int) v;

  a = (a < 0 ? 1.0 + a : a);
  return (a > 0.5 ? 2 - 2 * a : 2 * a);
}

static gdouble
cosinus (gdouble v)
{
  return 0.5 - 0.5 * sin ((v + 0.25) * G_PI * 2);
}

static void
assign_block_4 (guchar *dest, gdouble grey, params *p)
{
  dest[0] = p->r + (gint) (grey * p->dr);
  dest[1] = p->g + (gint) (grey * p->dg);
  dest[2] = p->b + (gint) (grey * p->db);
  dest[3] = p->a + (gint) (grey * p->da);
}

static void
assign_block_3 (guchar *dest, gdouble grey, params *p)
{
  dest[0] = p->r + (gint) (grey * p->dr);
  dest[1] = p->g + (gint) (grey * p->dg);
  dest[2] = p->b + (gint) (grey * p->db);
}

static void
assign_block_2 (guchar *dest, gdouble grey, params *p)
{
  dest[0] = (guchar) (grey * 255.0);
  dest[1] = p->a + (gint)(grey * p->da);
}

static void
assign_block_1 (guchar *dest, gdouble grey, params *p)
{
  dest[0]= (guchar) (grey * 255.0);
}

static void
compute_block_x (guchar *dest_row, guint rowstride,
                 gint x0, gint y0, gint w, gint h,
                 gint bpp,
                 void (*assign)(guchar *dest, gdouble grey, params *p),
                 params *p)
{
  gint     i, j;
  gdouble  x, y, grey;
  gdouble  pow_exp;
  guchar  *dest;

  pow_exp = exp (svals.blend_power);

  for (j = y0; j < y0 + h; j++)
    {
      y= ((gdouble) j) / p->height;
      dest = dest_row;
      for (i = x0; i < x0 + w; i++)
        {
          gdouble c;

          x = ((gdouble) i) / p->width;

          c = 0.5 * sin(p->c31 * x + p->c32 * y + p->c33);

          grey = sin(p->c11 * x + p->c12 * y + p->c13) * (0.5 + 0.5 * c) +
            sin(p->c21 * x + p->c22 * y + p->c23) * (0.5 - 0.5 * c);
          grey = pow(p->blend(svals.cmplx * (0.5 + 0.5 * grey)), pow_exp);

          assign (dest, grey, p);
          dest += bpp;
        }
      dest_row += rowstride;
    }
}

static void
alpha_scale_cb (GtkAdjustment *adj,
                gpointer       data)
{
  GimpColorButton *color_button;
  GimpRGB          color;

  if (!data)
    return;

  color_button = GIMP_COLOR_BUTTON (data);

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (color_button), &color);
  gimp_rgb_set_alpha (&color, adj->value);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (color_button), &color);
}

static void
alpha_scale_update (GtkWidget *color_button,
                    gpointer   data)
{
  GtkAdjustment *adj;
  GimpRGB        color;

  adj = GTK_ADJUSTMENT (data);

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (color_button), &color);
  gtk_adjustment_set_value (adj, color.a);

  sinus_do_preview (NULL);
}

static void
sinus_toggle_button_update (GtkWidget *widget,
                            gpointer   data)
{
  gimp_toggle_button_update (widget, data);
  sinus_do_preview (NULL);
}

static void
sinus_radio_button_update (GtkWidget *widget,
                           gpointer   data)
{
  gimp_radio_button_update (widget, data);
  sinus_do_preview (NULL);
}

static void
sinus_double_adjustment_update (GtkAdjustment *adjustment,
                                gpointer       data)
{
  gimp_double_adjustment_update (adjustment, data);
  sinus_do_preview (NULL);
}

static void
sinus_random_update (GObject   *unused,
                     gpointer   data)
{
  sinus_do_preview (NULL);
}

/*****************************************/
/* The note book                         */
/*****************************************/

gint
sinus_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *main_hbox;
  GtkWidget *preview;
  GtkWidget *notebook;
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *toggle;
  GtkWidget *push_col1 = NULL;
  GtkWidget *push_col2 = NULL;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  /* Create Main window with a vbox */
  /* ============================== */
  dlg = gimp_dialog_new (_("Sinus"), PLUG_IN_BINARY,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  main_hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_hbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (main_hbox);

  /* Create preview */
  /* ============== */
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  preview = mw_preview_new (vbox, thePreview);

  /* Create the notebook */
  /* =================== */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (main_hbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  /* Create the drawing settings frame */
  /* ================================= */
  page = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  frame = gimp_frame_new (_("Drawing Settings"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER(frame), table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_X scale:"), 140, 8,
                              svals.scalex, 0.0001, 100.0, 0.0001, 5, 4,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (sinus_double_adjustment_update),
                    &svals.scalex);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("_Y scale:"), 140, 8,
                              svals.scaley, 0.0001, 100.0, 0.0001, 5, 4,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (sinus_double_adjustment_update),
                    &svals.scaley);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                              _("Co_mplexity:"), 140, 8,
                              svals.cmplx, 0.0, 15.0, 0.01, 5, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (sinus_double_adjustment_update),
                    &svals.cmplx);

  gtk_widget_show (table);

  frame= gimp_frame_new (_("Calculation Settings"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (3, 1, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  hbox = gimp_random_seed_new (&svals.seed, &svals.random_seed);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                     _("R_andom seed:"), 1.0, 0.5,
                                     hbox, 1, TRUE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label),
                                 GIMP_RANDOM_SEED_SPINBUTTON (hbox));

  g_signal_connect (GIMP_RANDOM_SEED_SPINBUTTON_ADJ (hbox),
                    "value-changed", G_CALLBACK (sinus_random_update), NULL);
  gtk_widget_show (table);

  toggle = gtk_check_button_new_with_mnemonic (_("_Force tiling?"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), svals.tiling);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (sinus_toggle_button_update),
                    &svals.tiling);

  vbox2 = gimp_int_radio_group_new (FALSE, NULL,
                                    G_CALLBACK (sinus_radio_button_update),
                                    &svals.perturbation, svals.perturbation,

                                    _("_Ideal"),     IDEAL,     NULL,
                                    _("_Distorted"), PERTURBED, NULL,

                                    NULL);

  gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_show (vbox2);

  label = gtk_label_new_with_mnemonic (_("_Settings"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);

  /* Color settings dialog: */
  /* ====================== */
  page = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  if (drawable_is_grayscale)
    {
      frame = gimp_frame_new (_("Colors"));
      gtk_box_pack_start(GTK_BOX(page), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox = gtk_vbox_new (FALSE, 6);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      /*if in grey scale, the colors are necessarily black and white */
      label = gtk_label_new (_("The colors are white and black."));
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_container_add (GTK_CONTAINER (vbox), label);
      gtk_widget_show (label);
    }
  else
    {
      frame = gimp_int_radio_group_new (TRUE, _("Colors"),
                                        G_CALLBACK (sinus_radio_button_update),
                                        &svals.colors, svals.colors,

                                        _("Bl_ack & white"),
                                        B_W, NULL,
                                        _("_Foreground & background"),
                                        USE_FG_BG, NULL,
                                        _("C_hoose here:"),
                                        USE_COLORS, NULL,

                                        NULL);

      gtk_box_pack_start(GTK_BOX(page), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox = GTK_BIN (frame)->child;

      hbox = gtk_hbox_new (FALSE, 12);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      push_col1 = gimp_color_button_new (_("First color"), 32, 32,
                                         &svals.col1,
                                         GIMP_COLOR_AREA_SMALL_CHECKS);
      gtk_box_pack_start (GTK_BOX (hbox), push_col1, FALSE, FALSE, 0);
      gtk_widget_show (push_col1);

      g_signal_connect (push_col1, "color-changed",
                        G_CALLBACK (gimp_color_button_get_color),
                        &svals.col1);

      push_col2 = gimp_color_button_new (_("Second color"), 32, 32,
                                         &svals.col2,
                                         GIMP_COLOR_AREA_SMALL_CHECKS);
      gtk_box_pack_start (GTK_BOX (hbox), push_col2, FALSE, FALSE, 0);
      gtk_widget_show (push_col2);

      g_signal_connect (push_col2, "color-changed",
                        G_CALLBACK (gimp_color_button_get_color),
                        &svals.col2);

      gtk_widget_show (hbox);
    }

  frame = gimp_frame_new (_("Alpha Channels"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_set_sensitive (frame,
                            gimp_drawable_has_alpha (drawable->drawable_id));

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("F_irst color:"), 0, 0,
                              svals.col1.a, 0.0, 1.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (alpha_scale_cb),
                    push_col1);

  if (push_col1)
    g_signal_connect (push_col1, "color-changed",
                      G_CALLBACK (alpha_scale_update),
                      adj);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("S_econd color:"), 0, 0,
                              svals.col2.a, 0.0, 1.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (alpha_scale_cb),
                    push_col2);

  if (push_col2)
    g_signal_connect (push_col2, "color-changed",
                      G_CALLBACK (alpha_scale_update),
                      adj);

  gtk_widget_show (table);

  label = gtk_label_new_with_mnemonic (_("Co_lors"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);

  /* blend settings dialog: */
  /* ====================== */
  page = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  frame = gimp_frame_new (_("Blend Settings"));
  gtk_box_pack_start (GTK_BOX (page), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  frame =
    gimp_int_radio_group_new (TRUE, _("Gradient"),
                              G_CALLBACK (sinus_radio_button_update),
                              &svals.colorization, svals.colorization,

                              _("L_inear"),     LINEAR,   NULL,
                              _("Bili_near"),   BILINEAR, NULL,
                              _("Sin_usoidal"), SINUS,    NULL,

                              NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_Exponent:"), 0, 0,
                              svals.blend_power, -7.5, 7.5, 0.01, 5.0, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (sinus_double_adjustment_update),
                    &svals.blend_power);

  gtk_widget_show (table);

  label = gtk_label_new_with_mnemonic (_("_Blend"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);

  gtk_widget_show (dlg);

  sinus_do_preview (preview);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}

/******************************************************************/
/* Draw preview image. if DoCompute is TRUE then recompute image. */
/******************************************************************/

void
sinus_do_preview (GtkWidget *widget)
{
  static GtkWidget *theWidget = NULL;
  gint              rowsize;
  guchar           *buf;
  params            p;

  if (!do_preview)
    return;

  if (theWidget == NULL)
    {
      theWidget = widget;
    }

  rowsize = thePreview->width * thePreview->bpp;
  buf = g_new (guchar, thePreview->width*thePreview->height*thePreview->bpp);

  p.height = thePreview->height;
  p.width = thePreview->width;

  prepare_coef (&p);

  if (thePreview->bpp == 3) /* [dindinx]: it looks to me that this is always true... */
    compute_block_x (buf, rowsize, 0, 0, thePreview->width, thePreview->height,
                     3, assign_block_3, &p);
  else if (thePreview->bpp == 1)
    {
      compute_block_x (buf, rowsize, 0, 0, thePreview->width,
                       thePreview->height, 1, assign_block_1, &p);
    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (theWidget),
                          0, 0, thePreview->width, thePreview->height,
                          GIMP_RGB_IMAGE,
                          buf,
                          rowsize);
  g_free (buf);
}

static void
mw_preview_toggle_callback (GtkWidget *widget,
                            gpointer   data)
{
  gimp_toggle_button_update (widget, data);
  sinus_do_preview (NULL);
}

static mwPreview *
mw_preview_build_virgin (GimpDrawable *drw)
{
  mwPreview *mwp;

  mwp = g_new (mwPreview, 1);

  if (drw->width > drw->height)
    {
      mwp->scale  = (gdouble) drw->width / (gdouble) PREVIEW_SIZE;
      mwp->width  = PREVIEW_SIZE;
      mwp->height = drw->height / mwp->scale;
    }
  else
    {
      mwp->scale  = (gdouble) drw->height / (gdouble) PREVIEW_SIZE;
      mwp->height = PREVIEW_SIZE;
      mwp->width  = drw->width / mwp->scale;
    }

  mwp->bpp  = 3;
  mwp->bits = NULL;

  return mwp;
}

static GtkWidget *
mw_preview_new (GtkWidget *parent,
                mwPreview *mwp)
{
  GtkWidget *preview;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *button;

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (parent), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  frame = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1, TRUE);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (preview, mwp->width, mwp->height);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  button = gtk_check_button_new_with_mnemonic (_("Do _preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), do_preview);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (mw_preview_toggle_callback),
                    &do_preview);

  return preview;
}
