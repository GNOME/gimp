/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
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
#ifndef __GIMP_MENU_H__
#define __GIMP_MENU_H__


#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



typedef gint (*GimpConstraintFunc) (gint32   image_id,
				    gint32   drawable_id,
				    gpointer data);
typedef void (*GimpMenuCallback)   (gint32   id,
				    gpointer data);

GtkWidget* gimp_image_menu_new    (GimpConstraintFunc constraint,
				   GimpMenuCallback   callback,
				   gpointer           data,
				   gint32             active_image);
GtkWidget* gimp_layer_menu_new    (GimpConstraintFunc constraint,
				   GimpMenuCallback   callback,
				   gpointer           data,
				   gint32             active_layer);
GtkWidget* gimp_channel_menu_new  (GimpConstraintFunc constraint,
				   GimpMenuCallback   callback,
				   gpointer           data,
				   gint32             active_channel);
GtkWidget* gimp_drawable_menu_new (GimpConstraintFunc constraint,
				   GimpMenuCallback   callback,
				   gpointer           data,
				   gint32             active_drawable);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GIMP_MENU_H__ */
