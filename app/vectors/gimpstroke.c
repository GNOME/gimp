/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpstroke.c
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
#include "gimpstroke.h"



/*  private variables  */


static GObjectClass *parent_class = NULL;

/* Prototypes */

static void         gimp_stroke_class_init           (GimpStrokeClass  *klass);
static void         gimp_stroke_init                 (GimpStroke       *stroke);

static void         gimp_stroke_finalize             (GObject          *object);

static GimpAnchor * gimp_stroke_real_anchor_get      (const GimpStroke *stroke,
                                                      const GimpCoords *coord);
static GimpAnchor * gimp_stroke_real_anchor_get_next (const GimpStroke *stroke,
                                                      const GimpAnchor *prev);
static void gimp_stroke_real_anchor_move_relative (GimpStroke       *stroke,
                                                   GimpAnchor       *anchor,
                                                   const GimpCoords *deltacoord,
                                                   const gint        type);
static void gimp_stroke_real_anchor_move_absolute (GimpStroke       *stroke,
                                                   GimpAnchor       *anchor,
                                                   const GimpCoords *deltacoord,
                                                   const gint        type);


GType
gimp_stroke_get_type (void)
{
  static GType stroke_type = 0;

  if (! stroke_type)
    {
      static const GTypeInfo stroke_info =
      {
        sizeof (GimpStrokeClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_stroke_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpStroke),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_stroke_init,
      };

      stroke_type = g_type_register_static (G_TYPE_OBJECT,
                                            "GimpStroke", 
                                            &stroke_info, 0);
    }

  return stroke_type;
}

static void
gimp_stroke_class_init (GimpStrokeClass *klass)
{
  GObjectClass      *object_class;

  object_class      = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize             = gimp_stroke_finalize;

  klass->changed                     = NULL;
  klass->removed                     = NULL;

  klass->anchor_get		     = gimp_stroke_real_anchor_get;
  klass->anchor_get_next	     = gimp_stroke_real_anchor_get_next;
  klass->anchor_move_relative	     = gimp_stroke_real_anchor_move_relative;
  klass->anchor_move_absolute	     = gimp_stroke_real_anchor_move_absolute;
  klass->anchor_delete		     = NULL;

  klass->get_length		     = NULL;
  klass->get_distance		     = NULL;
  klass->interpolate		     = NULL;

  klass->temp_anchor_get	     = NULL;
  klass->temp_anchor_set	     = NULL;
  klass->temp_anchor_fix	     = NULL;

  klass->make_bezier		     = NULL;

  klass->get_draw_anchors            = NULL;
  klass->get_draw_controls           = NULL;
  klass->get_draw_lines              = NULL;
}

static void
gimp_stroke_init (GimpStroke *stroke)
{
  stroke->anchors = NULL;
  stroke->next    = NULL;
};

static void
gimp_stroke_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* Calling the virtual functions */

GimpAnchor *
gimp_stroke_anchor_get (const GimpStroke *stroke,
			const GimpCoords *coord)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return (GIMP_STROKE_GET_CLASS (stroke))->anchor_get (stroke, coord);
}


static GimpAnchor *
gimp_stroke_real_anchor_get (const GimpStroke *stroke,
                             const GimpCoords *coord)
{
  gdouble     dx, dy, mindist;
  GList      *list;
  GimpAnchor *anchor = NULL;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  list = stroke->anchors;
  if (list)
    {
      dx = coord->x - ((GimpAnchor *) list->data)->position.x;
      dy = coord->y - ((GimpAnchor *) list->data)->position.y;
      anchor = (GimpAnchor *) list->data;
      mindist = dx * dx + dy * dy;
      list = list->next;
    }

  while (list)
    {
      dx = coord->x - ((GimpAnchor *) list->data)->position.x;
      dy = coord->y - ((GimpAnchor *) list->data)->position.y;
      if (mindist > dx * dx + dy * dy)
        {
          mindist = dx * dx + dy * dy;
          anchor = (GimpAnchor *) list->data;
        }
      list = list->next;
    }

  return anchor;
}


GimpAnchor *
gimp_stroke_anchor_get_next (const GimpStroke *stroke,
		             const GimpAnchor *prev)
{
  GimpStrokeClass *stroke_class;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);
  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  return stroke_class->anchor_get_next (stroke, prev);
}


static GimpAnchor *
gimp_stroke_real_anchor_get_next (const GimpStroke *stroke,
                                  const GimpAnchor *prev)
{
  GList      *listitem;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  if (prev)
    {
      listitem = g_list_find (stroke->anchors, prev);
      if (listitem)
        listitem = g_list_next (listitem);
    }
  else
    listitem = stroke->anchors;

  if (listitem)
    return (GimpAnchor *) listitem->data;
 
  return NULL;
}


void
gimp_stroke_anchor_select (GimpStroke        *stroke,
                           GimpAnchor        *anchor,
                           gboolean           exclusive)
{
  GimpStrokeClass *stroke_class;

  g_return_if_fail (GIMP_IS_STROKE (stroke));

  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  if (stroke_class->anchor_select)
    stroke_class->anchor_select (stroke, anchor, exclusive);
  else
    {
      GList *cur_ptr;

      cur_ptr = stroke->anchors;

      if (exclusive)
        {
          while (cur_ptr)
            {
              ((GimpAnchor *) cur_ptr->data)->selected = FALSE;
              cur_ptr = g_list_next (cur_ptr);
            }
        }
      
      if ((cur_ptr = g_list_find (stroke->anchors, anchor)) != NULL)
        ((GimpAnchor *) cur_ptr->data)->selected = TRUE;
    }
}


void
gimp_stroke_anchor_move_relative (GimpStroke        *stroke,
                                  GimpAnchor        *anchor,
				  const GimpCoords  *deltacoord,
                                  const gint         type)
{
  GimpStrokeClass *stroke_class;

  g_return_if_fail (GIMP_IS_STROKE (stroke));

  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  stroke_class->anchor_move_relative (stroke, anchor, deltacoord, type);
}


static void
gimp_stroke_real_anchor_move_relative (GimpStroke        *stroke,
                                       GimpAnchor        *anchor,
                                       const GimpCoords  *deltacoord,
                                       const gint         type)
{
  /*
   * There should be a test that ensures that the anchor is owned by
   * the stroke...
   */
  if (anchor) {
    anchor->position.x += deltacoord->x;
    anchor->position.y += deltacoord->y;
  }
}


void
gimp_stroke_anchor_move_absolute (GimpStroke       *stroke,
                                   GimpAnchor        *anchor,
				   const GimpCoords  *coord,
                                   const gint         type)
{
  GimpStrokeClass *stroke_class;

  g_return_if_fail (GIMP_IS_STROKE (stroke));

  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  stroke_class->anchor_move_absolute (stroke, anchor, coord, type);
}


static void
gimp_stroke_real_anchor_move_absolute (GimpStroke        *stroke,
                                       GimpAnchor        *anchor,
                                       const GimpCoords  *coord,
                                       const gint         type)
{
  /*
   * There should be a test that ensures that the anchor is owned by
   * the stroke...
   */
  if (anchor) {
    anchor->position.x = coord->x;
    anchor->position.y = coord->y;
  }
}


void
gimp_stroke_anchor_delete (GimpStroke     *stroke,
                            GimpAnchor      *anchor)
{
  GimpStrokeClass *stroke_class;

  g_return_if_fail (GIMP_IS_STROKE (stroke));

  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  if (stroke_class->anchor_delete)
    stroke_class->anchor_delete (stroke, anchor);
  else
    g_printerr ("gimp_stroke_anchor_delete: default implementation\n");

  return;
}


gdouble
gimp_stroke_get_length (const GimpStroke *stroke)
{
  GimpStrokeClass *stroke_class;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), 0);

  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  if (stroke_class->get_length)
    return stroke_class->get_length (stroke);
  else
    g_printerr ("gimp_stroke_get_length: default implementation\n");

  return 0;
}


gdouble
gimp_stroke_get_distance (const GimpStroke *stroke,
			   const GimpCoords  *coord)
{
  GimpStrokeClass *stroke_class;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), 0);

  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  if (stroke_class->get_distance)
    return stroke_class->get_distance (stroke, coord);
  else
    g_printerr ("gimp_stroke_get_distance: default implementation\n");

  return 0;
}


GArray *
gimp_stroke_interpolate (const GimpStroke  *stroke,
                         const gdouble      precision,
                         gboolean          *ret_closed)
{
  GimpStrokeClass *stroke_class;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), 0);

  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  if (stroke_class->interpolate)
    return stroke_class->interpolate (stroke, precision,
                                      ret_closed);
  else
    g_printerr ("gimp_stroke_interpolate: default implementation\n");

  return 0;
}


GimpAnchor *
gimp_stroke_temp_anchor_get (const GimpStroke *stroke)
{
  GimpStrokeClass *stroke_class;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  if (stroke_class->temp_anchor_get)
    return stroke_class->temp_anchor_get (stroke);
  else
    g_printerr ("gimp_stroke_temp_anchor_get: default implementation\n");

  return NULL;
}


GimpAnchor *
gimp_stroke_temp_anchor_set (GimpStroke *stroke,
			      const GimpCoords  *coord)
{
  GimpStrokeClass *stroke_class;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  if (stroke_class->temp_anchor_set)
    return stroke_class->temp_anchor_set (stroke, coord);
  else
    g_printerr ("gimp_stroke_temp_anchor_set: default implementation\n");

  return NULL;
}


gboolean
gimp_stroke_temp_anchor_fix (GimpStroke *stroke)
{
  GimpStrokeClass *stroke_class;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), FALSE);

  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  if (stroke_class->temp_anchor_fix)
    return stroke_class->temp_anchor_fix (stroke);
  else
    g_printerr ("gimp_stroke_temp_anchor_fix: default implementation\n");

  return FALSE;
}


GimpStroke *
gimp_stroke_make_bezier (const GimpStroke *stroke)
{
  GimpStrokeClass *stroke_class;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  if (stroke_class->make_bezier)
    return stroke_class->make_bezier (stroke);
  else
    g_printerr ("gimp_stroke_make_bezier: default implementation\n");

  return NULL;
}



GList *
gimp_stroke_get_draw_anchors (const GimpStroke  *stroke)
{
  GimpStrokeClass *stroke_class;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  if (stroke_class->get_draw_anchors)
    return stroke_class->get_draw_anchors (stroke);
  else
    {
      GList *cur_ptr, *ret_list = NULL;

      cur_ptr = stroke->anchors;

      while (cur_ptr)
        {
          if (((GimpAnchor *) cur_ptr->data)->type == ANCHOR_HANDLE)
            ret_list = g_list_append (ret_list, cur_ptr->data);
          cur_ptr = g_list_next (cur_ptr);
        }

      return ret_list;
    }

}


GList *
gimp_stroke_get_draw_controls (const GimpStroke  *stroke,
                               const GimpAnchor  *active)
{
  GimpStrokeClass *stroke_class;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  if (stroke_class->get_draw_controls)
    return stroke_class->get_draw_controls (stroke, active);
  else
    {
      GList *cur_ptr, *ret_list = NULL;

      cur_ptr = stroke->anchors;

      while (cur_ptr)
        {
          if (((GimpAnchor *) cur_ptr->data)->type == CONTROL_HANDLE)
            {
              if (cur_ptr->next &&
                  ((GimpAnchor *) cur_ptr->next->data)->type == ANCHOR_HANDLE &&
                  ((GimpAnchor *) cur_ptr->next->data)->selected)
                ret_list = g_list_append (ret_list, cur_ptr->data);
              else if (cur_ptr->prev &&
                  ((GimpAnchor *) cur_ptr->prev->data)->type == ANCHOR_HANDLE &&
                  ((GimpAnchor *) cur_ptr->prev->data)->selected)
                ret_list = g_list_append (ret_list, cur_ptr->data);
            }
          cur_ptr = g_list_next (cur_ptr);
        }

      return ret_list;
    }

  return NULL;
}
          
GArray *
gimp_stroke_get_draw_lines (const GimpStroke  *stroke,
                            const GimpAnchor  *active)
{
  GimpStrokeClass *stroke_class;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  stroke_class = GIMP_STROKE_GET_CLASS (stroke);

  if (stroke_class->get_draw_lines)
    return stroke_class->get_draw_lines (stroke, active);
  else
    {
      GList *cur_ptr;
      GArray *ret_lines = g_array_new (FALSE, FALSE, sizeof (GimpCoords));

      cur_ptr = stroke->anchors;

      while (cur_ptr)
        {
          if (((GimpAnchor *) cur_ptr->data)->type == ANCHOR_HANDLE &&
              ((GimpAnchor *) cur_ptr->data)->selected)
            {
              if (cur_ptr->next)
                {
                  ret_lines = g_array_append_val (ret_lines, 
                                ((GimpAnchor *) cur_ptr->data)->position);

                  ret_lines = g_array_append_val (ret_lines, 
                                ((GimpAnchor *) cur_ptr->next->data)->position);

                }
              if (cur_ptr->prev)
                {
                  ret_lines = g_array_append_val (ret_lines, 
                                ((GimpAnchor *) cur_ptr->data)->position);

                  ret_lines = g_array_append_val (ret_lines, 
                                ((GimpAnchor *) cur_ptr->prev->data)->position);

                }
            }
          cur_ptr = g_list_next (cur_ptr);
        }

      return ret_lines;
    }
}

