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
#include "config.h"

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "appenv.h"
#include "install.h"
#include "gimprc.h"
#include "gimpui.h"

#include "libgimp/gimpintl.h"
#include "libgimp/gimpenv.h"

#ifndef G_OS_WIN32
#  ifndef __EMX__
#  define USER_INSTALL "user_install"
#  else
#  include <process.h>
#  define USER_INSTALL "user_install.cmd"
#  endif
#else
#  define STRICT
#  include <windows.h>
#  define USER_INSTALL "user_install.bat"
#endif

static void install_run               (InstallCallback);
static void install_help              (InstallCallback);
static void help_install_callback     (GtkWidget *, gpointer);
static void help_ignore_callback      (GtkWidget *, gpointer);
static void help_quit_callback        (GtkWidget *, gpointer);
static void install_continue_callback (GtkWidget *, gpointer);
static void install_quit_callback     (GtkWidget *, gpointer);

static GtkWidget *help_widget;
static GtkWidget *install_widget;

void
install_verify (InstallCallback install_callback)
{
  int properly_installed = TRUE;
  char *filename;
  struct stat stat_buf;

  filename = gimp_directory ();
  /* gimp_directory now always returns something */

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
  GtkWidget *text;
  GtkWidget *table;
  GtkWidget *vsb;
  GtkAdjustment *vadj;
  GdkFont   *font_strong;
  GdkFont   *font_emphasis;
  GdkFont   *font;
  static const struct {
    gint    font;
    char   *text;
  } help_lines[] = {
    { 2, N_("The GIMP - GNU Image Manipulation Program\n\n") },
    { 1, "Copyright (C) 1995 Spencer Kimball and Peter Mattis\n" },
    { 0, "\n" },

    { 0,
      N_("This program is free software; you can redistribute it and/or modify\n"
	 "it under the terms of the GNU General Public License as published by\n"
	 "the Free Software Foundation; either version 2 of the License, or\n"
	 "(at your option) any later version.\n") },
    { 0, "\n" },
    { 0, N_("This program is distributed in the hope that it will be useful,\n"
	    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
	    "See the GNU General Public License for more details.\n") },
    { 0, "\n" },
    { 0, N_("You should have received a copy of the GNU General Public License\n"
	    "along with this program; if not, write to the Free Software\n"
	    "Foundation, Inc., 59 Temple Place - Suite 330, Boston,\n"
	    "MA 02111-1307, USA.\n") },
    { 0, "\n\n" },

    { 2, N_("Personal GIMP Installation\n\n") },
    { 0, N_("For a proper GIMP installation, a subdirectory called\n") },
    { 1, NULL }, /* will be replaced with gimp_directory() */
    { 0, N_(" needs to be created.  This\n"
	    "subdirectory will contain a number of important files:\n\n") },

    { 1, "gimprc\n" },
    { 0, N_("\t\tThe gimprc is used to store personal preferences\n"
	    "\t\tthat affect GIMP's default behavior.\n"
	    "\t\tPaths to search for brushes, palettes, gradients,\n"
	    "\t\tpatterns, plug-ins and modules can also configured here.\n") },

    { 1, "pluginrc\n" },
    { 0, N_("\t\tPlug-ins and extensions are external programs run by\n"
	    "\t\tthe GIMP which provide additional functionality.\n"
	    "\t\tThese programs are searched for at run-time and\n"
	    "\t\tinformation about their functionality and mod-times\n"
	    "\t\tis cached in this file.  This file is intended to\n"
	    "\t\tbe GIMP-readable only, and should not be edited.\n") },

    { 1, "menurc\n" },
    { 0, N_("\t\tKey shortcuts can be dynamically redefined in The GIMP.\n"
	    "\t\tThe menurc is a dump of your configuration so it can.\n"
	    "\t\tbe remembered for the next session. You may edit this\n"
	    "\t\tfile if you wish, but it is much easier to define the\n"
	    "\t\tkeys from within The GIMP. Deleting this file will\n"
	    "\t\trestore the default shortcuts.\n") },

    { 1, "sessionrc\n" },
    { 0, N_("\t\tThe sessionrc is used to store what dialog windows were\n"
            "\t\topen the last time you quit The GIMP. You can configure\n"
            "\t\tThe GIMP to reopen these dialogs at the saved position.\n") },

    { 1, "unitrc\n" },
    { 0, N_("\t\tThe unitrc is used to store your user units database.\n"
	    "\t\tYou can define additional units and use them just\n"
	    "\t\tlike you use the built-in units inches, millimeters,\n"
	    "\t\tpoints and picas. This file is overwritten each time\n"
	    "\t\tyou quit the GIMP.\n") },

    { 1, "brushes\n" },
    { 0, N_("\t\tThis is a subdirectory which can be used to store\n"
	    "\t\tuser defined brushes.  The default gimprc file\n"
	    "\t\tchecks this subdirectory in addition to the system-\n"
	    "\t\twide gimp brushes installation when searching for\n"
	    "\t\tbrushes.\n") },

    { 1, "generated_brushes\n" },
    { 0, N_("\t\tThis is a subdirectory which is used to store brushes\n"
	    "\t\tthat are created with the brush editor.  The default\n"
	    "\t\tgimprc file checks this subdirectory when searching for\n"
	    "\t\tgenerated brushes.\n") },

    { 1, "gradients\n" },
    { 0, N_("\t\tThis is a subdirectory which can be used to store\n"
	    "\t\tuser defined gradients.  The default gimprc file\n"
	    "\t\tchecks this subdirectory in addition to the system-\n"
	    "\t\twide gimp gradients installation when searching for\n"
	    "\t\tgradients.\n") },

    { 1, "gfig\n" },
    { 0, N_("\t\tThis is a subdirectory which can be used to store\n"
	    "\t\tuser defined figures to be used by the gfig plug-in.\n"
	    "\t\tThe default gimprc file checks this subdirectory in\n"
	    "\t\taddition to the systemwide gimp gfig installation\n"
	    "\t\twhen searching for gfig figures.\n") },

    { 1, "gflares\n" },
    { 0, N_("\t\tThis is a subdirectory which can be used to store\n"
	    "\t\tuser defined gflares to be used by the gflare plug-in.\n"
	    "\t\tThe default gimprc file checks this subdirectory in\n"
	    "\t\taddition to the systemwide gimp gflares installation\n"
	    "\t\twhen searching for gflares.\n") },

    { 1, "fractalexplorer\n" },
    { 0, N_("\t\tThis is a subdirectory which can be used to store\n"
	    "\t\tuser defined fractals to be used by the FractalExplorer\n"
	    "\t\tplug-in. The default gimprc file checks this subdirectory in\n"
	    "\t\taddition to the systemwide gimp FractalExplorer installation\n"
	    "\t\twhen searching for fractals.\n") },

    { 1, "palettes\n" },
    { 0, N_("\t\tThis is a subdirectory which can be used to store\n"
	    "\t\tuser defined palettes.  The default gimprc file\n"
	    "\t\tchecks only this subdirectory (not the system-wide\n"
	    "\t\tinstallation) when searching for palettes.  During\n"
	    "\t\tinstallation, the system palettes will be copied\n"
	    "\t\there.  This is done to allow modifications made to\n"
	    "\t\tpalettes during GIMP execution to persist across\n"
	    "\t\tsessions.\n") },

    { 1, "patterns\n" },
    { 0, N_("\t\tThis is a subdirectory which can be used to store\n"
	    "\t\tuser defined patterns.  The default gimprc file\n"
	    "\t\tchecks this subdirectory in addition to the system-\n"
	    "\t\twide gimp patterns installation when searching for\n"
	    "\t\tpatterns.\n") },

    { 1, "plug-ins\n" },
    { 0, N_("\t\tThis is a subdirectory which can be used to store\n"
	    "\t\tuser created, temporary, or otherwise non-system-\n"
	    "\t\tsupported plug-ins.  The default gimprc file\n"
	    "\t\tchecks this subdirectory in addition to the system-\n"
	    "\t\twide GIMP plug-in directories when searching for\n"
	    "\t\tplug-ins.\n") },

    { 1, "modules\n" },
    { 0, N_("\t\tThis subdirectory can be used to store user created,\n"
	    "\t\ttemporary, or otherwise non-system-supported DLL modules.\n"
	    "\t\tThe default gimprc file checks this subdirectory\n"
	    "\t\tin addition to the system-wide GIMP module directory\n"
	    "\t\twhen searching for modules to load when initializing.\n") },

    { 1, "scripts\n" },
    { 0, N_("\t\tThis subdirectory is used by the GIMP to store \n"
	    "\t\tuser created and installed scripts. The default gimprc\n"
	    "\t\tfile checks this subdirectory in addition to the system\n"
	    "\t\t-wide gimp scripts subdirectory when searching for scripts\n") },

    { 1, "tmp\n" },
    { 0, N_("\t\tThis subdirectory is used by the GIMP to temporarily\n"
	    "\t\tstore undo buffers to reduce memory usage.  If GIMP is\n"
	    "\t\tunceremoniously killed, files may persist in this directory\n"
	    "\t\tof the form: gimp<#>.<#>.  These files are useless across\n"
	    "\t\tGIMP sessions and can be destroyed with impunity.\n") }
  };
  gint nhelp_lines = sizeof (help_lines) / sizeof (help_lines[0]);
  gint i;

  help_widget = gimp_dialog_new (_("GIMP Installation"), "gimp_installation",
				 NULL, NULL,
				 GTK_WIN_POS_CENTER,
				 FALSE, TRUE, FALSE,

				 _("Install"), help_install_callback,
				 callback, NULL, NULL, TRUE, FALSE,
				 _("Ignore"), help_ignore_callback,
				 callback, NULL, NULL, FALSE, FALSE,
				 _("Quit"), help_quit_callback,
				 callback, NULL, NULL, FALSE, TRUE,

				 NULL);

  table  = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (help_widget)->vbox), table);

  vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  vsb = gtk_vscrollbar_new (vadj);
  text = gtk_text_new (NULL, vadj);
  gtk_text_set_editable (GTK_TEXT (text), FALSE);
  gtk_widget_set_usize (text, 450, 475);

  gtk_table_attach (GTK_TABLE (table), vsb, 1, 2, 0, 1,
		    0, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  font_strong = gdk_fontset_load ( _("-*-helvetica-bold-r-normal-*-*-120-*-*-*-*-*-*,*"));
  font_emphasis = gdk_font_load ("-*-helvetica-medium-o-normal-*-*-100-*-*-*-*-*-*");
  font = gdk_fontset_load ( _("-*-helvetica-medium-r-normal-*-*-100-*-*-*-*-*-*,*"));

  /*  Realize the widget before allowing new text to be inserted  */
  gtk_widget_realize (text);

  for (i = 0; i < nhelp_lines; i++)
    if (help_lines[i].text == NULL)
      /*  inserting gimp_directory () this way is a little ugly  */
      gtk_text_insert (GTK_TEXT (text),
		       (help_lines[i].font == 2) ? font_strong :
		       (help_lines[i].font == 1) ? font_emphasis : font,
		       NULL, NULL,
		       gimp_directory (), -1);
    else
      gtk_text_insert (GTK_TEXT (text),
		       (help_lines[i].font == 2) ? font_strong :
		       (help_lines[i].font == 1) ? font_emphasis : font,
		       NULL, NULL,
		       gettext (help_lines[i].text), -1);

  /* scroll back to the top */
  gtk_adjustment_set_value (GTK_ADJUSTMENT (vadj), 0.0);

  gtk_widget_show (vsb);
  gtk_widget_show (text);
  gtk_widget_show (table);
  gtk_widget_show (help_widget);
}

static void
help_install_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  InstallCallback callback;

  callback = (InstallCallback) client_data;
  gtk_widget_destroy (help_widget);
  install_run (callback);
}

static void
help_ignore_callback (GtkWidget *widget,
		      gpointer   client_data)
{
  InstallCallback callback;

  callback = (InstallCallback) client_data;
  gtk_widget_destroy (help_widget);
  (* callback) ();
}

static void
help_quit_callback (GtkWidget *widget,
		    gpointer   client_data)
{
  gtk_widget_destroy (help_widget);
  gtk_exit (0);
}

#ifdef G_OS_WIN32

char *
quote_spaces (char *string)
{
  int nspaces = 0;
  char *p = string, *q, *new;

  while (*p)
    {
      if (*p == ' ')
	nspaces++;
      p++;
    }

  if (nspaces == 0)
    return g_strdup (string);

  new = g_malloc (strlen (string) + nspaces*2 + 1);

  p = string;
  q = new;
  while (*p)
    {
      if (*p == ' ')
	{
	  *q++ = '"';
	  *q++ = ' ';
	  *q++ = '"';
	}
      else
	*q++ = *p;
      p++;
    }
  *q = '\0';

  return new;
}

#endif

static void
install_run (InstallCallback callback)
{
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

  install_widget = gimp_dialog_new (_("Installation Log"), "installation_log",
				    NULL, NULL,
				    GTK_WIN_POS_CENTER,
				    FALSE, TRUE, FALSE,

				    _("Continue"), install_continue_callback,
				    callback, NULL, NULL, TRUE, FALSE,
				    _("Quit"), install_quit_callback,
				    callback, NULL, NULL, FALSE, TRUE,

				    NULL);

  table  = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (install_widget)->vbox), table);

  vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  vsb  = gtk_vscrollbar_new (vadj);
  text = gtk_text_new (NULL, vadj);
  gtk_widget_set_usize (text, 384, 356);

  gtk_table_attach (GTK_TABLE (table), vsb, 1, 2, 0, 1,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    0, 0);

  font_strong = gdk_fontset_load ( _("-*-helvetica-bold-r-normal-*-*-120-*-*-*-*-*-*,*"));
  font = gdk_fontset_load ( _("-*-helvetica-medium-r-normal-*-*-120-*-*-*-*-*-*,*"));

  /*  Realize the text widget before inserting text strings  */
  gtk_widget_realize (text);

#ifndef G_OS_WIN32
  gtk_text_insert (GTK_TEXT (text), font_strong, NULL, NULL, _("User Installation Log\n\n"), -1);
#endif

  /*  Generate output  */
  g_snprintf (buffer, sizeof(buffer), "%s" G_DIR_SEPARATOR_S USER_INSTALL,
	      gimp_data_directory ());
  if ((err = stat (buffer, &stat_buf)) != 0)
    {
      gtk_text_insert (GTK_TEXT (text), font, NULL, NULL, buffer, -1);
      gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		       _(" does not exist.  Cannot install.\n"), -1);
      executable = FALSE;
    }
#ifdef S_IXUSR
  else if (! (S_IXUSR & stat_buf.st_mode) || ! (S_IRUSR & stat_buf.st_mode))
    {
      gtk_text_insert (GTK_TEXT (text), font, NULL, NULL, buffer, -1);
      gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
		       _(" has invalid permissions.\nCannot install."), -1);
      executable = FALSE;
    }
#endif

  if (executable == TRUE)
    {
#ifdef G_OS_WIN32
      char *quoted_data_dir, *quoted_user_dir;

      /* On Windows, it is common for the GIMP data directory
       * to have spaces in it ("c:\Program Files\GIMP"). Put spaces in quotes.
       */
      quoted_data_dir = quote_spaces (gimp_data_directory ());
      quoted_user_dir = quote_spaces (gimp_directory ());

      /* The Microsoft _popen doesn't work in Windows applications, sigh.
       * Do the installation by calling system(). The user_install.bat
       * ends with a pause command, so the user has to press enter in
       * the console window to continue, and thus has a chance to read
       * at the window contents.
       */

      AllocConsole ();
      g_snprintf (buffer, sizeof(buffer), "%s" G_DIR_SEPARATOR_S USER_INSTALL " %s %s",
		  quoted_data_dir, quoted_data_dir,
		  quoted_user_dir);

      if (system (buffer) == -1)
	executable = FALSE;
      g_free (quoted_data_dir);
      g_free (quoted_user_dir);

      gtk_text_insert (GTK_TEXT (text), font_strong, NULL, NULL,
		       _("Did you notice any error messages\n"
		         "in the console window? If not, installation\n"
		         "was successful! Otherwise, quit and investigate\n"
		         "the possible reason...\n"), -1);
#else
#ifndef __EMX__
      g_snprintf (buffer, sizeof(buffer), "%s" G_DIR_SEPARATOR_S USER_INSTALL " %s %s",
		  gimp_data_directory (), gimp_data_directory(),
		  gimp_directory ());
#else
      g_snprintf (buffer, sizeof(buffer), "cmd.exe /c %s" G_DIR_SEPARATOR_S USER_INSTALL " %s %s",
		  gimp_data_directory (), gimp_data_directory(),
		  gimp_directory ());
      {
	char *s = buffer + 10;
	while (*s)
	  {
	    if (*s == '/') *s = '\\';
	    s++;
	  }
      }
#endif

      /* urk - should really use something better than popen(), since
       * we can't tell if the installation script failed --austin */
      if ((pfp = popen (buffer, "r")) != NULL)
	{
	  while (fgets (buffer, sizeof (buffer), pfp))
	    gtk_text_insert (GTK_TEXT (text), font, NULL, NULL, buffer, -1);
	  pclose (pfp);

	  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL,
			   _("\nInstallation successful!\n"), -1);
	}
      else
	executable = FALSE;
#endif /* !G_OS_WIN32 */
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
install_continue_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  InstallCallback callback;

#ifdef G_OS_WIN32
  FreeConsole ();
#endif

  callback = (InstallCallback) client_data;
  gtk_widget_destroy (install_widget);
  (* callback) ();
}

static void
install_quit_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  gtk_widget_destroy (install_widget);
  gtk_exit (0);
}
