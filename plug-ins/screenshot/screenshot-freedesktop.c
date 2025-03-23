/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Screenshot plug-in
 * Copyright 1998-2007 Sven Neumann <sven@gimp.org>
 * Copyright 2003      Henrik Brix Andersen <brix@gimp.org>
 * Copyright 2016      Michael Natterer <mitch@gimp.org>
 * Copyright 2017      Jehan <jehan@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#include "screenshot.h"
#include "screenshot-freedesktop.h"


static GDBusProxy *proxy = NULL;

gboolean
screenshot_freedesktop_available (void)
{
  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                         NULL,
                                         "org.freedesktop.portal.Desktop",
                                         "/org/freedesktop/portal/desktop",
                                         "org.freedesktop.portal.Screenshot",
                                         NULL, NULL);

  if (proxy)
    {
      GError *error = NULL;

      g_dbus_proxy_call_sync (proxy, "org.freedesktop.DBus.Peer.Ping",
                              NULL,
                              G_DBUS_CALL_FLAGS_NONE,
                              -1, NULL, &error);
      if (! error)
        return TRUE;

      g_clear_error (&error);

      g_object_unref (proxy);
      proxy = NULL;
    }

  return FALSE;
}

ScreenshotCapabilities
screenshot_freedesktop_get_capabilities (void)
{
  /* Portal has no capabilities other than root screenshot! */
  return 0;
}

static void
screenshot_freedesktop_dbus_signal (GDBusProxy  *proxy,
                                    gchar       *sender_name,
                                    gchar       *signal_name,
                                    GVariant    *parameters,
                                    GimpImage  **image)
{
  if (g_strcmp0 (signal_name, "Response") == 0)
    {
      GVariant *results;
      guint32   response;

      g_variant_get (parameters, "(u@a{sv})",
                     &response,
                     &results);

      /* Possible values:
       * 0: Success, the request is carried out
       * 1: The user cancelled the interaction
       * 2: The user interaction was ended in some other way
       * Cf. https://github.com/flatpak/xdg-desktop-portal/blob/master/data/org.freedesktop.portal.Request.xml
       */
      if (response == 0)
        {
          gchar *uri;

          if (g_variant_lookup (results, "uri", "s", &uri))
            {
              GFile *file = g_file_new_for_uri (uri);

              *image = gimp_file_load (GIMP_RUN_NONINTERACTIVE, file);

              /* Delete the actual file. */
              g_file_delete (file, NULL, NULL);

              g_object_unref (file);
              g_free (uri);
            }
        }

      g_variant_unref (results);
      /* Quit anyway. */
      gtk_main_quit ();
    }
}

GimpPDBStatusType
screenshot_freedesktop_shoot (GdkMonitor  *monitor,
                              GimpImage  **image,
                              GError     **error)
{
  GVariant        *retval;
  GVariantBuilder *options;
  gchar           *opath         = NULL;
  gchar           *parent_window = NULL;
#if defined (GDK_WINDOWING_X11) || defined (GDK_WINDOWING_WAYLAND)
  GBytes          *handle;

  handle = gimp_progress_get_window_handle ();
#endif

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    {
      GdkWindow *window;
      Window    *handle_data;
      Window     window_id;
      gsize      handle_size;

      handle_data = (Window *) g_bytes_get_data (handle, &handle_size);
      g_return_val_if_fail (handle_size == sizeof (Window), GIMP_PDB_EXECUTION_ERROR);
      window_id = *handle_data;

      window = gdk_x11_window_foreign_new_for_display (gdk_display_get_default (), window_id);
      if (window)
        {
          gint id;

          id = GDK_WINDOW_XID (window);
          parent_window = g_strdup_printf ("x11:0x%x", id);
        }
    }
#endif
#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default ()))
    {
      char  *handle_data;
      gchar *handle_str;
      gsize  handle_size;

      handle_data = (char *) g_bytes_get_data (handle, &handle_size);
      /* Even though this should be the case by design, this ensures the
       * string is NULL-terminated to avoid out-of allocated memory access.
       */
      handle_str = g_strndup (handle_data, handle_size);
      parent_window = g_strdup_printf ("wayland:%s", handle_str);
      g_free (handle_str);
    }
#endif

#if defined (GDK_WINDOWING_X11) || defined (GDK_WINDOWING_WAYLAND)
  g_bytes_unref (handle);
#endif

  options = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
  /* "interactive" option will display the options first (otherwise, it
   * makes a screenshot first, then proposes to tweak. Since version 2
   * of the API. For older implementations, it should just be ignored.
   */
  g_variant_builder_add (options, "{sv}", "interactive", g_variant_new_boolean (TRUE));

  retval = g_dbus_proxy_call_sync (proxy, "Screenshot",
                                   g_variant_new ("(sa{sv})",
                                                  parent_window ? parent_window : "",
                                                  options, NULL),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1, NULL, error);
  g_free (parent_window);
  g_object_unref (proxy);
  g_variant_builder_unref (options);

  proxy = NULL;
  if (retval)
    {
      g_variant_get (retval, "(o)", &opath);
      g_variant_unref (retval);
    }

  if (opath)
    {
      GDBusProxy *proxy2 = NULL;

      proxy2 = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                              G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                              NULL,
                                              "org.freedesktop.portal.Desktop",
                                              opath,
                                              "org.freedesktop.portal.Request",
                                              NULL, NULL);
      *image = NULL;
      g_signal_connect (proxy2, "g-signal",
                        G_CALLBACK (screenshot_freedesktop_dbus_signal),
                        image);

      gtk_main ();
      g_object_unref (proxy2);
      g_free (opath);

      /* Signal got a response. */
      if (*image)
        {
          if (! gimp_image_get_color_profile (*image))
            {
              /* The Freedesktop portal does not return a profile, so we
               * don't have color characterization through the API.
               * Ideally then, the returned screenshot image would have
               * embedded profile, but this depends on each desktop
               * implementation of the portal (and at time of writing,
               * the GNOME implementation of Freedesktop portal at least
               * didn't embed a profile with the returned PNG image).
               *
               * As a last resort, we assign the profile of current
               * monitor. This will actually only work if we use the
               * portal on X11 (because we don't have monitor's profile
               * access on Wayland AFAIK), and only as long as this is a
               * single-display setup.
               *
               * We need to figure out how to do better color management for
               * portal screenshots. TODO!
               */
              GimpColorProfile *profile;

              profile = gimp_monitor_get_color_profile (monitor);
              if (profile)
                {
                  gimp_image_set_color_profile (*image, profile);
                  g_object_unref (profile);
                }
            }

          return GIMP_PDB_SUCCESS;
        }
    }

  return GIMP_PDB_EXECUTION_ERROR;
}
