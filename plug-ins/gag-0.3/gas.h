#ifndef _GAS_H_
#define _GAS_H_

#include "expressions.h"

extern int expression_max_depth;
extern DBL gas_const_prob;

extern DBL (*gas_leaf_probability)(int depth);		/* determines if end expression now or not */
extern DBL (*gas_operator_weight)(int index);		/* weight of operator */
extern DBL (*random_number)();

DBL gas_leaf_prob_const(int);
DBL gas_leaf_prob_desc(int);

DBL gas_operator_weight_const(int);

NODE *gas_random(int, int);
NODE *gas_mate(DBL, NODE *, NODE *);

/*
  functions & variables for MUTATING
*/

extern DBL gas_mutate_prob;

typedef NODE *(*gas_mutate_func)(NODE *, NODE *, int );

typedef struct  {
  gas_mutate_func	func;
  char			*desc;
  DBL			weight;
  char			noo;		/* how many arguments must have such node to aply this mutate function ? */
} gas_mutate_methods_struct;		/* -1 .. arbitrary, 
					   -2 - greater than zero (ie non constant), 
					   -3 nofd != 0 
					   -4 op->nofp == -1
					 */

NODE *gas_mutate( NODE *, NODE *, int );

extern int Number_of_MutFunc;
extern gas_mutate_methods_struct  MutFuncs[];

DBL gas_mfw_array( int, NODE *, int );
DBL gas_mfw_const( int, NODE *, int );

extern DBL  (*gas_get_mf_weight)( int, NODE *, int);

#endif

