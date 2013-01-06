/* HSV color selector for GTK+
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Simon Budig <Simon.Budig@unix-ag.org> (original code)
 *          Federico Mena-Quintero <federico@gimp.org> (cleanup for GTK+)
 *          Jonathan Blandford <jrb@redhat.com> (cleanup for GTK+)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __GIMP_COLOR_WHEEL_H__
#define __GIMP_COLOR_WHEEL_H__

G_BEGIN_DECLS

#define GIMP_TYPE_COLOR_WHEEL            (gimp_color_wheel_get_type ())
#define GIMP_COLOR_WHEEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_WHEEL, GimpColorWheel))
#define GIMP_COLOR_WHEEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_WHEEL, GimpColorWheelClass))
#define GIMP_IS_COLOR_WHEEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_WHEEL))
#define GIMP_IS_COLOR_WHEEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_WHEEL))
#define GIMP_COLOR_WHEEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_WHEEL, GimpColorWheelClass))


typedef struct _GimpColorWheel      GimpColorWheel;
typedef struct _GimpColorWheelClass GimpColorWheelClass;

struct _GimpColorWheel
{
  GtkWidget parent_instance;

  /* Private data */
  gpointer priv;
};

struct _GimpColorWheelClass
{
  GtkWidgetClass parent_class;

  /* Notification signals */
  void (* changed) (GimpColorWheel   *wheel);

  /* Keybindings */
  void (* move)    (GimpColorWheel   *wheel,
                    GtkDirectionType  type);

  /* Padding for future expansion */
  void (*_gimp_reserved1) (void);
  void (*_gimp_reserved2) (void);
  void (*_gimp_reserved3) (void);
  void (*_gimp_reserved4) (void);
};


GType       gimp_color_wheel_get_type          (void) G_GNUC_CONST;
GtkWidget * gimp_color_wheel_new               (void);

void        gimp_color_wheel_set_color         (GimpColorWheel *wheel,
                                                double          h,
                                                double          s,
                                                double          v);
void        gimp_color_wheel_get_color         (GimpColorWheel *wheel,
                                                gdouble        *h,
                                                gdouble        *s,
                                                gdouble        *v);

void        gimp_color_wheel_set_ring_fraction (GimpColorWheel *wheel,
                                                gdouble         fraction);
gdouble     gimp_color_wheel_get_ring_fraction (GimpColorWheel *wheel);

gboolean    gimp_color_wheel_is_adjusting      (GimpColorWheel *wheel);

G_END_DECLS

#endif /* __GIMP_COLOR_WHEEL_H__ */
