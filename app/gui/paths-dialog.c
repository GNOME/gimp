/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Andy Thomas alt@picnic.demon.co.uk
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
 * Some of this code is based on the layers_dialog box code.
 */
#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>

#include "appenv.h"
#include "draw_core.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gimage.h"
#include "gimpimage.h"
#include "gimpdrawable.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "gimprc.h"
#include "gimpset.h"
#include "gimpui.h"
#include "image_render.h"
#include "lc_dialogP.h"
#include "menus.h"
#include "ops_buttons.h"
#include "bezier_select.h"
#include "bezier_selectP.h"
#include "path.h"
#include "pathP.h"
#include "path_transform.h"
#include "paths_dialog.h"
#include "paths_dialogP.h"
#include "undo.h"

#include "drawable_pvt.h"

#include "libgimp/gimpmath.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/new.xpm"
#include "pixmaps/duplicate.xpm"
#include "pixmaps/delete.xpm"
#include "pixmaps/pennorm.xpm"
#include "pixmaps/penadd.xpm"
#include "pixmaps/pendel.xpm"
#include "pixmaps/penedit.xpm"
#include "pixmaps/penstroke.xpm"
#include "pixmaps/toselection.xpm"
#include "pixmaps/topath.xpm"
#include "pixmaps/path.xbm"
#include "pixmaps/locked.xbm"

typedef struct _PathsDialog PathsDialog;

struct _PathsDialog
{
  GtkWidget *vbox;
  GtkWidget *paths_list;

  GtkWidget     *ops_menu;
  GtkAccelGroup *accel_group;

  gdouble ratio;
  gint    image_width, image_height;
  gint    gimage_width, gimage_height;

  /* pixmaps for the no preview bitmap */
  GdkPixmap *pixmap_normal;
  GdkPixmap *pixmap_selected;
  GdkPixmap *pixmap_locked_normal;
  GdkPixmap *pixmap_locked_selected;

  /*  state information  */
  gint       selsigid;
  GimpImage *gimage;
  GdkGC     *gc;   
  GdkColor   black;
  GdkColor   white;
  gint       selected_row_num;
  gboolean   been_selected;
  PathList  *current_path_list;
};

typedef struct _PathWidget PathWidget;

struct _PathWidget
{
  GdkPixmap *paths_pixmap;
  Path      *bzp;
};

typedef struct _PathCounts PathCounts;

struct _PathCounts
{
  CountCurves  c_count;             /* Must be the first element */
  gint         total_count;         /* Total number of curves    */
};

static PathsDialog *paths_dialog = NULL;
static Path        *copy_pp      = NULL;

static void     paths_dialog_realized        (GtkWidget      *widget);
static void     paths_select_row             (GtkWidget      *widget,
					      gint            row,
					      gint            column, 
					      GdkEventButton *event,
					      gpointer        data);
static void     paths_unselect_row           (GtkWidget      *widget,
					      gint            row,
					      gint            column,
					      GdkEventButton *event,
					      gpointer        data);
static gint     paths_list_events            (GtkWidget      *widget,
					      GdkEvent       *event);
static void     paths_dialog_map_callback    (GtkWidget      *widget,
					      gpointer        data);
static void     paths_dialog_unmap_callback  (GtkWidget      *widget,
					      gpointer        data);
static void     paths_update_paths           (gpointer        data,
					      gint            row);
static void     paths_update_preview         (BezierSelect   *bezier_sel);
static void     paths_dialog_preview_extents           (void);
static void     paths_dialog_new_point_callback        (GtkWidget *, gpointer);
static void     paths_dialog_add_point_callback        (GtkWidget *, gpointer);
static void     paths_dialog_delete_point_callback     (GtkWidget *, gpointer);
static void     paths_dialog_edit_point_callback       (GtkWidget *, gpointer);
static void     paths_dialog_advanced_to_path_callback (GtkWidget *, gpointer);
static void     paths_dialog_null_callback             (GtkWidget *, gpointer);

static void     path_close                   (Path *);

/*  the ops buttons  */
static GtkSignalFunc to_path_ext_callbacks[] = 
{ 
  paths_dialog_advanced_to_path_callback,          /* SHIFT */
  paths_dialog_null_callback,                      /* CTRL  */
  paths_dialog_null_callback,                      /* MOD1  */
  paths_dialog_null_callback,                      /* SHIFT + CTRL */
};

static OpsButton paths_ops_buttons[] =
{
  { new_xpm, paths_dialog_new_path_callback, NULL,
    N_("New Path"),
    "paths/new_path.html",
    NULL, 0 },
  { duplicate_xpm, paths_dialog_dup_path_callback, NULL,
    N_("Duplicate Path"),
    "paths/duplicate_path.html",
    NULL, 0 },
  { toselection_xpm, paths_dialog_path_to_sel_callback, NULL,
    N_("Path to Selection"),
    "paths/path_to_selection.html",
    NULL, 0 },
  { topath_xpm, paths_dialog_sel_to_path_callback, to_path_ext_callbacks,
    N_("Selection to Path"),
    "filters/sel2path.html",
    NULL, 0 },
  { penstroke_xpm, paths_dialog_stroke_path_callback, NULL,
    N_("Stroke Path"),
    "paths/stroke_path.html",
    NULL, 0 },
  { delete_xpm, paths_dialog_delete_path_callback, NULL,
    N_("Delete Path"),
    "paths/delete_path.html",
    NULL, 0 },
  { NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

static OpsButton point_ops_buttons[] =
{
  { pennorm_xpm, paths_dialog_new_point_callback, NULL,
    N_("New Point"),
    "#new_point_button",
    NULL, 0 },
  { penadd_xpm, paths_dialog_add_point_callback, NULL,
    N_("Add Point"),
    "#add_point_button",
    NULL, 0 },
  { pendel_xpm, paths_dialog_delete_point_callback, NULL,
    N_("Delete Point"),
    "#delete_point_button",
    NULL, 0 },
  { penedit_xpm, paths_dialog_edit_point_callback, NULL,
    N_("Edit Point"),
    "#edit_point_button",
    NULL, 0 },
  { NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

static void
paths_dialog_set_menu_sensitivity (void)
{
  gboolean gimage = FALSE;  /*  is there a gimage  */
  gboolean pp     = FALSE;  /*  paths present  */
  gboolean cpp    = FALSE;  /*  is there a path in the pate buffer  */

  if (! paths_dialog)
    return;

  if (paths_dialog->gimage)
    gimage = TRUE;

  if (gimage && gimp_image_get_paths (paths_dialog->gimage))
    pp = TRUE;

  if (copy_pp)
    cpp = TRUE;

#define SET_SENSITIVE(menu,condition) \
        menus_set_sensitive ("<Paths>/" menu, (condition) != 0)
#define SET_OPS_SENSITIVE(button,condition) \
        gtk_widget_set_sensitive (paths_ops_buttons[(button)].widget, \
                                 (condition) != 0)
#define SET_POINT_SENSITIVE(button,condition) \
        gtk_widget_set_sensitive (point_ops_buttons[(button)].widget, \
                                 (condition) != 0)

  SET_SENSITIVE ("New Path", gimage);
  SET_OPS_SENSITIVE (0, gimage);

  SET_SENSITIVE ("Duplicate Path", pp);
  SET_OPS_SENSITIVE (1, pp);

  SET_SENSITIVE ("Path to Selection", pp);
  SET_OPS_SENSITIVE (2, pp);

  SET_SENSITIVE ("Selection to Path", gimage);
  SET_OPS_SENSITIVE (3, gimage);

  SET_SENSITIVE ("Stroke Path", pp);
  SET_OPS_SENSITIVE (4, pp);

  SET_SENSITIVE ("Delete Path", pp);
  SET_OPS_SENSITIVE (5, pp);

  SET_SENSITIVE ("Copy Path", pp);
  SET_SENSITIVE ("Paste Path", pp && cpp);

  SET_SENSITIVE ("Import Path...", gimage);
  SET_SENSITIVE ("Export Path...", pp);

  SET_SENSITIVE ("Edit Path Attributes...", pp);

  /*  new point  */
  SET_POINT_SENSITIVE (0, pp);

  /*  add point  */
  SET_POINT_SENSITIVE (1, pp);

  /*  selete point  */
  SET_POINT_SENSITIVE (2, pp);

  /*  edit point  */
  SET_POINT_SENSITIVE (3, pp);

#undef SET_POINT_SENSITIVE
#undef SET_OPS_SENSITIVE
#undef SET_SENSITIVE
}

void
paths_dialog_set_default_op (void)
{
  if(paths_dialog != NULL) /* Bug #5049: Clients may call this because it is possible */
                           /* to create a path before the L&C dialog exists.          */
 
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (point_ops_buttons[0].widget),
	 			  TRUE);
}

GtkWidget *
paths_dialog_create (void)
{
  GtkWidget *vbox;
  GtkWidget *paths_list;
  GtkWidget *scrolled_win;  
  GtkWidget *button_box;  

  if (paths_dialog)
    return paths_dialog->vbox;

  paths_dialog = g_new0 (PathsDialog, 1);

  /*  The paths box  */
  paths_dialog->vbox = gtk_event_box_new ();

  gimp_help_set_help_data (paths_dialog->vbox, NULL,
			   "dialogs/paths/paths.html");

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (paths_dialog->vbox), vbox);

  /* The point operations */
  button_box = ops_button_box_new (point_ops_buttons, OPS_BUTTON_RADIO);
  gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, TRUE, 2);
  gtk_widget_show (button_box);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_widget_set_usize (scrolled_win, LIST_WIDTH, LIST_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 2);

  paths_dialog->paths_list = paths_list = gtk_clist_new (2);
  gtk_signal_connect (GTK_OBJECT (vbox), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroyed),
		      &paths_dialog);

  gtk_clist_set_selection_mode (GTK_CLIST (paths_list), GTK_SELECTION_BROWSE);
  gtk_clist_set_reorderable (GTK_CLIST (paths_list), FALSE);
  gtk_clist_set_column_width (GTK_CLIST (paths_list), 0, locked_width);
  gtk_clist_set_column_min_width (GTK_CLIST (paths_list), 1, 
				  LIST_WIDTH - locked_width - 4);
  gtk_clist_set_column_auto_resize (GTK_CLIST (paths_list), 1, TRUE);

  gtk_container_add (GTK_CONTAINER (scrolled_win), paths_list);
  gtk_signal_connect (GTK_OBJECT (paths_list), "event",
		      (GtkSignalFunc) paths_list_events,
		      paths_dialog);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (paths_list), 
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win))); 
  GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (scrolled_win)->vscrollbar,
			  GTK_CAN_FOCUS); 

  paths_dialog->selsigid =
    gtk_signal_connect (GTK_OBJECT (paths_list), "select_row",
			GTK_SIGNAL_FUNC (paths_select_row),
			NULL);

  gtk_signal_connect (GTK_OBJECT (paths_list), "unselect_row",
		      GTK_SIGNAL_FUNC (paths_unselect_row),
		      NULL);
      
  gtk_widget_show (scrolled_win);
  gtk_widget_show (paths_list);

  gtk_signal_connect (GTK_OBJECT (paths_dialog->vbox), "realize",
		      GTK_SIGNAL_FUNC (paths_dialog_realized),
		      NULL);

  gtk_widget_show (vbox);
  gtk_widget_show (paths_dialog->vbox);

  /*  The ops buttons  */
  button_box = ops_button_box_new (paths_ops_buttons, OPS_BUTTON_NORMAL);
  gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 2);
  gtk_widget_show (button_box);

  menus_get_paths_menu (&paths_dialog->ops_menu,
			&paths_dialog->accel_group);

  /*  Set up signals for map/unmap for the accelerators  */
  gtk_signal_connect (GTK_OBJECT (vbox), "map",
		      (GtkSignalFunc) paths_dialog_map_callback,
		      NULL);
  gtk_signal_connect (GTK_OBJECT (vbox), "unmap",
		      (GtkSignalFunc) paths_dialog_unmap_callback,
		      NULL);

  paths_dialog_set_menu_sensitivity ();

  paths_dialog_set_default_op ();
  
  return paths_dialog->vbox;
}

static void 
paths_dialog_realized (GtkWidget *widget)
{
  GdkColormap *colormap;
  gchar dash_list[2]= {3,3};

  /* Help out small displays */
  if (preview_size < 64)
    dash_list[1] = 1;

  paths_dialog->gc = gdk_gc_new (widget->window);
  gdk_gc_set_dashes (paths_dialog->gc, 2, dash_list, 2);
  colormap = gtk_widget_get_colormap (paths_dialog->paths_list);
  gdk_color_parse ("black", &paths_dialog->black);
  gdk_color_alloc (colormap, &paths_dialog->black);
  gdk_color_parse ("white", &paths_dialog->white);
  gdk_color_alloc (colormap, &paths_dialog->white);
}

/* Clears out row when list element is deleted/destroyed */
static void
clear_pathwidget (gpointer data)
{
  PathWidget *pwidget = data;

  if (pwidget)
    {
      if (pwidget->paths_pixmap)
	gdk_pixmap_unref (pwidget->paths_pixmap);

      g_free (pwidget);
    }
}


static Path*
path_dialog_new (GimpImage *gimage,
		 gint       name_seed, 
		 gpointer   data)
{
  Path  *bzp;
  gchar *s;

  s = g_strdup_printf (_("Path %d"), name_seed);
  bzp = path_new (gimage, BEZIER, (GSList *) data, 0, 0, 0, 0, s);

  return bzp;
}

static PathPoint*
path_start_last_seg (GSList *plist)
{
  PathPoint *retp = plist->data;
  while (plist)
    {
      if (((PathPoint*) (plist->data))->type == BEZIER_MOVE &&
	  g_slist_next (plist))
	{
	  plist = g_slist_next (plist);
	  retp = plist->data;
	}
      plist = g_slist_next (plist);
    }  
  return retp;
}

static void
path_close (Path *bzp)
{
  PathPoint *pdata;
  PathPoint *pathpoint;

  /* bzpaths are only really closed when converted to the BezierSelect ones */
  bzp->closed = 1;
  /* first point */
  pdata = (PathPoint*)bzp->path_details->data;

  if (g_slist_length (bzp->path_details) < 5)
    {
      int i;
      for (i = 0 ; i < 2 ; i++)
	{
	  pathpoint = g_new0 (PathPoint, 1);
	  pathpoint->type = (i & 1) ? BEZIER_ANCHOR : BEZIER_CONTROL;
	  pathpoint->x = pdata->x+i;
	  pathpoint->y = pdata->y+i;

	  bzp->path_details = g_slist_append (bzp->path_details, pathpoint);
	}
    }
  pathpoint = g_new0 (PathPoint, 1);
  pdata = path_start_last_seg (bzp->path_details);
  pathpoint->type = BEZIER_CONTROL;
  pathpoint->x = pdata->x;
  pathpoint->y = pdata->y;
  /* printf("Closing to x,y %d,%d\n",(gint)pdata->x,(gint)pdata->y); */
  bzp->path_details = g_slist_append (bzp->path_details, pathpoint);
  bzp->state = BEZIER_EDIT;
}

static void 
bz_change_name_row_to (gint   row,
		       gchar *text)
{
  PathWidget *pwidget;

  pwidget = (PathWidget *)
    gtk_clist_get_row_data (GTK_CLIST (paths_dialog->paths_list), row);

  if (!pwidget)
    return;

  g_free (pwidget->bzp->name);

  pwidget->bzp->name = g_strdup (text);
}

static void 
paths_set_dash_line (GdkGC    *gc,
		     gboolean  state)
{
  gdk_gc_set_foreground (paths_dialog->gc, &paths_dialog->black);

  if (state)
    {
      gdk_gc_set_line_attributes (gc, 0, GDK_LINE_ON_OFF_DASH,
				  GDK_CAP_BUTT, GDK_JOIN_ROUND);
    }
  else
    {
      gdk_gc_set_line_attributes (gc, 0, GDK_LINE_SOLID,
				  GDK_CAP_BUTT, GDK_JOIN_ROUND);
    }
}

static void 
clear_pixmap_preview (PathWidget *pwidget)
{
  guchar *rgb_buf;

  rgb_buf = g_new0 (guchar,
		    (paths_dialog->image_width + 4)
		    *(paths_dialog->image_height + 4) * 3);

  memset (rgb_buf,0xFF,(paths_dialog->image_width + 4)
	  *(paths_dialog->image_height + 4) * 3);

  gdk_draw_rgb_image (pwidget->paths_pixmap,
		      paths_dialog->gc,
		      0,
		      0,
		      paths_dialog->image_width + 4,
		      paths_dialog->image_height + 4,
		      GDK_RGB_DITHER_NORMAL,
		      rgb_buf,
		      (paths_dialog->image_width + 4)*3);

  paths_set_dash_line (paths_dialog->gc,FALSE);

  gdk_draw_rectangle (pwidget->paths_pixmap, 
		      paths_dialog->gc, FALSE, 0, 0, 
		      paths_dialog->image_width+3,
		      paths_dialog->image_height+3);

  gdk_draw_rectangle (pwidget->paths_pixmap, 
		      paths_dialog->gc, FALSE, 1, 1, 
		      paths_dialog->image_width+1,
		      paths_dialog->image_height+1);

  g_free (rgb_buf);
}

/* insrow == -1 -> append else insert at insrow */
void 
paths_add_path (Path *bzp,
		gint  insrow)
{
  /* Create a new entry in the list */
  PathWidget *pwidget;
  gint row;
  gchar *row_data[2];

  pwidget = g_new0 (PathWidget, 1);

  if (!GTK_WIDGET_REALIZED (paths_dialog->vbox))
    gtk_widget_realize (paths_dialog->vbox);

  paths_dialog_preview_extents ();

  if (preview_size)
    {
      /* Need to add this to the list */
      pwidget->paths_pixmap =  gdk_pixmap_new (paths_dialog->vbox->window,
					       paths_dialog->image_width + 4,  
					       paths_dialog->image_height + 4,
					       -1);
      clear_pixmap_preview (pwidget);
    }
  else
    {
      if (!paths_dialog->pixmap_normal)
	{
	  paths_dialog->pixmap_normal =
	    gdk_pixmap_create_from_data (paths_dialog->vbox->window,
					 path_bits, 
					 paths_dialog->image_width,
					 paths_dialog->image_height,
					 -1,
					 &paths_dialog->vbox->style->fg[GTK_STATE_SELECTED],
					 &paths_dialog->vbox->style->bg[GTK_STATE_SELECTED]);
	  paths_dialog->pixmap_selected =
	    gdk_pixmap_create_from_data (paths_dialog->vbox->window,
					 path_bits, 
					 paths_dialog->image_width,
					 paths_dialog->image_height,
					 -1,
					 &paths_dialog->vbox->style->fg[GTK_STATE_NORMAL],
					 &paths_dialog->vbox->style->bg[GTK_STATE_SELECTED]);
	}
       pwidget->paths_pixmap = paths_dialog->pixmap_normal;
    }

  if (!paths_dialog->pixmap_locked_normal)
    {
      paths_dialog->pixmap_locked_normal = 
	gdk_pixmap_create_from_data (paths_dialog->vbox->window,
				     locked_bits, locked_width, locked_height, -1,
				     &paths_dialog->vbox->style->fg[GTK_STATE_NORMAL],
				     &paths_dialog->vbox->style->white);
      paths_dialog->pixmap_locked_selected = 
	gdk_pixmap_create_from_data (paths_dialog->vbox->window,
				     locked_bits, locked_width, locked_height, -1,
				     &paths_dialog->vbox->style->fg[GTK_STATE_SELECTED],
				     &paths_dialog->vbox->style->bg[GTK_STATE_SELECTED]);
    }

  gtk_clist_set_row_height (GTK_CLIST (paths_dialog->paths_list),
			    paths_dialog->image_height + 6);

  row_data[0] = "";
  row_data[1] = "";

  if (insrow == -1)
    row = gtk_clist_append (GTK_CLIST (paths_dialog->paths_list),
			    row_data);
  else
    row = gtk_clist_insert (GTK_CLIST (paths_dialog->paths_list),
			    insrow,
			    row_data);

  gtk_clist_set_pixtext (GTK_CLIST (paths_dialog->paths_list),
			 row,
			 1,
			 bzp->name,
			 2,
			 pwidget->paths_pixmap,
			 NULL);

  gtk_clist_set_row_data_full (GTK_CLIST (paths_dialog->paths_list),
			       row,
			       (gpointer) pwidget,
			       clear_pathwidget);

  gtk_signal_handler_block (GTK_OBJECT (paths_dialog->paths_list),
			    paths_dialog->selsigid);
  gtk_clist_select_row (GTK_CLIST (paths_dialog->paths_list),
			paths_dialog->current_path_list->last_selected_row,
			1);
  gtk_signal_handler_unblock (GTK_OBJECT (paths_dialog->paths_list),
			      paths_dialog->selsigid);

  pwidget->bzp = bzp;
}

static void
paths_dialog_preview_extents (void)
{
  GimpImage *gimage;

  if (!paths_dialog)
    return;

 if (! (gimage = paths_dialog->gimage))
    return;

  paths_dialog->gimage_width  = gimage->width;
  paths_dialog->gimage_height = gimage->height;

  /*  Get the image width and height variables, based on the gimage  */
  if (gimage->width > gimage->height)
    paths_dialog->ratio = (double) preview_size / (double) gimage->width;
  else
    paths_dialog->ratio = (double) preview_size / (double) gimage->height;

  if (preview_size)
    {
      paths_dialog->image_width = (int) (paths_dialog->ratio * gimage->width);
      paths_dialog->image_height = (int) (paths_dialog->ratio * gimage->height);
      if (paths_dialog->image_width < 1) paths_dialog->image_width = 1;
      if (paths_dialog->image_height < 1) paths_dialog->image_height = 1;
    }
  else
    {
      paths_dialog->image_width  = path_width;
      paths_dialog->image_height = path_height;
    }
}

static void
paths_select_row (GtkWidget      *widget, 
		  gint            row,
		  gint            column,
		  GdkEventButton *event,
		  gpointer        data)
{
  PathWidget *pwidget;
  Path* bzp;
  BezierSelect * bsel;
  GDisplay *gdisp;
  gint last_row;

  pwidget = (PathWidget *) gtk_clist_get_row_data (GTK_CLIST (widget), row);

  if (!pwidget ||
      (paths_dialog->current_path_list->last_selected_row == row &&
       paths_dialog->been_selected == TRUE))
    {
      if (column)
	return;
    }

  last_row = paths_dialog->current_path_list->last_selected_row;

  bzp = (Path*)g_slist_nth_data(paths_dialog->current_path_list->bz_paths,row);

  g_return_if_fail(bzp != NULL);

  if(column == 0)
    {
      if(bzp->locked == 0)
	{
	  bzp->locked = 1;
	  if (paths_dialog->selected_row_num == row)
	    gtk_clist_set_pixmap (GTK_CLIST (paths_dialog->paths_list),
				  row,
				  0,
				  paths_dialog->pixmap_locked_selected,
				  NULL);
	  else
	    gtk_clist_set_pixmap (GTK_CLIST (paths_dialog->paths_list),
				  row,
				  0,
				  paths_dialog->pixmap_locked_normal,
				  NULL);
      	}
      else
	{
	  gint tmprow;

	  bzp->locked = 0;
	  gtk_clist_set_text (GTK_CLIST (paths_dialog->paths_list),
			      row,
			      0,
			      "");
	  /* There should be an easier way of updating the preview! */
	  bsel = path_to_beziersel (bzp);
	  tmprow = paths_dialog->current_path_list->last_selected_row;
	  paths_dialog->current_path_list->last_selected_row = row;
	  paths_update_preview (bsel);
	  bezier_select_free (bsel);
	  paths_dialog->current_path_list->last_selected_row = tmprow;
	  paths_dialog->selected_row_num = tmprow;
	}

      /* Put hightlight back on the old original selection */
      gtk_signal_handler_block (GTK_OBJECT (paths_dialog->paths_list),
				paths_dialog->selsigid);

      gtk_clist_select_row (GTK_CLIST (paths_dialog->paths_list),
			    last_row,
			    1);

      gtk_signal_handler_unblock (GTK_OBJECT (paths_dialog->paths_list),
				  paths_dialog->selsigid);

      return;
    }

  paths_dialog->selected_row_num = row;
  paths_dialog->current_path_list->last_selected_row = row;
  paths_dialog->been_selected = TRUE;

  if(bzp->locked)
    gtk_clist_set_pixmap (GTK_CLIST (paths_dialog->paths_list),
			  row,
			  0,
			  paths_dialog->pixmap_locked_selected,
			  NULL);

  bsel = path_to_beziersel (bzp);
  gdisp = gdisplays_check_valid (paths_dialog->current_path_list->gdisp,
				 paths_dialog->gimage);
  if (!gdisp)
    {
      /*g_warning("Lost image which bezier curve belonged to");*/
      return;
    }
  bezier_paste_bezierselect_to_current (gdisp, bsel);
  paths_update_preview (bsel);
  bezier_select_free (bsel);
}

static void
paths_unselect_row (GtkWidget      *widget, 
		    gint            row,
		    gint            column,
		    GdkEventButton *event,
		    gpointer        data)
{
  PathWidget *pwidget;
  Path* bzp;

  pwidget = (PathWidget *) gtk_clist_get_row_data (GTK_CLIST (widget), row);

  if (!pwidget)
    return;

  bzp = pwidget->bzp;

  g_return_if_fail (bzp != NULL);

  if (column && bzp->locked)
    {
      gtk_clist_set_pixmap (GTK_CLIST (paths_dialog->paths_list),
			    row,
			    0,
			    paths_dialog->pixmap_locked_normal,
			    NULL);
    }
}

void
paths_dialog_update (GimpImage* gimage)
{
  PathList* new_path_list;
  GSList *plist;
  gint loop;
  gint tmprow;

  if (!paths_dialog || !gimage)
    return;

  if (paths_dialog->gimage == gimage &&
      paths_dialog->current_path_list ==
      (PathList*) gimp_image_get_paths (gimage))
    return;

  paths_dialog->gimage = gimage;

  paths_dialog_preview_extents ();

  /*  clear clist out  */
  gtk_clist_freeze (GTK_CLIST (paths_dialog->paths_list));
  gtk_clist_clear (GTK_CLIST (paths_dialog->paths_list));
  gtk_clist_thaw (GTK_CLIST (paths_dialog->paths_list));

  /*  Find bz list  */
  new_path_list = (PathList*) gimp_image_get_paths (gimage);

  paths_dialog->current_path_list = new_path_list;
  paths_dialog->been_selected = FALSE;

  paths_dialog_set_menu_sensitivity ();

  paths_dialog_set_default_op ();

  if (!new_path_list)
    return;

  /* update the clist to reflect this images bz list */
  /* go around the image list populating the clist */

  if (gimage != new_path_list->gimage)
    {
      g_warning("paths list: internal list error");
    }

  plist = new_path_list->bz_paths;
  loop = 0;

  tmprow = paths_dialog->current_path_list->last_selected_row;
  while (plist)
    {
      paths_update_paths (plist->data,loop);
      loop++;
      plist = g_slist_next (plist);
    }
  paths_dialog->current_path_list->last_selected_row = tmprow;
  paths_dialog->selected_row_num = tmprow;

  /* select last one */

  gtk_signal_handler_block (GTK_OBJECT (paths_dialog->paths_list),
			    paths_dialog->selsigid);
  gtk_clist_select_row (GTK_CLIST (paths_dialog->paths_list),
			paths_dialog->current_path_list->last_selected_row,
			1);
  gtk_signal_handler_unblock (GTK_OBJECT (paths_dialog->paths_list),
			      paths_dialog->selsigid);

  gtk_clist_moveto (GTK_CLIST (paths_dialog->paths_list),
		    paths_dialog->current_path_list->last_selected_row,
		    0,
		    0.5,
		    0.0);
}

static void
paths_update_paths (gpointer data,
		    gint     row)
{
  Path* bzp;
  BezierSelect *bezier_sel;

  paths_add_path ((bzp = (Path*) data), -1);
  /* Now fudge the drawing....*/
  bezier_sel = path_to_beziersel (bzp);
  paths_dialog->current_path_list->last_selected_row = row;
  paths_update_preview (bezier_sel);
  bezier_select_free (bezier_sel);
 
  if (bzp->locked)
    {
      if (paths_dialog->selected_row_num == row)
	gtk_clist_set_pixmap (GTK_CLIST (paths_dialog->paths_list),
			      row,
			      0,
			      paths_dialog->pixmap_locked_selected,
			      NULL);
      else
	gtk_clist_set_pixmap (GTK_CLIST (paths_dialog->paths_list),
			      row,
			      0,
			      paths_dialog->pixmap_locked_normal,
			      NULL);
    }
}

static void
do_rename_paths_callback (GtkWidget *widget, 
			  gchar     *text,
			  gpointer   data)
{
  GdkBitmap *mask;
  guint8     spacing;
  GdkPixmap *pixmap;

  if (!(GTK_CLIST (data)->selection))
    return;

  gtk_clist_get_pixtext (GTK_CLIST (paths_dialog->paths_list),
			 paths_dialog->selected_row_num,
			 1,
			 NULL,
			 &spacing,
			 &pixmap,
			 &mask);

  gtk_clist_set_pixtext (GTK_CLIST (data),
			 paths_dialog->selected_row_num,
			 1,
			 text,
			 spacing,
			 pixmap,
			 mask);

  bz_change_name_row_to (paths_dialog->selected_row_num, text);
}

static void
paths_dialog_edit_path_query (GtkWidget *widget)
{
  GtkWidget *qbox;
  GdkBitmap *mask;
  gchar *text;
  gint   ret;

  /* Get the current name */
  ret = gtk_clist_get_pixtext (GTK_CLIST (paths_dialog->paths_list),
			       paths_dialog->selected_row_num,
			       1,
			       &text,
			       NULL,
			       NULL,
			       &mask);

  qbox = gimp_query_string_box (_("Edit Path Attributes"),
				gimp_standard_help_func,
				"paths/dialogs/rename_path.html",
				_("Enter a new name for the path"),
				text,
				NULL, NULL,
				do_rename_paths_callback, widget);
  gtk_widget_show (qbox);
}

static gint
paths_list_events (GtkWidget *widget,
		   GdkEvent  *event)
{
  GdkEventButton *bevent;
  static gint     last_row = -1;
  gint            this_column;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      if (!gtk_clist_get_selection_info (GTK_CLIST (paths_dialog->paths_list),
					 bevent->x,
					 bevent->y,
					 &last_row, &this_column))
	last_row = -1;
      else if (paths_dialog->selected_row_num != last_row)
	last_row = -1;

      if (bevent->button == 3)
	gtk_menu_popup (GTK_MENU (paths_dialog->ops_menu), 
			NULL, NULL, NULL, NULL, bevent->button, bevent->time);
      break;

    case GDK_2BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (last_row != -1 && 
	  gtk_clist_get_selection_info (GTK_CLIST (paths_dialog->paths_list),
					bevent->x,
					bevent->y,
					NULL, &this_column))
	{
	  if (this_column == 1)
	    {
	      paths_dialog_edit_path_query (widget);
	      return TRUE;
	    }
	  else
	    return FALSE;
	}
      else
	return FALSE;
      
    default:
      break;
    }
  return FALSE;
}

static PathList *
path_add_to_current (PathList  *pip,
		     Path      *bzp,
		     GimpImage *gimage,
		     gint       pos)
{
  /* add bzp to current list */
  if (!pip)
    {
      /* This image does not have a list */
      pip = path_list_new (gimage, 0, NULL);

      /* add to gimage */
      gimp_image_set_paths (gimage, pip);
    }

  if (pos < 0)
    pip->bz_paths = g_slist_append (pip->bz_paths,bzp);
  else
    pip->bz_paths = g_slist_insert (pip->bz_paths,bzp,pos);

  return pip;
}

static Path*
paths_dialog_new_path (PathList  **plp,
		       gpointer    points,
		       GimpImage  *gimage,
		       gint        pos)
{
  static gint nseed = 0;
  Path* bzp;

  bzp  = path_dialog_new (gimage, nseed++, points);
  *plp = path_add_to_current (*plp, bzp, gimage, pos);

  return (bzp);
}

void 
paths_dialog_new_path_callback (GtkWidget *widget, 
				gpointer   data)
{
  BezierSelect * bsel;
  GDisplay *gdisp;
  Path* bzp;

  bzp = paths_dialog_new_path (&paths_dialog->current_path_list,
			       NULL,
			       paths_dialog->gimage,
			       paths_dialog->selected_row_num);

  paths_add_path (bzp, paths_dialog->selected_row_num);

  paths_dialog_set_menu_sensitivity ();

  paths_dialog_set_default_op ();

  /* Clear the path display out */
  bsel = path_to_beziersel (bzp);
  gdisp = gdisplays_check_valid (paths_dialog->current_path_list->gdisp,
				 paths_dialog->gimage);
  bezier_paste_bezierselect_to_current (gdisp, bsel);
  bezier_select_free (bsel);
}

void 
paths_dialog_delete_path_callback (GtkWidget *widget, 
				   gpointer   udata)
{
  Path* bzp;
  PathList* plp;
  gboolean new_sz;
  gint row = paths_dialog->selected_row_num;
  BezierSelect *bsel  = NULL;
  GDisplay     *gdisp = NULL;

  g_return_if_fail (paths_dialog->current_path_list != NULL);

  /* Get current selection... ignore if none */
  if (paths_dialog->selected_row_num < 0)
    return;
  
  /* Get bzpath structure & delete its content */
  plp = paths_dialog->current_path_list;
  bzp = (Path*) g_slist_nth_data (plp->bz_paths, row); 

  /* Remove from list */
  plp->bz_paths = g_slist_remove (plp->bz_paths, bzp);
  new_sz = (g_slist_length (plp->bz_paths) > 0);
  path_free (bzp);

  /* Remove from the clist ... */ 
  gtk_signal_handler_block_by_data (GTK_OBJECT (paths_dialog->paths_list), 
				    paths_dialog);
  gtk_clist_remove (GTK_CLIST (paths_dialog->paths_list), row);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (paths_dialog->paths_list), 
				      paths_dialog);

  /* If now empty free everything up */
  if (!plp->bz_paths || g_slist_length (plp->bz_paths) == 0)
    {
      gtk_signal_disconnect (GTK_OBJECT (plp->gimage),
			     plp->sig_id);

      gimp_image_set_paths (plp->gimage, NULL);
      path_list_free (plp);

      /* Paste an empty BezierSelect to the current display to emulate an empty path list */

      bsel = g_new0 (BezierSelect, 1);
      bezier_select_reset (bsel);
      gdisp = gdisplays_check_valid (paths_dialog->current_path_list->gdisp,
				     paths_dialog->gimage);

      bezier_paste_bezierselect_to_current (gdisp, bsel);
      bezier_select_free (bsel);

      paths_dialog->current_path_list = NULL;

      paths_dialog_set_menu_sensitivity ();
    }

  if (!new_sz)
    paths_dialog_set_default_op ();
}

void 
paths_dialog_paste_path_callback (GtkWidget *widget, 
				  gpointer   data)
{
  Path* bzp;
  PathList* plp;
  PathPoint* pp;
  BezierSelect * bezier_sel;
  gint tmprow;
  GDisplay *gdisp;

  gint row = paths_dialog->selected_row_num;

  g_return_if_fail (paths_dialog->current_path_list != NULL);

  if (!copy_pp)
    return;

  /* Get current selection... ignore if none */
  if (paths_dialog->selected_row_num < 0)
    return;
  
  /* Get bzpath structure  */
  plp = paths_dialog->current_path_list;
  if (!plp)
    return;

  bzp = (Path*) g_slist_nth_data (plp->bz_paths, row); 

  if (bzp->path_details)
    {
      pp = bzp->path_details->data;
      pp->type = BEZIER_MOVE;
      bzp->path_details = g_slist_concat (copy_pp->path_details,
					  bzp->path_details);
    }
  else
    {
      bzp->closed = TRUE;
      bzp->path_details = copy_pp->path_details;
      bzp->state = copy_pp->state;
    }

  /* First point on new curve is a moveto */
  copy_pp->path_details = NULL;
  path_free (copy_pp);
  copy_pp = NULL;

  paths_dialog_set_menu_sensitivity ();

  /* Now fudge the drawing....*/
  bezier_sel = path_to_beziersel (bzp);
  tmprow = paths_dialog->current_path_list->last_selected_row;
  paths_dialog->current_path_list->last_selected_row = row;
  gdisp = gdisplays_check_valid (paths_dialog->current_path_list->gdisp,
				 paths_dialog->gimage);
  bezier_paste_bezierselect_to_current (gdisp, bezier_sel);
  paths_update_preview (bezier_sel);
  bezier_select_free (bezier_sel);
  paths_dialog->current_path_list->last_selected_row = tmprow;
}

void 
paths_dialog_copy_path_callback (GtkWidget *widget,
				 gpointer   data)
{
  Path* bzp;
  PathList* plp;
  gint row = paths_dialog->selected_row_num;

  g_return_if_fail (paths_dialog->current_path_list != NULL);

  /* Get current selection... ignore if none */
  if (paths_dialog->selected_row_num < 0)
    return;
  
  /* Get bzpath structure  */
  plp = paths_dialog->current_path_list;
  bzp = (Path*) g_slist_nth_data (plp->bz_paths, row);

  if (!bzp->path_details || g_slist_length (bzp->path_details) <= 5)
    return;

  /* And store in static array */
  copy_pp = path_copy (paths_dialog->gimage, bzp);

  /* All paths that are in the cut buffer must be closed */
  if (!copy_pp->closed)
    path_close (copy_pp);

  paths_dialog_set_menu_sensitivity ();
}

void 
paths_dialog_dup_path_callback (GtkWidget *widget, 
				gpointer   data)
{
  Path* bzp;
  PathList* plp;
  BezierSelect * bezier_sel;
  gint row;
  gint tmprow;

  g_return_if_fail (paths_dialog->current_path_list != NULL);

  /* Get current selection... ignore if none */
  if (paths_dialog->selected_row_num < 0)
    return;
  
  row = paths_dialog->current_path_list->last_selected_row;
  /* Get bzpath structure  */
  plp = paths_dialog->current_path_list;
  bzp = (Path*) g_slist_nth_data (plp->bz_paths, row); 

  /* Insert at the current position */
  bzp = path_copy (paths_dialog->gimage, bzp);
  plp->bz_paths = g_slist_insert (plp->bz_paths, bzp, row);
  paths_add_path (bzp, row);

  /* Now fudge the drawing....*/
  bezier_sel = path_to_beziersel (bzp);
  tmprow = paths_dialog->current_path_list->last_selected_row;
  paths_dialog->current_path_list->last_selected_row = row;
  paths_update_preview (bezier_sel);
  bezier_select_free (bezier_sel);
  paths_dialog->current_path_list->last_selected_row = tmprow;
}

static void 
paths_dialog_advanced_to_path_callback (GtkWidget *widget, 
					gpointer   data)
{
  ProcRecord *proc_rec;
  Argument   *args;
  GimpImage *gimage;

  /*  find the sel2path PDB record  */
  if ((proc_rec = procedural_db_lookup ("plug_in_sel2path_advanced")) == NULL)
    {
      g_message ("paths_dialog_adavanced_to_path_callback(): selection to path (advanced) procedure lookup failed");
      return;
    }

  gimage = paths_dialog->gimage;

  /*  plug-in arguments as if called by <Image>/Filters/...  */
  args = g_new (Argument, 3);
  args[0].arg_type = PDB_INT32;
  args[0].value.pdb_int = RUN_INTERACTIVE;
  args[1].arg_type = PDB_IMAGE;
  args[1].value.pdb_int = (gint32) pdb_image_to_id (gimage);
  args[2].arg_type = PDB_DRAWABLE;
  args[2].value.pdb_int = (gint32) (gimage_active_drawable (gimage))->ID;

  plug_in_run (proc_rec, args, 3, FALSE, TRUE,
	       (gimage_active_drawable (gimage))->ID);

  g_free (args);

}

static void 
paths_dialog_null_callback (GtkWidget *widget, 
			    gpointer   data)
{
  /* Maybe some more here later? */
}

void 
paths_dialog_sel_to_path_callback (GtkWidget *widget, 
				   gpointer   data)
{
  ProcRecord *proc_rec;
  Argument   *args;
  GimpImage *gimage;

  /*  find the sel2path PDB record  */
  if ((proc_rec = procedural_db_lookup ("plug_in_sel2path")) == NULL)
    {
      g_message ("paths_dialog_sel_to_path_callback(): selection to path procedure lookup failed");
      return;
    }

  gimage = paths_dialog->gimage;

  /*  plug-in arguments as if called by <Image>/Filters/...  */
  args = g_new (Argument, 3);
  args[0].arg_type = PDB_INT32;
  args[0].value.pdb_int = RUN_INTERACTIVE;
  args[1].arg_type = PDB_IMAGE;
  args[1].value.pdb_int = (gint32) pdb_image_to_id (gimage);
  args[2].arg_type = PDB_DRAWABLE;
  args[2].value.pdb_int = (gint32) (gimage_active_drawable (gimage))->ID;

  plug_in_run (proc_rec, args, 3, FALSE, TRUE,
	       (gimage_active_drawable (gimage))->ID);

  g_free (args);
}


void 
paths_dialog_path_to_sel_callback (GtkWidget *widget, 
				   gpointer   data)
{
  Path* bzp;
  PathList* plp;
  BezierSelect * bezier_sel;
  GDisplay  * gdisp;
  gint row = paths_dialog->selected_row_num;

  g_return_if_fail (paths_dialog->current_path_list != NULL);

  /* Get current selection... ignore if none */
  if (paths_dialog->selected_row_num < 0)
    return;
  
  /* Get bzpath structure  */
  plp = paths_dialog->current_path_list;
  bzp = (Path*) g_slist_nth_data (plp->bz_paths, row); 

  /* Return if no point list */
  if (!bzp->path_details)
    return;

  /* Now do the selection....*/
  gdisp = gdisplays_check_valid (paths_dialog->current_path_list->gdisp,
				 paths_dialog->gimage);

  if (!bzp->closed)
    {
      Path* bzpcopy = path_copy (paths_dialog->gimage,bzp);
      /* Close it */
      path_close (bzpcopy);
      bezier_sel = path_to_beziersel (bzpcopy);
      path_free (bzpcopy);
      bezier_to_selection (bezier_sel, gdisp);
      bezier_select_free (bezier_sel);

      /* Force display to show no closed curve */
      bezier_sel = path_to_beziersel (bzp);
      bezier_paste_bezierselect_to_current (gdisp, bezier_sel);
      bezier_select_free (bezier_sel);
    }
  else
    {
      bezier_sel = path_to_beziersel (bzp);
      bezier_to_selection (bezier_sel, gdisp);
      bezier_select_free (bezier_sel);
    }
}

void 
paths_dialog_stroke_path_callback (GtkWidget *widget, 
				   gpointer   data)
{
  Path* bzp;
  PathList* plp;
  gint row = paths_dialog->selected_row_num;

  g_return_if_fail (paths_dialog->current_path_list != NULL);

  /* Get current selection... ignore if none */
  if (paths_dialog->selected_row_num < 0)
    return;
  
  /* Get bzpath structure  */
  plp = paths_dialog->current_path_list;
  bzp = (Path*) g_slist_nth_data (plp->bz_paths, row); 

  /* Now do the stroke....*/
  path_stroke (paths_dialog->gimage, paths_dialog->current_path_list, bzp);
}

void
paths_dialog_edit_path_attributes_callback (GtkWidget *widget,
					    gpointer   data)
{
  if (paths_dialog && paths_dialog->paths_list)
    paths_dialog_edit_path_query (paths_dialog->paths_list);
}

static void
paths_dialog_map_callback (GtkWidget *widget,
			   gpointer   data)
{
  if (!paths_dialog)
    return;

  gtk_window_add_accel_group (GTK_WINDOW (lc_dialog->shell),
			      paths_dialog->accel_group);

  paths_dialog_preview_extents ();
}

static void
paths_dialog_unmap_callback (GtkWidget *widget,
			     gpointer   data)
{
  if (!paths_dialog)
    return;
  
  gtk_window_remove_accel_group (GTK_WINDOW (lc_dialog->shell),
				 paths_dialog->accel_group);
}

void
paths_dialog_destroy_cb (GtkObject *object,
			 gpointer   data)
{
  GimpImage *gimage = (GimpImage *)object;
  PathList* new_path_list;

  if (!paths_dialog)
    return;

  if (paths_dialog->current_path_list && 
      gimage == paths_dialog->current_path_list->gimage)
    {
      /* showing could be last so remove here.. might get 
	 done again if not the last one
      */
      paths_dialog->current_path_list = NULL;
      paths_dialog->been_selected = FALSE;

      gtk_clist_freeze (GTK_CLIST (paths_dialog->paths_list));
      gtk_clist_clear (GTK_CLIST (paths_dialog->paths_list));
      gtk_clist_thaw (GTK_CLIST (paths_dialog->paths_list));
    }

  /* Find bz list */  
  new_path_list = (PathList*) gimp_image_get_paths (gimage);

  if (!new_path_list)
    return; /* Already removed - signal handler just left in the air */

  path_list_free (new_path_list);

  gimp_image_set_paths (gimage, NULL);
}

/* Functions used from the bezier code .. tie in with this code */
static GSList *
pathpoints_create (BezierSelect *sel)
{
  gint i;
  GSList *list = NULL;
  PathPoint *pathpoint;
  BezierPoint *pts = (BezierPoint *) sel->points;
  BezierPoint *start_pnt = pts;
  gint need_move = 0;

  for (i=0; i< sel->num_points; i++)
    {
      pathpoint = path_point_new ((need_move)?BEZIER_MOVE:pts->type,
				  (gdouble)pts->x,(gdouble)pts->y);
      need_move = 0;
      list = g_slist_append (list, pathpoint);
      if(pts->next_curve)
	{
	  /* The curve must loop back on itself */
	  if(start_pnt != pts->next)
	    g_warning("Curve of of sync");
	  
	  need_move = 1;
	  pts = pts->next_curve;
	  start_pnt = pts;
	}
      else
	{
	  pts = pts->next;
	}
    }
  return (list);
}

GSList *
pathpoints_copy (GSList *list)
{
  GSList *slcopy = NULL;
  PathPoint* pdata;
  PathPoint* pathpoint;
  while(list)
    {
      pathpoint = g_new0(PathPoint,1);
      pdata = (PathPoint*)list->data;
      pathpoint->type = pdata->type;
      pathpoint->x = pdata->x;
      pathpoint->y = pdata->y;
      slcopy = g_slist_append(slcopy,pathpoint);
      list = g_slist_next(list);
    }
  return slcopy;
}

void
pathpoints_free (GSList *list)
{
  if (!list)
    return;

  g_slist_foreach (list, (GFunc)path_point_free, NULL);
  g_slist_free (list);
}

static void
paths_update_bzpath (PathList     *plp,
		     BezierSelect *bezier_sel)
{
  Path* p;

  p = (Path*)g_slist_nth_data(plp->bz_paths,plp->last_selected_row);
  
  if(p->path_details) 
    pathpoints_free (p->path_details); 
  
  p->path_details = pathpoints_create(bezier_sel);
  p->closed = bezier_sel->closed;
  p->state  = bezier_sel->state;
}

static gboolean
paths_replaced_current (PathList     *plp,
			BezierSelect *bezier_sel)
{
  /* Is there a currently selected path in this image? */
  /* ALT if(paths_dialog && plp &&  */
  if(plp && 
     plp->last_selected_row >= 0)
    {  
      paths_update_bzpath(plp,bezier_sel);
      return TRUE;
    }
  return FALSE;
}

static gint 
number_curves_in_path (GSList *plist)
{
  gint count = 0;
  while(plist)
    {
      if(((PathPoint*)(plist->data))->type == BEZIER_MOVE &&
	 g_slist_next(plist))
	{
	  count++;
	}
      plist = g_slist_next(plist);
    }  
  return count;
}

static void 
paths_draw_segment_points (BezierSelect *bezier_sel, 
			   GdkPoint     *pnt, 
			   int           npoints,
			   gpointer      udata)
{
  /* 
   * hopefully the image points are already in image space co-ords.
   * so just scale by ratio factor and draw 'em
   */
  gint loop;
  gint pcount = 0;
  GdkPoint * copy_pnt = g_new(GdkPoint,npoints);
  GdkPoint * cur_pnt  = copy_pnt;
  GdkPoint * last_pnt  = NULL;
  PathWidget *pwidget;
  gint row;
  PathCounts *curve_count = (PathCounts *) udata;

  /* we could remove duplicate points here */

  for(loop = 0; loop < npoints; loop++)
    {
      /* The "2" is because we have a boarder */
      cur_pnt->x = 2+(int) (paths_dialog->ratio * pnt->x);
      cur_pnt->y = 2+(int) (paths_dialog->ratio * pnt->y);
      pnt++;
      if(last_pnt &&
	 last_pnt->x == cur_pnt->x &&
	 last_pnt->y == cur_pnt->y)
	{
	  /* same as last ... don't need this one */
	  continue;
	}

/*       printf("converting %d [%d,%d] => [%d,%d]\n", */
/* 	     pcount,(int)pnt->x,(int)pnt->y,(int)cur_pnt->x,(int)cur_pnt->y); */
      last_pnt = cur_pnt;
      pcount++;
      cur_pnt++;
    }

  row = paths_dialog->current_path_list->last_selected_row;

  pwidget = (PathWidget *)
    gtk_clist_get_row_data (GTK_CLIST (paths_dialog->paths_list), row);
  
  if(pcount < 2)
    return;

  g_return_if_fail(pwidget != NULL);

  if(curve_count->c_count.count < curve_count->total_count || 
     bezier_sel->closed)
    paths_set_dash_line(paths_dialog->gc,FALSE);
  else
    paths_set_dash_line(paths_dialog->gc,TRUE);
  
  gdk_draw_lines (pwidget->paths_pixmap,
		   paths_dialog->gc, copy_pnt, pcount);

  g_free(copy_pnt);
}

static void
paths_update_preview (BezierSelect *bezier_sel)
{
  gint row;
  PathCounts curve_count;

  if(paths_dialog &&
     paths_dialog->current_path_list &&
     (row = paths_dialog->current_path_list->last_selected_row) >= 0 &&
     preview_size)
    {
      PathWidget *pwidget;

      pwidget = (PathWidget *)
	gtk_clist_get_row_data (GTK_CLIST (paths_dialog->paths_list), row);

      /* Clear pixmap */
      clear_pixmap_preview (pwidget);

      curve_count.total_count =
	number_curves_in_path (pwidget->bzp->path_details);

      /* update .. */
      bezier_draw_curve (bezier_sel, paths_draw_segment_points,
			 IMAGE_COORDS, &curve_count);

      /* update the pixmap */
      gtk_clist_set_pixtext (GTK_CLIST (paths_dialog->paths_list),
			     row,
			     1,
			     pwidget->bzp->name,
			     2,
			     pwidget->paths_pixmap,
			     NULL);
    }
}

static void 
paths_dialog_new_point_callback (GtkWidget *widget, 
				 gpointer   udata)
{
  bezier_select_mode (EXTEND_NEW);
}

static void 
paths_dialog_add_point_callback (GtkWidget *widget, 
				 gpointer   udata)
{
  bezier_select_mode (EXTEND_ADD);
}

static void 
paths_dialog_delete_point_callback (GtkWidget *widget, 
				    gpointer   udata)
{
  bezier_select_mode (EXTEND_REMOVE);
}

static void 
paths_dialog_edit_point_callback (GtkWidget *widget, 
				  gpointer   udata)
{
  bezier_select_mode (EXTEND_EDIT);
}

void
paths_dialog_flush (void)
{
  GimpImage *gimage;

  if (!paths_dialog)
    return;

 if (!(gimage = paths_dialog->gimage))
    return;

  gimage = paths_dialog->gimage;

  /* Check current_path_list since we might not have a valid preview.
   * which means it should be removed.. Or if we have one
   * created it!
   */
  if ((paths_dialog->current_path_list == NULL) ||
      (gimage->width != paths_dialog->gimage_width) ||
      (gimage->height != paths_dialog->gimage_height))
    {
      paths_dialog->gimage = NULL;
      paths_dialog_update (gimage);
    }
}

void 
paths_first_button_press (BezierSelect *bezier_sel,
			  GDisplay     *gdisp)
{
  /* First time a button is pressed in this display */
  /* We have two choices here 
     Either:-
     1) We already have a paths item in the list. 
        => In this case the new one replaces the current entry. We
	need a callback into the bezier code to free things up.
     2) We don't have an entry. 
        => Create a new one and add this curve.

     In either case we need to update the preview widget..

     All this of course depends on the fact that gdisp is the same
     as before. 
  */
  Path* bzp; 
  PathList* plp;

  if(paths_dialog)
    {
      paths_dialog->been_selected = FALSE;
      /*ALT return;*/
    }

  /* Button not pressed in this image...
   * find which one it was pressed in if any.
   */
  plp = (PathList*)gimp_image_get_paths(gdisp->gimage);      

  /* Since beziers are part of the save format.. make the image dirty */
/*   undo_push_cantundo(gdisp->gimage, _("path modification")); */
  
  if(!paths_replaced_current(plp,bezier_sel))
    {
      bzp = paths_dialog_new_path (&plp, pathpoints_create(bezier_sel),gdisp->gimage,-1);
      bzp->closed = bezier_sel->closed;
      bzp->state  = bezier_sel->state;
      if(paths_dialog && paths_dialog->gimage == gdisp->gimage)
	{
	  paths_dialog->current_path_list = plp;
	  paths_add_path(bzp,-1);
	}
    }
}

void
paths_newpoint_current (BezierSelect *bezier_sel,
			GDisplay     *gdisp)
{
  /*  Check if currently showing the paths we are updating  */
  if (paths_dialog &&
      gdisp->gimage == paths_dialog->gimage)
    {
      paths_dialog_set_menu_sensitivity ();

      paths_update_preview (bezier_sel);
    }

  paths_first_button_press (bezier_sel, gdisp);
}

void 
paths_new_bezier_select_tool (void)
{
  if (paths_dialog)
    paths_dialog->been_selected = FALSE;
}


/**************************************************************/
/* Code to save/load from filesystem                          */
/**************************************************************/

static GtkWidget *file_dlg = 0;
static int load_store;

static void
path_write_current_to_file (FILE  *f,
			    Path*  bzp)
{
  GSList *list = bzp->path_details;
  PathPoint* pdata;

  fprintf(f, "Name: %s\n", bzp->name);
  fprintf(f, "#POINTS: %d\n", g_slist_length(bzp->path_details));
  fprintf(f, "CLOSED: %d\n", bzp->closed==1?1:0);
  fprintf(f, "DRAW: %d\n", 0);
  fprintf(f, "STATE: %d\n", bzp->state);

  while (list)
    {
      pdata = (PathPoint*)list->data;
      fprintf(f,"TYPE: %d X: %d Y: %d\n", pdata->type, (gint)pdata->x, (gint)pdata->y);
      list = g_slist_next(list);
    }
}

static void
file_ok_callback (GtkWidget *widget,
		  gpointer   client_data) 
{
  GtkFileSelection *fs;
  FILE *f; 
  char* filename;
  Path* bzpath;
  GSList * pts_list = NULL;
  PathList* plp;
  gint row = paths_dialog->selected_row_num;
  gint this_path_count = 0;

  fs = GTK_FILE_SELECTION (file_dlg);
  filename = gtk_file_selection_get_filename (fs);

  if (load_store) 
    {
      f = fopen(filename, "rb");

      if(!f)
	{
	  g_message(_("Unable to open file %s"),filename);
	  return;
	}
      
      while(!feof(f))
	{
	  gchar *txt = g_new(gchar,512);
	  gchar *txtstart = txt;
	  gint readfields = 0;
	  int val, type, closed, i, draw, state;
	  double x,y;

	  if(!fgets(txt,512,f) || strlen(txt) < 7)
	    {
	      g_message(_("Failed to read from %s"),filename);
	      gtk_widget_hide (file_dlg);  
	      return;
	    }

	  txt += 6; /* Miss out 'Name: ' bit */
	  txt[strlen(txt)-1] = '\0';

	  readfields += fscanf(f, "#POINTS: %d\n", &val);
 	  readfields += fscanf(f, "CLOSED: %d\n", &closed);
	  readfields += fscanf(f, "DRAW: %d\n", &draw);
	  readfields += fscanf(f, "STATE: %d\n", &state);

	  if(readfields != 4)
	    {
	      g_message(_("Failed to read path from %s"),filename);
	      gtk_widget_hide (file_dlg);  
	      return;
	    }

	  if(val <= 0)
	    {
	      g_message(_("No points specified in path file %s"),filename);
	      gtk_widget_hide (file_dlg);  
	      return;
	    }

	  for(i=0; i< val; i++)
	    {
	      PathPoint* bpt;
	      readfields = fscanf(f,"TYPE: %d X: %lg Y: %lg\n", &type, &x, &y);
	      if(readfields != 3)
		{
		  g_message(_("Failed to read path points from %s"),filename);
		  gtk_widget_hide (file_dlg);  
		  return;
		}
	      this_path_count++;
	      switch(type)
		{
		case BEZIER_ANCHOR:
		case BEZIER_CONTROL:
		  break;
		case BEZIER_MOVE:
		  if(this_path_count < 6)
		    {
		      g_warning("Invalid single point in path\n");
		      gtk_widget_hide (file_dlg);  
		      return;
		    }
		  this_path_count = 0;
		  break;
		default:
		  g_warning("Invalid point type passed\n");
		  gtk_widget_hide (file_dlg);  
		  return;
		}

	      bpt = path_point_new (type, (gdouble)x, (gdouble)y);
	      pts_list = g_slist_append(pts_list,bpt);
	    }

	  bzpath = path_new(paths_dialog->gimage,
			    BEZIER,
			    pts_list,
			    closed,
			    state,
			    0, /* Can't be locked */
			    0, /* No tattoo assigned */
			    txt);
	  
	  g_free(txtstart);

	  paths_dialog->current_path_list = 
	    path_add_to_current(paths_dialog->current_path_list,
				  bzpath,
				  paths_dialog->gimage,
				  row);
	  paths_add_path (bzpath, row);

	  gtk_clist_select_row(GTK_CLIST(paths_dialog->paths_list),
			       paths_dialog->current_path_list->last_selected_row,
			       1);

	  paths_dialog_set_menu_sensitivity ();

	  if (!val)
	    paths_dialog_set_default_op ();
	}
      fclose (f);
    } 
  else 
    {
      Path* bzp;

      /* Get current selection... ignore if none */
      if(paths_dialog->selected_row_num < 0)
	return;
      
      /* Get bzpath structure  */
      plp = paths_dialog->current_path_list;
      bzp = (Path*)g_slist_nth_data(plp->bz_paths,row); 

      f = fopen(filename, "wb");
      if (NULL == f) 
	{
	  g_message (_("open failed on %s: %s\n"), filename, g_strerror(errno));
	  return;
	}

      /* Write the current selection out. */
      path_write_current_to_file (f,bzp);

      fclose (f);
    }
  gtk_widget_hide (file_dlg);  
}

static void
file_cancel_callback (GtkWidget *widget,
		      gpointer   data) 
{
  gtk_widget_hide (file_dlg);
}

static void
make_file_dlg (gpointer data) 
{
  file_dlg = gtk_file_selection_new (_("Load/Store Bezier Curves"));
  gtk_window_set_wmclass (GTK_WINDOW (file_dlg), "load_save_path", "Gimp");
  gtk_window_set_position (GTK_WINDOW (file_dlg), GTK_WIN_POS_MOUSE);

  gtk_container_set_border_width (GTK_CONTAINER (file_dlg), 2);
  gtk_container_set_border_width (GTK_CONTAINER (GTK_FILE_SELECTION (file_dlg)->button_area), 2);

  gtk_signal_connect
    (GTK_OBJECT (GTK_FILE_SELECTION (file_dlg)->cancel_button), "clicked",
     GTK_SIGNAL_FUNC (file_cancel_callback),
     data);
  gtk_signal_connect
    (GTK_OBJECT (GTK_FILE_SELECTION (file_dlg)->ok_button), "clicked",
     GTK_SIGNAL_FUNC (file_ok_callback),
     data);
  gtk_signal_connect (GTK_OBJECT (file_dlg), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_widget_hide),
		      NULL);

  /*  Connect the "F1" help key  */
  gimp_help_connect_help_accel (file_dlg, gimp_standard_help_func, NULL);
}

void 
paths_dialog_import_path_callback (GtkWidget *widget,
				   gpointer   data)
{
  /* Read and add at current position */
  if (!file_dlg) 
    {
      make_file_dlg (NULL);
    } 
  else 
    {
      if (GTK_WIDGET_VISIBLE (file_dlg))
	return;
    }

  gimp_help_set_help_data (file_dlg, NULL, "paths/dialogs/import_path.html");

  gtk_window_set_title (GTK_WINDOW (file_dlg), _("Load Path"));
  load_store = 1;
  gtk_widget_show (file_dlg);
}

void 
paths_dialog_export_path_callback (GtkWidget *widget,
				   gpointer   data)
{
  /* Export the path to a file */
  if (!file_dlg) 
    {
      make_file_dlg (NULL);
    } 
  else 
    {
      if (GTK_WIDGET_VISIBLE (file_dlg))
	return;
    }

  gimp_help_set_help_data (file_dlg, NULL, "paths/dialogs/export_path.html");

  gtk_window_set_title (GTK_WINDOW (file_dlg), _("Store Path"));
  load_store = 0;
  gtk_widget_show (file_dlg);
}

/*************************************/
/* Function for transforming paths   */
/*************************************/

/* These functions are the undo functions for the paths
 * that have undergone transformations. 
 *
 * Generally speaking paths do not belong with the undo 
 * structures. However when a path undergoes a transformation 
 * then THIS path transformation should be part of the undo.
 * We do have a problem here since a point could have been
 * added to the path after the transformation. This 
 * point will be lost if the undo stuff is performed. It would 
 * then appear that this point is part of the undo structure.
 * I think it is fair that this happens since the user is telling
 * us to restore the state before the transformation took place.
 * Note tattoos are used to find which paths have been stored in the
 * undo buffer. So deleted paths will not suddenly reappear. (I did say
 * generally paths are not part of the undo structures).
 */
PathUndo *
path_transform_start_undo (GimpImage *gimage)
{
  /* Save only the locked paths away */
  PathList *plp;
  GSList   *plist;
  Path*     p;
  Path*     p_copy;
  GSList   *undo_list = NULL;

  /* Get bzpath structure  */
  plp = (PathList*) gimp_image_get_paths (gimage);
  
  if (!plp)
    return NULL;
  
  plist = plp->bz_paths;

  for (plist = plp->bz_paths; plist; plist = g_slist_next (plist))
    {
      p = (Path*)plist->data;
      if (p->locked)
	{
	  /* save away for a rainy day */
	  p_copy = path_copy (NULL, p); /* NULL means dont want new tattoo */
	  undo_list = g_slist_append (undo_list, p_copy);
	}
    }

  return (PathUndo *) undo_list;
}

void
path_transform_free_undo (PathUndo *pundo)
{
  GSList *pundolist = (GSList*)pundo;
  Path   *p;
  /* free data associated with the transform path undo */

  while (pundolist)
    {
      p = (Path*)pundolist->data;
      path_free (p);
      pundolist = g_slist_next (pundolist);
    }

  g_slist_free (pundolist);
}

void
path_transform_do_undo (GimpImage *gimage,
			PathUndo  *pundo)
{
  GSList *pundolist;
  /* Restore the paths as they were before this transform took place. */
  Path *p_undo;
  Path *p;
  BezierSelect *bezier_sel;
  gint tmprow;
  gint loop;
  gboolean preview_update = FALSE;
  PathList *plp;
  GSList *plist;

  /* free data associated with the transform path undo */

  for (pundolist = pundo; pundolist; pundolist = g_slist_next(pundolist))
    {
      p_undo = (Path*)pundolist->data;
      /* Find the old path and replace it */
      p = path_get_path_by_tattoo (gimage, p_undo->tattoo);
      if (p)
	{
	  /* Path is still around... undo the transform stuff */
	  pathpoints_free (p->path_details);
	  p->closed = p_undo->closed;
	  p->state = p_undo->state;
	  p->pathtype = p_undo->pathtype;
	  p->path_details = pathpoints_copy (p_undo->path_details);
	  preview_update = TRUE;
	}
    }

  if (preview_update && paths_dialog)
    {
      /* Heck the previews need updating...*/
      plp = (PathList *) gimp_image_get_paths (gimage);
      plist = plp->bz_paths;
      loop = 0;
      
      while (plist && 
	     g_slist_length (plist) &&
	     paths_dialog->current_path_list)
	{
	  bezier_sel = path_to_beziersel (plist->data);
	  tmprow = paths_dialog->current_path_list->last_selected_row;
	  paths_dialog->current_path_list->last_selected_row = loop;
	  paths_update_preview (bezier_sel);
	  bezier_select_free (bezier_sel);
	  paths_dialog->current_path_list->last_selected_row = tmprow;
	  paths_dialog->selected_row_num = tmprow;
	  loop++;
	  plist = g_slist_next (plist);
	}

      /* Force selection .. it may have changed */
      if (bezier_tool_selected () && paths_dialog->current_path_list)
	{
	  gtk_clist_select_row (GTK_CLIST (paths_dialog->paths_list),
				paths_dialog->current_path_list->last_selected_row,
				1);
	}
    }
}

static void
transform_func (GimpImage  *gimage, 
		int         flip, 
		gdouble     x, 
		gdouble     y)
{
  PathList      *plp;
  Path          *p;
  Path          *p_copy;
  GSList        *points_list;
  BezierSelect  *bezier_sel;
  GSList        *plist;
  gint           loop;
  gint           tmprow;

  /* As a first off lets just translate the current path */

  /* Get bzpath structure  */
  plp = (PathList *) gimp_image_get_paths (gimage);

  if (!plp)
    return;

  loop = 0;
  for (plist = plp->bz_paths; plist; plist = g_slist_next (plist))
    {
      p = (Path *) plist->data;

      if (p->locked)
	{
	  p_copy = p;
	  
	  for (points_list = p_copy->path_details; points_list;
	       points_list = g_slist_next (points_list))
	    {
	      PathPoint *ppoint = points_list->data;

	      if (flip)
		{
		  if (x > 0.0)
		    {
		      ppoint->y = gimage->height - ppoint->y;
		    }
		  else
		    {
		      ppoint->x = gimage->width - ppoint->x;
		    }
		}
	      else
		{
		  ppoint->y += y;
		  ppoint->x += x;
		}
	    }
	  
	  /* Only update if we have a dialog, we have a currently 
	   * selected path and its the showing the same image.
	   */

	  if(paths_dialog && 
	     paths_dialog->current_path_list &&
	     paths_dialog->gimage == gimage)
	    {
	      /* Now fudge the drawing....*/
	      bezier_sel = path_to_beziersel(p_copy);
	      tmprow = paths_dialog->current_path_list->last_selected_row;
	      paths_dialog->current_path_list->last_selected_row = loop;
	      paths_update_preview(bezier_sel);
	      bezier_select_free(bezier_sel);
	      paths_dialog->current_path_list->last_selected_row = tmprow;
	      paths_dialog->selected_row_num = tmprow;
	    }
	}
      loop++;
    }
}

void
path_transform_flip_horz (GimpImage  *gimage)
{
  transform_func (gimage, TRUE, 0.0, 0);
}

void
path_transform_flip_vert (GimpImage  *gimage)
{
  transform_func (gimage, TRUE, 1.0, 0);
}

void 
path_transform_xy (GimpImage *gimage,
		    gint       x,
		    gint       y)
{
  transform_func (gimage, FALSE, (gdouble) x, (gdouble) y);
}

void
path_transform_current_path (GimpImage   *gimage,
			     GimpMatrix3  transform,
			     gboolean     forpreview)
{
  PathList      *plp;
  Path          *p;
  Path          *p_copy;
  GSList        *points_list;
  BezierSelect  *bezier_sel;
  GSList        *plist;
  gint           loop;
  gint           tmprow;

  /* As a first off lets just translate the current path */

  /* Get bzpath structure  */
  plp = (PathList *) gimp_image_get_paths (gimage);

  if (!plp)
    return;

  loop = 0;
  for (plist = plp->bz_paths; plist; plist = g_slist_next (plist))
    {
      p = (Path *) plist->data;

      if (p->locked)
	{
	  if (forpreview)
	    p_copy = path_copy (NULL, p); /* NULL means dont want new tattoo */
	  else
	    p_copy = p;

	  for (points_list = p_copy->path_details; points_list;
	       points_list = g_slist_next (points_list))
	    {
	      gdouble newx,newy;
	      PathPoint *ppoint = points_list->data;

	      /*       printf("[x,y] = [%g,%g]\n",ppoint->x, ppoint->y); */

	      gimp_matrix3_transform_point (transform,
					    ppoint->x,
					    ppoint->y,
					    &newx,&newy);

	      /*       printf("->[x,y] = [%g,%g]\n", newx, newy); */

	      ppoint->x = newx;
	      ppoint->y = newy;
	    }
	  
	  /* Only update if we have a dialog, we have a currently 
	   * selected path and its the showing the same image.
	   */

	  if (paths_dialog && 
	      paths_dialog->current_path_list &&
	      paths_dialog->gimage == gimage)
	    {
	      /* Now fudge the drawing....*/
	      bezier_sel = path_to_beziersel (p_copy);
	      tmprow = paths_dialog->current_path_list->last_selected_row;
	      paths_dialog->current_path_list->last_selected_row = loop;
	      paths_update_preview (bezier_sel);
	      bezier_select_free (bezier_sel);
	      paths_dialog->current_path_list->last_selected_row = tmprow;
	      paths_dialog->selected_row_num = tmprow;
	    }

	  if (forpreview)
	    path_free (p_copy);
	}
      loop++;
    }
}

void
path_transform_draw_current (GDisplay    *gdisp, 
			     DrawCore    *core,
			     GimpMatrix3  transform)
{
  PathList     *plp;
  Path         *bzp;
  BezierSelect *bezier_sel;
  Path         *p_copy;
  GSList       *points_list;
  GSList       *plist;

  /* Get bzpath structure  */
  plp = (PathList *) gimp_image_get_paths (gdisp->gimage);

  if (!plp)
    return;

  for (plist = plp->bz_paths; plist; plist = g_slist_next (plist))
    {
      bzp = (Path *) plist->data;
      /* This image path is locked */
      if (bzp->locked)
	{
	  p_copy = path_copy (NULL, bzp); /* NULL means dont want new tattoo */
      
	  for (points_list = p_copy->path_details; points_list;
	       points_list = g_slist_next (points_list))
	    {
	      gdouble newx,newy;
	      PathPoint *ppoint = points_list->data;

	      /*       printf("[x,y] = [%g,%g]\n",ppoint->x, ppoint->y); */

	      gimp_matrix3_transform_point (transform,
					    ppoint->x,
					    ppoint->y,
					    &newx,&newy);

	      /*       printf("->[x,y] = [%g,%g]\n", newx, newy); */

	      ppoint->x = newx;
	      ppoint->y = newy;
	    }

	  bezier_sel = path_to_beziersel (p_copy);
	  bezier_sel->core = core; /* A bit hacky */
	  bezier_draw (gdisp, bezier_sel);
	  bezier_select_free (bezier_sel);
	  path_free (p_copy);
	}
    }
}


/*************************************/
/* PDB function aids                 */
/*************************************/

/* Return TRUE if setting the path worked, else false */

gboolean
path_set_path (GimpImage *gimage,
	       gchar     *pname)
{
  gint      row = 0;
  gboolean  found = FALSE;
  GSList   *tlist;
  PathList *plp;

  /* Get bzpath structure  */
  plp = (PathList *) gimp_image_get_paths (gimage);

  if(!plp)
    return FALSE;

  for (tlist = plp->bz_paths; tlist; tlist = g_slist_next (tlist))
    {
      gchar *test_str = ((Path*)(tlist->data))->name;
      if (strcmp (pname, test_str) == 0)
	{
	  found = TRUE;
	  break;
	}
      row++;
    }

  if(!found)
    return FALSE;

  if(paths_dialog)
    {
      gtk_clist_select_row(GTK_CLIST(paths_dialog->paths_list),
			   row,
			   1);
    }
  else
    {
      plp->last_selected_row = row;
    }

  return TRUE;
}

/* Set a path with the given set of points. */
/* We assume that there are enough points */
/* Return TRUE if path created OK. */

gboolean
path_set_path_points (GimpImage *gimage,
		      gchar     *pname,
		      gint       ptype,
		      gint       pclosed,
		      gint       num_pnts,
		      gdouble   *pnts)
{
  PathList     *plist    = gimp_image_get_paths(gimage);
  GSList       *pts_list = NULL;
  Path         *bzpath;
  BezierSelect *bezier_sel;
  gint          pcount   = 0;
  gint          this_path_count = 0;

  if (num_pnts < 6 ||
      (pclosed && ((num_pnts/3) % 3)) ||
      (!pclosed && ((num_pnts/3) % 3) != 2))
    {
      g_warning ("wrong number of points\n");
      return FALSE;
    }

  if (ptype != BEZIER)
    ptype = BEZIER;

  while (num_pnts)
    {
      PathPoint *bpt;
      gint type;
      gdouble x; 
      gdouble y;
      
/*       if((pcount/2)%3) */
/* 	type = BEZIER_CONTROL; */
/*       else */
/* 	type = BEZIER_ANCHOR; */

      x = pnts[pcount++];
      y = pnts[pcount++];
      type = (gint)pnts[pcount++];
      this_path_count++;

      switch (type)
	{
	case BEZIER_ANCHOR:
	case BEZIER_CONTROL:
	  break;
	case BEZIER_MOVE:
	  if (this_path_count < 6)
	    {
	      g_warning ("Invalid single point in path\n");
	      return FALSE;
	    }
	  this_path_count = 0;
	  break;
	default:
	  g_warning ("Invalid point type passed\n");
	  return FALSE;
	}

      
/*       printf("New point type = %s, x = %d y= %d\n", */
/* 	     (type==BEZIER_CONTROL)?"CNTL":"ANCH", */
/* 	     (int)x, */
/* 	     (int)y); */
      
      bpt = path_point_new (type, (gdouble)x, (gdouble)y);
      pts_list = g_slist_append(pts_list,bpt);

      num_pnts -= 3;
    }
  
  bzpath = path_new (gimage,
		     ptype,
		     pts_list,
		     pclosed,
		     (pclosed) ? BEZIER_EDIT : BEZIER_ADD, /* state */
		     0, /* Can't be locked */
		     0, /* No tattoo assigned */
		     pname);

  bezier_sel = path_to_beziersel(bzpath);

  /* Only add if paths dialog showing this window */
  if (paths_dialog && paths_dialog->gimage == gimage)
    { 
      paths_dialog->current_path_list =  
	path_add_to_current (paths_dialog->current_path_list, 
			     bzpath, 
			     paths_dialog->gimage, 
			     0); 

      paths_add_path (bzpath, 0); 

      /* Update the preview */
      paths_dialog->current_path_list->last_selected_row = 0;
      paths_update_preview (bezier_sel);

      gtk_clist_select_row (GTK_CLIST (paths_dialog->paths_list),
			    paths_dialog->current_path_list->last_selected_row,
			    1);

      paths_dialog_set_menu_sensitivity ();
    }
  else
    {
      GDisplay *gdisp;

      if (!plist)
	{
	  /* If we haven't got a paths dialog */
	  GSList *bzp_list = NULL;

	  bzp_list = g_slist_append (bzp_list, bzpath);
	  plist = path_list_new (gimage, 0, bzp_list);
	  gimp_image_set_paths (gimage, plist);
	}
      else
	{
	  path_add_to_current (plist, bzpath, gimage, 0);
	}

      /* This is a little HACK.. we need to find a display
       * to put the path image on.
       */

      gdisp = gdisplays_check_valid (NULL, gimage);

      /* Mark this path as selected */
      plist->last_selected_row = 0;

      /* Only paste if we have an image to paste to! */
      if (gdisp)
	bezier_paste_bezierselect_to_current (gdisp, bezier_sel);
    }

  bezier_select_free (bezier_sel);

  return TRUE;
}

gboolean
path_delete_path (GimpImage *gimage,
		  gchar     *pname)
{
  gint      row = 0;
  gboolean  found = FALSE;
  GSList   *tlist;
  PathList *plp;

  if (!pname || !gimage)
    {
      g_warning ("paths_delete_path: invalid path");
      return FALSE;
    }

  /* Removed the named path ... */
  /* Get bzpath structure  */
  plp = (PathList *) gimp_image_get_paths (gimage);

  if(!plp)
    return FALSE;

  for (tlist = plp->bz_paths; tlist; tlist = g_slist_next (tlist))
    {
      gchar *test_str = ((Path *) (tlist->data))->name;
      if (strcmp (pname, test_str) == 0)
	{
	  found = TRUE;
	  break;
	}
      row++;
    }

  if (!found)
    return FALSE;

  plp->bz_paths = g_slist_remove (plp->bz_paths, tlist->data);
  /* If now empty free everything up */
  if (!plp->bz_paths || g_slist_length (plp->bz_paths) == 0)
    {
      gtk_signal_disconnect (GTK_OBJECT (plp->gimage), plp->sig_id);
      gimp_image_set_paths (plp->gimage, NULL);
      path_list_free (plp);
    }

  /* Redisplay if required */
  if (paths_dialog && paths_dialog->gimage == gimage)
    {
      paths_dialog->current_path_list = NULL;
      paths_dialog_flush ();
    }

  return TRUE;
}
