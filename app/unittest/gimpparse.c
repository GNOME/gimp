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

#include "unittest.h"
#include "globals.c"
#if 1
#include "../gimprc.c"
#endif
char * get_personal(const char *filename)
{
  char *full_filename = gimp_personal_rc_file(filename);
  
  g_print ("%-50s = gimp_personal_rc_file(\"%s\");\n",
	   full_filename, filename);
  return (full_filename);
}


void dump_ParseInfo(ParseInfo *p)
{
    printf("*** ParseInfo ***\n");
    printf("fp=%p ",p->fp);
    printf("linenum=%d ",p->linenum);
    printf("charnum=%d ",p->charnum);
    printf("position=%d\n",p->position);
    printf("buffer_size=%d ",p->buffer_size);
    printf("tokenbuf_size=%d\n",p->tokenbuf_size); 
    printf("buffer=>%s<\n",p->buffer);
    printf("tokenbuf=>%s<\n",p->tokenbuf);   
}

int
get_token2 (ParseInfo *info)
{
  char *buffer;
  char *tokenbuf;
  int tokenpos = 0;
  int state;
  int count;
  int slashed;

  dump_ParseInfo(info);
  printf("get_token2 %p\n",info);
  state = 0;
  buffer = info->buffer;
  tokenbuf = info->tokenbuf;
  slashed = FALSE;

  while (1)
    {
	  printf("get_token2 %p\n",info);
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

static int
peek_next_token2 ()
{
    printf("peek_next_token next_token=%d\n",next_token);
  if (next_token == -1)
    next_token = get_token2 (&parse_info);

  return next_token;
}
static int
parse_statement2 ()
{
  int token;
  int i;
  
  printf(" parse_statement2\n");
  token = peek_next_token2 ();
  if (!token)
    return DONE;
  printf("token=%d\n",token);
  if (token != TOKEN_LEFT_PAREN)
    return ERROR;
  token = get_next_token ();
printf("token=%d\n",token);
  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL))
    return ERROR;
  token = get_next_token ();
printf("token=%d\n",token);
  for (i = 0; i < nfuncs; i++)
    if (strcmp (funcs[i].name, token_sym) == 0)
      switch (funcs[i].type)
	{
	case TT_STRING:
	  return parse_string (funcs[i].val1p, funcs[i].val2p);
	case TT_PATH:
	  return parse_path (funcs[i].val1p, funcs[i].val2p);
	case TT_DOUBLE:
	  return parse_double (funcs[i].val1p, funcs[i].val2p);
	case TT_FLOAT:
	  return parse_float (funcs[i].val1p, funcs[i].val2p);
	case TT_INT:
	  return parse_int (funcs[i].val1p, funcs[i].val2p);
	case TT_BOOLEAN:
	  return parse_boolean (funcs[i].val1p, funcs[i].val2p);
	case TT_POSITION:
	  return parse_position (funcs[i].val1p, funcs[i].val2p);
	case TT_MEMSIZE:
	  return parse_mem_size (funcs[i].val1p, funcs[i].val2p);
	case TT_IMAGETYPE:
	  return parse_image_type (funcs[i].val1p, funcs[i].val2p);
	case TT_XCOLORCUBE:
	  return parse_color_cube (funcs[i].val1p, funcs[i].val2p);
	case TT_XPREVSIZE:
	  return parse_preview_size (funcs[i].val1p, funcs[i].val2p);
	case TT_XUNIT:
	  return parse_units (funcs[i].val1p, funcs[i].val2p);
	case TT_XPLUGIN:
	  return parse_plug_in (funcs[i].val1p, funcs[i].val2p);
	case TT_XPLUGINDEF:
	  return parse_plug_in_def (funcs[i].val1p, funcs[i].val2p);
	case TT_XMENUPATH:
	  return parse_menu_path (funcs[i].val1p, funcs[i].val2p);
	case TT_XDEVICE:
	  return parse_device (funcs[i].val1p, funcs[i].val2p);
	case TT_XSESSIONINFO:
	  return parse_session_info (funcs[i].val1p, funcs[i].val2p);
	case TT_XUNITINFO:
	  return parse_unit_info (funcs[i].val1p, funcs[i].val2p);
	case TT_XPARASITE:
	  return parse_parasite (funcs[i].val1p, funcs[i].val2p);
	}

  return parse_unknown (token_sym);
}

void
parse_gimprc_file2 (char *filename)
{
  int status;
  char rfilename[MAXPATHLEN];
#if 1
  char *gimp_dir;

  gimp_dir = gimp_directory ();
   printf(" SET GLOBAL variables !!!\n");
  add_gimp_directory_token (gimp_dir);
#ifdef __EMX__
  add_x11root_token(getenv("X11ROOT"));
#endif
  parse_buffers_init();
/*  next_token = -1;*/
#endif
  printf("parse_gimprc_file2 %s\n",filename);
  if (!g_path_is_absolute (filename))
    {
      g_snprintf (rfilename, MAXPATHLEN, "%s" G_DIR_SEPARATOR_S "%s",
		  g_get_home_dir (), filename);
      filename = rfilename;
    }
  printf("parse_gimprc_file2 %s\n",filename);
  parse_info.fp = fopen (filename, "rt");
  printf("fp=%p\n",parse_info.fp);
  if (!parse_info.fp)
  {
      g_print (_("error opening: \"%s\"\n"), filename);
      return;
  }

  if ((be_verbose == TRUE) || (no_splash == TRUE))
    g_print (_("parsing \"%s\"\n"), filename);

  cur_token = -1;
  next_token = -1;
  printf("set parse_info\n");
  
  parse_info.position = -1;
  parse_info.linenum = 1;
  parse_info.charnum = 1;
  parse_info.inc_linenum = FALSE;
  parse_info.inc_charnum = FALSE;
  printf("set parse_info done\n");
  dump_ParseInfo(&parse_info);
  done = FALSE;
  while ((status = parse_statement ()) == OK)
      printf("status= %d\n",status);
  dump_ParseInfo(&parse_info);

  fclose (parse_info.fp);

  if (status == ERROR)
    {
      g_print (_("error parsing: \"%s\"\n"), filename);
      g_print (_("  at line %d column %d\n"), parse_info.linenum, parse_info.charnum);
      g_print (_("  unexpected token: %s\n"), token_sym);
    }
}

void parse_gimp_files(char *rcfile)
{
    char *filename = get_personal(rcfile);
    
    printf ("call parse_gimprc_file filename=%s\n",filename);
    parse_gimprc_file2 (filename);

}

int
main (int argc, char **argv)
{
    if (argc < 2)
    {
	printf ("%s <rc file>\n",argv[0]);
	printf ("ex: %s gimprc\n",argv[0]);
	printf ("ex: %s unitrc\n",argv[0]);
	printf ("ex: %s menurc\n",argv[0]);
	printf ("ex: %s pluginrc\n",argv[0]);
	printf ("ex: %s sessionrc\n",argv[0]);
	printf ("ex: %s devicerc\n",argv[0]);
	printf ("ex: %s ideas\n",argv[0]);
	exit (0);
    }
    parse_gimp_files(argv[1]);
}

