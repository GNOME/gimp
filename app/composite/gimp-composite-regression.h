#ifndef gimp_composite_regression_h
#define gimp_composite_regression_h
/*
 * The following typedefs are temporary and only used in regression testing.
	*/
typedef struct
{
  guint8  v;
} gimp_v8_t;

typedef struct
{
  guint8  v;
  guint8  a;
} gimp_va8_t;

typedef struct
{
  guint8  r;
  guint8  g;
  guint8  b;
} gimp_rgb8_t;

typedef struct
{
  guint8  r;
  guint8  g;
  guint8  b;
  guint8  a;
} gimp_rgba8_t;

#ifdef GIMP_COMPOSIE_16BIT
typedef struct
{
  guint16  v;
} gimp_v16_t;

typedef struct
{
  guint16  v;
  guint16  a;
} gimp_va16_t;

typedef struct
{
  guint16  r;
  guint16  g;
  guint16  b;
} gimp_rgb16_t;

typedef struct
{
  guint16  r;
  guint16  g;
  guint16  b;
  guint16  a;
} gimp_rgba16_t;
#endif

#ifdef GIMP_COMPOSIE_32BIT
typedef struct
{
  guint32  v;
} gimp_v32_t;

typedef struct
{
  guint32  v;
  guint32  a;
} gimp_va32_t;

typedef struct
{
  guint32  r;
  guint32  g;
  guint32  b;
} gimp_rgb32_t;

typedef struct
{
  guint32  r;
  guint32  g;
  guint32  b;
  guint32  a;
} gimp_rgba32_t;
#endif

extern double gimp_composite_regression_time_function (u_long, void (*)(), GimpCompositeContext *);
extern int gimp_composite_regression_comp_rgba8 (char *, gimp_rgba8_t *, gimp_rgba8_t *, gimp_rgba8_t *, gimp_rgba8_t *, u_long);
extern int gimp_composite_regression_comp_va8 (char *, gimp_va8_t *, gimp_va8_t *, gimp_va8_t *, gimp_va8_t *, u_long);
extern int gimp_composite_regression_compare_contexts (char *, GimpCompositeContext *, GimpCompositeContext *);
extern void gimp_composite_regression_dump_rgba8 (char *, gimp_rgba8_t *, u_long);
extern void gimp_composite_regression_print_rgba8 (gimp_rgba8_t *);
extern void gimp_composite_regression_print_va8 (gimp_va8_t *);
extern void gimp_composite_regression_timer_report (char *, double, double);

extern gimp_rgba8_t *gimp_composite_regression_random_rgba8 (u_long);
extern gimp_rgba8_t *gimp_composite_regression_fixed_rgba8 (u_long);
extern GimpCompositeContext *gimp_composite_context_init (GimpCompositeContext *,
																																																										GimpCompositeOperation,
																																																										GimpPixelFormat,
																																																										GimpPixelFormat,
																																																										GimpPixelFormat,
																																																										GimpPixelFormat,
																																																										unsigned long,
																																																										unsigned char *,
																																																										unsigned char *,
																																																										unsigned char *,
																																																										unsigned char *);
#endif
