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
