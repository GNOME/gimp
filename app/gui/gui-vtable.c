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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimperrorconsole.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpprogress.h"

#include "brush-select.h"
#include "dialogs.h"
#include "font-select.h"
#include "gradient-select.h"
#include "menus.h"
#include "palette-select.h"
#include "pattern-select.h"
#include "plug-in-menus.h"
#include "themes.h"


/*  local function prototypes  */

static void           gui_threads_enter        (Gimp          *gimp);
static void           gui_threads_leave        (Gimp          *gimp);
static void           gui_set_busy             (Gimp          *gimp);
static void           gui_unset_busy           (Gimp          *gimp);
static void           gui_message              (Gimp          *gimp,
                                                const gchar   *domain,
                                                const gchar   *message);
static GimpObject   * gui_display_new          (GimpImage     *gimage,
                                                gdouble        scale);
static void           gui_menus_init           (Gimp          *gimp,
                                                GSList        *plug_in_defs,
                                                const gchar   *plugins_domain);
static void           gui_menus_create_entry   (Gimp          *gimp,
                                                PlugInProcDef *proc_def,
                                                const gchar   *locale_domain,
                                                const gchar   *help_domain);
static void           gui_menus_delete_entry   (Gimp          *gimp,
                                                const gchar   *menu_path);
static GimpProgress * gui_start_progress       (Gimp          *gimp,
                                                gint           gdisp_ID,
                                                const gchar   *message,
                                                GCallback      cancel_cb,
                                                gpointer       cancel_data);
static GimpProgress * gui_restart_progress     (Gimp          *gimp,
                                                GimpProgress  *progress,
                                                const gchar   *message,
                                                GCallback      cancel_cb,
                                                gpointer       cancel_data);
static void           gui_update_progress      (Gimp          *gimp,
                                                GimpProgress  *progress,
                                                gdouble        percentage);
static void           gui_end_progress         (Gimp          *gimp,
                                                GimpProgress  *progress);
static void           gui_pdb_dialogs_check    (Gimp          *gimp);
static const gchar  * gui_get_program_class    (Gimp          *gimp);
static gchar        * gui_get_display_name     (Gimp          *gimp,
                                                gint           gdisp_ID,
                                                gint          *monitor_number);
static const gchar  * gui_get_theme_dir        (Gimp          *gimp);


/*  public functions  */

void
gui_vtable_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->gui_threads_enter_func     = gui_threads_enter;
  gimp->gui_threads_leave_func     = gui_threads_leave;
  gimp->gui_set_busy_func          = gui_set_busy;
  gimp->gui_unset_busy_func        = gui_unset_busy;
  gimp->gui_message_func           = gui_message;
  gimp->gui_create_display_func    = gui_display_new;
  gimp->gui_menus_init_func        = gui_menus_init;
  gimp->gui_menus_create_func      = gui_menus_create_entry;
  gimp->gui_menus_delete_func      = gui_menus_delete_entry;
  gimp->gui_progress_start_func    = gui_start_progress;
  gimp->gui_progress_restart_func  = gui_restart_progress;
  gimp->gui_progress_update_func   = gui_update_progress;
  gimp->gui_progress_end_func      = gui_end_progress;
  gimp->gui_pdb_dialogs_check_func = gui_pdb_dialogs_check;
  gimp->gui_get_program_class_func = gui_get_program_class;
  gimp->gui_get_display_name_func  = gui_get_display_name;
  gimp->gui_get_theme_dir_func     = gui_get_theme_dir;
}


/*  private functions  */

static void
gui_threads_enter (Gimp *gimp)
{
  GDK_THREADS_ENTER ();
}

static void
gui_threads_leave (Gimp *gimp)
{
  GDK_THREADS_LEAVE ();
}

static void
gui_set_busy (Gimp *gimp)
{
  gimp_displays_set_busy (gimp);
  gimp_dialog_factories_set_busy ();

  gdk_flush ();
}

static void
gui_unset_busy (Gimp *gimp)
{
  gimp_displays_unset_busy (gimp);
  gimp_dialog_factories_unset_busy ();

  gdk_flush ();
}

static void
gui_message (Gimp        *gimp,
             const gchar *domain,
             const gchar *message)
{
  if (gimp->message_handler == GIMP_ERROR_CONSOLE)
    {
      GtkWidget *dockable;

      dockable = gimp_dialog_factory_dialog_raise (global_dock_factory,
                                                   gdk_screen_get_default (),
                                                   "gimp-error-console", -1);

      if (dockable)
        {
          GimpErrorConsole *console;

          console = GIMP_ERROR_CONSOLE (GTK_BIN (dockable)->child);

          gimp_error_console_add (console, GIMP_STOCK_WARNING, domain, message);

          return;
        }

      gimp->message_handler = GIMP_MESSAGE_BOX;
    }

  gimp_message_box (GIMP_STOCK_WARNING, domain, message, NULL, NULL);
}

static GimpObject *
gui_display_new (GimpImage *gimage,
                 gdouble    scale)
{
  GimpDisplayShell *shell;
  GimpDisplay      *gdisp;

  gdisp = gimp_display_new (gimage, scale,
                            global_menu_factory,
                            gimp_item_factory_from_path ("<Image>"));

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_context_set_display (gimp_get_user_context (gimage->gimp), gdisp);

  gimp_item_factory_update (shell->menubar_factory, shell);

  return GIMP_OBJECT (gdisp);
}

static void
gui_menus_init (Gimp        *gimp,
                GSList      *plug_in_defs,
                const gchar *std_plugins_domain)
{
  plug_in_menus_init (gimp, plug_in_defs, std_plugins_domain);
}

static void
gui_menus_create_entry (Gimp          *gimp,
                        PlugInProcDef *proc_def,
                        const gchar   *locale_domain,
                        const gchar   *help_domain)
{
  plug_in_menus_create_entry (NULL, proc_def, locale_domain, help_domain);
}

static void
gui_menus_delete_entry (Gimp        *gimp,
                        const gchar *menu_path)
{
  plug_in_menus_delete_entry (menu_path);
}

static GimpProgress *
gui_start_progress (Gimp        *gimp,
                    gint         gdisp_ID,
                    const gchar *message,
                    GCallback    cancel_cb,
                    gpointer     cancel_data)
{
  GimpDisplay *gdisp = NULL;

  if (gdisp_ID > 0)
    gdisp = gimp_display_get_by_ID (gimp, gdisp_ID);

  return gimp_progress_start (gdisp, message, TRUE, cancel_cb, cancel_data);
}

static GimpProgress *
gui_restart_progress (Gimp         *gimp,
                      GimpProgress *progress,
                      const gchar  *message,
                      GCallback     cancel_cb,
                      gpointer      cancel_data)
{
  return gimp_progress_restart (progress, message, cancel_cb, cancel_data);
}

static void
gui_update_progress (Gimp         *gimp,
                     GimpProgress *progress,
                     gdouble       percentage)
{
  gimp_progress_update (progress, percentage);
}

static void
gui_end_progress (Gimp         *gimp,
                  GimpProgress *progress)
{
  gimp_progress_end (progress);
}

static void
gui_pdb_dialogs_check (Gimp *gimp)
{
  brush_select_dialogs_check ();
  font_select_dialogs_check ();
  gradient_select_dialogs_check ();
  palette_select_dialogs_check ();
  pattern_select_dialogs_check ();
}

static const gchar *
gui_get_program_class (Gimp *gimp)
{
  return gdk_get_program_class ();
}

static gchar *
gui_get_display_name (Gimp *gimp,
                      gint  gdisp_ID,
                      gint *monitor_number)
{
  GimpDisplay *gdisp = NULL;
  GdkScreen   *screen;
  gint         monitor;

  if (gdisp_ID > 0)
    gdisp = gimp_display_get_by_ID (gimp, gdisp_ID);

  if (gdisp)
    {
      screen  = gtk_widget_get_screen (gdisp->shell);
      monitor = gdk_screen_get_monitor_at_window (screen,
                                                  gdisp->shell->window);
    }
  else
    {
      gint x, y;

      gdk_display_get_pointer (gdk_display_get_default (),
                               &screen, &x, &y, NULL);
      monitor = gdk_screen_get_monitor_at_point (screen, x, y);
    }

  *monitor_number = monitor;

  if (screen)
    return gdk_screen_make_display_name (screen);

  return NULL;
}

static const gchar *
gui_get_theme_dir (Gimp *gimp)
{
  return themes_get_theme_dir (gimp, GIMP_GUI_CONFIG (gimp->config)->theme);
}
