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
#include "gimpui.h"

#include "libgimp-intl.h"


#define FONT_SELECT_DATA_KEY  "gimp-font-selct-data"


typedef struct _FontSelect FontSelect;

struct _FontSelect
{
  gchar               *title;
  GimpRunFontCallback  callback;
  gpointer             data;

  GtkWidget           *button;
  GtkWidget           *label;

  gchar               *font_name;      /* Local copy */

  const gchar         *temp_font_callback;
};


/*  local function prototypes  */

static void   gimp_font_select_widget_callback (const gchar *name,
                                                gboolean     closing,
                                                gpointer     data);
static void   gimp_font_select_widget_clicked  (GtkWidget   *widget,
                                                FontSelect  *font_sel);
static void   gimp_font_select_widget_destroy  (GtkWidget   *widget,
                                                FontSelect  *font_sel);


/**
 * gimp_font_select_widget_new:
 * @title:     Title of the dialog to use or %NULL means to use the default
 *             title.
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
  FontSelect *font_sel;
  GtkWidget  *hbox;
  GtkWidget  *image;

  g_return_val_if_fail (callback != NULL, NULL);

  if (! title)
    title = _("Font Selection");

  font_sel = g_new0 (FontSelect, 1);

  font_sel->title    = g_strdup (title);
  font_sel->callback = callback;
  font_sel->data     = data;

  font_sel->font_name = g_strdup (font_name);

  font_sel->button = gtk_button_new ();

  g_signal_connect (font_sel->button, "clicked",
                    G_CALLBACK (gimp_font_select_widget_clicked),
                    font_sel);
  g_signal_connect (font_sel->button, "destroy",
                    G_CALLBACK (gimp_font_select_widget_destroy),
                    font_sel);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (font_sel->button), hbox);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GIMP_STOCK_FONT, GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  font_sel->label = gtk_label_new (font_name);
  gtk_box_pack_start (GTK_BOX (hbox), font_sel->label, TRUE, TRUE, 4);
  gtk_widget_show (font_sel->label);

  g_object_set_data (G_OBJECT (font_sel->button),
                     FONT_SELECT_DATA_KEY, font_sel);

  return font_sel->button;
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
  FontSelect *font_sel;

  font_sel = g_object_get_data (G_OBJECT (widget), FONT_SELECT_DATA_KEY);

  g_return_if_fail (font_sel != NULL);

  if (font_sel->temp_font_callback)
    {
      gimp_font_select_destroy (font_sel->temp_font_callback);
      font_sel->temp_font_callback = NULL;
    }
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
  FontSelect *font_sel;

  font_sel = g_object_get_data (G_OBJECT (widget), FONT_SELECT_DATA_KEY);

  g_return_if_fail (font_sel != NULL);

  if (font_sel->temp_font_callback)
    gimp_fonts_set_popup (font_sel->temp_font_callback, font_name);
  else
    gimp_font_select_widget_callback (font_name, FALSE, font_sel);
}


/*  private functions  */

static void
gimp_font_select_widget_callback (const gchar *name,
                                  gboolean     closing,
                                  gpointer     data)
{
  FontSelect *font_sel = (FontSelect *) data;

  g_free (font_sel->font_name);
  font_sel->font_name = g_strdup (name);

  gtk_label_set_text (GTK_LABEL (font_sel->label), name);

  if (font_sel->callback)
    font_sel->callback (name, closing, font_sel->data);

  if (closing)
    font_sel->temp_font_callback = NULL;
}

static void
gimp_font_select_widget_clicked (GtkWidget  *widget,
                                 FontSelect *font_sel)
{
  if (font_sel->temp_font_callback)
    {
      /*  calling gimp_fonts_set_popup() raises the dialog  */
      gimp_fonts_set_popup (font_sel->temp_font_callback, font_sel->font_name);
    }
  else
    {
      font_sel->temp_font_callback =
        gimp_font_select_new (font_sel->title,
                              font_sel->font_name,
                              gimp_font_select_widget_callback,
                              font_sel);
    }
}

static void
gimp_font_select_widget_destroy (GtkWidget  *widget,
                                 FontSelect *font_sel)
{
  if (font_sel->temp_font_callback)
    {
      gimp_font_select_destroy (font_sel->temp_font_callback);
      font_sel->temp_font_callback = NULL;
    }

  g_free (font_sel->title);
  g_free (font_sel->font_name);
  g_free (font_sel);
}
