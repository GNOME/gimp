/* #ident "@(#)run_tbl.c	3.1 95/08/30 Copyright (c) 1994 Gert Doering" */

/* run_tbl.c
 *
 * this file is part of the mgetty+sendfax distribution
 *
 * compute a set of arrays that will speed up finding the run length
 * of black or white bits in a byte enourmously
 * (I do not include the array as it is larger than the source and
 * computation is fast enough - 0.005 secs on my machine...)
 */

static unsigned char tab[9] = { 0x00,
		 	0x01, 0x03, 0x07, 0x0f,
		 	0x1f, 0x3f, 0x7f, 0xff };

char w_rtab[8][256];
char b_rtab[8][256];

void make_run_tables ( void )
{
int i, j, k, m;		/* byte = kkkjjjmm, "j" starts at bit "i" */
int mask;		/* black mask (all ones), start bit i, j bits wide */

    for ( i=0; i<8; i++ )		/* for (all starting bits) */
    {
	for ( j=i+1; j>=0; j-- )	/* for (all possible run lengths) */
	{
	    mask = tab[j] << ((i-j)+1);

	    if ( i == 7 )		/* no fill bits to the left */
	    {
		if ( j == i+1 )		/* no fill bits to the right */
		{
		    w_rtab[i][0   ] = j;
		    b_rtab[i][mask] = j;
		}
		else			/* fill bits to the right */
		for ( m = ( 1 << (i-j) ) -1 ; m >= 0; m-- )
		{
		    w_rtab[i][0   + (1<<(i-j)) + m ] = j;
		    b_rtab[i][mask+ 0          + m ] = j;
		}
	    }
	    else			/* i != 7 -> fill higher bits */
	    for ( k = (0x80>>i) -1; k>= 0; k-- )
	    {
		if ( j == i+1 )		/* no noise to the right */
		{
		    w_rtab[i][ (k << (i+1)) + 0    ] = j;
		    b_rtab[i][ (k << (i+1)) + mask ] = j;
		}
		else			/* fill bits to left + right */
		for ( m = ( 1 << (i-j) ) -1; m >= 0; m-- )
		{
		    w_rtab[i][ (k << (i+1)) + 0    + (1<<(i-j)) + m ] = j;
		    b_rtab[i][ (k << (i+1)) + mask +     0      + m ] = j;
		}
	    }		/* end for (k) [noise left of top bit] */
	} 		/* end for (j) [span] */
    }			/* end for (i) [top bit] */
}
