/* The GIMP -- an image manipulation program
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

#include "config.h"

#include <gtk/gtk.h>

#include "gimpmarshal.h"


typedef void (* GimpSignal_NONE__INT_INT_INT_INT) (GtkObject *object,
						   gint       arg1,
						   gint       arg2,
						   gint       arg3,
						   gint       arg4,
						   gpointer   user_data);

void
gimp_marshal_NONE__INT_INT_INT_INT (GtkObject     *object,
				    GtkSignalFunc  func,
				    gpointer       func_data,
				    GtkArg        *args)
{
  (* (GimpSignal_NONE__INT_INT_INT_INT) func) (object,
					       GTK_VALUE_INT (args[0]),
					       GTK_VALUE_INT (args[1]),
					       GTK_VALUE_INT (args[2]),
					       GTK_VALUE_INT (args[3]),
					       func_data);
}


typedef void (* GimpSignal_NONE__INT_INT_INT) (GtkObject *object,
					       gint       arg1,
					       gint       arg2,
					       gint       arg3,
					       gpointer   user_data);

void
gimp_marshal_NONE__INT_INT_INT (GtkObject     *object,
				GtkSignalFunc  func,
				gpointer       func_data,
				GtkArg        *args)
{
  (* (GimpSignal_NONE__INT_INT_INT) func) (object,
					   GTK_VALUE_INT (args[0]),
					   GTK_VALUE_INT (args[1]),
					   GTK_VALUE_INT (args[2]),
					   func_data);
}


typedef void (* GimpSignal_NONE__INT_POINTER_POINTER) (GtkObject *object,
						       gint       arg1,
						       gpointer   arg2,
						       gpointer   arg3,
						       gpointer   user_data);

void
gimp_marshal_NONE__INT_POINTER_POINTER (GtkObject     *object,
					GtkSignalFunc  func,
					gpointer       func_data,
					GtkArg        *args)
{
  (* (GimpSignal_NONE__INT_POINTER_POINTER) func) (object,
						   GTK_VALUE_INT (args[0]),
						   GTK_VALUE_POINTER (args[1]),
						   GTK_VALUE_POINTER (args[2]),
						   func_data);
}


typedef void (* GimpSignal_NONE__DOUBLE) (GtkObject *object,
					  gdouble    arg1,
					  gpointer   user_data);

void
gimp_marshal_NONE__DOUBLE (GtkObject     *object,
			   GtkSignalFunc  func,
			   gpointer       func_data,
			   GtkArg        *args)
{
  (* (GimpSignal_NONE__DOUBLE) func) (object,
				      GTK_VALUE_DOUBLE (args[0]),
				      func_data);
}


typedef gint (* GimpSignal_INT__POINTER) (GtkObject *object,
					  gpointer   arg1,
					  gpointer   user_data);

void
gimp_marshal_INT__POINTER (GtkObject     *object,
			   GtkSignalFunc  func,
			   gpointer       func_data,
			   GtkArg        *args)
{
  *GTK_RETLOC_INT (args[1]) =
    (* (GimpSignal_INT__POINTER) func) (object,
					GTK_VALUE_POINTER (args[0]),
					func_data);
}


typedef gpointer (* GimpSignal_POINTER__NONE) (GtkObject *object,
					       gpointer   user_data);

void
gimp_marshal_POINTER__NONE (GtkObject     *object,
			    GtkSignalFunc  func,
			    gpointer       func_data,
			    GtkArg        *args)
{
  *GTK_RETLOC_POINTER (args[0]) =
    (* (GimpSignal_POINTER__NONE) func) (object,
					 func_data);
}


typedef gpointer (* GimpSignal_POINTER__INT) (GtkObject *object,
					      gint       arg1,
					      gpointer   user_data);

void
gimp_marshal_POINTER__INT (GtkObject     *object,
			   GtkSignalFunc  func,
			   gpointer       func_data,
			   GtkArg        *args)
{
  *GTK_RETLOC_POINTER (args[1]) =
    (* (GimpSignal_POINTER__INT) func) (object,
					GTK_VALUE_INT (args[0]),
					func_data);
}


typedef gpointer (* GimpSignal_POINTER__INT_INT) (GtkObject *object,
						  gint       arg1,
						  gint       arg2,
						  gpointer   user_data);

void
gimp_marshal_POINTER__INT_INT (GtkObject     *object,
			       GtkSignalFunc  func,
			       gpointer       func_data,
			       GtkArg        *args)
{
  *GTK_RETLOC_POINTER (args[2]) =
    (* (GimpSignal_POINTER__INT_INT) func) (object,
					    GTK_VALUE_INT (args[0]),
					    GTK_VALUE_INT (args[1]),
					    func_data);
}


typedef gpointer (* GimpSignal_POINTER__POINTER) (GtkObject *object,
						  gpointer   arg1,
						  gpointer   user_data);

void
gimp_marshal_POINTER__POINTER (GtkObject     *object,
			       GtkSignalFunc  func,
			       gpointer       func_data,
			       GtkArg        *args)
{
  *GTK_RETLOC_POINTER (args[1]) =
    (* (GimpSignal_POINTER__POINTER) func) (object,
					    GTK_VALUE_POINTER (args[0]),
					    func_data);
}


typedef gpointer (* GimpSignal_POINTER__POINTER_INT) (GtkObject *object,
						      gpointer   arg1,
						      gint       arg2,
						      gpointer   user_data);

void
gimp_marshal_POINTER__POINTER_INT (GtkObject     *object,
				   GtkSignalFunc  func,
				   gpointer       func_data,
				   GtkArg        *args)
{
  *GTK_RETLOC_POINTER (args[2]) =
    (* (GimpSignal_POINTER__POINTER_INT) func) (object,
						GTK_VALUE_POINTER (args[0]),
						GTK_VALUE_INT (args[1]),
						func_data);
}


typedef gpointer (* GimpSignal_POINTER__POINTER_INT_INT) (GtkObject *object,
							  gpointer   arg1,
							  gint       arg2,
							  gint       arg3,
							  gpointer   user_data);

void
gimp_marshal_POINTER__POINTER_INT_INT (GtkObject     *object,
				       GtkSignalFunc  func,
				       gpointer       func_data,
				       GtkArg        *args)
{
  *GTK_RETLOC_POINTER (args[3]) =
    (* (GimpSignal_POINTER__POINTER_INT_INT) func) (object,
						    GTK_VALUE_POINTER (args[0]),
						    GTK_VALUE_INT (args[1]),
						    GTK_VALUE_INT (args[2]),
						    func_data);
}
