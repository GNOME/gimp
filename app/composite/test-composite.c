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


int
main (int argc, char *argv[])
{
  GimpCompositeContext ctx;
  GimpCompositeContext ctx_generic;
  GimpCompositeContext ctx_va8;
  GimpCompositeContext ctx_va8_generic;
  int iterations;
  gimp_rgba8_t *d1;
  gimp_rgba8_t *d2;
  gimp_rgba8_t *rgba8A;
  gimp_rgba8_t *rgba8B;
  gimp_va8_t *va8A;
  gimp_va8_t *va8B;
  gimp_va8_t *va8_d1;
  gimp_va8_t *va8_d2;
		double ft0, ft1;
  unsigned long i;
  unsigned long n_pixels;

		iterations = 1;
		n_pixels = 256*256+1;

		if (argc > 1) {
				iterations = atoi(argv[1]);
				if (argc > 2) {
						n_pixels = atol(argv[2]);
				}
		}

		printf("iterations %d, n_pixels %lu\n", iterations, n_pixels);

  rgba8A = (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);
  rgba8B = (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);
  va8A =   (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);
  va8B =   (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);
  d1 =     (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);
  d2 =     (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);
  va8_d1 = (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);
  va8_d2 = (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);

  srand(314159);

  for (i = 0; i < n_pixels; i++) {
#if 0
    rgba8A[i].r = rand() % 256;
    rgba8A[i].g = rand() % 256;
    rgba8A[i].b = rand() % 256;
    rgba8A[i].a = rand() % 256;

    rgba8B[i].r = rand() % 256;
    rgba8B[i].g = rand() % 256;
    rgba8B[i].b = rand() % 256;
    rgba8B[i].a = rand() % 256;
#else
    rgba8A[i].r = 255-i;
    rgba8A[i].g = 255-i;
    rgba8A[i].b = 255-i;
    rgba8A[i].a = 255-i;

    rgba8B[i].r = i;
    rgba8B[i].g = i;
    rgba8B[i].b = i;
    rgba8B[i].a = i;

    va8A[i].v = i;
    va8A[i].a = 255-i;
    va8B[i].v = i;
    va8B[i].a = i;
#endif
  }

  gimp_composite_init();

#define do_add
#define do_darken
#define do_difference
#define do_lighten
#define do_multiply
#define do_subtract
#define do_screen
#define do_grainextract
#define do_grainmerge
#define do_divide
#define do_dodge
#define do_swap
#define do_scale
#define do_burn

  ctx.A = (unsigned char *) rgba8A;
  ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  ctx.B = (unsigned char *) rgba8B;
  ctx.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  ctx.D = (unsigned char *) d2;
  ctx.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  ctx.M = NULL;
  ctx.pixelformat_M = GIMP_PIXELFORMAT_ANY;
  ctx.n_pixels = n_pixels;
  ctx.scale.scale = 2;

  ctx_generic.A = (unsigned char *) rgba8A;
  ctx_generic.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;
  ctx_generic.B = (unsigned char *) rgba8B;
  ctx_generic.pixelformat_B = GIMP_PIXELFORMAT_RGBA8;
  ctx_generic.D = (unsigned char *) d1;
  ctx_generic.pixelformat_D = GIMP_PIXELFORMAT_RGBA8;
  ctx_generic.M = NULL;
  ctx_generic.pixelformat_M = GIMP_PIXELFORMAT_ANY;
  ctx_generic.n_pixels = n_pixels;
  ctx_generic.scale.scale = 2;

  ctx_va8.A = (unsigned char *) va8A;
  ctx_va8.pixelformat_A = GIMP_PIXELFORMAT_VA8;
  ctx_va8.B = (unsigned char *) va8B;
  ctx_va8.pixelformat_B = GIMP_PIXELFORMAT_VA8;
  ctx_va8.D = (unsigned char *) va8_d2;
  ctx_va8.pixelformat_D = GIMP_PIXELFORMAT_VA8;
  ctx_va8.M = NULL;
  ctx_va8.pixelformat_M = GIMP_PIXELFORMAT_ANY;
  ctx_va8.n_pixels = n_pixels;
  ctx_va8.scale.scale = 2;

  ctx_va8_generic.A = (unsigned char *) va8A;
  ctx_va8_generic.pixelformat_A = GIMP_PIXELFORMAT_VA8;
  ctx_va8_generic.B = (unsigned char *) va8B;
  ctx_va8_generic.pixelformat_B = GIMP_PIXELFORMAT_VA8;
  ctx_va8_generic.D = (unsigned char *) va8_d1;
  ctx_va8_generic.pixelformat_D = GIMP_PIXELFORMAT_VA8;
  ctx_va8_generic.M = NULL;
  ctx_va8_generic.pixelformat_M = GIMP_PIXELFORMAT_ANY;
  ctx_va8_generic.n_pixels = n_pixels;
  ctx_va8_generic.scale.scale = 2;

		printf("%-17s %17s %17s %17s\n", "Operation", "Generic Time", "Optimised Time", "Generic/Optimised");

#ifdef do_burn
  /* burn */
  ctx.op = GIMP_COMPOSITE_BURN;

		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_burn_any_any_any_generic, &ctx_generic);
  gimp_composite_regression_compare_contexts("burn rgba8", &ctx, &ctx_generic);
  gimp_composite_regression_timer_report("burn rgba8", ft0, ft1);

  ctx_va8.op = GIMP_COMPOSITE_BURN;
  ctx_va8_generic.op = GIMP_COMPOSITE_BURN;
		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx_va8);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_burn_any_any_any_generic, &ctx_va8_generic);
  gimp_composite_regression_compare_contexts("burn va8", &ctx_va8, &ctx_va8_generic);
  gimp_composite_regression_timer_report("burn va8", ft0, ft1);
#endif

#ifdef do_dodge
  /* dodge */
  ctx.op = GIMP_COMPOSITE_DODGE;

		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_dodge_any_any_any_generic, &ctx_generic);
  gimp_composite_regression_compare_contexts("dodge", &ctx, &ctx_generic);
  gimp_composite_regression_timer_report("dodge", ft0, ft1);
#endif

#ifdef do_divide
  /* divide */
  ctx.op = GIMP_COMPOSITE_DIVIDE;

		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_divide_any_any_any_generic, &ctx_generic);
  gimp_composite_regression_compare_contexts("divide", &ctx, &ctx_generic);
  gimp_composite_regression_timer_report("divide", ft0, ft1);
#endif

#ifdef do_grainextract
  /* grainextract */
  ctx.op = GIMP_COMPOSITE_GRAIN_EXTRACT;
		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_grain_extract_any_any_any_generic, &ctx_generic);
  gimp_composite_regression_compare_contexts("grain extract", &ctx, &ctx_generic);
  gimp_composite_regression_timer_report("grain extract", ft0, ft1);
#endif

#ifdef do_grainmerge
  ctx.op = GIMP_COMPOSITE_GRAIN_MERGE;
		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_grain_merge_any_any_any_generic, &ctx_generic);
  gimp_composite_regression_compare_contexts("grain merge", &ctx, &ctx_generic);
  gimp_composite_regression_timer_report("grain merge", ft0, ft1);
#endif

#ifdef do_scale
  ctx.op = GIMP_COMPOSITE_SCALE;
		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_scale_any_any_any_generic, &ctx_generic);
  gimp_composite_regression_compare_contexts("scale", &ctx, &ctx_generic);
  gimp_composite_regression_timer_report("scale", ft0, ft1);
#endif

#ifdef do_screen
  ctx.op = GIMP_COMPOSITE_SCREEN;
		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_screen_any_any_any_generic, &ctx_generic);
  gimp_composite_regression_compare_contexts("screen", &ctx, &ctx_generic);
  gimp_composite_regression_timer_report("screen", ft0, ft1);
#endif

#ifdef do_lighten
  ctx.op = GIMP_COMPOSITE_LIGHTEN;
		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_lighten_any_any_any_generic, &ctx_generic);
  gimp_composite_regression_compare_contexts("lighten", &ctx, &ctx_generic);
  gimp_composite_regression_timer_report("lighten", ft0, ft1);
#endif

#ifdef do_darken
  /* darken */
  ctx.op = GIMP_COMPOSITE_DARKEN;
		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_darken_any_any_any_generic, &ctx_generic);
  gimp_composite_regression_compare_contexts("darken", &ctx, &ctx_generic);
  gimp_composite_regression_timer_report("darken", ft0, ft1);
#endif

#ifdef do_difference
  ctx.op = GIMP_COMPOSITE_DIFFERENCE;
		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_difference_any_any_any_generic, &ctx_generic);
  gimp_composite_regression_compare_contexts("difference", &ctx, &ctx_generic);
  gimp_composite_regression_timer_report("difference", ft0, ft1);
#endif

#ifdef do_multiply
  ctx.op = GIMP_COMPOSITE_MULTIPLY;
		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_multiply_any_any_any_generic, &ctx_generic);
  gimp_composite_regression_compare_contexts("multiply", &ctx, &ctx_generic);
  gimp_composite_regression_timer_report("multiply", ft0, ft1);
#endif

#ifdef do_subtract
  ctx.op = GIMP_COMPOSITE_SUBTRACT;
		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_subtract_any_any_any_generic, &ctx_generic);
  gimp_composite_regression_compare_contexts("subtract", &ctx, &ctx_generic);
  gimp_composite_regression_timer_report("subtract", ft0, ft1);
#endif

#ifdef do_add
  ctx.op = GIMP_COMPOSITE_ADDITION;
		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_addition_any_any_any_generic, &ctx_generic);
  gimp_composite_regression_compare_contexts("addition", &ctx, &ctx_generic);
  gimp_composite_regression_timer_report("addition", ft0, ft1);
#endif

#ifdef do_swap
  ctx.op = GIMP_COMPOSITE_SWAP;
		ft0 = gimp_composite_regression_time_function(iterations, gimp_composite_dispatch, &ctx);
		ft1 = gimp_composite_regression_time_function(iterations, gimp_composite_swap_any_any_any_generic, &ctx_generic);
  gimp_composite_regression_compare_contexts("swap", &ctx, &ctx_generic);
  gimp_composite_regression_timer_report("swap", ft0, ft1);
#endif

  return (EXIT_SUCCESS);
}
