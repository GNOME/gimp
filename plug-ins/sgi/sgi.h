/*
 * "$Id$"
 *
 *   SGI image file format library definitions.
 *
 *   Copyright 1997 Michael Sweet (mike@easysw.com)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Revision History:
 *
 *   $Log$
 *   Revision 1.3  1998/03/26 02:08:30  yosh
 *   * applied gimp-quinet-980122-0 and tweaked the tests a bit, this makes the
 *   optional library tests in configure.
 *
 *   * applied gimp-jbuhler-980321-0, fixes more warnings in plug-ins
 *
 *   -Yosh
 *
 *   Revision 1.2  1998/03/25 02:17:34  yosh
 *   * configure.in: corrected test for libXmu for some systems; added test for
 *   difftime
 *
 *   * app/main.c: use glib ATEXIT macro
 *
 *   * app/text_tool.c: applied gimp-stric-980321-0 (text preview refresh)
 *
 *   * plug-ins/script-fu/script-fu-console.c: don't need to init gtkpreview stuff,
 *   since we don't use them
 *
 *   * plug-ins/script-fu/: many portability fixes
 *
 *   * plug-ins/pnm/pnm.c: sprintf portability patch
 *
 *   * plug-ins now #define RAND_MAX if needed
 *
 *   * plug-ins/sparkle/sparkle.c: applied gimp-joke-980322-1
 *
 *   -Yosh
 *
 *   Revision 1.1.1.1  1997/11/24 22:04:37  sopwith
 *   Let's try this import one last time.
 *
 *   Revision 1.3  1997/11/18 03:04:29  nobody
 *   fixed ugly comment-bugs introduced by evil darkwing
 *   keep out configuration empty dirs
 *   	--darkwing
 *
 *   Revision 1.2  1997/11/17 05:44:04  nobody
 *   updated ChangeLog
 *   dropped non-working doc/Makefile entries
 *   applied many fixes from the registry as well as the devel ML
 *   applied missing patches by Art Haas
 *
 *   	--darkwing
 *
 *   Revision 1.2  1997/06/18  00:55:28  mike
 *   Updated to hold length table when writing.
 *   Updated to hold current length when doing ARLE.
 *
 *   Revision 1.1  1997/06/15  03:37:19  mike
 *   Initial revision
 */

#ifndef _SGI_H_
#  define _SGI_H_

#  include <stdio.h>
#  include <stdlib.h>
#  include <unistd.h>
#  include <string.h>

#  ifdef __cplusplus
extern "C" {
#  endif


/*
 * Constants...
 */

#  define SGI_MAGIC	474	/* Magic number in image file */

#  define SGI_READ	0	/* Read from an SGI image file */
#  define SGI_WRITE	1	/* Write to an SGI image file */

#  define SGI_COMP_NONE	0	/* No compression */
#  define SGI_COMP_RLE	1	/* Run-length encoding */
#  define SGI_COMP_ARLE	2	/* Agressive run-length encoding */


/*
 * Image structure...
 */

typedef struct
{
  FILE			*file;		/* Image file */
  int			mode,		/* File open mode */
			bpp,		/* Bytes per pixel/channel */
			comp;		/* Compression */
  unsigned short	xsize,		/* Width in pixels */
			ysize,		/* Height in pixels */
			zsize;		/* Number of channels */
  long			firstrow,	/* File offset for first row */
			nextrow,	/* File offset for next row */
			**table,	/* Offset table for compression */
			**length;	/* Length table for compression */
  short			*arle_row;	/* Advanced RLE compression buffer */
  long			arle_offset,	/* Advanced RLE buffer offset */
			arle_length;	/* Advanced RLE buffer length */
} sgi_t;


/*
 * Prototypes...
 */

extern int	sgiClose(sgi_t *sgip);
extern int	sgiGetRow(sgi_t *sgip, short *row, int y, int z);
extern sgi_t	*sgiOpen(char *filename, int mode, int comp, int bpp,
		         int xsize, int ysize, int zsize);
extern int	sgiPutRow(sgi_t *sgip, short *row, int y, int z);

#  ifdef __cplusplus
}
#  endif
#endif /* !_SGI_H_ */

/*
 * End of "$Id$".
 */
