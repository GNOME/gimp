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
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "appenv.h"
#include "actionarea.h"
#include "install.h"
#include "interface.h"
#include "gimprc.h"


static void install_run (InstallCallback);
static void install_help (InstallCallback);
static void help_install_callback (GtkWidget *, gpointer);
static void help_ignore_callback (GtkWidget *, gpointer);
static void help_quit_callback (GtkWidget *, gpointer);
static void install_continue_callback (GtkWidget *, gpointer);
static void install_quit_callback (GtkWidget *, gpointer);

static GtkWidget *help_widget;
static GtkWidget *install_widget;

void
install_verify (InstallCallback install_callback)
{
  int properly_installed = TRUE;
  char *filename;
  struct stat stat_buf;

  filename = gimp_directory ();
  if ('\000' == filename[0])
    {
      g_warning ("No home directory--skipping GIMP user installation.");
      (* install_callback) ();
    }

  if (stat (filename, &stat_buf) != 0)
    properly_installed = FALSE;

  /*  If there is already a proper installation, invoke the callback  */
  if (properly_installed)
    {
      (* install_callback) ();
    }
  /*  Otherwise, prepare for installation  */
  else if (no_interface)
    {
      g_print ("The GIMP is not properly installed for the current user\n");
      g_print ("User installation was skipped because the '--nointerface' flag was encountered\n");
      g_print ("To perform user installation, run the GIMP without the '--nointerface' flag\n");

      (* install_callback) ();
    }
  else
    {
      install_help (install_callback);
    }
}


/*********************/
/*  Local functions  */

static void
install_help (InstallCallback callback)
{
  static ActionAreaItem action_items[] =
  {
    { "Install", help_install_callback, NULL, NULL },
    { "Ignore", help_ignore_callback, NULL, NULL },
    { "Quit", help_quit_callback, NULL, NULL }
  };
  GtkWidget *text;
  GtkWidget *table;
  GtkWidget *vsb;
  GtkAdjustment *vadj;
  GdkFont   *font_strong;
  GdkFont   *font_emphasis;
  GdkFont   *font;

  help_widget = gtk_dialog_new ();
  gtk_signal_connect (GTK_OBJECT (help_widget), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_true),
		      NULL);
  gtk_window_set_wmclass (GTK_WINDOW (help_widget), "gimp_installation", "Gimp");
  gtk_window_set_title (GTK_WINDOW (help_widget), "GIMP Installation");
  gtk_window_position (GTK_WINDOW (help_widget), GTK_WIN_POS_CENTER);

  vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  vsb = gtk_vscrollbar_new (vadj);
  text = gtk_text_new (NULL, vadj);
  gtk_text_set_editable (GTK_TEXT (text), FALSE);
  gtk_widget_set_usize (text, 450, 475);

  table  = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);

  action_items[0].user_data = (void *) callback;
  action_items[1].user_data = (void *) callback;
  action_items[2].user_data = (void *) callback;
  build_action_area (GTK_DIALOG (help_widget), action_items, 3, 0);


  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (help_widget)->vbox), table, TRUE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), vsb, 1, 2, 0, 1,
		    0, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  gtk_container_border_width (GTK_CONTAINER (table), 2);

  font_strong = gdk_font_load ("-*-helvetica-bold-r-normal-*-*-120-*-*-*-*-*-*");
  font_emphasis = gdk_font_load ("-*-helvetica-medium-o-normal-*-*-100-*-*-*-*-*-*");
  font = gdk_font_load ("-*-helvetica-medium-r-normal-*-*-100-*-*-*-*-*-*");

  /*  Realize the widget before allowing new text to be inserted  */
  gtk_widget_realize (text);

  gtk_text_insert (GTK_TEXT (text), font_strong, NULL, NULL,
		   "The GIMP - GNU Image Manipulation Program\n\n", -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   "Copyright (C) 1995 Spencer Kimball and Peter Mattis\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "This program is free software; you can redistribute it and/or modify\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "it under the terms of the GNU General Public License as published by\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "the Free Software Foundation; either version 2 of the License, or\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "(at your option) any later version.\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "This program is distributed in the hope that it will be useful,\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "but WITHOUT ANY WARRANTY; without even the implied warranty of\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "See the GNU General Public License for more details.\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "You should have received a copy of the GNU General Public License\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "along with this program; if not, write to the Free Software\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\n\n", -1);

  gtk_text_insert (GTK_TEXT (text), font_strong, NULL, NULL,
		   "Personal GIMP Installation\n\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "For a proper GIMP installation, a subdirectory called\n", -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   gimp_directory (), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   " needs to be created.  This\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "subdirectory will contain a number of important files:\n\n", -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   "gimprc\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tThe gimprc is used to store personal preferences\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tsuch as default GIMP behaviors & plug-in hotkeys.\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tPaths to search for brushes, palettes, gradients\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tpatterns, and plug-ins are also configured here.\n", -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   "pluginrc\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tPlug-ins and extensions are extern programs run by\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tthe GIMP which provide additional functionality.\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tThese programs are searched for at run-time and\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tinformation about their functionality and mod-times\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tis cached in this file.  This file is intended to\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tbe GIMP-readable only, and should not be edited.\n", -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   "brushes\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tThis is a subdirectory which can be used to store\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tuser defined brushes.  The default gimprc file\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tchecks this subdirectory in addition to the system-\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\twide gimp brushes installation when searching for\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tbrushes.\n", -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   "gradients\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tThis is a subdirectory which can be used to store\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tuser defined gradients.  The default gimprc file\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tchecks this subdirectory in addition to the system-\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\twide gimp gradients installation when searching for\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tgradients.\n", -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   "palettes\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tThis is a subdirectory which can be used to store\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tuser defined palettes.  The default gimprc file\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tchecks only this subdirectory (not the system-wide\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tinstallation) when searching for palettes.  During\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tinstallation, the system palettes will be copied\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\there.  This is done to allow modifications made to\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tpalettes during GIMP execution to persist across\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tsessions.\n", -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   "patterns\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tThis is a subdirectory which can be used to store\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tuser defined patterns.  The default gimprc file\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tchecks this subdirectory in addition to the system-\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\twide gimp patterns installation when searching for\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tpatterns.\n", -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   "plug-ins\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tThis is a subdirectory which can be used to store\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tuser created, temporary, or otherwise non-system-\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tsupported plug-ins.  The default gimprc file\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tchecks this subdirectory in addition to the system-\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\twide GIMP plug-in directories when searching for\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tplug-ins.\n", -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   "scripts\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tThis subdirectory is used by the GIMP to store \n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tuser created and isntalled scripts. The default gimprc\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tfile checks this subdirectory in addition to the system\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\t-wide gimp scripts subdirectory when searching for scripts\n", -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   "tmp\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tThis subdirectory is used by the GIMP to temporarily\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tstore undo buffers to reduce memory usage.  If GIMP is\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tunceremoniously killed, files may persist in this directory\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tof the form: gimp<#>.<#>.  These files are useless across\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\t\tGIMP sessions and can be destroyed with impunity.\n", -1);

  gtk_widget_show (vsb);
  gtk_widget_show (text);
  gtk_widget_show (table);
  gtk_widget_show (help_widget);
}

static void
help_install_callback (GtkWidget *w,
		       gpointer   client_data)
{
  InstallCallback callback;

  callback = (InstallCallback) client_data;
  gtk_widget_destroy (help_widget);
  install_run (callback);
}

static void
help_ignore_callback (GtkWidget *w,
		      gpointer   client_data)
{
  InstallCallback callback;

  callback = (InstallCallback) client_data;
  gtk_widget_destroy (help_widget);
  (* callback) ();
}

static void
help_quit_callback (GtkWidget *w,
		    gpointer   client_data)
{
  gtk_widget_destroy (help_widget);
  gtk_exit (0);
}

static void
install_run (InstallCallback callback)
{
  static ActionAreaItem action_items[] =
  {
    { "Continue", install_continue_callback, NULL, NULL },
    { "Quit", install_quit_callback, NULL, NULL }
  };
  GtkWidget *text;
  GtkWidget *table;
  GtkWidget *vsb;
  GtkAdjustment *vadj;
  GdkFont   *font_strong;
  GdkFont   *font;
  FILE *pfp;
  char buffer[2048];
  struct stat stat_buf;
  int err;
  int executable = TRUE;

  install_widget = gtk_dialog_new ();
  gtk_signal_connect (GTK_OBJECT (install_widget), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_true),
		      NULL);
  gtk_window_set_wmclass (GTK_WINDOW (install_widget), "installation_log", "Gimp");
  gtk_window_set_title (GTK_WINDOW (install_widget), "Installation Log");
  gtk_window_position (GTK_WINDOW (install_widget), GTK_WIN_POS_CENTER);
  vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  vsb  = gtk_vscrollbar_new (vadj);
  text = gtk_text_new (NULL, vadj);
  gtk_widget_set_usize (text, 384, 256);

  table  = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);

  action_items[0].user_data = (void *) callback;
  action_items[1].user_data = (void *) callback;
  build_action_area (GTK_DIALOG (install_widget), action_items, 2, 0);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (install_widget)->vbox), table, TRUE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), vsb, 1, 2, 0, 1,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    0, 0);

  gtk_container_border_width (GTK_CONTAINER (table), 2);

  font_strong = gdk_font_load ("-*-helvetica-bold-r-normal-*-*-120-*-*-*-*-*-*");
  font = gdk_font_load ("-*-helvetica-medium-r-normal-*-*-120-*-*-*-*-*-*");

  /*  Realize the text widget before inserting text strings  */
  gtk_widget_realize (text);

  gtk_text_insert (GTK_TEXT (text), font_strong, NULL, NULL, "User Installation Log\n\n", -1);

  /*  Generate output  */
  sprintf (buffer, "%s/user_install", DATADIR);
  if ((err = stat (buffer, &stat_buf)) != 0)
    {
      gtk_text_insert (GTK_TEXT (text), font, NULL, NULL, buffer, -1);
      gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		       " does not exist.  Cannot install.\n", -1);
      executable = FALSE;
    }
  else if (! (S_IXUSR & stat_buf.st_mode) || ! (S_IRUSR & stat_buf.st_mode))
    {
      gtk_text_insert (GTK_TEXT (text), font, NULL, NULL, buffer, -1);
      gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		       " has invalid permissions.\nCannot install.", -1);
      executable = FALSE;
    }

  if (executable == TRUE)
    {
      sprintf (buffer, "%s/user_install %s %s", DATADIR, DATADIR,
	       gimp_directory ());
      if ((pfp = popen (buffer, "r")) != NULL)
	{
	  while (fgets (buffer, 2048, pfp))
	    gtk_text_insert (GTK_TEXT (text), font, NULL, NULL, buffer, -1);
	  pclose (pfp);

	  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
			   "\nInstallation successful!\n", -1);
	}
      else
	executable = FALSE;
    }
  if (executable == FALSE)
    gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		     "\nInstallation failed.  Contact system administrator.\n", -1);

  gtk_widget_show (vsb);
  gtk_widget_show (text);
  gtk_widget_show (table);
  gtk_widget_show (install_widget);
}

static void
install_continue_callback (GtkWidget *w,
			   gpointer   client_data)
{
  InstallCallback callback;

  callback = (InstallCallback) client_data;
  gtk_widget_destroy (install_widget);
  (* callback) ();
}

static void
install_quit_callback (GtkWidget *w,
		       gpointer   client_data)
{
  gtk_widget_destroy (install_widget);
  gtk_exit (0);
}
