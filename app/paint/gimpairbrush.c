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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "paint-types.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpsymmetry.h"

#include "gimpairbrush.h"
#include "gimpairbrushoptions.h"

#include "gimp-intl.h"


#define STAMP_MAX_FPS 60


enum
{
  STAMP,
  LAST_SIGNAL
};


static void       gimp_airbrush_finalize (GObject          *object);

static void       gimp_airbrush_paint    (GimpPaintCore    *paint_core,
                                          GList            *drawables,
                                          GimpPaintOptions *paint_options,
                                          GimpSymmetry     *sym,
                                          GimpPaintState    paint_state,
                                          guint32           time);
static void       gimp_airbrush_motion   (GimpPaintCore    *paint_core,
                                          GimpDrawable     *drawable,
                                          GimpPaintOptions *paint_options,
                                          GimpSymmetry     *sym);

static gboolean   gimp_airbrush_timeout  (gpointer          data);


G_DEFINE_TYPE (GimpAirbrush, gimp_airbrush, GIMP_TYPE_PAINTBRUSH)

#define parent_class gimp_airbrush_parent_class

static guint airbrush_signals[LAST_SIGNAL] = { 0 };


void
gimp_airbrush_register (Gimp                      *gimp,
                        GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_AIRBRUSH,
                GIMP_TYPE_AIRBRUSH_OPTIONS,
                "gimp-airbrush",
                _("Airbrush"),
                "gimp-tool-airbrush");
}

static void
gimp_airbrush_class_init (GimpAirbrushClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  object_class->finalize  = gimp_airbrush_finalize;

  paint_core_class->paint = gimp_airbrush_paint;

  airbrush_signals[STAMP] =
    g_signal_new ("stamp",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpAirbrushClass, stamp),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

static void
gimp_airbrush_init (GimpAirbrush *airbrush)
{
}

static void
gimp_airbrush_finalize (GObject *object)
{
  GimpAirbrush *airbrush = GIMP_AIRBRUSH (object);

  if (airbrush->timeout_id)
    {
      g_source_remove (airbrush->timeout_id);
      airbrush->timeout_id = 0;
    }

  g_clear_object (&airbrush->sym);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_airbrush_paint (GimpPaintCore    *paint_core,
                     GList            *drawables,
                     GimpPaintOptions *paint_options,
                     GimpSymmetry     *sym,
                     GimpPaintState    paint_state,
                     guint32           time)
{
  GimpAirbrush        *airbrush = GIMP_AIRBRUSH (paint_core);
  GimpAirbrushOptions *options  = GIMP_AIRBRUSH_OPTIONS (paint_options);
  GimpDynamics        *dynamics = GIMP_BRUSH_CORE (paint_core)->dynamics;
  GimpCoords           coords;

  g_return_if_fail (g_list_length (drawables) == 1);

  if (airbrush->timeout_id)
    {
      g_source_remove (airbrush->timeout_id);
      airbrush->timeout_id = 0;
    }

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      GIMP_PAINT_CORE_CLASS (parent_class)->paint (paint_core, drawables,
                                                   paint_options,
                                                   sym,
                                                   paint_state, time);
      break;

    case GIMP_PAINT_STATE_MOTION:
      coords = *(gimp_symmetry_get_origin (sym));
      gimp_airbrush_motion (paint_core, drawables->data, paint_options, sym);

      if ((options->rate != 0.0) && ! options->motion_only)
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (drawables->data));
          gdouble    fade_point;
          gdouble    dynamic_rate;
          gint       timeout;

          fade_point = gimp_paint_options_get_fade (paint_options, image,
                                                    paint_core->pixel_dist);

          airbrush->drawable      = drawables->data;
          airbrush->paint_options = paint_options;

          gimp_symmetry_set_origin (sym, drawables->data, &coords);
          if (airbrush->sym)
            g_object_unref (airbrush->sym);
          airbrush->sym = g_object_ref (sym);

          /* Base our timeout on the original stroke. */
          airbrush->coords = coords;

          dynamic_rate = gimp_dynamics_get_linear_value (dynamics,
                                                         GIMP_DYNAMICS_OUTPUT_RATE,
                                                         &coords,
                                                         paint_options,
                                                         fade_point);

          timeout = (1000.0 / STAMP_MAX_FPS) /
                    ((options->rate / 100.0) * dynamic_rate);

          airbrush->timeout_id = g_timeout_add_full (G_PRIORITY_HIGH,
                                                     timeout,
                                                     gimp_airbrush_timeout,
                                                     airbrush, NULL);
        }
      break;

    case GIMP_PAINT_STATE_FINISH:
      GIMP_PAINT_CORE_CLASS (parent_class)->paint (paint_core, drawables,
                                                   paint_options,
                                                   sym,
                                                   paint_state, time);

      g_clear_object (&airbrush->sym);
      break;
    }
}

static void
gimp_airbrush_motion (GimpPaintCore    *paint_core,
                      GimpDrawable     *drawable,
                      GimpPaintOptions *paint_options,
                      GimpSymmetry     *sym)

{
  GimpAirbrushOptions *options  = GIMP_AIRBRUSH_OPTIONS (paint_options);
  GimpDynamics        *dynamics = GIMP_BRUSH_CORE (paint_core)->dynamics;
  GimpImage           *image    = gimp_item_get_image (GIMP_ITEM (drawable));
  gdouble              opacity;
  gdouble              fade_point;
  GimpCoords          *coords;

  fade_point = gimp_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  coords = gimp_symmetry_get_origin (sym);

  opacity = (options->flow / 100.0 *
             gimp_dynamics_get_linear_value (dynamics,
                                             GIMP_DYNAMICS_OUTPUT_FLOW,
                                             coords,
                                             paint_options,
                                             fade_point));

  _gimp_paintbrush_motion (paint_core, drawable, paint_options,
                           sym, opacity);
}

static gboolean
gimp_airbrush_timeout (gpointer data)
{
  GimpAirbrush *airbrush = GIMP_AIRBRUSH (data);

  airbrush->timeout_id = 0;

  g_signal_emit (airbrush, airbrush_signals[STAMP], 0);

  return G_SOURCE_REMOVE;
}


/*  public functions  */


void
gimp_airbrush_stamp (GimpAirbrush *airbrush)
{
  GList *drawables;

  g_return_if_fail (GIMP_IS_AIRBRUSH (airbrush));

  gimp_symmetry_set_origin (airbrush->sym,
                            airbrush->drawable, &airbrush->coords);

  drawables = g_list_prepend (NULL, airbrush->drawable),
  gimp_airbrush_paint (GIMP_PAINT_CORE (airbrush),
                       drawables,
                       airbrush->paint_options,
                       airbrush->sym,
                       GIMP_PAINT_STATE_MOTION, 0);
  g_list_free (drawables);

  gimp_symmetry_clear_origin (airbrush->sym);
}
