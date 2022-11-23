/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvasprogress.c
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

#include "core/ligmaprogress.h"

#include "ligmacanvas.h"
#include "ligmacanvas-style.h"
#include "ligmacanvasitem-utils.h"
#include "ligmacanvasprogress.h"
#include "ligmadisplayshell.h"


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


typedef struct _LigmaCanvasProgressPrivate LigmaCanvasProgressPrivate;

struct _LigmaCanvasProgressPrivate
{
  LigmaHandleAnchor  anchor;
  gdouble           x;
  gdouble           y;

  gchar            *text;
  gdouble           value;

  guint64           last_update_time;
};

#define GET_PRIVATE(progress) \
        ((LigmaCanvasProgressPrivate *) ligma_canvas_progress_get_instance_private ((LigmaCanvasProgress *) (progress)))


/*  local function prototypes  */

static void             ligma_canvas_progress_iface_init   (LigmaProgressInterface *iface);

static void             ligma_canvas_progress_finalize     (GObject               *object);
static void             ligma_canvas_progress_set_property (GObject               *object,
                                                           guint                  property_id,
                                                           const GValue          *value,
                                                           GParamSpec            *pspec);
static void             ligma_canvas_progress_get_property (GObject               *object,
                                                           guint                  property_id,
                                                           GValue                *value,
                                                           GParamSpec            *pspec);
static void             ligma_canvas_progress_draw         (LigmaCanvasItem        *item,
                                                           cairo_t               *cr);
static cairo_region_t * ligma_canvas_progress_get_extents  (LigmaCanvasItem        *item);
static gboolean         ligma_canvas_progress_hit          (LigmaCanvasItem        *item,
                                                           gdouble                x,
                                                           gdouble                y);

static LigmaProgress   * ligma_canvas_progress_start        (LigmaProgress          *progress,
                                                           gboolean               cancellable,
                                                           const gchar           *message);
static void             ligma_canvas_progress_end          (LigmaProgress          *progress);
static gboolean         ligma_canvas_progress_is_active    (LigmaProgress          *progress);
static void             ligma_canvas_progress_set_text     (LigmaProgress          *progress,
                                                           const gchar           *message);
static void             ligma_canvas_progress_set_value    (LigmaProgress          *progress,
                                                           gdouble                percentage);
static gdouble          ligma_canvas_progress_get_value    (LigmaProgress          *progress);
static void             ligma_canvas_progress_pulse        (LigmaProgress          *progress);
static gboolean         ligma_canvas_progress_message      (LigmaProgress          *progress,
                                                           Ligma                  *ligma,
                                                           LigmaMessageSeverity    severity,
                                                           const gchar           *domain,
                                                           const gchar           *message);


G_DEFINE_TYPE_WITH_CODE (LigmaCanvasProgress, ligma_canvas_progress,
                         LIGMA_TYPE_CANVAS_ITEM,
                         G_ADD_PRIVATE (LigmaCanvasProgress)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PROGRESS,
                                                ligma_canvas_progress_iface_init))

#define parent_class ligma_canvas_progress_parent_class


static void
ligma_canvas_progress_class_init (LigmaCanvasProgressClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->finalize     = ligma_canvas_progress_finalize;
  object_class->set_property = ligma_canvas_progress_set_property;
  object_class->get_property = ligma_canvas_progress_get_property;

  item_class->draw           = ligma_canvas_progress_draw;
  item_class->get_extents    = ligma_canvas_progress_get_extents;
  item_class->hit            = ligma_canvas_progress_hit;

  g_object_class_install_property (object_class, PROP_ANCHOR,
                                   g_param_spec_enum ("anchor", NULL, NULL,
                                                      LIGMA_TYPE_HANDLE_ANCHOR,
                                                      LIGMA_HANDLE_ANCHOR_CENTER,
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
}

static void
ligma_canvas_progress_iface_init (LigmaProgressInterface *iface)
{
  iface->start     = ligma_canvas_progress_start;
  iface->end       = ligma_canvas_progress_end;
  iface->is_active = ligma_canvas_progress_is_active;
  iface->set_text  = ligma_canvas_progress_set_text;
  iface->set_value = ligma_canvas_progress_set_value;
  iface->get_value = ligma_canvas_progress_get_value;
  iface->pulse     = ligma_canvas_progress_pulse;
  iface->message   = ligma_canvas_progress_message;
}

static void
ligma_canvas_progress_init (LigmaCanvasProgress *progress)
{
}

static void
ligma_canvas_progress_finalize (GObject *object)
{
  LigmaCanvasProgressPrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->text, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_canvas_progress_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaCanvasProgressPrivate *private = GET_PRIVATE (object);

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
ligma_canvas_progress_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  LigmaCanvasProgressPrivate *private = GET_PRIVATE (object);

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
ligma_canvas_progress_transform (LigmaCanvasItem *item,
                                gdouble        *x,
                                gdouble        *y,
                                gint           *width,
                                gint           *height)
{
  LigmaCanvasProgressPrivate *private = GET_PRIVATE (item);
  GtkWidget                 *canvas  = ligma_canvas_item_get_canvas (item);
  PangoLayout               *layout;

  layout = ligma_canvas_get_layout (LIGMA_CANVAS (canvas), "%s",
                                   private->text);

  pango_layout_get_pixel_size (layout, width, height);

  *width = MAX (*width, 2 * RADIUS);

  *width  += 2 * BORDER;
  *height += 3 * BORDER + 2 * RADIUS;

  ligma_canvas_item_transform_xy_f (item,
                                   private->x, private->y,
                                   x, y);

  ligma_canvas_item_shift_to_north_west (private->anchor,
                                        *x, *y,
                                        *width,
                                        *height,
                                        x, y);

  *x = floor (*x);
  *y = floor (*y);

  return layout;
}

static void
ligma_canvas_progress_draw (LigmaCanvasItem *item,
                           cairo_t        *cr)
{
  LigmaCanvasProgressPrivate *private = GET_PRIVATE (item);
  GtkWidget                 *canvas  = ligma_canvas_item_get_canvas (item);
  gdouble                    x, y;
  gint                       width, height;

  ligma_canvas_progress_transform (item, &x, &y, &width, &height);

  cairo_move_to (cr, x, y);
  cairo_line_to (cr, x + width, y);
  cairo_line_to (cr, x + width, y + height - BORDER - 2 * RADIUS);
  cairo_line_to (cr, x + 2 * BORDER + 2 * RADIUS, y + height - BORDER - 2 * RADIUS);
  cairo_arc (cr, x + BORDER + RADIUS, y + height - BORDER - RADIUS,
             BORDER + RADIUS, 0, G_PI);
  cairo_close_path (cr);

  _ligma_canvas_item_fill (item, cr);

  cairo_move_to (cr, x + BORDER, y + BORDER);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
  pango_cairo_show_layout (cr,
                           ligma_canvas_get_layout (LIGMA_CANVAS (canvas),
                                                   "%s", private->text));

  ligma_canvas_set_tool_bg_style (ligma_canvas_item_get_canvas (item), cr);
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
ligma_canvas_progress_get_extents (LigmaCanvasItem *item)
{
  cairo_rectangle_int_t rectangle;
  gdouble               x, y;
  gint                  width, height;

  ligma_canvas_progress_transform (item, &x, &y, &width, &height);

  /*  add 1px on each side because fill()'s default impl does the same  */
  rectangle.x      = (gint) x - 1;
  rectangle.y      = (gint) y - 1;
  rectangle.width  = width  + 2;
  rectangle.height = height + 2;

  return cairo_region_create_rectangle (&rectangle);
}

static gboolean
ligma_canvas_progress_hit (LigmaCanvasItem *item,
                          gdouble         x,
                          gdouble         y)
{
  gdouble px, py;
  gint    pwidth, pheight;
  gdouble tx, ty;

  ligma_canvas_progress_transform (item, &px, &py, &pwidth, &pheight);
  ligma_canvas_item_transform_xy_f (item, x, y, &tx, &ty);

  pheight -= BORDER + 2 * RADIUS;

  return (tx >= px && tx < (px + pwidth) &&
          ty >= py && ty < (py + pheight));
}

static LigmaProgress *
ligma_canvas_progress_start (LigmaProgress *progress,
                            gboolean      cancellable,
                            const gchar  *message)
{
  LigmaCanvasProgressPrivate *private = GET_PRIVATE (progress);

  ligma_canvas_progress_set_text (progress, message);

  private->last_update_time = g_get_monotonic_time ();

  return progress;
}

static void
ligma_canvas_progress_end (LigmaProgress *progress)
{
}

static gboolean
ligma_canvas_progress_is_active (LigmaProgress *progress)
{
  return TRUE;
}

static void
ligma_canvas_progress_set_text (LigmaProgress *progress,
                               const gchar  *message)
{
  LigmaCanvasProgressPrivate *private = GET_PRIVATE (progress);
  cairo_region_t            *old_region;
  cairo_region_t            *new_region;

  old_region = ligma_canvas_item_get_extents (LIGMA_CANVAS_ITEM (progress));

  if (private->text)
    g_free (private->text);

  private->text = g_strdup (message);

  new_region = ligma_canvas_item_get_extents (LIGMA_CANVAS_ITEM (progress));

  cairo_region_union (new_region, old_region);
  cairo_region_destroy (old_region);

  _ligma_canvas_item_update (LIGMA_CANVAS_ITEM (progress), new_region);

  cairo_region_destroy (new_region);
}

static void
ligma_canvas_progress_set_value (LigmaProgress *progress,
                                gdouble       percentage)
{
  LigmaCanvasProgressPrivate *private = GET_PRIVATE (progress);

  if (percentage != private->value)
    {
      guint64 time = g_get_monotonic_time ();

      private->value = percentage;

      if (time - private->last_update_time >= MIN_UPDATE_INTERVAL)
        {
          cairo_region_t *region;

          private->last_update_time = time;

          region = ligma_canvas_item_get_extents (LIGMA_CANVAS_ITEM (progress));

          _ligma_canvas_item_update (LIGMA_CANVAS_ITEM (progress), region);

          cairo_region_destroy (region);
        }
    }
}

static gdouble
ligma_canvas_progress_get_value (LigmaProgress *progress)
{
  LigmaCanvasProgressPrivate *private = GET_PRIVATE (progress);

  return private->value;
}

static void
ligma_canvas_progress_pulse (LigmaProgress *progress)
{
}

static gboolean
ligma_canvas_progress_message (LigmaProgress        *progress,
                              Ligma                *ligma,
                              LigmaMessageSeverity  severity,
                              const gchar         *domain,
                              const gchar         *message)
{
  return FALSE;
}

LigmaCanvasItem *
ligma_canvas_progress_new (LigmaDisplayShell *shell,
                          LigmaHandleAnchor  anchor,
                          gdouble           x,
                          gdouble           y)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_CANVAS_PROGRESS,
                       "shell",  shell,
                       "anchor", anchor,
                       "x",      x,
                       "y",      y,
                       NULL);
}
