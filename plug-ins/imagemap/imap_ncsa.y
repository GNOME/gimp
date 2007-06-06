%{
/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
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
 */

#include <math.h>

#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include "imap_circle.h"
#include "imap_file.h"
#include "imap_main.h"
#include "imap_polygon.h"
#include "imap_rectangle.h"
#include "imap_string.h"

extern int ncsa_lex();
extern int ncsa_restart();
static void ncsa_error(char* s);

static Object_t *current_object;

%}

%union {
   int val;
   double value;
   char id[256];
}

%token<val> RECTANGLE POLYGON CIRCLE DEFAULT
%token<val> AUTHOR TITLE DESCRIPTION BEGIN_COMMENT
%token<value> FLOAT
%token<id> LINK COMMENT

%%

ncsa_file	: comment_lines area_list
		;

comment_lines	: /* empty */
		| comment_lines comment_line
		;

comment_line	: author_line
		| title_line
		| description_line
		| real_comment
		;

real_comment	: BEGIN_COMMENT COMMENT
		{
		}
		;

author_line	: AUTHOR COMMENT
		{
		   MapInfo_t *info = get_map_info();
		   g_strreplace(&info->author, $2);
		}
		;

title_line	: TITLE COMMENT
		{
		   MapInfo_t *info = get_map_info();
		   g_strreplace(&info->title, $2);
		}
		;

description_line: DESCRIPTION COMMENT
		{
		   MapInfo_t *info = get_map_info();
		   gchar *description;

		   description = g_strconcat(info->description, $2, "\n", 
					     NULL);
		   g_strreplace(&info->description, description);
		}
		;

area_list	: /* Empty */
		| area_list area
		;

area		: default
		| rectangle
		| circle
		| polygon
		| real_comment
		;

default		: DEFAULT LINK
		{
		   MapInfo_t *info = get_map_info();		      
		   g_strreplace(&info->default_url, $2);
		}
		;


rectangle	: RECTANGLE LINK FLOAT ',' FLOAT FLOAT ',' FLOAT
		{
		   gint x = (gint) $3;
		   gint y = (gint) $5;
		   gint width = (gint) fabs($6 - x);
		   gint height = (gint) fabs($8 - y);
		   current_object = create_rectangle(x, y, width, height);
		   object_set_url(current_object, $2);
		   add_shape(current_object);
		}
		;

circle		: CIRCLE LINK FLOAT ',' FLOAT FLOAT ',' FLOAT
		{
		   gint x = (gint) $3;
		   gint y = (gint) $5;
		   gint r = (gint) fabs($8 - $5);
		   current_object = create_circle(x, y, r);
		   object_set_url(current_object, $2);
		   add_shape(current_object);
		}
		;

polygon		: POLYGON LINK {current_object = create_polygon(NULL);} coord_list
		{
		   object_set_url(current_object, $2);
		   add_shape(current_object);
		}
		;

coord_list	: /* Empty */
		| coord_list coord
		{
		}
		;

coord		: FLOAT ',' FLOAT
		{
		   Polygon_t *polygon = ObjectToPolygon(current_object);
		   GdkPoint *point = new_point((gint) $1, (gint) $3);
		   polygon->points = g_list_append(polygon->points, 
						   (gpointer) point);
		}
		;

%%

static void 
ncsa_error(char* s)
{
   extern FILE *ncsa_in;
   ncsa_restart(ncsa_in);
}

gboolean
load_ncsa(const char* filename)
{
   gboolean status;
   extern FILE *ncsa_in;
   ncsa_in = g_fopen(filename, "r");
   if (ncsa_in) {
      status = !ncsa_parse();
      fclose(ncsa_in);
   } else {
      status = FALSE;
   }
   return status;
}
