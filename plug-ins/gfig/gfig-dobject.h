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

#ifndef __GFIG_DOBJECT_H__
#define __GFIG_DOBJECT_H__

#include "gfig-types.h"
#include "gfig-style.h"

typedef void        (*DobjDrawFunc)   (GfigObject *, cairo_t *);
typedef void        (*DobjFunc)       (GfigObject *);
typedef GfigObject *(*DobjGenFunc)    (GfigObject *);

typedef struct DobjPoints
{
  struct DobjPoints *next;
  GdkPoint           pnt;
  gboolean           found_me;
} DobjPoints;

typedef struct
{
  DobjType      type;       /* the object type for this class */
  const gchar  *name;

  /* virtuals */
  DobjDrawFunc  drawfunc;   /* How do I draw myself */
  DobjFunc      paintfunc;  /* Draw me on canvas */
  DobjGenFunc   copyfunc;   /* copy */
  void         (*update) (GdkPoint   *pnt);
} GfigObjectClass;

extern GfigObjectClass dobj_class[10];

/* The object itself */
struct _GfigObject
{
  DobjType         type;       /* What is the type? */
  GfigObjectClass *class;      /* What class does it belong to? */
  gint             type_data;  /* Extra data needed by the object */
  DobjPoints      *points;     /* List of points */
  Style            style;      /* this object's individual style settings */
  gint             style_no;   /* style index of this specific object */
};

/* States of the object */
#define GFIG_OK       0x0
#define GFIG_MODIFIED 0x1
#define GFIG_READONLY 0x2

extern GfigObject *obj_creating;
extern GfigObject *tmp_line;


DobjPoints *new_dobjpoint            (gint        x,
                                      gint        y);
void        do_save_obj              (GfigObject *obj,
                                      GString    *to);

DobjPoints *d_copy_dobjpoints        (DobjPoints *pnts);
void        free_one_obj             (GfigObject *obj);
void        d_delete_dobjpoints      (DobjPoints *pnts);
void        object_update            (GdkPoint   *pnt);
GList      *copy_all_objs            (GList      *objs);
void        draw_objects             (GList      *objs,
                                      gboolean    show_single,
                                      cairo_t    *cr);

GfigObject *d_load_object            (gchar      *desc,
                                      FILE       *fp);

GfigObject *d_new_object             (DobjType    type,
                                      gint        x,
                                      gint        y);

void        d_save_object            (GfigObject *obj,
                                      GString    *string);

void        free_all_objs            (GList      *objs);

void        clear_undo               (GimpGfig   *gfig);

void        new_obj_2edit            (GimpGfig   *gfig,
                                      GFigObj    *obj);

void        gfig_init_object_classes (void);

void        d_pnt_add_line           (GfigObject *obj,
                                      gint        x,
                                      gint        y,
                                      gint        pos);

#endif /* __GFIG_DOBJECT_H__ */


