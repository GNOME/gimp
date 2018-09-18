/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvaslayerboundary.c
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "core/gimpchannel.h"
#include "core/gimplayer.h"
#include "core/gimplayer-floating-selection.h"

#include "gimpcanvas-style.h"
#include "gimpcanvaslayerboundary.h"
#include "gimpdisplayshell.h"


enum
{
  PROP_0,
  PROP_LAYER,
  PROP_EDIT_MASK
};


typedef struct _GimpCanvasLayerBoundaryPrivate GimpCanvasLayerBoundaryPrivate;

struct _GimpCanvasLayerBoundaryPrivate
{
  GimpLayer *layer;
  gboolean   edit_mask;
};

#define GET_PRIVATE(layer_boundary) \
        ((GimpCanvasLayerBoundaryPrivate *) gimp_canvas_layer_boundary_get_instance_private ((GimpCanvasLayerBoundary *) (layer_boundary)))


/*  local function prototypes  */

static void             gimp_canvas_layer_boundary_set_property (GObject        *object,
                                                                 guint           property_id,
                                                                 const GValue   *value,
                                                                 GParamSpec     *pspec);
static void             gimp_canvas_layer_boundary_get_property (GObject        *object,
                                                                 guint           property_id,
                                                                 GValue         *value,
                                                                 GParamSpec     *pspec);
static void             gimp_canvas_layer_boundary_finalize     (GObject        *object);
static void             gimp_canvas_layer_boundary_draw         (GimpCanvasItem *item,
                                                                 cairo_t        *cr);
static cairo_region_t * gimp_canvas_layer_boundary_get_extents  (GimpCanvasItem *item);
static void             gimp_canvas_layer_boundary_stroke       (GimpCanvasItem *item,
                                                                 cairo_t        *cr);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCanvasLayerBoundary, gimp_canvas_layer_boundary,
                            GIMP_TYPE_CANVAS_RECTANGLE)

#define parent_class gimp_canvas_layer_boundary_parent_class


static void
gimp_canvas_layer_boundary_class_init (GimpCanvasLayerBoundaryClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = gimp_canvas_layer_boundary_set_property;
  object_class->get_property = gimp_canvas_layer_boundary_get_property;
  object_class->finalize     = gimp_canvas_layer_boundary_finalize;

  item_class->draw           = gimp_canvas_layer_boundary_draw;
  item_class->get_extents    = gimp_canvas_layer_boundary_get_extents;
  item_class->stroke         = gimp_canvas_layer_boundary_stroke;

  g_object_class_install_property (object_class, PROP_LAYER,
                                   g_param_spec_object ("layer", NULL, NULL,
                                                        GIMP_TYPE_LAYER,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_EDIT_MASK,
                                   g_param_spec_boolean ("edit-mask", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));
}

static void
gimp_canvas_layer_boundary_init (GimpCanvasLayerBoundary *layer_boundary)
{
}

static void
gimp_canvas_layer_boundary_finalize (GObject *object)
{
  GimpCanvasLayerBoundaryPrivate *private = GET_PRIVATE (object);

  if (private->layer)
    g_object_remove_weak_pointer (G_OBJECT (private->layer),
                                  (gpointer) &private->layer);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_canvas_layer_boundary_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  GimpCanvasLayerBoundaryPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LAYER:
      if (private->layer)
        g_object_remove_weak_pointer (G_OBJECT (private->layer),
                                      (gpointer) &private->layer);
      private->layer = g_value_get_object (value); /* don't ref */
      if (private->layer)
        g_object_add_weak_pointer (G_OBJECT (private->layer),
                                   (gpointer) &private->layer);
      break;
    case PROP_EDIT_MASK:
      private->edit_mask = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_layer_boundary_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  GimpCanvasLayerBoundaryPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LAYER:
      g_value_set_object (value, private->layer);
      break;
    case PROP_EDIT_MASK:
      g_value_set_boolean (value, private->edit_mask);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_layer_boundary_draw (GimpCanvasItem *item,
                                 cairo_t        *cr)
{
  GimpCanvasLayerBoundaryPrivate *private = GET_PRIVATE (item);

  if (private->layer)
    GIMP_CANVAS_ITEM_CLASS (parent_class)->draw (item, cr);
}

static cairo_region_t *
gimp_canvas_layer_boundary_get_extents (GimpCanvasItem *item)
{
  GimpCanvasLayerBoundaryPrivate *private = GET_PRIVATE (item);

  if (private->layer)
    return GIMP_CANVAS_ITEM_CLASS (parent_class)->get_extents (item);

  return NULL;
}

static void
gimp_canvas_layer_boundary_stroke (GimpCanvasItem *item,
                                   cairo_t        *cr)
{
  GimpCanvasLayerBoundaryPrivate *private = GET_PRIVATE (item);
  GimpDisplayShell               *shell   = gimp_canvas_item_get_shell (item);

  gimp_canvas_set_layer_style (gimp_canvas_item_get_canvas (item), cr,
                               private->layer,
                               shell->offset_x, shell->offset_y);
  cairo_stroke (cr);
}

GimpCanvasItem *
gimp_canvas_layer_boundary_new (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_CANVAS_LAYER_BOUNDARY,
                       "shell", shell,
                       NULL);
}

void
gimp_canvas_layer_boundary_set_layer (GimpCanvasLayerBoundary *boundary,
                                      GimpLayer               *layer)
{
  GimpCanvasLayerBoundaryPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_LAYER_BOUNDARY (boundary));
  g_return_if_fail (layer == NULL || GIMP_IS_LAYER (layer));

  private = GET_PRIVATE (boundary);

  if (layer && gimp_layer_is_floating_sel (layer))
    {
      GimpDrawable *drawable;

      drawable = gimp_layer_get_floating_sel_drawable (layer);

      if (GIMP_IS_CHANNEL (drawable))
        {
          /*  if the owner drawable is a channel, show no outline  */

          layer = NULL;
        }
      else
        {
          /*  otherwise, set the layer to the owner drawable  */

          layer = GIMP_LAYER (drawable);
        }
    }

  if (layer != private->layer)
    {
      gboolean edit_mask = FALSE;

      gimp_canvas_item_begin_change (GIMP_CANVAS_ITEM (boundary));

      if (layer)
        {
          GimpItem *item = GIMP_ITEM (layer);

          edit_mask = (gimp_layer_get_mask (layer) &&
                       gimp_layer_get_edit_mask (layer));

          g_object_set (boundary,
                        "x",      (gdouble) gimp_item_get_offset_x (item),
                        "y",      (gdouble) gimp_item_get_offset_y (item),
                        "width",  (gdouble) gimp_item_get_width  (item),
                        "height", (gdouble) gimp_item_get_height (item),
                        NULL);
        }

      g_object_set (boundary,
                    "layer",     layer,
                    "edit-mask", edit_mask,
                    NULL);

      gimp_canvas_item_end_change (GIMP_CANVAS_ITEM (boundary));
    }
  else if (layer && layer == private->layer)
    {
      GimpItem *item = GIMP_ITEM (layer);
      gint      lx, ly, lw, lh;
      gdouble   x, y, w ,h;
      gboolean  edit_mask;

      lx = gimp_item_get_offset_x (item);
      ly = gimp_item_get_offset_y (item);
      lw = gimp_item_get_width  (item);
      lh = gimp_item_get_height (item);

      edit_mask = (gimp_layer_get_mask (layer) &&
                   gimp_layer_get_edit_mask (layer));

      g_object_get (boundary,
                    "x",      &x,
                    "y",      &y,
                    "width",  &w,
                    "height", &h,
                    NULL);

      if (lx        != (gint) x ||
          ly        != (gint) y ||
          lw        != (gint) w ||
          lh        != (gint) h ||
          edit_mask != private->edit_mask)
        {
          gimp_canvas_item_begin_change (GIMP_CANVAS_ITEM (boundary));

          g_object_set (boundary,
                        "x",         (gdouble) lx,
                        "y",         (gdouble) ly,
                        "width",     (gdouble) lw,
                        "height",    (gdouble) lh,
                        "edit-mask", edit_mask,
                        NULL);

          gimp_canvas_item_end_change (GIMP_CANVAS_ITEM (boundary));
        }
    }
}
