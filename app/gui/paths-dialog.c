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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "draw_core.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "drawable.h"
#include "errors.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "gimpset.h"
#include "general.h"
#include "image_render.h"
#include "interface.h"
#include "layers_dialog.h"
#include "layers_dialogP.h"
#include "ops_buttons.h"
#include "paint_funcs.h"
#include "bezier_select.h"
#include "bezier_selectP.h"
#include "pathsP.h"
#include "paths_dialog.h"
#include "resize.h"
#include "session.h"
#include "undo.h"

#include "libgimp/gimpintl.h"

#include "tools/new.xpm"
#include "tools/duplicate.xpm"
#include "tools/delete.xpm"
#include "tools/pennorm.xpm"
#include "tools/penadd.xpm"
#include "tools/pendel.xpm"
#include "tools/penedit.xpm"
#include "tools/penstroke.xpm"
#include "tools/ptoselection.xpm"
#include "tools/path.xbm"

#define PREVIEW_EVENT_MASK GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK

#define PATHS_LIST_WIDTH 200
#define PATHS_LIST_HEIGHT 150

typedef struct {
  GtkWidget *paths_list;
  GtkWidget *vbox;
  GtkWidget *ops_menu;
  GtkAccelGroup *accel_group;

  double ratio;
  int image_width, image_height;
  int gimage_width, gimage_height;

  /*  state information  */
  gint        selsigid;
  GimpImage * gimage;
  GdkGC     * gc;   
  GdkColor    black;
  GdkColor    white;
  gint        selected_row_num;
  gboolean    been_selected;
  PATHIMAGELISTP current_path_list;
} PATHSLIST, *PATHSLISTP;

static PATHSLISTP paths_dialog = NULL;

typedef struct {
  GdkPixmap *paths_pixmap;
  GString   *text;
  BZPATHP   bzp;
} PATHWIDGET, *PATHWIDGETP;

static gint path_widget_preview_events (GtkWidget *, GdkEvent *);
static void paths_dialog_realized      (GtkWidget *widget);
static void paths_select_row           (GtkWidget *widget,gint row,gint column,GdkEventButton *event,gpointer data);
static void paths_unselect_row         (GtkWidget *widget,gint row,gint column,GdkEventButton *event,gpointer data);
static gint paths_list_events          (GtkWidget *widget,GdkEvent  *event);
static void paths_dialog_new_path_callback (GtkWidget *, gpointer);
static void paths_dialog_delete_path_callback (GtkWidget *, gpointer);
static void paths_dialog_map_callback  (GtkWidget *w,gpointer client_data);
static void paths_dialog_unmap_callback(GtkWidget *w,gpointer client_data);
static void paths_dialog_dup_path_callback(GtkWidget *w,gpointer client_data);
static void paths_dialog_stroke_path_callback(GtkWidget *w,gpointer client_data);
static void paths_dialog_path_to_sel_callback(GtkWidget *w,gpointer client_data);
static void paths_dialog_destroy_cb (GimpImage *image);
static void paths_update_paths(gpointer data,gint row);
static GSList *  bzpoints_copy(GSList *list);
static void bzpoints_free(GSList *list);
static void paths_update_preview(BezierSelect *bezier_sel);
static void paths_dialog_preview_extents (void);
static void paths_dialog_new_point_callback (GtkWidget *, gpointer);
static void paths_dialog_add_point_callback (GtkWidget *, gpointer);
static void paths_dialog_delete_point_callback (GtkWidget *, gpointer);
static void paths_dialog_edit_point_callback (GtkWidget *, gpointer);


#define NEW_PATH_BUTTON 1
#define DUP_PATH_BUTTON 2
#define PATH_TO_SEL_BUTTON 3
#define STROKE_PATH_BUTTON 4
#define DEL_PATH_BUTTON 5

static MenuItem paths_ops[] =
{
  { N_("New Path"), 'N', GDK_CONTROL_MASK,
    paths_dialog_new_path_callback, NULL, NULL, NULL },
  { N_("Duplicate Path"), 'C', GDK_CONTROL_MASK,
    paths_dialog_dup_path_callback, NULL, NULL, NULL },
  { N_("Path to Selection"), 'S', GDK_CONTROL_MASK,
    paths_dialog_path_to_sel_callback, NULL, NULL, NULL },
  { N_("Stroke Path"), 'T', GDK_CONTROL_MASK,
    paths_dialog_stroke_path_callback, NULL, NULL, NULL },
  { N_("Delete Path"), 'D', GDK_CONTROL_MASK,
    paths_dialog_delete_path_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

static OpsButton paths_ops_buttons[] =
{
  { new_xpm, paths_dialog_new_path_callback, N_("New Path"), NULL },
  { duplicate_xpm, paths_dialog_dup_path_callback, N_("Duplicate Path"), NULL },
  { ptoselection_xpm, paths_dialog_path_to_sel_callback, N_("Path to Selection"), NULL },
  { penstroke_xpm, paths_dialog_stroke_path_callback, N_("Stroke Path"), NULL },
  { delete_xpm, paths_dialog_delete_path_callback, N_("Delete Path"), NULL },
  { NULL, NULL, NULL, NULL }
};

#define POINT_NEW_BUTTON 1
#define POINT_ADD_BUTTON 2
#define POINT_DEL_BUTTON 3
#define POINT_EDIT_BUTTON 4

static OpsButton point_ops_buttons[] =
{
  { pennorm_xpm, paths_dialog_new_point_callback, N_("New Point"), NULL },
  { penadd_xpm, paths_dialog_add_point_callback, N_("Add Point"), NULL },
  { pendel_xpm, paths_dialog_delete_point_callback, N_("Delete Point"), NULL },
  { penedit_xpm, paths_dialog_edit_point_callback, N_("Edit Point"), NULL },
  { NULL, NULL, NULL, NULL }
};

static void
paths_ops_button_set_sensitive(gint but,gboolean sensitive)
{
  switch(but)
    {
    case NEW_PATH_BUTTON:
      gtk_widget_set_sensitive(paths_ops[0].widget,sensitive);
      gtk_widget_set_sensitive(paths_ops_buttons[0].widget,sensitive);
      break;
    case DUP_PATH_BUTTON:
      gtk_widget_set_sensitive(paths_ops[1].widget,sensitive);
      gtk_widget_set_sensitive(paths_ops_buttons[1].widget,sensitive);
      break;
    case PATH_TO_SEL_BUTTON:
      gtk_widget_set_sensitive(paths_ops[2].widget,sensitive);
      gtk_widget_set_sensitive(paths_ops_buttons[2].widget,sensitive);
      break;
    case STROKE_PATH_BUTTON:
      gtk_widget_set_sensitive(paths_ops[3].widget,sensitive);
      gtk_widget_set_sensitive(paths_ops_buttons[3].widget,sensitive);
      break;
    case DEL_PATH_BUTTON:
      gtk_widget_set_sensitive(paths_ops[4].widget,sensitive);
      gtk_widget_set_sensitive(paths_ops_buttons[4].widget,sensitive);
      break;
    default:
      g_warning(_("paths_ops_button_set_sensitive:: invalid button specified"));
      break;
    }
}

static void
point_ops_button_set_sensitive(gint but,gboolean sensitive)
{
  switch(but)
    {
    case POINT_NEW_BUTTON:
      gtk_widget_set_sensitive(point_ops_buttons[0].widget,sensitive);
      break;
    case POINT_ADD_BUTTON:
      gtk_widget_set_sensitive(point_ops_buttons[1].widget,sensitive);
      break;
    case POINT_DEL_BUTTON:
      gtk_widget_set_sensitive(point_ops_buttons[2].widget,sensitive);
      break;
    case POINT_EDIT_BUTTON:
      gtk_widget_set_sensitive(point_ops_buttons[3].widget,sensitive);
      break;
    default:
      g_warning(_("point_ops_button_set_sensitive:: invalid button specified"));
      break;
    }
}

static void
paths_list_destroy (GtkWidget *w)
{
  paths_dialog = NULL;
}

GtkWidget * paths_dialog_create()
{
  GtkWidget *vbox;
  GtkWidget *paths_list;
  GtkWidget *scrolled_win;  
  GtkWidget *button_box;  

  if(!paths_dialog)
    {
      paths_dialog = g_new0(PATHSLIST,1);

      /*  The paths box  */
      paths_dialog->vbox = vbox = gtk_vbox_new (FALSE, 1);

      /* The point operations */
      button_box = ops_button_box_new (lc_shell,
				       tool_tips, 
				       point_ops_buttons,
				       OPS_BUTTON_RADIO);

      gtk_container_set_border_width(GTK_CONTAINER(button_box),7);

      gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, TRUE, 2);
      gtk_widget_show (button_box);

      gtk_container_border_width (GTK_CONTAINER (vbox), 2);
      
      scrolled_win = gtk_scrolled_window_new (NULL, NULL);
      gtk_box_pack_start(GTK_BOX(vbox), scrolled_win, TRUE, TRUE, 0); 

      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_ALWAYS);

      paths_dialog->paths_list = paths_list = gtk_clist_new (1);

      gtk_signal_connect (GTK_OBJECT (vbox), "destroy",
			  (GtkSignalFunc) paths_list_destroy, NULL);

/*       gtk_clist_set_column_title(GTK_CLIST(paths_list), 0, "col1"); */
/*       gtk_clist_column_titles_show(GTK_CLIST(paths_list)); */

      gtk_container_add (GTK_CONTAINER (scrolled_win), paths_list);
      gtk_clist_set_selection_mode (GTK_CLIST (paths_list), GTK_SELECTION_BROWSE);
      gtk_signal_connect (GTK_OBJECT (paths_list), "event",
			  (GtkSignalFunc) paths_list_events,
			  paths_dialog);
      gtk_container_set_focus_vadjustment (GTK_CONTAINER (paths_list), 
					   gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win))); 
      GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (scrolled_win)->vscrollbar, GTK_CAN_FOCUS); 

      paths_dialog->selsigid = gtk_signal_connect(GTK_OBJECT(paths_list), "select_row",
				 GTK_SIGNAL_FUNC(paths_select_row),
				 (gpointer) NULL);

      gtk_signal_connect(GTK_OBJECT(paths_list), "unselect_row",
			 GTK_SIGNAL_FUNC(paths_unselect_row),
			 (gpointer) NULL);
      
      gtk_widget_show(scrolled_win);
      gtk_widget_show(paths_list);

      gtk_signal_connect(GTK_OBJECT(vbox),"realize",
			 (GtkSignalFunc)paths_dialog_realized,
			 (gpointer)NULL);

      gtk_widget_show (vbox);

      /* The ops buttons */

      button_box = ops_button_box_new (lc_shell,
				       tool_tips, 
				       paths_ops_buttons,
				       OPS_BUTTON_NORMAL);

      gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 2);
      gtk_widget_show (button_box);

      /*  Set up signals for map/unmap for the accelerators  */
      paths_dialog->accel_group = gtk_accel_group_new ();

      gtk_signal_connect (GTK_OBJECT (vbox), "map",
			  (GtkSignalFunc) paths_dialog_map_callback,
			  NULL);
      gtk_signal_connect (GTK_OBJECT (vbox), "unmap",
			  (GtkSignalFunc) paths_dialog_unmap_callback,
			  NULL);

      paths_dialog->ops_menu = build_menu (paths_ops,paths_dialog->accel_group);
      paths_ops_button_set_sensitive(DUP_PATH_BUTTON,FALSE);
      paths_ops_button_set_sensitive(DEL_PATH_BUTTON,FALSE);
      paths_ops_button_set_sensitive(STROKE_PATH_BUTTON,FALSE);
      point_ops_button_set_sensitive(POINT_ADD_BUTTON,FALSE);
      point_ops_button_set_sensitive(POINT_DEL_BUTTON,FALSE);
      point_ops_button_set_sensitive(POINT_NEW_BUTTON,FALSE);
      point_ops_button_set_sensitive(POINT_EDIT_BUTTON,FALSE);
      point_ops_button_set_sensitive(PATH_TO_SEL_BUTTON,FALSE);
    }
  
  return paths_dialog->vbox;
}

static void paths_dialog_realized(GtkWidget *widget)
{
  GdkColormap *colormap;
  gchar dash_list[2]= {3,3};

  paths_dialog->gc = gdk_gc_new(widget->window);
  gdk_gc_set_dashes(paths_dialog->gc,2,dash_list,2);
  colormap = gtk_widget_get_colormap(paths_dialog->paths_list);
  gdk_color_parse("black", &paths_dialog->black);
  gdk_color_alloc(colormap, &paths_dialog->black);
  gdk_color_parse("white", &paths_dialog->white);
  gdk_color_alloc(colormap, &paths_dialog->white);
}

/* Clears out row when list element is deleted/destroyed */
static void
clear_pathwidget(gpointer   data)
{
  PATHWIDGETP pwidget = data;
  
  if(pwidget)
    {
      g_free(pwidget);
    }
}

static void
bzpoint_free(gpointer data,gpointer user_data)
{
  BZPOINTP bzpoint = data;
  g_free(bzpoint);
}

static void 
bzpath_free(gpointer data,gpointer user_data)
{
  BZPATHP bzp = data;
  g_return_if_fail(bzp != NULL);
  g_string_free(bzp->name,TRUE);
  bzpoints_free(bzp->bezier_details);
  g_free(bzp);
}

static BZPATHP
bzpath_dialog_new(gint name_seed, gpointer udata)
{
  BZPATHP bzp = g_new0(BZPATH,1);

  GString *s = g_string_new (NULL);

  g_string_sprintf (s, "path %d",name_seed);

  bzp->name = s;
  bzp->bezier_details = (GSList *)udata; /* If called via button/menu this will be NULL */
  return bzp;
}

static BZPATHP
bzpath_copy(BZPATHP bzp)
{
  BZPATHP bzp_copy = g_new0(BZPATH,1);
  GString *s = g_string_new (NULL);

  g_string_sprintf (s, "%s copy",bzp->name->str);

  bzp_copy->name = s;
  bzp_copy->closed = bzp->closed;
  bzp_copy->state = bzp->state;
  bzp_copy->bezier_details = bzpoints_copy(bzp->bezier_details);

  return bzp_copy;
}

static void
bzpath_close(BZPATHP bzp)
{
  BZPOINTP pdata;
  BZPOINTP bzpoint;

  /* bzpaths are only really closed when converted to the BezierSelect ones */
  bzp->closed = 1;
  /* first point */
  pdata = (BZPOINTP)bzp->bezier_details->data;
	  
  if(g_slist_length(bzp->bezier_details) < 5)
    {
      int i;
      for (i = 0 ; i < 2 ; i++)
	{
	  bzpoint = g_new0(BZPOINT,1);
	  bzpoint->type = (i & 1)?BEZIER_ANCHOR:BEZIER_CONTROL;
	  bzpoint->x = pdata->x+i;
	  bzpoint->y = pdata->y+i;
	  bzp->bezier_details = g_slist_append(bzp->bezier_details,bzpoint);
	}
    }
  bzpoint = g_new0(BZPOINT,1);
  pdata = (BZPOINTP)bzp->bezier_details->data;
  bzpoint->type = BEZIER_CONTROL;
  bzpoint->x = pdata->x;
  bzpoint->y = pdata->y;
  bzp->bezier_details = g_slist_append(bzp->bezier_details,bzpoint);
}

static void
beziersel_free(BezierSelect *bezier_sel)
{
  bezier_select_reset (bezier_sel);
  g_free(bezier_sel);
}

static BezierSelect *
bzpath_to_beziersel(BZPATHP bzp)
{
  BezierSelect *bezier_sel;
  GSList *list;

  if(!bzp)
    {
      g_warning("bzpath_to_beziersel:: NULL bzp");
    }

  list = bzp->bezier_details;
  bezier_sel = g_new0 (BezierSelect,1);

  bezier_sel->num_points = 0;
  bezier_sel->mask = NULL;
  bezier_sel->core = NULL; /* not required will be reset in bezier code */
  bezier_select_reset (bezier_sel);
  bezier_sel->closed = bzp->closed;
/*   bezier_sel->state = BEZIER_ADD; */
  bezier_sel->state = bzp->state;

  while(list)
    {
      BZPOINTP pdata;
      pdata = (BZPOINTP)list->data;
      bezier_add_point(bezier_sel,pdata->type,pdata->x,pdata->y);
      list = g_slist_next(list);
    }
  
  if ( bezier_sel->closed )
    {
      bezier_sel->last_point->next = bezier_sel->points;
      bezier_sel->points->prev = bezier_sel->last_point;
      bezier_sel->cur_anchor = bezier_sel->points;
      bezier_sel->cur_control = bezier_sel-> points->next;
    }

  return bezier_sel;
}

static void
pathimagelist_free(PATHIMAGELISTP iml)
{
  g_return_if_fail(iml != NULL);
  if(iml->bz_paths)
    {
      g_slist_foreach(iml->bz_paths,bzpath_free,NULL);
      g_slist_free(iml->bz_paths);
    }
  g_free(iml);
}

static void 
bz_change_name_row_to(gint row,gchar *text)
{
  PATHWIDGETP pwidget;

  pwidget = (PATHWIDGETP)gtk_clist_get_row_data(GTK_CLIST(paths_dialog->paths_list),row);

  if(!pwidget)
    return;

  g_string_free(pwidget->bzp->name,TRUE);

  pwidget->bzp->name = g_string_new(text);
}

static void 
paths_set_dash_line(GdkGC *gc,gboolean state)
{
  if(state)
    {
      gdk_gc_set_line_attributes(gc,0,GDK_LINE_ON_OFF_DASH,GDK_CAP_BUTT,GDK_JOIN_ROUND);
    }
  else
    {
      gdk_gc_set_line_attributes(gc,0,GDK_LINE_SOLID,GDK_CAP_BUTT,GDK_JOIN_ROUND);
    }
}

static void 
clear_pixmap_preview(PATHWIDGETP pwidget)
{
  gchar *rgb_buf;

  rgb_buf = g_new0(gchar,
		   (paths_dialog->image_width + 4)
		   *(paths_dialog->image_height + 4)*3);

  memset(rgb_buf,0xFF,(paths_dialog->image_width + 4)
		   *(paths_dialog->image_height + 4)*3);

  gdk_draw_rgb_image (pwidget->paths_pixmap,
		      paths_dialog->gc,
		      0,
		      0,
		      paths_dialog->image_width + 4,
		      paths_dialog->image_height + 4,
		      GDK_RGB_DITHER_NORMAL,
		      rgb_buf,
		      (paths_dialog->image_width + 4)*3);

  gdk_gc_set_foreground(paths_dialog->gc, &paths_dialog->black);

  paths_set_dash_line(paths_dialog->gc,FALSE);

  gdk_draw_rectangle(pwidget->paths_pixmap, 
		     paths_dialog->gc, FALSE, 0, 0, 
		     paths_dialog->image_width+3,
		     paths_dialog->image_height+3);

  gdk_draw_rectangle(pwidget->paths_pixmap, 
		     paths_dialog->gc, FALSE, 1, 1, 
		     paths_dialog->image_width+1,
		     paths_dialog->image_height+1);
}

/* insrow == -1 -> append else insert at insrow */
void paths_add_path(BZPATHP bzp,gint insrow)
{
  /* Create a new entry in the list */
  PATHWIDGETP pwidget;
  gint row;
  gchar *row_data[1];

  pwidget = g_new0(PATHWIDGET,1);

  if(!GTK_WIDGET_REALIZED(paths_dialog->vbox))
    gtk_widget_realize(paths_dialog->vbox);

  paths_dialog_preview_extents();

  if(preview_size)
    {
      /* Need to add this to the list */
      pwidget->paths_pixmap =  gdk_pixmap_new(paths_dialog->vbox->window,
					      paths_dialog->image_width + 4,  
					      paths_dialog->image_height + 4,
					      -1);
      clear_pixmap_preview(pwidget);
    }
  else
    {
      pwidget->paths_pixmap =  
	gdk_pixmap_create_from_data (paths_dialog->vbox->window,
				     path_bits, 
				     paths_dialog->image_width,
				     paths_dialog->image_height,
				     -1,
				     &paths_dialog->vbox->style->fg[GTK_STATE_SELECTED],
				     &paths_dialog->vbox->style->bg[GTK_STATE_SELECTED]);
    }

  gtk_clist_set_row_height(GTK_CLIST(paths_dialog->paths_list),
			   paths_dialog->image_height + 6);

  row_data[0] = "";

  if(insrow == -1)
    row = gtk_clist_append(GTK_CLIST(paths_dialog->paths_list),
			   row_data);
  else
    row = gtk_clist_insert(GTK_CLIST(paths_dialog->paths_list),
			   insrow,
			   row_data);

  gtk_clist_set_pixtext(GTK_CLIST(paths_dialog->paths_list),
			row,
			0,
			bzp->name->str,
			2,
			pwidget->paths_pixmap,
			NULL);

  gtk_clist_set_row_data_full(GTK_CLIST(paths_dialog->paths_list),
			      row,
			      (gpointer)pwidget,
			      clear_pathwidget);

  gtk_signal_handler_block(GTK_OBJECT(paths_dialog->paths_list),paths_dialog->selsigid);
  gtk_clist_select_row(GTK_CLIST(paths_dialog->paths_list),
		       paths_dialog->current_path_list->last_selected_row,
		       0);
  gtk_signal_handler_unblock(GTK_OBJECT(paths_dialog->paths_list),paths_dialog->selsigid);

  pwidget->bzp = bzp;
}

static void
paths_dialog_preview_extents ()
{
  GImage *gimage;

  if (!paths_dialog)
    return;

 if (!(gimage = paths_dialog->gimage))
    return;

  gimage = paths_dialog->gimage;

  paths_dialog->gimage_width = gimage->width;
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
      paths_dialog->image_width = path_width;
      paths_dialog->image_height = path_height;
    }
}

static gint
path_widget_preview_events (GtkWidget *widget,
			     GdkEvent  *event)
{
  GdkEventButton *bevent;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      /*  Control-button press disables the application of the mask  */
      bevent = (GdkEventButton *) event;
      break;

    case GDK_EXPOSE:
      if (preview_size)
	{

/* 	  layer_widget_preview_redraw (layer_widget, preview_type); */
	  
/* 	  gdk_draw_pixmap (widget->window, */
/* 			   widget->style->black_gc, */
/* 			   *pixmap, */
/* 			   0, 0, 2, 2, */
/* 			   layersD->image_width, */
/* 			   layersD->image_height); */
	}

      break;

    default:
      break;
    }

  return FALSE;
}

static void
paths_select_row(GtkWidget *widget, 
		 gint row,
		 gint column,
		 GdkEventButton *event,
		 gpointer data)
{
  PATHWIDGETP pwidget;
  BZPATHP bzp;
  BezierSelect * bsel;
  GDisplay *gdisp;

  pwidget = (PATHWIDGETP)gtk_clist_get_row_data(GTK_CLIST(widget),row);

  if(!pwidget ||
     (paths_dialog->current_path_list->last_selected_row == row &&
     paths_dialog->been_selected == TRUE))
    return;

  paths_dialog->selected_row_num = row;
  paths_dialog->current_path_list->last_selected_row = row;
  paths_dialog->been_selected = TRUE;

  bzp = (BZPATHP)g_slist_nth_data(paths_dialog->current_path_list->bz_paths,row);

  g_return_if_fail(bzp != NULL);

  bsel = bzpath_to_beziersel(bzp);
  gdisp = gdisplays_check_valid(paths_dialog->current_path_list->gdisp,
				paths_dialog->gimage);
  if(!gdisp)
    {
      g_warning("Lost image which bezier curve belonged to");
      return;
    }
  bezier_paste_bezierselect_to_current(gdisp,bsel);
  paths_update_preview(bsel);
  beziersel_free(bsel);

  /* Draw white as the border */
 /*  gdk_gc_set_foreground(paths_dialog->gc, &paths_dialog->black); */

/*   gdk_draw_rectangle(pwidget->paths_pixmap,  */
/* 		     paths_dialog->gc, FALSE, 0, 0,  */
/* 		     paths_dialog->image_width+3, */
/* 		     paths_dialog->image_height+3); */

/*   gdk_draw_rectangle(pwidget->paths_pixmap,  */
/* 		     paths_dialog->gc, FALSE, 1, 1,  */
/* 		     paths_dialog->image_width+1, */
/* 		     paths_dialog->image_height+1); */

}

static void
paths_unselect_row(GtkWidget *widget, 
		 gint row,
		 gint column,
		 GdkEventButton *event,
		 gpointer data)
{
  PATHWIDGETP pwidget;

  pwidget = (PATHWIDGETP)gtk_clist_get_row_data(GTK_CLIST(widget),row);

  if(!pwidget)
    return;

/*   gdk_gc_set_foreground(paths_dialog->gc, &paths_dialog->white); */

/*   gdk_draw_rectangle(pwidget->paths_pixmap,  */
/* 		     paths_dialog->gc, FALSE, 0, 0,  */
/* 		     paths_dialog->image_width+3, */
/* 		     paths_dialog->image_height+3); */

/*   gdk_draw_rectangle(pwidget->paths_pixmap,  */
/* 		     paths_dialog->gc, FALSE, 1, 1,  */
/* 		     paths_dialog->image_width+1, */
/* 		     paths_dialog->image_height+1); */

}

void
paths_dialog_update (GimpImage* gimage)
{
  PATHIMAGELISTP new_path_list;
  GSList *plist;
  gint loop;
  gint tmprow;

  if (!paths_dialog || !gimage)
    return;

  /* The last pointer comparison forces update if something has changed
   * under our feet.
   */

  if (paths_dialog->gimage == gimage &&
      paths_dialog->current_path_list == (PATHIMAGELISTP)gimp_image_get_paths(gimage))
    return;

  paths_dialog->gimage=gimage;

  paths_dialog_preview_extents ();

  if(!GTK_WIDGET_REALIZED(paths_dialog->vbox))
    gtk_widget_realize(paths_dialog->vbox);
  /* ALT removed & replaced return;*/

  /* clear clist out */

  gtk_clist_freeze(GTK_CLIST(paths_dialog->paths_list));
  gtk_clist_clear(GTK_CLIST(paths_dialog->paths_list));
  gtk_clist_thaw(GTK_CLIST(paths_dialog->paths_list));

  /* Find bz list */

  new_path_list = (PATHIMAGELISTP)gimp_image_get_paths(gimage);

  paths_dialog->current_path_list = new_path_list;
  paths_dialog->been_selected = FALSE;

  if(!new_path_list)
    {
      /* No list assoc with this image */
      paths_ops_button_set_sensitive(DUP_PATH_BUTTON,FALSE);
      paths_ops_button_set_sensitive(DEL_PATH_BUTTON,FALSE);
      paths_ops_button_set_sensitive(STROKE_PATH_BUTTON,FALSE);
      point_ops_button_set_sensitive(POINT_ADD_BUTTON,FALSE);
      point_ops_button_set_sensitive(POINT_DEL_BUTTON,FALSE);
      point_ops_button_set_sensitive(POINT_NEW_BUTTON,FALSE);
      point_ops_button_set_sensitive(POINT_EDIT_BUTTON,FALSE);
      point_ops_button_set_sensitive(PATH_TO_SEL_BUTTON,FALSE);
      return;
    }
  else
    {
      paths_ops_button_set_sensitive(DUP_PATH_BUTTON,TRUE);
      paths_ops_button_set_sensitive(DEL_PATH_BUTTON,TRUE);
      paths_ops_button_set_sensitive(STROKE_PATH_BUTTON,TRUE);
      point_ops_button_set_sensitive(POINT_ADD_BUTTON,TRUE);
      point_ops_button_set_sensitive(POINT_DEL_BUTTON,TRUE);
      point_ops_button_set_sensitive(POINT_NEW_BUTTON,TRUE);
      point_ops_button_set_sensitive(POINT_EDIT_BUTTON,TRUE);
      point_ops_button_set_sensitive(PATH_TO_SEL_BUTTON,TRUE);
    }

  /* update the clist to reflect this images bz list */
  /* go around the image list populating the clist */

  if(gimage != new_path_list->gimage)
    {
      g_warning(_("paths list: internal list error"));
    }

  plist = new_path_list->bz_paths;
  loop = 0;

  tmprow = paths_dialog->current_path_list->last_selected_row;
  while(plist)
    {
      paths_update_paths(plist->data,loop);
      loop++;
      plist = g_slist_next(plist);
    }
  paths_dialog->current_path_list->last_selected_row = tmprow;
  paths_dialog->selected_row_num = tmprow;

  /*   g_slist_foreach(plist,paths_update_paths,NULL); */

  /* select last one */
  gtk_signal_handler_block(GTK_OBJECT(paths_dialog->paths_list),paths_dialog->selsigid);
  gtk_clist_select_row(GTK_CLIST(paths_dialog->paths_list),
		       paths_dialog->current_path_list->last_selected_row,
		       0);
  gtk_signal_handler_unblock(GTK_OBJECT(paths_dialog->paths_list),paths_dialog->selsigid);

  gtk_clist_moveto(GTK_CLIST(paths_dialog->paths_list),
		   paths_dialog->current_path_list->last_selected_row,
		   0,
		   0.5,
		   0.0);
}

static void
paths_update_paths(gpointer data,gint row)
{
  BZPATHP bzp;
  BezierSelect * bezier_sel;

  paths_add_path((bzp = (BZPATHP)data),-1);
  /* Now fudge the drawing....*/
  bezier_sel = bzpath_to_beziersel(bzp);
  paths_dialog->current_path_list->last_selected_row = row;
  paths_update_preview(bezier_sel);
  beziersel_free(bezier_sel);
}

static void
do_rename_paths_callback(GtkWidget *widget, gpointer call_data, gpointer client_data)
{
  gchar     *text;
  GdkBitmap *mask;
  guint8     spacing;
  GdkPixmap *pixmap;

  if(!(GTK_CLIST(call_data)->selection))
    return;

  text = g_strdup(client_data);

  gtk_clist_get_pixtext(GTK_CLIST(paths_dialog->paths_list),
			paths_dialog->selected_row_num,
			0,
			NULL,
			&spacing,
			&pixmap,
			&mask);


  gtk_clist_set_pixtext(GTK_CLIST(call_data),
			paths_dialog->selected_row_num,
			0,
			text,
			spacing,
			pixmap,
			mask);

  bz_change_name_row_to(paths_dialog->selected_row_num,text);
}

static void
paths_dialog_edit_path_query(GtkWidget *widget)
{
  gchar *text;
  gint   ret;
  GdkBitmap *mask;
  /* Get the current name */
  ret = gtk_clist_get_pixtext(GTK_CLIST(paths_dialog->paths_list),
			      paths_dialog->selected_row_num,
			      0,
			      &text,
			      NULL,
			      NULL,
			      &mask);

  query_string_box(N_("Rename path"),
		   N_("Enter a new name for the path"),
		   text,
		   do_rename_paths_callback, widget);
}

static gint
paths_list_events (GtkWidget *widget,
		   GdkEvent  *event)
{
  GdkEventKey *kevent;
  GdkEventButton *bevent;
  static gint last_row = -1;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      if(!gtk_clist_get_selection_info (GTK_CLIST(paths_dialog->paths_list),
				       bevent->x,
				       bevent->y,
				       &last_row,NULL))
	last_row = -1;
      else
	{
	  if(paths_dialog->selected_row_num != last_row)
	    last_row = -1;
	}

      if (bevent->button == 3 || bevent->button == 2)
	gtk_menu_popup (GTK_MENU (paths_dialog->ops_menu), NULL, NULL, NULL, NULL, bevent->button, bevent->time);
      break;
      
    case GDK_2BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if(last_row != -1 && 
	 gtk_clist_get_selection_info (GTK_CLIST(paths_dialog->paths_list),
				       bevent->x,
				       bevent->y,
				       NULL,NULL))
	{
	  paths_dialog_edit_path_query(widget);
	  return TRUE;
	}
      else
	return FALSE;
      
    case GDK_KEY_PRESS:
      kevent = (GdkEventKey *) event;
      switch (kevent->keyval)
	{
	case GDK_Up:
	  printf ("up arrow\n"); 
	  break;
	case GDK_Down:
	  printf ("down arrow\n");
	  break;
	default:
	  return FALSE;
	}
      return TRUE;
      
    default:
      break;
    }
  return FALSE;
}

static PATHIMAGELISTP
bzpath_add_to_current(PATHIMAGELISTP pip,BZPATHP bzp,GimpImage *gimage,gint pos)
{
  /* add bzp to current list */
  if(!pip)
    {
      /* This image does not have a list */
      pip = pathsList_new(gimage,0,NULL);

      /* add to gimage */
      gimp_image_set_paths(gimage,pip);
    }

  if(pos < 0)
    pip->bz_paths = g_slist_append(pip->bz_paths,bzp);
  else
    pip->bz_paths = g_slist_insert(pip->bz_paths,bzp,pos);

  return pip;
}

static BZPATHP
paths_dialog_new_path(PATHIMAGELISTP *plp,gpointer points,GimpImage *gimage,gint pos)
{
  static gint nseed = 0;
  BZPATHP bzp = bzpath_dialog_new(nseed++,points);
  *plp = bzpath_add_to_current(*plp,bzp,gimage,pos);
  return(bzp);
}

static void 
paths_dialog_new_path_callback (GtkWidget * widget, gpointer udata)
{
  BZPATHP bzp = paths_dialog_new_path(&paths_dialog->current_path_list,
				      NULL,
				      paths_dialog->gimage,
				      paths_dialog->selected_row_num);
  paths_add_path(bzp,paths_dialog->selected_row_num);
}

static void 
paths_dialog_delete_path_callback (GtkWidget * widget, gpointer udata)
{
  BZPATHP bzp;
  PATHIMAGELISTP plp;
  gboolean new_sz;
  gint row = paths_dialog->selected_row_num;

  g_return_if_fail(paths_dialog->current_path_list != NULL);

  /* Get current selection... ignore if none */
  if(paths_dialog->selected_row_num < 0)
    return;
  
  /* Get bzpath structure & delete its content */
  plp = paths_dialog->current_path_list;
  bzp = (BZPATHP)g_slist_nth_data(plp->bz_paths,row); 

  /* Remove from list */
  plp->bz_paths = g_slist_remove(plp->bz_paths,bzp);
  new_sz = (g_slist_length(plp->bz_paths) > 0);
  bzpath_free(bzp,NULL);

  /* If now empty free everything up */
  if(!plp->bz_paths || g_slist_length(plp->bz_paths) == 0)
    {
      gtk_signal_disconnect(GTK_OBJECT (plp->gimage),
			    plp->sig_id);
      gimp_image_set_paths(plp->gimage,NULL);
      pathimagelist_free(plp);
      paths_dialog->current_path_list = NULL;
    }

  /* Do this last since it might cause a new row to become selected */
  /* Remove from the clist ... */
  gtk_clist_remove(GTK_CLIST(paths_dialog->paths_list),row);

  paths_ops_button_set_sensitive(DUP_PATH_BUTTON,new_sz);
  paths_ops_button_set_sensitive(DEL_PATH_BUTTON,new_sz);
  paths_ops_button_set_sensitive(STROKE_PATH_BUTTON,new_sz);
  point_ops_button_set_sensitive(POINT_ADD_BUTTON,new_sz);
  point_ops_button_set_sensitive(POINT_DEL_BUTTON,new_sz);
  point_ops_button_set_sensitive(POINT_NEW_BUTTON,new_sz);
  point_ops_button_set_sensitive(POINT_EDIT_BUTTON,new_sz);
  point_ops_button_set_sensitive(PATH_TO_SEL_BUTTON,new_sz);
}

static void 
paths_dialog_dup_path_callback (GtkWidget * widget, gpointer udata)
{
  BZPATHP bzp;
  PATHIMAGELISTP plp;
  BezierSelect * bezier_sel;
  gint row = paths_dialog->selected_row_num;
  gint tmprow;

  g_return_if_fail(paths_dialog->current_path_list != NULL);

  /* Get current selection... ignore if none */
  if(paths_dialog->selected_row_num < 0)
    return;
  
  /* Get bzpath structure  */
  plp = paths_dialog->current_path_list;
  bzp = (BZPATHP)g_slist_nth_data(plp->bz_paths,row); 

  /* Insert at the current position */
  bzp = bzpath_copy(bzp);
  plp->bz_paths = g_slist_insert(plp->bz_paths,bzp,row);
  paths_add_path(bzp,row);

  /* Now fudge the drawing....*/
  bezier_sel = bzpath_to_beziersel(bzp);
  tmprow = paths_dialog->current_path_list->last_selected_row;
  paths_dialog->current_path_list->last_selected_row = row;
  paths_update_preview(bezier_sel);
  beziersel_free(bezier_sel);
  paths_dialog->current_path_list->last_selected_row = tmprow;
}

static void 
paths_dialog_path_to_sel_callback (GtkWidget * widget, gpointer udata)
{
  BZPATHP bzp;
  PATHIMAGELISTP plp;
  BezierSelect * bezier_sel;
  GDisplay  * gdisp;
  gint row = paths_dialog->selected_row_num;

  g_return_if_fail(paths_dialog->current_path_list != NULL);

  /* Get current selection... ignore if none */
  if(paths_dialog->selected_row_num < 0)
    return;
  
  /* Get bzpath structure  */
  plp = paths_dialog->current_path_list;
  bzp = (BZPATHP)g_slist_nth_data(plp->bz_paths,row); 

  /* Now do the stroke....*/
  gdisp = gdisplays_check_valid(paths_dialog->current_path_list->gdisp,
				paths_dialog->gimage);

  if(!bzp->closed)
    {
      BZPATHP bzpcopy = bzpath_copy(bzp);
      /* Close it */
      bzpath_close(bzpcopy);
      bezier_sel = bzpath_to_beziersel(bzpcopy);
      bzpath_free(bzpcopy,NULL);
      bezier_to_selection (bezier_sel, gdisp);
      beziersel_free(bezier_sel);
    }
  else
    {
      bezier_sel = bzpath_to_beziersel(bzp);
      bezier_to_selection (bezier_sel, gdisp);
      beziersel_free(bezier_sel);      
    }
}

static void 
paths_dialog_stroke_path_callback (GtkWidget * widget, gpointer udata)
{
  BZPATHP bzp;
  PATHIMAGELISTP plp;
  BezierSelect * bezier_sel;
  GDisplay  * gdisp;
  gint row = paths_dialog->selected_row_num;

  g_return_if_fail(paths_dialog->current_path_list != NULL);

  /* Get current selection... ignore if none */
  if(paths_dialog->selected_row_num < 0)
    return;
  
  /* Get bzpath structure  */
  plp = paths_dialog->current_path_list;
  bzp = (BZPATHP)g_slist_nth_data(plp->bz_paths,row); 

  /* Now do the stroke....*/
  gdisp = gdisplays_check_valid(paths_dialog->current_path_list->gdisp,
				paths_dialog->gimage);
  bezier_sel = bzpath_to_beziersel(bzp);
  bezier_stroke (bezier_sel, gdisp, SUBDIVIDE, !bzp->closed);
  beziersel_free(bezier_sel);
}

static void
paths_dialog_map_callback (GtkWidget *w,
			    gpointer   client_data)
{
  if (!paths_dialog)
    return;
  
  gtk_window_add_accel_group (GTK_WINDOW (gtk_widget_get_toplevel(paths_dialog->paths_list)),
			      paths_dialog->accel_group);

  paths_dialog_preview_extents ();
}

static void
paths_dialog_unmap_callback (GtkWidget *w,
			     gpointer   client_data)
{
  if (!paths_dialog)
    return;
  
  gtk_window_remove_accel_group (GTK_WINDOW (gtk_widget_get_toplevel(paths_dialog->paths_list)),
				 paths_dialog->accel_group);
}

static void
paths_dialog_destroy_cb (GimpImage *gimage)
{
  PATHIMAGELISTP new_path_list;

  if(!paths_dialog)
    return;

  if(paths_dialog->current_path_list && 
     gimage == paths_dialog->current_path_list->gimage)
    {
      /* showing could be last so remove here.. might get 
	 done again if not the last one
      */
      paths_dialog->current_path_list = NULL;
      paths_dialog->been_selected = FALSE;
      gtk_clist_freeze(GTK_CLIST(paths_dialog->paths_list));
      gtk_clist_clear(GTK_CLIST(paths_dialog->paths_list));
      gtk_clist_thaw(GTK_CLIST(paths_dialog->paths_list));
    }

  /* Find bz list */  
  new_path_list = (PATHIMAGELISTP)gimp_image_get_paths(gimage);

  if(!new_path_list)
    return; /* Already removed - signal handler jsut left in the air */

  pathimagelist_free(new_path_list);

  gimp_image_set_paths(gimage,NULL);
}


/* Functions used from the bezier code .. tie in with this code */

static void
bzpoints_free(GSList *list)
{
  if(!list)
    return;
  g_slist_foreach(list,bzpoint_free,NULL);
  g_slist_free(list);
}

static GSList *
bzpoints_create(BezierSelect *sel)
{
  gint i;
  GSList *list = NULL;
  BZPOINTP bzpoint;
  BezierPoint *pts = (BezierPoint *) sel->points;

  for (i=0; i< sel->num_points; i++)
    {
      bzpoint = bzpoint_new(pts->type,pts->x,pts->y);
      list = g_slist_append(list,bzpoint);
      pts = pts->next;
    }
  return(list);
}

static GSList *
bzpoints_copy(GSList *list)
{
  GSList *slcopy = NULL;
  BZPOINTP pdata;
  BZPOINTP bzpoint;
  while(list)
    {
      bzpoint = g_new0(BZPOINT,1);
      pdata = (BZPOINTP)list->data;
      bzpoint->type = pdata->type;
      bzpoint->x = pdata->x;
      bzpoint->y = pdata->y;
      slcopy = g_slist_append(slcopy,bzpoint);
      list = g_slist_next(list);
    }
  return slcopy;
}

static void
paths_update_bzpath(PATHIMAGELISTP plp,BezierSelect *bezier_sel)
{
  BZPATHP bzp;

  bzp = (BZPATHP)g_slist_nth_data(plp->bz_paths,plp->last_selected_row);
  
  if(bzp->bezier_details) 
    bzpoints_free(bzp->bezier_details); 
  
  bzp->bezier_details = bzpoints_create(bezier_sel);
  bzp->closed = bezier_sel->closed;
  bzp->state  = bezier_sel->state;
}

static gboolean
paths_replaced_current(PATHIMAGELISTP plp,BezierSelect *bezier_sel)
{
  /* Is there a currently selected path in this image? */
  if(paths_dialog && plp && 
     plp->last_selected_row >= 0)
    {  
      paths_update_bzpath(plp,bezier_sel);
      return TRUE;
    }
  return FALSE;
}

static void 
paths_draw_segment_points(BezierSelect *bezier_sel, 
			  GdkPoint *pnt, 
			  int npoints)
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
  PATHWIDGETP pwidget;
  gint row;

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

  pwidget = (PATHWIDGETP)gtk_clist_get_row_data(GTK_CLIST(paths_dialog->paths_list),row);
  
  if(pcount == 0)
    return;

  g_return_if_fail(pwidget != NULL);

  paths_set_dash_line(paths_dialog->gc,!bezier_sel->closed);

  gdk_draw_lines (pwidget->paths_pixmap,
		   paths_dialog->gc, copy_pnt, pcount);

  g_free(copy_pnt);
}

static void
paths_update_preview(BezierSelect *bezier_sel)
{
  gint row;
  if(paths_dialog &&
     paths_dialog->current_path_list &&
     (row = paths_dialog->current_path_list->last_selected_row) >= 0 &&
     preview_size)
    {
      PATHWIDGETP pwidget;
      pwidget = (PATHWIDGETP)gtk_clist_get_row_data(GTK_CLIST(paths_dialog->paths_list),row);

      /* Clear pixmap */
      clear_pixmap_preview(pwidget);

      /* update .. */
      bezier_draw_curve (bezier_sel,paths_draw_segment_points,IMAGE_COORDS);
      /* update the pixmap */
      gtk_clist_set_pixtext(GTK_CLIST(paths_dialog->paths_list),
			    row,
			    0,
			    pwidget->bzp->name->str,
			    2,
			    pwidget->paths_pixmap,
			    NULL);
    }
}

static void 
paths_dialog_new_point_callback (GtkWidget * widget, gpointer udata)
{
  bezier_select_mode(EXTEND_NEW);
}

static void 
paths_dialog_add_point_callback (GtkWidget * widget, gpointer udata)
{
  bezier_select_mode(EXTEND_ADD);
}

static void 
paths_dialog_delete_point_callback (GtkWidget * widget, gpointer udata)
{
  bezier_select_mode(EXTEND_REMOVE);
}

static void 
paths_dialog_edit_point_callback (GtkWidget * widget, gpointer udata)
{
  bezier_select_mode(EXTEND_EDIT);
}


void 
paths_first_button_press(BezierSelect *bezier_sel,GDisplay * gdisp)
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
  BZPATHP bzp; 
  PATHIMAGELISTP plp;

  if(!paths_dialog)
    return;

  paths_dialog->been_selected = FALSE;

  /* Button not pressed in this image...
   * find which one it was pressed in if any.
   */
  plp = (PATHIMAGELISTP)gimp_image_get_paths(gdisp->gimage);      

  /* Since beziers are part of the save format.. make the image dirty */
/*   gimp_image_dirty(gdisp->gimage); */
  
  if(!paths_replaced_current(plp,bezier_sel))
    {
      bzp = paths_dialog_new_path(&plp,bzpoints_create(bezier_sel),gdisp->gimage,-1);
      bzp->closed = bezier_sel->closed;
      bzp->state  = bezier_sel->state;
      if(paths_dialog->gimage == gdisp->gimage)
	{
	  paths_dialog->current_path_list = plp;
	  paths_add_path(bzp,-1);
	}
    }
}

void
paths_newpoint_current(BezierSelect *bezier_sel,GDisplay * gdisp)
{
  /* Check if currently showing the paths we are updating */
  if(paths_dialog &&
     gdisp->gimage == paths_dialog->gimage)
    {
      /* Enable the buttons!*/
      paths_ops_button_set_sensitive(DUP_PATH_BUTTON,TRUE);
      paths_ops_button_set_sensitive(DEL_PATH_BUTTON,TRUE);
      paths_ops_button_set_sensitive(STROKE_PATH_BUTTON,TRUE);
      point_ops_button_set_sensitive(POINT_NEW_BUTTON,TRUE);
      point_ops_button_set_sensitive(POINT_DEL_BUTTON,TRUE);
      point_ops_button_set_sensitive(POINT_ADD_BUTTON,TRUE);
      point_ops_button_set_sensitive(POINT_EDIT_BUTTON,TRUE);
      point_ops_button_set_sensitive(PATH_TO_SEL_BUTTON,TRUE);
      paths_update_preview(bezier_sel);
    }

  paths_first_button_press(bezier_sel,gdisp);
}

void 
paths_new_bezier_select_tool()
{
  if(paths_dialog)
    paths_dialog->been_selected = FALSE;
}


/**************************************************************/
/* Code to serialise the bezier curves.
 * The curves will be saved out in XCF property format.
 * The "save as XCF format" will prompt to save the curves away.
 *
 * Note the save should really used PDB function to get the
 * curves etc. But I have yet to do those 8-)
 */
/**************************************************************/


BZPOINTP 
bzpoint_new(gint type,
	    gint x, 
	    gint y)
{
  BZPOINTP bzpoint = g_new0(BZPOINT,1);

  bzpoint->type = type;
  bzpoint->x = x;
  bzpoint->y = y;
  return(bzpoint);
}

BZPATHP
bzpath_new(GSList * bezier_details,
	   gint     closed,
	   gint     state,
	   gint     locked,
	   gchar  * name)
{
  BZPATHP bzpath = g_new0(BZPATH,1);

  bzpath->bezier_details = bezier_details;
  bzpath->closed = closed;
  bzpath->state = state;
  bzpath->locked = locked;
  bzpath->name = g_string_new(name);

  return bzpath;
}

PathsList *
pathsList_new(GimpImage * gimage,
	      gint        last_selected_row,
	      GSList    * bz_paths)
{
  PATHIMAGELISTP pip = g_new0(PATHIMAGELIST,1);
  pip->gimage = gimage;
  pip->last_selected_row = last_selected_row;
  
  /* add connector to image delete/destroy */
  pip->sig_id = gtk_signal_connect(GTK_OBJECT (gimage),
				   "destroy",
				   GTK_SIGNAL_FUNC (paths_dialog_destroy_cb),
				   pip);

  pip->bz_paths = bz_paths;

  return (PathsList *)pip;
}
