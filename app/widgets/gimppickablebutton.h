/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapickablebutton.h
 * Copyright (C) 2013 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PICKABLE_BUTTON_H__
#define __LIGMA_PICKABLE_BUTTON_H__


#define LIGMA_TYPE_PICKABLE_BUTTON            (ligma_pickable_button_get_type ())
#define LIGMA_PICKABLE_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PICKABLE_BUTTON, LigmaPickableButton))
#define LIGMA_PICKABLE_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PICKABLE_BUTTON, LigmaPickableButtonClass))
#define LIGMA_IS_PICKABLE_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PICKABLE_BUTTON))
#define LIGMA_IS_PICKABLE_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PICKABLE_BUTTON))
#define LIGMA_PICKABLE_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PICKABLE_BUTTON, LigmaPickableButtonClass))


typedef struct _LigmaPickableButtonPrivate LigmaPickableButtonPrivate;
typedef struct _LigmaPickableButtonClass   LigmaPickableButtonClass;

struct _LigmaPickableButton
{
  LigmaButton                 parent_instance;

  LigmaPickableButtonPrivate *private;
};

struct _LigmaPickableButtonClass
{
  LigmaButtonClass  parent_class;
};


GType          ligma_pickable_button_get_type     (void) G_GNUC_CONST;

GtkWidget    * ligma_pickable_button_new          (LigmaContext        *context,
                                                  gint                view_size,
                                                  gint                view_border_width);

LigmaPickable * ligma_pickable_button_get_pickable (LigmaPickableButton *button);
void           ligma_pickable_button_set_pickable (LigmaPickableButton *button,
                                                  LigmaPickable       *pickable);


#endif /* __LIGMA_PICKABLE_BUTTON_H__ */
