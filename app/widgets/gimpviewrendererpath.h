/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererpath.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *                    Simon Budig <simon@gimp.org>
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

#include "gimpviewrenderer.h"


#define GIMP_TYPE_VIEW_RENDERER_PATH (gimp_view_renderer_path_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpViewRendererPath,
                          gimp_view_renderer_path,
                          GIMP, VIEW_RENDERER_PATH,
                          GimpViewRenderer)


struct _GimpViewRendererPathClass
{
  GimpViewRendererClass  parent_class;
};
