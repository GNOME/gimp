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

#ifdef HAVE_LIBMYPAINT

#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include <mypaint-brush.h>
#if 0
#include <mypaint-tiled-surface.h>
#include <mypaint-gegl-surface.h>
#endif

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"

#include "paint-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "config/gimpguiconfig.h" /* playground */

#include "core/gimp.h"
#include "core/gimp-palettes.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimptempbuf.h"

#include "gimpmybrushsurface.h"
#include "gimpmybrushoptions.h"
#include "gimpmybrush.h"

#include "gimp-intl.h"


struct _GimpMybrushPrivate
{
#if 0
  MyPaintGeglTiledSurface *surface;
#else
  GimpMybrushSurface      *surface;
#endif
  MyPaintBrush            *brush;
  gint64                   lastTime;
};


/*  local function prototypes  */

static void   gimp_mybrush_paint  (GimpPaintCore    *paint_core,
                                   GimpDrawable     *drawable,
                                   GimpPaintOptions *paint_options,
                                   const GimpCoords *coords,
                                   GimpPaintState    paint_state,
                                   guint32           time);
static void   gimp_mybrush_motion (GimpPaintCore    *paint_core,
                                   GimpDrawable     *drawable,
                                   GimpPaintOptions *paint_options,
                                   const GimpCoords *coords,
                                   guint32           time);


G_DEFINE_TYPE (GimpMybrush, gimp_mybrush, GIMP_TYPE_PAINT_CORE)

#define parent_class gimp_mybrush_parent_class


void
gimp_mybrush_register (Gimp                      *gimp,
                       GimpPaintRegisterCallback  callback)
{
  if (GIMP_GUI_CONFIG (gimp->config)->playground_mybrush_tool)
    (* callback) (gimp,
                  GIMP_TYPE_MYBRUSH,
                  GIMP_TYPE_MYBRUSH_OPTIONS,
                  "gimp-mybrush",
                  _("Mybrush"),
                  "gimp-tool-mybrush");
}

static void
gimp_mybrush_class_init (GimpMybrushClass *klass)
{
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  paint_core_class->paint = gimp_mybrush_paint;

  g_type_class_add_private (klass, sizeof (GimpMybrushPrivate));
}

static void
gimp_mybrush_init (GimpMybrush *mybrush)
{
  mybrush->private = G_TYPE_INSTANCE_GET_PRIVATE (mybrush,
                                                  GIMP_TYPE_MYBRUSH,
                                                  GimpMybrushPrivate);
}

static void
gimp_mybrush_paint (GimpPaintCore    *paint_core,
                    GimpDrawable     *drawable,
                    GimpPaintOptions *paint_options,
                    const GimpCoords *coords,
                    GimpPaintState    paint_state,
                    guint32           time)
{
  GimpMybrush        *mybrush = GIMP_MYBRUSH (paint_core);
  GimpMybrushOptions *options = GIMP_MYBRUSH_OPTIONS (paint_options);
  const gchar        *brush_data;
#if 0
  GeglBuffer         *buffer;
  GimpComponentMask   active_mask;
#endif
  GimpRGB             fg;
  GimpHSV             hsv;

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
        {
          GimpContext *context = GIMP_CONTEXT (paint_options);
          GimpRGB      foreground;

          gimp_context_get_foreground (context, &foreground);
          gimp_palettes_add_color_history (context->gimp,
                                           &foreground);

#if 0
          mybrush->private->surface = mypaint_gegl_tiled_surface_new ();

          buffer = mypaint_gegl_tiled_surface_get_buffer (mybrush->private->surface);
          buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                    gimp_item_get_width (GIMP_ITEM (drawable)),
                                                    gimp_item_get_height (GIMP_ITEM (drawable))),
                                    gegl_buffer_get_format (buffer));
          gegl_buffer_copy (gimp_drawable_get_buffer (drawable), NULL,
                            GEGL_ABYSS_NONE,
                            buffer, NULL);
          mypaint_gegl_tiled_surface_set_buffer (mybrush->private->surface, buffer);
          g_object_unref (buffer);
#else
          mybrush->private->surface = gimp_mypaint_surface_new (gimp_drawable_get_buffer (drawable),
                                                                gimp_drawable_get_active_mask (drawable));
#endif

          mybrush->private->brush = mypaint_brush_new ();
          mypaint_brush_from_defaults (mybrush->private->brush);
          brush_data = gimp_mybrush_options_get_brush_data (options);
          if (brush_data)
            mypaint_brush_from_string (mybrush->private->brush, brush_data);

#if 0
          active_mask = gimp_drawable_get_active_mask (drawable);

          mypaint_brush_set_base_value (mybrush->private->brush,
                                        MYPAINT_BRUSH_SETTING_LOCK_ALPHA,
                                        (active_mask & GIMP_COMPONENT_MASK_ALPHA) ?
                                        FALSE : TRUE);
#endif

          gimp_context_get_foreground (context, &fg);
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

          mypaint_brush_new_stroke (mybrush->private->brush);
          mybrush->private->lastTime = -1;
        }
      break;

    case GIMP_PAINT_STATE_MOTION:
      gimp_mybrush_motion (paint_core, drawable, paint_options, coords, time);
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
gimp_mybrush_motion (GimpPaintCore    *paint_core,
                     GimpDrawable     *drawable,
                     GimpPaintOptions *paint_options,
                     const GimpCoords *coords,
                     guint32           time)
{
  GimpMybrush        *mybrush = GIMP_MYBRUSH (paint_core);
  MyPaintRectangle    rect;

  mypaint_surface_begin_atomic ((MyPaintSurface *) mybrush->private->surface);


  if (mybrush->private->lastTime < 0)
    {
      /* First motion, so we need a zero pressure event to start the stroke */
      mybrush->private->lastTime = (gint64)time - 15;

      mypaint_brush_stroke_to (mybrush->private->brush,
                               (MyPaintSurface *) mybrush->private->surface,
                               coords->x,
                               coords->y,
                               0.0f,
                               coords->xtilt,
                               coords->ytilt,
                               1.0f /* Pretend the cursor hasn't moved in a while */);
    }

  mypaint_brush_stroke_to (mybrush->private->brush,
                           (MyPaintSurface *) mybrush->private->surface,
                           coords->x,
                           coords->y,
                           coords->pressure,
                           coords->xtilt,
                           coords->ytilt,
                           (time - mybrush->private->lastTime) * 0.001f);
  mybrush->private->lastTime = time;

  mypaint_surface_end_atomic ((MyPaintSurface *) mybrush->private->surface,
                              &rect);

  if (rect.width > 0 && rect.height > 0)
    {
#if 0
      GeglBuffer *src;

      src = mypaint_gegl_tiled_surface_get_buffer (mybrush->private->surface);

      gegl_buffer_copy (src,
                        (GeglRectangle *) &rect,
                        GEGL_ABYSS_NONE,
                        gimp_drawable_get_buffer (drawable),
                        NULL);
#endif
      paint_core->x1 = MIN (paint_core->x1, rect.x);
      paint_core->y1 = MIN (paint_core->y1, rect.y);
      paint_core->x2 = MAX (paint_core->x2, rect.x + rect.width);
      paint_core->y2 = MAX (paint_core->y2, rect.y + rect.height);

      gimp_drawable_update (drawable, rect.x, rect.y, rect.width, rect.height);
    }
}

#endif
