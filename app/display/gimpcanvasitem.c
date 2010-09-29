/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasitem.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "libgimpmath/gimpmath.h"

#include "gimpcanvasitem.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-style.h"


enum
{
  PROP_0,
  PROP_LINE_CAP,
  PROP_HIGHLIGHT
};


typedef struct _GimpCanvasItemPrivate GimpCanvasItemPrivate;

struct _GimpCanvasItemPrivate
{
  cairo_line_cap_t line_cap;
  gboolean         highlight;
  gint             suspend_stroking;
  gint             suspend_filling;
};

#define GET_PRIVATE(item) \
        G_TYPE_INSTANCE_GET_PRIVATE (item, \
                                     GIMP_TYPE_CANVAS_ITEM, \
                                     GimpCanvasItemPrivate)


/*  local function prototypes  */

static void        gimp_canvas_item_set_property     (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void        gimp_canvas_item_get_property     (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static void        gimp_canvas_item_real_draw        (GimpCanvasItem   *item,
                                                      GimpDisplayShell *shell,
                                                      cairo_t          *cr);
static GdkRegion * gimp_canvas_item_real_get_extents (GimpCanvasItem   *item,
                                                      GimpDisplayShell *shell);
static void        gimp_canvas_item_real_stroke      (GimpCanvasItem   *item,
                                                      GimpDisplayShell *shell,
                                                      cairo_t          *cr);
static void        gimp_canvas_item_real_fill        (GimpCanvasItem   *item,
                                                      GimpDisplayShell *shell,
                                                      cairo_t          *cr);


G_DEFINE_TYPE (GimpCanvasItem, gimp_canvas_item,
               GIMP_TYPE_OBJECT)

#define parent_class gimp_canvas_item_parent_class


static void
gimp_canvas_item_class_init (GimpCanvasItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_canvas_item_set_property;
  object_class->get_property = gimp_canvas_item_get_property;

  klass->draw                = gimp_canvas_item_real_draw;
  klass->get_extents         = gimp_canvas_item_real_get_extents;
  klass->stroke              = gimp_canvas_item_real_stroke;
  klass->fill                = gimp_canvas_item_real_fill;

  g_object_class_install_property (object_class, PROP_LINE_CAP,
                                   g_param_spec_int ("line-cap",
                                                     NULL, NULL,
                                                     CAIRO_LINE_CAP_BUTT,
                                                     CAIRO_LINE_CAP_SQUARE,
                                                     CAIRO_LINE_CAP_ROUND,
                                                     GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HIGHLIGHT,
                                   g_param_spec_boolean ("highlight",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GimpCanvasItemPrivate));
}

static void
gimp_canvas_item_init (GimpCanvasItem *item)
{
  GimpCanvasItemPrivate *private = GET_PRIVATE (item);

  private->line_cap         = CAIRO_LINE_CAP_ROUND;
  private->highlight        = FALSE;
  private->suspend_stroking = 0;
  private->suspend_filling  = 0;
}

static void
gimp_canvas_item_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpCanvasItemPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LINE_CAP:
      private->line_cap = g_value_get_int (value);
      break;
    case PROP_HIGHLIGHT:
      private->highlight = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_item_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpCanvasItemPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LINE_CAP:
      g_value_set_int (value, private->line_cap);
      break;
    case PROP_HIGHLIGHT:
      g_value_set_boolean (value, private->highlight);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_item_real_draw (GimpCanvasItem   *item,
                            GimpDisplayShell *shell,
                            cairo_t          *cr)
{
  g_warn_if_reached ();
}

static GdkRegion *
gimp_canvas_item_real_get_extents (GimpCanvasItem   *item,
                                   GimpDisplayShell *shell)
{
  return NULL;
}

static void
gimp_canvas_item_real_stroke (GimpCanvasItem   *item,
                              GimpDisplayShell *shell,
                              cairo_t          *cr)
{
  GimpCanvasItemPrivate *private = GET_PRIVATE (item);

  cairo_set_line_cap (cr, private->line_cap);

  gimp_display_shell_set_tool_bg_style (shell, cr);
  cairo_stroke_preserve (cr);

  gimp_display_shell_set_tool_fg_style (shell, cr, private->highlight);
  cairo_stroke (cr);
}

static void
gimp_canvas_item_real_fill (GimpCanvasItem   *item,
                            GimpDisplayShell *shell,
                            cairo_t          *cr)
{
  GimpCanvasItemPrivate *private = GET_PRIVATE (item);

  gimp_display_shell_set_tool_bg_style (shell, cr);
  cairo_set_line_width (cr, 2.0);
  cairo_stroke_preserve (cr);

  gimp_display_shell_set_tool_fg_style (shell, cr, private->highlight);
  cairo_fill (cr);
}


/*  public functions  */

void
gimp_canvas_item_draw (GimpCanvasItem   *item,
                       GimpDisplayShell *shell,
                       cairo_t          *cr)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  cairo_save (cr);

  GIMP_CANVAS_ITEM_GET_CLASS (item)->draw (item, shell, cr);

  cairo_restore (cr);
}

GdkRegion *
gimp_canvas_item_get_extents (GimpCanvasItem   *item,
                              GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), NULL);
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return GIMP_CANVAS_ITEM_GET_CLASS (item)->get_extents (item, shell);
}

void
gimp_canvas_item_set_line_cap (GimpCanvasItem   *item,
                               cairo_line_cap_t  line_cap)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  g_object_set (item,
                "line-cap", line_cap,
                NULL);
}

void
gimp_canvas_item_set_highlight (GimpCanvasItem *item,
                                gboolean        highlight)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  g_object_set (item,
                "highlight", highlight,
                NULL);
}

void
gimp_canvas_item_suspend_stroking (GimpCanvasItem *item)
{
  GimpCanvasItemPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  private = GET_PRIVATE (item);

  private->suspend_stroking++;
}

void
gimp_canvas_item_resume_stroking (GimpCanvasItem *item)
{
  GimpCanvasItemPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  private = GET_PRIVATE (item);

  g_return_if_fail (private->suspend_stroking > 0);

  private->suspend_stroking--;
}

void
gimp_canvas_item_suspend_filling (GimpCanvasItem *item)
{
  GimpCanvasItemPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  private = GET_PRIVATE (item);

  private->suspend_filling++;
}

void
gimp_canvas_item_resume_filling (GimpCanvasItem *item)
{
  GimpCanvasItemPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  private = GET_PRIVATE (item);

  g_return_if_fail (private->suspend_filling > 0);

  private->suspend_filling--;
}


/*  protected functions  */

void
_gimp_canvas_item_stroke (GimpCanvasItem   *item,
                          GimpDisplayShell *shell,
                          cairo_t          *cr)
{
  GimpCanvasItemPrivate *private = GET_PRIVATE (item);

  if (private->suspend_filling > 0)
    g_warning ("_gimp_canvas_item_stroke() on an item that is in a filling group");

  if (private->suspend_stroking == 0)
    {
      GIMP_CANVAS_ITEM_GET_CLASS (item)->stroke (item, shell, cr);
    }
  else
    {
      cairo_new_sub_path (cr);
    }
}

void
_gimp_canvas_item_fill (GimpCanvasItem   *item,
                        GimpDisplayShell *shell,
                        cairo_t          *cr)
{
  GimpCanvasItemPrivate *private = GET_PRIVATE (item);

  if (private->suspend_stroking > 0)
    g_warning ("_gimp_canvas_item_fill() on an item that is in a stroking group");

  if (private->suspend_filling == 0)
    {
      GIMP_CANVAS_ITEM_GET_CLASS (item)->fill (item, shell, cr);
    }
  else
    {
      cairo_new_sub_path (cr);
    }
}
