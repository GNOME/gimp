/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasprogress.c
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

#include "core/gimpprogress.h"

#include "gimpcanvas.h"
#include "gimpcanvas-style.h"
#include "gimpcanvasitem-utils.h"
#include "gimpcanvasprogress.h"
#include "gimpdisplayshell.h"


#define BORDER                5
#define RADIUS               20

#define MIN_UPDATE_INTERVAL  50000 /* microseconds */


enum
{
  PROP_0,
  PROP_ANCHOR,
  PROP_X,
  PROP_Y
};


typedef struct _GimpCanvasProgressPrivate GimpCanvasProgressPrivate;

struct _GimpCanvasProgressPrivate
{
  GimpHandleAnchor  anchor;
  gdouble           x;
  gdouble           y;

  gchar            *text;
  gdouble           value;

  guint64           last_update_time;
};

#define GET_PRIVATE(progress) \
        ((GimpCanvasProgressPrivate *) gimp_canvas_progress_get_instance_private ((GimpCanvasProgress *) (progress)))


/*  local function prototypes  */

static void             gimp_canvas_progress_iface_init   (GimpProgressInterface *iface);

static void             gimp_canvas_progress_finalize     (GObject               *object);
static void             gimp_canvas_progress_set_property (GObject               *object,
                                                           guint                  property_id,
                                                           const GValue          *value,
                                                           GParamSpec            *pspec);
static void             gimp_canvas_progress_get_property (GObject               *object,
                                                           guint                  property_id,
                                                           GValue                *value,
                                                           GParamSpec            *pspec);
static void             gimp_canvas_progress_draw         (GimpCanvasItem        *item,
                                                           cairo_t               *cr);
static cairo_region_t * gimp_canvas_progress_get_extents  (GimpCanvasItem        *item);
static gboolean         gimp_canvas_progress_hit          (GimpCanvasItem        *item,
                                                           gdouble                x,
                                                           gdouble                y);

static GimpProgress   * gimp_canvas_progress_start        (GimpProgress          *progress,
                                                           gboolean               cancellable,
                                                           const gchar           *message);
static void             gimp_canvas_progress_end          (GimpProgress          *progress);
static gboolean         gimp_canvas_progress_is_active    (GimpProgress          *progress);
static void             gimp_canvas_progress_set_text     (GimpProgress          *progress,
                                                           const gchar           *message);
static void             gimp_canvas_progress_set_value    (GimpProgress          *progress,
                                                           gdouble                percentage);
static gdouble          gimp_canvas_progress_get_value    (GimpProgress          *progress);
static void             gimp_canvas_progress_pulse        (GimpProgress          *progress);
static gboolean         gimp_canvas_progress_message      (GimpProgress          *progress,
                                                           Gimp                  *gimp,
                                                           GimpMessageSeverity    severity,
                                                           const gchar           *domain,
                                                           const gchar           *message);


G_DEFINE_TYPE_WITH_CODE (GimpCanvasProgress, gimp_canvas_progress,
                         GIMP_TYPE_CANVAS_ITEM,
                         G_ADD_PRIVATE (GimpCanvasProgress)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROGRESS,
                                                gimp_canvas_progress_iface_init))

#define parent_class gimp_canvas_progress_parent_class


static void
gimp_canvas_progress_class_init (GimpCanvasProgressClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->finalize     = gimp_canvas_progress_finalize;
  object_class->set_property = gimp_canvas_progress_set_property;
  object_class->get_property = gimp_canvas_progress_get_property;

  item_class->draw           = gimp_canvas_progress_draw;
  item_class->get_extents    = gimp_canvas_progress_get_extents;
  item_class->hit            = gimp_canvas_progress_hit;

  g_object_class_install_property (object_class, PROP_ANCHOR,
                                   g_param_spec_enum ("anchor", NULL, NULL,
                                                      GIMP_TYPE_HANDLE_ANCHOR,
                                                      GIMP_HANDLE_ANCHOR_CENTER,
                                                      GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_double ("x", NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_double ("y", NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_canvas_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start     = gimp_canvas_progress_start;
  iface->end       = gimp_canvas_progress_end;
  iface->is_active = gimp_canvas_progress_is_active;
  iface->set_text  = gimp_canvas_progress_set_text;
  iface->set_value = gimp_canvas_progress_set_value;
  iface->get_value = gimp_canvas_progress_get_value;
  iface->pulse     = gimp_canvas_progress_pulse;
  iface->message   = gimp_canvas_progress_message;
}

static void
gimp_canvas_progress_init (GimpCanvasProgress *progress)
{
}

static void
gimp_canvas_progress_finalize (GObject *object)
{
  GimpCanvasProgressPrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->text, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_canvas_progress_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpCanvasProgressPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ANCHOR:
      private->anchor = g_value_get_enum (value);
      break;
    case PROP_X:
      private->x = g_value_get_double (value);
      break;
    case PROP_Y:
      private->y = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_progress_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpCanvasProgressPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ANCHOR:
      g_value_set_enum (value, private->anchor);
      break;
    case PROP_X:
      g_value_set_double (value, private->x);
      break;
    case PROP_Y:
      g_value_set_double (value, private->y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static PangoLayout *
gimp_canvas_progress_transform (GimpCanvasItem *item,
                                gdouble        *x,
                                gdouble        *y,
                                gint           *width,
                                gint           *height)
{
  GimpCanvasProgressPrivate *private = GET_PRIVATE (item);
  GtkWidget                 *canvas  = gimp_canvas_item_get_canvas (item);
  PangoLayout               *layout;

  layout = gimp_canvas_get_layout (GIMP_CANVAS (canvas), "%s",
                                   private->text);

  pango_layout_get_pixel_size (layout, width, height);

  *width = MAX (*width, 2 * RADIUS);

  *width  += 2 * BORDER;
  *height += 3 * BORDER + 2 * RADIUS;

  gimp_canvas_item_transform_xy_f (item,
                                   private->x, private->y,
                                   x, y);

  gimp_canvas_item_shift_to_north_west (private->anchor,
                                        *x, *y,
                                        *width,
                                        *height,
                                        x, y);

  *x = floor (*x);
  *y = floor (*y);

  return layout;
}

static void
gimp_canvas_progress_draw (GimpCanvasItem *item,
                           cairo_t        *cr)
{
  GimpCanvasProgressPrivate *private = GET_PRIVATE (item);
  GtkWidget                 *canvas  = gimp_canvas_item_get_canvas (item);
  gdouble                    x, y;
  gint                       width, height;

  gimp_canvas_progress_transform (item, &x, &y, &width, &height);

  cairo_move_to (cr, x, y);
  cairo_line_to (cr, x + width, y);
  cairo_line_to (cr, x + width, y + height - BORDER - 2 * RADIUS);
  cairo_line_to (cr, x + 2 * BORDER + 2 * RADIUS, y + height - BORDER - 2 * RADIUS);
  cairo_arc (cr, x + BORDER + RADIUS, y + height - BORDER - RADIUS,
             BORDER + RADIUS, 0, G_PI);
  cairo_close_path (cr);

  _gimp_canvas_item_fill (item, cr);

  cairo_move_to (cr, x + BORDER, y + BORDER);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
  pango_cairo_show_layout (cr,
                           gimp_canvas_get_layout (GIMP_CANVAS (canvas),
                                                   "%s", private->text));

  gimp_canvas_set_tool_bg_style (gimp_canvas_item_get_canvas (item), cr);
  cairo_arc (cr, x + BORDER + RADIUS, y + height - BORDER - RADIUS,
             RADIUS, - G_PI / 2.0, 2 * G_PI - G_PI / 2.0);
  cairo_fill (cr);

  cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 1.0);
  cairo_move_to (cr, x + BORDER + RADIUS, y + height - BORDER - RADIUS);
  cairo_arc (cr, x + BORDER + RADIUS, y + height - BORDER - RADIUS,
             RADIUS, - G_PI / 2.0, 2 * G_PI * private->value - G_PI / 2.0);
  cairo_fill (cr);
}

static cairo_region_t *
gimp_canvas_progress_get_extents (GimpCanvasItem *item)
{
  cairo_rectangle_int_t rectangle;
  gdouble               x, y;
  gint                  width, height;

  gimp_canvas_progress_transform (item, &x, &y, &width, &height);

  /*  add 1px on each side because fill()'s default impl does the same  */
  rectangle.x      = (gint) x - 1;
  rectangle.y      = (gint) y - 1;
  rectangle.width  = width  + 2;
  rectangle.height = height + 2;

  return cairo_region_create_rectangle (&rectangle);
}

static gboolean
gimp_canvas_progress_hit (GimpCanvasItem *item,
                          gdouble         x,
                          gdouble         y)
{
  gdouble px, py;
  gint    pwidth, pheight;
  gdouble tx, ty;

  gimp_canvas_progress_transform (item, &px, &py, &pwidth, &pheight);
  gimp_canvas_item_transform_xy_f (item, x, y, &tx, &ty);

  pheight -= BORDER + 2 * RADIUS;

  return (tx >= px && tx < (px + pwidth) &&
          ty >= py && ty < (py + pheight));
}

static GimpProgress *
gimp_canvas_progress_start (GimpProgress *progress,
                            gboolean      cancellable,
                            const gchar  *message)
{
  GimpCanvasProgressPrivate *private = GET_PRIVATE (progress);

  gimp_canvas_progress_set_text (progress, message);

  private->last_update_time = g_get_monotonic_time ();

  return progress;
}

static void
gimp_canvas_progress_end (GimpProgress *progress)
{
}

static gboolean
gimp_canvas_progress_is_active (GimpProgress *progress)
{
  return TRUE;
}

static void
gimp_canvas_progress_set_text (GimpProgress *progress,
                               const gchar  *message)
{
  GimpCanvasProgressPrivate *private = GET_PRIVATE (progress);
  cairo_region_t            *old_region;
  cairo_region_t            *new_region;

  old_region = gimp_canvas_item_get_extents (GIMP_CANVAS_ITEM (progress));

  if (private->text)
    g_free (private->text);

  private->text = g_strdup (message);

  new_region = gimp_canvas_item_get_extents (GIMP_CANVAS_ITEM (progress));

  cairo_region_union (new_region, old_region);
  cairo_region_destroy (old_region);

  _gimp_canvas_item_update (GIMP_CANVAS_ITEM (progress), new_region);

  cairo_region_destroy (new_region);
}

static void
gimp_canvas_progress_set_value (GimpProgress *progress,
                                gdouble       percentage)
{
  GimpCanvasProgressPrivate *private = GET_PRIVATE (progress);

  if (percentage != private->value)
    {
      guint64 time = g_get_monotonic_time ();

      private->value = percentage;

      if (time - private->last_update_time >= MIN_UPDATE_INTERVAL)
        {
          cairo_region_t *region;

          private->last_update_time = time;

          region = gimp_canvas_item_get_extents (GIMP_CANVAS_ITEM (progress));

          _gimp_canvas_item_update (GIMP_CANVAS_ITEM (progress), region);

          cairo_region_destroy (region);
        }
    }
}

static gdouble
gimp_canvas_progress_get_value (GimpProgress *progress)
{
  GimpCanvasProgressPrivate *private = GET_PRIVATE (progress);

  return private->value;
}

static void
gimp_canvas_progress_pulse (GimpProgress *progress)
{
}

static gboolean
gimp_canvas_progress_message (GimpProgress        *progress,
                              Gimp                *gimp,
                              GimpMessageSeverity  severity,
                              const gchar         *domain,
                              const gchar         *message)
{
  return FALSE;
}

GimpCanvasItem *
gimp_canvas_progress_new (GimpDisplayShell *shell,
                          GimpHandleAnchor  anchor,
                          gdouble           x,
                          gdouble           y)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_CANVAS_PROGRESS,
                       "shell",  shell,
                       "anchor", anchor,
                       "x",      x,
                       "y",      y,
                       NULL);
}
