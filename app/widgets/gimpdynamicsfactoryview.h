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


#define GIMP_TYPE_DYNAMICS_FACTORY_VIEW (gimp_dynamics_factory_view_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpDynamicsFactoryView,
                          gimp_dynamics_factory_view,
                          GIMP, DYNAMICS_FACTORY_VIEW,
                          GimpDataFactoryView)


struct _GimpDynamicsFactoryViewClass
{
  GimpDataFactoryViewClass  parent_class;
};


GtkWidget * gimp_dynamics_factory_view_new (GimpViewType     view_type,
                                            GimpDataFactory *factory,
                                            GimpContext     *context,
                                            gint             view_size,
                                            gint             view_border_width,
                                            GimpMenuFactory *menu_factory);
