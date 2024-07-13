/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppath.c
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
#include "core/gimppaintinfo.h"
#include "core/gimpstrokeoptions.h"

#include "paint/gimppaintcore-stroke.h"
#include "paint/gimppaintoptions.h"

#include "gimpanchor.h"
#include "gimppath.h"
#include "gimppath-preview.h"
#include "gimpstroke.h"

#include "gimp-intl.h"


enum
{
  FREEZE,
  THAW,
  LAST_SIGNAL
};


static void       gimp_path_finalize         (GObject           *object);

static gint64     gimp_path_get_memsize      (GimpObject        *object,
                                              gint64            *gui_size);

static gboolean   gimp_path_is_attached      (GimpItem          *item);
static GimpItemTree * gimp_path_get_tree     (GimpItem          *item);
static gboolean   gimp_path_bounds           (GimpItem          *item,
                                              gdouble           *x,
                                              gdouble           *y,
                                              gdouble           *width,
                                              gdouble           *height);
static GimpItem * gimp_path_duplicate        (GimpItem          *item,
                                              GType              new_type);
static void       gimp_path_convert          (GimpItem          *item,
                                              GimpImage         *dest_image,
                                              GType              old_type);
static void       gimp_path_translate        (GimpItem          *item,
                                              gdouble            offset_x,
                                              gdouble            offset_y,
                                              gboolean           push_undo);
static void       gimp_path_scale            (GimpItem          *item,
                                              gint               new_width,
                                              gint               new_height,
                                              gint               new_offset_x,
                                              gint               new_offset_y,
                                              GimpInterpolationType  interp_type,
                                              GimpProgress      *progress);
static void       gimp_path_resize           (GimpItem          *item,
                                              GimpContext       *context,
                                              GimpFillType       fill_type,
                                              gint               new_width,
                                              gint               new_height,
                                              gint               offset_x,
                                              gint               offset_y);
static void       gimp_path_flip             (GimpItem          *item,
                                              GimpContext       *context,
                                              GimpOrientationType  flip_type,
                                              gdouble            axis,
                                              gboolean           clip_result);
static void       gimp_path_rotate           (GimpItem          *item,
                                              GimpContext       *context,
                                              GimpRotationType   rotate_type,
                                              gdouble            center_x,
                                              gdouble            center_y,
                                              gboolean           clip_result);
static void       gimp_path_transform        (GimpItem          *item,
                                              GimpContext       *context,
                                              const GimpMatrix3 *matrix,
                                              GimpTransformDirection direction,
                                              GimpInterpolationType interp_type,
                                              GimpTransformResize   clip_result,
                                              GimpProgress      *progress);
static GimpTransformResize
                  gimp_path_get_clip         (GimpItem          *item,
                                              GimpTransformResize clip_result);
static gboolean   gimp_path_fill             (GimpItem          *item,
                                              GimpDrawable      *drawable,
                                              GimpFillOptions   *fill_options,
                                              gboolean           push_undo,
                                              GimpProgress      *progress,
                                              GError           **error);
static gboolean   gimp_path_stroke           (GimpItem          *item,
                                              GimpDrawable      *drawable,
                                              GimpStrokeOptions *stroke_options,
                                              gboolean           push_undo,
                                              GimpProgress      *progress,
                                              GError           **error);
static void       gimp_path_to_selection     (GimpItem          *item,
                                              GimpChannelOps     op,
                                              gboolean           antialias,
                                              gboolean           feather,
                                              gdouble            feather_radius_x,
                                              gdouble            feather_radius_y);

static void       gimp_path_real_freeze             (GimpPath          *path);
static void       gimp_path_real_thaw               (GimpPath          *path);
static void       gimp_path_real_stroke_add         (GimpPath          *path,
                                                     GimpStroke        *stroke);
static void       gimp_path_real_stroke_remove      (GimpPath          *path,
                                                     GimpStroke        *stroke);
static GimpStroke * gimp_path_real_stroke_get       (GimpPath          *path,
                                                     const GimpCoords  *coord);
static GimpStroke *gimp_path_real_stroke_get_next   (GimpPath          *path,
                                                     GimpStroke        *prev);
static gdouble gimp_path_real_stroke_get_length     (GimpPath          *path,
                                                     GimpStroke        *prev);
static GimpAnchor * gimp_path_real_anchor_get       (GimpPath          *path,
                                                     const GimpCoords  *coord,
                                                     GimpStroke       **ret_stroke);
static void       gimp_path_real_anchor_delete      (GimpPath          *path,
                                                     GimpAnchor        *anchor);
static gdouble    gimp_path_real_get_length         (GimpPath          *path,
                                                     const GimpAnchor  *start);
static gdouble    gimp_path_real_get_distance       (GimpPath          *path,
                                                     const GimpCoords  *coord);
static gint       gimp_path_real_interpolate        (GimpPath          *path,
                                                     GimpStroke        *stroke,
                                                     gdouble            precision,
                                                     gint               max_points,
                                                     GimpCoords        *ret_coords);

static GimpBezierDesc * gimp_path_make_bezier       (GimpPath          *path);
static GimpBezierDesc * gimp_path_real_make_bezier  (GimpPath          *path);


G_DEFINE_TYPE (GimpPath, gimp_path, GIMP_TYPE_ITEM)

#define parent_class gimp_path_parent_class

static guint gimp_path_signals[LAST_SIGNAL] = { 0 };


static void
gimp_path_class_init (GimpPathClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);

  gimp_path_signals[FREEZE] =
    g_signal_new ("freeze",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpPathClass, freeze),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_path_signals[THAW] =
    g_signal_new ("thaw",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPathClass, thaw),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize            = gimp_path_finalize;

  gimp_object_class->get_memsize    = gimp_path_get_memsize;

  viewable_class->get_new_preview   = gimp_path_get_new_preview;
  viewable_class->default_icon_name = "gimp-path";

  item_class->is_attached           = gimp_path_is_attached;
  item_class->get_tree              = gimp_path_get_tree;
  item_class->bounds                = gimp_path_bounds;
  item_class->duplicate             = gimp_path_duplicate;
  item_class->convert               = gimp_path_convert;
  item_class->translate             = gimp_path_translate;
  item_class->scale                 = gimp_path_scale;
  item_class->resize                = gimp_path_resize;
  item_class->flip                  = gimp_path_flip;
  item_class->rotate                = gimp_path_rotate;
  item_class->transform             = gimp_path_transform;
  item_class->get_clip              = gimp_path_get_clip;
  item_class->fill                  = gimp_path_fill;
  item_class->stroke                = gimp_path_stroke;
  item_class->to_selection          = gimp_path_to_selection;
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

  klass->freeze                     = gimp_path_real_freeze;
  klass->thaw                       = gimp_path_real_thaw;

  klass->stroke_add                 = gimp_path_real_stroke_add;
  klass->stroke_remove              = gimp_path_real_stroke_remove;
  klass->stroke_get                 = gimp_path_real_stroke_get;
  klass->stroke_get_next            = gimp_path_real_stroke_get_next;
  klass->stroke_get_length          = gimp_path_real_stroke_get_length;

  klass->anchor_get                 = gimp_path_real_anchor_get;
  klass->anchor_delete              = gimp_path_real_anchor_delete;

  klass->get_length                 = gimp_path_real_get_length;
  klass->get_distance               = gimp_path_real_get_distance;
  klass->interpolate                = gimp_path_real_interpolate;

  klass->make_bezier                = gimp_path_real_make_bezier;
}

static void
gimp_path_init (GimpPath *path)
{
  gimp_item_set_visible (GIMP_ITEM (path), FALSE, FALSE);

  path->strokes        = g_queue_new ();
  path->stroke_to_list = g_hash_table_new (g_direct_hash, g_direct_equal);
  path->last_stroke_id = 0;
  path->freeze_count   = 0;
  path->precision      = 0.2;

  path->bezier_desc    = NULL;
  path->bounds_valid   = FALSE;
}

static void
gimp_path_finalize (GObject *object)
{
  GimpPath *path = GIMP_PATH (object);

  if (path->bezier_desc)
    {
      gimp_bezier_desc_free (path->bezier_desc);
      path->bezier_desc = NULL;
    }

  if (path->strokes)
    {
      g_queue_free_full (path->strokes, (GDestroyNotify) g_object_unref);
      path->strokes = NULL;
    }

  if (path->stroke_to_list)
    {
      g_hash_table_destroy (path->stroke_to_list);
      path->stroke_to_list = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_path_get_memsize (GimpObject *object,
                       gint64     *gui_size)
{
  GimpPath *path;
  GList    *list;
  gint64    memsize = 0;

  path = GIMP_PATH (object);

  for (list = path->strokes->head; list; list = g_list_next (list))
    memsize += (gimp_object_get_memsize (GIMP_OBJECT (list->data), gui_size) +
                sizeof (GList));

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_path_is_attached (GimpItem *item)
{
  GimpImage *image = gimp_item_get_image (item);

  return (GIMP_IS_IMAGE (image) &&
          gimp_container_have (gimp_image_get_paths (image),
                               GIMP_OBJECT (item)));
}

static GimpItemTree *
gimp_path_get_tree (GimpItem *item)
{
  if (gimp_item_is_attached (item))
    {
      GimpImage *image = gimp_item_get_image (item);

      return gimp_image_get_path_tree (image);
    }

  return NULL;
}

static gboolean
gimp_path_bounds (GimpItem *item,
                  gdouble  *x,
                  gdouble  *y,
                  gdouble  *width,
                  gdouble  *height)
{
  GimpPath *path = GIMP_PATH (item);

  if (! path->bounds_valid)
    {
      GimpStroke *stroke;

      path->bounds_empty = TRUE;
      path->bounds_x1 = path->bounds_x2 = 0.0;
      path->bounds_y1 = path->bounds_y2 = 0.0;

      for (stroke = gimp_path_stroke_get_next (path, NULL);
           stroke;
           stroke = gimp_path_stroke_get_next (path, stroke))
        {
          GArray   *stroke_coords;
          gboolean  closed;

          stroke_coords = gimp_stroke_interpolate (stroke, 1.0, &closed);

          if (stroke_coords)
            {
              GimpCoords point;
              gint       i;

              if (path->bounds_empty && stroke_coords->len > 0)
                {
                  point = g_array_index (stroke_coords, GimpCoords, 0);

                  path->bounds_x1 = path->bounds_x2 = point.x;
                  path->bounds_y1 = path->bounds_y2 = point.y;

                  path->bounds_empty = FALSE;
                }

              for (i = 0; i < stroke_coords->len; i++)
                {
                  point = g_array_index (stroke_coords, GimpCoords, i);

                  path->bounds_x1 = MIN (path->bounds_x1, point.x);
                  path->bounds_y1 = MIN (path->bounds_y1, point.y);
                  path->bounds_x2 = MAX (path->bounds_x2, point.x);
                  path->bounds_y2 = MAX (path->bounds_y2, point.y);
                }

              g_array_free (stroke_coords, TRUE);
            }
        }

      path->bounds_valid = TRUE;
    }

  *x      = path->bounds_x1;
  *y      = path->bounds_y1;
  *width  = path->bounds_x2 - path->bounds_x1;
  *height = path->bounds_y2 - path->bounds_y1;

  return ! path->bounds_empty;
}

static GimpItem *
gimp_path_duplicate (GimpItem *item,
                     GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_PATH), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (GIMP_IS_PATH (new_item))
    {
      GimpPath *path     = GIMP_PATH (item);
      GimpPath *new_path = GIMP_PATH (new_item);

      gimp_path_copy_strokes (path, new_path);
    }

  return new_item;
}

static void
gimp_path_convert (GimpItem  *item,
                   GimpImage *dest_image,
                   GType      old_type)
{
  gimp_item_set_size (item,
                      gimp_image_get_width  (dest_image),
                      gimp_image_get_height (dest_image));

  GIMP_ITEM_CLASS (parent_class)->convert (item, dest_image, old_type);
}

static void
gimp_path_translate (GimpItem *item,
                     gdouble   offset_x,
                     gdouble   offset_y,
                     gboolean  push_undo)
{
  GimpPath *path = GIMP_PATH (item);
  GList    *list;

  gimp_path_freeze (path);

  if (push_undo)
    gimp_image_undo_push_path_mod (gimp_item_get_image (item),
                                   _("Move Path"),
                                   path);

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_translate (stroke, offset_x, offset_y);
    }

  gimp_path_thaw (path);
}

static void
gimp_path_scale (GimpItem              *item,
                 gint                   new_width,
                 gint                   new_height,
                 gint                   new_offset_x,
                 gint                   new_offset_y,
                 GimpInterpolationType  interpolation_type,
                 GimpProgress          *progress)
{
  GimpPath  *path  = GIMP_PATH (item);
  GimpImage *image = gimp_item_get_image (item);
  GList     *list;

  gimp_path_freeze (path);

  if (gimp_item_is_attached (item))
    gimp_image_undo_push_path_mod (image, NULL, path);

  for (list = path->strokes->head; list; list = g_list_next (list))
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

  gimp_path_thaw (path);
}

static void
gimp_path_resize (GimpItem     *item,
                  GimpContext  *context,
                  GimpFillType  fill_type,
                  gint          new_width,
                  gint          new_height,
                  gint          offset_x,
                  gint          offset_y)
{
  GimpPath  *path  = GIMP_PATH (item);
  GimpImage *image = gimp_item_get_image (item);
  GList     *list;

  gimp_path_freeze (path);

  if (gimp_item_is_attached (item))
    gimp_image_undo_push_path_mod (image, NULL, path);

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_translate (stroke, offset_x, offset_y);
    }

  GIMP_ITEM_CLASS (parent_class)->resize (item, context, fill_type,
                                          gimp_image_get_width  (image),
                                          gimp_image_get_height (image),
                                          0, 0);

  gimp_path_thaw (path);
}

static void
gimp_path_flip (GimpItem            *item,
                GimpContext         *context,
                GimpOrientationType  flip_type,
                gdouble              axis,
                gboolean             clip_result)
{
  GimpPath    *path = GIMP_PATH (item);
  GList       *list;
  GimpMatrix3  matrix;

  gimp_matrix3_identity (&matrix);
  gimp_transform_matrix_flip (&matrix, flip_type, axis);

  gimp_path_freeze (path);

  gimp_image_undo_push_path_mod (gimp_item_get_image (item),
                                 _("Flip Path"),
                                 path);

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_transform (stroke, &matrix, NULL);
    }

  gimp_path_thaw (path);
}

static void
gimp_path_rotate (GimpItem         *item,
                  GimpContext      *context,
                  GimpRotationType  rotate_type,
                  gdouble           center_x,
                  gdouble           center_y,
                  gboolean          clip_result)
{
  GimpPath    *path = GIMP_PATH (item);
  GList       *list;
  GimpMatrix3  matrix;

  gimp_matrix3_identity (&matrix);
  gimp_transform_matrix_rotate (&matrix, rotate_type, center_x, center_y);

  gimp_path_freeze (path);

  gimp_image_undo_push_path_mod (gimp_item_get_image (item),
                                 _("Rotate Path"),
                                 path);

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_transform (stroke, &matrix, NULL);
    }

  gimp_path_thaw (path);
}

static void
gimp_path_transform (GimpItem               *item,
                     GimpContext            *context,
                     const GimpMatrix3      *matrix,
                     GimpTransformDirection  direction,
                     GimpInterpolationType   interpolation_type,
                     GimpTransformResize     clip_result,
                     GimpProgress           *progress)
{
  GimpPath    *path = GIMP_PATH (item);
  GimpMatrix3  local_matrix;
  GQueue       strokes;
  GList       *list;

  gimp_path_freeze (path);

  gimp_image_undo_push_path_mod (gimp_item_get_image (item),
                                 _("Transform Path"),
                                 path);

  local_matrix = *matrix;

  if (direction == GIMP_TRANSFORM_BACKWARD)
    gimp_matrix3_invert (&local_matrix);

  g_queue_init (&strokes);

  while (! g_queue_is_empty (path->strokes))
    {
      GimpStroke *stroke = g_queue_peek_head (path->strokes);

      g_object_ref (stroke);

      gimp_path_stroke_remove (path, stroke);

      gimp_stroke_transform (stroke, &local_matrix, &strokes);

      g_object_unref (stroke);
    }

  path->last_stroke_id = 0;

  for (list = strokes.head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_path_stroke_add (path, stroke);

      g_object_unref (stroke);
    }

  g_queue_clear (&strokes);

  gimp_path_thaw (path);
}

static GimpTransformResize
gimp_path_get_clip (GimpItem            *item,
                    GimpTransformResize  clip_result)
{
  return GIMP_TRANSFORM_RESIZE_ADJUST;
}

static gboolean
gimp_path_fill (GimpItem         *item,
                GimpDrawable     *drawable,
                GimpFillOptions  *fill_options,
                gboolean          push_undo,
                GimpProgress     *progress,
                GError          **error)
{
  GimpPath *path = GIMP_PATH (item);

  if (g_queue_is_empty (path->strokes))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Not enough points to fill"));
      return FALSE;
    }

  return gimp_drawable_fill_path (drawable, fill_options,
                                     path, push_undo, error);
}

static gboolean
gimp_path_stroke (GimpItem           *item,
                  GimpDrawable       *drawable,
                  GimpStrokeOptions  *stroke_options,
                  gboolean            push_undo,
                  GimpProgress       *progress,
                  GError            **error)
{
  GimpPath *path   = GIMP_PATH (item);
  gboolean  retval = FALSE;

  if (g_queue_is_empty (path->strokes))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Not enough points to stroke"));
      return FALSE;
    }

  switch (gimp_stroke_options_get_method (stroke_options))
    {
    case GIMP_STROKE_LINE:
      retval = gimp_drawable_stroke_path (drawable, stroke_options,
                                          path, push_undo, error);
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

        retval = gimp_paint_core_stroke_path (core, drawable,
                                              paint_options,
                                              emulate_dynamics,
                                              path, push_undo, error);

        g_object_unref (core);
      }
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  return retval;
}

static void
gimp_path_to_selection (GimpItem       *item,
                        GimpChannelOps  op,
                        gboolean        antialias,
                        gboolean        feather,
                        gdouble         feather_radius_x,
                        gdouble         feather_radius_y)
{
  GimpPath    *path  = GIMP_PATH (item);
  GimpImage   *image = gimp_item_get_image (item);

  gimp_channel_select_path (gimp_image_get_mask (image),
                            GIMP_ITEM_GET_CLASS (item)->to_selection_desc,
                            path,
                            op, antialias,
                            feather, feather_radius_x, feather_radius_x,
                            TRUE);
}

static void
gimp_path_real_freeze (GimpPath *path)
{
  /*  release cached bezier representation  */
  if (path->bezier_desc)
    {
      gimp_bezier_desc_free (path->bezier_desc);
      path->bezier_desc = NULL;
    }

  /*  invalidate bounds  */
  path->bounds_valid = FALSE;
}

static void
gimp_path_real_thaw (GimpPath *path)
{
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (path));
}


/*  public functions  */

GimpPath *
gimp_path_new (GimpImage   *image,
               const gchar *name)
{
  GimpPath *path;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  path = GIMP_PATH (gimp_item_new (GIMP_TYPE_PATH,
                                   image, name,
                                   0, 0,
                                   gimp_image_get_width  (image),
                                   gimp_image_get_height (image)));

  return path;
}

GimpPath *
gimp_path_get_parent (GimpPath *path)
{
  g_return_val_if_fail (GIMP_IS_PATH (path), NULL);

  return GIMP_PATH (gimp_viewable_get_parent (GIMP_VIEWABLE (path)));
}

void
gimp_path_freeze (GimpPath *path)
{
  g_return_if_fail (GIMP_IS_PATH (path));

  path->freeze_count++;

  if (path->freeze_count == 1)
    g_signal_emit (path, gimp_path_signals[FREEZE], 0);
}

void
gimp_path_thaw (GimpPath *path)
{
  g_return_if_fail (GIMP_IS_PATH (path));
  g_return_if_fail (path->freeze_count > 0);

  path->freeze_count--;

  if (path->freeze_count == 0)
    g_signal_emit (path, gimp_path_signals[THAW], 0);
}

void
gimp_path_copy_strokes (GimpPath *src_path,
                        GimpPath *dest_path)
{
  g_return_if_fail (GIMP_IS_PATH (src_path));
  g_return_if_fail (GIMP_IS_PATH (dest_path));

  gimp_path_freeze (dest_path);

  g_queue_free_full (dest_path->strokes, (GDestroyNotify) g_object_unref);
  dest_path->strokes = g_queue_new ();
  g_hash_table_remove_all (dest_path->stroke_to_list);

  dest_path->last_stroke_id = 0;

  gimp_path_add_strokes (src_path, dest_path);

  gimp_path_thaw (dest_path);
}


void
gimp_path_add_strokes (GimpPath *src_path,
                       GimpPath *dest_path)
{
  GList *stroke;

  g_return_if_fail (GIMP_IS_PATH (src_path));
  g_return_if_fail (GIMP_IS_PATH (dest_path));

  gimp_path_freeze (dest_path);

  for (stroke = src_path->strokes->head;
       stroke != NULL;
       stroke = g_list_next (stroke))
    {
      GimpStroke *newstroke = gimp_stroke_duplicate (stroke->data);

      g_queue_push_tail (dest_path->strokes, newstroke);

      /* Also add to {stroke: GList node} map */
      g_hash_table_insert (dest_path->stroke_to_list,
                           newstroke,
                           g_queue_peek_tail_link (dest_path->strokes));

      dest_path->last_stroke_id++;
      gimp_stroke_set_id (newstroke,
                          dest_path->last_stroke_id);
    }

  gimp_path_thaw (dest_path);
}


void
gimp_path_stroke_add (GimpPath    *path,
                      GimpStroke  *stroke)
{
  g_return_if_fail (GIMP_IS_PATH (path));
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  gimp_path_freeze (path);

  GIMP_PATH_GET_CLASS (path)->stroke_add (path, stroke);

  gimp_path_thaw (path);
}

static void
gimp_path_real_stroke_add (GimpPath    *path,
                           GimpStroke  *stroke)
{
  /*
   * Don't prepend into path->strokes.  See ChangeLog 2003-05-21
   * --Mitch
   */
  g_queue_push_tail (path->strokes, g_object_ref (stroke));

  /* Also add to {stroke: GList node} map */
  g_hash_table_insert (path->stroke_to_list,
                       stroke,
                       g_queue_peek_tail_link (path->strokes));

  path->last_stroke_id++;
  gimp_stroke_set_id (stroke, path->last_stroke_id);
}

void
gimp_path_stroke_remove (GimpPath    *path,
                         GimpStroke  *stroke)
{
  g_return_if_fail (GIMP_IS_PATH (path));
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  gimp_path_freeze (path);

  GIMP_PATH_GET_CLASS (path)->stroke_remove (path, stroke);

  gimp_path_thaw (path);
}

static void
gimp_path_real_stroke_remove (GimpPath    *path,
                              GimpStroke  *stroke)
{
  GList *list = g_hash_table_lookup (path->stroke_to_list, stroke);

  if (list)
    {
      g_queue_delete_link (path->strokes, list);
      g_hash_table_remove (path->stroke_to_list, stroke);
      g_object_unref (stroke);
    }
}

gint
gimp_path_get_n_strokes (GimpPath *path)
{
  g_return_val_if_fail (GIMP_IS_PATH (path), 0);

  return g_queue_get_length (path->strokes);
}


GimpStroke *
gimp_path_stroke_get (GimpPath         *path,
                      const GimpCoords *coord)
{
  g_return_val_if_fail (GIMP_IS_PATH (path), NULL);

  return GIMP_PATH_GET_CLASS (path)->stroke_get (path, coord);
}

static GimpStroke *
gimp_path_real_stroke_get (GimpPath         *path,
                           const GimpCoords *coord)
{
  GimpStroke *minstroke = NULL;
  gdouble     mindist   = G_MAXDOUBLE;
  GList      *list;

  for (list = path->strokes->head; list; list = g_list_next (list))
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
gimp_path_stroke_get_by_id (GimpPath *path,
                            gint      id)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_PATH (path), NULL);

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      if (gimp_stroke_get_id (list->data) == id)
        return list->data;
    }

  return NULL;
}


GimpStroke *
gimp_path_stroke_get_next (GimpPath    *path,
                           GimpStroke  *prev)
{
  g_return_val_if_fail (GIMP_IS_PATH (path), NULL);

  return GIMP_PATH_GET_CLASS (path)->stroke_get_next (path, prev);
}

static GimpStroke *
gimp_path_real_stroke_get_next (GimpPath    *path,
                                GimpStroke  *prev)
{
  if (! prev)
    {
      return g_queue_peek_head (path->strokes);
    }
  else
    {
      GList *stroke = g_hash_table_lookup (path->stroke_to_list, prev);

      g_return_val_if_fail (stroke != NULL, NULL);

      return stroke->next ? stroke->next->data : NULL;
    }
}


gdouble
gimp_path_stroke_get_length (GimpPath    *path,
                             GimpStroke  *stroke)
{
  g_return_val_if_fail (GIMP_IS_PATH (path), 0.0);
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), 0.0);

  return GIMP_PATH_GET_CLASS (path)->stroke_get_length (path, stroke);
}

static gdouble
gimp_path_real_stroke_get_length (GimpPath    *path,
                                  GimpStroke  *stroke)
{
  g_return_val_if_fail (GIMP_IS_PATH (path), 0.0);
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), 0.0);

  return gimp_stroke_get_length (stroke, path->precision);
}


GimpAnchor *
gimp_path_anchor_get (GimpPath          *path,
                      const GimpCoords  *coord,
                      GimpStroke       **ret_stroke)
{
  g_return_val_if_fail (GIMP_IS_PATH (path), NULL);

  return GIMP_PATH_GET_CLASS (path)->anchor_get (path, coord,
                                                    ret_stroke);
}

static GimpAnchor *
gimp_path_real_anchor_get (GimpPath          *path,
                           const GimpCoords  *coord,
                           GimpStroke       **ret_stroke)
{
  GimpAnchor *minanchor = NULL;
  gdouble     mindist   = -1;
  GList      *list;

  for (list = path->strokes->head; list; list = g_list_next (list))
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
gimp_path_anchor_delete (GimpPath    *path,
                         GimpAnchor  *anchor)
{
  g_return_if_fail (GIMP_IS_PATH (path));
  g_return_if_fail (anchor != NULL);

  GIMP_PATH_GET_CLASS (path)->anchor_delete (path, anchor);
}

static void
gimp_path_real_anchor_delete (GimpPath    *path,
                              GimpAnchor  *anchor)
{
}


void
gimp_path_anchor_select (GimpPath    *path,
                         GimpStroke  *target_stroke,
                         GimpAnchor  *anchor,
                         gboolean     selected,
                         gboolean     exclusive)
{
  GList *list;

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_anchor_select (stroke,
                                 stroke == target_stroke ? anchor : NULL,
                                 selected, exclusive);
    }
}


gdouble
gimp_path_get_length (GimpPath         *path,
                      const GimpAnchor *start)
{
  g_return_val_if_fail (GIMP_IS_PATH (path), 0.0);

  return GIMP_PATH_GET_CLASS (path)->get_length (path, start);
}

static gdouble
gimp_path_real_get_length (GimpPath         *path,
                           const GimpAnchor *start)
{
  g_printerr ("gimp_path_get_length: default implementation\n");

  return 0;
}


gdouble
gimp_path_get_distance (GimpPath         *path,
                        const GimpCoords *coord)
{
  g_return_val_if_fail (GIMP_IS_PATH (path), 0.0);

  return GIMP_PATH_GET_CLASS (path)->get_distance (path, coord);
}

static gdouble
gimp_path_real_get_distance (GimpPath         *path,
                             const GimpCoords *coord)
{
  g_printerr ("gimp_path_get_distance: default implementation\n");

  return 0;
}

gint
gimp_path_interpolate (GimpPath    *path,
                       GimpStroke  *stroke,
                       gdouble      precision,
                       gint         max_points,
                       GimpCoords  *ret_coords)
{
  g_return_val_if_fail (GIMP_IS_PATH (path), 0);

  return GIMP_PATH_GET_CLASS (path)->interpolate (path, stroke,
                                                  precision, max_points,
                                                  ret_coords);
}

static gint
gimp_path_real_interpolate (GimpPath    *path,
                            GimpStroke  *stroke,
                            gdouble      precision,
                            gint         max_points,
                            GimpCoords  *ret_coords)
{
  g_printerr ("gimp_path_interpolate: default implementation\n");

  return 0;
}

const GimpBezierDesc *
gimp_path_get_bezier (GimpPath *path)
{
  g_return_val_if_fail (GIMP_IS_PATH (path), NULL);

  if (! path->bezier_desc)
    {
      path->bezier_desc = gimp_path_make_bezier (path);
    }

  return  path->bezier_desc;
}

static GimpBezierDesc *
gimp_path_make_bezier (GimpPath *path)
{
  return GIMP_PATH_GET_CLASS (path)->make_bezier (path);
}

static GimpBezierDesc *
gimp_path_real_make_bezier (GimpPath *path)
{
  GimpStroke     *stroke;
  GArray         *cmd_array;
  GimpBezierDesc *ret_bezdesc = NULL;

  cmd_array = g_array_new (FALSE, FALSE, sizeof (cairo_path_data_t));

  for (stroke = gimp_path_stroke_get_next (path, NULL);
       stroke;
       stroke = gimp_path_stroke_get_next (path, stroke))
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
