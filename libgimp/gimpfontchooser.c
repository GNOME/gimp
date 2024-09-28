/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpfontchooser.c
 * Copyright (C) 2003  Sven Neumann  <sven@gimp.org>
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
#include "gimpfontchooser.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimpfontchooser
 * @title: GimpFontChooser
 * @short_description: A button which pops up a font selection dialog.
 *
 * A button which pops up a font selection dialog.
 **/

struct _GimpFontChooser
{
  GimpResourceChooser  parent_instance;

  GtkWidget           *label;
};


static void gimp_font_chooser_draw_interior (GimpResourceChooser *self);


static const GtkTargetEntry drag_target = { "application/x-gimp-font-name", 0, 0 };


G_DEFINE_FINAL_TYPE (GimpFontChooser, gimp_font_chooser, GIMP_TYPE_RESOURCE_CHOOSER)


static void
gimp_font_chooser_class_init (GimpFontChooserClass *klass)
{
  GimpResourceChooserClass *superclass = GIMP_RESOURCE_CHOOSER_CLASS (klass);

  superclass->draw_interior = gimp_font_chooser_draw_interior;
  superclass->resource_type = GIMP_TYPE_FONT;
}

static void
gimp_font_chooser_init (GimpFontChooser *self)
{
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *image;

  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (self), button);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_add (GTK_CONTAINER (button), hbox);

  image = gtk_image_new_from_icon_name (GIMP_ICON_FONT,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  self->label = gtk_label_new ("unknown");
  gtk_box_pack_start (GTK_BOX (hbox), self->label, TRUE, TRUE, 4);

  gtk_widget_show_all (GTK_WIDGET (self));

  _gimp_resource_chooser_set_drag_target (GIMP_RESOURCE_CHOOSER (self),
                                         hbox, &drag_target);

  _gimp_resource_chooser_set_clickable (GIMP_RESOURCE_CHOOSER (self), button);
}

static void
gimp_font_chooser_draw_interior (GimpResourceChooser *self)
{
  GimpFontChooser *font_select= GIMP_FONT_CHOOSER (self);
  GimpResource    *resource;
  gchar           *name = NULL;

  resource = gimp_resource_chooser_get_resource (self);

  if (resource)
    name = gimp_resource_get_name (resource);

  gtk_label_set_text (GTK_LABEL (font_select->label), name);
}


/**
 * gimp_font_chooser_new:
 * @title: (nullable): Title of the dialog to use or %NULL to use the default title.
 * @label: (nullable): Button label or %NULL for no label.
 * @font:  (nullable): Initial font.
 *
 * Creates a new #GtkWidget that lets a user choose a font.
 * You can put this widget in a plug-in dialog.
 *
 * When font is NULL, initial choice is from context.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_font_chooser_new (const gchar  *title,
                       const gchar  *label,
                       GimpFont     *font)
{
  GtkWidget *chooser;

  g_return_val_if_fail (font == NULL || GIMP_IS_FONT (font), NULL);

  if (font == NULL)
    font = gimp_context_get_font ();

   if (title)
     chooser = g_object_new (GIMP_TYPE_FONT_CHOOSER,
                             "title",     title,
                             "label",     label,
                             "resource",  font,
                             NULL);
   else
     chooser = g_object_new (GIMP_TYPE_FONT_CHOOSER,
                             "label",     label,
                             "resource",  font,
                             NULL);

  gimp_font_chooser_draw_interior (GIMP_RESOURCE_CHOOSER (chooser));

  return chooser;
}
