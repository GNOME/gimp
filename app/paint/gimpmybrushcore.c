/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"

#include "paint-types.h"

#include "gegl/ligma-gegl-utils.h"

#include "core/ligma.h"
#include "core/ligma-palettes.h"
#include "core/ligmadrawable.h"
#include "core/ligmaerror.h"
#include "core/ligmamybrush.h"
#include "core/ligmapickable.h"
#include "core/ligmasymmetry.h"

#include "ligmamybrushcore.h"
#include "ligmamybrushsurface.h"
#include "ligmamybrushoptions.h"

#include "ligma-intl.h"


struct _LigmaMybrushCorePrivate
{
  LigmaMybrush        *mybrush;
  LigmaMybrushSurface *surface;
  GList              *brushes;
  gboolean            synthetic;
  gint64              last_time;
};


/*  local function prototypes  */

static void      ligma_mybrush_core_finalize       (GObject           *object);

static gboolean  ligma_mybrush_core_start          (LigmaPaintCore     *paint_core,
                                                   GList             *drawables,
                                                   LigmaPaintOptions  *paint_options,
                                                   const LigmaCoords  *coords,
                                                   GError           **error);
static void      ligma_mybrush_core_interpolate    (LigmaPaintCore     *paint_core,
                                                   GList             *drawables,
                                                   LigmaPaintOptions  *paint_options,
                                                   guint32            time);
static void      ligma_mybrush_core_paint          (LigmaPaintCore     *paint_core,
                                                   GList             *drawables,
                                                   LigmaPaintOptions  *paint_options,
                                                   LigmaSymmetry      *sym,
                                                   LigmaPaintState     paint_state,
                                                   guint32            time);
static void      ligma_mybrush_core_motion         (LigmaPaintCore     *paint_core,
                                                   LigmaDrawable      *drawable,
                                                   LigmaPaintOptions  *paint_options,
                                                   LigmaSymmetry      *sym,
                                                   guint32            time);
static void      ligma_mybrush_core_create_brushes (LigmaMybrushCore   *mybrush,
                                                   LigmaDrawable      *drawable,
                                                   LigmaPaintOptions  *paint_options,
                                                   LigmaSymmetry      *sym);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaMybrushCore, ligma_mybrush_core,
                            LIGMA_TYPE_PAINT_CORE)

#define parent_class ligma_mybrush_core_parent_class


void
ligma_mybrush_core_register (Ligma                      *ligma,
                            LigmaPaintRegisterCallback  callback)
{
  (* callback) (ligma,
                LIGMA_TYPE_MYBRUSH_CORE,
                LIGMA_TYPE_MYBRUSH_OPTIONS,
                "ligma-mybrush",
                _("Mybrush"),
                "ligma-tool-mypaint-brush");
}

static void
ligma_mybrush_core_class_init (LigmaMybrushCoreClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  LigmaPaintCoreClass *paint_core_class = LIGMA_PAINT_CORE_CLASS (klass);

  object_class->finalize        = ligma_mybrush_core_finalize;

  paint_core_class->start       = ligma_mybrush_core_start;
  paint_core_class->paint       = ligma_mybrush_core_paint;
  paint_core_class->interpolate = ligma_mybrush_core_interpolate;
}

static void
ligma_mybrush_core_init (LigmaMybrushCore *mybrush)
{
  mybrush->private = ligma_mybrush_core_get_instance_private (mybrush);
}

static void
ligma_mybrush_core_finalize (GObject *object)
{
  LigmaMybrushCore *core = LIGMA_MYBRUSH_CORE (object);

  if (core->private->brushes)
    {
      g_list_free_full (core->private->brushes,
                        (GDestroyNotify) mypaint_brush_unref);
      core->private->brushes = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
ligma_mybrush_core_start (LigmaPaintCore     *paint_core,
                         GList             *drawables,
                         LigmaPaintOptions  *paint_options,
                         const LigmaCoords  *coords,
                         GError           **error)
{
  LigmaMybrushCore *core    = LIGMA_MYBRUSH_CORE (paint_core);
  LigmaContext     *context = LIGMA_CONTEXT (paint_options);

  core->private->mybrush = ligma_context_get_mybrush (context);

  if (! core->private->mybrush)
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("No MyPaint brushes available for use with this tool."));
      return FALSE;
    }

  return TRUE;
}

static void
ligma_mybrush_core_interpolate (LigmaPaintCore    *paint_core,
                               GList            *drawables,
                               LigmaPaintOptions *paint_options,
                               guint32           time)
{
  LigmaMybrushCore *mybrush = LIGMA_MYBRUSH_CORE (paint_core);

  /* If this is the first motion the brush has received then
   * we're being asked to draw a synthetic stroke in line mode
   */
  if (mybrush->private->last_time < 0)
    {
      LigmaCoords saved_coords = paint_core->cur_coords;

      paint_core->cur_coords = paint_core->last_coords;

      mybrush->private->synthetic = TRUE;

      ligma_paint_core_paint (paint_core, drawables, paint_options,
                             LIGMA_PAINT_STATE_MOTION, time);

      paint_core->cur_coords = saved_coords;
    }

  ligma_paint_core_paint (paint_core, drawables, paint_options,
                         LIGMA_PAINT_STATE_MOTION, time);

  paint_core->last_coords = paint_core->cur_coords;
}

static void
ligma_mybrush_core_paint (LigmaPaintCore    *paint_core,
                         GList            *drawables,
                         LigmaPaintOptions *paint_options,
                         LigmaSymmetry     *sym,
                         LigmaPaintState    paint_state,
                         guint32           time)
{
  LigmaMybrushCore *mybrush = LIGMA_MYBRUSH_CORE (paint_core);
  LigmaContext     *context = LIGMA_CONTEXT (paint_options);
  gint             offset_x;
  gint             offset_y;
  LigmaRGB          fg;

  g_return_if_fail (g_list_length (drawables) == 1);

  switch (paint_state)
    {
    case LIGMA_PAINT_STATE_INIT:
      ligma_context_get_foreground (context, &fg);
      ligma_palettes_add_color_history (context->ligma, &fg);
      ligma_symmetry_set_stateful (sym, TRUE);

      ligma_item_get_offset (drawables->data, &offset_x, &offset_y);
      mybrush->private->surface =
        ligma_mypaint_surface_new (ligma_drawable_get_buffer (drawables->data),
                                  ligma_drawable_get_active_mask (drawables->data),
                                  paint_core->mask_buffer,
                                  -offset_x, -offset_y,
                                  LIGMA_MYBRUSH_OPTIONS (paint_options));

      ligma_mybrush_core_create_brushes (mybrush, drawables->data, paint_options, sym);

      mybrush->private->last_time = -1;
      mybrush->private->synthetic = FALSE;
      break;

    case LIGMA_PAINT_STATE_MOTION:
      ligma_mybrush_core_motion (paint_core, drawables->data, paint_options,
                                sym, time);
      break;

    case LIGMA_PAINT_STATE_FINISH:
      ligma_symmetry_set_stateful (sym, FALSE);
      mypaint_surface_unref ((MyPaintSurface *) mybrush->private->surface);
      mybrush->private->surface = NULL;

      g_list_free_full (mybrush->private->brushes,
                        (GDestroyNotify) mypaint_brush_unref);
      mybrush->private->brushes = NULL;
      break;
    }
}

static void
ligma_mybrush_core_motion (LigmaPaintCore    *paint_core,
                          LigmaDrawable     *drawable,
                          LigmaPaintOptions *paint_options,
                          LigmaSymmetry     *sym,
                          guint32           time)
{
  LigmaMybrushCore  *mybrush = LIGMA_MYBRUSH_CORE (paint_core);
  MyPaintRectangle  rect;
  GList            *iter;
  gdouble           dt = 0.0;
  gint              off_x, off_y;
  gint              n_strokes;
  gint              i;

  ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);
  n_strokes = ligma_symmetry_get_size (sym);

  /* The number of strokes may change during a motion, depending on
   * the type of symmetry. When that happens, reset the brushes.
   */
  if (g_list_length (mybrush->private->brushes) != n_strokes)
    {
      ligma_mybrush_core_create_brushes (mybrush, drawable, paint_options, sym);
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
          LigmaCoords    coords = *(ligma_symmetry_get_coords (sym, i));

          mypaint_brush_stroke_to (brush,
                                   (MyPaintSurface *) mybrush->private->surface,
                                   coords.x - off_x,
                                   coords.y - off_y,
                                   0.0f,
                                   coords.xtilt,
                                   coords.ytilt,
                                   1.0f /* Pretend the cursor hasn't moved in a while */);
        }

      dt = 0.015;
    }
  else if (mybrush->private->synthetic)
    {
      LigmaVector2 v = { paint_core->cur_coords.x - paint_core->last_coords.x,
                        paint_core->cur_coords.y - paint_core->last_coords.y };

      dt = 0.0005 * ligma_vector2_length_val (v);
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
      LigmaCoords    coords   = *(ligma_symmetry_get_coords (sym, i));
      gdouble       pressure = coords.pressure;

      mypaint_brush_stroke_to (brush,
                               (MyPaintSurface *) mybrush->private->surface,
                               coords.x - off_x,
                               coords.y - off_y,
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

      ligma_drawable_update (drawable, rect.x, rect.y, rect.width, rect.height);
    }
}

static void
ligma_mybrush_core_create_brushes (LigmaMybrushCore  *mybrush,
                                  LigmaDrawable     *drawable,
                                  LigmaPaintOptions *paint_options,
                                  LigmaSymmetry     *sym)
{
  LigmaMybrushOptions *options = LIGMA_MYBRUSH_OPTIONS (paint_options);
  LigmaContext        *context = LIGMA_CONTEXT (paint_options);
  LigmaRGB             fg;
  LigmaHSV             hsv;
  gint                n_strokes;
  gint                i;

  if (mybrush->private->brushes)
    {
      g_list_free_full (mybrush->private->brushes,
                        (GDestroyNotify) mypaint_brush_unref);
      mybrush->private->brushes = NULL;
    }

  if (options->eraser)
    ligma_context_get_background (context, &fg);
  else
    ligma_context_get_foreground (context, &fg);

  ligma_pickable_srgb_to_image_color (LIGMA_PICKABLE (drawable),
                                     &fg, &fg);
  ligma_rgb_to_hsv (&fg, &hsv);

  n_strokes = ligma_symmetry_get_size (sym);

  for (i = 0; i < n_strokes; i++)
    {
      MyPaintBrush *brush = mypaint_brush_new ();
      const gchar  *brush_data;

      mypaint_brush_from_defaults (brush);
      brush_data = ligma_mybrush_get_brush_json (mybrush->private->mybrush);
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
                                    ligma_context_get_opacity (context));
      mypaint_brush_set_base_value (brush,
                                    MYPAINT_BRUSH_SETTING_HARDNESS,
                                    options->hardness);
      mypaint_brush_set_base_value (brush,
                                    MYPAINT_BRUSH_SETTING_ERASER,
                                    (options->eraser &&
                                     ligma_drawable_has_alpha (drawable)) ?
                                    1.0f : 0.0f);

      mypaint_brush_new_stroke (brush);

      mybrush->private->brushes = g_list_prepend (mybrush->private->brushes,
                                                  brush);
    }

  mybrush->private->brushes = g_list_reverse (mybrush->private->brushes);
}
