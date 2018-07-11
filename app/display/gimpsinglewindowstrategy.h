/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsinglewindowstrategy.h
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

#ifndef __GIMP_SINGLE_WINDOW_STRATEGY_H__
#define __GIMP_SINGLE_WINDOW_STRATEGY_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_SINGLE_WINDOW_STRATEGY            (gimp_single_window_strategy_get_type ())
#define GIMP_SINGLE_WINDOW_STRATEGY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SINGLE_WINDOW_STRATEGY, GimpSingleWindowStrategy))
#define GIMP_SINGLE_WINDOW_STRATEGY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SINGLE_WINDOW_STRATEGY, GimpSingleWindowStrategyClass))
#define GIMP_IS_SINGLE_WINDOW_STRATEGY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SINGLE_WINDOW_STRATEGY))
#define GIMP_IS_SINGLE_WINDOW_STRATEGY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SINGLE_WINDOW_STRATEGY))
#define GIMP_SINGLE_WINDOW_STRATEGY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SINGLE_WINDOW_STRATEGY, GimpSingleWindowStrategyClass))


typedef struct _GimpSingleWindowStrategyClass GimpSingleWindowStrategyClass;

struct _GimpSingleWindowStrategy
{
  GimpObject  parent_instance;
};

struct _GimpSingleWindowStrategyClass
{
  GimpObjectClass  parent_class;
};


GType        gimp_single_window_strategy_get_type          (void) G_GNUC_CONST;

GimpObject * gimp_single_window_strategy_get_singleton     (void);


#endif /* __GIMP_SINGLE_WINDOW_STRATEGY_H__ */
