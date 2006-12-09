/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"

#include "paint-types.h"

#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"

#include "gimpairbrush.h"
#include "gimpairbrushoptions.h"

#include "gimp-intl.h"


static void       gimp_airbrush_finalize (GObject          *object);

static void       gimp_airbrush_paint    (GimpPaintCore    *paint_core,
                                          GimpDrawable     *drawable,
                                          GimpPaintOptions *paint_options,
                                          GimpPaintState    paint_state,
                                          guint32           time);
static void       gimp_airbrush_motion   (GimpPaintCore    *paint_core,
                                          GimpDrawable     *drawable,
                                          GimpPaintOptions *paint_options);
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

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_airbrush_paint (GimpPaintCore    *paint_core,
                     GimpDrawable     *drawable,
                     GimpPaintOptions *paint_options,
                     GimpPaintState    paint_state,
                     guint32           time)
{
  GimpAirbrush        *airbrush = GIMP_AIRBRUSH (paint_core);
  GimpAirbrushOptions *options  = GIMP_AIRBRUSH_OPTIONS (paint_options);

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
                                                   paint_state, time);
      break;

    case GIMP_PAINT_STATE_MOTION:
      if (airbrush->timeout_id)
        {
          g_source_remove (airbrush->timeout_id);
          airbrush->timeout_id = 0;
        }

      gimp_airbrush_motion (paint_core, drawable, paint_options);

      if (options->rate != 0.0)
        {
          gdouble timeout;

          airbrush->drawable      = drawable;
          airbrush->paint_options = paint_options;

          timeout = (paint_options->pressure_options->rate ?
                  (10000 / (options->rate * PRESSURE_SCALE * paint_core->cur_coords.pressure)) :
            (10000 / options->rate));

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
                                                   paint_state, time);
      break;
    }
}

static void
gimp_airbrush_motion (GimpPaintCore    *paint_core,
                      GimpDrawable     *drawable,
                      GimpPaintOptions *paint_options)
{
  GimpAirbrushOptions *options = GIMP_AIRBRUSH_OPTIONS (paint_options);
  gdouble              opacity;
  gboolean             saved_pressure;

  opacity = options->pressure / 100.0;

  saved_pressure = paint_options->pressure_options->hardness;

  if (saved_pressure)
    opacity *= PRESSURE_SCALE * paint_core->cur_coords.pressure;

  paint_options->pressure_options->hardness = FALSE;
  _gimp_paintbrush_motion (paint_core, drawable, paint_options, opacity);
  paint_options->pressure_options->hardness = saved_pressure;
}

static gboolean
gimp_airbrush_timeout (gpointer data)
{
  GimpAirbrush *airbrush = GIMP_AIRBRUSH (data);

  gimp_airbrush_paint (GIMP_PAINT_CORE (airbrush),
                       airbrush->drawable,
                       airbrush->paint_options,
                       GIMP_PAINT_STATE_MOTION, 0);

  gimp_image_flush (gimp_item_get_image (GIMP_ITEM (airbrush->drawable)));

  return FALSE;
}
