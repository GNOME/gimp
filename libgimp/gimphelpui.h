/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphelpui.h
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GIMP_HELP_UI_H__
#define __GIMP_HELP_UI_H__

#include <gtk/gtk.h>

typedef void (* GimpHelpFunc) (gchar *help_data);


void  gimp_help_init               (void);
void  gimp_help_free               (void);

void  gimp_help_enable_tooltips    (void);
void  gimp_help_disable_tooltips   (void);

/*  the standard gimp help function  */
void  gimp_standard_help_func      (gchar        *help_data);

/*  connect the "F1" accelerator of a window  */
void  gimp_help_connect_help_accel (GtkWidget    *widget,
				    GimpHelpFunc  help_func,
				    gchar        *help_data);

/*  set help data for non-window widgets  */
void  gimp_help_set_help_data      (GtkWidget    *widget,
				    const gchar  *tool_tip,
				    gchar        *help_data);

/*  activate the context help inspector  */
void  gimp_context_help            (void);

#endif /* __GIMP_HELP_UI_H__ */
