/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppalettemenu.c
 * Copyright (C) 2004  Michael Natterer <mitch@gimp.org>
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
#include "gimppalettemenu.h"
#include "gimppaletteselectbutton.h"


/**
 * SECTION: gimppalettemenu
 * @title: gimppalettemenu
 * @short_description: A widget for selecting palettes.
 *
 * A widget for selecting palettes.
 **/


typedef struct
{
  GimpRunPaletteCallback callback;
  gpointer               data;
} CompatCallbackData;


static void compat_callback           (GimpPaletteSelectButton *palette_button,
                                       const gchar             *palette_name,
                                       gboolean                 dialog_closing,
                                       CompatCallbackData      *data);
static void compat_callback_data_free (CompatCallbackData      *data);


/**
 * gimp_palette_select_widget_new:
 * @title:        Title of the dialog to use or %NULL to use the default title.
 * @palette_name: Initial palette name.
 * @callback:     A function to call when the selected palette changes.
 * @data:         A pointer to arbitrary data to be used in the call to @callback.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a palette.  This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.2
 */
GtkWidget *
gimp_palette_select_widget_new (const gchar            *title,
                                const gchar            *palette_name,
                                GimpRunPaletteCallback  callback,
                                gpointer                data)
{
  GtkWidget          *palette_button;
  CompatCallbackData *compat_data;

  g_return_val_if_fail (callback != NULL, NULL);

  palette_button = gimp_palette_select_button_new (title, palette_name);

  compat_data = g_slice_new (CompatCallbackData);

  compat_data->callback = callback;
  compat_data->data     = data;

  g_signal_connect_data (palette_button, "palette-set",
                         G_CALLBACK (compat_callback),
                         compat_data,
                         (GClosureNotify) compat_callback_data_free, 0);

  return palette_button;
}

/**
 * gimp_palette_select_widget_close:
 * @widget: A palette select widget.
 *
 * Closes the popup window associated with @widget.
 *
 * Since: 2.2
 */
void
gimp_palette_select_widget_close (GtkWidget *widget)
{
  g_return_if_fail (widget != NULL);

  gimp_select_button_close_popup (GIMP_SELECT_BUTTON (widget));
}

/**
 * gimp_palette_select_widget_set:
 * @widget:       A palette select widget.
 * @palette_name: Palette name to set; %NULL means no change.
 *
 * Sets the current palette for the palette select widget.  Calls the
 * callback function if one was supplied in the call to
 * gimp_palette_select_widget_new().
 *
 * Since: 2.2
 */
void
gimp_palette_select_widget_set (GtkWidget   *widget,
                                const gchar *palette_name)
{
  g_return_if_fail (widget != NULL);

  gimp_palette_select_button_set_palette (GIMP_PALETTE_SELECT_BUTTON (widget),
                                          palette_name);
}


static void
compat_callback (GimpPaletteSelectButton *palette_button,
                 const gchar             *palette_name,
                 gboolean                 dialog_closing,
                 CompatCallbackData      *data)
{
  data->callback (palette_name, dialog_closing, data->data);
}

static void
compat_callback_data_free (CompatCallbackData *data)
{
  g_slice_free (CompatCallbackData, data);
}
