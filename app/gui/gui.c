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

#include <stdio.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"
#include "widgets/widgets-types.h"

#include "tools/tool_options_dialog.h"

#include "widgets/gimpdialogfactory.h"

#include "brush-select.h"
#include "color-select.h"
#include "devices.h"
#include "dialogs.h"
#include "docindex.h"
#include "errorconsole.h"
#include "file-open-dialog.h"
#include "file-save-dialog.h"
#include "gimprc.h"
#include "gradient-select.h"
#include "gui.h"
#include "gximage.h"
#include "image_render.h"
#include "lc_dialog.h"
#include "menus.h"
#include "palette-editor.h"
#include "pattern-select.h"
#include "session.h"
#include "toolbox.h"

#include "app_procs.h"

#include "libgimp/gimpintl.h"


static void   really_quit_callback (GtkWidget *button,
				    gboolean   quit,
				    gpointer   data);


/*  public functions  */

void
gui_init (void)
{
  file_open_menu_init ();
  file_save_menu_init ();

  menus_reorder_plugins ();

  gximage_init ();
  render_setup (transparency_type, transparency_size);

  dialogs_init ();

  devices_init ();
  session_init ();

  /*  tooltips  */
  gimp_help_init ();

  if (! show_tool_tips)
    gimp_help_disable_tooltips ();

  gimp_dialog_factory_dialog_new (global_dialog_factory, "gimp:toolbox");

  /*  Fill the "last opened" menu items with the first last_opened_size
   *  elements of the docindex
   */
  {
    FILE   *fp;
    gchar **filenames = g_new0 (gchar *, last_opened_size);
    gint    i;

    if ((fp = document_index_parse_init ()))
      {
	/*  read the filenames...  */
	for (i = 0; i < last_opened_size; i++)
	  if ((filenames[i] = document_index_parse_line (fp)) == NULL)
	    break;

	/*  ...and add them in reverse order  */
	for (--i; i >= 0; i--)
	  {
	    menus_last_opened_add (filenames[i]);
	    g_free (filenames[i]);
	  }

	fclose (fp);
      }

    g_free (filenames);
  }
}

void
gui_restore (void)
{
  color_select_init ();

  devices_restore ();
  session_restore ();
}

void
gui_post_init (void)
{
  if (show_tips)
    {
      gimp_dialog_factory_dialog_new (global_dialog_factory, "gimp:tips-dialog");
    }
}

void
gui_shutdown (void)
{
  session_save ();
  device_status_free ();

  brush_dialog_free ();
  pattern_dialog_free ();
  palette_dialog_free ();
  gradient_dialog_free ();
}

void
gui_exit (void)
{
  menus_quit ();
  gximage_free ();
  render_free ();

  dialogs_exit ();

  /*  handle this in the dialog factory:  */
  lc_dialog_free ();
  document_index_free ();
  error_console_free ();
  tool_options_dialog_free ();
  toolbox_free ();

  gimp_help_free ();
}

void
really_quit_dialog (void)
{
  GtkWidget *dialog;

  menus_set_sensitive ("<Toolbox>/File/Quit", FALSE);
  menus_set_sensitive ("<Image>/File/Quit", FALSE);

  dialog = gimp_query_boolean_box (_("Really Quit?"),
				   gimp_standard_help_func,
				   "dialogs/really_quit.html",
				   TRUE,
				   _("Some files unsaved.\n\nQuit the GIMP?"),
				   _("Quit"), _("Cancel"),
				   NULL, NULL,
				   really_quit_callback,
				   NULL);

  gtk_widget_show (dialog);
}


/*  private functions  */

static void
really_quit_callback (GtkWidget *button,
		      gboolean   quit,
		      gpointer   data)
{
  if (quit)
    {
      app_exit_finish ();
    }
  else
    {
      menus_set_sensitive ("<Toolbox>/File/Quit", TRUE);
      menus_set_sensitive ("<Image>/File/Quit", TRUE);
    }
}
