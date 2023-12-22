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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "gegl/gimp-babl.h"
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
#include "core/gimpimage-undo-push.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
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
                                                      GList            *drawables,
                                                      GimpPaintOptions *paint_options,
                                                      const GimpCoords *coords,
                                                      GError          **error);
static gboolean  gimp_paint_core_real_pre_paint      (GimpPaintCore    *core,
                                                      GList            *drawables,
                                                      GimpPaintOptions *options,
                                                      GimpPaintState    paint_state,
                                                      guint32           time);
static void      gimp_paint_core_real_paint          (GimpPaintCore    *core,
                                                      GList            *drawables,
                                                      GimpPaintOptions *options,
                                                      GimpSymmetry     *sym,
                                                      GimpPaintState    paint_state,
                                                      guint32           time);
static void      gimp_paint_core_real_post_paint     (GimpPaintCore    *core,
                                                      GList            *drawables,
                                                      GimpPaintOptions *options,
                                                      GimpPaintState    paint_state,
                                                      guint32           time);
static void      gimp_paint_core_real_interpolate    (GimpPaintCore    *core,
                                                      GList            *drawables,
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
  core->undo_buffers = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                              NULL, g_object_unref);
  core->original_bounds = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                 NULL, g_free);
}

static void
gimp_paint_core_finalize (GObject *object)
{
  GimpPaintCore *core = GIMP_PAINT_CORE (object);

  gimp_paint_core_cleanup (core);

  g_clear_pointer (&core->undo_desc, g_free);
  g_hash_table_unref (core->undo_buffers);
  g_hash_table_unref (core->original_bounds);
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
                            GList            *drawables,
                            GimpPaintOptions *paint_options,
                            const GimpCoords *coords,
                            GError          **error)
{
  return TRUE;
}

static gboolean
gimp_paint_core_real_pre_paint (GimpPaintCore    *core,
                                GList            *drawables,
                                GimpPaintOptions *paint_options,
                                GimpPaintState    paint_state,
                                guint32           time)
{
  return TRUE;
}

static void
gimp_paint_core_real_paint (GimpPaintCore    *core,
                            GList            *drawables,
                            GimpPaintOptions *paint_options,
                            GimpSymmetry     *sym,
                            GimpPaintState    paint_state,
                            guint32           time)
{
}

static void
gimp_paint_core_real_post_paint (GimpPaintCore    *core,
                                 GList            *drawables,
                                 GimpPaintOptions *paint_options,
                                 GimpPaintState    paint_state,
                                 guint32           time)
{
}

static void
gimp_paint_core_real_interpolate (GimpPaintCore    *core,
                                  GList            *drawables,
                                  GimpPaintOptions *paint_options,
                                  guint32           time)
{
  gimp_paint_core_paint (core, drawables, paint_options,
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
                       GList            *drawables,
                       GimpPaintOptions *paint_options,
                       GimpPaintState    paint_state,
                       guint32           time)
{
  GimpPaintCoreClass *core_class;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (drawables != NULL);
  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options));

  core_class = GIMP_PAINT_CORE_GET_CLASS (core);

  if (core_class->pre_paint (core, drawables,
                             paint_options,
                             paint_state, time))
    {
      GimpSymmetry *sym;
      GimpImage    *image;

      image = gimp_item_get_image (GIMP_ITEM (drawables->data));

      if (paint_state == GIMP_PAINT_STATE_MOTION)
        {
          /* Save coordinates for gimp_paint_core_interpolate() */
          core->last_paint.x = core->cur_coords.x;
          core->last_paint.y = core->cur_coords.y;
        }

      sym = g_object_ref (gimp_image_get_active_symmetry (image));
      gimp_symmetry_set_origin (sym, drawables->data, &core->cur_coords);

      core_class->paint (core, drawables,
                         paint_options,
                         sym, paint_state, time);

      gimp_symmetry_clear_origin (sym);
      g_object_unref (sym);

      core_class->post_paint (core, drawables,
                              paint_options,
                              paint_state, time);
    }
}

gboolean
gimp_paint_core_start (GimpPaintCore     *core,
                       GList             *drawables,
                       GimpPaintOptions  *paint_options,
                       const GimpCoords  *coords,
                       GError           **error)
{
  GimpImage     *image;
  GimpChannel   *mask;
  gint           max_width  = 0;
  gint           max_height = 0;
  GeglRectangle *rect;

  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (g_list_length (drawables) > 0, FALSE);
  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  for (GList *iter = drawables; iter; iter = iter->next)
    g_return_val_if_fail (gimp_item_is_attached (iter->data), FALSE);

  image = gimp_item_get_image (GIMP_ITEM (drawables->data));

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
  core->cur_coords   = *coords;

  if (paint_options->use_applicator)
    core->applicators = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_object_unref);
  else
    core->applicators  = NULL;

  if (! GIMP_PAINT_CORE_GET_CLASS (core)->start (core, drawables,
                                                 paint_options,
                                                 coords, error))
    {
      return FALSE;
    }

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

  for (GList *iter = drawables; iter; iter = iter->next)
    {
      /*  Allocate the undo structures  */
      rect         = g_new (GeglRectangle, 1);
      rect->width  = gimp_item_get_width (GIMP_ITEM (iter->data));
      rect->height = gimp_item_get_height (GIMP_ITEM (iter->data));
      gimp_item_get_offset (GIMP_ITEM (iter->data), &rect->x, &rect->y);

      g_hash_table_insert (core->original_bounds, iter->data,
                           rect);
      g_hash_table_insert (core->undo_buffers, iter->data,
                           gimp_gegl_buffer_dup (gimp_drawable_get_buffer (iter->data)));

      max_width  = MAX (max_width, gimp_item_get_width (iter->data));
      max_height = MAX (max_height, gimp_item_get_height (iter->data));
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

  mask = gimp_image_get_mask (image);

  /*  don't apply the mask to itself and don't apply an empty mask  */
  if (! gimp_channel_is_empty (mask) &&
      (g_list_length (drawables) > 1 || GIMP_DRAWABLE (mask) != drawables->data))
    {
      GeglBuffer *mask_buffer;

      mask_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

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
          GimpApplicator *applicator;

          applicator = gimp_applicator_new (NULL);
          g_hash_table_insert (core->applicators, iter->data, applicator);

          if (core->mask_buffer)
            {
              gint offset_x;
              gint offset_y;

              gimp_applicator_set_mask_buffer (applicator,
                                               core->mask_buffer);
              gimp_item_get_offset (iter->data, &offset_x, &offset_y);
              gimp_applicator_set_mask_offset (applicator, -offset_x, -offset_y);
            }

          gimp_applicator_set_affect (applicator,
                                      gimp_drawable_get_active_mask (iter->data));
          gimp_applicator_set_dest_buffer (applicator,
                                           gimp_drawable_get_buffer (iter->data));
        }
    }

  /* initialize the lock_blink_state */
  core->lock_blink_state = GIMP_PAINT_LOCK_NOT_BLINKED;

  /*  Freeze the drawable preview so that it isn't constantly updated.  */
  for (GList *iter = drawables; iter; iter = iter->next)
    gimp_viewable_preview_freeze (GIMP_VIEWABLE (iter->data));

  return TRUE;
}

void
gimp_paint_core_finish (GimpPaintCore *core,
                        GList         *drawables,
                        gboolean       push_undo)
{
  GimpImage *image;
  gboolean   undo_group_started = FALSE;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));

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

  image = gimp_item_get_image (GIMP_ITEM (drawables->data));

  for (GList *iter = drawables; iter; iter = iter->next)
    {
      /*  Determine if any part of the image has been altered--
       *  if nothing has, then just go to the next drawable...
       */
      if ((core->x2 == core->x1) || (core->y2 == core->y1))
        {
          gimp_viewable_preview_thaw (GIMP_VIEWABLE (iter->data));
          continue;
        }

      if (push_undo)
        {
          GeglBuffer    *undo_buffer;
          GeglBuffer    *buffer;
          GeglBuffer    *drawable_buffer;
          GeglRectangle  rect;
          GeglRectangle  old_rect;

          if (! g_hash_table_steal_extended (core->undo_buffers, iter->data,
                                             NULL, (gpointer*) &undo_buffer))
            {
              g_critical ("%s: missing undo buffer for '%s'.",
                          G_STRFUNC, gimp_object_get_name (iter->data));
              continue;
            }

          if (! undo_group_started)
            {
              gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_PAINT,
                                           core->undo_desc);
              undo_group_started = TRUE;
            }

          /* get new and old bounds of drawable */
          old_rect = *(GeglRectangle*) g_hash_table_lookup (core->original_bounds, iter->data);

          gimp_item_get_offset (GIMP_ITEM (iter->data), &rect.x, &rect.y);
          rect.width = gimp_item_get_width (GIMP_ITEM (iter->data));
          rect.height = gimp_item_get_height (GIMP_ITEM (iter->data));

          /* Making copy of entire buffer consumes more memory, so do that only when buffer has resized */
          if (rect.x == old_rect.x         &&
              rect.y == old_rect.y         &&
              rect.width == old_rect.width &&
              rect.height == old_rect.height)
            {
              gimp_rectangle_intersect (core->x1, core->y1, core->x2 - core->x1,
                                        core->y2 - core->y1, 0, 0,
                                        gimp_item_get_width  (GIMP_ITEM (iter->data)),
                                        gimp_item_get_height (GIMP_ITEM (iter->data)),
                                        &rect.x, &rect.y, &rect.width, &rect.height);

              gegl_rectangle_align_to_buffer (&rect, &rect, undo_buffer,
                                              GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

              GIMP_PAINT_CORE_GET_CLASS (core)->push_undo (core, image, NULL);

              buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, rect.width, rect.height),
                                        gimp_drawable_get_format (iter->data));

              gimp_gegl_buffer_copy (undo_buffer,
                                     &rect,
                                     GEGL_ABYSS_NONE,
                                     buffer,
                                     GEGL_RECTANGLE (0, 0, 0, 0));

              gimp_drawable_push_undo (iter->data, NULL,
                                       buffer, rect.x, rect.y, rect.width, rect.height);
            }
          else
            {
              /* drawable is expanded only if drawable is layer or layer mask*/
              g_return_if_fail (GIMP_IS_LAYER (iter->data) || GIMP_IS_LAYER_MASK (iter->data));

              /* create a copy of original buffer from undo data */
              buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                        old_rect.width,
                                                        old_rect.height),
                                        gimp_drawable_get_format (iter->data));

              gimp_gegl_buffer_copy (undo_buffer,
                                     GEGL_RECTANGLE (old_rect.x - rect.x,
                                                     old_rect.y - rect.y,
                                                     old_rect.width,
                                                     old_rect.height),
                                     GEGL_ABYSS_NONE,
                                     buffer,
                                     GEGL_RECTANGLE (0, 0, 0, 0));

              /* make a backup copy of drawable to restore */
              drawable_buffer = g_object_ref (gimp_drawable_get_buffer (GIMP_DRAWABLE (iter->data)));

              if (GIMP_IS_LAYER_MASK (drawables->data) || GIMP_LAYER (drawables->data)->mask)
                {
                  GeglBuffer   *other_new;
                  GeglBuffer   *other_old;
                  GimpDrawable *other_drawable;

                  if (GIMP_IS_LAYER_MASK (drawables->data))
                    other_drawable = GIMP_DRAWABLE ((GIMP_LAYER_MASK (drawables->data))->layer);
                  else
                    other_drawable = GIMP_DRAWABLE (GIMP_LAYER (drawables->data)->mask);

                  /* create a copy of original buffer by taking the required area */
                  other_old = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                               old_rect.width,
                                                               old_rect.height),
                                               gimp_drawable_get_format (other_drawable));

                  gimp_gegl_buffer_copy (gimp_drawable_get_buffer (other_drawable),
                                         GEGL_RECTANGLE (old_rect.x - rect.x,
                                                         old_rect.y - rect.y,
                                                         old_rect.width,
                                                         old_rect.height),
                                         GEGL_ABYSS_NONE,
                                         other_old,
                                         GEGL_RECTANGLE (0, 0, 0, 0));

                  /* make a backup copy of drawable to restore */
                  other_new = g_object_ref (gimp_drawable_get_buffer (other_drawable));

                  gimp_drawable_set_buffer_full (other_drawable, FALSE, NULL,
                                                 other_old, &old_rect,
                                                 FALSE);
                  gimp_drawable_set_buffer_full (other_drawable, TRUE, NULL,
                                                 other_new, &rect,
                                                 FALSE);

                  g_object_unref (other_new);
                  g_object_unref (other_old);
                }
              /* Restore drawable to state before painting started */
              gimp_drawable_set_buffer_full (iter->data, FALSE, NULL,
                                             buffer, &old_rect,
                                             FALSE);
              /* Change the drawable again but push undo this time */
              gimp_drawable_set_buffer_full (iter->data, TRUE, NULL,
                                             drawable_buffer, &rect,
                                             FALSE);

              g_object_unref (drawable_buffer);
            }

          g_object_unref (buffer);
          g_object_unref (undo_buffer);
        }

      gimp_viewable_preview_thaw (GIMP_VIEWABLE (iter->data));
    }

  core->image_pickable = NULL;

  g_clear_object (&core->saved_proj_buffer);

  if (undo_group_started)
    gimp_image_undo_group_end (image);
}

void
gimp_paint_core_cancel (GimpPaintCore *core,
                        GList         *drawables)
{
  gint x, y;
  gint width, height;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if ((core->x2 == core->x1) || (core->y2 == core->y1))
    return;

  for (GList *iter = drawables; iter; iter = iter->next)
    {
      if (gimp_rectangle_intersect (core->x1, core->y1,
                                    core->x2 - core->x1,
                                    core->y2 - core->y1,
                                    0, 0,
                                    gimp_item_get_width  (GIMP_ITEM (iter->data)),
                                    gimp_item_get_height (GIMP_ITEM (iter->data)),
                                    &x, &y, &width, &height))
        {
          GeglBuffer    *undo_buffer;
          GeglRectangle  new_rect;
          GeglRectangle  old_rect;

          if (! g_hash_table_steal_extended (core->undo_buffers, iter->data,
                                             NULL, (gpointer*) &undo_buffer))
            {
              g_critical ("%s: missing undo buffer for '%s'.",
                          G_STRFUNC, gimp_object_get_name (iter->data));
              continue;
            }

          old_rect = *(GeglRectangle*) g_hash_table_lookup (core->original_bounds, iter->data);

          gimp_item_get_offset (GIMP_ITEM (iter->data), &new_rect.x, &new_rect.y);
          new_rect.width = gimp_item_get_width (GIMP_ITEM (iter->data));
          new_rect.height = gimp_item_get_height (GIMP_ITEM (iter->data));

          if (new_rect.x == old_rect.x         &&
              new_rect.y == old_rect.y         &&
              new_rect.width == old_rect.width &&
              new_rect.height == old_rect.height)
            {
              GeglRectangle  rect;

              gegl_rectangle_align_to_buffer (&rect,
                                              GEGL_RECTANGLE (x, y, width, height),
                                              gimp_drawable_get_buffer (iter->data),
                                              GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

              gimp_gegl_buffer_copy (undo_buffer,
                                     &rect,
                                     GEGL_ABYSS_NONE,
                                     gimp_drawable_get_buffer (iter->data),
                                     &rect);

              gimp_drawable_update (iter->data, x, y, width, height);
            }
          else
            {
              GeglBuffer    *buffer;
              GeglRectangle  bbox;

              /* drawable is expanded only if drawable is layer or layer mask,
               * so drawable cannot be anything else */
              g_return_if_fail (GIMP_IS_LAYER (iter->data) || GIMP_IS_LAYER_MASK (iter->data));

              /* When canceling painting with drawable expansion, ensure that
               * the drawable display outside the reverted size is not shown. We
               * cannot use gimp_drawable_update() because it won't work while
               * painting. Directly emit the "update" signal.
               * */
              bbox = gimp_drawable_get_bounding_box (iter->data);
              g_signal_emit_by_name (iter->data, "update", bbox.x, bbox.y, bbox.width, bbox.height);

              /* create a copy of original buffer from undo data */
              buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                        old_rect.width,
                                                        old_rect.height),
                                        gimp_drawable_get_format (iter->data));

              gimp_gegl_buffer_copy (undo_buffer,
                                     GEGL_RECTANGLE (old_rect.x - new_rect.x,
                                                     old_rect.y - new_rect.y,
                                                     old_rect.width,
                                                     old_rect.height),
                                     GEGL_ABYSS_NONE,
                                     buffer,
                                     GEGL_RECTANGLE (0, 0, 0, 0));

              if (GIMP_IS_LAYER_MASK (drawables->data) || GIMP_LAYER (drawables->data)->mask)
                {
                  GeglBuffer   *other_old;
                  GimpDrawable *other_drawable;

                  if (GIMP_IS_LAYER_MASK (drawables->data))
                    other_drawable = GIMP_DRAWABLE ((GIMP_LAYER_MASK (drawables->data))->layer);
                  else
                    other_drawable = GIMP_DRAWABLE (GIMP_LAYER (drawables->data)->mask);

                  /* create a copy of original buffer by taking the required area */
                  other_old = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                               old_rect.width,
                                                               old_rect.height),
                                               gimp_drawable_get_format (other_drawable));

                  gimp_gegl_buffer_copy (gimp_drawable_get_buffer (other_drawable),
                                         GEGL_RECTANGLE (old_rect.x - new_rect.x,
                                                         old_rect.y - new_rect.y,
                                                         old_rect.width,
                                                         old_rect.height),
                                         GEGL_ABYSS_NONE,
                                         other_old,
                                         GEGL_RECTANGLE (0, 0, 0, 0));

                  gimp_drawable_set_buffer_full (other_drawable, FALSE, NULL,
                                                 other_old, &old_rect,
                                                 FALSE);

                  g_object_unref (other_old);
                }
              /* Restore drawable to state before painting started */
              gimp_drawable_set_buffer_full (iter->data, FALSE, NULL,
                                             buffer, &old_rect,
                                             FALSE);

              gimp_drawable_update (iter->data, 0, 0, -1, -1);

              g_object_unref (buffer);
            }

          g_object_unref (undo_buffer);
        }

      gimp_viewable_preview_thaw (GIMP_VIEWABLE (iter->data));
    }

  g_clear_object (&core->saved_proj_buffer);
}

void
gimp_paint_core_cleanup (GimpPaintCore *core)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));

  g_hash_table_remove_all (core->undo_buffers);
  g_hash_table_remove_all (core->original_bounds);

  g_clear_object (&core->saved_proj_buffer);
  g_clear_object (&core->canvas_buffer);
  g_clear_object (&core->paint_buffer);
}

void
gimp_paint_core_interpolate (GimpPaintCore    *core,
                             GList            *drawables,
                             GimpPaintOptions *paint_options,
                             const GimpCoords *coords,
                             guint32           time)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (drawables != NULL);
  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options));
  g_return_if_fail (coords != NULL);

  core->cur_coords = *coords;

  GIMP_PAINT_CORE_GET_CLASS (core)->interpolate (core, drawables,
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


gboolean
gimp_paint_core_expand_drawable (GimpPaintCore    *core,
                                 GimpDrawable     *drawable,
                                 GimpPaintOptions *options,
                                 gint              x1,
                                 gint              x2,
                                 gint              y1,
                                 gint              y2,
                                 gint             *new_off_x,
                                 gint             *new_off_y)
{
  gint          drawable_width, drawable_height;
  gint          drawable_offset_x, drawable_offset_y;
  gint          image_width, image_height;
  gint          new_width, new_height;
  gint          expand_amount;
  gboolean      show_all;
  gboolean      outside_image;
  GimpImage    *image     = gimp_item_get_image (GIMP_ITEM (drawable));
  GimpLayer    *layer;

  drawable_width  = gimp_item_get_width  (GIMP_ITEM (drawable));
  drawable_height = gimp_item_get_height (GIMP_ITEM (drawable));
  gimp_item_get_offset (GIMP_ITEM (drawable), &drawable_offset_x, &drawable_offset_y);

  new_width  = drawable_width;
  new_height = drawable_height;
  *new_off_x = 0;
  *new_off_y = 0;

  image_width  = gimp_image_get_width (image);
  image_height = gimp_image_get_height (image);

  expand_amount = options->expand_amount;
  show_all      = gimp_paint_core_get_show_all (core);
  outside_image = x2 < -drawable_offset_x || x1 > image_width - drawable_offset_x ||
                  y2 < -drawable_offset_y || y1 > image_height - drawable_offset_y;

  /* Don't expand if drawable is anything other
   * than layer or layer mask */
  if (GIMP_IS_LAYER_MASK (drawable))
    layer = (GIMP_LAYER_MASK (drawable))->layer;
  else if (GIMP_IS_LAYER (drawable))
    layer = GIMP_LAYER (drawable);
  else
    return FALSE;

  if (!gimp_paint_core_get_show_all (core) && outside_image)
    return FALSE;

  if (!options->expand_use)
    return FALSE;

  if (x1 < 0)
    {
      if (show_all)
        {
          new_width += expand_amount - x1;
          *new_off_x += expand_amount - x1;
        }
      else if (drawable_offset_x > 0)
        {
          new_width += expand_amount - x1;
          *new_off_x += expand_amount - x1;
          if (*new_off_x > drawable_offset_x)
            {
              new_width -= *new_off_x - drawable_offset_x;
              *new_off_x = drawable_offset_x;
            }
        }
    }
  if (y1 < 0)
    {
      if (show_all)
        {
          new_height += expand_amount - y1;
          *new_off_y += expand_amount - y1;
        }
      else if (drawable_offset_y > 0)
        {
          new_height += expand_amount - y1;
          *new_off_y += expand_amount - y1;
          if (*new_off_y > drawable_offset_y)
            {
              new_height -= *new_off_y - drawable_offset_y;
              *new_off_y = drawable_offset_y;
            }
        }
    }
  if (x2 > drawable_width)
    {
      if (show_all)
        {
          new_width += x2 - drawable_width + expand_amount;
        }
      else if (drawable_width + drawable_offset_x < image_width)
        {
          new_width += x2 - drawable_width + expand_amount;
          if (new_width + drawable_offset_x - *new_off_x > image_width)
            new_width = image_width + *new_off_x - drawable_offset_x;
        }
    }
  if (y2 > drawable_height)
    {
      if (show_all)
        {
          new_height += y2 - drawable_height + expand_amount;
        }
      else if (drawable_height + drawable_offset_y < image_height)
        {
          new_height += y2 - drawable_height + expand_amount;
          if (new_height + drawable_offset_y - *new_off_y > image_height)
            new_height = image_height + *new_off_y - drawable_offset_y;
        }
    }

  if (new_width != drawable_width   || *new_off_x ||
      new_height != drawable_height || *new_off_y)
    {
      GeglColor    *color;
      GimpPattern  *pattern;
      GimpContext  *context   = GIMP_CONTEXT (options);
      GimpFillType  fill_type = options->expand_fill_type;
      gboolean      context_has_image;
      GimpFillType  mask_fill_type;
      GeglBuffer   *undo_buffer;
      GeglBuffer   *new_buffer;

      if (gimp_item_get_lock_position (GIMP_ITEM (layer)))
        {
          if (core->lock_blink_state == GIMP_PAINT_LOCK_NOT_BLINKED)
            core->lock_blink_state = GIMP_PAINT_LOCK_BLINK_PENDING;

          /* Since we are not expanding, set new offset to zero */
          *new_off_x = 0;
          *new_off_y = 0;

          return FALSE;
        }

      mask_fill_type = options->expand_mask_fill_type == GIMP_ADD_MASK_BLACK ?
                         GIMP_FILL_TRANSPARENT :
                         GIMP_FILL_WHITE;

      /* The image field of context is null but
       * is required for Filling with Middle Gray */
      context_has_image = context->image != NULL;
      if (!context_has_image)
        context->image = image;

      /* ensure that every expansion is pushed to undo stack */
      if (core->x2 == core->x1)
        core->x2++;
      if (core->y2 == core->y1)
        core->y2++;

      g_object_freeze_notify (G_OBJECT (layer));

      gimp_drawable_disable_resize_undo (GIMP_DRAWABLE (layer));
      GIMP_LAYER_GET_CLASS (layer)->resize (layer, context, fill_type,
                                            new_width, new_height,
                                            *new_off_x, *new_off_y);
      gimp_drawable_enable_resize_undo (GIMP_DRAWABLE (layer));

      if (layer->mask)
        {
          g_object_freeze_notify (G_OBJECT (layer->mask));

          gimp_drawable_disable_resize_undo (GIMP_DRAWABLE (layer->mask));
          GIMP_ITEM_GET_CLASS (layer->mask)->resize (GIMP_ITEM (layer->mask), context,
                                                     mask_fill_type, new_width, new_height,
                                                     *new_off_x, *new_off_y);
          gimp_drawable_enable_resize_undo (GIMP_DRAWABLE (layer->mask));

          g_object_thaw_notify (G_OBJECT (layer->mask));
        }

      g_object_thaw_notify (G_OBJECT (layer));

      gimp_image_flush (image);

      if (GIMP_IS_LAYER_MASK (drawable))
        {
          fill_type = mask_fill_type;
        }
      else if (fill_type == GIMP_FILL_TRANSPARENT &&
               ! gimp_drawable_has_alpha (drawable))
        {
          fill_type = GIMP_FILL_BACKGROUND;
        }

      new_buffer = gimp_gegl_buffer_resize (core->canvas_buffer, new_width, new_height,
                                            -(*new_off_x), -(*new_off_y), NULL, NULL, 0, 0);
      g_object_unref (core->canvas_buffer);
      core->canvas_buffer = new_buffer;

      gimp_get_fill_params (context, fill_type, &color, &pattern, NULL);
      if (! gimp_drawable_has_alpha (drawable))
        gimp_color_set_alpha (color, 1.0);

      undo_buffer = g_hash_table_lookup (core->undo_buffers, drawable);
      g_object_ref (undo_buffer);

      new_buffer = gimp_gegl_buffer_resize (undo_buffer, new_width, new_height,
                                            -(*new_off_x), -(*new_off_y), color,
                                            pattern, 0, 0);
      g_hash_table_insert (core->undo_buffers, drawable, new_buffer);
      g_object_unref (undo_buffer);
      g_clear_object (&color);

      /* Restore context to its original state */
      if (!context_has_image)
        context->image = NULL;

      return TRUE;
    }
  return FALSE;
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
gimp_paint_core_get_orig_image (GimpPaintCore *core,
                                GimpDrawable  *drawable)
{
  GeglBuffer *undo_buffer;

  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), NULL);

  undo_buffer = g_hash_table_lookup (core->undo_buffers, drawable);
  g_return_val_if_fail (undo_buffer != NULL, NULL);

  return undo_buffer;
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
  gint               width  = gegl_buffer_get_width  (core->paint_buffer);
  gint               height = gegl_buffer_get_height (core->paint_buffer);
  GimpComponentMask  affect = gimp_drawable_get_active_mask (drawable);
  GeglBuffer        *undo_buffer;

  undo_buffer = g_hash_table_lookup (core->undo_buffers, drawable);

  if (! affect)
    return;

  if (core->applicators)
    {
      GimpApplicator *applicator;

      applicator = g_hash_table_lookup (core->applicators, drawable);

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

          gimp_applicator_set_src_buffer (applicator, undo_buffer);
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

          gimp_applicator_set_src_buffer (applicator,
                                          gimp_drawable_get_buffer (drawable));
        }

      gimp_applicator_set_apply_buffer (applicator,
                                        core->paint_buffer);
      gimp_applicator_set_apply_offset (applicator,
                                        core->paint_buffer_x,
                                        core->paint_buffer_y);

      gimp_applicator_set_opacity (applicator, image_opacity);
      gimp_applicator_set_mode (applicator, paint_mode,
                                GIMP_LAYER_COLOR_SPACE_AUTO,
                                GIMP_LAYER_COLOR_SPACE_AUTO,
                                gimp_layer_mode_get_paint_composite_mode (paint_mode));

      /*  apply the paint area to the image  */
      gimp_applicator_blit (applicator,
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

          algorithms |= GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK;

          /* dest_buffer -> paint_buf -> dest_buffer */
          params.src_buffer = params.dest_buffer;
        }

      gimp_item_get_offset (GIMP_ITEM (drawable),
                            &params.mask_offset_x, &params.mask_offset_y);
      params.mask_offset_x = -params.mask_offset_x;
      params.mask_offset_y = -params.mask_offset_y;
      params.mask_buffer   = core->mask_buffer;
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
  GeglBuffer        *undo_buffer;
  gint               width, height;
  GimpComponentMask  affect;

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

  undo_buffer = g_hash_table_lookup (core->undo_buffers, drawable);

  if (core->applicators)
    {
      GimpApplicator *applicator;
      GeglRectangle   mask_rect;
      GeglBuffer     *mask_buffer;
      gint            offset_x;
      gint            offset_y;

      applicator = g_hash_table_lookup (core->applicators, drawable);

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

          gimp_applicator_set_src_buffer (applicator, undo_buffer);
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

          gimp_applicator_set_src_buffer (applicator,
                                          gimp_drawable_get_buffer (drawable));
        }

      gimp_item_get_offset (GIMP_ITEM (drawable), &offset_x, &offset_y);
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
            GEGL_RECTANGLE (aligned_combined_mask_rect.x + offset_x,
                            aligned_combined_mask_rect.y + offset_y,
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

      gimp_applicator_set_mask_buffer (applicator, mask_buffer);
      gimp_applicator_set_mask_offset (applicator,
                                       core->paint_buffer_x - mask_rect.x,
                                       core->paint_buffer_y - mask_rect.y);

      gimp_applicator_set_apply_buffer (applicator,
                                        core->paint_buffer);
      gimp_applicator_set_apply_offset (applicator,
                                        core->paint_buffer_x,
                                        core->paint_buffer_y);

      gimp_applicator_set_opacity (applicator, image_opacity);
      gimp_applicator_set_mode (applicator, GIMP_LAYER_MODE_REPLACE,
                                GIMP_LAYER_COLOR_SPACE_AUTO,
                                GIMP_LAYER_COLOR_SPACE_AUTO,
                                gimp_layer_mode_get_paint_composite_mode (
                                  GIMP_LAYER_MODE_REPLACE));

      /*  apply the paint area to the image  */
      gimp_applicator_blit (applicator,
                            GEGL_RECTANGLE (core->paint_buffer_x,
                                            core->paint_buffer_y,
                                            width, height));

      gimp_applicator_set_mask_buffer (applicator, core->mask_buffer);
      gimp_applicator_set_mask_offset (applicator, -offset_x, -offset_y);

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
