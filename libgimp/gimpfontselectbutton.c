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


#define FSEL_DATA_KEY  "__fsel_data"

typedef struct
{
  gchar               *title;
  GimpRunFontCallback  callback;
  GtkWidget           *button;
  GtkWidget           *label;
  gchar               *font_name;      /* Local copy */
  gchar               *font_popup_pnt; /* Pointer use to control the popup */
  gpointer             data;
} FSelect;


static void
font_select_invoker (const gchar *name,
                     gboolean     closing,
                     gpointer     data)
{
  FSelect *fsel = (FSelect*) data;

  g_free (fsel->font_name);
  fsel->font_name = g_strdup (name);

  gtk_label_set_text (GTK_LABEL (fsel->label), name);

  if (fsel->callback)
    fsel->callback (name, closing, fsel->data);

  if (closing)
    fsel->font_popup_pnt = NULL;
}


static void
fonts_select_callback (GtkWidget *widget,
                       gpointer   data)
{
  FSelect *fsel = (FSelect*) data;

  if (fsel->font_popup_pnt)
    {
      /*  calling gimp_fonts_set_popup() raises the dialog  */
      gimp_fonts_set_popup (fsel->font_popup_pnt, fsel->font_name); 
    }
  else
    {
      fsel->font_popup_pnt = 
	gimp_interactive_selection_font (fsel->title ?
					 fsel->title : _("Font Selection"),
					 fsel->font_name,
					 font_select_invoker, fsel);
    }
}

/**
 * gimp_font_select_widget:
 * @title: Title of the dialog to use or %NULL means to use the default title.
 * @font_name: Initial font name. 
 * @callback: a function to call when the selected font changes.
 * @data: a pointer to arbitary data to be used in the call to @callback.
 *
 * Creates a new #GtkWidget that completely controls the selection of a 
 * font.  This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns:A #GtkWidget that you can use in your UI.
 */
GtkWidget * 
gimp_font_select_widget (const gchar         *title,
                         const gchar         *font_name, 
                         GimpRunFontCallback  callback,
                         gpointer             data)
{
  GtkWidget *hbox;
  GtkWidget *image;
  FSelect   *fsel;

  g_return_val_if_fail (font_name != NULL, NULL);

  fsel = g_new0 (FSelect, 1);
  
  fsel->callback  = callback;
  fsel->data      = data;
  fsel->font_name = g_strdup (font_name);
  fsel->title     = g_strdup (title);

  fsel->button = gtk_button_new ();
  
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (fsel->button), hbox);
  gtk_widget_show (hbox);
  
  fsel->label = gtk_label_new (font_name);
  gtk_box_pack_start (GTK_BOX (hbox), fsel->label, TRUE, TRUE, 4);
  gtk_widget_show (fsel->label);

  image = gtk_image_new_from_stock (GTK_STOCK_SELECT_FONT,
                                    GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_end (GTK_BOX (hbox), image, FALSE, FALSE, 4);
  gtk_widget_show (image);

  g_signal_connect (fsel->button, "clicked",
                    G_CALLBACK (fonts_select_callback),
                    fsel);

  g_object_set_data (G_OBJECT (fsel->button), FSEL_DATA_KEY, fsel);

  return fsel->button;
}

/**
 * gimp_font_select_widget_close_popup:
 * @widget: A font select widget.
 *
 * Closes the popup window associated with @widget.
 */
void
gimp_font_select_widget_close_popup (GtkWidget *widget)
{
  FSelect *fsel;

  fsel = (FSelect *) g_object_get_data (G_OBJECT (widget), FSEL_DATA_KEY);

  if (fsel && fsel->font_popup_pnt)
    {
      gimp_fonts_close_popup (fsel->font_popup_pnt);
      fsel->font_popup_pnt = NULL;
    }
}

/**
 * gimp_font_select_widget_set_popup:
 * @widget: A font select widget.
 * @font_name: Font name to set; %NULL means no change. 
 *
 * Sets the current font for the font
 * select widget.  Calls the callback function if one was
 * supplied in the call to gimp_font_select_widget().
 */
void
gimp_font_select_widget_set_popup (GtkWidget   *widget,
                                   const gchar *font_name)
{
  FSelect  *fsel;
  
  fsel = (FSelect*) g_object_get_data (G_OBJECT (widget), FSEL_DATA_KEY);

  if (fsel)
    {
      font_select_invoker (font_name, FALSE, fsel);
      
      if (fsel->font_popup_pnt)
	gimp_fonts_set_popup (fsel->font_popup_pnt, (gchar *) font_name);
    }
}

