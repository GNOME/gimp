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

#ifndef _MESSAGE_WINDOW_H_
#define _MESSAGE_WINDOW_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MESSAGE_WINDOW(obj)					GTK_CHECK_CAST(obj, message_window_get_type(), MessageWindow)
#define MESSAGE_WINDOW_CLASS(klass)	GTK_CHECK_CLASS_CAST(klass, message_window_get_type(), MessageWindowClass)
#define IS_MESSAGE_WINDOW(obj)			GTK_CHECK_TYPE(obj, message_window_get_type())


typedef struct _MessageWindow				MessageWindow;
typedef struct _MessageWindowClass	MessageWindowClass;

struct _MessageWindow
{
	GtkWindow window;

	GtkWidget *text;
  GtkWidget *dismiss_button;

	guint contains_messages : 1;
};

struct _MessageWindowClass
{
	GtkWindowClass parent_class;
};


guint				message_window_get_type	(void);
GtkWidget*	message_window_new			(const gchar *title);
void				message_window_clear		(MessageWindow *mw);
void				message_window_append		(MessageWindow *mw, const gchar *msg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MESSAGE_WINDOW_H_ */

/* vim: set ts=2 sw=2 tw=79 ai nowrap: */
