/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppalettechooser.c
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
#include "gimppalettechooser.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimppalettechooser
 * @title: GimpPaletteChooser
 * @short_description: A button which pops up a palette selection dialog.
 *
 * A button which pops up a palette selection dialog.
 **/

struct _GimpPaletteChooser
{
  GimpResourceChooser  parent_instance;

  GtkWidget           *label;
  GtkWidget           *button;
};


static void gimp_palette_chooser_draw_interior (GimpResourceChooser *self);


static const GtkTargetEntry drag_target = { "application/x-gimp-palette-name", 0, 0 };


G_DEFINE_FINAL_TYPE (GimpPaletteChooser, gimp_palette_chooser, GIMP_TYPE_RESOURCE_CHOOSER)


static void
gimp_palette_chooser_class_init (GimpPaletteChooserClass *klass)
{
  GimpResourceChooserClass *superclass = GIMP_RESOURCE_CHOOSER_CLASS (klass);

  superclass->draw_interior = gimp_palette_chooser_draw_interior;
  superclass->resource_type = GIMP_TYPE_PALETTE;
}

static void
gimp_palette_chooser_init (GimpPaletteChooser *self)
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

  gimp_resource_chooser_set_drag_target (GIMP_RESOURCE_CHOOSER (self),
                                         hbox, &drag_target);
  gimp_resource_chooser_set_clickable (GIMP_RESOURCE_CHOOSER (self), self->button);
}

static void
gimp_palette_chooser_draw_interior (GimpResourceChooser *self)
{
  GimpPaletteChooser *palette_select= GIMP_PALETTE_CHOOSER (self);
  GimpResource            *resource;
  gchar                   *name = NULL;

  resource = gimp_resource_chooser_get_resource (self);

  if (resource)
    name = gimp_resource_get_name (resource);

  gtk_label_set_text (GTK_LABEL (palette_select->label), name);
}

/**
 * gimp_palette_chooser_new:
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
gimp_palette_chooser_new (const gchar  *title,
                          const gchar  *label,
                          GimpResource *resource)
{
  GtkWidget *self;

  if (resource == NULL)
    resource = GIMP_RESOURCE (gimp_context_get_palette ());

   if (title)
     self = g_object_new (GIMP_TYPE_PALETTE_CHOOSER,
                          "title",     title,
                          "label",     label,
                          "resource",  resource,
                          NULL);
   else
     self = g_object_new (GIMP_TYPE_PALETTE_CHOOSER,
                          "label",     label,
                          "resource",  resource,
                          NULL);

  gimp_palette_chooser_draw_interior (GIMP_RESOURCE_CHOOSER (self));

  return self;
}
