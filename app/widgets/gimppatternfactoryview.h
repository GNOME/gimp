/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapatternfactoryview.h
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PATTERN_FACTORY_VIEW_H__
#define __LIGMA_PATTERN_FACTORY_VIEW_H__

#include "ligmadatafactoryview.h"


#define LIGMA_TYPE_PATTERN_FACTORY_VIEW            (ligma_pattern_factory_view_get_type ())
#define LIGMA_PATTERN_FACTORY_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PATTERN_FACTORY_VIEW, LigmaPatternFactoryView))
#define LIGMA_PATTERN_FACTORY_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PATTERN_FACTORY_VIEW, LigmaPatternFactoryViewClass))
#define LIGMA_IS_PATTERN_FACTORY_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PATTERN_FACTORY_VIEW))
#define LIGMA_IS_PATTERN_FACTORY_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PATTERN_FACTORY_VIEW))
#define LIGMA_PATTERN_FACTORY_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PATTERN_FACTORY_VIEW, LigmaPatternFactoryViewClass))


typedef struct _LigmaPatternFactoryViewClass  LigmaPatternFactoryViewClass;

struct _LigmaPatternFactoryView
{
  LigmaDataFactoryView  parent_instance;
};

struct _LigmaPatternFactoryViewClass
{
  LigmaDataFactoryViewClass  parent_class;
};


GType       ligma_pattern_factory_view_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_pattern_factory_view_new      (LigmaViewType     view_type,
                                                LigmaDataFactory *factory,
                                                LigmaContext     *context,
                                                gint             view_size,
                                                gint             view_border_width,
                                                LigmaMenuFactory *menu_factory);


#endif  /*  __LIGMA_PATTERN_FACTORY_VIEW_H__  */
