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
#include "paint.h"
#include "pixelrow.h"
#include "pixelarea.h"



typedef struct _PixelAreaGroup PixelAreaGroup;

struct _PixelAreaGroup
{
  GList * pixelareas;
  int     region_width;
  int     region_height;
  int     portion_width;
  int     portion_height;
};

static PixelAreaGroup * pixelareagroup_new            (void);
static void             pixelareagroup_delete         (PixelAreaGroup *);
static void             pixelareagroup_addarea        (PixelAreaGroup *, PixelArea *);
static PixelAreaGroup * pixelareagroup_configure      (PixelAreaGroup *);
static int              pixelareagroup_portion_height (PixelAreaGroup *);
static int              pixelareagroup_portion_width  (PixelAreaGroup *);

static void             pixelarea_ref                 (PixelArea *);
static void             pixelarea_unref               (PixelArea *);






void 
pixelarea_init  (
                 PixelArea * pa,
                 Canvas * c,
                 Canvas * init,
                 int x,
                 int y,
                 int w,
                 int h,
                 int will_dirty
                 )
{
  pa->x = x;
  pa->y = y;

  if (w == 0)
    pa->w = canvas_width (c);
  else
    pa->w = w;

  if (h == 0)
    pa->h = canvas_height (c);
  else
    pa->h = h;

  pa->canvas = c;
  pa->init = init;
  pa->dirty = will_dirty;
  pa->startx = x;
  pa->starty = y;
}


void 
pixelarea_getdata  (
                    PixelArea * pa,
                    PixelRow * pr,
                    int row
                    )
{
  if ((row >= pa->h) || (row < 0))
    pixelrow_init (pr,
                   tag_new (PRECISION_NONE, FORMAT_NONE, ALPHA_NONE),
                   NULL,
                   0);
  else
    pixelrow_init (pr,
                   pixelarea_tag (pa),
                   pixelarea_data (pa) + row * pixelarea_rowstride (pa),
                   pa->w);
}

/* copies the data from the PixelArea to the PixelRow */
void
pixelarea_copy_row ( 
		    PixelArea *pa,
		    PixelRow *pr,
		    int x,
		    int y,
                    int length, 
                    int subsample
		   )
{
}


/* copies the data from the PixelArea to the PixelRow col*/
void
pixelarea_copy_col ( 
		    PixelArea *pa,
		    PixelRow *col,
		    int x,
		    int y,
                    int length, 
                    int subsample
		   )
{
}


/* copies the data from the PixelRow to the PixelArea */
void
pixelarea_write_row ( 
		    PixelArea *pa,
		    PixelRow *pr,
		    int x,
		    int y,
                    int length 
		   )
{
}


/* copies the data from the PixelRow col to the PixelRow */
void
pixelarea_write_col ( 
		    PixelArea *pa,
		    PixelRow *row,
		    int x,
		    int y,
                    int length 
		   )
{
}

Paint *
pixelarea_convert_paint (
                         PixelArea *pa,
                         Paint * paint
                         )
{
  Paint * new = paint;
  
  if (! tag_equal (paint_tag (paint), pixelarea_tag (pa)))
    {
      Precision precision = tag_precision (pixelarea_tag (pa));
      Format format = tag_format (pixelarea_tag (pa));
      Alpha alpha = tag_alpha (pixelarea_tag (pa));
      
      new = paint_clone (paint);
      
      if ((paint_set_precision (new, precision) != precision) ||
          (paint_set_format (new, format) != format) ||
          (paint_set_alpha (new, alpha) != alpha))
        {
          paint_delete (new);
          return NULL;
        }
    }
  
  return new;
}


Tag 
pixelarea_tag  (
                PixelArea * pa
                )
{
  return canvas_tag (pa->canvas);
}


int 
pixelarea_width  (
                  PixelArea * pa
                  )
{
  return pa->w;
}


int 
pixelarea_height  (
                   PixelArea * pa
                   )
{
  return pa->h;
}


int 
pixelarea_x  (
              PixelArea * pa
              )
{
  return pa->x;
}


int 
pixelarea_y  (
              PixelArea * pa
              )
{
  return pa->y;
}


guchar * 
pixelarea_data (
                PixelArea * pa
                )
{
  return canvas_data (pa->canvas, pa->x, pa->y);
}


int 
pixelarea_rowstride (
                     PixelArea * pa
                     )
{
  return canvas_rowstride (pa->canvas, pa->x, pa->y);
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
  
  pag = pixelareagroup_new ();

  va_start (ap, num_areas);
  while (num_areas --)
    {
      PixelArea * pa = va_arg (ap, PixelArea *);      
      pixelareagroup_addarea (pag, pa);
    }
  va_end (ap);

  return (void *) pixelareagroup_configure (pag);
}


void *
pixelarea_process  (
                    void * x
                    )
{
  PixelAreaGroup *pag = (PixelAreaGroup *) x;
  GList * list = pag->pixelareas;

  /* unref all the current pieces, then move to the right, moving down
     if we hit the edge of the overall region */
  while (list)
    {
      PixelArea * pa = (PixelArea *) list->data;
      if (pa != NULL)
	{
          pixelarea_unref (pa);
	  pa->x += pag->portion_width;
	  if ((pa->x - pa->startx) >= pag->region_width)
	    {
	      pa->x = pa->startx;
	      pa->y += pag->portion_height;
	    }
	}
      list = g_list_next (list);
    }

  return (void *) pixelareagroup_configure (pag);
}


void 
pixelarea_process_stop (
                        void * x
                        )
{
  PixelAreaGroup *pag = (PixelAreaGroup *) x;
  GList * list = pag->pixelareas;

  while (list)
    {
      PixelArea * pa = (PixelArea *) list->data;
      if (pa != NULL)
	{
          pixelarea_unref (pa);
	}
      list = g_list_next (list);
    }

  pixelareagroup_delete (pag);
}



static void
pixelarea_ref (
               PixelArea * pa
               )
{
  if (canvas_ref (pa->canvas, pa->x, pa->y) == TRUE)
    {
      canvas_init (pa->canvas, pa->init, pa->x, pa->y);
    }
}


static void
pixelarea_unref (
                 PixelArea * pa                 
                 )
{
  canvas_unref (pa->canvas, pa->x, pa->y);
}




/*-------------------------------------------------------------------------------

                                PixelAreaGroup

  -------------------------------------------------------------------------------*/
  
static PixelAreaGroup *
pixelareagroup_new (
                    void
                    )
{
  PixelAreaGroup *pag;
  
  pag = (PixelAreaGroup *) g_malloc (sizeof (PixelAreaGroup));

  pag->pixelareas = NULL;
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
  if (pag->pixelareas)
    /* the data items are pointers to PixelAreas, which are always
       located on the stack, so don;t free them... */
    g_list_free (pag->pixelareas);
  g_free (pag);
}


static void
pixelareagroup_addarea (
                        PixelAreaGroup *pag,
                        PixelArea *pa
                        )
{
  pag->pixelareas = g_list_append (pag->pixelareas, pa);
  if (pa != NULL)
    {
      pag->region_width  = MIN (pag->region_width, pa->w);
      pag->region_height = MIN (pag->region_height, pa->h);
    }
}


static PixelAreaGroup *
pixelareagroup_configure (
                          PixelAreaGroup * pag
                          )
{
  GList * list;

  /*  Determine the next portion width and height */
  if ((pixelareagroup_portion_width (pag) == 0) ||
      (pixelareagroup_portion_height (pag) == 0))
    {
      pixelareagroup_delete (pag);
      return NULL;
    }

  /* fault each portion into memory */
  list = pag->pixelareas;
  while (list)
    {
      PixelArea * pa = (PixelArea *) list->data;
      if (pa)
	{
          pixelarea_ref (pa);
          pa->w = pag->portion_width;
          pa->h = pag->portion_height;
	}
      list = g_list_next (list);
    }

  return pag;
}



static int
pixelareagroup_portion_height (
                               PixelAreaGroup * pag
                               )
{
  GList * list = pag->pixelareas;
  int min_height = G_MAXINT;
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
	  if (height < min_height)
	    min_height = height;
	}
      list = g_list_next (list);
    }

  pag->portion_height = min_height;
  return min_height;
}


static int
pixelareagroup_portion_width (
                              PixelAreaGroup * pag
                              )
{
  GList * list = pag->pixelareas;
  int min_width = G_MAXINT;
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
	  if (width < min_width)
	    min_width = width;
	}
      list = g_list_next (list);
    }

  pag->portion_width = min_width;
  return min_width;
}



