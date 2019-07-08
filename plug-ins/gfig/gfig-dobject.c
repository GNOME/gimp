/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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

#include <stdlib.h>
#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gfig.h"
#include "gfig-dialog.h"
#include "gfig-style.h"
#include "gfig-arc.h"
#include "gfig-bezier.h"
#include "gfig-circle.h"
#include "gfig-dobject.h"
#include "gfig-ellipse.h"
#include "gfig-line.h"
#include "gfig-poly.h"
#include "gfig-rectangle.h"
#include "gfig-spiral.h"
#include "gfig-star.h"

#include "libgimp/stdplugins-intl.h"

static GfigObject *operation_obj = NULL;
static GdkPoint   *move_all_pnt; /* Point moving all from */


static void draw_one_obj         (GfigObject *obj,
                                  cairo_t    *cr);
static void do_move_obj          (GfigObject *obj,
                                  GdkPoint   *to_pnt);
static void do_move_all_obj      (GdkPoint   *to_pnt);
static void do_move_obj_pnt      (GfigObject *obj,
                                  GdkPoint   *to_pnt);
static void remove_obj_from_list (GFigObj    *obj,
                                  GfigObject *del_obj);
static gint scan_obj_points      (DobjPoints *opnt,
                                  GdkPoint   *pnt);

void
d_save_object (GfigObject *obj,
               GString    *string)
{
  do_save_obj (obj, string);

  switch (obj->type)
    {
    case BEZIER:
    case POLY:
    case SPIRAL:
    case STAR:
      g_string_append_printf (string, "<EXTRA>\n");
      g_string_append_printf (string, "%d\n</EXTRA>\n", obj->type_data);
      break;
    default:
      break;
    }
}

static DobjType
gfig_read_object_type (gchar *desc)
{
  gchar    *ptr = desc;
  DobjType  type;

  if (*ptr != '<')
    return OBJ_TYPE_NONE;

  ptr++;

  for (type = LINE; type < NUM_OBJ_TYPES; type++)
    {
      if (ptr == strstr (ptr, dobj_class[type].name))
        return type;
    }

  return OBJ_TYPE_NONE;
}

GfigObject *
d_load_object (gchar *desc,
               FILE  *fp)
{
  GfigObject *new_obj = NULL;
  gint        xpnt;
  gint        ypnt;
  gchar       buf[MAX_LOAD_LINE];
  DobjType    type;

  type = gfig_read_object_type (desc);
  if (type == OBJ_TYPE_NONE)
    {
      g_message ("Error loading object: type not recognized.");
      return NULL;
    }

  while (get_line (buf, MAX_LOAD_LINE, fp, 0))
    {
      if (sscanf (buf, "%d %d", &xpnt, &ypnt) != 2)
        {
          /* Read <EXTRA> block if there is one */
          if (!strcmp ("<EXTRA>", buf))
            {
              if ( !new_obj)
                {
                  g_message ("Error while loading object (no points)");
                  return NULL;
                }

              get_line (buf, MAX_LOAD_LINE, fp, 0);

              if (sscanf (buf, "%d", &new_obj->type_data) != 1)
                {
                  g_message ("Error while loading object (no type data)");
                  g_free (new_obj);
                  return NULL;
                }

              get_line (buf, MAX_LOAD_LINE, fp, 0);
              if (strcmp ("</EXTRA>", buf))
                {
                  g_message ("Syntax error while loading object");
                  g_free (new_obj);
                  return NULL;
                }
              /* Go around and read the last line */
              continue;
            }
          else
            return new_obj;
        }

      if (!new_obj)
        new_obj = d_new_object (type, xpnt, ypnt);
      else
        d_pnt_add_line (new_obj, xpnt, ypnt, -1);
    }

  return new_obj;
}

GfigObject *
d_new_object (DobjType    type,
              gint        x,
              gint        y)
{
  GfigObject *nobj = g_new0 (GfigObject, 1);

  nobj->type = type;
  nobj->class = &dobj_class[type];
  nobj->points = new_dobjpoint (x, y);

  nobj->type_data = 0;

  if (type == BEZIER)
    {
      nobj->type_data = 4;
    }
  else if (type == POLY)
    {
      nobj->type_data = 3;  /* default to 3 sides */
    }
  else if (type == SPIRAL)
    {
      nobj->type_data = 4;  /* default to 4 turns */
    }
  else if (type == STAR)
    {
      nobj->type_data = 3;  /* default to 3 sides 6 points */
    }

  return nobj;
}

void
gfig_init_object_classes (void)
{
  d_arc_object_class_init ();
  d_line_object_class_init ();
  d_rectangle_object_class_init ();
  d_circle_object_class_init ();
  d_ellipse_object_class_init ();
  d_poly_object_class_init ();
  d_spiral_object_class_init ();
  d_star_object_class_init ();
  d_bezier_object_class_init ();
}

/* Delete a list of points */
void
d_delete_dobjpoints (DobjPoints * pnts)
{
  DobjPoints *next;
  DobjPoints *pnt2del = pnts;

  while (pnt2del)
    {
      next = pnt2del->next;
      g_free (pnt2del);
      pnt2del = next;
    }
}

DobjPoints *
new_dobjpoint (gint x, gint y)
{
  DobjPoints *npnt = g_new0 (DobjPoints, 1);

  npnt->pnt.x = x;
  npnt->pnt.y = y;

  return npnt;
}

DobjPoints *
d_copy_dobjpoints (DobjPoints *pnts)
{
  DobjPoints *ret = NULL;
  DobjPoints *head = NULL;
  DobjPoints *newpnt;
  DobjPoints *pnt2copy;

  for (pnt2copy = pnts; pnt2copy; pnt2copy = pnt2copy->next)
    {
      newpnt = new_dobjpoint (pnt2copy->pnt.x, pnt2copy->pnt.y);

      if (!ret)
        {
          head = ret = newpnt;
        }
      else
        {
          head->next = newpnt;
          head = newpnt;
        }
    }

  return ret;
}

static DobjPoints *
get_diffs (GfigObject *obj,
           gint       *xdiff,
           gint       *ydiff,
           GdkPoint   *to_pnt)
{
  DobjPoints *spnt;

  g_assert (obj != NULL);

  for (spnt = obj->points; spnt; spnt = spnt->next)
    {
      if (spnt->found_me)
        {
          *xdiff = spnt->pnt.x - to_pnt->x;
          *ydiff = spnt->pnt.y - to_pnt->y;
          return spnt;
        }
    }
  return NULL;
}

static gboolean
inside_sqr (GdkPoint *cpnt,
            GdkPoint *testpnt)
{
  /* Return TRUE if testpnt is near cpnt */
  gint x  = cpnt->x;
  gint y  = cpnt->y;
  gint tx = testpnt->x;
  gint ty = testpnt->y;

  return (abs (x - tx) <= SQ_SIZE && abs (y - ty) < SQ_SIZE);
}

static gboolean
scan_obj_points (DobjPoints *opnt,
                 GdkPoint   *pnt)
{
  while (opnt)
    {
      if (inside_sqr (&opnt->pnt, pnt))
        {
          opnt->found_me = TRUE;
          return TRUE;
        }
      opnt->found_me = FALSE;
      opnt = opnt->next;
    }
  return FALSE;
}

static GfigObject *
get_nearest_objs (GFigObj  *obj,
                  GdkPoint *pnt)
{
  /* Nearest object to given point or NULL */
  GList      *all;
  GfigObject *test_obj;
  gint        count = 0;

  if (!obj)
    return NULL;

  for (all = obj->obj_list; all; all = g_list_next (all))
    {
      test_obj = all->data;

      if (count == obj_show_single || obj_show_single == -1)
        if (scan_obj_points (test_obj->points, pnt))
          {
            return test_obj;
          }
      count++;
    }
  return NULL;
}

void
object_operation_start (GdkPoint *pnt,
                        gboolean  shift_down)
{
  GfigObject *new_obj;

  /* Find point in given object list */
  operation_obj = get_nearest_objs (gfig_context->current_obj, pnt);

  /* Special case if shift down && move obj then moving all objs */

  if (shift_down && selvals.otype == MOVE_OBJ)
    {
      move_all_pnt = g_new (GdkPoint, 1);
      *move_all_pnt = *pnt; /* Structure copy */
      setup_undo ();
      return;
    }

  if (!operation_obj)
    return;/* None to work on */

  gfig_context->selected_obj = operation_obj;

  setup_undo ();

  switch (selvals.otype)
    {
    case MOVE_OBJ:
      if (operation_obj->type == BEZIER)
        {
          tmp_bezier = operation_obj;
        }
      break;

    case MOVE_POINT:
      if (operation_obj->type == BEZIER)
        {
          tmp_bezier = operation_obj;
        }
      /* If shift is down the break into sep lines */
      if ((operation_obj->type == POLY
          || operation_obj->type == STAR)
         && shift_down)
        {
          switch (operation_obj->type)
            {
            case POLY:
              d_poly2lines (operation_obj);
              break;
            case STAR:
              d_star2lines (operation_obj);
              break;
            default:
              break;
            }
          /* Re calc which object point we are looking at */
          scan_obj_points (operation_obj->points, pnt);
          gtk_widget_queue_draw (gfig_context->preview);
        }
      break;

    case COPY_OBJ:
      /* Copy the "operation object" */
      /* Then bung us into "copy/move" mode */

      new_obj = (GfigObject*) operation_obj->class->copyfunc (operation_obj);
      if (new_obj)
        {
          gfig_style_copy (&new_obj->style, &operation_obj->style, "Object");
          scan_obj_points (new_obj->points, pnt);
          add_to_all_obj (gfig_context->current_obj, new_obj);
          operation_obj = new_obj;
          selvals.otype = MOVE_COPY_OBJ;
          gtk_widget_queue_draw (gfig_context->preview);
        }
      break;

    case DEL_OBJ:
      remove_obj_from_list (gfig_context->current_obj, operation_obj);
      break;

    case SELECT_OBJ:
      /* don't need to do anything */
      break;

    case MOVE_COPY_OBJ: /* Never when button down */
    default:
      g_warning ("Internal error selvals.otype object operation start");
      break;
    }

}

void
object_operation_end (GdkPoint *pnt,
                      gboolean  shift_down)
{
  if (selvals.otype != DEL_OBJ && operation_obj &&
      operation_obj->type == BEZIER)
    {
      tmp_bezier = NULL; /* use as switch */
    }

  if (operation_obj && selvals.otype != DEL_OBJ)
    gfig_style_set_context_from_style (&operation_obj->style);

  operation_obj = NULL;

  if (move_all_pnt)
    {
      g_free (move_all_pnt);
      move_all_pnt = NULL;
    }

  /* Special case - if copying mode MUST be copy when button up received */
  if (selvals.otype == MOVE_COPY_OBJ)
    selvals.otype = COPY_OBJ;
}

/* Move object around */
void
object_operation (GdkPoint *to_pnt,
                  gboolean  shift_down)
{
  /* Must do different things depending on object type */
  /* but must have object to operate on! */

  /* Special case - if shift own and move_obj then move ALL objects */
  if (move_all_pnt && shift_down && selvals.otype == MOVE_OBJ)
    {
      do_move_all_obj (to_pnt);
      return;
    }

  if (!operation_obj)
    return;

  switch (selvals.otype)
    {
    case MOVE_OBJ:
    case MOVE_COPY_OBJ:
      switch (operation_obj->type)
        {
        case LINE:
        case RECTANGLE:
        case CIRCLE:
        case ELLIPSE:
        case POLY:
        case ARC:
        case STAR:
        case SPIRAL:
        case BEZIER:
          do_move_obj (operation_obj, to_pnt);
          break;
        default:
          /* Internal error */
          g_warning ("Internal error in operation_obj->type");
          break;
        }
      break;
    case MOVE_POINT:
      switch (operation_obj->type)
        {
        case LINE:
        case RECTANGLE:
        case CIRCLE:
        case ELLIPSE:
        case POLY:
        case ARC:
        case STAR:
        case SPIRAL:
        case BEZIER:
          do_move_obj_pnt (operation_obj, to_pnt);
          break;
        default:
          /* Internal error */
          g_warning ("Internal error in operation_obj->type");
          break;
        }
      break;
    case DEL_OBJ:
    case SELECT_OBJ:
      break;
    case COPY_OBJ: /* Should have been changed to MOVE_COPY_OBJ */
    default:
      g_warning ("Internal error selvals.otype");
      break;
    }
}

static void
update_pnts (GfigObject *obj,
             gint        xdiff,
             gint        ydiff)
{
  DobjPoints *spnt;

  g_assert (obj != NULL);

  /* Update all pnts */
  for (spnt = obj->points; spnt; spnt = spnt->next)
    {
      spnt->pnt.x -= xdiff;
      spnt->pnt.y -= ydiff;
    }
}

static void
remove_obj_from_list (GFigObj    *obj,
                      GfigObject *del_obj)
{
  /* Nearest object to given point or NULL */

  g_assert (del_obj != NULL);

  if (g_list_find (obj->obj_list, del_obj))
    {
      obj->obj_list = g_list_remove (obj->obj_list, del_obj);

      free_one_obj (del_obj);

      if (obj->obj_list)
        gfig_context->selected_obj = obj->obj_list->data;
      else
        gfig_context->selected_obj = NULL;

      if (obj_show_single != -1)
        {
          /* We've just deleted the only visible one */
          draw_grid_clear ();
          obj_show_single = -1; /* Show entry again */
        }

      gtk_widget_queue_draw (gfig_context->preview);
    }
  else
    g_warning (_("Hey, where has the object gone?"));
}

static void
do_move_all_obj (GdkPoint *to_pnt)
{
  /* Move all objects in one go */
  /* Undraw/then draw in new pos */
  gint xdiff = move_all_pnt->x - to_pnt->x;
  gint ydiff = move_all_pnt->y - to_pnt->y;

  if (xdiff || ydiff)
    {
      GList *all;

      for (all = gfig_context->current_obj->obj_list; all; all = all->next)
        {
          GfigObject *obj = all->data;

          update_pnts (obj, xdiff, ydiff);
        }

      *move_all_pnt = *to_pnt;

      gtk_widget_queue_draw (gfig_context->preview);
    }
}

void
do_save_obj (GfigObject *obj,
             GString    *string)
{
  DobjPoints *spnt;

  for (spnt = obj->points; spnt; spnt = spnt->next)
    {
      g_string_append_printf (string, "%d %d\n", spnt->pnt.x, spnt->pnt.y);
    }
}

static void
do_move_obj (GfigObject *obj,
             GdkPoint   *to_pnt)
{
  /* Move the whole line - undraw the line to start with */
  /* Then draw in new pos */
  gint xdiff = 0;
  gint ydiff = 0;

  get_diffs (obj, &xdiff, &ydiff, to_pnt);

  if (xdiff || ydiff)
    {
      update_pnts (obj, xdiff, ydiff);

      gtk_widget_queue_draw (gfig_context->preview);
    }
}

static void
do_move_obj_pnt (GfigObject *obj,
                 GdkPoint   *to_pnt)
{
  /* Move the whole line - undraw the line to start with */
  /* Then draw in new pos */
  DobjPoints *spnt;
  gint xdiff = 0;
  gint ydiff = 0;

  spnt = get_diffs (obj, &xdiff, &ydiff, to_pnt);

  if ((!xdiff && !ydiff) || !spnt)
    return;

  spnt->pnt.x = spnt->pnt.x - xdiff;
  spnt->pnt.y = spnt->pnt.y - ydiff;

  /* Draw in new pos */
  gtk_widget_queue_draw (gfig_context->preview);
}

/* copy objs */
GList *
copy_all_objs (GList *objs)
{
  GList *new_all_objs = NULL;

  while (objs)
    {
      GfigObject *object = objs->data;
      GfigObject *new_object = (GfigObject *) object->class->copyfunc (object);
      gfig_style_copy (&new_object->style, &object->style, "Object");

      new_all_objs = g_list_prepend (new_all_objs, new_object);

      objs = objs->next;
    }

  new_all_objs = g_list_reverse (new_all_objs);

  return new_all_objs;
}

/* Screen refresh */
static void
draw_one_obj (GfigObject *obj,
              cairo_t    *cr)
{
  obj->class->drawfunc (obj, cr);
}

void
draw_objects (GList    *objs,
              gboolean  show_single,
              cairo_t  *cr)
{
  /* Show_single - only one object to draw Unless shift
   * is down in which case show all.
   */

  gint count = 0;

  while (objs)
    {
      if (!show_single || count == obj_show_single || obj_show_single == -1)
        draw_one_obj (objs->data, cr);

      objs = g_list_next (objs);
      count++;
    }
}

void
prepend_to_all_obj (GFigObj *fobj,
                    GList   *nobj)
{
  setup_undo (); /* Remember ME */

  fobj->obj_list = g_list_concat (fobj->obj_list, nobj);
}

static void
scale_obj_points (DobjPoints *opnt,
                  gdouble     scale_x,
                  gdouble     scale_y)
{
  while (opnt)
    {
      opnt->pnt.x = (gint) (opnt->pnt.x * scale_x);
      opnt->pnt.y = (gint) (opnt->pnt.y * scale_y);
      opnt = opnt->next;
    }
}

void
add_to_all_obj (GFigObj    *fobj,
                GfigObject *obj)
{
  GList *nobj = NULL;

  nobj = g_list_append (nobj, obj);

  if (need_to_scale)
    scale_obj_points (obj->points, scale_x_factor, scale_y_factor);

  prepend_to_all_obj (fobj, nobj);

  /* initialize style when we add the object */
  gfig_context->selected_obj = obj;
}

/* First button press -- start drawing object */
/*
 * object_start() creates a new object of the type specified in the
 * button panel.  It is activated by a button press, and causes
 * a small square to be drawn at the initial point.  The style of
 * the new object is set to values taken from the style control
 * widgets.
 */
void
object_start (GdkPoint *pnt,
              gboolean  shift_down)
{
  /* start for the current object */
  if (!selvals.scaletoimage)
    {
      need_to_scale = 1;
      selvals.scaletoimage = 1;
    }
  else
    {
      need_to_scale = 0;
    }

  switch (selvals.otype)
    {
    case LINE:
      /* Shift means we are still drawing */
      d_line_start (pnt, shift_down);
      break;
    case RECTANGLE:
      d_rectangle_start (pnt, shift_down);
      break;
    case CIRCLE:
      d_circle_start (pnt, shift_down);
      break;
    case ELLIPSE:
      d_ellipse_start (pnt, shift_down);
      break;
    case POLY:
      d_poly_start (pnt, shift_down);
      break;
    case ARC:
      d_arc_start (pnt, shift_down);
      break;
    case STAR:
      d_star_start (pnt, shift_down);
      break;
    case SPIRAL:
      d_spiral_start (pnt, shift_down);
      break;
    case BEZIER:
      d_bezier_start (pnt, shift_down);
      break;
    default:
      /* Internal error */
      break;
    }

  if (obj_creating)
    {
      if (gfig_context->debug_styles)
        g_printerr ("Creating object, setting style from context\n");
      gfig_style_set_style_from_context (&obj_creating->style);
    }

}

void
object_end (GdkPoint *pnt,
            gboolean  shift_down)
{
  /* end for the current object */
  /* Add onto global object list */

  /* If shift is down may carry on drawing */
  switch (selvals.otype)
    {
    case LINE:
      d_line_end (pnt, shift_down);
      break;
    case RECTANGLE:
      d_rectangle_end (pnt, shift_down);
      break;
    case CIRCLE:
      d_circle_end (pnt, shift_down);
      break;
    case ELLIPSE:
      d_ellipse_end (pnt, shift_down);
      break;
    case POLY:
      d_poly_end (pnt, shift_down);
      break;
    case STAR:
      d_star_end (pnt, shift_down);
      break;
    case ARC:
      d_arc_end (pnt, shift_down);
      break;
    case SPIRAL:
      d_spiral_end (pnt, shift_down);
      break;
    case BEZIER:
      d_bezier_end (pnt, shift_down);
      break;
    default:
      /* Internal error */
      break;
    }

  if (need_to_scale)
    {
      need_to_scale = 0;
      selvals.scaletoimage = 0;
    }
}

/* Stuff for the generation/deletion of objects. */

/* Objects are easy one they are created - you just go down the object
 * list calling the draw function for each object but... when they
 * are been created we have to be a little more careful. When
 * the first point is placed on the canvas we create the object,
 * the mouse position then defines the next point that can move around.
 * careful how we draw this position.
 */

void
free_one_obj (GfigObject *obj)
{
  d_delete_dobjpoints (obj->points);
  g_free (obj);
}

void
free_all_objs (GList *objs)
{
  g_list_free_full (objs, (GDestroyNotify) free_one_obj);
}

gchar *
get_line (gchar *buf,
          gint   s,
          FILE  *from,
          gint   init)
{
  gint slen;
  char *ret;

  if (init)
    line_no = 1;
  else
    line_no++;

  do
    {
      ret = fgets (buf, s, from);
    }
  while (!ferror (from) && buf[0] == '#');

  slen = strlen (buf);

  /* The last newline is a pain */
  if (slen > 0)
    buf[slen - 1] = '\0';

  /* Check and remove an '\r' too from Windows */
  if ((slen > 1) && (buf[slen - 2] == '\r'))
    buf[slen - 2] = '\0';

  if (ferror (from))
    {
      g_warning (_("Error reading file"));
      return NULL;
    }

#ifdef DEBUG
  printf ("Processing line '%s'\n", buf);
#endif /* DEBUG */

  return ret;
}

void
clear_undo (void)
{
  int lv;

  for (lv = undo_level; lv >= 0; lv--)
    {
      free_all_objs (undo_table[lv]);
      undo_table[lv] = NULL;
    }

  undo_level = -1;

  gfig_dialog_action_set_sensitive ("undo", FALSE);
}

void
setup_undo (void)
{
  /* Copy object list to undo buffer */
#if DEBUG
  printf ("setup undo level [%d]\n", undo_level);
#endif /*DEBUG*/

  if (!gfig_context->current_obj)
    {
      /* If no current_obj must be loading -> no undo */
      return;
    }

  if (undo_level >= selvals.maxundo - 1)
    {
      int loop;
      /* the little one in the bed said "roll over".. */
      if (undo_table[0])
        free_one_obj (undo_table[0]->data);
      for (loop = 0; loop < undo_level; loop++)
        {
          undo_table[loop] = undo_table[loop + 1];
        }
    }
  else
    {
      undo_level++;
    }
  undo_table[undo_level] =
    copy_all_objs (gfig_context->current_obj->obj_list);

  gfig_dialog_action_set_sensitive ("undo", TRUE);

  gfig_context->current_obj->obj_status |= GFIG_MODIFIED;
}

void
new_obj_2edit (GFigObj *obj)
{
  GFigObj *old_current = gfig_context->current_obj;

  /* Clear undo levels */
  /* redraw the preview */
  /* Set up options as define in the selected object */

  clear_undo ();

  /* Point at this one */
  gfig_context->current_obj = obj;

  /* Show all objects to start with */
  obj_show_single = -1;

  /* Change options */
  options_update (old_current);

  /* redraw with new */
  gtk_widget_queue_draw (gfig_context->preview);

  if (obj->obj_status & GFIG_READONLY)
    {
      g_message (_("Editing read-only object - "
                   "you will not be able to save it"));

      gfig_dialog_action_set_sensitive ("save", FALSE);
    }
  else
    {
      gfig_dialog_action_set_sensitive ("save", TRUE);
    }
}

/* Add a point to a line (given x, y)
 * pos = 0 = head
 * pos = -1 = tail
 * 0 < pos = nth position
 */

void
d_pnt_add_line (GfigObject *obj,
                gint        x,
                gint        y,
                gint        pos)
{
  DobjPoints *npnts = new_dobjpoint (x, y);

  g_assert (obj != NULL);

  if (!pos)
    {
      /* Add to head */
      npnts->next = obj->points;
      obj->points = npnts;
    }
  else
    {
      DobjPoints *pnt = obj->points;

      /* Go down chain until the end if pos */
      while (pos < 0 || pos-- > 0)
        {
          if (!(pnt->next) || !pos)
            {
              npnts->next = pnt->next;
              pnt->next = npnts;
              break;
            }
          else
            {
              pnt = pnt->next;
            }
        }
    }
}
