 /* The GIMP -- an image manipulation program
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <string.h>

#include "appenv.h"
#include "brushes.h"
#include "canvas.h"
#include "clone.h"
#include "draw_core.h"
#include "drawable.h"
#include "gimage.h"
#include "gdisplay.h"
#include "interface.h"
#include "paint_core_16.h"
#include "paint_funcs_area.h"
#include "patterns.h"
#include "pixelarea.h"
#include "procedural_db.h"
#include "tools.h"

#define INTENSITY(r,g,b) (r * 0.30 + g * 0.59 + b * 0.11 + 0.001)

#define TARGET_HEIGHT  15
#define TARGET_WIDTH   15

typedef enum
{
  ImageClone,
  PatternClone
} CloneType;

/*  forward function declarations  */
static void         clone_draw            (Tool *);
static void         clone_motion          (PaintCore16 *, GimpDrawable *, GimpDrawable *, CloneType, int, int);
static void         clone_line_pattern    (GImage *, GimpDrawable *, GPatternP, unsigned char *,
					   int, int, int, int);

static Argument *   clone_invoker         (Argument *);


static GimpDrawable *non_gui_src_drawable;
static int non_gui_offset_x;
static int non_gui_offset_y;
static CloneType non_gui_type;

typedef struct _CloneOptions CloneOptions;
struct _CloneOptions
{
  CloneType type;
  int       aligned;
};

/*  local variables  */
static int          src_x = 0;                /*                         */
static int          src_y = 0;                /*  position of clone src  */
static int          dest_x = 0;               /*                         */
static int          dest_y = 0;               /*  position of clone src  */
static int          offset_x = 0;             /*                         */
static int          offset_y = 0;             /*  offset for cloning     */
static int          first = TRUE;
static int          trans_tx, trans_ty;       /*  transformed target  */
static int          src_gdisp_ID = -1;        /*  ID of source gdisplay  */
static GimpDrawable * src_drawable_ = NULL;   /*  source drawable */
static CloneOptions *clone_options = NULL;


/*
     This stuff was my "super-smart" copy_area().  i've just moved it
     here for the moment until we make a decision on Paint, PixelRow,
     and so on.
*/
#include "pixelrow.h"


void 
copy_row  (
           PixelRow * src_row,
           PixelRow * dest_row
           )
{
  switch (tag_precision (pixelrow_tag (src_row)))
    {
    case PRECISION_U8:
      copy_row_u8 (src_row, dest_row);
      break;
    case PRECISION_U16:
      copy_row_u16 (src_row, dest_row);
      break;
    case PRECISION_FLOAT:
      copy_row_float (src_row, dest_row);
      break;
    case PRECISION_NONE:
      g_warning ("doh in copy_row()");
      break;	
    }
}

/*******************************************************
 8bit routines
********************************************************/
static void
copy_row_u8_rgb_to_u8_rgb (
                           PixelRow * src_row,
                           PixelRow * dest_row
                           )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);

  if (tag_alpha (stag) == tag_alpha (dtag))
    {
      memcpy (pixelrow_data (dest_row),
              pixelrow_data (src_row),
              w * tag_bytes (stag));
    }
  else
    {
      guint8 * s = (guint8*) pixelrow_data (src_row);
      guint8 * d = (guint8*) pixelrow_data (dest_row);
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            d[3] = 255;
            s += 3;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            s += 4;
            d += 3;
          }
    }
}


static void
copy_row_u8_gray_to_u8_gray (
                           PixelRow * src_row,
                           PixelRow * dest_row
                           )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);

  if (tag_alpha (stag) == tag_alpha (dtag))
    {
      memcpy (pixelrow_data (dest_row),
              pixelrow_data (src_row),
              w * tag_bytes (stag));
    }
  else
    {
      guint8 * s = (guint8*) pixelrow_data (src_row);
      guint8 * d = (guint8*) pixelrow_data (dest_row);
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0];
            d[1] = 255;
            s += 1;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = s[0];
            s += 2;
            d += 1;
          }
    }
}


static void 
copy_row_u8_gray_to_u8_rgb  (
                             PixelRow * src_row,
                             PixelRow * dest_row
                             )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint8 * s = (guint8*) pixelrow_data (src_row);
  guint8 * d = (guint8*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0];
            d[3] = s[1];
            s += 2;
            d += 4;
          }
      else
        while (w--)
          {
            if (s[1] < 128)
              d[0] = d[1] = d[2] = 0;
            else
              d[0] = d[1] = d[2] = s[0];
            s += 2;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0];
            d[3] = 255;
            s += 1;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0];
            s += 1;
            d += 3;
          }
    }
}


static void 
copy_row_u8_rgb_to_u8_gray  (
                             PixelRow * src_row,
                             PixelRow * dest_row
                             )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint8 * s = (guint8*) pixelrow_data (src_row);
  guint8 * d = (guint8*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            d[1] = s[3];
            s += 4;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            s += 4;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            d[1] = 255;
            s += 3;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            s += 3;
            d += 1;
          }
    }
}

static void 
copy_row_u8_rgb_to_u16_rgb  (
                             PixelRow * src_row,
                             PixelRow * dest_row
                             )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint8 * s = (guint8*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] * 255;
            d[1] = s[1] * 255;
            d[2] = s[2] * 255;
            d[3] = s[3] * 255;
            s += 4;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0] * 255;
            d[1] = s[1] * 255;
            d[2] = s[2] * 255;
            s += 4;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] * 255;
            d[1] = s[1] * 255;
            d[2] = s[2] * 255;
            d[3] = 65535;
            s += 3;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0] * 255;
            d[1] = s[1] * 255;
            d[2] = s[2] * 255;
            s += 3;
            d += 3;
          }
    }
}


static void 
copy_row_u8_rgb_to_u16_gray  (
                              PixelRow * src_row,
                              PixelRow * dest_row
                              )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint8 * s = (guint8*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY ((s[0]*255), (s[1]*255), (s[2]*255));
            d[1] = s[3] * 255;
            s += 4;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY ((s[0]*255), (s[1]*255), (s[2]*255));
            s += 4;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY ((s[0]*255), (s[1]*255), (s[2]*255));
            d[1] = 65535;
            s += 3;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY ((s[0]*255), (s[1]*255), (s[2]*255));
            s += 3;
            d += 1;
          }
    }
}


static void 
copy_row_u8_rgb_to_float_rgb  (
                               PixelRow * src_row,
                               PixelRow * dest_row
                               )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint8 * s = (guint8*) pixelrow_data (src_row);
  gfloat * d = (gfloat*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] / (gfloat) 255;
            d[1] = s[1] / (gfloat) 255;
            d[2] = s[2] / (gfloat) 255;
            d[3] = s[3] / (gfloat) 255;
            s += 4;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0] / (gfloat) 255;
            d[1] = s[1] / (gfloat) 255;
            d[2] = s[2] / (gfloat) 255;
            s += 4;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] / (gfloat) 255;
            d[1] = s[1] / (gfloat) 255;
            d[2] = s[2] / (gfloat) 255;
            d[3] = 1.0;
            s += 3;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0] / (gfloat) 255;
            d[1] = s[1] / (gfloat) 255;
            d[2] = s[2] / (gfloat) 255;
            s += 3;
            d += 3;
          }
    }
}


static void 
copy_row_u8_rgb_to_float_gray  (
                                PixelRow * src_row,
                                PixelRow * dest_row
                                )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint8 * s = (guint8*) pixelrow_data (src_row);
  gfloat * d = (gfloat*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY ((s[0]/(gfloat)255), (s[1]/(gfloat)255), (s[2]/(gfloat)255));
            d[1] = s[3] / (gfloat) 255;
            s += 4;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY ((s[0]/(gfloat)255), (s[1]/(gfloat)255), (s[2]/(gfloat)255));
            s += 4;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY ((s[0]/(gfloat)255), (s[1]/(gfloat)255), (s[2]/(gfloat)255));
            d[1] = 1.0;
            s += 3;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY ((s[0]/(gfloat)255), (s[1]/(gfloat)255), (s[2]/(gfloat)255));
            s += 3;
            d += 1;
          }
    }
}


static void
copy_row_u8_rgb_to_float (
                          PixelRow * src_row,
                          PixelRow * dest_row
                          )
{
  switch (tag_format (pixelrow_tag (dest_row)))
    {
    case FORMAT_RGB:
      copy_row_u8_rgb_to_float_rgb (src_row, dest_row);
      break;
    case FORMAT_GRAY:
      copy_row_u8_rgb_to_float_gray (src_row, dest_row);
      break;
    case FORMAT_INDEXED:
    case FORMAT_NONE:
      g_warning ("doh in copy_row_u8_rgb_to_float()");
      break;	
    }
}

static void
copy_row_u8_rgb_to_u16 (
                        PixelRow * src_row,
                        PixelRow * dest_row
                        )
{
  switch (tag_format (pixelrow_tag (dest_row)))
    {
    case FORMAT_RGB:
      copy_row_u8_rgb_to_u16_rgb (src_row, dest_row);
      break;
    case FORMAT_GRAY:
      copy_row_u8_rgb_to_u16_gray (src_row, dest_row);
      break;
    case FORMAT_INDEXED:
    case FORMAT_NONE:
      g_warning ("doh in copy_row_u8_rgb_to_u16()");
      break; 
    }
}

static void
copy_row_u8_rgb_to_u8 (
                       PixelRow * src_row,
                       PixelRow * dest_row
                       )
{
  switch (tag_format (pixelrow_tag (dest_row)))
    {
    case FORMAT_RGB:
      copy_row_u8_rgb_to_u8_rgb (src_row, dest_row);
      break;
    case FORMAT_GRAY:
      copy_row_u8_rgb_to_u8_gray (src_row, dest_row);
      break;
    case FORMAT_INDEXED:
    case FORMAT_NONE:
      g_warning ("doh in copy_row_u8_rgb_to_u8()");
      break;	
    }
}

static void
copy_row_u8_rgb (
                 PixelRow * src_row,
                 PixelRow * dest_row
                 )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      copy_row_u8_rgb_to_u8 (src_row, dest_row);
      break;
    case PRECISION_U16:
      copy_row_u8_rgb_to_u16 (src_row, dest_row);
      break; 
    case PRECISION_FLOAT:
      copy_row_u8_rgb_to_float (src_row, dest_row);
      break;
    case PRECISION_NONE:
      g_warning ("doh in copy_row_u8_rgb()");
      break;	
    }
}

static void 
copy_row_u8_gray_to_u8  (
                         PixelRow * src_row,
                         PixelRow * dest_row
                         )
{
  switch (tag_format (pixelrow_tag (dest_row)))
    {
    case FORMAT_RGB:
      copy_row_u8_gray_to_u8_rgb (src_row, dest_row);
      break;
    case FORMAT_GRAY:
      copy_row_u8_gray_to_u8_gray (src_row, dest_row);
      break;
    case FORMAT_INDEXED:
    case FORMAT_NONE:
      g_warning ("doh in copy_row_u8_gray_to_u8()");
      break;	
    }
}

static void 
copy_row_u8_gray  (
                   PixelRow * src_row,
                   PixelRow * dest_row
                   )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      copy_row_u8_gray_to_u8 (src_row, dest_row);
      break;
    case PRECISION_U16:
    case PRECISION_FLOAT:
    case PRECISION_NONE:
      g_warning ("doh in copy_row_u8_gray()");
      break;	
    }
}

void
copy_row_u8 (
             PixelRow * src_row,
             PixelRow * dest_row
             )
{
  switch (tag_format (pixelrow_tag (src_row)))
    {
    case FORMAT_RGB:
      copy_row_u8_rgb (src_row, dest_row);
      break;
    case FORMAT_GRAY:
      copy_row_u8_gray (src_row, dest_row);
      break;
    case FORMAT_INDEXED:
    case FORMAT_NONE:
      g_warning ("doh in copy_row_u8()");
      break;	
    }
}






/*******************************************************
 16bit routines
********************************************************/

static void 
copy_row_u16_rgb_to_u16_rgb  (
                              PixelRow * src_row,
                              PixelRow * dest_row
                              )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);

  if (tag_alpha (stag) == tag_alpha (dtag))
    {
      memcpy (pixelrow_data (dest_row),
              pixelrow_data (src_row),
              w * tag_bytes (stag));
    }
  else
    {
      guint16 * s = (guint16*)pixelrow_data (src_row);
      guint16 * d = (guint16*)pixelrow_data (dest_row);
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            d[3] = 65535;
            s += 3;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            s += 4;
            d += 3;
          }
    }
}

static void 
copy_row_u16_gray_to_u16_gray  (
                                PixelRow * src_row,
                                PixelRow * dest_row
                                )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);

  if (tag_alpha (stag) == tag_alpha (dtag))
    {
      memcpy (pixelrow_data (dest_row),
              pixelrow_data (src_row),
              w * tag_bytes (stag));
    }
  else
    {
      guint16 * s = (guint16*)pixelrow_data (src_row);
      guint16 * d = (guint16*)pixelrow_data (dest_row);
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0];
            d[1] = 65535;
            s += 1;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = s[0];
            s += 2;
            d += 1;
          }
    }
}


static void 
copy_row_u16_gray_to_u16_rgb  (
                               PixelRow * src_row,
                               PixelRow * dest_row
                               )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s =(guint16*) pixelrow_data (src_row);
  guint16 * d =(guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0];
            d[3] = s[1];
            s += 2;
            d += 4;
          }
      else
        while (w--)
          {
            if (s[1] < 32767)
              d[0] = d[1] = d[2] = 0;
            else
              d[0] = d[1] = d[2] = s[0];
            s += 2;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0];
            d[3] = 65535;
            s += 1;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0];
            s += 1;
            d += 3;
          }
    }
}


static void 
copy_row_u16_rgb_to_u16_gray  (
                               PixelRow * src_row,
                               PixelRow * dest_row
                               )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*)pixelrow_data (src_row);
  guint16 * d = (guint16*)pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            d[1] = s[3];
            s += 4;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            s += 4;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            d[1] = 65535;
            s += 3;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            s += 3;
            d += 1;
          }
    }
}

static void
copy_row_u16_rgb_to_u16 (
                       PixelRow * src_row,
                       PixelRow * dest_row
                       )
{
  switch (tag_format (pixelrow_tag (dest_row)))
    {
    case FORMAT_RGB:
      copy_row_u16_rgb_to_u16_rgb (src_row, dest_row);
      break;
    case FORMAT_GRAY:
      copy_row_u16_rgb_to_u16_gray (src_row, dest_row);
      break;
    case FORMAT_INDEXED:
    case FORMAT_NONE:
      g_warning ("doh in copy_row_u16_rgb_to_u16()");
      break;	
    }
}

static void
copy_row_u16_rgb (
                 PixelRow * src_row,
                 PixelRow * dest_row
                 )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U16:
      copy_row_u16_rgb_to_u16 (src_row, dest_row);
      break;	
    case PRECISION_U8:
    case PRECISION_FLOAT:
    case PRECISION_NONE:
      g_warning ("doh in copy_row_u16_rgb()");
      break;	
    }
}

static void 
copy_row_u16_gray_to_u16  (
                         PixelRow * src_row,
                         PixelRow * dest_row
                         )
{
  switch (tag_format (pixelrow_tag (dest_row)))
    {
    case FORMAT_RGB:
      copy_row_u16_gray_to_u16_rgb (src_row, dest_row);
      break;
    case FORMAT_GRAY:
      copy_row_u16_gray_to_u16_gray (src_row, dest_row);
      break;
    case FORMAT_INDEXED:
    case FORMAT_NONE:
      g_warning ("doh in copy_row_u16_gray_to_u16()");
      break;	
    }
}

static void 
copy_row_u16_gray  (
                   PixelRow * src_row,
                   PixelRow * dest_row
                   )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U16:
      copy_row_u16_gray_to_u16 (src_row, dest_row);
      break;
    case PRECISION_U8:
    case PRECISION_FLOAT:
    case PRECISION_NONE:
      g_warning ("doh in copy_row_u16_gray()");
      break;	
    }
}

void
copy_row_u16 (
             PixelRow * src_row,
             PixelRow * dest_row
             )
{
  switch (tag_format (pixelrow_tag (src_row)))
    {
    case FORMAT_RGB:
      copy_row_u16_rgb (src_row, dest_row);
      break;
    case FORMAT_GRAY:
      copy_row_u16_gray (src_row, dest_row);
      break;
    case FORMAT_INDEXED:
    case FORMAT_NONE:
      g_warning ("doh in copy_row_u16()");
      break;	
    }
}




/*******************************************************
 float routines
********************************************************/
static void 
copy_row_float_rgb_to_u8_rgb  (
                               PixelRow * src_row,
                               PixelRow * dest_row
                               )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  guint8 * d = (guint8*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] * 255;
            d[1] = s[1] * 255;
            d[2] = s[2] * 255;
            d[3] = s[3] * 255;
            s += 4;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0] * 255;
            d[1] = s[1] * 255;
            d[2] = s[2] * 255;
            s += 4;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] * 255;
            d[1] = s[1] * 255;
            d[2] = s[2] * 255;
            d[3] = 255;
            s += 3;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0] * 255;
            d[1] = s[1] * 255;
            d[2] = s[2] * 255;
            s += 3;
            d += 3;
          }
    }
}


static void 
copy_row_float_rgb_to_u8_gray  (
                                PixelRow * src_row,
                                PixelRow * dest_row
                                )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  guint8 * d = (guint8*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY ((255*s[0]), (255*s[1]), (255*s[2]));
            d[1] = s[3] * 255;
            s += 4;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY ((255*s[0]), (255*s[1]), (255*s[2]));
            s += 4;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY ((255*s[0]), (255*s[1]), (255*s[2]));
            d[1] = 255;
            s += 3;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY ((255*s[0]), (255*s[1]), (255*s[2]));
            s += 3;
            d += 1;
          }
    }
}


static void 
copy_row_float_rgb_to_u16_rgb  (
                                PixelRow * src_row,
                                PixelRow * dest_row
                                )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] * 65535;
            d[1] = s[1] * 65535;
            d[2] = s[2] * 65535;
            d[3] = s[3] * 65535;
            s += 4;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0] * 65535;
            d[1] = s[1] * 65535;
            d[2] = s[2] * 65535;
            s += 4;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] * 65535;
            d[1] = s[1] * 65535;
            d[2] = s[2] * 65535;
            d[3] = 65535;
            s += 3;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0] * 65535;
            d[1] = s[1] * 65535;
            d[2] = s[2] * 65535;
            s += 3;
            d += 3;
          }
    }
}


static void 
copy_row_float_rgb_to_u16_gray  (
                                 PixelRow * src_row,
                                 PixelRow * dest_row
                                 )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY ((65535*s[0]), (65535*s[1]), (65535*s[2]));
            d[1] = s[3] * 65535;
            s += 4;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY ((65535*s[0]), (65535*s[1]), (65535*s[2]));
            s += 4;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY ((65535*s[0]), (65535*s[1]), (65535*s[2]));
            d[1] = 65535;
            s += 3;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY ((65535*s[0]), (65535*s[1]), (65535*s[2]));
            s += 3;
            d += 1;
          }
    }
}

static void 
copy_row_float_rgb_to_u8  (
                           PixelRow * src_row,
                           PixelRow * dest_row
                           )
{
  switch (tag_format (pixelrow_tag (dest_row)))
    {
    case FORMAT_RGB:
      copy_row_float_rgb_to_u8_rgb (src_row, dest_row);
      break;
    case FORMAT_GRAY:
      copy_row_float_rgb_to_u8_gray (src_row, dest_row);
      break;
    case FORMAT_INDEXED:
    case FORMAT_NONE:
      g_warning ("doh in copy_row_float_rgb_to_u8()");
      break;	
    }
}

static void 
copy_row_float_rgb_to_u16  (
                            PixelRow * src_row,
                            PixelRow * dest_row
                            )
{
  switch (tag_format (pixelrow_tag (dest_row)))
    {
    case FORMAT_RGB:
      copy_row_float_rgb_to_u16_rgb (src_row, dest_row);
      break;
    case FORMAT_GRAY:
      copy_row_float_rgb_to_u16_gray (src_row, dest_row);
      break;
    case FORMAT_INDEXED:
    case FORMAT_NONE:
      g_warning ("doh in copy_row_float_rgb_to_u16()");
      break;	
    }
}

static void 
copy_row_float_rgb  (
                     PixelRow * src_row,
                     PixelRow * dest_row
                     )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      copy_row_float_rgb_to_u8(src_row, dest_row);
      break;	
    case PRECISION_U16:
      copy_row_float_rgb_to_u16 (src_row, dest_row);
      break;	
    case PRECISION_FLOAT:
    case PRECISION_NONE:
      g_warning ("doh in copy_row_float_rgb()");
      break;	
    }
}

void 
copy_row_float  (
                 PixelRow * src_row,
                 PixelRow * dest_row
                 )
{
  switch (tag_format (pixelrow_tag (src_row)))
    {
    case FORMAT_RGB:
      copy_row_float_rgb (src_row, dest_row);
      break;
    case FORMAT_GRAY:
    case FORMAT_INDEXED:
    case FORMAT_NONE:
      g_warning ("doh in copy_row_float()");
      break;	
    }
}



















static void
clone_toggle_update (GtkWidget *widget,
		     gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
clone_type_callback (GtkWidget *w,
		     gpointer   client_data)
{
  clone_options->type =(CloneType) client_data;
}

static CloneOptions *
create_clone_options (void)
{
  CloneOptions *options;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *aligned_toggle;
  GtkWidget *radio_frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GSList *group = NULL;
  int i;
  char *button_names[2] =
  {
    "Image Source",
    "Pattern Source"
  };

  /*  the new options structure  */
  options = (CloneOptions *) g_malloc (sizeof (CloneOptions));
  options->type = ImageClone;
  options->aligned = TRUE;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  label = gtk_label_new ("Clone Tool Options");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the radio frame and box  */
  radio_frame = gtk_frame_new ("Source");
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

  /*  the radio buttons  */
  for (i = 0; i < 2; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, button_names[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) clone_type_callback,
			  (void *)((long) i));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);

  /*  the aligned toggle button  */
  aligned_toggle = gtk_check_button_new_with_label ("Aligned");
  gtk_box_pack_start (GTK_BOX (vbox), aligned_toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (aligned_toggle), "toggled",
		      (GtkSignalFunc) clone_toggle_update,
		      &options->aligned);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (aligned_toggle), options->aligned);
  gtk_widget_show (aligned_toggle);

  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (CLONE, vbox);

  return options;
}

void *
clone_paint_func (PaintCore16 *paint_core,
		  GimpDrawable *drawable,
		  int        state)
{
  GDisplay * gdisp;
  GDisplay * src_gdisp;
  int x1, y1, x2, y2;

  gdisp = (GDisplay *) active_tool->gdisp_ptr;

  switch (state)
    {
    case MOTION_PAINT :
      x1 = paint_core->curx;
      y1 = paint_core->cury;
      x2 = paint_core->lastx;
      y2 = paint_core->lasty;

      /*  If the control key is down, move the src target and return */
      if (paint_core->state & ControlMask)
	{
	  src_x = x1;
	  src_y = y1;
	  first = TRUE;
	}
      /*  otherwise, update the target  */
      else
	{
	  dest_x = x1;
	  dest_y = y1;
	  if (first)
	    {
	      offset_x = src_x - dest_x;
	      offset_y = src_y - dest_y;
	      first = FALSE;
	    }
	  else
	    {
	      src_x = dest_x + offset_x;
	      src_y = dest_y + offset_y;
	    }

	  clone_motion (paint_core, drawable, src_drawable_, clone_options->type, offset_x, offset_y);
	}

      draw_core_pause (paint_core->core, active_tool);
      break;

    case INIT_PAINT :
      if (paint_core->state & ControlMask)
	{
	  src_gdisp_ID = gdisp->ID;
	  src_drawable_ = drawable;
	  src_x = paint_core->curx;
	  src_y = paint_core->cury;
	  first = TRUE;
	}
      else if (clone_options->aligned == FALSE)
	first = TRUE;

      if (clone_options->type == PatternClone)
	if (!get_active_pattern ())
	  message_box ("No patterns available for this operation.", NULL, NULL);
      break;

    case FINISH_PAINT :
      draw_core_stop (paint_core->core, active_tool);
      return NULL;
      break;

    default :
      break;
    }

  /*  Calculate the coordinates of the target  */
  src_gdisp = gdisplay_get_ID (src_gdisp_ID);
  if (!src_gdisp)
    {
      src_gdisp_ID = gdisp->ID;
      src_gdisp = gdisplay_get_ID (src_gdisp_ID);
    }

  /*  Find the target cursor's location onscreen  */
  gdisplay_transform_coords (src_gdisp, src_x, src_y, &trans_tx, &trans_ty, 1);

  if (state == INIT_PAINT)
    /*  Initialize the tool drawing core  */
    draw_core_start (paint_core->core,
		     src_gdisp->canvas->window,
		     active_tool);
  else if (state == MOTION_PAINT)
    draw_core_resume (paint_core->core, active_tool);

  return NULL;
}

Tool *
tools_new_clone ()
{
  Tool * tool;
  PaintCore16 * private;

  if (! clone_options)
    clone_options = create_clone_options ();

  tool = paint_core_16_new (CLONE);

  private = (PaintCore16 *) tool->private;
  private->paint_func = clone_paint_func;
  private->core->draw_func = clone_draw;

  return tool;
}

void
tools_free_clone (Tool *tool)
{
  paint_core_16_free (tool);
}

static void
clone_draw (Tool *tool)
{
  PaintCore16 * paint_core;

  paint_core = (PaintCore16 *) tool->private;

  if (clone_options->type == ImageClone)
    {
      gdk_draw_line (paint_core->core->win, paint_core->core->gc,
		     trans_tx - (TARGET_WIDTH >> 1), trans_ty,
		     trans_tx + (TARGET_WIDTH >> 1), trans_ty);
      gdk_draw_line (paint_core->core->win, paint_core->core->gc,
		     trans_tx, trans_ty - (TARGET_HEIGHT >> 1),
		     trans_tx, trans_ty + (TARGET_HEIGHT >> 1));
    }
}



static int 
clone_motion_image  (
                     PaintCore16 * paint_core,
                     Canvas * painthit,
                     GimpDrawable * drawable,
                     GimpDrawable * src_drawable,
                     int x,
                     int y
                     )
{
  GImage * src_gimage = drawable_gimage (src_drawable);
  GImage * gimage = drawable_gimage (drawable);
  int rc = FALSE;

  if (gimage && src_gimage)
    {
      PixelArea srcPR, destPR;
      int x1, y1, x2, y2;
    
      if (src_drawable != drawable)
        {
          x1 = BOUNDS (x,
                       0, drawable_width (src_drawable));
          y1 = BOUNDS (y,
                       0, drawable_height (src_drawable));
          x2 = BOUNDS (x + canvas_width (painthit),
                       0, drawable_width (src_drawable));
          y2 = BOUNDS (y + canvas_height (painthit),
                       0, drawable_height (src_drawable));
          if (!(x2 - x1) || !(y2 - y1))
            return FALSE;
          
          pixelarea_init (&srcPR, drawable_data_canvas (src_drawable), NULL,
                          x1, y1, (x2 - x1), (y2 - y1), FALSE);
        }
      else
        {
          Canvas * orig;
          
          x1 = BOUNDS (x,
                       0, drawable_width (drawable));
          y1 = BOUNDS (y,
                       0, drawable_height (drawable));
          x2 = BOUNDS (x + canvas_width (painthit),
                       0, drawable_width (drawable));
          y2 = BOUNDS (y + canvas_height (painthit),
                       0, drawable_height (drawable));
          if (!(x2 - x1) || !(y2 - y1))
            return FALSE;

          orig = paint_core_16_area_original (paint_core, drawable,
                                              x1, y1, x2, y2);
          pixelarea_init (&srcPR, orig, NULL,
                          0, 0, (x2 - x1), (y2 - y1), FALSE);
        }
      
      pixelarea_init (&destPR, painthit, NULL,
                      (x1 - x), (y1 - y), (x2 - x1), (y2 - y1), TRUE);

      copy_area (&srcPR, &destPR);
      
      rc = TRUE;
    }
  return rc;
}


static int 
clone_motion_pattern  (
                       PaintCore16 * paint_core,
                       Canvas * painthit,
                       GimpDrawable * drawable,
                       GPatternP pattern,
                       int x,
                       int y
                       )
{
  GImage * gimage;
  int rc = FALSE;
  
  gimage = drawable_gimage (drawable);
  if (gimage)
    {
      PixelArea painthit_area;
      void * pag;
      gint pattern_width = canvas_width (pattern->mask_canvas);
      gint pattern_height = canvas_height (pattern->mask_canvas);

      pixelarea_init (&painthit_area, painthit, NULL,
                      0, 0, 0, 0, TRUE);
      for (pag = pixelarea_register (1, &painthit_area);
           pag != NULL;
           pag = pixelarea_process (pag))
        {
	   PixelArea painthit_portion_area, pat_area;
	   GdkRectangle pat_rect;
	   GdkRectangle painthit_portion_rect;
	   GdkRectangle r;
	   gint x_pat, y_pat;
	   gint x_pat_start, y_pat_start;
	   gint x_portion, y_portion, w_portion, h_portion;
	   gint extra;
	   x_portion = pixelarea_x ( &painthit_area );
	   y_portion = pixelarea_y ( &painthit_area );
	   w_portion = pixelarea_width ( &painthit_area );
	   h_portion = pixelarea_height ( &painthit_area );
	   
           if (x + x_portion >= 0)
             extra = (x + x_portion) % pattern_width;
           else
	   {
	     extra = (x + x_portion) % pattern_width;
	     extra += pattern_width;
	   }
	   x_pat_start = (x + x_portion) - extra; 
           
	   if (y + y_portion >= 0)
             extra = (y + y_portion) % pattern_height;
           else
	   {
	     extra = (y + y_portion) % pattern_height;
	     extra += pattern_height;
	   }
	   y_pat_start = (y + y_portion) - extra;
	   
	   x_pat = x_pat_start;
	   y_pat = y_pat_start;
	   /* tile the portion with the pattern */ 
	   while (1)
	   {
	     pat_rect.x = x_pat;
	     pat_rect.y = y_pat;
	     pat_rect.width = pattern_width;
	     pat_rect.height = pattern_height;

	     painthit_portion_rect.x =  x + x_portion;
	     painthit_portion_rect.y =  y + y_portion;
	     painthit_portion_rect.width = w_portion;
	     painthit_portion_rect.height = h_portion;
	     
	     gdk_rectangle_intersect (&pat_rect, &painthit_portion_rect, &r);

	     pixelarea_init ( &painthit_portion_area, painthit, NULL, 
				r.x - painthit_portion_rect.x, 
				r.y - painthit_portion_rect.y, 
				r.width, r.height, TRUE);	 
	     pixelarea_init ( &pat_area, pattern->mask_canvas, NULL, 
				r.x - pat_rect.x, r.y - pat_rect.y, 
				r.width, r.height, FALSE);	 
	     
	     copy_area (&pat_area, &painthit_portion_area);
	     
             x_pat += pattern_width;
	     if (x_pat >= (x + x_portion) + w_portion)
	     {
		x_pat = x_pat_start;
		y_pat += pattern_height;
		if ( y_pat >= (y + y_portion) + h_portion )
		  break;
	     }
	   }
        }
      rc = TRUE;
    }

  return rc;
}


static void 
clone_motion  (
               PaintCore16 * paint_core,
               GimpDrawable * drawable,
               GimpDrawable * src_drawable,
               CloneType type,
               int offset_x,
               int offset_y
               )
{
  Canvas * painthit = paint_core_16_area (paint_core, drawable);
  int rc = FALSE;

  if (painthit)
    {
      switch (type)
        {
        case ImageClone:
          {
            rc = clone_motion_image (paint_core, painthit,
                                     drawable, src_drawable,
                                     paint_core->x + offset_x,
                                     paint_core->y + offset_y);
          }
          break;
        case PatternClone:
          {
            GPatternP pattern;
            if ((pattern = get_active_pattern ()))
              rc = clone_motion_pattern (paint_core, painthit,
                                         drawable, pattern,
                                         paint_core->x + offset_x,
                                         paint_core->y + offset_y);
          }
          break;
        }
      
      if (rc == TRUE)
        paint_core_16_area_paste (paint_core, drawable,
                                  1.0, (gfloat) get_brush_opacity (),
                                  SOFT, CONSTANT,
                                  get_brush_paint_mode ());
    }
}


static void
clone_line_pattern (GImage        *dest,
		    GimpDrawable  *drawable,
		    GPatternP      pattern,
		    unsigned char *d,
		    int            x,
		    int            y,
		    int            bytes,
		    int            width)
{
#if 0
  unsigned char *pat, *p;
  int color, alpha;
  int i;

  /*  Make sure x, y are positive  */
  while (x < 0)
    x += pattern->mask->width;
  while (y < 0)
    y += pattern->mask->height;

  /*  Get a pointer to the appropriate scanline of the pattern buffer  */
  pat = temp_buf_data (pattern->mask) +
    (y % pattern->mask->height) * pattern->mask->width * pattern->mask->bytes;
  color = (pattern->mask->bytes == 3) ? RGB : GRAY;

  alpha = bytes - 1;

  for (i = 0; i < width; i++)
    {
      p = pat + ((i + x) % pattern->mask->width) * pattern->mask->bytes;

      gimage_transform_color (dest, drawable, p, d, color);

      d[alpha] = OPAQUE_OPACITY;

      d += bytes;
    }
#endif
}

static void *
clone_non_gui_paint_func (PaintCore16 *paint_core,
			  GimpDrawable *drawable,
			  int        state)
{
  clone_motion (paint_core, drawable, non_gui_src_drawable,
		non_gui_type, non_gui_offset_x, non_gui_offset_y);

  return NULL;
}


/*  The clone procedure definition  */
ProcArg clone_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_DRAWABLE,
    "src_drawable",
    "the source drawable"
  },
  { PDB_INT32,
    "clone_type",
    "the type of clone: { IMAGE-CLONE (0), PATTERN-CLONE (1) }"
  },
  { PDB_FLOAT,
    "src_x",
    "the x coordinate in the source image"
  },
  { PDB_FLOAT,
    "src_y",
    "the y coordinate in the source image"
  },
  { PDB_INT32,
    "num_strokes",
    "number of stroke control points (count each coordinate as 2 points)"
  },
  { PDB_FLOATARRAY,
    "strokes",
    "array of stroke coordinates: {s1.x, s1.y, s2.x, s2.y, ..., sn.x, sn.y}"
  }
};


ProcRecord clone_proc =
{
  "gimp_clone",
  "Clone from the source to the dest drawable using the current brush",
  "This tool clones (copies) from the source drawable starting at the specified source coordinates to the dest drawable.  If the \"clone_type\" argument is set to PATTERN-CLONE, then the current pattern is used as the source and the \"src_drawable\" argument is ignored.  Pattern cloning assumes a tileable pattern and mods the sum of the src coordinates and subsequent stroke offsets with the width and height of the pattern.  For image cloning, if the sum of the src coordinates and subsequent stroke offsets exceeds the extents of the src drawable, then no paint is transferred.  The clone tool is capable of transforming between any image types including RGB->Indexed--although converting from any type to indexed is significantly slower.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  8,
  clone_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { clone_invoker } },
};


static Argument *
clone_invoker (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  GimpDrawable *src_drawable;
  double src_x, src_y;
  int num_strokes;
  double *stroke_array;
  CloneType int_value;
  int i;

  drawable = NULL;
  num_strokes = 0;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  the src drawable  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      src_drawable = drawable_get_ID (int_value);
      if (src_drawable == NULL || gimage != drawable_gimage (src_drawable))
	success = FALSE;
      else
	non_gui_src_drawable = src_drawable;
    }
  /*  the clone type  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      switch (int_value)
	{
	case 0: non_gui_type = ImageClone; break;
	case 1: non_gui_type = PatternClone; break;
	default: success = FALSE;
	}
    }
  /*  x, y offsets  */
  if (success)
    {
      src_x = args[4].value.pdb_float;
      src_y = args[5].value.pdb_float;
    }
  /*  num strokes  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      if (int_value > 0)
	num_strokes = int_value / 2;
      else
	success = FALSE;
    }

  /*  point array  */
  if (success)
    stroke_array = (double *) args[7].value.pdb_pointer;

  if (success)
    /*  init the paint core  */
    success = paint_core_16_init (&non_gui_paint_core_16, drawable,
                                  stroke_array[0], stroke_array[1]);

  if (success)
    {
      /*  set the paint core's paint func  */
      non_gui_paint_core_16.paint_func = clone_non_gui_paint_func;

      non_gui_paint_core_16.startx = non_gui_paint_core_16.lastx = stroke_array[0];
      non_gui_paint_core_16.starty = non_gui_paint_core_16.lasty = stroke_array[1];

      non_gui_offset_x = (int) (src_x - non_gui_paint_core_16.startx);
      non_gui_offset_y = (int) (src_y - non_gui_paint_core_16.starty);

      if (num_strokes == 1)
	clone_non_gui_paint_func (&non_gui_paint_core_16, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core_16.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core_16.cury = stroke_array[i * 2 + 1];

	  paint_core_16_interpolate (&non_gui_paint_core_16, drawable);

	  non_gui_paint_core_16.lastx = non_gui_paint_core_16.curx;
	  non_gui_paint_core_16.lasty = non_gui_paint_core_16.cury;
	}

      /*  finish the painting  */
      paint_core_16_finish (&non_gui_paint_core_16, drawable, -1);

      /*  cleanup  */
      paint_core_16_cleanup ();
    }

  return procedural_db_return_args (&clone_proc, success);
}
