/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewablebutton.h
 * Copyright (C) 2003-2005 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_VIEWABLE_BUTTON_H__
#define __LIGMA_VIEWABLE_BUTTON_H__


#define LIGMA_TYPE_VIEWABLE_BUTTON            (ligma_viewable_button_get_type ())
#define LIGMA_VIEWABLE_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VIEWABLE_BUTTON, LigmaViewableButton))
#define LIGMA_VIEWABLE_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_VIEWABLE_BUTTON, LigmaViewableButtonClass))
#define LIGMA_IS_VIEWABLE_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_VIEWABLE_BUTTON))
#define LIGMA_IS_VIEWABLE_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_VIEWABLE_BUTTON))
#define LIGMA_VIEWABLE_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_VIEWABLE_BUTTON, LigmaViewableButtonClass))


typedef struct _LigmaViewableButtonClass LigmaViewableButtonClass;

struct _LigmaViewableButton
{
  LigmaButton         parent_instance;

  LigmaContainer     *container;
  LigmaContext       *context;

  LigmaViewType       popup_view_type;
  gint               popup_view_size;

  gint               button_view_size;
  gint               view_border_width;

  LigmaDialogFactory *dialog_factory;
  gchar             *dialog_identifier;
  gchar             *dialog_icon_name;
  gchar             *dialog_tooltip;

  GtkWidget         *view;
};

struct _LigmaViewableButtonClass
{
  LigmaButtonClass  parent_class;
};


GType       ligma_viewable_button_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_viewable_button_new      (LigmaContainer      *container,
                                           LigmaContext        *context,
                                           LigmaViewType        view_type,
                                           gint                button_view_size,
                                           gint                view_size,
                                           gint                view_border_width,
                                           LigmaDialogFactory  *dialog_factory,
                                           const gchar        *dialog_identifier,
                                           const gchar        *dialog_icon_name,
                                           const gchar        *dialog_tooltip);

LigmaViewType ligma_viewable_button_get_view_type (LigmaViewableButton *button);
void         ligma_viewable_button_set_view_type (LigmaViewableButton *button,
                                                 LigmaViewType        view_type);

gint         ligma_viewable_button_get_view_size (LigmaViewableButton *button);
void         ligma_viewable_button_set_view_size (LigmaViewableButton *button,
                                                 gint                view_size);


#endif /* __LIGMA_VIEWABLE_BUTTON_H__ */
