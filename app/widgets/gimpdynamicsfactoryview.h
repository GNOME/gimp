/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdynamicsfactoryview.h
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


#define GIMP_TYPE_DYNAMICS_FACTORY_VIEW            (gimp_dynamics_factory_view_get_type ())
#define GIMP_DYNAMICS_FACTORY_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DYNAMICS_FACTORY_VIEW, GimpDynamicsFactoryView))
#define GIMP_DYNAMICS_FACTORY_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DYNAMICS_FACTORY_VIEW, GimpDynamicsFactoryViewClass))
#define GIMP_IS_DYNAMICS_FACTORY_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DYNAMICS_FACTORY_VIEW))
#define GIMP_IS_DYNAMICS_FACTORY_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DYNAMICS_FACTORY_VIEW))
#define GIMP_DYNAMICS_FACTORY_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DYNAMICS_FACTORY_VIEW, GimpDynamicsFactoryViewClass))


typedef struct _GimpDynamicsFactoryViewClass  GimpDynamicsFactoryViewClass;

struct _GimpDynamicsFactoryView
{
  GimpDataFactoryView  parent_instance;
};

struct _GimpDynamicsFactoryViewClass
{
  GimpDataFactoryViewClass  parent_class;
};


GType       gimp_dynamics_factory_view_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_dynamics_factory_view_new     (GimpViewType     view_type,
                                                GimpDataFactory *factory,
                                                GimpContext     *context,
                                                gint             view_size,
                                                gint             view_border_width,
                                                GimpMenuFactory *menu_factory);
