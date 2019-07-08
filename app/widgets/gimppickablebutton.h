/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppickablebutton.h
 * Copyright (C) 2013 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_PICKABLE_BUTTON_H__
#define __GIMP_PICKABLE_BUTTON_H__


#define GIMP_TYPE_PICKABLE_BUTTON            (gimp_pickable_button_get_type ())
#define GIMP_PICKABLE_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PICKABLE_BUTTON, GimpPickableButton))
#define GIMP_PICKABLE_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PICKABLE_BUTTON, GimpPickableButtonClass))
#define GIMP_IS_PICKABLE_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PICKABLE_BUTTON))
#define GIMP_IS_PICKABLE_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PICKABLE_BUTTON))
#define GIMP_PICKABLE_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PICKABLE_BUTTON, GimpPickableButtonClass))


typedef struct _GimpPickableButtonPrivate GimpPickableButtonPrivate;
typedef struct _GimpPickableButtonClass   GimpPickableButtonClass;

struct _GimpPickableButton
{
  GimpButton                 parent_instance;

  GimpPickableButtonPrivate *private;
};

struct _GimpPickableButtonClass
{
  GimpButtonClass  parent_class;
};


GType          gimp_pickable_button_get_type     (void) G_GNUC_CONST;

GtkWidget    * gimp_pickable_button_new          (GimpContext        *context,
                                                  gint                view_size,
                                                  gint                view_border_width);

GimpPickable * gimp_pickable_button_get_pickable (GimpPickableButton *button);
void           gimp_pickable_button_set_pickable (GimpPickableButton *button,
                                                  GimpPickable       *pickable);


#endif /* __GIMP_PICKABLE_BUTTON_H__ */
