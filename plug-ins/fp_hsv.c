#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "libgimp/gimp.h"

typedef double hsv;
void rgb_to_hsv (hsv r,
		 hsv g,
		 hsv b,
		 hsv *h,
		 hsv *s,
		 hsv *l)
{
  hsv v;
  hsv m;
  hsv vm;
  hsv r2, g2, b2;

  v = MAX(r,g);
  v = MAX(v,b);
  m = MIN(r,g);
  m = MIN(m,b);

  if ((*l = (m + v) / 2.0) <= 0.0)
    {
      *s=*h=0;
      return;
    }
  if ((*s = vm = v - m) > 0.0)
    {
      *s /= (*l <= 0.5) ? (v + m ) :
	(2.0 - v - m) ;
    }
  else
    {
      *h=0;
      return;
    }

  r2 = (v - r) / vm;
  g2 = (v - g) / vm;
  b2 = (v - b) / vm;

  if (r == v)
    *h = (g == m ? 5.0 + b2 : 1.0 - g2);
  else if (g == v)
    *h = (b == m ? 1.0 + r2 : 3.0 - b2);
  else
    *h = (r == m ? 3.0 + g2 : 5.0 - r2);

  *h /= 6;
}

void hsv_to_rgb (hsv  h,
		 hsv  sl,
		 hsv  l,
		 hsv *r,
		 hsv *g,
		 hsv *b)
{
  hsv v;

  v = (l <= 0.5) ? (l * (1.0 + sl)) : (l + sl - l * sl);
  if (v <= 0)
    {
      *r = *g = *b = 0.0;
    }
  else
    {
      hsv m;
      hsv sv;
      gint sextant;
      hsv fract, vsf, mid1, mid2;

      m = l + l - v;
      sv = (v - m ) / v;
      h *= 6.0;
      sextant = h;
      fract = h - sextant;
      vsf = v * sv * fract;
      mid1 = m + vsf;
      mid2 = v - vsf;
      switch (sextant)
	{
	case 0: *r = v; *g = mid1; *b = m; break;
	case 1: *r = mid2; *g = v; *b = m; break;
	case 2: *r = m; *g = v; *b = mid1; break;
	case 3: *r = m; *g = mid2; *b = v; break;
	case 4: *r = mid1; *g = m; *b = v; break;
	case 5: *r = v; *g = m; *b = mid2; break;
	}
    }
}
