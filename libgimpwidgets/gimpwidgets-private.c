/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpwidgets-private.c
 * Copyright (C) 2003 Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <babl/babl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpicons.h"
#include "gimpwidgets-private.h"

#include "libgimp/libgimp-intl.h"


static gboolean       gimp_widgets_initialized  = FALSE;

GimpHelpFunc          _gimp_standard_help_func  = NULL;
GimpGetColorFunc      _gimp_get_foreground_func = NULL;
GimpGetColorFunc      _gimp_get_background_func = NULL;
GimpEnsureModulesFunc _gimp_ensure_modules_func = NULL;


static void
gimp_widgets_init_foreign_enums (void)
{
  static const GimpEnumDesc input_mode_descs[] =
  {
    { GDK_MODE_DISABLED, NC_("input-mode", "Disabled"), NULL },
    { GDK_MODE_SCREEN,   NC_("input-mode", "Screen"),   NULL },
    { GDK_MODE_WINDOW,   NC_("input-mode", "Window"),   NULL },
    { 0, NULL, NULL }
  };

  gimp_type_set_translation_domain (GDK_TYPE_INPUT_MODE,
                                    GETTEXT_PACKAGE "-libgimp");
  gimp_type_set_translation_context (GDK_TYPE_INPUT_MODE, "input-mode");
  gimp_enum_set_value_descriptions (GDK_TYPE_INPUT_MODE, input_mode_descs);
}

void
gimp_widgets_init (GimpHelpFunc          standard_help_func,
                   GimpGetColorFunc      get_foreground_func,
                   GimpGetColorFunc      get_background_func,
                   GimpEnsureModulesFunc ensure_modules_func)
{
  GtkIconTheme *icon_theme;

  g_return_if_fail (standard_help_func != NULL);

  if (gimp_widgets_initialized)
    g_error ("gimp_widgets_init() must only be called once!");

  _gimp_standard_help_func  = standard_help_func;
  _gimp_get_foreground_func = get_foreground_func;
  _gimp_get_background_func = get_background_func;
  _gimp_ensure_modules_func = ensure_modules_func;

  babl_init (); /* color selectors use babl */

  gimp_icons_init ();

  icon_theme = gtk_icon_theme_get_for_screen (gdk_screen_get_default ());

  if (gtk_icon_theme_has_icon (icon_theme, GIMP_ICON_WILBER "-symbolic"))
    gtk_window_set_default_icon_name (GIMP_ICON_WILBER "-symbolic");
  else
    gtk_window_set_default_icon_name (GIMP_ICON_WILBER);

  gimp_widgets_init_foreign_enums ();

  gimp_widgets_initialized = TRUE;
}

/* clean up babl (in particular, so that the fish cache is constructed) if the
 * compiler supports destructors
 */
#ifdef HAVE_FUNC_ATTRIBUTE_DESTRUCTOR

__attribute__ ((destructor))
static void
gimp_widgets_exit (void)
{
  if (gimp_widgets_initialized)
    babl_exit ();
}

#elif defined (__GNUC__)

#warning babl_init() not paired with babl_exit()

#endif
