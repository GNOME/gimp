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

#ifndef __GENERAL_H__
#define __GENERAL_H__


#include <stdio.h>


typedef enum
{
  TOKEN_EOF,
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_SYMBOL,
  TOKEN_STRING,
  TOKEN_NUMBER
} TokenDataType;


typedef struct _ParseInfo  ParseInfo;

struct _ParseInfo
{
  FILE  *fp;
  gchar *buffer;
  gchar *tokenbuf;
  gint   linenum;
  gint   charnum;
  gint   position;
  gint   buffer_size;
  gint   tokenbuf_size;
  guint  inc_linenum : 1;
  guint  inc_charnum : 1;
};


gint         get_token            (ParseInfo *info);
gint         find_token           (gchar     *line,
				   gchar     *token_r,
				   gint       maxlen);
const char * iso_8601_date_format (gchar     *user_buf,
				   gint       strict);


extern gchar   *token_str;
extern gchar   *token_sym;
extern gdouble  token_num;
extern gint     token_int;


#endif /* __GENERAL_H__ */
