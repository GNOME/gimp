typedef union {
   int val;
   double value;
   char id[256];
} YYSTYPE;
#define	RECTANGLE	258
#define	POLYGON	259
#define	CIRCLE	260
#define	DEFAULT	261
#define	AUTHOR	262
#define	TITLE	263
#define	DESCRIPTION	264
#define	BEGIN_COMMENT	265
#define	FLOAT	266
#define	LINK	267
#define	COMMENT	268


extern YYSTYPE ncsa_lval;
