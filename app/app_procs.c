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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include "appenv.h"
#include "app_procs.h"
#include "batch.h"
#include "brushes.h"
#include "color_transfer.h"
#include "curves.h"
#include "gdisplay.h"
#include "colormaps.h"
#include "fileops.h"
#include "gimprc.h"
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
#include "temp_buf.h"
#include "tile_swap.h"
#include "tips_dialog.h"
#include "tools.h"
#include "undo.h"
#include "xcf.h"
#include <gtk/gtk.h>

/*  Function prototype for affirmation dialog when exiting application  */
static void      really_quit_dialog (void);
static Argument* quit_invoker       (Argument *args);
static void make_initialization_status_window(void);
static void destroy_initialization_status_window(void);


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
      win_initstatus = label1 = label2 = pbar = NULL;
      gtk_idle_remove(idle_tag);
    }
}

static void my_idle_proc(void)
{
  /* Do nothing. This is needed to stop the GIMP
     from blocking in gtk_main_iteration() */
}

static void
make_initialization_status_window(void)
{
  if(no_interface == FALSE)
    {
      GtkWidget *vbox;

      win_initstatus = gtk_window_new(GTK_WINDOW_DIALOG);
      gtk_window_set_wmclass (GTK_WINDOW(win_initstatus), "gimp_startup", "Gimp");
      gtk_window_set_title(GTK_WINDOW(win_initstatus),
			   "GIMP Startup");

      vbox = gtk_vbox_new(TRUE, 5);
      gtk_container_add(GTK_CONTAINER(win_initstatus), vbox);
      
      label1 = gtk_label_new("");
      gtk_box_pack_start_defaults(GTK_BOX(vbox), label1);
      label2 = gtk_label_new("");
      gtk_box_pack_start_defaults(GTK_BOX(vbox), label2);
      
      pbar = gtk_progress_bar_new();
      gtk_box_pack_start_defaults(GTK_BOX(vbox), pbar);
      
      gtk_widget_show(vbox);
      gtk_widget_show(label1);
      gtk_widget_show(label2);
      gtk_widget_show(pbar);
      
      gtk_window_position(GTK_WINDOW(win_initstatus),
			  GTK_WIN_POS_CENTER);
      
      gtk_widget_show(win_initstatus);

      gtk_window_set_policy (GTK_WINDOW (win_initstatus), FALSE, TRUE, FALSE);
    }
}

void
app_init_update_status(char *label1val,
		       char *label2val,
		       float pct_progress)
{
  if(no_interface == FALSE
     && win_initstatus)
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
	  gtk_label_set(GTK_LABEL(label2), label2val);
	}
      if(pct_progress >= 0
	 && GTK_PROGRESS_BAR(pbar)->percentage != pct_progress)
	{
	  gtk_progress_bar_update(GTK_PROGRESS_BAR(pbar), pct_progress);
	}
      gtk_widget_draw(win_initstatus, &area);
      idle_tag = gtk_idle_add((GtkFunction) my_idle_proc, NULL);
      gtk_main_iteration();
      gtk_idle_remove(idle_tag);
    }
}

/* #define RESET_BAR() app_init_update_status("", "", 0) */
#define RESET_BAR()

void
app_init ()
{
  char filename[MAXPATHLEN];
  char *gimp_dir;
  char *path;

  make_initialization_status_window();

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

  gimp_dir = gimp_directory ();
  if (gimp_dir[0] != '\000')
    {
      sprintf (filename, "%s/gtkrc", gimp_dir);

      if (be_verbose == TRUE)
      g_print ("parsing \"%s\"\n", filename);
      app_init_update_status("Resource configuration", filename, -1);

      gtk_rc_parse (filename);
    }

  RESET_BAR();
  file_ops_pre_init ();    /*  pre-initialize the file types  */
  RESET_BAR();
  xcf_init ();             /*  initialize the xcf file format routines */
  RESET_BAR();
  parse_gimprc ();         /*  parse the local GIMP configuration file  */
  if (no_data == FALSE)
    {
      RESET_BAR();
      brushes_init ();         /*  initialize the list of gimp brushes  */
      RESET_BAR();
      patterns_init ();        /*  initialize the list of gimp patterns  */
      RESET_BAR();
      palettes_init ();        /*  initialize the list of gimp palettes  */
      RESET_BAR();
      gradients_init ();       /*  initialize the list of gimp gradients  */
    }
  RESET_BAR();
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
      create_toolbox ();
      gximage_init ();
      render_setup (transparency_type, transparency_size);
      tools_options_dialog_new ();
      tools_select (RECT_SELECT);
      if (show_tips)
	tips_dialog_create ();
    }

  color_transfer_init ();
  get_active_brush ();
  get_active_pattern ();
  paint_funcs_setup ();

}

void
app_exit_finish ()
{
  static gint once = 0;
  if (once) return;
  once = 1;

  lc_dialog_free ();
  gdisplays_delete ();
  global_edit_free ();
  named_buffers_free ();
  swapping_free ();
  brushes_free ();
  patterns_free ();
  palettes_free ();
  gradients_free ();
  hue_saturation_free ();
  curves_free ();
  levels_free ();
  brush_select_dialog_free ();
  pattern_select_dialog_free ();
  palette_free ();
  paint_funcs_free ();
  procedural_db_free ();
  plug_in_kill ();
  menus_quit ();
  tile_swap_exit ();

  /*  Things to do only if there is an interface  */
  if (no_interface == FALSE)
    {
      gximage_free ();
      render_free ();
      tools_options_dialog_free ();
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
  else
    toolbox_free ();
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
really_quit_cancel_callback (GtkButton *button,
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
  really_quit_cancel_callback (GTK_BUTTON(widget), (GtkWidget *) client_data);

  return FALSE;
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
