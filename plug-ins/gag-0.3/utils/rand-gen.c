#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DBL double

#define TABLE_SIZE	256

typedef struct {
  DBL x, y;
} Vector2d;

static int perm_tab[TABLE_SIZE];
static Vector2d grad_tab[TABLE_SIZE];


void main(void)
{
  int i, j, k, t;
  DBL m;

  srand( time(NULL) );

  for (i = 0; i < TABLE_SIZE; i++)
    perm_tab[i] = i;
  for (i = 0; i < TABLE_SIZE; i++) {
    j = rand () % TABLE_SIZE;
    k = rand () % TABLE_SIZE;
    t = perm_tab[j];
    perm_tab[j] = perm_tab[k];
    perm_tab[k] = t;
  }
  printf("static int perm_tab[TABLE_SIZE]= {\n  ");
  for (i=0; i < TABLE_SIZE; i++)
    {
      if (i != 0) printf(",");
      if ((i!= 0) && ((i & 15) == 0)) printf("\n  ");
      printf(" %3d", perm_tab[i]);
    }
  printf("\n};\n\n");

  /*  Initialize the gradient table  */

  printf("static Vector2d grad_tab[TABLE_SIZE]= {\n  ");
  for (i = 0; i < TABLE_SIZE; i++) {
    do {
      grad_tab[i].x = (double)(rand () - (RAND_MAX >> 1)) / (RAND_MAX >> 1);
      grad_tab[i].y = (double)(rand () - (RAND_MAX >> 1)) / (RAND_MAX >> 1);
      m = grad_tab[i].x * grad_tab[i].x + grad_tab[i].y * grad_tab[i].y;
    } while (m == 0.0 || m > 1.0);
    m = 1.0 / sqrt(m);
    grad_tab[i].x *= m;
    grad_tab[i].y *= m;

    if (i != 0) printf(",");
    if (i!= 0 && (i % 3 ==0)) printf("\n  ");
    printf("  {%10f, %10f}", grad_tab[i].x, grad_tab[i].y);
  }
  printf("\n};\n");
}
