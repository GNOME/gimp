/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpasyncset.h
 * Copyright (C) 2018 Ell
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


#define GIMP_TYPE_ASYNC_SET            (gimp_async_set_get_type ())
#define GIMP_ASYNC_SET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ASYNC_SET, GimpAsyncSet))
#define GIMP_ASYNC_SET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ASYNC_SET, GimpAsyncSetClass))
#define GIMP_IS_ASYNC_SET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ASYNC_SET))
#define GIMP_IS_ASYNC_SET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ASYNC_SET))
#define GIMP_ASYNC_SET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ASYNC_SET, GimpAsyncSetClass))


typedef struct _GimpAsyncSetPrivate GimpAsyncSetPrivate;
typedef struct _GimpAsyncSetClass   GimpAsyncSetClass;

struct _GimpAsyncSet
{
  GObject              parent_instance;

  GimpAsyncSetPrivate *priv;
};

struct _GimpAsyncSetClass
{
  GObjectClass  parent_class;
};


GType          gimp_async_set_get_type (void) G_GNUC_CONST;

GimpAsyncSet * gimp_async_set_new      (void);

void           gimp_async_set_add      (GimpAsyncSet *async_set,
                                        GimpAsync    *async);
void           gimp_async_set_remove   (GimpAsyncSet *async_set,
                                        GimpAsync    *async);
void           gimp_async_set_clear    (GimpAsyncSet *async_set);
gboolean       gimp_async_set_is_empty (GimpAsyncSet *async_set);
