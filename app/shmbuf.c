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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "shmbuf.h"
#include "procedural_db.h"
#include "trace.h"

/* for now we can only attach to one shm segment */
static int SHMid = 0;
static char * SHMaddr = NULL;
static char * SHMPtr = NULL;	/* SHMaddr + offset */

struct _ShmBuf
{
  Tag      tag;
  int      width;
  int      height;
  Canvas * canvas;

  int      valid;
  int      ref_count;
  void *   data;

  int      shmid;
  char *   shmaddr;
  
  int      bytes;
};



ShmBuf * 
shmbuf_new  (
             Tag tag,
             int w,
             int h,
             Canvas * c
             )
{
  ShmBuf *f;
  
  f = (ShmBuf *) g_malloc (sizeof (ShmBuf));

  f->tag = tag;
  f->width = w;
  f->height = h;
  f->canvas = c;
  
  f->valid = FALSE;
  f->ref_count = 0;
  f->data = NULL;

  f->shmid = SHMid;
  f->shmaddr = SHMPtr;

  f->bytes = tag_bytes (tag);
  
  return f;
}


void
shmbuf_delete (
                ShmBuf * f
                )
{
  if (f)
    {
      shmbuf_portion_unalloc (f, 0, 0);
      g_free (f);
    }
}


void
shmbuf_info (
              ShmBuf * f
              )
{
  if (f)
    {
      trace_begin ("Shmbuf 0x%x", f);
      trace_printf ("%d by %d shm buffer", f->width, f->height);
      trace_printf ("%s %s %s",
                    tag_string_precision (tag_precision (shmbuf_tag (f))),
                    tag_string_format (tag_format (shmbuf_tag (f))),
                    tag_string_alpha (tag_alpha (shmbuf_tag (f))));
      trace_printf ("ref %d for 0x%x", f->ref_count, f->data);
      trace_end ();
    }
}


Tag
shmbuf_tag (
             ShmBuf * f
             )
{
  if (f)
    return f->tag;
  return tag_null ();
}


Precision
shmbuf_precision (
                   ShmBuf * f
                   )
{
  return tag_precision (shmbuf_tag (f));
}


Format
shmbuf_format (
                ShmBuf * f
                )
{
  return tag_format (shmbuf_tag (f));
}


Alpha
shmbuf_alpha (
               ShmBuf * f
               )
{
  return tag_alpha (shmbuf_tag (f));
}


guint 
shmbuf_width  (
                ShmBuf * f
                )
{
  if (f)
    return f->width;
  return 0;
}


guint 
shmbuf_height  (
                 ShmBuf * f
                 )
{
  if (f)
    return f->height;
  return 0;
}









RefRC 
shmbuf_portion_refro  (
                       ShmBuf * f,
                       int x,
                       int y
                      )
{
  RefRC rc = REFRC_FAIL;
  
  if (f && (x < f->width) && (y < f->height))
    {
      if (f->valid == FALSE)
        if (canvas_autoalloc (f->canvas) == AUTOALLOC_ON)
          (void) shmbuf_portion_alloc (f, x, y);
      
      if (f->valid == TRUE)
        {
          f->ref_count++;
          rc = REFRC_OK;
        }
    }

  return rc;
}


RefRC 
shmbuf_portion_refrw  (
                        ShmBuf * f,
                        int x,
                        int y
                        )
{
  RefRC rc = REFRC_FAIL;
  
  if (f && (x < f->width) && (y < f->height))
    {
      if (f->valid == FALSE)
        if (canvas_autoalloc (f->canvas) == AUTOALLOC_ON)
          (void) shmbuf_portion_alloc (f, x, y);
      
      if (f->valid == TRUE)
        {
          f->ref_count++;
          rc = REFRC_OK;
        }
    }

  return rc;
}


RefRC 
shmbuf_portion_unref  (
                        ShmBuf * f,
                        int x,
                        int y
                        )
{
  RefRC rc = REFRC_FAIL;

  if (f && (x < f->width) && (y < f->height))
    {
      if (f->ref_count > 0)
        f->ref_count--;
      else
        g_warning ("shmbuf unreffing with ref_count==0");

      rc = REFRC_OK;
    }
  
  return rc;
}


guint 
shmbuf_portion_width  (
                        ShmBuf * f,
                        int x,
                        int y
                        )
{
  if (f && (x < f->width) && (y < f->height))
    return f->width - x;
  return 0;
}


guint 
shmbuf_portion_height  (
                         ShmBuf * f,
                         int x,
                         int y
                         )
{
  if (f && (x < f->width) && (y < f->height))
    return f->height - y;
  return 0;
}


guint 
shmbuf_portion_y  (
                    ShmBuf * f,
                    int x,
                    int y
                    )
{
  /* always 0 */
  return 0;
}


guint 
shmbuf_portion_x  (
                    ShmBuf * f,
                    int x,
                    int y
                    )
{
  /* always 0 */
  return 0;
}


guchar * 
shmbuf_portion_data  (
                       ShmBuf * f,
                       int x,
                       int y
                       )
{
  if (f && (x < f->width) && (y < f->height))
    {
      if (f->data)
        return (guchar*)f->data + ((y * f->width) + x) * f->bytes;
    }
  return NULL;
}


guint 
shmbuf_portion_rowstride  (
                            ShmBuf * f,
                            int x,
                            int y
                            )
{
  if (f && (x < f->width) && (y < f->height))
    return f->width * f->bytes;
  return 0;
}


guint
shmbuf_portion_alloced (
                         ShmBuf * f,
                         int x,
                         int y
                         )
{
  if (f && (x < f->width) && (y < f->height))
    return f->valid;
  return FALSE;
}


guint 
shmbuf_portion_alloc  (
                        ShmBuf * f,
                        int x,
                        int y
                        )
{
  if (f && (x < f->width) && (y < f->height))
    {
      if (f->valid == TRUE)
        {
          return TRUE;
        }
      else
        {
          f->data = f->shmaddr;
          if (f->data != 0)
            {
              f->valid = TRUE;
              return TRUE;
            }
        }
    }
  return FALSE;
}


guint 
shmbuf_portion_unalloc  (
                          ShmBuf * f,
                          int x,
                          int y
                          )
{
  if (f && (x < f->width) && (y < f->height))
    {
      if ((f->valid == TRUE) && (f->ref_count == 0))
        {
          f->data = NULL;
          f->valid = FALSE;
          return TRUE;
        }
    }
  return FALSE;
}






/****************/
/*  SHMSEG_NEW  */

static Argument * 
shmseg_new_invoker  (
                     Argument * args
                     )
{
  int int_value, success;
  Argument *return_args;
  
  int key;
  int size;
  int perms;
  int shmid;
  
  key = 0;
  size = 0;
  perms = 0;
  shmid = 0;
  
  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      key = int_value;
    }
  if (success)
    {
      size = args[1].value.pdb_int;
      success = (size > 0);
    }
  if (success)
    {
      int_value = args[2].value.pdb_int;
      int_value &= 0777;
      perms = int_value;
    }
  
  if (success)
    {
      shmid = shmget (key, size, IPC_CREAT | IPC_EXCL | perms);
      success = (shmid != -1);
    }
  
  return_args = procedural_db_return_args (&shmseg_new_proc, success);

  if (success)
    return_args[0].value.pdb_int = shmid;

  return return_args;
}

/*  The procedure definition  */
ProcArg shmseg_new_args[] =
{
  { PDB_INT32,
    "shmkey",
    "the shared memory key"
  },
  { PDB_INT32,
    "size",
    "the segment size in bytes"
  },
  { PDB_INT32,
    "perms",
    "the segment permissions (standard 9 bit unix perms)"
  }
};

ProcArg shmseg_new_out_args[] =
{
  { PDB_INT32,
    "shmsegid",
    "identifier for the new segment"
  }
};

ProcRecord shmseg_new_proc =
{
  "shmseg_new",
  "Create a new shared memeory segment",
  "This procedure creates a new pool of memory suitable for use by ShmBuf Canvases.  If the specified shmkey already exists, the operation returns an invalid shmsegid.  In this case, use shmseg-attach to connect to the segment.",
  "Ray Lehtiniemi",
  "Ray Lehtiniemi",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  shmseg_new_args,

  /*  Output arguments  */
  1,
  shmseg_new_out_args,

  /*  Exec method  */
  { { shmseg_new_invoker } },
};


/*******************/
/*  SHMSEG_DELETE  */

static Argument * 
shmseg_delete_invoker  (
                        Argument * args
                        )
{
  int int_value, success;
  Argument *return_args;
  
  int shmid = 0;
  
  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      shmid = int_value;
    }
  
  if (success)
    {
      int foo = shmctl (shmid, IPC_RMID, 0);
      success = (foo != -1);
    }
  
  return_args = procedural_db_return_args (&shmseg_delete_proc, success);

  return return_args;
}

/*  The procedure definition  */
ProcArg shmseg_delete_args[] =
{
  { PDB_INT32,
    "shmid",
    "the shared memory id"
  },
};

ProcRecord shmseg_delete_proc =
{
  "shmseg_delete",
  "Delete a shared memeory segment",
  "This procedure destroys a shared memory segment.",
  "Ray Lehtiniemi",
  "Ray Lehtiniemi",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  shmseg_delete_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { shmseg_delete_invoker } },
};



/*******************/
/*  SHMSEG_ATTACH  */

static Argument * 
shmseg_attach_invoker  (
                        Argument * args
                        )
{
  int int_value, success;
  Argument *return_args;
  
  int shmid = 0;
  int offset = 0;
  char * addr;
  
  success = (SHMaddr == NULL);

  /*printf( "Attach invoker: %X\n", SHMaddr );*/

  if (success)
    {
      int_value = args[0].value.pdb_int;
      shmid = int_value;

      int_value = args[1].value.pdb_int;
      offset = int_value;
    }
  /*printf( "Attach invoker: shmid: %d, offset: %d\n", shmid, offset );*/
  
  if (success)
    {
      addr = shmat (shmid, 0, 0);
      success = (addr != (char*) -1);
    }
  /*printf( "Attach invoker: addr: %X (%d)\n", addr, success );*/
  
  if (success)
    {
      /* remember who we're attached to */
      SHMid = shmid;
      SHMaddr = addr;
	  SHMPtr = addr + offset;
    }
  
  return_args = procedural_db_return_args (&shmseg_attach_proc, success);

  return return_args;
}

/*  The procedure definition  */
ProcArg shmseg_attach_args[] =
{
  { PDB_INT32,
    "shmid",
    "the shared memory id"
  },
  { PDB_INT32,
    "offset",
    "offset to start of image data"
  }
};

ProcRecord shmseg_attach_proc =
{
  "shmseg_attach",
  "Attach to a shared memeory segment",
  "This procedure attaches to a shared memory segment.",
  "Ray Lehtiniemi",
  "Ray Lehtiniemi",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  shmseg_attach_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { shmseg_attach_invoker } },
};


/*******************/
/*  SHMSEG_DETACH  */

static Argument * 
shmseg_detach_invoker  (
                        Argument * args
                        )
{
  int success;
  Argument *return_args;
  
  success = (SHMaddr != NULL);
  
  if (success)
    {
      int foo = shmdt (SHMaddr);
      success = (foo != -1);
    }
  
  if (success)
    {
      /* forget who we're attached to */
      SHMid = 0;
      SHMaddr = NULL;
      SHMPtr = NULL;
    }
  
  return_args = procedural_db_return_args (&shmseg_detach_proc, success);

  return return_args;
}

/*  The procedure definition  */
ProcRecord shmseg_detach_proc =
{
  "shmseg_detach",
  "Detach from a shared memeory segment",
  "This procedure detaches from a shared memory segment.",
  "Ray Lehtiniemi",
  "Ray Lehtiniemi",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { shmseg_detach_invoker } },
};



/*******************/
/*  SHMSEG_STATUS  */

static Argument * 
shmseg_status_invoker  (
                        Argument * args
                        )
{
  int success;
  Argument *return_args;
  
  success = TRUE;
  
  return_args = procedural_db_return_args (&shmseg_status_proc, success);

  if (success)
    return_args[0].value.pdb_int = SHMid;

  if (success)
    return_args[1].value.pdb_int = (int)SHMPtr;

  return return_args;
}

/*  The procedure definition  */
ProcArg shmseg_status_out_args[] =
{
  { PDB_INT32,
    "shmid",
    "identifier for the current segment"
  },
  { PDB_INT32,
    "shmaddr",
    "attach point for the current segment"
  }
};

ProcRecord shmseg_status_proc =
{
  "shmseg_status",
  "Return the current shmid and attach point, if any",
  "This procedure describes the current shared memory segment.",
  "Ray Lehtiniemi",
  "Ray Lehtiniemi",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  2,
  shmseg_status_out_args,

  /*  Exec method  */
  { { shmseg_status_invoker } },
};



extern struct _ProcRecord shmbuf_new_proc;
extern struct _ProcRecord shmbuf_delete_proc;
