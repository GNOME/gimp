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

#include "gimp.h"
#include "gimpui.h"

#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"


/*  local function prototypes  */

static void  gimp_ui_help_func (const gchar *help_id,
                                gpointer     help_data);


/*  public functions  */

/**
 * gimp_ui_init:
 * @prog_name: The name of the plug-in which will be passed as argv[0] to
 *             gtk_init(). It's a convention to use the name of the
 *             executable and _not_ the PDB procedure name or something.
 * @preview: %TRUE if the plug-in has some kind of preview in it's UI.
 *           Note that passing %TRUE is recommended also if one of the
 *           used GIMP Library widgets contains a preview (like the image
 *           menu returned by gimp_image_menu_new()).
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

  GimpWidgetsVTable vtable;

  const gchar  *display_name;
  gint          argc;
  gchar       **argv;
  gchar        *user_gtkrc;
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

  argc    = 3;
  argv    = g_new (gchar *, 3);
  argv[0] = g_strdup (prog_name);
  argv[1] = g_strdup_printf ("--name=%s",  gimp_wm_name ());
  argv[2] = g_strdup_printf ("--class=%s", gimp_wm_class ());

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  user_gtkrc = gimp_personal_rc_file ("gtkrc");
  gtk_rc_parse (user_gtkrc);
  g_free (user_gtkrc);

  gdk_rgb_set_min_colors (gimp_min_colors ());
  gdk_rgb_set_install (gimp_install_cmap ());

  screen = gdk_screen_get_default ();
  gtk_widget_set_default_colormap (gdk_screen_get_rgb_colormap (screen));

  /*  Set the gamma after installing the colormap because
   *  gtk_preview_set_gamma() initializes GdkRGB if not already done
   */
  if (preview)
    gtk_preview_set_gamma (gimp_gamma ());

  /*  Initialize the eeky vtable needed by libgimpwidgets  */
  vtable.unit_get_number_of_units = gimp_unit_get_number_of_units;
  vtable.unit_get_number_of_built_in_units = gimp_unit_get_number_of_built_in_units;
  vtable.unit_get_factor          = gimp_unit_get_factor;
  vtable.unit_get_digits          = gimp_unit_get_digits;
  vtable.unit_get_identifier      = gimp_unit_get_identifier;
  vtable.unit_get_symbol          = gimp_unit_get_symbol;
  vtable.unit_get_abbreviation    = gimp_unit_get_abbreviation;
  vtable.unit_get_singular        = gimp_unit_get_singular;
  vtable.unit_get_plural          = gimp_unit_get_plural;

  gimp_widgets_init (&vtable,
                     gimp_ui_help_func,
                     gimp_palette_get_foreground,
                     gimp_palette_get_background);

  if (! gimp_show_tool_tips ())
    gimp_help_disable_tooltips ();

  initialized = TRUE;
}


/*  private functions  */

static void
gimp_ui_help_func (const gchar *help_id,
                   gpointer     help_data)
{
  gimp_help (gimp_get_progname (), help_id);
}
