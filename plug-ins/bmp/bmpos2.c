/* bmpos2.c			*/
/*	                	*/
/* asbjorn.pettersen@dualog.no  */

/* 
 * The GIMP -- an image manipulation program
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
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include "bmp.h"

static void dump_file_head (struct Bitmap_File_Head_Struct *p)
{
  printf("bfSize=%ld\n",p->bfSize);
  printf("bfoffs=%ld\n",p->bfOffs);
  printf("biSize=%ld\n",p->biSize);	
}

static void dump_os2_head (struct Bitmap_OS2_Head_Struct *p)
{
  printf("bcWidth =%4d ",p->bcWidth);
  printf("bcHeigth=%4d\n",p->bcHeight);
  printf("bcPlanes=%4d ",p->bcPlanes);
  printf("bcBitCnt=%4d\n",p->bcBitCnt);
}

int read_os2_head1 (FILE *fd, int headsz, struct Bitmap_Head_Struct *p)
{
  guchar puffer[150];

  if (!ReadOK (fd, puffer, headsz))
    {
      return 0;
    }
  
  Bitmap_OS2_Head.bcWidth  = ToS (&puffer[0]);
  Bitmap_OS2_Head.bcHeight = ToS (&puffer[2]);
  Bitmap_OS2_Head.bcPlanes = ToS (&puffer[4]);
  Bitmap_OS2_Head.bcBitCnt = ToS (&puffer[6]);
#if 0
  dump_os2_head (&Bitmap_OS2_Head);
#endif
  p->biHeight    = Bitmap_OS2_Head.bcHeight;
  p->biWidth     = Bitmap_OS2_Head.bcWidth;
  p->biPlanes    = Bitmap_OS2_Head.bcPlanes;
  p->biBitCnt    = Bitmap_OS2_Head.bcBitCnt;
  return 1;
}

