#ifndef __RCM_HSV__
#define __RCM_HSV__

typedef double hsv;
void rgb_to_hsv (hsv  r,
		 hsv  g,
		 hsv  b,
		 hsv *h,
		 hsv *s,
		 hsv *l);

void hsv_to_rgb (hsv  h,
		 hsv  sl,
		 hsv  l,
		 hsv *r,
		 hsv *g,
		 hsv *b);


#endif
