/*
   flame - cosmic recursive fractal flames
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <string.h>
#include <time.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"
#include "libgimp/stdplugins-intl.h"

#include "megawidget.h"

#include "libifs.h"
#include "rect.h"
#include "cmap.h"

#define variation_same   (-2)

/* Declare local functions. */
static void query(void);
static void run(char *name,
		int nparams,
		GParam * param,
		int *nreturn_vals,
		GParam ** return_vals);
static gint dialog();

static void doit(GDrawable * drawable);

static void set_flame_preview();
static void load_callback(GtkWidget * widget, gpointer data);
static void store_callback(GtkWidget * widget, gpointer data);
static void set_edit_preview();
static void menu_cb(GtkWidget * widget, gpointer data);
static void my_mw_update_cb(gpointer data);
static void init_mutants();

char buffer[10000];
GtkWidget *cmap_preview;
GtkWidget *flame_preview;
int preview_width, preview_height;
GtkWidget *dlg;
static GtkWidget *file_dlg = 0;
static int load_store;

GtkWidget *edit_dlg = 0;

#define preview_size 150
#define edit_preview_size 85
#define nmutants 9

control_point edit_cp;
control_point mutants[nmutants];
GtkWidget *edit_previews[nmutants];
double pick_speed = 0.2;


GPlugInInfo PLUG_IN_INFO =
{
  NULL, /* init_proc */
  NULL, /* quit_proc */
  query, /* query_proc */
  run, /* run_proc */
};

int run_flag = 0;

#define black_drawable (-2)
#define gradient_drawable (-3)
#define table_drawable (-4)



struct {
  int randomize;  /* superseded */
  int variation;
  gint32 cmap_drawable;
  control_point cp;
} config;

frame_spec f = {0.0, &config.cp, 1, 0.0};


MAIN()


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

  INIT_I18N();
  gimp_install_procedure("plug_in_flame",
			 _("cosmic recursive fractal flames"),
			 _("use Smooth Palette to make colormaps"),
			 "Scott Draves",
			 "Scott Draves",
			 "1997",
			 N_("<Image>/Filters/Render/Nature/Flame..."),
			 "RGB*",
			 PROC_PLUG_IN,
			 nargs, nreturn_vals,
			 args, return_vals);
}

static void maybe_init_cp() {
  if (0 == config.cp.spatial_oversample) {
    config.randomize = 0;
    config.variation = variation_same;
    config.cmap_drawable = gradient_drawable;
    random_control_point(&config.cp, variation_random);
    config.cp.center[0] = 0.0;
    config.cp.center[1] = 0.0;
    config.cp.pixels_per_unit = 100;
    config.cp.spatial_oversample = 2;
    config.cp.gamma = 2.0;
    config.cp.contrast = 1.0;
    config.cp.brightness = 1.0;
    config.cp.spatial_filter_radius = 0.75;
    config.cp.sample_density = 5.0;
    config.cp.zoom = 0.0;
    config.cp.nbatches = 1;
    config.cp.white_level = 200;
    config.cp.cmap_index = 72;
    /* cheating */
    config.cp.width = 256;
    config.cp.height = 256;
  }
}

static void run(char *name, int n_params, GParam * param, int *nreturn_vals,
		GParam ** return_vals)
{
  static GParam values[1];
  GDrawable *drawable = NULL;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  SRAND_FUNC (time(NULL));

  run_mode = param[0].data.d_int32;
  
  if (run_mode == RUN_NONINTERACTIVE) {
    status = STATUS_CALLING_ERROR;
  } else {
    INIT_I18N_UI();
    gimp_get_data("plug_in_flame", &config);
    /* XXX i tried using the init routine, but it didn't work. */
    mw_update_cb = my_mw_update_cb;
    maybe_init_cp();

    drawable = gimp_drawable_get(param[2].data.d_drawable);
    config.cp.width = drawable->width;
    config.cp.height = drawable->height;

    if (run_mode == RUN_INTERACTIVE) {      
      if (!dialog()) {
	status = STATUS_EXECUTION_ERROR;
      }
    }
  }


  if (status == STATUS_SUCCESS) {

    if (gimp_drawable_is_rgb(drawable->id)) {
      gimp_progress_init( _("Drawing Flame..."));
      gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));

      doit(drawable);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush();
      gimp_set_data("plug_in_flame", &config, sizeof(config));
    } else {
      status = STATUS_EXECUTION_ERROR;
    }
    gimp_drawable_detach(drawable);
  }


  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static void
drawable_to_cmap(control_point *cp) {
  int i, j;
  GPixelRgn pr;
  GDrawable *d;
  guchar *p;
  int indexed;

  if (table_drawable >= config.cmap_drawable) {
    i = table_drawable - config.cmap_drawable;
    get_cmap(i, cp->cmap, 256);
  } else if (black_drawable == config.cmap_drawable) {
    for (i = 0; i < 256; i++)
      for (j = 0; j < 3; j++)
	cp->cmap[i][j] = 0.0;
  } else if (gradient_drawable == config.cmap_drawable) {
    gdouble *g = gimp_gradients_sample_uniform(256);
    for (i = 0; i < 256; i++)
      for (j = 0; j < 3; j++)
	cp->cmap[i][j] = g[i*4 + j];
    g_free(g);
  } else {
    d = gimp_drawable_get(config.cmap_drawable);
    indexed = gimp_drawable_is_indexed(config.cmap_drawable);
    p = (guchar *) g_malloc(d->bpp);
    gimp_pixel_rgn_init(&pr, d, 0, 0,
			d->width, d->height, FALSE, FALSE);
    for (i = 0; i < 256; i++) {
      gimp_pixel_rgn_get_pixel(&pr, p, i % d->width,
			       (i / d->width) % d->height);
      for (j = 0; j < 3; j++)
	cp->cmap[i][j] =
	  (d->bpp >= 3) ? (p[j] / 255.0) : (p[0]/255.0);
    }
    g_free(p);
  }
}

static void doit(GDrawable * drawable)
{
  gint width, height;
  guchar *tmp;
  gint bytes;

  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  if (3 != bytes && 4 != bytes) {
    fprintf(stderr, _("only works with three or four channels, not %d.\n"), bytes);
    return;
  }

  tmp = (guchar *) g_malloc(width * height * 4);

  /* render */
  config.cp.width = width;
  config.cp.height = height;
  if (config.randomize)
    random_control_point(&config.cp, config.variation);
  drawable_to_cmap(&config.cp);
  render_rectangle(&f, tmp, width, field_both, 4,
		   gimp_progress_update);

  /* update destination */
  if (4 == bytes) {
    GPixelRgn pr;
    gimp_pixel_rgn_init(&pr, drawable, 0, 0, width, height,
			TRUE, TRUE);
    gimp_pixel_rgn_set_rect(&pr, tmp, 0, 0, width, height);
  } else if (3 == bytes) {
    int i, j;
    GPixelRgn src_pr, dst_pr;
    guchar *sl = (guchar *) g_malloc(3 * width);

    gimp_pixel_rgn_init(&src_pr, drawable,
			0, 0, width, height, FALSE, FALSE);
    gimp_pixel_rgn_init(&dst_pr, drawable,
			0, 0, width, height, TRUE, TRUE);
    for (i = 0; i < height; i++) {
      guchar *rr = tmp + 4 * i * width;
      guchar *sld = sl;
      gimp_pixel_rgn_get_rect(&src_pr, sl, 0, i, width, 1);
      for (j = 0; j < width; j++) {
	int k, alpha = rr[3];
	for (k = 0; k < 3; k++) {
	  int t = (rr[k] + ((sld[k] * (256-alpha)) >> 8));
	  if (t > 255) t = 255;
	  sld[k] = t;
	}
	rr += 4;
	sld += 3;
      }
      gimp_pixel_rgn_set_rect(&dst_pr, sl, 0, i, width, 1);
    }
    g_free(sl);
  } else
    printf("oops\n");
  g_free(tmp);
  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, 0, 0, width, height);
}


static void close_callback(GtkWidget * widget, gpointer data)
{
  gtk_main_quit();
}

static void ok_callback(GtkWidget * widget, gpointer data)
{
  run_flag = 1;
  gtk_widget_destroy(GTK_WIDGET(data));
  if (edit_dlg)
    gtk_widget_destroy(edit_dlg);
}

static void file_ok_callback(GtkWidget * widget, gpointer data) {
  GtkFileSelection *fs;
  char* filename;
  fs = GTK_FILE_SELECTION (data);
  filename = gtk_file_selection_get_filename (fs);
  if (load_store) {
    FILE *f = fopen(filename, "r");
    int i, c;
    char *ss;

    if (NULL == f) {
      perror(filename);
      return;
    }
    i = 0;
    ss = buffer;
    do {
      c = getc(f);
      if (EOF == c)
	break;
      ss[i++] = c;
    } while (';' != c);
    parse_control_point(&ss, &config.cp);
    fclose(f);
    /* i want to update the existing dialogue, but it's
       too painful */
    gimp_set_data("plug_in_flame", &config, sizeof(config));
    /* gtk_widget_destroy(dlg); */
    set_flame_preview();
    set_edit_preview();
  } else {
    FILE *f = fopen(filename, "w");
    if (NULL == f) {
      perror(filename);
      return;
    }
    print_control_point(f, &config.cp, 0);
    fclose(f);
  }
  gtk_widget_hide (file_dlg);  
}

static void file_cancel_callback(GtkWidget * widget, gpointer data) {
  gtk_widget_hide (file_dlg);
}

static void
make_file_dlg() {
  file_dlg = gtk_file_selection_new ( _("Load/Store Flame"));
  gtk_window_position (GTK_WINDOW (file_dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT (file_dlg),
		     "delete_event",
		     (GtkSignalFunc) gtk_widget_hide_on_delete,
		     NULL);
  gtk_quit_add_destroy (1, GTK_OBJECT (file_dlg));
  gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION (file_dlg)->cancel_button),
		     "clicked", (GtkSignalFunc) file_cancel_callback, file_dlg);
  gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION (file_dlg)->ok_button),
		     "clicked", (GtkSignalFunc) file_ok_callback, file_dlg);
}

static void randomize_callback(GtkWidget * widget, gpointer data) {
  random_control_point(&edit_cp, config.variation);
  init_mutants();
  set_edit_preview();
}

static void edit_close_callback(GtkWidget * widget, gpointer data) {
  gtk_widget_hide(edit_dlg);
}
  
static void edit_ok_callback(GtkWidget * widget, gpointer data) {
  gtk_widget_hide(edit_dlg);
  config.cp = edit_cp;
  set_flame_preview();  
}

static void edit_cancel_callback(GtkWidget * widget, gpointer data) {
  gtk_widget_hide(edit_dlg);
}

static void init_mutants() {
  int i;
  for (i = 0; i < nmutants; i++) {
    mutants[i] = edit_cp;
    random_control_point(mutants + i, config.variation);
    if (variation_same == config.variation)
      copy_variation(mutants + i, &edit_cp);
  }
}

static void my_mw_update_cb(gpointer data) {
  double *fd = (double *) data;
  if (fd == &pick_speed)
    set_edit_preview();
  else if (&config.cp.brightness == fd ||
	   &config.cp.contrast == fd ||
	   &config.cp.gamma == fd ||
	   &config.cp.zoom == fd ||
	   &config.cp.center[0] == fd ||
	   &config.cp.center[1] == fd)
    set_flame_preview();
}   

static void set_edit_preview() {
  int y, i, j;
  guchar *b;
  control_point pcp;
  static frame_spec pf = {0.0, 0, 1, 0.0};
  int nbytes = edit_preview_size * edit_preview_size * 3;

  if (NULL == edit_previews[0]) return;

  b = g_malloc(nbytes);
  maybe_init_cp();
  drawable_to_cmap(&edit_cp);
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++) {
      int mut = i*3 + j;
      pf.cps = &pcp;
      if (1 == i && 1 == j) {
	pcp = edit_cp;
      } else {
	control_point ends[2];
	ends[0] = edit_cp;
	ends[1] = mutants[mut];
	ends[0].time = 0.0;
	ends[1].time = 1.0;
	interpolate(ends, 2, pick_speed, &pcp);
      }
      pcp.pixels_per_unit =
	(pcp.pixels_per_unit * edit_preview_size) / pcp.width;
      pcp.width = edit_preview_size;
      pcp.height = edit_preview_size;

      pcp.sample_density = 1;
      pcp.spatial_oversample = 1;
      pcp.spatial_filter_radius = 0.5;

      drawable_to_cmap(&pcp);

      render_rectangle(&pf, b, edit_preview_size, field_both, 3, NULL);

      for (y = 0; y < edit_preview_size; y++)
	gtk_preview_draw_row(GTK_PREVIEW (edit_previews[mut]),
			     b + y * edit_preview_size * 3,
			     0, y, edit_preview_size);
      gtk_widget_draw (edit_previews[mut], NULL);  
    }
  g_free(b);
}

static void preview_clicked(GtkWidget * widget, gpointer data) {
  int mut = (int) data;
  if (mut == 4) {
    control_point t = edit_cp;
    init_mutants();
    edit_cp = t;
  } else {
    control_point ends[2];
    ends[0] = edit_cp;
    ends[1] = mutants[mut];
    ends[0].time = 0.0;
    ends[1].time = 1.0;
    interpolate(ends, 2, pick_speed, &edit_cp);
  }
  set_edit_preview();
}


static void
edit_callback(GtkWidget * widget, gpointer data) {
  edit_cp = config.cp;
  if (0 == edit_dlg) {
    GtkWidget *table;
    GtkWidget *button;
    GtkWidget *box, *frame, *vbox;
    int i, j;

    edit_dlg = gtk_dialog_new();
  
    gtk_window_set_title(GTK_WINDOW(edit_dlg), _("Edit Flame"));
    gtk_window_position(GTK_WINDOW(edit_dlg), GTK_WIN_POS_MOUSE);
    gtk_signal_connect(GTK_OBJECT(edit_dlg), "destroy",
		       (GtkSignalFunc) edit_close_callback, NULL);
    gtk_quit_add_destroy (1, GTK_OBJECT (edit_dlg));
    gtk_signal_connect(GTK_OBJECT(edit_dlg), "delete_event",
		       (GtkSignalFunc) gtk_widget_hide_on_delete, NULL);

    button = gtk_button_new_with_label( _("OK"));
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) edit_ok_callback, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(edit_dlg)->action_area),
		       button, TRUE, TRUE, 0);
    gtk_widget_grab_default(button);
    gtk_widget_show(button);

    button = gtk_button_new_with_label( _("Cancel"));
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) edit_cancel_callback, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(edit_dlg)->action_area),
		       button, TRUE, TRUE, 0);
    gtk_widget_grab_default(button);
    gtk_widget_show(button);

    frame = gtk_frame_new( _("Directions"));
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_border_width(GTK_CONTAINER(frame), 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(edit_dlg)->vbox),
		       frame, TRUE, TRUE, 0);
    gtk_widget_show(frame);

    table = gtk_table_new(3, 3, FALSE);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_widget_show(table);

    for (i = 0; i < 3; i++)
      for (j = 0; j < 3; j++) {
	int mut = i*3 + j;
	edit_previews[mut] = gtk_preview_new (GTK_PREVIEW_COLOR);
	gtk_preview_size (GTK_PREVIEW (edit_previews[mut]),
			  edit_preview_size, edit_preview_size);
	button = gtk_button_new();
	gtk_container_add (GTK_CONTAINER(button), edit_previews[mut]);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   (GtkSignalFunc) preview_clicked,
			   (gpointer) mut);
	gtk_table_attach (GTK_TABLE (table), button, i, i+1, j, j+1,
			  GTK_EXPAND, GTK_EXPAND, 0, 0);
	gtk_widget_show (edit_previews[mut]);
	gtk_widget_show (button);
      }

    frame = gtk_frame_new( _("Controls"));
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_border_width(GTK_CONTAINER(frame), 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(edit_dlg)->vbox),
		       frame, TRUE, TRUE, 0);
    gtk_widget_show(frame);

    vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_add (GTK_CONTAINER (frame), vbox);
    gtk_widget_show(vbox);

    table = gtk_table_new(2, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox),
		       table, TRUE, FALSE, 10);
    gtk_widget_show(table);
    
    mw_fscale_entry_new(table, _("Speed"), 0.05, 0.5, 0.01, 0.1,
			0.0, 0, 1, 1, 2, &pick_speed);

    box = gtk_hbox_new (TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox),
		       box, TRUE, FALSE, 10);
    gtk_widget_show(box);

    button = gtk_button_new_with_label( _("Randomize"));
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			      (GtkSignalFunc) randomize_callback,
			      (gpointer) 0);
    gtk_box_pack_start(GTK_BOX(box), button, TRUE, FALSE, 0);
    gtk_widget_show(button);

    {
      static struct {
	char *name;
	int value;
      } menu_items[] = {
	{ N_("Same"), variation_same },
	{ N_("Random"), variation_random },
	{ N_("Linear"), 0},
	{ N_("Sinusoidal"), 1},
	{ N_("Spherical"), 2},
	{ N_("Swirl"), 3},
	{ N_("Horseshoe"), 4},
	{ N_("Polar"), 5},
	{ N_("Bent"), 6},
	{ NULL, 0},
      };
      GtkWidget *hbox;
      GtkWidget *option_menu;
      GtkWidget *menu, *w;
      int i;

      hbox = gtk_hbox_new (FALSE, 5);
      gtk_box_pack_start(GTK_BOX(box), hbox, TRUE, FALSE, 10);
      gtk_widget_show(hbox);

      w = gtk_label_new( _("Variation:"));
      gtk_misc_set_alignment(GTK_MISC(w), 0.0, 0.5);
      gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 0);
      gtk_widget_show(w);

      option_menu = gtk_option_menu_new ();
      gtk_box_pack_start(GTK_BOX(hbox), option_menu, FALSE, FALSE, 0);
      i = 0;
      menu = gtk_menu_new ();
      while (menu_items[i].name) {
	GtkWidget *menu_item;
	menu_item = gtk_menu_item_new_with_label( gettext(menu_items[i].name));
	gtk_container_add (GTK_CONTAINER (menu), menu_item);
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			    (GtkSignalFunc) menu_cb,
			    (gpointer) menu_items[i].value);
	gtk_widget_show (menu_item);
	i++;
      }
      gtk_menu_set_active (GTK_MENU (menu), config.variation + 2);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
      gtk_widget_show (option_menu);
    }
    init_mutants();
  }
  set_edit_preview();

  if (!GTK_WIDGET_VISIBLE(edit_dlg))
    gtk_widget_show(edit_dlg);
}

static void load_callback(GtkWidget * widget, gpointer data) {
  if (!file_dlg) {
    make_file_dlg();
  } else {
    if (GTK_WIDGET_VISIBLE(file_dlg))
      return;
  }
  gtk_window_set_title(GTK_WINDOW (file_dlg), _("Load Flame"));
  load_store = 1;
  gtk_widget_show (file_dlg);
}

static void store_callback(GtkWidget * widget, gpointer data) {
  if (!file_dlg) {
    make_file_dlg();
  } else {
    if (GTK_WIDGET_VISIBLE(file_dlg))
      return;
  }
  gtk_window_set_title(GTK_WINDOW (file_dlg), _("Store Flame"));
  load_store = 0;
  gtk_widget_show (file_dlg);
}

static void menu_cb(GtkWidget * widget, gpointer data) {
  config.variation = (int) data;
  if (variation_same != config.variation)
    random_control_point(&edit_cp, config.variation);
  init_mutants();
  set_edit_preview();
}

static void set_flame_preview() {
  int y;
  guchar *b;
  control_point pcp;
  static frame_spec pf = {0.0, 0, 1, 0.0};

  if (NULL == flame_preview)
    return;

  b = g_malloc(preview_width * preview_height * 3);

  maybe_init_cp();
  drawable_to_cmap(&config.cp);

  pf.cps = &pcp;
  pcp = config.cp;
  pcp.pixels_per_unit =
    (pcp.pixels_per_unit * preview_width) / pcp.width;
  pcp.width = preview_width;
  pcp.height = preview_height;
  pcp.sample_density = 1;
  pcp.spatial_oversample = 1;
  pcp.spatial_filter_radius = 0.1;
  render_rectangle(&pf, b, preview_width, field_both, 3, NULL);

  for (y = 0; y < preview_size; y++)
    gtk_preview_draw_row(GTK_PREVIEW (flame_preview),
			 b+y*preview_width*3, 0, y, preview_width);
  g_free(b);

  gtk_widget_draw (flame_preview, NULL);
}

static void set_cmap_preview() {
  int i, x, y;
  guchar b[96];

  if (NULL == cmap_preview)
    return;

  drawable_to_cmap(&config.cp);

  for (y = 0; y < 32; y+=4) {
    for (x = 0; x < 32; x++) {
      int j;
      i = x + (y/4)*32;
      for (j = 0; j < 3; j++)
	b[x*3+j] = config.cp.cmap[i][j]*255.0;
    }
    gtk_preview_draw_row (GTK_PREVIEW (cmap_preview), b, 0, y, 32);
    gtk_preview_draw_row (GTK_PREVIEW (cmap_preview), b, 0, y+1, 32);
    gtk_preview_draw_row (GTK_PREVIEW (cmap_preview), b, 0, y+2, 32);
    gtk_preview_draw_row (GTK_PREVIEW (cmap_preview), b, 0, y+3, 32);
  }

  gtk_widget_draw (cmap_preview, NULL);  
}

static void gradient_cb(GtkWidget * widget, gpointer data) {
  config.cmap_drawable = (int)data;
  set_cmap_preview();
  set_flame_preview();
  /*  set_edit_preview(); */
}

static void cmap_callback(gint32 id, gpointer data) {
  config.cmap_drawable = id;
  set_cmap_preview();
  set_flame_preview();
  /* set_edit_preview(); */
}


static gint
cmap_constrain (gint32 image_id, gint32 drawable_id, gpointer data) {
  
  return ! gimp_drawable_is_indexed (drawable_id);
}


static gint dialog() {
  GtkWidget *hbbox;
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
  argv[0] = g_strdup("flame");

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
  gtk_window_set_title(GTK_WINDOW(dlg), _("Flame"));
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

  frame = gtk_frame_new( _("Rendering"));
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);

  box = gtk_vbox_new (FALSE, 5);
  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_widget_show(box);

  table = gtk_table_new(7, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(box), table, FALSE, FALSE, 0);

  gtk_container_border_width(GTK_CONTAINER(table), 10);
  gtk_widget_show(table);

  gtk_table_set_row_spacings(GTK_TABLE(table), 10);
  gtk_table_set_col_spacings(GTK_TABLE(table), 10);

  row = 1;

  /* this zoom - gamma should redraw flame preview */
  mw_fscale_entry_new(table, _("Brightness"), 0, 5, 1, 1, 0,
                      0, 1, row, row+1, &config.cp.brightness); row++;
  mw_fscale_entry_new(table, _("Contrast"), 0, 5, 1, 1, 0,
                      0, 1, row, row+1, &config.cp.contrast); row++;
  mw_fscale_entry_new(table, _("Gamma"), 1, 5, 1, 1, 0,
                      0, 1, row, row+1, &config.cp.gamma); row++;
  mw_fscale_entry_new(table, _("Sample Density"), 0.1, 20, 1, 5, 0,
                      0, 1, row, row+1, &config.cp.sample_density); row++;
  mw_iscale_entry_new(table, _("Spatial Oversample"), 1, 4, 1, 1, 0,
                      0, 1, row, row+1, &config.cp.spatial_oversample); row++;
  mw_fscale_entry_new(table, _("Spatial Filter Radius"), 0, 4, 1, 1, 0,
                      0, 1, row, row+1, &config.cp.spatial_filter_radius); row++;

{
    GtkWidget *menu;
    GtkWidget *hbox;
    GtkWidget *menuitem;
    GtkWidget *option_menu = gtk_option_menu_new ();
    gint32 save_drawable = config.cmap_drawable;

    hbox = gtk_hbox_new (FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 10);
    gtk_widget_show(hbox);

    w = gtk_label_new( _("Colormap:"));
    gtk_misc_set_alignment(GTK_MISC(w), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 10);
    gtk_widget_show(w);

    gtk_box_pack_start(GTK_BOX(hbox), option_menu, TRUE, TRUE, 10);
    menu = gimp_drawable_menu_new(cmap_constrain, cmap_callback,
				  0, config.cmap_drawable);

    config.cmap_drawable = save_drawable;
#if 0
    menuitem = gtk_menu_item_new_with_label( _("Black"));
    gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
		       (GtkSignalFunc) gradient_cb,
		       (gpointer) black_drawable);
    gtk_menu_prepend(GTK_MENU (menu), menuitem);
    if (black_drawable == save_drawable)
      gtk_menu_set_active(GTK_MENU(menu), 0);
    gtk_widget_show(menuitem);
#endif
    {
      static char *names[] =
      {"sunny harvest", "rose", "calcoast09",
       "klee insula-dulcamara",
       "ernst anti-pope", "gris josette"};
      static int good[] = {10, 20, 68, 79, 70, 75}; 
      int i, n = (sizeof good) / (sizeof *good);
      for (i = 0; i < n; i++) {
	int d = table_drawable - good[i];
	menuitem = gtk_menu_item_new_with_label(names[i]);
	gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
			   (GtkSignalFunc) gradient_cb,
			   (gpointer) d);
	gtk_menu_prepend(GTK_MENU (menu), menuitem);
	if (d == save_drawable)
	  gtk_menu_set_active(GTK_MENU(menu), 0);
	gtk_widget_show(menuitem);
      }
    }


    menuitem = gtk_menu_item_new_with_label( _("Custom Gradient"));
    gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
		       (GtkSignalFunc) gradient_cb,
		       (gpointer) gradient_drawable);
    gtk_menu_prepend(GTK_MENU (menu), menuitem);
    if (gradient_drawable == save_drawable)
      gtk_menu_set_active(GTK_MENU(menu), 0);
    gtk_widget_show(menuitem);

    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
    gtk_widget_show (option_menu);

    cmap_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
    gtk_preview_size (GTK_PREVIEW (cmap_preview), 32, 32);

    gtk_box_pack_start(GTK_BOX(hbox), cmap_preview, TRUE, TRUE, 10);
    gtk_widget_show (cmap_preview);
    set_cmap_preview();
  }

  frame = gtk_frame_new( _("Camera"));
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);

  table = gtk_table_new(4, 2, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 10);
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_widget_show(table);

  gtk_table_set_row_spacings(GTK_TABLE(table), 10);
  gtk_table_set_col_spacings(GTK_TABLE(table), 10);

  row = 1;
  
  mw_fscale_entry_new(table, _("Zoom"), -4, 4, 1, 1, 0,
                      0, 1, row, row+1, &config.cp.zoom); row++;
  mw_fscale_entry_new(table, _("X"), -2, 2, 0.5, 0.5, 0,
                      0, 1, row, row+1, &config.cp.center[0]); row++;
  mw_fscale_entry_new(table, _("Y"), -2, 2, 0.5, 0.5, 0,
                      0, 1, row, row+1, &config.cp.center[1]); row++;
  

  flame_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  {
    double aspect = config.cp.width / (double) config.cp.height;
    if (aspect > 1.0) {
      preview_width = preview_size;
      preview_height = preview_size/aspect;
    } else {
      preview_width = preview_size*aspect;
      preview_height = preview_size;
    }
  }
  gtk_preview_size (GTK_PREVIEW (flame_preview), preview_width, preview_height);

  box = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox),
		     box, FALSE, FALSE, 0);
  gtk_widget_show(box);
  
  frame = gtk_frame_new( _("Preview"));
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_container_add(GTK_CONTAINER(frame), flame_preview);
  gtk_box_pack_start(GTK_BOX(box), frame, TRUE, FALSE, 0);
  gtk_widget_show(frame);

  gtk_widget_show (flame_preview);
  set_flame_preview();

  {
    GtkWidget *vbox = gtk_vbox_new (TRUE, 5);
    gtk_box_pack_start(GTK_BOX(box), vbox, TRUE, FALSE, 0);
    gtk_widget_show(vbox);
  
    button = gtk_button_new_with_label( _("Shape Edit"));
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			      (GtkSignalFunc) edit_callback,
			      (gpointer) 0);
  
    gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, FALSE, 10);
    gtk_widget_show(button);

    button = gtk_button_new_with_label( _("Load"));
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			      (GtkSignalFunc) load_callback,
			      (gpointer) 0);
  
    gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, FALSE, 10);
    gtk_widget_show(button);

    button = gtk_button_new_with_label( _("Store"));
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			      (GtkSignalFunc) store_callback,
			      (gpointer) 0);
  
    gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, FALSE, 10);
    gtk_widget_show(button);
  }

  gtk_widget_show(dlg);
  gtk_main();
  gdk_flush();

  return run_flag;
}
