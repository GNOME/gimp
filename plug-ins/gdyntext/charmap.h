/*
 * GIMP Dynamic Text -- This is a plug-in for The GIMP 1.0
 * Copyright (C) 1998,1999,2000 Marco Lamberto <lm@geocities.com>
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

#ifndef _CHARMAP_H_
#define _CHARMAP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CHARMAP(obj)								GTK_CHECK_CAST(obj, charmap_get_type(), CharMap)
#define CHARMAP_CLASS(klass)				GTK_CHECK_CLASS_CAST(klass, charmap_get_type(), CharMapClass)
#define IS_CHARMAP(obj)							GTK_CHECK_TYPE(obj, charmap_get_type())


typedef struct _CharMap							CharMap;
typedef struct _CharMapClass				CharMapClass;


struct _CharMap
{
	GtkVBox vbox;

	GtkWidget *buttons[256];
	gint width;
	gint height;
	gint32 current_char;
};

struct _CharMapClass
{
	GtkVBoxClass parent_class;

	void (* char_selected)(CharMap *cm);
};


guint				charmap_get_type(void);
GtkWidget*	charmap_new(void);
void				charmap_set_font(CharMap *cm, GdkFont *font);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CHARMAP_H_ */

/* vim: set ts=2 sw=2 tw=79 ai nowrap: */
