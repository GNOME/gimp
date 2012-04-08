/*
Copyright (C) 1999-2002 Adam D. Moss (the "Author").  All Rights Reserved.

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

  This code module concerns itself with conversion from a hard-coded
  RGB colour space (sRGB by default) to CIE L*a*b* and back again with
  (primarily) precision and (secondarily) speed, oriented largely
  towards the purposes of quantifying the PERCEPTUAL difference between
  two arbitrary RGB colours with a minimum of fuss.

  Motivation One: The author is disheartened at the amount of graphics
  processing software around which uses weighted or non-weighted
  Euclidean distance between co-ordinates within a (poorly-defined) RGB
  space as the basis of what should really be an estimate of perceptual
  difference to the human eye.  Certainly it's fast to do it that way,
  but please think carefully about whether your particular application
  should be tolerating sloppy results for the sake of real-time response.

  Motivation Two: Lack of tested, re-usable and free code available
  for this purpose.  The difficulty in finding something similar to
  CPercep with a free license motivated this project; I hope that this
  code also serves to illustrate how to perform the
  R'G'B'->XYZ->L*a*b*->XYZ->R'G'B' transformation correctly since I
  was distressed to note how many of the equations and code snippets
  on the net were omitting the reverse transform and/or were using
  incorrectly-derived or just plain wrong constants.

  TODO: document functions, rename erroneously-named arguments
*/

#include "config.h"

#include <babl/babl.h>

#include "cpercep.h"

void
cpercep_rgb_to_space (double  inr,
                      double  ing,
                      double  inb,
                      double *outr,
                      double *outg,
                      double *outb)
{
  float input[3] = {inr/255.0f, ing/255.0f, inb/255.0f};
  float output[3];
  babl_process (babl_fish (babl_format ("R'G'B' float"),
                           babl_format ("CIE Lab float")),
                input, output, 1);
  *outr = output[0];
  *outg = output[1];
  *outb = output[2];
}


void
cpercep_space_to_rgb (double  inr,
                      double  ing,
                      double  inb,
                      double *outr,
                      double *outg,
                      double *outb)
{
  float input[3] = {inr, ing, inb};
  float output[3];
    babl_process (babl_fish (babl_format ("CIE Lab float"),
                             babl_format ("R'G'B' float")),
                  input, output, 1);
  *outr = output[0] * 255;
  *outg = output[1] * 255;
  *outb = output[2] * 255;
}
