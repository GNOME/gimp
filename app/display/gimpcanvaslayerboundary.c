/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvaslayerboundary.c
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "core/ligmachannel.h"
#include "core/ligmalayer.h"
#include "core/ligmalayer-floating-selection.h"

#include "ligmacanvas-style.h"
#include "ligmacanvaslayerboundary.h"
#include "ligmadisplayshell.h"


enum
{
  PROP_0,
  PROP_LAYERS,
  PROP_EDIT_MASK
};


typedef struct _LigmaCanvasLayerBoundaryPrivate LigmaCanvasLayerBoundaryPrivate;

struct _LigmaCanvasLayerBoundaryPrivate
{
  GList    *layers;
  gboolean  edit_mask;
};

#define GET_PRIVATE(layer_boundary) \
        ((LigmaCanvasLayerBoundaryPrivate *) ligma_canvas_layer_boundary_get_instance_private ((LigmaCanvasLayerBoundary *) (layer_boundary)))


/*  local function prototypes  */

static void             ligma_canvas_layer_boundary_set_property (GObject        *object,
                                                                 guint           property_id,
                                                                 const GValue   *value,
                                                                 GParamSpec     *pspec);
static void             ligma_canvas_layer_boundary_get_property (GObject        *object,
                                                                 guint           property_id,
                                                                 GValue         *value,
                                                                 GParamSpec     *pspec);
static void             ligma_canvas_layer_boundary_finalize     (GObject        *object);
static void             ligma_canvas_layer_boundary_draw         (LigmaCanvasItem *item,
                                                                 cairo_t        *cr);
static cairo_region_t * ligma_canvas_layer_boundary_get_extents  (LigmaCanvasItem *item);
static void             ligma_canvas_layer_boundary_stroke       (LigmaCanvasItem *item,
                                                                 cairo_t        *cr);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasLayerBoundary, ligma_canvas_layer_boundary,
                            LIGMA_TYPE_CANVAS_RECTANGLE)

#define parent_class ligma_canvas_layer_boundary_parent_class


static void
ligma_canvas_layer_boundary_class_init (LigmaCanvasLayerBoundaryClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = ligma_canvas_layer_boundary_set_property;
  object_class->get_property = ligma_canvas_layer_boundary_get_property;
  object_class->finalize     = ligma_canvas_layer_boundary_finalize;

  item_class->draw           = ligma_canvas_layer_boundary_draw;
  item_class->get_extents    = ligma_canvas_layer_boundary_get_extents;
  item_class->stroke         = ligma_canvas_layer_boundary_stroke;

  g_object_class_install_property (object_class, PROP_LAYERS,
                                   g_param_spec_pointer ("layers", NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_EDIT_MASK,
                                   g_param_spec_boolean ("edit-mask", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
}

static void
ligma_canvas_layer_boundary_init (LigmaCanvasLayerBoundary *layer_boundary)
{
}

static void
ligma_canvas_layer_boundary_finalize (GObject *object)
{
  LigmaCanvasLayerBoundaryPrivate *private = GET_PRIVATE (object);

  if (private->layers)
    {
      GList *iter;

      for (iter = private->layers; iter; iter = iter->next)
        if (iter->data)
          g_object_remove_weak_pointer (G_OBJECT (iter->data),
                                        (gpointer) &iter->data);

      g_list_free (private->layers);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_canvas_layer_boundary_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  LigmaCanvasLayerBoundaryPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LAYERS:
      if (private->layers)
        {
          GList *iter;
          for (iter = private->layers; iter; iter = iter->next)
            {
              if (iter->data)
                g_object_remove_weak_pointer (G_OBJECT (iter->data),
                                              (gpointer) &iter->data);
            }
          g_list_free (private->layers);
        }
      private->layers = g_list_copy (g_value_get_pointer (value));
      if (private->layers)
        {
          GList *iter;
          for (iter = private->layers; iter; iter = iter->next)
            g_object_add_weak_pointer (G_OBJECT (iter->data),
                                       (gpointer) &iter->data);
        }
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
ligma_canvas_layer_boundary_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  LigmaCanvasLayerBoundaryPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LAYERS:
      g_value_set_pointer (value, private->layers);
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
ligma_canvas_layer_boundary_draw (LigmaCanvasItem *item,
                                 cairo_t        *cr)
{
  LigmaCanvasLayerBoundaryPrivate *private = GET_PRIVATE (item);

  if (private->layers)
    LIGMA_CANVAS_ITEM_CLASS (parent_class)->draw (item, cr);
}

static cairo_region_t *
ligma_canvas_layer_boundary_get_extents (LigmaCanvasItem *item)
{
  LigmaCanvasLayerBoundaryPrivate *private = GET_PRIVATE (item);

  if (private->layers)
    return LIGMA_CANVAS_ITEM_CLASS (parent_class)->get_extents (item);

  return NULL;
}

static void
ligma_canvas_layer_boundary_stroke (LigmaCanvasItem *item,
                                   cairo_t        *cr)
{
  LigmaCanvasLayerBoundaryPrivate *private = GET_PRIVATE (item);
  LigmaDisplayShell               *shell   = ligma_canvas_item_get_shell (item);
  GList                          *iter;

  for (iter = private->layers; iter; iter = iter->next)
    {
      ligma_canvas_set_layer_style (ligma_canvas_item_get_canvas (item), cr,
                                   iter->data, shell->offset_x, shell->offset_y);
      cairo_stroke (cr);
    }
}

LigmaCanvasItem *
ligma_canvas_layer_boundary_new (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_CANVAS_LAYER_BOUNDARY,
                       "shell", shell,
                       NULL);
}

void
ligma_canvas_layer_boundary_set_layers (LigmaCanvasLayerBoundary *boundary,
                                       GList                   *layers)
{
  LigmaCanvasLayerBoundaryPrivate *private;
  GList                          *shown_layers;
  GList                          *iter;
  gboolean                        changed;

  g_return_if_fail (LIGMA_IS_CANVAS_LAYER_BOUNDARY (boundary));

  private = GET_PRIVATE (boundary);

  if (g_list_length (layers) == 1 && ligma_layer_is_floating_sel (layers->data))
    {
      LigmaDrawable *drawable;

      drawable = ligma_layer_get_floating_sel_drawable (layers->data);

      if (LIGMA_IS_CHANNEL (drawable))
        {
          /*  if the owner drawable is a channel, show no outline  */

          shown_layers = NULL;
        }
      else
        {
          /*  otherwise, set the layer to the owner drawable  */

          shown_layers = g_list_prepend (NULL, LIGMA_LAYER (drawable));
        }
    }
  else
    {
      shown_layers = g_list_copy (layers);
    }

  changed = (g_list_length (shown_layers) != g_list_length (private->layers));
  if (! changed)
    {
      for (iter = shown_layers; iter; iter = iter->next)
        {
          if (! g_list_find (private->layers, iter->data))
            {
              changed = TRUE;
              break;
            }
        }
    }

  if (changed)
    {
      gboolean edit_mask = FALSE;
      gint     x1        = G_MAXINT;
      gint     x2        = G_MININT;
      gint     y1        = G_MAXINT;
      gint     y2        = G_MININT;

      for (iter = layers; iter; iter = iter->next)
        {
          if (iter->data)
            {
              LigmaItem *item = LIGMA_ITEM (iter->data);

              x1 = MIN (x1, ligma_item_get_offset_x (item));
              y1 = MIN (y1, ligma_item_get_offset_y (item));

              x2 = MAX (x2,
                        ligma_item_get_offset_x (item) +
                        ligma_item_get_width    (item));
              y2 = MAX (y2,
                        ligma_item_get_offset_y (item) +
                        ligma_item_get_height   (item));

              /* We can only edit one layer mask at a time. */
              if (ligma_layer_get_mask (iter->data) &&
                  ligma_layer_get_edit_mask (iter->data))
                edit_mask = TRUE;
            }
        }

      ligma_canvas_item_begin_change (LIGMA_CANVAS_ITEM (boundary));

      if (layers)
        g_object_set (boundary,
                      "x",      (gdouble) x1,
                      "y",      (gdouble) y1,
                      "width",  (gdouble) (x2 - x1),
                      "height", (gdouble) (y2 - y1),
                      NULL);
      g_object_set (boundary,
                    "layers",    layers,
                    "edit-mask", edit_mask,
                    NULL);

      ligma_canvas_item_end_change (LIGMA_CANVAS_ITEM (boundary));
    }
  else if (layers)
    {
      gboolean edit_mask = FALSE;
      gint     x1        = G_MAXINT;
      gint     x2        = G_MININT;
      gint     y1        = G_MAXINT;
      gint     y2        = G_MININT;
      gdouble  x, y, w, h;

      g_object_get (boundary,
                    "x",      &x,
                    "y",      &y,
                    "width",  &w,
                    "height", &h,
                    NULL);

      for (iter = layers; iter; iter = iter->next)
        {
          if (iter->data)
            {
              LigmaItem *item = LIGMA_ITEM (iter->data);

              x1 = MIN (x1, ligma_item_get_offset_x (item));
              y1 = MIN (y1, ligma_item_get_offset_y (item));

              x2 = MAX (x2,
                        ligma_item_get_offset_x (item) +
                        ligma_item_get_width    (item));
              y2 = MAX (y2,
                        ligma_item_get_offset_y (item) +
                        ligma_item_get_height   (item));

              if (ligma_layer_get_mask (iter->data) &&
                  ligma_layer_get_edit_mask (iter->data))
                edit_mask = TRUE;
            }
        }

      if (x1        != (gint) x ||
          x1        != (gint) y ||
          (x2 - x1) != (gint) w ||
          (y2 - y1) != (gint) h ||
          edit_mask != private->edit_mask)
        {
          ligma_canvas_item_begin_change (LIGMA_CANVAS_ITEM (boundary));

          g_object_set (boundary,
                        "x",      (gdouble) x1,
                        "y",      (gdouble) y1,
                        "width",  (gdouble) (x2 - x1),
                        "height", (gdouble) (y2 - y1),
                        "edit-mask", edit_mask,
                        NULL);

          ligma_canvas_item_end_change (LIGMA_CANVAS_ITEM (boundary));
        }
    }
  g_list_free (shown_layers);
}
