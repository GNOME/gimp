/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#if __GNUC__ >= 3

/*
 * Convert the low 8bit byte of the src to 16bit words in dst.
 */
#define mmx_low_bytes_to_words(src,dst,zero) \
         "\tmovq      %%"#src", %%"#dst"\n" \
         "\tpunpcklbw %%"#zero", %%"#dst"\n"

/*
 * Convert the high 8bit byte of the src to 16bit words in dst.
 */
#define mmx_high_bytes_to_words(src,dst,zero) \
         "\tmovq      %%"#src", %%"#dst"\n" \
         "\tpunpckhbw %%"#zero", %%"#dst"\n"

#define xmm_low_bytes_to_words(src,dst,zero) \
         "\tmovdqu     %%"#src", %%"#dst"; " \
         "\tpunpcklbw %%"#zero", %%"#dst"\n"

#define xmm_high_bytes_to_words(src,dst,zero) \
         "\tmovdqu     %%"#src", %%"#dst"; " \
         "\tpunpckhbw %%"#zero", %%"#dst"\n"

/* a = INT_MULT(a,b) */
#define mmx_int_mult(a,b,w128) \
                  "\tpmullw    %%"#b",    %%"#a"; " \
                  "\tpaddw     %%"#w128", %%"#a"; " \
                  "\tmovq      %%"#a",    %%"#b"; " \
                  "\tpsrlw     $8,        %%"#b"; " \
                  "\tpaddw     %%"#a",    %%"#b"; " \
                  "\tpsrlw     $8,        %%"#b"\n"

#define sse2_int_mult(a,b,w128) \
                  "\tpmullw    %%"#b",    %%"#a"; " \
                  "\tpaddw     %%"#w128", %%"#a"; " \
                  "\tmovdqu    %%"#a",    %%"#b"; " \
                  "\tpsrlw     $8,        %%"#b"; " \
                  "\tpaddw     %%"#a",    %%"#b"; " \
                  "\tpsrlw     $8,        %%"#b"\n"

/*
 * Double-word divide.  Adjusted for subsequent unsigned packing
 * (high-order bit of each word is cleared)
 * Clobbers eax, ecx edx
 */
#define pdivwX(dividend,divisor,quotient) "movd %%" #dividend ",%%eax\n" \
                                          "movd %%" #divisor  ",%%ecx\n" \
                                          "xorl %%edx,%%edx\n"           \
                                          "divw %%cx\n"                  \
                                          "roll $16, %%eax\n"            \
                                          "roll $16, %%ecx\n"            \
                                          "xorl %%edx,%%edx\n"           \
                                          "divw %%cx\n"                  \
                                          "btr $15, %%eax\n"             \
                                          "roll $16, %%eax\n"            \
                                          "btr $15, %%eax\n"             \
                                          "movd %%eax,%%" #quotient "\n"



/*
 * Quadword divide.  No adjustment for subsequent unsigned packing
 * (high-order bit of each word is left alone)
 * clobber list must include: "%eax", "%ecx", "%edx", divisor quotient
 */
#define pdivwqX(dividend,divisor,quotient) "movd   %%" #dividend ",%%eax; " \
                                          "movd   %%" #divisor  ",%%ecx; " \
                                          "xorl   %%edx,%%edx; "           \
                                          "divw   %%cx; "                  \
                                          "roll   $16, %%eax; "            \
                                          "roll   $16, %%ecx; "            \
                                          "xorl   %%edx,%%edx; "           \
                                          "divw   %%cx; "                  \
                                          "roll   $16, %%eax; "            \
                                          "movd   %%eax,%%" #quotient "; " \
                                          "psrlq $32,%%" #dividend ";"     \
                                          "psrlq $32,%%" #divisor ";"      \
                                          "movd   %%" #dividend ",%%eax; " \
                                          "movd   %%" #divisor  ",%%ecx; " \
                                          "xorl   %%edx,%%edx; "           \
                                          "divw   %%cx; "                  \
                                          "roll   $16, %%eax; "            \
                                          "roll   $16, %%ecx; "            \
                                          "xorl   %%edx,%%edx; "           \
                                          "divw   %%cx; "                  \
                                          "roll   $16, %%eax; "            \
                                          "movd   %%eax,%%" #divisor ";"   \
                                          "psllq  $32,%%" #divisor ";"     \
                                          "por    %%" #divisor ",%%" #quotient ";"
#define pdivwqX_clobber "%eax", "%ecx", "%edx", "%cc"

/*
 * Quadword divide.  Adjusted for subsequent unsigned packing
 * (high-order bit of each word is cleared)
 */
#define pdivwuqX(dividend,divisor,quotient) \
                                          pdivwX(dividend,divisor,quotient) \
                                            "psrlq  $32,%%" #dividend ";"   \
                                            "psrlq  $32,%%" #divisor ";"    \
                                          pdivwX(dividend,divisor,quotient) \
                                          "movd   %%eax,%%" #divisor ";"    \
                                            "psllq  $32,%%" #divisor ";"    \
                                            "por    %%" #divisor ",%%" #quotient ";"
#define pdivwuqX_clobber pdivwqX_clobber

#define xmm_pdivwqX(dividend,divisor,quotient,scratch) "movd   %%" #dividend ",%%eax; " \
                                                       "movd   %%" #divisor  ",%%ecx; " \
                                                       "xorl   %%edx,%%edx; "           \
                                                       "divw   %%cx; "                  \
                                                       "roll   $16, %%eax; "            \
                                                       "roll   $16, %%ecx; "            \
                                                       "xorl   %%edx,%%edx; "           \
                                                       "divw   %%cx; "                  \
                                                       "roll   $16, %%eax; "            \
                                                       "movd   %%eax,%%" #quotient "; " \
                                                       "psrlq $32,%%" #divisor ";"      \
                                                       "psrlq $32,%%" #dividend ";"     \
                                                       "movd   %%" #dividend ",%%eax; " \
                                                       "movd   %%" #divisor  ",%%ecx; " \
                                                       "xorl   %%edx,%%edx; "           \
                                                       "divw   %%cx; "                  \
                                                       "roll   $16, %%eax; "            \
                                                       "roll   $16, %%ecx; "            \
                                                       "xorl   %%edx,%%edx; "           \
                                                       "divw   %%cx; "                  \
                                                       "roll   $16, %%eax; "            \
                                                       "movd   %%eax,%%" #scratch ";"   \
                                                       "psllq  $32,%%" #scratch ";"     \
                                                       "psrlq $32,%%" #divisor ";"      \
                                                       "psrlq $32,%%" #dividend ";"     \
                                                       "movd   %%" #dividend ",%%eax; " \
                                                       "movd   %%" #divisor  ",%%ecx; " \
                                                       "xorl   %%edx,%%edx; "           \
                                                       "divw   %%cx; "                  \
                                                       "roll   $16, %%eax; "            \
                                                       "roll   $16, %%ecx; "            \
                                                       "xorl   %%edx,%%edx; "           \
                                                       "divw   %%cx; "                  \
                                                       "roll   $16, %%eax; "            \
                                                       "movd   %%eax,%%" #scratch ";"   \
                                                       "psllq  $64,%%" #scratch ";"     \
                                                       "psrlq $32,%%" #divisor ";"      \
                                                       "psrlq $32,%%" #dividend ";"     \
                                                       "movd   %%" #dividend ",%%eax; " \
                                                       "movd   %%" #divisor  ",%%ecx; " \
                                                       "xorl   %%edx,%%edx; "           \
                                                       "divw   %%cx; "                  \
                                                       "roll   $16, %%eax; "            \
                                                       "roll   $16, %%ecx; "            \
                                                       "xorl   %%edx,%%edx; "           \
                                                       "divw   %%cx; "                  \
                                                       "roll   $16, %%eax; "            \
                                                       "movd   %%eax,%%" #scratch ";"   \
                                                       "psllq  $96,%%" #scratch ";"     \
                                                       "por    %%" #scratch ",%%" #quotient ";"

#define xmm_pdivwX(dividend,divisor,quotient) "movd %%" #dividend ",%%eax; " \
                                              "movd %%" #divisor  ",%%ecx; " \
                                              "xorl %%edx,%%edx; "           \
                                              "divw %%cx; "                  \
                                              "roll $16, %%eax; "            \
                                              "roll $16, %%ecx; "            \
                                              "xorl %%edx,%%edx; "           \
                                              "divw %%cx; "                  \
                                              "btr $15, %%eax; "             \
                                              "roll $16, %%eax; "            \
                                              "btr $15, %%eax; "             \
                                              "movd %%eax,%%" #quotient ";"

#define xmm_pdivwuqX(dividend,divisor,quotient,scratch) \
                                          xmm_pdivwX(dividend,divisor,scratch)      \
                                            "movd   %%"#scratch ",%%"#quotient  ";" \
                                          "psrlq  $32,%%"#dividend              ";" \
                                          "psrlq  $32,%%"#divisor               ";" \
                                          xmm_pdivwX(dividend,divisor,scratch)      \
                                            "psllq  $32,%%"#scratch             ";" \
                                            "por    %%"#scratch ",%%"#quotient  ";" \
                                          "psrlq  $32,%%"#dividend              ";" \
                                          "psrlq  $32,%%"#divisor               ";" \
                                          xmm_pdivwX(dividend,divisor,scratch)      \
                                            "psllq  $64,%%"#scratch             ";" \
                                            "por    %%"#scratch ",%%"#quotient  ";" \
                                          "psrlq  $32,%%"#dividend              ";" \
                                          "psrlq  $32,%%"#divisor               ";" \
                                          xmm_pdivwX(dividend,divisor,scratch)      \
                                                                                                                                                                                                                                                                                                                                                  "psllq  $96,%%"#scratch             ";" \
                                            "por    %%"#scratch ",%%"#quotient

/* equivalent to the INT_MULT() macro in gimp-composite-generic.c */
/*
 * opr2 = INT_MULT(opr1, opr2, t)
 *
 * Operates across quad-words using x86 word (16bit) value.
 * Result is left in opr2
 *
 * opr1 = opr1 * opr2 + w128
 * opr2 = opr1
 * opr2 = ((opr2 >> 8) + opr1) >> 8
 */
#define pmulwX(opr1,opr2,w128) \
                  "\tpmullw    %%"#opr2", %%"#opr1"; " \
                  "\tpaddw     %%"#w128", %%"#opr1"; " \
                  "\tmovq      %%"#opr1", %%"#opr2"; " \
                  "\tpsrlw     $8,        %%"#opr2"; " \
                  "\tpaddw     %%"#opr1", %%"#opr2"; " \
                  "\tpsrlw     $8,        %%"#opr2"\n"

#define xmm_pmulwX(opr1,opr2,w128) \
                  "\tpmullw    %%"#opr2", %%"#opr1"; " \
                  "\tpaddw     %%"#w128", %%"#opr1"; " \
                  "\tmovdqu    %%"#opr1", %%"#opr2"; " \
                  "\tpsrlw     $8,        %%"#opr2"; " \
                  "\tpaddw     %%"#opr1", %%"#opr2"; " \
                  "\tpsrlw     $8,        %%"#opr2"\n"

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;
typedef unsigned long long uint64;
typedef struct { uint64 __uint64[2]; } uint128;

extern const guint32 va8_alpha_mask[2];
extern const guint32 va8_b255[2];
extern const guint32 va8_w1[2];
extern const guint32 va8_w255[2];
#endif /* __GNUC__ >= 3 */
