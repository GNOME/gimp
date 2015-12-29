/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include <mypaint-brush.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"

#include "paint-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimp-palettes.h"
#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpmybrush.h"

#include "gimpmybrushcore.h"
#include "gimpmybrushsurface.h"
#include "gimpmybrushoptions.h"

#include "gimp-intl.h"


struct _GimpMybrushCorePrivate
{
  GimpMybrush             *mybrush;
  GimpMybrushSurface      *surface;
  MyPaintBrush            *brush;
  gint64                   last_time;
};


/*  local function prototypes  */

static gboolean  gimp_mybrush_core_start  (GimpPaintCore     *paint_core,
                                           GimpDrawable      *drawable,
                                           GimpPaintOptions  *paint_options,
                                           const GimpCoords  *coords,
                                           GError           **error);
static void      gimp_mybrush_core_paint  (GimpPaintCore     *paint_core,
                                           GimpDrawable      *drawable,
                                           GimpPaintOptions  *paint_options,
                                           const GimpCoords  *coords,
                                           GimpPaintState     paint_state,
                                           guint32            time);
static void      gimp_mybrush_core_motion (GimpPaintCore     *paint_core,
                                           GimpDrawable      *drawable,
                                           GimpPaintOptions  *paint_options,
                                           const GimpCoords  *coords,
                                           guint32            time);


G_DEFINE_TYPE (GimpMybrushCore, gimp_mybrush_core, GIMP_TYPE_PAINT_CORE)

#define parent_class gimp_mybrush_core_parent_class


void
gimp_mybrush_core_register (Gimp                      *gimp,
                            GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_MYBRUSH_CORE,
                GIMP_TYPE_MYBRUSH_OPTIONS,
                "gimp-mybrush",
                _("Mybrush"),
                "gimp-tool-mypaint-brush");
}

static void
gimp_mybrush_core_class_init (GimpMybrushCoreClass *klass)
{
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  paint_core_class->start = gimp_mybrush_core_start;
  paint_core_class->paint = gimp_mybrush_core_paint;

  g_type_class_add_private (klass, sizeof (GimpMybrushCorePrivate));
}

static void
gimp_mybrush_core_init (GimpMybrushCore *mybrush)
{
  mybrush->private = G_TYPE_INSTANCE_GET_PRIVATE (mybrush,
                                                  GIMP_TYPE_MYBRUSH_CORE,
                                                  GimpMybrushCorePrivate);
}

static gboolean
gimp_mybrush_core_start (GimpPaintCore     *paint_core,
                         GimpDrawable      *drawable,
                         GimpPaintOptions  *paint_options,
                         const GimpCoords  *coords,
                         GError           **error)
{
  GimpMybrushCore *core    = GIMP_MYBRUSH_CORE (paint_core);
  GimpContext     *context = GIMP_CONTEXT (paint_options);

  core->private->mybrush = gimp_context_get_mybrush (context);

  if (! core->private->mybrush)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("No MyPaint brushes available for use with this tool."));
      return FALSE;
    }

  return TRUE;
}

static void
gimp_mybrush_core_paint (GimpPaintCore    *paint_core,
                         GimpDrawable     *drawable,
                         GimpPaintOptions *paint_options,
                         const GimpCoords *coords,
                         GimpPaintState    paint_state,
                         guint32           time)
{
  GimpMybrushCore    *mybrush = GIMP_MYBRUSH_CORE (paint_core);
  GimpMybrushOptions *options = GIMP_MYBRUSH_OPTIONS (paint_options);
  GimpContext        *context = GIMP_CONTEXT (paint_options);
  const gchar        *brush_data;
  GimpRGB             fg;
  GimpHSV             hsv;

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      gimp_context_get_foreground (context, &fg);
      gimp_palettes_add_color_history (context->gimp, &fg);

      mybrush->private->surface = gimp_mypaint_surface_new (gimp_drawable_get_buffer (drawable),
                                                            gimp_drawable_get_active_mask (drawable));

      mybrush->private->brush = mypaint_brush_new ();
      mypaint_brush_from_defaults (mybrush->private->brush);
      brush_data = gimp_mybrush_get_brush_json (mybrush->private->mybrush);
      if (brush_data)
        mypaint_brush_from_string (mybrush->private->brush, brush_data);

      gimp_rgb_to_hsv (&fg, &hsv);

      mypaint_brush_set_base_value (mybrush->private->brush,
                                    MYPAINT_BRUSH_SETTING_COLOR_H,
                                    hsv.h);
      mypaint_brush_set_base_value (mybrush->private->brush,
                                    MYPAINT_BRUSH_SETTING_COLOR_S,
                                    hsv.s);
      mypaint_brush_set_base_value (mybrush->private->brush,
                                    MYPAINT_BRUSH_SETTING_COLOR_V,
                                    hsv.v);

      mypaint_brush_set_base_value (mybrush->private->brush,
                                    MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC,
                                    options->radius);
      mypaint_brush_set_base_value (mybrush->private->brush,
                                    MYPAINT_BRUSH_SETTING_OPAQUE,
                                    options->opaque * gimp_context_get_opacity (context));
      mypaint_brush_set_base_value (mybrush->private->brush,
                                    MYPAINT_BRUSH_SETTING_HARDNESS,
                                    options->hardness);
      mypaint_brush_set_base_value (mybrush->private->brush,
                                    MYPAINT_BRUSH_SETTING_ERASER,
                                    options->eraser ? 1.0f : 0.0f);

      mypaint_brush_new_stroke (mybrush->private->brush);
      mybrush->private->last_time = -1;
      break;

    case GIMP_PAINT_STATE_MOTION:
      gimp_mybrush_core_motion (paint_core, drawable, paint_options,
                                coords, time);
      break;

    case GIMP_PAINT_STATE_FINISH:
      mypaint_surface_unref ((MyPaintSurface *) mybrush->private->surface);
      mybrush->private->surface = NULL;

      mypaint_brush_unref (mybrush->private->brush);
      mybrush->private->brush = NULL;
      break;
    }
}

static void
gimp_mybrush_core_motion (GimpPaintCore    *paint_core,
                          GimpDrawable     *drawable,
                          GimpPaintOptions *paint_options,
                          const GimpCoords *coords,
                          guint32           time)
{
  GimpMybrushCore  *mybrush = GIMP_MYBRUSH_CORE (paint_core);
  MyPaintRectangle  rect;
  gdouble           pressure;

  mypaint_surface_begin_atomic ((MyPaintSurface *) mybrush->private->surface);

  if (mybrush->private->last_time < 0)
    {
      /* First motion, so we need a zero pressure event to start the stroke */
      mybrush->private->last_time = (gint64) time - 15;

      mypaint_brush_stroke_to (mybrush->private->brush,
                               (MyPaintSurface *) mybrush->private->surface,
                               coords->x,
                               coords->y,
                               0.0f,
                               coords->xtilt,
                               coords->ytilt,
                               1.0f /* Pretend the cursor hasn't moved in a while */);
    }

  pressure = coords->pressure;

  /* libmypaint expects non-extended devices to default to 0.5 pressure */
  if (! coords->extended)
    pressure = 0.5f;

  mypaint_brush_stroke_to (mybrush->private->brush,
                           (MyPaintSurface *) mybrush->private->surface,
                           coords->x,
                           coords->y,
                           pressure,
                           coords->xtilt,
                           coords->ytilt,
                           (time - mybrush->private->last_time) * 0.001f);

  mybrush->private->last_time = time;

  mypaint_surface_end_atomic ((MyPaintSurface *) mybrush->private->surface,
                              &rect);

  if (rect.width > 0 && rect.height > 0)
    {
      paint_core->x1 = MIN (paint_core->x1, rect.x);
      paint_core->y1 = MIN (paint_core->y1, rect.y);
      paint_core->x2 = MAX (paint_core->x2, rect.x + rect.width);
      paint_core->y2 = MAX (paint_core->y2, rect.y + rect.height);

      gimp_drawable_update (drawable, rect.x, rect.y, rect.width, rect.height);
    }
}
