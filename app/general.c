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
#include <glib.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#ifdef G_OS_WIN32
#endif

#include "general.h"

/*****/


gchar   *token_str;
gchar   *token_sym;
gdouble  token_num;
gint     token_int;

gint
get_token (ParseInfo *info)
{
  gchar *buffer;
  gchar *tokenbuf;
  gint   tokenpos = 0;
  gint   state;
  gint   count;
  gint   slashed;

  state    = 0;
  buffer   = info->buffer;
  tokenbuf = info->tokenbuf;
  slashed  = FALSE;

  while (1)
    {
      if (info->inc_charnum && info->charnum)
	info->charnum += 1;
      if (info->inc_linenum && info->linenum)
	{
	  info->linenum += 1;
	  info->charnum = 1;
	}

      info->inc_linenum = FALSE;
      info->inc_charnum = FALSE;

      if (info->position >= (info->buffer_size - 1))
	info->position = -1;
      if ((info->position == -1) || (buffer[info->position] == '\0'))
	{
	  count = fread (buffer, sizeof (char), info->buffer_size - 1, info->fp);
	  if ((count == 0) && feof (info->fp))
	    return TOKEN_EOF;
	  buffer[count] = '\0';
	  info->position = 0;
	}

      info->inc_charnum = TRUE;
      if ((buffer[info->position] == '\n') ||
	  (buffer[info->position] == '\r'))
	info->inc_linenum = TRUE;

      switch (state)
	{
	case 0:
	  if (buffer[info->position] == '#')
	    {
	      info->position += 1;
	      state = 4;
	    }
	  else if (buffer[info->position] == '(')
	    {
	      info->position += 1;
	      return TOKEN_LEFT_PAREN;
	    }
	  else if (buffer[info->position] == ')')
	    {
	      info->position += 1;
	      return TOKEN_RIGHT_PAREN;
	    }
	  else if (buffer[info->position] == '"')
	    {
	      info->position += 1;
	      state = 1;
	      slashed = FALSE;
	    }
	  else if ((buffer[info->position] == '-') || isdigit (buffer[info->position]))
	    {
	      tokenbuf[tokenpos++] = buffer[info->position];
	      info->position += 1;
	      state = 3;
	    }
	  else if ((buffer[info->position] != ' ') &&
		   (buffer[info->position] != '\t') &&
		   (buffer[info->position] != '\n') &&
		   (buffer[info->position] != '\r'))
	    {
	      tokenbuf[tokenpos++] = buffer[info->position];
	      info->position += 1;
	      state = 2;
	    }
	  else if (buffer[info->position] == '\'')
	    {
	      info->position += 1;
	      state = 2;
	    }
	  else
	    {
	      info->position += 1;
	    }
	  break;
	case 1:
	  if ((buffer[info->position] == '\\') && (!slashed))
	    {
	      slashed = TRUE;
	      info->position += 1;
	    }
	  else if (slashed || (buffer[info->position] != '"'))
	    {
	      if (slashed && buffer[info->position] == 'n')
		tokenbuf[tokenpos++] = '\n';
	      else if (slashed && buffer[info->position] == 'r')
		tokenbuf[tokenpos++] = '\r';
	      else if (slashed && buffer[info->position] == 'z') /* ^Z */
		tokenbuf[tokenpos++] = '\032';
	      else if (slashed && buffer[info->position] == '0') /* \0 */
		tokenbuf[tokenpos++] = '\0';
	      else if (slashed && buffer[info->position] == '"')
		tokenbuf[tokenpos++] = '"';
	      else
	        tokenbuf[tokenpos++] = buffer[info->position];
	      slashed = FALSE;
	      info->position += 1;
	    }
	  else
	    {
	      tokenbuf[tokenpos] = '\0';
	      token_str = tokenbuf;
	      token_sym = tokenbuf;
              token_int = tokenpos;
	      info->position += 1;
	      return TOKEN_STRING;
	    }
	  break;
	case 2:
	  if ((buffer[info->position] != ' ') &&
	      (buffer[info->position] != '\t') &&
	      (buffer[info->position] != '\n') &&
	      (buffer[info->position] != '\r') &&
	      (buffer[info->position] != '"') &&
	      (buffer[info->position] != '(') &&
	      (buffer[info->position] != ')'))
	    {
	      tokenbuf[tokenpos++] = buffer[info->position];
	      info->position += 1;
	    }
	  else
	    {
	      tokenbuf[tokenpos] = '\0';
	      token_sym = tokenbuf;
	      return TOKEN_SYMBOL;
	    }
	  break;
	case 3:
	  if (isdigit (buffer[info->position]) ||
	      (buffer[info->position] == '.'))
	    {
	      tokenbuf[tokenpos++] = buffer[info->position];
	      info->position += 1;
	    }
	  else if ((buffer[info->position] != ' ') &&
		   (buffer[info->position] != '\t') &&
		   (buffer[info->position] != '\n') &&
		   (buffer[info->position] != '\r') &&
		   (buffer[info->position] != '"') &&
		   (buffer[info->position] != '(') &&
		   (buffer[info->position] != ')'))
	    {
	      tokenbuf[tokenpos++] = buffer[info->position];
	      info->position += 1;
	      state = 2;
	    }
	  else
	    {
	      tokenbuf[tokenpos] = '\0';
	      token_sym = tokenbuf;
	      token_num = atof (tokenbuf);
	      token_int = atoi (tokenbuf);
	      return TOKEN_NUMBER;
	    }
	  break;
	case 4:
	  if (buffer[info->position] == '\n')
	    state = 0;
	  info->position += 1;
	  break;
	}
    }
}

/* Parse "line" and look for a string of characters without spaces
   following a '(' and copy them into "token_r".  Return the number of
   characters stored, or 0. */
gint
find_token (gchar *line,
	    gchar *token_r,
	    gint   maxlen)
{
  gchar *sp;
  gchar *dp;
  gint   i;

  /* FIXME: This should be replaced by a more intelligent parser which
     checks for '\', '"' and nested parentheses.  See get_token().  */
  for (sp = line; *sp; sp++)
    if (*sp == '(' || *sp == '#')
      break;
  if (*sp == '\000' || *sp == '#')
    {
      *token_r = '\000';
      return 0;
    }
  dp = token_r;
  sp++;
  i = 0;
  while ((*sp != '\000') && (*sp != ' ') && (*sp != '\t') && i < maxlen)
    {
      *dp = *sp;
      dp++;
      sp++;
      i++;
    }
  if (*sp == '\000' || i >= maxlen)
    {
      *token_r = '\000';
      return 0;
    }
  *dp = '\000';
  return i;
}

/* Generate a string containing the date and time and return a pointer
   to it.  If user_buf is not NULL, the string is stored in that
   buffer (21 bytes).  If strict is FALSE, the 'T' between the date
   and the time is replaced by a space, which makes the format a bit
   more readable (IMHO). */
const gchar *
iso_8601_date_format (gchar *user_buf,
		      gint   strict)
{
  static gchar  static_buf[21];
  gchar        *buf;
  time_t        clock;
  struct tm    *now;

  if (user_buf != NULL)
    buf = user_buf;
  else
    buf = static_buf;

  clock = time (NULL);
  now = gmtime (&clock);

  /* date format derived from ISO 8601:1988 */
  sprintf (buf, "%04d-%02d-%02d%c%02d:%02d:%02d%c",
	   now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
	   (strict ? 'T' : ' '),
	   now->tm_hour, now->tm_min, now->tm_sec,
	   (strict ? 'Z' : '\000'));

  return buf;
}
