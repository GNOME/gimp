#ifndef __DISPLAYLUT_H__
#define __DIPSLAYLUT_H__

#include <gdk/gdktypes.h>

#define IM_FLOAT16_TO_FILM_LOOKUP_START  15326
#define IM_FLOAT16_TO_FILM_LOOKUP_END    16252   

extern unsigned char IM_Float16ToFilmLookup[];

#define IM_GAMMA22LOOKUP_START  13717
#define IM_GAMMA22LOOKUP_END    16254

extern unsigned char IM_Gamma22Lookup[];

void display_u8_init(void);
guint8 display_u8_from_float(gfloat code);
guint8 display_u8_from_float16(guint16 code);

#endif
