typedef union {
   int val;
   double value;
   char id[256];
} YYSTYPE;
#define	RECTANGLE	257
#define	POLYGON	258
#define	CIRCLE	259
#define	DEFAULT	260
#define	AUTHOR	261
#define	TITLE	262
#define	DESCRIPTION	263
#define	BEGIN_COMMENT	264
#define	FLOAT	265
#define	LINK	266
#define	COMMENT	267


extern YYSTYPE ncsa_lval;
