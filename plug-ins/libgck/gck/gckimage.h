/***************************************************************************/
/* GCK - The General Convenience Kit. Generally useful conveniece routines */
/* for GIMP plug-in writers and users of the GDK/GTK libraries.            */
/* Copyright (C) 1996 Tom Bech                                             */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2 of the License, or       */
/* (at your option) any later version.                                     */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software             */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,   */
/* USA.                                                                    */
/***************************************************************************/

#ifndef __GCKIMAGE_H__
#define _-GCKIMAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  guint width;
  guint height;
  unsigned long *indextab;
  unsigned long *numbytes;
  unsigned long *rowsize;
  guchar *data;
} RGBImage;

RGBImage *RGB_image_new(guint width,guint height);

void      RGB_image_destroy(RGBImage *image);

void      RGB_image_fill(RGBImage *image,guchar red,guchar green,guchar blue);

void      RGB_image_draw_rectangle(RGBImage *image,gint filled,gint x,gint y,gint width,
                                   gint height,guchar red,guchar green,guchar blue);

void      RGB_image_draw_line(RGBImage *image,int x1,int y1,int x2,int y2,
                              guchar red,guchar green,guchar blue);

void      RGB_image_draw_arc(RGBImage *image,gint filled,int x,int y,guint radius,
                             guint angle1,guint angle2,guchar red,guchar green,guchar blue);

#ifdef __cplusplus
}
#endif

#endif
