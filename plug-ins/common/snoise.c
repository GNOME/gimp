/* Solid Noise plug-in -- creates solid noise textures
 * Copyright (C) 1997, 1998 Marcelo de Gomensoro Malheiros
 *
 * The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/* Solid Noise plug-in version 1.04, May 2004
 *
 * This plug-in generates solid noise textures based on the
 * `Noise' and `Turbulence' functions described in the paper
 *
 *    Perlin, K, and Hoffert, E. M., "Hypertexture",
 *    Computer Graphics 23, 3 (August 1989)
 *
 * The algorithm implemented here also makes possible the
 * creation of seamless tiles.
 *
 * You can contact me at <malheiro@dca.fee.unicamp.br>.
 * Comments and improvements for this code are welcome.
 *
 * The overall plug-in structure is based on the Whirl plug-in,
 * which is Copyright (C) 1997 Federico Mena Quintero
 */

/* Version 1.04:
 *
 *  Dynamic preview added (Yeti <yeti@physics.muni.cz>).
 *
 * Version 1.03:
 *
 *  Added patch from Kevin Turner <kevint@poboxes.com> to use the
 *  current time as the random seed. Thank you!
 *  Incorporated some portability changes from the GIMP distribution.
 *
 * Version 1.02:
 *
 *  Fixed a stupid bug with the alpha channel.
 *  Fixed a rounding bug for small tilable textures.
 *  Now the dialog is more compact; using the settings from gtkrc.
 *
 * Version 1.01:
 *
 *  Quick fix for wrong pdb declaration. Also changed default seed to 1.
 *  Thanks to Adrian Likins and Federico Mena for the patch!
 *
 * Version 1.0:
 *
 *  Initial release.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/*---- Defines ----*/

#define TABLE_SIZE    64
#define WEIGHT(T)     ((2.0*fabs(T)-3.0)*(T)*(T)+1.0)

#define SCALE_WIDTH   128
#define SIZE_MIN      0.1
#define SIZE_MAX      16.0

#define PREVIEW_SIZE  128

/*---- Typedefs ----*/

typedef struct
{
  gint     tilable;
  gint     turbulent;
  guint    seed;
  gint     detail;
  gdouble  xsize;
  gdouble  ysize;
  gboolean random_seed;
} SolidNoiseValues;


/*---- Prototypes ----*/

static void        query (void);
static void        run   (const gchar      *name,
                          gint              nparams,
                          const GimpParam  *param,
                          gint             *nreturn_vals,
                          GimpParam       **return_vals);

static void        solid_noise               (GimpDrawable *drawable);
static void        solid_noise_init          (void);
static gdouble     plain_noise               (gdouble       x,
                                              gdouble       y,
                                              guint         s);
static gdouble     noise                     (gdouble       x,
                                              gdouble       y);

static gboolean    solid_noise_dialog        (void);
static GtkWidget * snoise_preview_new        (void);
static void        solid_noise_draw_one_tile (GimpPixelRgn *rgn,
                                              gdouble       width,
                                              gdouble       height,
                                              gint          xoffset,
                                              gint          yoffset,
                                              gint          chns,
                                              gboolean      has_alpha);


/*---- Variables ----*/

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static SolidNoiseValues snvals =
{
  0,   /* tilable       */
  0,   /* turbulent     */
  0,   /* seed          */
  1,   /* detail        */
  4.0, /* xsize         */
  4.0, /* ysize         */
  FALSE,
};

static gint         xclip, yclip;
static gdouble      offset, factor;
static gdouble      xsize, ysize;
static gint         perm_tab[TABLE_SIZE];
static GimpVector2  grad_tab[TABLE_SIZE];

static GtkWidget   *preview_image = NULL;
static GdkPixbuf   *preview       = NULL;

/*---- Functions ----*/

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",  "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",     "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",  "Input drawable" },
    { GIMP_PDB_INT32,    "tilable",   "Create a tilable output (n=0/y=1)" },
    { GIMP_PDB_INT32,    "turbulent", "Make a turbulent noise (n=0/y=1)" },
    { GIMP_PDB_INT32,    "seed",      "Random seed" },
    { GIMP_PDB_INT32,    "detail",    "Detail level (0 - 15)" },
    { GIMP_PDB_FLOAT,    "xsize",     "Horizontal texture size" },
    { GIMP_PDB_FLOAT,    "ysize",     "Vertical texture size" }
  };

  gimp_install_procedure ("plug_in_solid_noise",
                          "Creates a grayscale noise texture",
                          "Generates 2D textures using Perlin's classic "
                          "solid noise function.",
                          "Marcelo de Gomensoro Malheiros",
                          "Marcelo de Gomensoro Malheiros",
                          "May 2004, v1.04",
                          N_("_Solid Noise..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register ("plug_in_solid_noise",
                             N_("<Image>/Filters/Render/Clouds"));
}


static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam values[1];

  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status;

  status = GIMP_PDB_SUCCESS;
  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  See how we will run  */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data("plug_in_solid_noise", &snvals);

      /*  Get information from the dialog  */
      if (!solid_noise_dialog ())
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Test number of arguments  */
      if (nparams != 9)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          snvals.tilable   = param[3].data.d_int32;
          snvals.turbulent = param[4].data.d_int32;
          snvals.seed      = param[5].data.d_int32;
          snvals.detail    = param[6].data.d_int32;
          snvals.xsize     = param[7].data.d_float;
          snvals.ysize     = param[8].data.d_float;

          if (snvals.random_seed)
            snvals.seed = g_random_int ();
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_solid_noise", &snvals);

      if (snvals.random_seed)
        snvals.seed = g_random_int ();
      break;

    default:
      break;
    }

  /*  Create texture  */
  if ((status == GIMP_PDB_SUCCESS) &&
      (gimp_drawable_is_rgb (drawable->drawable_id) ||
       gimp_drawable_is_gray (drawable->drawable_id)))
    {
      /*  Set the tile cache size  */
      gimp_tile_cache_ntiles ((drawable->width + gimp_tile_width () - 1) /
                              gimp_tile_width ());

      /*  Run!  */
      solid_noise (drawable);

      /*  If run mode is interactive, flush displays  */
      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE ||
          run_mode == GIMP_RUN_WITH_LAST_VALS)
        {
          gimp_set_data ("plug_in_solid_noise",
                         &snvals, sizeof (SolidNoiseValues));
        }
    }
  else
    {
      /* gimp_message ("solid noise: cannot operate on indexed color images"); */
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


static void
solid_noise (GimpDrawable *drawable)
{
  GimpPixelRgn  dest_rgn;
  gint          chns;
  gint          sel_x1, sel_y1, sel_x2, sel_y2;
  gint          sel_width, sel_height;
  gint          progress, max_progress;
  gpointer      pr;
  gboolean      preview_mode;
  gboolean      has_alpha;

  preview_mode = !drawable;

  /*  Get selection area  */
  if (preview_mode)
    {
      sel_x1 = sel_y1 = 0;
      sel_x2 = sel_y2 = sel_width = sel_height = PREVIEW_SIZE;
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id,
                                 &sel_x1, &sel_y1, &sel_x2, &sel_y2);
      sel_width = sel_x2 - sel_x1;
      sel_height = sel_y2 - sel_y1;
    }

  /*  Initialization  */
  solid_noise_init ();
  if (!preview_mode)
    gimp_progress_init (_("Solid Noise..."));

  progress = 0;
  max_progress = sel_width * sel_height;
  has_alpha = FALSE;

  if (preview_mode)
    {
      dest_rgn.x = dest_rgn.y = 0;
      dest_rgn.w = dest_rgn.h = PREVIEW_SIZE;

      dest_rgn.bpp = chns  = gdk_pixbuf_get_n_channels (preview);
      dest_rgn.rowstride   = gdk_pixbuf_get_rowstride (preview);
      dest_rgn.data        = gdk_pixbuf_get_pixels (preview);
    }
  else
    {
      chns = gimp_drawable_bpp (drawable->drawable_id);

      if (gimp_drawable_has_alpha (drawable->drawable_id))
        {
          chns--;
          has_alpha = TRUE;
        }

      gimp_pixel_rgn_init (&dest_rgn, drawable, sel_x1, sel_y1, sel_width,
                           sel_height, TRUE, TRUE);
    }

  /*  One, two, three, go!  */
  if (preview_mode)
    {
      solid_noise_draw_one_tile (&dest_rgn, sel_width, sel_height,
                                 sel_x1, sel_y1, chns, has_alpha);
    }
  else
    {
      for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
          pr != NULL;
          pr = gimp_pixel_rgns_process (pr))
        {
          solid_noise_draw_one_tile (&dest_rgn, sel_width, sel_height,
                                     sel_x1, sel_y1, chns, has_alpha);

          /*  Update progress  */
          progress += dest_rgn.w * dest_rgn.h;
          gimp_progress_update ((double) progress / (double) max_progress);
        }
    }

  /*  Update the drawable  */
  if (preview_mode)
    {
      gtk_widget_queue_draw (preview_image);
    }
  else
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id,
                            sel_x1, sel_y1, sel_width, sel_height);
    }
}

static void
solid_noise_draw_one_tile (GimpPixelRgn *dest_rgn,
                           gdouble       width,
                           gdouble       height,
                           gint          xoffset,
                           gint          yoffset,
                           gint          chns,
                           gboolean      has_alpha)
{
  guchar  *dest, *dest_row;
  gint     row, col, i;
  guchar   val;

  dest_row = dest_rgn->data;

  for (row = dest_rgn->y; row < (dest_rgn->y + dest_rgn->h); row++)
    {
      dest = dest_row;

      for (col = dest_rgn->x; col < (dest_rgn->x + dest_rgn->w); col++)
        {
          val = (guchar) floor (255.0 * noise ((col - xoffset) / width,
                                               (row - yoffset) / height));

          for (i = 0; i < chns; i++)
            *dest++ = val;

          if (has_alpha)
            *dest++ = 255;
        }

      dest_row += dest_rgn->rowstride;
    }
}

static void
solid_noise_init (void)
{
  gint     i, j, k, t;
  gdouble  m;
  GRand   *gr;

  gr = g_rand_new ();

  g_rand_set_seed (gr, snvals.seed);

  /*  Force sane parameters  */
  snvals.detail = CLAMP (snvals.detail, 0, 15);
  snvals.xsize  = CLAMP (snvals.xsize, SIZE_MIN, SIZE_MAX);
  snvals.ysize  = CLAMP (snvals.ysize, SIZE_MIN, SIZE_MAX);

  /*  Set scaling factors  */
  if (snvals.tilable)
    {
      xsize = ceil (snvals.xsize);
      ysize = ceil (snvals.ysize);
      xclip = (gint) xsize;
      yclip = (gint) ysize;
    }
  else
    {
      xsize = snvals.xsize;
      ysize = snvals.ysize;
    }

  /*  Set totally empiric normalization values  */
  if (snvals.turbulent)
    {
      offset = 0.0;
      factor = 1.0;
    }
  else
    {
      offset = 0.94;
      factor = 0.526;
    }

  /*  Initialize the permutation table  */
  for (i = 0; i < TABLE_SIZE; i++)
    perm_tab[i] = i;

  for (i = 0; i < (TABLE_SIZE >> 1); i++)
    {
      j = g_rand_int_range (gr, 0, TABLE_SIZE);
      k = g_rand_int_range (gr, 0, TABLE_SIZE);
      t = perm_tab[j];
      perm_tab[j] = perm_tab[k];
      perm_tab[k] = t;
    }

  /*  Initialize the gradient table  */
  for (i = 0; i < TABLE_SIZE; i++)
    {
      do
        {
          grad_tab[i].x = g_rand_double_range (gr, -1, 1);
          grad_tab[i].y = g_rand_double_range (gr, -1, 1);
          m = grad_tab[i].x * grad_tab[i].x + grad_tab[i].y * grad_tab[i].y;
        }
      while (m == 0.0 || m > 1.0);

      m = 1.0 / sqrt(m);
      grad_tab[i].x *= m;
      grad_tab[i].y *= m;
    }

  g_rand_free (gr);
}


static gdouble
plain_noise (gdouble x,
             gdouble y,
             guint   s)
{
  GimpVector2 v;
  gint        a, b, i, j, n;
  gdouble     sum;

  sum = 0.0;
  x *= s;
  y *= s;
  a = (int) floor (x);
  b = (int) floor (y);

  for (i = 0; i < 2; i++)
    for (j = 0; j < 2; j++)
      {
        if (snvals.tilable)
          n = perm_tab[(((a + i) % (xclip * s)) + perm_tab[((b + j) % (yclip * s)) % TABLE_SIZE]) % TABLE_SIZE];
        else
          n = perm_tab[(a + i + perm_tab[(b + j) % TABLE_SIZE]) % TABLE_SIZE];
        v.x = x - a - i;
        v.y = y - b - j;
        sum += WEIGHT(v.x) * WEIGHT(v.y) * (grad_tab[n].x * v.x + grad_tab[n].y * v.y);
      }

  return sum / s;
}


static gdouble
noise (gdouble x,
       gdouble y)
{
  gint i;
  guint s;
  gdouble sum;

  s = 1;
  sum = 0.0;
  x *= xsize;
  y *= ysize;

  for (i = 0; i <= snvals.detail; i++)
    {
      if (snvals.turbulent)
        sum += fabs (plain_noise (x, y, s));
      else
        sum += plain_noise (x, y, s);
      s <<= 1;
    }

  return (sum+offset)*factor;
}

static GtkWidget*
snoise_preview_new (void)
{
  GtkWidget *frame = gtk_frame_new (NULL);

  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

  preview = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
                            PREVIEW_SIZE, PREVIEW_SIZE);
  gdk_pixbuf_fill (preview, 0);

  preview_image = gtk_image_new_from_pixbuf (preview);
  g_object_unref (preview);

  gtk_container_add (GTK_CONTAINER (frame), preview_image);
  gtk_widget_show (preview_image);

  return frame;
}


static gboolean
solid_noise_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *seed_hbox;
  GtkWidget *spinbutton;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init ("snoise", FALSE);

  /*  Dialog initialization  */
  dlg = gimp_dialog_new (_("Solid Noise"), "snoise",
                         NULL, 0,
                         gimp_standard_help_func, "plug-in-solid-noise",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = snoise_preview_new ();
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  Table  */
  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  Random Seed  */
  seed_hbox = gimp_random_seed_new (&snvals.seed, &snvals.random_seed);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                     _("_Random seed:"), 1.0, 0.5,
                                     seed_hbox, 1, TRUE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label),
                                 GIMP_RANDOM_SEED_SPINBUTTON (seed_hbox));
  g_signal_connect_swapped (GIMP_RANDOM_SEED_SPINBUTTON_ADJ (seed_hbox),
                           "value_changed",
                            G_CALLBACK (solid_noise),
                            NULL);

  /*  Detail  */
  spinbutton = gimp_spin_button_new (&adj, snvals.detail,
                                     1, 15, 1, 3, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("_Detail:"), 0.0, 0.5,
                             spinbutton, 1, TRUE);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &snvals.detail);
  g_signal_connect_swapped (adj, "value_changed",
                            G_CALLBACK (solid_noise),
                            NULL);

  /*  Turbulent  */
  toggle = gtk_check_button_new_with_mnemonic ( _("T_urbulent"));
  gtk_table_attach (GTK_TABLE (table), toggle, 2, 3, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_FILL, 1, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), snvals.turbulent);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &snvals.turbulent);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (solid_noise),
                            NULL);

  /*  Tilable  */
  toggle = gtk_check_button_new_with_mnemonic ( _("T_ilable"));
  gtk_table_attach (GTK_TABLE (table), toggle, 2, 3, 1, 2,
                    GTK_SHRINK | GTK_FILL, GTK_FILL, 1, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), snvals.tilable);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &snvals.tilable);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (solid_noise),
                            NULL);

  /*  X Size  */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                              _("_X size:"), SCALE_WIDTH, 0,
                              snvals.xsize, SIZE_MIN, SIZE_MAX, 0.1, 1.0, 1,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &snvals.xsize);
  g_signal_connect_swapped (adj, "value_changed",
                            G_CALLBACK (solid_noise),
                            NULL);

  /*  Y Size  */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
                              _("_Y size:"), SCALE_WIDTH, 0,
                              snvals.ysize, SIZE_MIN, SIZE_MAX, 0.1, 1.0, 1,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &snvals.ysize);
  g_signal_connect_swapped (adj, "value_changed",
                            G_CALLBACK (solid_noise),
                            NULL);

  solid_noise (NULL);
  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}
