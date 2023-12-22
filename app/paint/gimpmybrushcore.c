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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include "core/gimppickable.h"
#include "core/gimpsymmetry.h"

#include "gimpmybrushcore.h"
#include "gimpmybrushsurface.h"
#include "gimpmybrushoptions.h"

#include "gimp-intl.h"


struct _GimpMybrushCorePrivate
{
  GimpMybrush        *mybrush;
  GimpMybrushSurface *surface;
  GList              *brushes;
  gboolean            synthetic;
  gint64              last_time;
};


/*  local function prototypes  */

static void      gimp_mybrush_core_finalize       (GObject           *object);

static gboolean  gimp_mybrush_core_start          (GimpPaintCore     *paint_core,
                                                   GList             *drawables,
                                                   GimpPaintOptions  *paint_options,
                                                   const GimpCoords  *coords,
                                                   GError           **error);
static void      gimp_mybrush_core_interpolate    (GimpPaintCore     *paint_core,
                                                   GList             *drawables,
                                                   GimpPaintOptions  *paint_options,
                                                   guint32            time);
static void      gimp_mybrush_core_paint          (GimpPaintCore     *paint_core,
                                                   GList             *drawables,
                                                   GimpPaintOptions  *paint_options,
                                                   GimpSymmetry      *sym,
                                                   GimpPaintState     paint_state,
                                                   guint32            time);
static void      gimp_mybrush_core_motion         (GimpPaintCore     *paint_core,
                                                   GimpDrawable      *drawable,
                                                   GimpPaintOptions  *paint_options,
                                                   GimpSymmetry      *sym,
                                                   guint32            time);
static void      gimp_mybrush_core_create_brushes (GimpMybrushCore   *mybrush,
                                                   GimpDrawable      *drawable,
                                                   GimpPaintOptions  *paint_options,
                                                   GimpSymmetry      *sym);


G_DEFINE_TYPE_WITH_PRIVATE (GimpMybrushCore, gimp_mybrush_core,
                            GIMP_TYPE_PAINT_CORE)

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
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  object_class->finalize        = gimp_mybrush_core_finalize;

  paint_core_class->start       = gimp_mybrush_core_start;
  paint_core_class->paint       = gimp_mybrush_core_paint;
  paint_core_class->interpolate = gimp_mybrush_core_interpolate;
}

static void
gimp_mybrush_core_init (GimpMybrushCore *mybrush)
{
  mybrush->private = gimp_mybrush_core_get_instance_private (mybrush);
}

static void
gimp_mybrush_core_finalize (GObject *object)
{
  GimpMybrushCore *core = GIMP_MYBRUSH_CORE (object);

  if (core->private->brushes)
    {
      g_list_free_full (core->private->brushes,
                        (GDestroyNotify) mypaint_brush_unref);
      core->private->brushes = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_mybrush_core_start (GimpPaintCore     *paint_core,
                         GList             *drawables,
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
gimp_mybrush_core_interpolate (GimpPaintCore    *paint_core,
                               GList            *drawables,
                               GimpPaintOptions *paint_options,
                               guint32           time)
{
  GimpMybrushCore *mybrush = GIMP_MYBRUSH_CORE (paint_core);

  /* If this is the first motion the brush has received then
   * we're being asked to draw a synthetic stroke in line mode
   */
  if (mybrush->private->last_time < 0)
    {
      GimpCoords saved_coords = paint_core->cur_coords;

      paint_core->cur_coords = paint_core->last_coords;

      mybrush->private->synthetic = TRUE;

      gimp_paint_core_paint (paint_core, drawables, paint_options,
                             GIMP_PAINT_STATE_MOTION, time);

      paint_core->cur_coords = saved_coords;
    }

  gimp_paint_core_paint (paint_core, drawables, paint_options,
                         GIMP_PAINT_STATE_MOTION, time);

  paint_core->last_coords = paint_core->cur_coords;
}

static void
gimp_mybrush_core_paint (GimpPaintCore    *paint_core,
                         GList            *drawables,
                         GimpPaintOptions *paint_options,
                         GimpSymmetry     *sym,
                         GimpPaintState    paint_state,
                         guint32           time)
{
  GimpMybrushCore *mybrush = GIMP_MYBRUSH_CORE (paint_core);
  GimpContext     *context = GIMP_CONTEXT (paint_options);
  gint             offset_x;
  gint             offset_y;

  g_return_if_fail (g_list_length (drawables) == 1);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      gimp_palettes_add_color_history (context->gimp, gimp_context_get_foreground (context));
      gimp_symmetry_set_stateful (sym, TRUE);

      gimp_item_get_offset (drawables->data, &offset_x, &offset_y);
      mybrush->private->surface =
        gimp_mypaint_surface_new (gimp_drawable_get_buffer (drawables->data),
                                  gimp_drawable_get_active_mask (drawables->data),
                                  paint_core->mask_buffer,
                                  -offset_x, -offset_y,
                                  GIMP_MYBRUSH_OPTIONS (paint_options));

      gimp_mybrush_core_create_brushes (mybrush, drawables->data, paint_options, sym);

      mybrush->private->last_time = -1;
      mybrush->private->synthetic = FALSE;
      break;

    case GIMP_PAINT_STATE_MOTION:
      gimp_mybrush_core_motion (paint_core, drawables->data, paint_options,
                                sym, time);
      break;

    case GIMP_PAINT_STATE_FINISH:
      gimp_symmetry_set_stateful (sym, FALSE);
      mypaint_surface_unref ((MyPaintSurface *) mybrush->private->surface);
      mybrush->private->surface = NULL;

      g_list_free_full (mybrush->private->brushes,
                        (GDestroyNotify) mypaint_brush_unref);
      mybrush->private->brushes = NULL;
      break;
    }
}

static void
gimp_mybrush_core_motion (GimpPaintCore    *paint_core,
                          GimpDrawable     *drawable,
                          GimpPaintOptions *paint_options,
                          GimpSymmetry     *sym,
                          guint32           time)
{
  GimpMybrushCore  *mybrush = GIMP_MYBRUSH_CORE (paint_core);
  MyPaintRectangle  rect;
  GimpCoords        origin;
  GList            *iter;
  gdouble           dt = 0.0;
  gint              off_x, off_y;
  gint              n_strokes;
  gint              i;

  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
  n_strokes = gimp_symmetry_get_size (sym);

  origin    = *(gimp_symmetry_get_origin (sym));
  origin.x -= off_x;
  origin.y -= off_y;
  gimp_symmetry_set_origin (sym, drawable, &origin);

  /* The number of strokes may change during a motion, depending on
   * the type of symmetry. When that happens, reset the brushes.
   */
  if (g_list_length (mybrush->private->brushes) != n_strokes)
    {
      gimp_mybrush_core_create_brushes (mybrush, drawable, paint_options, sym);
    }

  mypaint_surface_begin_atomic ((MyPaintSurface *) mybrush->private->surface);

  if (mybrush->private->last_time < 0)
    {
      /* First motion, so we need zero pressure events to start the strokes */
      for (iter = mybrush->private->brushes, i = 0;
           iter;
           iter = g_list_next (iter), i++)
        {
          MyPaintBrush *brush  = iter->data;
          GimpCoords    coords = *(gimp_symmetry_get_coords (sym, i));

          mypaint_brush_stroke_to (brush,
                                   (MyPaintSurface *) mybrush->private->surface,
                                   coords.x,
                                   coords.y,
                                   0.0f,
                                   coords.xtilt,
                                   coords.ytilt,
                                   1.0f /* Pretend the cursor hasn't moved in a while */);
        }

      dt = 0.015;
    }
  else if (mybrush->private->synthetic)
    {
      GimpVector2 v = { paint_core->cur_coords.x - paint_core->last_coords.x,
                        paint_core->cur_coords.y - paint_core->last_coords.y };

      dt = 0.0005 * gimp_vector2_length_val (v);
    }
  else
    {
      dt = (time - mybrush->private->last_time) * 0.001;
    }

  for (iter = mybrush->private->brushes, i = 0;
       iter;
       iter = g_list_next (iter), i++)
    {
      MyPaintBrush *brush    = iter->data;
      GimpCoords    coords   = *(gimp_symmetry_get_coords (sym, i));
      gdouble       pressure = coords.pressure;
      gboolean      expanded;
      gfloat        radius   = 100;
      gint          x1, x2, y1, y2;
      gint          offset_change_x, offset_change_y;
      gint          off_x_surf, off_y_surf;
      gint          off_x, off_y;

      x1 = coords.x - radius;
      y1 = coords.y - radius;
      x2 = coords.x + radius;
      y2 = coords.y + radius;

      expanded = gimp_paint_core_expand_drawable (paint_core, drawable, paint_options,
                                                  x1, x2, y1, y2,
                                                  &offset_change_x, &offset_change_y);

      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
      if (expanded)
        gimp_mypaint_surface_set_buffer (mybrush->private->surface, gimp_drawable_get_buffer (drawable),
                                         off_x, off_y);

      gimp_mypaint_surface_get_offset (mybrush->private->surface, &off_x_surf, &off_y_surf);
      coords.x -= off_x_surf;
      coords.y -= off_y_surf;

      if (offset_change_x || offset_change_y)
        {
          gimp_mypaint_surface_set_offset (mybrush->private->surface,
                                           off_x_surf + offset_change_x,
                                           off_y_surf + offset_change_y);

          origin    = *(gimp_symmetry_get_origin (sym));
          origin.x += offset_change_x;
          origin.y += offset_change_y;
          gimp_symmetry_set_origin (sym, drawable, &origin);
        }

      mypaint_brush_stroke_to (brush,
                               (MyPaintSurface *) mybrush->private->surface,
                               coords.x,
                               coords.y,
                               pressure,
                               coords.xtilt,
                               coords.ytilt,
                               dt);
    }

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

static void
gimp_mybrush_core_create_brushes (GimpMybrushCore  *mybrush,
                                  GimpDrawable     *drawable,
                                  GimpPaintOptions *paint_options,
                                  GimpSymmetry     *sym)
{
  GimpMybrushOptions *options = GIMP_MYBRUSH_OPTIONS (paint_options);
  GimpContext        *context = GIMP_CONTEXT (paint_options);
  GeglColor          *color;
  gfloat              hsv[3];
  gint                n_strokes;
  gint                i;

  if (mybrush->private->brushes)
    {
      g_list_free_full (mybrush->private->brushes,
                        (GDestroyNotify) mypaint_brush_unref);
      mybrush->private->brushes = NULL;
    }

  if (options->eraser)
    color = gimp_context_get_background (context);
  else
    color = gimp_context_get_foreground (context);

  gegl_color_get_pixel (color, babl_format_with_space ("HSV float", gimp_drawable_get_space (drawable)), hsv);

  n_strokes = gimp_symmetry_get_size (sym);

  for (i = 0; i < n_strokes; i++)
    {
      MyPaintBrush *brush = mypaint_brush_new ();
      const gchar  *brush_data;

      mypaint_brush_from_defaults (brush);
      brush_data = gimp_mybrush_get_brush_json (mybrush->private->mybrush);
      if (brush_data)
        mypaint_brush_from_string (brush, brush_data);

      if (! mypaint_brush_get_base_value (brush,
                                          MYPAINT_BRUSH_SETTING_RESTORE_COLOR))
        {
          mypaint_brush_set_base_value (brush,
                                        MYPAINT_BRUSH_SETTING_COLOR_H,
                                        hsv[0]);
          mypaint_brush_set_base_value (brush,
                                        MYPAINT_BRUSH_SETTING_COLOR_S,
                                        hsv[1]);
          mypaint_brush_set_base_value (brush,
                                        MYPAINT_BRUSH_SETTING_COLOR_V,
                                        hsv[2]);
        }

      mypaint_brush_set_base_value (brush,
                                    MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC,
                                    options->radius);
      mypaint_brush_set_base_value (brush,
                                    MYPAINT_BRUSH_SETTING_OPAQUE,
                                    options->opaque *
                                    gimp_context_get_opacity (context));
      mypaint_brush_set_base_value (brush,
                                    MYPAINT_BRUSH_SETTING_HARDNESS,
                                    options->hardness);
      mypaint_brush_set_base_value (brush,
                                    MYPAINT_BRUSH_SETTING_ERASER,
                                    (options->eraser &&
                                     gimp_drawable_has_alpha (drawable)) ?
                                    1.0f : 0.0f);

      mypaint_brush_new_stroke (brush);

      mybrush->private->brushes = g_list_prepend (mybrush->private->brushes,
                                                  brush);
    }

  mybrush->private->brushes = g_list_reverse (mybrush->private->brushes);
}
