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


/*  Function prototype for affirmation dialog when exiting application  */
static void      app_exit_finish    (void);
static void      really_quit_dialog (void);
static Argument* quit_invoker       (Argument *args);


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

void
app_init ()
{
  char filename[MAXPATHLEN];
  char *gimp_dir;
  char *path;

  /*
   *  Initialize the procedural database
   *    We need to do this first because any of the init
   *    procedures might install or query it as needed.
   */
  procedural_db_init ();
  internal_procs_init ();
  procedural_db_register (&quit_proc);

  gimp_dir = gimp_directory ();
  if (gimp_dir[0] != '\000')
    {
      sprintf (filename, "%s/gtkrc", gimp_dir);
      g_print ("parsing \"%s\"\n", filename);
      gtk_rc_parse (filename);
    }

  file_ops_pre_init ();    /*  pre-initialize the file types  */
  xcf_init ();             /*  initialize the xcf file format routines */
  parse_gimprc ();         /*  parse the local GIMP configuration file  */
  if (no_data == FALSE)
    {
      brushes_init ();         /*  initialize the list of gimp brushes  */
      patterns_init ();        /*  initialize the list of gimp patterns  */
      palettes_init ();        /*  initialize the list of gimp palettes  */
      gradients_init ();       /*  initialize the list of gimp gradients  */
    }
  plug_in_init ();         /*  initialize the plug in structures  */
  file_ops_post_init ();   /*  post-initialize the file types  */

  /* Add the swap file  */
  if (swap_path == NULL)
    swap_path = "/tmp";
  path = g_new (gchar, strlen (swap_path) + 32);
  sprintf (path, "%s/gimpswap.%ld", swap_path, (long)getpid ());
  tile_swap_add (path, NULL, NULL);
  g_free (path);

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

static void
app_exit_finish ()
{
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

  gtk_exit (0);
}

void
app_exit (int kill_it)
{
  /*  If it's the user's perogative, and there are dirty images  */
  if (kill_it == 0 && gdisplays_dirty () && no_interface == FALSE)
    really_quit_dialog ();
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
  app_exit_finish ();
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
