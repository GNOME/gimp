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
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafiles.h"
#include "core/gimpimage.h"

#include "widgets/gimpdialogfactory.h"

#include "brush-select.h"
#include "color-select.h"
#include "devices.h"
#include "dialogs.h"
#include "docindex.h"
#include "file-open-dialog.h"
#include "file-save-dialog.h"
#include "gradient-select.h"
#include "gui.h"
#include "gximage.h"
#include "image_render.h"
#include "menus.h"
#include "palette-select.h"
#include "palette-import-dialog.h"
#include "pattern-select.h"
#include "session.h"
#include "tool-options-dialog.h"
#include "toolbox.h"

#include "appenv.h"
#include "app_procs.h"
#include "gdisplay.h"     /* for gdisplay_*_override_cursor()  */
#include "gdisplay_ops.h" /* for gdisplay_xserver_resolution() */
#include "gimprc.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/wilber2.xpm"


/*  local function prototypes  */

static void   gui_set_busy                        (Gimp        *gimp);
static void   gui_unset_busy                      (Gimp        *gimp);
static void   gui_display_new                     (GimpImage   *gimage);

static gint   gui_rotate_the_shield_harmonics     (GtkWidget   *widget,
						   GdkEvent    *eevent,
						   gpointer     data);
static void   gui_really_quit_callback            (GtkWidget   *button,
						   gboolean     quit,
						   gpointer     data);

static void   gui_display_changed                 (GimpContext *context,
						   GDisplay    *display,
						   gpointer     data);

static void   gui_image_disconnect                (GimpImage   *gimage,
						   gpointer     data);
static void   gui_image_mode_changed              (GimpImage   *gimage,
						   gpointer     data);
static void   gui_image_colormap_changed          (GimpImage   *gimage,
						   gint         ncol,
						   gpointer     data);
static void   gui_image_name_changed              (GimpImage   *gimage,
						   gpointer     data);
static void   gui_image_size_changed              (GimpImage   *gimage,
						   gpointer     data);
static void   gui_image_alpha_changed             (GimpImage   *gimage,
						   gpointer     data);
static void   gui_image_update                    (GimpImage   *gimage,
						   gint         x,
						   gint         y,
						   gint         w,
						   gint         h,
						   gpointer     data);


/*  global variables  */

extern GSList *display_list;  /*  from gdisplay.c  */


/*  private variables  */

static GQuark image_disconnect_handler_id       = 0;
static GQuark image_mode_changed_handler_id     = 0;
static GQuark image_colormap_changed_handler_id = 0;
static GQuark image_name_changed_handler_id     = 0;
static GQuark image_size_changed_handler_id     = 0;
static GQuark image_alpha_changed_handler_id    = 0;
static GQuark image_update_handler_id           = 0;

static GHashTable *themes_hash = NULL;


/*  public functions  */

static void
gui_themes_dir_foreach_func (const gchar *filename,
			     gpointer     loader_data)
{
  GHashTable *hash;
  gchar      *basename;

  hash = (GHashTable *) loader_data;

  basename = g_path_get_basename (filename);

  if (be_verbose)
    g_print (_("adding theme \"%s\" (%s)\n"), basename, filename);

  g_hash_table_insert (hash,
		       basename,
		       g_strdup (filename));
}

void
gui_libs_init (gint    *argc,
	       gchar ***argv)
{
  gchar *theme_dir;
  gchar *gtkrc;

  gimp_stock_init ();

  themes_hash = g_hash_table_new_full (g_str_hash,
				       g_str_equal,
				       g_free,
				       g_free);

  if (gimprc.theme_path)
    {
      gimp_datafiles_read_directories (gimprc.theme_path,
				       TYPE_DIRECTORY,
				       gui_themes_dir_foreach_func,
				       themes_hash);
    }

  if (gimprc.theme)
    theme_dir = g_hash_table_lookup (themes_hash, gimprc.theme);
  else
    theme_dir = g_hash_table_lookup (themes_hash, "Default");

  if (theme_dir)
    {
      gtkrc = g_strconcat (theme_dir,
			   G_DIR_SEPARATOR_S,
			   "gtkrc",
			   NULL);
    }
  else
    {
      /*  get the hardcoded default theme gtkrc  */

      gtkrc = g_strdup (gimp_gtkrc ());
    }

  if (be_verbose)
    g_print (_("parsing \"%s\"\n"), gtkrc);

  gtk_rc_parse (gtkrc);

  g_free (gtkrc);

  /*  parse the user gtkrc  */

  gtkrc = gimp_personal_rc_file ("gtkrc");

  if (be_verbose)
    g_print (_("parsing \"%s\"\n"), gtkrc);

  gtk_rc_parse (gtkrc);

  g_free (gtkrc);
}

void
gui_init (Gimp *gimp)
{
  gimp->create_display_func = gui_display_new;
  gimp->gui_set_busy_func   = gui_set_busy;
  gimp->gui_unset_busy_func = gui_unset_busy;

  if (gimprc.always_restore_session)
    restore_session = TRUE;

  image_disconnect_handler_id =
    gimp_container_add_handler (gimp->images, "disconnect",
				G_CALLBACK (gui_image_disconnect),
				gimp);

  image_mode_changed_handler_id =
    gimp_container_add_handler (gimp->images, "mode_changed",
				G_CALLBACK (gui_image_mode_changed),
				gimp);

  image_colormap_changed_handler_id =
    gimp_container_add_handler (gimp->images, "colormap_changed",
				G_CALLBACK (gui_image_colormap_changed),
				gimp);

  image_name_changed_handler_id =
    gimp_container_add_handler (gimp->images, "name_changed",
				G_CALLBACK (gui_image_name_changed),
				gimp);

  image_size_changed_handler_id =
    gimp_container_add_handler (gimp->images, "size_changed",
				G_CALLBACK (gui_image_size_changed),
				gimp);

  image_alpha_changed_handler_id =
    gimp_container_add_handler (gimp->images, "alpha_changed",
				G_CALLBACK (gui_image_alpha_changed),
				gimp);

  image_update_handler_id =
    gimp_container_add_handler (gimp->images, "update",
				G_CALLBACK (gui_image_update),
				gimp);

  g_signal_connect (G_OBJECT (gimp_get_user_context (gimp)), "display_changed",
		    G_CALLBACK (gui_display_changed),
		    gimp);

  /* make sure the monitor resolution is valid */
  if (gimprc.monitor_xres < GIMP_MIN_RESOLUTION ||
      gimprc.monitor_yres < GIMP_MIN_RESOLUTION)
    {
      gdisplay_xserver_resolution (&gimprc.monitor_xres, &gimprc.monitor_yres);
      gimprc.using_xserver_resolution = TRUE;
    }

  file_open_dialog_menu_init ();
  file_save_dialog_menu_init ();

  menus_reorder_plugins ();

  gximage_init ();
  render_setup (gimprc.transparency_type, gimprc.transparency_size);

  dialogs_init (gimp);

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
}

void
gui_restore (Gimp *gimp)
{
  color_select_init ();

  devices_restore ();
  session_restore ();
}

void
gui_post_init (Gimp *gimp)
{
  if (gimprc.show_tips)
    {
      gimp_dialog_factory_dialog_new (global_dialog_factory, "gimp:tips-dialog");
    }
}

void
gui_shutdown (Gimp *gimp)
{
  session_save ();
  device_status_free ();

  brush_dialog_free ();
  pattern_dialog_free ();
  palette_dialog_free ();
  gradient_dialog_free ();

  gdisplays_delete ();
}

void
gui_exit (Gimp *gimp)
{
  menus_quit ();
  gximage_free ();
  render_free ();

  dialogs_exit (gimp);

  /*  handle this in the dialog factory:  */
  document_index_free ();
  tool_options_dialog_free ();
  toolbox_free ();

  gimp_help_free ();

  gimp_container_remove_handler (gimp->images, image_disconnect_handler_id);
  gimp_container_remove_handler (gimp->images, image_mode_changed_handler_id);
  gimp_container_remove_handler (gimp->images, image_colormap_changed_handler_id);
  gimp_container_remove_handler (gimp->images, image_name_changed_handler_id);
  gimp_container_remove_handler (gimp->images, image_size_changed_handler_id);
  gimp_container_remove_handler (gimp->images, image_alpha_changed_handler_id);
  gimp_container_remove_handler (gimp->images, image_update_handler_id);

  image_disconnect_handler_id       = 0;
  image_mode_changed_handler_id     = 0;
  image_colormap_changed_handler_id = 0;
  image_name_changed_handler_id     = 0;
  image_size_changed_handler_id     = 0;
  image_alpha_changed_handler_id    = 0;
  image_update_handler_id           = 0;

  if (themes_hash)
    {
      g_hash_table_destroy (themes_hash);
      themes_hash = NULL;
    }
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
				   GTK_STOCK_QUIT, GTK_STOCK_CANCEL,
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

  gimp_context_set_display (gimp_get_user_context (gimage->gimp), gdisp);

  if (double_speed)
    gtk_signal_connect_after (GTK_OBJECT (gdisp->canvas), "expose_event",
			      G_CALLBACK (gui_rotate_the_shield_harmonics),
			      NULL);
}

static void
gui_set_busy (Gimp *gimp)
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
  gimp_dialog_factories_idle ();

  gdk_flush ();
}

static void
gui_unset_busy (Gimp *gimp)
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
  gimp_dialog_factories_unidle ();
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

  g_signal_handlers_disconnect_by_func (G_OBJECT (widget),
					gui_rotate_the_shield_harmonics,
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

  gdk_drawable_unref (pixmap);
  gdk_drawable_unref (mask);

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


/*  FIXME: this junk should mostly go to the display subsystem  */

static void
gui_display_changed (GimpContext *context,
		     GDisplay    *display,
		     gpointer     data)
{
  Gimp *gimp;

  gimp = (Gimp *) data;

  gdisplay_set_menu_sensitivity (display);
}

static void
gui_image_disconnect (GimpImage *gimage,
		      gpointer   data)
{
  Gimp *gimp;

  gimp = (Gimp *) data;

  /*  check if this is the last image  */
  if (gimp_container_num_children (gimp->images) == 1)
    {
      gimp_dialog_factory_dialog_raise (global_dialog_factory, "gimp:toolbox");
    }
}

static void
gui_image_mode_changed (GimpImage *gimage,
			gpointer   data)
{
  Gimp *gimp;

  gimp = (Gimp *) data;

  gimp_image_invalidate_layer_previews (gimage);
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));
  gdisplays_update_title (gimage);
  gdisplays_update_full (gimage);
}

static void
gui_image_colormap_changed (GimpImage *gimage,
			    gint       ncol,
			    gpointer   data)
{
  Gimp *gimp;

  gimp = (Gimp *) data;

  if (gimp_image_base_type (gimage) == INDEXED)
    gdisplays_update_full (gimage);
}

static void
gui_image_name_changed (GimpImage *gimage,
			gpointer   data)
{
  Gimp *gimp;

  gimp = (Gimp *) data;

  gdisplays_update_title (gimage);

  palette_import_image_renamed (gimage);
}

static void
gui_image_size_changed (GimpImage *gimage,
			gpointer   data)
{
  Gimp *gimp;

  gimp = (Gimp *) data;

  /*  shrink wrap and update all views  */
  gdisplays_resize_cursor_label (gimage);
  gdisplays_update_full (gimage);
  gdisplays_shrink_wrap (gimage);
}

static void
gui_image_alpha_changed (GimpImage *gimage,
			 gpointer   data)
{
  Gimp *gimp;

  gimp = (Gimp *) data;

  gdisplays_update_title (gimage);
}

static void
gui_image_update (GimpImage *gimage,
		  gint       x,
		  gint       y,
		  gint       w,
		  gint       h,
		  gpointer   data)
{
  Gimp *gimp;

  gimp = (Gimp *) data;

  gdisplays_update_area (gimage, x, y, w, h);
}
