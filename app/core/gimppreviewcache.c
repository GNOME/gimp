/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Andy Thomas alt@gimp.org
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "gimpdrawableP.h"
#include "gimage.h"
#include "temp_buf.h"
#include "gimppreviewcache.h"

static gint 
preview_cache_compare(gconstpointer  a,
		      gconstpointer  b)
{
  PreviewCache *pc1 = (PreviewCache *)a;
  PreviewCache *pc2 = (PreviewCache *)b;

  if(pc1->width > pc2->width && pc1->height > pc2->height)
    return -1;

  return 1;
}

static void
preview_cache_find_exact(gpointer data, gpointer udata)
{
  PreviewCache *pc = (PreviewCache *)data;
  PreviewNearest *pNearest = (PreviewNearest *)udata;

/*   printf("this value w,h [%d,%d]\n",pc->width,pc->height); */

/*   if(pNearest->pc) */
/*       printf("current nearest value w,h [%d,%d]\n",pNearest->pc->width,pNearest->pc->height); */

  if(pNearest->pc)
    return;

  if(pc->width == pNearest->width &&
     pc->height == pNearest->height)
    {
      /* Ok we could make the preview out of this one...
       * If we already have it are these bigger dimensions? 
       */
      pNearest->pc = pc;
      return;
    }
}

static void
preview_cache_find_biggest(gpointer data, gpointer udata)
{
  PreviewCache *pc = (PreviewCache *)data;
  PreviewNearest *pNearest = (PreviewNearest *)udata;

/*   printf("this value w,h [%d,%d]\n",pc->width,pc->height); */

/*   if(pNearest->pc) */
/*       printf("current nearest value w,h [%d,%d]\n",pNearest->pc->width,pNearest->pc->height); */
  
  if(pc->width >= pNearest->width &&
     pc->height >= pNearest->height)
    {
      /* Ok we could make the preview out of this one...
       * If we already have it are these bigger dimensions? 
       */
      if(pNearest->pc)
	{
	  if(pNearest->pc->width > pc->width &&
	     pNearest->pc->height > pc->height)
	    return;
	}
      pNearest->pc = pc;
    }
}

static void
preview_cache_remove_smallest(GSList **plist)
{
  GSList *cur = *plist;
  PreviewCache *smallest = NULL;
  
/*   printf("Removing smallest\n"); */

  if(!cur)
    return;

  do
    {
      if(!smallest)
	{
	  smallest = cur->data;
/* 	  printf("init smallest  %d,%d\n",smallest->width,smallest->height); */
	}
      else
	{
	  PreviewCache *pcthis = cur->data;
/* 	  printf("Checking %d,%d\n",pcthis->width,pcthis->height); */
	  if((smallest->height*smallest->width) >=
	     (pcthis->height*pcthis->width))
	    {
	      smallest = pcthis;
/* 	      printf("smallest now  %d,%d\n",smallest->width,smallest->height); */
	    }
	}
    } while((cur = g_slist_next(cur)));

  *plist = g_slist_remove(*plist,smallest);
/*   printf("removed %d,%d\n",smallest->width,smallest->height); */
/*   printf("removed smallest\n"); */
}

static void
preview_cache_invalidate(gpointer data, gpointer udata)
{
  PreviewCache *pc = (PreviewCache *)data;

  temp_buf_free (pc->preview);
  g_free(pc);
}

static void
preview_cache_print(gpointer data, gpointer udata)
{
/*   PreviewCache *pc = (PreviewCache *)data; */
  if(!data)
    {
/*       printf("\tNo Cache\n"); */
      return;
    }
/*   printf("\tvalue w,h [%d,%d] => %p\n",pc->width,pc->height,pc->preview); */
}

void
gimp_preview_cache_invalidate(GSList **plist)
{
/*   printf("gimp_preview_cache_invalidate\n"); */
  g_slist_foreach(*plist,preview_cache_print,NULL);

  g_slist_foreach(*plist,preview_cache_invalidate,NULL);
  *plist = NULL;
}

void 
gimp_preview_cache_add(GSList **plist,
		       TempBuf *buf)
{
  PreviewCache *pc;

/*   printf("gimp_preview_cache_add %d %d\n",buf->width,buf->height); */
  g_slist_foreach(*plist,preview_cache_print,NULL);

  if(g_slist_length(*plist) > MAX_CACHE_PREVIEWS)
    {
      /* Remove the smallest */
      preview_cache_remove_smallest(plist);
    }

  pc = g_new0(PreviewCache,1);
  pc->preview = buf; 
  pc->width = buf->width;
  pc->height = buf->height;
  *plist = g_slist_insert_sorted(*plist,pc,preview_cache_compare);
}

TempBuf *
gimp_preview_cache_get(GSList **plist,
		       gint width,
		       gint height)
{
  PreviewNearest pn;
  PreviewCache *pc;

/*   printf("gimp_preview_cache_get %d %d\n",width,height); */
  g_slist_foreach(*plist,preview_cache_print,NULL);

  pn.pc = NULL;
  pn.width = width;
  pn.height = height;

  g_slist_foreach(*plist,preview_cache_find_exact,&pn);

  if(pn.pc && pn.pc->preview)
    {
/*       printf("extact value w,h [%d,%d] => %p\n",pn.pc->width,pn.pc->height,pn.pc->preview);  */
      return pn.pc->preview;
    }

  g_slist_foreach(*plist,preview_cache_find_biggest,&pn);

  if(pn.pc)
    {
      gint pwidth;
      gint pheight;
      gdouble x_ratio;
      gdouble y_ratio;
      guchar *src_data;
      guchar *dest_data;
      gint loop1,loop2;

/*       printf("nearest value w,h [%d,%d] => %p\n",pn.pc->width,pn.pc->height,pn.pc->preview); */

/*       if(pn.pc->width == width && */
/* 	 pn.pc->height == height) */
/* 	return pn.pc->preview; */

      if(!pn.pc->preview)
	{
	  g_error("gimp_preview_cache_get:: Invalid cache item");
	  return NULL;
	}

      /* Make up new preview from the large one... */

      pwidth = pn.pc->preview->width;
      pheight = pn.pc->preview->height;

      /* Now get the real one and add to cache */
/*       printf("Must create from large preview\n"); */
      pc = g_new0(PreviewCache,1);
      pc->preview = temp_buf_new(width,height,pn.pc->preview->bytes,0,0,NULL);
      /* preview from nearest bigger one */

      if (width)
        x_ratio = (gdouble)pwidth/(gdouble)width;
      else
        x_ratio = 0.0;
      if (height)
        y_ratio = (gdouble)pheight/(gdouble)height;
      else
        y_ratio = 0.0;
      src_data = temp_buf_data(pn.pc->preview);
      dest_data = temp_buf_data(pc->preview);

/*       printf("x_ratio , y_ratio [%f,%f]\n",x_ratio,y_ratio); */

      for(loop1 = 0 ; loop1 < height ; loop1++)
	for(loop2 = 0 ; loop2 < width ; loop2++)
	  {
	    int i;
	    guchar *src_pixel = src_data +
	      ((gint)(loop2*x_ratio))*pn.pc->preview->bytes +
	      ((gint)(loop1*y_ratio))*pwidth*pn.pc->preview->bytes;
	    guchar *dest_pixel = dest_data + 
	      (loop2+loop1*width)*pn.pc->preview->bytes;

	    for(i = 0 ; i < pn.pc->preview->bytes; i++)
	      *dest_pixel++ = *src_pixel++;
	  }

      pc->width = width;
      pc->height = height;
      *plist = g_slist_insert_sorted(*plist,pc,preview_cache_compare); 
/*       printf("New preview created [%d,%d] => %p\n",pc->width,pc->height,pc->preview);  */

      return pc->preview;
    }

/*   printf("gimp_preview_cache_get returning NULL\n");  */
  return NULL;
}
