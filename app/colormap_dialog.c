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

#include "apptypes.h"

#include <colormap_dialog.p.h>


GtkType _gimp_colormap_dialog_type;
static void gimp_colormap_dialog_class_init (GimpColormapDialogClass* klass);
static void gimp_colormap_dialog_init (GimpColormapDialog* colormap_dialog);
static guint _gimp_colormap_dialog_signal_selected;
static void _gimp_colormap_dialog_demarshall_0 (GtkObject*, GtkSignalFunc, gpointer, GtkArg*);
typedef void (*_GimpColormapDialogHandler_0)(GtkObject*, gpointer);
static void _gimp_colormap_dialog_demarshall_0 (
	GtkObject* object,
	GtkSignalFunc func,
	gpointer user_data,
	GtkArg* args){
	(void)args;
	((_GimpColormapDialogHandler_0)func)(object,
	user_data);
}
#ifdef GCG_IMPL
#	include GCG_IMPL
#else
#	include "colormap_dialog.i.c"
#endif

void _gimp_colormap_dialog_init_type (void){
	static GtkTypeInfo info = {
		"GimpColormapDialog",
		sizeof (GimpColormapDialog),
		sizeof (GimpColormapDialogClass),
		(GtkClassInitFunc) gimp_colormap_dialog_class_init,
		(GtkObjectInitFunc) gimp_colormap_dialog_init,
		NULL,
		NULL,
		NULL,
	};
	if (!_gimp_colormap_dialog_type)
		_gimp_colormap_dialog_type = gtk_type_unique (GTK_TYPE_DIALOG, &info);
}

static void gimp_colormap_dialog_class_init (GimpColormapDialogClass* klass){
	GtkObjectClass* obklass = (GtkObjectClass*) klass;
	(void) obklass;
	_gimp_colormap_dialog_signal_selected =
	gtk_signal_new("selected",
		GTK_RUN_FIRST,
		obklass->type,
		GTK_SIGNAL_OFFSET (GimpColormapDialogClass, selected),
		_gimp_colormap_dialog_demarshall_0,
		GTK_TYPE_NONE,
		0);
	gtk_object_class_add_signals(obklass,
		&_gimp_colormap_dialog_signal_selected,
		1);
#ifdef COLORMAP_DIALOG_CLASS_INIT
	COLORMAP_DIALOG_CLASS_INIT (klass);
#endif
}

static void gimp_colormap_dialog_init (GimpColormapDialog* colormap_dialog){
	(void) colormap_dialog;
#ifdef COLORMAP_DIALOG_INIT
	COLORMAP_DIALOG_INIT (colormap_dialog);
#endif
}

GimpColormapDialog* gimp_colormap_dialog_create (
	GimpSet* context){
	g_assert (!context || GIMP_IS_SET(context));
#ifdef COLORMAP_DIALOG_CREATE
	return COLORMAP_DIALOG_CREATE (context);
#else
	g_error("Not implemented: Gimp.ColormapDialog.create");
#endif
}

GimpImage* gimp_colormap_dialog_image (
	const GimpColormapDialog* colormap_dialog){
	g_assert (colormap_dialog);
	g_assert (GIMP_IS_COLORMAP_DIALOG(colormap_dialog));
	return colormap_dialog->image;
}

gint gimp_colormap_dialog_col_index (
	const GimpColormapDialog* colormap_dialog){
	g_assert (colormap_dialog);
	g_assert (GIMP_IS_COLORMAP_DIALOG(colormap_dialog));
	return colormap_dialog->col_index;
}

void gimp_colormap_dialog_connect_selected (
	GimpColormapDialogHandler_selected handler,
	gpointer user_data,
	GimpColormapDialog* colormap_dialog){
	g_assert (colormap_dialog);
	g_assert (GIMP_IS_COLORMAP_DIALOG(colormap_dialog));
	gtk_signal_connect((GtkObject*)colormap_dialog,
		"selected",
		(GtkSignalFunc)handler,
		user_data);
}

void gimp_colormap_dialog_selected (
	GimpColormapDialog* colormap_dialog){
	g_assert (colormap_dialog);
	g_assert (GIMP_IS_COLORMAP_DIALOG(colormap_dialog));
	{
	gtk_signal_emitv((GtkObject*)colormap_dialog, _gimp_colormap_dialog_signal_selected, NULL);
	}
}

