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
#ifndef __GIMP_SIGNAL_H__
#define __GIMP_SIGNAL_H__

#include <gtk/gtksignal.h>

/* This is the gtk "signal id" */
typedef guint GimpSignalID;

typedef const struct _GimpSignalType GimpSignalType;

/* The arguments are encoded in the names.. */

extern GimpSignalType* const gimp_sigtype_void;
typedef void (*GimpHandlerVoid) (GtkObject*,
				 gpointer);

extern GimpSignalType* const gimp_sigtype_pointer;
typedef void (*GimpHandlerPointer) (GtkObject*, gpointer,
				    gpointer);

extern GimpSignalType* const gimp_sigtype_int;
typedef void (*GimpHandlerInt) (GtkObject*, gint,
				gpointer);

extern GimpSignalType* const gimp_sigtype_double;
typedef void (*GimpHandlerDouble) (GtkObject*, gdouble,
				   gpointer);

extern GimpSignalType* const gimp_sigtype_int_int_int;
typedef void (*GimpHandlerIntIntInt) (GtkObject*, gint, gint, gint,
				      gpointer);

extern GimpSignalType* const gimp_sigtype_int_int_int_int;
typedef void (*GimpHandlerIntIntIntInt) (GtkObject*, gint, gint, gint, gint,
					 gpointer);

GimpSignalID gimp_signal_new (const gchar      *name,
			      GtkSignalRunType  signal_flags,
			      GtkType           object_type,
			      guint             function_offset,
			      GimpSignalType   *sig_type);

#endif /* __GIMP_SIGNAL_H__ */
