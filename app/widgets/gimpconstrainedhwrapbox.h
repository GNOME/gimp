/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkHWrapBox: Horizontal wrapping box widget
 * Copyright (C) 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_CONSTRAINED_HWRAP_BOX_H__
#define __GIMP_CONSTRAINED_HWRAP_BOX_H__


#include "gtkhwrapbox.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GIMP_TYPE_CONSTRAINED_HWRAP_BOX	           (gimp_constrained_hwrap_box_get_type ())
#define GIMP_CONSTRAINED_HWRAP_BOX(obj)	           (GTK_CHECK_CAST ((obj), GIMP_TYPE_CONSTRAINED_HWRAP_BOX, GimpConstrainedHWrapBox))
#define GIMP_CONSTRAINED_HWRAP_BOX_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONSTRAINED_HWRAP_BOX, GimpConstrainedHWrapBoxClass))
#define GIMP_IS_CONSTRAINED_HWRAP_BOX(obj)	   (GTK_CHECK_TYPE ((obj), GIMP_TYPE_CONSTRAINED_HWRAP_BOX))
#define GIMP_IS_CONSTRAINED_HWRAP_BOX_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONSTRAINED_HWRAP_BOX))
#define GIMP_CONSTRAINED_HWRAP_BOX_GET_CLASS(obj)  (GIMP_CONSTRAINED_HWRAP_BOX_CLASS (((GtkObject *) (obj))->klass))


typedef struct _GimpConstrainedHWrapBox      GimpConstrainedHWrapBox;
typedef struct _GimpConstrainedHWrapBoxClass GimpConstrainedHWrapBoxClass;

struct _GimpConstrainedHWrapBox
{
  GtkHWrapBox  parent_instance;
};

struct _GimpConstrainedHWrapBoxClass
{
  GtkHWrapBoxClass  parent_class;
};


GtkType	    gimp_constrained_hwrap_box_get_type (void);
GtkWidget * gimp_constrained_hwrap_box_new      (gboolean homogeneous);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_CONSTRAINED_HWRAP_BOX_H__ */
