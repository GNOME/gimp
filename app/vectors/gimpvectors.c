/* The GIMP -- an image manipulation program
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

#include "glib-object.h"

#include "vectors-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpmarshal.h"

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


static void       gimp_vectors_class_init   (GimpVectorsClass *klass);
static void       gimp_vectors_init         (GimpVectors      *vectors);

static void       gimp_vectors_finalize     (GObject          *object);

static gsize      gimp_vectors_get_memsize  (GimpObject       *object);

static GimpItem * gimp_vectors_duplicate    (GimpItem         *item,
                                             GType             new_type,
                                             gboolean          add_alpha);
static void       gimp_vectors_translate    (GimpItem         *item,
                                             gint              offset_x,
                                             gint              offset_y,
                                             gboolean          push_undo);
static void       gimp_vectors_scale        (GimpItem         *item,
                                             gint              new_width,
                                             gint              new_height,
                                             gint              new_offset_x,
                                             gint              new_offset_y,
                                             GimpInterpolationType  interp_type);
static void       gimp_vectors_resize       (GimpItem         *item,
                                             gint              new_width,
                                             gint              new_height,
                                             gint              offset_x,
                                             gint              offset_y);

static void       gimp_vectors_real_thaw (GimpVectors      *vectors);


/*  private variables  */

static guint gimp_vectors_signals[LAST_SIGNAL] = { 0 };

static GimpItemClass *parent_class = NULL;


GType
gimp_vectors_get_type (void)
{
  static GType vectors_type = 0;

  if (! vectors_type)
    {
      static const GTypeInfo vectors_info =
      {
        sizeof (GimpVectorsClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_vectors_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpVectors),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_vectors_init,
      };

      vectors_type = g_type_register_static (GIMP_TYPE_ITEM,
                                             "GimpVectors",
                                             &vectors_info, 0);
    }

  return vectors_type;
}

static void
gimp_vectors_class_init (GimpVectorsClass *klass)
{
  GObjectClass      *object_class;
  GimpObjectClass   *gimp_object_class;
  GimpViewableClass *viewable_class;
  GimpItemClass     *item_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);
  viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  item_class        = GIMP_ITEM_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

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

  object_class->finalize          = gimp_vectors_finalize;

  gimp_object_class->get_memsize  = gimp_vectors_get_memsize;

  viewable_class->get_new_preview = gimp_vectors_get_new_preview;

  item_class->duplicate           = gimp_vectors_duplicate;
  item_class->translate           = gimp_vectors_translate;
  item_class->scale               = gimp_vectors_scale;
  item_class->resize              = gimp_vectors_resize;
  item_class->default_name        = _("Path");
  item_class->rename_desc         = _("Rename Path");

  klass->freeze                   = NULL;
  klass->thaw                     = gimp_vectors_real_thaw;

  klass->stroke_add               = NULL;
  klass->stroke_get		  = NULL;
  klass->stroke_get_next	  = NULL;
  klass->stroke_get_length	  = NULL;

  klass->anchor_get		  = NULL;

  klass->get_length		  = NULL;
  klass->get_distance		  = NULL;
  klass->interpolate		  = NULL;

  klass->make_bezier		  = NULL;
}

static void
gimp_vectors_init (GimpVectors *vectors)
{
  vectors->strokes      = NULL;
  vectors->freeze_count = 0;
};

static void
gimp_vectors_finalize (GObject *object)
{
  GimpVectors *vectors;

  vectors = GIMP_VECTORS (object);

#ifdef __GNUC__
#warning FIXME: implement gimp_vectors_finalize()
#endif

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gsize
gimp_vectors_get_memsize (GimpObject *object)
{
  GimpVectors *vectors;
  GList       *list;
  gsize        memsize = 0;

  vectors = GIMP_VECTORS (object);

  for (list = vectors->strokes; list; list = g_list_next (list))
    memsize += (gimp_object_get_memsize (GIMP_OBJECT (list->data)) +
                sizeof (GList));

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object);
}


static GimpItem *
gimp_vectors_duplicate (GimpItem *item,
                        GType     new_type,
                        gboolean  add_alpha)
{
  GimpVectors *vectors;
  GimpItem    *new_item;
  GimpVectors *new_vectors;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_VECTORS), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type,
                                                        add_alpha);

  if (! GIMP_IS_VECTORS (new_item))
    return new_item;

  vectors     = GIMP_VECTORS (item);
  new_vectors = GIMP_VECTORS (new_item);

  gimp_vectors_copy_strokes (vectors, new_vectors);

  return new_item;
}

static void
gimp_vectors_translate (GimpItem *item,
                        gint      offset_x,
                        gint      offset_y,
                        gboolean  push_undo)
{
  GimpVectors *vectors;
  GList       *list;

  vectors = GIMP_VECTORS (item);

  gimp_vectors_freeze (vectors);

  if (push_undo)
    gimp_image_undo_push_vectors_mod (gimp_item_get_image (item),
                                      _("Move Path"),
                                      vectors);

  for (list = vectors->strokes; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;
      GList      *list2;

      for (list2 = stroke->anchors; list2; list2 = g_list_next (list2))
        {
          GimpAnchor *anchor = list2->data;

          anchor->position.x += offset_x;
          anchor->position.y += offset_y;
        }
    }

  gimp_vectors_thaw (vectors);
}

static void
gimp_vectors_scale (GimpItem              *item,
                    gint                   new_width,
                    gint                   new_height,
                    gint                   new_offset_x,
                    gint                   new_offset_y,
                    GimpInterpolationType  interpolation_type)
{
  GimpVectors *vectors;
  GList       *list;

  vectors = GIMP_VECTORS (item);

  gimp_vectors_freeze (vectors);

  gimp_image_undo_push_vectors_mod (gimp_item_get_image (item),
                                    _("Scale Path"),
                                    vectors);

  for (list = vectors->strokes; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;
      GList      *list2;

      for (list2 = stroke->anchors; list2; list2 = g_list_next (list2))
        {
          GimpAnchor *anchor = list2->data;

          anchor->position.x *= (gdouble) new_width  / (gdouble) item->width;
          anchor->position.y *= (gdouble) new_height / (gdouble) item->height;
        }
    }

  GIMP_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                         new_offset_x, new_offset_y,
                                         interpolation_type);

  gimp_vectors_thaw (vectors);
}

static void
gimp_vectors_resize (GimpItem *item,
                     gint      new_width,
                     gint      new_height,
                     gint      offset_x,
                     gint      offset_y)
{
  GimpVectors *vectors;
  GList       *list;

  vectors = GIMP_VECTORS (item);

  gimp_vectors_freeze (vectors);

  gimp_image_undo_push_vectors_mod (gimp_item_get_image (item),
                                    _("Resize Path"),
                                    vectors);

  for (list = vectors->strokes; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;
      GList      *list2;

      for (list2 = stroke->anchors; list2; list2 = g_list_next (list2))
        {
          GimpAnchor *anchor = list2->data;

          anchor->position.x += offset_x;
          anchor->position.y += offset_y;
        }
    }

  GIMP_ITEM_CLASS (parent_class)->resize (item, new_width, new_height,
                                          offset_x, offset_y);

  gimp_vectors_thaw (vectors);
}

static void 
gimp_vectors_real_thaw (GimpVectors *vectors)
{
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (vectors));
}


/*  public functions  */

GimpVectors *
gimp_vectors_new (GimpImage   *gimage,
		  const gchar *name)
{
  GimpVectors *vectors;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  vectors = g_object_new (GIMP_TYPE_VECTORS, NULL);

  gimp_item_configure (GIMP_ITEM (vectors), gimage,
                       0, 0, gimage->width, gimage->height,
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
  GList *current_lstroke;

  g_return_if_fail (GIMP_IS_VECTORS (src_vectors));
  g_return_if_fail (GIMP_IS_VECTORS (dest_vectors));

  gimp_vectors_freeze (dest_vectors);

  if (dest_vectors->strokes)
    {
#ifdef __GNUC__
#warning FIXME: free old dest_vectors->strokes
#endif
    }

  dest_vectors->strokes = g_list_copy (src_vectors->strokes);
  current_lstroke = dest_vectors->strokes;

  while (current_lstroke)
    {
      current_lstroke->data = gimp_stroke_duplicate (current_lstroke->data);
      current_lstroke = g_list_next (current_lstroke);
    }

  gimp_vectors_thaw (dest_vectors);
}


/* Calling the virtual functions */

GimpAnchor *
gimp_vectors_anchor_get (const GimpVectors *vectors,
                         const GimpCoords  *coord,
                         GimpStroke       **ret_stroke)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->anchor_get)
    return vectors_class->anchor_get (vectors, coord, ret_stroke);
  else
    {
      gdouble     dx, dy, mindist;
      GList      *stroke;
      GimpAnchor *anchor = NULL, *minanchor = NULL;

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
                  mindist = dx * dx + dy * dy;
                  minanchor = anchor;
                  if (ret_stroke)
                    *ret_stroke = stroke->data;
                }
            }
        }
      return minanchor;
    }

  return NULL;
}


void
gimp_vectors_stroke_add (GimpVectors *vectors,
                         GimpStroke  *stroke)
{
  GimpVectorsClass *vectors_class;

  g_return_if_fail (GIMP_IS_VECTORS (vectors));
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  gimp_vectors_freeze (vectors);

  if (vectors_class->stroke_add)
    {
      vectors_class->stroke_add (vectors, stroke);
    }
  else
    {
      vectors->strokes = g_list_prepend (vectors->strokes, stroke);
      g_object_ref (stroke);
    }

  gimp_vectors_thaw (vectors);
}


GimpStroke *
gimp_vectors_stroke_get (const GimpVectors *vectors,
                         const GimpCoords  *coord)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->stroke_get)
    return vectors_class->stroke_get (vectors, coord);
  else
    {
      gdouble     dx, dy, mindist;
      GList      *stroke;
      GimpStroke *minstroke = NULL;
      GimpAnchor *anchor = NULL;

      mindist = -1;
      stroke = vectors->strokes;

      while (stroke)
        {
          anchor = gimp_stroke_anchor_get (stroke->data, coord);
          if (anchor)
            {
              dx = coord->x - anchor->position.x;
              dy = coord->y - anchor->position.y;
              if (mindist > dx * dx + dy * dy || mindist < 0)
                {
                  mindist = dx * dx + dy * dy;
                  minstroke = GIMP_STROKE (stroke->data);
                }
            }
          stroke = g_list_next (stroke);
        }
      return minstroke;
    }

  return NULL;
}


GimpStroke *
gimp_vectors_stroke_get_next (const GimpVectors *vectors,
                              const GimpStroke  *prev)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->stroke_get_next)
    return vectors_class->stroke_get_next (vectors, prev);
  else
    {
      if (!prev)
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

  return NULL;
}


gdouble
gimp_vectors_stroke_get_length (const GimpVectors *vectors,
                                const GimpStroke  *prev)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->stroke_get_length)
    return vectors_class->stroke_get_length (vectors, prev);
  else
    g_printerr ("gimp_vectors_stroke_get_length: default implementation\n");

  return 0;
}


gdouble
gimp_vectors_get_length (const GimpVectors *vectors,
                         const GimpAnchor  *start)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->get_length)
    return vectors_class->get_length (vectors, start);
  else
    g_printerr ("gimp_vectors_get_length: default implementation\n");

  return 0;
}


gdouble
gimp_vectors_get_distance (const GimpVectors *vectors,
			   const GimpCoords  *coord)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->get_distance)
    return vectors_class->get_distance (vectors, coord);
  else
    g_printerr ("gimp_vectors_get_distance: default implementation\n");

  return 0;
}


gint
gimp_vectors_interpolate (const GimpVectors *vectors,
                          const GimpStroke  *stroke,
                          const gdouble      precision,
                          const gint         max_points,
                          GimpCoords        *ret_coords)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->interpolate)
    return vectors_class->interpolate (vectors, stroke, precision, max_points, ret_coords);
  else
    g_printerr ("gimp_vectors_interpolate: default implementation\n");

  return 0;
}


GimpVectors *
gimp_vectors_make_bezier (const GimpVectors *vectors)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->make_bezier)
    return vectors_class->make_bezier (vectors);
  else
    g_printerr ("gimp_vectors_make_bezier: default implementation\n");

  return NULL;
}
