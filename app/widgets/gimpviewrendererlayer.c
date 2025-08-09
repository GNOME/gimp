/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererlayer.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimplinklayer.h"

#include "text/gimptextlayer.h"

#include "gimpviewrendererlayer.h"


static void   gimp_view_renderer_layer_render (GimpViewRenderer *renderer,
                                               GtkWidget        *widget);


G_DEFINE_TYPE (GimpViewRendererLayer,
               gimp_view_renderer_layer,
               GIMP_TYPE_VIEW_RENDERER_DRAWABLE)

#define parent_class gimp_view_renderer_layer_parent_class


static void
gimp_view_renderer_layer_class_init (GimpViewRendererLayerClass *klass)
{
  GimpViewRendererClass *renderer_class = GIMP_VIEW_RENDERER_CLASS (klass);

  renderer_class->render = gimp_view_renderer_layer_render;
}

static void
gimp_view_renderer_layer_init (GimpViewRendererLayer *renderer)
{
}

static void
gimp_view_renderer_layer_render (GimpViewRenderer *renderer,
                                 GtkWidget        *widget)
{
  const gchar *icon_name = NULL;

  if (gimp_layer_is_floating_sel (GIMP_LAYER (renderer->viewable)))
    {
      icon_name = GIMP_ICON_LAYER_FLOATING_SELECTION;
    }
  else if (gimp_item_is_text_layer (GIMP_ITEM (renderer->viewable)) ||
           (GIMP_IS_LINK_LAYER (renderer->viewable) &&
            gimp_link_layer_is_monitored (GIMP_LINK_LAYER (renderer->viewable))))
    {
      icon_name = gimp_viewable_get_icon_name (renderer->viewable);
    }
  else
    {
      GimpContainer *children = gimp_viewable_get_children (renderer->viewable);

      if (children)
        {
          GimpItem  *item  = GIMP_ITEM (renderer->viewable);
          GimpImage *image = gimp_item_get_image (item);

          if (gimp_container_get_n_children (children) == 0)
            icon_name = "folder";
          else if (image && ! image->gimp->config->group_layer_previews)
            icon_name = gimp_viewable_get_icon_name (renderer->viewable);
        }
    }

  if (icon_name)
    gimp_view_renderer_render_icon (renderer, widget,
                                    icon_name,
                                    gtk_widget_get_scale_factor (widget));
  else
    GIMP_VIEW_RENDERER_CLASS (parent_class)->render (renderer, widget);
}
