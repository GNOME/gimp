#if __GNUC__ >= 3

#define mmx_low_bytes_to_words(src,dst,zero) \
         "\tmovq      %%"#src", %%"#dst"; " \
         "\tpunpcklbw %%"#zero", %%"#dst"\n"

#define mmx_high_bytes_to_words(src,dst,zero) \
         "\tmovq      %%"#src", %%"#dst"; " \
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
#define pdivwX(dividend,divisor,quotient) "movd %%" #dividend ",%%eax; " \
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



/*
 * Quadword divide.  No adjustment for subsequent unsigned packing
 * (high-order bit of each word is left alone)
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
 
#endif
