/***************************************************
 * file: gtclenums.h
 *
 * Copyright (c) 1996 Eric L. Hernes (erich@rrnet.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */

/*
 * enums that are needed both by the
 * interface file and at compile time
 */

/* bucket fill stuff */
enum {
  FG_BUCKET_FILL,
  BG_BUCKET_FILL,
  PATTERN_BUCKET_FILL
};

/* blend stuff */
enum {
  LINEAR,
  BILINEAR,
  RADIAL,
  SQUARE,
  CONICAL_SYMMETRIC,
  CONICAL_ASYMMETRIC,
  SHAPEBURST_ANGULAR,
  SHAPEBURST_SPHERICAL,
  SHAPEBURST_DIMPLED
};
enum {
  FG_BG_RGB,
  FG_BG_HSV,
  FG_TRANS,
  CUSTOM
};

/* layer modes */
enum {
  NORMAL       = 0,
  DISSOLVE     = 1,
  BEHIND       = 2,
  MULTIPLY     = 3,
  SCREEN       = 4,
  OVERLAY      = 5,
  DIFFERENCE   = 6,
  ADDITION     = 7,
  SUBTRACT     = 8,
  DARKEN_ONLY  = 9,
  LIGHTEN_ONLY = 10,
  HUE          = 11,
  SATURATION   = 12,
  COLOR        = 13,
  VALUE        = 14
};

/* gimp image get components */
enum {
  RED_CHANNEL,
  GREEN_CHANNEL,
  BLUE_CHANNEL,
  GRAY_CHANNEL,
  INDEXED_CHANNEL
};

/* layer masks */
enum {
  WHITE_MASK,
  BLACK_MASK,
  ALPHA_MASK
};

/* remove layer mask */
enum {
  APPLY,
  DISCARD
};

/* types of layer merges */
enum {
  EXPAND_AS_NECESSARY,
  CLIP_TO_IMAGE,
  CLIP_TO_BOTTOM_LAYER
};

/* rect_select args */
enum {
  ADD,
  SUB,
  REPLACE
};

/* cloning */
enum {
  IMAGE_CLONE,
  PATTERN_CLONE
};

/* convolving type */
enum {
  BLUR,
  SHARPEN
};
