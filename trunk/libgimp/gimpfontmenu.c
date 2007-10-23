/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpfontmenu.c
 * Copyright (C) 2003  Sven Neumann  <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "gimp.h"

#include "gimpuitypes.h"
#include "gimpfontselectbutton.h"

#undef GIMP_DISABLE_DEPRECATED
#include "gimpfontmenu.h"


typedef struct
{
  GimpRunFontCallback callback;
  gpointer            data;
} CompatCallbackData;


static void compat_callback           (GimpFontSelectButton *font_button,
                                       const gchar          *font_name,
                                       gboolean              dialog_closing,
                                       CompatCallbackData   *data);
static void compat_callback_data_free (CompatCallbackData   *data);


/**
 * gimp_font_select_widget_new:
 * @title:     Title of the dialog to use or %NULL to use the default title.
 * @font_name: Initial font name.
 * @callback:  A function to call when the selected font changes.
 * @data:      A pointer to arbitary data to be used in the call to @callback.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a font.  This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 */
GtkWidget *
gimp_font_select_widget_new (const gchar         *title,
                             const gchar         *font_name,
                             GimpRunFontCallback  callback,
                             gpointer             data)
{
  GtkWidget          *font_button;
  CompatCallbackData *compat_data;

  g_return_val_if_fail (callback != NULL, NULL);

  font_button = gimp_font_select_button_new (title, font_name);

  compat_data = g_slice_new (CompatCallbackData);

  compat_data->callback = callback;
  compat_data->data     = data;

  g_signal_connect_data (font_button, "font-set",
                         G_CALLBACK (compat_callback),
                         compat_data,
                         (GClosureNotify) compat_callback_data_free, 0);

  return font_button;
}

/**
 * gimp_font_select_widget_close:
 * @widget: A font select widget.
 *
 * Closes the popup window associated with @widget.
 */
void
gimp_font_select_widget_close (GtkWidget *widget)
{
  g_return_if_fail (widget != NULL);

  gimp_select_button_close_popup (GIMP_SELECT_BUTTON (widget));
}

/**
 * gimp_font_select_widget_set;
 * @widget:    A font select widget.
 * @font_name: Font name to set; %NULL means no change.
 *
 * Sets the current font for the font select widget.  Calls the
 * callback function if one was supplied in the call to
 * gimp_font_select_widget_new().
 */
void
gimp_font_select_widget_set (GtkWidget   *widget,
                             const gchar *font_name)
{
  g_return_if_fail (widget != NULL);

  gimp_font_select_button_set_font (GIMP_FONT_SELECT_BUTTON (widget),
                                    font_name);
}


static void
compat_callback (GimpFontSelectButton *font_button,
                 const gchar          *font_name,
                 gboolean              dialog_closing,
                 CompatCallbackData   *data)
{
  data->callback (font_name, dialog_closing, data->data);
}

static void
compat_callback_data_free (CompatCallbackData *data)
{
  g_slice_free (CompatCallbackData, data);
}
