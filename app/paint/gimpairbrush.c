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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "paint-types.h"

#include "core/ligma.h"
#include "core/ligmabrush.h"
#include "core/ligmadrawable.h"
#include "core/ligmadynamics.h"
#include "core/ligmagradient.h"
#include "core/ligmaimage.h"
#include "core/ligmasymmetry.h"

#include "ligmaairbrush.h"
#include "ligmaairbrushoptions.h"

#include "ligma-intl.h"


#define STAMP_MAX_FPS 60


enum
{
  STAMP,
  LAST_SIGNAL
};


static void       ligma_airbrush_finalize (GObject          *object);

static void       ligma_airbrush_paint    (LigmaPaintCore    *paint_core,
                                          GList            *drawables,
                                          LigmaPaintOptions *paint_options,
                                          LigmaSymmetry     *sym,
                                          LigmaPaintState    paint_state,
                                          guint32           time);
static void       ligma_airbrush_motion   (LigmaPaintCore    *paint_core,
                                          LigmaDrawable     *drawable,
                                          LigmaPaintOptions *paint_options,
                                          LigmaSymmetry     *sym);

static gboolean   ligma_airbrush_timeout  (gpointer          data);


G_DEFINE_TYPE (LigmaAirbrush, ligma_airbrush, LIGMA_TYPE_PAINTBRUSH)

#define parent_class ligma_airbrush_parent_class

static guint airbrush_signals[LAST_SIGNAL] = { 0 };


void
ligma_airbrush_register (Ligma                      *ligma,
                        LigmaPaintRegisterCallback  callback)
{
  (* callback) (ligma,
                LIGMA_TYPE_AIRBRUSH,
                LIGMA_TYPE_AIRBRUSH_OPTIONS,
                "ligma-airbrush",
                _("Airbrush"),
                "ligma-tool-airbrush");
}

static void
ligma_airbrush_class_init (LigmaAirbrushClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  LigmaPaintCoreClass *paint_core_class = LIGMA_PAINT_CORE_CLASS (klass);

  object_class->finalize  = ligma_airbrush_finalize;

  paint_core_class->paint = ligma_airbrush_paint;

  airbrush_signals[STAMP] =
    g_signal_new ("stamp",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaAirbrushClass, stamp),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

static void
ligma_airbrush_init (LigmaAirbrush *airbrush)
{
}

static void
ligma_airbrush_finalize (GObject *object)
{
  LigmaAirbrush *airbrush = LIGMA_AIRBRUSH (object);

  if (airbrush->timeout_id)
    {
      g_source_remove (airbrush->timeout_id);
      airbrush->timeout_id = 0;
    }

  g_clear_object (&airbrush->sym);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_airbrush_paint (LigmaPaintCore    *paint_core,
                     GList            *drawables,
                     LigmaPaintOptions *paint_options,
                     LigmaSymmetry     *sym,
                     LigmaPaintState    paint_state,
                     guint32           time)
{
  LigmaAirbrush        *airbrush = LIGMA_AIRBRUSH (paint_core);
  LigmaAirbrushOptions *options  = LIGMA_AIRBRUSH_OPTIONS (paint_options);
  LigmaDynamics        *dynamics = LIGMA_BRUSH_CORE (paint_core)->dynamics;

  g_return_if_fail (g_list_length (drawables) == 1);

  if (airbrush->timeout_id)
    {
      g_source_remove (airbrush->timeout_id);
      airbrush->timeout_id = 0;
    }

  switch (paint_state)
    {
    case LIGMA_PAINT_STATE_INIT:
      LIGMA_PAINT_CORE_CLASS (parent_class)->paint (paint_core, drawables,
                                                   paint_options,
                                                   sym,
                                                   paint_state, time);
      break;

    case LIGMA_PAINT_STATE_MOTION:
      ligma_airbrush_motion (paint_core, drawables->data, paint_options, sym);

      if ((options->rate != 0.0) && ! options->motion_only)
        {
          LigmaImage  *image = ligma_item_get_image (LIGMA_ITEM (drawables->data));
          gdouble     fade_point;
          gdouble     dynamic_rate;
          gint        timeout;
          LigmaCoords *coords;

          fade_point = ligma_paint_options_get_fade (paint_options, image,
                                                    paint_core->pixel_dist);

          airbrush->drawable      = drawables->data;
          airbrush->paint_options = paint_options;

          if (airbrush->sym)
            g_object_unref (airbrush->sym);
          airbrush->sym = g_object_ref (sym);

          /* Base our timeout on the original stroke. */
          coords = ligma_symmetry_get_origin (sym);

          airbrush->coords = *coords;

          dynamic_rate = ligma_dynamics_get_linear_value (dynamics,
                                                         LIGMA_DYNAMICS_OUTPUT_RATE,
                                                         coords,
                                                         paint_options,
                                                         fade_point);

          timeout = (1000.0 / STAMP_MAX_FPS) /
                    ((options->rate / 100.0) * dynamic_rate);

          airbrush->timeout_id = g_timeout_add_full (G_PRIORITY_HIGH,
                                                     timeout,
                                                     ligma_airbrush_timeout,
                                                     airbrush, NULL);
        }
      break;

    case LIGMA_PAINT_STATE_FINISH:
      LIGMA_PAINT_CORE_CLASS (parent_class)->paint (paint_core, drawables,
                                                   paint_options,
                                                   sym,
                                                   paint_state, time);

      g_clear_object (&airbrush->sym);
      break;
    }
}

static void
ligma_airbrush_motion (LigmaPaintCore    *paint_core,
                      LigmaDrawable     *drawable,
                      LigmaPaintOptions *paint_options,
                      LigmaSymmetry     *sym)

{
  LigmaAirbrushOptions *options  = LIGMA_AIRBRUSH_OPTIONS (paint_options);
  LigmaDynamics        *dynamics = LIGMA_BRUSH_CORE (paint_core)->dynamics;
  LigmaImage           *image    = ligma_item_get_image (LIGMA_ITEM (drawable));
  gdouble              opacity;
  gdouble              fade_point;
  LigmaCoords          *coords;

  fade_point = ligma_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  coords = ligma_symmetry_get_origin (sym);

  opacity = (options->flow / 100.0 *
             ligma_dynamics_get_linear_value (dynamics,
                                             LIGMA_DYNAMICS_OUTPUT_FLOW,
                                             coords,
                                             paint_options,
                                             fade_point));

  _ligma_paintbrush_motion (paint_core, drawable, paint_options,
                           sym, opacity);
}

static gboolean
ligma_airbrush_timeout (gpointer data)
{
  LigmaAirbrush *airbrush = LIGMA_AIRBRUSH (data);

  airbrush->timeout_id = 0;

  g_signal_emit (airbrush, airbrush_signals[STAMP], 0);

  return G_SOURCE_REMOVE;
}


/*  public functions  */


void
ligma_airbrush_stamp (LigmaAirbrush *airbrush)
{
  GList *drawables;

  g_return_if_fail (LIGMA_IS_AIRBRUSH (airbrush));

  ligma_symmetry_set_origin (airbrush->sym,
                            airbrush->drawable, &airbrush->coords);

  drawables = g_list_prepend (NULL, airbrush->drawable),
  ligma_airbrush_paint (LIGMA_PAINT_CORE (airbrush),
                       drawables,
                       airbrush->paint_options,
                       airbrush->sym,
                       LIGMA_PAINT_STATE_MOTION, 0);
  g_list_free (drawables);

  ligma_symmetry_clear_origin (airbrush->sym);
}
