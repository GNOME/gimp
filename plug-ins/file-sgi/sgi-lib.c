/*
 * SGI image file format library routines.
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
 *
 * Contents:
 *
 *   sgiClose()    - Close an SGI image file.
 *   sgiGetRow()   - Get a row of image data from a file.
 *   sgiOpen()     - Open an SGI image file for reading or writing.
 *   sgiOpenFile() - Open an SGI image file for reading or writing.
 *   sgiPutRow()   - Put a row of image data to a file.
 *   getlong()     - Get a 32-bit big-endian integer.
 *   getshort()    - Get a 16-bit big-endian integer.
 *   putlong()     - Put a 32-bit big-endian integer.
 *   putshort()    - Put a 16-bit big-endian integer.
 *   read_rle8()   - Read 8-bit RLE data.
 *   read_rle16()  - Read 16-bit RLE data.
 *   write_rle8()  - Write 8-bit RLE data.
 *   write_rle16() - Write 16-bit RLE data.
 *
 * Revision History:
 *
 *   $Log$
 *   Revision 1.9  2005/03/04 13:23:31  neo
 *   2005-03-04  Sven Neumann  <sven@gimp.org>
 *
 *      * plug-ins/FractalExplorer
 *      * plug-ins/Lighting
 *      * plug-ins/bmp
 *      * plug-ins/dbbrowser
 *      * plug-ins/faxg3
 *      * plug-ins/fits
 *      * plug-ins/flame
 *      * plug-ins/gfig
 *      * plug-ins/gflare
 *      * plug-ins/gfli
 *      * plug-ins/gimpressionist
 *      * plug-ins/ifscompose
 *      * plug-ins/jpeg
 *      * plug-ins/maze
 *      * plug-ins/pagecurl
 *      * plug-ins/print
 *      * plug-ins/rcm
 *      * plug-ins/script-fu
 *      * plug-ins/sel2path
 *      * plug-ins/sgi
 *      * plug-ins/twain
 *      * plug-ins/winicon
 *      * plug-ins/xjt: ported to gstdio, removed unnecessary includes,
 *      minor fixes to filename handling here and there.
 *
 *   Revision 1.8  2003/04/07 11:59:33  neo
 *   2003-04-07  Sven Neumann  <sven@gimp.org>
 *
 *      * plug-ins/sgi/sgi.h
 *      * plug-ins/sgi/sgilib.c: applied a patch from marek@aki.cz that
 *      adds support for reading SGI files in little-endian format. Fixes
 *      bug #106610.
 *
 *   Revision 1.7  1998/06/06 23:22:21  yosh
 *   * adding Lighting plugin
 *
 *   * updated despeckle, png, sgi, and sharpen
 *
 *   -Yosh
 *
 *   Revision 1.5  1998/04/23  17:40:49  mike
 *   Updated to support 16-bit <unsigned> image data.
 *
 *   Revision 1.4  1998/02/05  17:10:58  mike
 *   Added sgiOpenFile() function for opening an existing file pointer.
 *
 *   Revision 1.3  1997/07/02  16:40:16  mike
 *   sgiOpen() wasn't opening files with "rb" or "wb+".  This caused problems
 *   on PCs running Windows/DOS...
 *
 *   Revision 1.2  1997/06/18  00:55:28  mike
 *   Updated to hold length table when writing.
 *   Updated to hold current length when doing ARLE.
 *   Wasn't writing length table on close.
 *   Wasn't saving new line into arle_row when necessary.
 *
 *   Revision 1.1  1997/06/15  03:37:19  mike
 *   Initial revision
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "sgi-lib.h"


/*
 * Local functions...
 */

static int      getlong(sgi_t*);
static int      getshort(sgi_t*);
static int      putlong(long, sgi_t*);
static int      putshort(unsigned short, sgi_t*);
static int      read_rle8(sgi_t*, unsigned short *, int);
static int      read_rle16(sgi_t*, unsigned short *, int);
static int      write_rle8(sgi_t*, unsigned short *, int);
static int      write_rle16(sgi_t*, unsigned short *, int);


/*
 * 'sgiClose()' - Close an SGI image file.
 */

int
sgiClose(sgi_t *sgip)   /* I - SGI image */
{
  int   i;              /* Return status */
  long  *offset;        /* Looping var for offset table */


  if (sgip == NULL)
    return (-1);

  if (sgip->mode == SGI_WRITE && sgip->comp != SGI_COMP_NONE)
  {
   /*
    * Write the scanline offset table to the file...
    */

    fseek(sgip->file, 512, SEEK_SET);

    for (i = sgip->ysize * sgip->zsize, offset = sgip->table[0];
         i > 0;
         i --, offset ++)
      if (putlong(offset[0], sgip) < 0)
        return (-1);

    for (i = sgip->ysize * sgip->zsize, offset = sgip->length[0];
         i > 0;
         i --, offset ++)
      if (putlong(offset[0], sgip) < 0)
        return (-1);
  };

  if (sgip->table != NULL)
  {
    free(sgip->table[0]);
    free(sgip->table);
  };

  if (sgip->length != NULL)
  {
    free(sgip->length[0]);
    free(sgip->length);
  };

  if (sgip->comp == SGI_COMP_ARLE)
    free(sgip->arle_row);

  i = fclose(sgip->file);
  free(sgip);

  return (i);
}


/*
 * 'sgiGetRow()' - Get a row of image data from a file.
 */

int
sgiGetRow(sgi_t          *sgip, /* I - SGI image */
          unsigned short *row,  /* O - Row to read */
          int            y,     /* I - Line to read */
          int            z)     /* I - Channel to read */
{
  int   x;              /* X coordinate */
  long  offset;         /* File offset */


  if (sgip == NULL ||
      row == NULL ||
      y < 0 || y >= sgip->ysize ||
      z < 0 || z >= sgip->zsize)
    return (-1);

  switch (sgip->comp)
  {
    case SGI_COMP_NONE :
       /*
        * Seek to the image row - optimize buffering by only seeking if
        * necessary...
        */

        offset = 512 + (y + z * sgip->ysize) * sgip->xsize * sgip->bpp;
        if (offset != ftell(sgip->file))
          fseek(sgip->file, offset, SEEK_SET);

        if (sgip->bpp == 1)
        {
          for (x = sgip->xsize; x > 0; x --, row ++)
            *row = getc(sgip->file);
        }
        else
        {
          for (x = sgip->xsize; x > 0; x --, row ++)
            *row = getshort(sgip);
        };
        break;

    case SGI_COMP_RLE :
        offset = sgip->table[z][y];
        if (offset != ftell(sgip->file))
          fseek(sgip->file, offset, SEEK_SET);

        if (sgip->bpp == 1)
          return (read_rle8(sgip, row, sgip->xsize));
        else
          return (read_rle16(sgip, row, sgip->xsize));
        break;
  };

  return (0);
}


/*
 * 'sgiOpen()' - Open an SGI image file for reading or writing.
 */

sgi_t *
sgiOpen(const char *filename,   /* I - File to open */
        int         mode,       /* I - Open mode (SGI_READ or SGI_WRITE) */
        int         comp,       /* I - Type of compression */
        int         bpp,        /* I - Bytes per pixel */
        int         xsize,      /* I - Width of image in pixels */
        int         ysize,      /* I - Height of image in pixels */
        int         zsize)      /* I - Number of channels */
{
  sgi_t *sgip;          /* New SGI image file */
  FILE  *file;          /* Image file pointer */


  if (mode == SGI_READ)
    file = g_fopen(filename, "rb");
  else
    file = g_fopen(filename, "w+b");

  if (file == NULL)
    return (NULL);

  if ((sgip = sgiOpenFile(file, mode, comp, bpp, xsize, ysize, zsize)) == NULL)
    fclose(file);

  return (sgip);
}


/*
 * 'sgiOpenFile()' - Open an SGI image file for reading or writing.
 */

sgi_t *
sgiOpenFile(FILE *file, /* I - File to open */
            int  mode,  /* I - Open mode (SGI_READ or SGI_WRITE) */
            int  comp,  /* I - Type of compression */
            int  bpp,   /* I - Bytes per pixel */
            int  xsize, /* I - Width of image in pixels */
            int  ysize, /* I - Height of image in pixels */
            int  zsize) /* I - Number of channels */
{
  int   i, j;           /* Looping var */
  char  name[80];       /* Name of file in image header */
  short magic;          /* Magic number */
  sgi_t *sgip;          /* New image pointer */


  if ((sgip = calloc(sizeof(sgi_t), 1)) == NULL)
    return (NULL);

  sgip->file = file;
  sgip->swapBytes = 0;

  switch (mode)
  {
    case SGI_READ :
        sgip->mode = SGI_READ;

        magic = getshort(sgip);
        if (magic != SGI_MAGIC)
        {
          /* try little endian format */
          magic = ((magic >> 8) & 0x00ff) | ((magic << 8) & 0xff00);
          if(magic != SGI_MAGIC) {
            free(sgip);
            return (NULL);
          } else {
            sgip->swapBytes = 1;
          }
        }

        sgip->comp  = getc(sgip->file);
        sgip->bpp   = getc(sgip->file);
        getshort(sgip);         /* Dimensions */
        sgip->xsize = getshort(sgip);
        sgip->ysize = getshort(sgip);
        sgip->zsize = getshort(sgip);
        getlong(sgip);          /* Minimum pixel */
        getlong(sgip);          /* Maximum pixel */

        if (sgip->comp)
        {
         /*
          * This file is compressed; read the scanline tables...
          */

          fseek(sgip->file, 512, SEEK_SET);

          sgip->table    = calloc(sgip->zsize, sizeof(long *));
          if (sgip->table == NULL)
            {
              free(sgip);
              return (NULL);
            }
          sgip->table[0] = calloc(sgip->ysize * sgip->zsize, sizeof(long));
          if (sgip->table[0] == NULL)
            {
              free(sgip->table);
              free(sgip);
              return (NULL);
            }
          for (i = 1; i < sgip->zsize; i ++)
            sgip->table[i] = sgip->table[0] + i * sgip->ysize;

          for (i = 0; i < sgip->zsize; i ++)
            for (j = 0; j < sgip->ysize; j ++)
              sgip->table[i][j] = getlong(sgip);
        };
        break;

    case SGI_WRITE :
        if (xsize < 1 ||
            ysize < 1 ||
            zsize < 1 ||
            bpp < 1 || bpp > 2 ||
            comp < SGI_COMP_NONE || comp > SGI_COMP_ARLE)
        {
          free(sgip);
          return (NULL);
        };

        sgip->mode = SGI_WRITE;

        putshort(SGI_MAGIC, sgip);
        putc((sgip->comp = comp) != 0, sgip->file);
        putc(sgip->bpp = bpp, sgip->file);
        putshort(3, sgip);              /* Dimensions */
        putshort(sgip->xsize = xsize, sgip);
        putshort(sgip->ysize = ysize, sgip);
        putshort(sgip->zsize = zsize, sgip);
        if (bpp == 1)
        {
          putlong(0, sgip);     /* Minimum pixel */
          putlong(255, sgip);   /* Maximum pixel */
        }
        else
        {
          putlong(-32768, sgip);        /* Minimum pixel */
          putlong(32767, sgip); /* Maximum pixel */
        };
        putlong(0, sgip);               /* Reserved */

        memset(name, 0, sizeof(name));
        fwrite(name, sizeof(name), 1, sgip->file);

        for (i = 0; i < 102; i ++)
          putlong(0, sgip);

        switch (comp)
        {
          case SGI_COMP_NONE : /* No compression */
             /*
              * This file is uncompressed.  To avoid problems with sparse files,
              * we need to write blank pixels for the entire image...
              */

              if (bpp == 1)
              {
                for (i = xsize * ysize * zsize; i > 0; i --)
                  putc(0, sgip->file);
              }
              else
              {
                for (i = xsize * ysize * zsize; i > 0; i --)
                  putshort(0, sgip);
              };
              break;

          case SGI_COMP_ARLE : /* Aggressive RLE */
              sgip->arle_row    = (unsigned short *)calloc(xsize, sizeof(unsigned short));
              if (sgip->arle_row == NULL)
                {
                  free(sgip);
                  return (NULL);
                }
              sgip->arle_offset = 0;

          case SGI_COMP_RLE : /* Run-Length Encoding */
             /*
              * This file is compressed; write the (blank) scanline tables...
              */

              for (i = 2 * ysize * zsize; i > 0; i --)
                putlong(0, sgip);

              sgip->firstrow = ftell(sgip->file);
              sgip->nextrow  = ftell(sgip->file);
              sgip->table    = calloc(sgip->zsize, sizeof(long *));
              if (sgip->table == NULL)
                {
                  free(sgip->arle_row);
                  free(sgip);
                  return (NULL);
                }
              sgip->table[0] = calloc(sgip->ysize * sgip->zsize, sizeof(long));
              if (sgip->table[0] == NULL)
                {
                  free(sgip->table);
                  free(sgip->arle_row);
                  free(sgip);
                  return (NULL);
                }
              for (i = 1; i < sgip->zsize; i ++)
                sgip->table[i] = sgip->table[0] + i * sgip->ysize;
              sgip->length    = calloc(sgip->zsize, sizeof(long *));
              sgip->length[0] = calloc(sgip->ysize * sgip->zsize, sizeof(long));
              for (i = 1; i < sgip->zsize; i ++)
                sgip->length[i] = sgip->length[0] + i * sgip->ysize;
              break;
        };
        break;

    default :
        free(sgip);
        return (NULL);
  };

  return (sgip);
}


/*
 * 'sgiPutRow()' - Put a row of image data to a file.
 */

int
sgiPutRow(sgi_t          *sgip, /* I - SGI image */
          unsigned short *row,  /* I - Row to write */
          int            y,     /* I - Line to write */
          int            z)     /* I - Channel to write */
{
  int   x;              /* X coordinate */
  long  offset;         /* File offset */


  if (sgip == NULL ||
      row == NULL ||
      y < 0 || y >= sgip->ysize ||
      z < 0 || z >= sgip->zsize)
    return (-1);

  switch (sgip->comp)
  {
    case SGI_COMP_NONE :
       /*
        * Seek to the image row - optimize buffering by only seeking if
        * necessary...
        */

        offset = 512 + (y + z * sgip->ysize) * sgip->xsize * sgip->bpp;
        if (offset != ftell(sgip->file))
          fseek(sgip->file, offset, SEEK_SET);

        if (sgip->bpp == 1)
        {
          for (x = sgip->xsize; x > 0; x --, row ++)
            putc(*row, sgip->file);
        }
        else
        {
          for (x = sgip->xsize; x > 0; x --, row ++)
            putshort(*row, sgip);
        };
        break;

    case SGI_COMP_ARLE :
        if (sgip->table[z][y] != 0)
          return (-1);

       /*
        * First check the last row written...
        */

        if (sgip->arle_offset > 0)
        {
          for (x = 0; x < sgip->xsize; x ++)
            if (row[x] != sgip->arle_row[x])
              break;

          if (x == sgip->xsize)
          {
            sgip->table[z][y]  = sgip->arle_offset;
            sgip->length[z][y] = sgip->arle_length;
            return (0);
          };
        };

       /*
        * If that didn't match, search all the previous rows...
        */

        fseek(sgip->file, sgip->firstrow, SEEK_SET);

        if (sgip->bpp == 1)
        {
          do
          {
            sgip->arle_offset = ftell(sgip->file);
            if ((sgip->arle_length = read_rle8(sgip, sgip->arle_row, sgip->xsize)) < 0)
            {
              x = 0;
              break;
            };

            for (x = 0; x < sgip->xsize; x ++)
              if (row[x] != sgip->arle_row[x])
                break;
          }
          while (x < sgip->xsize);
        }
        else
        {
          do
          {
            sgip->arle_offset = ftell(sgip->file);
            if ((sgip->arle_length = read_rle16(sgip, sgip->arle_row, sgip->xsize)) < 0)
            {
              x = 0;
              break;
            };

            for (x = 0; x < sgip->xsize; x ++)
              if (row[x] != sgip->arle_row[x])
                break;
          }
          while (x < sgip->xsize);
        };

        if (x == sgip->xsize)
        {
          sgip->table[z][y]  = sgip->arle_offset;
          sgip->length[z][y] = sgip->arle_length;
          return (0);
        }
        else
          fseek(sgip->file, 0, SEEK_END);       /* Clear EOF */

    case SGI_COMP_RLE :
        if (sgip->table[z][y] != 0)
          return (-1);

        offset = sgip->table[z][y] = sgip->nextrow;

        if (offset != ftell(sgip->file))
          fseek(sgip->file, offset, SEEK_SET);

        if (sgip->bpp == 1)
          x = write_rle8(sgip, row, sgip->xsize);
        else
          x = write_rle16(sgip, row, sgip->xsize);

        if (sgip->comp == SGI_COMP_ARLE)
        {
          sgip->arle_offset = offset;
          sgip->arle_length = x;
          memcpy(sgip->arle_row, row, sgip->xsize * sizeof(short));
        };

        sgip->nextrow      = ftell(sgip->file);
        sgip->length[z][y] = x;

        return (x);
  };

  return (0);
}


/*
 * 'getlong()' - Get a 32-bit big-endian integer.
 */

static int
getlong(sgi_t *sgip)    /* I - SGI image to read from */
{
  unsigned char b[4];


  fread(b, 4, 1, sgip->file);
  if(sgip->swapBytes)
    return ((b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0]);
  else
    return ((b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3]);
}


/*
 * 'getshort()' - Get a 16-bit big-endian integer.
 */

static int
getshort(sgi_t *sgip)   /* I - SGI image to read from */
{
  unsigned char b[2];


  fread(b, 2, 1, sgip->file);
  if(sgip->swapBytes)
    return ((b[1] << 8) | b[0]);
  else
    return ((b[0] << 8) | b[1]);
}


/*
 * 'putlong()' - Put a 32-bit big-endian integer.
 */

static int
putlong(long n,         /* I - Long to write */
        sgi_t *sgip)    /* I - File to write to */
{
  if (putc(n >> 24, sgip->file) == EOF)
    return (EOF);
  if (putc(n >> 16, sgip->file) == EOF)
    return (EOF);
  if (putc(n >> 8, sgip->file) == EOF)
    return (EOF);
  if (putc(n, sgip->file) == EOF)
    return (EOF);
  else
    return (0);
}


/*
 * 'putshort()' - Put a 16-bit big-endian integer.
 */

static int
putshort(unsigned short n,      /* I - Short to write */
         sgi_t *sgip)           /* I - File to write to */
{
  if (putc(n >> 8, sgip->file) == EOF)
    return (EOF);
  if (putc(n, sgip->file) == EOF)
    return (EOF);
  else
    return (0);
}


/*
 * 'read_rle8()' - Read 8-bit RLE data.
 */

static int
read_rle8(sgi_t *sgip,          /* I - SGI image to read from */
          unsigned short *row,  /* O - Data */
          int            xsize) /* I - Width of data in pixels */
{
  int   i,              /* Looping var */
        ch,             /* Current character */
        count,          /* RLE count */
        length;         /* Number of bytes read... */


  length = 0;

  while (xsize > 0)
  {
    if ((ch = getc(sgip->file)) == EOF)
      return (-1);
    length ++;

    count = MIN (ch & 127, xsize);
    if (count == 0)
      break;

    if (ch & 128)
    {
      for (i = 0; i < count; i ++, row ++, xsize --, length ++)
        *row = getc(sgip->file);
    }
    else
    {
      ch = getc(sgip->file);
      length ++;
      for (i = 0; i < count; i ++, row ++, xsize --)
        *row = ch;
    };
  };

  return (xsize > 0 ? -1 : length);
}


/*
 * 'read_rle16()' - Read 16-bit RLE data.
 */

static int
read_rle16(sgi_t *sgip,         /* I - SGI image to read from */
           unsigned short *row, /* O - Data */
           int            xsize)/* I - Width of data in pixels */
{
  int   i,              /* Looping var */
        ch,             /* Current character */
        count,          /* RLE count */
        length;         /* Number of bytes read... */


  length = 0;

  while (xsize > 0)
  {
    if ((ch = getshort(sgip)) == EOF)
      return (-1);
    length ++;

    count = MIN (ch & 127, xsize);
    if (count == 0)
      break;

    if (ch & 128)
    {
      for (i = 0; i < count; i ++, row ++, xsize --, length ++)
        *row = getshort(sgip);
    }
    else
    {
      ch = getshort(sgip);
      length ++;
      for (i = 0; i < count; i ++, row ++, xsize --)
        *row = ch;
    };
  };

  return (xsize > 0 ? -1 : length * 2);
}


/*
 * 'write_rle8()' - Write 8-bit RLE data.
 */

static int
write_rle8(sgi_t *sgip,         /* I - SGI image to write to */
           unsigned short *row, /* I - Data */
           int            xsize)/* I - Width of data in pixels */
{
  int                   length, /* Length of output line */
                        count,  /* Number of repeated/non-repeated pixels */
                        i,      /* Looping var */
                        x;      /* Looping var */
  unsigned short        *start, /* Start of sequence */
                        repeat; /* Repeated pixel */


  for (x = xsize, length = 0; x > 0;)
  {
    start = row;
    row   += 2;
    x     -= 2;

    while (x > 0 && (row[-2] != row[-1] || row[-1] != row[0]))
    {
      row ++;
      x --;
    };

    row -= 2;
    x   += 2;

    count = row - start;
    while (count > 0)
    {
      i     = count > 126 ? 126 : count;
      count -= i;

      if (putc(128 | i, sgip->file) == EOF)
        return (-1);
      length ++;

      while (i > 0)
      {
        if (putc(*start, sgip->file) == EOF)
          return (-1);
        start ++;
        i --;
        length ++;
      };
    };

    if (x <= 0)
      break;

    start  = row;
    repeat = row[0];

    row ++;
    x --;

    while (x > 0 && *row == repeat)
    {
      row ++;
      x --;
    };

    count = row - start;
    while (count > 0)
    {
      i     = count > 126 ? 126 : count;
      count -= i;

      if (putc(i, sgip->file) == EOF)
        return (-1);
      length ++;

      if (putc(repeat, sgip->file) == EOF)
        return (-1);
      length ++;
    };
  };

  length ++;

  if (putc(0, sgip->file) == EOF)
    return (-1);
  else
    return (length);
}


/*
 * 'write_rle16()' - Write 16-bit RLE data.
 */

static int
write_rle16(sgi_t *sgip,        /* I - SGI image to write to */
            unsigned short *row,/* I - Data */
            int            xsize)/* I - Width of data in pixels */
{
  int                   length, /* Length of output line */
                        count,  /* Number of repeated/non-repeated pixels */
                        i,      /* Looping var */
                        x;      /* Looping var */
  unsigned short        *start, /* Start of sequence */
                        repeat; /* Repeated pixel */


  for (x = xsize, length = 0; x > 0;)
  {
    start = row;
    row   += 2;
    x     -= 2;

    while (x > 0 && (row[-2] != row[-1] || row[-1] != row[0]))
    {
      row ++;
      x --;
    };

    row -= 2;
    x   += 2;

    count = row - start;
    while (count > 0)
    {
      i     = count > 126 ? 126 : count;
      count -= i;

      if (putshort(128 | i, sgip) == EOF)
        return (-1);
      length ++;

      while (i > 0)
      {
        if (putshort(*start, sgip) == EOF)
          return (-1);
        start ++;
        i --;
        length ++;
      };
    };

    if (x <= 0)
      break;

    start  = row;
    repeat = row[0];

    row ++;
    x --;

    while (x > 0 && *row == repeat)
    {
      row ++;
      x --;
    };

    count = row - start;
    while (count > 0)
    {
      i     = count > 126 ? 126 : count;
      count -= i;

      if (putshort(i, sgip) == EOF)
        return (-1);
      length ++;

      if (putshort(repeat, sgip) == EOF)
        return (-1);
      length ++;
    };
  };

  length ++;

  if (putshort(0, sgip) == EOF)
    return (-1);
  else
    return (2 * length);
}
