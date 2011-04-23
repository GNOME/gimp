%{
/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

extern int cern_lex(void);
extern int cern_restart(FILE *cern_in);
static void cern_error(char* s);

static Object_t *current_object;

%}

%union {
   int val;
   double value;
   char *id;
}

%token<val> RECTANGLE POLYGON CIRCLE DEFAULT
%token<val> AUTHOR DESCRIPTION BEGIN_COMMENT
%token<value> FLOAT
%token<id> COMMENT LINK

%%

cern_file	: area_list
		;

area_list	: /* Empty */
		| area_list area
		;

area		: default
		| rectangle
		| circle
		| polygon
		| comment_line
		;

default		: DEFAULT LINK
		{
		   MapInfo_t *info = get_map_info();
		   g_strreplace(&info->default_url, $2);
                   g_free ($2);
		}
		;


rectangle	: RECTANGLE '(' FLOAT ',' FLOAT ')' '(' FLOAT ',' FLOAT ')' LINK
		{
		   gint x = (gint) $3;
		   gint y = (gint) $5;
		   gint width = (gint) fabs($8 - x);
		   gint height = (gint) fabs($10 - y);
		   current_object = create_rectangle(x, y, width, height);
		   object_set_url(current_object, $12);
		   add_shape(current_object);
                   g_free ($12);
		}
		;

circle		: CIRCLE '(' FLOAT ',' FLOAT ')' FLOAT LINK
		{
		   gint x = (gint) $3;
		   gint y = (gint) $5;
		   gint r = (gint) $7;
		   current_object = create_circle(x, y, r);
		   object_set_url(current_object, $8);
		   add_shape(current_object);
                   g_free ($8);
		}
		;

polygon		: POLYGON {current_object = create_polygon(NULL);} coord_list LINK
		{
		   object_set_url(current_object, $4);
		   add_shape(current_object);
                   g_free ($4);
		}
		;

coord_list	: /* Empty */
		| coord_list coord
		{
		}
		;

coord		: '(' FLOAT ',' FLOAT ')'
		{
		   Polygon_t *polygon = ObjectToPolygon(current_object);
		   GdkPoint *point = new_point((gint) $2, (gint) $4);
		   polygon->points = g_list_append(polygon->points, 
						   (gpointer) point);
		}
		;

comment_line	: author_line
		| description_line
		| real_comment
		;

real_comment	: BEGIN_COMMENT COMMENT
		{
		  g_free ($2);
		}
		;

author_line	: AUTHOR COMMENT
		{
		   MapInfo_t *info = get_map_info();
		   g_strreplace(&info->author, $2);
		   g_free ($2);
		}
		;

description_line: DESCRIPTION COMMENT
		{
		   MapInfo_t *info = get_map_info();
		   gchar *description;

		   description = g_strconcat(info->description, $2, "\n", 
					     NULL);
		   g_strreplace(&info->description, description);
		   g_free ($2);
		}
		;


%%

static void 
cern_error(char* s)
{
   extern FILE *cern_in;
   cern_restart(cern_in);
}

gboolean
load_cern(const char* filename)
{
   gboolean status;
   extern FILE *cern_in;
   cern_in = g_fopen(filename, "r");
   if (cern_in) {
      status = !cern_parse();
      fclose(cern_in);
   } else {
      status = FALSE;
   }
   return status;
}
