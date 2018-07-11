/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbrushmenu.c
 * Copyright (C) 1998 Andy Thomas
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

#include <gtk/gtk.h>

#include "gimp.h"

#include "gimpuitypes.h"
#include "gimpbrushmenu.h"
#include "gimpbrushselectbutton.h"


/**
 * SECTION: gimpbrushmenu
 * @title: gimpbrushmenu
 * @short_description: A widget for selecting brushes.
 *
 * A widget for selecting brushes.
 **/


typedef struct
{
  GimpRunBrushCallback callback;
  gpointer             data;
} CompatCallbackData;


static void compat_callback           (GimpBrushSelectButton *brush_button,
                                       const gchar           *brush_name,
                                       gdouble                opacity,
                                       gint                   spacing,
                                       GimpLayerMode          paint_mode,
                                       gint                   width,
                                       gint                   height,
                                       const guchar          *mask_data,
                                       gboolean               dialog_closing,
                                       CompatCallbackData    *data);
static void compat_callback_data_free (CompatCallbackData    *data);


/**
 * gimp_brush_select_widget_new:
 * @title:      Title of the dialog to use or %NULL to use the default title.
 * @brush_name: Initial brush name or %NULL to use current selection.
 * @opacity:    Initial opacity. -1 means to use current opacity.
 * @spacing:    Initial spacing. -1 means to use current spacing.
 * @paint_mode: Initial paint mode.  -1 means to use current paint mode.
 * @callback:   A function to call when the selected brush changes.
 * @data:       A pointer to arbitrary data to be used in the call to @callback.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a #GimpBrush. This widget is suitable for placement in a table in
 * a plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 */
GtkWidget *
gimp_brush_select_widget_new (const gchar          *title,
                              const gchar          *brush_name,
                              gdouble               opacity,
                              gint                  spacing,
                              GimpLayerMode         paint_mode,
                              GimpRunBrushCallback  callback,
                              gpointer              data)
{
  GtkWidget          *brush_button;
  CompatCallbackData *compat_data;

  g_return_val_if_fail (callback != NULL, NULL);

  brush_button = gimp_brush_select_button_new (title, brush_name,
                                               opacity, spacing, paint_mode);

  compat_data = g_slice_new (CompatCallbackData);

  compat_data->callback = callback;
  compat_data->data     = data;

  g_signal_connect_data (brush_button, "brush-set",
                         G_CALLBACK (compat_callback),
                         compat_data,
                         (GClosureNotify) compat_callback_data_free, 0);

  return brush_button;
}

/**
 * gimp_brush_select_widget_close:
 * @widget: A brush select widget.
 *
 * Closes the popup window associated with @widget.
 */
void
gimp_brush_select_widget_close (GtkWidget *widget)
{
  g_return_if_fail (widget != NULL);

  gimp_select_button_close_popup (GIMP_SELECT_BUTTON (widget));
}

/**
 * gimp_brush_select_widget_set:
 * @widget:     A brush select widget.
 * @brush_name: Brush name to set; %NULL means no change.
 * @opacity:    Opacity to set. -1.0 means no change.
 * @spacing:    Spacing to set. -1 means no change.
 * @paint_mode: Paint mode to set.  -1 means no change.
 *
 * Sets the current brush and other values for the brush select
 * widget.  Calls the callback function if one was supplied in the
 * call to gimp_brush_select_widget_new().
 */
void
gimp_brush_select_widget_set (GtkWidget     *widget,
                              const gchar   *brush_name,
                              gdouble        opacity,
                              gint           spacing,
                              GimpLayerMode  paint_mode)
{
  g_return_if_fail (widget != NULL);

  gimp_brush_select_button_set_brush (GIMP_BRUSH_SELECT_BUTTON (widget),
                                      brush_name, opacity, spacing, paint_mode);
}


static void
compat_callback (GimpBrushSelectButton *brush_button,
                 const gchar           *brush_name,
                 gdouble                opacity,
                 gint                   spacing,
                 GimpLayerMode          paint_mode,
                 gint                   width,
                 gint                   height,
                 const guchar          *mask_data,
                 gboolean               dialog_closing,
                 CompatCallbackData    *data)
{
  data->callback (brush_name, opacity, spacing, paint_mode,
                  width, height, mask_data, dialog_closing, data->data);
}

static void
compat_callback_data_free (CompatCallbackData *data)
{
  g_slice_free (CompatCallbackData, data);
}
