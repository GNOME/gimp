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

#include "tag.h"


/* fields within a Tag */

#define MASK_PRECISION  0xff
#define SHIFT_PRECISION 24

#define MASK_FORMAT     0xff
#define SHIFT_FORMAT    16

#define MASK_ALPHA      0xff
#define SHIFT_ALPHA     8


Tag
tag_new (
         Precision  p,
         Format     f,
         Alpha      a
         )
{
  return (
          ((p & MASK_PRECISION) << SHIFT_PRECISION) |
          ((f & MASK_FORMAT) << SHIFT_FORMAT) |
          ((a & MASK_ALPHA) << SHIFT_ALPHA)
          );
    
}


Precision
tag_precision (
               Tag x
               )
{
  return ((x >> SHIFT_PRECISION) & MASK_PRECISION);
}


Format
tag_format (
            Tag x
            )
{
  return ((x >> SHIFT_FORMAT) & MASK_FORMAT);
}


Alpha
tag_alpha (
           Tag x
           )
{
  return ((x >> SHIFT_ALPHA) & MASK_ALPHA);
}


Tag
tag_set_precision (
                   Tag y,
                   Precision x
                   )
{
  switch (x)
    {
    case PRECISION_NONE:
    case PRECISION_U8:
    case PRECISION_U16:
    case PRECISION_FLOAT:
      break;
    default:
      x = PRECISION_NONE;
      break;
    }
  y &= ~(MASK_PRECISION << SHIFT_PRECISION);
  y |= ((x & MASK_PRECISION) << SHIFT_PRECISION);
  return y;
}


Tag
tag_set_format (
                Tag y,
                Format x
                )
{
  switch (x)
    {
    case FORMAT_NONE:
    case FORMAT_RGB:
    case FORMAT_GRAY:
    case FORMAT_INDEXED:
      break;
    default:
      x = FORMAT_NONE;
      break;
    }
  y &= ~(MASK_FORMAT << SHIFT_FORMAT);
  y |= ((x & MASK_FORMAT) << SHIFT_FORMAT);
  return y;
}


Tag
tag_set_alpha (
               Tag y,
               Alpha x
               )
{
  switch (x)
    {
    case ALPHA_NONE:
    case ALPHA_NO:
    case ALPHA_YES:
      break;
    default:
      x = ALPHA_NONE;
      break;
    }
  y &= ~(MASK_ALPHA << SHIFT_ALPHA);
  y |= ((x & MASK_ALPHA) << SHIFT_ALPHA);
  return y;
}


guint
tag_num_channels (
           Tag x
           )
{
  int y;
  
  switch (tag_format (x))
    {
    case FORMAT_RGB:
      y = 3;
      break;
    case FORMAT_GRAY:
    case FORMAT_INDEXED:
      y = 1;
      break;
    case FORMAT_NONE:
    default:
      return 0;
    }

  switch (tag_alpha(x))
    {
    case ALPHA_YES:
      y++;
      break;
    case ALPHA_NO:
      break;
    case ALPHA_NONE:
    default:
      return 0;
    }

  return y;
}

guint
tag_bytes (
           Tag x
           )
{
  int y;
  
  y = tag_num_channels (x);
  switch (tag_precision (x))
    {
    case PRECISION_U8:
      y *= sizeof(guint8);
      break;
    case PRECISION_U16:
      y *= sizeof(guint16);
      break;
    case PRECISION_FLOAT:
      y *= sizeof(gfloat);
      break;
    case PRECISION_NONE:
      return 0;
    }
  
  return y;
}


guint
tag_equal (
           Tag  a,
           Tag  b
           )
{
  if ((tag_format (a) == tag_format (b)) &&
      (tag_precision (a) == tag_precision (b)) &&
      (tag_alpha (a) == tag_alpha (b)))
    return TRUE;
  return FALSE;
}


guchar *
tag_string_precision (
                      Precision x
                      )
{
  switch (x)
    {
    case PRECISION_NONE:
      return "<uninit precision>";
    case PRECISION_U8:
      return "8 bit";
    case PRECISION_U16:
      return "16 bit";
    case PRECISION_FLOAT:
      return "floating point";
    }
  return "<invalid precision!>";
}


guchar *
tag_string_format (
                   Format x
                   )
{
  switch (x)
    {
    case FORMAT_NONE:
      return "<uninit format>";
    case FORMAT_RGB:
      return "RGB";
    case FORMAT_GRAY:
      return "grayscale";
    case FORMAT_INDEXED:
      return "indexed";
    }
  return "<invalid format!>";
}


guchar *
tag_string_alpha (
                  Alpha x
                  )
{
  switch (x)
    {
    case ALPHA_NONE:
      return "<uninit alpha>";
    case ALPHA_NO:
      return "noalpha";
    case ALPHA_YES:
      return "alpha";
    }
  return "<invalid alpha!>";
}










/* hack hack */
Tag 
tag_by_bytes  (
               guint bytes
               )
{
  switch (bytes)
    {
    case 1:
      return tag_new (PRECISION_U8, FORMAT_GRAY, ALPHA_NO);
      break;
    case 2:
      return tag_new (PRECISION_U8, FORMAT_GRAY, ALPHA_YES);
      break;
    case 3:
      return tag_new (PRECISION_U8, FORMAT_RGB, ALPHA_NO);
      break;
    case 4:
      return tag_new (PRECISION_U8, FORMAT_RGB, ALPHA_YES);
      break;
    }
  return tag_new (PRECISION_NONE, FORMAT_NONE, ALPHA_NONE);
}
