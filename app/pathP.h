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
#ifndef  __PATHP_H__
#define  __PATHP_H__

#include "gimpimageF.h"   /* Tattoo   */

struct _PathPoint
{
  guint32 type;
  gdouble x;
  gdouble y;
};

struct _Path
{
  GSList    *path_details;
  gint       pathtype; /* Only beziers to start with */
  gboolean   closed;
  guint32    state;
  guint32    locked;   /* Only bottom bit used */
  Tattoo     tattoo;   /* The tattoo for the path */
  gchar     *name;
};

gboolean      path_set_path        (GimpImage *gimage, 
				    gchar     *pname);
gboolean      path_set_path_points (GimpImage *gimage,
				    gchar     *pname,
				    gint       ptype,
				    gint       pclosed,
				    gint       num_pnts,
				    gdouble   *pnts);
gboolean      path_delete_path     (GimpImage *gimage,
				    gchar     *pname);

GSList*       pathpoints_copy      (GSList *list);
void          pathpoints_free      (GSList *list);

#endif  /*  __PATHP_H__  */




