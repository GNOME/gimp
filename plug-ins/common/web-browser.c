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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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


typedef struct _Browser      Browser;
typedef struct _BrowserClass BrowserClass;

struct _Browser
{
  GimpPlugIn parent_instance;
};

struct _BrowserClass
{
  GimpPlugInClass parent_class;
};


#define BROWSER_TYPE  (browser_get_type ())
#define BROWSER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BROWSER_TYPE, Browser))

GType                   browser_get_type         (void) G_GNUC_CONST;

static GList          * browser_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * browser_create_procedure (GimpPlugIn           *plug_in,
                                                  const gchar          *name);

static GimpValueArray * browser_run              (GimpProcedure        *procedure,
                                                  GimpProcedureConfig  *config,
                                                  gpointer              run_data);

static gboolean         browser_open_url         (GtkWindow            *window,
                                                  const gchar          *url,
                                                  GError              **error);


G_DEFINE_TYPE (Browser, browser, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (BROWSER_TYPE)
DEFINE_STD_SET_I18N


static void
browser_class_init (BrowserClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = browser_query_procedures;
  plug_in_class->create_procedure = browser_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
browser_init (Browser *browser)
{
}

static GList *
browser_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
browser_create_procedure (GimpPlugIn  *plug_in,
                          const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      browser_run, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Open an URL in the user specified "
                                        "web browser",
                                        "Opens the given URL in the user "
                                        "specified web browser.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Henrik Brix Andersen <brix@gimp.org>",
                                      "2003",
                                      "2003/09/16");

      gimp_procedure_add_string_argument (procedure, "url",
                                          "URL",
                                         "URL to open",
                                         "https://www.gimp.org/",
                                         G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
browser_run (GimpProcedure        *procedure,
             GimpProcedureConfig  *config,
             gpointer              run_data)
{
  GError *error = NULL;
  gchar  *url   = NULL;

  g_object_get (config, "url", &url, NULL);
  if (! browser_open_url (NULL, url, &error))
    {
      g_free (url);
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }

  g_free (url);
  return gimp_procedure_new_return_values (procedure,
                                           GIMP_PDB_SUCCESS,
                                           NULL);
}

static gboolean
browser_open_url (GtkWindow    *window,
                  const gchar  *url,
                  GError      **error)
{
#ifdef G_OS_WIN32

  HINSTANCE hinst = ShellExecute (GetDesktopWindow(),
                                  "open", url, NULL, NULL, SW_SHOW);

  if ((intptr_t) hinst <= 32)
    {
      const gchar *err;

      switch ((intptr_t) hinst)
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

  gimp_ui_init (PLUG_IN_BINARY);

  return gtk_show_uri_on_window (window,
                                 url,
                                 GDK_CURRENT_TIME,
                                 error);

#endif
}
