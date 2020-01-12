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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "gimpcanvas-style.h"
#include "gimpcanvasitem.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-transform.h"


enum
{
  PROP_0,
  PROP_SHELL,
  PROP_VISIBLE,
  PROP_LINE_CAP,
  PROP_HIGHLIGHT
};

enum
{
  UPDATE,
  LAST_SIGNAL
};


struct _GimpCanvasItemPrivate
{
  GimpDisplayShell *shell;
  gboolean          visible;
  cairo_line_cap_t  line_cap;
  gboolean          highlight;
  gint              suspend_stroking;
  gint              suspend_filling;
  gint              change_count;
  cairo_region_t   *change_region;
};


/*  local function prototypes  */

static void             gimp_canvas_item_dispose          (GObject         *object);
static void             gimp_canvas_item_constructed      (GObject         *object);
static void             gimp_canvas_item_set_property     (GObject         *object,
                                                           guint            property_id,
                                                           const GValue    *value,
                                                           GParamSpec      *pspec);
static void             gimp_canvas_item_get_property     (GObject         *object,
                                                           guint            property_id,
                                                           GValue          *value,
                                                           GParamSpec      *pspec);
static void  gimp_canvas_item_dispatch_properties_changed (GObject         *object,
                                                           guint            n_pspecs,
                                                           GParamSpec     **pspecs);

static void             gimp_canvas_item_real_draw        (GimpCanvasItem  *item,
                                                           cairo_t         *cr);
static cairo_region_t * gimp_canvas_item_real_get_extents (GimpCanvasItem  *item);
static void             gimp_canvas_item_real_stroke      (GimpCanvasItem  *item,
                                                           cairo_t         *cr);
static void             gimp_canvas_item_real_fill        (GimpCanvasItem  *item,
                                                           cairo_t         *cr);
static gboolean         gimp_canvas_item_real_hit         (GimpCanvasItem  *item,
                                                           gdouble          x,
                                                           gdouble          y);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCanvasItem, gimp_canvas_item, GIMP_TYPE_OBJECT)

#define parent_class gimp_canvas_item_parent_class

static guint item_signals[LAST_SIGNAL] = { 0 };


static void
gimp_canvas_item_class_init (GimpCanvasItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose                     = gimp_canvas_item_dispose;
  object_class->constructed                 = gimp_canvas_item_constructed;
  object_class->set_property                = gimp_canvas_item_set_property;
  object_class->get_property                = gimp_canvas_item_get_property;
  object_class->dispatch_properties_changed = gimp_canvas_item_dispatch_properties_changed;

  klass->update                             = NULL;
  klass->draw                               = gimp_canvas_item_real_draw;
  klass->get_extents                        = gimp_canvas_item_real_get_extents;
  klass->stroke                             = gimp_canvas_item_real_stroke;
  klass->fill                               = gimp_canvas_item_real_fill;
  klass->hit                                = gimp_canvas_item_real_hit;

  item_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpCanvasItemClass, update),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  g_object_class_install_property (object_class, PROP_SHELL,
                                   g_param_spec_object ("shell",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DISPLAY_SHELL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_VISIBLE,
                                   g_param_spec_boolean ("visible",
                                                         NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE));

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
}

static void
gimp_canvas_item_init (GimpCanvasItem *item)
{
  GimpCanvasItemPrivate *private;

  item->private = gimp_canvas_item_get_instance_private (item);
  private = item->private;

  private->shell            = NULL;
  private->visible          = TRUE;
  private->line_cap         = CAIRO_LINE_CAP_ROUND;
  private->highlight        = FALSE;
  private->suspend_stroking = 0;
  private->suspend_filling  = 0;
  private->change_count     = 1; /* avoid emissions during construction */
  private->change_region    = NULL;
}

static void
gimp_canvas_item_constructed (GObject *object)
{
  GimpCanvasItem *item = GIMP_CANVAS_ITEM (object);

  gimp_assert (GIMP_IS_DISPLAY_SHELL (item->private->shell));

  item->private->change_count = 0; /* undo hack from init() */

  G_OBJECT_CLASS (parent_class)->constructed (object);
}

static void
gimp_canvas_item_dispose (GObject *object)
{
  GimpCanvasItem *item = GIMP_CANVAS_ITEM (object);

  item->private->change_count++; /* avoid emissions during destruction */

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_canvas_item_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpCanvasItem        *item    = GIMP_CANVAS_ITEM (object);
  GimpCanvasItemPrivate *private = item->private;

  switch (property_id)
    {
    case PROP_SHELL:
      private->shell = g_value_get_object (value); /* don't ref */
      break;
    case PROP_VISIBLE:
      private->visible = g_value_get_boolean (value);
      break;
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
  GimpCanvasItem        *item    = GIMP_CANVAS_ITEM (object);
  GimpCanvasItemPrivate *private = item->private;

  switch (property_id)
    {
    case PROP_SHELL:
      g_value_set_object (value, private->shell);
      break;
    case PROP_VISIBLE:
      g_value_set_boolean (value, private->visible);
      break;
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
gimp_canvas_item_dispatch_properties_changed (GObject     *object,
                                              guint        n_pspecs,
                                              GParamSpec **pspecs)
{
  GimpCanvasItem *item = GIMP_CANVAS_ITEM (object);

  G_OBJECT_CLASS (parent_class)->dispatch_properties_changed (object,
                                                              n_pspecs,
                                                              pspecs);

  if (_gimp_canvas_item_needs_update (item))
    {
      cairo_region_t *region = gimp_canvas_item_get_extents (item);

      if (region)
        {
          g_signal_emit (object, item_signals[UPDATE], 0,
                         region);
          cairo_region_destroy (region);
        }
    }
}

static void
gimp_canvas_item_real_draw (GimpCanvasItem *item,
                            cairo_t        *cr)
{
  g_warn_if_reached ();
}

static cairo_region_t *
gimp_canvas_item_real_get_extents (GimpCanvasItem *item)
{
  return NULL;
}

static void
gimp_canvas_item_real_stroke (GimpCanvasItem *item,
                              cairo_t        *cr)
{
  cairo_set_line_cap (cr, item->private->line_cap);

  gimp_canvas_set_tool_bg_style (gimp_canvas_item_get_canvas (item), cr);
  cairo_stroke_preserve (cr);

  gimp_canvas_set_tool_fg_style (gimp_canvas_item_get_canvas (item), cr,
                                 item->private->highlight);
  cairo_stroke (cr);
}

static void
gimp_canvas_item_real_fill (GimpCanvasItem *item,
                            cairo_t        *cr)
{
  gimp_canvas_set_tool_bg_style (gimp_canvas_item_get_canvas (item), cr);
  cairo_set_line_width (cr, 2.0);
  cairo_stroke_preserve (cr);

  gimp_canvas_set_tool_fg_style (gimp_canvas_item_get_canvas (item), cr,
                                 item->private->highlight);
  cairo_fill (cr);
}

static gboolean
gimp_canvas_item_real_hit (GimpCanvasItem *item,
                           gdouble         x,
                           gdouble         y)
{
  return FALSE;
}


/*  public functions  */

GimpDisplayShell *
gimp_canvas_item_get_shell (GimpCanvasItem *item)
{
  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), NULL);

  return item->private->shell;
}

GimpImage *
gimp_canvas_item_get_image (GimpCanvasItem *item)
{
  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), NULL);

  return gimp_display_get_image (item->private->shell->display);
}

GtkWidget *
gimp_canvas_item_get_canvas (GimpCanvasItem *item)
{
  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), NULL);

  return item->private->shell->canvas;
}

void
gimp_canvas_item_draw (GimpCanvasItem *item,
                       cairo_t        *cr)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));
  g_return_if_fail (cr != NULL);

  if (item->private->visible)
    {
      cairo_save (cr);
      GIMP_CANVAS_ITEM_GET_CLASS (item)->draw (item, cr);
      cairo_restore (cr);
    }
}

cairo_region_t *
gimp_canvas_item_get_extents (GimpCanvasItem *item)
{
  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), NULL);

  if (item->private->visible)
    return GIMP_CANVAS_ITEM_GET_CLASS (item)->get_extents (item);

  return NULL;
}

gboolean
gimp_canvas_item_hit (GimpCanvasItem   *item,
                      gdouble           x,
                      gdouble           y)
{
  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), FALSE);

  if (item->private->visible)
    return GIMP_CANVAS_ITEM_GET_CLASS (item)->hit (item, x, y);

  return FALSE;
}

void
gimp_canvas_item_set_visible (GimpCanvasItem *item,
                              gboolean        visible)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  if (item->private->visible != visible)
    {
      gimp_canvas_item_begin_change (item);
      g_object_set (G_OBJECT (item),
                    "visible", visible,
                    NULL);
      gimp_canvas_item_end_change (item);
    }
}

gboolean
gimp_canvas_item_get_visible (GimpCanvasItem *item)
{
  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), FALSE);

  return item->private->visible;
}

void
gimp_canvas_item_set_line_cap (GimpCanvasItem   *item,
                               cairo_line_cap_t  line_cap)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  if (item->private->line_cap != line_cap)
    {
      gimp_canvas_item_begin_change (item);
      g_object_set (G_OBJECT (item),
                    "line-cap", line_cap,
                    NULL);
      gimp_canvas_item_end_change (item);
    }
}

void
gimp_canvas_item_set_highlight (GimpCanvasItem *item,
                                gboolean        highlight)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  if (item->private->highlight != highlight)
    {
      g_object_set (G_OBJECT (item),
                    "highlight", highlight,
                    NULL);
    }
}

gboolean
gimp_canvas_item_get_highlight (GimpCanvasItem *item)
{
  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), FALSE);

  return item->private->highlight;
}

void
gimp_canvas_item_begin_change (GimpCanvasItem *item)
{
  GimpCanvasItemPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  private = item->private;

  private->change_count++;

  if (private->change_count == 1 &&
      g_signal_has_handler_pending (item, item_signals[UPDATE], 0, FALSE))
    {
      private->change_region = gimp_canvas_item_get_extents (item);
    }
}

void
gimp_canvas_item_end_change (GimpCanvasItem *item)
{
  GimpCanvasItemPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  private = item->private;

  g_return_if_fail (private->change_count > 0);

  private->change_count--;

  if (private->change_count == 0)
    {
      if (g_signal_has_handler_pending (item, item_signals[UPDATE], 0, FALSE))
        {
          cairo_region_t *region = gimp_canvas_item_get_extents (item);

          if (! region)
            {
              region = private->change_region;
            }
          else if (private->change_region)
            {
              cairo_region_union (region, private->change_region);
              cairo_region_destroy (private->change_region);
            }

          private->change_region = NULL;

          if (region)
            {
              g_signal_emit (item, item_signals[UPDATE], 0,
                             region);
              cairo_region_destroy (region);
            }
        }
      else
        {
          g_clear_pointer (&private->change_region, cairo_region_destroy);
        }
    }
}

void
gimp_canvas_item_suspend_stroking (GimpCanvasItem *item)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  item->private->suspend_stroking++;
}

void
gimp_canvas_item_resume_stroking (GimpCanvasItem *item)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  g_return_if_fail (item->private->suspend_stroking > 0);

  item->private->suspend_stroking--;
}

void
gimp_canvas_item_suspend_filling (GimpCanvasItem *item)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  item->private->suspend_filling++;
}

void
gimp_canvas_item_resume_filling (GimpCanvasItem *item)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  g_return_if_fail (item->private->suspend_filling > 0);

  item->private->suspend_filling--;
}

void
gimp_canvas_item_transform (GimpCanvasItem *item,
                            cairo_t        *cr)
{
  GimpCanvasItemPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));
  g_return_if_fail (cr != NULL);

  private = item->private;

  cairo_translate (cr, -private->shell->offset_x, -private->shell->offset_y);
  cairo_scale (cr, private->shell->scale_x, private->shell->scale_y);
}

void
gimp_canvas_item_transform_xy (GimpCanvasItem *item,
                               gdouble         x,
                               gdouble         y,
                               gint           *tx,
                               gint           *ty)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  gimp_display_shell_zoom_xy (item->private->shell, x, y, tx, ty);
}

void
gimp_canvas_item_transform_xy_f (GimpCanvasItem *item,
                                 gdouble         x,
                                 gdouble         y,
                                 gdouble        *tx,
                                 gdouble        *ty)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  gimp_display_shell_zoom_xy_f (item->private->shell, x, y, tx, ty);
}

/**
 * gimp_canvas_item_transform_distance:
 * @item: a #GimpCanvasItem
 * @x1:   start point X in image coordinates
 * @y1:   start point Y in image coordinates
 * @x2:   end point X in image coordinates
 * @y2:   end point Y in image coordinates
 *
 * If you just need to compare distances, consider to use
 * gimp_canvas_item_transform_distance_square() instead.
 *
 * Returns: the distance between the given points in display coordinates
 **/
gdouble
gimp_canvas_item_transform_distance (GimpCanvasItem *item,
                                     gdouble         x1,
                                     gdouble         y1,
                                     gdouble         x2,
                                     gdouble         y2)
{
  return sqrt (gimp_canvas_item_transform_distance_square (item,
                                                           x1, y1, x2, y2));
}

/**
 * gimp_canvas_item_transform_distance_square:
 * @item: a #GimpCanvasItem
 * @x1:   start point X in image coordinates
 * @y1:   start point Y in image coordinates
 * @x2:   end point X in image coordinates
 * @y2:   end point Y in image coordinates
 *
 * This function is more effective than
 * gimp_canvas_item_transform_distance() as it doesn't perform a
 * sqrt(). Use this if you just need to compare distances.
 *
 * Returns: the square of the distance between the given points in
 *          display coordinates
 **/
gdouble
gimp_canvas_item_transform_distance_square (GimpCanvasItem   *item,
                                            gdouble           x1,
                                            gdouble           y1,
                                            gdouble           x2,
                                            gdouble           y2)
{
  gdouble tx1, ty1;
  gdouble tx2, ty2;

  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), 0.0);

  gimp_canvas_item_transform_xy_f (item, x1, y1, &tx1, &ty1);
  gimp_canvas_item_transform_xy_f (item, x2, y2, &tx2, &ty2);

  return SQR (tx2 - tx1) + SQR (ty2 - ty1);
}

void
gimp_canvas_item_untransform_viewport (GimpCanvasItem *item,
                                       gint           *x,
                                       gint           *y,
                                       gint           *w,
                                       gint           *h)
{
  GimpDisplayShell *shell;
  gdouble           x1, y1;
  gdouble           x2, y2;

  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  shell = item->private->shell;

  gimp_display_shell_unrotate_bounds (shell,
                                      0.0,               0.0,
                                      shell->disp_width, shell->disp_height,
                                      &x1,               &y1,
                                      &x2,               &y2);

  *x = floor (x1);
  *y = floor (y1);
  *w = ceil  (x2) - *x;
  *h = ceil  (y2) - *y;
}


/*  protected functions  */

void
_gimp_canvas_item_update (GimpCanvasItem *item,
                          cairo_region_t *region)
{
  g_signal_emit (item, item_signals[UPDATE], 0,
                 region);
}

gboolean
_gimp_canvas_item_needs_update (GimpCanvasItem *item)
{
  return (item->private->change_count == 0 &&
          g_signal_has_handler_pending (item, item_signals[UPDATE], 0, FALSE));
}

void
_gimp_canvas_item_stroke (GimpCanvasItem *item,
                          cairo_t        *cr)
{
  if (item->private->suspend_filling > 0)
    g_warning ("_gimp_canvas_item_stroke() on an item that is in a filling group");

  if (item->private->suspend_stroking == 0)
    {
      GIMP_CANVAS_ITEM_GET_CLASS (item)->stroke (item, cr);
    }
  else
    {
      cairo_new_sub_path (cr);
    }
}

void
_gimp_canvas_item_fill (GimpCanvasItem *item,
                        cairo_t        *cr)
{
  if (item->private->suspend_stroking > 0)
    g_warning ("_gimp_canvas_item_fill() on an item that is in a stroking group");

  if (item->private->suspend_filling == 0)
    {
      GIMP_CANVAS_ITEM_GET_CLASS (item)->fill (item, cr);
    }
  else
    {
      cairo_new_sub_path (cr);
    }
}
