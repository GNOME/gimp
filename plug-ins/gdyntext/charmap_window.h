/*
 * GIMP Dynamic Text -- This is a plug-in for The GIMP 1.0
 * Copyright (C) 1998,1999 Marco Lamberto <lm@geocities.com>
 * Web page: http://www.geocities.com/Tokyo/1474/gimp/
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
 *
 * $Id$
 */

#ifndef _CHARMAP_WINDOW_H_
#define _CHARMAP_WINDOW_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CHARMAP_WINDOW(obj)					GTK_CHECK_CAST(obj, charmap_window_get_type(), CharMapWindow)
#define CHARMAP_WINDOW_CLASS(klass)	GTK_CHECK_CLASS_CAST(klass, charmap_window_get_type(), CharMapWindowClass)
#define IS_CHARMAP_WINDOW(obj)			GTK_CHECK_TYPE(obj, charmap_window_get_type())


typedef struct _CharMapWindow				CharMapWindow;
typedef struct _CharMapWindowClass	CharMapWindowClass;


struct _CharMapWindow
{
	GtkWindow window;

	GtkWidget *charmap;
	GtkWidget *scrolledwindow;
	GtkWidget *label;
	GtkWidget *insert_button;
	GtkWidget *close_button;
};

struct _CharMapWindowClass
{
	GtkWindowClass parent_class;
};


guint				charmap_window_get_type(void);
GtkWidget*	charmap_window_new(const gchar *title);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CHARMAP_WINDOW_H_ */

/* vim: set ts=2 sw=2 tw=79 ai nowrap: */
