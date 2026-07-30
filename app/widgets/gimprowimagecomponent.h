/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprowimagecomponent.h
 * Copyright (C) 2026 Michael Natterer <mitch@gimp.org>
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

#include "gimprowimage.h"


#define GIMP_TYPE_ROW_IMAGE_COMPONENT (gimp_row_image_component_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpRowImageComponent,
                          gimp_row_image_component,
                          GIMP, ROW_IMAGE_COMPONENT,
                          GimpRowImage)


struct _GimpRowImageComponentClass
{
  GimpRowImageClass  parent_class;
};


void              gimp_row_image_component_set_channel (GimpRowImageComponent *row,
                                                        GimpChannelType        channel);
GimpChannelType   gimp_row_image_component_get_channel (GimpRowImageComponent *row);
