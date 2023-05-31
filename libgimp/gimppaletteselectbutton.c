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
  GimpResourceSelectButton parent_instance;

  GtkWidget *palette_name_label;
  GtkWidget *drag_region_widget;
  GtkWidget *button;
};


static void gimp_palette_select_button_finalize        (GObject                  *object);
static void gimp_palette_select_button_draw_interior   (GimpResourceSelectButton *self);

static GtkWidget *gimp_palette_select_button_create_interior  (GimpPaletteSelectButton     *self);


static const GtkTargetEntry drag_target = { "application/x-gimp-palette-name", 0, 0 };


G_DEFINE_FINAL_TYPE (GimpPaletteSelectButton,
                     gimp_palette_select_button,
                     GIMP_TYPE_RESOURCE_SELECT_BUTTON)


static void
gimp_palette_select_button_class_init (GimpPaletteSelectButtonClass *klass)
{
  GObjectClass                  *object_class = G_OBJECT_CLASS (klass);
  GimpResourceSelectButtonClass *superclass   = GIMP_RESOURCE_SELECT_BUTTON_CLASS (klass);

  object_class->finalize     = gimp_palette_select_button_finalize;

  superclass->draw_interior = gimp_palette_select_button_draw_interior;
  superclass->resource_type = GIMP_TYPE_PALETTE;
}

static void
gimp_palette_select_button_init (GimpPaletteSelectButton *self)
{
  GtkWidget *interior;

  interior = gimp_palette_select_button_create_interior (self);

  gimp_resource_select_button_embed_interior (GIMP_RESOURCE_SELECT_BUTTON (self), interior);

  gimp_resource_select_button_set_drag_target (GIMP_RESOURCE_SELECT_BUTTON (self),
                                               self->drag_region_widget,
                                               &drag_target);
  gimp_resource_select_button_set_clickable (GIMP_RESOURCE_SELECT_BUTTON (self),
                                             self->button);
}

/**
 * gimp_palette_select_button_new:
 * @title: (nullable): Title of the dialog to use or %NULL to use the default title.
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
                                GimpResource *resource)
{
  GtkWidget *self;

  if (resource == NULL)
    resource = GIMP_RESOURCE (gimp_context_get_palette ());

   if (title)
     self = g_object_new (GIMP_TYPE_PALETTE_SELECT_BUTTON,
                          "title",     title,
                          "resource",  resource,
                          NULL);
   else
     self = g_object_new (GIMP_TYPE_PALETTE_SELECT_BUTTON,
                          "resource",  resource,
                          NULL);

  gimp_palette_select_button_draw_interior (GIMP_RESOURCE_SELECT_BUTTON (self));

  return self;
}


/* Getter and setter.
 * We could omit these, and use only the superclass methods.
 * But script-fu-interface.c uses these, until FUTURE.
 */

/**
 * gimp_palette_select_button_get_palette:
 * @self: A #GimpPaletteSelectButton
 *
 * Gets the currently selected palette.
 *
 * Returns: (transfer none): an internal copy of the palette which must not be freed.
 *
 * Since: 2.4
 */
GimpPalette *
gimp_palette_select_button_get_palette (GimpPaletteSelectButton *self)
{
  g_return_val_if_fail (GIMP_IS_PALETTE_SELECT_BUTTON (self), NULL);

  /* Delegate to super w upcast arg and downcast result. */
  return (GimpPalette *) gimp_resource_select_button_get_resource ((GimpResourceSelectButton*) self);
}

/**
 * gimp_palette_select_button_set_palette:
 * @self: A #GimpPaletteSelectButton
 * @palette: Palette to set.
 *
 * Sets the currently selected palette.
 * Usually you should not call this; the user is in charge.
 * Changes the selection in both the button and it's popup chooser.
 *
 * Since: 2.4
 */
void
gimp_palette_select_button_set_palette (GimpPaletteSelectButton *self,
                                        GimpPalette             *palette)
{
  g_return_if_fail (GIMP_IS_PALETTE_SELECT_BUTTON (self));

  g_debug ("%s", G_STRFUNC);

  /* Delegate to super with upcasts */
  gimp_resource_select_button_set_resource (GIMP_RESOURCE_SELECT_BUTTON (self), GIMP_RESOURCE (palette));
}


/*  private functions  */

static void
gimp_palette_select_button_finalize (GObject *object)
{
  g_debug ("%s called", G_STRFUNC);

  /* Has no allocations.*/

  /* Chain up. */
  G_OBJECT_CLASS (gimp_palette_select_button_parent_class)->finalize (object);
}




/* This is NOT an implementation of virtual function.
 *
 * Create a widget that is the interior of a button.
 * Super creates the button, self creates interior.
 * Button is-a container and self calls super to add interior to the container.
 *
 * Special: an hbox containing a general icon for a palette and
 * a label that is the name of the palette family and style.
 * FUTURE: label styled in the current palette family and style.
 */
static GtkWidget*
gimp_palette_select_button_create_interior (GimpPaletteSelectButton *self)
{
  GtkWidget   *button;
  GtkWidget   *hbox;
  GtkWidget   *image;
  GtkWidget   *label;
  gchar       *palette_name = "unknown";

  g_debug ("%s", G_STRFUNC);

  /* Outermost is-a button. */
  button = gtk_button_new ();

  /* inside the button is hbox. */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_add (GTK_CONTAINER (button), hbox);

  /* first item in hbox is an icon. */
  image = gtk_image_new_from_icon_name (GIMP_ICON_PALETTE,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  /* Second item in hbox is palette name.
   * The initial text is dummy, a draw will soon refresh it.
   * This function does not know the resource/palette.
   */
  label = gtk_label_new (palette_name);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 4);

  /* Ensure sub widgets saved for subsequent use. */

  self->palette_name_label = label;  /* Save label for redraw. */
  self->drag_region_widget = hbox;
  self->button = button;

  /* This subclass does not connect to draw signal on interior widget. */

  /* Return the whole interior, which is-a button. */
  return button;
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

  gtk_label_set_text (GTK_LABEL (palette_select->palette_name_label), name);
}
