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

#ifndef _FONT_SELECTION_H_
#define _FONT_SELECTION_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define FONT_SELECTION(obj)					GTK_CHECK_CAST(obj, font_selection_get_type(), FontSelection)
#define FONT_SELECTION_CLASS(klass)	GTK_CHECK_CLASS_CAST(klass, font_selection_get_type(), FontSelectionClass)
#define IS_FONT_SELECTION(obj)			GTK_CHECK_TYPE(obj, font_selection_get_type())


typedef struct _FontSelection				FontSelection;
typedef struct _FontSelectionClass	FontSelectionClass;


typedef enum
{
  FONT_METRIC_PIXELS = 0,
	FONT_METRIC_POINTS = 1
} FontMetricType;


struct _FontSelection
{
	GtkVBox hbox;

	GtkWidget *font_family;
	GtkWidget *font_style;
	GtkWidget *font_size;
	GtkWidget *font_metric;

	GdkFont *font;
	GList *font_names;
	GHashTable *font_properties;
};

struct _FontSelectionClass
{
	GtkHBoxClass parent_class;

	void (* font_changed)(FontSelection *fs);
};


guint						font_selection_get_type(void);
GtkWidget*			font_selection_new(void);
void						font_selection_set_font_family(FontSelection *fs, gchar *family);
void						font_selection_set_font_style(FontSelection *fs, const gchar *style);
void						font_selection_set_font_size(FontSelection *fs, const gint size);
void						font_selection_set_font_metric(FontSelection *fs, FontMetricType fm);
gchar*					font_selection_get_font_family(FontSelection *fs);
gchar*					font_selection_get_font_style(FontSelection *fs);
gint						font_selection_get_font_size(FontSelection *fs);
FontMetricType	font_selection_get_font_metric(FontSelection *fs);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FONT_SELECTION_H_ */

/* vim: set ts=2 sw=2 tw=79 ai nowrap: */
