/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppickbutton-kwin.c
 * Copyright (C) 2017 Jehan <jehan@gimp.org>
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"
#include "gimppickbutton.h"
#include "gimppickbutton-default.h"
#include "gimppickbutton-kwin.h"

#include "libgimp/libgimp-intl.h"

gboolean
_gimp_pick_button_kwin_available (void)
{
  GDBusProxy *proxy = NULL;
  gboolean    available = FALSE;

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                         NULL,
                                         "org.kde.KWin",
                                         "/ColorPicker",
                                         "org.kde.kwin.ColorPicker",
                                         NULL, NULL);

  if (proxy)
    {
      GError *error = NULL;

      g_dbus_proxy_call_sync (proxy, "org.freedesktop.DBus.Peer.Ping",
                              NULL,
                              G_DBUS_CALL_FLAGS_NONE,
                              -1, NULL, &error);
      if (! error)
        available = TRUE;

      g_clear_error (&error);
      g_object_unref (proxy);
    }

  return available;
}

/* entry point to this file, called from gimppickbutton.c */
void
_gimp_pick_button_kwin_pick (GimpPickButton *button)
{
  GDBusProxy *proxy = NULL;
  GError     *error = NULL;
  GVariant   *retval;

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                         NULL,
                                         "org.kde.KWin",
                                         "/ColorPicker",
                                         "org.kde.kwin.ColorPicker",
                                         NULL, NULL);
  g_return_if_fail (proxy);

  retval = g_dbus_proxy_call_sync (proxy, "pick", NULL,
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1, NULL, &error);
  if (retval)
    {
      GimpRGB rgb;
      guint32 color;

      g_variant_get (retval, "((u))", &color);
      g_variant_unref (retval);
      /* Returned value is ARGB stored in uint32. */
      gimp_rgba_set_uchar (&rgb,
                           (color  >> 16 ) & 0xff, /* Red                           */
                           (color >> 8) & 0xff,    /* Green                         */
                           color & 0xff,           /* Blue: least significant byte. */
                           (color >> 24) & 0xff);  /* Alpha: most significant byte. */
      g_signal_emit_by_name (button, "color-picked", &rgb);
    }
  else
    {
      /* I had failure of KDE's color picking API. So let's just
       * fallback to the default color picking when this happens. This
       * will at least work on X11.
       * See: https://bugs.kde.org/show_bug.cgi?id=387720
       */
      if (error)
        g_warning ("KWin backend for color picking failed with error: %s",
                   error->message);
      _gimp_pick_button_default_pick (GIMP_PICK_BUTTON (button));
    }
  g_clear_error (&error);
  g_object_unref (proxy);
}
