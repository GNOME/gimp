/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppalettemenu.c
 * Copyright (C) 2004  Michael Natterer <mitch@gimp.org>
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


#define PALETTE_SELECT_DATA_KEY  "gimp-palette-selct-data"


typedef struct _PaletteSelect PaletteSelect;

struct _PaletteSelect
{
  gchar                  *title;
  GimpRunPaletteCallback  callback;
  gpointer                data;

  GtkWidget              *button;
  GtkWidget              *label;

  gchar                  *palette_name;      /* Local copy */

  const gchar            *temp_palette_callback;
};


/*  local function prototypes  */

static void   gimp_palette_select_widget_callback (const gchar   *name,
                                                   gboolean       closing,
                                                   gpointer       data);
static void   gimp_palette_select_widget_clicked  (GtkWidget     *widget,
                                                   PaletteSelect *palette_sel);
static void   gimp_palette_select_widget_destroy  (GtkWidget     *widget,
                                                   PaletteSelect *palette_sel);


/**
 * gimp_palette_select_widget_new:
 * @title:        Title of the dialog to use or %NULL means to use the default
 *                title.
 * @palette_name: Initial palette name.
 * @callback:     A function to call when the selected palette changes.
 * @data:         A pointer to arbitary data to be used in the call to @callback.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a palette.  This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: GIMP 2.2
 */
GtkWidget *
gimp_palette_select_widget_new (const gchar            *title,
                                const gchar            *palette_name,
                                GimpRunPaletteCallback  callback,
                                gpointer                data)
{
  PaletteSelect *palette_sel;
  GtkWidget     *hbox;
  GtkWidget     *image;

  g_return_val_if_fail (callback != NULL, NULL);

  if (! title)
    title = _("Palette Selection");

  palette_sel = g_new0 (PaletteSelect, 1);

  palette_sel->title    = g_strdup (title);
  palette_sel->callback = callback;
  palette_sel->data     = data;

  palette_sel->palette_name = g_strdup (palette_name);

  palette_sel->button = gtk_button_new ();

  g_signal_connect (palette_sel->button, "clicked",
                    G_CALLBACK (gimp_palette_select_widget_clicked),
                    palette_sel);
  g_signal_connect (palette_sel->button, "destroy",
                    G_CALLBACK (gimp_palette_select_widget_destroy),
                    palette_sel);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (palette_sel->button), hbox);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GIMP_STOCK_PALETTE, GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  palette_sel->label = gtk_label_new (palette_name);
  gtk_box_pack_start (GTK_BOX (hbox), palette_sel->label, TRUE, TRUE, 4);
  gtk_widget_show (palette_sel->label);

  g_object_set_data (G_OBJECT (palette_sel->button),
                     PALETTE_SELECT_DATA_KEY, palette_sel);

  return palette_sel->button;
}

/**
 * gimp_palette_select_widget_close:
 * @widget: A palette select widget.
 *
 * Closes the popup window associated with @widget.
 *
 * Since: GIMP 2.2
 */
void
gimp_palette_select_widget_close (GtkWidget *widget)
{
  PaletteSelect *palette_sel;

  palette_sel = g_object_get_data (G_OBJECT (widget), PALETTE_SELECT_DATA_KEY);

  g_return_if_fail (palette_sel != NULL);

  if (palette_sel->temp_palette_callback)
    {
      gimp_palette_select_destroy (palette_sel->temp_palette_callback);
      palette_sel->temp_palette_callback = NULL;
    }
}

/**
 * gimp_palette_select_widget_set;
 * @widget:       A palette select widget.
 * @palette_name: Palette name to set; %NULL means no change.
 *
 * Sets the current palette for the palette select widget.  Calls the
 * callback function if one was supplied in the call to
 * gimp_palette_select_widget_new().
 *
 * Since: GIMP 2.2
 */
void
gimp_palette_select_widget_set (GtkWidget   *widget,
                                const gchar *palette_name)
{
  PaletteSelect *palette_sel;

  palette_sel = g_object_get_data (G_OBJECT (widget), PALETTE_SELECT_DATA_KEY);

  g_return_if_fail (palette_sel != NULL);

  if (palette_sel->temp_palette_callback)
    gimp_palettes_set_popup (palette_sel->temp_palette_callback, palette_name);
  else
    gimp_palette_select_widget_callback (palette_name, FALSE, palette_sel);
}


/*  private functions  */

static void
gimp_palette_select_widget_callback (const gchar *name,
                                     gboolean     closing,
                                     gpointer     data)
{
  PaletteSelect *palette_sel = (PaletteSelect *) data;

  g_free (palette_sel->palette_name);
  palette_sel->palette_name = g_strdup (name);

  gtk_label_set_text (GTK_LABEL (palette_sel->label), name);

  if (palette_sel->callback)
    palette_sel->callback (name, closing, palette_sel->data);

  if (closing)
    palette_sel->temp_palette_callback = NULL;
}

static void
gimp_palette_select_widget_clicked (GtkWidget     *widget,
                                    PaletteSelect *palette_sel)
{
  if (palette_sel->temp_palette_callback)
    {
      /*  calling gimp_palettes_set_popup() raises the dialog  */
      gimp_palettes_set_popup (palette_sel->temp_palette_callback,
                               palette_sel->palette_name);
    }
  else
    {
      palette_sel->temp_palette_callback =
        gimp_palette_select_new (palette_sel->title,
                                 palette_sel->palette_name,
                                 gimp_palette_select_widget_callback,
                                 palette_sel);
    }
}

static void
gimp_palette_select_widget_destroy (GtkWidget     *widget,
                                    PaletteSelect *palette_sel)
{
  if (palette_sel->temp_palette_callback)
    {
      gimp_palette_select_destroy (palette_sel->temp_palette_callback);
      palette_sel->temp_palette_callback = NULL;
    }

  g_free (palette_sel->title);
  g_free (palette_sel->palette_name);
  g_free (palette_sel);
}
