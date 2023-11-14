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
#include <gegl.h>
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
gimp_widgets_init (GimpHelpFunc           standard_help_func,
                   GimpGetColorFunc       get_foreground_func,
                   GimpGetColorFunc       get_background_func,
                   GimpEnsureModulesFunc  ensure_modules_func,
                   const gchar           *test_base_dir)
{
  GList       *icons   = NULL;
  const gchar *cat_dir;
  gchar       *base_dir;
  gchar       *path;
  GdkPixbuf   *pixbuf;
  GError      *error = NULL;

  g_return_if_fail (standard_help_func != NULL);

  if (gimp_widgets_initialized)
    g_error ("gimp_widgets_init() must only be called once!");

  _gimp_standard_help_func  = standard_help_func;
  _gimp_get_foreground_func = get_foreground_func;
  _gimp_get_background_func = get_background_func;
  _gimp_ensure_modules_func = ensure_modules_func;

  babl_init (); /* color selectors use babl */

  gimp_icons_init ();

  if (test_base_dir)
    {
      cat_dir  = "";
      base_dir = g_build_filename (test_base_dir, "desktop", NULL);
    }
  else
    {
      cat_dir  = "apps";
#ifdef ENABLE_RELOCATABLE_RESOURCES
      base_dir = g_build_filename (gimp_installation_directory (), "share", "icons", "hicolor", NULL);
#else
      base_dir = g_build_filename (DATAROOTDIR, "icons", "hicolor", NULL);
#endif
    }

  /* Loading the application icons. Unfortunately GTK doesn't know how
   * to load any size from a single SVG, so we have to generate common
   * sizes ourselves.
   * To be fair, it could with gtk_window_set_default_icon_name() but
   * then the application icon is dependant to the theme and for now at
   * least, we want the installed icon.
   */
  path   = g_build_filename (base_dir, "16x16", cat_dir, "gimp.png", NULL);
  pixbuf = gdk_pixbuf_new_from_file (path, &error);
  if (pixbuf)
    icons = g_list_prepend (icons, pixbuf);
  else
    g_printerr ("Application icon missing: %s (%s)\n", path, error->message);
  g_clear_error (&error);
  g_free (path);

  path   = g_build_filename (base_dir, "32x32", cat_dir, "gimp.png", NULL);
  pixbuf = gdk_pixbuf_new_from_file (path, &error);
  if (pixbuf)
    icons = g_list_prepend (icons, pixbuf);
  else
    g_printerr ("Application icon missing: %s (%s)\n", path, error->message);
  g_clear_error (&error);
  g_free (path);

  path   = g_build_filename (base_dir, "48x48", cat_dir, "gimp.png", NULL);
  pixbuf = gdk_pixbuf_new_from_file (path, &error);
  if (pixbuf)
    icons = g_list_prepend (icons, pixbuf);
  else
    g_printerr ("Application icon missing: %s (%s)\n", path, error->message);
  g_clear_error (&error);
  g_free (path);

  path   = g_build_filename (base_dir, "64x64", cat_dir, "gimp.png", NULL);
  pixbuf = gdk_pixbuf_new_from_file (path, &error);
  if (pixbuf)
    icons = g_list_prepend (icons, pixbuf);
  else
    g_printerr ("Application icon missing: %s (%s)\n", path, error->message);
  g_clear_error (&error);
  g_free (path);

  path   = g_build_filename (base_dir, "scalable", cat_dir, "gimp.svg", NULL);
  pixbuf = gdk_pixbuf_new_from_file_at_size (path, 128, 128, &error);
  if (pixbuf)
    {
      /* Various common sizes from the same SVG. Why I go into such
       * exhaustive list of sizes is that nowadays desktops/OSes use
       * quite big icon sizes and in some cases, when they don't find
       * the right one, GTK may render quite ugly resized/skewed image.
       */
      icons = g_list_prepend (icons, pixbuf);

      pixbuf = gdk_pixbuf_new_from_file_at_size (path, 144, 144, NULL);
      icons = g_list_prepend (icons, pixbuf);

      pixbuf = gdk_pixbuf_new_from_file_at_size (path, 160, 160, NULL);
      icons = g_list_prepend (icons, pixbuf);

      pixbuf = gdk_pixbuf_new_from_file_at_size (path, 176, 176, NULL);
      icons = g_list_prepend (icons, pixbuf);

      pixbuf = gdk_pixbuf_new_from_file_at_size (path, 192, 192, NULL);
      icons = g_list_prepend (icons, pixbuf);

      pixbuf = gdk_pixbuf_new_from_file_at_size (path, 224, 224, NULL);
      icons = g_list_prepend (icons, pixbuf);
    }
  else
    {
      g_printerr ("Application icon missing: %s (%s)\n", path, error->message);
      g_clear_error (&error);
    }
  g_free (path);

  path   = g_build_filename (base_dir, "256x256", cat_dir, "gimp.png", NULL);
  pixbuf = gdk_pixbuf_new_from_file (path, &error);
  if (pixbuf)
    icons = g_list_prepend (icons, pixbuf);
  else
    g_printerr ("Application icon missing: %s\n", path);
  g_free (path);

  gtk_window_set_default_icon_list (icons);
  g_list_free_full (icons, g_object_unref);

  gimp_widgets_init_foreign_enums ();

  gimp_widgets_initialized = TRUE;
  g_free (base_dir);
}

/**
 * gimp_widget_set_identifier:
 * @widget:
 * @identifier:
 *
 * Set an identifier which can be used by the various gimp_blink_*()
 * API. As a default, property widget will use the synced property name
 * as widget identifier. You can always use this function to override a
 * given widget identifier with a more specific name.
 *
 * Note that when a widget is bound to a property, in other words when
 * in one of our propwidgets API, you should rather use
 * gimp_widget_set_bound_property() because it allows more easily to
 * tweak values.
 * gimp_widget_set_identifier() is more destined to random widgets which
 * you just want to be able to blink.
 *
 * It's doesn't need to be in public API because it is only used for
 * internal blinking ability in core GIMP GUI.
 */
void
gimp_widget_set_identifier (GtkWidget   *widget,
                            const gchar *identifier)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_object_set_data_full (G_OBJECT (widget),
                          "gimp-widget-identifier",
                          g_strdup (identifier),
                          (GDestroyNotify) g_free);
}

/**
 * gimp_widget_set_bound_property:
 * @widget:
 * @config:
 * @property_name:
 *
 * This is similar to gimp_widget_set_identifier() because the
 * property_name can be used as identifier by our blink API.
 * You can still set explicitly (and additionally)
 * gimp_widget_set_identifier() for rare cases where 2 widgets in a same
 * container would bind the same property (or 2 properties named the
 * same way for 2 different config objects). The identifier will be used
 * in priority to the property name (which can still be used for
 * changing the widget value, so it remains important to also set it).
 *
 * It's doesn't need to be in public API because it is only used for
 * internal blinking ability in core GIMP GUI.
 */
void
gimp_widget_set_bound_property (GtkWidget   *widget,
                                GObject     *config,
                                const gchar *property_name)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_object_set_data_full (G_OBJECT (widget),
                          "gimp-widget-property-name",
                          g_strdup (property_name),
                          (GDestroyNotify) g_free);
  g_object_set_data_full (G_OBJECT (widget),
                          "gimp-widget-property-config",
                          g_object_ref (config),
                          (GDestroyNotify) g_object_unref);
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
