/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpfontselectbutton.c
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
#include "gimpfontselectbutton.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimpfontselectbutton
 * @title: GimpFontSelectButton
 * @short_description: A button which pops up a font selection dialog.
 *
 * A button which pops up a font selection dialog.
 **/

struct _GimpFontSelectButton
{
  /* !! Not a pointer, is contained. */
  GimpResourceSelectButton parent_instance;

  GtkWidget *font_name_label;
  GtkWidget *drag_region_widget;
  GtkWidget *button;
};

/*  local  */

/* implement virtual */
static void gimp_font_select_button_finalize        (GObject                  *object);
static void gimp_font_select_button_draw_interior   (GimpResourceSelectButton *self);

/* Called at init. */
static GtkWidget *gimp_font_select_button_create_interior  (GimpFontSelectButton     *self);

/* A GtkTargetEntry has a string and two ints. This is one, but treat as an array.*/
static const GtkTargetEntry drag_target = { "application/x-gimp-font-name", 0, 0 };

G_DEFINE_FINAL_TYPE (GimpFontSelectButton,
                     gimp_font_select_button,
                     GIMP_TYPE_RESOURCE_SELECT_BUTTON)


static void
gimp_font_select_button_class_init (GimpFontSelectButtonClass *klass)
{
  /* Alias cast klass to super classes. */
  GObjectClass                  *object_class  = G_OBJECT_CLASS (klass);
  GimpResourceSelectButtonClass *superclass    = GIMP_RESOURCE_SELECT_BUTTON_CLASS (klass);

  g_debug ("%s called", G_STRFUNC);

  /* Override virtual. */
  object_class->finalize     = gimp_font_select_button_finalize;

  /* Implement pure virtual functions. */
  superclass->draw_interior = gimp_font_select_button_draw_interior;

  /* Set data member of class. */
  superclass->resource_type = GIMP_TYPE_FONT;

  /* We don't define property getter/setters: use superclass getter/setters.
   * But super property name is "resource", not "font"
   */
}

static void
gimp_font_select_button_init (GimpFontSelectButton *self)
{
  GtkWidget *interior;

  g_debug ("%s called", G_STRFUNC);

  /* Specialize super:
   *     - embed our widget interior instance to super widget instance.
   *     - tell super our dnd widget
   *     - tell super our clickable button
   * Call superclass methods, with upcasts.
   * These are on instance, not our subclass.
   */
  interior = gimp_font_select_button_create_interior (self);
  /* require self has sub widgets initialized. */

  /* Embed the whole button.*/
  gimp_resource_select_button_embed_interior (GIMP_RESOURCE_SELECT_BUTTON (self), interior);

  /* Self knows the GtkTargetEntry, super creates target and handles receive drag. */
  gimp_resource_select_button_set_drag_target (GIMP_RESOURCE_SELECT_BUTTON (self),
                                               self->drag_region_widget,
                                               &drag_target);
  /* Super handles button clicks. */
  gimp_resource_select_button_set_clickable (GIMP_RESOURCE_SELECT_BUTTON (self),
                                             self->button);
}

/**
 * gimp_font_select_button_new:
 * @title: (nullable): Title of the dialog to use or %NULL to use the default title.
 * @resource: (nullable): Initial font.
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
gimp_font_select_button_new (const gchar  *title,
                             GimpResource *resource)
{
  GtkWidget *self;

  g_debug ("%s called", G_STRFUNC);

  if (resource == NULL)
    {
      g_debug ("%s defaulting font from context", G_STRFUNC);
      resource = GIMP_RESOURCE (gimp_context_get_font ());
    }
  g_assert (resource != NULL);
  /* This method is polymorphic, so a factory can call it, but requires Font. */
  g_return_val_if_fail (GIMP_IS_FONT (resource), NULL);

  /* Create instance of self (not super.)
   * This will call superclass init, self class init, superclass init, and instance init.
   * Self subclass class_init will specialize by implementing virtual funcs
   * that open and set remote chooser dialog, and that draw self interior.
   *
   * !!! property belongs to superclass and is named "resource"
   */
   if (title)
     self = g_object_new (GIMP_TYPE_FONT_SELECT_BUTTON,
                          "title",     title,
                          "resource",  resource,
                          NULL);
   else
     self = g_object_new (GIMP_TYPE_FONT_SELECT_BUTTON,
                          "resource",  resource,
                          NULL);

  /* We don't subscribe to events from super (such as draw events.)
   * Super will call our draw method when it's resource changes.
   * Except that the above setting of property happens too late,
   * so we now draw the initial resource.
   */

  /* Draw with the initial resource. Cast self from Widget. */
  gimp_font_select_button_draw_interior (GIMP_RESOURCE_SELECT_BUTTON (self));

  g_debug ("%s returns", G_STRFUNC);

  return self;
}


/* Getter and setter.
 * We could omit these, and use only the superclass methods.
 * But script-fu-interface.c uses these, until FUTURE.
 */

/**
 * gimp_font_select_button_get_font:
 * @self: A #GimpFontSelectButton
 *
 * Gets the currently selected font.
 *
 * Returns: (transfer none): an internal copy of the font which must not be freed.
 *
 * Since: 2.4
 */
GimpFont *
gimp_font_select_button_get_font (GimpFontSelectButton *self)
{
  g_return_val_if_fail (GIMP_IS_FONT_SELECT_BUTTON (self), NULL);

  /* Delegate to super w upcast arg and downcast result. */
  return (GimpFont *) gimp_resource_select_button_get_resource ((GimpResourceSelectButton*) self);
}

/**
 * gimp_font_select_button_set_font:
 * @self: A #GimpFontSelectButton
 * @font: Font to set.
 *
 * Sets the currently selected font.
 * Usually you should not call this; the user is in charge.
 * Changes the selection in both the button and it's popup chooser.
 *
 * Since: 2.4
 */
void
gimp_font_select_button_set_font (GimpFontSelectButton *self,
                                  GimpFont             *font)
{
  g_return_if_fail (GIMP_IS_FONT_SELECT_BUTTON (self));

  g_debug ("%s", G_STRFUNC);

  /* Delegate to super with upcasts */
  gimp_resource_select_button_set_resource (GIMP_RESOURCE_SELECT_BUTTON (self), GIMP_RESOURCE (font));
}


/*  private functions  */

static void
gimp_font_select_button_finalize (GObject *object)
{
  g_debug ("%s called", G_STRFUNC);

  /* Has no allocations.*/

  /* Chain up. */
  G_OBJECT_CLASS (gimp_font_select_button_parent_class)->finalize (object);
}




/* This is NOT an implementation of virtual function.
 *
 * Create a widget that is the interior of a button.
 * Super creates the button, self creates interior.
 * Button is-a container and self calls super to add interior to the container.
 *
 * Special: an hbox containing a general icon for a font and
 * a label that is the name of the font family and style.
 * FUTURE: label styled in the current font family and style.
 */
static GtkWidget*
gimp_font_select_button_create_interior (GimpFontSelectButton *self)
{
  GtkWidget   *button;
  GtkWidget   *hbox;
  GtkWidget   *image;
  GtkWidget   *label;
  gchar       *font_name = "unknown";

  g_debug ("%s", G_STRFUNC);

  /* Outermost is-a button. */
  button = gtk_button_new ();

  /* inside the button is hbox. */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_add (GTK_CONTAINER (button), hbox);

  /* first item in hbox is an icon. */
  image = gtk_image_new_from_icon_name (GIMP_ICON_FONT,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  /* Second item in hbox is font name.
   * The initial text is dummy, a draw will soon refresh it.
   * This function does not know the resource/font.
   */
  label = gtk_label_new (font_name);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 4);

  /* Ensure sub widgets saved for subsequent use. */

  self->font_name_label = label;  /* Save label for redraw. */
  self->drag_region_widget = hbox;
  self->button = button;

  /* This subclass does not connect to draw signal on interior widget. */

  /* Return the whole interior, which is-a button. */
  return button;
}


/* Knows how to draw self interior.
 * Self knows resource, it is not passed.
 *
 * Overrides virtual method in super, so it is generic on Resource.
 */
static void
gimp_font_select_button_draw_interior (GimpResourceSelectButton *self)
{
  gchar                *font_name;
  GimpResource         *resource;
  GimpFontSelectButton *self_as_font_select;

  g_debug ("%s", G_STRFUNC);

  g_return_if_fail (GIMP_IS_FONT_SELECT_BUTTON (self));
  self_as_font_select = GIMP_FONT_SELECT_BUTTON (self);

  g_object_get (self, "resource", &resource, NULL);

  /* For now, the "id" property of the resource is the name.
   * FUTURE: core will support name distinct from ID.
   */
  g_object_get (resource, "id", &font_name, NULL);

  /* We are not keeping a copy of font name, nothing to free. */

  /* Not styling the text using the chosen font,
   * just replacing the text with the name of the chosen font.
   */
  gtk_label_set_text (GTK_LABEL (self_as_font_select->font_name_label), font_name);
}
