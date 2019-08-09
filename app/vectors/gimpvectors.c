/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpvectors.c
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "vectors-types.h"

#include "core/gimp.h"
#include "core/gimp-transform-utils.h"
#include "core/gimpbezierdesc.h"
#include "core/gimpchannel-select.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable-fill.h"
#include "core/gimpdrawable-stroke.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpmarshal.h"
#include "core/gimppaintinfo.h"
#include "core/gimpstrokeoptions.h"

#include "paint/gimppaintcore-stroke.h"
#include "paint/gimppaintoptions.h"

#include "gimpanchor.h"
#include "gimpstroke.h"
#include "gimpvectors.h"
#include "gimpvectors-preview.h"

#include "gimp-intl.h"


enum
{
  FREEZE,
  THAW,
  LAST_SIGNAL
};


static void       gimp_vectors_finalize      (GObject           *object);

static gint64     gimp_vectors_get_memsize   (GimpObject        *object,
                                              gint64            *gui_size);

static gboolean   gimp_vectors_is_attached   (GimpItem          *item);
static GimpItemTree * gimp_vectors_get_tree  (GimpItem          *item);
static gboolean   gimp_vectors_bounds        (GimpItem          *item,
                                              gdouble           *x,
                                              gdouble           *y,
                                              gdouble           *width,
                                              gdouble           *height);
static GimpItem * gimp_vectors_duplicate     (GimpItem          *item,
                                              GType              new_type);
static void       gimp_vectors_convert       (GimpItem          *item,
                                              GimpImage         *dest_image,
                                              GType              old_type);
static void       gimp_vectors_translate     (GimpItem          *item,
                                              gdouble            offset_x,
                                              gdouble            offset_y,
                                              gboolean           push_undo);
static void       gimp_vectors_scale         (GimpItem          *item,
                                              gint               new_width,
                                              gint               new_height,
                                              gint               new_offset_x,
                                              gint               new_offset_y,
                                              GimpInterpolationType  interp_type,
                                              GimpProgress      *progress);
static void       gimp_vectors_resize        (GimpItem          *item,
                                              GimpContext       *context,
                                              GimpFillType       fill_type,
                                              gint               new_width,
                                              gint               new_height,
                                              gint               offset_x,
                                              gint               offset_y);
static void       gimp_vectors_flip          (GimpItem          *item,
                                              GimpContext       *context,
                                              GimpOrientationType  flip_type,
                                              gdouble            axis,
                                              gboolean           clip_result);
static void       gimp_vectors_rotate        (GimpItem          *item,
                                              GimpContext       *context,
                                              GimpRotationType   rotate_type,
                                              gdouble            center_x,
                                              gdouble            center_y,
                                              gboolean           clip_result);
static void       gimp_vectors_transform     (GimpItem          *item,
                                              GimpContext       *context,
                                              const GimpMatrix3 *matrix,
                                              GimpTransformDirection direction,
                                              GimpInterpolationType interp_type,
                                              GimpTransformResize   clip_result,
                                              GimpProgress      *progress);
static GimpTransformResize
                  gimp_vectors_get_clip      (GimpItem          *item,
                                              GimpTransformResize clip_result);
static gboolean   gimp_vectors_fill          (GimpItem          *item,
                                              GimpDrawable      *drawable,
                                              GimpFillOptions   *fill_options,
                                              gboolean           push_undo,
                                              GimpProgress      *progress,
                                              GError           **error);
static gboolean   gimp_vectors_stroke        (GimpItem          *item,
                                              GimpDrawable      *drawable,
                                              GimpStrokeOptions *stroke_options,
                                              gboolean           push_undo,
                                              GimpProgress      *progress,
                                              GError           **error);
static void       gimp_vectors_to_selection  (GimpItem          *item,
                                              GimpChannelOps     op,
                                              gboolean           antialias,
                                              gboolean           feather,
                                              gdouble            feather_radius_x,
                                              gdouble            feather_radius_y);

static void       gimp_vectors_real_freeze          (GimpVectors       *vectors);
static void       gimp_vectors_real_thaw            (GimpVectors       *vectors);
static void       gimp_vectors_real_stroke_add      (GimpVectors       *vectors,
                                                     GimpStroke        *stroke);
static void       gimp_vectors_real_stroke_remove   (GimpVectors       *vectors,
                                                     GimpStroke        *stroke);
static GimpStroke * gimp_vectors_real_stroke_get    (GimpVectors       *vectors,
                                                     const GimpCoords  *coord);
static GimpStroke *gimp_vectors_real_stroke_get_next(GimpVectors       *vectors,
                                                     GimpStroke        *prev);
static gdouble gimp_vectors_real_stroke_get_length  (GimpVectors       *vectors,
                                                     GimpStroke        *prev);
static GimpAnchor * gimp_vectors_real_anchor_get    (GimpVectors       *vectors,
                                                     const GimpCoords  *coord,
                                                     GimpStroke       **ret_stroke);
static void       gimp_vectors_real_anchor_delete   (GimpVectors       *vectors,
                                                     GimpAnchor        *anchor);
static gdouble    gimp_vectors_real_get_length      (GimpVectors       *vectors,
                                                     const GimpAnchor  *start);
static gdouble    gimp_vectors_real_get_distance    (GimpVectors       *vectors,
                                                     const GimpCoords  *coord);
static gint       gimp_vectors_real_interpolate     (GimpVectors       *vectors,
                                                     GimpStroke        *stroke,
                                                     gdouble            precision,
                                                     gint               max_points,
                                                     GimpCoords        *ret_coords);

static GimpBezierDesc * gimp_vectors_make_bezier      (GimpVectors     *vectors);
static GimpBezierDesc * gimp_vectors_real_make_bezier (GimpVectors     *vectors);


G_DEFINE_TYPE (GimpVectors, gimp_vectors, GIMP_TYPE_ITEM)

#define parent_class gimp_vectors_parent_class

static guint gimp_vectors_signals[LAST_SIGNAL] = { 0 };


static void
gimp_vectors_class_init (GimpVectorsClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);

  gimp_vectors_signals[FREEZE] =
    g_signal_new ("freeze",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpVectorsClass, freeze),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_vectors_signals[THAW] =
    g_signal_new ("thaw",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpVectorsClass, thaw),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->finalize            = gimp_vectors_finalize;

  gimp_object_class->get_memsize    = gimp_vectors_get_memsize;

  viewable_class->get_new_preview   = gimp_vectors_get_new_preview;
  viewable_class->default_icon_name = "gimp-path";

  item_class->is_attached           = gimp_vectors_is_attached;
  item_class->get_tree              = gimp_vectors_get_tree;
  item_class->bounds                = gimp_vectors_bounds;
  item_class->duplicate             = gimp_vectors_duplicate;
  item_class->convert               = gimp_vectors_convert;
  item_class->translate             = gimp_vectors_translate;
  item_class->scale                 = gimp_vectors_scale;
  item_class->resize                = gimp_vectors_resize;
  item_class->flip                  = gimp_vectors_flip;
  item_class->rotate                = gimp_vectors_rotate;
  item_class->transform             = gimp_vectors_transform;
  item_class->get_clip              = gimp_vectors_get_clip;
  item_class->fill                  = gimp_vectors_fill;
  item_class->stroke                = gimp_vectors_stroke;
  item_class->to_selection          = gimp_vectors_to_selection;
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

  klass->freeze                     = gimp_vectors_real_freeze;
  klass->thaw                       = gimp_vectors_real_thaw;

  klass->stroke_add                 = gimp_vectors_real_stroke_add;
  klass->stroke_remove              = gimp_vectors_real_stroke_remove;
  klass->stroke_get                 = gimp_vectors_real_stroke_get;
  klass->stroke_get_next            = gimp_vectors_real_stroke_get_next;
  klass->stroke_get_length          = gimp_vectors_real_stroke_get_length;

  klass->anchor_get                 = gimp_vectors_real_anchor_get;
  klass->anchor_delete              = gimp_vectors_real_anchor_delete;

  klass->get_length                 = gimp_vectors_real_get_length;
  klass->get_distance               = gimp_vectors_real_get_distance;
  klass->interpolate                = gimp_vectors_real_interpolate;

  klass->make_bezier                = gimp_vectors_real_make_bezier;
}

static void
gimp_vectors_init (GimpVectors *vectors)
{
  gimp_item_set_visible (GIMP_ITEM (vectors), FALSE, FALSE);

  vectors->strokes        = g_queue_new ();
  vectors->stroke_to_list = g_hash_table_new (g_direct_hash, g_direct_equal);
  vectors->last_stroke_ID = 0;
  vectors->freeze_count   = 0;
  vectors->precision      = 0.2;

  vectors->bezier_desc    = NULL;
  vectors->bounds_valid   = FALSE;
}

static void
gimp_vectors_finalize (GObject *object)
{
  GimpVectors *vectors = GIMP_VECTORS (object);

  if (vectors->bezier_desc)
    {
      gimp_bezier_desc_free (vectors->bezier_desc);
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
gimp_vectors_get_memsize (GimpObject *object,
                          gint64     *gui_size)
{
  GimpVectors *vectors;
  GList       *list;
  gint64       memsize = 0;

  vectors = GIMP_VECTORS (object);

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    memsize += (gimp_object_get_memsize (GIMP_OBJECT (list->data), gui_size) +
                sizeof (GList));

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_vectors_is_attached (GimpItem *item)
{
  GimpImage *image = gimp_item_get_image (item);

  return (GIMP_IS_IMAGE (image) &&
          gimp_container_have (gimp_image_get_vectors (image),
                               GIMP_OBJECT (item)));
}

static GimpItemTree *
gimp_vectors_get_tree (GimpItem *item)
{
  if (gimp_item_is_attached (item))
    {
      GimpImage *image = gimp_item_get_image (item);

      return gimp_image_get_vectors_tree (image);
    }

  return NULL;
}

static gboolean
gimp_vectors_bounds (GimpItem *item,
                     gdouble  *x,
                     gdouble  *y,
                     gdouble  *width,
                     gdouble  *height)
{
  GimpVectors *vectors = GIMP_VECTORS (item);

  if (! vectors->bounds_valid)
    {
      GimpStroke *stroke;

      vectors->bounds_empty = TRUE;
      vectors->bounds_x1 = vectors->bounds_x2 = 0.0;
      vectors->bounds_y1 = vectors->bounds_y2 = 0.0;

      for (stroke = gimp_vectors_stroke_get_next (vectors, NULL);
           stroke;
           stroke = gimp_vectors_stroke_get_next (vectors, stroke))
        {
          GArray   *stroke_coords;
          gboolean  closed;

          stroke_coords = gimp_stroke_interpolate (stroke, 1.0, &closed);

          if (stroke_coords)
            {
              GimpCoords point;
              gint       i;

              if (vectors->bounds_empty && stroke_coords->len > 0)
                {
                  point = g_array_index (stroke_coords, GimpCoords, 0);

                  vectors->bounds_x1 = vectors->bounds_x2 = point.x;
                  vectors->bounds_y1 = vectors->bounds_y2 = point.y;

                  vectors->bounds_empty = FALSE;
                }

              for (i = 0; i < stroke_coords->len; i++)
                {
                  point = g_array_index (stroke_coords, GimpCoords, i);

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

static GimpItem *
gimp_vectors_duplicate (GimpItem *item,
                        GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_VECTORS), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (GIMP_IS_VECTORS (new_item))
    {
      GimpVectors *vectors     = GIMP_VECTORS (item);
      GimpVectors *new_vectors = GIMP_VECTORS (new_item);

      gimp_vectors_copy_strokes (vectors, new_vectors);
    }

  return new_item;
}

static void
gimp_vectors_convert (GimpItem  *item,
                      GimpImage *dest_image,
                      GType      old_type)
{
  gimp_item_set_size (item,
                      gimp_image_get_width  (dest_image),
                      gimp_image_get_height (dest_image));

  GIMP_ITEM_CLASS (parent_class)->convert (item, dest_image, old_type);
}

static void
gimp_vectors_translate (GimpItem *item,
                        gdouble   offset_x,
                        gdouble   offset_y,
                        gboolean  push_undo)
{
  GimpVectors *vectors = GIMP_VECTORS (item);
  GList       *list;

  gimp_vectors_freeze (vectors);

  if (push_undo)
    gimp_image_undo_push_vectors_mod (gimp_item_get_image (item),
                                      _("Move Path"),
                                      vectors);

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_translate (stroke, offset_x, offset_y);
    }

  gimp_vectors_thaw (vectors);
}

static void
gimp_vectors_scale (GimpItem              *item,
                    gint                   new_width,
                    gint                   new_height,
                    gint                   new_offset_x,
                    gint                   new_offset_y,
                    GimpInterpolationType  interpolation_type,
                    GimpProgress          *progress)
{
  GimpVectors *vectors = GIMP_VECTORS (item);
  GimpImage   *image   = gimp_item_get_image (item);
  GList       *list;

  gimp_vectors_freeze (vectors);

  if (gimp_item_is_attached (item))
    gimp_image_undo_push_vectors_mod (image, NULL, vectors);

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_scale (stroke,
                         (gdouble) new_width  / (gdouble) gimp_item_get_width  (item),
                         (gdouble) new_height / (gdouble) gimp_item_get_height (item));
      gimp_stroke_translate (stroke, new_offset_x, new_offset_y);
    }

  GIMP_ITEM_CLASS (parent_class)->scale (item,
                                         gimp_image_get_width  (image),
                                         gimp_image_get_height (image),
                                         0, 0,
                                         interpolation_type, progress);

  gimp_vectors_thaw (vectors);
}

static void
gimp_vectors_resize (GimpItem     *item,
                     GimpContext  *context,
                     GimpFillType  fill_type,
                     gint          new_width,
                     gint          new_height,
                     gint          offset_x,
                     gint          offset_y)
{
  GimpVectors *vectors = GIMP_VECTORS (item);
  GimpImage   *image   = gimp_item_get_image (item);
  GList       *list;

  gimp_vectors_freeze (vectors);

  if (gimp_item_is_attached (item))
    gimp_image_undo_push_vectors_mod (image, NULL, vectors);

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_translate (stroke, offset_x, offset_y);
    }

  GIMP_ITEM_CLASS (parent_class)->resize (item, context, fill_type,
                                          gimp_image_get_width  (image),
                                          gimp_image_get_height (image),
                                          0, 0);

  gimp_vectors_thaw (vectors);
}

static void
gimp_vectors_flip (GimpItem            *item,
                   GimpContext         *context,
                   GimpOrientationType  flip_type,
                   gdouble              axis,
                   gboolean             clip_result)
{
  GimpVectors *vectors = GIMP_VECTORS (item);
  GList       *list;
  GimpMatrix3  matrix;

  gimp_matrix3_identity (&matrix);
  gimp_transform_matrix_flip (&matrix, flip_type, axis);

  gimp_vectors_freeze (vectors);

  gimp_image_undo_push_vectors_mod (gimp_item_get_image (item),
                                    _("Flip Path"),
                                    vectors);

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_transform (stroke, &matrix, NULL);
    }

  gimp_vectors_thaw (vectors);
}

static void
gimp_vectors_rotate (GimpItem         *item,
                     GimpContext      *context,
                     GimpRotationType  rotate_type,
                     gdouble           center_x,
                     gdouble           center_y,
                     gboolean          clip_result)
{
  GimpVectors *vectors = GIMP_VECTORS (item);
  GList       *list;
  GimpMatrix3  matrix;

  gimp_matrix3_identity (&matrix);
  gimp_transform_matrix_rotate (&matrix, rotate_type, center_x, center_y);

  gimp_vectors_freeze (vectors);

  gimp_image_undo_push_vectors_mod (gimp_item_get_image (item),
                                    _("Rotate Path"),
                                    vectors);

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_transform (stroke, &matrix, NULL);
    }

  gimp_vectors_thaw (vectors);
}

static void
gimp_vectors_transform (GimpItem               *item,
                        GimpContext            *context,
                        const GimpMatrix3      *matrix,
                        GimpTransformDirection  direction,
                        GimpInterpolationType   interpolation_type,
                        GimpTransformResize     clip_result,
                        GimpProgress           *progress)
{
  GimpVectors *vectors = GIMP_VECTORS (item);
  GimpMatrix3  local_matrix;
  GQueue       strokes;
  GList       *list;

  gimp_vectors_freeze (vectors);

  gimp_image_undo_push_vectors_mod (gimp_item_get_image (item),
                                    _("Transform Path"),
                                    vectors);

  local_matrix = *matrix;

  if (direction == GIMP_TRANSFORM_BACKWARD)
    gimp_matrix3_invert (&local_matrix);

  g_queue_init (&strokes);

  while (! g_queue_is_empty (vectors->strokes))
    {
      GimpStroke *stroke = g_queue_peek_head (vectors->strokes);

      g_object_ref (stroke);

      gimp_vectors_stroke_remove (vectors, stroke);

      gimp_stroke_transform (stroke, &local_matrix, &strokes);

      g_object_unref (stroke);
    }

  vectors->last_stroke_ID = 0;

  for (list = strokes.head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_vectors_stroke_add (vectors, stroke);

      g_object_unref (stroke);
    }

  g_queue_clear (&strokes);

  gimp_vectors_thaw (vectors);
}

static GimpTransformResize
gimp_vectors_get_clip (GimpItem            *item,
                       GimpTransformResize  clip_result)
{
  return GIMP_TRANSFORM_RESIZE_ADJUST;
}

static gboolean
gimp_vectors_fill (GimpItem         *item,
                   GimpDrawable     *drawable,
                   GimpFillOptions  *fill_options,
                   gboolean          push_undo,
                   GimpProgress     *progress,
                   GError          **error)
{
  GimpVectors *vectors = GIMP_VECTORS (item);

  if (g_queue_is_empty (vectors->strokes))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Not enough points to fill"));
      return FALSE;
    }

  return gimp_drawable_fill_vectors (drawable, fill_options,
                                     vectors, push_undo, error);
}

static gboolean
gimp_vectors_stroke (GimpItem           *item,
                     GimpDrawable       *drawable,
                     GimpStrokeOptions  *stroke_options,
                     gboolean            push_undo,
                     GimpProgress       *progress,
                     GError            **error)
{
  GimpVectors *vectors = GIMP_VECTORS (item);
  gboolean     retval  = FALSE;

  if (g_queue_is_empty (vectors->strokes))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Not enough points to stroke"));
      return FALSE;
    }

  switch (gimp_stroke_options_get_method (stroke_options))
    {
    case GIMP_STROKE_LINE:
      retval = gimp_drawable_stroke_vectors (drawable, stroke_options,
                                             vectors, push_undo, error);
      break;

    case GIMP_STROKE_PAINT_METHOD:
      {
        GimpPaintInfo    *paint_info;
        GimpPaintCore    *core;
        GimpPaintOptions *paint_options;
        gboolean          emulate_dynamics;

        paint_info = gimp_context_get_paint_info (GIMP_CONTEXT (stroke_options));

        core = g_object_new (paint_info->paint_type, NULL);

        paint_options = gimp_stroke_options_get_paint_options (stroke_options);
        emulate_dynamics = gimp_stroke_options_get_emulate_dynamics (stroke_options);

        retval = gimp_paint_core_stroke_vectors (core, drawable,
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
gimp_vectors_to_selection (GimpItem       *item,
                           GimpChannelOps  op,
                           gboolean        antialias,
                           gboolean        feather,
                           gdouble         feather_radius_x,
                           gdouble         feather_radius_y)
{
  GimpVectors *vectors = GIMP_VECTORS (item);
  GimpImage   *image   = gimp_item_get_image (item);

  gimp_channel_select_vectors (gimp_image_get_mask (image),
                               GIMP_ITEM_GET_CLASS (item)->to_selection_desc,
                               vectors,
                               op, antialias,
                               feather, feather_radius_x, feather_radius_x,
                               TRUE);
}

static void
gimp_vectors_real_freeze (GimpVectors *vectors)
{
  /*  release cached bezier representation  */
  if (vectors->bezier_desc)
    {
      gimp_bezier_desc_free (vectors->bezier_desc);
      vectors->bezier_desc = NULL;
    }

  /*  invalidate bounds  */
  vectors->bounds_valid = FALSE;
}

static void
gimp_vectors_real_thaw (GimpVectors *vectors)
{
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (vectors));
}


/*  public functions  */

GimpVectors *
gimp_vectors_new (GimpImage   *image,
                  const gchar *name)
{
  GimpVectors *vectors;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  vectors = GIMP_VECTORS (gimp_item_new (GIMP_TYPE_VECTORS,
                                         image, name,
                                         0, 0,
                                         gimp_image_get_width  (image),
                                         gimp_image_get_height (image)));

  return vectors;
}

GimpVectors *
gimp_vectors_get_parent (GimpVectors *vectors)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  return GIMP_VECTORS (gimp_viewable_get_parent (GIMP_VIEWABLE (vectors)));
}

void
gimp_vectors_freeze (GimpVectors *vectors)
{
  g_return_if_fail (GIMP_IS_VECTORS (vectors));

  vectors->freeze_count++;

  if (vectors->freeze_count == 1)
    g_signal_emit (vectors, gimp_vectors_signals[FREEZE], 0);
}

void
gimp_vectors_thaw (GimpVectors *vectors)
{
  g_return_if_fail (GIMP_IS_VECTORS (vectors));
  g_return_if_fail (vectors->freeze_count > 0);

  vectors->freeze_count--;

  if (vectors->freeze_count == 0)
    g_signal_emit (vectors, gimp_vectors_signals[THAW], 0);
}

void
gimp_vectors_copy_strokes (GimpVectors *src_vectors,
                           GimpVectors *dest_vectors)
{
  g_return_if_fail (GIMP_IS_VECTORS (src_vectors));
  g_return_if_fail (GIMP_IS_VECTORS (dest_vectors));

  gimp_vectors_freeze (dest_vectors);

  g_queue_free_full (dest_vectors->strokes, (GDestroyNotify) g_object_unref);
  dest_vectors->strokes = g_queue_new ();
  g_hash_table_remove_all (dest_vectors->stroke_to_list);

  dest_vectors->last_stroke_ID = 0;

  gimp_vectors_add_strokes (src_vectors, dest_vectors);

  gimp_vectors_thaw (dest_vectors);
}


void
gimp_vectors_add_strokes (GimpVectors *src_vectors,
                          GimpVectors *dest_vectors)
{
  GList *stroke;

  g_return_if_fail (GIMP_IS_VECTORS (src_vectors));
  g_return_if_fail (GIMP_IS_VECTORS (dest_vectors));

  gimp_vectors_freeze (dest_vectors);

  for (stroke = src_vectors->strokes->head;
       stroke != NULL;
       stroke = g_list_next (stroke))
    {
      GimpStroke *newstroke = gimp_stroke_duplicate (stroke->data);

      g_queue_push_tail (dest_vectors->strokes, newstroke);

      /* Also add to {stroke: GList node} map */
      g_hash_table_insert (dest_vectors->stroke_to_list,
                           newstroke,
                           g_queue_peek_tail_link (dest_vectors->strokes));

      dest_vectors->last_stroke_ID++;
      gimp_stroke_set_ID (newstroke,
                          dest_vectors->last_stroke_ID);
    }

  gimp_vectors_thaw (dest_vectors);
}


void
gimp_vectors_stroke_add (GimpVectors *vectors,
                         GimpStroke  *stroke)
{
  g_return_if_fail (GIMP_IS_VECTORS (vectors));
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  gimp_vectors_freeze (vectors);

  GIMP_VECTORS_GET_CLASS (vectors)->stroke_add (vectors, stroke);

  gimp_vectors_thaw (vectors);
}

static void
gimp_vectors_real_stroke_add (GimpVectors *vectors,
                              GimpStroke  *stroke)
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

  vectors->last_stroke_ID++;
  gimp_stroke_set_ID (stroke, vectors->last_stroke_ID);
}

void
gimp_vectors_stroke_remove (GimpVectors *vectors,
                            GimpStroke  *stroke)
{
  g_return_if_fail (GIMP_IS_VECTORS (vectors));
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  gimp_vectors_freeze (vectors);

  GIMP_VECTORS_GET_CLASS (vectors)->stroke_remove (vectors, stroke);

  gimp_vectors_thaw (vectors);
}

static void
gimp_vectors_real_stroke_remove (GimpVectors *vectors,
                                 GimpStroke  *stroke)
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
gimp_vectors_get_n_strokes (GimpVectors *vectors)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0);

  return g_queue_get_length (vectors->strokes);
}


GimpStroke *
gimp_vectors_stroke_get (GimpVectors      *vectors,
                         const GimpCoords *coord)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  return GIMP_VECTORS_GET_CLASS (vectors)->stroke_get (vectors, coord);
}

static GimpStroke *
gimp_vectors_real_stroke_get (GimpVectors      *vectors,
                              const GimpCoords *coord)
{
  GimpStroke *minstroke = NULL;
  gdouble     mindist   = G_MAXDOUBLE;
  GList      *list;

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;
      GimpAnchor *anchor = gimp_stroke_anchor_get (stroke, coord);

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

GimpStroke *
gimp_vectors_stroke_get_by_ID (GimpVectors *vectors,
                               gint         id)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      if (gimp_stroke_get_ID (list->data) == id)
        return list->data;
    }

  return NULL;
}


GimpStroke *
gimp_vectors_stroke_get_next (GimpVectors *vectors,
                              GimpStroke  *prev)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  return GIMP_VECTORS_GET_CLASS (vectors)->stroke_get_next (vectors, prev);
}

static GimpStroke *
gimp_vectors_real_stroke_get_next (GimpVectors *vectors,
                                   GimpStroke  *prev)
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
gimp_vectors_stroke_get_length (GimpVectors *vectors,
                                GimpStroke  *stroke)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0.0);
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), 0.0);

  return GIMP_VECTORS_GET_CLASS (vectors)->stroke_get_length (vectors, stroke);
}

static gdouble
gimp_vectors_real_stroke_get_length (GimpVectors *vectors,
                                     GimpStroke  *stroke)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0.0);
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), 0.0);

  return gimp_stroke_get_length (stroke, vectors->precision);
}


GimpAnchor *
gimp_vectors_anchor_get (GimpVectors       *vectors,
                         const GimpCoords  *coord,
                         GimpStroke       **ret_stroke)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  return GIMP_VECTORS_GET_CLASS (vectors)->anchor_get (vectors, coord,
                                                       ret_stroke);
}

static GimpAnchor *
gimp_vectors_real_anchor_get (GimpVectors       *vectors,
                              const GimpCoords  *coord,
                              GimpStroke       **ret_stroke)
{
  GimpAnchor *minanchor = NULL;
  gdouble     mindist   = -1;
  GList      *list;

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;
      GimpAnchor *anchor = gimp_stroke_anchor_get (stroke, coord);

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
gimp_vectors_anchor_delete (GimpVectors *vectors,
                            GimpAnchor  *anchor)
{
  g_return_if_fail (GIMP_IS_VECTORS (vectors));
  g_return_if_fail (anchor != NULL);

  GIMP_VECTORS_GET_CLASS (vectors)->anchor_delete (vectors, anchor);
}

static void
gimp_vectors_real_anchor_delete (GimpVectors *vectors,
                                 GimpAnchor  *anchor)
{
}


void
gimp_vectors_anchor_select (GimpVectors *vectors,
                            GimpStroke  *target_stroke,
                            GimpAnchor  *anchor,
                            gboolean     selected,
                            gboolean     exclusive)
{
  GList *list;

  for (list = vectors->strokes->head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_anchor_select (stroke,
                                 stroke == target_stroke ? anchor : NULL,
                                 selected, exclusive);
    }
}


gdouble
gimp_vectors_get_length (GimpVectors      *vectors,
                         const GimpAnchor *start)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0.0);

  return GIMP_VECTORS_GET_CLASS (vectors)->get_length (vectors, start);
}

static gdouble
gimp_vectors_real_get_length (GimpVectors      *vectors,
                              const GimpAnchor *start)
{
  g_printerr ("gimp_vectors_get_length: default implementation\n");

  return 0;
}


gdouble
gimp_vectors_get_distance (GimpVectors      *vectors,
                           const GimpCoords *coord)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0.0);

  return GIMP_VECTORS_GET_CLASS (vectors)->get_distance (vectors, coord);
}

static gdouble
gimp_vectors_real_get_distance (GimpVectors      *vectors,
                                const GimpCoords *coord)
{
  g_printerr ("gimp_vectors_get_distance: default implementation\n");

  return 0;
}

gint
gimp_vectors_interpolate (GimpVectors *vectors,
                          GimpStroke  *stroke,
                          gdouble      precision,
                          gint         max_points,
                          GimpCoords  *ret_coords)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0);

  return GIMP_VECTORS_GET_CLASS (vectors)->interpolate (vectors, stroke,
                                                        precision, max_points,
                                                        ret_coords);
}

static gint
gimp_vectors_real_interpolate (GimpVectors *vectors,
                               GimpStroke  *stroke,
                               gdouble      precision,
                               gint         max_points,
                               GimpCoords  *ret_coords)
{
  g_printerr ("gimp_vectors_interpolate: default implementation\n");

  return 0;
}

const GimpBezierDesc *
gimp_vectors_get_bezier (GimpVectors *vectors)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  if (! vectors->bezier_desc)
    {
      vectors->bezier_desc = gimp_vectors_make_bezier (vectors);
    }

  return  vectors->bezier_desc;
}

static GimpBezierDesc *
gimp_vectors_make_bezier (GimpVectors *vectors)
{
  return GIMP_VECTORS_GET_CLASS (vectors)->make_bezier (vectors);
}

static GimpBezierDesc *
gimp_vectors_real_make_bezier (GimpVectors *vectors)
{
  GimpStroke     *stroke;
  GArray         *cmd_array;
  GimpBezierDesc *ret_bezdesc = NULL;

  cmd_array = g_array_new (FALSE, FALSE, sizeof (cairo_path_data_t));

  for (stroke = gimp_vectors_stroke_get_next (vectors, NULL);
       stroke;
       stroke = gimp_vectors_stroke_get_next (vectors, stroke))
    {
      GimpBezierDesc *bezdesc = gimp_stroke_make_bezier (stroke);

      if (bezdesc)
        {
          cmd_array = g_array_append_vals (cmd_array, bezdesc->data,
                                           bezdesc->num_data);
          gimp_bezier_desc_free (bezdesc);
        }
    }

  if (cmd_array->len > 0)
    ret_bezdesc = gimp_bezier_desc_new ((cairo_path_data_t *) cmd_array->data,
                                        cmd_array->len);

  g_array_free (cmd_array, FALSE);

  return ret_bezdesc;
}
