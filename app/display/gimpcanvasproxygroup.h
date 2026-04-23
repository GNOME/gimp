/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasproxygroup.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include "gimpcanvasgroup.h"


#define GIMP_TYPE_CANVAS_PROXY_GROUP (gimp_canvas_proxy_group_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpCanvasProxyGroup,
                          gimp_canvas_proxy_group,
                          GIMP, CANVAS_PROXY_GROUP,
                          GimpCanvasGroup)


struct _GimpCanvasProxyGroupClass
{
  GimpCanvasGroupClass  parent_class;
};


GimpCanvasItem * gimp_canvas_proxy_group_new         (GimpDisplayShell     *shell);

void             gimp_canvas_proxy_group_add_item    (GimpCanvasProxyGroup *group,
                                                      gpointer              object,
                                                      GimpCanvasItem       *proxy_item);
void             gimp_canvas_proxy_group_remove_item (GimpCanvasProxyGroup *group,
                                                      gpointer              object);
GimpCanvasItem * gimp_canvas_proxy_group_get_item    (GimpCanvasProxyGroup *group,
                                                      gpointer              object);
