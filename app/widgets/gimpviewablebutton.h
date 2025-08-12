/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewablebutton.h
 * Copyright (C) 2003-2005 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


#define GIMP_TYPE_VIEWABLE_BUTTON            (gimp_viewable_button_get_type ())
#define GIMP_VIEWABLE_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VIEWABLE_BUTTON, GimpViewableButton))
#define GIMP_VIEWABLE_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VIEWABLE_BUTTON, GimpViewableButtonClass))
#define GIMP_IS_VIEWABLE_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_VIEWABLE_BUTTON))
#define GIMP_IS_VIEWABLE_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VIEWABLE_BUTTON))
#define GIMP_VIEWABLE_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VIEWABLE_BUTTON, GimpViewableButtonClass))


typedef struct _GimpViewableButtonClass GimpViewableButtonClass;

struct _GimpViewableButton
{
  GimpButton         parent_instance;

  GimpContainer     *container;
  GimpContext       *context;

  GimpViewType       popup_view_type;
  gint               popup_view_size;

  gint               button_view_size;
  gint               view_border_width;

  GimpDialogFactory *dialog_factory;
  gchar             *dialog_identifier;
  gchar             *dialog_icon_name;
  gchar             *dialog_tooltip;

  GtkWidget         *view;
};

struct _GimpViewableButtonClass
{
  GimpButtonClass  parent_class;
};


GType       gimp_viewable_button_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_viewable_button_new      (GimpContainer      *container,
                                           GimpContext        *context,
                                           GimpViewType        view_type,
                                           gint                button_view_size,
                                           gint                view_size,
                                           gint                view_border_width,
                                           GimpDialogFactory  *dialog_factory,
                                           const gchar        *dialog_identifier,
                                           const gchar        *dialog_icon_name,
                                           const gchar        *dialog_tooltip);

GimpViewType gimp_viewable_button_get_view_type (GimpViewableButton *button);
void         gimp_viewable_button_set_view_type (GimpViewableButton *button,
                                                 GimpViewType        view_type);

gint         gimp_viewable_button_get_view_size (GimpViewableButton *button);
void         gimp_viewable_button_set_view_size (GimpViewableButton *button,
                                                 GimpViewSize        view_size);
