/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.  See the file COPYING for details.
 *
 */

#include "glace.h"

/* GLACE_GIMP implements these as macros, not functions. */
/* But this is wrapper-specific stuff, so that should be okay, right? */
/* 
 * The alternative would be just to #ifdef them out and define macros
 * in glaceG.c [jas].
 */
#ifdef GLACE_GIMP
#define GlaceWMalloc(size) g_malloc(size)
#define GlaceWRealloc(p, size) g_realloc(p, size)
#define GlaceWCalloc(nObj, size) g_malloc0((size) * (nObj))
#define GlaceWFree(ptr) g_free(ptr)
#else
void *GlaceWMalloc(size_t size);
void *GlaceWRealloc(void *p,size_t size);
void *GlaceWCalloc(size_t nObj, size_t size);
void GlaceWFree(void *p);
#endif

#if 0
#if __STDC__
#define ARGS(alist) alist
#else /*__STDC__*/
#define ARGS(alist) ()
#define const
#endif /*__STDC__*/
#endif



/*
 * For glaceCfg.c
 */

#define HLB 15
#define BIG_TMP_BITS 31
#define MID_TMP_BITS 15
#define ACC_BITS 14
#define OUT_BITS 8
#define AFT_FILTVAL ldexp(255.0, BIG_TMP_BITS-8)
#define MAFT_FILTVAL ldexp(255.0, MID_TMP_BITS-8)
#define BSHIFT (BIG_TMP_BITS-HLB)
#define MSHIFT (MID_TMP_BITS-HLB)
#define NOT_FILTVAL ldexp(255.0, HLB-8-1)
#define FORCE_NO_BIT_SHIFT (-1000)
#define MAX_BIT_SHIFT (32)

#define MAX_MESSAGE_LEN 150
