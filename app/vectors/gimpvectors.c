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

#include "gimpanchor.h"
#include "gimpstroke.h"
#include "gimpvectors.h"
#include "gimpvectors-preview.h"


static void       gimp_vectors_class_init  (GimpVectorsClass *klass);
static void       gimp_vectors_init        (GimpVectors      *vectors);

static void       gimp_vectors_finalize    (GObject          *object);

static gsize      gimp_vectors_get_memsize (GimpObject       *object);

static GimpItem * gimp_vectors_duplicate   (GimpItem         *item,
                                            GType             new_type,
                                            gboolean          add_alpha);


/*  private variables  */

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

  object_class->finalize          = gimp_vectors_finalize;

  gimp_object_class->get_memsize  = gimp_vectors_get_memsize;

  viewable_class->get_new_preview = gimp_vectors_get_new_preview;

  item_class->duplicate           = gimp_vectors_duplicate;

  klass->changed                  = NULL;

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
  vectors->strokes = NULL;
};

static void
gimp_vectors_finalize (GObject *object)
{
#ifdef __GNUC__
#warning FIXME: implement gimp_vectors_finalize()
#endif

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gsize
gimp_vectors_get_memsize (GimpObject *object)
{
  GimpVectors *vectors;
  GimpStroke  *stroke;
  gsize        memsize = 0;

  vectors = GIMP_VECTORS (object);

  for (stroke = vectors->strokes; stroke; stroke = stroke->next)
    memsize += gimp_object_get_memsize (GIMP_OBJECT (stroke));

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

  return new_item;;
}


/*  public functions  */

GimpVectors *
gimp_vectors_new (GimpImage   *gimage,
		  const gchar *name)
{
  GimpVectors *vectors;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  vectors = g_object_new (GIMP_TYPE_VECTORS, NULL);

  gimp_item_configure (GIMP_ITEM (vectors), gimage, name);

  return vectors;
}

void
gimp_vectors_copy_strokes (const GimpVectors *src_vectors,
                           GimpVectors       *dest_vectors)
{
  g_return_if_fail (GIMP_IS_VECTORS (src_vectors));
  g_return_if_fail (GIMP_IS_VECTORS (dest_vectors));

#ifdef __GNUC__
#warning FIXME: implement gimp_vectors_copy_strokes()
#endif
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
      GimpStroke *stroke;
      GimpAnchor *anchor = NULL, *minanchor = NULL;

      mindist = -1;

      for (stroke = vectors->strokes; stroke; stroke = stroke->next)
        {
          anchor = gimp_stroke_anchor_get (stroke, coord);
          if (anchor)
            {
              dx = coord->x - anchor->position.x;
              dy = coord->y - anchor->position.y;
              if (mindist > dx * dx + dy * dy || mindist < 0)
                {
                  mindist = dx * dx + dy * dy;
                  minanchor = anchor;
                  if (ret_stroke)
                    *ret_stroke = stroke;
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

  if (vectors_class->stroke_add)
    {
      vectors_class->stroke_add (vectors, stroke);
    }
  else
    {
      stroke->next = vectors->strokes;
      vectors->strokes = stroke;
      g_object_ref (stroke);
    }
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
      GimpStroke *stroke;
      GimpStroke *minstroke = NULL;
      GimpAnchor *anchor = NULL;

      mindist = -1;
      stroke = GIMP_STROKE (vectors->strokes);

      while (stroke)
        {
          anchor = gimp_stroke_anchor_get (stroke, coord);
          if (anchor)
            {
              dx = coord->x - anchor->position.x;
              dy = coord->y - anchor->position.y;
              if (mindist > dx * dx + dy * dy || mindist < 0)
                {
                  mindist = dx * dx + dy * dy;
                  minstroke = stroke;
                }
            }
          stroke = stroke->next;
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
          return vectors->strokes;
        }
      else
        {
          g_return_val_if_fail (GIMP_IS_STROKE  (prev), NULL);
          return prev->next;
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

