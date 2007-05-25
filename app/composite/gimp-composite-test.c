#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "base/base-types.h"

#include "gimp-composite.h"
#include "gimp-composite-regression.h"
#include "gimp-composite-util.h"
#include "gimp-composite-generic.h"

int
gimp_composite_regression(int iterations, int n_pixels)
{
  GimpCompositeContext generic_ctx;
  GimpCompositeContext special_ctx;
  double ft0;
  double ft1;
  gimp_rgba8_t *rgba8D1;
  gimp_rgba8_t *rgba8D2;
  gimp_rgba8_t *rgba8A;
  gimp_rgba8_t *rgba8B;
  gimp_rgba8_t *rgba8M;
  gimp_va8_t *va8A;
  gimp_va8_t *va8B;
  gimp_va8_t *va8M;
  gimp_va8_t *va8D1;
  gimp_va8_t *va8D2;
  int i;

  rgba8A =  (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);
  rgba8B =  (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);
  rgba8M =  (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);
  rgba8D1 = (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);
  rgba8D2 = (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);
  va8A =    (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);
  va8B =    (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);
  va8M =    (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);
  va8D1 =   (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);
  va8D2 =   (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);

  for (i = 0; i < n_pixels; i++) {
    rgba8A[i].r = 255-i;
    rgba8A[i].g = 255-i;
    rgba8A[i].b = 255-i;
    rgba8A[i].a = 255-i;

    rgba8B[i].r = i;
    rgba8B[i].g = i;
    rgba8B[i].b = i;
    rgba8B[i].a = i;

    rgba8M[i].r = i;
    rgba8M[i].g = i;
    rgba8M[i].b = i;
    rgba8M[i].a = i;

    va8A[i].v = i;
    va8A[i].a = 255-i;
    va8B[i].v = i;
    va8B[i].a = i;
    va8M[i].v = i;
    va8M[i].a = i;
  }

  return EXIT_SUCCESS;
}

int
main(int argc, char *argv[])
{
  int iterations;
  int n_pixels;

  srand(314159);

  iterations = 1;
  n_pixels = 256*256;

  return gimp_composite_regression (iterations, n_pixels);
}
