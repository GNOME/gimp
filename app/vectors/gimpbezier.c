/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbezier.c
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

#include "gimpanchor.h"
#include "gimpbezier.h"


/* private BezierStroke struct */

typedef struct _GimpBezierStroke GimpBezierStroke;

struct _GimpBezierStroke
{
  gboolean   closed;

  GList    * controls; /* of GimpBezierNodes */
};

typedef struct _GimpBezierNode GimpBezierNode;

struct _GimpBezierNode
{
  GimpAnchor * ctrl_prev;
  GimpAnchor * ctrl;
  GimpAnchor * ctrl_next;
};


/*  private variables  */

static GimpVectorsClass *parent_class = NULL;


static void
gimp_bezier_finalize (GObject *object)
{
    /* blablabla */
}

/*  local function prototypes  */
static void     gimp_bezier_init		(GimpBezier  *vectors);

static void
gimp_bezier_class_init (GimpBezierClass *klass)
{
  GObjectClass      *object_class;
  GimpObjectClass   *gimp_object_class;
  GimpViewableClass *viewable_class;
  GimpVectorsClass  *vectors_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);
  viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  vectors_class     = GIMP_VECTORS_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  vectors_class->anchor_set = gimp_bezier_anchor_set;
  vectors_class->anchor_get_next = gimp_bezier_anchor_get_next;
  vectors_class->anchor_move_absolute = gimp_bezier_anchor_move_absolute;
  /* Lots of pointers missing */

  object_class->finalize = gimp_bezier_finalize;
}


GType
gimp_bezier_get_type (void)
{
  static GType bezier_type = 0;

  if (! bezier_type)
    {
      static const GTypeInfo bezier_info =
      {
        sizeof (GimpBezierClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_bezier_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpBezier),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_bezier_init,
      };

      bezier_type = g_type_register_static (GIMP_TYPE_VECTORS,
                                            "GimpBezier", 
                                            &bezier_info, 0);
    }

  return bezier_type;
}


static void
gimp_bezier_init		(GimpBezier	   *bezier)
{
  bezier->anchors = NULL;
  bezier->strokes = NULL;
};
  

GimpAnchor *
gimp_bezier_anchor_get (const GimpVectors  *vectors,
			const GimpCoords   *coord)
{
  gdouble     dx, dy, mindist;
  GList      *list;
  GimpBezier *bezier;
  GimpAnchor *anchor = NULL;

  g_return_val_if_fail (GIMP_IS_BEZIER (vectors), NULL);
  bezier = GIMP_BEZIER(vectors);

  list = bezier->anchors;
  if (list) {
    dx = coord->x - ((GimpAnchor *) list->data)->position.x;
    dy = coord->y - ((GimpAnchor *) list->data)->position.y;
    anchor = (GimpAnchor *) list->data;
    mindist = dx * dx + dy * dy;
    list = list->next;
  }

  while (list) {
    dx = coord->x - ((GimpAnchor *) list->data)->position.x;
    dy = coord->y - ((GimpAnchor *) list->data)->position.y;
    if (mindist > dx * dx + dy * dy) {
      mindist = dx * dx + dy * dy;
      anchor = (GimpAnchor *) list->data;
    }
    list = list->next;
  }

  return anchor;
}


/* prev == NULL: "first" anchor */
GimpAnchor *
gimp_bezier_anchor_get_next (const GimpVectors  *vectors,
			     const GimpAnchor   *prev)
{
  static GList *last_shown = NULL;
  GimpBezier   *bezier;

  g_return_val_if_fail (GIMP_IS_BEZIER (vectors), NULL);
  bezier = GIMP_BEZIER(vectors);

  if (!prev) {
    last_shown = bezier->anchors;
    return (GimpAnchor *) last_shown->data;
  } else {
    if (!last_shown || last_shown->data != prev) {
      last_shown = g_list_find (bezier->anchors, prev);
    }
    last_shown = last_shown->next;
  }

  if (last_shown)
    return (GimpAnchor *) last_shown->data;
  else
    return NULL;
}

GimpAnchor *
gimp_bezier_anchor_set (const GimpVectors  *vectors,
			const GimpCoords   *coord,
                        const gboolean      new_stroke)
{
  GimpBezier *bezier;
  GimpAnchor *anchor;

  g_return_val_if_fail (GIMP_IS_BEZIER (vectors), NULL);
  bezier = GIMP_BEZIER(vectors);

  anchor = g_new0 (GimpAnchor, 1);
  
  anchor->position.x = coord->x;
  anchor->position.y = coord->y;
  anchor->position.pressure = 1;
  anchor->active = FALSE;

  bezier->anchors = g_list_append (bezier->anchors, anchor);

  return anchor;
}


void
gimp_bezier_anchor_move_relative (GimpVectors	   *vectors,
				  GimpAnchor	   *anchor,
				  const GimpCoords *deltacoord,
                                  const gint        type)
{
  GimpBezier *bezier;

  g_return_if_fail (GIMP_IS_BEZIER (vectors));
  bezier = GIMP_BEZIER(vectors);

  if (anchor) {
    anchor->position.x += deltacoord->x;
    anchor->position.y += deltacoord->y;
  }
}


void
gimp_bezier_anchor_move_absolute (GimpVectors	   *vectors,
				  GimpAnchor	   *anchor,
				  const GimpCoords *coord,
                                  const gint        type)
{
  GimpBezier *bezier;

  g_return_if_fail (GIMP_IS_BEZIER (vectors));
  bezier = GIMP_BEZIER(vectors);

  if (anchor) {
    anchor->position.x = coord->x;
    anchor->position.y = coord->y;
  }
}

void
gimp_bezier_anchor_delete (GimpVectors	   *vectors,
                           GimpAnchor	   *anchor);


/* accessing the shape of the curve */

gdouble
gimp_bezier_get_length (const GimpVectors *vectors,
                        const GimpAnchor  *start);


/* returns the number of valid coordinates */
gint
gimp_bezier_interpolate	(const GimpVectors *vectors,
                         const GimpStroke  *start,
                         const gdouble      precision,
                         const gint         max_points,
                         GimpCoords 	   *ret_coords);

/* Allow a singular temorary anchor (marking the "working point")? */

GimpAnchor *
gimp_bezier_temp_anchor_get (const GimpVectors  *vectors);

GimpAnchor *
gimp_bezier_temp_anchor_set (GimpVectors        *vectors,
                             const GimpCoords   *coord);

gboolean
gimp_bezier_temp_anchor_fix (GimpVectors	   *vectors);

void
gimp_bezier_temp_anchor_delete (GimpVectors	   *vectors);

