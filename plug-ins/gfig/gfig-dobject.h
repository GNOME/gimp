/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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

#ifndef __GFIG_DOBJECT_H__
#define __GFIG_DOBJECT_H__

struct Dobject; /* fwd declaration for DobjFunc */

typedef void            (*DobjFunc)     (struct Dobject *);
typedef struct Dobject *(*DobjGenFunc)  (struct Dobject *);
typedef struct Dobject *(*DobjLoadFunc) (FILE *);
typedef void            (*DobjSaveFunc) (struct Dobject *, FILE *);

typedef enum
{
  LINE,
  CIRCLE,
  ELLIPSE,
  ARC,
  POLY,
  STAR,
  SPIRAL,
  BEZIER,
  MOVE_OBJ,
  MOVE_POINT,
  COPY_OBJ,
  MOVE_COPY_OBJ,
  DEL_OBJ,
  NULL_OPER
} DobjType;

typedef struct DobjPoints
{
  struct DobjPoints *next;
  GdkPoint           pnt;
  gint               found_me;
} DobjPoints;

/* The object itself */
typedef struct Dobject
{
  DobjType      type;      /* What is the type? */
  gint          type_data; /* Extra data needed by the object */
  DobjPoints   *points;    /* List of points */
  DobjFunc      drawfunc;  /* How do I draw myself */
  DobjFunc      paintfunc; /* Draw me on canvas */
  DobjGenFunc   copyfunc;  /* copy */
  DobjLoadFunc  loadfunc;  /* Load this type of object */
  DobjSaveFunc  savefunc;  /* Save me out */
} Dobject;

typedef struct DAllObjs
{
  struct DAllObjs *next; 
  Dobject         *obj; /* Object on list */
} DAllObjs;

/* States of the object */
#define GFIG_OK       0x0
#define GFIG_MODIFIED 0x1
#define GFIG_READONLY 0x2

extern Dobject *obj_creating;

void d_pnt_add_line (Dobject *obj,
		     gint     x,
		     gint     y,
		     gint     pos);

DobjPoints     *new_dobjpoint		(gint x, gint y);
void		do_save_obj 		(Dobject *obj, FILE *to);

DobjPoints     *d_copy_dobjpoints 	(DobjPoints * pnts);
void 		free_one_obj 		(Dobject *obj);
void 		d_delete_dobjpoints	(DobjPoints * pnts);
void 		object_update 		(GdkPoint *pnt);
DAllObjs       *copy_all_objs           (DAllObjs *objs);
void       	draw_objects            (DAllObjs *objs, gint show_single);

void       object_start            (GdkPoint *pnt, gint);
void       object_operation        (GdkPoint *pnt, gint);
void       object_operation_start  (GdkPoint *pnt, gint shift_down);
void       object_operation_end    (GdkPoint *pnt, gint);
void       object_end              (GdkPoint *pnt, gint shift_down);

#endif /* __GFIG_DOBJECT_H__ */


