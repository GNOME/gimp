#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>

#include "gimp-composite.h"
#include "gimp-composite-util.h"

#undef use_oldmmx

extern void xxx_3a(rgba8_t *, rgba8_t *, rgba8_t *, u_long);

main(int argc, char *argv[])
{
  double f;
  GimpCompositeContext ctx;
  GimpCompositeContext ctx_generic;
  GimpCompositeContext ctx_va8;
  GimpCompositeContext ctx_va8_generic;
  int iterations;
  rgba8_t *d1;
  rgba8_t *d2;
  rgba8_t *rgba8A;
  rgba8_t *rgba8B;
  va8_t *va8A;
  va8_t *va8B;
  va8_t *va8_d1;
  va8_t *va8_d2;
  struct timeval t0, t1, new_elapsed, old_elapsed;
  unsigned long i;
  unsigned long n_pixels;

  iterations = atoi(argv[1]);
  n_pixels = atol(argv[2]);

  rgba8A = (rgba8_t *) calloc(sizeof(rgba8_t), n_pixels+1);
  rgba8B = (rgba8_t *) calloc(sizeof(rgba8_t), n_pixels+1);
  va8A = (va8_t *) calloc(sizeof(va8_t), n_pixels+1);
  va8B = (va8_t *) calloc(sizeof(va8_t), n_pixels+1);
  d1 = (rgba8_t *) calloc(sizeof(rgba8_t), n_pixels+1);
  d2 = (rgba8_t *) calloc(sizeof(rgba8_t), n_pixels+1);
  va8_d1 = (va8_t *) calloc(sizeof(va8_t), n_pixels+1);
  va8_d2 = (va8_t *) calloc(sizeof(va8_t), n_pixels+1);

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


#define timer_fsecs(tv) ((double) ((tv).tv_sec) + (double) ((tv).tv_usec / 1000000.0))
#define timer_report(name,t1,t2) printf("%15s %15.10f %15.10f %15.10f\n", name, timer_fsecs(t1), timer_fsecs(t2), timer_fsecs(t1)/timer_fsecs(t2));

#ifdef do_burn
  /* burn */
  gettimeofday(&t0, NULL);
  ctx.op = GIMP_COMPOSITE_BURN;
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_burn_any_any_any_generic(&ctx_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_rgba8("burn rgba8", ctx.A, ctx.B, ctx_generic.D, ctx.D, ctx.n_pixels);
  timer_report("burn rgba8", old_elapsed, new_elapsed);

  gettimeofday(&t0, NULL);
  ctx_va8.op = GIMP_COMPOSITE_BURN;
  ctx_va8_generic.op = GIMP_COMPOSITE_BURN;
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx_va8); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_burn_any_any_any_generic(&ctx_va8_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_va8("burn rgba8", ctx_va8.A, ctx_va8.B, ctx_va8_generic.D, ctx_va8.D, ctx_va8.n_pixels);
  timer_report("burn va8", old_elapsed, new_elapsed);
#endif

#ifdef do_dodge
  /* dodge */
  gettimeofday(&t0, NULL);
  ctx.op = GIMP_COMPOSITE_DODGE;
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_dodge_any_any_any_generic(&ctx_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_rgba8("dodge", ctx.A, ctx.B, ctx_generic.D, ctx.D, ctx.n_pixels);
  timer_report("dodge", old_elapsed, new_elapsed);
#endif

#ifdef do_divide
  /* divide */
  ctx.op = GIMP_COMPOSITE_DIVIDE;
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_divide_any_any_any_generic(&ctx_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_rgba8("divide", ctx.A, ctx.B, ctx_generic.D, ctx.D, ctx.n_pixels);
  timer_report("divide",  old_elapsed, new_elapsed);
#endif

#ifdef do_grainextract
  /* grainextract */
  ctx.op = GIMP_COMPOSITE_GRAIN_EXTRACT;
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_grain_extract_any_any_any_generic(&ctx_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_rgba8("grain extract", ctx.A, ctx.B, ctx_generic.D, ctx.D, ctx.n_pixels);
  timer_report("grainextract",  old_elapsed, new_elapsed);
#endif

#ifdef do_grainmerge
  ctx.op = GIMP_COMPOSITE_GRAIN_MERGE;
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_grain_merge_any_any_any_generic(&ctx_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_rgba8("grain merge", ctx.A, ctx.B, ctx_generic.D, ctx.D, ctx.n_pixels);
  timer_report("grainmerge",  old_elapsed, new_elapsed);
#endif

#ifdef do_scale
  gettimeofday(&t0, NULL);
  ctx.op = GIMP_COMPOSITE_SCALE;
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);

  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_scale_any_any_any_generic(&ctx_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_rgba8("scale", ctx.A, NULL, ctx_generic.D, ctx.D, ctx.n_pixels);
  timer_report("scale", old_elapsed, new_elapsed);
#endif

#ifdef do_screen
  gettimeofday(&t0, NULL);
  ctx.op = GIMP_COMPOSITE_SCREEN;
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_screen_any_any_any_generic(&ctx_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_rgba8("screen", ctx.A, ctx.B, ctx_generic.D, ctx.D, ctx.n_pixels);
  timer_report("screen",  old_elapsed, new_elapsed);
#endif

#ifdef do_lighten
  gettimeofday(&t0, NULL);
  ctx.op = GIMP_COMPOSITE_LIGHTEN;
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_lighten_any_any_any_generic(&ctx_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_rgba8("lighten", ctx.A, ctx.B, ctx_generic.D, ctx.D, ctx.n_pixels);
  timer_report("lighten",  old_elapsed, new_elapsed);
#endif

#ifdef do_darken
  /* darken */
  gettimeofday(&t0, NULL);
  ctx.op = GIMP_COMPOSITE_DARKEN;
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_darken_any_any_any_generic(&ctx_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_rgba8("darken", ctx.A, ctx.B, ctx_generic.D, ctx.D, ctx.n_pixels);
  timer_report("darken",  old_elapsed, new_elapsed);
#endif

#ifdef do_difference
  gettimeofday(&t0, NULL);
  ctx.op = GIMP_COMPOSITE_DIFFERENCE;
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_difference_any_any_any_generic(&ctx_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_rgba8("difference", ctx.A, ctx.B, ctx_generic.D, ctx.D, ctx.n_pixels);
  timer_report("difference",  old_elapsed, new_elapsed);
#endif

#ifdef do_multiply
  gettimeofday(&t0, NULL);
  ctx.op = GIMP_COMPOSITE_MULTIPLY;
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_multiply_any_any_any_generic(&ctx_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_rgba8("multiply", ctx.A, ctx.B, ctx_generic.D, ctx.D, ctx.n_pixels);
  timer_report("multiply",  old_elapsed, new_elapsed);
#endif
  
#ifdef do_subtract
  gettimeofday(&t0, NULL);
  ctx.op = GIMP_COMPOSITE_SUBTRACT;
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_subtract_any_any_any_generic(&ctx_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_rgba8("subtract", ctx.A, ctx.B, ctx_generic.D, ctx.D, ctx.n_pixels);
  timer_report("subtract",  old_elapsed, new_elapsed);
#endif

#ifdef do_add
  gettimeofday(&t0, NULL);
  ctx.op = GIMP_COMPOSITE_ADDITION;
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_addition_any_any_any_generic(&ctx_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_rgba8("addition", ctx.A, ctx.B, ctx_generic.D, ctx.D, ctx.n_pixels);
  timer_report("add",  old_elapsed, new_elapsed);
#endif

#ifdef do_swap
  gettimeofday(&t0, NULL);
  ctx.op = GIMP_COMPOSITE_SWAP;
  for (i = 0; i < iterations; i++) { gimp_composite_dispatch(&ctx); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &new_elapsed);
  gettimeofday(&t0, NULL);
  for (i = 0; i < iterations; i++) { gimp_composite_swap_any_any_any_generic(&ctx_generic); }
  gettimeofday(&t1, NULL);
  timersub(&t1, &t0, &old_elapsed);
  comp_rgba8("swap", ctx.A, ctx.B, ctx_generic.A, ctx.A, ctx.n_pixels);
  comp_rgba8("swap", ctx.A, ctx.B, ctx_generic.B, ctx.B, ctx.n_pixels);
  timer_report("swap",  old_elapsed, new_elapsed);
#endif

  return (0);
}

print_rgba8(rgba8_t *p)
{
  printf("#%02x%02x%02x,%02X", p->r, p->g, p->b, p->a);
  fflush(stdout);
}

print_va8(va8_t *va8)
{
  printf("#%02x,%02X", va8->v, va8->a);
  fflush(stdout);
}

comp_rgba8(char *str, rgba8_t *rgba8A, rgba8_t *rgba8B, rgba8_t *expected, rgba8_t *got, u_long length)
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
      printf("%s %8d A=", str, i); print_rgba8(&rgba8A[i]);
      if (rgba8B != (rgba8_t *) 0) {
        printf(" B="); print_rgba8(&rgba8B[i]);
      }
      printf("   ");
      printf("exp=");
      print_rgba8(&expected[i]);
      printf(" got=");
      print_rgba8(&got[i]);
      printf("\n");
    }
    if (fail_count > 5)
      break;
  }

  return (fail_count);
}

comp_va8(char *str, va8_t *va8A, va8_t *va8B, va8_t *expected, va8_t *got, u_long length)
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
      printf("%s %8d A=", str, i); print_va8(&va8A[i]);
      if (va8B != (va8_t *) 0) { printf(" B="); print_va8(&va8B[i]); }
      printf("   ");
      printf("exp=");
      print_va8(&expected[i]);
      printf(" got=");
      print_va8(&got[i]);
      printf("\n");
    }
    if (fail_count > 5)
      break;
  }

  return (fail_count);
}


dump_rgba8(char *str, rgba8_t *rgba, u_long length)
{
  int i;

  printf("%s\n", str);

  for (i = 0; i < length; i++) {
    printf("%5d: ", i);
    print_rgba8(&rgba[i]);
    printf("\n");
  }
}

void
xxx_3a(rgba8_t *a, rgba8_t *b, rgba8_t *c, u_long length)
{
  int i;

  for (i = 0; i < length; i++) {
    printf("%5d: ", i);
    print_rgba8(&a[i]);
    printf(" ");
    print_rgba8(&b[i]);
    printf(" ");
    print_rgba8(&c[i]);
    printf("\n");
  }
}
