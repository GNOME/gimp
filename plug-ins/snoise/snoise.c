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


#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "gtk/gtk.h"
#include "libgimp/gimp.h"


/*---- Defines ----*/

#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif /* RAND_MAX */

#define TABLE_SIZE 64
#define WEIGHT(T) ((2.0*fabs(T)-3.0)*(T)*(T)+1.0)

#define ENTRY_WIDTH 40
#define SCALE_WIDTH 128
#define SCALE_MAX 16.0


/*---- Typedefs ----*/

typedef struct {
  gint    tilable;
  gint    turbulent;
  gint    seed;
  gint    detail;
  gdouble xsize;
  gdouble ysize;
  /*  Interface only  */
  gint    timeseed;
} SolidNoiseValues;

typedef struct {
  gint run;
} SolidNoiseInterface;

typedef struct {
  double x, y;
} Vector2d;


/*---- Prototypes ----*/

static void query (void);
static void run (char *name, int nparams, GParam  *param,
                 int *nreturn_vals, GParam **return_vals);

static void solid_noise (GDrawable *drawable);
static void solid_noise_init (void);
static double plain_noise (double x, double y, unsigned int s);
static double noise (double x, double y);

static gint solid_noise_dialog (void);
static void dialog_close_callback (GtkWidget *widget, gpointer data);
static void dialog_toggle_update (GtkWidget *widget, gpointer data);
static void dialog_entry_callback (GtkWidget *widget, gpointer data);
static void dialog_scale_callback (GtkAdjustment *adjustment, gdouble *value);
static void dialog_ok_callback (GtkWidget *widget, gpointer data);


/*---- Variables ----*/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};

static SolidNoiseValues snvals = {
  0,   /* tilable */
  0,   /* turbulent */
  1,   /* seed */
  1,   /* detail */
  4.0, /* xsize */
  4.0, /* ysize */
  0    /* use time seed */ 
};

static SolidNoiseInterface snint = {
  FALSE /* run */
};

static int xclip, yclip;
static double offset, factor;
static double xsize, ysize;
static int perm_tab[TABLE_SIZE];
static Vector2d grad_tab[TABLE_SIZE];


/*---- Functions ----*/

MAIN()


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
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;
  
  gimp_install_procedure ("plug_in_solid_noise",
			  "Creates a grayscale noise texture",
			  "Generates 2D textures using Perlin's classic solid noise function.",
			  "Marcelo de Gomensoro Malheiros",
			  "Marcelo de Gomensoro Malheiros",
			  "Apr 1998, v1.03",
			  "<Image>/Filters/Render/Solid Noise",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs,
                          nreturn_vals,
			  args,
                          return_vals);
}


static void
run (char *name, int nparams, GParam *param, int *nreturn_vals,
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
  switch (run_mode) {
  case RUN_INTERACTIVE:
    /*  Possibly retrieve data  */
    gimp_get_data("plug_in_solid_noise", &snvals);

    /*  Get information from the dialog  */
    if (!solid_noise_dialog ())
      return;

    break;

  case RUN_NONINTERACTIVE:
    /*  Test number of arguments  */
    if (nparams == 9) {
      snvals.tilable = param[3].data.d_int32;
      snvals.turbulent = param[4].data.d_int32;
      snvals.seed = param[5].data.d_int32;
      snvals.detail = param[6].data.d_int32;
      snvals.xsize = param[7].data.d_float;
      snvals.ysize = param[8].data.d_float;
    }
    else
      status = STATUS_CALLING_ERROR;
    break;

  case RUN_WITH_LAST_VALS:
    /*  Possibly retrieve data  */
    gimp_get_data("plug_in_solid_noise", &snvals);
    break;
    
  default:
    break;
  }
  
  /*  Create texture  */
  if ((status == STATUS_SUCCESS) && (gimp_drawable_color (drawable->id) ||
      gimp_drawable_gray (drawable->id)))
    {
      /*  Set the tile cache size  */
      gimp_tile_cache_ntiles((drawable->width + gimp_tile_width() - 1) / gimp_tile_width());

      /*  Run!  */
      solid_noise (drawable);

      /*  If run mode is interactive, flush displays  */
      if (run_mode != RUN_NONINTERACTIVE)
        gimp_displays_flush();

      /*  Store data  */
      if (run_mode==RUN_INTERACTIVE || run_mode==RUN_WITH_LAST_VALS)
        gimp_set_data("plug_in_solid_noise", &snvals,
                      sizeof(SolidNoiseValues));
    }
  else
    {
      /* gimp_message ("solid noise: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }  
  
  values[0].data.d_status = status;
  
  gimp_drawable_detach(drawable);
}


static void
solid_noise (GDrawable *drawable)
{
  GPixelRgn dest_rgn;
  gint chns, i, has_alpha, row, col;
  gint sel_x1, sel_y1, sel_x2, sel_y2;
  gint sel_width, sel_height;
  gint progress, max_progress;
  gpointer pr;
  guchar *dest, *dest_row;
  guchar val;

  /*  Get selection area  */
  gimp_drawable_mask_bounds (drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);
  sel_width = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  /*  Initialization  */
  solid_noise_init ();
  gimp_progress_init ("Solid Noise...");
  progress = 0;
  max_progress = sel_width * sel_height;
  chns = gimp_drawable_bpp (drawable->id);
  has_alpha = 0;
  if (gimp_drawable_has_alpha (drawable->id)) {
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
  int i, j, k, t;
  double m;

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
  if (snvals.tilable) {
    xsize = ceil (snvals.xsize);
    ysize = ceil (snvals.ysize);
    xclip = (int) xsize;
    yclip = (int) ysize;
  }
  else {
    xsize = snvals.xsize;
    ysize = snvals.ysize;
  }

  /*  Set totally empiric normalization values  */
  if (snvals.turbulent) {
    offset=0.0;
    factor=1.0;
  }
  else {
    offset=0.94;
    factor=0.526;
  }
  
  /*  Initialize the permutation table  */
  for (i = 0; i < TABLE_SIZE; i++)
    perm_tab[i] = i;
  for (i = 0; i < (TABLE_SIZE >> 1); i++) {
    j = rand () % TABLE_SIZE;
    k = rand () % TABLE_SIZE;
    t = perm_tab[j];
    perm_tab[j] = perm_tab[k];
    perm_tab[k] = t;
  }
  
  /*  Initialize the gradient table  */
  for (i = 0; i < TABLE_SIZE; i++) {
    do {
      grad_tab[i].x = (double)(rand () - (RAND_MAX >> 1)) / (RAND_MAX >> 1);
      grad_tab[i].y = (double)(rand () - (RAND_MAX >> 1)) / (RAND_MAX >> 1);
      m = grad_tab[i].x * grad_tab[i].x + grad_tab[i].y * grad_tab[i].y;
    } while (m == 0.0 || m > 1.0);
    m = 1.0 / sqrt(m);
    grad_tab[i].x *= m;
    grad_tab[i].y *= m;
  }
}


static double
plain_noise (double x, double y, unsigned int s)
{
  Vector2d v;
  int a, b, i, j, n;
  double sum;

  sum = 0.0;
  x *= s;
  y *= s;
  a = (int) floor (x);
  b = (int) floor (y);

  for (i = 0; i < 2; i++)
    for (j = 0; j < 2; j++) {
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


static double
noise (double x, double y)
{
  int i;
  unsigned int s;
  double sum;

  s = 1;
  sum = 0.0;
  x *= xsize;
  y *= ysize;
  
  for (i = 0; i <= snvals.detail; i++) {
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
  GtkWidget *toggle;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *seed_hbox;
  GtkWidget *time_button;
  GtkWidget *scale;
  GtkObject *scale_data;
  gchar **argv;
  gint  argc;
  gchar buffer[32];

  /*  Set args  */
  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("snoise");
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  /*  Dialog initialization  */
  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Solid Noise");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      (GtkSignalFunc) dialog_close_callback, NULL);

  /*  Table  */
  table = gtk_table_new (4, 3, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);

  /*  Entry #1  */
  label = gtk_label_new ("Seed");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_FILL, GTK_FILL, 1, 0);
  gtk_widget_show (label);
  
  seed_hbox = gtk_hbox_new (FALSE, 2);
  gtk_table_attach (GTK_TABLE (table), seed_hbox, 1, 2, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (seed_hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%d", snvals.seed);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      (GtkSignalFunc) dialog_entry_callback, &snvals.seed);
  gtk_widget_show (entry);

  time_button = gtk_toggle_button_new_with_label ("Time");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(time_button), snvals.timeseed);
  gtk_signal_connect (GTK_OBJECT (time_button), "toggled",
		      (GtkSignalFunc) dialog_toggle_update,
		      &snvals.timeseed);
  gtk_box_pack_end (GTK_BOX (seed_hbox), time_button, FALSE, FALSE, 0);
  gtk_widget_show (time_button);
  gtk_widget_show (seed_hbox);

  /*  Entry #2  */
  label = gtk_label_new ("Detail");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_FILL, GTK_FILL, 1, 0);
  gtk_widget_show (label);
  
  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%d", snvals.detail);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      (GtkSignalFunc) dialog_entry_callback, &snvals.detail);
  gtk_widget_show (entry);

  /*  Check button #1  */
  toggle = gtk_check_button_new_with_label ("Turbulent");
  gtk_table_attach (GTK_TABLE (table), toggle, 2, 3, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 1, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), snvals.turbulent);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) dialog_toggle_update, &snvals.turbulent);
  gtk_widget_show (toggle);
  
  /*  Check button #2  */
  toggle = gtk_check_button_new_with_label ("Tilable");
  gtk_table_attach (GTK_TABLE (table), toggle, 2, 3, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 1, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), snvals.tilable);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) dialog_toggle_update, &snvals.tilable);
  gtk_widget_show (toggle);
  
  /*  Scale #1  */
  label = gtk_label_new ("X Size");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_FILL, GTK_FILL, 1, 0);
  gtk_widget_show (label);

  scale_data = gtk_adjustment_new (snvals.xsize, 0.1, SCALE_MAX,
                                   0.1, 0.1, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 3, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 1);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) dialog_scale_callback, &snvals.xsize);
  gtk_widget_show (scale);

  /*  Scale #2  */
  label = gtk_label_new ("Y Size");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                    GTK_FILL, GTK_FILL, 1, 0);
  gtk_widget_show (label);

  scale_data = gtk_adjustment_new (snvals.ysize, 0.1, SCALE_MAX,
                                   0.1, 0.1, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 3, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 1);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) dialog_scale_callback, &snvals.ysize);
  gtk_widget_show (scale);

  /*  Button #1  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) dialog_ok_callback, dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  /*  Button #2  */
  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
                             GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_show (button);

  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return snint.run;
}


static void
dialog_close_callback (GtkWidget *widget, gpointer data)
{
  gtk_main_quit ();
}


static void
dialog_toggle_update (GtkWidget *widget, gpointer data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}


static void
dialog_entry_callback (GtkWidget *widget, gpointer data)
{
  gint *text_val;

  text_val = (gint *) data;

  *text_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
}


static void
dialog_scale_callback (GtkAdjustment *adjustment, gdouble *value)
{
  *value = adjustment->value;
}


static void
dialog_ok_callback (GtkWidget *widget, gpointer data)
{
  snint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}
