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
  GList * pixelareas;
  int     autoref;
  int     region_width;
  int     region_height;
  int     portion_width;
  int     portion_height;
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
  if (pa)
    pa->canvas = c;
  pixelarea_resize (pa, x, y, w, h, will_dirty);
}


void 
pixelarea_resize (
                 PixelArea * pa,
                 int x,
                 int y,
                 int w,
                 int h,
                 int will_dirty
                 )
{
  if (pa)
    {
      pa->x = x;
      pa->y = y;

      pa->w = canvas_width (pa->canvas);
      if ((w > 0) && (w <= pa->w))
        pa->w = w;

      pa->h = canvas_height (pa->canvas);
      if ((h > 0) && (h <= pa->h))
        pa->h = h;


      pa->dirty = will_dirty;
      pa->startx = pa->x;
      pa->starty = pa->y;
    }
}


void 
pixelarea_getdata  (
                    PixelArea * pa,
                    PixelRow * pr,
                    int row
                    )
{
  if (pa && pixelarea_data (pa) && (row >= 0) && (row < pa->h))
    pixelrow_init (pr,
                   pixelarea_tag (pa),
                   pixelarea_data (pa) + row * pixelarea_rowstride (pa),
                   pa->w);
  else
    pixelrow_init (pr,
                   tag_null (),
                   NULL,
                   0);
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
  if (pa)
    return canvas_tag (pa->canvas);
  return tag_null ();
}


int 
pixelarea_width  (
                  PixelArea * pa
                  )
{
  if (pa)
    return pa->w;
  return 0;
}


int 
pixelarea_height  (
                   PixelArea * pa
                   )
{
  if (pa)
    return pa->h;
  return 0;
}


int 
pixelarea_x  (
              PixelArea * pa
              )
{
  if (pa)
    return pa->x;
  return 0;
}


int 
pixelarea_y  (
              PixelArea * pa
              )
{
  if (pa)
    return pa->y;
  return 0;
}


guchar * 
pixelarea_data (
                PixelArea * pa
                )
{
  if (pa)
    return canvas_portion_data (pa->canvas, pa->x, pa->y);
  return NULL;
}


int 
pixelarea_rowstride (
                     PixelArea * pa
                     )
{
  if (pa)
    return canvas_portion_rowstride (pa->canvas, pa->x, pa->y);
  return 0;
}


guint
pixelarea_ref (
               PixelArea * pa
               )
{
  guint rc = FALSE;
  
  if (pa)
    {
      RefRC rrc = REFRC_FAIL;
      
      switch (pa->dirty)
        {
        case TRUE:
          rrc = canvas_portion_refrw (pa->canvas, pa->x, pa->y);
          break;
        case FALSE:
        default:
          rrc = canvas_portion_refro (pa->canvas, pa->x, pa->y);
          break;
        }

      switch (rrc)
        {
        case REFRC_OK:
          rc = TRUE;
          break;
        case REFRC_FAIL:
        default:
          rc = FALSE;
          break;
        }
    }
  
  return rc;
}


guint
pixelarea_unref (
                 PixelArea * pa                 
                 )
{
  guint rc = FALSE;
  
  if (pa)
    {
      switch (canvas_portion_unref (pa->canvas, pa->x, pa->y))
        {
        case REFRC_OK:
          rc = TRUE;
          break;
        case REFRC_FAIL:
        default:
          break;
        }
    }
  
  return rc;
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
  pag->portion_width = 0;
  pag->portion_height = 0;

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
      
      for (list = pag->pixelareas;
           list;
           list = g_list_next (list))
        {
          if ((PixelArea *) list->data == pa)
            return;
        }

      pag->pixelareas = g_list_append (pag->pixelareas, pa);

      pag->region_width  = MIN (pag->region_width, pa->w);
      pag->region_height = MIN (pag->region_height, pa->h);
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
        g_warning ("pixelareagroup_unconfigure() failed to unref...");
    }
}



static PixelAreaGroup * 
next_chunk  (
             PixelAreaGroup * pag
             )
{
  GList * list;
  
  g_return_val_if_fail (pag != NULL, NULL);

  /* move all areas to the next chunk */
  for (list = pag->pixelareas;
       list;
       list = g_list_next (list))
    {
      PixelArea * pa = (PixelArea *) list->data;

      pa->x += pag->portion_width;

      if ((pa->x - pa->startx) >= pag->region_width)
        {
          pa->x = pa->startx;
          pa->y += pag->portion_height;
        }
      
      if ((pa->y - pa->starty) >= pag->region_height)
        {
          del_group (pag);
          return NULL;
        }
    }

  /* determine width and height of next chunk */
  pag->portion_width = G_MAXINT;
  pag->portion_height = G_MAXINT;

  for (list = pag->pixelareas;
       list;
       list = g_list_next (list))
    {
      PixelArea * pa = (PixelArea *) list->data;

      guint w = CLAMP (canvas_portion_width (pa->canvas,
                                             pa->x, pa->y),
                       0,
                       pag->region_width - (pa->x - pa->startx)); 
      
      guint h = CLAMP (canvas_portion_height (pa->canvas,
                                              pa->x, pa->y),
                       0,
                       pag->region_height - (pa->y - pa->starty)); 
      
      pag->portion_width = MIN (w, pag->portion_width);
      pag->portion_height = MIN (h, pag->portion_height);
    }

  if ((pag->portion_width == 0) || (pag->portion_height == 0))
    {
      del_group (pag);
      return NULL;
    }

  /* adjust the width and height of each area */
  for (list = pag->pixelareas;
       list;
       list = g_list_next (list))
    {
      PixelArea * pa = (PixelArea *) list->data;
      
      pa->w = pag->portion_width;
      pa->h = pag->portion_height;
    }

  return pag;
}
