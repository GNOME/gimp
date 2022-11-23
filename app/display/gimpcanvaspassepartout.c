/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvaspassepartout.c
 * Copyright (C) 2010 Sven Neumann <sven@ligma.org>
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

#include "display-types.h"

#include "ligmacanvas-style.h"
#include "ligmacanvasitem-utils.h"
#include "ligmacanvaspassepartout.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-scale.h"


enum
{
  PROP_0,
  PROP_OPACITY,
};


typedef struct _LigmaCanvasPassePartoutPrivate LigmaCanvasPassePartoutPrivate;

struct _LigmaCanvasPassePartoutPrivate
{
  gdouble opacity;
};

#define GET_PRIVATE(item) \
        ((LigmaCanvasPassePartoutPrivate *) ligma_canvas_passe_partout_get_instance_private ((LigmaCanvasPassePartout *) (item)))


/*  local function prototypes  */

static void             ligma_canvas_passe_partout_set_property (GObject        *object,
                                                                guint           property_id,
                                                                const GValue   *value,
                                                                GParamSpec     *pspec);
static void             ligma_canvas_passe_partout_get_property (GObject        *object,
                                                                guint           property_id,
                                                                GValue         *value,
                                                                GParamSpec     *pspec);
static void             ligma_canvas_passe_partout_draw         (LigmaCanvasItem *item,
                                                                cairo_t        *cr);
static cairo_region_t * ligma_canvas_passe_partout_get_extents  (LigmaCanvasItem *item);
static void             ligma_canvas_passe_partout_fill         (LigmaCanvasItem *item,
                                                                cairo_t        *cr);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasPassePartout, ligma_canvas_passe_partout,
                            LIGMA_TYPE_CANVAS_RECTANGLE)

#define parent_class ligma_canvas_passe_partout_parent_class


static void
ligma_canvas_passe_partout_class_init (LigmaCanvasPassePartoutClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = ligma_canvas_passe_partout_set_property;
  object_class->get_property = ligma_canvas_passe_partout_get_property;

  item_class->draw           = ligma_canvas_passe_partout_draw;
  item_class->get_extents    = ligma_canvas_passe_partout_get_extents;
  item_class->fill           = ligma_canvas_passe_partout_fill;

  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity", NULL, NULL,
                                                        0.0,
                                                        1.0, 0.5,
                                                        LIGMA_PARAM_READWRITE));
}

static void
ligma_canvas_passe_partout_init (LigmaCanvasPassePartout *passe_partout)
{
}

static void
ligma_canvas_passe_partout_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  LigmaCanvasPassePartoutPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_OPACITY:
      priv->opacity = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_passe_partout_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  LigmaCanvasPassePartoutPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_OPACITY:
      g_value_set_double (value, priv->opacity);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_passe_partout_draw (LigmaCanvasItem *item,
                                cairo_t        *cr)
{
  LigmaDisplayShell *shell = ligma_canvas_item_get_shell (item);
  gint              x, y;
  gint              w, h;

  if (! ligma_display_shell_get_infinite_canvas (shell))
    {
      x = -shell->offset_x;
      y = -shell->offset_y;

      ligma_display_shell_scale_get_image_size (shell, &w, &h);
    }
  else
    {
      ligma_canvas_item_untransform_viewport (item, &x, &y, &w, &h);
    }

  cairo_rectangle (cr, x, y, w, h);

  LIGMA_CANVAS_ITEM_CLASS (parent_class)->draw (item, cr);
}

static cairo_region_t *
ligma_canvas_passe_partout_get_extents (LigmaCanvasItem *item)
{
  LigmaDisplayShell      *shell = ligma_canvas_item_get_shell (item);
  cairo_rectangle_int_t  rectangle;

  if (! ligma_display_shell_get_infinite_canvas (shell))
    {
      cairo_region_t *inner;
      cairo_region_t *outer;

      rectangle.x = - shell->offset_x;
      rectangle.y = - shell->offset_y;
      ligma_display_shell_scale_get_image_size (shell,
                                               &rectangle.width,
                                               &rectangle.height);

      outer = cairo_region_create_rectangle (&rectangle);

      inner = LIGMA_CANVAS_ITEM_CLASS (parent_class)->get_extents (item);

      cairo_region_xor (outer, inner);

      cairo_region_destroy (inner);

      return outer;
    }
  else
    {
      ligma_canvas_item_untransform_viewport (item,
                                             &rectangle.x,
                                             &rectangle.y,
                                             &rectangle.width,
                                             &rectangle.height);

      return cairo_region_create_rectangle (&rectangle);
    }
}

static void
ligma_canvas_passe_partout_fill (LigmaCanvasItem *item,
                                cairo_t        *cr)
{
  LigmaCanvasPassePartoutPrivate *priv = GET_PRIVATE (item);

  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_clip (cr);

  ligma_canvas_set_passe_partout_style (ligma_canvas_item_get_canvas (item), cr);
  cairo_paint_with_alpha (cr, priv->opacity);
}

LigmaCanvasItem *
ligma_canvas_passe_partout_new (LigmaDisplayShell *shell,
                               gdouble           x,
                               gdouble           y,
                               gdouble           width,
                               gdouble           height)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_CANVAS_PASSE_PARTOUT,
                       "shell", shell,
                       "x",      x,
                       "y",      y,
                       "width",  width,
                       "height", height,
                       "filled", TRUE,
                       NULL);
}
