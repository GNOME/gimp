/*
   fuse - associative image reconstruction
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpmenu.h"
#include "libgimp/gimpwire.h"


/* Declare local functions. */
static void query(void);
static void run(char *name,
		int nparams,
		GParam * param,
		int *nreturn_vals,
		GParam ** return_vals);
static gint dialog();

static void doit(GDrawable *);

typedef unsigned long pixel;

typedef struct {
  pixel *pixels;
  int width, height, stride;
} image;


GPlugInInfo PLUG_IN_INFO =
{
  NULL, /* init_proc */
  NULL, /* quit_proc */
  query, /* query_proc */
  run, /* run_proc */
};

int run_flag = 0;


GtkWidget *dlg;

#define preview_size 200
GtkWidget *preview;


struct {
  gint32 input_image_id;
  int tile_size;
  int overlap;
  int order;
  int acceleration;
  int search_time;
} config = {
  -1,
  25,
  8,
  0,
  0,
  10
};



MAIN();

static void query()
{
  static GParamDef args[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (unused)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable"},
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof(args) / sizeof(args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure("plug_in_fuse",
			 "associative image reconstruction",
			 "uhm, image dissociation",
			 "Scott Draves",
			 "Scott Draves",
			 "1997",
			 "<Image>/Filters/Transforms/Fuse",
			 "RGB*",
			 PROC_PLUG_IN,
			 nargs, nreturn_vals,
			 args, return_vals);
}

static void run(char *name, int n_params, GParam * param, int *nreturn_vals,
		GParam ** return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  srandom(time(0));

  run_mode = param[0].data.d_int32;
  
  if (run_mode == RUN_NONINTERACTIVE) {
    status = STATUS_CALLING_ERROR;
  } else {
    gimp_get_data("plug_in_fuse", &config);

    drawable = gimp_drawable_get(param[2].data.d_drawable);

    if (run_mode == RUN_INTERACTIVE) {      
      if (!dialog()) {
	status = STATUS_EXECUTION_ERROR;
      }
    }
  }


  if (status == STATUS_SUCCESS) {

    if (gimp_drawable_color(drawable->id)) {
      gimp_progress_init("Fusing...");
      gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));

      doit(drawable);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush();
      gimp_set_data("plug_in_fuse", &config, sizeof(config));
    } else {
      status = STATUS_EXECUTION_ERROR;
    }
    gimp_drawable_detach(drawable);
  }

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}


double
compare(image *in, image *out) {
  int x, y;
  double r = 0.0;
  double t = 0.0;
  for (y = 0; y < in->height; y++) {
    pixel *p = in->pixels + y * in->stride;
    pixel *q = out->pixels + y * out->stride;
    for (x = 0; x < in->width; x++) {
      double rr, z;
      guchar *i = (guchar *) p;
      guchar *o = (guchar *) q;
      int d = i[0] - (int) o[0];
      z = i[3] * o[3];
      if (0 == i[3] || 0 == o[3])
	continue;
      rr = d * d;
      d = i[1] - o[1];
      rr += d * d;
      d = i[2] - o[2];
      rr += d * d;
      r += rr * z;
      t += z;
    }
  }

  if (t < 0.5)
    return -1.0;

  return r/t;
}

/* use bbox of src */
void
blit(image *dst, image *src) {
  int x, y;
  for (y = 0; y < src->height; y++) {
    pixel *p = dst->pixels + y * dst->stride;
    pixel *q = src->pixels + y * src->stride;
    for (x = 0; x < src->width; x++) {
      guchar *o = (guchar *) p;
      guchar *i = (guchar *) q;
      int s = i[3];
      if (255 == s)
	p[x] = q[x];
      else {
	o[0] = (o[0] * (255 - s) + i[0] * s) >> 8;
	o[1] = (o[1] * (255 - s) + i[1] * s) >> 8;
	o[2] = (o[2] * (255 - s) + i[2] * s) >> 8;
      }
    }
  }
}

void
source_match(image *in, image *out) {
  int i;
  double best_d = HUGE;
  image best_tile;

  if (0 == out->width || 0 == out->height)
    return;

  for (i = 0; i < 1+(config.search_time<<8); i++) {
    image tile;
    double d;

    tile.pixels =
      in->pixels + (random() % (in->width - out->width)) +
      in->stride * (random() % (in->height - out->height));
    tile.stride = in->stride;
    tile.width = out->width;
    tile.height = out->height;

    d = compare(&tile, out);
    if (d < 0.0) {
      best_tile = tile;
      break;
    }
    if (d < best_d) {
      best_d = d;
      best_tile = tile;
    }
  }
  blit(out, &best_tile);
}


void
do_fuse(image *in, image *out) {
  int i, parity = 0;
  double r, rad;
  double bo;
  int *prmute;

  bo = config.tile_size - config.overlap;

  if (config.tile_size >= in->width ||
      config.tile_size >= in->height)
    return;

  rad = sqrt(out->width * out->width +
	     out->height * out->height) / 2;

  prmute = malloc(sizeof(int) * (1 + rad * 2.0 * M_PI / bo));
      
  for (r = 0; r < rad; r += bo) {
    int n = 1 + r * 2.0 * M_PI / bo;
    for (i = 0; i < n; i++)
      prmute[i] = i;
    for (i = 0; i < 2*n; i++) {
      int i1 = random() % n;
      int i2 = random() % n;
      int t = prmute[i1];
      prmute[i1] = prmute[i2];
      prmute[i2] = t;
    }
    parity++;
    for (i = 0; i < n; i++) {
      image tile;
      int x, y, w, h;
      double theta = 2.0 * M_PI * prmute[i] / n;
      if (parity&1)
	theta += M_PI / n;
      w = config.tile_size;
      h = config.tile_size;
      x = (out->width - w) / 2 + r * cos(theta);
      y = (out->height - h) / 2 + r * sin(theta);

      /* clip */
      if (x < 0) {w += x; x = 0;}
      if (y < 0) {h += y; y = 0;}
      if (x > out->width - w) w = out->width - x;
      if (y > out->height - h) h = out->height - y;

      tile.pixels = out->pixels + y * out->stride + x;
      tile.stride = out->stride;
      tile.width = w;
      tile.height = h;

      source_match(in, &tile);
      if (0) {
	int y;
	for (y = 0; y < preview_size; y++)
	  gtk_preview_draw_row(GTK_PREVIEW (preview),
			       (guchar *)(out->pixels+y*out->stride),
			       0, y, preview_size);
      }
      gimp_progress_update((1-i/(double)n) * r * r / (rad * rad) +
			   i/(double)n * (r + bo) * (r + bo) / (rad * rad));
    }
  }
  free(prmute);
}

static void
doit(GDrawable * target_drawable) {
  image in, out;
  gint bytes;
  GPixelRgn pr;
  GDrawable *input_drawable;

  in.width = gimp_image_width(config.input_image_id);
  in.height = gimp_image_height(config.input_image_id);
  in.stride = in.width;


#if 0
  if (4 == bytes) {
    printf("using 4 channel image\n");
  }
  if (3 != bytes && 4 != bytes) {
    fprintf(stderr, "only works with three or four channels, not %d.\n", bytes);
    return;
  }
#endif

  /* allocate and initialize input buffer */

  in.pixels = (pixel *) malloc(in.width * in.height * 4);
  if (in.pixels == NULL) {
    fprintf(stderr, "cannot malloc %d bytes for input.\n", in.width * in.height * 4);
    return;
  }

  {
    int nlayers;
    gint32 *layer_ids;
    layer_ids = gimp_image_get_layers (config.input_image_id, &nlayers);
    if ((NULL == layer_ids) || (nlayers <= 0)) {
      fprintf(stderr, "fuse: error getting layer IDs\n");
      return;
    }
    input_drawable = gimp_drawable_get(layer_ids[0]);
    bytes = input_drawable->bpp;

    gimp_pixel_rgn_init(&pr, input_drawable,
			0, 0, in.width, in.height, FALSE, FALSE);
    if (4 == bytes)
      gimp_pixel_rgn_get_rect(&pr, (guchar *)in.pixels, 0, 0, in.width, in.height);
    else if (3 == bytes) {
      int i;
      guchar *tb = malloc (in.width * bytes);
      for (i = 0; i < in.height; i++) {
	int j;
	gimp_pixel_rgn_get_rect(&pr, tb, 0, i, in.width, 1);
	for (j = 0; j < in.width; j++) {
	  guchar *d = (guchar *) (in.pixels + (in.width * i) + j);
	  guchar *s = tb + j * bytes;
	  d[0] = s[0];
	  d[1] = s[1];
	  d[2] = s[2];
	  d[3] = 255;
	}
      }
      free(tb);
    } else if (1 == bytes) {
      int i;
      guchar *tb = malloc (in.width * bytes);
      for (i = 0; i < in.height; i++) {
	int j;
	gimp_pixel_rgn_get_rect(&pr, tb, 0, i, in.width, 1);
	for (j = 0; j < in.width; j++) {
	  guchar *d = (guchar *) (in.pixels + (in.width * i) + j);
	  guchar *s = tb + j * bytes;
	  d[0] = d[1] = d[2] = s[0];
	  d[3] = 255;
	}
      }
      free(tb);
    }
  }

  out.width = target_drawable->width;
  out.height = target_drawable->height;
  out.stride = out.width;

  out.pixels = (pixel *) malloc(out.width * out.height * 4);
  if (out.pixels == NULL) {
    fprintf(stderr, "cannot malloc %d bytes for output.\n",
	    out.width * out.height * 4);
    return;
  }

  if (1)
    do_fuse(&in, &out);
  else {
    int i, j;
    for (i = 0; i < out.height; i++)
      for (j = 0; j < out.width; j++)
	out.pixels[i * out.stride + j] = random();
  }
  

  gimp_pixel_rgn_init(&pr, target_drawable,
		      0, 0, out.width, out.height, FALSE, FALSE);

  if (4 == bytes) {
    gimp_pixel_rgn_set_rect(&pr, (guchar *)out.pixels, 0, 0, out.width, out.height);
  } else {
    int i;
    guchar *tb = malloc (out.width * bytes);
    for (i = 0; i < out.height; i++) {
      int j;
      for (j = 0; j < out.width; j++) {
	char *s = (char *) (out.pixels + (out.width * i) + j);
	char *d = tb + j * bytes;
	d[0] = s[0];
	d[1] = s[1];
	d[2] = s[2];
      }
      gimp_pixel_rgn_set_rect(&pr, tb, 0, i, out.width, 1);
    }
    free(tb);
  }
  gimp_drawable_flush(target_drawable);
  /*
  gimp_drawable_merge_shadow(target_drawable->id, TRUE);
  */
  gimp_drawable_update(target_drawable->id, 0, 0,
		       target_drawable->width, target_drawable->height);
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

static gint
not_indexed_constrain (gint32 image_id, gint32 drawable_id, gpointer data) {

  if ((gimp_image_width (image_id) < config.tile_size)
      && (gimp_image_width (image_id) < config.tile_size))
    return 0;
    
  return INDEXED != gimp_image_base_type (image_id);
}


static void
input_callback(gint32 id, gpointer data) {
  config.input_image_id = id;
}


static void
overlap_callback (GtkWidget *widget, gpointer   data)
{
  config.overlap = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
  if (config.overlap > config.tile_size)
    config.overlap = config.tile_size;
}

static void
tile_size_callback (GtkWidget *widget, gpointer   data)
{
  config.tile_size = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
  if (config.tile_size < 0)
    config.tile_size = 1;
}


static void
search_time_callback (GtkWidget *widget, gpointer   data)
{
  config.search_time = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
  if (config.search_time < 0)
    config.search_time = 0;
}


static gint dialog() {
  GtkWidget *button;
  GtkWidget *table;
  GtkWidget *box;
  GtkWidget *w;
  GtkWidget *frame;
  gchar **argv;
  gint argc;
  int row;
  guchar *color_cube;

  argc = 1;
  argv = g_new(gchar *, 1);
  argv[0] = g_strdup("fuse");

  gtk_init(&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
			      color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  dlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Fuse");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
		     (GtkSignalFunc) close_callback, NULL);

  button = gtk_button_new_with_label("Ok");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			    (GtkSignalFunc) gtk_widget_destroy,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show(button);

  box = GTK_DIALOG(dlg)->vbox;

  {
    GtkWidget *menu;
    GtkWidget *menuitem;
    GtkWidget *hbox;
    GtkWidget *option_menu = gtk_option_menu_new ();

    hbox = gtk_hbox_new (FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 10);
    gtk_widget_show(hbox);

    w = gtk_label_new("Tile Source:");
    gtk_misc_set_alignment(GTK_MISC(w), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 10);
    gtk_widget_show(w);

    gtk_box_pack_start(GTK_BOX(hbox), option_menu, TRUE, TRUE, 10);
    menu = gimp_image_menu_new(not_indexed_constrain, input_callback,
			       0, config.input_image_id);

    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
    gtk_widget_show (option_menu);
  }

  {
    GtkWidget *label;
    GtkWidget *entry;
    GtkWidget *hbox;
    char buffer[20];
    hbox = gtk_hbox_new (FALSE, 5);
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Tile Size: ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    entry = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    sprintf (buffer, "%d", config.tile_size);
    gtk_entry_set_text (GTK_ENTRY (entry), buffer);
    gtk_signal_connect (GTK_OBJECT (entry), "changed",
			(GtkSignalFunc) tile_size_callback,
			NULL);
    gtk_widget_show (entry);

    gtk_widget_show (hbox);
  }


  {
    GtkWidget *label;
    GtkWidget *entry;
    GtkWidget *hbox;
    char buffer[20];
    hbox = gtk_hbox_new (FALSE, 5);
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Overlap: ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    entry = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    sprintf (buffer, "%d", config.overlap);
    gtk_entry_set_text (GTK_ENTRY (entry), buffer);
    gtk_signal_connect (GTK_OBJECT (entry), "changed",
			(GtkSignalFunc) overlap_callback,
			NULL);
    gtk_widget_show (entry);

    gtk_widget_show (hbox);
  }


  {
    GtkWidget *label;
    GtkWidget *entry;
    GtkWidget *hbox;
    char buffer[20];
    hbox = gtk_hbox_new (FALSE, 5);
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Search Time: ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    entry = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    sprintf (buffer, "%d", config.search_time);
    gtk_entry_set_text (GTK_ENTRY (entry), buffer);
    gtk_signal_connect (GTK_OBJECT (entry), "changed",
			(GtkSignalFunc) search_time_callback,
			NULL);
    gtk_widget_show (entry);

    gtk_widget_show (hbox);
  }

  gtk_widget_show(dlg);
  gtk_main();
  gdk_flush();

  return run_flag;
}
