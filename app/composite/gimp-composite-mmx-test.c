#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>

#include <glib-object.h>

#include "base/base-types.h"

#include "gimp-composite.h"
#include "gimp-composite-dispatch.h"
#include "gimp-composite-regression.h"
#include "gimp-composite-util.h"
#include "gimp-composite-generic.h"
#include "gimp-composite-mmx.h"

int
gimp_composite_mmx_test(int iterations, int n_pixels)
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


  /* gimp_composite_multiply_rgba8_rgba8_rgba8 */
  memset((void *) &special_ctx, 0, sizeof(special_ctx));
  special_ctx.op = GIMP_COMPOSITE_MULTIPLY;
  special_ctx.n_pixels = n_pixels;
  special_ctx.scale.scale = 2;
  special_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.A = (unsigned char *) rgba8A;
  special_ctx.B = (unsigned char *) rgba8B;
  special_ctx.M = (unsigned char *) rgba8B;
  special_ctx.D = (unsigned char *) rgba8D1;
  memset(special_ctx.D, 0, special_ctx.n_pixels * gimp_composite_pixel_bpp[special_ctx.pixelformat_D]);
  memset((void *) &generic_ctx, 0, sizeof(special_ctx));
  generic_ctx.op = GIMP_COMPOSITE_MULTIPLY;
  generic_ctx.n_pixels = n_pixels;
  generic_ctx.scale.scale = 2;
  generic_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.A = (unsigned char *) rgba8A;
  generic_ctx.B = (unsigned char *) rgba8B;
  generic_ctx.M = (unsigned char *) rgba8B;
  generic_ctx.D = (unsigned char *) rgba8D2;
  memset(generic_ctx.D, 0, generic_ctx.n_pixels * gimp_composite_pixel_bpp[generic_ctx.pixelformat_D]);
  ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_multiply_rgba8_rgba8_rgba8_mmx, &special_ctx);
  gimp_composite_regression_compare_contexts("multiply", &generic_ctx, &special_ctx);
  gimp_composite_regression_timer_report("multiply", ft0, ft1);

  /* gimp_composite_screen_rgba8_rgba8_rgba8 */
  memset((void *) &special_ctx, 0, sizeof(special_ctx));
  special_ctx.op = GIMP_COMPOSITE_SCREEN;
  special_ctx.n_pixels = n_pixels;
  special_ctx.scale.scale = 2;
  special_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.A = (unsigned char *) rgba8A;
  special_ctx.B = (unsigned char *) rgba8B;
  special_ctx.M = (unsigned char *) rgba8B;
  special_ctx.D = (unsigned char *) rgba8D1;
  memset(special_ctx.D, 0, special_ctx.n_pixels * gimp_composite_pixel_bpp[special_ctx.pixelformat_D]);
  memset((void *) &generic_ctx, 0, sizeof(special_ctx));
  generic_ctx.op = GIMP_COMPOSITE_SCREEN;
  generic_ctx.n_pixels = n_pixels;
  generic_ctx.scale.scale = 2;
  generic_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.A = (unsigned char *) rgba8A;
  generic_ctx.B = (unsigned char *) rgba8B;
  generic_ctx.M = (unsigned char *) rgba8B;
  generic_ctx.D = (unsigned char *) rgba8D2;
  memset(generic_ctx.D, 0, generic_ctx.n_pixels * gimp_composite_pixel_bpp[generic_ctx.pixelformat_D]);
  ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_screen_rgba8_rgba8_rgba8_mmx, &special_ctx);
  gimp_composite_regression_compare_contexts("screen", &generic_ctx, &special_ctx);
  gimp_composite_regression_timer_report("screen", ft0, ft1);

  /* gimp_composite_difference_rgba8_rgba8_rgba8 */
  memset((void *) &special_ctx, 0, sizeof(special_ctx));
  special_ctx.op = GIMP_COMPOSITE_DIFFERENCE;
  special_ctx.n_pixels = n_pixels;
  special_ctx.scale.scale = 2;
  special_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.A = (unsigned char *) rgba8A;
  special_ctx.B = (unsigned char *) rgba8B;
  special_ctx.M = (unsigned char *) rgba8B;
  special_ctx.D = (unsigned char *) rgba8D1;
  memset(special_ctx.D, 0, special_ctx.n_pixels * gimp_composite_pixel_bpp[special_ctx.pixelformat_D]);
  memset((void *) &generic_ctx, 0, sizeof(special_ctx));
  generic_ctx.op = GIMP_COMPOSITE_DIFFERENCE;
  generic_ctx.n_pixels = n_pixels;
  generic_ctx.scale.scale = 2;
  generic_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.A = (unsigned char *) rgba8A;
  generic_ctx.B = (unsigned char *) rgba8B;
  generic_ctx.M = (unsigned char *) rgba8B;
  generic_ctx.D = (unsigned char *) rgba8D2;
  memset(generic_ctx.D, 0, generic_ctx.n_pixels * gimp_composite_pixel_bpp[generic_ctx.pixelformat_D]);
  ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_difference_rgba8_rgba8_rgba8_mmx, &special_ctx);
  gimp_composite_regression_compare_contexts("difference", &generic_ctx, &special_ctx);
  gimp_composite_regression_timer_report("difference", ft0, ft1);

  /* gimp_composite_addition_rgba8_rgba8_rgba8 */
  memset((void *) &special_ctx, 0, sizeof(special_ctx));
  special_ctx.op = GIMP_COMPOSITE_ADDITION;
  special_ctx.n_pixels = n_pixels;
  special_ctx.scale.scale = 2;
  special_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.A = (unsigned char *) rgba8A;
  special_ctx.B = (unsigned char *) rgba8B;
  special_ctx.M = (unsigned char *) rgba8B;
  special_ctx.D = (unsigned char *) rgba8D1;
  memset(special_ctx.D, 0, special_ctx.n_pixels * gimp_composite_pixel_bpp[special_ctx.pixelformat_D]);
  memset((void *) &generic_ctx, 0, sizeof(special_ctx));
  generic_ctx.op = GIMP_COMPOSITE_ADDITION;
  generic_ctx.n_pixels = n_pixels;
  generic_ctx.scale.scale = 2;
  generic_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.A = (unsigned char *) rgba8A;
  generic_ctx.B = (unsigned char *) rgba8B;
  generic_ctx.M = (unsigned char *) rgba8B;
  generic_ctx.D = (unsigned char *) rgba8D2;
  memset(generic_ctx.D, 0, generic_ctx.n_pixels * gimp_composite_pixel_bpp[generic_ctx.pixelformat_D]);
  ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_addition_rgba8_rgba8_rgba8_mmx, &special_ctx);
  gimp_composite_regression_compare_contexts("addition", &generic_ctx, &special_ctx);
  gimp_composite_regression_timer_report("addition", ft0, ft1);

  /* gimp_composite_subtract_rgba8_rgba8_rgba8 */
  memset((void *) &special_ctx, 0, sizeof(special_ctx));
  special_ctx.op = GIMP_COMPOSITE_SUBTRACT;
  special_ctx.n_pixels = n_pixels;
  special_ctx.scale.scale = 2;
  special_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.A = (unsigned char *) rgba8A;
  special_ctx.B = (unsigned char *) rgba8B;
  special_ctx.M = (unsigned char *) rgba8B;
  special_ctx.D = (unsigned char *) rgba8D1;
  memset(special_ctx.D, 0, special_ctx.n_pixels * gimp_composite_pixel_bpp[special_ctx.pixelformat_D]);
  memset((void *) &generic_ctx, 0, sizeof(special_ctx));
  generic_ctx.op = GIMP_COMPOSITE_SUBTRACT;
  generic_ctx.n_pixels = n_pixels;
  generic_ctx.scale.scale = 2;
  generic_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.A = (unsigned char *) rgba8A;
  generic_ctx.B = (unsigned char *) rgba8B;
  generic_ctx.M = (unsigned char *) rgba8B;
  generic_ctx.D = (unsigned char *) rgba8D2;
  memset(generic_ctx.D, 0, generic_ctx.n_pixels * gimp_composite_pixel_bpp[generic_ctx.pixelformat_D]);
  ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_subtract_rgba8_rgba8_rgba8_mmx, &special_ctx);
  gimp_composite_regression_compare_contexts("subtract", &generic_ctx, &special_ctx);
  gimp_composite_regression_timer_report("subtract", ft0, ft1);

  /* gimp_composite_darken_rgba8_rgba8_rgba8 */
  memset((void *) &special_ctx, 0, sizeof(special_ctx));
  special_ctx.op = GIMP_COMPOSITE_DARKEN;
  special_ctx.n_pixels = n_pixels;
  special_ctx.scale.scale = 2;
  special_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.A = (unsigned char *) rgba8A;
  special_ctx.B = (unsigned char *) rgba8B;
  special_ctx.M = (unsigned char *) rgba8B;
  special_ctx.D = (unsigned char *) rgba8D1;
  memset(special_ctx.D, 0, special_ctx.n_pixels * gimp_composite_pixel_bpp[special_ctx.pixelformat_D]);
  memset((void *) &generic_ctx, 0, sizeof(special_ctx));
  generic_ctx.op = GIMP_COMPOSITE_DARKEN;
  generic_ctx.n_pixels = n_pixels;
  generic_ctx.scale.scale = 2;
  generic_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.A = (unsigned char *) rgba8A;
  generic_ctx.B = (unsigned char *) rgba8B;
  generic_ctx.M = (unsigned char *) rgba8B;
  generic_ctx.D = (unsigned char *) rgba8D2;
  memset(generic_ctx.D, 0, generic_ctx.n_pixels * gimp_composite_pixel_bpp[generic_ctx.pixelformat_D]);
  ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_darken_rgba8_rgba8_rgba8_mmx, &special_ctx);
  gimp_composite_regression_compare_contexts("darken", &generic_ctx, &special_ctx);
  gimp_composite_regression_timer_report("darken", ft0, ft1);

  /* gimp_composite_lighten_rgba8_rgba8_rgba8 */
  memset((void *) &special_ctx, 0, sizeof(special_ctx));
  special_ctx.op = GIMP_COMPOSITE_LIGHTEN;
  special_ctx.n_pixels = n_pixels;
  special_ctx.scale.scale = 2;
  special_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.A = (unsigned char *) rgba8A;
  special_ctx.B = (unsigned char *) rgba8B;
  special_ctx.M = (unsigned char *) rgba8B;
  special_ctx.D = (unsigned char *) rgba8D1;
  memset(special_ctx.D, 0, special_ctx.n_pixels * gimp_composite_pixel_bpp[special_ctx.pixelformat_D]);
  memset((void *) &generic_ctx, 0, sizeof(special_ctx));
  generic_ctx.op = GIMP_COMPOSITE_LIGHTEN;
  generic_ctx.n_pixels = n_pixels;
  generic_ctx.scale.scale = 2;
  generic_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.A = (unsigned char *) rgba8A;
  generic_ctx.B = (unsigned char *) rgba8B;
  generic_ctx.M = (unsigned char *) rgba8B;
  generic_ctx.D = (unsigned char *) rgba8D2;
  memset(generic_ctx.D, 0, generic_ctx.n_pixels * gimp_composite_pixel_bpp[generic_ctx.pixelformat_D]);
  ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_lighten_rgba8_rgba8_rgba8_mmx, &special_ctx);
  gimp_composite_regression_compare_contexts("lighten", &generic_ctx, &special_ctx);
  gimp_composite_regression_timer_report("lighten", ft0, ft1);

  /* gimp_composite_divide_rgba8_rgba8_rgba8 */
  memset((void *) &special_ctx, 0, sizeof(special_ctx));
  special_ctx.op = GIMP_COMPOSITE_DIVIDE;
  special_ctx.n_pixels = n_pixels;
  special_ctx.scale.scale = 2;
  special_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.A = (unsigned char *) rgba8A;
  special_ctx.B = (unsigned char *) rgba8B;
  special_ctx.M = (unsigned char *) rgba8B;
  special_ctx.D = (unsigned char *) rgba8D1;
  memset(special_ctx.D, 0, special_ctx.n_pixels * gimp_composite_pixel_bpp[special_ctx.pixelformat_D]);
  memset((void *) &generic_ctx, 0, sizeof(special_ctx));
  generic_ctx.op = GIMP_COMPOSITE_DIVIDE;
  generic_ctx.n_pixels = n_pixels;
  generic_ctx.scale.scale = 2;
  generic_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.A = (unsigned char *) rgba8A;
  generic_ctx.B = (unsigned char *) rgba8B;
  generic_ctx.M = (unsigned char *) rgba8B;
  generic_ctx.D = (unsigned char *) rgba8D2;
  memset(generic_ctx.D, 0, generic_ctx.n_pixels * gimp_composite_pixel_bpp[generic_ctx.pixelformat_D]);
  ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_divide_rgba8_rgba8_rgba8_mmx, &special_ctx);
  gimp_composite_regression_compare_contexts("divide", &generic_ctx, &special_ctx);
  gimp_composite_regression_timer_report("divide", ft0, ft1);

  /* gimp_composite_dodge_rgba8_rgba8_rgba8 */
  memset((void *) &special_ctx, 0, sizeof(special_ctx));
  special_ctx.op = GIMP_COMPOSITE_DODGE;
  special_ctx.n_pixels = n_pixels;
  special_ctx.scale.scale = 2;
  special_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.A = (unsigned char *) rgba8A;
  special_ctx.B = (unsigned char *) rgba8B;
  special_ctx.M = (unsigned char *) rgba8B;
  special_ctx.D = (unsigned char *) rgba8D1;
  memset(special_ctx.D, 0, special_ctx.n_pixels * gimp_composite_pixel_bpp[special_ctx.pixelformat_D]);
  memset((void *) &generic_ctx, 0, sizeof(special_ctx));
  generic_ctx.op = GIMP_COMPOSITE_DODGE;
  generic_ctx.n_pixels = n_pixels;
  generic_ctx.scale.scale = 2;
  generic_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.A = (unsigned char *) rgba8A;
  generic_ctx.B = (unsigned char *) rgba8B;
  generic_ctx.M = (unsigned char *) rgba8B;
  generic_ctx.D = (unsigned char *) rgba8D2;
  memset(generic_ctx.D, 0, generic_ctx.n_pixels * gimp_composite_pixel_bpp[generic_ctx.pixelformat_D]);
  ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_dodge_rgba8_rgba8_rgba8_mmx, &special_ctx);
  gimp_composite_regression_compare_contexts("dodge", &generic_ctx, &special_ctx);
  gimp_composite_regression_timer_report("dodge", ft0, ft1);

  /* gimp_composite_burn_rgba8_rgba8_rgba8 */
  memset((void *) &special_ctx, 0, sizeof(special_ctx));
  special_ctx.op = GIMP_COMPOSITE_BURN;
  special_ctx.n_pixels = n_pixels;
  special_ctx.scale.scale = 2;
  special_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.A = (unsigned char *) rgba8A;
  special_ctx.B = (unsigned char *) rgba8B;
  special_ctx.M = (unsigned char *) rgba8B;
  special_ctx.D = (unsigned char *) rgba8D1;
  memset(special_ctx.D, 0, special_ctx.n_pixels * gimp_composite_pixel_bpp[special_ctx.pixelformat_D]);
  memset((void *) &generic_ctx, 0, sizeof(special_ctx));
  generic_ctx.op = GIMP_COMPOSITE_BURN;
  generic_ctx.n_pixels = n_pixels;
  generic_ctx.scale.scale = 2;
  generic_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.A = (unsigned char *) rgba8A;
  generic_ctx.B = (unsigned char *) rgba8B;
  generic_ctx.M = (unsigned char *) rgba8B;
  generic_ctx.D = (unsigned char *) rgba8D2;
  memset(generic_ctx.D, 0, generic_ctx.n_pixels * gimp_composite_pixel_bpp[generic_ctx.pixelformat_D]);
  ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_burn_rgba8_rgba8_rgba8_mmx, &special_ctx);
  gimp_composite_regression_compare_contexts("burn", &generic_ctx, &special_ctx);
  gimp_composite_regression_timer_report("burn", ft0, ft1);

  /* gimp_composite_swap_rgba8_rgba8_rgba8 */
  memset((void *) &special_ctx, 0, sizeof(special_ctx));
  special_ctx.op = GIMP_COMPOSITE_SWAP;
  special_ctx.n_pixels = n_pixels;
  special_ctx.scale.scale = 2;
  special_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.A = (unsigned char *) rgba8A;
  special_ctx.B = (unsigned char *) rgba8B;
  special_ctx.M = (unsigned char *) rgba8B;
  special_ctx.D = (unsigned char *) rgba8D1;
  memset(special_ctx.D, 0, special_ctx.n_pixels * gimp_composite_pixel_bpp[special_ctx.pixelformat_D]);
  memset((void *) &generic_ctx, 0, sizeof(special_ctx));
  generic_ctx.op = GIMP_COMPOSITE_SWAP;
  generic_ctx.n_pixels = n_pixels;
  generic_ctx.scale.scale = 2;
  generic_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.A = (unsigned char *) rgba8A;
  generic_ctx.B = (unsigned char *) rgba8B;
  generic_ctx.M = (unsigned char *) rgba8B;
  generic_ctx.D = (unsigned char *) rgba8D2;
  memset(generic_ctx.D, 0, generic_ctx.n_pixels * gimp_composite_pixel_bpp[generic_ctx.pixelformat_D]);
  ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_swap_rgba8_rgba8_rgba8_mmx, &special_ctx);
  gimp_composite_regression_compare_contexts("swap", &generic_ctx, &special_ctx);
  gimp_composite_regression_timer_report("swap", ft0, ft1);

  /* gimp_composite_scale_rgba8_rgba8_rgba8 */
  memset((void *) &special_ctx, 0, sizeof(special_ctx));
  special_ctx.op = GIMP_COMPOSITE_SCALE;
  special_ctx.n_pixels = n_pixels;
  special_ctx.scale.scale = 2;
  special_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  special_ctx.A = (unsigned char *) rgba8A;
  special_ctx.B = (unsigned char *) rgba8B;
  special_ctx.M = (unsigned char *) rgba8B;
  special_ctx.D = (unsigned char *) rgba8D1;
  memset(special_ctx.D, 0, special_ctx.n_pixels * gimp_composite_pixel_bpp[special_ctx.pixelformat_D]);
  memset((void *) &generic_ctx, 0, sizeof(special_ctx));
  generic_ctx.op = GIMP_COMPOSITE_SCALE;
  generic_ctx.n_pixels = n_pixels;
  generic_ctx.scale.scale = 2;
  generic_ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.pixelformat_M = GIMP_PIXELFORMAT_RGBA8;
  generic_ctx.A = (unsigned char *) rgba8A;
  generic_ctx.B = (unsigned char *) rgba8B;
  generic_ctx.M = (unsigned char *) rgba8B;
  generic_ctx.D = (unsigned char *) rgba8D2;
  memset(generic_ctx.D, 0, generic_ctx.n_pixels * gimp_composite_pixel_bpp[generic_ctx.pixelformat_D]);
  ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_scale_rgba8_rgba8_rgba8_mmx, &special_ctx);
  gimp_composite_regression_compare_contexts("scale", &generic_ctx, &special_ctx);
  gimp_composite_regression_timer_report("scale", ft0, ft1);
  return (0);
}

int
main(int argc, char *argv[])
{
  int iterations;
  int n_pixels;

  srand(314159);

  putenv("GIMP_COMPOSITE=0x1");

  iterations = 1;
  n_pixels = 512*512;

  gimp_composite_generic_install();

  return (gimp_composite_mmx_test(iterations, n_pixels));
}
