/*
Copyright (C) 1997-2002 Adam D. Moss (the "Author").  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is fur-
nished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CON-
NECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the Author of the
Software shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from the Author.
*/

/*
  cpercep.c: The CPercep Functions v0.9: 2002-02-10
  Adam D. Moss: adam@gimp.org <http://www.foxbox.org/adam/code/cpercep/>

  TODO: document functions, rename erroneously-named arguments
*/

#ifndef __CPERCEP_H__
#define __CPERCEP_H__


void  cpercep_init         (void);

void  cpercep_rgb_to_space (double  inr,
                            double  ing,
                            double  inb,
                            double *outr,
                            double *outg,
                            double *outb);

void  cpercep_space_to_rgb (double  inr,
                            double  ing,
                            double  inb,
                            double *outr,
                            double *outg,
                            double *outb);


#if 0
/* This is in the header so that it can potentially be inlined. */
static const double
cpercep_distance_space (const double L1, const double a1, const double b1,
                        const double L2, const double a2, const double b2)
{
  const double Ld = L1 - L2;
  const double ad = a1 - a2;
  const double bd = b1 - b2;

  return (Ld*Ld + ad*ad + bd*bd);
}
#endif


#endif /* __CPERCEP_H__ */
