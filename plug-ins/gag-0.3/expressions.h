#ifndef _EXPRESSIONS_H_
#define _EXPRESSIONS_H_

#ifdef DEBUG
#	define DPRINT(S)	{ printf("Debug: %s\n",S); }
#	define DMPRINT(S,I,J,K)	{ printf(S,I,J,K); }
#else
#	define DPRINT(S)
#	define DMPRINT(SI,J,K)
#endif

#define		INFINITY		 	1e100

#define 	MAX_NUMBER_OF_PARAMETERS   	10
#define		MAX_TREE_DEPTH			20

#define		MAX_STACK_ENTRIES		1000  /* max number of operators in one expression 
							 I hope it is enough :-)			*/

typedef double 			DBL;
typedef struct expr_node_struct NODE;
typedef struct func_info_struct FUNC_INFO;

typedef	void 	(func_type)(void);

typedef	func_type (*expr_func[3]);


struct func_info_struct {
  char 		*name;
  

  char		nofp;			/* number of parameters,  nofp < 0 -=> arbitrary number of parameters */
  char		nofd;			/* number of data entries (doubles) - for vectors and doubles	nofp==0 || nofd==0 */ 
  char 		ret_type;		/* type of return: 1,3,4, -1 (whatever) */
  char		*paramtypes;

  expr_func	*funcs;			
  expr_func	last_func;		/* this function is inserted to the stack as last... */
};

struct expr_node_struct {
  FUNC_INFO		*op;

  DBL			data[4];		/* float values - such as constants and vectors ((rgb) or (rgbt) vectors) */
  NODE			*offsprings[MAX_NUMBER_OF_PARAMETERS];	/* too lazy to mallocate.... */
  int			noo;			/* number of offsprings in expression tree */
  char			tof;			/* type of return,.. */
};

struct	expr_compiled_struct {
  func_type	*funcs;
  DBL		*data;
  NODE		*nodes;
};

extern FUNC_INFO FuncInfo[];
extern int Number_of_Functions;

extern DBL x_val, y_val;

void eval_xy( DBL *, DBL, DBL);
void prepare_node( NODE *n);
NODE *node_copy(NODE *n);
void destroy_node( NODE *n);
void expr_fprint( FILE *, int, NODE *);
int expr_sprint(char *s, NODE *n);

/*
         Some useful functions
*/

NODE  *parse_prefix_expression(char **string);

DBL  	dblrand();
void   	init_random();
void 	expr_init();
DBL 	solid_noise (DBL, DBL);

DBL double_and (DBL p1, DBL p2);
DBL double_xor (DBL p1, DBL p2);
DBL double_or  (DBL p1, DBL p2);


/* this pointers are necessary for computation */

extern NODE	**n_top;
extern DBL	*r_top, *g_top, *b_top,* alfa_top;
extern DBL	*data_top;

/* wrapping functions - serve to project interval <-infty,infty> into <0..255> */
/* simple: real to color... */

extern unsigned char (*wrap_func)(DBL);

unsigned char wrap_func_cut(DBL);
unsigned char wrap_func_rep(DBL);
unsigned char wrap_func_pingpong(DBL);

#endif
