/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppaletteselect.c
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
  gchar                  *palette_callback;
  guint                   idle_id;
  gchar                  *palette_name;
  gint                    num_colors;
  GimpRunPaletteCallback  callback;
  gboolean                closing;
  gpointer                data;
  GDestroyNotify          data_destroy;
} GimpPaletteData;


/*  local function prototypes  */

static void             gimp_palette_data_free (GimpPaletteData      *data);

static GimpValueArray * gimp_temp_palette_run   (GimpProcedure        *procedure,
                                                 const GimpValueArray *args,
                                                 gpointer              run_data);
static gboolean         gimp_temp_palette_idle  (GimpPaletteData      *data);


/*  public functions  */

const gchar *
gimp_palette_select_new (const gchar            *title,
                         const gchar            *palette_name,
                         GimpRunPaletteCallback  callback,
                         gpointer                data,
                         GDestroyNotify          data_destroy)
{
  GimpPlugIn      *plug_in = gimp_get_plug_in ();
  GimpProcedure   *procedure;
  gchar           *palette_callback;
  GimpPaletteData *palette_data;

  palette_callback = gimp_pdb_temp_procedure_name (gimp_get_pdb ());

  palette_data = g_slice_new0 (GimpPaletteData);

  palette_data->palette_callback = palette_callback;
  palette_data->callback         = callback;
  palette_data->data             = data;
  palette_data->data_destroy     = data_destroy;

  procedure = gimp_procedure_new (plug_in,
                                  palette_callback,
                                  GIMP_PDB_PROC_TYPE_TEMPORARY,
                                  gimp_temp_palette_run,
                                  palette_data,
                                  (GDestroyNotify)
                                  gimp_palette_data_free);

  GIMP_PROC_ARG_STRING (procedure, "palette-name",
                        "Palette name",
                        "The palette name",
                        NULL,
                        G_PARAM_READWRITE);

  GIMP_PROC_ARG_INT (procedure, "num-colors",
                     "Num colors",
                     "Number of colors",
                     0, G_MAXINT, 0,
                     G_PARAM_READWRITE);

  GIMP_PROC_ARG_BOOLEAN (procedure, "closing",
                         "Closing",
                         "If the dialog was closing",
                         FALSE,
                         G_PARAM_READWRITE);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  if (gimp_palettes_popup (palette_callback, title, palette_name))
    {
      /* Allow callbacks to be watched */
      gimp_plug_in_extension_enable (plug_in);

      return palette_callback;
    }

  gimp_plug_in_remove_temp_procedure (plug_in, palette_callback);

  return NULL;
}

void
gimp_palette_select_destroy (const gchar *palette_callback)
{
  GimpPlugIn *plug_in = gimp_get_plug_in ();

  g_return_if_fail (palette_callback != NULL);

  gimp_plug_in_remove_temp_procedure (plug_in, palette_callback);
}


/*  private functions  */

static void
gimp_palette_data_free (GimpPaletteData *data)
{
  if (data->idle_id)
    g_source_remove (data->idle_id);

  if (data->palette_callback)
    {
      gimp_palettes_close_popup (data->palette_callback);
      g_free (data->palette_callback);
    }

  g_free (data->palette_name);

  if (data->data_destroy)
    data->data_destroy (data->data);

  g_slice_free (GimpPaletteData, data);
}

static GimpValueArray *
gimp_temp_palette_run (GimpProcedure        *procedure,
                       const GimpValueArray *args,
                       gpointer              run_data)
{
  GimpPaletteData *data = run_data;

  g_free (data->palette_name);

  data->palette_name = GIMP_VALUES_DUP_STRING  (args, 0);
  data->num_colors   = GIMP_VALUES_GET_INT     (args, 1);
  data->closing      = GIMP_VALUES_GET_BOOLEAN (args, 2);

  if (! data->idle_id)
    data->idle_id = g_idle_add ((GSourceFunc) gimp_temp_palette_idle,
                                data);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
gimp_temp_palette_idle (GimpPaletteData *data)
{
  data->idle_id = 0;

  if (data->callback)
    data->callback (data->palette_name,
                    data->closing,
                    data->data);

  if (data->closing)
    {
      gchar *palette_callback = data->palette_callback;

      data->palette_callback = NULL;
      gimp_palette_select_destroy (palette_callback);
      g_free (palette_callback);
    }

  return G_SOURCE_REMOVE;
}
