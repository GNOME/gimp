/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 2013 Daniel Sabo
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-nodes.h"
#include "gegl/gimp-gegl-utils.h"
#include "gegl/gimpapplicator.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-symmetry.h"
#include "core/gimpimage-undo.h"
#include "core/gimppickable.h"
#include "core/gimpprojection.h"
#include "core/gimpsymmetry.h"
#include "core/gimptempbuf.h"

#include "gimppaintcore.h"
#include "gimppaintcoreundo.h"
#include "gimppaintcore-loops.h"
#include "gimppaintoptions.h"

#include "gimpairbrush.h"

#include "gimp-intl.h"


#define STROKE_BUFFER_INIT_SIZE 2000

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
                                                      GimpSymmetry     *sym,
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
static GeglBuffer *
               gimp_paint_core_real_get_paint_buffer (GimpPaintCore    *core,
                                                      GimpDrawable     *drawable,
                                                      GimpPaintOptions *options,
                                                      GimpLayerMode     paint_mode,
                                                      const GimpCoords *coords,
                                                      gint             *paint_buffer_x,
                                                      gint             *paint_buffer_y,
                                                      gint             *paint_width,
                                                      gint             *paint_height);
static GimpUndo* gimp_paint_core_real_push_undo      (GimpPaintCore    *core,
                                                      GimpImage        *image,
                                                      const gchar      *undo_desc);


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
  klass->get_paint_buffer    = gimp_paint_core_real_get_paint_buffer;
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

  g_clear_pointer (&core->undo_desc, g_free);

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
                            GimpSymmetry     *sym,
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

static GeglBuffer *
gimp_paint_core_real_get_paint_buffer (GimpPaintCore    *core,
                                       GimpDrawable     *drawable,
                                       GimpPaintOptions *paint_options,
                                       GimpLayerMode     paint_mode,
                                       const GimpCoords *coords,
                                       gint             *paint_buffer_x,
                                       gint             *paint_buffer_y,
                                       gint             *paint_width,
                                       gint             *paint_height)
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
      GimpSymmetry *sym;
      GimpImage    *image;
      GimpItem     *item;

      item  = GIMP_ITEM (drawable);
      image = gimp_item_get_image (item);

      if (paint_state == GIMP_PAINT_STATE_MOTION)
        {
          /* Save coordinates for gimp_paint_core_interpolate() */
          core->last_paint.x = core->cur_coords.x;
          core->last_paint.y = core->cur_coords.y;
        }

      sym = g_object_ref (gimp_image_get_active_symmetry (image));
      gimp_symmetry_set_origin (sym, drawable, &core->cur_coords);

      core_class->paint (core, drawable,
                         paint_options,
                         sym, paint_state, time);

      gimp_symmetry_clear_origin (sym);
      g_object_unref (sym);

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
  GimpImage   *image;
  GimpItem    *item;
  GimpChannel *mask;

  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item  = GIMP_ITEM (drawable);
  image = gimp_item_get_image (item);

  if (core->stroke_buffer)
    {
      g_array_free (core->stroke_buffer, TRUE);
      core->stroke_buffer = NULL;
    }

  core->stroke_buffer = g_array_sized_new (TRUE, TRUE,
                                           sizeof (GimpCoords),
                                           STROKE_BUFFER_INIT_SIZE);

  /* remember the last stroke's endpoint for later undo */
  core->start_coords = core->last_coords;

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

  /*  Set the image pickable  */
  if (! core->show_all)
    core->image_pickable = GIMP_PICKABLE (image);
  else
    core->image_pickable = GIMP_PICKABLE (gimp_image_get_projection (image));

  /*  Allocate the saved proj structure  */
  g_clear_object (&core->saved_proj_buffer);

  if (core->use_saved_proj)
    {
      GeglBuffer *buffer = gimp_pickable_get_buffer (core->image_pickable);

      core->saved_proj_buffer = gimp_gegl_buffer_dup (buffer);
    }

  /*  Allocate the canvas blocks structure  */
  if (core->canvas_buffer)
    g_object_unref (core->canvas_buffer);

  core->canvas_buffer =
    gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                     gimp_item_get_width  (item),
                                     gimp_item_get_height (item)),
                     babl_format ("Y float"));

  /*  Get the initial undo extents  */

  core->x1 = core->x2 = core->cur_coords.x;
  core->y1 = core->y2 = core->cur_coords.y;

  core->last_paint.x = -1e6;
  core->last_paint.y = -1e6;

  mask = gimp_image_get_mask (image);

  /*  don't apply the mask to itself and don't apply an empty mask  */
  if (GIMP_DRAWABLE (mask) != drawable && ! gimp_channel_is_empty (mask))
    {
      GeglBuffer *mask_buffer;
      gint        offset_x;
      gint        offset_y;

      mask_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));
      gimp_item_get_offset (item, &offset_x, &offset_y);

      core->mask_buffer   = g_object_ref (mask_buffer);
      core->mask_x_offset = -offset_x;
      core->mask_y_offset = -offset_y;
    }
  else
    {
      core->mask_buffer = NULL;
    }

  if (paint_options->use_applicator)
    {
      core->applicator = gimp_applicator_new (NULL);

      if (core->mask_buffer)
        {
          gimp_applicator_set_mask_buffer (core->applicator,
                                           core->mask_buffer);
          gimp_applicator_set_mask_offset (core->applicator,
                                           core->mask_x_offset,
                                           core->mask_y_offset);
        }

      gimp_applicator_set_affect (core->applicator,
                                  gimp_drawable_get_active_mask (drawable));
      gimp_applicator_set_dest_buffer (core->applicator,
                                       gimp_drawable_get_buffer (drawable));
    }

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

  g_clear_object (&core->applicator);

  if (core->stroke_buffer)
    {
      g_array_free (core->stroke_buffer, TRUE);
      core->stroke_buffer = NULL;
    }

  g_clear_object (&core->mask_buffer);

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
      GeglBuffer    *buffer;
      GeglRectangle  rect;

      gimp_rectangle_intersect (core->x1, core->y1,
                                core->x2 - core->x1, core->y2 - core->y1,
                                0, 0,
                                gimp_item_get_width  (GIMP_ITEM (drawable)),
                                gimp_item_get_height (GIMP_ITEM (drawable)),
                                &rect.x, &rect.y, &rect.width, &rect.height);

      gegl_rectangle_align_to_buffer (&rect, &rect, core->undo_buffer,
                                      GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_PAINT,
                                   core->undo_desc);

      GIMP_PAINT_CORE_GET_CLASS (core)->push_undo (core, image, NULL);

      buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, rect.width, rect.height),
                                gimp_drawable_get_format (drawable));

      gimp_gegl_buffer_copy (core->undo_buffer,
                             &rect,
                             GEGL_ABYSS_NONE,
                             buffer,
                             GEGL_RECTANGLE (0, 0, 0, 0));

      gimp_drawable_push_undo (drawable, NULL,
                               buffer, rect.x, rect.y, rect.width, rect.height);

      g_object_unref (buffer);

      gimp_image_undo_group_end (image);
    }

  core->image_pickable = NULL;

  g_clear_object (&core->undo_buffer);
  g_clear_object (&core->saved_proj_buffer);

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
      GeglRectangle rect;

      gegl_rectangle_align_to_buffer (&rect,
                                      GEGL_RECTANGLE (x, y, width, height),
                                      gimp_drawable_get_buffer (drawable),
                                      GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      gimp_gegl_buffer_copy (core->undo_buffer,
                             &rect,
                             GEGL_ABYSS_NONE,
                             gimp_drawable_get_buffer (drawable),
                             &rect);
    }

  g_clear_object (&core->undo_buffer);
  g_clear_object (&core->saved_proj_buffer);

  gimp_drawable_update (drawable, x, y, width, height);

  gimp_viewable_preview_thaw (GIMP_VIEWABLE (drawable));
}

void
gimp_paint_core_cleanup (GimpPaintCore *core)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));

  g_clear_object (&core->undo_buffer);
  g_clear_object (&core->saved_proj_buffer);
  g_clear_object (&core->canvas_buffer);
  g_clear_object (&core->paint_buffer);
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
gimp_paint_core_set_show_all (GimpPaintCore *core,
                              gboolean       show_all)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));

  core->show_all = show_all;
}

gboolean
gimp_paint_core_get_show_all (GimpPaintCore *core)
{
  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), FALSE);

  return core->show_all;
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
 * @core:                   the #GimpPaintCore
 * @options:                the #GimpPaintOptions to use
 * @constrain_15_degrees:   the modifier state
 * @constrain_offset_angle: the angle by which to offset the lines, in degrees
 * @constrain_xres:         the horizontal resolution
 * @constrain_yres:         the vertical resolution
 *
 * Adjusts core->last_coords and core_cur_coords in preparation to
 * drawing a straight line. If @center_pixels is TRUE the endpoints
 * get pushed to the center of the pixels. This avoids artifacts
 * for e.g. the hard mode. The rounding of the slope to 15 degree
 * steps if ctrl is pressed happens, as does rounding the start and
 * end coordinates (which may be fractional in high zoom modes) to
 * the center of pixels.
 **/
void
gimp_paint_core_round_line (GimpPaintCore    *core,
                            GimpPaintOptions *paint_options,
                            gboolean          constrain_15_degrees,
                            gdouble           constrain_offset_angle,
                            gdouble           constrain_xres,
                            gdouble           constrain_yres)
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
                         GIMP_CONSTRAIN_LINE_15_DEGREES,
                         constrain_offset_angle,
                         constrain_xres, constrain_yres);
}


/*  protected functions  */

GeglBuffer *
gimp_paint_core_get_paint_buffer (GimpPaintCore    *core,
                                  GimpDrawable     *drawable,
                                  GimpPaintOptions *paint_options,
                                  GimpLayerMode     paint_mode,
                                  const GimpCoords *coords,
                                  gint             *paint_buffer_x,
                                  gint             *paint_buffer_y,
                                  gint             *paint_width,
                                  gint             *paint_height)
{
  GeglBuffer *paint_buffer;

  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), NULL);
  g_return_val_if_fail (coords != NULL, NULL);
  g_return_val_if_fail (paint_buffer_x != NULL, NULL);
  g_return_val_if_fail (paint_buffer_y != NULL, NULL);

  paint_buffer =
    GIMP_PAINT_CORE_GET_CLASS (core)->get_paint_buffer (core, drawable,
                                                        paint_options,
                                                        paint_mode,
                                                        coords,
                                                        paint_buffer_x,
                                                        paint_buffer_y,
                                                        paint_width,
                                                        paint_height);

  core->paint_buffer_x = *paint_buffer_x;
  core->paint_buffer_y = *paint_buffer_y;

  return paint_buffer;
}

GimpPickable *
gimp_paint_core_get_image_pickable (GimpPaintCore *core)
{
  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (core->image_pickable != NULL, NULL);

  return core->image_pickable;
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
                       const GimpTempBuf        *paint_mask,
                       gint                      paint_mask_offset_x,
                       gint                      paint_mask_offset_y,
                       GimpDrawable             *drawable,
                       gdouble                   paint_opacity,
                       gdouble                   image_opacity,
                       GimpLayerMode             paint_mode,
                       GimpPaintApplicationMode  mode)
{
  gint              width  = gegl_buffer_get_width  (core->paint_buffer);
  gint              height = gegl_buffer_get_height (core->paint_buffer);
  GimpComponentMask affect = gimp_drawable_get_active_mask (drawable);

  if (! affect)
    return;

  if (core->applicator)
    {
      /*  If the mode is CONSTANT:
       *   combine the canvas buffer and the paint mask to the paint buffer
       */
      if (mode == GIMP_PAINT_CONSTANT)
        {
          /* Some tools (ink) paint the mask to paint_core->canvas_buffer
           * directly. Don't need to copy it in this case.
           */
          if (paint_mask != NULL)
            {
              GeglBuffer *paint_mask_buffer =
                gimp_temp_buf_create_buffer ((GimpTempBuf *) paint_mask);

              gimp_gegl_combine_mask_weird (paint_mask_buffer,
                                            GEGL_RECTANGLE (paint_mask_offset_x,
                                                            paint_mask_offset_y,
                                                            width, height),
                                            core->canvas_buffer,
                                            GEGL_RECTANGLE (core->paint_buffer_x,
                                                            core->paint_buffer_y,
                                                            width, height),
                                            paint_opacity,
                                            GIMP_IS_AIRBRUSH (core));

              g_object_unref (paint_mask_buffer);
            }

          gimp_gegl_apply_mask (core->canvas_buffer,
                                GEGL_RECTANGLE (core->paint_buffer_x,
                                                core->paint_buffer_y,
                                                width, height),
                                core->paint_buffer,
                                GEGL_RECTANGLE (0, 0, width, height),
                                1.0);

          gimp_applicator_set_src_buffer (core->applicator,
                                          core->undo_buffer);
        }
      /*  Otherwise:
       *   combine the paint mask to the paint buffer directly
       */
      else
        {
          GeglBuffer *paint_mask_buffer =
            gimp_temp_buf_create_buffer ((GimpTempBuf *) paint_mask);

          gimp_gegl_apply_mask (paint_mask_buffer,
                                GEGL_RECTANGLE (paint_mask_offset_x,
                                                paint_mask_offset_y,
                                                width, height),
                                core->paint_buffer,
                                GEGL_RECTANGLE (0, 0, width, height),
                                paint_opacity);

          g_object_unref (paint_mask_buffer);

          gimp_applicator_set_src_buffer (core->applicator,
                                          gimp_drawable_get_buffer (drawable));
        }

      gimp_applicator_set_apply_buffer (core->applicator,
                                        core->paint_buffer);
      gimp_applicator_set_apply_offset (core->applicator,
                                        core->paint_buffer_x,
                                        core->paint_buffer_y);

      gimp_applicator_set_opacity (core->applicator, image_opacity);
      gimp_applicator_set_mode (core->applicator, paint_mode,
                                GIMP_LAYER_COLOR_SPACE_AUTO,
                                GIMP_LAYER_COLOR_SPACE_AUTO,
                                gimp_layer_mode_get_paint_composite_mode (paint_mode));

      /*  apply the paint area to the image  */
      gimp_applicator_blit (core->applicator,
                            GEGL_RECTANGLE (core->paint_buffer_x,
                                            core->paint_buffer_y,
                                            width, height));
    }
  else
    {
      GimpPaintCoreLoopsParams    params = {};
      GimpPaintCoreLoopsAlgorithm algorithms = GIMP_PAINT_CORE_LOOPS_ALGORITHM_NONE;

      params.paint_buf          = gimp_gegl_buffer_get_temp_buf (core->paint_buffer);
      params.paint_buf_offset_x = core->paint_buffer_x;
      params.paint_buf_offset_y = core->paint_buffer_y;

      if (! params.paint_buf)
        return;

      params.dest_buffer = gimp_drawable_get_buffer (drawable);

      if (mode == GIMP_PAINT_CONSTANT)
        {
          params.canvas_buffer = core->canvas_buffer;

          /* This step is skipped by the ink tool, which writes
           * directly to canvas_buffer
           */
          if (paint_mask != NULL)
            {
              /* Mix paint mask and canvas_buffer */
              params.paint_mask          = paint_mask;
              params.paint_mask_offset_x = paint_mask_offset_x;
              params.paint_mask_offset_y = paint_mask_offset_y;
              params.stipple             = GIMP_IS_AIRBRUSH (core);
              params.paint_opacity       = paint_opacity;

              algorithms |= GIMP_PAINT_CORE_LOOPS_ALGORITHM_COMBINE_PAINT_MASK_TO_CANVAS_BUFFER;
            }

          /* Write canvas_buffer to paint_buf */
          algorithms |= GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_COMP_MASK;

          /* undo buf -> paint_buf -> dest_buffer */
          params.src_buffer = core->undo_buffer;
        }
      else
        {
          g_return_if_fail (paint_mask);

          /* Write paint_mask to paint_buf, does not modify canvas_buffer */
          params.paint_mask          = paint_mask;
          params.paint_mask_offset_x = paint_mask_offset_x;
          params.paint_mask_offset_y = paint_mask_offset_y;
          params.paint_opacity       = paint_opacity;

          algorithms |= GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK;

          /* dest_buffer -> paint_buf -> dest_buffer */
          params.src_buffer = params.dest_buffer;
        }

      params.mask_buffer   = core->mask_buffer;
      params.mask_offset_x = core->mask_x_offset;
      params.mask_offset_y = core->mask_y_offset;
      params.image_opacity = image_opacity;
      params.paint_mode    = paint_mode;

      algorithms |= GIMP_PAINT_CORE_LOOPS_ALGORITHM_DO_LAYER_BLEND;

      if (affect != GIMP_COMPONENT_MASK_ALL)
        {
          params.affect = affect;

          algorithms |= GIMP_PAINT_CORE_LOOPS_ALGORITHM_MASK_COMPONENTS;
        }

      gimp_paint_core_loops_process (&params, algorithms);
    }

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
                         const GimpTempBuf        *paint_mask,
                         gint                      paint_mask_offset_x,
                         gint                      paint_mask_offset_y,
                         GimpDrawable             *drawable,
                         gdouble                   paint_opacity,
                         gdouble                   image_opacity,
                         GimpPaintApplicationMode  mode)
{
  gint              width, height;
  GimpComponentMask affect;

  if (! gimp_drawable_has_alpha (drawable))
    {
      gimp_paint_core_paste (core, paint_mask,
                             paint_mask_offset_x,
                             paint_mask_offset_y,
                             drawable,
                             paint_opacity,
                             image_opacity,
                             GIMP_LAYER_MODE_NORMAL,
                             mode);
      return;
    }

  width  = gegl_buffer_get_width  (core->paint_buffer);
  height = gegl_buffer_get_height (core->paint_buffer);

  affect = gimp_drawable_get_active_mask (drawable);

  if (! affect)
    return;

  if (core->applicator)
    {
      GeglRectangle  mask_rect;
      GeglBuffer    *mask_buffer;

      /*  If the mode is CONSTANT:
       *   combine the paint mask to the canvas buffer, and use it as the mask
       *   buffer
       */
      if (mode == GIMP_PAINT_CONSTANT)
        {
          /* Some tools (ink) paint the mask to paint_core->canvas_buffer
           * directly. Don't need to copy it in this case.
           */
          if (paint_mask != NULL)
            {
              GeglBuffer *paint_mask_buffer =
                gimp_temp_buf_create_buffer ((GimpTempBuf *) paint_mask);

              gimp_gegl_combine_mask_weird (paint_mask_buffer,
                                            GEGL_RECTANGLE (paint_mask_offset_x,
                                                            paint_mask_offset_y,
                                                            width, height),
                                            core->canvas_buffer,
                                            GEGL_RECTANGLE (core->paint_buffer_x,
                                                            core->paint_buffer_y,
                                                            width, height),
                                            paint_opacity,
                                            GIMP_IS_AIRBRUSH (core));

              g_object_unref (paint_mask_buffer);
            }

          mask_buffer = g_object_ref (core->canvas_buffer);
          mask_rect   = *GEGL_RECTANGLE (core->paint_buffer_x,
                                         core->paint_buffer_y,
                                         width, height);

          gimp_applicator_set_src_buffer (core->applicator,
                                          core->undo_buffer);
        }
      /*  Otherwise:
       *   use the paint mask as the mask buffer directly
       */
      else
        {
          mask_buffer =
            gimp_temp_buf_create_buffer ((GimpTempBuf *) paint_mask);
          mask_rect   = *GEGL_RECTANGLE (paint_mask_offset_x,
                                         paint_mask_offset_y,
                                         width, height);

          gimp_applicator_set_src_buffer (core->applicator,
                                          gimp_drawable_get_buffer (drawable));
        }

      if (core->mask_buffer)
        {
          GeglBuffer    *combined_mask_buffer;
          GeglRectangle  combined_mask_rect;
          GeglRectangle  aligned_combined_mask_rect;

          combined_mask_rect = *GEGL_RECTANGLE (core->paint_buffer_x,
                                                core->paint_buffer_y,
                                                width, height);

          gegl_rectangle_align_to_buffer (
            &aligned_combined_mask_rect, &combined_mask_rect,
            gimp_drawable_get_buffer (drawable),
            GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

          combined_mask_buffer = gegl_buffer_new (&aligned_combined_mask_rect,
                                                  babl_format ("Y float"));

          gimp_gegl_buffer_copy (
            core->mask_buffer,
            GEGL_RECTANGLE (aligned_combined_mask_rect.x -
                            core->mask_x_offset,
                            aligned_combined_mask_rect.y -
                            core->mask_y_offset,
                            aligned_combined_mask_rect.width,
                            aligned_combined_mask_rect.height),
            GEGL_ABYSS_NONE,
            combined_mask_buffer,
            &aligned_combined_mask_rect);

          gimp_gegl_combine_mask (mask_buffer,          &mask_rect,
                                  combined_mask_buffer, &combined_mask_rect,
                                  1.0);

          g_object_unref (mask_buffer);

          mask_buffer = combined_mask_buffer;
          mask_rect   = combined_mask_rect;
        }

      gimp_applicator_set_mask_buffer (core->applicator, mask_buffer);
      gimp_applicator_set_mask_offset (core->applicator,
                                       core->paint_buffer_x - mask_rect.x,
                                       core->paint_buffer_y - mask_rect.y);

      gimp_applicator_set_apply_buffer (core->applicator,
                                        core->paint_buffer);
      gimp_applicator_set_apply_offset (core->applicator,
                                        core->paint_buffer_x,
                                        core->paint_buffer_y);

      gimp_applicator_set_opacity (core->applicator, image_opacity);
      gimp_applicator_set_mode (core->applicator, GIMP_LAYER_MODE_REPLACE,
                                GIMP_LAYER_COLOR_SPACE_AUTO,
                                GIMP_LAYER_COLOR_SPACE_AUTO,
                                gimp_layer_mode_get_paint_composite_mode (
                                  GIMP_LAYER_MODE_REPLACE));

      /*  apply the paint area to the image  */
      gimp_applicator_blit (core->applicator,
                            GEGL_RECTANGLE (core->paint_buffer_x,
                                            core->paint_buffer_y,
                                            width, height));

      gimp_applicator_set_mask_buffer (core->applicator, core->mask_buffer);
      gimp_applicator_set_mask_offset (core->applicator,
                                       core->mask_x_offset,
                                       core->mask_y_offset);

      g_object_unref (mask_buffer);
    }
  else
    {
      gimp_paint_core_paste (core, paint_mask,
                             paint_mask_offset_x,
                             paint_mask_offset_y,
                             drawable,
                             paint_opacity,
                             image_opacity,
                             GIMP_LAYER_MODE_REPLACE,
                             mode);
      return;
    }

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
    return; /* Paint core has not initialized yet */

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
        return; /* Just don't bother, nothing to do */

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
              rate = gaussian_weight * exp (-velocity_sum * velocity_sum /
                                            (2 * gaussian_weight2));
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
