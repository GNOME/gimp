/* gap_match.c
 * 1998.10.14 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains:
 * layername and layerstacknumber matching procedures
 *
 */
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* revision history:
 * version 0.97.00  1998.10.14  hof: - created module 
 */
#include "config.h"
 
/* SYSTEM (UNIX) includes */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/* GIMP includes */
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_match.h"

int
p_is_empty (char *str)
{
  if(str == NULL)  return(TRUE);
  if(*str == '\0') return(TRUE);
  
  while(*str != '\0')
  {
    if(*str != ' ') return(FALSE);
    str++;
  }
  
  return(TRUE);
}

/* ============================================================================
 * p_substitute_framenr
 *    copy new_layername to buffer
 *    and substitute [####] by curr frame number
 * ============================================================================
 */
void
p_substitute_framenr (char *buffer, int buff_len, char *new_layername, long curr)
{
  int l_idx;
  int l_digits;
  int l_cpy;
  char  l_fmt_str[21];
  
  l_fmt_str[0] = '%';
  l_fmt_str[1] = '0';
  l_digits = 0;

  l_idx = 0;
  if(new_layername != NULL)
  {
    while((l_idx < (buff_len-1)) && (*new_layername != '\0') )
    {
      l_cpy    = 1;
      switch(*new_layername)
      {
        case '[':
	  if((new_layername[1] == '#') && (l_digits == 0))
	  {
	    l_cpy    = 0;
	    l_digits = 1;
	  }
	  break;
        case '#':
	  if(l_digits > 0)
	  {
	    l_cpy    = 0;
	    l_digits++;
	  }
	  break;
        case ']':
	  if(l_digits > 0)
	  {
	    l_digits--;
	    sprintf(&l_fmt_str[2], "%dd", l_digits);
	    sprintf(&buffer[l_idx], l_fmt_str, (int)curr);
	    l_idx += l_digits; 
            l_digits = 0;
	    l_cpy    = 0;
	  }
	  break;
	default:
	  l_digits = 0;
	  break;
      }
      if(l_cpy != 0)
      {
        buffer[l_idx] = (*new_layername);
        l_idx++;
      }
     
      new_layername++;

    }
  }
  
  buffer[l_idx] = '\0';
}	/* end p_substitute_framenr */


void str_toupper(char *str)
{
  if(str != NULL)
  {
     while(*str != '\0')
     {
       *str = toupper(*str);
       str++;
     }
  }
}
  

/* match layer_idx (int) with pattern
 * pattern contains a list like that:
 *  "0, 3-4, 7, 10"
 */
int p_match_number(gint32 layer_idx, char *pattern)
{
   char    l_digit_buff[128];
   char   *l_ptr;
   int     l_idx;
   gint32  l_num, l_range_start;
   
   l_idx = 0;
   l_num = -1; l_range_start = -1;
   for(l_ptr = pattern; 1 == 1; l_ptr++)
   {
      if(isdigit(*l_ptr))
      {
         l_digit_buff[l_idx] = *l_ptr;   /* collect digits here */
         l_idx++;
      }
      else
      {
         if(l_idx > 0)
         {
            /* now we are one character past a number */
            l_digit_buff[l_idx] = '\0';
            l_num = atol(l_digit_buff);  /* scann the number */
            
            if(l_num == layer_idx)
            {
               return(TRUE);             /* matches number exactly */
            }
            
            if((l_range_start >= 0)
            && (layer_idx >= l_range_start) && (layer_idx <= l_num ))
            {
               return(TRUE);             /* matches given range */
            }

            l_range_start = -1;     /* disable number for opening a range */
            l_idx = 0;
          }

          switch(*l_ptr)
          {
            case '\0':
               return (FALSE);
               break;
            case ' ':
            case '\t':
               break;
            case ',':
               l_num = -1;              /* disable number for opening a range */
               break;
            case '-':
                 if (l_num >= 0)
                 {
                    l_range_start = l_num;  /* prev. number opens a range */
                 }
                 break;
            default:
               /* found illegal characters */
               /* return (FALSE); */  
               l_num = -1;             /* disable number for opening a range */
               l_range_start = -1;     /* disable number for opening a range */
               
               break;
          }
      }
   }   /* end for */

   return(FALSE);
}	/* end p_match_number */
  

/* simple stringmatching without wildcards */
int p_match_name(char *layername, char *pattern, gint32 mode, gint32 case_sensitive)
{
   int l_idx;
   int l_llen;
   int l_plen;
   char *l_name_ptr;
   char *l_patt_ptr;
   char  l_name_buff[256];
   char  l_patt_buff[256];

   if(pattern == NULL)   return (FALSE);
   if(layername == NULL) return (FALSE);
 
   if(case_sensitive)
   {
     /* case sensitive can compare on the originals */
     l_name_ptr = layername;
     l_patt_ptr = pattern;
   }
   else
   {
     /* ignore case by converting everything to UPPER before comare */
     strcpy (l_name_buff, layername);
     strcpy (l_patt_buff, pattern);
 
     str_toupper (l_name_buff);
     str_toupper (l_patt_buff);
      
     l_name_ptr = l_name_buff;
     l_patt_ptr = l_patt_buff;
   }

   switch (mode)
   {
      case MTCH_EQUAL:
           if (0 == strcmp(l_name_ptr, l_patt_ptr))
           {
             return(TRUE);
           }
           break;
      case MTCH_START:
           l_plen = strlen(l_patt_ptr);
           if (0 == strncmp(l_name_ptr, l_patt_ptr, l_plen))
           {
             return(TRUE);
           }
           break;
      case MTCH_END:
           l_llen = strlen(l_name_ptr);
           l_plen = strlen(l_patt_ptr);
           if(l_llen > l_plen)
           {
             if(0 == strncmp(&l_name_ptr[l_llen - l_plen], l_patt_ptr, l_plen))
             {
               return(TRUE);
             }
           }
           break;
      case MTCH_ANYWHERE:
           l_llen = strlen(l_name_ptr);
           l_plen = strlen(l_patt_ptr);
           for(l_idx = 0; l_idx <= (l_llen - l_plen); l_idx++)
           {
              if (strncmp(&l_name_ptr[l_idx], l_patt_ptr, l_plen) == 0)
              {
                 return (TRUE);
              }
           }
           break;
      default:
           break;
   
   }

   return (FALSE);

}

int p_match_layer(gint32 layer_idx, char *layername, char *pattern,
                  gint32 mode, gint32 case_sensitive, gint32 invert,
                  gint nlayers, gint32 layer_id)
{
   int l_rc;
   switch(mode)
   {
     case MTCH_NUMBERLIST:
          l_rc = p_match_number(layer_idx, pattern);
          break;
     case MTCH_INV_NUMBERLIST:
          l_rc = p_match_number((nlayers -1) - layer_idx, pattern);
          break;
     case MTCH_ALL_VISIBLE:
          l_rc = gimp_layer_get_visible(layer_id);
          break;
     default:
          l_rc = p_match_name(layername, pattern, mode, case_sensitive);
          break;
   }
   
   if(invert == TRUE)
   {
      if(l_rc == FALSE)  { return(TRUE); }
      return(FALSE);
   }
   return (l_rc);
}
