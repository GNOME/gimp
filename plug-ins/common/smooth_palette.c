/*
   smooth palette - derive smooth palette from image
   Copyright (C) 1997  Scott Draves <spot@cs.cmu.edu>

   The GIMP -- an image manipulation program
   Copyright (C) 1995 Spencer Kimball and Peter Mattis

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "config.h"
#include "libgimp/gimp.h"
#include "gtk/gtk.h"
#include "libgimp/stdplugins-intl.h"

/* Declare local functions. */
static void query(void);
static void run(char *name,
		int nparams,
		GParam * param,
		int *nreturn_vals,
		GParam ** return_vals);
static gint dialog();

static gint32 doit(GDrawable * drawable, gint32 *layer_id);

GPlugInInfo PLUG_IN_INFO =
{
  NULL, /* init_proc */
  NULL, /* quit_proc */
  query, /* query_proc */
  run, /* run_proc */
};

int run_flag = 0;


MAIN()


static void query()
{
  static GParamDef args[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (unused)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable"},
    {PARAM_INT32, "width", "Width"},
    {PARAM_INT32, "height", "Height"},
    {PARAM_INT32, "ntries", "Search Time"},
    {PARAM_INT32, "show_image","Show Image?"},
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof(args) / sizeof(args[0]);
  static int nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure("plug_in_smooth_palette",
			 _("derive smooth palette from image"),
			 "help!",
			 "Scott Draves",
			 "Scott Draves",
			 "1997",
			 N_("<Image>/Filters/Colors/Smooth Palette..."),
			 "RGB*",
			 PROC_PLUG_IN,
			 nargs, nreturn_vals,
			 args, return_vals);
}


static struct {
  gint width;
  gint height;
  gint ntries;
  gint try_size;
  gint show_image;
} config = {
  256,
  64,
  50,
  10000,
  1
};


static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[3];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  GDrawable *drawable;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 3;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  values[1].type = PARAM_IMAGE;
  values[2].type = PARAM_LAYER;

  switch (run_mode) {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_get_data ("plug_in_smooth_palette", &config);
      if (! dialog ()) return;
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      if (nparams != 7)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  config.width = param[3].data.d_int32;
	  config.height = param[4].data.d_int32;
	  config.ntries = param[5].data.d_int32;
	  config.show_image = (param[6].data.d_int32) ? TRUE : FALSE;
	}
      if (status == STATUS_SUCCESS && 
	  ((config.width <= 0) || (config.height <= 0) || config.ntries <= 0))
	status = STATUS_CALLING_ERROR;
      
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_smooth_palette", &config);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS) {
    drawable = gimp_drawable_get(param[2].data.d_drawable);
    if (gimp_drawable_is_rgb(drawable->id)) {
      gimp_progress_init ( _("Deriving smooth palette..."));
      gimp_tile_cache_ntiles (2 * (drawable->width + 1) / gimp_tile_width ());
      values[1].data.d_image = doit(drawable, &values[2].data.d_layer);
      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_smooth_palette", &config, sizeof (config));
      if (config.show_image)
	gimp_display_new (values[1].data.d_image);
    } else
      status = STATUS_EXECUTION_ERROR;
    gimp_drawable_detach(drawable);
  }

  values[0].data.d_status = status;
}

#define R (rand())

static long
pix_diff(guchar *pal, int bpp, int i, int j) {
  long r = 0;
  int k;

  for (k = 0; k < bpp; k++) {
    int p1 = pal[j * bpp + k];
    int p2 = pal[i * bpp + k];
    r += (p1 - p2) * (p1 - p2);
  }
  return r;
}

static void
pix_swap(guchar *pal, int bpp, int i, int j) {
  int k;
  for (k = 0; k < bpp; k++) {
    guchar t = pal[j * bpp + k];
    pal[j * bpp + k] = pal[i * bpp + k];
    pal[i * bpp + k] = t;
  }
}

static gint32
doit(GDrawable * drawable, gint32 *layer_id) {
  gint32 new_image_id;
  GDrawable *new_layer;
  int psize, i, j;
  guchar *pal;
  int bpp = drawable->bpp;
  GPixelRgn pr;

  srand(time(0));

  new_image_id = gimp_image_new (config.width, config.height, RGB);
  *layer_id = gimp_layer_new (new_image_id, _("Background"),
			      config.width, config.height,
			      gimp_drawable_type (drawable->id),
			      100, NORMAL_MODE);
  gimp_image_add_layer (new_image_id, *layer_id, 0);
  new_layer = gimp_drawable_get (*layer_id);

  psize = config.width;

  pal = malloc(psize * bpp);

  gimp_pixel_rgn_init(&pr, drawable, 0, 0, drawable->width,
		      drawable->height,
		      FALSE, FALSE);

  /* get initial palette */
  for (i = 0; i < psize; i++) {
    int x = R % drawable->width;
    int y = R % drawable->height;
    gimp_pixel_rgn_get_pixel(&pr, pal + bpp * i, x, y);
  }

  /* reorder */
  if (1) {
    guchar *pal_best = malloc(psize * bpp);
    guchar *original = malloc(psize * bpp);
    double len_best = 0;
    int try;

    memcpy(pal_best, pal, bpp * psize);
    memcpy(original, pal, bpp * psize);

    for (try = 0; try < config.ntries; try++) {
      double len;
      if (!(try%5))
	gimp_progress_update(try/(double)config.ntries);
      memcpy(pal, original, bpp * psize);

      /* scramble */
      for (i = 1; i < psize; i++)
	pix_swap(pal, bpp, i, R % psize);

      /* measure */
      len = 0.0;
      for (i = 1; i < psize; i++)
	len += pix_diff(pal, bpp, i, i-1);

      /* improve */
      for (i = 0; i < config.try_size; i++) {
	int i0 = 1 + (R % (psize-2));
	int i1 = 1 + (R % (psize-2));
	long as_is, swapd;
	if (1 == (i0 - i1)) {
	  as_is = (pix_diff(pal, bpp, i1 - 1, i1) +
		   pix_diff(pal, bpp, i0, i0 + 1));
	  swapd = (pix_diff(pal, bpp, i1 - 1, i0) +
		   pix_diff(pal, bpp, i1, i0 + 1));
	} else if (1 == (i1 - i0)) {
	  as_is = (pix_diff(pal, bpp, i0 - 1, i0) +
		   pix_diff(pal, bpp, i1, i1 + 1));
	  swapd = (pix_diff(pal, bpp, i0 - 1, i1) +
		   pix_diff(pal, bpp, i0, i1 + 1));
	} else {
	  as_is = (pix_diff(pal, bpp, i0, i0 + 1) +
		   pix_diff(pal, bpp, i0, i0 - 1) +
		   pix_diff(pal, bpp, i1, i1 + 1) +
		   pix_diff(pal, bpp, i1, i1 - 1));
	  swapd = (pix_diff(pal, bpp, i1, i0 + 1) +
		   pix_diff(pal, bpp, i1, i0 - 1) +
		   pix_diff(pal, bpp, i0, i1 + 1) +
		   pix_diff(pal, bpp, i0, i1 - 1));
	}
	if (swapd < as_is) {
	  pix_swap(pal, bpp, i0, i1);
	  len += swapd - as_is;
	}
      }
      /* best? */
      if (0 == try || len < len_best) {
	memcpy(pal_best, pal, bpp * psize);
	len_best = len;
      }
    }
    memcpy(pal, pal_best, bpp * psize);
    free(pal_best);
    free(original);
    /* clean */
    for (i = 1; i < 4 * psize; i++) {
      long as_is, swapd;
      int i0 = 1 + R % (psize - 2);
      int i1 = i0 + 1;
      as_is = (pix_diff(pal, bpp, i0 - 1, i0) +
	       pix_diff(pal, bpp, i1, i1 + 1));
      swapd = (pix_diff(pal, bpp, i0 - 1, i1) +
	       pix_diff(pal, bpp, i0, i1 + 1));
      if (swapd < as_is) {
	pix_swap(pal, bpp, i0, i1);
	len_best += swapd - as_is;
      }
    }
  }

  /* store smooth palette */
  gimp_pixel_rgn_init(&pr, new_layer, 0, 0,
		      config.width, config.height,
		      TRUE, TRUE);
  for (j = 0; j < config.height; j++)
    for (i = 0; i < config.width; i++)
      gimp_pixel_rgn_set_pixel(&pr, pal + bpp * i, i, j);
  free(pal);

  gimp_drawable_flush(new_layer);
  gimp_drawable_merge_shadow(new_layer->id, TRUE);
  gimp_drawable_update(new_layer->id, 0, 0,
		       config.width, config.height);

  return new_image_id;
}


static void close_callback(GtkWidget * widget, gpointer data)
{
  gtk_main_quit();
}

static void ok_callback(GtkWidget * widget, gpointer data)
{
  run_flag = 1;
  gtk_widget_destroy(GTK_WIDGET(data));
}

static void callback(GtkWidget * widget, gpointer data)
{
  if (&config.width == data) {
    config.width = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
  } else if (&config.height == data) {
    config.height = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
  } else if (&config.ntries == data) {
    config.ntries = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
  } else {
    fprintf(stderr, "bad data in callback: %x\n", (int) data);
  }
}

static gint dialog()
{
  GtkWidget *dlg;
  GtkWidget *hbbox;
  GtkWidget *button;
  GtkWidget *table;
  GtkWidget *w;
  gchar **argv;
  gint argc;
  char b[12];

  argc = 1;
  argv = g_new(gchar *, 1);
  argv[0] = g_strdup("smooth palette");

  gtk_init(&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), _("Smooth Palette"));
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
		     (GtkSignalFunc) close_callback, NULL);

  /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dlg)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
 
  button = gtk_button_new_with_label ( _("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( _("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  table = gtk_table_new(4, 4, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show(table);

  gtk_table_set_row_spacings(GTK_TABLE(table), 10);
  gtk_table_set_col_spacings(GTK_TABLE(table), 10);

  {
    w = gtk_label_new( _("Width:"));
    gtk_misc_set_alignment(GTK_MISC(w), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), w, 0, 2, 0, 1,
		     GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_widget_show(w);

    w = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), w, 2, 4, 0, 1,
		     GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_widget_set_usize(w, 50, 0);
    sprintf(b, "%d", config.width);
    gtk_entry_set_text(GTK_ENTRY(w), b);
    gtk_signal_connect(GTK_OBJECT(w), "changed",
		       (GtkSignalFunc) callback, &config.width);
    gtk_widget_show(w);
  }
  {
    w = gtk_label_new( _("Height:"));
    gtk_misc_set_alignment(GTK_MISC(w), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), w, 0, 2, 2, 3,
		     GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_widget_show(w);

    w = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), w, 2, 4, 2, 3,
		     GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_widget_set_usize(w, 50, 0);
    sprintf(b, "%d", config.height);
    gtk_entry_set_text(GTK_ENTRY(w), b);
    gtk_signal_connect(GTK_OBJECT(w), "changed",
		       (GtkSignalFunc) callback, &config.height);
    gtk_widget_show(w);
  }
  {
    w = gtk_label_new( _("Search Time:"));
    gtk_misc_set_alignment(GTK_MISC(w), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), w, 0, 2, 3, 4,
		     GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_widget_show(w);

    w = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), w, 2, 4, 3, 4,
		     GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_widget_set_usize(w, 50, 0);
    sprintf(b, "%d", config.ntries);
    gtk_entry_set_text(GTK_ENTRY(w), b);
    gtk_signal_connect(GTK_OBJECT(w), "changed",
		       (GtkSignalFunc) callback, &config.ntries);
    gtk_widget_show(w);
  }

  gtk_widget_show(dlg);
  gtk_main();
  gdk_flush();

  return run_flag;
}
