%{
/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include "imap_circle.h"
#include "imap_file.h"
#include "imap_main.h"
#include "imap_polygon.h"
#include "imap_rectangle.h"
#include "imap_string.h"

extern int csim_lex(void);
extern int csim_restart(FILE *csim_in);
static void csim_error(char* s);

static enum {UNDEFINED, RECTANGLE, CIRCLE, POLYGON} current_type;
static Object_t *current_object;
static MapInfo_t *_map_info;

%}

%union {
  int val;
  double value;
  char *id;
}

%token<val> IMG SRC WIDTH HEIGHT BORDER USEMAP
%token<val> START_MAP END_MAP NAME AREA SHAPE COORDS ALT HREF NOHREF
%token<val> TARGET ONMOUSEOVER ONMOUSEOUT ONFOCUS ONBLUR
%token<val> AUTHOR DESCRIPTION BEGIN_COMMENT END_COMMENT
%token<value> FLOAT
%token<id> STRING

%type<val> integer_value

%%

csim_file	: image start_map comment_lines area_list end_map
		;

image		: '<' IMG SRC '=' STRING image_tags xhtml_close
		{
		   g_strreplace(&_map_info->image_name, $5);
		   g_free ($5);
		}
		;

image_tags	: /* Empty */
		| image_tags image_tag
		;

image_tag	: image_width
		| image_height
		| BORDER '=' integer_value {}
		| USEMAP '=' STRING { g_free ($3); }
		| ALT '=' STRING { g_free ($3); }
		;

image_width	: WIDTH '=' integer_value
		{
		   _map_info->old_image_width = $3;
		}
		;

image_height	: HEIGHT '=' integer_value
		{
		   _map_info->old_image_height = $3;
		}
		;

integer_value	: FLOAT
		{
		  $$ = (gint) $1;
		}
		| STRING
		{
		  $$ = (gint) g_ascii_strtod ($1, NULL);
		  g_free ($1);
		}
		;

start_map	: '<' START_MAP NAME '=' STRING '>'
		{
		   g_strreplace(&_map_info->title, $5);
		   g_free ($5);
		}
		;

comment_lines	: /* empty */
		| comment_lines comment_line
		;

comment_line	: author_line
		| description_line
		| real_comment
		;

real_comment	: BEGIN_COMMENT STRING END_COMMENT
		{
		  g_free ($2);
		}
		;

author_line	: AUTHOR STRING END_COMMENT
		{
		   g_strreplace(&_map_info->author, $2);
		   g_free ($2);
		}
		;

description_line: DESCRIPTION STRING END_COMMENT
		{
		   gchar *description;

		   description = g_strconcat(_map_info->description, $2, "\n",
					     NULL);
		   g_strreplace(&_map_info->description, description);
		   g_free ($2);
		}
		;

area_list	: /* empty */
		| area_list area
		;

area		: '<' AREA tag_list xhtml_close
		{
		   if (current_type != UNDEFINED)
		      add_shape(current_object);
		}
		;

xhtml_close	: '>'
		| '/' '>'
		;

tag_list	: /* Empty */
		| tag_list tag
		;

tag		: shape_tag
		| coords_tag
		| href_tag
		| nohref_tag
		| alt_tag
		| target_tag
		| onmouseover_tag
		| onmouseout_tag
		| onfocus_tag
		| onblur_tag
		;

shape_tag	: SHAPE '=' STRING
		{
		   if (!g_ascii_strcasecmp($3, "RECT")) {
		      current_object = create_rectangle(0, 0, 0, 0);
		      current_type = RECTANGLE;
		   } else if (!g_ascii_strcasecmp($3, "CIRCLE")) {
		      current_object = create_circle(0, 0, 0);
		      current_type = CIRCLE;
		   } else if (!g_ascii_strcasecmp($3, "POLY")) {
		      current_object = create_polygon(NULL);
		      current_type = POLYGON;
		   } else if (!g_ascii_strcasecmp($3, "DEFAULT")) {
		      current_type = UNDEFINED;
		   }
		   g_free ($3);
		}
		;

coords_tag	: COORDS '=' STRING
		{
		   char *p;
		   if (current_type == RECTANGLE) {
		      Rectangle_t *rectangle;

		      rectangle = ObjectToRectangle(current_object);
		      p = strtok($3, ",");
		      rectangle->x = atoi(p);
		      p = strtok(NULL, ",");
		      rectangle->y = atoi(p);
		      p = strtok(NULL, ",");
		      rectangle->width = atoi(p) - rectangle->x;
		      p = strtok(NULL, ",");
		      rectangle->height = atoi(p) - rectangle->y;
		   } else if (current_type == CIRCLE) {
		      Circle_t *circle;

		      circle = ObjectToCircle(current_object);
		      p = strtok($3, ",");
		      circle->x = atoi(p);
		      p = strtok(NULL, ",");
		      circle->y = atoi(p);
		      p = strtok(NULL, ",");
		      circle->r = atoi(p);
		   } else if (current_type == POLYGON) {
		      Polygon_t *polygon = ObjectToPolygon(current_object);
		      GList *points;
		      GdkPoint *point, *first;
		      gint x, y;

		      p = strtok($3, ",");
		      x = atoi(p);
		      p = strtok(NULL, ",");
		      y = atoi(p);
		      point = new_point(x, y);
		      points = g_list_append(NULL, (gpointer) point);

		      while(1) {
			 p = strtok(NULL, ",");
			 if (!p)
			    break;
			 x = atoi(p);
			 p = strtok(NULL, ",");
			 y = atoi(p);
			 point = new_point(x, y);
			 points = g_list_append(points, (gpointer) point);
		      }
		      /* Remove last point if duplicate */
		      first = (GdkPoint*) points->data;
		      polygon->points = points;
		      if (first->x == point->x && first->y == point->y)
			 polygon_remove_last_point(polygon);
		      polygon->points = points;
		   }

		   g_free ($3);
		}
		;

href_tag	: HREF '=' STRING
		{
		   if (current_type == UNDEFINED) {
		      g_strreplace(&_map_info->default_url, $3);
		   } else {
		      object_set_url(current_object, $3);
		   }
		   g_free ($3);
		}
		;

nohref_tag	: NOHREF optional_value
		{
		}
		;

optional_value	: /* Empty */
		| '=' STRING
		{
		   g_free ($2);
		}
		;

alt_tag		: ALT '=' STRING
		{
		   object_set_comment(current_object, $3);
		   g_free ($3);
		}
		;

target_tag	: TARGET '=' STRING
		{
		   object_set_target(current_object, $3);
		   g_free ($3);
		}
		;

onmouseover_tag	: ONMOUSEOVER '=' STRING
		{
		   object_set_mouse_over(current_object, $3);
		   g_free ($3);
		}
		;

onmouseout_tag	: ONMOUSEOUT '=' STRING
		{
		   object_set_mouse_out(current_object, $3);
		   g_free ($3);
		}
		;

onfocus_tag	: ONFOCUS '=' STRING
		{
		   object_set_focus(current_object, $3);
		   g_free ($3);
		}
		;

onblur_tag	: ONBLUR '=' STRING
		{
		   object_set_blur(current_object, $3);
		   g_free ($3);
		}
		;

end_map		: '<' END_MAP '>'
		;

%%

static void
csim_error(char* s)
{
   extern FILE *csim_in;
   csim_restart(csim_in);
}

gboolean
load_csim (const char* filename)
{
  gboolean status;
  extern FILE *csim_in;
  csim_in = g_fopen(filename, "r");
  if (csim_in) {
    _map_info = get_map_info();
    status = !csim_parse();
    fclose(csim_in);
  } else {
    status = FALSE;
  }
  return status;
}
