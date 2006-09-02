/* The GIMP -- an image manipulation program
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

#include "libgimpbase/gimpbase.h"

#include "paint-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "gimpsourcecore.h"
#include "gimpsourceoptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_SRC_DRAWABLE,
  PROP_SRC_X,
  PROP_SRC_Y
};


static void   gimp_source_core_set_property     (GObject          *object,
                                                 guint             property_id,
                                                 const GValue     *value,
                                                 GParamSpec       *pspec);
static void   gimp_source_core_get_property     (GObject          *object,
                                                 guint             property_id,
                                                 GValue           *value,
                                                 GParamSpec       *pspec);

static void   gimp_source_core_paint            (GimpPaintCore    *paint_core,
                                                 GimpDrawable     *drawable,
                                                 GimpPaintOptions *paint_options,
                                                 GimpPaintState    paint_state,
                                                 guint32           time);

static void   gimp_source_core_motion           (GimpSourceCore   *source_core,
                                                 GimpDrawable     *drawable,
                                                 GimpPaintOptions *paint_options);
static void   gimp_source_core_set_src_drawable (GimpSourceCore   *source_core,
                                                 GimpDrawable     *drawable);


G_DEFINE_TYPE (GimpSourceCore, gimp_source_core, GIMP_TYPE_BRUSH_CORE)


static void
gimp_source_core_class_init (GimpSourceCoreClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);
  GimpBrushCoreClass *brush_core_class = GIMP_BRUSH_CORE_CLASS (klass);

  object_class->set_property               = gimp_source_core_set_property;
  object_class->get_property               = gimp_source_core_get_property;

  paint_core_class->paint                  = gimp_source_core_paint;

  brush_core_class->handles_changing_brush = TRUE;

  klass->motion                            = NULL;

  g_object_class_install_property (object_class, PROP_SRC_DRAWABLE,
                                   g_param_spec_object ("src-drawable",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DRAWABLE,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SRC_X,
                                   g_param_spec_double ("src-x", NULL, NULL,
                                                        0, GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SRC_Y,
                                   g_param_spec_double ("src-y", NULL, NULL,
                                                        0, GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_source_core_init (GimpSourceCore *source_core)
{
  source_core->set_source   = FALSE;

  source_core->src_drawable = NULL;
  source_core->src_x        = 0.0;
  source_core->src_y        = 0.0;

  source_core->orig_src_x   = 0.0;
  source_core->orig_src_y   = 0.0;

  source_core->offset_x     = 0.0;
  source_core->offset_y     = 0.0;
  source_core->first_stroke = TRUE;
}

static void
gimp_source_core_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpSourceCore *source_core = GIMP_SOURCE_CORE (object);

  switch (property_id)
    {
    case PROP_SRC_DRAWABLE:
      gimp_source_core_set_src_drawable (source_core,
                                         g_value_get_object (value));
      break;
    case PROP_SRC_X:
      source_core->src_x = g_value_get_double (value);
      break;
    case PROP_SRC_Y:
      source_core->src_y = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_source_core_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpSourceCore *source_core = GIMP_SOURCE_CORE (object);

  switch (property_id)
    {
    case PROP_SRC_DRAWABLE:
      g_value_set_object (value, source_core->src_drawable);
      break;
    case PROP_SRC_X:
      g_value_set_int (value, source_core->src_x);
      break;
    case PROP_SRC_Y:
      g_value_set_int (value, source_core->src_y);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_source_core_paint (GimpPaintCore    *paint_core,
                        GimpDrawable     *drawable,
                        GimpPaintOptions *paint_options,
                        GimpPaintState    paint_state,
                        guint32           time)
{
  GimpSourceCore    *source_core = GIMP_SOURCE_CORE (paint_core);
  GimpSourceOptions *options     = GIMP_SOURCE_OPTIONS (paint_options);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      if (source_core->set_source)
        {
          gimp_source_core_set_src_drawable (source_core, drawable);

          source_core->src_x = paint_core->cur_coords.x;
          source_core->src_y = paint_core->cur_coords.y;

          source_core->first_stroke = TRUE;
        }
      else if (options->align_mode == GIMP_SOURCE_ALIGN_NO)
        {
          source_core->orig_src_x = source_core->src_x;
          source_core->orig_src_y = source_core->src_y;

          source_core->first_stroke = TRUE;
        }
      break;

    case GIMP_PAINT_STATE_MOTION:
      if (source_core->set_source)
        {
          /*  If the control key is down, move the src target and return */

          source_core->src_x = paint_core->cur_coords.x;
          source_core->src_y = paint_core->cur_coords.y;

          source_core->first_stroke = TRUE;
        }
      else
        {
          /*  otherwise, update the target  */

          gint dest_x;
          gint dest_y;

          dest_x = paint_core->cur_coords.x;
          dest_y = paint_core->cur_coords.y;

          if (options->align_mode == GIMP_SOURCE_ALIGN_REGISTERED)
            {
              source_core->offset_x = 0;
              source_core->offset_y = 0;
            }
          else if (options->align_mode == GIMP_SOURCE_ALIGN_FIXED)
            {
              source_core->offset_x = source_core->src_x - dest_x;
              source_core->offset_y = source_core->src_y - dest_y;
            }
          else if (source_core->first_stroke)
            {
              source_core->offset_x = source_core->src_x - dest_x;
              source_core->offset_y = source_core->src_y - dest_y;

              source_core->first_stroke = FALSE;
            }

          source_core->src_x = dest_x + source_core->offset_x;
          source_core->src_y = dest_y + source_core->offset_y;

          gimp_source_core_motion (source_core, drawable, paint_options);
        }
      break;

    case GIMP_PAINT_STATE_FINISH:
      if (options->align_mode == GIMP_SOURCE_ALIGN_NO &&
          ! source_core->first_stroke)
        {
          source_core->src_x = source_core->orig_src_x;
          source_core->src_y = source_core->orig_src_y;
        }
      break;

    default:
      break;
    }

  g_object_notify (G_OBJECT (source_core), "src-x");
  g_object_notify (G_OBJECT (source_core), "src-y");
}

static void
gimp_source_core_motion (GimpSourceCore   *source_core,
                         GimpDrawable     *drawable,
                         GimpPaintOptions *paint_options)
{
  GIMP_SOURCE_CORE_GET_CLASS (source_core)->motion (source_core,
                                                    drawable,
                                                    paint_options);
}

static void
gimp_source_core_src_drawable_removed (GimpDrawable   *drawable,
                                       GimpSourceCore *source_core)
{
  if (drawable == source_core->src_drawable)
    {
      source_core->src_drawable = NULL;
    }

  g_signal_handlers_disconnect_by_func (drawable,
                                        gimp_source_core_src_drawable_removed,
                                        source_core);
}

static void
gimp_source_core_set_src_drawable (GimpSourceCore *source_core,
                                   GimpDrawable   *drawable)
{
  if (source_core->src_drawable == drawable)
    return;

  if (source_core->src_drawable)
    g_signal_handlers_disconnect_by_func (source_core->src_drawable,
                                          gimp_source_core_src_drawable_removed,
                                          source_core);

  source_core->src_drawable = drawable;

  if (source_core->src_drawable)
    g_signal_connect (source_core->src_drawable, "removed",
                      G_CALLBACK (gimp_source_core_src_drawable_removed),
                      source_core);

  g_object_notify (G_OBJECT (source_core), "src-drawable");
}
