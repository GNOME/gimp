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
#define	DESCRIPTION	263
#define	BEGIN_COMMENT	264
#define	FLOAT	265
#define	COMMENT	266
#define	LINK	267


extern YYSTYPE cern_lval;
