#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#include "expressions.h"

DBL x_val, y_val;

static NODE	*nodes[MAX_STACK_ENTRIES];
static int	num_of_nodes;
static DBL	r_values[MAX_STACK_ENTRIES],
                g_values[MAX_STACK_ENTRIES],
                b_values[MAX_STACK_ENTRIES],
                alfa_values[MAX_STACK_ENTRIES],
                data_values[MAX_STACK_ENTRIES];
static func_type	*func_st[MAX_STACK_ENTRIES];

NODE	**n_top;

DBL	*r_top, *g_top, *b_top,* alfa_top;
DBL	*data_top;



void conv_1_3(void);
void conv_1_4(void);
void conv_3_1(void);
void conv_3_4(void);
void conv_4_1(void);
void conv_4_3(void);
void insert_convert_func(int ifrom, int ito);

/**************************************************

        EVALUATION & COMPILATION OF EXP

**************************************************/

void eval_xy( DBL *result,DBL x, DBL y)
{
  func_type **func;
  int	i;

  x_val= x;
  y_val= y;

  r_top= r_values - 1;
  g_top= g_values - 1;
  b_top= b_values - 1;
  alfa_top= alfa_values - 1;
  n_top= nodes;
  data_top= data_values;
  func= func_st;

  for (i=num_of_nodes; i ; (*func)(), i--, n_top++, func++);

  result[0]= *r_top;
  result[1]= *g_top;
  result[2]= *b_top;
  result[3]= *alfa_top;
}

char find_tof( NODE *n)
{
  int i;
  char tmp,ret=1;

  for (i=0; i < n->noo; i++)
    {	
      tmp = find_tof( (n->offsprings)[i] );
      if (tmp > ret) ret= tmp;
    }
  if (n->op->ret_type == -1)
    n-> tof= ret;
  else
    n-> tof= n->op->ret_type;

  return n->tof;
}

void st_insert( func_type *func, NODE *n )
{
  nodes[num_of_nodes]=n;
  func_st[num_of_nodes]= func;
  num_of_nodes++;
}

void compile(NODE *n)
{
  int 	i;
  int	ret, oret;
  int	cret;

  ret= n->tof;

  cret= ret-1;
  if (cret > 0) cret--;  /* 1,3,4 -=> 0,1,2 */  

  for (i=0; i < n -> noo; i++)
    {
      if ( n -> op-> funcs != NULL)
	if ( ((n->op->funcs)[i][cret]) != NULL)
	  st_insert( ((n->op->funcs)[i][cret]), n );
      compile( (n->offsprings)[i] );
      oret= (n->offsprings)[i]->tof;
      if (ret != oret)
	insert_convert_func(oret, ret);
    }

  if ( n ->op-> funcs != NULL)
    if ( ((n->op->funcs)[i][cret]) != NULL)
      st_insert( ((n->op->funcs)[i][cret]), n );

  if ( (n->op->last_func)[cret] != NULL)
    st_insert( (n->op->last_func)[cret], n );

  for (i=0; i < n->op->nofd; i++)
    {
      *data_top= n->data[i];
      data_top++;
    }
}


void prepare_node( NODE *n)
{
  char 	lastret;

  lastret= find_tof( n );

  num_of_nodes=0;
  data_top=data_values;
  compile( n );

  if (lastret==3) insert_convert_func(3,4);
  if (lastret==1) insert_convert_func(1,4);
}

void conv_1_3(void)
{
  g_top++;  *g_top= *r_top;
  b_top++;  *b_top= *r_top;
}

void conv_1_4(void)
{
  g_top++; *g_top= *r_top; 
  b_top++; *b_top= *r_top;
  alfa_top++;  *alfa_top= 1.0;
}

void conv_3_1(void)
{
  *r_top= *r_top * 0.299 + *g_top * 0.587 + *b_top * 0.114;
  g_top--;
  b_top--;
}

void conv_3_4(void)
{
  alfa_top++; *alfa_top= 1.0;
}

void conv_4_1(void)
{
  *r_top= *r_top * 0.299 + *g_top * 0.587 + *b_top * 0.114;
  g_top--;
  b_top--;
  alfa_top--;
}

void conv_4_3(void)
{
  --alfa_top;
}

void insert_convert_func(int ifrom, int ito)
{
  if (ifrom==ito) return;
  if (ifrom == 1)
    if (ito == 3)
      st_insert( conv_1_3, NULL );
    else
      st_insert( conv_1_4, NULL );
  else
    if (ifrom==3)
      if (ito==1)
	st_insert( conv_3_1, NULL );
      else
	st_insert( conv_3_4, NULL );
    else
      if (ito==1)
	st_insert( conv_4_1, NULL  );	
      else
	st_insert( conv_4_3, NULL );
}

/************************************************/

void destroy_node(NODE *n)
{
  int i;

  if (n==NULL) return;
  for (i=0; i < n->noo; i++)
    destroy_node( (n->offsprings)[i] );
  free( n );
  return;
}


DBL dblrand()
{
  return ((DBL) rand()) / ((DBL) RAND_MAX);
}

void init_random()
{
  srand( time(NULL) );
}


NODE *node_copy(NODE *n)
{
  NODE *nn;
  int	i;

  nn= (NODE *) malloc (sizeof(NODE));

  *nn= *n;
  for (i=0; i<n->noo; i++)
    (nn->offsprings)[i]= node_copy( (n->offsprings)[i] );

  return nn;
}

void expr_fprint(FILE *f, int i, NODE *n)
{
  char   tmp[100];
  int 	j;

  memset(tmp, ' ', 100);
  tmp[i%100]=0;

  if (n->op->nofd == 1)
    { /* float */
      fprintf(f,"%s%.8f\n", tmp, n->data[0]);
      return;
    }
  if (n->op->nofd >= 3)
    {
      fprintf(f, "%s#(", tmp);
      for (j=0; j < n->op->nofd - 1; j++)
	fprintf(f,"%.8f ",n->data[j]);
      fprintf(f,"%.8f",n->data[j]);
      fprintf(f, ")\n");
      return;
    }

  fprintf (f, "%s(%s\n",tmp, n->op->name);
  for (j=0; j<n->noo; j++)
    expr_fprint(f, i+2, (n->offsprings)[j] );
  fprintf (f, "%s)\n",tmp);
}

int expr_sprint(char *s, NODE *n)
{
  int 	len=0,j,t;

  if (n->op->nofd == 1)
    { /* float */
      return sprintf(s,"%.8f\n", n->data[0]);
    }

  if (n->op->nofd >= 3)
    {
      len= sprintf(s,"#(");
      s+=len;
      for (j=0; j < n->op->nofd - 1; j++)
	{
	  t= sprintf(s,"%.8f ",n->data[j]);
	  s+= t;
	  len+=t;
	}
      t= sprintf(s,"%.8f)",n->data[j]);
      s+=t; len+=t;
      return len;
    }

  len= sprintf (s,"(%s", n->op->name);
  s+=len;

  for (j=0; j<n->noo; j++)
    {
      *s=' ';
      s++; len++;
      t= expr_sprint(s, (n->offsprings)[j] ); 
      s+= t; len+= t;
    }
  *s= ')';
  s++; len++;
  *s='\000';
  return len;
}

/******************************

  FUNCTIONS FOR PARSING EXPRESSIONS

******************************/

static int find_op(char *c)
{
  int i;
  for (i=0; i< Number_of_Functions; i++)
    if (strcmp(c,FuncInfo[i].name)==0) return i;
  return -1;
}

#define EAT_EMPTY_SPACE(PTR)	\
       while (isspace(*PTR)) (PTR)++;

#define PERROR(S)		\
   {				\
     int j;			\
     if (S!=NULL)		\
       printf("Error: %s\n",S);	\
     for (j=0; j < params; j++)	\
       destroy_node(offs[j]);	\
     return NULL;		\
   }				

NODE  *parse_prefix_expression(char **c)
{
  int 	p, params;
  int	i;
  NODE	*n;
  DBL	v;
  NODE	*(offs[MAX_NUMBER_OF_PARAMETERS]);
  char	buff[50];


  EAT_EMPTY_SPACE(*c);

  params=0;
  if (**c=='(')   /* operator */
    {
      char *tmp;
      char  vtmp;	
      (*c)++;
      EAT_EMPTY_SPACE(*c);
      tmp= *c;
      while ((!isspace(**c))&&(**c!=')')) (*c)++;
      vtmp= **c;
      **c= 0;
      if ((p= find_op(tmp))==-1)
	{sprintf(buff,"unknown operator: %s",tmp); PERROR(buff) };
      **c= vtmp;
      for (i=0;i<=MAX_NUMBER_OF_PARAMETERS;i++, params++)
	{
	  EAT_EMPTY_SPACE(*c);
	  if (**c == ')') break;
	  offs[i]= parse_prefix_expression(c);
	  if (offs[i]==NULL) 
	    PERROR((char *)NULL);
	}

      if (**c != ')')
	  PERROR("')' expected");

      (*c)++;
      if (((FuncInfo[p].nofp == -1) && (i==0)) ||
	  ((FuncInfo[p].nofp != -1) && (i!= FuncInfo[p].nofp)))
	PERROR("wrong number of parameters");
    
      n= (NODE *) malloc(sizeof(NODE));
      n->op= &(FuncInfo[p]);
      n->noo=i;
      memcpy((n->offsprings), offs, i * sizeof(NODE *));

      return n;
    }
  if ((**c=='.')||(**c=='+')||(**c=='-')|| isdigit(**c))
      {
	v= strtod(*c, c);
	
	if ((p= find_op("const"))==-1)
	  PERROR("Warning: there is no const in expression.tmpl");

	n= (NODE *) malloc(sizeof(NODE));
	n->op= &(FuncInfo[p]);
	n->noo=0;
	(n->data)[0]= v;
	return n;
      }
  if (**c=='#')
    {
      DBL tmp[10];
      int i;

      (*c)++;
      EAT_EMPTY_SPACE(*c);
      if (**c != '(')
	PERROR("'(' expected");
      (*c)++;
      for ( i = 0;  i < 5; i++)
	{
	  tmp[i]= strtod(*c, c);
	  EAT_EMPTY_SPACE(*c);
	  if (**c==')') break;
	}
      if ((i != 2) && (i != 3)) PERROR("Wrong number of vector components");
      (*c)++;

      n= (NODE *) malloc(sizeof(NODE));
      n->op= &(FuncInfo[ find_op( (i==2) ? "vector3" : "vector4" ) ]);
      n->noo=0;
      for (; i >=0; i--)
	(n->data)[i]=tmp[i];
      return n;
    }

  sprintf(buff,"Unexpected character: %c",**c);
  PERROR(buff);
}


DBL double_xor (DBL p1, DBL p2)
{
#if DBL_SIZE==8
 
  int	*i1, *i2, *i3;
  DBL	ret;

  i1= (int *) &p1;
  i2= (int *) &p2;
  i3= (int *) &ret;

  *i3= (*i1) ^ (*i2);
  i1++; i2++; i3++;
  *i3= (*i1) ^ (*i2);
  
  return ret;
#else
# error "Please rewrite this function for your architecture... :-)"
#endif
}

DBL double_or (DBL p1, DBL p2)
{
#if DBL_SIZE==8
 
  int	*i1, *i2, *i3;
  DBL	ret;

  i1= (int *) &p1;
  i2= (int *) &p2;
  i3= (int *) &ret;

  *i3= (*i1) | (*i2);
  i1++; i2++; i3++;
  *i3= (*i1) | (*i2);
  
  return ret;
#else
# error "Please rewrite this function for your architecture... :-)"
#endif
}

DBL double_and (DBL p1, DBL p2)
{
#if  DBL_SIZE==8
 
  int	*i1, *i2, *i3;
  DBL	ret;

  i1= (int *) &p1;
  i2= (int *) &p2;
  i3= (int *) &ret;

  *i3= (*i1) & (*i2);
  i1++; i2++; i3++;
  *i3= (*i1) * (*i2);
  
  return ret;
#else
# error "Please rewrite this function for your architecture... :-)"
#endif
}

unsigned char (*wrap_func)(DBL)= wrap_func_rep;

unsigned char wrap_func_cut(DBL n)
{
  if (n < 0.0) return 0;
  if (n > 1.0) return 255;
  return (unsigned char) (n*255.0);
}

unsigned char wrap_func_rep(DBL n)
{
  return (unsigned char) (255.0 * (n - floor(n)));
}

unsigned char wrap_func_pingpong(DBL n)
{
 if ( ((int ) (n - floor(n))) & 1 )
   return  (unsigned char) ((1.0 - n + floor(n)) * 255 );
 else
   return (unsigned char) (255.0 * (n - floor(n)));
}

/******************************

        NOISE

 I was in hurry so I borrowed code from snoise plug-in
 by Marcelo de Gomensoro Malheiros

  Thank you !

******************************/

#define TABLE_SIZE	256
#define	WEIGHT(T)   	((2.0*fabs(T) - 3.0)*(T)*(T)+1.0) 

typedef struct {
  DBL x, y;
} Vector2d;

static int perm_tab[TABLE_SIZE]= {
   254,  20, 180,  81, 171,  17,  83,   7,  18,  72, 154,  11,  12, 134, 251,  15,
   173,  33, 252, 109,  21,   2, 101, 124,  24, 149, 136, 181, 112,  22, 220,  68,
    67, 164,  87, 125,  44, 244,  38,  55,  40, 248, 138, 226,  36, 139, 103,  88,
    75,  49,  74,  43,  16, 216, 210,  90, 203, 150,  58,  59,  29,  26, 159,  63,
    64, 167,  66, 253, 105,  69, 232, 250,  50, 147,  95, 225,   4, 175,  71, 132,
    80, 162, 200, 118, 247, 185,  45,   5,  91, 104,  97, 186, 187, 128,  41,  99,
    96,  39, 131, 196, 148, 184, 102,  78,  34,  77, 106, 133, 211, 212, 108, 143,
   137, 240,  51,  60, 215, 224, 152, 119, 120, 183,   6, 130, 158, 114, 229, 110,
    14,  61, 166, 156, 227,  84,  57, 231,  92,  31, 161,   1, 209, 178, 142,  73,
   129, 145,   3, 201,  98,  23, 177,  56,  25, 188, 127,  30, 202,   9, 219, 169,
   160,  28, 198, 163, 135, 165, 193, 197,   8, 123, 117, 207, 192,  93,  19,  82,
   126,  46, 214,  53, 155, 189, 208, 235, 113, 213,   0,  13,  27, 153, 190,  52,
   151, 100, 194,  86,  10, 206, 255, 245, 176,  35, 172, 146, 204, 205, 236, 111,
    42, 140,  54, 122, 233,  70,  94, 141, 121, 195, 218,  37, 191, 221, 170, 157,
   239, 241, 179, 182, 228, 249, 230, 168,  89, 174, 234,  79, 107, 237, 238,  65,
    85, 223, 242, 243, 222,  48, 116,  47,  62, 246, 199,  76,  32, 115, 144, 217
};

static Vector2d grad_tab[TABLE_SIZE]= {
    { -0.340904,   0.940098},  { -0.933457,  -0.358689},  { -0.776360,  -0.630290},
    {  0.925639,  -0.378408},  {  0.084779,  -0.996400},  {  0.127247,  -0.991871},
    { -0.527260,   0.849704},  { -0.468794,  -0.883308},  {  0.454024,   0.890989},
    {  0.971369,   0.237577},  { -0.016268,   0.999868},  {  0.905447,   0.424459},
    { -0.083460,  -0.996511},  {  0.333623,   0.942707},  { -0.378842,  -0.925461},
    { -0.003822,   0.999993},  { -0.993272,  -0.115804},  {  0.784827,  -0.619715},
    {  0.999963,  -0.008588},  {  0.067801,  -0.997699},  { -0.431667,  -0.902033},
    {  0.836585,  -0.547838},  {  0.868525,  -0.495645},  { -0.694388,   0.719601},
    {  0.955062,  -0.296405},  {  0.966409,  -0.257010},  {  0.785965,  -0.618271},
    {  0.431877,  -0.901932},  {  0.801459,  -0.598050},  { -0.567396,  -0.823445},
    { -0.401807,   0.915724},  {  0.150691,   0.988581},  { -0.284235,  -0.958755},
    { -0.855926,   0.517098},  { -0.857330,  -0.514767},  {  0.581591,   0.813482},
    {  0.026909,  -0.999638},  { -0.997178,   0.075076},  {  0.168251,  -0.985744},
    { -0.952632,   0.304125},  {  0.982900,  -0.184139},  {  0.804087,  -0.594511},
    { -0.774009,  -0.633175},  { -0.869432,   0.494052},  { -0.568740,  -0.822518},
    { -0.667502,   0.744608},  { -0.903558,  -0.428467},  {  0.999705,  -0.024288},
    {  0.857228,   0.514937},  {  0.779031,  -0.626985},  { -0.254156,  -0.967163},
    {  0.797137,   0.603798},  {  0.352136,   0.935949},  {  0.192778,  -0.981242},
    {  0.908687,   0.417477},  { -0.976788,   0.214207},  { -0.947997,  -0.318280},
    {  0.989617,  -0.143730},  { -0.918658,  -0.395053},  {  0.984556,  -0.175068},
    {  0.828093,   0.560591},  {  0.796336,   0.604854},  { -0.402179,  -0.915561},
    { -0.791991,   0.610532},  { -0.238364,   0.971176},  {  0.012233,   0.999925},
    { -0.992889,  -0.119040},  { -0.391251,   0.920284},  {  0.500683,   0.865631},
    { -0.996647,  -0.081818},  {  0.631211,   0.775611},  {  0.632632,   0.774453},
    {  0.994086,   0.108596},  { -0.349918,   0.936780},  {  0.528357,  -0.849022},
    {  0.264896,  -0.964277},  {  0.886484,   0.462760},  { -0.456045,   0.889957},
    {  0.948901,   0.315574},  { -0.762707,   0.646744},  {  0.975860,   0.218399},
    {  0.966273,   0.257521},  { -0.785780,  -0.618506},  { -0.729822,   0.683638},
    {  0.409543,   0.912291},  { -0.815786,   0.578354},  { -0.754687,  -0.656085},
    { -0.945713,   0.325003},  { -0.722036,  -0.691855},  {  0.071182,  -0.997463},
    { -0.622102,  -0.782936},  {  0.126728,   0.991937},  {  0.841445,   0.540343},
    { -0.714751,   0.699379},  {  0.792850,  -0.609417},  {  0.419202,  -0.907893},
    {  0.975360,   0.220620},  { -0.924087,   0.382183},  {  0.605739,  -0.795663},
    { -0.762163,   0.647385},  { -0.774114,   0.633046},  {  0.999734,   0.023045},
    { -0.921373,  -0.388681},  { -0.665487,   0.746410},  { -0.589795,  -0.807553},
    { -0.568207,   0.822886},  {  0.921615,   0.388105},  { -0.272729,  -0.962091},
    {  0.675038,  -0.737783},  {  0.313405,   0.949620},  {  0.741975,   0.670428},
    { -0.497474,  -0.867479},  { -0.864656,   0.502364},  {  0.975093,   0.221797},
    { -0.989268,  -0.146109},  {  0.980503,  -0.196505},  { -0.798513,  -0.601978},
    {  0.240607,  -0.970623},  { -0.393014,   0.919533},  {  0.084637,  -0.996412},
    { -0.467677,  -0.883899},  {  0.062876,   0.998021},  { -0.903742,  -0.428078},
    {  0.983263,  -0.182191},  {  0.251135,   0.967952},  { -0.970324,   0.241807},
    {  0.126711,  -0.991940},  {  0.274262,   0.961655},  {  0.997314,  -0.073242},
    { -0.923689,   0.383144},  {  0.496499,  -0.868037},  { -0.053434,  -0.998571},
    { -0.933781,  -0.357845},  { -0.851126,   0.524961},  {  0.853828,   0.520555},
    {  0.271485,  -0.962443},  {  0.365644,  -0.930755},  {  0.017294,  -0.999850},
    { -0.850236,   0.526402},  {  0.679557,   0.733622},  { -0.245285,   0.969451},
    {  0.985317,  -0.170734},  {  0.457752,  -0.889080},  {  0.981181,   0.193092},
    {  0.298175,  -0.954511},  {  0.824137,   0.566390},  {  0.218332,  -0.975875},
    { -0.508714,  -0.860935},  {  0.875234,  -0.483699},  {  0.508538,  -0.861039},
    { -0.207871,  -0.978156},  {  0.424732,   0.905319},  { -0.383099,   0.923707},
    {  0.181913,  -0.983315},  {  0.888078,   0.459693},  { -0.379999,  -0.924987},
    { -0.028810,  -0.999585},  {  0.196453,  -0.980513},  {  0.029305,  -0.999571},
    {  0.221861,  -0.975078},  {  0.668131,  -0.744044},  {  0.682511,  -0.730875},
    {  0.058054,  -0.998313},  { -0.085181,  -0.996365},  {  0.225010,  -0.974357},
    {  0.180953,  -0.983492},  { -0.806028,  -0.591877},  {  0.998739,   0.050204},
    { -0.989319,   0.145767},  {  0.172447,   0.985019},  { -0.935325,   0.353790},
    { -0.908229,   0.418473},  {  0.959731,   0.280919},  {  0.840561,   0.541718},
    {  0.991777,  -0.127978},  {  0.682358,  -0.731018},  {  0.574395,  -0.818578},
    { -0.121657,  -0.992572},  { -0.432900,  -0.901442},  { -0.999856,   0.016945},
    { -0.550735,  -0.834680},  { -0.978364,   0.206890},  {  0.943332,   0.331851},
    {  0.166796,   0.985991},  {  0.222451,   0.974944},  {  0.765695,   0.643204},
    { -0.449596,  -0.893232},  { -0.984817,  -0.173596},  {  0.290036,   0.957016},
    { -0.638124,   0.769934},  {  0.102617,   0.994721},  { -0.757623,  -0.652692},
    { -0.983166,   0.182717},  {  0.977482,  -0.211021},  {  0.739869,   0.672751},
    { -0.942317,   0.334721},  { -0.515641,   0.856805},  {  0.270002,   0.962860},
    { -0.598114,  -0.801411},  {  0.313478,   0.949596},  {  0.229444,   0.973322},
    { -0.697821,  -0.716272},  {  0.062743,  -0.998030},  { -0.158614,   0.987341},
    { -0.529376,  -0.848387},  {  0.191143,  -0.981562},  {  0.601182,  -0.799112},
    {  0.976461,   0.215694},  { -0.893194,  -0.449672},  { -0.786058,   0.618153},
    { -0.999201,   0.039958},  { -0.991278,   0.131790},  {  0.475348,  -0.879798},
    {  0.399412,  -0.916772},  {  0.157454,  -0.987526},  {  0.998391,   0.056713},
    { -0.942481,   0.334260},  { -0.041131,  -0.999154},  {  0.501194,   0.865335},
    { -0.649762,  -0.760137},  {  0.682994,   0.730424},  {  0.717300,   0.696765},
    { -0.362297,   0.932063},  {  0.980846,   0.194787},  {  0.037116,  -0.999311},
    { -0.999897,  -0.014356},  { -0.919892,  -0.392171},  { -0.950399,   0.311032},
    {  0.683221,   0.730212},  {  0.702763,  -0.711424},  {  0.945516,   0.325574},
    { -0.728458,  -0.685091},  { -0.791297,   0.611433},  { -0.707706,   0.706507},
    { -0.342698,   0.939446},  {  0.585173,  -0.810908},  {  0.637824,  -0.770182},
    {  0.232521,   0.972591},  {  0.814065,   0.580774},  { -0.488897,  -0.872342},
    { -0.486172,  -0.873863},  {  0.219057,  -0.975712},  {  0.876775,  -0.480900},
    {  0.446701,  -0.894683},  {  0.954116,   0.299437},  { -0.201992,   0.979387},
    {  0.613498,   0.789696},  { -0.062938,  -0.998017},  { -0.711028,   0.703164},
    { -0.175823,   0.984422},  {  0.867027,   0.498261},  {  0.170418,  -0.985372},
    { -0.993594,   0.113005},  { -0.999964,   0.008470},  { -0.236945,   0.971523},
    {  0.993534,   0.113539}
};


DBL
solid_noise (DBL x, DBL y)
{
  Vector2d v;
  int a, b, i, j, n;
  int	ha, hb;
  DBL sum;

  sum = 0.0;

  x*=  5.;  /* GAG operates on interval <0,1>,<0,1> so noise (x,y) would be */
  y*=  5.;  /* so uninteresting when unscaled :-) */

  a = (int) floor (x);
  b = (int) floor (y);


  ha= (a % TABLE_SIZE) + TABLE_SIZE;  /* to avoid negative values of a or b */
  hb= (b % TABLE_SIZE) + TABLE_SIZE;

  for (i = 0; i < 2; i++)
    for (j = 0; j < 2; j++) {
      n = perm_tab[(ha + i + perm_tab[(hb + j) % TABLE_SIZE]) % TABLE_SIZE];
      v.x = x - a - i;
      v.y = y - b - j;
      sum += WEIGHT(v.x) * WEIGHT(v.y) * (grad_tab[n].x * v.x + grad_tab[n].y * v.y);
    }

  return sum;
}

/******************************

  expression library initialisation

******************************/


void expr_init()
{
  init_random();
}


