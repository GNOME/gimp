/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbrushselect.c
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
  gchar                *brush_callback;
  guint                 idle_id;
  gchar                *brush_name;
  gdouble               opacity;
  gint                  spacing;
  gint                  paint_mode;
  gint                  width;
  gint                  height;
  guchar               *brush_mask_data;
  GimpRunBrushCallback  callback;
  gboolean              closing;
  gpointer              data;
  GDestroyNotify        data_destroy;
} GimpBrushData;


/*  local function prototypes  */

static void             gimp_brush_data_free (GimpBrushData        *data);

static GimpValueArray * gimp_temp_brush_run  (GimpProcedure        *procedure,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static gboolean         gimp_temp_brush_idle (GimpBrushData        *data);


/*  public functions  */

const gchar *
gimp_brush_select_new (const gchar          *title,
                       const gchar          *brush_name,
                       gdouble               opacity,
                       gint                  spacing,
                       GimpLayerMode         paint_mode,
                       GimpRunBrushCallback  callback,
                       gpointer              data,
                       GDestroyNotify        data_destroy)
{
  GimpPlugIn    *plug_in = gimp_get_plug_in ();
  GimpProcedure *procedure;
  gchar         *brush_callback;
  GimpBrushData *brush_data;

  brush_callback = gimp_pdb_temp_procedure_name (gimp_get_pdb ());

  brush_data = g_slice_new0 (GimpBrushData);

  brush_data->brush_callback = brush_callback;
  brush_data->callback       = callback;
  brush_data->data           = data;
  brush_data->data_destroy   = data_destroy;

  procedure = gimp_procedure_new (plug_in,
                                  brush_callback,
                                  GIMP_PDB_PROC_TYPE_TEMPORARY,
                                  gimp_temp_brush_run,
                                  brush_data,
                                  (GDestroyNotify)
                                  gimp_brush_data_free);

  GIMP_PROC_ARG_STRING (procedure, "brush-name",
                        "Brush name",
                        "The brush name",
                        NULL,
                        G_PARAM_READWRITE);

  GIMP_PROC_ARG_DOUBLE (procedure, "opacity",
                        "Opacity",
                        NULL,
                        0.0, 100.0, 100.0,
                        G_PARAM_READWRITE);

  GIMP_PROC_ARG_INT (procedure, "spacing",
                     "Spacing",
                     NULL,
                     -1, 1000, 20,
                     G_PARAM_READWRITE);

  GIMP_PROC_ARG_ENUM (procedure, "paint-mode",
                      "Paint mode",
                      NULL,
                      GIMP_TYPE_LAYER_MODE,
                      GIMP_LAYER_MODE_NORMAL,
                      G_PARAM_READWRITE);

  GIMP_PROC_ARG_INT (procedure, "mask-width",
                     "Brush width",
                     NULL,
                     0, 10000, 0,
                     G_PARAM_READWRITE);

  GIMP_PROC_ARG_INT (procedure, "mask-height",
                     "Brush height",
                     NULL,
                     0, 10000, 0,
                     G_PARAM_READWRITE);

  GIMP_PROC_ARG_INT (procedure, "mask-len",
                     "Mask length",
                     "Length of brush mask data",
                     0, G_MAXINT, 0,
                     G_PARAM_READWRITE);

  GIMP_PROC_ARG_UINT8_ARRAY (procedure, "mask-data",
                             "Mask data",
                             "The brush mask data",
                             G_PARAM_READWRITE);

  GIMP_PROC_ARG_BOOLEAN (procedure, "closing",
                         "Closing",
                         "If the dialog was closing",
                         FALSE,
                         G_PARAM_READWRITE);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  if (gimp_brushes_popup (brush_callback, title, brush_name,
                          opacity, spacing, paint_mode))
    {
      /* Allow callbacks to be watched */
      gimp_plug_in_extension_enable (plug_in);

      return brush_callback;
    }

  gimp_plug_in_remove_temp_procedure (plug_in, brush_callback);

  return NULL;
}

void
gimp_brush_select_destroy (const gchar *brush_callback)
{
  GimpPlugIn *plug_in = gimp_get_plug_in ();

  g_return_if_fail (brush_callback != NULL);

  gimp_plug_in_remove_temp_procedure (plug_in, brush_callback);
}


/*  private functions  */

static void
gimp_brush_data_free (GimpBrushData *data)
{
  if (data->idle_id)
    g_source_remove (data->idle_id);

  if (data->brush_callback)
    {
      gimp_brushes_close_popup (data->brush_callback);
      g_free (data->brush_callback);
    }

  g_free (data->brush_name);
  g_free (data->brush_mask_data);

  if (data->data_destroy)
    data->data_destroy (data->data);

  g_slice_free (GimpBrushData, data);
}

static GimpValueArray *
gimp_temp_brush_run (GimpProcedure        *procedure,
                     const GimpValueArray *args,
                     gpointer              run_data)
{
  GimpBrushData *data = run_data;

  g_free (data->brush_name);
  g_free (data->brush_mask_data);

  data->brush_name      = GIMP_VALUES_DUP_STRING      (args, 0);
  data->opacity         = GIMP_VALUES_GET_DOUBLE      (args, 1);
  data->spacing         = GIMP_VALUES_GET_INT         (args, 2);
  data->paint_mode      = GIMP_VALUES_GET_ENUM        (args, 3);
  data->width           = GIMP_VALUES_GET_INT         (args, 4);
  data->height          = GIMP_VALUES_GET_INT         (args, 5);
  data->brush_mask_data = GIMP_VALUES_DUP_UINT8_ARRAY (args, 7);
  data->closing         = GIMP_VALUES_GET_BOOLEAN     (args, 8);

  if (! data->idle_id)
    data->idle_id = g_idle_add ((GSourceFunc) gimp_temp_brush_idle, data);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
gimp_temp_brush_idle (GimpBrushData *data)
{
  data->idle_id = 0;

  if (data->callback)
    data->callback (data->brush_name,
                    data->opacity,
                    data->spacing,
                    data->paint_mode,
                    data->width,
                    data->height,
                    data->brush_mask_data,
                    data->closing,
                    data->data);

  if (data->closing)
    {
      gchar *brush_callback = data->brush_callback;

      data->brush_callback = NULL;
      gimp_brush_select_destroy (brush_callback);
      g_free (brush_callback);
    }

  return G_SOURCE_REMOVE;
}
