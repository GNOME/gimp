/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "paint-types.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-loops.h"
#include "gegl/ligma-gegl-nodes.h"
#include "gegl/ligma-gegl-utils.h"
#include "gegl/ligmaapplicator.h"

#include "core/ligma.h"
#include "core/ligma-utils.h"
#include "core/ligmachannel.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-guides.h"
#include "core/ligmaimage-symmetry.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmapickable.h"
#include "core/ligmaprojection.h"
#include "core/ligmasymmetry.h"
#include "core/ligmatempbuf.h"

#include "ligmapaintcore.h"
#include "ligmapaintcoreundo.h"
#include "ligmapaintcore-loops.h"
#include "ligmapaintoptions.h"

#include "ligmaairbrush.h"

#include "ligma-intl.h"


#define STROKE_BUFFER_INIT_SIZE 2000

enum
{
  PROP_0,
  PROP_UNDO_DESC
};


/*  local function prototypes  */

static void      ligma_paint_core_finalize            (GObject          *object);
static void      ligma_paint_core_set_property        (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void      ligma_paint_core_get_property        (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static gboolean  ligma_paint_core_real_start          (LigmaPaintCore    *core,
                                                      GList            *drawables,
                                                      LigmaPaintOptions *paint_options,
                                                      const LigmaCoords *coords,
                                                      GError          **error);
static gboolean  ligma_paint_core_real_pre_paint      (LigmaPaintCore    *core,
                                                      GList            *drawables,
                                                      LigmaPaintOptions *options,
                                                      LigmaPaintState    paint_state,
                                                      guint32           time);
static void      ligma_paint_core_real_paint          (LigmaPaintCore    *core,
                                                      GList            *drawables,
                                                      LigmaPaintOptions *options,
                                                      LigmaSymmetry     *sym,
                                                      LigmaPaintState    paint_state,
                                                      guint32           time);
static void      ligma_paint_core_real_post_paint     (LigmaPaintCore    *core,
                                                      GList            *drawables,
                                                      LigmaPaintOptions *options,
                                                      LigmaPaintState    paint_state,
                                                      guint32           time);
static void      ligma_paint_core_real_interpolate    (LigmaPaintCore    *core,
                                                      GList            *drawables,
                                                      LigmaPaintOptions *options,
                                                      guint32           time);
static GeglBuffer *
               ligma_paint_core_real_get_paint_buffer (LigmaPaintCore    *core,
                                                      LigmaDrawable     *drawable,
                                                      LigmaPaintOptions *options,
                                                      LigmaLayerMode     paint_mode,
                                                      const LigmaCoords *coords,
                                                      gint             *paint_buffer_x,
                                                      gint             *paint_buffer_y,
                                                      gint             *paint_width,
                                                      gint             *paint_height);
static LigmaUndo* ligma_paint_core_real_push_undo      (LigmaPaintCore    *core,
                                                      LigmaImage        *image,
                                                      const gchar      *undo_desc);


G_DEFINE_TYPE (LigmaPaintCore, ligma_paint_core, LIGMA_TYPE_OBJECT)

#define parent_class ligma_paint_core_parent_class

static gint global_core_ID = 1;


static void
ligma_paint_core_class_init (LigmaPaintCoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = ligma_paint_core_finalize;
  object_class->set_property = ligma_paint_core_set_property;
  object_class->get_property = ligma_paint_core_get_property;

  klass->start               = ligma_paint_core_real_start;
  klass->pre_paint           = ligma_paint_core_real_pre_paint;
  klass->paint               = ligma_paint_core_real_paint;
  klass->post_paint          = ligma_paint_core_real_post_paint;
  klass->interpolate         = ligma_paint_core_real_interpolate;
  klass->get_paint_buffer    = ligma_paint_core_real_get_paint_buffer;
  klass->push_undo           = ligma_paint_core_real_push_undo;

  g_object_class_install_property (object_class, PROP_UNDO_DESC,
                                   g_param_spec_string ("undo-desc", NULL, NULL,
                                                        _("Paint"),
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_paint_core_init (LigmaPaintCore *core)
{
  core->ID = global_core_ID++;
  core->undo_buffers = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_object_unref);
}

static void
ligma_paint_core_finalize (GObject *object)
{
  LigmaPaintCore *core = LIGMA_PAINT_CORE (object);

  ligma_paint_core_cleanup (core);

  g_clear_pointer (&core->undo_desc, g_free);
  g_hash_table_unref (core->undo_buffers);
  if (core->applicators)
    g_hash_table_unref (core->applicators);

  if (core->stroke_buffer)
    {
      g_array_free (core->stroke_buffer, TRUE);
      core->stroke_buffer = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_paint_core_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaPaintCore *core = LIGMA_PAINT_CORE (object);

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
ligma_paint_core_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaPaintCore *core = LIGMA_PAINT_CORE (object);

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
ligma_paint_core_real_start (LigmaPaintCore    *core,
                            GList            *drawables,
                            LigmaPaintOptions *paint_options,
                            const LigmaCoords *coords,
                            GError          **error)
{
  return TRUE;
}

static gboolean
ligma_paint_core_real_pre_paint (LigmaPaintCore    *core,
                                GList            *drawables,
                                LigmaPaintOptions *paint_options,
                                LigmaPaintState    paint_state,
                                guint32           time)
{
  return TRUE;
}

static void
ligma_paint_core_real_paint (LigmaPaintCore    *core,
                            GList            *drawables,
                            LigmaPaintOptions *paint_options,
                            LigmaSymmetry     *sym,
                            LigmaPaintState    paint_state,
                            guint32           time)
{
}

static void
ligma_paint_core_real_post_paint (LigmaPaintCore    *core,
                                 GList            *drawables,
                                 LigmaPaintOptions *paint_options,
                                 LigmaPaintState    paint_state,
                                 guint32           time)
{
}

static void
ligma_paint_core_real_interpolate (LigmaPaintCore    *core,
                                  GList            *drawables,
                                  LigmaPaintOptions *paint_options,
                                  guint32           time)
{
  ligma_paint_core_paint (core, drawables, paint_options,
                         LIGMA_PAINT_STATE_MOTION, time);

  core->last_coords = core->cur_coords;
}

static GeglBuffer *
ligma_paint_core_real_get_paint_buffer (LigmaPaintCore    *core,
                                       LigmaDrawable     *drawable,
                                       LigmaPaintOptions *paint_options,
                                       LigmaLayerMode     paint_mode,
                                       const LigmaCoords *coords,
                                       gint             *paint_buffer_x,
                                       gint             *paint_buffer_y,
                                       gint             *paint_width,
                                       gint             *paint_height)
{
  return NULL;
}

static LigmaUndo *
ligma_paint_core_real_push_undo (LigmaPaintCore *core,
                                LigmaImage     *image,
                                const gchar   *undo_desc)
{
  return ligma_image_undo_push (image, LIGMA_TYPE_PAINT_CORE_UNDO,
                               LIGMA_UNDO_PAINT, undo_desc,
                               0,
                               "paint-core", core,
                               NULL);
}


/*  public functions  */

void
ligma_paint_core_paint (LigmaPaintCore    *core,
                       GList            *drawables,
                       LigmaPaintOptions *paint_options,
                       LigmaPaintState    paint_state,
                       guint32           time)
{
  LigmaPaintCoreClass *core_class;

  g_return_if_fail (LIGMA_IS_PAINT_CORE (core));
  g_return_if_fail (drawables != NULL);
  g_return_if_fail (LIGMA_IS_PAINT_OPTIONS (paint_options));

  core_class = LIGMA_PAINT_CORE_GET_CLASS (core);

  if (core_class->pre_paint (core, drawables,
                             paint_options,
                             paint_state, time))
    {
      LigmaSymmetry *sym;
      LigmaImage    *image;

      image = ligma_item_get_image (LIGMA_ITEM (drawables->data));

      if (paint_state == LIGMA_PAINT_STATE_MOTION)
        {
          /* Save coordinates for ligma_paint_core_interpolate() */
          core->last_paint.x = core->cur_coords.x;
          core->last_paint.y = core->cur_coords.y;
        }

      sym = g_object_ref (ligma_image_get_active_symmetry (image));
      ligma_symmetry_set_origin (sym, drawables->data, &core->cur_coords);

      core_class->paint (core, drawables,
                         paint_options,
                         sym, paint_state, time);

      ligma_symmetry_clear_origin (sym);
      g_object_unref (sym);

      core_class->post_paint (core, drawables,
                              paint_options,
                              paint_state, time);
    }
}

gboolean
ligma_paint_core_start (LigmaPaintCore     *core,
                       GList             *drawables,
                       LigmaPaintOptions  *paint_options,
                       const LigmaCoords  *coords,
                       GError           **error)
{
  LigmaImage   *image;
  LigmaChannel *mask;
  gint         max_width  = 0;
  gint         max_height = 0;

  g_return_val_if_fail (LIGMA_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (g_list_length (drawables) > 0, FALSE);
  g_return_val_if_fail (LIGMA_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  for (GList *iter = drawables; iter; iter = iter->next)
    g_return_val_if_fail (ligma_item_is_attached (iter->data), FALSE);

  image = ligma_item_get_image (LIGMA_ITEM (drawables->data));

  if (core->stroke_buffer)
    {
      g_array_free (core->stroke_buffer, TRUE);
      core->stroke_buffer = NULL;
    }

  core->stroke_buffer = g_array_sized_new (TRUE, TRUE,
                                           sizeof (LigmaCoords),
                                           STROKE_BUFFER_INIT_SIZE);

  /* remember the last stroke's endpoint for later undo */
  core->start_coords = core->last_coords;
  core->cur_coords   = *coords;

  if (paint_options->use_applicator)
    core->applicators = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_object_unref);
  else
    core->applicators  = NULL;

  if (! LIGMA_PAINT_CORE_GET_CLASS (core)->start (core, drawables,
                                                 paint_options,
                                                 coords, error))
    {
      return FALSE;
    }

  /*  Set the image pickable  */
  if (! core->show_all)
    core->image_pickable = LIGMA_PICKABLE (image);
  else
    core->image_pickable = LIGMA_PICKABLE (ligma_image_get_projection (image));

  /*  Allocate the saved proj structure  */
  g_clear_object (&core->saved_proj_buffer);

  if (core->use_saved_proj)
    {
      GeglBuffer *buffer = ligma_pickable_get_buffer (core->image_pickable);

      core->saved_proj_buffer = ligma_gegl_buffer_dup (buffer);
    }

  for (GList *iter = drawables; iter; iter = iter->next)
    {
      /*  Allocate the undo structures  */
      g_hash_table_insert (core->undo_buffers, iter->data,
                           ligma_gegl_buffer_dup (ligma_drawable_get_buffer (iter->data)));
      max_width  = MAX (max_width, ligma_item_get_width (iter->data));
      max_height = MAX (max_height, ligma_item_get_height (iter->data));
    }

  /*  Allocate the canvas blocks structure  */
  if (core->canvas_buffer)
    g_object_unref (core->canvas_buffer);

  core->canvas_buffer =
    gegl_buffer_new (GEGL_RECTANGLE (0, 0, max_width, max_height),
                     babl_format ("Y float"));

  /*  Get the initial undo extents  */

  core->x1 = core->x2 = core->cur_coords.x;
  core->y1 = core->y2 = core->cur_coords.y;

  core->last_paint.x = -1e6;
  core->last_paint.y = -1e6;

  mask = ligma_image_get_mask (image);

  /*  don't apply the mask to itself and don't apply an empty mask  */
  if (! ligma_channel_is_empty (mask) &&
      (g_list_length (drawables) > 1 || LIGMA_DRAWABLE (mask) != drawables->data))
    {
      GeglBuffer *mask_buffer;

      mask_buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask));

      core->mask_buffer   = g_object_ref (mask_buffer);
    }
  else
    {
      core->mask_buffer = NULL;
    }

  if (paint_options->use_applicator)
    {
      for (GList *iter = drawables; iter; iter = iter->next)
        {
          LigmaApplicator *applicator;

          applicator = ligma_applicator_new (NULL);
          g_hash_table_insert (core->applicators, iter->data, applicator);

          if (core->mask_buffer)
            {
              gint offset_x;
              gint offset_y;

              ligma_applicator_set_mask_buffer (applicator,
                                               core->mask_buffer);
              ligma_item_get_offset (iter->data, &offset_x, &offset_y);
              ligma_applicator_set_mask_offset (applicator, -offset_x, -offset_y);
            }

          ligma_applicator_set_affect (applicator,
                                      ligma_drawable_get_active_mask (iter->data));
          ligma_applicator_set_dest_buffer (applicator,
                                           ligma_drawable_get_buffer (iter->data));
        }
    }

  /*  Freeze the drawable preview so that it isn't constantly updated.  */
  for (GList *iter = drawables; iter; iter = iter->next)
    ligma_viewable_preview_freeze (LIGMA_VIEWABLE (iter->data));

  return TRUE;
}

void
ligma_paint_core_finish (LigmaPaintCore *core,
                        GList         *drawables,
                        gboolean       push_undo)
{
  LigmaImage *image;
  gboolean   undo_group_started = FALSE;

  g_return_if_fail (LIGMA_IS_PAINT_CORE (core));

  if (core->applicators)
    {
      g_hash_table_unref (core->applicators);
      core->applicators = NULL;
    }

  if (core->stroke_buffer)
    {
      g_array_free (core->stroke_buffer, TRUE);
      core->stroke_buffer = NULL;
    }

  g_clear_object (&core->mask_buffer);

  image = ligma_item_get_image (LIGMA_ITEM (drawables->data));

  for (GList *iter = drawables; iter; iter = iter->next)
    {
      /*  Determine if any part of the image has been altered--
       *  if nothing has, then just go to the next drawable...
       */
      if ((core->x2 == core->x1) || (core->y2 == core->y1))
        {
          ligma_viewable_preview_thaw (LIGMA_VIEWABLE (iter->data));
          continue;
        }

      if (push_undo)
        {
          GeglBuffer    *undo_buffer;
          GeglBuffer    *buffer;
          GeglRectangle  rect;

          if (! g_hash_table_steal_extended (core->undo_buffers, iter->data,
                                             NULL, (gpointer*) &undo_buffer))
            {
              g_critical ("%s: missing undo buffer for '%s'.",
                          G_STRFUNC, ligma_object_get_name (iter->data));
              continue;
            }

          ligma_rectangle_intersect (core->x1, core->y1,
                                    core->x2 - core->x1, core->y2 - core->y1,
                                    0, 0,
                                    ligma_item_get_width  (LIGMA_ITEM (iter->data)),
                                    ligma_item_get_height (LIGMA_ITEM (iter->data)),
                                    &rect.x, &rect.y, &rect.width, &rect.height);

          gegl_rectangle_align_to_buffer (&rect, &rect, undo_buffer,
                                          GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

          if (! undo_group_started)
            {
              ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_PAINT,
                                           core->undo_desc);
              undo_group_started = TRUE;
            }

          LIGMA_PAINT_CORE_GET_CLASS (core)->push_undo (core, image, NULL);

          buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, rect.width, rect.height),
                                    ligma_drawable_get_format (iter->data));

          ligma_gegl_buffer_copy (undo_buffer,
                                 &rect,
                                 GEGL_ABYSS_NONE,
                                 buffer,
                                 GEGL_RECTANGLE (0, 0, 0, 0));

          ligma_drawable_push_undo (iter->data, NULL,
                                   buffer, rect.x, rect.y, rect.width, rect.height);

          g_object_unref (buffer);
          g_object_unref (undo_buffer);
        }

      ligma_viewable_preview_thaw (LIGMA_VIEWABLE (iter->data));
    }

  core->image_pickable = NULL;

  g_clear_object (&core->saved_proj_buffer);

  if (undo_group_started)
    ligma_image_undo_group_end (image);
}

void
ligma_paint_core_cancel (LigmaPaintCore *core,
                        GList         *drawables)
{
  gint x, y;
  gint width, height;

  g_return_if_fail (LIGMA_IS_PAINT_CORE (core));

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if ((core->x2 == core->x1) || (core->y2 == core->y1))
    return;

  for (GList *iter = drawables; iter; iter = iter->next)
    {
      if (ligma_rectangle_intersect (core->x1, core->y1,
                                    core->x2 - core->x1,
                                    core->y2 - core->y1,
                                    0, 0,
                                    ligma_item_get_width  (LIGMA_ITEM (iter->data)),
                                    ligma_item_get_height (LIGMA_ITEM (iter->data)),
                                    &x, &y, &width, &height))
        {
          GeglBuffer    *undo_buffer;
          GeglRectangle  rect;

          if (! g_hash_table_steal_extended (core->undo_buffers, iter->data,
                                             NULL, (gpointer*) &undo_buffer))
            {
              g_critical ("%s: missing undo buffer for '%s'.",
                          G_STRFUNC, ligma_object_get_name (iter->data));
              continue;
            }

          gegl_rectangle_align_to_buffer (&rect,
                                          GEGL_RECTANGLE (x, y, width, height),
                                          ligma_drawable_get_buffer (iter->data),
                                          GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

          ligma_gegl_buffer_copy (undo_buffer,
                                 &rect,
                                 GEGL_ABYSS_NONE,
                                 ligma_drawable_get_buffer (iter->data),
                                 &rect);
          g_object_unref (undo_buffer);
        }

      ligma_drawable_update (iter->data, x, y, width, height);

      ligma_viewable_preview_thaw (LIGMA_VIEWABLE (iter->data));
    }

  g_clear_object (&core->saved_proj_buffer);
}

void
ligma_paint_core_cleanup (LigmaPaintCore *core)
{
  g_return_if_fail (LIGMA_IS_PAINT_CORE (core));

  g_hash_table_remove_all (core->undo_buffers);

  g_clear_object (&core->saved_proj_buffer);
  g_clear_object (&core->canvas_buffer);
  g_clear_object (&core->paint_buffer);
}

void
ligma_paint_core_interpolate (LigmaPaintCore    *core,
                             GList            *drawables,
                             LigmaPaintOptions *paint_options,
                             const LigmaCoords *coords,
                             guint32           time)
{
  g_return_if_fail (LIGMA_IS_PAINT_CORE (core));
  g_return_if_fail (drawables != NULL);
  g_return_if_fail (LIGMA_IS_PAINT_OPTIONS (paint_options));
  g_return_if_fail (coords != NULL);

  core->cur_coords = *coords;

  LIGMA_PAINT_CORE_GET_CLASS (core)->interpolate (core, drawables,
                                                 paint_options, time);
}

void
ligma_paint_core_set_show_all (LigmaPaintCore *core,
                              gboolean       show_all)
{
  g_return_if_fail (LIGMA_IS_PAINT_CORE (core));

  core->show_all = show_all;
}

gboolean
ligma_paint_core_get_show_all (LigmaPaintCore *core)
{
  g_return_val_if_fail (LIGMA_IS_PAINT_CORE (core), FALSE);

  return core->show_all;
}

void
ligma_paint_core_set_current_coords (LigmaPaintCore    *core,
                                    const LigmaCoords *coords)
{
  g_return_if_fail (LIGMA_IS_PAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  core->cur_coords = *coords;
}

void
ligma_paint_core_get_current_coords (LigmaPaintCore    *core,
                                    LigmaCoords       *coords)
{
  g_return_if_fail (LIGMA_IS_PAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  *coords = core->cur_coords;
}

void
ligma_paint_core_set_last_coords (LigmaPaintCore    *core,
                                 const LigmaCoords *coords)
{
  g_return_if_fail (LIGMA_IS_PAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  core->last_coords = *coords;
}

void
ligma_paint_core_get_last_coords (LigmaPaintCore *core,
                                 LigmaCoords    *coords)
{
  g_return_if_fail (LIGMA_IS_PAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  *coords = core->last_coords;
}

/**
 * ligma_paint_core_round_line:
 * @core:                   the #LigmaPaintCore
 * @options:                the #LigmaPaintOptions to use
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
ligma_paint_core_round_line (LigmaPaintCore    *core,
                            LigmaPaintOptions *paint_options,
                            gboolean          constrain_15_degrees,
                            gdouble           constrain_offset_angle,
                            gdouble           constrain_xres,
                            gdouble           constrain_yres)
{
  g_return_if_fail (LIGMA_IS_PAINT_CORE (core));
  g_return_if_fail (LIGMA_IS_PAINT_OPTIONS (paint_options));

  if (ligma_paint_options_get_brush_mode (paint_options) == LIGMA_BRUSH_HARD)
    {
      core->last_coords.x = floor (core->last_coords.x) + 0.5;
      core->last_coords.y = floor (core->last_coords.y) + 0.5;
      core->cur_coords.x  = floor (core->cur_coords.x ) + 0.5;
      core->cur_coords.y  = floor (core->cur_coords.y ) + 0.5;
    }

  if (constrain_15_degrees)
    ligma_constrain_line (core->last_coords.x, core->last_coords.y,
                         &core->cur_coords.x, &core->cur_coords.y,
                         LIGMA_CONSTRAIN_LINE_15_DEGREES,
                         constrain_offset_angle,
                         constrain_xres, constrain_yres);
}


/*  protected functions  */

GeglBuffer *
ligma_paint_core_get_paint_buffer (LigmaPaintCore    *core,
                                  LigmaDrawable     *drawable,
                                  LigmaPaintOptions *paint_options,
                                  LigmaLayerMode     paint_mode,
                                  const LigmaCoords *coords,
                                  gint             *paint_buffer_x,
                                  gint             *paint_buffer_y,
                                  gint             *paint_width,
                                  gint             *paint_height)
{
  GeglBuffer *paint_buffer;

  g_return_val_if_fail (LIGMA_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)), NULL);
  g_return_val_if_fail (LIGMA_IS_PAINT_OPTIONS (paint_options), NULL);
  g_return_val_if_fail (coords != NULL, NULL);
  g_return_val_if_fail (paint_buffer_x != NULL, NULL);
  g_return_val_if_fail (paint_buffer_y != NULL, NULL);

  paint_buffer =
    LIGMA_PAINT_CORE_GET_CLASS (core)->get_paint_buffer (core, drawable,
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

LigmaPickable *
ligma_paint_core_get_image_pickable (LigmaPaintCore *core)
{
  g_return_val_if_fail (LIGMA_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (core->image_pickable != NULL, NULL);

  return core->image_pickable;
}

GeglBuffer *
ligma_paint_core_get_orig_image (LigmaPaintCore *core,
                                LigmaDrawable  *drawable)
{
  GeglBuffer *undo_buffer;

  g_return_val_if_fail (LIGMA_IS_PAINT_CORE (core), NULL);

  undo_buffer = g_hash_table_lookup (core->undo_buffers, drawable);
  g_return_val_if_fail (undo_buffer != NULL, NULL);

  return undo_buffer;
}

GeglBuffer *
ligma_paint_core_get_orig_proj (LigmaPaintCore *core)
{
  g_return_val_if_fail (LIGMA_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (core->saved_proj_buffer != NULL, NULL);

  return core->saved_proj_buffer;
}

void
ligma_paint_core_paste (LigmaPaintCore            *core,
                       const LigmaTempBuf        *paint_mask,
                       gint                      paint_mask_offset_x,
                       gint                      paint_mask_offset_y,
                       LigmaDrawable             *drawable,
                       gdouble                   paint_opacity,
                       gdouble                   image_opacity,
                       LigmaLayerMode             paint_mode,
                       LigmaPaintApplicationMode  mode)
{
  gint               width  = gegl_buffer_get_width  (core->paint_buffer);
  gint               height = gegl_buffer_get_height (core->paint_buffer);
  LigmaComponentMask  affect = ligma_drawable_get_active_mask (drawable);
  GeglBuffer        *undo_buffer;

  undo_buffer = g_hash_table_lookup (core->undo_buffers, drawable);

  if (! affect)
    return;

  if (core->applicators)
    {
      LigmaApplicator *applicator;

      applicator = g_hash_table_lookup (core->applicators, drawable);

      /*  If the mode is CONSTANT:
       *   combine the canvas buffer and the paint mask to the paint buffer
       */
      if (mode == LIGMA_PAINT_CONSTANT)
        {
          /* Some tools (ink) paint the mask to paint_core->canvas_buffer
           * directly. Don't need to copy it in this case.
           */
          if (paint_mask != NULL)
            {
              GeglBuffer *paint_mask_buffer =
                ligma_temp_buf_create_buffer ((LigmaTempBuf *) paint_mask);

              ligma_gegl_combine_mask_weird (paint_mask_buffer,
                                            GEGL_RECTANGLE (paint_mask_offset_x,
                                                            paint_mask_offset_y,
                                                            width, height),
                                            core->canvas_buffer,
                                            GEGL_RECTANGLE (core->paint_buffer_x,
                                                            core->paint_buffer_y,
                                                            width, height),
                                            paint_opacity,
                                            LIGMA_IS_AIRBRUSH (core));

              g_object_unref (paint_mask_buffer);
            }

          ligma_gegl_apply_mask (core->canvas_buffer,
                                GEGL_RECTANGLE (core->paint_buffer_x,
                                                core->paint_buffer_y,
                                                width, height),
                                core->paint_buffer,
                                GEGL_RECTANGLE (0, 0, width, height),
                                1.0);

          ligma_applicator_set_src_buffer (applicator, undo_buffer);
        }
      /*  Otherwise:
       *   combine the paint mask to the paint buffer directly
       */
      else
        {
          GeglBuffer *paint_mask_buffer =
            ligma_temp_buf_create_buffer ((LigmaTempBuf *) paint_mask);

          ligma_gegl_apply_mask (paint_mask_buffer,
                                GEGL_RECTANGLE (paint_mask_offset_x,
                                                paint_mask_offset_y,
                                                width, height),
                                core->paint_buffer,
                                GEGL_RECTANGLE (0, 0, width, height),
                                paint_opacity);

          g_object_unref (paint_mask_buffer);

          ligma_applicator_set_src_buffer (applicator,
                                          ligma_drawable_get_buffer (drawable));
        }

      ligma_applicator_set_apply_buffer (applicator,
                                        core->paint_buffer);
      ligma_applicator_set_apply_offset (applicator,
                                        core->paint_buffer_x,
                                        core->paint_buffer_y);

      ligma_applicator_set_opacity (applicator, image_opacity);
      ligma_applicator_set_mode (applicator, paint_mode,
                                LIGMA_LAYER_COLOR_SPACE_AUTO,
                                LIGMA_LAYER_COLOR_SPACE_AUTO,
                                ligma_layer_mode_get_paint_composite_mode (paint_mode));

      /*  apply the paint area to the image  */
      ligma_applicator_blit (applicator,
                            GEGL_RECTANGLE (core->paint_buffer_x,
                                            core->paint_buffer_y,
                                            width, height));
    }
  else
    {
      LigmaPaintCoreLoopsParams    params = {};
      LigmaPaintCoreLoopsAlgorithm algorithms = LIGMA_PAINT_CORE_LOOPS_ALGORITHM_NONE;

      params.paint_buf          = ligma_gegl_buffer_get_temp_buf (core->paint_buffer);
      params.paint_buf_offset_x = core->paint_buffer_x;
      params.paint_buf_offset_y = core->paint_buffer_y;

      if (! params.paint_buf)
        return;

      params.dest_buffer = ligma_drawable_get_buffer (drawable);

      if (mode == LIGMA_PAINT_CONSTANT)
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
              params.stipple             = LIGMA_IS_AIRBRUSH (core);
              params.paint_opacity       = paint_opacity;

              algorithms |= LIGMA_PAINT_CORE_LOOPS_ALGORITHM_COMBINE_PAINT_MASK_TO_CANVAS_BUFFER;
            }

          /* Write canvas_buffer to paint_buf */
          algorithms |= LIGMA_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_COMP_MASK;

          /* undo buf -> paint_buf -> dest_buffer */
          params.src_buffer = undo_buffer;
        }
      else
        {
          g_return_if_fail (paint_mask);

          /* Write paint_mask to paint_buf, does not modify canvas_buffer */
          params.paint_mask          = paint_mask;
          params.paint_mask_offset_x = paint_mask_offset_x;
          params.paint_mask_offset_y = paint_mask_offset_y;
          params.paint_opacity       = paint_opacity;

          algorithms |= LIGMA_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK;

          /* dest_buffer -> paint_buf -> dest_buffer */
          params.src_buffer = params.dest_buffer;
        }

      ligma_item_get_offset (LIGMA_ITEM (drawable),
                            &params.mask_offset_x, &params.mask_offset_y);
      params.mask_offset_x = -params.mask_offset_x;
      params.mask_offset_y = -params.mask_offset_y;
      params.mask_buffer   = core->mask_buffer;
      params.image_opacity = image_opacity;
      params.paint_mode    = paint_mode;

      algorithms |= LIGMA_PAINT_CORE_LOOPS_ALGORITHM_DO_LAYER_BLEND;

      if (affect != LIGMA_COMPONENT_MASK_ALL)
        {
          params.affect = affect;

          algorithms |= LIGMA_PAINT_CORE_LOOPS_ALGORITHM_MASK_COMPONENTS;
        }

      ligma_paint_core_loops_process (&params, algorithms);
    }

  /*  Update the undo extents  */
  core->x1 = MIN (core->x1, core->paint_buffer_x);
  core->y1 = MIN (core->y1, core->paint_buffer_y);
  core->x2 = MAX (core->x2, core->paint_buffer_x + width);
  core->y2 = MAX (core->y2, core->paint_buffer_y + height);

  /*  Update the drawable  */
  ligma_drawable_update (drawable,
                        core->paint_buffer_x,
                        core->paint_buffer_y,
                        width, height);
}

/* This works similarly to ligma_paint_core_paste. However, instead of
 * combining the canvas to the paint core drawable using one of the
 * combination modes, it uses a "replace" mode (i.e. transparent
 * pixels in the canvas erase the paint core drawable).

 * When not drawing on alpha-enabled images, it just paints using
 * NORMAL mode.
 */
void
ligma_paint_core_replace (LigmaPaintCore            *core,
                         const LigmaTempBuf        *paint_mask,
                         gint                      paint_mask_offset_x,
                         gint                      paint_mask_offset_y,
                         LigmaDrawable             *drawable,
                         gdouble                   paint_opacity,
                         gdouble                   image_opacity,
                         LigmaPaintApplicationMode  mode)
{
  GeglBuffer        *undo_buffer;
  gint               width, height;
  LigmaComponentMask  affect;

  if (! ligma_drawable_has_alpha (drawable))
    {
      ligma_paint_core_paste (core, paint_mask,
                             paint_mask_offset_x,
                             paint_mask_offset_y,
                             drawable,
                             paint_opacity,
                             image_opacity,
                             LIGMA_LAYER_MODE_NORMAL,
                             mode);
      return;
    }

  width  = gegl_buffer_get_width  (core->paint_buffer);
  height = gegl_buffer_get_height (core->paint_buffer);

  affect = ligma_drawable_get_active_mask (drawable);

  if (! affect)
    return;

  undo_buffer = g_hash_table_lookup (core->undo_buffers, drawable);

  if (core->applicators)
    {
      LigmaApplicator *applicator;
      GeglRectangle   mask_rect;
      GeglBuffer     *mask_buffer;
      gint            offset_x;
      gint            offset_y;

      applicator = g_hash_table_lookup (core->applicators, drawable);

      /*  If the mode is CONSTANT:
       *   combine the paint mask to the canvas buffer, and use it as the mask
       *   buffer
       */
      if (mode == LIGMA_PAINT_CONSTANT)
        {
          /* Some tools (ink) paint the mask to paint_core->canvas_buffer
           * directly. Don't need to copy it in this case.
           */
          if (paint_mask != NULL)
            {
              GeglBuffer *paint_mask_buffer =
                ligma_temp_buf_create_buffer ((LigmaTempBuf *) paint_mask);

              ligma_gegl_combine_mask_weird (paint_mask_buffer,
                                            GEGL_RECTANGLE (paint_mask_offset_x,
                                                            paint_mask_offset_y,
                                                            width, height),
                                            core->canvas_buffer,
                                            GEGL_RECTANGLE (core->paint_buffer_x,
                                                            core->paint_buffer_y,
                                                            width, height),
                                            paint_opacity,
                                            LIGMA_IS_AIRBRUSH (core));

              g_object_unref (paint_mask_buffer);
            }

          mask_buffer = g_object_ref (core->canvas_buffer);
          mask_rect   = *GEGL_RECTANGLE (core->paint_buffer_x,
                                         core->paint_buffer_y,
                                         width, height);

          ligma_applicator_set_src_buffer (applicator, undo_buffer);
        }
      /*  Otherwise:
       *   use the paint mask as the mask buffer directly
       */
      else
        {
          mask_buffer =
            ligma_temp_buf_create_buffer ((LigmaTempBuf *) paint_mask);
          mask_rect   = *GEGL_RECTANGLE (paint_mask_offset_x,
                                         paint_mask_offset_y,
                                         width, height);

          ligma_applicator_set_src_buffer (applicator,
                                          ligma_drawable_get_buffer (drawable));
        }

      ligma_item_get_offset (LIGMA_ITEM (drawable), &offset_x, &offset_y);
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
            ligma_drawable_get_buffer (drawable),
            GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

          combined_mask_buffer = gegl_buffer_new (&aligned_combined_mask_rect,
                                                  babl_format ("Y float"));

          ligma_gegl_buffer_copy (
            core->mask_buffer,
            GEGL_RECTANGLE (aligned_combined_mask_rect.x + offset_x,
                            aligned_combined_mask_rect.y + offset_y,
                            aligned_combined_mask_rect.width,
                            aligned_combined_mask_rect.height),
            GEGL_ABYSS_NONE,
            combined_mask_buffer,
            &aligned_combined_mask_rect);

          ligma_gegl_combine_mask (mask_buffer,          &mask_rect,
                                  combined_mask_buffer, &combined_mask_rect,
                                  1.0);

          g_object_unref (mask_buffer);

          mask_buffer = combined_mask_buffer;
          mask_rect   = combined_mask_rect;
        }

      ligma_applicator_set_mask_buffer (applicator, mask_buffer);
      ligma_applicator_set_mask_offset (applicator,
                                       core->paint_buffer_x - mask_rect.x,
                                       core->paint_buffer_y - mask_rect.y);

      ligma_applicator_set_apply_buffer (applicator,
                                        core->paint_buffer);
      ligma_applicator_set_apply_offset (applicator,
                                        core->paint_buffer_x,
                                        core->paint_buffer_y);

      ligma_applicator_set_opacity (applicator, image_opacity);
      ligma_applicator_set_mode (applicator, LIGMA_LAYER_MODE_REPLACE,
                                LIGMA_LAYER_COLOR_SPACE_AUTO,
                                LIGMA_LAYER_COLOR_SPACE_AUTO,
                                ligma_layer_mode_get_paint_composite_mode (
                                  LIGMA_LAYER_MODE_REPLACE));

      /*  apply the paint area to the image  */
      ligma_applicator_blit (applicator,
                            GEGL_RECTANGLE (core->paint_buffer_x,
                                            core->paint_buffer_y,
                                            width, height));

      ligma_applicator_set_mask_buffer (applicator, core->mask_buffer);
      ligma_applicator_set_mask_offset (applicator, -offset_x, -offset_y);

      g_object_unref (mask_buffer);
    }
  else
    {
      ligma_paint_core_paste (core, paint_mask,
                             paint_mask_offset_x,
                             paint_mask_offset_y,
                             drawable,
                             paint_opacity,
                             image_opacity,
                             LIGMA_LAYER_MODE_REPLACE,
                             mode);
      return;
    }

  /*  Update the undo extents  */
  core->x1 = MIN (core->x1, core->paint_buffer_x);
  core->y1 = MIN (core->y1, core->paint_buffer_y);
  core->x2 = MAX (core->x2, core->paint_buffer_x + width);
  core->y2 = MAX (core->y2, core->paint_buffer_y + height);

  /*  Update the drawable  */
  ligma_drawable_update (drawable,
                        core->paint_buffer_x,
                        core->paint_buffer_y,
                        width, height);
}

/**
 * Smooth and store coords in the stroke buffer
 */

void
ligma_paint_core_smooth_coords (LigmaPaintCore    *core,
                               LigmaPaintOptions *paint_options,
                               LigmaCoords       *coords)
{
  LigmaSmoothingOptions *smoothing_options = paint_options->smoothing_options;
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
          LigmaCoords *next_coords = &g_array_index (history,
                                                    LigmaCoords, i);

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
