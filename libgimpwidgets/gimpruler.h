/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_RULER_H__
#define __GIMP_RULER_H__

G_BEGIN_DECLS

#define GIMP_TYPE_RULER            (gimp_ruler_get_type ())
#define GIMP_RULER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_RULER, GimpRuler))
#define GIMP_RULER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_RULER, GimpRulerClass))
#define GIMP_IS_RULER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_RULER))
#define GIMP_IS_RULER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_RULER))
#define GIMP_RULER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_RULER, GimpRulerClass))


typedef struct _GimpRulerClass   GimpRulerClass;

struct _GimpRuler
{
  GtkWidget  parent_instance;
};

struct _GimpRulerClass
{
  GtkWidgetClass  parent_class;

  /* Padding for future expansion */
  void (*_gimp_reserved1) (void);
  void (*_gimp_reserved2) (void);
  void (*_gimp_reserved3) (void);
  void (*_gimp_reserved4) (void);
};


GType       gimp_ruler_get_type            (void) G_GNUC_CONST;

GtkWidget * gimp_ruler_new                 (GtkOrientation  orientation);

void        gimp_ruler_add_track_widget    (GimpRuler      *ruler,
                                            GtkWidget      *widget);
void        gimp_ruler_remove_track_widget (GimpRuler      *ruler,
                                            GtkWidget      *widget);

void        gimp_ruler_set_unit            (GimpRuler      *ruler,
                                            GimpUnit        unit);
GimpUnit    gimp_ruler_get_unit            (GimpRuler      *ruler);
void        gimp_ruler_set_position        (GimpRuler      *ruler,
                                            gdouble         position);
gdouble     gimp_ruler_get_position        (GimpRuler      *ruler);
void        gimp_ruler_set_range           (GimpRuler      *ruler,
                                            gdouble         lower,
                                            gdouble         upper,
                                            gdouble         max_size);
void        gimp_ruler_get_range           (GimpRuler      *ruler,
                                            gdouble        *lower,
                                            gdouble        *upper,
                                            gdouble        *max_size);

G_END_DECLS

#endif /* __GIMP_RULER_H__ */
