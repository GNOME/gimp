#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {
   int val;
   double value;
   char id[256];
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	RECTANGLE	257
# define	POLYGON	258
# define	CIRCLE	259
# define	DEFAULT	260
# define	AUTHOR	261
# define	TITLE	262
# define	DESCRIPTION	263
# define	BEGIN_COMMENT	264
# define	FLOAT	265
# define	LINK	266
# define	COMMENT	267


extern YYSTYPE ncsa_lval;

#endif /* not BISON_Y_TAB_H */
