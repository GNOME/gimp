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

/* Solid Noise plug-in version 1.03, Apr 1998
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

/* Version 1.03:
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

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/*---- Defines ----*/

#define TABLE_SIZE 64
#define WEIGHT(T) ((2.0*fabs(T)-3.0)*(T)*(T)+1.0)

#define SCALE_WIDTH 128
#define SCALE_MAX    16.0


/*---- Typedefs ----*/

typedef struct
{
  gint    tilable;
  gint    turbulent;
  gint    seed;
  gint    detail;
  gdouble xsize;
  gdouble ysize;
  /*  Interface only  */
  gint    timeseed;
} SolidNoiseValues;

typedef struct
{
  gint run;
} SolidNoiseInterface;


/*---- Prototypes ----*/

static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GParam  *param,
		   gint    *nreturn_vals,
		   GParam **return_vals);

static void    solid_noise      (GDrawable *drawable);
static void    solid_noise_init (void);
static gdouble plain_noise      (gdouble    x,
				 gdouble    y,
				 guint      s);
static gdouble noise            (gdouble    x,
				 gdouble    y);

static gint    solid_noise_dialog    (void);
static void    dialog_ok_callback    (GtkWidget *widget,
				      gpointer   data);


/*---- Variables ----*/

GPlugInInfo PLUG_IN_INFO =
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
  1,   /* seed          */
  1,   /* detail        */
  4.0, /* xsize         */
  4.0, /* ysize         */
  0    /* use time seed */ 
};

static SolidNoiseInterface snint =
{
  FALSE /* run */
};

static gint        xclip, yclip;
static gdouble     offset, factor;
static gdouble     xsize, ysize;
static gint        perm_tab[TABLE_SIZE];
static GimpVector2 grad_tab[TABLE_SIZE];


/*---- Functions ----*/

MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT32, "tilable", "Create a tilable output (n=0/y=1)" },
    { PARAM_INT32, "turbulent", "Make a turbulent noise (n=0/y=1)" },
    { PARAM_INT32, "seed", "Random seed" },
    { PARAM_INT32, "detail", "Detail level (0 - 15)" },
    { PARAM_FLOAT, "xsize", "Horizontal texture size" },
    { PARAM_FLOAT, "ysize", "Vertical texture size" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("plug_in_solid_noise",
			  "Creates a grayscale noise texture",
			  "Generates 2D textures using Perlin's classic solid noise function.",
			  "Marcelo de Gomensoro Malheiros",
			  "Marcelo de Gomensoro Malheiros",
			  "Apr 1998, v1.03",
			  N_("<Image>/Filters/Render/Clouds/Solid Noise..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, 0,
			  args, NULL);
}


static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status;
  
  status = STATUS_SUCCESS;
  run_mode = param[0].data.d_int32;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;
  
  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  See how we will run  */
  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      /*  Possibly retrieve data  */
      gimp_get_data("plug_in_solid_noise", &snvals);

      /*  Get information from the dialog  */
      if (!solid_noise_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Test number of arguments  */
      if (nparams != 9)
	{
	  status = STATUS_CALLING_ERROR;
	}
      else
	{
	  snvals.tilable   = param[3].data.d_int32;
	  snvals.turbulent = param[4].data.d_int32;
	  snvals.seed      = param[5].data.d_int32;
	  snvals.detail    = param[6].data.d_int32;
	  snvals.xsize     = param[7].data.d_float;
	  snvals.ysize     = param[8].data.d_float;
	}
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_solid_noise", &snvals);
      break;

    default:
      break;
    }
  
  /*  Create texture  */
  if ((status == STATUS_SUCCESS) && (gimp_drawable_is_rgb (drawable->id) ||
				     gimp_drawable_is_gray (drawable->id)))
    {
      /*  Set the tile cache size  */
      gimp_tile_cache_ntiles ((drawable->width + gimp_tile_width () - 1) /
			      gimp_tile_width ());

      /*  Run!  */
      solid_noise (drawable);

      /*  If run mode is interactive, flush displays  */
      if (run_mode != RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == RUN_INTERACTIVE || run_mode == RUN_WITH_LAST_VALS)
        gimp_set_data ("plug_in_solid_noise", &snvals,
		       sizeof (SolidNoiseValues));
    }
  else
    {
      /* gimp_message ("solid noise: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }  

  values[0].data.d_status = status;
  
  gimp_drawable_detach (drawable);
}


static void
solid_noise (GDrawable *drawable)
{
  GPixelRgn dest_rgn;
  gint      chns, i, has_alpha, row, col;
  gint      sel_x1, sel_y1, sel_x2, sel_y2;
  gint      sel_width, sel_height;
  gint      progress, max_progress;
  gpointer  pr;
  guchar   *dest, *dest_row;
  guchar    val;

  /*  Get selection area  */
  gimp_drawable_mask_bounds (drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);
  sel_width = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  /*  Initialization  */
  solid_noise_init ();
  gimp_progress_init ( _("Solid Noise..."));
  progress = 0;
  max_progress = sel_width * sel_height;
  chns = gimp_drawable_bpp (drawable->id);
  has_alpha = 0;
  if (gimp_drawable_has_alpha (drawable->id))
    {
      chns--;
      has_alpha = 1;
    }
  gimp_pixel_rgn_init (&dest_rgn, drawable, sel_x1, sel_y1, sel_width,
                       sel_height, TRUE, TRUE);

  /*  One, two, three, go!  */
  for (pr = gimp_pixel_rgns_register (1, &dest_rgn); pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      dest_row = dest_rgn.data;

      for (row = dest_rgn.y; row < (dest_rgn.y + dest_rgn.h); row++)
        {
          dest = dest_row;
          
          for (col = dest_rgn.x; col < (dest_rgn.x + dest_rgn.w); col++)
            {
              val = (guchar) floor (255.0 * noise ((double) (col - sel_x1) / sel_width, (double) (row - sel_y1) / sel_height));
              for (i = 0; i < chns; i++)
                *dest++ = val;
              if (has_alpha)
                *dest++ = 255;
            }

          dest_row += dest_rgn.rowstride;;
        }

      /*  Update progress  */
      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / (double) max_progress);
    }

  /*  Update the drawable  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, sel_x1, sel_y1, sel_width, sel_height);
}


static void
solid_noise_init (void)
{
  gint    i, j, k, t;
  gdouble m;

  /*  Force sane parameters  */
  if (snvals.detail < 0)
    snvals.detail = 0;
  if (snvals.detail > 15)
    snvals.detail = 15;
  if (snvals.seed < 0)
    snvals.seed = 0;
  
  /*  Define the pseudo-random number generator seed  */
  if (snvals.timeseed)
    snvals.seed = time(NULL);
  srand (snvals.seed);

  /*  Set scaling factors  */
  if (snvals.tilable)
    {
      xsize = ceil (snvals.xsize);
      ysize = ceil (snvals.ysize);
      xclip = (int) xsize;
      yclip = (int) ysize;
    }
  else
    {
      xsize = snvals.xsize;
      ysize = snvals.ysize;
    }

  /*  Set totally empiric normalization values  */
  if (snvals.turbulent)
    {
      offset=0.0;
      factor=1.0;
    }
  else
    {
      offset=0.94;
      factor=0.526;
    }
  
  /*  Initialize the permutation table  */
  for (i = 0; i < TABLE_SIZE; i++)
    perm_tab[i] = i;
  for (i = 0; i < (TABLE_SIZE >> 1); i++)
    {
      j = rand () % TABLE_SIZE;
      k = rand () % TABLE_SIZE;
      t = perm_tab[j];
      perm_tab[j] = perm_tab[k];
      perm_tab[k] = t;
    }
  
  /*  Initialize the gradient table  */
  for (i = 0; i < TABLE_SIZE; i++)
    {
      do
	{
	  grad_tab[i].x =
	    (double)(rand () - (G_MAXRAND >> 1)) / (G_MAXRAND >> 1);
	  grad_tab[i].y =
	    (double)(rand () - (G_MAXRAND >> 1)) / (G_MAXRAND >> 1);
	  m = grad_tab[i].x * grad_tab[i].x + grad_tab[i].y * grad_tab[i].y;
	}
      while (m == 0.0 || m > 1.0);

      m = 1.0 / sqrt(m);
      grad_tab[i].x *= m;
      grad_tab[i].y *= m;
    }
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


static gint
solid_noise_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *table;
  GtkWidget *seed_hbox;
  GtkWidget *spinbutton;
  GtkObject *adj;

  gimp_ui_init ("snoise", FALSE);

  /*  Dialog initialization  */
  dlg = gimp_dialog_new (_("Solid Noise"), "snoise",
			 gimp_plugin_help_func, "filters/snoise.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), dialog_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  gimp_help_init ();

  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  Table  */
  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /*  Random Seed  */
  seed_hbox = gimp_random_seed_new (&snvals.seed,
				    &snvals.timeseed,
				    TRUE, FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Random Seed:"), 1.0, 0.5,
			     seed_hbox, 1, TRUE);

  /*  Detail  */
  spinbutton = gimp_spin_button_new (&adj, snvals.detail,
				     1, 15, 1, 3, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Detail:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &snvals.detail);

  /*  Turbulent  */
  toggle = gtk_check_button_new_with_label ( _("Turbulent"));
  gtk_table_attach (GTK_TABLE (table), toggle, 2, 3, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_FILL, 1, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), snvals.turbulent);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &snvals.turbulent);
  gtk_widget_show (toggle);

  /*  Tilable  */
  toggle = gtk_check_button_new_with_label ( _("Tilable"));
  gtk_table_attach (GTK_TABLE (table), toggle, 2, 3, 1, 2,
                    GTK_SHRINK | GTK_FILL, GTK_FILL, 1, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), snvals.tilable);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &snvals.tilable);
  gtk_widget_show (toggle);

  /*  X Size  */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("X Size:"), SCALE_WIDTH, 0,
			      snvals.xsize, 0.1, SCALE_MAX, 0.1, 1.0, 1,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &snvals.xsize);

  /*  Y Size  */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
			      _("Y Size:"), SCALE_WIDTH, 0,
			      snvals.ysize, 0.1, SCALE_MAX, 0.1, 1.0, 1,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &snvals.ysize);

  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gimp_help_free ();
  gdk_flush ();

  return snint.run;
}

static void
dialog_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  snint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
