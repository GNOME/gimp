
#ifndef __GIMP_COMPOSITE_UTIL_H__
#define __GIMP_COMPOSITE_UTIL_H__

typedef struct
{
  guchar  r;
  guchar  g;
  guchar  b;
  guchar  a;
} rgba8_t;

typedef struct
{
  guchar  r;
  guchar  g;
  guchar  b;
} rgb8_t;

typedef struct
{
  guchar  v;
} v8_t;

typedef struct
{
  guchar  v;
  guchar  a;
} va8_t;

extern int gimp_composite_bpp[];

#endif  /* __GIMP_COMPOSITE_UTIL_H__ */
