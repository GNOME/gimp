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
#include "trace.h"


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

static PixelAreaGroup * pixelareagroup_new            (int);
static void             pixelareagroup_delete         (PixelAreaGroup *);
static void             pixelareagroup_addarea        (PixelAreaGroup *, PixelArea *);
static void *           pixelareagroup_configure      (PixelAreaGroup *);
static void             pixelareagroup_unconfigure    (PixelAreaGroup *, void *);
static int              pixelareagroup_portion_height (PixelAreaGroup *);
static int              pixelareagroup_portion_width  (PixelAreaGroup *);




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
pixelarea_info  (
                 PixelArea * pa
                 )
{
  trace_begin ("PixelArea 0x%x", pa);
  trace_printf ("coord (%d,%d)", pa->x, pa->y);
  trace_printf ("start (%d,%d)", pa->startx, pa->starty);
  trace_printf ("width (%d,%d)", pa->w, pa->h);
  trace_printf ("canvas: 0x%x", pa->canvas);
  trace_printf ("dirty: 0x%d", pa->dirty);
  trace_end ();
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


/* copies the data from the PixelArea to the PixelRow */
#define PIXELAREA_C_1_cw
void
pixelarea_copy_row ( 
		    PixelArea *pa,
		    PixelRow *pr,
		    int x,
		    int y,
                    int width, 
                    int subsample
		   )
#define FIXME
/* this doesn't work right for subsampling, and it's holding up
   channel and layer previews */
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

          cur += subsample;
          pr_data += bytes * subsample;
          area_data += bytes * subsample;
        }

      remainder = cur - portion_width;
    }
}


/* copies the data from the PixelArea to the PixelRow col*/
#define PIXELAREA_C_2_cw
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
        area_data += rowstride;          	
      } 
    }
}


/* copies the data from the PixelRow to the PixelArea */
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
      memcpy(area_data, pr_data, tag_bytes (pa_tag) * portion_width);
      pr_data += portion_width;
    }
}


/* copies the data from the PixelRow col to the PixelArea */
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
	  *col_data++ = *area_data++;
        area_data += rowstride;
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

  if (num_areas < 1)
    return NULL;
  
  pag = pixelareagroup_new (TRUE);

  va_start (ap, num_areas);
  while (num_areas --)
    {
      PixelArea * pa = va_arg (ap, PixelArea *);      
      pixelareagroup_addarea (pag, pa);
    }
  va_end (ap);

  return pixelareagroup_configure (pag);
}


void * 
pixelarea_register_noref  (
                           int num_areas,
                           ...
                           )
{
  PixelAreaGroup *pag;
  va_list ap;

  if (num_areas < 1)
    return NULL;
  
  pag = pixelareagroup_new (FALSE);

  va_start (ap, num_areas);
  while (num_areas --)
    {
      PixelArea * pa = va_arg (ap, PixelArea *);      
      pixelareagroup_addarea (pag, pa);
    }
  va_end (ap);

  return pixelareagroup_configure (pag);
}


void *
pixelarea_process  (
                    void * x
                    )
{
  PixelAreaGroup *pag = (PixelAreaGroup *) x;

  pixelareagroup_unconfigure (pag, NULL);

  return pixelareagroup_configure (pag);
}


void 
pixelarea_process_stop (
                        void * x
                        )
{
  PixelAreaGroup *pag = (PixelAreaGroup *) x;
  if (pag)
    {
      GList * list = pag->pixelareas;

      while (list)
        {
          PixelArea * pa = (PixelArea *) list->data;
          if ((pa != NULL) && pag->autoref)
            {
              if (pixelarea_unref (pa) != TRUE)
                g_warning ("pixelarea_process_stop() failed to pixelarea_unref()?!?");
            }
          list = g_list_next (list);
        }
    }
  
  pixelareagroup_delete (pag);
}



/*-------------------------------------------------------------------------------

                                PixelAreaGroup

  -------------------------------------------------------------------------------*/
  
static PixelAreaGroup *
pixelareagroup_new (
                    int autoref
                    )
{
  PixelAreaGroup *pag;
  
  pag = (PixelAreaGroup *) g_malloc (sizeof (PixelAreaGroup));

  pag->pixelareas = NULL;
  pag->autoref = autoref;
  pag->region_width = G_MAXINT;
  pag->region_height = G_MAXINT;
  pag->portion_width = 0;
  pag->portion_height = 0;

  return pag;
}


static void
pixelareagroup_delete (
                       PixelAreaGroup * pag
                       )
{
  if (pag)
    {
      if (pag->pixelareas)
        /* the data items are pointers to PixelAreas, which are always
           located on the stack, so don;t free them... */
        g_list_free (pag->pixelareas);
      g_free (pag);
    }
}


static void
pixelareagroup_addarea (
                        PixelAreaGroup * pag,
                        PixelArea * pa
                        )
{
  if (pag)
    {
      pag->pixelareas = g_list_append (pag->pixelareas, pa);
      if (pa)
        {
          pag->region_width  = MIN (pag->region_width, pa->w);
          pag->region_height = MIN (pag->region_height, pa->h);
        }
    }
}


static void *
pixelareagroup_configure (
                          PixelAreaGroup * pag
                          )
{
  guint success = FALSE;

  if (pag == NULL)
    return NULL;
  
  /* loop until all areas work or we find a null portion */
  while (success == FALSE)
    {
      GList * list;

      /*  Determine the next portion width and height */
      if ((pixelareagroup_portion_width (pag) == 0) ||
          (pixelareagroup_portion_height (pag) == 0))
        {
          pixelareagroup_delete (pag);
          return NULL;
        }

      /* adjust the width and height of each area */
      for (list = pag->pixelareas;
           list;
           list = g_list_next (list))
        {
          PixelArea * pa = (PixelArea *) list->data;
          if (pa)
            {
              pa->w = pag->portion_width;
              pa->h = pag->portion_height;
            }
        }

      /* if we don;t want to ref, we're done */
      if (pag->autoref != TRUE)
        return (void *) pag;
      
      /* assume everything will work */
      success = TRUE;
      
      /* fault each portion into memory */
      for (list = pag->pixelareas;
           list && (success == TRUE);
           list = g_list_next (list))
        {
          PixelArea * pa = (PixelArea *) list->data;
          if (pa)
            success = pixelarea_ref (pa);

          if (success == FALSE)
            pixelareagroup_unconfigure (pag, list);
        }
    }
  
  /* not reached */
  return pag;
}


static void
pixelareagroup_unconfigure (
                            PixelAreaGroup * pag,
                            void * listtail
                            )
{
  GList * list;

  if (pag == NULL)
    return;

  /* unref as many pieces as requested */
  if (pag->autoref)
    {
      for (list = pag->pixelareas;
           list != listtail;
           list = g_list_next (list))
        {
          PixelArea * pa = (PixelArea *) list->data;
          if (pa)
            if (pixelarea_unref (pa) != TRUE)
              g_warning ("pixelareagroup_unconfigure() failed to unref...");
        }
    }

  /* move to the right, moving down if we hit the edge of the
     overall region */
  for (list = pag->pixelareas;
       list;
       list = g_list_next (list))
    {
      PixelArea * pa = (PixelArea *) list->data;
      if (pa)
        {
          pa->x += pag->portion_width;
          if ((pa->x - pa->startx) >= pag->region_width)
            {
              pa->x = pa->startx;
              pa->y += pag->portion_height;
            }
        }
    }
}


static int
pixelareagroup_portion_height (
                               PixelAreaGroup * pag
                               )
{
  int min_height = G_MAXINT;

  if (pag)
    {
      GList * list = pag->pixelareas;
      int height;

      /* find the smallest contiguous height among the areas */
      while (list)
        {
          PixelArea * pa = (PixelArea *) list->data;
          if (pa)
            {
              if ((pa->y - pa->starty) >= pag->region_height)
                {
                  min_height = 0;
                  break;
                }
              height = canvas_portion_height (pa->canvas,
                                              pa->x, pa->y);          
#define PIXELAREA_C_4_cw
              height = CLAMP (height,
                              0 , pag->region_height - (pa->y - pa->starty)); 
              if (height < min_height)
                min_height = height;
            }
          list = g_list_next (list);
        }
      
      pag->portion_height = min_height;
    }
  
  return min_height;
}


static int
pixelareagroup_portion_width (
                              PixelAreaGroup * pag
                              )
{
  int min_width = G_MAXINT;

  if (pag)
    {
      GList * list = pag->pixelareas;
      int width;

      /* find the smallest contiguous width among the areas */
      while (list)
        {
          PixelArea * pa = (PixelArea *) list->data;
          if (pa)
            {
              if ((pa->x - pa->startx) >= pag->region_width)
                {
                  min_width = 0;
                  break;
                }
              width = canvas_portion_width (pa->canvas,
                                            pa->x, pa->y);          
#define PIXELAREA_C_3_cw
              width = CLAMP (width,
                             0, pag->region_width - (pa->x - pa->startx)); 
              if (width < min_width)
                min_width = width;
            }
          list = g_list_next (list);
        }
      
      pag->portion_width = min_width;
    }
  
  return min_width;
}

