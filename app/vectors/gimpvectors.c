/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmavectors.c
 * Copyright (C) 2002 Simon Budig  <simon@ligma.org>
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"

#include "vectors-types.h"

#include "core/ligma.h"
#include "core/ligma-transform-utils.h"
#include "core/ligmabezierdesc.h"
#include "core/ligmachannel-select.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmadrawable-fill.h"
#include "core/ligmadrawable-stroke.h"
#include "core/ligmaerror.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-undo-push.h"
#include "core/ligmapaintinfo.h"
#include "core/ligmastrokeoptions.h"

#include "paint/ligmapaintcore-stroke.h"
#include "paint/ligmapaintoptions.h"

#include "ligmaanchor.h"
#include "ligmastroke.h"
#include "ligmavectors.h"
#include "ligmavectors-preview.h"

#include "ligma-intl.h"


enum
{
  FREEZE,
  THAW,
  LAST_SIGNAL
};


static void       ligma_vectors_finalize      (GObject           *object);

static gint64     ligma_vectors_get_memsize   (LigmaObject        *object,
                                              gint64            *gui_size);

static gboolean   ligma_vectors_is_attached   (LigmaItem          *item);
static LigmaItemTree * ligma_vectors_get_tree  (LigmaItem          *item);
static gboolean   ligma_vectors_bounds        (LigmaItem          *item,
                                              gdouble           *x,
                                              gdouble           *y,
                                              gdouble           *width,
                                              gdouble           *height);
static LigmaItem * ligma_vectors_duplicate     (LigmaItem          *item,
                                              GType              new_type);
static void       ligma_vectors_convert       (LigmaItem          *item,
                                              LigmaImage         *dest_image,
                                              GType              old_type);
static void       ligma_vectors_translate     (LigmaItem          *item,
                                              gdouble            offset_x,
                                              gdouble            offset_y,
                                              gboolean           push_undo);
static void       ligma_vectors_scale         (LigmaItem          *item,
                                              gint               new_width,
                                              gint               new_height,
                                              gint               new_offset_x,
                                              gint               new_offset_y,
                                              LigmaInterpolationType  interp_type,
                                              LigmaProgress      *progress);
static void       ligma_vectors_resize        (LigmaItem          *item,
                                              LigmaContext       *context,
                                              LigmaFillType       fill_type,
                                              gint               new_width,
                                              gint               new_height,
                                              gint               offset_x,
                                              gint               offset_y);
static void       ligma_vectors_flip          (LigmaItem          *item,
                                              LigmaContext       *context,
                                              LigmaOrientationType  flip_type,
                                              gdouble            axis,
                                              gboolean           clip_result);
static void       ligma_vectors_rotate        (LigmaItem          *item,
                                              LigmaContext       *context,
                                              LigmaRotationType   rotate_type,
                                              gdouble            center_x,
                                              gdouble            center_y,
                                              gboolean           clip_result);
static void       ligma_vectors_transform     (LigmaItem          *item,
                                              LigmaContext       *context,
                                              const LigmaMatrix3 *matrix,
                                              LigmaTransformDirection direction,
                                              LigmaInterpolationType interp_type,
                                              LigmaTransformResize   clip_result,
                                              LigmaProgress      *progress);
static LigmaTransformResize
                  ligma_vectors_get_clip      (LigmaItem          *item,
                                              LigmaTransformResize clip_result);
static gboolean   ligma_vectors_fill          (LigmaItem          *item,
                                              LigmaDrawable      *drawable,
                                              LigmaFillOptions   *fill_options,
                                              gboolean           push_undo,
                                              LigmaProgress      *progress,
                                              GError           **error);
static gboolean   ligma_vectors_stroke        (LigmaItem          *item,
                                              LigmaDrawable      *drawable,
                                              LigmaStrokeOptions *stroke_options,
                                              gboolean           push_undo,
                                              LigmaProgress      *progress,
                                              GError           **error);
static void       ligma_vectors_to_selection  (LigmaItem          *item,
                                              LigmaChannelOps     op,
                                              gboolean           antialias,
                                              gboolean           feather,
                                              gdouble            feather_radius_x,
                                              gdouble            feather_radius_y);

static void       ligma_vectors_real_freeze          (LigmaVectors       *vectors);
static void       ligma_vectors_real_thaw            (LigmaVectors       *vectors);
static void       ligma_vectors_real_stroke_add      (LigmaVectors       *vectors,
                                                     LigmaStroke        *stroke);
static void       ligma_vectors_real_stroke_remove   (LigmaVectors       *vectors,
                                                     LigmaStroke        *stroke);
static LigmaStroke * ligma_vectors_real_stroke_get    (LigmaVectors       *vectors,
                                                     const LigmaCoords  *coord);
static LigmaStroke *ligma_vectors_real_stroke_get_next(LigmaVectors       *vectors,
                                                     LigmaStroke        *prev);
static gdouble ligma_vectors_real_stroke_get_length  (LigmaVectors       *vectors,
                                                     LigmaStroke        *prev);
static LigmaAnchor * ligma_vectors_real_anchor_get    (LigmaVectors       *vectors,
                                                     const LigmaCoords  *coord,
                                                     LigmaStroke       **ret_stroke);
static void       ligma_vectors_real_anchor_delete   (LigmaVectors       *vectors,
                                                     LigmaAnchor        *anchor);
static gdouble    ligma_vectors_real_get_length      (LigmaVectors       *vectors,
                                                     const LigmaAnchor  *start);
static gdouble    ligma_vectors_real_get_distance    (LigmaVectors       *vectors,
                                                     const LigmaCoords  *coord);
static gint       ligma_vectors_real_interpolate     (LigmaVectors       *vectors,
                                                     LigmaStroke        *stroke,
                                                     gdouble            precision,
                                                     gint               max_points,
                                                     LigmaCoords        *ret_coords);

static LigmaBezierDesc * ligma_vectors_make_bezier      (LigmaVectors     *vectors);
static LigmaBezierDesc * ligma_vectors_real_make_bezier (LigmaVectors     *vectors);


G_DEFINE_TYPE (LigmaVectors, ligma_vectors, LIGMA_TYPE_ITEM)

#define parent_class ligma_vectors_parent_class

static guint ligma_vectors_signals[LAST_SIGNAL] = { 0 };


static void
ligma_vectors_class_init (LigmaVectorsClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);
  LigmaItemClass     *item_class        = LIGMA_ITEM_CLASS (klass);

  ligma_vectors_signals[FREEZE] =
    g_signal_new ("freeze",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaVectorsClass, freeze),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_vectors_signals[THAW] =
    g_signal_new ("thaw",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaVectorsClass, thaw),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize            = ligma_vectors_finalize;

  ligma_object_class->get_memsize    = ligma_vectors_get_memsize;

  viewable_class->get_new_preview   = ligma_vectors_get_new_preview;
  viewable_class->default_icon_name = "ligma-path";

  item_class->is_attached           = ligma_vectors_is_attached;
  item_class->get_tree              = ligma_vectors_get_tree;
  item_class->bounds                = ligma_vectors_bounds;
  item_class->duplicate             = ligma_vectors_duplicate;
  item_class->convert               = ligma_vectors_convert;
  item_class->translate             = ligma_vectors_translate;
  item_class->scale                 = ligma_vectors_scale;
  item_class->resize                = ligma_vectors_resize;
  item_class->flip                  = ligma_vectors_flip;
  item_class->rotate                = ligma_vectors_rotate;
  item_class->transform             = ligma_vectors_transform;
  item_class->get_clip              = ligma_vectors_get_clip;
  item_class->fill                  = ligma_vectors_fill;
  item_class->stroke                = ligma_vectors_stroke;
  item_class->to_selection          = ligma_vectors_to_selection;
  item_class->default_name          = _("Path");
  item_class->rename_desc           = C_("undo-type", "Rename Path");
  item_class->translate_desc        = C_("undo-type", "Move Path");
  item_class->scale_desc            = C_("undo-type", "Scale Path");
  item_class->resize_desc           = C_("undo-type", "Resize Path");
  item_class->flip_desc             = C_("undo-type", "Flip Path");
  item_class->rotate_desc           = C_("undo-type", "Rotate Path");
  item_class->transform_desc        = C_("undo-type", "Transform Path");
  item_class->fill_desc             = C_("undo-type", "Fill Path");
  item_class->stroke_desc           = C_("undo-type", "Stroke Path");
  item_class->to_selection_desc     = C_("undo-type", "Path to Selection");
  item_class->reorder_desc          = C_("undo-type", "Reorder Path");
  item_class->raise_desc            = C_("undo-type", "Raise Path");
  item_class->raise_to_top_desc     = C_("undo-type", "Raise Path to Top");
  item_class->lower_desc            = C_("undo-type", "Lower Path");
  item_class->lower_to_bottom_desc  = C_("undo-type", "Lower Path to Bottom");
  item_class->raise_failed          = _("Path cannot be raised higher.");
  item_class->lower_failed          = _("Path cannot be lowered more.");

  klass->freeze                     = ligma_vectors_real_freeze;
  klass->thaw                       = ligma_vectors_real_thaw;

  klass->stroke_add                 = ligma_vectors_real_stroke_add;
  klass->stroke_remove              = ligma_vectors_real_stroke_remove;
  klass->stroke_get                 = ligma_vectors_real_stroke_get;
  klass->stroke_get_next            = ligma_vectors_real_stroke_get_next;
  klass->stroke_get_length          = ligma_vectors_real_stroke_get_length;

  klass->anchor_get                 = ligma_vectors_real_anchor_get;
  klass->anchor_delete              = ligma_vectors_real_anchor_delete;

  klass->get_length                 = ligma_vectors_real_get_length;
  klass->get_distance               = ligma_vectors_real_get_distance;
  klass->interpolate                = ligma_vectors_real_interpolate;

  klass->make_bezier                = ligma_vectors_real_make_bezier;
}

static void
ligma_vectors_init (LigmaVectors *vectors)
{
  ligma_item_set_visible (LIGMA_ITEM (vectors), FALSE, FALSE);

  vectors->strokes        = g_queue_new ();
  vectors->stroke_to_list = g_hash_table_new (g_direct_hash, g_direct_equal);
  vectors->last_stroke_id = 0;
  vectors->freeze_count   = 0;
  vectors->precision      = 0.2;

  vectors->bezier_desc    = NULL;
  vectors->bounds_valid   = FALSE;
}

static void
ligma_vectors_finalize (GObject *object)
{
  LigmaVectors *vectors = LIGMA_VECTORS (object);

  if (vectors->bezier_desc)
    {
      ligma_bezier_desc_free (vectors->bezier_desc);
      vectors->bezier_desc = NULL;
    }

  if (vectors->strokes)
    {
      g_queue_free_full (vectors->strokes, (GDestroyNotify) g_object_unref);
      vectors->strokes = NULL;
    }

  if (vectors->stroke_to_list)
    {
      g_hash_table_destroy (vectors->stroke_to_list);
      vectors->stroke_to_list = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
ligma_vectors_get_memsize (LigmaObject *object,
                          gint64     *gui_size)
{
  LigmaVectors *vectors;
  GList       *list;
  gint64       memsize = 0;

  vectors = LIGMA_VECTORS (object);

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    memsize += (ligma_object_get_memsize (LIGMA_OBJECT (list->data), gui_size) +
                sizeof (GList));

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
ligma_vectors_is_attached (LigmaItem *item)
{
  LigmaImage *image = ligma_item_get_image (item);

  return (LIGMA_IS_IMAGE (image) &&
          ligma_container_have (ligma_image_get_vectors (image),
                               LIGMA_OBJECT (item)));
}

static LigmaItemTree *
ligma_vectors_get_tree (LigmaItem *item)
{
  if (ligma_item_is_attached (item))
    {
      LigmaImage *image = ligma_item_get_image (item);

      return ligma_image_get_vectors_tree (image);
    }

  return NULL;
}

static gboolean
ligma_vectors_bounds (LigmaItem *item,
                     gdouble  *x,
                     gdouble  *y,
                     gdouble  *width,
                     gdouble  *height)
{
  LigmaVectors *vectors = LIGMA_VECTORS (item);

  if (! vectors->bounds_valid)
    {
      LigmaStroke *stroke;

      vectors->bounds_empty = TRUE;
      vectors->bounds_x1 = vectors->bounds_x2 = 0.0;
      vectors->bounds_y1 = vectors->bounds_y2 = 0.0;

      for (stroke = ligma_vectors_stroke_get_next (vectors, NULL);
           stroke;
           stroke = ligma_vectors_stroke_get_next (vectors, stroke))
        {
          GArray   *stroke_coords;
          gboolean  closed;

          stroke_coords = ligma_stroke_interpolate (stroke, 1.0, &closed);

          if (stroke_coords)
            {
              LigmaCoords point;
              gint       i;

              if (vectors->bounds_empty && stroke_coords->len > 0)
                {
                  point = g_array_index (stroke_coords, LigmaCoords, 0);

                  vectors->bounds_x1 = vectors->bounds_x2 = point.x;
                  vectors->bounds_y1 = vectors->bounds_y2 = point.y;

                  vectors->bounds_empty = FALSE;
                }

              for (i = 0; i < stroke_coords->len; i++)
                {
                  point = g_array_index (stroke_coords, LigmaCoords, i);

                  vectors->bounds_x1 = MIN (vectors->bounds_x1, point.x);
                  vectors->bounds_y1 = MIN (vectors->bounds_y1, point.y);
                  vectors->bounds_x2 = MAX (vectors->bounds_x2, point.x);
                  vectors->bounds_y2 = MAX (vectors->bounds_y2, point.y);
                }

              g_array_free (stroke_coords, TRUE);
            }
        }

      vectors->bounds_valid = TRUE;
    }

  *x      = vectors->bounds_x1;
  *y      = vectors->bounds_y1;
  *width  = vectors->bounds_x2 - vectors->bounds_x1;
  *height = vectors->bounds_y2 - vectors->bounds_y1;

  return ! vectors->bounds_empty;
}

static LigmaItem *
ligma_vectors_duplicate (LigmaItem *item,
                        GType     new_type)
{
  LigmaItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, LIGMA_TYPE_VECTORS), NULL);

  new_item = LIGMA_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (LIGMA_IS_VECTORS (new_item))
    {
      LigmaVectors *vectors     = LIGMA_VECTORS (item);
      LigmaVectors *new_vectors = LIGMA_VECTORS (new_item);

      ligma_vectors_copy_strokes (vectors, new_vectors);
    }

  return new_item;
}

static void
ligma_vectors_convert (LigmaItem  *item,
                      LigmaImage *dest_image,
                      GType      old_type)
{
  ligma_item_set_size (item,
                      ligma_image_get_width  (dest_image),
                      ligma_image_get_height (dest_image));

  LIGMA_ITEM_CLASS (parent_class)->convert (item, dest_image, old_type);
}

static void
ligma_vectors_translate (LigmaItem *item,
                        gdouble   offset_x,
                        gdouble   offset_y,
                        gboolean  push_undo)
{
  LigmaVectors *vectors = LIGMA_VECTORS (item);
  GList       *list;

  ligma_vectors_freeze (vectors);

  if (push_undo)
    ligma_image_undo_push_vectors_mod (ligma_item_get_image (item),
                                      _("Move Path"),
                                      vectors);

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      LigmaStroke *stroke = list->data;

      ligma_stroke_translate (stroke, offset_x, offset_y);
    }

  ligma_vectors_thaw (vectors);
}

static void
ligma_vectors_scale (LigmaItem              *item,
                    gint                   new_width,
                    gint                   new_height,
                    gint                   new_offset_x,
                    gint                   new_offset_y,
                    LigmaInterpolationType  interpolation_type,
                    LigmaProgress          *progress)
{
  LigmaVectors *vectors = LIGMA_VECTORS (item);
  LigmaImage   *image   = ligma_item_get_image (item);
  GList       *list;

  ligma_vectors_freeze (vectors);

  if (ligma_item_is_attached (item))
    ligma_image_undo_push_vectors_mod (image, NULL, vectors);

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      LigmaStroke *stroke = list->data;

      ligma_stroke_scale (stroke,
                         (gdouble) new_width  / (gdouble) ligma_item_get_width  (item),
                         (gdouble) new_height / (gdouble) ligma_item_get_height (item));
      ligma_stroke_translate (stroke, new_offset_x, new_offset_y);
    }

  LIGMA_ITEM_CLASS (parent_class)->scale (item,
                                         ligma_image_get_width  (image),
                                         ligma_image_get_height (image),
                                         0, 0,
                                         interpolation_type, progress);

  ligma_vectors_thaw (vectors);
}

static void
ligma_vectors_resize (LigmaItem     *item,
                     LigmaContext  *context,
                     LigmaFillType  fill_type,
                     gint          new_width,
                     gint          new_height,
                     gint          offset_x,
                     gint          offset_y)
{
  LigmaVectors *vectors = LIGMA_VECTORS (item);
  LigmaImage   *image   = ligma_item_get_image (item);
  GList       *list;

  ligma_vectors_freeze (vectors);

  if (ligma_item_is_attached (item))
    ligma_image_undo_push_vectors_mod (image, NULL, vectors);

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      LigmaStroke *stroke = list->data;

      ligma_stroke_translate (stroke, offset_x, offset_y);
    }

  LIGMA_ITEM_CLASS (parent_class)->resize (item, context, fill_type,
                                          ligma_image_get_width  (image),
                                          ligma_image_get_height (image),
                                          0, 0);

  ligma_vectors_thaw (vectors);
}

static void
ligma_vectors_flip (LigmaItem            *item,
                   LigmaContext         *context,
                   LigmaOrientationType  flip_type,
                   gdouble              axis,
                   gboolean             clip_result)
{
  LigmaVectors *vectors = LIGMA_VECTORS (item);
  GList       *list;
  LigmaMatrix3  matrix;

  ligma_matrix3_identity (&matrix);
  ligma_transform_matrix_flip (&matrix, flip_type, axis);

  ligma_vectors_freeze (vectors);

  ligma_image_undo_push_vectors_mod (ligma_item_get_image (item),
                                    _("Flip Path"),
                                    vectors);

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      LigmaStroke *stroke = list->data;

      ligma_stroke_transform (stroke, &matrix, NULL);
    }

  ligma_vectors_thaw (vectors);
}

static void
ligma_vectors_rotate (LigmaItem         *item,
                     LigmaContext      *context,
                     LigmaRotationType  rotate_type,
                     gdouble           center_x,
                     gdouble           center_y,
                     gboolean          clip_result)
{
  LigmaVectors *vectors = LIGMA_VECTORS (item);
  GList       *list;
  LigmaMatrix3  matrix;

  ligma_matrix3_identity (&matrix);
  ligma_transform_matrix_rotate (&matrix, rotate_type, center_x, center_y);

  ligma_vectors_freeze (vectors);

  ligma_image_undo_push_vectors_mod (ligma_item_get_image (item),
                                    _("Rotate Path"),
                                    vectors);

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      LigmaStroke *stroke = list->data;

      ligma_stroke_transform (stroke, &matrix, NULL);
    }

  ligma_vectors_thaw (vectors);
}

static void
ligma_vectors_transform (LigmaItem               *item,
                        LigmaContext            *context,
                        const LigmaMatrix3      *matrix,
                        LigmaTransformDirection  direction,
                        LigmaInterpolationType   interpolation_type,
                        LigmaTransformResize     clip_result,
                        LigmaProgress           *progress)
{
  LigmaVectors *vectors = LIGMA_VECTORS (item);
  LigmaMatrix3  local_matrix;
  GQueue       strokes;
  GList       *list;

  ligma_vectors_freeze (vectors);

  ligma_image_undo_push_vectors_mod (ligma_item_get_image (item),
                                    _("Transform Path"),
                                    vectors);

  local_matrix = *matrix;

  if (direction == LIGMA_TRANSFORM_BACKWARD)
    ligma_matrix3_invert (&local_matrix);

  g_queue_init (&strokes);

  while (! g_queue_is_empty (vectors->strokes))
    {
      LigmaStroke *stroke = g_queue_peek_head (vectors->strokes);

      g_object_ref (stroke);

      ligma_vectors_stroke_remove (vectors, stroke);

      ligma_stroke_transform (stroke, &local_matrix, &strokes);

      g_object_unref (stroke);
    }

  vectors->last_stroke_id = 0;

  for (list = strokes.head; list; list = g_list_next (list))
    {
      LigmaStroke *stroke = list->data;

      ligma_vectors_stroke_add (vectors, stroke);

      g_object_unref (stroke);
    }

  g_queue_clear (&strokes);

  ligma_vectors_thaw (vectors);
}

static LigmaTransformResize
ligma_vectors_get_clip (LigmaItem            *item,
                       LigmaTransformResize  clip_result)
{
  return LIGMA_TRANSFORM_RESIZE_ADJUST;
}

static gboolean
ligma_vectors_fill (LigmaItem         *item,
                   LigmaDrawable     *drawable,
                   LigmaFillOptions  *fill_options,
                   gboolean          push_undo,
                   LigmaProgress     *progress,
                   GError          **error)
{
  LigmaVectors *vectors = LIGMA_VECTORS (item);

  if (g_queue_is_empty (vectors->strokes))
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Not enough points to fill"));
      return FALSE;
    }

  return ligma_drawable_fill_vectors (drawable, fill_options,
                                     vectors, push_undo, error);
}

static gboolean
ligma_vectors_stroke (LigmaItem           *item,
                     LigmaDrawable       *drawable,
                     LigmaStrokeOptions  *stroke_options,
                     gboolean            push_undo,
                     LigmaProgress       *progress,
                     GError            **error)
{
  LigmaVectors *vectors = LIGMA_VECTORS (item);
  gboolean     retval  = FALSE;

  if (g_queue_is_empty (vectors->strokes))
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Not enough points to stroke"));
      return FALSE;
    }

  switch (ligma_stroke_options_get_method (stroke_options))
    {
    case LIGMA_STROKE_LINE:
      retval = ligma_drawable_stroke_vectors (drawable, stroke_options,
                                             vectors, push_undo, error);
      break;

    case LIGMA_STROKE_PAINT_METHOD:
      {
        LigmaPaintInfo    *paint_info;
        LigmaPaintCore    *core;
        LigmaPaintOptions *paint_options;
        gboolean          emulate_dynamics;

        paint_info = ligma_context_get_paint_info (LIGMA_CONTEXT (stroke_options));

        core = g_object_new (paint_info->paint_type, NULL);

        paint_options = ligma_stroke_options_get_paint_options (stroke_options);
        emulate_dynamics = ligma_stroke_options_get_emulate_dynamics (stroke_options);

        retval = ligma_paint_core_stroke_vectors (core, drawable,
                                                 paint_options,
                                                 emulate_dynamics,
                                                 vectors, push_undo, error);

        g_object_unref (core);
      }
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  return retval;
}

static void
ligma_vectors_to_selection (LigmaItem       *item,
                           LigmaChannelOps  op,
                           gboolean        antialias,
                           gboolean        feather,
                           gdouble         feather_radius_x,
                           gdouble         feather_radius_y)
{
  LigmaVectors *vectors = LIGMA_VECTORS (item);
  LigmaImage   *image   = ligma_item_get_image (item);

  ligma_channel_select_vectors (ligma_image_get_mask (image),
                               LIGMA_ITEM_GET_CLASS (item)->to_selection_desc,
                               vectors,
                               op, antialias,
                               feather, feather_radius_x, feather_radius_x,
                               TRUE);
}

static void
ligma_vectors_real_freeze (LigmaVectors *vectors)
{
  /*  release cached bezier representation  */
  if (vectors->bezier_desc)
    {
      ligma_bezier_desc_free (vectors->bezier_desc);
      vectors->bezier_desc = NULL;
    }

  /*  invalidate bounds  */
  vectors->bounds_valid = FALSE;
}

static void
ligma_vectors_real_thaw (LigmaVectors *vectors)
{
  ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (vectors));
}


/*  public functions  */

LigmaVectors *
ligma_vectors_new (LigmaImage   *image,
                  const gchar *name)
{
  LigmaVectors *vectors;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  vectors = LIGMA_VECTORS (ligma_item_new (LIGMA_TYPE_VECTORS,
                                         image, name,
                                         0, 0,
                                         ligma_image_get_width  (image),
                                         ligma_image_get_height (image)));

  return vectors;
}

LigmaVectors *
ligma_vectors_get_parent (LigmaVectors *vectors)
{
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), NULL);

  return LIGMA_VECTORS (ligma_viewable_get_parent (LIGMA_VIEWABLE (vectors)));
}

void
ligma_vectors_freeze (LigmaVectors *vectors)
{
  g_return_if_fail (LIGMA_IS_VECTORS (vectors));

  vectors->freeze_count++;

  if (vectors->freeze_count == 1)
    g_signal_emit (vectors, ligma_vectors_signals[FREEZE], 0);
}

void
ligma_vectors_thaw (LigmaVectors *vectors)
{
  g_return_if_fail (LIGMA_IS_VECTORS (vectors));
  g_return_if_fail (vectors->freeze_count > 0);

  vectors->freeze_count--;

  if (vectors->freeze_count == 0)
    g_signal_emit (vectors, ligma_vectors_signals[THAW], 0);
}

void
ligma_vectors_copy_strokes (LigmaVectors *src_vectors,
                           LigmaVectors *dest_vectors)
{
  g_return_if_fail (LIGMA_IS_VECTORS (src_vectors));
  g_return_if_fail (LIGMA_IS_VECTORS (dest_vectors));

  ligma_vectors_freeze (dest_vectors);

  g_queue_free_full (dest_vectors->strokes, (GDestroyNotify) g_object_unref);
  dest_vectors->strokes = g_queue_new ();
  g_hash_table_remove_all (dest_vectors->stroke_to_list);

  dest_vectors->last_stroke_id = 0;

  ligma_vectors_add_strokes (src_vectors, dest_vectors);

  ligma_vectors_thaw (dest_vectors);
}


void
ligma_vectors_add_strokes (LigmaVectors *src_vectors,
                          LigmaVectors *dest_vectors)
{
  GList *stroke;

  g_return_if_fail (LIGMA_IS_VECTORS (src_vectors));
  g_return_if_fail (LIGMA_IS_VECTORS (dest_vectors));

  ligma_vectors_freeze (dest_vectors);

  for (stroke = src_vectors->strokes->head;
       stroke != NULL;
       stroke = g_list_next (stroke))
    {
      LigmaStroke *newstroke = ligma_stroke_duplicate (stroke->data);

      g_queue_push_tail (dest_vectors->strokes, newstroke);

      /* Also add to {stroke: GList node} map */
      g_hash_table_insert (dest_vectors->stroke_to_list,
                           newstroke,
                           g_queue_peek_tail_link (dest_vectors->strokes));

      dest_vectors->last_stroke_id++;
      ligma_stroke_set_id (newstroke,
                          dest_vectors->last_stroke_id);
    }

  ligma_vectors_thaw (dest_vectors);
}


void
ligma_vectors_stroke_add (LigmaVectors *vectors,
                         LigmaStroke  *stroke)
{
  g_return_if_fail (LIGMA_IS_VECTORS (vectors));
  g_return_if_fail (LIGMA_IS_STROKE (stroke));

  ligma_vectors_freeze (vectors);

  LIGMA_VECTORS_GET_CLASS (vectors)->stroke_add (vectors, stroke);

  ligma_vectors_thaw (vectors);
}

static void
ligma_vectors_real_stroke_add (LigmaVectors *vectors,
                              LigmaStroke  *stroke)
{
  /*
   * Don't prepend into vector->strokes.  See ChangeLog 2003-05-21
   * --Mitch
   */
  g_queue_push_tail (vectors->strokes, g_object_ref (stroke));

  /* Also add to {stroke: GList node} map */
  g_hash_table_insert (vectors->stroke_to_list,
                       stroke,
                       g_queue_peek_tail_link (vectors->strokes));

  vectors->last_stroke_id++;
  ligma_stroke_set_id (stroke, vectors->last_stroke_id);
}

void
ligma_vectors_stroke_remove (LigmaVectors *vectors,
                            LigmaStroke  *stroke)
{
  g_return_if_fail (LIGMA_IS_VECTORS (vectors));
  g_return_if_fail (LIGMA_IS_STROKE (stroke));

  ligma_vectors_freeze (vectors);

  LIGMA_VECTORS_GET_CLASS (vectors)->stroke_remove (vectors, stroke);

  ligma_vectors_thaw (vectors);
}

static void
ligma_vectors_real_stroke_remove (LigmaVectors *vectors,
                                 LigmaStroke  *stroke)
{
  GList *list = g_hash_table_lookup (vectors->stroke_to_list, stroke);

  if (list)
    {
      g_queue_delete_link (vectors->strokes, list);
      g_hash_table_remove (vectors->stroke_to_list, stroke);
      g_object_unref (stroke);
    }
}

gint
ligma_vectors_get_n_strokes (LigmaVectors *vectors)
{
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), 0);

  return g_queue_get_length (vectors->strokes);
}


LigmaStroke *
ligma_vectors_stroke_get (LigmaVectors      *vectors,
                         const LigmaCoords *coord)
{
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), NULL);

  return LIGMA_VECTORS_GET_CLASS (vectors)->stroke_get (vectors, coord);
}

static LigmaStroke *
ligma_vectors_real_stroke_get (LigmaVectors      *vectors,
                              const LigmaCoords *coord)
{
  LigmaStroke *minstroke = NULL;
  gdouble     mindist   = G_MAXDOUBLE;
  GList      *list;

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      LigmaStroke *stroke = list->data;
      LigmaAnchor *anchor = ligma_stroke_anchor_get (stroke, coord);

      if (anchor)
        {
          gdouble dx = coord->x - anchor->position.x;
          gdouble dy = coord->y - anchor->position.y;

          if (mindist > dx * dx + dy * dy)
            {
              mindist   = dx * dx + dy * dy;
              minstroke = stroke;
            }
        }
    }

  return minstroke;
}

LigmaStroke *
ligma_vectors_stroke_get_by_id (LigmaVectors *vectors,
                               gint         id)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), NULL);

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      if (ligma_stroke_get_id (list->data) == id)
        return list->data;
    }

  return NULL;
}


LigmaStroke *
ligma_vectors_stroke_get_next (LigmaVectors *vectors,
                              LigmaStroke  *prev)
{
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), NULL);

  return LIGMA_VECTORS_GET_CLASS (vectors)->stroke_get_next (vectors, prev);
}

static LigmaStroke *
ligma_vectors_real_stroke_get_next (LigmaVectors *vectors,
                                   LigmaStroke  *prev)
{
  if (! prev)
    {
      return g_queue_peek_head (vectors->strokes);
    }
  else
    {
      GList *stroke = g_hash_table_lookup (vectors->stroke_to_list, prev);

      g_return_val_if_fail (stroke != NULL, NULL);

      return stroke->next ? stroke->next->data : NULL;
    }
}


gdouble
ligma_vectors_stroke_get_length (LigmaVectors *vectors,
                                LigmaStroke  *stroke)
{
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), 0.0);
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), 0.0);

  return LIGMA_VECTORS_GET_CLASS (vectors)->stroke_get_length (vectors, stroke);
}

static gdouble
ligma_vectors_real_stroke_get_length (LigmaVectors *vectors,
                                     LigmaStroke  *stroke)
{
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), 0.0);
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), 0.0);

  return ligma_stroke_get_length (stroke, vectors->precision);
}


LigmaAnchor *
ligma_vectors_anchor_get (LigmaVectors       *vectors,
                         const LigmaCoords  *coord,
                         LigmaStroke       **ret_stroke)
{
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), NULL);

  return LIGMA_VECTORS_GET_CLASS (vectors)->anchor_get (vectors, coord,
                                                       ret_stroke);
}

static LigmaAnchor *
ligma_vectors_real_anchor_get (LigmaVectors       *vectors,
                              const LigmaCoords  *coord,
                              LigmaStroke       **ret_stroke)
{
  LigmaAnchor *minanchor = NULL;
  gdouble     mindist   = -1;
  GList      *list;

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      LigmaStroke *stroke = list->data;
      LigmaAnchor *anchor = ligma_stroke_anchor_get (stroke, coord);

      if (anchor)
        {
          gdouble dx = coord->x - anchor->position.x;
          gdouble dy = coord->y - anchor->position.y;

          if (mindist > dx * dx + dy * dy || mindist < 0)
            {
              mindist   = dx * dx + dy * dy;
              minanchor = anchor;

              if (ret_stroke)
                *ret_stroke = stroke;
            }
        }
    }

  return minanchor;
}


void
ligma_vectors_anchor_delete (LigmaVectors *vectors,
                            LigmaAnchor  *anchor)
{
  g_return_if_fail (LIGMA_IS_VECTORS (vectors));
  g_return_if_fail (anchor != NULL);

  LIGMA_VECTORS_GET_CLASS (vectors)->anchor_delete (vectors, anchor);
}

static void
ligma_vectors_real_anchor_delete (LigmaVectors *vectors,
                                 LigmaAnchor  *anchor)
{
}


void
ligma_vectors_anchor_select (LigmaVectors *vectors,
                            LigmaStroke  *target_stroke,
                            LigmaAnchor  *anchor,
                            gboolean     selected,
                            gboolean     exclusive)
{
  GList *list;

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      LigmaStroke *stroke = list->data;

      ligma_stroke_anchor_select (stroke,
                                 stroke == target_stroke ? anchor : NULL,
                                 selected, exclusive);
    }
}


gdouble
ligma_vectors_get_length (LigmaVectors      *vectors,
                         const LigmaAnchor *start)
{
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), 0.0);

  return LIGMA_VECTORS_GET_CLASS (vectors)->get_length (vectors, start);
}

static gdouble
ligma_vectors_real_get_length (LigmaVectors      *vectors,
                              const LigmaAnchor *start)
{
  g_printerr ("ligma_vectors_get_length: default implementation\n");

  return 0;
}


gdouble
ligma_vectors_get_distance (LigmaVectors      *vectors,
                           const LigmaCoords *coord)
{
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), 0.0);

  return LIGMA_VECTORS_GET_CLASS (vectors)->get_distance (vectors, coord);
}

static gdouble
ligma_vectors_real_get_distance (LigmaVectors      *vectors,
                                const LigmaCoords *coord)
{
  g_printerr ("ligma_vectors_get_distance: default implementation\n");

  return 0;
}

gint
ligma_vectors_interpolate (LigmaVectors *vectors,
                          LigmaStroke  *stroke,
                          gdouble      precision,
                          gint         max_points,
                          LigmaCoords  *ret_coords)
{
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), 0);

  return LIGMA_VECTORS_GET_CLASS (vectors)->interpolate (vectors, stroke,
                                                        precision, max_points,
                                                        ret_coords);
}

static gint
ligma_vectors_real_interpolate (LigmaVectors *vectors,
                               LigmaStroke  *stroke,
                               gdouble      precision,
                               gint         max_points,
                               LigmaCoords  *ret_coords)
{
  g_printerr ("ligma_vectors_interpolate: default implementation\n");

  return 0;
}

const LigmaBezierDesc *
ligma_vectors_get_bezier (LigmaVectors *vectors)
{
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), NULL);

  if (! vectors->bezier_desc)
    {
      vectors->bezier_desc = ligma_vectors_make_bezier (vectors);
    }

  return  vectors->bezier_desc;
}

static LigmaBezierDesc *
ligma_vectors_make_bezier (LigmaVectors *vectors)
{
  return LIGMA_VECTORS_GET_CLASS (vectors)->make_bezier (vectors);
}

static LigmaBezierDesc *
ligma_vectors_real_make_bezier (LigmaVectors *vectors)
{
  LigmaStroke     *stroke;
  GArray         *cmd_array;
  LigmaBezierDesc *ret_bezdesc = NULL;

  cmd_array = g_array_new (FALSE, FALSE, sizeof (cairo_path_data_t));

  for (stroke = ligma_vectors_stroke_get_next (vectors, NULL);
       stroke;
       stroke = ligma_vectors_stroke_get_next (vectors, stroke))
    {
      LigmaBezierDesc *bezdesc = ligma_stroke_make_bezier (stroke);

      if (bezdesc)
        {
          cmd_array = g_array_append_vals (cmd_array, bezdesc->data,
                                           bezdesc->num_data);
          ligma_bezier_desc_free (bezdesc);
        }
    }

  if (cmd_array->len > 0)
    ret_bezdesc = ligma_bezier_desc_new ((cairo_path_data_t *) cmd_array->data,
                                        cmd_array->len);

  g_array_free (cmd_array, FALSE);

  return ret_bezdesc;
}
