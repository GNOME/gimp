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

/* This implements a widget derived from gtk_button that visualizes
   it's state with two different pixmaps showing a closed and a 
   broken chain. It's intented to be used with the gimpsizeentry
   widget. The usage is quite similar to the one the gtk_toggle_button
   provides. 
 */


#ifndef __GIMP_CHAIN_BUTTON_H__
#define __GIMP_CHAIN_BUTTON_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GIMP_CHAIN_BUTTON(obj)            GTK_CHECK_CAST (obj, gimp_chain_button_get_type (), GimpChainButton)
#define GIMP_CHAIN_BUTTON_CLASS(klass)    GTK_CHECK_CLASS_CAST (klass, gimp_chain_button_get_type (), GimpChainButtonClass)
#define GIMP_IS_CHAIN_BUTTON(obj)         GTK_CHECK_TYPE (obj, gimp_chain_button_get_type ())

typedef struct _GimpChainButton       GimpChainButton;
typedef struct _GimpChainButtonClass  GimpChainButtonClass;

typedef enum {
  GIMP_CHAIN_TOP,
  GIMP_CHAIN_LEFT,
  GIMP_CHAIN_BOTTOM,
  GIMP_CHAIN_RIGHT
} GimpChainPosition;


struct _GimpChainButton
{
  GtkHBox hbox;

  GimpChainPosition  position;
  GtkTooltips       *tooltips;
  gchar             *tip;
  GtkWidget         *pixmap;
  GdkPixmap         *broken;
  GdkBitmap         *broken_mask;
  GdkPixmap         *chain;
  GdkBitmap         *chain_mask;
  gboolean           active;
};

struct _GimpChainButtonClass
{
  GtkButtonClass parent_class;

  void (* gimp_chain_button) (GimpChainButton *gcb);
};


GtkType    gimp_chain_button_get_type   (void);
GtkWidget* gimp_chain_button_new        (GimpChainPosition  position);
void       gimp_chain_button_set_active (GimpChainButton   *gcb,
					 gboolean           is_active);
gboolean   gimp_chain_button_get_active (GimpChainButton   *gcb);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CHAIN_BUTTON_H__ */


