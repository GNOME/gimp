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
screenshot_freedesktop_dbus_signal (GDBusProxy *proxy,
                                    gchar      *sender_name,
                                    gchar      *signal_name,
                                    GVariant   *parameters,
                                    gint32     *image_ID)
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
              gchar *path = g_file_get_path (file);

              *image_ID = gimp_file_load (GIMP_RUN_NONINTERACTIVE,
                                          path, path);
              gimp_image_set_filename (*image_ID, "screenshot.png");

              /* Delete the actual file. */
              g_file_delete (file, NULL, NULL);

              g_object_unref (file);
              g_free (path);
              g_free (uri);
            }
        }

      g_variant_unref (results);
      /* Quit anyway. */
      gtk_main_quit ();
    }
}

GimpPDBStatusType
screenshot_freedesktop_shoot (ScreenshotValues  *shootvals,
                              GdkScreen         *screen,
                              gint32            *image_ID,
                              GError           **error)
{
  GVariant *retval;
  gchar    *opath = NULL;

  if (shootvals->shoot_type != SHOOT_ROOT)
    {
      /* This should not happen. */
      return GIMP_PDB_EXECUTION_ERROR;
    }

  if (shootvals->screenshot_delay > 0)
    screenshot_delay (shootvals->screenshot_delay);

  retval = g_dbus_proxy_call_sync (proxy, "Screenshot",
                                   g_variant_new ("(sa{sv})", "", NULL),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1, NULL, error);
  g_object_unref (proxy);
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
      *image_ID = 0;
      g_signal_connect (proxy2, "g-signal",
                        G_CALLBACK (screenshot_freedesktop_dbus_signal),
                        image_ID);

      gtk_main ();
      g_object_unref (proxy2);
      g_free (opath);

      /* Signal got a response. */
      if (*image_ID)
        {
          GimpColorProfile *profile;

          /* Just assign profile of current monitor. This will work only
           * as long as this is a single-display setup.
           * We need to figure out how to do better color management for
           * portal screenshots.
           * TODO!
           */
          profile = gimp_screen_get_color_profile (screen,
                                                   shootvals->monitor);
          if (profile)
            {
              gimp_image_set_color_profile (*image_ID, profile);
              g_object_unref (profile);
            }

          return GIMP_PDB_SUCCESS;
        }
    }

  return GIMP_PDB_EXECUTION_ERROR;
}
