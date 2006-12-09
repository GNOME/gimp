/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef  __GIMP_INK_H__
#define  __GIMP_INK_H__


#include "gimppaintcore.h"
#include "gimpink-blob.h"


#define DIST_SMOOTHER_BUFFER 10
#define TIME_SMOOTHER_BUFFER 10


#define GIMP_TYPE_INK            (gimp_ink_get_type ())
#define GIMP_INK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_INK, GimpInk))
#define GIMP_INK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_INK, GimpInkClass))
#define GIMP_IS_INK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_INK))
#define GIMP_IS_INK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_INK))
#define GIMP_INK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_INK, GimpInkClass))


typedef struct _GimpInk      GimpInk;
typedef struct _GimpInkClass GimpInkClass;

struct _GimpInk
{
  GimpPaintCore  parent_instance;

  Blob          *start_blob;   /*  starting blob (for undo)       */

  Blob          *cur_blob;     /*  current blob                   */
  Blob          *last_blob;    /*  blob for last cursor position  */

  /* circular distance history buffer */
  gdouble        dt_buffer[DIST_SMOOTHER_BUFFER];
  gint           dt_index;

  /* circular timing history buffer */
  guint32        ts_buffer[TIME_SMOOTHER_BUFFER];
  gint           ts_index;

  guint32        last_time;     /*  previous time of a motion event  */

  gboolean       init_velocity;
};

struct _GimpInkClass
{
  GimpPaintCoreClass  parent_class;
};


void    gimp_ink_register (Gimp                      *gimp,
                           GimpPaintRegisterCallback  callback);

GType   gimp_ink_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_INK_H__  */
