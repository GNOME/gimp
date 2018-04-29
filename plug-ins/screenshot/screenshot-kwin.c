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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib.h>
#include <glib/gstdio.h> /* g_unlink() */

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "screenshot.h"
#include "screenshot-kwin.h"


static GDBusProxy *proxy = NULL;


gboolean
screenshot_kwin_available (void)
{
  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                         NULL,
                                         "org.kde.KWin",
                                         "/Screenshot",
                                         "org.kde.kwin.Screenshot",
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
screenshot_kwin_get_capabilities (void)
{
  return (SCREENSHOT_CAN_SHOOT_DECORATIONS |
          SCREENSHOT_CAN_SHOOT_POINTER     |
          SCREENSHOT_CAN_SHOOT_WINDOW      |
          SCREENSHOT_CAN_PICK_WINDOW);
  /* TODO: SCREENSHOT_CAN_SHOOT_REGION.
   * The KDE API has "screenshotArea" method but no method to get
   * coordinates could be found. See below.
   */
}

GimpPDBStatusType
screenshot_kwin_shoot (ScreenshotValues  *shootvals,
                       GdkMonitor        *monitor,
                       gint32            *image_ID,
                       GError           **error)
{
  gchar       *filename = NULL;
  const gchar *method   = NULL;
  GVariant    *args     = NULL;
  GVariant    *retval;
  gint32       mask;

  switch (shootvals->shoot_type)
    {
    case SHOOT_ROOT:
      if (shootvals->screenshot_delay > 0)
        screenshot_delay (shootvals->screenshot_delay);
      else
        {
          /* As an exception, I force a delay of at least 0.5 seconds
           * for KWin. Because of windows effect slowly fading out, the
           * screenshot plug-in GUI was constantly visible (with
           * transparency as it is fading out) in 0s-delay screenshots.
           */
          g_usleep (500000);
        }

      method = "screenshotFullscreen";
      args   = g_variant_new ("(b)", shootvals->show_cursor);

      /* FIXME: figure profile */
      break;

    case SHOOT_REGION:
      break;
      /* FIXME: GNOME-shell has a "SelectArea" returning coordinates
       * which can be fed to "ScreenshotArea". KDE has the equivalent
       * "screenshotArea", but no "SelectArea" equivalent that I could
       * find.
       * Also at first, I expected "interactive" method to take care of
       * the whole selecting-are-then-screenshotting workflow, but this
       * is apparently only made to select interactively a specific
       * window, not an area.
       */
      method = "screenshotArea";
      args   = g_variant_new ("(iiii)",
                              shootvals->x1,
                              shootvals->y1,
                              shootvals->x2 - shootvals->x1,
                              shootvals->y2 - shootvals->y1);
      args   = NULL;

      break;

    case SHOOT_WINDOW:
      if (shootvals->select_delay > 0)
        screenshot_delay (shootvals->select_delay);

      /* XXX I expected "screenshotWindowUnderCursor" method to be the
       * right one, but it returns nothing, nor is there a file
       * descriptor in argument. So I don't understand how to grab the
       * screenshot. Also "interactive" changes the cursor to a
       * crosshair, waiting for click, which is more helpful than
       * immediate screenshot under cursor.
       */
      method = "interactive";
      mask = (shootvals->decorate ? 1 : 0) |
             (shootvals->show_cursor ? 1 << 1 : 0);
      args   = g_variant_new ("(i)", mask);

      /* FIXME: figure monitor */
      break;
    }

  retval = g_dbus_proxy_call_sync (proxy, method, args,
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1, NULL, error);
  if (! retval)
    goto failure;

  g_variant_get (retval, "(s)",
                 &filename);
  g_variant_unref (retval);

  if (filename)
    {
      GimpColorProfile *profile;

      *image_ID = gimp_file_load (GIMP_RUN_NONINTERACTIVE,
                                  filename, filename);
      gimp_image_set_filename (*image_ID, "screenshot.png");

      /* This is very wrong in multi-display setups since we have no
       * idea which profile is to be used. Let's keep it anyway and
       * assume always the monitor 0, which will still work in common
       * cases.
       */
      profile = gimp_monitor_get_color_profile (monitor);

      if (profile)
        {
          gimp_image_set_color_profile (*image_ID, profile);
          g_object_unref (profile);
        }

      g_unlink (filename);
      g_free (filename);

      g_object_unref (proxy);
      proxy = NULL;

      return GIMP_PDB_SUCCESS;
    }

 failure:

  if (filename)
    g_free (filename);

  g_object_unref (proxy);
  proxy = NULL;

  return GIMP_PDB_EXECUTION_ERROR;
}
