/* The GIMP -- an image manipulation program
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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include "appenv.h"
#include "app_procs.h"
#include "batch.h"
#include "brushes.h"
#include "color_transfer.h"
#include "curves.h"
#include "devices.h"
#include "gdisplay.h"
#include "colormaps.h"
#include "fileops.h"
#include "gimprc.h"
#include "gimpset.h"
#include "global_edit.h"
#include "gradient.h"
#include "gximage.h"
#include "hue_saturation.h"
#include "image_render.h"
#include "interface.h"
#include "internal_procs.h"
#include "layers_dialog.h"
#include "levels.h"
#include "menus.h"
#include "paint_funcs.h"
#include "palette.h"
#include "patterns.h"
#include "plug_in.h"
#include "procedural_db.h"
#include "session.h"
#include "temp_buf.h"
#include "tile_swap.h"
#include "tips_dialog.h"
#include "tools.h"
#include "undo.h"
#include "xcf.h"
#include "errors.h"

#include "config.h"

#define LOGO_WIDTH_MIN 300
#define LOGO_HEIGHT_MIN 110 
#define NAME "The GIMP"
#define BROUGHT "brought to you by"
#define AUTHORS "Spencer Kimball and Peter Mattis" 
 
#define SHOW_NEVER 0
#define SHOW_LATER 1
#define SHOW_NOW 2
 
/*  Function prototype for affirmation dialog when exiting application  */
static void      really_quit_dialog (void);
static Argument* quit_invoker       (Argument *args);
static void make_initialization_status_window(void);
static void destroy_initialization_status_window(void);
static int splash_logo_load (GtkWidget *window);
static int splash_logo_load_size (GtkWidget *window);
static void splash_logo_draw (GtkWidget *widget);
static void splash_text_draw (GtkWidget *widget);
static void splash_logo_expose (GtkWidget *widget);


static gint is_app_exit_finish_done = FALSE;

static ProcArg quit_args[] =
{
  { PDB_INT32,
    "kill",
    "Flag specifying whether to kill the gimp process or exit normally" },
};

static ProcRecord quit_proc =
{
  "gimp_quit",
  "Causes the gimp to exit gracefully",
  "The internal procedure which can either be used to make the gimp quit normally, or to have the gimp clean up its resources and exit immediately. The normaly shutdown process allows for querying the user to save any dirty images.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,
  1,
  quit_args,
  0,
  NULL,
  { { quit_invoker } },
};


void
gimp_init (int    gimp_argc,
	   char **gimp_argv)
{
  /* Initialize the application */
  app_init ();

  /* Parse the rest of the command line arguments as images to load */
  if (gimp_argc > 0)
    while (gimp_argc--)
      {
	if (*gimp_argv)
	  file_open (*gimp_argv, *gimp_argv);
	gimp_argv++;
      }

  batch_init ();

  /* Handle showing dialogs with gdk_quit_adds here  */
  if (!no_interface && show_tips)
    tips_dialog_create ();
}


static GtkWidget *logo_area = NULL;
static GdkPixmap *logo_pixmap = NULL;
static int logo_width = 0;
static int logo_height = 0;
static int logo_area_width = 0;
static int logo_area_height = 0;
static int show_logo = SHOW_NEVER;
static int max_label_length = MAXPATHLEN;

static int
splash_logo_load_size (GtkWidget *window)
{
  char buf[1024];
  FILE *fp;

  if (logo_pixmap)
    return TRUE;

  sprintf (buf, "%s/gimp1_1_splash.ppm", DATADIR);

  fp = fopen (buf, "r");
  if (!fp)
    return 0;

  fgets (buf, 1024, fp);
  if (strcmp (buf, "P6\n") != 0)
    {
      fclose (fp);
      return 0;
    }

  fgets (buf, 1024, fp);
  fgets (buf, 1024, fp);
  sscanf (buf, "%d %d", &logo_width, &logo_height);

  fclose (fp);
  return TRUE;
}

static int
splash_logo_load (GtkWidget *window)
{
  GtkWidget *preview;
  GdkGC *gc;
  char buf[1024];
  unsigned char *pixelrow;
  FILE *fp;
  int count;
  int i;

  if (logo_pixmap)
    return TRUE;

  sprintf (buf, "%s/gimp1_1_splash.ppm", DATADIR);

  fp = fopen (buf, "r");
  if (!fp)
    return 0;

  fgets (buf, 1024, fp);
  if (strcmp (buf, "P6\n") != 0)
    {
      fclose (fp);
      return 0;
    }

  fgets (buf, 1024, fp);
  fgets (buf, 1024, fp);
  sscanf (buf, "%d %d", &logo_width, &logo_height);

  fgets (buf, 1024, fp);
  if (strcmp (buf, "255\n") != 0)
    {
      fclose (fp);
      return 0;
    }

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), logo_width, logo_height);
  pixelrow = g_new (guchar, logo_width * 3);

  for (i = 0; i < logo_height; i++)
    {
      count = fread (pixelrow, sizeof (unsigned char), logo_width * 3, fp);
      if (count != (logo_width * 3))
	{
	  gtk_widget_destroy (preview);
	  g_free (pixelrow);
	  fclose (fp);
	  return 0;
	}
      gtk_preview_draw_row (GTK_PREVIEW (preview), pixelrow, 0, i, logo_width);
    }

  gtk_widget_realize (window);
  logo_pixmap = gdk_pixmap_new (window->window, logo_width, logo_height,
				gtk_preview_get_visual ()->depth);
  gc = gdk_gc_new (logo_pixmap);
  gtk_preview_put (GTK_PREVIEW (preview),
		   logo_pixmap, gc,
		   0, 0, 0, 0, logo_width, logo_height);
  gdk_gc_destroy (gc);

  gtk_widget_unref (preview);
  g_free (pixelrow);

  fclose (fp);
  return TRUE;
}

static void
splash_text_draw (GtkWidget *widget)
{
  GdkFont *font = NULL;

  font = gdk_font_load ("-Adobe-Helvetica-Bold-R-Normal--*-140-*-*-*-*-*-*");
  gdk_draw_string (widget->window,
		   font,
		   widget->style->fg_gc[GTK_STATE_NORMAL],
		   ((logo_area_width - gdk_string_width (font, NAME)) / 2), 
		   (0.25 * logo_area_height),
		   NAME);

  font = gdk_font_load ("-Adobe-Helvetica-Bold-R-Normal--*-120-*-*-*-*-*-*");
  gdk_draw_string (widget->window,
		   font,
		   widget->style->fg_gc[GTK_STATE_NORMAL],
		   ((logo_area_width - gdk_string_width (font, GIMP_VERSION)) / 2), 
		   (0.45 * logo_area_height),
		   GIMP_VERSION);
  gdk_draw_string (widget->window,
		   font,
		   widget->style->fg_gc[GTK_STATE_NORMAL],
		   ((logo_area_width - gdk_string_width (font, BROUGHT)) / 2), 
		   (0.65 * logo_area_height),
		   BROUGHT);
  gdk_draw_string (widget->window,
		   font,
		   widget->style->fg_gc[GTK_STATE_NORMAL],
		   ((logo_area_width - gdk_string_width (font, AUTHORS)) / 2), 
		   (0.80 * logo_area_height),
		   AUTHORS);
  /*  
   *  This is a hack: we try to compute a good guess for the maximum number
   *  of charcters that will fit into the splash-screen using the given font 
   */
  max_label_length = (float)strlen (AUTHORS) * 
        ( (float)logo_area_width / (float)gdk_string_width (font, AUTHORS) ); 
}

static void
splash_logo_draw (GtkWidget *widget)
{
  gdk_draw_pixmap (widget->window,
		   widget->style->black_gc,
		   logo_pixmap, 
		   0, 0,
		   ((logo_area_width - logo_width) / 2), ((logo_area_height - logo_height) / 2),
		   logo_width, logo_height);
}

static void
splash_logo_expose (GtkWidget *widget)
{
  switch (show_logo) {
     case SHOW_NEVER: 
     case SHOW_LATER:
       splash_text_draw (widget);
       break;
     case SHOW_NOW:
       splash_logo_draw (widget);
  }
}

static GtkWidget *win_initstatus = NULL;
static GtkWidget *label1 = NULL;
static GtkWidget *label2 = NULL;
static GtkWidget *pbar = NULL;
static gint idle_tag = -1;

static void
destroy_initialization_status_window(void)
{
  if(win_initstatus)
    {
      gtk_widget_destroy(win_initstatus);
      if (logo_pixmap != NULL)
	gdk_pixmap_unref(logo_pixmap);
      win_initstatus = label1 = label2 = pbar = logo_area = NULL;
      logo_pixmap = NULL;
      gtk_idle_remove(idle_tag);
    }
}

static void
make_initialization_status_window(void)
{
  if (no_interface == FALSE)
    {
      if (no_splash == FALSE)
	{
	  GtkWidget *vbox;

	  win_initstatus = gtk_window_new(GTK_WINDOW_DIALOG);
	  gtk_signal_connect (GTK_OBJECT (win_initstatus), "delete_event",
			      GTK_SIGNAL_FUNC (gtk_true),
			      NULL);
	  gtk_window_set_wmclass (GTK_WINDOW(win_initstatus), "gimp_startup", "Gimp");
	  gtk_window_set_title(GTK_WINDOW(win_initstatus),
		               "GIMP Startup");

	  if (no_splash_image == FALSE && splash_logo_load_size (win_initstatus)) 
	    {
	      show_logo = SHOW_LATER;
	    }

	  vbox = gtk_vbox_new(FALSE, 4);
	  gtk_container_add(GTK_CONTAINER(win_initstatus), vbox);

	  gtk_widget_push_visual (gtk_preview_get_visual ());
	  gtk_widget_push_colormap  (gtk_preview_get_cmap ());

	  logo_area = gtk_drawing_area_new ();

	  gtk_widget_pop_colormap ();
	  gtk_widget_pop_visual ();

	  gtk_signal_connect (GTK_OBJECT (logo_area), "expose_event",
			      (GtkSignalFunc) splash_logo_expose, NULL);
	  logo_area_width = ( logo_width > LOGO_WIDTH_MIN ) ? logo_width : LOGO_WIDTH_MIN;
	  logo_area_height = ( logo_height > LOGO_HEIGHT_MIN ) ? logo_height : LOGO_HEIGHT_MIN;
	  gtk_drawing_area_size (GTK_DRAWING_AREA (logo_area), logo_area_width, logo_area_height);
	  gtk_box_pack_start_defaults(GTK_BOX(vbox), logo_area);

	  label1 = gtk_label_new("");
	  gtk_box_pack_start_defaults(GTK_BOX(vbox), label1);
	  label2 = gtk_label_new("");
	  gtk_box_pack_start_defaults(GTK_BOX(vbox), label2);
      
	  pbar = gtk_progress_bar_new();
	  gtk_box_pack_start_defaults(GTK_BOX(vbox), pbar);
      
	  gtk_widget_show(vbox);
	  gtk_widget_show (logo_area);
	  gtk_widget_show(label1);
	  gtk_widget_show(label2);
	  gtk_widget_show(pbar);
      
	  gtk_window_position(GTK_WINDOW(win_initstatus),
			      GTK_WIN_POS_CENTER);
      
	  gtk_widget_show(win_initstatus);

	  gtk_window_set_policy (GTK_WINDOW (win_initstatus), FALSE, TRUE, FALSE);
	}
    }
}

void
app_init_update_status(char *label1val,
		       char *label2val,
		       float pct_progress)
{
  char *temp;

  if(no_interface == FALSE && no_splash == FALSE && win_initstatus)
    {
      GdkRectangle area = {0, 0, -1, -1};
      if(label1val
	 && strcmp(label1val, GTK_LABEL(label1)->label))
	{
	  gtk_label_set(GTK_LABEL(label1), label1val);
	}
      if(label2val
	 && strcmp(label2val, GTK_LABEL(label2)->label))
	{
	  while ( strlen (label2val) > max_label_length )
	    {
	      temp = strchr (label2val, '/');
	      if (temp == NULL)  /* for sanity */
		break;
	      temp++;
	      label2val = temp;
	    }
	  gtk_label_set(GTK_LABEL(label2), label2val);
	}
      if(pct_progress >= 0
	 && GTK_PROGRESS_BAR(pbar)->percentage != pct_progress)
	{
	  gtk_progress_bar_update(GTK_PROGRESS_BAR(pbar), pct_progress);
	}
      gtk_widget_draw(win_initstatus, &area);
      idle_tag = gtk_idle_add((GtkFunction) gtk_true, NULL);
      gtk_main_iteration();
      gtk_idle_remove(idle_tag);
    }
}

/* #define RESET_BAR() app_init_update_status("", "", 0) */
#define RESET_BAR()

void
app_init (void)
{
  char filename[MAXPATHLEN];
  char *gimp_dir;
  char *path;

  gimp_dir = gimp_directory ();
  if (gimp_dir[0] != '\000')
    {
      sprintf (filename, "%s/gtkrc", gimp_dir);

      if ((be_verbose == TRUE) || (no_splash == TRUE))
	g_print ("parsing \"%s\"\n", filename);

      gtk_rc_parse (filename);
    }

  make_initialization_status_window();
  if (no_interface == FALSE && no_splash == FALSE && win_initstatus) {
    splash_text_draw (logo_area);
  }

  /* Create the context of all existing images */

  image_context=gimp_set_new(GIMP_TYPE_IMAGE, TRUE);
  
  /*
   *  Initialize the procedural database
   *    We need to do this first because any of the init
   *    procedures might install or query it as needed.
   */
  procedural_db_init ();
  RESET_BAR();
  internal_procs_init ();
  RESET_BAR();
  procedural_db_register (&quit_proc);

  RESET_BAR();
  parse_gimprc ();         /*  parse the local GIMP configuration file  */

  /* Now we are ready to draw the splash-screen-image to the start-up window */
  if (no_interface == FALSE)
    {
      if (no_splash_image == FALSE && show_logo && splash_logo_load (win_initstatus)) {
	show_logo = SHOW_NOW;
	splash_logo_draw (logo_area);
      }
    }

  RESET_BAR();
  file_ops_pre_init ();    /*  pre-initialize the file types  */
  RESET_BAR();
  xcf_init ();             /*  initialize the xcf file format routines */

  app_init_update_status ("Looking for data files", "Brushes", 0.00);
  brushes_init (no_data);         /*  initialize the list of gimp brushes  */
  app_init_update_status (NULL, "Patterns", 0.25);
  patterns_init (no_data);        /*  initialize the list of gimp patterns  */
  app_init_update_status (NULL, "Palettes", 0.50);
  palettes_init (no_data);        /*  initialize the list of gimp palettes  */
  app_init_update_status (NULL, "Gradients", 0.75);
  gradients_init (no_data);       /*  initialize the list of gimp gradients  */
  app_init_update_status (NULL, NULL, 1.00);

  plug_in_init ();         /*  initialize the plug in structures  */
  RESET_BAR();
  file_ops_post_init ();   /*  post-initialize the file types  */

  /* Add the swap file  */
  if (swap_path == NULL)
    swap_path = "/tmp";
  path = g_new (gchar, strlen (swap_path) + 32);
  sprintf (path, "%s/gimpswap.%ld", swap_path, (long)getpid ());
  tile_swap_add (path, NULL, NULL);
  g_free (path);

  destroy_initialization_status_window();

  /*  Things to do only if there is an interface  */
  if (no_interface == FALSE)
    {
      get_standard_colormaps ();
      devices_init ();
      session_init ();
      create_toolbox ();
      gximage_init ();
      render_setup (transparency_type, transparency_size);
      tools_options_dialog_new ();
      tools_select (RECT_SELECT);
      message_handler = MESSAGE_BOX;
    }

  color_transfer_init ();
  get_active_brush ();
  get_active_pattern ();
  paint_funcs_setup ();

}

int
app_exit_finish_done (void)
{
  return is_app_exit_finish_done;
}

void
app_exit_finish (void)
{
  if (app_exit_finish_done ())
    return;
  is_app_exit_finish_done = TRUE;

  message_handler = CONSOLE;

  lc_dialog_free ();
  gdisplays_delete ();
  global_edit_free ();
  named_buffers_free ();
  swapping_free ();
  brushes_free ();
  patterns_free ();
  palettes_free ();
  gradients_free ();
  grad_free_gradient_editor ();
  hue_saturation_free ();
  curves_free ();
  levels_free ();
  brush_select_dialog_free ();
  pattern_select_dialog_free ();
  palette_free ();
  paint_funcs_free ();
  plug_in_kill ();
  procedural_db_free ();
  menus_quit ();
  tile_swap_exit ();

  /*  Things to do only if there is an interface  */
  if (no_interface == FALSE)
    {
      gximage_free ();
      render_free ();
      tools_options_dialog_free ();
      save_sessionrc ();
    }

  /*  gtk_exit (0); */
  gtk_main_quit();
}

void
app_exit (int kill_it)
{
  /*  If it's the user's perogative, and there are dirty images  */
  if (kill_it == 0 && gdisplays_dirty () && no_interface == FALSE)
    really_quit_dialog ();
  else if (no_interface == FALSE)
    toolbox_free ();
  else
    app_exit_finish ();
}

/********************************************************
 *   Routines to query exiting the application          *
 ********************************************************/

static void
really_quit_callback (GtkButton *button,
		      GtkWidget *dialog)
{
  gtk_widget_destroy (dialog);
  toolbox_free ();
}

static void
really_quit_cancel_callback (GtkWidget *widget,
			     GtkWidget *dialog)
{
  menus_set_sensitive ("<Toolbox>/File/Quit", TRUE);
  menus_set_sensitive ("<Image>/File/Quit", TRUE);
  gtk_widget_destroy (dialog);
}

static gint
really_quit_delete_callback (GtkWidget *widget,
			     GdkEvent  *event,
			     gpointer client_data) 
{
  really_quit_cancel_callback (widget, (GtkWidget *) client_data);

  return TRUE;
}

static void
really_quit_dialog ()
{
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *label;

  menus_set_sensitive ("<Toolbox>/File/Quit", FALSE);
  menus_set_sensitive ("<Image>/File/Quit", FALSE);

  dialog = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "really_quit", "Gimp");
  gtk_window_set_title (GTK_WINDOW (dialog), "Really Quit?");
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), 2);

  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
		      (GtkSignalFunc) really_quit_delete_callback,
		      dialog);
 
  button = gtk_button_new_with_label ("Yes");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) really_quit_callback,
		      dialog);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("No");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) really_quit_cancel_callback,
		      dialog);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  label = gtk_label_new ("Some files unsaved.  Quit the GIMP?");
  gtk_misc_set_padding (GTK_MISC (label), 10, 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dialog);
}

static Argument*
quit_invoker (Argument *args)
{
  Argument *return_args;
  int kill_it;

  kill_it = args[0].value.pdb_int;
  app_exit (kill_it);

  return_args = procedural_db_return_args (&quit_proc, TRUE);

  return return_args;
}
