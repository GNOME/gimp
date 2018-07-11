/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpThrobber
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_THROBBER_H__
#define __GIMP_THROBBER_H__

G_BEGIN_DECLS


#define GIMP_TYPE_THROBBER            (gimp_throbber_get_type ())
#define GIMP_THROBBER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_THROBBER, GimpThrobber))
#define GIMP_THROBBER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_THROBBER, GimpThrobberClass))
#define GIMP_IS_THROBBER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_THROBBER))
#define GIMP_IS_THROBBER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_THROBBER))
#define GIMP_THROBBER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMP_TYPE_THROBBER, GimpThrobberClass))


typedef struct _GimpThrobber        GimpThrobber;
typedef struct _GimpThrobberClass   GimpThrobberClass;
typedef struct _GimpThrobberPrivate GimpThrobberPrivate;

struct _GimpThrobber
{
  GtkToolItem          parent;

  /*< private >*/
  GimpThrobberPrivate *priv;
};

struct _GimpThrobberClass
{
  GtkToolItemClass parent_class;

  /* signal */
  void  (* clicked) (GimpThrobber *button);
};

GType         gimp_throbber_get_type      (void) G_GNUC_CONST;

GtkToolItem * gimp_throbber_new           (const gchar  *icon_name);
void          gimp_throbber_set_icon_name (GimpThrobber *button,
                                           const gchar  *icon_name);
const gchar * gimp_throbber_get_icon_name (GimpThrobber *button);
void          gimp_throbber_set_image     (GimpThrobber *button,
                                           GtkWidget    *image);
GtkWidget   * gimp_throbber_get_image     (GimpThrobber *button);


G_END_DECLS

#endif /* __GIMP_THROBBER_H__ */
