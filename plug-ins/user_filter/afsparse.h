/*
 * Written 1997 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
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
 *
 */
#ifndef AFSPARSE_H
#define AFSPARSE_H

#define YYSTYPE s_afstree *

#ifdef YYDEBUG
extern int yydebug;
#endif

s_afstree *get_afstree(char *s);
void free_tree(s_afstree *a);
void do_parse(char *s);
int yylex();
int yyparse();

#endif