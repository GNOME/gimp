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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools/tools-types.h"
#include "widgets/widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "widgets/gimpdialogfactory.h"

#include "brush-select.h"
#include "color-select.h"
#include "devices.h"
#include "dialogs.h"
#include "docindex.h"
#include "errorconsole.h"
#include "file-open-dialog.h"
#include "file-save-dialog.h"
#include "gradient-select.h"
#include "gui.h"
#include "gximage.h"
#include "image_render.h"
#include "menus.h"
#include "palette-editor.h"
#include "pattern-select.h"
#include "session.h"
#include "tool-options-dialog.h"
#include "toolbox.h"

#include "appenv.h"
#include "app_procs.h"
#include "dialog_handler.h"
#include "gdisplay.h"     /* for gdisplay_*_override_cursor()  */
#include "gdisplay_ops.h" /* for gdisplay_xserver_resolution() */
#include "gimprc.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/wilber2.xpm"


/*  local function prototypes  */

static void   gui_display_new                 (GimpImage *gimage);
static gint   gui_rotate_the_shield_harmonics (GtkWidget *widget,
					       GdkEvent  *eevent,
					       gpointer   data);
static void   gui_really_quit_callback        (GtkWidget *button,
					       gboolean   quit,
					       gpointer   data);


/*  global variables  */

extern GSList *display_list;  /*  from gdisplay.c  */


/*  public functions  */

void
gui_init (Gimp *gimp)
{
  /* make sure the monitor resolution is valid */
  if (gimprc.monitor_xres < GIMP_MIN_RESOLUTION ||
      gimprc.monitor_yres < GIMP_MIN_RESOLUTION)
    {
      gdisplay_xserver_resolution (&gimprc.monitor_xres, &gimprc.monitor_yres);
      gimprc.using_xserver_resolution = TRUE;
    }

  file_open_menu_init ();
  file_save_menu_init ();

  menus_reorder_plugins ();

  gximage_init ();
  render_setup (gimprc.transparency_type, gimprc.transparency_size);

  dialogs_init ();

  devices_init ();
  session_init ();

  /*  tooltips  */
  gimp_help_init ();

  if (! gimprc.show_tool_tips)
    gimp_help_disable_tooltips ();

  gimp_dialog_factory_dialog_new (global_dialog_factory, "gimp:toolbox");

  /*  Fill the "last opened" menu items with the first last_opened_size
   *  elements of the docindex
   */
  {
    FILE   *fp;
    gchar **filenames = g_new0 (gchar *, gimprc.last_opened_size);
    gint    i;

    if ((fp = document_index_parse_init ()))
      {
	/*  read the filenames...  */
	for (i = 0; i < gimprc.last_opened_size; i++)
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

  gimp->create_display_func = gui_display_new;
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
  if (gimprc.show_tips)
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
  document_index_free ();
  error_console_free ();
  tool_options_dialog_free ();
  toolbox_free ();

  gimp_help_free ();
}

void
gui_set_busy (void)
{
  GDisplay *gdisp;
  GSList   *list;

  /* Canvases */
  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GDisplay *) list->data;
      gdisplay_install_override_cursor (gdisp, GDK_WATCH);
    }

  /* Dialogs */
  dialog_idle_all ();

  gdk_flush ();
}

void
gui_unset_busy (void)
{
  GDisplay *gdisp;
  GSList   *list;

  /* Canvases */
  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GDisplay *) list->data;
      gdisplay_remove_override_cursor (gdisp);
    }

  /* Dialogs */
  dialog_unidle_all ();
}

void
gui_really_quit_dialog (void)
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
				   gui_really_quit_callback,
				   NULL);

  gtk_widget_show (dialog);
}


/*  private functions  */

static void
gui_display_new (GimpImage *gimage)
{
  GDisplay *gdisp;

  gdisp = gdisplay_new (gimage, 0x0101);

  gimp_context_set_display (gimp_context_get_user (), gdisp);

  if (double_speed)
    gtk_signal_connect_after (GTK_OBJECT (gdisp->canvas), "expose_event",
			      GTK_SIGNAL_FUNC (gui_rotate_the_shield_harmonics),
			      NULL);
}

static gint
gui_rotate_the_shield_harmonics (GtkWidget *widget,
				 GdkEvent  *eevent,
				 gpointer   data)
{
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask   = NULL;
  gint       width  = 0;
  gint       height = 0;

  gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
				 GTK_SIGNAL_FUNC (gui_rotate_the_shield_harmonics),
				 data);

  pixmap = gdk_pixmap_create_from_xpm_d (widget->window,
					 &mask,
					 NULL,
					 wilber2_xpm);

  gdk_window_get_size (pixmap, &width, &height);

  if (widget->allocation.width  >= width &&
      widget->allocation.height >= height)
    {
      gint x, y;

      x = (widget->allocation.width  - width) / 2;
      y = (widget->allocation.height - height) / 2;

      gdk_gc_set_clip_mask (widget->style->black_gc, mask);
      gdk_gc_set_clip_origin (widget->style->black_gc, x, y);

      gdk_draw_pixmap (widget->window,
		       widget->style->black_gc,
		       pixmap, 0, 0,
		       x, y,
		       width, height);

      gdk_gc_set_clip_mask (widget->style->black_gc, NULL);
      gdk_gc_set_clip_origin (widget->style->black_gc, 0, 0);
    }

  gdk_pixmap_unref (pixmap);
  gdk_bitmap_unref (mask);

  return FALSE;
}

static void
gui_really_quit_callback (GtkWidget *button,
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
