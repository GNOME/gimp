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

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafiles.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-render.h"
#include "display/gximage.h"

#include "widgets/gimpdialogfactory.h"

#include "brush-select.h"
#include "color-select.h"
#include "devices.h"
#include "dialogs.h"
#include "file-open-dialog.h"
#include "file-save-dialog.h"
#include "gradient-select.h"
#include "gui.h"
#include "menus.h"
#include "palette-select.h"
#include "palette-import-dialog.h"
#include "pattern-select.h"
#include "session.h"
#include "tool-options-dialog.h"
#include "toolbox.h"

#include "gimprc.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/wilber2.xpm"


/*  local function prototypes  */

static void         gui_set_busy                    (Gimp        *gimp);
static void         gui_unset_busy                  (Gimp        *gimp);
static GimpObject * gui_display_new                 (GimpImage   *gimage,
                                                     guint        scale);

static gint         gui_rotate_the_shield_harmonics (GtkWidget   *widget,
                                                     GdkEvent    *eevent,
                                                     gpointer     data);
static void         gui_really_quit_callback        (GtkWidget   *button,
                                                     gboolean     quit,
                                                     gpointer     data);

static void         gui_display_changed             (GimpContext *context,
                                                     GimpDisplay *display,
                                                     gpointer     data);
static void         gui_image_disconnect            (GimpImage   *gimage,
                                                     gpointer     data);
static void         gui_image_name_changed          (GimpImage   *gimage,
                                                     gpointer     data);


/*  private variables  */

static GQuark image_disconnect_handler_id   = 0;
static GQuark image_name_changed_handler_id = 0;

static GHashTable *themes_hash = NULL;


/*  public functions  */

static void
gui_themes_dir_foreach_func (const gchar *filename,
			     gpointer     loader_data)
{
  Gimp  *gimp;
  gchar *basename;

  gimp = (Gimp *) loader_data;

  basename = g_path_get_basename (filename);

  if (gimp->be_verbose)
    g_print (_("adding theme \"%s\" (%s)\n"), basename, filename);

  g_hash_table_insert (themes_hash,
		       basename,
		       g_strdup (filename));
}

void
gui_libs_init (Gimp    *gimp,
               gint    *argc,
	       gchar ***argv)
{
  gchar *theme_dir;
  gchar *gtkrc;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (argc != NULL);
  g_return_if_fail (argv != NULL);

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
				       gimp);
    }

  if (gimprc.theme)
    theme_dir = g_hash_table_lookup (themes_hash, gimprc.theme);
  else
    theme_dir = g_hash_table_lookup (themes_hash, "Default");

  if (theme_dir)
    {
      gtkrc = g_build_filename (theme_dir, "gtkrc", NULL);
    }
  else
    {
      /*  get the hardcoded default theme gtkrc  */

      gtkrc = g_strdup (gimp_gtkrc ());
    }

  if (gimp->be_verbose)
    g_print (_("parsing \"%s\"\n"), gtkrc);

  gtk_rc_parse (gtkrc);

  g_free (gtkrc);

  /*  parse the user gtkrc  */

  gtkrc = gimp_personal_rc_file ("gtkrc");

  if (gimp->be_verbose)
    g_print (_("parsing \"%s\"\n"), gtkrc);

  gtk_rc_parse (gtkrc);

  g_free (gtkrc);
}

void
gui_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->create_display_func = gui_display_new;
  gimp->gui_set_busy_func   = gui_set_busy;
  gimp->gui_unset_busy_func = gui_unset_busy;

  image_disconnect_handler_id =
    gimp_container_add_handler (gimp->images, "disconnect",
				G_CALLBACK (gui_image_disconnect),
				gimp);

  image_name_changed_handler_id =
    gimp_container_add_handler (gimp->images, "name_changed",
				G_CALLBACK (gui_image_name_changed),
				gimp);

  g_signal_connect (G_OBJECT (gimp_get_user_context (gimp)), "display_changed",
		    G_CALLBACK (gui_display_changed),
		    gimp);

  /* make sure the monitor resolution is valid */
  if (gimprc.monitor_xres < GIMP_MIN_RESOLUTION ||
      gimprc.monitor_yres < GIMP_MIN_RESOLUTION)
    {
      gui_get_screen_resolution (&gimprc.monitor_xres, &gimprc.monitor_yres);
      gimprc.using_xserver_resolution = TRUE;
    }

  /*  tooltips  */
  gimp_help_init ();

  if (! gimprc.show_tool_tips)
    gimp_help_disable_tooltips ();

  menus_init (gimp);

  gximage_init ();
  render_setup (gimprc.transparency_type, gimprc.transparency_size);

  dialogs_init (gimp);

  devices_init ();
  session_init (gimp);
}

void
gui_restore (Gimp     *gimp,
             gboolean  restore_session)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  file_open_dialog_menu_init (gimp);
  file_save_dialog_menu_init (gimp);

  menus_restore (gimp);

  gimp_dialog_factory_dialog_new (global_dialog_factory, "gimp:toolbox");

  color_select_init ();

  devices_restore ();

  if (gimprc.always_restore_session || restore_session)
    session_restore (gimp);
}

void
gui_post_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimprc.show_tips)
    {
      gimp_dialog_factory_dialog_new (global_dialog_factory, "gimp:tips-dialog");
    }
}

void
gui_shutdown (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  session_save (gimp);
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
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  menus_exit (gimp);
  gximage_free ();
  render_free ();

  dialogs_exit (gimp);

  /*  handle this in the dialog factory:  */
  tool_options_dialog_free (gimp);
  toolbox_free (gimp);

  gimp_help_free ();

  gimp_container_remove_handler (gimp->images, image_disconnect_handler_id);
  gimp_container_remove_handler (gimp->images, image_name_changed_handler_id);

  image_disconnect_handler_id   = 0;
  image_name_changed_handler_id = 0;

  if (themes_hash)
    {
      g_hash_table_destroy (themes_hash);
      themes_hash = NULL;
    }
}

void
gui_get_screen_resolution (gdouble   *xres,
                           gdouble   *yres)
{
  gint width, height;
  gint widthMM, heightMM;

  g_return_if_fail (xres != NULL);
  g_return_if_fail (yres != NULL);

  width  = gdk_screen_width ();
  height = gdk_screen_height ();

  widthMM  = gdk_screen_width_mm ();
  heightMM = gdk_screen_height_mm ();

  /*
   * From xdpyinfo.c:
   *
   * there are 2.54 centimeters to an inch; so there are 25.4 millimeters.
   *
   *     dpi = N pixels / (M millimeters / (25.4 millimeters / 1 inch))
   *         = N pixels / (M inch / 25.4)
   *         = N * 25.4 pixels / M inch
   */

  *xres = (width  * 25.4) / ((gdouble) widthMM);
  *yres = (height * 25.4) / ((gdouble) heightMM);
}

void
gui_really_quit_dialog (GCallback quit_func)
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
				   quit_func);

  gtk_widget_show (dialog);
}


/*  private functions  */

gboolean double_speed = FALSE;

static GimpObject *
gui_display_new (GimpImage *gimage,
                 guint      scale)
{
  GimpDisplay *gdisp;

  gdisp = gimp_display_new (gimage, scale);

  gimp_context_set_display (gimp_get_user_context (gimage->gimp), gdisp);

  if (double_speed)
    {
      GimpDisplayShell *shell;

      shell = GIMP_DISPLAY_SHELL (gdisp->shell);

      g_signal_connect_after (G_OBJECT (shell->canvas), "expose_event",
                              G_CALLBACK (gui_rotate_the_shield_harmonics),
                              NULL);
    }

  return GIMP_OBJECT (gdisp);
}

static void
gui_set_busy (Gimp *gimp)
{
  gdisplays_set_busy ();
  gimp_dialog_factories_idle ();
}

static void
gui_unset_busy (Gimp *gimp)
{
  gdisplays_unset_busy ();
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

  gdk_drawable_get_size (pixmap, &width, &height);

  if (widget->allocation.width  >= width &&
      widget->allocation.height >= height)
    {
      gint x, y;

      x = (widget->allocation.width  - width) / 2;
      y = (widget->allocation.height - height) / 2;

      gdk_gc_set_clip_mask (widget->style->black_gc, mask);
      gdk_gc_set_clip_origin (widget->style->black_gc, x, y);

      gdk_draw_drawable (widget->window,
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
  GCallback quit_func;

  quit_func = G_CALLBACK (data);

  if (quit)
    {
      (* quit_func) ();
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
		     GimpDisplay *display,
		     gpointer     data)
{
  Gimp *gimp;

  gimp = (Gimp *) data;

  if (display)
    gimp_display_shell_set_menu_sensitivity (GIMP_DISPLAY_SHELL (display->shell));
  else
    gimp_display_shell_set_menu_sensitivity (NULL);
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
gui_image_name_changed (GimpImage *gimage,
			gpointer   data)
{
  Gimp *gimp;

  gimp = (Gimp *) data;

  palette_import_image_renamed (gimage);
}
