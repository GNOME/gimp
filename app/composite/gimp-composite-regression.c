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

void
gimp_composite_regression_print_rgba8(gimp_rgba8_t *p)
{
  printf("#%02x%02x%02x,%02X", p->r, p->g, p->b, p->a);
  fflush(stdout);
}

void
gimp_composite_regression_print_va8(gimp_va8_t *va8)
{
  printf("#%02x,%02X", va8->v, va8->a);
  fflush(stdout);
}

int
gimp_composite_regression_compare_contexts(char *operation, GimpCompositeContext *ctx1, GimpCompositeContext *ctx2)
{
		switch (ctx1->pixelformat_D) {
		case GIMP_PIXELFORMAT_ANY:
		case GIMP_PIXELFORMAT_N:
		default:
				printf("Bad pixelformat! %d\n", ctx1->pixelformat_A);
				exit(1);
				break;

		case GIMP_PIXELFORMAT_V8:
				if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
						printf("%s: failed to agree\n", operation);
						return (1);
				}
				break;

		case GIMP_PIXELFORMAT_VA8:
				if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
						printf("%s: failed to agree\n", operation);
						return (1);
				}
				break;

		case GIMP_PIXELFORMAT_RGB8:
				if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
						printf("%s: failed to agree\n", operation);
						return (1);
				}
				break;

		case GIMP_PIXELFORMAT_RGBA8:
				gimp_composite_regression_comp_rgba8(operation, (gimp_rgba8_t *) ctx1->A, (gimp_rgba8_t *) ctx1->B, (gimp_rgba8_t *) ctx1->D, (gimp_rgba8_t *) ctx2->D, ctx1->n_pixels);
				break;

#if GIMP_COMPOSITE_16BIT
		case GIMP_PIXELFORMAT_V16:
				if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
						printf("%s: failed to agree\n", operation);
						return (1);
				}
				break;

		case GIMP_PIXELFORMAT_VA16:
				if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
						printf("%s: failed to agree\n", operation);
						return (1);
				}
				break;

		case GIMP_PIXELFORMAT_RGB16:
				if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
						printf("%s: failed to agree\n", operation);
						return (1);
				}
				break;

		case GIMP_PIXELFORMAT_RGBA16:
				if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
						printf("%s: failed to agree\n", operation);
						return (1);
				}
				break;

#endif
#if GIMP_COMPOSITE_32BIT
		case GIMP_PIXELFORMAT_V32:
				if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
						printf("%s: failed to agree\n", operation);
						return (1);
				}
				break;

		case GIMP_PIXELFORMAT_VA32:
				if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
						printf("%s: failed to agree\n", operation);
						return (1);
				}
				break;

		case GIMP_PIXELFORMAT_RGB32:
				if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
						printf("%s: failed to agree\n", operation);
						return (1);
				}
				break;

		case GIMP_PIXELFORMAT_RGBA32:
				if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
						printf("%s: failed to agree\n", operation);
						return (1);
				}
				break;

#endif
		}


		return (0);
}


int
gimp_composite_regression_comp_rgba8(char *str, gimp_rgba8_t *rgba8A, gimp_rgba8_t *rgba8B, gimp_rgba8_t *expected, gimp_rgba8_t *got, u_long length)
{
  int i;
  int failed;
  int fail_count;

  fail_count = 0;

  for (i = 0; i < length; i++) {
    failed = 0;

    if (expected[i].r != got[i].r) { failed = 1; }
    if (expected[i].g != got[i].g) { failed = 1; }
    if (expected[i].b != got[i].b) { failed = 1; }
    if (expected[i].a != got[i].a) { failed = 1; }
    if (failed) {
      fail_count++;
      printf("%s %8d A=", str, i); gimp_composite_regression_print_rgba8(&rgba8A[i]);
      if (rgba8B != (gimp_rgba8_t *) 0) {
        printf(" B="); gimp_composite_regression_print_rgba8(&rgba8B[i]);
      }
      printf("   ");
      printf("exp=");
      gimp_composite_regression_print_rgba8(&expected[i]);
      printf(" got=");
      gimp_composite_regression_print_rgba8(&got[i]);
      printf("\n");
    }
    if (fail_count > 5)
      break;
  }

  return (fail_count);
}

int
gimp_composite_regression_comp_va8(char *str, gimp_va8_t *va8A, gimp_va8_t *va8B, gimp_va8_t *expected, gimp_va8_t *got, u_long length)
{
  int i;
  int failed;
  int fail_count;

  fail_count = 0;

  for (i = 0; i < length; i++) {
    failed = 0;

    if (expected[i].v != got[i].v) { failed = 1; }
    if (expected[i].a != got[i].a) { failed = 1; }
    if (failed) {
      fail_count++;
      printf("%s %8d A=", str, i); gimp_composite_regression_print_va8(&va8A[i]);
      if (va8B != (gimp_va8_t *) 0) { printf(" B="); gimp_composite_regression_print_va8(&va8B[i]); }
      printf("   ");
      printf("exp=");
      gimp_composite_regression_print_va8(&expected[i]);
      printf(" got=");
      gimp_composite_regression_print_va8(&got[i]);
      printf("\n");
    }
    if (fail_count > 5)
      break;
  }

  return (fail_count);
}

void
gimp_composite_regression_dump_rgba8(char *str, gimp_rgba8_t *rgba, u_long length)
{
  int i;

  printf("%s\n", str);

  for (i = 0; i < length; i++) {
    printf("%5d: ", i);
    gimp_composite_regression_print_rgba8(&rgba[i]);
    printf("\n");
  }
}

#define tv_to_secs(tv) ((double) ((tv).tv_sec) + (double) ((tv).tv_usec / 1000000.0))

#define timer_report(name,t1,t2) printf("%-17s %17.7f %17.7f %17.7f%c\n", name, tv_to_secs(t1), tv_to_secs(t2), tv_to_secs(t1)/tv_to_secs(t2), tv_to_secs(t1)/tv_to_secs(t2) > 1.0 ? ' ' : '*');

void
gimp_composite_regression_timer_report(char *name, double t1, double t2)
{
		printf("%-17s %17.7f %17.7f %17.7f%c\n", name, t1, t2, t1/t2, t1/t2 > 1.0 ? ' ' : '*');
}

double
gimp_composite_regression_time_function(int iterations, void (*func)(), GimpCompositeContext *ctx)
{
		struct timeval t0;
		struct timeval t1;
		struct timeval tv_elapsed;
		int i;

  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { (*func)(ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &tv_elapsed);

		return (tv_to_secs(tv_elapsed));
}
