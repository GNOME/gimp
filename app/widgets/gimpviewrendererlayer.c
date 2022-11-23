/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrendererlayer.c
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

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmaimage.h"

#include "text/ligmatextlayer.h"

#include "ligmaviewrendererlayer.h"


static void   ligma_view_renderer_layer_render (LigmaViewRenderer *renderer,
                                               GtkWidget        *widget);


G_DEFINE_TYPE (LigmaViewRendererLayer, ligma_view_renderer_layer,
               LIGMA_TYPE_VIEW_RENDERER_DRAWABLE)

#define parent_class ligma_view_renderer_layer_parent_class


static void
ligma_view_renderer_layer_class_init (LigmaViewRendererLayerClass *klass)
{
  LigmaViewRendererClass *renderer_class = LIGMA_VIEW_RENDERER_CLASS (klass);

  renderer_class->render = ligma_view_renderer_layer_render;
}

static void
ligma_view_renderer_layer_init (LigmaViewRendererLayer *renderer)
{
}

static void
ligma_view_renderer_layer_render (LigmaViewRenderer *renderer,
                                 GtkWidget        *widget)
{
  const gchar *icon_name = NULL;

  if (ligma_layer_is_floating_sel (LIGMA_LAYER (renderer->viewable)))
    {
      icon_name = LIGMA_ICON_LAYER_FLOATING_SELECTION;
    }
  else if (ligma_item_is_text_layer (LIGMA_ITEM (renderer->viewable)))
    {
      icon_name = ligma_viewable_get_icon_name (renderer->viewable);
    }
  else
    {
      LigmaContainer *children = ligma_viewable_get_children (renderer->viewable);

      if (children)
        {
          LigmaItem  *item  = LIGMA_ITEM (renderer->viewable);
          LigmaImage *image = ligma_item_get_image (item);

          if (ligma_container_get_n_children (children) == 0)
            icon_name = "folder";
          else if (image && ! image->ligma->config->group_layer_previews)
            icon_name = ligma_viewable_get_icon_name (renderer->viewable);
        }
    }

  if (icon_name)
    ligma_view_renderer_render_icon (renderer, widget, icon_name);
  else
    LIGMA_VIEW_RENDERER_CLASS (parent_class)->render (renderer, widget);
}
