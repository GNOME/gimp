#ifndef gimp_composite_util
#define gimp_composite_util
/*
 *
 */

typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
} rgba8_t;

typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} rgb8_t;

typedef struct {
  unsigned char v;
} v8_t;

typedef struct {
  unsigned char v;
  unsigned char a;
} va8_t;

extern int gimp_composite_bpp[];
#endif
