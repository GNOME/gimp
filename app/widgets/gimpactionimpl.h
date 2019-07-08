/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpactionimpl.h
 * Copyright (C) 2004-2019 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_ACTION_IMPL_H__
#define __GIMP_ACTION_IMPL_H__


#define GIMP_TYPE_ACTION_IMPL            (gimp_action_impl_get_type ())
#define GIMP_ACTION_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ACTION_IMPL, GimpActionImpl))
#define GIMP_ACTION_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ACTION_IMPL, GimpActionImplClass))
#define GIMP_IS_ACTION_IMPL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ACTION_IMPL))
#define GIMP_IS_ACTION_IMPL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), GIMP_TYPE_ACTION_IMPL))
#define GIMP_ACTION_IMPL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMP_TYPE_ACTION_IMPL, GimpActionImplClass))

typedef struct _GimpActionImpl      GimpActionImpl;
typedef struct _GimpActionImplClass GimpActionImplClass;

struct _GimpActionImpl
{
  GtkAction           parent_instance;

  GimpContext        *context;

  GimpRGB            *color;
  GimpViewable       *viewable;
  PangoEllipsizeMode  ellipsize;
  gint                max_width_chars;
};

struct _GimpActionImplClass
{
  GtkActionClass parent_class;
};

GType         gimp_action_impl_get_type       (void) G_GNUC_CONST;

GimpAction  * gimp_action_impl_new            (const gchar   *name,
                                               const gchar   *label,
                                               const gchar   *tooltip,
                                               const gchar   *icon_name,
                                               const gchar   *help_id);


#endif  /* __GIMP_ACTION_IMPL_H__ */
