/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Andy Thomas alt@picnic.demon.co.uk
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
#ifndef  __PATH_H__
#define  __PATH_H__

#include "bezier_select.h"

typedef struct _Path      Path;
typedef struct _PathPoint PathPoint;

typedef GSList PathUndo;

typedef struct 
{
  GimpImage *gimage;
  GDisplay  *gdisp; /* This is a hack.. Needed so we can get back to 
                     * the same display that these curves were added
		     * too. That way when an entry in the paths dialog
		     * is clicked the bezier tool can be targeted at 
		     * correct display. Note this display could have been
		     * deleted (ie different view), but gdisplays_check_valid()
		     * function will take care of that.. In this case we just
		     * pick a display that the gimage is rendered in.
		     */
  GSList    *bz_paths;  /* list of BZPATHP */
  guint      sig_id;
  gint32     last_selected_row;
} PathList;

typedef enum 
{
  BEZIER = 1
} PathType;

Path*         path_new                  (GimpImage *gimage,
					 PathType   ptype,
					 GSList    *path_details,
					 gint       closed,
					 gint       state,
					 gint       locked,
					 gint       tattoo,  
					 gchar     *name);
Path*         path_copy                 (GimpImage *gimage, 
					 Path      *path);
void          path_free                 (Path      *path);

Tattoo        path_get_tattoo           (Path      *path);
Path*         path_get_path_by_tattoo   (GimpImage *gimage, 
					 Tattoo     tattoo);

void          path_stroke               (GimpImage *gimage,
					 PathList  *pl,
					 Path      *bzp);
gint          path_distance             (Path      *bzp,
					 gdouble    dist,
					 gint      *x,
					 gint      *y, 
					 gdouble   *grad);

PathPoint*    path_point_new            (guint      type, 
					 gdouble    x, 
					 gdouble    y);
void          path_point_free           (PathPoint *pathpoint);

PathList*     path_list_new             (GimpImage *gimage, 
					 gint       last_selected_row, 
					 GSList    *bz_paths);
void          path_list_free            (PathList  *plist);

BezierSelect* path_to_beziersel         (Path      *path);

#endif  /*  __PATH_H__  */

