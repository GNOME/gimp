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
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "appenv.h"
#include "actionarea.h"
#include "install.h"
#include "interface.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"

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
      g_message (_("No home directory--skipping GIMP user installation."));
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
      g_print (_("The GIMP is not properly installed for the current user\n"));
      g_print (_("User installation was skipped because the '--nointerface' flag was encountered\n"));
      g_print (_("To perform user installation, run the GIMP without the '--nointerface' flag\n"));

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
    { N_("Install"), help_install_callback, NULL, NULL },
    { N_("Ignore"), help_ignore_callback, NULL, NULL },
    { N_("Quit"), help_quit_callback, NULL, NULL }
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
  gtk_window_set_title (GTK_WINDOW (help_widget), _("GIMP Installation"));
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
		   _("The GIMP - GNU Image Manipulation Program\n\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   _("Copyright (C) 1995 Spencer Kimball and Peter Mattis\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("This program is free software; you can redistribute it and/or modify\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("it under the terms of the GNU General Public License as published by\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("the Free Software Foundation; either version 2 of the License, or\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("(at your option) any later version.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("This program is distributed in the hope that it will be useful,\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("but WITHOUT ANY WARRANTY; without even the implied warranty of\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("See the GNU General Public License for more details.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\n", -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("You should have received a copy of the GNU General Public License\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("along with this program; if not, write to the Free Software\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   "\n\n", -1);

  gtk_text_insert (GTK_TEXT (text), font_strong, NULL, NULL,
		   _("Personal GIMP Installation\n\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("For a proper GIMP installation, a subdirectory called\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   gimp_directory (), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _(" needs to be created.  This\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("subdirectory will contain a number of important files:\n\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   _("gimprc\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tThe gimprc is used to store personal preferences\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tsuch as default GIMP behaviors & plug-in hotkeys.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tPaths to search for brushes, palettes, gradients\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tpatterns, and plug-ins are also configured here.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   _("pluginrc\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tPlug-ins and extensions are extern programs run by\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tthe GIMP which provide additional functionality.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tThese programs are searched for at run-time and\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tinformation about their functionality and mod-times\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tis cached in this file.  This file is intended to\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tbe GIMP-readable only, and should not be edited.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   _("brushes\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tThis is a subdirectory which can be used to store\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tuser defined brushes.  The default gimprc file\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tchecks this subdirectory in addition to the system-\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\twide gimp brushes installation when searching for\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tbrushes.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   _("gradients\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tThis is a subdirectory which can be used to store\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tuser defined gradients.  The default gimprc file\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tchecks this subdirectory in addition to the system-\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\twide gimp gradients installation when searching for\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tgradients.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   _("gfig\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tThis is a subdirectory which can be used to store\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tuser defined figures to be used by the gfig plug-in.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tThe default gimprc file checks this subdirectory in\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\taddition to the systemwide gimp gfig installation\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\twhen searching for gfig figures.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   _("gflares\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tThis is a subdirectory which can be used to store\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tuser defined gflares to be used by the gflare plug-in.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tThe default gimprc file checks this subdirectory in\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\taddition to the systemwide gimp gflares installation\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\twhen searching for gflares.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   _("palettes\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tThis is a subdirectory which can be used to store\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tuser defined palettes.  The default gimprc file\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tchecks only this subdirectory (not the system-wide\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tinstallation) when searching for palettes.  During\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tinstallation, the system palettes will be copied\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\there.  This is done to allow modifications made to\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tpalettes during GIMP execution to persist across\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tsessions.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   _("patterns\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tThis is a subdirectory which can be used to store\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tuser defined patterns.  The default gimprc file\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tchecks this subdirectory in addition to the system-\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\twide gimp patterns installation when searching for\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tpatterns.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   _("plug-ins\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tThis is a subdirectory which can be used to store\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tuser created, temporary, or otherwise non-system-\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tsupported plug-ins.  The default gimprc file\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tchecks this subdirectory in addition to the system-\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\twide GIMP plug-in directories when searching for\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tplug-ins.\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   _("scripts\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tThis subdirectory is used by the GIMP to store \n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tuser created and installed scripts. The default gimprc\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tfile checks this subdirectory in addition to the system\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\t-wide gimp scripts subdirectory when searching for scripts\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font_emphasis, NULL, NULL,
		   _("tmp\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tThis subdirectory is used by the GIMP to temporarily\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tstore undo buffers to reduce memory usage.  If GIMP is\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tunceremoniously killed, files may persist in this directory\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tof the form: gimp<#>.<#>.  These files are useless across\n"), -1);
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		   _("\t\tGIMP sessions and can be destroyed with impunity.\n"), -1);

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
    { N_("Continue"), install_continue_callback, NULL, NULL },
    { N_("Quit"), install_quit_callback, NULL, NULL }
  };
  GtkWidget *text;
  GtkWidget *table;
  GtkWidget *vsb;
  GtkAdjustment *vadj;
  GdkFont   *font_strong;
  GdkFont   *font;
  FILE *pfp;
  char buffer[2048];
  char *gimp_data_dir;
  struct stat stat_buf;
  int err;
  int executable = TRUE;

  install_widget = gtk_dialog_new ();
  gtk_signal_connect (GTK_OBJECT (install_widget), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_true),
		      NULL);
  gtk_window_set_wmclass (GTK_WINDOW (install_widget), "installation_log", "Gimp");
  gtk_window_set_title (GTK_WINDOW (install_widget), _("Installation Log"));
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

  gtk_text_insert (GTK_TEXT (text), font_strong, NULL, NULL, _("User Installation Log\n\n"), -1);

  /*  Generate output  */
  if ((gimp_data_dir = getenv ("GIMP_DATADIR")) != NULL)
    sprintf (buffer, "%s/user_install", gimp_data_dir);
  else
    sprintf (buffer, "%s/user_install", DATADIR);

  if ((err = stat (buffer, &stat_buf)) != 0)
    {
      gtk_text_insert (GTK_TEXT (text), font, NULL, NULL, buffer, -1);
      gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		       _(" does not exist.  Cannot install.\n"), -1);
      executable = FALSE;
    }
  else if (! (S_IXUSR & stat_buf.st_mode) || ! (S_IRUSR & stat_buf.st_mode))
    {
      gtk_text_insert (GTK_TEXT (text), font, NULL, NULL, buffer, -1);
      gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		       _(" has invalid permissions.\nCannot install."), -1);
      executable = FALSE;
    }

  if (executable == TRUE)
    {
      if (gimp_data_dir)
	sprintf (buffer, "%s/user_install %s %s", gimp_data_dir, gimp_data_dir,
		 gimp_directory ());
      else
	sprintf (buffer, "%s/user_install %s %s", DATADIR, DATADIR,
		 gimp_directory ());

      if ((pfp = popen (buffer, "r")) != NULL)
	{
	  while (fgets (buffer, 2048, pfp))
	    gtk_text_insert (GTK_TEXT (text), font, NULL, NULL, buffer, -1);
	  pclose (pfp);

	  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
			   _("\nInstallation successful!\n"), -1);
	}
      else
	executable = FALSE;
    }
  if (executable == FALSE)
    gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		     _("\nInstallation failed.  Contact system administrator.\n"), -1);

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
