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

#include <stdarg.h>

#include "canvas.h"
#include "pixelrow.h"
#include "pixelarea.h"


typedef struct _PixelAreaGroup PixelAreaGroup;

struct _PixelAreaGroup
{
  /* the areas we iterate over */
  GList * pixelareas;

  /* should we autoconfigure the areas */
  int     autoref;

  /* the size of the intersection of all areas */
  int     region_width;
  int     region_height;

  /* the current chunk */
  BoundBox chunk;
};


static PixelAreaGroup * new_group      (int);
static void             del_group      (PixelAreaGroup *);
static void             add_area       (PixelAreaGroup *, PixelArea *);
static PixelAreaGroup * next_chunk     (PixelAreaGroup *);
static void *           configure      (PixelAreaGroup *);
static void             unconfigure    (PixelAreaGroup *, void *);




void 
pixelarea_init  (
                 PixelArea * pa,
                 Canvas * c,
                 int x,
                 int y,
                 int w,
                 int h,
                 int will_dirty
                 )
{
  pixelarea_init2 (pa, c,
                   x, y,
                   w, h,
                   ((will_dirty == TRUE) ? REFTYPE_WRITE : REFTYPE_READ),
                   EDGETYPE_NEXT);
}


void 
pixelarea_init2  (
                  PixelArea * pa,
                  Canvas * c,
                  guint x,
                  guint y,
                  guint w,
                  guint h,
                  RefType r,
                  EdgeType e
                  )
{
  g_return_if_fail (pa != NULL);
  g_return_if_fail (c != NULL);

  pa->canvas = c;

  /* handle the shorthand for width and height */
  if (w == 0) w = canvas_width (c);
  if (h == 0) h = canvas_height (c);
      
  /* clip the area to the canvas bounds */
  pa->area.x2 = CLAMP (x+w, 0, canvas_width (c));
  pa->area.y2 = CLAMP (y+h, 0, canvas_height (c));
  pa->area.x1 = CLAMP (x, 0, canvas_width (c));
  pa->area.y1 = CLAMP (y, 0, canvas_height (c));

  /* start the chunk at the top of the area, with zero size */
  pa->chunk.x1 = pa->area.x1;
  pa->chunk.y1 = pa->area.y1;
  pa->chunk.x2 = pa->area.x1;
  pa->chunk.y2 = pa->area.y1;

  /* remember how to ref the canvas portions */
  switch (r)
    {
    case REFTYPE_WRITE:
    case REFTYPE_READ:
    case REFTYPE_NONE:
      pa->reftype = r;
      break;
    default:
      g_warning ("bad ref type, setting to REFTYPE_WRITE");
      pa->reftype = REFTYPE_WRITE;
      break;
    }

  /* remember how to handle edges */
  switch (e)
    {
    case EDGETYPE_NEXT:
    case EDGETYPE_WRAP:
      pa->edgetype = e;
      break;

    case EDGETYPE_NONE:
    default:
      g_warning ("bad edge type, setting to EDGETYPE_NEXT");
      pa->edgetype = EDGETYPE_NEXT;
      break;
    }

  pa->tag = canvas_tag (c);
  pa->data = NULL;
  pa->rowstride = 0;
  pa->is_reffed = 0;
}

void 
pixelarea_getdata  (
                    PixelArea * pa,
                    PixelRow * pr,
                    int row
                    )
{
  if (pa && pa->is_reffed && (row >= 0) &&
      (row < (pa->chunk.y2 - pa->chunk.y1)))
    {
      pixelrow_init (pr,
                     pa->tag,
                     pa->data + row * pa->rowstride,
                     pa->chunk.x2 - pa->chunk.x1);
    }
  else
    {
      pixelrow_init (pr,
                     tag_null (),
                     NULL,
                     0);
    }
}


#define FIXME /* this belongs elsewhere */
void
pixelarea_copy_row ( 
		    PixelArea *pa,
		    PixelRow *pr,
		    int x,
		    int y,
                    int width, 
                    int subsample
		   )
{
  guchar * pr_data = pixelrow_data (pr);
  int bytes = tag_bytes (pixelarea_tag (pa));
  int b, remainder;
  PixelArea area;
  void *pag;
  
  /* get a pixel area of height 1 for the wanted row. */ 
  pixelarea_init (&area, pa->canvas, x, y ,width, 1, FALSE);  

  remainder = 0;

  for (pag = pixelarea_register (1, &area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      int cur = remainder;
      guchar * area_data = pixelarea_data (&area) + cur * bytes;
      int portion_width = pixelarea_width (&area);

      while (cur < portion_width)
        {
          for (b = 0; b < bytes; b++)
            pr_data[b] = area_data[b];

          cur ++;
          pr_data += bytes;
          area_data += bytes;
        }

      remainder = cur - portion_width;
    }
}


#define FIXME /* this belongs elsewhere */
void
pixelarea_copy_col ( 
		    PixelArea *pa,
		    PixelRow *col,
		    int x,
		    int y,
                    int height, 
                    int subsample
		   )
{
  guchar *col_data = pixelrow_data (col);
  void *pag;
  PixelArea area;
  Tag pa_tag = pixelarea_tag (pa);  
  int bytes = tag_bytes (pa_tag);
  int b;
  
  /* get a pixel area of width 1 for the wanted column. */ 
  pixelarea_init (&area, pa->canvas, x, y , 1, height, FALSE);  
  
  for (pag = pixelarea_register (1, &area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      guchar *area_data = pixelarea_data (&area);
      int portion_height = pixelarea_height (&area); 
      int rowstride = pixelarea_rowstride (&area); 
      while (portion_height --)
      { 
        b = bytes;
        while ( b --)
	  *col_data++ = *area_data++;
        area_data += rowstride - bytes;          	
      } 
    }
}


#define FIXME /* this belongs elsewhere */
void
pixelarea_write_row ( 
		    PixelArea *pa,
		    PixelRow *pr,
		    int x,
		    int y,
                    int width 
		   )
{
  guchar *pr_data = pixelrow_data (pr);
  void *pag;
  PixelArea area;
  Tag pa_tag = pixelarea_tag (pa);  
  int bytes = tag_bytes (pa_tag);
  
  /* check to see if we are off the end of canvas */ 
  if ( x + width > canvas_width (pa->canvas) ) 
    width = canvas_width (pa->canvas)  - x;  
  
  /* get a pixel area of height 1 for the row. */ 
  pixelarea_init (&area, pa->canvas, x, y ,width, 1, TRUE);  
  
  for (pag = pixelarea_register (1, &area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      guchar *area_data = pixelarea_data (&area);
      int portion_width = pixelarea_width (&area);  
      memcpy(area_data, pr_data, bytes * portion_width);
      pr_data += portion_width * bytes;
    }
}


#define FIXME /* this belongs elsewhere */
void
pixelarea_write_col ( 
		    PixelArea *pa,
		    PixelRow *col,
		    int x,
		    int y,
                    int height 
		   )
{
  guchar *col_data = pixelrow_data (col);
  void *pag;
  PixelArea area;
  Tag pa_tag = pixelarea_tag (pa);
  int bytes = tag_bytes (pa_tag);
  int b;
  
  /* check to see if we are off the end of canvas */ 
  if ( y + height > canvas_height (pa->canvas) ) 
    height = canvas_height (pa->canvas)  - y;  
  
  /* get a pixel area of width 1 for the wanted column.*/ 
  pixelarea_init (&area, pa->canvas, x, y , 1, height, FALSE);  
  
  for (pag = pixelarea_register (1, &area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      guchar * area_data= pixelarea_data (&area);
      int portion_height = pixelarea_height (&area); 
      int rowstride = pixelarea_rowstride (&area); 
      while (portion_height --)
      { 
        b = bytes;
        while ( b --)
	  *area_data++ = *col_data++;
        area_data += rowstride - bytes;
      } 
    }
}


Tag 
pixelarea_tag  (
                PixelArea * pa
                )
{
  if (pa == NULL)
    return tag_null ();
  return pa->tag;
}


int 
pixelarea_areawidth  (
                      PixelArea * pa
                      )
{
  g_return_val_if_fail (pa != NULL, 0);
  return pa->area.x2 - pa->area.x1;
}


int 
pixelarea_areaheight  (
                       PixelArea * pa
                       )
{
  g_return_val_if_fail (pa != NULL, 0);
  return pa->area.y2 - pa->area.y1;
}


int 
pixelarea_x  (
              PixelArea * pa
              )
{
  g_return_val_if_fail (pa != NULL, 0);
  return pa->chunk.x1;
}


int 
pixelarea_y  (
              PixelArea * pa
              )
{
  g_return_val_if_fail (pa != NULL, 0);
  return pa->chunk.y1;
}


int 
pixelarea_width  (
                  PixelArea * pa
                  )
{
  g_return_val_if_fail (pa != NULL, 0);
  return pa->chunk.x2 - pa->chunk.x1;
}


int 
pixelarea_height  (
                   PixelArea * pa
                   )
{
  g_return_val_if_fail (pa != NULL, 0);
  return pa->chunk.y2 - pa->chunk.y1;
}


guchar * 
pixelarea_data (
                PixelArea * pa
                )
{
  g_return_val_if_fail (pa != NULL, NULL);
  return pa->data;
}


int 
pixelarea_rowstride (
                     PixelArea * pa
                     )
{
  g_return_val_if_fail (pa != NULL, 0);
  return pa->rowstride;
}


guint
pixelarea_ref (
               PixelArea * pa
               )
{
  RefRC rc = REFRC_FAIL;
  
  g_return_val_if_fail (pa != NULL, FALSE);
  g_return_val_if_fail (pa->is_reffed == 0, TRUE);
  
  switch (pa->reftype)
    {
    case REFTYPE_WRITE:
      rc = canvas_portion_refrw (pa->canvas,
                                 pa->chunk.x1, pa->chunk.y1);
      break;
      
    case REFTYPE_READ:
      rc = canvas_portion_refro (pa->canvas,
                                 pa->chunk.x1, pa->chunk.y1);
      break;
      
    case REFTYPE_NONE:
      rc = REFRC_OK;
      break;
    }

  if (rc != REFRC_OK)
    {
      return FALSE;
    }

  pa->data = canvas_portion_data (pa->canvas,
                                  pa->chunk.x1, pa->chunk.y1);
  pa->rowstride = canvas_portion_rowstride (pa->canvas,
                                            pa->chunk.x1, pa->chunk.y1);;
  pa->is_reffed = 1;
  
  return TRUE;
}


guint
pixelarea_unref (
                 PixelArea * pa                 
                 )
{
  RefRC rc = REFRC_FAIL;
  
  g_return_val_if_fail (pa != NULL, FALSE);
  g_return_val_if_fail (pa->is_reffed == 1, TRUE);
  
  switch (pa->reftype)
    {
    case REFTYPE_WRITE:
    case REFTYPE_READ:
      rc = canvas_portion_unref (pa->canvas,
                                 pa->chunk.x1, pa->chunk.y1);
      break;
      
     case REFTYPE_NONE:
      rc = REFRC_OK;
      break;
    }

  pa->data = NULL;
  pa->rowstride = 0;
  pa->is_reffed = 0;
  
  if (rc != REFRC_OK)
    return FALSE;
  
  return TRUE;
}



void *
pixelarea_register (
                    int num_areas,
                    ...
                    )
{
  PixelAreaGroup *pag;
  va_list ap;

  g_return_val_if_fail ((num_areas > 0), NULL);
  
  pag = new_group (TRUE);

  va_start (ap, num_areas);
  while (num_areas --)
    {
      PixelArea * pa = va_arg (ap, PixelArea *);      
      add_area (pag, pa);
    }
  va_end (ap);

  return configure (next_chunk (pag));
}

#define FIXME /* clean this up */
void * 
pixelarea_register_noref  (
                           int num_areas,
                           ...
                           )
{
  PixelAreaGroup *pag;
  va_list ap;

  g_return_val_if_fail ((num_areas > 0), NULL);
  
  pag = new_group (FALSE);

  va_start (ap, num_areas);
  while (num_areas --)
    {
      PixelArea * pa = va_arg (ap, PixelArea *);      
      add_area (pag, pa);
    }
  va_end (ap);

  return configure (next_chunk (pag));
}


void *
pixelarea_process  (
                    void * x
                    )
{
  PixelAreaGroup * pag = (PixelAreaGroup *) x;
  
  g_return_val_if_fail (pag != NULL, NULL);
  
  unconfigure (pag, NULL);

  return configure (next_chunk (pag));
}


void 
pixelarea_process_stop (
                        void * x
                        )
{
  PixelAreaGroup * pag = (PixelAreaGroup *) x;

  g_return_if_fail (pag != NULL);

  unconfigure (pag, NULL);  

  del_group (pag);
}



/*-----------------------------------------------------------------------

                            PixelAreaGroup

  -----------------------------------------------------------------------*/
  
static PixelAreaGroup * 
new_group  (
            int autoref
            )
{
  PixelAreaGroup * pag;
  
  pag = (PixelAreaGroup *) g_malloc (sizeof (PixelAreaGroup));

  pag->pixelareas = NULL;

  pag->autoref = autoref;

  pag->region_width = G_MAXINT;
  pag->region_height = G_MAXINT;

  pag->chunk.x1 = 0;
  pag->chunk.y1 = 0;
  pag->chunk.x2 = 0;
  pag->chunk.y2 = 0;

  return pag;
}


/* the data items are pointers to PixelAreas, which are always
   located on the stack, so don;t free them... */
static void 
del_group  (
            PixelAreaGroup * pag
            )
{
  g_return_if_fail (pag != NULL);

  if (pag->pixelareas)
    g_list_free (pag->pixelareas);
  
  g_free (pag);
}


static void 
add_area  (
           PixelAreaGroup * pag,
           PixelArea * pa
           )
{
  g_return_if_fail (pag != NULL);

  if (pa)
    {
      GList * list;

      /* it's okay to register an area twice, but we only keep one */
      for (list = pag->pixelareas;
           list;
           list = g_list_next (list))
        {
          if ((PixelArea *) list->data == pa)
            return;
        }

      pag->pixelareas = g_list_append (pag->pixelareas, pa);

      if (pa->edgetype == EDGETYPE_NEXT)
        {
          pag->region_width = MIN (pag->region_width,
                                   pa->area.x2 - pa->area.x1);
          
          pag->region_height = MIN (pag->region_height,
                                    pa->area.y2 - pa->area.y1);
        }
    }
}


/* ref all areas, looping through chunks if required */
static void * 
configure  (
            PixelAreaGroup * pag
            )
{
  PixelArea * pa;
  GList * list;

 try_again:

  if (pag == NULL)
    return NULL;
  
  if (pag->autoref != TRUE)
    return (void *) pag;

  for (list = pag->pixelareas;
       list;
       list = g_list_next (list))
    {
      pa = (PixelArea *) list->data;

      if (pixelarea_ref (pa) == FALSE)
        {
          unconfigure (pag, list);
          pag = next_chunk (pag);
          goto try_again;
        }
    }
  
  return (void *) pag;
}


/* unref as many areas as requested */
static void 
unconfigure  (
              PixelAreaGroup * pag,
              void * listtail
              )
{
  GList * list;

  g_return_if_fail (pag != NULL);

  if (pag->autoref != TRUE)
    return;
      
  for (list = pag->pixelareas;
       list != listtail;
       list = g_list_next (list))
    {
      PixelArea * pa = (PixelArea *) list->data;
      if (pixelarea_unref (pa) != TRUE)
        g_warning ("failed to unref...");
    }
}



static PixelAreaGroup * 
next_chunk  (
             PixelAreaGroup * pag
             )
{
  GList * list;
  guint ww = G_MAXINT;
  guint hh = G_MAXINT;
  
  g_return_val_if_fail (pag != NULL, NULL);

  /* move all chunks to the right or to the next row */
  if (pag->chunk.x2 < pag->region_width)
    {
      pag->chunk.x1 = pag->chunk.x2;

      for (list = pag->pixelareas;
           list;
           list = g_list_next (list))
        {
          PixelArea * pa = (PixelArea *) list->data;
          
          if ((pa->edgetype == EDGETYPE_WRAP) &&
              (pa->chunk.x2 >= pa->area.x2))
            {
              pa->chunk.x2 = pa->area.x1;
            }

          pa->chunk.x1 = pa->chunk.x2;
        }
    }
  else
    {
      pag->chunk.x1 = 0;
      pag->chunk.x2 = 0;
      
      pag->chunk.y1 = pag->chunk.y2;      

      for (list = pag->pixelareas;
           list;
           list = g_list_next (list))
        {
          PixelArea * pa = (PixelArea *) list->data;

          pa->chunk.x1 = pa->area.x1;
          pa->chunk.x2 = pa->area.x1;

          if ((pa->edgetype == EDGETYPE_WRAP) &&
              (pa->chunk.y2 >= pa->area.y2))
            {
              pa->chunk.y2 = pa->area.y1;
            }

          pa->chunk.y1 = pa->chunk.y2;          
        }
    }

  /* determine width and height of next chunk */
  for (list = pag->pixelareas;
       list;
       list = g_list_next (list))
    {
      PixelArea * pa = (PixelArea *) list->data;
      
      guint w = CLAMP (canvas_portion_width (pa->canvas,
                                             pa->chunk.x1, pa->chunk.y1),
                       0,
                       pag->region_width - (pa->chunk.x1 - pa->area.x1)); 
      
      guint h = CLAMP (canvas_portion_height (pa->canvas,
                                              pa->chunk.x1, pa->chunk.y1),
                       0,
                       pag->region_height - (pa->chunk.y1 - pa->area.y1)); 
        
      ww = MIN (w, ww);
      hh = MIN (h, hh);
    
      if ((ww == 0) || (hh == 0))
        {
          del_group (pag);
          return NULL;
        }
    }
  
  /* adjust the width and height of each area */
  pag->chunk.x2 += ww;
  
  if (pag->chunk.y2 == pag->chunk.y1)
    pag->chunk.y2 += hh;
  
  for (list = pag->pixelareas;
       list;
       list = g_list_next (list))
    {
      PixelArea * pa = (PixelArea *) list->data;
      
      pa->chunk.x2 += ww;

      if (pa->chunk.y2 == pa->chunk.y1)
        pa->chunk.y2 += hh;
    }

  return pag;
}
