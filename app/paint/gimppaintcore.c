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

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimppickable.h"
#include "core/gimpprojection.h"

#include "gimppaintcore.h"
#include "gimppaintcoreundo.h"
#include "gimppaintoptions.h"

#include "gimpairbrush.h"

#include "gimp-intl.h"

#define STROKE_BUFFER_INIT_SIZE      2000

enum
{
  PROP_0,
  PROP_UNDO_DESC
};


/*  local function prototypes  */

static void      gimp_paint_core_finalize            (GObject          *object);
static void      gimp_paint_core_set_property        (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void      gimp_paint_core_get_property        (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static gboolean  gimp_paint_core_real_start          (GimpPaintCore    *core,
                                                      GimpDrawable     *drawable,
                                                      GimpPaintOptions *paint_options,
                                                      const GimpCoords *coords,
                                                      GError          **error);
static gboolean  gimp_paint_core_real_pre_paint      (GimpPaintCore    *core,
                                                      GimpDrawable     *drawable,
                                                      GimpPaintOptions *options,
                                                      GimpPaintState    paint_state,
                                                      guint32           time);
static void      gimp_paint_core_real_paint          (GimpPaintCore    *core,
                                                      GimpDrawable     *drawable,
                                                      GimpPaintOptions *options,
                                                      const GimpCoords *coords,
                                                      GimpPaintState    paint_state,
                                                      guint32           time);
static void      gimp_paint_core_real_post_paint     (GimpPaintCore    *core,
                                                      GimpDrawable     *drawable,
                                                      GimpPaintOptions *options,
                                                      GimpPaintState    paint_state,
                                                      guint32           time);
static void      gimp_paint_core_real_interpolate    (GimpPaintCore    *core,
                                                      GimpDrawable     *drawable,
                                                      GimpPaintOptions *options,
                                                      guint32           time);
static TempBuf * gimp_paint_core_real_get_paint_area (GimpPaintCore    *core,
                                                      GimpDrawable     *drawable,
                                                      GimpPaintOptions *options,
                                                      const GimpCoords *coords);
static GimpUndo* gimp_paint_core_real_push_undo      (GimpPaintCore    *core,
                                                      GimpImage        *image,
                                                      const gchar      *undo_desc);

static void      paint_mask_to_canvas_buffer         (GimpPaintCore    *core,
                                                      PixelRegion      *paint_maskPR,
                                                      gdouble           paint_opacity);
static void      paint_mask_to_paint_area            (GimpPaintCore    *core,
                                                      PixelRegion      *paint_maskPR,
                                                      gdouble           paint_opacity);
static void      canvas_buffer_to_paint_area         (GimpPaintCore    *core);


G_DEFINE_TYPE (GimpPaintCore, gimp_paint_core, GIMP_TYPE_OBJECT)

#define parent_class gimp_paint_core_parent_class

static gint global_core_ID = 1;


static void
gimp_paint_core_class_init (GimpPaintCoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_paint_core_finalize;
  object_class->set_property = gimp_paint_core_set_property;
  object_class->get_property = gimp_paint_core_get_property;

  klass->start               = gimp_paint_core_real_start;
  klass->pre_paint           = gimp_paint_core_real_pre_paint;
  klass->paint               = gimp_paint_core_real_paint;
  klass->post_paint          = gimp_paint_core_real_post_paint;
  klass->interpolate         = gimp_paint_core_real_interpolate;
  klass->get_paint_area      = gimp_paint_core_real_get_paint_area;
  klass->push_undo           = gimp_paint_core_real_push_undo;

  g_object_class_install_property (object_class, PROP_UNDO_DESC,
                                   g_param_spec_string ("undo-desc", NULL, NULL,
                                                        _("Paint"),
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_paint_core_init (GimpPaintCore *core)
{
  core->ID = global_core_ID++;
}

static void
gimp_paint_core_finalize (GObject *object)
{
  GimpPaintCore *core = GIMP_PAINT_CORE (object);

  gimp_paint_core_cleanup (core);

  g_free (core->undo_desc);
  core->undo_desc = NULL;

  if (core->stroke_buffer)
    {
      g_array_free (core->stroke_buffer, TRUE);
      core->stroke_buffer = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_paint_core_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpPaintCore *core = GIMP_PAINT_CORE (object);

  switch (property_id)
    {
    case PROP_UNDO_DESC:
      g_free (core->undo_desc);
      core->undo_desc = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_paint_core_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpPaintCore *core = GIMP_PAINT_CORE (object);

  switch (property_id)
    {
    case PROP_UNDO_DESC:
      g_value_set_string (value, core->undo_desc);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_paint_core_real_start (GimpPaintCore    *core,
                            GimpDrawable     *drawable,
                            GimpPaintOptions *paint_options,
                            const GimpCoords *coords,
                            GError          **error)
{
  return TRUE;
}

static gboolean
gimp_paint_core_real_pre_paint (GimpPaintCore    *core,
                                GimpDrawable     *drawable,
                                GimpPaintOptions *paint_options,
                                GimpPaintState    paint_state,
                                guint32           time)
{
  return TRUE;
}

static void
gimp_paint_core_real_paint (GimpPaintCore    *core,
                            GimpDrawable     *drawable,
                            GimpPaintOptions *paint_options,
                            const GimpCoords *coords,
                            GimpPaintState    paint_state,
                            guint32           time)
{
}

static void
gimp_paint_core_real_post_paint (GimpPaintCore    *core,
                                 GimpDrawable     *drawable,
                                 GimpPaintOptions *paint_options,
                                 GimpPaintState    paint_state,
                                 guint32           time)
{
}

static void
gimp_paint_core_real_interpolate (GimpPaintCore    *core,
                                  GimpDrawable     *drawable,
                                  GimpPaintOptions *paint_options,
                                  guint32           time)
{
  gimp_paint_core_paint (core, drawable, paint_options,
                         GIMP_PAINT_STATE_MOTION, time);

  core->last_coords = core->cur_coords;
}

static TempBuf *
gimp_paint_core_real_get_paint_area (GimpPaintCore    *core,
                                     GimpDrawable     *drawable,
                                     GimpPaintOptions *paint_options,
                                     const GimpCoords *coords)
{
  return NULL;
}

static GimpUndo *
gimp_paint_core_real_push_undo (GimpPaintCore *core,
                                GimpImage     *image,
                                const gchar   *undo_desc)
{
  return gimp_image_undo_push (image, GIMP_TYPE_PAINT_CORE_UNDO,
                               GIMP_UNDO_PAINT, undo_desc,
                               0,
                               "paint-core", core,
                               NULL);
}


/*  public functions  */

void
gimp_paint_core_paint (GimpPaintCore    *core,
                       GimpDrawable     *drawable,
                       GimpPaintOptions *paint_options,
                       GimpPaintState    paint_state,
                       guint32           time)
{
  GimpPaintCoreClass *core_class;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options));

  core_class = GIMP_PAINT_CORE_GET_CLASS (core);

  if (core_class->pre_paint (core, drawable,
                             paint_options,
                             paint_state, time))
    {

      if (paint_state == GIMP_PAINT_STATE_MOTION)
        {
          /* Save coordinates for gimp_paint_core_interpolate() */
          core->last_paint.x = core->cur_coords.x;
          core->last_paint.y = core->cur_coords.y;
        }

      core_class->paint (core, drawable,
                         paint_options,
                         &core->cur_coords,
                         paint_state, time);

      core_class->post_paint (core, drawable,
                              paint_options,
                              paint_state, time);
    }
}

gboolean
gimp_paint_core_start (GimpPaintCore     *core,
                       GimpDrawable      *drawable,
                       GimpPaintOptions  *paint_options,
                       const GimpCoords  *coords,
                       GError           **error)
{
  GimpItem *item;

  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item = GIMP_ITEM (drawable);

  if (core->stroke_buffer)
    {
      g_array_free (core->stroke_buffer, TRUE);
      core->stroke_buffer = NULL;
    }

  core->stroke_buffer = g_array_sized_new (TRUE, TRUE,
                                           sizeof (GimpCoords),
                                           STROKE_BUFFER_INIT_SIZE);

  core->cur_coords = *coords;

  if (! GIMP_PAINT_CORE_GET_CLASS (core)->start (core, drawable,
                                                 paint_options,
                                                 coords, error))
    {
      return FALSE;
    }

  /*  Allocate the undo structure  */
  if (core->undo_buffer)
    g_object_unref (core->undo_buffer);

  core->undo_buffer = gimp_gegl_buffer_dup (gimp_drawable_get_buffer (drawable));

  /*  Allocate the saved proj structure  */
  if (core->saved_proj_buffer)
    {
      g_object_unref (core->saved_proj_buffer);
      core->saved_proj_buffer = NULL;
    }

  if (core->use_saved_proj)
    {
      GimpImage    *image    = gimp_item_get_image (item);
      GimpPickable *pickable = GIMP_PICKABLE (gimp_image_get_projection (image));
      GeglBuffer   *buffer   = gimp_pickable_get_buffer (pickable);

      core->saved_proj_buffer = gimp_gegl_buffer_dup (buffer);
    }

  /*  Allocate the canvas blocks structure  */
  if (core->canvas_buffer)
    g_object_unref (core->canvas_buffer);

  core->canvas_buffer =
    gimp_gegl_buffer_new (GIMP_GEGL_RECT (0, 0,
                                          gimp_item_get_width  (item),
                                          gimp_item_get_height (item)),
                          babl_format ("Y u8"));
  gegl_buffer_clear (core->canvas_buffer, NULL);

  /*  Get the initial undo extents  */

  core->x1 = core->x2 = core->cur_coords.x;
  core->y1 = core->y2 = core->cur_coords.y;

  core->last_paint.x = -1e6;
  core->last_paint.y = -1e6;

  /*  Freeze the drawable preview so that it isn't constantly updated.  */
  gimp_viewable_preview_freeze (GIMP_VIEWABLE (drawable));

  return TRUE;
}

void
gimp_paint_core_finish (GimpPaintCore *core,
                        GimpDrawable  *drawable,
                        gboolean       push_undo)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  if (core->stroke_buffer)
    {
      g_array_free (core->stroke_buffer, TRUE);
      core->stroke_buffer = NULL;
    }

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if ((core->x2 == core->x1) || (core->y2 == core->y1))
    {
      gimp_viewable_preview_thaw (GIMP_VIEWABLE (drawable));
      return;
    }

  if (push_undo)
    {
      GeglBuffer *buffer;
      gint        x, y, width, height;

      gimp_rectangle_intersect (core->x1, core->y1,
                                core->x2 - core->x1, core->y2 - core->y1,
                                0, 0,
                                gimp_item_get_width  (GIMP_ITEM (drawable)),
                                gimp_item_get_height (GIMP_ITEM (drawable)),
                                &x, &y, &width, &height);

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_PAINT,
                                   core->undo_desc);

      GIMP_PAINT_CORE_GET_CLASS (core)->push_undo (core, image, NULL);

      buffer = gimp_gegl_buffer_new (GIMP_GEGL_RECT (0, 0, width, height),
                                     gimp_drawable_get_format (drawable));

      gegl_buffer_copy (core->undo_buffer,
                        GIMP_GEGL_RECT (x, y, width, height),
                        buffer,
                        GIMP_GEGL_RECT (0, 0, 0, 0));

      gimp_drawable_push_undo (drawable, NULL,
                               buffer, x, y, width, height);

      g_object_unref (buffer);

      gimp_image_undo_group_end (image);
    }

  g_object_unref (core->undo_buffer);
  core->undo_buffer = NULL;

  if (core->saved_proj_buffer)
    {
      g_object_unref (core->saved_proj_buffer);
      core->saved_proj_buffer = NULL;
    }

  gimp_viewable_preview_thaw (GIMP_VIEWABLE (drawable));
}

void
gimp_paint_core_cancel (GimpPaintCore *core,
                        GimpDrawable  *drawable)
{
  gint x, y;
  gint width, height;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if ((core->x2 == core->x1) || (core->y2 == core->y1))
    return;

  if (gimp_rectangle_intersect (core->x1, core->y1,
                                core->x2 - core->x1,
                                core->y2 - core->y1,
                                0, 0,
                                gimp_item_get_width  (GIMP_ITEM (drawable)),
                                gimp_item_get_height (GIMP_ITEM (drawable)),
                                &x, &y, &width, &height))
    {
      gegl_buffer_copy (core->undo_buffer,
                        GIMP_GEGL_RECT (x, y, width, height),
                        gimp_drawable_get_buffer (drawable),
                        GIMP_GEGL_RECT (x, y, width, height));
    }

  g_object_unref (core->undo_buffer);
  core->undo_buffer = NULL;

  if (core->saved_proj_buffer)
    {
      g_object_unref (core->saved_proj_buffer);
      core->saved_proj_buffer = NULL;
    }

  gimp_drawable_update (drawable, x, y, width, height);

  gimp_viewable_preview_thaw (GIMP_VIEWABLE (drawable));
}

void
gimp_paint_core_cleanup (GimpPaintCore *core)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));

  if (core->undo_buffer)
    {
      g_object_unref (core->undo_buffer);
      core->undo_buffer = NULL;
    }

  if (core->saved_proj_buffer)
    {
      g_object_unref (core->saved_proj_buffer);
      core->saved_proj_buffer = NULL;
    }

  if (core->canvas_buffer)
    {
      g_object_unref (core->canvas_buffer);
      core->canvas_buffer = NULL;
    }

  if (core->paint_area)
    {
      temp_buf_free (core->paint_area);
      core->paint_area = NULL;
    }

  if (core->paint_buffer)
    {
      g_object_unref (core->paint_buffer);
      core->paint_buffer = NULL;
    }
}

void
gimp_paint_core_interpolate (GimpPaintCore    *core,
                             GimpDrawable     *drawable,
                             GimpPaintOptions *paint_options,
                             const GimpCoords *coords,
                             guint32           time)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options));
  g_return_if_fail (coords != NULL);

  core->cur_coords = *coords;

  GIMP_PAINT_CORE_GET_CLASS (core)->interpolate (core, drawable,
                                                 paint_options, time);
}

void
gimp_paint_core_set_current_coords (GimpPaintCore    *core,
                                    const GimpCoords *coords)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  core->cur_coords = *coords;
}

void
gimp_paint_core_get_current_coords (GimpPaintCore    *core,
                                    GimpCoords       *coords)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  *coords = core->cur_coords;

}

void
gimp_paint_core_set_last_coords (GimpPaintCore    *core,
                                 const GimpCoords *coords)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  core->last_coords = *coords;
}

void
gimp_paint_core_get_last_coords (GimpPaintCore *core,
                                 GimpCoords    *coords)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  *coords = core->last_coords;
}

/**
 * gimp_paint_core_round_line:
 * @core:                 the #GimpPaintCore
 * @options:              the #GimpPaintOptions to use
 * @constrain_15_degrees: the modifier state
 *
 * Adjusts core->last_coords and core_cur_coords in preparation to
 * drawing a straight line. If @center_pixels is TRUE the endpoints
 * get pushed to the center of the pixels. This avoids artefacts
 * for e.g. the hard mode. The rounding of the slope to 15 degree
 * steps if ctrl is pressed happens, as does rounding the start and
 * end coordinates (which may be fractional in high zoom modes) to
 * the center of pixels.
 **/
void
gimp_paint_core_round_line (GimpPaintCore    *core,
                            GimpPaintOptions *paint_options,
                            gboolean          constrain_15_degrees)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options));

  if (gimp_paint_options_get_brush_mode (paint_options) == GIMP_BRUSH_HARD)
    {
      core->last_coords.x = floor (core->last_coords.x) + 0.5;
      core->last_coords.y = floor (core->last_coords.y) + 0.5;
      core->cur_coords.x  = floor (core->cur_coords.x ) + 0.5;
      core->cur_coords.y  = floor (core->cur_coords.y ) + 0.5;
    }

  if (constrain_15_degrees)
    gimp_constrain_line (core->last_coords.x, core->last_coords.y,
                         &core->cur_coords.x, &core->cur_coords.y,
                         GIMP_CONSTRAIN_LINE_15_DEGREES);
}


/*  protected functions  */

TempBuf *
gimp_paint_core_get_paint_area (GimpPaintCore    *core,
                                GimpDrawable     *drawable,
                                GimpPaintOptions *paint_options,
                                const GimpCoords *coords)
{
  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), NULL);
  g_return_val_if_fail (coords != NULL, NULL);

  return GIMP_PAINT_CORE_GET_CLASS (core)->get_paint_area (core, drawable,
                                                           paint_options,
                                                           coords);
}

GeglBuffer *
gimp_paint_core_get_paint_buffer (GimpPaintCore    *core,
                                  GimpDrawable     *drawable,
                                  GimpPaintOptions *paint_options,
                                  const GimpCoords *coords,
                                  gint             *paint_buffer_x,
                                  gint             *paint_buffer_y)
{
  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), NULL);
  g_return_val_if_fail (coords != NULL, NULL);
  g_return_val_if_fail (paint_buffer_x != NULL, NULL);
  g_return_val_if_fail (paint_buffer_y != NULL, NULL);

  gimp_paint_core_get_paint_area (core, drawable, paint_options, coords);

  *paint_buffer_x = core->paint_buffer_x;
  *paint_buffer_y = core->paint_buffer_y;

  return core->paint_buffer;
}

GeglBuffer *
gimp_paint_core_get_orig_image (GimpPaintCore *core)
{
  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (core->undo_buffer != NULL, NULL);

  return core->undo_buffer;
}

GeglBuffer *
gimp_paint_core_get_orig_proj (GimpPaintCore *core)
{
  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (core->saved_proj_buffer != NULL, NULL);

  return core->saved_proj_buffer;
}

void
gimp_paint_core_paste (GimpPaintCore            *core,
                       PixelRegion              *paint_maskPR,
                       GimpDrawable             *drawable,
                       gdouble                   paint_opacity,
                       gdouble                   image_opacity,
                       GimpLayerModeEffects      paint_mode,
                       GimpPaintApplicationMode  mode)
{
  GeglBuffer *base_buffer = NULL;
  gint        width, height;

  /*  If the mode is CONSTANT:
   *   combine the canvas buf, the paint mask to the canvas buffer
   */
  if (mode == GIMP_PAINT_CONSTANT)
    {
      /* Some tools (ink) paint the mask to paint_core->canvas_buffer
       * directly. Don't need to copy it in this case.
       */
      if (paint_maskPR->tiles !=
          gimp_gegl_buffer_get_tiles (core->canvas_buffer))
        {
          paint_mask_to_canvas_buffer (core, paint_maskPR, paint_opacity);
        }

      canvas_buffer_to_paint_area (core);

      base_buffer = core->undo_buffer;
    }
  /*  Otherwise:
   *   combine the canvas buf and the paint mask to the canvas buf
   */
  else
    {
      paint_mask_to_paint_area (core, paint_maskPR, paint_opacity);
    }

  width  = gegl_buffer_get_width  (core->paint_buffer);
  height = gegl_buffer_get_height (core->paint_buffer);

  /*  apply the paint area to the image  */
  gimp_drawable_apply_buffer (drawable,
                              core->paint_buffer,
                              GIMP_GEGL_RECT (0, 0, width, height),
                              FALSE, NULL,
                              image_opacity, paint_mode,
                              base_buffer, /*  specify an alternative src1  */
                              core->paint_buffer_x,
                              core->paint_buffer_y,
                              NULL,
                              core->paint_buffer_x,
                              core->paint_buffer_y);

  /*  Update the undo extents  */
  core->x1 = MIN (core->x1, core->paint_buffer_x);
  core->y1 = MIN (core->y1, core->paint_buffer_y);
  core->x2 = MAX (core->x2, core->paint_buffer_x + width);
  core->y2 = MAX (core->y2, core->paint_buffer_y + height);

  /*  Update the drawable  */
  gimp_drawable_update (drawable,
                        core->paint_buffer_x,
                        core->paint_buffer_y,
                        width, height);
}

/* This works similarly to gimp_paint_core_paste. However, instead of
 * combining the canvas to the paint core drawable using one of the
 * combination modes, it uses a "replace" mode (i.e. transparent
 * pixels in the canvas erase the paint core drawable).

 * When not drawing on alpha-enabled images, it just paints using
 * NORMAL mode.
 */
void
gimp_paint_core_replace (GimpPaintCore            *core,
                         PixelRegion              *paint_maskPR,
                         GimpDrawable             *drawable,
                         gdouble                   paint_opacity,
                         gdouble                   image_opacity,
                         GimpPaintApplicationMode  mode)
{
  gint width, height;

  if (! gimp_drawable_has_alpha (drawable))
    {
      gimp_paint_core_paste (core, paint_maskPR, drawable,
                             paint_opacity,
                             image_opacity, GIMP_NORMAL_MODE,
                             mode);
      return;
    }

  width  = gegl_buffer_get_width  (core->paint_buffer);
  height = gegl_buffer_get_height (core->paint_buffer);

  if (mode == GIMP_PAINT_CONSTANT)
    {
      /* Some tools (ink) paint the mask to paint_core->canvas_buffer
       * directly. Don't need to copy it in this case.
       */
      if (paint_maskPR->tiles !=
          gimp_gegl_buffer_get_tiles (core->canvas_buffer))
        {
          /* combine the paint mask and the canvas buffer */
          paint_mask_to_canvas_buffer (core, paint_maskPR, paint_opacity);

          /* initialize the maskPR from the canvas buffer */
          pixel_region_init (paint_maskPR,
                             gimp_gegl_buffer_get_tiles (core->canvas_buffer),
                             core->paint_buffer_x,
                             core->paint_buffer_y,
                             width, height,
                             FALSE);
        }
    }
  else
    {
      /* The mask is just the paint_maskPR */
    }

  /*  apply the paint area to the image  */
  gimp_drawable_replace_buffer (drawable, core->paint_buffer,
                                GIMP_GEGL_RECT (0, 0, width, height),
                                FALSE, NULL,
                                image_opacity,
                                paint_maskPR,
                                core->paint_buffer_x,
                                core->paint_buffer_y);

  /*  Update the undo extents  */
  core->x1 = MIN (core->x1, core->paint_buffer_x);
  core->y1 = MIN (core->y1, core->paint_buffer_y);
  core->x2 = MAX (core->x2, core->paint_buffer_x + width);
  core->y2 = MAX (core->y2, core->paint_buffer_y + height);

  /*  Update the drawable  */
  gimp_drawable_update (drawable,
                        core->paint_buffer_x,
                        core->paint_buffer_y,
                        width, height);
}

/**
 * Smooth and store coords in the stroke buffer
 */

void
gimp_paint_core_smooth_coords (GimpPaintCore    *core,
                               GimpPaintOptions *paint_options,
                               GimpCoords       *coords)
{
  GimpSmoothingOptions *smoothing_options = paint_options->smoothing_options;
  GArray               *history           = core->stroke_buffer;

  if (core->stroke_buffer == NULL)
    return; /* Paint core has not initalized yet */

  if (smoothing_options->use_smoothing &&
      smoothing_options->smoothing_quality > 0)
    {
      gint       i;
      guint      length;
      gint       min_index;
      gdouble    gaussian_weight  = 0.0;
      gdouble    gaussian_weight2 = SQR (smoothing_options->smoothing_factor);
      gdouble    velocity_sum     = 0.0;
      gdouble    scale_sum        = 0.0;

      g_array_append_val (history, *coords);

      if (history->len < 2)
        return; /* Just dont bother, nothing to do */

      coords->x = coords->y = 0.0;

      length = MIN (smoothing_options->smoothing_quality, history->len);

      min_index = history->len - length;

      if (gaussian_weight2 != 0.0)
        gaussian_weight = 1 / (sqrt (2 * G_PI) * smoothing_options->smoothing_factor);

      for (i = history->len - 1; i >= min_index; i--)
        {
          gdouble     rate        = 0.0;
          GimpCoords *next_coords = &g_array_index (history,
                                                    GimpCoords, i);

          if (gaussian_weight2 != 0.0)
            {
              /* We use gaussian function with velocity as a window function */
              velocity_sum += next_coords->velocity * 100;
              rate = gaussian_weight * exp (-velocity_sum*velocity_sum / (2 * gaussian_weight2));
            }

          scale_sum += rate;
          coords->x += rate * next_coords->x;
          coords->y += rate * next_coords->y;
        }

      if (scale_sum != 0.0)
        {
          coords->x /= scale_sum;
          coords->y /= scale_sum;
        }

    }

}


static void
canvas_buffer_to_paint_area (GimpPaintCore *core)
{
  PixelRegion srcPR;
  PixelRegion maskPR;

  /*  combine the canvas buffer and the paint area  */
  pixel_region_init_temp_buf (&srcPR, core->paint_area,
                              0, 0,
                              core->paint_area->width,
                              core->paint_area->height);

  pixel_region_init (&maskPR,
                     gimp_gegl_buffer_get_tiles (core->canvas_buffer),
                     core->paint_area->x,
                     core->paint_area->y,
                     core->paint_area->width,
                     core->paint_area->height,
                     FALSE);

  /*  apply the canvas buffer to the paint area  */
  apply_mask_to_region (&srcPR, &maskPR, OPAQUE_OPACITY);
}

static void
paint_mask_to_canvas_buffer (GimpPaintCore *core,
                             PixelRegion   *paint_maskPR,
                             gdouble        paint_opacity)
{
  PixelRegion srcPR;

  /*   combine the paint mask and the canvas buffer  */
  pixel_region_init (&srcPR,
                     gimp_gegl_buffer_get_tiles (core->canvas_buffer),
                     core->paint_area->x,
                     core->paint_area->y,
                     core->paint_area->width,
                     core->paint_area->height,
                     TRUE);

  /*  combine the mask to the canvas tiles  */
  combine_mask_and_region (&srcPR, paint_maskPR,
                           paint_opacity * 255.999, GIMP_IS_AIRBRUSH (core));
}

static void
paint_mask_to_paint_area (GimpPaintCore *core,
                          PixelRegion   *paint_maskPR,
                          gdouble        paint_opacity)
{
  PixelRegion srcPR;

  /*  combine the canvas buf and the paint mask to the canvas buf  */
  pixel_region_init_temp_buf (&srcPR, core->paint_area,
                              0, 0,
                              core->paint_area->width,
                              core->paint_area->height);

  /*  apply the mask  */
  apply_mask_to_region (&srcPR, paint_maskPR, paint_opacity * 255.999);
}
