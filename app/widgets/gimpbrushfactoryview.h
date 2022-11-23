/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabrushfactoryview.h
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

#ifndef __LIGMA_BRUSH_FACTORY_VIEW_H__
#define __LIGMA_BRUSH_FACTORY_VIEW_H__

#include "ligmadatafactoryview.h"


#define LIGMA_TYPE_BRUSH_FACTORY_VIEW            (ligma_brush_factory_view_get_type ())
#define LIGMA_BRUSH_FACTORY_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BRUSH_FACTORY_VIEW, LigmaBrushFactoryView))
#define LIGMA_BRUSH_FACTORY_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BRUSH_FACTORY_VIEW, LigmaBrushFactoryViewClass))
#define LIGMA_IS_BRUSH_FACTORY_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BRUSH_FACTORY_VIEW))
#define LIGMA_IS_BRUSH_FACTORY_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BRUSH_FACTORY_VIEW))
#define LIGMA_BRUSH_FACTORY_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BRUSH_FACTORY_VIEW, LigmaBrushFactoryViewClass))


typedef struct _LigmaBrushFactoryViewClass  LigmaBrushFactoryViewClass;

struct _LigmaBrushFactoryView
{
  LigmaDataFactoryView  parent_instance;

  GtkWidget           *spacing_scale;
  GtkAdjustment       *spacing_adjustment;

  gboolean             change_brush_spacing;
  GQuark               spacing_changed_handler_id;
};

struct _LigmaBrushFactoryViewClass
{
  LigmaDataFactoryViewClass  parent_class;

  /* Signals */

  void (* spacing_changed) (LigmaBrushFactoryView *view);
};


GType       ligma_brush_factory_view_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_brush_factory_view_new      (LigmaViewType     view_type,
                                              LigmaDataFactory *factory,
                                              LigmaContext     *context,
                                              gboolean         change_brush_spacing,
                                              gint             view_size,
                                              gint             view_border_width,
                                              LigmaMenuFactory *menu_factory);


#endif  /*  __LIGMA_BRUSH_FACTORY_VIEW_H__  */
