/* #ident "@(#)g3.h	3.1 95/08/30 Copyright (c) Gert Doering" */

#ifndef NULL
#define NULL 0L
#endif

/* nr_bits is set to ( bit_length MOD FBITS ) by build_g3_tree,
 * nr_pels is the number of pixels to write for that code,
 * bit_code is the code itself (msb2lsb), and bit_length its length
 */

struct g3code { int nr_bits, nr_pels, bit_code, bit_length; };

/* tables for makeup / terminal codes white / black, extended m_codes */
extern struct g3code t_white[], m_white[], t_black[], m_black[], m_ext[];

/* The number of bits looked up simultaneously determines the amount
 * of memory used by the program - some values:
 * 10 bits : 87 Kbytes, 8 bits : 20 Kbytes
 *  5 bits :  6 Kbytes, 1 bit  :  4 Kbytes
 * - naturally, using less bits is also slower...
 */

/*
#define FBITS 5
#define BITM 0x1f
*/

#define FBITS 8
#define BITM 0xff

/*
#define FBITS 12
#define BITM 0xfff
*/

#define BITN 1<<FBITS

struct g3_tree { int nr_bits;
		 struct g3_tree *	nextb[ BITN ];
		 };

#define g3_leaf g3code

extern void tree_add_node ( struct g3_tree *p, struct g3code * g3c,
		                   int bit_code, int bit_length );
extern void build_tree ( struct g3_tree ** p, struct g3code * c );

#ifdef DEBUG
extern void print_g3_tree ( char * t, struct g3_tree * p );
#endif

extern void init_byte_tab ( int reverse, int byte_tab[] );


