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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __GENERAL_H__
#define __GENERAL_H__


#include <stdio.h>


#define TOKEN_EOF         0
#define TOKEN_LEFT_PAREN  1
#define TOKEN_RIGHT_PAREN 2
#define TOKEN_SYMBOL      3
#define TOKEN_STRING      4
#define TOKEN_NUMBER      5


typedef struct _ParseInfo  ParseInfo;

struct _ParseInfo
{
  FILE *fp;
  char *buffer;
  char *tokenbuf;
  int linenum;
  int charnum;
  int position;
  int buffer_size;
  int tokenbuf_size;
  unsigned int inc_linenum : 1;
  unsigned int inc_charnum : 1;
};


char * prune_filename (char *);
char * search_in_path (char *, char *);
char * xstrsep (char **p, char *delim);
int    get_token (ParseInfo *info);
int    find_token (char *line, char *token_r, int maxlen);
char * iso_8601_date_format (char *user_buf, int strict);

extern char *token_str;
extern char *token_sym;
extern double token_num;
extern int token_int;


#endif /* __GENERAL_H__ */
