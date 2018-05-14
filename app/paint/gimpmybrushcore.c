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
                                                   GimpDrawable      *drawable,
                                                   GimpPaintOptions  *paint_options,
                                                   const GimpCoords  *coords,
                                                   GError           **error);
static void      gimp_mybrush_core_interpolate    (GimpPaintCore     *paint_core,
                                                   GimpDrawable      *drawable,
                                                   GimpPaintOptions  *paint_options,
                                                   guint32            time);
static void      gimp_mybrush_core_paint          (GimpPaintCore     *paint_core,
                                                   GimpDrawable      *drawable,
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
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  object_class->finalize        = gimp_mybrush_core_finalize;

  paint_core_class->start       = gimp_mybrush_core_start;
  paint_core_class->paint       = gimp_mybrush_core_paint;
  paint_core_class->interpolate = gimp_mybrush_core_interpolate;

  g_type_class_add_private (klass, sizeof (GimpMybrushCorePrivate));
}

static void
gimp_mybrush_core_init (GimpMybrushCore *mybrush)
{
  mybrush->private = G_TYPE_INSTANCE_GET_PRIVATE (mybrush,
                                                  GIMP_TYPE_MYBRUSH_CORE,
                                                  GimpMybrushCorePrivate);
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
gimp_mybrush_core_interpolate (GimpPaintCore    *paint_core,
                               GimpDrawable     *drawable,
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

      gimp_paint_core_paint (paint_core, drawable, paint_options,
                             GIMP_PAINT_STATE_MOTION, time);

      paint_core->cur_coords = saved_coords;
    }

  gimp_paint_core_paint (paint_core, drawable, paint_options,
                         GIMP_PAINT_STATE_MOTION, time);

  paint_core->last_coords = paint_core->cur_coords;
}

static void
gimp_mybrush_core_paint (GimpPaintCore    *paint_core,
                         GimpDrawable     *drawable,
                         GimpPaintOptions *paint_options,
                         GimpSymmetry     *sym,
                         GimpPaintState    paint_state,
                         guint32           time)
{
  GimpMybrushCore *mybrush = GIMP_MYBRUSH_CORE (paint_core);
  GimpContext     *context = GIMP_CONTEXT (paint_options);
  GimpRGB          fg;

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      gimp_context_get_foreground (context, &fg);
      gimp_palettes_add_color_history (context->gimp, &fg);
      gimp_symmetry_set_stateful (sym, TRUE);

      mybrush->private->surface =
        gimp_mypaint_surface_new (gimp_drawable_get_buffer (drawable),
                                  gimp_drawable_get_active_mask (drawable),
                                  paint_core->mask_buffer,
                                  paint_core->mask_x_offset,
                                  paint_core->mask_y_offset,
                                  GIMP_MYBRUSH_OPTIONS (paint_options));

      gimp_mybrush_core_create_brushes (mybrush, drawable, paint_options, sym);

      mybrush->private->last_time = -1;
      mybrush->private->synthetic = FALSE;
      break;

    case GIMP_PAINT_STATE_MOTION:
      gimp_mybrush_core_motion (paint_core, drawable, paint_options,
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
  GList            *iter;
  gdouble           dt = 0.0;
  gint              n_strokes;
  gint              i;

  n_strokes = gimp_symmetry_get_size (sym);

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
          GimpCoords   *coords = gimp_symmetry_get_coords (sym, i);

          mypaint_brush_stroke_to (brush,
                                   (MyPaintSurface *) mybrush->private->surface,
                                   coords->x,
                                   coords->y,
                                   0.0f,
                                   coords->xtilt,
                                   coords->ytilt,
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
      GimpCoords   *coords   = gimp_symmetry_get_coords (sym, i);
      gdouble       pressure = coords->pressure;

      mypaint_brush_stroke_to (brush,
                               (MyPaintSurface *) mybrush->private->surface,
                               coords->x,
                               coords->y,
                               pressure,
                               coords->xtilt,
                               coords->ytilt,
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
  GimpRGB             fg;
  GimpHSV             hsv;
  gint                n_strokes;
  gint                i;

  if (mybrush->private->brushes)
    {
      g_list_free_full (mybrush->private->brushes,
                        (GDestroyNotify) mypaint_brush_unref);
      mybrush->private->brushes = NULL;
    }

  if (options->eraser)
    gimp_context_get_background (context, &fg);
  else
    gimp_context_get_foreground (context, &fg);

  gimp_pickable_srgb_to_image_color (GIMP_PICKABLE (drawable),
                                     &fg, &fg);
  gimp_rgb_to_hsv (&fg, &hsv);

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
                                        hsv.h);
          mypaint_brush_set_base_value (brush,
                                        MYPAINT_BRUSH_SETTING_COLOR_S,
                                        hsv.s);
          mypaint_brush_set_base_value (brush,
                                        MYPAINT_BRUSH_SETTING_COLOR_V,
                                        hsv.v);
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
