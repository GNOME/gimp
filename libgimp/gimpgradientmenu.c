/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpgradientmenu.c
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
#include "gimpgradientmenu.h"
#include "gimpgradientselectbutton.h"


/**
 * SECTION: gimpgradientmenu
 * @title: gimpgradientmenu
 * @short_description: A widget for selecting gradients.
 *
 * A widget for selecting gradients.
 **/


typedef struct
{
  GimpRunGradientCallback callback;
  gpointer                data;
} CompatCallbackData;


static void compat_callback           (GimpGradientSelectButton *gradient_button,
                                       const gchar              *gradient_name,
                                       gint                      width,
                                       const gdouble            *grad_data,
                                       gboolean                  dialog_closing,
                                       CompatCallbackData       *data);
static void compat_callback_data_free (CompatCallbackData       *data);


/**
 * gimp_gradient_select_widget_new:
 * @title:         Title of the dialog to use or %NULL to use the default title.
 * @gradient_name: Initial gradient name.
 * @callback:      A function to call when the selected gradient changes.
 * @data:          A pointer to arbitrary data to be used in the call to @callback.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a gradient.  This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.2
 */
GtkWidget *
gimp_gradient_select_widget_new (const gchar             *title,
                                 const gchar             *gradient_name,
                                 GimpRunGradientCallback  callback,
                                 gpointer                 data)
{
  GtkWidget          *gradient_button;
  CompatCallbackData *compat_data;

  g_return_val_if_fail (callback != NULL, NULL);

  gradient_button = gimp_gradient_select_button_new (title, gradient_name);

  compat_data = g_slice_new (CompatCallbackData);

  compat_data->callback = callback;
  compat_data->data     = data;

  g_signal_connect_data (gradient_button, "gradient-set",
                         G_CALLBACK (compat_callback),
                         compat_data,
                         (GClosureNotify) compat_callback_data_free, 0);

  return gradient_button;
}

/**
 * gimp_gradient_select_widget_close:
 * @widget: A gradient select widget.
 *
 * Closes the popup window associated with @widget.
 */
void
gimp_gradient_select_widget_close (GtkWidget *widget)
{
  g_return_if_fail (widget != NULL);

  gimp_select_button_close_popup (GIMP_SELECT_BUTTON (widget));
}

/**
 * gimp_gradient_select_widget_set:
 * @widget:        A gradient select widget.
 * @gradient_name: Gradient name to set.
 *
 * Sets the current gradient for the gradient select widget.  Calls the
 * callback function if one was supplied in the call to
 * gimp_gradient_select_widget_new().
 */
void
gimp_gradient_select_widget_set (GtkWidget   *widget,
                                 const gchar *gradient_name)
{
  g_return_if_fail (widget != NULL);

  gimp_gradient_select_button_set_gradient (GIMP_GRADIENT_SELECT_BUTTON (widget),
                                            gradient_name);
}


static void
compat_callback (GimpGradientSelectButton *gradient_button,
                 const gchar              *gradient_name,
                 gint                      width,
                 const gdouble            *grad_data,
                 gboolean                  dialog_closing,
                 CompatCallbackData       *data)
{
  data->callback (gradient_name, width, grad_data, dialog_closing, data->data);
}

static void
compat_callback_data_free (CompatCallbackData *data)
{
  g_slice_free (CompatCallbackData, data);
}
