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

extern void gimp_composite_regression_print_vector (guchar vector[],
                                                                                                                                                                                                                                                                                                                                                                                                                                GimpPixelFormat format,
                                                                                                                                                                                                                                                                                                                                                                                                                                gulong n_pixels);

extern void gimp_composite_regression_print_vector_v8 (gimp_v8_t v[],
                                                                                                                                                                                                                                                                                                                                                                                                                                                        unsigned int n_pixels);

extern void gimp_composite_regression_print_vector_va8 (gimp_va8_t v[],
                                                                                                                                                                                                                                                                                                                                                                                                                                                                unsigned int n_pixels);

extern void gimp_composite_regression_print_vector_rgb8 (gimp_rgb8_t v[],
                                                                                                                                                                                                                                                                                                                                                                                                                                                                        unsigned int n_pixels);

extern void gimp_composite_regression_print_vector_rgba8 (gimp_rgba8_t v[],
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                unsigned int n_pixels);


extern double gimp_composite_regression_time_function (gulong iterations,
                                                                                                                                                                                                                                                                                                                                                                                                                                                        void (*func)(),
                                                                                                                                                                                                                                                                                                                                                                                                                                                        GimpCompositeContext *ctx);
extern int gimp_composite_regression_comp_rgba8 (char *str,
                                                                                                                                                                                                                                                                                                                                                                                                        gimp_rgba8_t *rgba8A,
                                                                                                                                                                                                                                                                                                                                                                                                        gimp_rgba8_t *rgba8B,
                                                                                                                                                                                                                                                                                                                                                                                                        gimp_rgba8_t *expected,
                                                                                                                                                                                                                                                                                                                                                                                                        gimp_rgba8_t *actual,
                                                                                                                                                                                                                                                                                                                                                                                                        gulong length);
extern int gimp_composite_regression_comp_va8 (char *str,
                                                                                                                                                                                                                                                                                                                                                                                        gimp_va8_t *va8A,
                                                                                                                                                                                                                                                                                                                                                                                        gimp_va8_t *va8B,
                                                                                                                                                                                                                                                                                                                                                                                        gimp_va8_t *expected,
                                                                                                                                                                                                                                                                                                                                                                                        gimp_va8_t *actual,
                                                                                                                                                                                                                                                                                                                                                                                        gulong length);
extern int gimp_composite_regression_compare_contexts (char *,
                                                                                                                                                                                                                                                                                                                                                                                                                                                        GimpCompositeContext *ctx1,
                                                                                                                                                                                                                                                                                                                                                                                                                                                        GimpCompositeContext *ctx2);
extern void gimp_composite_regression_dump_rgba8 (char *str,
                                                                                                                                                                                                                                                                                                                                                                                                                gimp_rgba8_t *rgba,
                                                                                                                                                                                                                                                                                                                                                                                                                gulong n_pixels);
extern void gimp_composite_regression_print_rgba8 (gimp_rgba8_t *rgba8);
extern void gimp_composite_regression_print_va8 (gimp_va8_t *va8);
extern void gimp_composite_regression_timer_report (char *name,
                                                                                                                                                                                                                                                                                                                                                                                                                                double t1,
                                                                                                                                                                                                                                                                                                                                                                                                                                double t2);

extern gimp_rgba8_t *gimp_composite_regression_random_rgba8 (gulong n_pixels);
extern gimp_rgba8_t *gimp_composite_regression_fixed_rgba8 (gulong n_pixels);
extern GimpCompositeContext *gimp_composite_context_init (GimpCompositeContext *ctx,
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                GimpCompositeOperation op,
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                GimpPixelFormat a_format,
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                GimpPixelFormat b_format,
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                GimpPixelFormat d_format,
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                GimpPixelFormat m_format,
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                unsigned long n_pixels,
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                unsigned char *A,
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                unsigned char *B,
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                unsigned char *M,
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                unsigned char *D);
#endif
