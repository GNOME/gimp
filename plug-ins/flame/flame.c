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
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#ifdef G_OS_WIN32
#  include <io.h>
#  ifndef S_ISREG
#    define S_ISREG(m) ((m) & _S_IFREG)
#  endif
#endif

#include "flame.h"

#define VARIATION_SAME   (-2)

/* Declare local functions. */
static void query             (void);
static void run               (gchar   *name,
		               gint     nparams,
		               GimpParam  *param,
		               gint    *nreturn_vals,
			       GimpParam **return_vals);
static void doit              (GimpDrawable *drawable);

static gint dialog            (void);
static void set_flame_preview (void);
static void load_callback     (GtkWidget *widget,
			       gpointer   data);
static void save_callback     (GtkWidget *widget,
			       gpointer   data);
static void set_edit_preview  (void);
static void menu_cb           (GtkWidget *widget,
			       gpointer   data);
static void init_mutants      (void);

#define BUFFER_SIZE 10000

static gchar      buffer[BUFFER_SIZE];
static GtkWidget *cmap_preview;
static GtkWidget *flame_preview;
static gint       preview_width, preview_height;
static GtkWidget *dlg;
static GtkWidget *load_button = NULL;
static GtkWidget *save_button = NULL;
static GtkWidget *file_dlg = NULL;
static gint       load_save;

static GtkWidget *edit_dlg = NULL;

#define SCALE_WIDTH       150
#define PREVIEW_SIZE      150
#define EDIT_PREVIEW_SIZE 85
#define NMUTANTS          9

static control_point  edit_cp;
static control_point  mutants[NMUTANTS];
static GtkWidget     *edit_previews[NMUTANTS];
static gdouble        pick_speed = 0.2;


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static gint run_flag = FALSE;

#define BLACK_DRAWABLE    (-2)
#define GRADIENT_DRAWABLE (-3)
#define TABLE_DRAWABLE    (-4)


struct
{
  gint          randomize;  /* superseded */
  gint          variation;
  gint32        cmap_drawable;
  control_point cp;
} config;

static frame_spec f = { 0.0, &config.cp, 1, 0.0 };


MAIN ()


static void 
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
  };
  static gint nargs = sizeof(args) / sizeof(args[0]);

  gimp_install_procedure ("plug_in_flame",
			  "Creates cosmic recursive fractal flames",
			  "Creates cosmic recursive fractal flames",
			  "Scott Draves",
			  "Scott Draves",
			  "1997",
			  N_("<Image>/Filters/Render/Nature/Flame..."),
			  "RGB*",
			  GIMP_PLUGIN,
			  nargs, 0,
			  args, NULL);
}

static void 
maybe_init_cp (void) 
{
  if (0 == config.cp.spatial_oversample)
    {
      config.randomize = 0;
      config.variation = VARIATION_SAME;
      config.cmap_drawable = GRADIENT_DRAWABLE;
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

static void 
run (gchar   *name, 
     gint     n_params, 
     GimpParam  *param, 
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[1];
  GimpDrawable *drawable = NULL;
  GimpRunModeType run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  SRAND_FUNC (time (NULL));

  run_mode = param[0].data.d_int32;
  
  if (run_mode == GIMP_RUN_NONINTERACTIVE)
    {
      status = GIMP_PDB_CALLING_ERROR;
    }
  else
    {
      INIT_I18N_UI();
      gimp_get_data ("plug_in_flame", &config);
      maybe_init_cp ();

      drawable = gimp_drawable_get (param[2].data.d_drawable);
      config.cp.width = drawable->width;
      config.cp.height = drawable->height;

      if (run_mode == GIMP_RUN_INTERACTIVE)
	{
	  if (!dialog ())
	    {
	      status = GIMP_PDB_EXECUTION_ERROR;
	    }
	}
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (gimp_drawable_is_rgb (drawable->id))
	{
	  gimp_progress_init (_("Drawing Flame..."));
	  gimp_tile_cache_ntiles (2 * (drawable->width /
				       gimp_tile_width () + 1));

	  doit (drawable);

	  if (run_mode != GIMP_RUN_NONINTERACTIVE)
	    gimp_displays_flush ();
	  gimp_set_data ("plug_in_flame", &config, sizeof (config));
	}
      else
	{
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
      gimp_drawable_detach (drawable);
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static void
drawable_to_cmap (control_point *cp) 
{
  gint       i, j;
  GimpPixelRgn  pr;
  GimpDrawable *d;
  guchar    *p;
  gint       indexed;

  if (TABLE_DRAWABLE >= config.cmap_drawable)
    {
      i = TABLE_DRAWABLE - config.cmap_drawable;
      get_cmap (i, cp->cmap, 256);
    }
  else if (BLACK_DRAWABLE == config.cmap_drawable)
    {
      for (i = 0; i < 256; i++)
	for (j = 0; j < 3; j++)
	  cp->cmap[i][j] = 0.0;
    }
  else if (GRADIENT_DRAWABLE == config.cmap_drawable)
    {
      gdouble *g = gimp_gradients_sample_uniform (256);
      for (i = 0; i < 256; i++)
	for (j = 0; j < 3; j++)
	  cp->cmap[i][j] = g[i*4 + j];
      g_free (g);
    }
  else
    {
      d = gimp_drawable_get (config.cmap_drawable);
      indexed = gimp_drawable_is_indexed (config.cmap_drawable);
      p = g_new (guchar, d->bpp);
      gimp_pixel_rgn_init (&pr, d, 0, 0,
			   d->width, d->height, FALSE, FALSE);
      for (i = 0; i < 256; i++)
	{
	  gimp_pixel_rgn_get_pixel (&pr, p, i % d->width,
				    (i / d->width) % d->height);
	  for (j = 0; j < 3; j++)
	    cp->cmap[i][j] =
	      (d->bpp >= 3) ? (p[j] / 255.0) : (p[0]/255.0);
	}
      g_free (p);
    }
}

static void 
doit (GimpDrawable *drawable)
{
  gint    width, height;
  guchar *tmp;
  gint    bytes;

  width  = drawable->width;
  height = drawable->height;
  bytes  = drawable->bpp;

  if (3 != bytes && 4 != bytes)
    {
      g_message (_("Flame works only on RGB drawables."));
      return;
    }

  tmp = g_new (guchar, width * height * 4);

  /* render */
  config.cp.width = width;
  config.cp.height = height;
  if (config.randomize)
    random_control_point (&config.cp, config.variation);
  drawable_to_cmap (&config.cp);
  render_rectangle (&f, tmp, width, field_both, 4,
		    gimp_progress_update);

  /* update destination */
  if (4 == bytes)
    {
      GimpPixelRgn pr;
      gimp_pixel_rgn_init (&pr, drawable, 0, 0, width, height,
			   TRUE, TRUE);
      gimp_pixel_rgn_set_rect (&pr, tmp, 0, 0, width, height);
    }
  else if (3 == bytes)
    {
      gint       i, j;
      GimpPixelRgn  src_pr, dst_pr;
      guchar    *sl;

      sl = g_new (guchar, 3 * width);

      gimp_pixel_rgn_init (&src_pr, drawable,
			   0, 0, width, height, FALSE, FALSE);
      gimp_pixel_rgn_init (&dst_pr, drawable,
			   0, 0, width, height, TRUE, TRUE);
      for (i = 0; i < height; i++)
	{
	  guchar *rr = tmp + 4 * i * width;
	  guchar *sld = sl;

	  gimp_pixel_rgn_get_rect (&src_pr, sl, 0, i, width, 1);
	  for (j = 0; j < width; j++)
	    {
	      gint k, alpha = rr[3];

	      for (k = 0; k < 3; k++)
		{
		  gint t = (rr[k] + ((sld[k] * (256-alpha)) >> 8));

		  if (t > 255) t = 255;
		  sld[k] = t;
		}
	      rr += 4;
	      sld += 3;
	    }
	  gimp_pixel_rgn_set_rect (&dst_pr, sl, 0, i, width, 1);
	}
      g_free (sl);
    }

  g_free (tmp);
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, 0, 0, width, height);
}


static void 
ok_callback (GtkWidget *widget, 
	     gpointer   data)
{
  run_flag = TRUE;

  if (edit_dlg)
    gtk_widget_destroy (edit_dlg);
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
file_cancel_callback (GtkWidget *widget,
		      gpointer   data)
{
  gtk_widget_hide (widget);

  if (! GTK_WIDGET_SENSITIVE (load_button))
    gtk_widget_set_sensitive (load_button, TRUE);

  if (! GTK_WIDGET_SENSITIVE (save_button))
    gtk_widget_set_sensitive (save_button, TRUE);
}

static void 
file_ok_callback (GtkWidget *widget, 
		  gpointer   data) 
{
  GtkFileSelection *fs;
  gchar            *filename;
  struct stat       filestat;

  fs = GTK_FILE_SELECTION (data);
  filename = gtk_file_selection_get_filename (fs);

  if (load_save)
    {
      FILE  *f;
      gint   i, c;
      gchar *ss;

      if (stat (filename, &filestat))
	{
	  g_message ("%s: %s", filename, g_strerror (errno));
	  return;
	}

      if (! S_ISREG (filestat.st_mode))
	{
	  g_message (_("%s: Is not a regular file"), filename);
	  return;
	}

      f = fopen (filename, "r");

      if (f == NULL)
	{
	  g_message ("%s: %s", filename, g_strerror (errno));
	  return;
	}

      i = 0;
      ss = buffer;
      do
	{
	  c = getc (f);
	  if (EOF == c)
	    break;
	  ss[i++] = c;
	}
      while (i < BUFFER_SIZE && ';' != c);
      parse_control_point (&ss, &config.cp);
      fclose (f);
      /* i want to update the existing dialogue, but it's
	 too painful */
      gimp_set_data ("plug_in_flame", &config, sizeof (config));
      /* gtk_widget_destroy(dlg); */
      set_flame_preview ();
      set_edit_preview ();
    }
  else
    {
      FILE *f = fopen (filename, "w");

      if (NULL == f)
	{
	  g_message ("%s: %s", filename, g_strerror (errno));
	  return;
	}

      print_control_point (f, &config.cp, 0);
      fclose (f);
    }

  file_cancel_callback (GTK_WIDGET (data), NULL);
}

static void
make_file_dlg (void) 
{
  file_dlg = gtk_file_selection_new (NULL);
  gtk_quit_add_destroy (1, GTK_OBJECT (file_dlg));

  gtk_window_position (GTK_WINDOW (file_dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect_object (GTK_OBJECT (file_dlg), "delete_event",
			     GTK_SIGNAL_FUNC (file_cancel_callback),
			     GTK_OBJECT (file_dlg));
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_dlg)->ok_button),
		      "clicked",
		      GTK_SIGNAL_FUNC (file_ok_callback),
		      file_dlg);
  gtk_signal_connect_object
    (GTK_OBJECT (GTK_FILE_SELECTION (file_dlg)->cancel_button),
     "clicked",
     GTK_SIGNAL_FUNC (file_cancel_callback),
     GTK_OBJECT (file_dlg));

  gimp_help_connect_help_accel (file_dlg,
				gimp_standard_help_func, "filters/flame.html");
}

static void 
randomize_callback (GtkWidget *widget, 
		    gpointer   data)
{
  random_control_point (&edit_cp, config.variation);
  init_mutants ();
  set_edit_preview ();
}

static void 
edit_ok_callback (GtkWidget *widget, 
		  gpointer   data)
{
  gtk_widget_hide (edit_dlg);
  config.cp = edit_cp;
  set_flame_preview ();  
}

static void 
init_mutants (void)
{
  gint i;

  for (i = 0; i < NMUTANTS; i++)
    {
      mutants[i] = edit_cp;
      random_control_point (mutants + i, config.variation);
      if (VARIATION_SAME == config.variation)
	copy_variation (mutants + i, &edit_cp);
    }
}

static void 
set_edit_preview (void) 
{
  gint           y, i, j;
  guchar        *b;
  control_point  pcp;
  gint           nbytes = EDIT_PREVIEW_SIZE * EDIT_PREVIEW_SIZE * 3;

  static frame_spec pf = { 0.0, 0, 1, 0.0 };

  if (NULL == edit_previews[0])
    return;

  b = g_new (guchar, nbytes);
  maybe_init_cp ();
  drawable_to_cmap (&edit_cp);
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      {
	gint mut = i*3 + j;

	pf.cps = &pcp;
	if (1 == i && 1 == j)
	  {
	    pcp = edit_cp;
	  }
	else
	  {
	    control_point ends[2];
	    ends[0] = edit_cp;
	    ends[1] = mutants[mut];
	    ends[0].time = 0.0;
	    ends[1].time = 1.0;
	    interpolate (ends, 2, pick_speed, &pcp);
	  }
	pcp.pixels_per_unit =
	  (pcp.pixels_per_unit * EDIT_PREVIEW_SIZE) / pcp.width;
	pcp.width = EDIT_PREVIEW_SIZE;
	pcp.height = EDIT_PREVIEW_SIZE;

	pcp.sample_density = 1;
	pcp.spatial_oversample = 1;
	pcp.spatial_filter_radius = 0.5;

	drawable_to_cmap (&pcp);

	render_rectangle (&pf, b, EDIT_PREVIEW_SIZE, field_both, 3, NULL);

	for (y = 0; y < EDIT_PREVIEW_SIZE; y++)
	  gtk_preview_draw_row (GTK_PREVIEW (edit_previews[mut]),
				b + y * EDIT_PREVIEW_SIZE * 3,
				0, y, EDIT_PREVIEW_SIZE);
	gtk_widget_draw (edit_previews[mut], NULL);  
      }
  g_free (b);
}

static void 
preview_clicked (GtkWidget *widget, 
		 gpointer   data) 
{
  gint mut = (gint) data;

  if (mut == 4)
    {
      control_point t = edit_cp;
      init_mutants ();
      edit_cp = t;
    }
  else
    {
      control_point ends[2];
      ends[0] = edit_cp;
      ends[1] = mutants[mut];
      ends[0].time = 0.0;
      ends[1].time = 1.0;
      interpolate (ends, 2, pick_speed, &edit_cp);
    }
  set_edit_preview ();
}

static void
edit_callback (GtkWidget *widget, 
	       gpointer   data) 
{
  edit_cp = config.cp;

  if (edit_dlg == NULL)
    {
      GtkWidget *main_vbox;
      GtkWidget *frame;
      GtkWidget *table;
      GtkWidget *vbox;
      GtkWidget *hbox;
      GtkWidget *button;
      GtkWidget *optionmenu;
      GtkWidget *label;
      GtkObject *adj;
      gint i, j;

      edit_dlg = gimp_dialog_new (_("Edit Flame"), "flame",
				  gimp_standard_help_func, "filters/flame.html",
				  GTK_WIN_POS_MOUSE,
				  FALSE, FALSE, FALSE,

				  _("OK"), edit_ok_callback,
				  NULL, NULL, NULL, TRUE, FALSE,
				  _("Cancel"), gtk_widget_hide,
				  NULL, 1, NULL, FALSE, TRUE,

				  NULL);

      gtk_quit_add_destroy (1, GTK_OBJECT (edit_dlg));

      main_vbox = gtk_vbox_new (FALSE, 4);
      gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (edit_dlg)->vbox), main_vbox,
			  FALSE, FALSE, 0);

      frame = gtk_frame_new (_("Directions"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      table = gtk_table_new (3, 3, FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (table), 4);
      gtk_table_set_row_spacings (GTK_TABLE (table), 4);
      gtk_table_set_col_spacings (GTK_TABLE (table), 4);
      gtk_container_add (GTK_CONTAINER (frame), table);
      gtk_widget_show (table);

      for (i = 0; i < 3; i++)
	for (j = 0; j < 3; j++)
	  {
	    gint mut = i * 3 + j;

	    edit_previews[mut] = gtk_preview_new (GTK_PREVIEW_COLOR);
	    gtk_preview_size (GTK_PREVIEW (edit_previews[mut]),
			      EDIT_PREVIEW_SIZE, EDIT_PREVIEW_SIZE);
	    button = gtk_button_new ();
	    gtk_container_add (GTK_CONTAINER(button), edit_previews[mut]);
	    gtk_signal_connect (GTK_OBJECT (button), "clicked",
				GTK_SIGNAL_FUNC (preview_clicked),
				(gpointer) mut);
	    gtk_table_attach (GTK_TABLE (table), button, i, i+1, j, j+1,
			      GTK_EXPAND, GTK_EXPAND, 0, 0);
	    gtk_widget_show (edit_previews[mut]);
	    gtk_widget_show (button);
	  }

      frame = gtk_frame_new( _("Controls"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox = gtk_vbox_new (FALSE, 4);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      table = gtk_table_new (1, 3, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 4);
      gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
      gtk_widget_show(table);

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
				  _("Speed:"), SCALE_WIDTH, 0,
				  pick_speed,
				  0.05, 0.5, 0.01, 0.1, 2,
				  TRUE, 0, 0,
				  NULL, NULL);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
			  &pick_speed);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (set_edit_preview),
			  NULL);

      hbox = gtk_hbox_new (FALSE, 4);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      button = gtk_button_new_with_label( _("Randomize"));
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				 GTK_SIGNAL_FUNC (randomize_callback),
				 NULL);
      gtk_widget_show (button);

      optionmenu =
	gimp_option_menu_new2 (FALSE, menu_cb,
			       &config.variation,
			       (gpointer) VARIATION_SAME,

			       _("Same"),     (gpointer) VARIATION_SAME, NULL,
			       _("Random"),   (gpointer) variation_random, NULL,
			       _("Linear"),     (gpointer) 0, NULL,
			       _("Sinusoidal"), (gpointer) 1, NULL,
			       _("Spherical"),  (gpointer) 2, NULL,
			       _("Swirl"),      (gpointer) 3, NULL,
			       _("Horseshoe"),  (gpointer) 4, NULL,
			       _("Polar"),      (gpointer) 5, NULL,
			       _("Bent"),       (gpointer) 6, NULL,

			       NULL);
      gtk_box_pack_end (GTK_BOX (hbox), optionmenu, FALSE, FALSE, 0);
      gtk_widget_show (optionmenu);

      label = gtk_label_new (_("Variation:"));
      gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      gtk_widget_show (main_vbox);

      init_mutants ();
    }

  set_edit_preview ();

  if (!GTK_WIDGET_VISIBLE (edit_dlg))
    gtk_widget_show (edit_dlg);
  else
    gdk_window_raise (edit_dlg->window);
}

static void 
load_callback (GtkWidget *widget, 
	       gpointer   data) 
{
  if (!file_dlg)
    {
      make_file_dlg ();
    }
  else
    {
      if (GTK_WIDGET_VISIBLE (file_dlg))
	{
	  gdk_window_raise (file_dlg->window);
	  return;
	}
    }
  gtk_window_set_title (GTK_WINDOW (file_dlg), _("Load Flame"));
  load_save = 1;
  gtk_widget_set_sensitive (save_button, FALSE);
  gtk_widget_show (file_dlg);
}

static void 
save_callback (GtkWidget *widget, 
	       gpointer   data) 
{
  if (!file_dlg)
    {
      make_file_dlg ();
    }
  else
    {
      if (GTK_WIDGET_VISIBLE (file_dlg))
	{
	  gdk_window_raise (file_dlg->window);
	  return;
	}
    }
  gtk_window_set_title (GTK_WINDOW (file_dlg), _("Save Flame"));
  load_save = 0;
  gtk_widget_set_sensitive (load_button, FALSE);
  gtk_widget_show (file_dlg);
}

static void 
menu_cb (GtkWidget *widget, 
	 gpointer   data) 
{
  gimp_menu_item_update (widget, data);

  if (VARIATION_SAME != config.variation)
    random_control_point (&edit_cp, config.variation);
  init_mutants ();
  set_edit_preview ();
}

static void 
set_flame_preview (void) 
{
  gint    y;
  guchar *b;
  control_point pcp;

  static frame_spec pf = {0.0, 0, 1, 0.0};

  if (NULL == flame_preview)
    return;

  b = g_new (guchar, preview_width * preview_height * 3);

  maybe_init_cp ();
  drawable_to_cmap (&config.cp);

  pf.cps = &pcp;
  pcp = config.cp;
  pcp.pixels_per_unit =
    (pcp.pixels_per_unit * preview_width) / pcp.width;
  pcp.width = preview_width;
  pcp.height = preview_height;
  pcp.sample_density = 1;
  pcp.spatial_oversample = 1;
  pcp.spatial_filter_radius = 0.1;
  render_rectangle (&pf, b, preview_width, field_both, 3, NULL);

  for (y = 0; y < PREVIEW_SIZE; y++)
    gtk_preview_draw_row (GTK_PREVIEW (flame_preview),
			  b+y*preview_width*3, 0, y, preview_width);
  g_free (b);

  gtk_widget_draw (flame_preview, NULL);
}

static void 
set_cmap_preview (void) 
{
  gint i, x, y;
  guchar b[96];

  if (NULL == cmap_preview)
    return;

  drawable_to_cmap (&config.cp);

  for (y = 0; y < 32; y += 4)
    {
      for (x = 0; x < 32; x++)
	{
	  gint j;

	  i = x + (y/4) * 32;
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

static void 
gradient_cb (GtkWidget *widget, 
	     gpointer   data) 
{
  config.cmap_drawable = (gint) data;
  set_cmap_preview();
  set_flame_preview();
  /* set_edit_preview(); */
}

static void 
cmap_callback (gint32   id, 
	       gpointer data) 
{
  config.cmap_drawable = id;
  set_cmap_preview();
  set_flame_preview();
  /* set_edit_preview(); */
}

static gint
cmap_constrain (gint32   image_id, 
		gint32   drawable_id, 
		gpointer data) 
{  
  return ! gimp_drawable_is_indexed (drawable_id);
}


static gint 
dialog (void) 
{
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *abox;
  GtkWidget *pframe;
  GtkWidget *button;
  GtkWidget *table;
  GtkWidget *box;
  GtkObject *adj;

  gimp_ui_init ("flame", TRUE);

  dlg = gimp_dialog_new (_("Flame"), "flame",
			 gimp_standard_help_func, "filters/flame.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox,
		      FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  box = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  frame = gtk_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (box), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (abox), 4);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);

  pframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), pframe);
  gtk_widget_show (pframe);

  frame = gtk_frame_new (_("Flame"));
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  {
    GtkWidget *vbbox;

    box = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box), 4);
    gtk_container_add (GTK_CONTAINER (frame), box);
    gtk_widget_show (box);

    vbbox= gtk_vbutton_box_new ();
    gtk_box_set_homogeneous (GTK_BOX (vbbox), FALSE);
    gtk_button_box_set_spacing (GTK_BUTTON_BOX (vbbox), 4);
    gtk_box_pack_start (GTK_BOX (box), vbbox, FALSE, FALSE, 0);
    gtk_widget_show (vbbox);
  
    button = gtk_button_new_with_label (_("Edit Flame"));
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (edit_callback),
			NULL);
    gtk_box_pack_start (GTK_BOX (vbbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    load_button = button = gtk_button_new_with_label (_("Load Flame"));
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (load_callback),
			NULL);
    gtk_box_pack_start (GTK_BOX (vbbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    save_button = button = gtk_button_new_with_label (_("Save Flame"));
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (save_callback),
			NULL);
    gtk_box_pack_start (GTK_BOX (vbbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);
  }

  frame = gtk_frame_new (_("Rendering"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  box = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (box), 4);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  table = gtk_table_new (6, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (box), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Brightness:"), SCALE_WIDTH, 0,
			      config.cp.brightness,
			      0, 5, 0.1, 1, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &config.cp.brightness);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (set_flame_preview),
		      NULL);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Contrast:"), SCALE_WIDTH, 0,
			      config.cp.contrast,
			      0, 5, 0.1, 1, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &config.cp.contrast);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (set_flame_preview),
		      NULL);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Gamma:"), SCALE_WIDTH, 0,
			      config.cp.gamma,
			      1, 5, 0.1, 1, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &config.cp.gamma);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (set_flame_preview),
		      NULL);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
			      _("Sample Density:"), SCALE_WIDTH, 0,
			      config.cp.sample_density,
			      0.1, 20, 1, 5, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &config.cp.sample_density);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 4,
			      _("Spatial Oversample:"), SCALE_WIDTH, 0,
			      config.cp.spatial_oversample,
			      0, 1, 0.01, 0.1, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &config.cp.spatial_oversample);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 5,
			      _("Spatial Filter Radius:"), SCALE_WIDTH, 0,
			      config.cp.spatial_filter_radius,
			      0, 4, 0.2, 1, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &config.cp.spatial_filter_radius);

  {
    GtkWidget *sep;
    GtkWidget *menu;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *menuitem;
    GtkWidget *option_menu;
    gint32 save_drawable = config.cmap_drawable;

    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box), sep, FALSE, FALSE, 0);
    gtk_widget_show (sep);

    hbox = gtk_hbox_new (FALSE, 4);
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    label = gtk_label_new (_("Colormap:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    option_menu = gtk_option_menu_new ();
    gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
    menu = gimp_drawable_menu_new (cmap_constrain, cmap_callback,
				   0, config.cmap_drawable);

    config.cmap_drawable = save_drawable;
#if 0
    menuitem = gtk_menu_item_new_with_label (_("Black"));
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			GTK_SIGNAL_FUNC (gradient_cb),
			(gpointer) BLACK_DRAWABLE);
    gtk_menu_prepend (GTK_MENU (menu), menuitem);
    if (BLACK_DRAWABLE == save_drawable)
      gtk_menu_set_active (GTK_MENU (menu), 0);
    gtk_widget_show (menuitem); 
#endif
    {
      static gchar *names[] =
      {
	"sunny harvest",
	"rose",
	"calcoast09",
	"klee insula-dulcamara",
	"ernst anti-pope",
	"gris josette"
      };
      static gint good[] = { 10, 20, 68, 79, 70, 75 };
      gint i, n = (sizeof good) / (sizeof good[0]);

      for (i = 0; i < n; i++)
	{
	  gint d = TABLE_DRAWABLE - good[i];

	  menuitem = gtk_menu_item_new_with_label (names[i]);
	  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			      GTK_SIGNAL_FUNC (gradient_cb),
			      (gpointer) d);
	  gtk_menu_prepend (GTK_MENU (menu), menuitem);
	  if (d == save_drawable)
	    gtk_menu_set_active (GTK_MENU (menu), 0);
	  gtk_widget_show (menuitem);
	}
    }

    menuitem = gtk_menu_item_new_with_label (_("Custom Gradient"));
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			GTK_SIGNAL_FUNC (gradient_cb),
			(gpointer) GRADIENT_DRAWABLE);
    gtk_menu_prepend (GTK_MENU (menu), menuitem);
    if (GRADIENT_DRAWABLE == save_drawable)
      gtk_menu_set_active (GTK_MENU (menu), 0);
    gtk_widget_show (menuitem);

    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
    gtk_widget_show (option_menu);

    cmap_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
    gtk_preview_size (GTK_PREVIEW (cmap_preview), 32, 32);

    gtk_box_pack_end (GTK_BOX (hbox), cmap_preview, FALSE, FALSE, 0);
    gtk_widget_show (cmap_preview);
    set_cmap_preview ();
  }

  frame = gtk_frame_new (_("Camera"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Zoom:"), SCALE_WIDTH, 0,
			      config.cp.zoom,
			      -4, 4, 0.5, 1, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &config.cp.zoom);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (set_flame_preview),
		      NULL);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("X:"), SCALE_WIDTH, 0,
			      config.cp.center[0],
			      -2, 2, 0.5, 1, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &config.cp.center[0]);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (set_flame_preview),
		      NULL);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("X:"), SCALE_WIDTH, 0,
			      config.cp.center[1],
			      -2, 2, 0.5, 1, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &config.cp.center[1]);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (set_flame_preview),
		      NULL);

  flame_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  {
    gdouble aspect = config.cp.width / (double) config.cp.height;

    if (aspect > 1.0)
      {
	preview_width = PREVIEW_SIZE;
	preview_height = PREVIEW_SIZE / aspect;
      }
    else
      {
	preview_width = PREVIEW_SIZE * aspect;
	preview_height = PREVIEW_SIZE;
      }
  }
  gtk_preview_size (GTK_PREVIEW (flame_preview), preview_width, preview_height);
  gtk_container_add (GTK_CONTAINER (pframe), flame_preview);
  gtk_widget_show (flame_preview);
  set_flame_preview ();

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return run_flag;
}
