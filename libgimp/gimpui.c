/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpmodule/gimpmodule.h"

#include "gimp.h"
#include "gimpui.h"

#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"


/*  local function prototypes  */

static void  gimp_ui_help_func   (const gchar *help_id,
                                  gpointer     help_data);
static void  gimp_ensure_modules (void);


/*  public functions  */

/**
 * gimp_ui_init:
 * @prog_name: The name of the plug-in which will be passed as argv[0] to
 *             gtk_init(). It's a convention to use the name of the
 *             executable and _not_ the PDB procedure name or something.
 * @preview:   This parameter is unused and exists for historical
 *             reasons only.
 *
 * This function initializes GTK+ with gtk_init() and initializes GDK's
 * image rendering subsystem (GdkRGB) to follow the GIMP main program's
 * colormap allocation/installation policy.
 *
 * The GIMP's colormap policy can be determinded by the user with the
 * gimprc variables @min_colors and @install_cmap.
 **/
void
gimp_ui_init (const gchar *prog_name,
	      gboolean     preview)
{
  static gboolean initialized = FALSE;

  const gchar  *display_name;
  gint          argc;
  gchar       **argv;
  gchar        *themerc;
  GdkScreen    *screen;

  g_return_if_fail (prog_name != NULL);

  if (initialized)
    return;

  display_name = gimp_display_name ();

  if (display_name)
    {
      const gchar *var_name = NULL;

#if defined (GDK_WINDOWING_X11)
      var_name = "DISPLAY";
#elif defined (GDK_WINDOWING_DIRECTFB) || defined (GDK_WINDOWING_FB)
      var_name = "GDK_DISPLAY";
#endif

      if (var_name)
        putenv (g_strdup_printf ("%s=%s", var_name, display_name));
    }

  argc    = 2;
  argv    = g_new (gchar *, 2);
  argv[0] = g_strdup (prog_name);
  argv[1] = g_strdup_printf ("--class=%s", gimp_wm_class ());

  gtk_init (&argc, &argv);

  themerc = gimp_personal_rc_file ("themerc");
  gtk_rc_add_default_file (themerc);
  g_free (themerc);

  gdk_rgb_set_min_colors (gimp_min_colors ());
  gdk_rgb_set_install (gimp_install_cmap ());

  screen = gdk_screen_get_default ();
  gtk_widget_set_default_colormap (gdk_screen_get_rgb_colormap (screen));

  gimp_widgets_init (gimp_ui_help_func,
                     gimp_context_get_foreground,
                     gimp_context_get_background,
                     gimp_ensure_modules);

  if (! gimp_show_tool_tips ())
    gimp_help_disable_tooltips ();

  gimp_dialogs_show_help_button (gimp_show_help_button ());

  initialized = TRUE;
}


/*  private functions  */

static void
gimp_ui_help_func (const gchar *help_id,
                   gpointer     help_data)
{
  gimp_help (NULL, help_id);
}

static void
gimp_ensure_modules (void)
{
  static GimpModuleDB *module_db = NULL;

  if (! module_db)
    {
      gchar *load_inhibit;
      gchar *module_path;

      load_inhibit = gimp_get_module_load_inhibit ();
      module_path  = gimp_gimprc_query ("module-path");

      module_db = gimp_module_db_new (FALSE);
      gimp_module_db_set_load_inhibit (module_db, load_inhibit);
      gimp_module_db_load (module_db, module_path);

      g_free (load_inhibit);
      g_free (module_path);
    }
}
