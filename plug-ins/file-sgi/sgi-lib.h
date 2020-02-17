/*
 * SGI image file format library definitions.
 *
 * Copyright 1997-1998 Michael Sweet (mike@easysw.com)
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __SGI_LIB_H__
#define __SGI_LIB_H__

G_BEGIN_DECLS

/*
 * Constants...
 */

#  define SGI_MAGIC     474     /* Magic number in image file */

#  define SGI_READ      0       /* Read from an SGI image file */
#  define SGI_WRITE     1       /* Write to an SGI image file */

#  define SGI_COMP_NONE 0       /* No compression */
#  define SGI_COMP_RLE  1       /* Run-length encoding */
#  define SGI_COMP_ARLE 2       /* Aggressive run-length encoding */


/*
 * Image structure...
 */

typedef struct
{
  FILE                  *file;          /* Image file */
  int                   mode,           /* File open mode */
                        bpp,            /* Bytes per pixel/channel */
                        comp,           /* Compression */
                        swapBytes;      /* SwapBytes flag */
  unsigned short        xsize,          /* Width in pixels */
                        ysize,          /* Height in pixels */
                        zsize;          /* Number of channels */
  long                  firstrow,       /* File offset for first row */
                        nextrow,        /* File offset for next row */
                        **table,        /* Offset table for compression */
                        **length;       /* Length table for compression */
  unsigned short        *arle_row;      /* Advanced RLE compression buffer */
  long                  arle_offset,    /* Advanced RLE buffer offset */
                        arle_length;    /* Advanced RLE buffer length */
} sgi_t;


/*
 * Prototypes...
 */

extern int      sgiClose     (sgi_t *sgip);
extern int      sgiGetRow    (sgi_t *sgip,
                              unsigned short *row,
                              int y,
                              int z);
extern sgi_t    *sgiOpen     (const char *filename,
                              int mode,
                              int comp,
                              int bpp,
                              int xsize,
                              int ysize,
                              int zsize);
extern sgi_t    *sgiOpenFile (FILE *file,
                              int mode,
                              int comp,
                              int bpp,
                              int xsize,
                              int ysize,
                              int zsize);
extern int      sgiPutRow    (sgi_t *sgip,
                              unsigned short *row,
                              int y,
                              int z);

G_END_DECLS

#endif /* !__SGI_LIB_H__ */
