/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpvectors.c
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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
#include "libgimpmath/gimpmath.h"

#include "vectors-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable-stroke.h"
#include "core/gimp-transform-utils.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpmarshal.h"
#include "core/gimppaintinfo.h"
#include "core/gimpstrokedesc.h"

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


static void       gimp_vectors_finalize     (GObject          *object);

static gint64     gimp_vectors_get_memsize  (GimpObject       *object,
                                             gint64           *gui_size);

static gboolean   gimp_vectors_is_attached  (GimpItem         *item);
static GimpItem * gimp_vectors_duplicate    (GimpItem         *item,
                                             GType             new_type,
                                             gboolean          add_alpha);
static void       gimp_vectors_convert      (GimpItem         *item,
                                             GimpImage        *dest_image);
static void       gimp_vectors_translate    (GimpItem         *item,
                                             gint              offset_x,
                                             gint              offset_y,
                                             gboolean          push_undo);
static void       gimp_vectors_scale        (GimpItem         *item,
                                             gint              new_width,
                                             gint              new_height,
                                             gint              new_offset_x,
                                             gint              new_offset_y,
                                             GimpInterpolationType  interp_type,
                                             GimpProgress     *progress);
static void       gimp_vectors_resize       (GimpItem         *item,
                                             GimpContext      *context,
                                             gint              new_width,
                                             gint              new_height,
                                             gint              offset_x,
                                             gint              offset_y);
static void       gimp_vectors_flip         (GimpItem         *item,
                                             GimpContext      *context,
                                             GimpOrientationType  flip_type,
                                             gdouble           axis,
                                             gboolean          clip_result);
static void       gimp_vectors_rotate       (GimpItem         *item,
                                             GimpContext      *context,
                                             GimpRotationType  rotate_type,
                                             gdouble           center_x,
                                             gdouble           center_y,
                                             gboolean          clip_result);
static void       gimp_vectors_transform    (GimpItem         *item,
                                             GimpContext      *context,
                                             const GimpMatrix3 *matrix,
                                             GimpTransformDirection direction,
                                             GimpInterpolationType interp_type,
                                             gint              recursion_level,
                                             GimpTransformResize   clip_result,
                                             GimpProgress     *progress);
static gboolean   gimp_vectors_stroke       (GimpItem         *item,
                                             GimpDrawable     *drawable,
                                             GimpStrokeDesc   *stroke_desc,
                                             GimpProgress     *progress);

static void       gimp_vectors_real_thaw            (GimpVectors       *vectors);
static void       gimp_vectors_real_stroke_add      (GimpVectors       *vectors,
                                                     GimpStroke        *stroke);
static void       gimp_vectors_real_stroke_remove   (GimpVectors       *vectors,
                                                     GimpStroke        *stroke);
static GimpStroke * gimp_vectors_real_stroke_get    (const GimpVectors *vectors,
                                                     const GimpCoords  *coord);
static GimpStroke *gimp_vectors_real_stroke_get_next(const GimpVectors *vectors,
                                                     const GimpStroke  *prev);
static gdouble gimp_vectors_real_stroke_get_length  (const GimpVectors *vectors,
                                                     const GimpStroke  *prev);
static GimpAnchor * gimp_vectors_real_anchor_get    (const GimpVectors *vectors,
                                                     const GimpCoords  *coord,
                                                     GimpStroke       **ret_stroke);
static void       gimp_vectors_real_anchor_delete   (GimpVectors       *vectors,
                                                     GimpAnchor        *anchor);
static gdouble    gimp_vectors_real_get_length      (const GimpVectors *vectors,
                                                     const GimpAnchor  *start);
static gdouble    gimp_vectors_real_get_distance    (const GimpVectors *vectors,
                                                     const GimpCoords  *coord);
static gint       gimp_vectors_real_interpolate     (const GimpVectors *vectors,
                                                     const GimpStroke  *stroke,
                                                     gdouble            precision,
                                                     gint               max_points,
                                                     GimpCoords        *ret_coords);
static GimpVectors * gimp_vectors_real_make_bezier  (const GimpVectors *vectors);


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
                  G_SIGNAL_RUN_FIRST,
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

  object_class->finalize           = gimp_vectors_finalize;

  gimp_object_class->get_memsize   = gimp_vectors_get_memsize;

  viewable_class->get_new_preview  = gimp_vectors_get_new_preview;
  viewable_class->default_stock_id = "gimp-path";

  item_class->is_attached          = gimp_vectors_is_attached;
  item_class->duplicate            = gimp_vectors_duplicate;
  item_class->convert              = gimp_vectors_convert;
  item_class->translate            = gimp_vectors_translate;
  item_class->scale                = gimp_vectors_scale;
  item_class->resize               = gimp_vectors_resize;
  item_class->flip                 = gimp_vectors_flip;
  item_class->rotate               = gimp_vectors_rotate;
  item_class->transform            = gimp_vectors_transform;
  item_class->stroke               = gimp_vectors_stroke;
  item_class->default_name         = _("Path");
  item_class->rename_desc          = _("Rename Path");
  item_class->translate_desc       = _("Move Path");
  item_class->scale_desc           = _("Scale Path");
  item_class->resize_desc          = _("Resize Path");
  item_class->flip_desc            = _("Flip Path");
  item_class->rotate_desc          = _("Rotate Path");
  item_class->transform_desc       = _("Transform Path");
  item_class->stroke_desc          = _("Stroke Path");

  klass->freeze                    = NULL;
  klass->thaw                      = gimp_vectors_real_thaw;

  klass->stroke_add                = gimp_vectors_real_stroke_add;
  klass->stroke_remove             = gimp_vectors_real_stroke_remove;
  klass->stroke_get                = gimp_vectors_real_stroke_get;
  klass->stroke_get_next           = gimp_vectors_real_stroke_get_next;
  klass->stroke_get_length         = gimp_vectors_real_stroke_get_length;

  klass->anchor_get                = gimp_vectors_real_anchor_get;
  klass->anchor_delete             = gimp_vectors_real_anchor_delete;

  klass->get_length                = gimp_vectors_real_get_length;
  klass->get_distance              = gimp_vectors_real_get_distance;
  klass->interpolate               = gimp_vectors_real_interpolate;

  klass->make_bezier               = gimp_vectors_real_make_bezier;
}

static void
gimp_vectors_init (GimpVectors *vectors)
{
  GimpItem *item = GIMP_ITEM (vectors);

  item->visible           = FALSE;
  vectors->strokes        = NULL;
  vectors->last_stroke_ID = 0;
  vectors->freeze_count   = 0;
  vectors->precision      = 0.2;
}

static void
gimp_vectors_finalize (GObject *object)
{
  GimpVectors *vectors = GIMP_VECTORS (object);

  if (vectors->strokes)
    {
      g_list_foreach (vectors->strokes, (GFunc) g_object_unref, NULL);
      g_list_free (vectors->strokes);
      vectors->strokes = NULL;
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

  for (list = vectors->strokes; list; list = g_list_next (list))
    memsize += (gimp_object_get_memsize (GIMP_OBJECT (list->data), gui_size) +
                sizeof (GList));

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_vectors_is_attached (GimpItem *item)
{
  return (GIMP_IS_IMAGE (item->image) &&
          gimp_container_have (item->image->vectors, GIMP_OBJECT (item)));
}

static GimpItem *
gimp_vectors_duplicate (GimpItem *item,
                        GType     new_type,
                        gboolean  add_alpha)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_VECTORS), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type,
                                                        add_alpha);

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
                      GimpImage *dest_image)
{
  item->width  = dest_image->width;
  item->height = dest_image->height;

  GIMP_ITEM_CLASS (parent_class)->convert (item, dest_image);
}

static void
gimp_vectors_translate (GimpItem *item,
                        gint      offset_x,
                        gint      offset_y,
                        gboolean  push_undo)
{
  GimpVectors *vectors = GIMP_VECTORS (item);
  GList       *list;

  gimp_vectors_freeze (vectors);

  if (push_undo)
    gimp_image_undo_push_vectors_mod (gimp_item_get_image (item),
                                      _("Move Path"),
                                      vectors);

  for (list = vectors->strokes; list; list = g_list_next (list))
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

  for (list = vectors->strokes; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_scale (stroke,
                         (gdouble) new_width  / (gdouble) item->width,
                         (gdouble) new_height / (gdouble) item->height);
      gimp_stroke_translate (stroke, new_offset_x, new_offset_y);
    }

  GIMP_ITEM_CLASS (parent_class)->scale (item, image->width, image->height,
                                         0, 0, interpolation_type, progress);

  gimp_vectors_thaw (vectors);
}

static void
gimp_vectors_resize (GimpItem    *item,
                     GimpContext *context,
                     gint         new_width,
                     gint         new_height,
                     gint         offset_x,
                     gint         offset_y)
{
  GimpVectors *vectors = GIMP_VECTORS (item);
  GimpImage   *image   = gimp_item_get_image (item);
  GList       *list;

  gimp_vectors_freeze (vectors);

  if (gimp_item_is_attached (item))
    gimp_image_undo_push_vectors_mod (image, NULL, vectors);

  for (list = vectors->strokes; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_translate (stroke, offset_x, offset_y);
    }

  GIMP_ITEM_CLASS (parent_class)->resize (item, context,
                                          image->width, image->height, 0, 0);

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

  for (list = vectors->strokes; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_transform (stroke, &matrix);
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

  for (list = vectors->strokes; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_transform (stroke, &matrix);
    }

  gimp_vectors_thaw (vectors);
}

static void
gimp_vectors_transform (GimpItem               *item,
                        GimpContext            *context,
                        const GimpMatrix3      *matrix,
                        GimpTransformDirection  direction,
                        GimpInterpolationType   interpolation_type,
                        gint                    recursion_level,
                        GimpTransformResize     clip_result,
                        GimpProgress           *progress)
{
  GimpVectors *vectors = GIMP_VECTORS (item);
  GimpMatrix3  local_matrix;
  GList       *list;

  gimp_vectors_freeze (vectors);

  gimp_image_undo_push_vectors_mod (gimp_item_get_image (item),
                                    _("Transform Path"),
                                    vectors);

  local_matrix = *matrix;

  if (direction == GIMP_TRANSFORM_BACKWARD)
    gimp_matrix3_invert (&local_matrix);

  for (list = vectors->strokes; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_stroke_transform (stroke, &local_matrix);
    }

  gimp_vectors_thaw (vectors);
}

static gboolean
gimp_vectors_stroke (GimpItem       *item,
                     GimpDrawable   *drawable,
                     GimpStrokeDesc *stroke_desc,
                     GimpProgress   *progress)
{
  GimpVectors *vectors = GIMP_VECTORS (item);
  gboolean     retval  = FALSE;

  /*  return successfully on an empty path, there's nothing to stroke  */
  if (! vectors->strokes)
    return TRUE;

  switch (stroke_desc->method)
    {
    case GIMP_STROKE_METHOD_LIBART:
      gimp_drawable_stroke_vectors (drawable,
                                    stroke_desc->stroke_options,
                                    vectors);
      retval = TRUE;
      break;

    case GIMP_STROKE_METHOD_PAINT_CORE:
      {
        GimpPaintCore *core;

        core = g_object_new (stroke_desc->paint_info->paint_type, NULL);

        retval = gimp_paint_core_stroke_vectors (core, drawable,
                                                 stroke_desc->paint_options,
                                                 vectors);

        g_object_unref (core);
      }
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  return retval;
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

  vectors = g_object_new (GIMP_TYPE_VECTORS, NULL);

  gimp_item_configure (GIMP_ITEM (vectors), image,
                       0, 0, image->width, image->height,
                       name);

  return vectors;
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
gimp_vectors_copy_strokes (const GimpVectors *src_vectors,
                           GimpVectors       *dest_vectors)
{
  g_return_if_fail (GIMP_IS_VECTORS (src_vectors));
  g_return_if_fail (GIMP_IS_VECTORS (dest_vectors));

  gimp_vectors_freeze (dest_vectors);

  if (dest_vectors->strokes)
    {
      g_list_foreach (dest_vectors->strokes, (GFunc) g_object_unref, NULL);
      g_list_free (dest_vectors->strokes);
    }

  dest_vectors->strokes = NULL;
  dest_vectors->last_stroke_ID = 0;

  gimp_vectors_add_strokes (src_vectors, dest_vectors);

  gimp_vectors_thaw (dest_vectors);
}


void
gimp_vectors_add_strokes (const GimpVectors *src_vectors,
                          GimpVectors       *dest_vectors)
{
  GList *current_lstroke;
  GList *strokes_copy;

  g_return_if_fail (GIMP_IS_VECTORS (src_vectors));
  g_return_if_fail (GIMP_IS_VECTORS (dest_vectors));

  gimp_vectors_freeze (dest_vectors);

  strokes_copy = g_list_copy (src_vectors->strokes);
  current_lstroke = strokes_copy;

  while (current_lstroke)
    {
      current_lstroke->data = gimp_stroke_duplicate (current_lstroke->data);
      dest_vectors->last_stroke_ID ++;
      gimp_stroke_set_ID (current_lstroke->data,
                          dest_vectors->last_stroke_ID);
      current_lstroke = g_list_next (current_lstroke);
    }

  dest_vectors->strokes = g_list_concat (dest_vectors->strokes, strokes_copy);

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
  /*  Don't g_list_prepend() here.  See ChangeLog 2003-05-21 --Mitch  */

  vectors->strokes = g_list_append (vectors->strokes, stroke);
  vectors->last_stroke_ID ++;
  gimp_stroke_set_ID (stroke, vectors->last_stroke_ID);
  g_object_ref (stroke);
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
  GList *list;

  list = g_list_find (vectors->strokes, stroke);

  if (list)
    {
      vectors->strokes = g_list_delete_link (vectors->strokes, list);
      g_object_unref (stroke);
    }
}

gint
gimp_vectors_get_n_strokes (const GimpVectors *vectors)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0);

  return g_list_length (vectors->strokes);
}


GimpStroke *
gimp_vectors_stroke_get (const GimpVectors *vectors,
                         const GimpCoords  *coord)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  return GIMP_VECTORS_GET_CLASS (vectors)->stroke_get (vectors, coord);
}

static GimpStroke *
gimp_vectors_real_stroke_get (const GimpVectors *vectors,
                              const GimpCoords  *coord)
{
  GList      *stroke;
  gdouble     mindist   = G_MAXDOUBLE;
  GimpStroke *minstroke = NULL;

  for (stroke = vectors->strokes; stroke; stroke = g_list_next (stroke))
    {
      GimpAnchor *anchor = gimp_stroke_anchor_get (stroke->data, coord);

      if (anchor)
        {
          gdouble dx, dy;

          dx = coord->x - anchor->position.x;
          dy = coord->y - anchor->position.y;

          if (mindist > dx * dx + dy * dy)
            {
              mindist   = dx * dx + dy * dy;
              minstroke = GIMP_STROKE (stroke->data);
            }
        }
    }

  return minstroke;
}

GimpStroke *
gimp_vectors_stroke_get_by_ID (const GimpVectors *vectors,
                               gint               id)
{
  GList      *stroke;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  for (stroke = vectors->strokes; stroke; stroke = g_list_next (stroke))
    {
      if (gimp_stroke_get_ID (stroke->data) == id)
        return stroke->data;
    }

  return NULL;
}


GimpStroke *
gimp_vectors_stroke_get_next (const GimpVectors *vectors,
                              const GimpStroke  *prev)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  return GIMP_VECTORS_GET_CLASS (vectors)->stroke_get_next (vectors, prev);
}

static GimpStroke *
gimp_vectors_real_stroke_get_next (const GimpVectors *vectors,
                                   const GimpStroke  *prev)
{
  if (! prev)
    {
      return vectors->strokes ? vectors->strokes->data : NULL;
    }
  else
    {
      GList *stroke;

      stroke = g_list_find (vectors->strokes, prev);

      g_return_val_if_fail (stroke != NULL, NULL);

      return stroke->next ? GIMP_STROKE (stroke->next->data) : NULL;
    }
}


gdouble
gimp_vectors_stroke_get_length (const GimpVectors *vectors,
                                const GimpStroke  *stroke)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0.0);
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), 0.0);

  return GIMP_VECTORS_GET_CLASS (vectors)->stroke_get_length (vectors, stroke);
}

static gdouble
gimp_vectors_real_stroke_get_length (const GimpVectors *vectors,
                                     const GimpStroke  *stroke)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0.0);
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), 0.0);

  return (gimp_stroke_get_length (stroke, vectors->precision));

  return 0.0;
}


GimpAnchor *
gimp_vectors_anchor_get (const GimpVectors *vectors,
                         const GimpCoords  *coord,
                         GimpStroke       **ret_stroke)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  return GIMP_VECTORS_GET_CLASS (vectors)->anchor_get (vectors, coord,
                                                       ret_stroke);
}

static GimpAnchor *
gimp_vectors_real_anchor_get (const GimpVectors *vectors,
                              const GimpCoords  *coord,
                              GimpStroke       **ret_stroke)
{
  gdouble     dx, dy, mindist;
  GList      *stroke;
  GimpAnchor *anchor    = NULL;
  GimpAnchor *minanchor = NULL;

  mindist = -1;

  for (stroke = vectors->strokes; stroke; stroke = g_list_next (stroke))
    {
      anchor = gimp_stroke_anchor_get (GIMP_STROKE (stroke->data), coord);

      if (anchor)
        {
          dx = coord->x - anchor->position.x;
          dy = coord->y - anchor->position.y;

          if (mindist > dx * dx + dy * dy || mindist < 0)
            {
              mindist   = dx * dx + dy * dy;
              minanchor = anchor;

              if (ret_stroke)
                *ret_stroke = stroke->data;
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
  GList      *stroke_list;
  GimpStroke *stroke;

  for (stroke_list = vectors->strokes; stroke_list;
       stroke_list = g_list_next (stroke_list))
    {
      stroke = GIMP_STROKE (stroke_list->data);
      gimp_stroke_anchor_select (stroke,
                                 stroke == target_stroke ? anchor : NULL,
                                 selected, exclusive);
    }
}


gdouble
gimp_vectors_get_length (const GimpVectors *vectors,
                         const GimpAnchor  *start)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0.0);

  return GIMP_VECTORS_GET_CLASS (vectors)->get_length (vectors, start);
}

static gdouble
gimp_vectors_real_get_length (const GimpVectors *vectors,
                              const GimpAnchor  *start)
{
  g_printerr ("gimp_vectors_get_length: default implementation\n");

  return 0;
}


gdouble
gimp_vectors_get_distance (const GimpVectors *vectors,
                           const GimpCoords  *coord)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0.0);

  return GIMP_VECTORS_GET_CLASS (vectors)->get_distance (vectors, coord);
}

static gdouble
gimp_vectors_real_get_distance (const GimpVectors *vectors,
                                const GimpCoords  *coord)
{
  g_printerr ("gimp_vectors_get_distance: default implementation\n");

  return 0;
}

gboolean
gimp_vectors_bounds (const GimpVectors  *vectors,
                     gdouble            *x1,
                     gdouble            *y1,
                     gdouble            *x2,
                     gdouble            *y2)
{
  GArray     *stroke_coords;
  GimpStroke *cur_stroke;
  gint        i;
  gboolean    has_strokes = FALSE;
  gboolean    closed;
  GimpCoords  point;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (x1 != NULL, FALSE);
  g_return_val_if_fail (y1 != NULL, FALSE);
  g_return_val_if_fail (x2 != NULL, FALSE);
  g_return_val_if_fail (y2 != NULL, FALSE);

  for (cur_stroke = gimp_vectors_stroke_get_next (vectors, NULL);
       cur_stroke;
       cur_stroke = gimp_vectors_stroke_get_next (vectors, cur_stroke))
    {
      stroke_coords = gimp_stroke_interpolate (cur_stroke, 1.0, &closed);

      if (stroke_coords)
        {
          if (! has_strokes && stroke_coords->len > 0)
            {
              has_strokes = TRUE;
              point = g_array_index (stroke_coords, GimpCoords, 0);
              *x1 = *x2 = point.x;
              *y1 = *y2 = point.y;
            }

          for (i=0; i < stroke_coords->len; i++)
            {
              point = g_array_index (stroke_coords, GimpCoords, i);
              *x1 = MIN (*x1, point.x);
              *y1 = MIN (*y1, point.y);
              *x2 = MAX (*x2, point.x);
              *y2 = MAX (*y2, point.y);
            }
          g_array_free (stroke_coords, TRUE);
        }
    }

  return has_strokes;
}

gint
gimp_vectors_interpolate (const GimpVectors *vectors,
                          const GimpStroke  *stroke,
                          gdouble            precision,
                          gint               max_points,
                          GimpCoords        *ret_coords)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0);

  return GIMP_VECTORS_GET_CLASS (vectors)->interpolate (vectors, stroke,
                                                        precision, max_points,
                                                        ret_coords);
}

static gint
gimp_vectors_real_interpolate (const GimpVectors *vectors,
                               const GimpStroke  *stroke,
                               gdouble            precision,
                               gint               max_points,
                               GimpCoords        *ret_coords)
{
  g_printerr ("gimp_vectors_interpolate: default implementation\n");

  return 0;
}


GimpVectors *
gimp_vectors_make_bezier (const GimpVectors *vectors)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  return GIMP_VECTORS_GET_CLASS (vectors)->make_bezier (vectors);
}

static GimpVectors *
gimp_vectors_real_make_bezier (const GimpVectors *vectors)
{
  g_printerr ("gimp_vectors_make_bezier: default implementation\n");

  return NULL;
}

