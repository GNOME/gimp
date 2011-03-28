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

#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "core/gimpmarshal.h"

#include "gimpcanvasitem.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-style.h"


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


typedef struct _GimpCanvasItemPrivate GimpCanvasItemPrivate;

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

#define GET_PRIVATE(item) \
        G_TYPE_INSTANCE_GET_PRIVATE (item, \
                                     GIMP_TYPE_CANVAS_ITEM, \
                                     GimpCanvasItemPrivate)


/*  local function prototypes  */

static void             gimp_canvas_item_constructed      (GObject          *object);
static void             gimp_canvas_item_set_property     (GObject          *object,
                                                           guint             property_id,
                                                           const GValue     *value,
                                                           GParamSpec       *pspec);
static void             gimp_canvas_item_get_property     (GObject          *object,
                                                           guint             property_id,
                                                           GValue           *value,
                                                           GParamSpec       *pspec);
static void  gimp_canvas_item_dispatch_properties_changed (GObject          *object,
                                                           guint             n_pspecs,
                                                           GParamSpec      **pspecs);

static void             gimp_canvas_item_real_draw        (GimpCanvasItem   *item,
                                                           GimpDisplayShell *shell,
                                                           cairo_t          *cr);
static cairo_region_t * gimp_canvas_item_real_get_extents (GimpCanvasItem   *item,
                                                           GimpDisplayShell *shell);
static void             gimp_canvas_item_real_stroke      (GimpCanvasItem   *item,
                                                           GimpDisplayShell *shell,
                                                           cairo_t          *cr);
static void             gimp_canvas_item_real_fill        (GimpCanvasItem   *item,
                                                           GimpDisplayShell *shell,
                                                           cairo_t          *cr);
static gboolean         gimp_canvas_item_real_hit         (GimpCanvasItem   *item,
                                                           GimpDisplayShell *shell,
                                                           gdouble           x,
                                                           gdouble           y);


G_DEFINE_TYPE (GimpCanvasItem, gimp_canvas_item,
               GIMP_TYPE_OBJECT)

#define parent_class gimp_canvas_item_parent_class

static guint item_signals[LAST_SIGNAL] = { 0 };


static void
gimp_canvas_item_class_init (GimpCanvasItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

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
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
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

  g_type_class_add_private (klass, sizeof (GimpCanvasItemPrivate));
}

static void
gimp_canvas_item_init (GimpCanvasItem *item)
{
  GimpCanvasItemPrivate *private = GET_PRIVATE (item);

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
  GimpCanvasItemPrivate *private = GET_PRIVATE (object);

  g_assert (GIMP_IS_DISPLAY_SHELL (private->shell));

  private->change_count = 0; /* undo hack from init() */

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);
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
  GimpCanvasItemPrivate *private = GET_PRIVATE (object);

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
gimp_canvas_item_real_draw (GimpCanvasItem   *item,
                            GimpDisplayShell *shell,
                            cairo_t          *cr)
{
  g_warn_if_reached ();
}

static cairo_region_t *
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

static gboolean
gimp_canvas_item_real_hit (GimpCanvasItem   *item,
                           GimpDisplayShell *shell,
                           gdouble           x,
                           gdouble           y)
{
  return FALSE;
}


/*  public functions  */

void
gimp_canvas_item_draw (GimpCanvasItem *item,
                       cairo_t        *cr)
{
  GimpCanvasItemPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));
  g_return_if_fail (cr != NULL);

  private = GET_PRIVATE (item);

  if (private->visible)
    {
      cairo_save (cr);
      GIMP_CANVAS_ITEM_GET_CLASS (item)->draw (item, private->shell, cr);
      cairo_restore (cr);
    }
}

cairo_region_t *
gimp_canvas_item_get_extents (GimpCanvasItem *item)
{
  GimpCanvasItemPrivate *private;

  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), NULL);

  private = GET_PRIVATE (item);

  if (private->visible)
    return GIMP_CANVAS_ITEM_GET_CLASS (item)->get_extents (item, private->shell);

  return NULL;
}

gboolean
gimp_canvas_item_hit (GimpCanvasItem   *item,
                      gdouble           x,
                      gdouble           y)
{
  GimpCanvasItemPrivate *private;

  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), FALSE);

  private = GET_PRIVATE (item);

  return GIMP_CANVAS_ITEM_GET_CLASS (item)->hit (item, private->shell, x, y);
}

void
gimp_canvas_item_set_visible (GimpCanvasItem *item,
                              gboolean        visible)
{
  GimpCanvasItemPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  private = GET_PRIVATE (item);

  if (private->visible != visible)
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
  GimpCanvasItemPrivate *private;

  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), FALSE);

  private = GET_PRIVATE (item);

  return private->visible;
}

void
gimp_canvas_item_set_line_cap (GimpCanvasItem   *item,
                               cairo_line_cap_t  line_cap)
{
  GimpCanvasItemPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  private = GET_PRIVATE (item);

  if (private->line_cap != line_cap)
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
  GimpCanvasItemPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  private = GET_PRIVATE (item);

  if (private->highlight != highlight)
    {
      g_object_set (G_OBJECT (item),
                    "highlight", highlight,
                    NULL);
    }
}

gboolean
gimp_canvas_item_get_highlight (GimpCanvasItem *item)
{
  GimpCanvasItemPrivate *private;

  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), FALSE);

  private = GET_PRIVATE (item);

  return private->highlight;
}

void
gimp_canvas_item_begin_change (GimpCanvasItem *item)
{
  GimpCanvasItemPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  private = GET_PRIVATE (item);

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

  private = GET_PRIVATE (item);

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
      else if (private->change_region)
        {
          cairo_region_destroy (private->change_region);
          private->change_region = NULL;
        }
    }
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
_gimp_canvas_item_update (GimpCanvasItem *item,
                          cairo_region_t *region)
{
  g_signal_emit (item, item_signals[UPDATE], 0,
                 region);
}

gboolean
_gimp_canvas_item_needs_update (GimpCanvasItem *item)
{
  GimpCanvasItemPrivate *private = GET_PRIVATE (item);

  return (private->change_count == 0 &&
          g_signal_has_handler_pending (item, item_signals[UPDATE], 0, FALSE));
}

void
_gimp_canvas_item_stroke (GimpCanvasItem *item,
                          cairo_t        *cr)
{
  GimpCanvasItemPrivate *private = GET_PRIVATE (item);

  if (private->suspend_filling > 0)
    g_warning ("_gimp_canvas_item_stroke() on an item that is in a filling group");

  if (private->suspend_stroking == 0)
    {
      GIMP_CANVAS_ITEM_GET_CLASS (item)->stroke (item, private->shell, cr);
    }
  else
    {
      cairo_new_sub_path (cr);
    }
}

void
_gimp_canvas_item_fill (GimpCanvasItem   *item,
                        cairo_t          *cr)
{
  GimpCanvasItemPrivate *private = GET_PRIVATE (item);

  if (private->suspend_stroking > 0)
    g_warning ("_gimp_canvas_item_fill() on an item that is in a stroking group");

  if (private->suspend_filling == 0)
    {
      GIMP_CANVAS_ITEM_GET_CLASS (item)->fill (item, private->shell, cr);
    }
  else
    {
      cairo_new_sub_path (cr);
    }
}
