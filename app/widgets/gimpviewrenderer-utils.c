/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrenderer-utils.c
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/ligmabrush.h"
#include "core/ligmabuffer.h"
#include "core/ligmagradient.h"
#include "core/ligmaimage.h"
#include "core/ligmaimagefile.h"

#include "core/ligmaimageproxy.h"
#include "core/ligmalayer.h"
#include "core/ligmapalette.h"

#include "vectors/ligmavectors.h"

#include "ligmaviewrenderer-utils.h"
#include "ligmaviewrendererbrush.h"
#include "ligmaviewrendererbuffer.h"
#include "ligmaviewrendererlayer.h"
#include "ligmaviewrenderergradient.h"
#include "ligmaviewrendererimage.h"
#include "ligmaviewrendererimagefile.h"
#include "ligmaviewrendererpalette.h"
#include "ligmaviewrenderervectors.h"


GType
ligma_view_renderer_type_from_viewable_type (GType viewable_type)
{
  GType type = LIGMA_TYPE_VIEW_RENDERER;

  g_return_val_if_fail (g_type_is_a (viewable_type, LIGMA_TYPE_VIEWABLE),
                        G_TYPE_NONE);

  if (g_type_is_a (viewable_type, LIGMA_TYPE_BRUSH))
    {
      type = LIGMA_TYPE_VIEW_RENDERER_BRUSH;
    }
  else if (g_type_is_a (viewable_type, LIGMA_TYPE_BUFFER))
    {
      type = LIGMA_TYPE_VIEW_RENDERER_BUFFER;
    }
  else if (g_type_is_a (viewable_type, LIGMA_TYPE_IMAGE) ||
           g_type_is_a (viewable_type, LIGMA_TYPE_IMAGE_PROXY))
    {
      type = LIGMA_TYPE_VIEW_RENDERER_IMAGE;
    }
  else if (g_type_is_a (viewable_type, LIGMA_TYPE_LAYER))
    {
      type = LIGMA_TYPE_VIEW_RENDERER_LAYER;
    }
  else if (g_type_is_a (viewable_type, LIGMA_TYPE_DRAWABLE))
    {
      type = LIGMA_TYPE_VIEW_RENDERER_DRAWABLE;
    }
  else if (g_type_is_a (viewable_type, LIGMA_TYPE_GRADIENT))
    {
      type = LIGMA_TYPE_VIEW_RENDERER_GRADIENT;
    }
  else if (g_type_is_a (viewable_type, LIGMA_TYPE_VECTORS))
    {
      type = LIGMA_TYPE_VIEW_RENDERER_VECTORS;
    }
  else if (g_type_is_a (viewable_type, LIGMA_TYPE_IMAGEFILE))
    {
      type = LIGMA_TYPE_VIEW_RENDERER_IMAGEFILE;
    }
  else if (g_type_is_a (viewable_type, LIGMA_TYPE_PALETTE))
    {
      type = LIGMA_TYPE_VIEW_RENDERER_PALETTE;
    }

  return type;
}
