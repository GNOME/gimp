/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-raw-utils.h -- raw file format plug-in
 * Copyright (C) 2016 Tobias Ellinghaus <me@houz.org>
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

#include <gdk/gdk.h>

#ifdef GDK_WINDOWING_QUARTZ
#include <CoreServices/CoreServices.h>
#endif

#ifdef GDK_WINDOWING_WIN32
#include <windows.h>
#endif

#include <libgimp/gimp.h>

#include "file-raw-utils.h"


gchar *
file_raw_get_executable_path (const gchar *main_executable,
                              const gchar *suffix,
                              const gchar *env_variable,
                              const gchar *mac_bundle_id,
                              const gchar *win32_registry_key_base,
                              gboolean    *search_path)
{
  /*
   * First check for the environment variable.
   * Next do platform specific checks (bundle lookup on Mac, registry stuff
   * on Windows).
   * Last resort is hoping for the executable to be in PATH.
   */

  /*
   * Look for env variable. That can be set directly or via an environ file.
   * We assume that just appending the suffix to that value will work.
   * That means that on Windows there should be no ".exe"!
   */
  const gchar *dt_env = env_variable ? g_getenv (env_variable) : NULL;

  if (dt_env)
    return g_strconcat (dt_env, suffix, NULL);

#if defined (GDK_WINDOWING_QUARTZ)
  if (mac_bundle_id)
    {
      CFStringRef bundle_id;

      /* For macOS, attempt searching for an app bundle first. */
      bundle_id = CFStringCreateWithCString (NULL, mac_bundle_id,
                                             kCFStringEncodingUTF8);
      if (bundle_id)
        {
          OSStatus status;
          CFURLRef bundle_url = NULL;

          status = LSFindApplicationForInfo (kLSUnknownCreator,
                                             bundle_id, NULL, NULL,
                                             &bundle_url);
          if (status >= 0)
            {
              CFBundleRef  bundle;
              CFURLRef     exec_url, absolute_url;
              CFStringRef  path;
              gchar       *ret;
              CFIndex      len;

              bundle = CFBundleCreate (kCFAllocatorDefault, bundle_url);
              CFRelease (bundle_url);

              exec_url = CFBundleCopyExecutableURL (bundle);
              absolute_url = CFURLCopyAbsoluteURL (exec_url);
              path = CFURLCopyFileSystemPath (absolute_url, kCFURLPOSIXPathStyle);

              /* This gets us the length in UTF16 characters, we multiply by 2
               * to make sure we have a buffer big enough to fit the UTF8 string.
               */
              len = CFStringGetLength (path);
              ret = g_malloc0 (len * 2 * sizeof (gchar));
              if (! CFStringGetCString (path, ret, 2 * len * sizeof (gchar),
                                        kCFStringEncodingUTF8))
                ret = NULL;

              CFRelease (path);
              CFRelease (absolute_url);
              CFRelease (exec_url);
              CFRelease (bundle);

              if (ret)
                return ret;
            }

          CFRelease (bundle_id);
        }
      /* else, app bundle was not found, try path search as last resort. */
    }

#elif defined (GDK_WINDOWING_WIN32)
  if (win32_registry_key_base)
    {
      /* Look for the application in the Windows registry. */
      char  *registry_key;
      char   path[MAX_PATH];
      DWORD  buffer_size = sizeof (path);
      long   status;

      if (suffix)
        registry_key = g_strconcat (win32_registry_key_base, suffix, ".exe", NULL);
      else
        registry_key = g_strconcat (win32_registry_key_base, ".exe", NULL);

      /* Check HKCU first in case there is a user specific installation. */
      status = RegGetValue (HKEY_CURRENT_USER, registry_key, "", RRF_RT_ANY,
                            NULL, (PVOID)&path, &buffer_size);

      if (status != ERROR_SUCCESS)
        status = RegGetValue (HKEY_LOCAL_MACHINE, registry_key, "", RRF_RT_ANY,
                              NULL, (PVOID)&path, &buffer_size);

      g_free (registry_key);

      if (status == ERROR_SUCCESS)
        return g_strdup (path);
    }
#endif

  /* Finally, the last resort. */
  *search_path = TRUE;

  if (suffix)
    return g_strconcat (main_executable, suffix, NULL);

  return g_strdup (main_executable);
}
