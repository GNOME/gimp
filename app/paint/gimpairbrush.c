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


static void       gimp_airbrush_finalize (GObject          *object);

static void       gimp_airbrush_paint    (GimpPaintCore    *paint_core,
                                          GimpDrawable     *drawable,
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
}

static void
gimp_airbrush_init (GimpAirbrush *airbrush)
{
  airbrush->timeout_id = 0;
  airbrush->sym    = NULL;
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

  if (airbrush->sym)
    g_object_unref (airbrush->sym);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_airbrush_paint (GimpPaintCore    *paint_core,
                     GimpDrawable     *drawable,
                     GimpPaintOptions *paint_options,
                     GimpSymmetry     *sym,
                     GimpPaintState    paint_state,
                     guint32           time)
{
  GimpAirbrush        *airbrush = GIMP_AIRBRUSH (paint_core);
  GimpAirbrushOptions *options  = GIMP_AIRBRUSH_OPTIONS (paint_options);
  GimpDynamics        *dynamics = GIMP_BRUSH_CORE (paint_core)->dynamics;

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      if (airbrush->timeout_id)
        {
          g_source_remove (airbrush->timeout_id);
          airbrush->timeout_id = 0;
        }

      GIMP_PAINT_CORE_CLASS (parent_class)->paint (paint_core, drawable,
                                                   paint_options,
                                                   sym,
                                                   paint_state, time);
      break;

    case GIMP_PAINT_STATE_MOTION:
      if (airbrush->timeout_id)
        {
          g_source_remove (airbrush->timeout_id);
          airbrush->timeout_id = 0;
        }

      gimp_airbrush_motion (paint_core, drawable, paint_options, sym);

      if ((options->rate != 0.0) && (!options->motion_only))
        {
          GimpImage  *image = gimp_item_get_image (GIMP_ITEM (drawable));
          gdouble     fade_point;
          gdouble     dynamic_rate;
          gint        timeout;
          GimpCoords *coords;

          fade_point = gimp_paint_options_get_fade (paint_options, image,
                                                    paint_core->pixel_dist);

          airbrush->drawable      = drawable;
          airbrush->paint_options = paint_options;

          if (airbrush->sym)
            g_object_unref (airbrush->sym);
          airbrush->sym = g_object_ref (sym);

          /* Base our timeout on the original stroke. */
          coords = gimp_symmetry_get_origin (sym);

          dynamic_rate = gimp_dynamics_get_linear_value (dynamics,
                                                         GIMP_DYNAMICS_OUTPUT_RATE,
                                                         coords,
                                                         paint_options,
                                                         fade_point);

          timeout = 10000 / (options->rate * dynamic_rate);

          airbrush->timeout_id = g_timeout_add (timeout,
                                                gimp_airbrush_timeout,
                                                airbrush);
        }
      break;

    case GIMP_PAINT_STATE_FINISH:
      if (airbrush->timeout_id)
        {
          g_source_remove (airbrush->timeout_id);
          airbrush->timeout_id = 0;
        }

      GIMP_PAINT_CORE_CLASS (parent_class)->paint (paint_core, drawable,
                                                   paint_options,
                                                   sym,
                                                   paint_state, time);
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

  gimp_airbrush_paint (GIMP_PAINT_CORE (airbrush),
                       airbrush->drawable,
                       airbrush->paint_options,
                       airbrush->sym,
                       GIMP_PAINT_STATE_MOTION, 0);

  gimp_image_flush (gimp_item_get_image (GIMP_ITEM (airbrush->drawable)));

  return FALSE;
}
