/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererlayer.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "text/gimptextlayer.h"

#include "gimpviewrendererlayer.h"


static void   gimp_view_renderer_layer_render (GimpViewRenderer *renderer,
                                               GtkWidget        *widget);


G_DEFINE_TYPE (GimpViewRendererLayer, gimp_view_renderer_layer,
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
  const gchar *stock_id = NULL;

  if (gimp_layer_is_floating_sel (GIMP_LAYER (renderer->viewable)))
    {
      stock_id = GIMP_STOCK_FLOATING_SELECTION;
    }
  else if (gimp_drawable_is_text_layer (GIMP_DRAWABLE (renderer->viewable)))
    {
      stock_id = gimp_viewable_get_stock_id (renderer->viewable);
    }

  if (stock_id)
    gimp_view_renderer_default_render_stock (renderer, widget, stock_id);
  else
    GIMP_VIEW_RENDERER_CLASS (parent_class)->render (renderer, widget);
}
