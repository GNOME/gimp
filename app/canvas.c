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

#include <string.h>

#include "canvas.h"
#include "flatbuf.h"
#include "shmbuf.h"
#include "tilebuf.h"
#include "trace.h"


struct _Canvas
{
  /* the image size */
  int   width;
  int   height;

  /* should a ref of a non-existent area allocate the memory? */
  AutoAlloc  autoalloc;

  /* the physical buffer holding the image */
  Storage    storage;
  void *     rep;

  /* function and data for initializing new memory */
  CanvasInitFunc init_func;
  void *         init_data;
  
  /* cached info about the physical rep */
  Tag   tag;
  int   bytes;

  /* this is so wrong */
  int x;
  int y;
};


int 
canvas_fixme_getx  (
                    Canvas * c
                    )
{
  return (c ? c->x : 0);
}

int 
canvas_fixme_gety  (
                    Canvas * c
                    )
{
  return (c ? c->y : 0);
}

int 
canvas_fixme_setx  (
                    Canvas * c,
                    int n
                    )
{
  return (c ? c->x=n : 0);
}

int 
canvas_fixme_sety  (
                    Canvas * c,
                    int n
                    )
{
  return (c ? c->y=n : 0);
}


Canvas *
canvas_new (
            Tag tag,
            int w,
            int h,
            Storage storage
            )
{
  Canvas *c;

  c = (Canvas *) g_malloc (sizeof (Canvas));

  c->storage = storage;
  c->autoalloc = AUTOALLOC_ON;
  
  c->tag    = tag;
  c->bytes  = tag_bytes (tag);
  c->width  = w;
  c->height = h;

  c->init_func = NULL;
  c->init_data = NULL;
    
  c->x = 0;
  c->y = 0;
  
  switch (storage)
    {
    case STORAGE_FLAT:
      c->rep = (void *) flatbuf_new (tag, w, h, c);
      break;
    case STORAGE_TILED:
      c->rep = (void *) tilebuf_new (tag, w, h, c);
      break;
    case STORAGE_SHM:
      c->rep = (void *) shmbuf_new (tag, w, h, c);
      break;
    default:
      c->storage = STORAGE_NONE;
      c->rep = NULL;
      c->tag = tag_null ();
      c->bytes = tag_bytes (c->tag);
      break;
    }

  return c;
}


void 
canvas_delete  (
                Canvas * c
                )
{
  if (c)
    {
      if (c->rep)
        {
          switch (c->storage)
            {
            case STORAGE_FLAT:
              flatbuf_delete ((FlatBuf *) c->rep);
              break;
            case STORAGE_TILED:
              tilebuf_delete ((TileBuf *) c->rep);
              break;
            case STORAGE_SHM:
              shmbuf_delete ((ShmBuf *) c->rep);
              break;
            default:
              break;
            }
        }
      g_free (c);
    }
}


void
canvas_info (
             Canvas *c
             )
{
  if (c)
    {
      trace_begin ("Canvas 0x%x", c);
      trace_printf ("Width: %d  Height: %d", c->width, c->height);
      trace_printf (c->autoalloc==AUTOALLOC_ON ? "AutoAlloc" : "No AutoAlloc" );
      if (c->rep)
        {
          switch (c->storage)
            {
            case STORAGE_TILED:
              tilebuf_info ((TileBuf *) c->rep);
              break;
            case STORAGE_FLAT:
              flatbuf_info ((FlatBuf *) c->rep);
              break;
            case STORAGE_SHM:
              shmbuf_info ((ShmBuf *) c->rep);
              break;
            default:
              trace_printf ("Unknown image data");
              break;
            }
        }
      else
        {
          trace_printf ("No image data");
        }
      trace_end ();
    }
}


Tag 
canvas_tag  (
             Canvas * c
             )
{
  return ( c ? c->tag : tag_null ());
}


Precision 
canvas_precision  (
                   Canvas * c
                   )
{
  return ( c ? tag_precision (c->tag) : PRECISION_NONE);
}


Format 
canvas_format  (
                Canvas * c
                )
{
  return ( c ? tag_format (c->tag) : FORMAT_NONE);
}


Alpha 
canvas_alpha  (
               Canvas * c
               )
{
  return ( c ? tag_alpha (c->tag) : ALPHA_NONE);
}


Storage 
canvas_storage  (
                 Canvas * c
                 )
{
  return ( c ? c->storage : STORAGE_NONE);
}


AutoAlloc
canvas_autoalloc (
                  Canvas * c
                  )
{
  return ( c ? c->autoalloc : AUTOALLOC_NONE);
}


AutoAlloc
canvas_set_autoalloc (
                      Canvas * c,
                      AutoAlloc aa
                      )
{
  if (c)
    switch (aa)
      {
      case AUTOALLOC_OFF:
      case AUTOALLOC_ON:
        c->autoalloc = aa;

      default:
        break;
      }
  return canvas_autoalloc (c);
}


int 
canvas_width  (
               Canvas * c
               )
{
  return ( c ? c->width : 0);
}


int 
canvas_height  (
                Canvas * c
                )
{
  return ( c ? c->height : 0);
}


int 
canvas_bytes  (
               Canvas * c
               )
{
  return ( c ? c->bytes : 0);
}




/* debugging counters */
int ref_ro = 0;
int ref_rw = 0;
int ref_un = 0;

int ref_fa = 0;
int ref_uf = 0;

RefRC 
canvas_portion_refro  (
                       Canvas * c,
                       int x,
                       int y
                       )
{
  RefRC rc = REFRC_FAIL;
  
  if (c && c->rep)
    {
      switch (c->storage)
        {
        case STORAGE_TILED:
          rc = tilebuf_portion_refro ((TileBuf *) c->rep, x, y);
          break;
        case STORAGE_FLAT:
          rc = flatbuf_portion_refro ((FlatBuf *) c->rep, x, y);
          break;
        case STORAGE_SHM:
          rc = shmbuf_portion_refro ((ShmBuf *) c->rep, x, y);
          break;
        default:
          break;
        }
    }
  
  switch (rc)
    {
    case REFRC_OK:
      ref_ro++;
      break;
    case REFRC_FAIL:
    default:
      ref_fa++;
      break;
    }
  return rc;
}


RefRC 
canvas_portion_refrw  (
                       Canvas * c,
                       int x,
                       int y
                       )
{
  RefRC rc = REFRC_FAIL;
  
  if (c && c->rep)
    {
      switch (c->storage)
        {
        case STORAGE_TILED:
          rc = tilebuf_portion_refrw ((TileBuf *) c->rep, x, y);
          break;
        case STORAGE_FLAT:
          rc = flatbuf_portion_refrw ((FlatBuf *) c->rep, x, y);
          break;
        case STORAGE_SHM:
          rc = shmbuf_portion_refrw ((ShmBuf *) c->rep, x, y);
          break;
        default:
          break;
        }
    }
  
  switch (rc)
    {
    case REFRC_OK:
      ref_rw++;
      break;
    case REFRC_FAIL:
    default:
      ref_fa++;
      break;
    }
  return rc;
}


RefRC 
canvas_portion_unref  (
                       Canvas * c,
                       int x,
                       int y
                       )
{
  RefRC rc = REFRC_FAIL;
  

  if (c && c->rep)
    {
      switch (c->storage)
        {
        case STORAGE_TILED:
          rc = tilebuf_portion_unref ((TileBuf *) c->rep, x, y);
          break;
        case STORAGE_FLAT:
          rc = flatbuf_portion_unref ((FlatBuf *) c->rep, x, y);
          break;
        case STORAGE_SHM:
          rc = shmbuf_portion_unref ((ShmBuf *) c->rep, x, y);
          break;
        default:
          break;
        }
    }
  
  switch (rc)
    {
    case REFRC_OK:
      ref_un++;
      break;
    case REFRC_FAIL:
    default:
      ref_uf++;
      break;
    }

  return rc;
}


guint 
canvas_portion_width  (
                       Canvas * c,
                       int x,
                       int y
                       )
{
  if (c && c->rep)
    {
      switch (c->storage)
        {
        case STORAGE_TILED:
          return tilebuf_portion_width ((TileBuf *) c->rep, x, y);
          break;
        case STORAGE_FLAT:
          return flatbuf_portion_width ((FlatBuf *) c->rep, x, y);
          break;
        case STORAGE_SHM:
          return shmbuf_portion_width ((ShmBuf *) c->rep, x, y);
          break;
        default:
          break;
        }
    }

  return 0;
}


guint 
canvas_portion_height  (
                        Canvas * c,
                        int x,
                        int y
                        )
{
  if (c && c->rep)
    {
      switch (c->storage)
        {
        case STORAGE_TILED:
          return tilebuf_portion_height ((TileBuf *) c->rep, x, y);
          break;
        case STORAGE_FLAT:
          return flatbuf_portion_height ((FlatBuf *) c->rep, x, y);
          break;
        case STORAGE_SHM:
          return shmbuf_portion_height ((ShmBuf *) c->rep, x, y);
          break;
        default:
          break;
        }
    }

  return 0;
}


guint 
canvas_portion_y  (
                   Canvas * c,
                   int x,
                   int y
                   )
{
  if (c && c->rep)
    {
      switch (c->storage)
        {
        case STORAGE_TILED:
          return tilebuf_portion_y ((TileBuf *) c->rep, x, y);
          break;
        case STORAGE_FLAT:
          return flatbuf_portion_y ((FlatBuf *) c->rep, x, y);
          break;
        case STORAGE_SHM:
          return shmbuf_portion_y ((ShmBuf *) c->rep, x, y);
          break;
        default:
          break;
        }
    }

  return 0;
}


guint 
canvas_portion_x  (
                   Canvas * c,
                   int x,
                   int y
                   )
{
  if (c && c->rep)
    {
      switch (c->storage)
        {
        case STORAGE_TILED:
          return tilebuf_portion_x ((TileBuf *) c->rep, x, y);
          break;
        case STORAGE_FLAT:
          return flatbuf_portion_x ((FlatBuf *) c->rep, x, y);
          break;
        case STORAGE_SHM:
          return shmbuf_portion_x ((ShmBuf *) c->rep, x, y);
          break;
        default:
          break;
        }
    }

  return 0;
}


guchar * 
canvas_portion_data  (
                      Canvas * c,
                      int x,
                      int y
                      )
{
  if (c && c->rep)
    {
      switch (c->storage)
        {
        case STORAGE_TILED:
          return tilebuf_portion_data ((TileBuf *) c->rep, x, y);
          break;
        case STORAGE_FLAT:
          return flatbuf_portion_data ((FlatBuf *) c->rep, x, y);
          break;
        case STORAGE_SHM:
          return shmbuf_portion_data ((ShmBuf *) c->rep, x, y);
          break;
        default:
          break;
        }
    }

  return NULL;
}


guint 
canvas_portion_rowstride  (
                           Canvas * c,
                           int x,
                           int y
                           )
{
  if (c && c->rep)
    {
      switch (c->storage)
        {
        case STORAGE_TILED:
          return tilebuf_portion_rowstride ((TileBuf *) c->rep, x, y);
          break;
        case STORAGE_FLAT:
          return flatbuf_portion_rowstride ((FlatBuf *) c->rep, x, y);
          break;
        case STORAGE_SHM:
          return shmbuf_portion_rowstride ((ShmBuf *) c->rep, x, y);
          break;
        default:
          break;
        }
    }

  return 0;
}


guint 
canvas_portion_alloced  (
                         Canvas * c,
                         int x,
                         int y
                         )
{
  if (c && c->rep)
    {
      switch (c->storage)
        {
        case STORAGE_TILED:
          return tilebuf_portion_alloced ((TileBuf *) c->rep, x, y);
          break;
        case STORAGE_FLAT:
          return flatbuf_portion_alloced ((FlatBuf *) c->rep, x, y);
          break;
        case STORAGE_SHM:
          return shmbuf_portion_alloced ((ShmBuf *) c->rep, x, y);
          break;
        default:
          break;
        }
    }

  return FALSE;
}


guint 
canvas_portion_alloc  (
                       Canvas * c,
                       int x,
                       int y
                       )
{
  if (c && c->rep)
    {
      switch (c->storage)
        {
        case STORAGE_TILED:
          return tilebuf_portion_alloc ((TileBuf *) c->rep, x, y);
          break;
        case STORAGE_FLAT:
          return flatbuf_portion_alloc ((FlatBuf *) c->rep, x, y);
          break;
        case STORAGE_SHM:
          return shmbuf_portion_alloc ((ShmBuf *) c->rep, x, y);
          break;
        default:
          break;
        }
    }

  return FALSE;
}


guint 
canvas_portion_unalloc  (
                         Canvas * c,
                         int x,
                         int y
                         )
{
  if (c && c->rep)
    {
      switch (c->storage)
        {
        case STORAGE_TILED:
          return tilebuf_portion_unalloc ((TileBuf *) c->rep, x, y);
          break;
        case STORAGE_FLAT:
          return flatbuf_portion_unalloc ((FlatBuf *) c->rep, x, y);
          break;
        case STORAGE_SHM:
          return shmbuf_portion_unalloc ((ShmBuf *) c->rep, x, y);
          break;
        default:
          break;
        }
    }

  return FALSE;
}


/*
   FIXME:

   this doesn;t work right for tiles.  edge tiles only get the
   "on-image" portion inited.  the part that lies off the image isn;t
   inited because the portion_width and portion_height ignore it.

   the proper fix for this is to split portion out of canvas and deal
   only with tiles at the canvas level.  for now we just memset the
   chunk to zero in portion_alloc */

guint 
canvas_portion_init  (
                      Canvas * c,
                      int x,
                      int y
                      )
{
  if (c)
    {
      x = canvas_portion_x (c, x, y);
      y = canvas_portion_y (c, x, y);

      if (c->init_func)
        {
          return c->init_func (c, x, y, c->init_data);
        }
      else
        {
          guint w = canvas_portion_width (c, x, y);
          guint h = canvas_portion_height (c, x, y);
          guchar * d = canvas_portion_data (c, x, y);

          if (d != NULL)
            {
              memset (d, 0, w * h * canvas_bytes (c));
              return TRUE;
            }
        }
    }
  return FALSE;
}

void
canvas_portion_init_setup (
                           Canvas * c,
                           CanvasInitFunc func,
                           void * data
                           )
{
  if (c)
    {
      c->init_func = func;
      c->init_data = data;
    }
}
