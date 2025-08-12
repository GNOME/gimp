/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrushfactoryview.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "gimpdatafactoryview.h"


#define GIMP_TYPE_BRUSH_FACTORY_VIEW            (gimp_brush_factory_view_get_type ())
#define GIMP_BRUSH_FACTORY_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BRUSH_FACTORY_VIEW, GimpBrushFactoryView))
#define GIMP_BRUSH_FACTORY_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BRUSH_FACTORY_VIEW, GimpBrushFactoryViewClass))
#define GIMP_IS_BRUSH_FACTORY_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BRUSH_FACTORY_VIEW))
#define GIMP_IS_BRUSH_FACTORY_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BRUSH_FACTORY_VIEW))
#define GIMP_BRUSH_FACTORY_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BRUSH_FACTORY_VIEW, GimpBrushFactoryViewClass))


typedef struct _GimpBrushFactoryViewClass  GimpBrushFactoryViewClass;

struct _GimpBrushFactoryView
{
  GimpDataFactoryView  parent_instance;

  GtkWidget           *spacing_scale;
  GtkAdjustment       *spacing_adjustment;

  gboolean             change_brush_spacing;
  GQuark               spacing_changed_handler_id;
};

struct _GimpBrushFactoryViewClass
{
  GimpDataFactoryViewClass  parent_class;

  /* Signals */

  void (* spacing_changed) (GimpBrushFactoryView *view);
};


GType       gimp_brush_factory_view_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_brush_factory_view_new      (GimpViewType     view_type,
                                              GimpDataFactory *factory,
                                              GimpContext     *context,
                                              gboolean         change_brush_spacing,
                                              gint             view_size,
                                              gint             view_border_width,
                                              GimpMenuFactory *menu_factory);
