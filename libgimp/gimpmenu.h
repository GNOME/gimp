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

/* Popup the brush dialog interactively */
typedef void (* GRunBrushCallback) (gchar *, /* Name */
                                    gdouble, /* opacity */
                                    gint,    /* spacing */
                                    gint,    /* paint_mode */
                                    gint,    /* width */
                                    gint,    /* height */
                                    gchar *, /* mask data */
                                    gint,    /* dialog closing */
				    gpointer /* user data */);

/* Popup the pattern dialog */
typedef void (*GRunPatternCallback) (gchar *, /* Name */
				     gint,    /* Width */
				     gint,    /* Heigth */
				     gint,    /* bytes pp in mask */
				     gchar *, /* mask data */
				     gint,    /* dialog closing */
				     gpointer /* user data */);

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

void * gimp_interactive_selection_brush (gchar *dialogname,
				       gchar *brush_name,
				       gdouble opacity,
				       gint spacing,
				       gint paint_mode,
				       GRunBrushCallback callback,
				       gpointer udata);

GtkWidget * gimp_brush_select_widget(gchar * dname,
				     gchar * ibrush, 
				     gdouble opacity,
				     gint spacing,
				     gint paint_mode,
				     GRunBrushCallback cback,
				     gpointer);

gint gimp_brush_select_widget_close_popup(GtkWidget *w);

gint gimp_brush_select_widget_set_popup(GtkWidget *w,
					gchar *pname,
					gdouble opacity,
					gint spacing,
					gint paint_mode);

gchar *gimp_brushes_get_brush_data (gchar *pname,
				    gdouble *opacity,
				    gint *spacing,
				    gint *paint_mode,
				    gint  *width,
				    gint  *height,
				    gchar  **mask_data);

gint gimp_brush_set_popup(void * popup_pnt, 
			  gchar * pname,
			  gdouble opacity,
			  gint spacing,
			  gint paint_mode);

gint gimp_brush_close_popup(void * popup_pnt);

void * gimp_interactive_selection_pattern (gchar *dialogtitle,
					 gchar *pattern_name,
					 GRunPatternCallback callback,
					 gpointer udata);

GtkWidget * gimp_pattern_select_widget(gchar * dname,
				       gchar * ipattern, 
				       GRunPatternCallback cback,
				       gpointer);

gint gimp_pattern_select_widget_close_popup(GtkWidget *w);

gint gimp_pattern_select_widget_set_popup(GtkWidget *w,gchar *pname);

gchar *gimp_pattern_get_pattern_data (gchar *pname,
			       gint  *width,
			       gint  *height,
			       gint  *bytes,
			       gchar  **mask_data);

gint gimp_pattern_set_popup(void * popup_pnt, gchar * pname);

gint gimp_pattern_close_popup(void * popup_pnt);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GIMP_MENU_H__ */
