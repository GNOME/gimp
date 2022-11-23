/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacircle.h
 * Copyright (C) 2014 Michael Natterer <mitch@ligma.org>
 *
 * Based on code from the color-rotate plug-in
 * Copyright (C) 1997-1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                         Based on code from Pavel Grinfeld (pavel@ml.com)
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

#ifndef __LIGMA_CIRCLE_H__
#define __LIGMA_CIRCLE_H__


#define LIGMA_TYPE_CIRCLE            (ligma_circle_get_type ())
#define LIGMA_CIRCLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CIRCLE, LigmaCircle))
#define LIGMA_CIRCLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CIRCLE, LigmaCircleClass))
#define LIGMA_IS_CIRCLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_CIRCLE))
#define LIGMA_IS_CIRCLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CIRCLE))
#define LIGMA_CIRCLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CIRCLE, LigmaCircleClass))


typedef struct _LigmaCirclePrivate LigmaCirclePrivate;
typedef struct _LigmaCircleClass   LigmaCircleClass;

struct _LigmaCircle
{
  GtkWidget          parent_instance;

  LigmaCirclePrivate *priv;
};

struct _LigmaCircleClass
{
  GtkWidgetClass  parent_class;

  void (* reset_target) (LigmaCircle *circle);
};


GType          ligma_circle_get_type                (void) G_GNUC_CONST;

GtkWidget    * ligma_circle_new                     (void);

gboolean       _ligma_circle_has_grab               (LigmaCircle *circle);
gdouble        _ligma_circle_get_angle_and_distance (LigmaCircle *circle,
                                                    gdouble     event_x,
                                                    gdouble     event_y,
                                                    gdouble    *distance);


#endif /* __LIGMA_CIRCLE_H__ */
