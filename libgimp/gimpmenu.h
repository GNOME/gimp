/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmenu.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */        

#ifndef __GIMP_MENU_H__
#define __GIMP_MENU_H__

G_BEGIN_DECLS


typedef gint (* GimpConstraintFunc)   (gint32    image_id,
				       gint32    drawable_id,
				       gpointer  data);
typedef void (* GimpMenuCallback)     (gint32    any_id,
				       gpointer  data);


typedef void (* GimpRunBrushCallback)    (const gchar   *name,
					  gdouble        opacity,
					  gint           spacing,
					  gint           paint_mode,
					  gint           width,
					  gint           height,
					  const guchar  *mask_data,
					  gboolean       dialog_closing,
					  gpointer       user_data);

typedef void (* GimpRunFontCallback)     (const gchar   *name,
					  gboolean       dialog_closing,
					  gpointer       user_data);

typedef void (* GimpRunGradientCallback) (const gchar   *name,
					  gint           width,
					  const gdouble *grad_data,
					  gboolean       dialog_closing,
					  gpointer       user_data);

typedef void (* GimpRunPatternCallback)  (const gchar   *name,
					  gint           width,
					  gint           height,
					  gint           bpp,
					  const guchar  *mask_data,
					  gboolean       dialog_closing,
					  gpointer       user_data);
  

GtkWidget * gimp_image_menu_new    (GimpConstraintFunc constraint,
				    GimpMenuCallback   callback,
				    gpointer           data,
				    gint32             active_image);
GtkWidget * gimp_layer_menu_new    (GimpConstraintFunc constraint,
				    GimpMenuCallback   callback,
				    gpointer           data,
				    gint32             active_layer);
GtkWidget * gimp_channel_menu_new  (GimpConstraintFunc constraint,
				    GimpMenuCallback   callback,
				    gpointer           data,
				    gint32             active_channel);
GtkWidget * gimp_drawable_menu_new (GimpConstraintFunc constraint,
				    GimpMenuCallback   callback,
				    gpointer           data,
				    gint32             active_drawable);


gchar    * gimp_interactive_selection_brush    (const gchar          *title,
						const gchar          *brush_name,
						gdouble               opacity,
						gint                  spacing,
						GimpLayerModeEffects  paint_mode,
						GimpRunBrushCallback  callback,
						gpointer              data);
  
GtkWidget * gimp_brush_select_widget           (const gchar          *title,
						const gchar          *brush_name, 
						gdouble               opacity,
						gint                  spacing,
						GimpLayerModeEffects  paint_mode,
						GimpRunBrushCallback  callback,
						gpointer              data);
  
void      gimp_brush_select_widget_set_popup   (GtkWidget            *widget,
						const gchar          *brush_name,
						gdouble               opacity,
						gint                  spacing,
						GimpLayerModeEffects  paint_mode);
void      gimp_brush_select_widget_close_popup (GtkWidget            *widget);
  

gchar   * gimp_interactive_selection_font      (const gchar          *title,
                                                const gchar          *font_name,
                                                GimpRunFontCallback   callback,
                                                gpointer              data);

GtkWidget * gimp_font_select_widget            (const gchar          *title,
                                                const gchar          *font_name, 
                                                GimpRunFontCallback   callback,
                                                gpointer              data);
  
void      gimp_font_select_widget_close_popup  (GtkWidget            *widget);
void      gimp_font_select_widget_set_popup    (GtkWidget            *widget,
                                                const gchar          *font_name);


gchar   * gimp_interactive_selection_gradient     (const gchar      *title,
						   const gchar      *gradient_name,
						   gint              sample_size,
						   GimpRunGradientCallback  callback,
						   gpointer          data);
  
GtkWidget * gimp_gradient_select_widget           (const gchar      *title,
						   const gchar      *gradient_name, 
						   GimpRunGradientCallback  callback,
						   gpointer          data);

void      gimp_gradient_select_widget_close_popup (GtkWidget        *widget);
void      gimp_gradient_select_widget_set_popup   (GtkWidget        *widget,
						   const gchar      *gradient_name);


gchar   * gimp_interactive_selection_pattern      (const gchar      *title,
						   const gchar      *pattern_name,
						   GimpRunPatternCallback  callback,
						   gpointer          data);

GtkWidget * gimp_pattern_select_widget            (const gchar      *title,
						   const gchar      *pattern_name, 
						   GimpRunPatternCallback  callback,
						   gpointer          data);
  
void      gimp_pattern_select_widget_close_popup  (GtkWidget        *widget);
void      gimp_pattern_select_widget_set_popup    (GtkWidget        *widget,
						   const gchar      *pattern_name);


G_END_DECLS

#endif /* __GIMP_MENU_H__ */
