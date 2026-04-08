/*   GIMP - The GNU Image Manipulation Program
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   WIA Scanner plug-in
 *
 *   Copyright (C) 2026 Alex S.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib.h>		/* Needed when compiling with gcc */
#include <wia.h>
#include <wia_lh.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libgimp/gimp.h"
#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_NAME        "wia-acquire"
#define PLUG_IN_DESCRIPTION _("Capture an image from a WIA datasource")
#define PLUG_IN_HELP        N_("This plug-in will capture an image from a WIA datasource")

#define MAX_FOLDER_PATH 1024

typedef struct _Wia      Wia;
typedef struct _WiaClass WiaClass;

struct _Wia
{
  GimpPlugIn      parent_instance;
};

struct _WiaClass
{
  GimpPlugInClass parent_class;
};


#define WIA_TYPE  (wia_get_type ())
#define WIA(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WIA_TYPE, Wia))

GType                   wia_get_type           (void) G_GNUC_CONST;

static GList          * wia_query_procedures   (GimpPlugIn           *plug_in);
static GimpProcedure  * wia_create_procedure   (GimpPlugIn           *plug_in,
                                                const gchar          *name);

static GimpValueArray * wia_run                (GimpProcedure        *procedure,
                                                GimpRunMode           run_mode,
                                                GimpImage            *image,
                                                GimpDrawable        **drawables,
                                                GimpProcedureConfig  *config,
                                                gpointer              run_data);

static GimpValueArray * wia_main               (GimpProcedure        *procedure,
                                                GimpProcedureConfig  *config);

static HRESULT          CreateWiaDeviceManager (IWiaDevMgr2 **ppWiaDevMgr);

G_DEFINE_TYPE (Wia, wia, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (WIA_TYPE)
DEFINE_STD_SET_I18N


static void
wia_class_init (WiaClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = wia_query_procedures;
  plug_in_class->create_procedure = wia_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
wia_init (Wia *wia)
{
}

static GList *
wia_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_NAME));
}

static GimpProcedure *
wia_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_NAME))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            wia_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE     |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES    |
                                           GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES |
                                           GIMP_PROCEDURE_SENSITIVE_NO_IMAGE);

      gimp_procedure_set_menu_label (procedure, _("_Scanner/Camera..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/File/Create");

      gimp_procedure_set_documentation (procedure,
                                        PLUG_IN_DESCRIPTION,
                                        PLUG_IN_HELP,
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Alx Sa",
                                      "Alx Sa",
                                      "2026");

      gimp_procedure_add_string_aux_argument (procedure, "scan-directory",
                                              NULL, NULL,
                                              NULL,
                                              GIMP_PARAM_READWRITE);

      gimp_procedure_add_core_object_array_return_value (procedure, "images",
                                                         "Array of acquired images",
                                                         "Array of acquired images",
                                                         GIMP_TYPE_IMAGE,
                                                         G_PARAM_READWRITE);
     }

  return procedure;
}

static GimpValueArray *
wia_run (GimpProcedure        *procedure,
         GimpRunMode           run_mode,
         GimpImage            *image,
         GimpDrawable        **drawables,
         GimpProcedureConfig  *config,
         gpointer              run_data)
{
  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_NONINTERACTIVE)
    /* Currently, we don't do non-interactive calls.
     * Bail if someone tries to call us non-interactively
     */
    return gimp_procedure_new_return_values (procedure, GIMP_PDB_CALLING_ERROR,
                                             NULL);

  return wia_main (procedure, config);
}

/* Windows methods */
static HRESULT
CreateWiaDeviceManager (IWiaDevMgr2 **ppWiaDevMgr)
{
  HRESULT hr;

  if (NULL == ppWiaDevMgr)
    return E_INVALIDARG;

  *ppWiaDevMgr = NULL;

  hr = CoCreateInstance (&CLSID_WiaDevMgr2, NULL, CLSCTX_LOCAL_SERVER,
                         &IID_IWiaDevMgr2, (void**) ppWiaDevMgr);

  return hr;
}

static GimpValueArray *
wia_main (GimpProcedure       *procedure,
          GimpProcedureConfig *config)
{
  GError  *error = NULL;
  HRESULT  hr    = CoInitialize (NULL);

  if (SUCCEEDED (hr))
    {
      IWiaDevMgr2 *pWiaDevMgr2 = NULL;

      hr = CreateWiaDeviceManager (&pWiaDevMgr2);

      if (SUCCEEDED (hr))
        {
          GimpImage      **images         = NULL;
          GimpValueArray  *return_vals    = NULL;
          HWND             window_handle;
          LONG             nFiles         = 0;
          LPWSTR          *rFilePaths     = NULL;
          IWiaItem2       *pItem          = NULL;
          BSTR             szFolder       = NULL;
          BSTR             szFileName;
          TCHAR            temp_path[MAX_FOLDER_PATH];
          gchar           *scan_directory = NULL;

          szFileName = SysAllocString (L"GIMP-SCANNED-IMAGE");

          /* Check if we have a saved directory, and if so, if it's still
           * valid. */
          g_object_get (config, "scan-directory", &scan_directory, NULL);
          if (scan_directory != NULL)
            {
              GDir *directory = g_dir_open (scan_directory, 0, NULL);

              if (directory)
                {
                  szFolder =
                    SysAllocString (g_utf8_to_utf16 (scan_directory, -1, NULL,
                                                     NULL, NULL));
                  g_dir_close (directory);
                }
              g_free (scan_directory);
            }

          /* Otherwise, use Window's temp location */
          if (szFolder == NULL)
            {
              GetTempPath (MAX_FOLDER_PATH, temp_path);
              szFolder = SysAllocString (g_utf8_to_utf16 (temp_path, -1, NULL,
                                                          NULL, NULL));
            }

          /* Values taken from app/gui/gui-unique.h */
          window_handle = FindWindowW (L"GimpWin32UniqueHandler", L"GimpProxy");

          hr = pWiaDevMgr2->lpVtbl->GetImageDlg (pWiaDevMgr2, 0, NULL, window_handle,
                                                 szFolder, szFileName, &nFiles,
                                                 &rFilePaths, &pItem);
          SysFreeString (szFolder);
          SysFreeString (szFileName);

          if (FAILED (hr))
            {
              pWiaDevMgr2->lpVtbl->Release (pWiaDevMgr2);
              pWiaDevMgr2 = NULL;

              CoUninitialize ();

              g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                           _("Unable to read scanned images."));

              return gimp_procedure_new_return_values (procedure,
                                                       GIMP_PDB_EXECUTION_ERROR,
                                                       NULL);
            }

          if (nFiles > 0)
            {
              images = g_new0 (GimpImage *, nFiles + 1);

              for (gint i = 0; i < nFiles; i++)
                {
                  GFile *file;
                  gchar *filename = g_utf16_to_utf8 (rFilePaths[i], -1, NULL,
                                                     NULL, NULL);

                  file = g_file_new_for_path (filename);

                  images[i] = gimp_file_load (GIMP_RUN_NONINTERACTIVE, file);
                  gimp_display_new (images[i]);
                  if (! gimp_image_undo_is_enabled (images[i]))
                    gimp_image_undo_enable (images[i]);

                  /* Store first file's directory for later use */
                  if (i == 0)
                    {
                      GFile *parent = g_file_get_parent (file);

                      g_object_set (config,
                                    "scan-directory",
                                    g_file_get_parse_name (parent),
                                    NULL);
                      g_object_unref (parent);
                    }

                  g_object_unref (file);
                  SysFreeString (rFilePaths[i]);
                  g_free (filename);
                }

              CoTaskMemFree (rFilePaths);
            }

          pWiaDevMgr2->lpVtbl->Release (pWiaDevMgr2);
          pWiaDevMgr2 = NULL;

          CoUninitialize ();

          return_vals = gimp_procedure_new_return_values (procedure,
                                                          GIMP_PDB_SUCCESS,
                                                          NULL);
          GIMP_VALUES_TAKE_CORE_OBJECT_ARRAY (return_vals, 1, images);

          return return_vals;
        }
      else
        {
          g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                       _("WIA driver not found, scanning not available"));

          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_EXECUTION_ERROR,
                                                   NULL);
        }

      CoUninitialize ();
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_EXECUTION_ERROR,
                                           NULL);
}
