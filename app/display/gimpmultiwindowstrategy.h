/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamultiwindowstrategy.h
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

#ifndef __LIGMA_MULTI_WINDOW_STRATEGY_H__
#define __LIGMA_MULTI_WINDOW_STRATEGY_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_MULTI_WINDOW_STRATEGY            (ligma_multi_window_strategy_get_type ())
#define LIGMA_MULTI_WINDOW_STRATEGY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MULTI_WINDOW_STRATEGY, LigmaMultiWindowStrategy))
#define LIGMA_MULTI_WINDOW_STRATEGY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MULTI_WINDOW_STRATEGY, LigmaMultiWindowStrategyClass))
#define LIGMA_IS_MULTI_WINDOW_STRATEGY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MULTI_WINDOW_STRATEGY))
#define LIGMA_IS_MULTI_WINDOW_STRATEGY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MULTI_WINDOW_STRATEGY))
#define LIGMA_MULTI_WINDOW_STRATEGY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MULTI_WINDOW_STRATEGY, LigmaMultiWindowStrategyClass))


typedef struct _LigmaMultiWindowStrategyClass LigmaMultiWindowStrategyClass;

struct _LigmaMultiWindowStrategy
{
  LigmaObject  parent_instance;
};

struct _LigmaMultiWindowStrategyClass
{
  LigmaObjectClass  parent_class;
};


GType        ligma_multi_window_strategy_get_type          (void) G_GNUC_CONST;

LigmaObject * ligma_multi_window_strategy_get_singleton     (void);


#endif /* __LIGMA_MULTI_WINDOW_STRATEGY_H__ */
