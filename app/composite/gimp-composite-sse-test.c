#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "base/base-types.h"

#include "gimp-composite.h"
#include "gimp-composite-regression.h"
#include "gimp-composite-util.h"
#include "gimp-composite-generic.h"
#include "gimp-composite-sse.h"

int
gimp_composite_sse_test (int iterations, int n_pixels)
{
#if defined(COMPILE_SSE_IS_OKAY)
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

  if (gimp_composite_sse_init () == 0)
    {
      g_print ("gimp_composite_sse: Instruction set is not available.\n");
      return EXIT_SUCCESS;
    }

  g_print ("\nRunning gimp_composite_sse tests...\n");

  rgba8A =  gimp_composite_regression_random_rgba8(n_pixels+1);
  rgba8B =  gimp_composite_regression_random_rgba8(n_pixels+1);
  rgba8M =  gimp_composite_regression_random_rgba8(n_pixels+1);
  rgba8D1 = (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);
  rgba8D2 = (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);
  va8A =    (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);
  va8B =    (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);
  va8M =    (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);
  va8D1 =   (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);
  va8D2 =   (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);

  for (i = 0; i < n_pixels; i++)
    {
      va8A[i].v = i;
      va8A[i].a = 255-i;
      va8B[i].v = i;
      va8B[i].a = i;
      va8M[i].v = i;
      va8M[i].a = i;
    }


  gimp_composite_context_init (&special_ctx, GIMP_COMPOSITE_ADDITION, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D2);
  gimp_composite_context_init (&generic_ctx, GIMP_COMPOSITE_ADDITION, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D1);
  ft0 = gimp_composite_regression_time_function (iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function (iterations, gimp_composite_addition_rgba8_rgba8_rgba8_sse, &special_ctx);
  if (gimp_composite_regression_compare_contexts ("addition", &generic_ctx, &special_ctx))
    {
      g_print ("addition_rgba8_rgba8_rgba8 failed\n");
      return EXIT_FAILURE;
    }
  gimp_composite_regression_timer_report ("addition_rgba8_rgba8_rgba8", ft0, ft1);

  gimp_composite_context_init (&special_ctx, GIMP_COMPOSITE_DARKEN, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D2);
  gimp_composite_context_init (&generic_ctx, GIMP_COMPOSITE_DARKEN, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D1);
  ft0 = gimp_composite_regression_time_function (iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function (iterations, gimp_composite_darken_rgba8_rgba8_rgba8_sse, &special_ctx);
  if (gimp_composite_regression_compare_contexts ("darken", &generic_ctx, &special_ctx))
    {
      g_print ("darken_rgba8_rgba8_rgba8 failed\n");
      return EXIT_FAILURE;
    }
  gimp_composite_regression_timer_report ("darken_rgba8_rgba8_rgba8", ft0, ft1);

  gimp_composite_context_init (&special_ctx, GIMP_COMPOSITE_DIFFERENCE, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D2);
  gimp_composite_context_init (&generic_ctx, GIMP_COMPOSITE_DIFFERENCE, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D1);
  ft0 = gimp_composite_regression_time_function (iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function (iterations, gimp_composite_difference_rgba8_rgba8_rgba8_sse, &special_ctx);
  if (gimp_composite_regression_compare_contexts ("difference", &generic_ctx, &special_ctx))
    {
      g_print ("difference_rgba8_rgba8_rgba8 failed\n");
      return EXIT_FAILURE;
    }
  gimp_composite_regression_timer_report ("difference_rgba8_rgba8_rgba8", ft0, ft1);

  gimp_composite_context_init (&special_ctx, GIMP_COMPOSITE_GRAIN_EXTRACT, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D2);
  gimp_composite_context_init (&generic_ctx, GIMP_COMPOSITE_GRAIN_EXTRACT, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D1);
  ft0 = gimp_composite_regression_time_function (iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function (iterations, gimp_composite_grain_extract_rgba8_rgba8_rgba8_sse, &special_ctx);
  if (gimp_composite_regression_compare_contexts ("grain_extract", &generic_ctx, &special_ctx))
    {
      g_print ("grain_extract_rgba8_rgba8_rgba8 failed\n");
      return EXIT_FAILURE;
    }
  gimp_composite_regression_timer_report ("grain_extract_rgba8_rgba8_rgba8", ft0, ft1);

  gimp_composite_context_init (&special_ctx, GIMP_COMPOSITE_GRAIN_MERGE, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D2);
  gimp_composite_context_init (&generic_ctx, GIMP_COMPOSITE_GRAIN_MERGE, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D1);
  ft0 = gimp_composite_regression_time_function (iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function (iterations, gimp_composite_grain_merge_rgba8_rgba8_rgba8_sse, &special_ctx);
  if (gimp_composite_regression_compare_contexts ("grain_merge", &generic_ctx, &special_ctx))
    {
      g_print ("grain_merge_rgba8_rgba8_rgba8 failed\n");
      return EXIT_FAILURE;
    }
  gimp_composite_regression_timer_report ("grain_merge_rgba8_rgba8_rgba8", ft0, ft1);

  gimp_composite_context_init (&special_ctx, GIMP_COMPOSITE_LIGHTEN, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D2);
  gimp_composite_context_init (&generic_ctx, GIMP_COMPOSITE_LIGHTEN, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D1);
  ft0 = gimp_composite_regression_time_function (iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function (iterations, gimp_composite_lighten_rgba8_rgba8_rgba8_sse, &special_ctx);
  if (gimp_composite_regression_compare_contexts ("lighten", &generic_ctx, &special_ctx))
    {
      g_print ("lighten_rgba8_rgba8_rgba8 failed\n");
      return EXIT_FAILURE;
    }
  gimp_composite_regression_timer_report ("lighten_rgba8_rgba8_rgba8", ft0, ft1);

  gimp_composite_context_init (&special_ctx, GIMP_COMPOSITE_MULTIPLY, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D2);
  gimp_composite_context_init (&generic_ctx, GIMP_COMPOSITE_MULTIPLY, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D1);
  ft0 = gimp_composite_regression_time_function (iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function (iterations, gimp_composite_multiply_rgba8_rgba8_rgba8_sse, &special_ctx);
  if (gimp_composite_regression_compare_contexts ("multiply", &generic_ctx, &special_ctx))
    {
      g_print ("multiply_rgba8_rgba8_rgba8 failed\n");
      return EXIT_FAILURE;
    }
  gimp_composite_regression_timer_report ("multiply_rgba8_rgba8_rgba8", ft0, ft1);

  gimp_composite_context_init (&special_ctx, GIMP_COMPOSITE_SCALE, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D2);
  gimp_composite_context_init (&generic_ctx, GIMP_COMPOSITE_SCALE, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D1);
  ft0 = gimp_composite_regression_time_function (iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function (iterations, gimp_composite_scale_rgba8_rgba8_rgba8_sse, &special_ctx);
  if (gimp_composite_regression_compare_contexts ("scale", &generic_ctx, &special_ctx))
    {
      g_print ("scale_rgba8_rgba8_rgba8 failed\n");
      return EXIT_FAILURE;
    }
  gimp_composite_regression_timer_report ("scale_rgba8_rgba8_rgba8", ft0, ft1);

  gimp_composite_context_init (&special_ctx, GIMP_COMPOSITE_SCREEN, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D2);
  gimp_composite_context_init (&generic_ctx, GIMP_COMPOSITE_SCREEN, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D1);
  ft0 = gimp_composite_regression_time_function (iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function (iterations, gimp_composite_screen_rgba8_rgba8_rgba8_sse, &special_ctx);
  if (gimp_composite_regression_compare_contexts ("screen", &generic_ctx, &special_ctx))
    {
      g_print ("screen_rgba8_rgba8_rgba8 failed\n");
      return EXIT_FAILURE;
    }
  gimp_composite_regression_timer_report ("screen_rgba8_rgba8_rgba8", ft0, ft1);

  gimp_composite_context_init (&special_ctx, GIMP_COMPOSITE_SUBTRACT, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D2);
  gimp_composite_context_init (&generic_ctx, GIMP_COMPOSITE_SUBTRACT, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D1);
  ft0 = gimp_composite_regression_time_function (iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function (iterations, gimp_composite_subtract_rgba8_rgba8_rgba8_sse, &special_ctx);
  if (gimp_composite_regression_compare_contexts ("subtract", &generic_ctx, &special_ctx))
    {
      g_print ("subtract_rgba8_rgba8_rgba8 failed\n");
      return EXIT_FAILURE;
    }
  gimp_composite_regression_timer_report ("subtract_rgba8_rgba8_rgba8", ft0, ft1);

  gimp_composite_context_init (&special_ctx, GIMP_COMPOSITE_SWAP, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D2);
  gimp_composite_context_init (&generic_ctx, GIMP_COMPOSITE_SWAP, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, GIMP_PIXELFORMAT_RGBA8, n_pixels, (unsigned char *) rgba8A, (unsigned char *) rgba8B, (unsigned char *) rgba8B, (unsigned char *) rgba8D1);
  ft0 = gimp_composite_regression_time_function (iterations, gimp_composite_dispatch, &generic_ctx);
  ft1 = gimp_composite_regression_time_function (iterations, gimp_composite_swap_rgba8_rgba8_rgba8_sse, &special_ctx);
  if (gimp_composite_regression_compare_contexts ("swap", &generic_ctx, &special_ctx))
    {
      g_print ("swap_rgba8_rgba8_rgba8 failed\n");
      return EXIT_FAILURE;
    }
  gimp_composite_regression_timer_report ("swap_rgba8_rgba8_rgba8", ft0, ft1);
#endif
  return EXIT_SUCCESS;
}

int
main (int argc, char *argv[])
{
  int iterations;
  int n_pixels;

  srand (314159);

  putenv ("GIMP_COMPOSITE=0x1");

  iterations = 10;
  n_pixels = 8388625;

  argv++, argc--;
  while (argc >= 2)
    {
      if (argc > 1 && (strcmp (argv[0], "--iterations") == 0 || strcmp (argv[0], "-i") == 0))
        {
          iterations = atoi(argv[1]);
          argc -= 2, argv++; argv++;
        }
      else if (argc > 1 && (strcmp (argv[0], "--n-pixels") == 0 || strcmp (argv[0], "-n") == 0))
        {
          n_pixels = atoi (argv[1]);
          argc -= 2, argv++; argv++;
        }
      else
        {
          g_print ("Usage: gimp-composites-*-test [-i|--iterations n] [-n|--n-pixels n]");
          return EXIT_FAILURE;
        }
    }

  gimp_composite_generic_install ();

  return (gimp_composite_sse_test (iterations, n_pixels));
}
