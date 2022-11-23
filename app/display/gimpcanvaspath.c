/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvaspath.c
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

#include "core/ligmabezierdesc.h"
#include "core/ligmaparamspecs.h"

#include "ligmacanvas-style.h"
#include "ligmacanvaspath.h"
#include "ligmadisplayshell.h"


enum
{
  PROP_0,
  PROP_PATH,
  PROP_X,
  PROP_Y,
  PROP_FILLED,
  PROP_PATH_STYLE
};


typedef struct _LigmaCanvasPathPrivate LigmaCanvasPathPrivate;

struct _LigmaCanvasPathPrivate
{
  cairo_path_t *path;
  gdouble       x;
  gdouble       y;
  gboolean      filled;
  LigmaPathStyle path_style;
};

#define GET_PRIVATE(path) \
        ((LigmaCanvasPathPrivate *) ligma_canvas_path_get_instance_private ((LigmaCanvasPath *) (path)))

/*  local function prototypes  */

static void             ligma_canvas_path_finalize     (GObject        *object);
static void             ligma_canvas_path_set_property (GObject        *object,
                                                       guint           property_id,
                                                       const GValue   *value,
                                                       GParamSpec     *pspec);
static void             ligma_canvas_path_get_property (GObject        *object,
                                                       guint           property_id,
                                                       GValue         *value,
                                                       GParamSpec     *pspec);
static void             ligma_canvas_path_draw         (LigmaCanvasItem *item,
                                                       cairo_t        *cr);
static cairo_region_t * ligma_canvas_path_get_extents  (LigmaCanvasItem *item);
static void             ligma_canvas_path_stroke       (LigmaCanvasItem *item,
                                                       cairo_t        *cr);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasPath, ligma_canvas_path,
                            LIGMA_TYPE_CANVAS_ITEM)

#define parent_class ligma_canvas_path_parent_class


static void
ligma_canvas_path_class_init (LigmaCanvasPathClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->finalize     = ligma_canvas_path_finalize;
  object_class->set_property = ligma_canvas_path_set_property;
  object_class->get_property = ligma_canvas_path_get_property;

  item_class->draw           = ligma_canvas_path_draw;
  item_class->get_extents    = ligma_canvas_path_get_extents;
  item_class->stroke         = ligma_canvas_path_stroke;

  g_object_class_install_property (object_class, PROP_PATH,
                                   g_param_spec_boxed ("path", NULL, NULL,
                                                       LIGMA_TYPE_BEZIER_DESC,
                                                       LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_double ("x", NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_double ("y", NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_FILLED,
                                   g_param_spec_boolean ("filled", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));


  g_object_class_install_property (object_class, PROP_PATH_STYLE,
                                   g_param_spec_enum ("path-style", NULL, NULL,
                                                      LIGMA_TYPE_PATH_STYLE,
                                                      LIGMA_PATH_STYLE_DEFAULT,
                                                      LIGMA_PARAM_READWRITE));
}

static void
ligma_canvas_path_init (LigmaCanvasPath *path)
{
}

static void
ligma_canvas_path_finalize (GObject *object)
{
  LigmaCanvasPathPrivate *private = GET_PRIVATE (object);

  if (private->path)
    {
      ligma_bezier_desc_free (private->path);
      private->path = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_canvas_path_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaCanvasPathPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_PATH:
      if (private->path)
        ligma_bezier_desc_free (private->path);
      private->path = g_value_dup_boxed (value);
      break;
    case PROP_X:
      private->x = g_value_get_double (value);
      break;
    case PROP_Y:
      private->y = g_value_get_double (value);
      break;
    case PROP_FILLED:
      private->filled = g_value_get_boolean (value);
      break;
    case PROP_PATH_STYLE:
      private->path_style = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_path_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaCanvasPathPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_PATH:
      g_value_set_boxed (value, private->path);
      break;
    case PROP_X:
      g_value_set_double (value, private->x);
      break;
    case PROP_Y:
      g_value_set_double (value, private->y);
      break;
    case PROP_FILLED:
      g_value_set_boolean (value, private->filled);
      break;
    case PROP_PATH_STYLE:
      g_value_set_enum (value, private->path_style);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_path_draw (LigmaCanvasItem *item,
                       cairo_t        *cr)
{
  LigmaCanvasPathPrivate *private = GET_PRIVATE (item);

  if (private->path)
    {
      cairo_save (cr);
      ligma_canvas_item_transform (item, cr);
      cairo_translate (cr, private->x, private->y);

      cairo_append_path (cr, private->path);
      cairo_restore (cr);

      if (private->filled)
        _ligma_canvas_item_fill (item, cr);
      else
        _ligma_canvas_item_stroke (item, cr);
    }
}

static cairo_region_t *
ligma_canvas_path_get_extents (LigmaCanvasItem *item)
{
  LigmaCanvasPathPrivate *private = GET_PRIVATE (item);
  GtkWidget             *canvas  = ligma_canvas_item_get_canvas (item);

  if (private->path && gtk_widget_get_realized (canvas))
    {
      cairo_surface_t       *surface;
      cairo_t               *cr;
      cairo_rectangle_int_t  rectangle;
      gdouble                x1, y1, x2, y2;

      surface = cairo_recording_surface_create (CAIRO_CONTENT_COLOR, NULL);
      cr = cairo_create (surface);
      cairo_surface_destroy (surface);

      cairo_save (cr);
      ligma_canvas_item_transform (item, cr);
      cairo_translate (cr, private->x, private->y);

      cairo_append_path (cr, private->path);
      cairo_restore (cr);

      cairo_path_extents (cr, &x1, &y1, &x2, &y2);

      cairo_destroy (cr);

      if (private->filled)
        {
          rectangle.x      = floor (x1 - 1.0);
          rectangle.y      = floor (y1 - 1.0);
          rectangle.width  = ceil (x2 + 1.0) - rectangle.x;
          rectangle.height = ceil (y2 + 1.0) - rectangle.y;
        }
      else
        {
          rectangle.x      = floor (x1 - 1.5);
          rectangle.y      = floor (y1 - 1.5);
          rectangle.width  = ceil (x2 + 1.5) - rectangle.x;
          rectangle.height = ceil (y2 + 1.5) - rectangle.y;
        }

      return cairo_region_create_rectangle (&rectangle);
    }

  return NULL;
}

static void
ligma_canvas_path_stroke (LigmaCanvasItem *item,
                         cairo_t        *cr)
{
  LigmaCanvasPathPrivate *private = GET_PRIVATE (item);
  GtkWidget             *canvas  = ligma_canvas_item_get_canvas (item);
  gboolean               active;

  switch (private->path_style)
    {
    case LIGMA_PATH_STYLE_VECTORS:
      active = ligma_canvas_item_get_highlight (item);

      ligma_canvas_set_vectors_bg_style (canvas, cr, active);
      cairo_stroke_preserve (cr);

      ligma_canvas_set_vectors_fg_style (canvas, cr, active);
      cairo_stroke (cr);
      break;

    case LIGMA_PATH_STYLE_OUTLINE:
      ligma_canvas_set_outline_bg_style (canvas, cr);
      cairo_stroke_preserve (cr);

      ligma_canvas_set_outline_fg_style (canvas, cr);
      cairo_stroke (cr);
      break;

    case LIGMA_PATH_STYLE_DEFAULT:
      LIGMA_CANVAS_ITEM_CLASS (parent_class)->stroke (item, cr);
      break;
    }
}

LigmaCanvasItem *
ligma_canvas_path_new (LigmaDisplayShell     *shell,
                      const LigmaBezierDesc *bezier,
                      gdouble               x,
                      gdouble               y,
                      gboolean              filled,
                      LigmaPathStyle         style)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_CANVAS_PATH,
                       "shell",      shell,
                       "path",       bezier,
                       "x",          x,
                       "y",          y,
                       "filled",     filled,
                       "path-style", style,
                       NULL);
}

void
ligma_canvas_path_set (LigmaCanvasItem       *path,
                      const LigmaBezierDesc *bezier)
{
  g_return_if_fail (LIGMA_IS_CANVAS_PATH (path));

  ligma_canvas_item_begin_change (path);

  g_object_set (path,
                "path", bezier,
                NULL);

  ligma_canvas_item_end_change (path);
}
