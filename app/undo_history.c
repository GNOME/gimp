/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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
 *
 * Undo history browser by Austin Donnelly <austin@gimp.org>
 */


/* TODO:
 *
 *  - reuse the L&C previews?
 *         Currently we use gimp_image_construct_composite_preview ()
 *	   which makes use of the preview_cache on a per layer basis.
 *
 *  - work out which (if any) is the clean image, and mark it as such.
 *         Currently, it's on the wrong line.
 *
 *  - undo names are less than useful.  This isn't a problem with
 *         undo_history.c itself, more with the rather chaotic way
 *         people have of picking an undo type when pushing undos, and
 *         inconsistent use of undo groups.  Maybe rather than
 *         specifying an (enum) type, it should be a const char * ?
 *
 * BUGS:
 *  - clean pixmap in wrong place
 *
 *  Initial rev 0.01, (c) 19 Sept 1999 Austin Donnelly <austin@gimp.org>
 *
 */

#include <gtk/gtk.h>
#include "gimprc.h"
#include "gimpui.h"
#include "temp_buf.h"
#include "undo.h"
#include "gimage_mask.h"
#include "dialog_handler.h"

#include "config.h"
#include "libgimp/gimpintl.h"
#include "libgimp/gimplimits.h"

#include "pixmaps/raise.xpm"
#include "pixmaps/lower.xpm"
#include "pixmaps/yes.xpm"
#include "pixmaps/question.xpm"

typedef struct
{
  GImage    *gimage;	      /* image we're tracking undo info for */
  GtkWidget *shell;	      /* dialog window */
  GtkWidget *clist;	      /* list of undo actions */
  GtkWidget *undo_button;     /* button to undo an operation */
  GtkWidget *redo_button;     /* button to redo an operation */
  int        old_selection;   /* previous selection in the clist */
  int        preview_size;    /* size of the previews (from preferences) */
} undo_history_st;

typedef struct 
{
  GtkCList *clist;
  gint      row;
  gint      size;
  GImage   *gimage;
} idle_preview_args;

/*
 * Theory of operation.
 *
 * Keep a clist.  Each row of the clist corresponds to an image as it
 * was at some time in the past, present or future.  The selected row
 * is the present image.  Rows below the selected one are in the
 * future - as redo operations are performed, they become the current
 * image.  Rows above the selected one are in the past - undo
 * operations move the highlight up.
 *
 * The slight fly in the ointment is that if rows are images, then how
 * should they be labelled?  An undo or redo operation goes _between_
 * two image states - it isn't an image state.  It's a pretty
 * arbitrary decision, but I've chosen to label a row with the name of
 * the action that brought the image into the state represented by
 * that row.  Thus, there is a special first row without a meaningful
 * label, which represents the image state before the first action has
 * been done to it.  The choice is between a special first row or a
 * special last row.  Since people mostly work near the leading edge,
 * not often going all the way back, I've chosen to put the special
 * case out of common sight.
 *
 * So, the undo stack contents appear above the selected row, and the
 * redo stack below it.
 *
 * The clist is initialised by mapping over the undo and redo stack.
 *
 * Once initialised, the dialog listens to undo_event signals from the
 * gimage.  These undo events allow us to track changes to the undo
 * and redo stacks.  We follow the events, making parallel changes to
 * the clist.  If we ever get out of sync, there is no mechanism to
 * notice or re-sync.  A few g_return_if_fails should catch some of
 * these cases.
 *
 * User clicks changing the selected row in the clist turn into
 * multiple calls to undo_pop or undo_redo, with appropriate signals
 * blocked so we don't get our own events back.
 *
 * The "Close" button hides the dialog, rather than destroying it.
 * This may well need to be changed, since the dialog will continue to
 * track updates, and if it's generating previews this might take too
 * long for large images. 
 *
 * The dialog is destroyed when the gimage it is tracking is
 * destroyed.  Note that a File/Revert destroys the current gimage and
 * so blows the undo/redo stacks.
 *
 * --austin, 19/9/1999
 */

/**************************************************************/
/* Static Data */

static GdkPixmap *clean_pixmap = NULL;
static GdkBitmap *clean_mask   = NULL;

static GdkPixmap *clear_pixmap = NULL;
static GdkBitmap *clear_mask   = NULL;

/**************************************************************/
/* Local functions */


static MaskBuf *
mask_render_preview (GImage        *gimage,
		     gint          *pwidth,
		     gint          *pheight)
{
  Channel * mask;
  MaskBuf * scaled_buf = NULL;
  PixelRegion srcPR, destPR;
  int subsample;
  int width, height;
  int scale;

  mask = gimage_get_mask (gimage);
  if ((drawable_width (GIMP_DRAWABLE(mask)) > *pwidth) ||
      (drawable_height (GIMP_DRAWABLE(mask)) > *pheight))
    {
      if (((float) drawable_width (GIMP_DRAWABLE (mask)) / (float) *pwidth) >
	  ((float) drawable_height (GIMP_DRAWABLE (mask)) / (float) *pheight))
	{
	  width = *pwidth;
	  height = (drawable_height (GIMP_DRAWABLE (mask)) * (*pwidth)) / drawable_width (GIMP_DRAWABLE (mask));
	}
      else
	{
	  width = (drawable_width (GIMP_DRAWABLE (mask)) * (*pheight)) / drawable_height (GIMP_DRAWABLE (mask));
	  height = *pheight;
	}

      scale = TRUE;
    }
  else
    {
      width = drawable_width (GIMP_DRAWABLE (mask));
      height = drawable_height (GIMP_DRAWABLE (mask));

      scale = FALSE;
    }

  /*  if the mask is empty, no need to scale and update again  */
  if (gimage_mask_is_empty (gimage))
    return NULL;

  if (scale)
    {
      /*  calculate 'acceptable' subsample  */
      subsample = 1;
      while ((width * (subsample + 1) * 2 < drawable_width (GIMP_DRAWABLE (mask))) &&
	     (height * (subsample + 1) * 2 < drawable_height (GIMP_DRAWABLE (mask))))
	subsample = subsample + 1;

      pixel_region_init (&srcPR, drawable_data (GIMP_DRAWABLE (mask)), 
			 0, 0, 
			 drawable_width (GIMP_DRAWABLE (mask)), 
			 drawable_height (GIMP_DRAWABLE (mask)), FALSE);

      scaled_buf = mask_buf_new (width, height);
      destPR.bytes = 1;
      destPR.x = 0;
      destPR.y = 0;
      destPR.w = width;
      destPR.h = height;
      destPR.rowstride = srcPR.bytes * width;
      destPR.data = mask_buf_data (scaled_buf);
      destPR.tiles = NULL;

      subsample_region (&srcPR, &destPR, subsample);
    }
  else
    {
      pixel_region_init (&srcPR, drawable_data (GIMP_DRAWABLE (mask)), 
			 0, 0, 
			 drawable_width (GIMP_DRAWABLE (mask)), 
			 drawable_height (GIMP_DRAWABLE (mask)), FALSE);

      scaled_buf = mask_buf_new (width, height);
      destPR.bytes = 1;
      destPR.x = 0;
      destPR.y = 0;
      destPR.w = width;
      destPR.h = height;
      destPR.rowstride = srcPR.bytes * width;
      destPR.data = mask_buf_data (scaled_buf);
      destPR.tiles = NULL;

      copy_region (&srcPR, &destPR);
    }

  *pheight = height;
  *pwidth = width;
   return scaled_buf;
}


static gint
undo_history_set_pixmap_idle (gpointer data)
{
  idle_preview_args *idle = data;
  static GdkGC *gc = NULL;
  TempBuf   *buf = NULL;
  GdkPixmap *pixmap;
  UndoType   utype;
  MaskBuf   *mbuf = NULL;
  guchar    *src;
  gdouble    r, g, b, a;
  gdouble    c0, c1;
  guchar    *p0, *p1, *even, *odd;
  gint       width, height, bpp;
  gint       x, y;

  if (!gc)
    gc = gdk_gc_new (GTK_WIDGET (idle->clist)->window);

  width  = idle->gimage->width;
  height = idle->gimage->height;

  /* Get right aspect ratio */  
  if (width > height)
    { 
      height = (gint)(((gdouble)idle->size * (gdouble)height) / (gdouble)width + 0.5);
      width  = (gint)(((gdouble)width * (gdouble)height)/ (gdouble)idle->gimage->height + 0.5);
    }
  else
    {
      width  = (gint)(((gdouble)idle->size * (gdouble)width) / (gdouble)height + 0.5);
      height = (gint)(((gdouble)height * (gdouble)width ) /(gdouble) idle->gimage->width + 0.5);
    }

  utype = undo_get_undo_top_type (idle->gimage);

  if((utype != MASK_UNDO && utype != QMASK_UNDO) || 
     (mbuf = mask_render_preview (idle->gimage,&width,&height)) == NULL)
    {
      buf = gimp_image_construct_composite_preview (idle->gimage, width, height);
      bpp = buf->bytes;
      src = temp_buf_data (buf);
    }
  else
    {
      src = mask_buf_data (mbuf); 
      bpp = 1; /* Always the case for masks */
    } 

  pixmap = gdk_pixmap_new (GTK_WIDGET (idle->clist)->window, width+2, height+2, -1);
  
  gdk_draw_rectangle (pixmap, 
		      GTK_WIDGET (idle->clist)->style->black_gc,
		      TRUE,
		      0, 0,
		      width + 2, height + 2);

  even = g_malloc (width * 3);
  odd  = g_malloc (width * 3);

  for (y = 0; y < height; y++)
    {
      p0 = even;
      p1 = odd;

      for (x = 0; x < width; x++)
	{
	  if (bpp == 4)
	    {
	      r = ((gdouble) src[x*4+0]) / 255.0;
	      g = ((gdouble) src[x*4+1]) / 255.0;
	      b = ((gdouble) src[x*4+2]) / 255.0;
	      a = ((gdouble) src[x*4+3]) / 255.0;
	    }
	  else if (bpp == 3)
	    {
	      r = ((gdouble) src[x*3+0]) / 255.0;
	      g = ((gdouble) src[x*3+1]) / 255.0;
	      b = ((gdouble) src[x*3+2]) / 255.0;
	      a = 1.0;
	    }
	  else
	    {
	      r = ((gdouble) src[x*bpp+0]) / 255.0;
	      g = b = r;
	      if (bpp == 2)
		a = ((gdouble) src[x*bpp+1]) / 255.0;
	      else
		a = 1.0;
	    }

	  if ((x / GIMP_CHECK_SIZE_SM) & 1)
	    {
	      c0 = GIMP_CHECK_LIGHT;
	      c1 = GIMP_CHECK_DARK;
	    }
	  else
	    {
	      c0 = GIMP_CHECK_DARK;
	      c1 = GIMP_CHECK_LIGHT;
	    }

	  *p0++ = (c0 + (r - c0) * a) * 255.0;
	  *p0++ = (c0 + (g - c0) * a) * 255.0;
	  *p0++ = (c0 + (b - c0) * a) * 255.0;

	  *p1++ = (c1 + (r - c1) * a) * 255.0;
	  *p1++ = (c1 + (g - c1) * a) * 255.0;
	  *p1++ = (c1 + (b - c1) * a) * 255.0;

	}
      
      if ((y / GIMP_CHECK_SIZE_SM) & 1)
	{
	  gdk_draw_rgb_image (pixmap, gc,
			      1, y + 1,
			      width, 1,
			      GDK_RGB_DITHER_NORMAL,
			      (guchar *) odd, 3);
	}
      else
	{
	  gdk_draw_rgb_image (pixmap, gc,
			      1, y + 1,
			      width, 1,
			      GDK_RGB_DITHER_NORMAL,
			      (guchar *) even, 3);
	}
      src += width * bpp;
    }

  g_free (even);
  g_free (odd);

  if(buf)
    temp_buf_free (buf);
  if(mbuf)
    mask_buf_free (mbuf);

  gtk_clist_set_row_data (idle->clist, idle->row, (gpointer)2);
  gtk_clist_set_pixmap (idle->clist, idle->row, 0, pixmap, NULL);
  gdk_pixmap_unref (pixmap);
  
  return (FALSE);
}

/* check if a preview is already made, otherwise gtk_idle_add the pixmap func */ 
static void
undo_history_set_pixmap (GtkCList *clist,
			 gint      row,
			 gint      size,
			 GImage   *gimage)
{
  static idle_preview_args idle;
  
  if (!size || ((gint)gtk_clist_get_row_data (clist, row)) == 2)
    return;
    
  idle.clist  = clist;
  idle.row    = row;
  idle.size   = size;
  idle.gimage = gimage;

  gtk_idle_add ((GtkFunction)undo_history_set_pixmap_idle, &idle);
}


/* close button clicked */
static void
undo_history_close_callback (GtkWidget *widget,
			     gpointer   data)
{
  undo_history_st *st = data;
  gtk_widget_hide (GTK_WIDGET (st->shell));
}

/* The gimage and shell destroy callbacks are split so we can:
 *   a) blow the shell when the image dissappears
 *   b) disconnect from the image if the shell dissappears (we don't
 *        want signals from the image to carry on using "st" once it's
 *        been freed.
 */

/* gimage renamed */
static void
undo_history_gimage_rename_callback (GimpImage *gimage,
				     gpointer   data)
{
  undo_history_st *st = data;
  gchar *title;

  title = g_strdup_printf (_("Undo History: %s"),
			   g_basename (gimage_filename (gimage)));
  gtk_window_set_title (GTK_WINDOW (st->shell), title);
  g_free (title);
}

/* gimage destroyed */
static void
undo_history_gimage_destroy_callback (GimpImage *gimage,
				      gpointer   data)
{
  undo_history_st *st = data;

  st->gimage = NULL;  /* not allowed to use this any more */
  dialog_unregister (st->shell);
  gtk_widget_destroy (GTK_WIDGET (st->shell));
  /* which continues in the function below: */
}

static void
undo_history_shell_destroy_callback (GtkWidget *widget,
				     gpointer   data)
{
  undo_history_st *st = data;

  if (st->gimage)
    gtk_signal_disconnect_by_data (GTK_OBJECT (st->gimage), st);
  g_free (st);
}

/* undo button clicked */
static void
undo_history_undo_callback (GtkWidget *widget,
			    gpointer   data)
{
  undo_history_st *st = data;

  undo_pop (st->gimage);
}

/* redo button clicked */
static void
undo_history_redo_callback (GtkWidget *widget,
			    gpointer   data)
{
  undo_history_st *st = data;

  undo_redo (st->gimage);
}


/* Always start clist with dummy entry for image state before
 * the first action on the undo stack */
static void
undo_history_prepend_special (GtkCList *clist)
{
  gchar *name = _("[ base image ]");
  gchar *namelist[3];
  gint   row;

  namelist[0] = NULL;
  namelist[1] = NULL;
  namelist[2] = name;

  row = gtk_clist_prepend (clist, namelist);
}


/* Recalculate which of the undo and redo buttons are meant to be sensitive */
static void
undo_history_set_sensitive (undo_history_st *st,
			    int              rows)
{
  gtk_widget_set_sensitive (st->undo_button, (st->old_selection != 0));
  gtk_widget_set_sensitive (st->redo_button, (st->old_selection != rows-1));
}


/* Track undo_event signals, telling us of changes to the undo and
 * redo stacks. */
static void
undo_history_undo_event (GtkWidget *widget,
			 int        ev,
			 gpointer   data)
{
  undo_history_st *st = data;
  undo_event_t event = ev;
  const char *name;
  char *namelist[3];
  GList *list;
  int cur_selection;
  GtkCList *clist;
  gint row;
  GdkPixmap *pixmap;
  GdkBitmap *mask;

  list = GTK_CLIST (st->clist)->selection;
  g_return_if_fail (list != NULL);
  cur_selection = GPOINTER_TO_INT (list->data);

  clist = GTK_CLIST (st->clist); 

  /* block select events */
  gtk_signal_handler_block_by_data (GTK_OBJECT (st->clist), st);

  switch (event)
    {
    case UNDO_PUSHED:
      /* clip everything after the current selection (ie, the
       * actions that are from the redo stack) */
      gtk_clist_freeze (clist);
      while (clist->rows > cur_selection + 1)
	gtk_clist_remove (clist, cur_selection + 1);

      /* find out what's new */
      name = undo_get_undo_name (st->gimage);
      namelist[0] = NULL;
      namelist[1] = NULL;
      namelist[2] = (char *) name;
      row = gtk_clist_append (clist, namelist);
      g_assert (clist->rows == cur_selection+2);

      undo_history_set_pixmap (clist, row, st->preview_size, st->gimage);

      /* always force selection to bottom, and scroll to it */
      gtk_clist_select_row (clist, clist->rows - 1, -1);
      gtk_clist_thaw (clist);
      gtk_clist_moveto (clist, clist->rows - 1, 0, 1.0, 0.0);
      cur_selection = clist->rows-1;
      break;

    case UNDO_EXPIRED:
      /* remove earliest row, but not our special first one */
      if (gtk_clist_get_pixmap (clist, 1, 0, &pixmap, &mask))
	gtk_clist_set_pixmap (clist, 0, 0, pixmap, mask);
      gtk_clist_remove (clist, 1);
      break;

    case UNDO_POPPED:
      /* move hilight up one */
      g_return_if_fail (cur_selection >= 1);
      gtk_clist_select_row (clist, cur_selection - 1, -1);
      cur_selection--;
      undo_history_set_pixmap (clist, cur_selection, st->preview_size, st->gimage);
      if ( !(gtk_clist_row_is_visible (clist, cur_selection) & GTK_VISIBILITY_FULL))
	gtk_clist_moveto (clist, cur_selection, 0, 0.0, 0.0);
      break;

     case UNDO_REDO:
       /* move hilight down one */
       g_return_if_fail (cur_selection+1 < clist->rows);
       gtk_clist_select_row (clist, cur_selection+1, -1);
       cur_selection++;
       undo_history_set_pixmap (clist, cur_selection, st->preview_size, st->gimage);
       if ( !(gtk_clist_row_is_visible (clist, cur_selection) & GTK_VISIBILITY_FULL))
	 gtk_clist_moveto (clist, cur_selection, 0, 1.0, 0.0);
       break;

    case UNDO_FREE:
      /* clear all info other that the special first line */
      gtk_clist_freeze (clist);
      gtk_clist_clear (clist);
      undo_history_prepend_special (clist);
      gtk_clist_thaw (clist);
      cur_selection = 0;
      break;
    }

  gtk_signal_handler_unblock_by_data (GTK_OBJECT (st->clist), st);

  st->old_selection = cur_selection;
  undo_history_set_sensitive (st, clist->rows);
}


static void
undo_history_select_row_callback (GtkWidget *widget,
				  gint       row,
				  gint       column,
				  gpointer   event,
				  gpointer   data)
{
  undo_history_st *st = data;
  int cur_selection;

  cur_selection = row;

  if (cur_selection == st->old_selection)
    return;

  /* Disable undo_event signals while we do these multiple undo or
   * redo actions. */
  gtk_signal_handler_block_by_func (GTK_OBJECT (st->gimage),
				    undo_history_undo_event, st);

  while (cur_selection < st->old_selection)
    {
      undo_pop (st->gimage);
      st->old_selection--;
    }
  while (cur_selection > st->old_selection)
    {
      undo_redo (st->gimage);
      st->old_selection++;
    }

  undo_history_set_pixmap (GTK_CLIST (widget), cur_selection, st->preview_size, st->gimage);

  gtk_signal_handler_unblock_by_func (GTK_OBJECT (st->gimage),
				      undo_history_undo_event, st);    

  undo_history_set_sensitive (st, GTK_CLIST(st->clist)->rows);
}


static void
undo_history_clean_callback (GtkWidget *widget,
			     gpointer   data)
{
  undo_history_st *st = data;
  int i;
  int nrows;
  GtkCList *clist;

  if (st->gimage->dirty != 0)
    return;

  /* The image is clean, so this is the version on disc.  Remove the
   * clean star from all other entries, and add it to the current
   * one. */

  /* XXX currently broken, since "clean" signal is emitted before
   * UNDO_POPPED event.  I don't want to change the order of the
   * signals.  So I'm a little stuck. --austin */

  clist = GTK_CLIST (st->clist);
  nrows = clist->rows;

  gtk_clist_freeze (clist);
  for (i=0; i < nrows; i++)
    gtk_clist_set_text (clist, i, 1, NULL);
  gtk_clist_set_pixmap (clist, st->old_selection, 1,
			clean_pixmap, clean_mask);
  gtk_clist_thaw (clist);
}


/* Used to build up initial contents of clist */
static int
undo_history_init_undo (const char *undoitemname,
			void       *data)
{
  undo_history_st *st = data;
  char *namelist[3];
  gint row;

  namelist[0] = NULL;
  namelist[1] = NULL;
  namelist[2] = (char *) undoitemname;
  row = gtk_clist_prepend (GTK_CLIST (st->clist), namelist);
  gtk_clist_set_pixmap (GTK_CLIST (st->clist), row, 0,
			clear_pixmap, clear_mask);

  return 0;
}

/* Ditto */
static int
undo_history_init_redo (const char *undoitemname,
			void       *data)
{
  undo_history_st *st = data;
  char *namelist[3];
  gint row;

  namelist[0] = NULL;  namelist[1] = NULL;
  namelist[2] = (char *) undoitemname;
  row = gtk_clist_append (GTK_CLIST (st->clist), namelist);
  gtk_clist_set_pixmap (GTK_CLIST (st->clist), row, 0,
			clear_pixmap, clear_mask);

  return 0;
}


/*************************************************************/
/* Publicly exported function */

GtkWidget *
undo_history_new (GImage *gimage)
{
  undo_history_st *st;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *scrolled_win;

  st = g_new0 (undo_history_st, 1);
  st->gimage = gimage;
  st->preview_size = preview_size;

  /*  gimage signals  */
  gtk_signal_connect (GTK_OBJECT (gimage), "undo_event",
		      GTK_SIGNAL_FUNC (undo_history_undo_event),
		      st);
  gtk_signal_connect (GTK_OBJECT (gimage), "rename",
		      GTK_SIGNAL_FUNC (undo_history_gimage_rename_callback),
		      st);
  gtk_signal_connect (GTK_OBJECT (gimage), "destroy",
		      GTK_SIGNAL_FUNC (undo_history_gimage_destroy_callback),
		      st);
  gtk_signal_connect (GTK_OBJECT (gimage), "clean",
		      GTK_SIGNAL_FUNC (undo_history_clean_callback),
		      st);

  /*  The shell and main vbox  */
  {
    gchar *title = g_strdup_printf (_("Undo History: %s"),
				    g_basename (gimage_filename (gimage)));
    st->shell = gimp_dialog_new (title, "undo_history",
				 gimp_standard_help_func,
				 "dialogs/undo_history.html",
				 GTK_WIN_POS_NONE,
				 FALSE, TRUE, FALSE,

				 _("Close"), undo_history_close_callback,
				 st, NULL, NULL, TRUE, TRUE,

				 NULL);
    dialog_register (st->shell);

    g_free (title);
  }

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (st->shell)->vbox), vbox);
  gtk_widget_show (vbox);

  gtk_signal_connect (GTK_OBJECT (st->shell), "destroy",
		      GTK_SIGNAL_FUNC (undo_history_shell_destroy_callback),
		      st);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (GTK_WIDGET (scrolled_win), 
			160 + st->preview_size,
			4 * (MAX (st->preview_size, 16) + 6));

  /* clist of undo actions */
  st->clist = gtk_clist_new (3);
  gtk_clist_set_selection_mode (GTK_CLIST (st->clist), GTK_SELECTION_BROWSE);
  gtk_clist_set_reorderable (GTK_CLIST (st->clist), FALSE);
  gtk_clist_set_row_height (GTK_CLIST (st->clist), MAX (st->preview_size, 16) + 4);
  gtk_clist_set_column_width (GTK_CLIST (st->clist), 0, st->preview_size + 2);
  gtk_clist_set_column_width (GTK_CLIST (st->clist), 1, 18);
  gtk_clist_set_column_min_width (GTK_CLIST (st->clist), 2, 64);

  /* allocate the pixmaps if not already done */
  if (!clean_pixmap)
    {
      GtkStyle *style;

      gtk_widget_realize (st->shell);
      style = gtk_widget_get_style (st->shell);

      clean_pixmap =
	gdk_pixmap_create_from_xpm_d (st->shell->window,
				      &clean_mask,
				      &style->bg[GTK_STATE_NORMAL],
				      yes_xpm);

      clear_pixmap =
	gdk_pixmap_create_from_xpm_d (st->shell->window,
				      &clear_mask,
				      &style->bg[GTK_STATE_NORMAL],
				      question_xpm);
   }

  /* work out the initial contents */
  undo_map_over_undo_stack (st->gimage, undo_history_init_undo, st);
  /* force selection to bottom */
  gtk_clist_select_row (GTK_CLIST (st->clist),
			GTK_CLIST (st->clist)->rows - 1, -1);
  undo_map_over_redo_stack (st->gimage, undo_history_init_redo, st);
  undo_history_prepend_special (GTK_CLIST (st->clist));
  st->old_selection = GPOINTER_TO_INT(GTK_CLIST(st->clist)->selection->data);

  /* draw the preview of the current state */
  undo_history_set_pixmap (GTK_CLIST (st->clist),
			   st->old_selection, st->preview_size, st->gimage);

  gtk_signal_connect (GTK_OBJECT (st->clist), "select_row",
		      GTK_SIGNAL_FUNC (undo_history_select_row_callback),
		      st);

  gtk_widget_show (GTK_WIDGET (st->clist));

  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (scrolled_win));
  gtk_container_add (GTK_CONTAINER (scrolled_win), st->clist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_ALWAYS);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  st->undo_button = button = gimp_pixmap_button_new (raise_xpm, _("Undo"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (undo_history_undo_callback),
		      st);
  gtk_widget_show (GTK_WIDGET (button));

  st->redo_button = button = gimp_pixmap_button_new (lower_xpm, _("Redo"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (undo_history_redo_callback),
		      st);
  gtk_widget_show (GTK_WIDGET (button));

  undo_history_set_sensitive (st, GTK_CLIST (st->clist)->rows);

  gtk_widget_show (GTK_WIDGET (st->shell));
  gtk_clist_moveto (GTK_CLIST (st->clist), st->old_selection, 0, 0.5, 0.0);

  return st->shell;
}
