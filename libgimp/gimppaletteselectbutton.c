/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppaletteselectbutton.c
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"

#include "gimpuitypes.h"
#include "gimppaletteselectbutton.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimppaletteselectbutton
 * @title: GimpPaletteSelectButton
 * @short_description: A button which pops up a palette selection dialog.
 *
 * A button which pops up a palette selection dialog.
 **/

struct _GimpPaletteSelectButton
{
  GimpResourceSelectButton  parent_instance;

  GtkWidget                *label;
  GtkWidget                *button;
};


static void gimp_palette_select_button_draw_interior (GimpResourceSelectButton *self);


static const GtkTargetEntry drag_target = { "application/x-gimp-palette-name", 0, 0 };


G_DEFINE_FINAL_TYPE (GimpPaletteSelectButton,
                     gimp_palette_select_button,
                     GIMP_TYPE_RESOURCE_SELECT_BUTTON)


static void
gimp_palette_select_button_class_init (GimpPaletteSelectButtonClass *klass)
{
  GimpResourceSelectButtonClass *superclass = GIMP_RESOURCE_SELECT_BUTTON_CLASS (klass);

  superclass->draw_interior = gimp_palette_select_button_draw_interior;
  superclass->resource_type = GIMP_TYPE_PALETTE;
}

static void
gimp_palette_select_button_init (GimpPaletteSelectButton *self)
{
  GtkWidget *hbox;
  GtkWidget *image;

  self->button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (self), self->button);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_add (GTK_CONTAINER (self->button), hbox);

  image = gtk_image_new_from_icon_name (GIMP_ICON_PALETTE,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  self->label = gtk_label_new ("unknown");
  gtk_box_pack_start (GTK_BOX (hbox), self->label, TRUE, TRUE, 4);

  gtk_widget_show_all (GTK_WIDGET (self));

  gimp_resource_select_button_set_drag_target (GIMP_RESOURCE_SELECT_BUTTON (self),
                                               hbox,
                                               &drag_target);
  gimp_resource_select_button_set_clickable (GIMP_RESOURCE_SELECT_BUTTON (self),
                                             self->button);
}

static void
gimp_palette_select_button_draw_interior (GimpResourceSelectButton *self)
{
  GimpPaletteSelectButton *palette_select= GIMP_PALETTE_SELECT_BUTTON (self);
  GimpResource            *resource;
  gchar                   *name = NULL;

  resource = gimp_resource_select_button_get_resource (self);

  if (resource)
    name = gimp_resource_get_name (resource);

  gtk_label_set_text (GTK_LABEL (palette_select->label), name);
}


/**
 * gimp_palette_select_button_new:
 * @title:    (nullable): Title of the dialog to use or %NULL to use the default title.
 * @label:    (nullable): Button label or %NULL for no label.
 * @resource: (nullable): Initial palette.
 *
 * Creates a new #GtkWidget that lets a user choose a palette.
 * You can put this widget in a table in a plug-in dialog.
 *
 * When palette is NULL, initial choice is from context.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_palette_select_button_new (const gchar  *title,
                                const gchar  *label,
                                GimpResource *resource)
{
  GtkWidget *self;

  if (resource == NULL)
    resource = GIMP_RESOURCE (gimp_context_get_palette ());

   if (title)
     self = g_object_new (GIMP_TYPE_PALETTE_SELECT_BUTTON,
                          "title",     title,
                          "label",     label,
                          "resource",  resource,
                          NULL);
   else
     self = g_object_new (GIMP_TYPE_PALETTE_SELECT_BUTTON,
                          "label",     label,
                          "resource",  resource,
                          NULL);

  gimp_palette_select_button_draw_interior (GIMP_RESOURCE_SELECT_BUTTON (self));

  return self;
}
