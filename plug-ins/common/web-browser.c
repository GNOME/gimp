/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Web Browser Plug-in
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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

#include <string.h> /* strlen, strstr */

#include <gtk/gtk.h>

#ifdef PLATFORM_OSX
#import <Cocoa/Cocoa.h>
#endif

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-web-browser"
#define PLUG_IN_BINARY "web-browser"
#define PLUG_IN_ROLE   "gimp-web-browser"


static void     query            (void);
static void     run              (const gchar      *name,
                                  gint              nparams,
                                  const GimpParam  *param,
                                  gint             *nreturn_vals,
                                  GimpParam       **return_vals);
static gboolean browser_open_url (GtkWindow        *window,
                                  const gchar      *url,
                                  GError          **error);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_STRING, "url", "URL to open" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          "Open an URL in the user specified web browser",
                          "Opens the given URL in the user specified web browser.",
                          "Henrik Brix Andersen <brix@gimp.org>",
                          "2003",
                          "2003/09/16",
                          NULL, NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar      *name,
     gint             nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpPDBStatusType  status;
  GError            *error = NULL;

  *nreturn_vals = 1;
  *return_vals  = values;

  status   = GIMP_PDB_SUCCESS;

  INIT_I18N ();

  if (nparams == 1 &&
      param[0].data.d_string != NULL &&
      strlen (param[0].data.d_string))
    {
      if (! browser_open_url (NULL, param[0].data.d_string, &error))
        {
          status                  = GIMP_PDB_EXECUTION_ERROR;
          *nreturn_vals           = 2;
          values[1].type          = GIMP_PDB_STRING;
          values[1].data.d_string = error->message;
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static gboolean
browser_open_url (GtkWindow    *window,
                  const gchar  *url,
                  GError      **error)
{
#ifdef G_OS_WIN32

  HINSTANCE hinst = ShellExecute (GetDesktopWindow(),
                                  "open", url, NULL, NULL, SW_SHOW);

  if ((gint) hinst <= 32)
    {
      const gchar *err;

      switch ((gint) hinst)
        {
          case 0 :
            err = _("The operating system is out of memory or resources.");
            break;
          case ERROR_FILE_NOT_FOUND :
            err = _("The specified file was not found.");
            break;
          case ERROR_PATH_NOT_FOUND :
            err = _("The specified path was not found.");
            break;
          case ERROR_BAD_FORMAT :
            err = _("The .exe file is invalid (non-Microsoft Win32 .exe or error in .exe image).");
            break;
          case SE_ERR_ACCESSDENIED :
            err = _("The operating system denied access to the specified file.");
            break;
          case SE_ERR_ASSOCINCOMPLETE :
            err = _("The file name association is incomplete or invalid.");
            break;
          case SE_ERR_DDEBUSY :
            err = _("DDE transaction busy");
            break;
          case SE_ERR_DDEFAIL :
            err = _("The DDE transaction failed.");
            break;
          case SE_ERR_DDETIMEOUT :
            err = _("The DDE transaction timed out.");
            break;
          case SE_ERR_DLLNOTFOUND :
            err = _("The specified DLL was not found.");
            break;
          case SE_ERR_NOASSOC :
            err = _("There is no application associated with the given file name extension.");
            break;
          case SE_ERR_OOM :
            err = _("There was not enough memory to complete the operation.");
            break;
          case SE_ERR_SHARE:
            err = _("A sharing violation occurred.");
            break;
          default :
            err = _("Unknown Microsoft Windows error.");
        }

      g_set_error (error, 0, 0, _("Failed to open '%s': %s"), url, err);

      return FALSE;
    }

  return TRUE;

#elif defined(PLATFORM_OSX)

  NSURL    *ns_url;
  gboolean  retval;

  @autoreleasepool
    {
      ns_url = [NSURL URLWithString: [NSString stringWithUTF8String: url]];
      retval = [[NSWorkspace sharedWorkspace] openURL: ns_url];
    }

  return retval;

#else

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  return gtk_show_uri_on_window (window,
                                 url,
                                 GDK_CURRENT_TIME,
                                 error);

#endif
}
