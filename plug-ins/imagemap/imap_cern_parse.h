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
#define	DESCRIPTION	262
#define	BEGIN_COMMENT	263
#define	FLOAT	264
#define	COMMENT	265
#define	LINK	266


extern YYSTYPE cern_lval;
