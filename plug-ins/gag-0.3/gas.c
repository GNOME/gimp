#include <stdio.h>
#include <stdlib.h>
#include "gas.h"

int	expression_max_depth=10;
DBL 	gas_const_prob= 0.3;
DBL	gas_mutate_prob= 0.2;

DBL (*gas_leaf_probability)(int depth)= gas_leaf_prob_desc;
DBL (*gas_operator_weights)(int index)= gas_operator_weight_const;
DBL (*random_number)()= dblrand;

DBL gas_operator_weight_const(int i)
{
  return 1.0;
}

DBL gas_leaf_prob_const(int depth)
{
  return (depth==expression_max_depth) ? 1.0 : gas_const_prob;
}

DBL gas_leaf_prob_desc(int depth)
{
  return  ((DBL) depth) / ((DBL) expression_max_depth);
}

NODE *gas_random( int depth, int maxdepth)
{
  int	i,j, leafonly=0;
  DBL	m,p;
  NODE	*node;

  if ( dblrand() < gas_leaf_probability(depth)) leafonly=1;
  if ( maxdepth <=0) leafonly= 1; 

  m=0.0;
  for (i=0; i < Number_of_Functions; i++)
    if ((leafonly != 1) || ( FuncInfo[i].nofp == 0))
      m+= gas_operator_weights( i );

  p= dblrand();
  p*= m;

  m= 0.0;
  for (i=0; i < Number_of_Functions; i++)
    {
      if ((leafonly != 1) || (FuncInfo[i].nofp == 0))
	m+= gas_operator_weights( i );
      if (p < m)
	break;
    }

  node= malloc( sizeof(NODE) );

  node->op=  &(FuncInfo[i]);
  node->noo= FuncInfo[i].nofp;

  if (node->noo == 0)
    {
      for (j=0; j<FuncInfo[i].nofd; j++)
	node->data[j]= random_number();
    }
  else
    {
      if (node->noo == -1)
	node->noo= 2 + (rand() % (MAX_NUMBER_OF_PARAMETERS - 1));
      for (j=0; j < node->noo; j++)
	(node->offsprings)[j]= gas_random( depth + 1, maxdepth - 1);
    }
  return node;
}

/*******************************

       MUTATING

******************************/

NODE *gm_alter_vector (NODE *, NODE *, int);
NODE *gm_alter_op (NODE *, NODE *, int);
NODE *gm_alter_noo (NODE *, NODE *, int);		   
NODE *gm_random (NODE *, NODE *, int);
NODE *gm_insert (NODE *, NODE *, int);
NODE *gm_degenerate (NODE *, NODE *, int);
NODE *gm_copy (NODE *, NODE *, int);
NODE *gm_mix_params (NODE *, NODE *, int);
NODE *gm_no_mut (NODE *, NODE *, int);		   

int	Number_of_MutFunc= 9;
gas_mutate_methods_struct  MutFuncs[9] = {
  { gm_alter_vector, 	"Fixme: description", 3.0,	-3},
  { gm_alter_op,	"Fixme: description", 3.0,	-1},
  { gm_alter_noo,	"Fixme: description", 2.0,	-4},
  { gm_mix_params,	"Fixme: description", 1.0,	-2},
  { gm_random,	 	"Fixme: description", 0.4,    	-1},
  { gm_insert,		"Fixme: description", 1.0,	-1},
  { gm_degenerate,	"Fixme: description", 1.0,	-2},
  { gm_copy,		"Fixme: description", /*0.5,*/ 0.0, -1},
  { gm_no_mut,		"Fixme: description", 0.0,	-1}
};


DBL gas_mfw_array( int i, NODE *n, int depth)
{
  int t;

  t= MutFuncs[i].noo;
  if ((t==-3) && (n->op->nofd != 0)) return 0.0;
  if ((t==-2) && (n->noo == 0)) return 0.0;
  if ((t==-4) && (n->op->nofd!=-1)) return 0.0;
  if ((t>= 0) && (n->noo != t)) return 0.0;
  return MutFuncs[i].weight;
}

DBL gas_mfw_const( int i, NODE *n, int depth)
{
  int t;

  t= MutFuncs[i].noo;
  if ((t==-3) && (n->op->nofd != 0)) return 0.0;
  if ((t==-2) && (n->noo == 0)) return 0.0;
  if ((t==-4) && (n->op->nofd!=-1)) return 0.0;
  if ((t>= 0) && (n->noo != t)) return 0.0;
  return 1.0;
}

DBL  (*gas_get_mf_weight)( int, NODE *, int)= gas_mfw_array;

NODE *gas_mutate( NODE *root, NODE *n, int depth)
{
  DBL  	m,p;
  int	i;

  if (dblrand() < gas_mutate_prob)
    {
      m= 0.0;

      for (i=0; i<Number_of_MutFunc; i++)
	{
	  m+= gas_get_mf_weight( i, n, depth);
	}
      p= dblrand() * m;

      m= 0.0;
      for (i=0; i < Number_of_MutFunc; i++)
	{
	  m+= gas_get_mf_weight( i,n,depth );
	  if (p < m) break;
	}

      return MutFuncs[i].func(root, n, depth);
    }
  else
    return gm_no_mut(root, n, depth+1);
}

NODE *gm_alter_vector (NODE *root, NODE *n, int depth)
{
  int i;

  DPRINT("mutate - alter scalar or scalar");  
  for (i=0; i < n->op->nofd; i++)
    n->data[i]= dblrand();
  return n;
}

NODE *gm_alter_op (NODE *root, NODE *n, int depth)
{
  FUNC_INFO 	*newfi, *oldfi;
  int		oo,nn,mm;

  DPRINT("mutate - alter operator");  

  oldfi= n->op;
  newfi= &(FuncInfo[ rand() % Number_of_Functions]);
  n->op= newfi;

  oo= n->noo;
  nn= newfi->nofp;
  if (nn==-1)  
    if (oo == 0)
      nn= 2;
    else
      nn= oo;

  n->noo=nn;

  if (nn < oo)
    {	/* destroy offsprings of no use */
      mm= nn;
      for ( ; nn < oo; nn++)
	destroy_node( (n->offsprings)[nn]);
    }
  else
    {  /* create new random ones... */
      mm=oo;
      for ( ; oo < nn; oo ++)
	(n->offsprings)[oo]= gas_random(depth, 2);
    }

      /* and now mutate old ones ...*/
  for( nn= 0; nn < mm; nn++)
    (n->offsprings)[nn]= gas_mutate(root, (n->offsprings)[nn], 
				    depth+1);

     /* generate necessary random floats... */
  oo= oldfi->nofd;
  nn= newfi->nofd;
  for ( ; oo < nn; oo++)
    (n->data)[oo]= dblrand();

  return n;
}

NODE *gm_alter_noo (NODE *root, NODE *n, int depth)
{
  int		oo,nn,mm;

  DPRINT("mutate - alter number of parameters");  

  if (n->op->nofp != -1)
    {
      for( nn= 0; nn < n->noo; nn++)
	(n->offsprings)[nn]= gas_mutate(root, (n->offsprings)[nn], 
					depth+1);    
    }

  oo= n->noo;
  nn= 2 + (rand() % (MAX_NUMBER_OF_PARAMETERS - 1));
  n->noo=nn;

  if (nn < oo)
    {	/* destroy offsprings of no use */
      mm= nn;
      for ( ; nn < oo; nn++)
	destroy_node( (n->offsprings)[nn]);
    }
  else
    {  /* create new random ones... */
      mm=oo;
      for ( ; oo < nn; oo ++)
	(n->offsprings)[oo]= gas_random(depth,2);
    }

      /* and now mutate old ones ...*/
  for( nn= 0; nn < mm; nn++)
    (n->offsprings)[nn]= gas_mutate(root, (n->offsprings)[nn], 
				    depth+1);

  return n;
}

NODE *gm_random (NODE *root, NODE *n, int depth)
{
  DPRINT("mutate - random");  
  destroy_node( n );
  return gas_random(depth,2);
}


NODE *gm_insert (NODE *root, NODE *n, int depth)
{
  NODE *nnode;
  int	j;

  DPRINT("mutate - insert");  
  nnode= gas_random(depth,2); /*generate random operand */

  if (nnode->noo == 0) 
    {	
      destroy_node(nnode);
      for (j=0; j < n->noo; j++)
	(n->offsprings)[j]=gas_mutate(root,(n->offsprings)[j],depth+1);
      return n;
    }

  j= rand() % (nnode->noo);
  destroy_node((nnode->offsprings)[j]);
  (nnode->offsprings)[j]= n;

  for (j=0; j < n->noo; j++)
    (n->offsprings)[j]= gas_mutate( root, (n->offsprings)[j], depth+2 );
  
  return nnode;
}


NODE *gm_degenerate (NODE *root, NODE *n, int depth)
{
  int 	i,j;
  NODE	*nn;
  if (n->noo==0) return n;

  DPRINT("mutate - degenerate");  
  j= rand() % (n->noo);
  for (i=0; i < n->noo; i++)
    if (i!=j)
      destroy_node( (n-> offsprings)[i] );
    else
      nn= gas_mutate(root, (n->offsprings) [i], depth);
  free( n );
  return nn;
}

NODE *gm_mix_params (NODE *root, NODE *n, int depth)
{
  NODE *tmp;
  int	k,i,j;

  DPRINT("mutate - mix");  

  if (n->noo < 2)
    {
      for (k=0; k < n->noo; k++)
	(n->offsprings)[k]= gas_mutate(root, n->offsprings[k], depth+1);
      return n;
    }

  for( k= (rand() % (n->noo)); k; k--)
    {
      i= rand() % (n->noo);
      j= rand() % (n->noo);
      if (i==j) i= (i+1) % (n->noo);

      tmp= (n->offsprings)[i];
      (n->offsprings)[i]= (n->offsprings)[j];
      (n->offsprings)[j]= tmp;
    }

  for (k=0; k < n->noo; k++)
    (n->offsprings)[k]= gas_mutate(root, n->offsprings[k], depth+1);
  return n;
}

NODE *gm_copy (NODE *root, NODE *n, int depth)
{
  DPRINT("mutate - copy");
  printf ("Attention - not yet implemented...\n");
  return n;
}

NODE *gm_no_mut (NODE *root, NODE *n, int depth)
{
  int i;

  for (i=0; i < n->noo; i++)
    (n->offsprings)[i]= gas_mutate(root, n->offsprings[i], depth+1);
  return n;
}

/******************************

          MATING

  disclaimer: please read following lines of code only if you are over
  18 years of age and you are not offensed by C source code

******************************/

NODE *gas_mate(DBL w, NODE *n1, NODE *n2)
{
  NODE  *nn;
  int	i,j;

  nn= (NODE *) malloc(sizeof(NODE));
  
  if (dblrand() < w) 
    *nn= *n1;
  else
    *nn= *n2;

  for (i= 0; i < nn->noo; i++)
    {
      if ((i < n1->noo) && (i < n2->noo))
	(nn->offsprings)[i]= 
	  gas_mate(w, (n1->offsprings)[i],(n2->offsprings)[i]);

      if ( i >= n1->noo)
	(nn->offsprings)[i]= node_copy( (n2->offsprings)[i] );

      if ( i >= n2->noo)
	(nn->offsprings)[i]= node_copy( (n1->offsprings)[i] );
    }

  j= n1->op->nofd;

  if (j > (n2->op->nofd)) j= n2->op->nofd;

  for ( i=0; i<j; i++)
    (nn->data)[i]=((n1->data)[i]+(n2->data)[i])/2.0;

  return nn;
}
