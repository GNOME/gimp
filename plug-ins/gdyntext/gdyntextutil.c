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

#include <string.h>
#include <glib.h>

#include "gdyntextutil.h"


/* substitution of '\' escaped sequences */
gchar *strunescape(const gchar *text)
{
	gchar *str = NULL;
	gchar *bstr = NULL;
	gchar *ustr = NULL;

	str = bstr = g_strdup(text);
	while ((str = strchr(str, '\\'))) {
		memcpy(str, str + 1, strlen(str) + 1);
		/* escape sequences recognition */
		switch (*str) {
		case 'n':
			*str = '\n';
			break;
		case 't':
			*str = '\t';
			break;
		}
		str++;
	}
	ustr = g_strdup(bstr);
	g_free(bstr);
	return ustr;
}


/* escapes by prepending a '\' to the following chars: '{' '}' '\n' '\t' '\' */
gchar *strescape(const gchar *_text)
{
	gchar *str = NULL;
	gchar *str_esc = NULL;
	gchar *text = NULL;
	gchar *buff = NULL;

	text = g_strdup(_text);
	buff = g_new0(char, (strlen(text) << 1) + 1);
	strcpy(buff, text);
	for (str = text, str_esc = buff; *str; str++, str_esc++) {
		switch (*str) {
			case '{':
			case '}':
			case '\\':
			case '\t':
			case '\n':
				strcpy(str_esc + 1, str);
				*str_esc = '\\';
				str_esc++;
				switch (*str) {
					case '\t': *str_esc = 't'; break;
					case '\n': *str_esc = 'n'; break;
				}
		}
	}
	str_esc = g_strdup(buff);
	g_free(buff);
	g_free(text);
	return str_esc;
}
