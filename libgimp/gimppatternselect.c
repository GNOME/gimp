/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppatternselect.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
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

#include "gimp.h"


typedef struct
{
  gchar                  *pattern_callback;
  guint                   idle_id;
  gchar                  *pattern_name;
  gint                    width;
  gint                    height;
  gint                    bytes;
  guchar                 *pattern_mask_data;
  GimpRunPatternCallback  callback;
  gboolean                closing;
  gpointer                data;
  GDestroyNotify          data_destroy;
} GimpPatternData;


/*  local function prototypes  */

static void             gimp_pattern_data_free (GimpPatternData      *data);

static GimpValueArray * gimp_temp_pattern_run  (GimpProcedure        *procedure,
                                                const GimpValueArray *args,
                                                gpointer              run_data);
static gboolean         gimp_temp_pattern_idle (GimpPatternData      *data);


/*  public functions  */

const gchar *
gimp_pattern_select_new (const gchar            *title,
                         const gchar            *pattern_name,
                         GimpRunPatternCallback  callback,
                         gpointer                data,
                         GDestroyNotify          data_destroy)
{
  GimpPlugIn      *plug_in = gimp_get_plug_in ();
  GimpProcedure   *procedure;
  gchar           *pattern_callback;
  GimpPatternData *pattern_data;

  pattern_callback = gimp_pdb_temp_procedure_name (gimp_get_pdb ());

  pattern_data = g_slice_new0 (GimpPatternData);

  pattern_data->pattern_callback = pattern_callback;
  pattern_data->callback         = callback;
  pattern_data->data             = data;
  pattern_data->data_destroy     = data_destroy;

  procedure = gimp_procedure_new (plug_in,
                                  pattern_callback,
                                  GIMP_PDB_PROC_TYPE_TEMPORARY,
                                  gimp_temp_pattern_run,
                                  pattern_data,
                                  (GDestroyNotify)
                                  gimp_pattern_data_free);

  GIMP_PROC_ARG_STRING (procedure, "pattern-name",
                        "Pattern name",
                        "The pattern name",
                        NULL,
                        G_PARAM_READWRITE);

  GIMP_PROC_ARG_INT (procedure, "mask-width",
                     "Mask width",
                     "Pattern width",
                     0, 10000, 0,
                     G_PARAM_READWRITE);

  GIMP_PROC_ARG_INT (procedure, "mask-height",
                     "Mask height",
                     "Pattern height",
                     0, 10000, 0,
                     G_PARAM_READWRITE);

  GIMP_PROC_ARG_INT (procedure, "mask-bpp",
                     "Mask bpp",
                     "Pattern bytes per pixel",
                     0, 10000, 0,
                     G_PARAM_READWRITE);

  GIMP_PROC_ARG_INT (procedure, "mask-len",
                     "Mask length",
                     "Length of pattern mask data",
                     0, G_MAXINT, 0,
                     G_PARAM_READWRITE);

  GIMP_PROC_ARG_UINT8_ARRAY (procedure, "mask-data",
                             "Mask data",
                             "The pattern mask data",
                             G_PARAM_READWRITE);

  GIMP_PROC_ARG_BOOLEAN (procedure, "closing",
                         "Closing",
                         "If the dialog was cloaing",
                         FALSE,
                         G_PARAM_READWRITE);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  if (gimp_patterns_popup (pattern_callback, title, pattern_name))
    {
      /* Allow callbacks to be watched */
      gimp_plug_in_extension_enable (plug_in);

      return pattern_callback;
    }

  gimp_plug_in_remove_temp_procedure (plug_in, pattern_callback);

  return NULL;
}

void
gimp_pattern_select_destroy (const gchar *pattern_callback)
{
  GimpPlugIn *plug_in = gimp_get_plug_in ();

  g_return_if_fail (pattern_callback != NULL);

  gimp_plug_in_remove_temp_procedure (plug_in, pattern_callback);
}


/*  private functions  */

static void
gimp_pattern_data_free (GimpPatternData *data)
{
  if (data->idle_id)
    g_source_remove (data->idle_id);

  if (data->pattern_callback)
    {
      gimp_patterns_close_popup (data->pattern_callback);
      g_free (data->pattern_callback);
    }

  g_free (data->pattern_name);
  g_free (data->pattern_mask_data);

  if (data->data_destroy)
    data->data_destroy (data->data);

  g_slice_free (GimpPatternData, data);
}

static GimpValueArray *
gimp_temp_pattern_run (GimpProcedure        *procedure,
                       const GimpValueArray *args,
                       gpointer              run_data)
{
  GimpPatternData *data = run_data;

  g_free (data->pattern_name);
  g_free (data->pattern_mask_data);

  data->pattern_name      = GIMP_VALUES_DUP_STRING      (args, 0);
  data->width             = GIMP_VALUES_GET_INT         (args, 1);
  data->height            = GIMP_VALUES_GET_INT         (args, 2);
  data->bytes             = GIMP_VALUES_GET_INT         (args, 3);
  data->pattern_mask_data = GIMP_VALUES_DUP_UINT8_ARRAY (args, 5);
  data->closing           = GIMP_VALUES_GET_BOOLEAN     (args, 6);

  if (! data->idle_id)
    data->idle_id = g_idle_add ((GSourceFunc) gimp_temp_pattern_idle, data);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
gimp_temp_pattern_idle (GimpPatternData *data)
{
  data->idle_id = 0;

  if (data->callback)
    data->callback (data->pattern_name,
                    data->width,
                    data->height,
                    data->bytes,
                    data->pattern_mask_data,
                    data->closing,
                    data->data);

  if (data->closing)
    {
      gchar *pattern_callback = data->pattern_callback;

      data->pattern_callback = NULL;
      gimp_pattern_select_destroy (pattern_callback);
      g_free (pattern_callback);
    }

  return G_SOURCE_REMOVE;
}
