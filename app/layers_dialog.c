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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdlib.h>
#include <string.h>
#include "gdk/gdkkeysyms.h"
#include "actionarea.h"
#include "appenv.h"
#include "draw_core.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "drawable.h"
#include "errors.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "general.h"
#include "image_render.h"
#include "interface.h"
#include "layers_dialog.h"
#include "layers_dialogP.h"
#include "lc_dialogP.h"
#include "menus.h"
#include "ops_buttons.h"
#include "paint_funcs.h"
#include "palette.h"
#include "resize.h"
#include "undo.h"

#include "libgimp/gimplimits.h"
#include "libgimp/gimpsizeentry.h"
#include "libgimp/gimpintl.h"

#include "pixmaps/eye.xbm"
#include "pixmaps/linked.xbm"
#include "pixmaps/layer.xbm"
#include "pixmaps/mask.xbm"

#include "pixmaps/new.xpm"
#include "pixmaps/raise.xpm"
#include "pixmaps/lower.xpm"
#include "pixmaps/duplicate.xpm"
#include "pixmaps/delete.xpm"
#include "pixmaps/anchor.xpm"

#include "layer_pvt.h"

#define PREVIEW_EVENT_MASK GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | \
                           GDK_ENTER_NOTIFY_MASK
#define BUTTON_EVENT_MASK  GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK | \
                           GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | \
                           GDK_BUTTON_RELEASE_MASK

#define LAYER_LIST_WIDTH  200
#define LAYER_LIST_HEIGHT 150

#define LAYER_PREVIEW 0
#define MASK_PREVIEW  1
#define FS_PREVIEW    2

#define NORMAL      0
#define SELECTED    1
#define INSENSITIVE 2

typedef struct _LayersDialog LayersDialog;

struct _LayersDialog
{
  GtkWidget *vbox;
  GtkWidget *mode_option_menu;
  GtkWidget *layer_list;
  GtkWidget *preserve_trans;
  GtkWidget *mode_box;
  GtkWidget *opacity_box;
  GtkWidget *ops_menu;
  GtkAccelGroup *accel_group;
  GtkAdjustment *opacity_data;
  GdkGC *red_gc;     /*  for non-applied layer masks  */
  GdkGC *green_gc;   /*  for visible layer masks      */
  GtkWidget *layer_preview;
  double ratio;
  int image_width, image_height;
  int gimage_width, gimage_height;

  /*  state information  */
  GimpImage *gimage;
  Layer     *active_layer;
  Channel   *active_channel;
  Layer     *floating_sel;
  GSList    *layer_widgets;
};

typedef struct _LayerWidget LayerWidget;

struct _LayerWidget
{
  GtkWidget *eye_widget;
  GtkWidget *linked_widget;
  GtkWidget *clip_widget;
  GtkWidget *layer_preview;
  GtkWidget *mask_preview;
  GtkWidget *list_item;
  GtkWidget *label;

  GImage *gimage;
  Layer *layer;
  GdkPixmap *layer_pixmap;
  GdkPixmap *mask_pixmap;
  int active_preview;
  int width, height;

  /*  state information  */
  int layer_mask;
  int apply_mask;
  int edit_mask;
  int show_mask;
  int visited;
};

/*  layers dialog widget routines  */
static void layers_dialog_preview_extents      (void);
static void layers_dialog_set_menu_sensitivity (void);
static void layers_dialog_set_active_layer     (Layer *);
static void layers_dialog_unset_layer          (Layer *);
static void layers_dialog_position_layer       (Layer *, int);
static void layers_dialog_add_layer            (Layer *);
static void layers_dialog_remove_layer         (Layer *);
static void layers_dialog_add_layer_mask       (Layer *);
static void layers_dialog_remove_layer_mask    (Layer *);
static void paint_mode_menu_callback           (GtkWidget *, gpointer);
static gint paint_mode_menu_get_position       (gint);
static void opacity_scale_update               (GtkAdjustment *, gpointer);
static void preserve_trans_update              (GtkWidget *, gpointer);
static gint layer_list_events                  (GtkWidget *, GdkEvent *);

/*  for (un)installing the menu accelarators  */
static void layers_dialog_map_callback   (GtkWidget *, gpointer);
static void layers_dialog_unmap_callback (GtkWidget *, gpointer);

/*  layer widget function prototypes  */
static LayerWidget *layer_widget_get_ID (Layer *);
static LayerWidget *layer_widget_create (GImage *, Layer *);

static void layer_widget_delete            (LayerWidget *);
static void layer_widget_select_update     (GtkWidget *, gpointer);
static gint layer_widget_button_events     (GtkWidget *, GdkEvent *);
static gint layer_widget_preview_events    (GtkWidget *, GdkEvent *);
static void layer_widget_boundary_redraw   (LayerWidget *, int);
static void layer_widget_preview_redraw    (LayerWidget *, int);
static void layer_widget_no_preview_redraw (LayerWidget *, int);
static void layer_widget_eye_redraw        (LayerWidget *);
static void layer_widget_linked_redraw     (LayerWidget *);
static void layer_widget_clip_redraw       (LayerWidget *);
static void layer_widget_exclusive_visible (LayerWidget *);
static void layer_widget_layer_flush       (GtkWidget *, gpointer);

/*  assorted query dialogs  */
static void layers_dialog_new_layer_query    (GimpImage*);
static void layers_dialog_edit_layer_query   (LayerWidget *);
static void layers_dialog_add_mask_query     (Layer *);
static void layers_dialog_apply_mask_query   (Layer *);
static void layers_dialog_scale_layer_query  (GImage *, Layer *);
static void layers_dialog_resize_layer_query (GImage *, Layer *);
void        layers_dialog_layer_merge_query  (GImage *, int);

/*
 *  Local data
 */
static LayersDialog *layersD = NULL;

static GdkPixmap *eye_pixmap[]    = { NULL, NULL, NULL };
static GdkPixmap *linked_pixmap[] = { NULL, NULL, NULL };
static GdkPixmap *layer_pixmap[]  = { NULL, NULL, NULL };
static GdkPixmap *mask_pixmap[]   = { NULL, NULL, NULL };

static int suspend_gimage_notify = 0;

/*  the option menu items -- the paint modes  */
static MenuItem option_items[] =
{
  { N_("Normal"), 0, 0,
    paint_mode_menu_callback, (gpointer) NORMAL_MODE, NULL, NULL },
  { N_("Dissolve"), 0, 0,
    paint_mode_menu_callback, (gpointer) DISSOLVE_MODE, NULL, NULL },
  { N_("Multiply (Burn)"), 0, 0,
    paint_mode_menu_callback, (gpointer) MULTIPLY_MODE, NULL, NULL },
  { N_("Divide (Dodge)"), 0, 0,
    paint_mode_menu_callback, (gpointer) DIVIDE_MODE, NULL, NULL },
  { N_("Screen"), 0, 0,
    paint_mode_menu_callback, (gpointer) SCREEN_MODE, NULL, NULL },
  { N_("Overlay"), 0, 0,
    paint_mode_menu_callback, (gpointer) OVERLAY_MODE, NULL, NULL },
  { N_("Difference"), 0, 0,
    paint_mode_menu_callback, (gpointer) DIFFERENCE_MODE, NULL, NULL },
  { N_("Addition"), 0, 0,
    paint_mode_menu_callback, (gpointer) ADDITION_MODE, NULL, NULL },
  { N_("Subtract"), 0, 0,
    paint_mode_menu_callback, (gpointer) SUBTRACT_MODE, NULL, NULL },
  { N_("Darken Only"), 0, 0,
    paint_mode_menu_callback, (gpointer) DARKEN_ONLY_MODE, NULL, NULL },
  { N_("Lighten Only"), 0, 0,
    paint_mode_menu_callback, (gpointer) LIGHTEN_ONLY_MODE, NULL, NULL },
  { N_("Hue"), 0, 0,
    paint_mode_menu_callback, (gpointer) HUE_MODE, NULL, NULL },
  { N_("Saturation"), 0, 0,
    paint_mode_menu_callback, (gpointer) SATURATION_MODE, NULL, NULL },
  { N_("Color"), 0, 0,
    paint_mode_menu_callback, (gpointer) COLOR_MODE, NULL, NULL },
  { N_("Value"), 0, 0,
    paint_mode_menu_callback, (gpointer) VALUE_MODE, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

/*  the ops buttons  */
static OpsButtonCallback raise_layers_ext_callbacks[] = 
{
  layers_dialog_raise_layer_to_top_callback, NULL, NULL, NULL
};

static OpsButtonCallback lower_layers_ext_callbacks[] = 
{
  layers_dialog_lower_layer_to_bottom_callback, NULL, NULL, NULL
};

static OpsButton layers_ops_buttons[] =
{
  { new_xpm, layers_dialog_new_layer_callback, NULL,
    N_("New Layer"), NULL, 0 },
  { raise_xpm, layers_dialog_raise_layer_callback, raise_layers_ext_callbacks,
    N_("Raise Layer    \n"
       "<Shift> To Top"), NULL, 0 },
  { lower_xpm, layers_dialog_lower_layer_callback, lower_layers_ext_callbacks,
    N_("Lower Layer       \n"
       "<Shift> To Bottom"), NULL, 0 },
  { duplicate_xpm, layers_dialog_duplicate_layer_callback, NULL,
    N_("Duplicate Layer"), NULL, 0 },
  { delete_xpm, layers_dialog_delete_layer_callback, NULL,
    N_("Delete Layer"), NULL, 0 },
  { anchor_xpm, layers_dialog_anchor_layer_callback, NULL,
    N_("Anchor Layer"), NULL, 0 },
  { NULL, NULL, NULL, NULL, NULL, 0 }
};


/************************************/
/*  Public layers dialog functions  */
/************************************/

GtkWidget *
layers_dialog_create ()
{
  GtkWidget *vbox;
  GtkWidget *util_box;
  GtkWidget *button_box;
  GtkWidget *label;
  GtkWidget *menu;
  GtkWidget *slider;
  GtkWidget *listbox;
  
  if (! layersD)
    {
      layersD = g_malloc (sizeof (LayersDialog));
      layersD->layer_preview = NULL;
      layersD->gimage = NULL;
      layersD->active_layer = NULL;
      layersD->active_channel = NULL;
      layersD->floating_sel = NULL;
      layersD->layer_widgets = NULL;
      layersD->green_gc = NULL;
      layersD->red_gc = NULL;

      if (preview_size)
	{
	  layersD->layer_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
	  gtk_preview_size (GTK_PREVIEW (layersD->layer_preview),
			    preview_size, preview_size);
	}

      /*  The main vbox  */
      layersD->vbox = vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

      /*  The layers commands pulldown menu  */
      menus_get_layers_menu (&layersD->ops_menu, &layersD->accel_group);

      /*  The Mode option menu, and the preserve transparency  */
      layersD->mode_box = util_box = gtk_hbox_new (FALSE, 1);
      gtk_box_pack_start (GTK_BOX (vbox), util_box, FALSE, FALSE, 0);

      label = gtk_label_new (_("Mode:"));
      gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);

      menu = build_menu (option_items, NULL);
      layersD->mode_option_menu = gtk_option_menu_new ();
      gtk_box_pack_start (GTK_BOX (util_box), layersD->mode_option_menu,
			  FALSE, FALSE, 2);

      gtk_widget_show (label);
      gtk_widget_show (layersD->mode_option_menu);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (layersD->mode_option_menu),
				menu);

      layersD->preserve_trans =
	gtk_check_button_new_with_label (_("Keep Trans."));
      gtk_box_pack_start (GTK_BOX (util_box), layersD->preserve_trans,
			  FALSE, FALSE, 2);
      gtk_signal_connect (GTK_OBJECT (layersD->preserve_trans), "toggled",
			  (GtkSignalFunc) preserve_trans_update,
			  layersD);
      gtk_widget_show (layersD->preserve_trans);
      gtk_widget_show (util_box);

      /*  Opacity scale  */
      layersD->opacity_box = util_box = gtk_hbox_new (FALSE, 1);
      gtk_box_pack_start (GTK_BOX (vbox), util_box, FALSE, FALSE, 0);

      label = gtk_label_new (_("Opacity:"));
      gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
      gtk_widget_show (label);

      layersD->opacity_data =
	GTK_ADJUSTMENT (gtk_adjustment_new (100.0, 0.0, 100.0, 1.0, 1.0, 0.0));
      slider = gtk_hscale_new (layersD->opacity_data);
      gtk_range_set_update_policy (GTK_RANGE (slider),
				   GTK_UPDATE_CONTINUOUS);
      gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_RIGHT);
      gtk_box_pack_start (GTK_BOX (util_box), slider, TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (layersD->opacity_data), "value_changed",
			  (GtkSignalFunc) opacity_scale_update,
			  layersD);
      gtk_widget_show (slider);

      gtk_widget_show (util_box);

      /*  The layers listbox  */
      listbox = gtk_scrolled_window_new (NULL, NULL);
      gtk_widget_set_usize (listbox, LAYER_LIST_WIDTH, LAYER_LIST_HEIGHT);
      gtk_box_pack_start (GTK_BOX (vbox), listbox, TRUE, TRUE, 2);

      layersD->layer_list = gtk_list_new ();
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (listbox),
					     layersD->layer_list);
      gtk_list_set_selection_mode (GTK_LIST (layersD->layer_list),
				   GTK_SELECTION_BROWSE);
      gtk_signal_connect (GTK_OBJECT (layersD->layer_list), "event",
			  (GtkSignalFunc) layer_list_events,
			  layersD);
      gtk_container_set_focus_vadjustment (GTK_CONTAINER (layersD->layer_list),
					   gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (listbox)));
      GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (listbox)->vscrollbar,
			      GTK_CAN_FOCUS);
      
      gtk_widget_show (layersD->layer_list);
      gtk_widget_show (listbox);

      /*  The ops buttons  */
      button_box = ops_button_box_new (lc_dialog->shell, tool_tips,
				       layers_ops_buttons, OPS_BUTTON_NORMAL);
      gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 2);
      gtk_widget_show (button_box);

      /*  Set up signals for map/unmap for the accelerators  */
      gtk_signal_connect (GTK_OBJECT (layersD->vbox), "map",
			  (GtkSignalFunc) layers_dialog_map_callback,
			  NULL);
      gtk_signal_connect (GTK_OBJECT (layersD->vbox), "unmap",
			  (GtkSignalFunc) layers_dialog_unmap_callback,
			  NULL);

      gtk_widget_show (vbox);
    }

  return layersD->vbox;
}

void
layers_dialog_free ()
{
  GSList *list;
  LayerWidget *lw;

  if (layersD == NULL)
    return;

  /*  Free all elements in the layers listbox  */
  gtk_list_clear_items (GTK_LIST (layersD->layer_list), 0, -1);

  list = layersD->layer_widgets;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      list = g_slist_next (list);
      layer_widget_delete (lw);
    }
  layersD->layer_widgets = NULL;
  layersD->active_layer = NULL;
  layersD->active_channel = NULL;
  layersD->floating_sel = NULL;

  if (layersD->layer_preview)
    gtk_object_sink (GTK_OBJECT (layersD->layer_preview));
  if (layersD->green_gc)
    gdk_gc_destroy (layersD->green_gc);
  if (layersD->red_gc)
    gdk_gc_destroy (layersD->red_gc);

  g_free (layersD);
  layersD = NULL;
}

void
layers_dialog_update (GimpImage* gimage)
{
  Layer *layer;
  LayerWidget *lw;
  GSList *list;
  GList *item_list;

  if (! layersD)
    return;
  if (layersD->gimage == gimage)
    return;

  layersD->gimage = gimage;

  suspend_gimage_notify++;

  /*  Free all elements in the layers listbox  */
  gtk_list_clear_items (GTK_LIST (layersD->layer_list), 0, -1);

  list = layersD->layer_widgets;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      list = g_slist_next (list);
      layer_widget_delete (lw);
    }

  if (layersD->layer_widgets)
    g_warning ("layersD->layer_widgets not empty!");
  layersD->layer_widgets = NULL;

  /*  Find the preview extents  */
  layers_dialog_preview_extents ();

  layersD->active_layer = NULL;
  layersD->active_channel = NULL;
  layersD->floating_sel = NULL;

  list = gimage->layers;
  item_list = NULL;

  while (list)
    {
      /*  create a layer list item  */
      layer = (Layer *) list->data;
      lw = layer_widget_create (gimage, layer);
      layersD->layer_widgets = g_slist_append (layersD->layer_widgets, lw);
      item_list = g_list_append (item_list, lw->list_item);

      list = g_slist_next (list);
    }

  /*  get the index of the active layer  */
  if (item_list)
    gtk_list_insert_items (GTK_LIST (layersD->layer_list), item_list, 0);

  suspend_gimage_notify--;
}

void
layers_dialog_flush ()
{
  GImage *gimage;
  Layer *layer;
  LayerWidget *lw;
  GSList *list;
  int gimage_pos;
  int pos;

  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  /*  Check if the gimage extents have changed  */
  if ((gimage->width != layersD->gimage_width) ||
      (gimage->height != layersD->gimage_height))
    {
      layersD->gimage = NULL;
      layers_dialog_update (gimage);
    }

  /*  Set all current layer widgets to visited = FALSE  */
  list = layersD->layer_widgets;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      lw->visited = FALSE;
      list = g_slist_next (list);
    }

  /*  Add any missing layers  */
  list = gimage->layers;
  while (list)
    {
      layer = (Layer *) list->data;
      lw = layer_widget_get_ID (layer);

      /*  If the layer isn't in the layer widget list, add it  */
      if (lw == NULL)
	layers_dialog_add_layer (layer);
      else
	lw->visited = TRUE;

      list = g_slist_next (list);
    }

  /*  Remove any extraneous layers  */
  list = layersD->layer_widgets;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      list = g_slist_next (list);
      if (lw->visited == FALSE)
	layers_dialog_remove_layer (lw->layer);
    }

  /*  Switch positions of items if necessary  */
  list = layersD->layer_widgets;
  pos = 0;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      list = g_slist_next (list);

      if ((gimage_pos = gimage_get_layer_index (gimage, lw->layer)) != pos)
	layers_dialog_position_layer (lw->layer, gimage_pos);

      pos++;
    }

  /*  Set the active layer  */
  if (layersD->active_layer != gimage->active_layer)
    layersD->active_layer = gimage->active_layer;

  /*  Set the active channel  */
  if (layersD->active_channel != gimage->active_channel)
    {
      layersD->active_channel = gimage->active_channel;

      /*  If there is an active channel, this list is single select  */
      if (layersD->active_channel != NULL)
	gtk_list_set_selection_mode (GTK_LIST (layersD->layer_list),
				     GTK_SELECTION_SINGLE);
      else
	gtk_list_set_selection_mode (GTK_LIST (layersD->layer_list),
				     GTK_SELECTION_BROWSE);
    }

  /*  set the menus if floating sel status has changed  */
  if (layersD->floating_sel != gimage->floating_sel)
    layersD->floating_sel = gimage->floating_sel;

  layers_dialog_set_menu_sensitivity ();

  gtk_container_foreach (GTK_CONTAINER (layersD->layer_list),
			 layer_widget_layer_flush, NULL);
}

void
layers_dialog_clear ()
{
  ops_button_box_set_insensitive (layers_ops_buttons);

  suspend_gimage_notify++;
  gtk_list_clear_items (GTK_LIST (layersD->layer_list), 0, -1);
  suspend_gimage_notify--;

  layersD->gimage = NULL;
}

/***********************/
/*  Preview functions  */
/***********************/

void
render_preview (TempBuf   *preview_buf,
		GtkWidget *preview_widget,
		int        width,
		int        height,
		int        channel)
{
  unsigned char *src, *s;
  unsigned char *cb;
  unsigned char *buf;
  int a;
  int i, j, b;
  int x1, y1, x2, y2;
  int rowstride;
  int color_buf;
  int color;
  int alpha;
  int has_alpha;
  int image_bytes;
  int offset;

  alpha = ALPHA_PIX;

  /*  Here are the different cases this functions handles correctly:
   *  1)  Offset preview_buf which does not necessarily cover full image area
   *  2)  Color conversion of preview_buf if it is gray and image is color
   *  3)  Background check buffer for transparent preview_bufs
   *  4)  Using the optional "channel" argument, one channel can be extracted
   *      from a multi-channel preview_buf and composited as a grayscale
   *  Prereqs:
   *  1)  Grayscale preview_bufs have bytes == {1, 2}
   *  2)  Color preview_bufs have bytes == {3, 4}
   *  3)  If image is gray, then preview_buf should have bytes == {1, 2}
   */
  color_buf = (GTK_PREVIEW (preview_widget)->type == GTK_PREVIEW_COLOR);
  image_bytes = (color_buf) ? 3 : 1;
  has_alpha = (preview_buf->bytes == 2 || preview_buf->bytes == 4);
  rowstride = preview_buf->width * preview_buf->bytes;

  /*  Determine if the preview buf supplied is color
   *   Generally, if the bytes == {3, 4}, this is true.
   *   However, if the channel argument supplied is not -1, then
   *   the preview buf is assumed to be gray despite the number of
   *   channels it contains
   */
  color = (preview_buf->bytes == 3 || preview_buf->bytes == 4) && (channel == -1);

  if (has_alpha)
    {
      buf = check_buf;
      alpha = (color) ? ALPHA_PIX : ((channel != -1) ? (preview_buf->bytes - 1) : ALPHA_G_PIX);
    }
  else
    buf = empty_buf;

  x1 = BOUNDS (preview_buf->x, 0, width);
  y1 = BOUNDS (preview_buf->y, 0, height);
  x2 = BOUNDS (preview_buf->x + preview_buf->width, 0, width);
  y2 = BOUNDS (preview_buf->y + preview_buf->height, 0, height);

  src = temp_buf_data (preview_buf) + (y1 - preview_buf->y) * rowstride +
    (x1 - preview_buf->x) * preview_buf->bytes;

  /*  One last thing for efficiency's sake:  */
  if (channel == -1)
    channel = 0;

  for (i = 0; i < height; i++)
    {
      if (i & 0x4)
	{
	  offset = 4;
	  cb = buf + offset * 3;
	}
      else
	{
	  offset = 0;
	  cb = buf;
	}

      /*  The interesting stuff between leading & trailing vertical transparency  */
      if (i >= y1 && i < y2)
	{
	  /*  Handle the leading transparency  */
	  for (j = 0; j < x1; j++)
	    for (b = 0; b < image_bytes; b++)
	      temp_buf[j * image_bytes + b] = cb[j * 3 + b];

	  /*  The stuff in the middle  */
	  s = src;
	  for (j = x1; j < x2; j++)
	    {
	      if (color)
		{
		  if (has_alpha)
		    {
		      a = s[alpha] << 8;

		      if ((j + offset) & 0x4)
			{
			  temp_buf[j * 3 + 0] = blend_dark_check [(a | s[RED_PIX])];
			  temp_buf[j * 3 + 1] = blend_dark_check [(a | s[GREEN_PIX])];
			  temp_buf[j * 3 + 2] = blend_dark_check [(a | s[BLUE_PIX])];
			}
		      else
			{
			  temp_buf[j * 3 + 0] = blend_light_check [(a | s[RED_PIX])];
			  temp_buf[j * 3 + 1] = blend_light_check [(a | s[GREEN_PIX])];
			  temp_buf[j * 3 + 2] = blend_light_check [(a | s[BLUE_PIX])];
			}
		    }
		  else
		    {
		      temp_buf[j * 3 + 0] = s[RED_PIX];
		      temp_buf[j * 3 + 1] = s[GREEN_PIX];
		      temp_buf[j * 3 + 2] = s[BLUE_PIX];
		    }
		}
	      else
		{
		  if (has_alpha)
		    {
		      a = s[alpha] << 8;

		      if ((j + offset) & 0x4)
			{
			  if (color_buf)
			    {
			      temp_buf[j * 3 + 0] = blend_dark_check [(a | s[GRAY_PIX])];
			      temp_buf[j * 3 + 1] = blend_dark_check [(a | s[GRAY_PIX])];
			      temp_buf[j * 3 + 2] = blend_dark_check [(a | s[GRAY_PIX])];
			    }
			  else
			    temp_buf[j] = blend_dark_check [(a | s[GRAY_PIX + channel])];
			}
		      else
			{
			  if (color_buf)
			    {
			      temp_buf[j * 3 + 0] = blend_light_check [(a | s[GRAY_PIX])];
			      temp_buf[j * 3 + 1] = blend_light_check [(a | s[GRAY_PIX])];
			      temp_buf[j * 3 + 2] = blend_light_check [(a | s[GRAY_PIX])];
			    }
			  else
			    temp_buf[j] = blend_light_check [(a | s[GRAY_PIX + channel])];
			}
		    }
		  else
		    {
		      if (color_buf)
			{
			  temp_buf[j * 3 + 0] = s[GRAY_PIX];
			  temp_buf[j * 3 + 1] = s[GRAY_PIX];
			  temp_buf[j * 3 + 2] = s[GRAY_PIX];
			}
		      else
			temp_buf[j] = s[GRAY_PIX + channel];
		    }
		}

	      s += preview_buf->bytes;
	    }

	  /*  Handle the trailing transparency  */
	  for (j = x2; j < width; j++)
	    for (b = 0; b < image_bytes; b++)
	      temp_buf[j * image_bytes + b] = cb[j * 3 + b];

	  src += rowstride;
	}
      else
	{
	  for (j = 0; j < width; j++)
	    for (b = 0; b < image_bytes; b++)
	      temp_buf[j * image_bytes + b] = cb[j * 3 + b];
	}

      gtk_preview_draw_row (GTK_PREVIEW (preview_widget), temp_buf, 0, i, width);
    }
}


void
render_fs_preview (GtkWidget *widget,
		   GdkPixmap *pixmap)
{
  int w, h;
  int x1, y1, x2, y2;
  GdkPoint poly[6];
  int foldh, foldw;
  int i;

  gdk_window_get_size (pixmap, &w, &h);

  x1 = 2;
  y1 = h / 8 + 2;
  x2 = w - w / 8 - 2;
  y2 = h - 2;
  gdk_draw_rectangle (pixmap, widget->style->bg_gc[GTK_STATE_NORMAL], 1,
		      0, 0, w, h);
  gdk_draw_rectangle (pixmap, widget->style->black_gc, 0,
		      x1, y1, (x2 - x1), (y2 - y1));

  foldw = w / 4;
  foldh = h / 4;
  x1 = w / 8 + 2;
  y1 = 2;
  x2 = w - 2;
  y2 = h - h / 8 - 2;

  poly[0].x = x1 + foldw; poly[0].y = y1;
  poly[1].x = x1 + foldw; poly[1].y = y1 + foldh;
  poly[2].x = x1; poly[2].y = y1 + foldh;
  poly[3].x = x1; poly[3].y = y2;
  poly[4].x = x2; poly[4].y = y2;
  poly[5].x = x2; poly[5].y = y1;
  gdk_draw_polygon (pixmap, widget->style->white_gc, 1, poly, 6);

  gdk_draw_line (pixmap, widget->style->black_gc,
		 x1, y1 + foldh, x1, y2);
  gdk_draw_line (pixmap, widget->style->black_gc,
		 x1, y2, x2, y2);
  gdk_draw_line (pixmap, widget->style->black_gc,
		 x2, y2, x2, y1);
  gdk_draw_line (pixmap, widget->style->black_gc,
		 x1 + foldw, y1, x2, y1);
  for (i = 0; i < foldw; i++)
    gdk_draw_line (pixmap, widget->style->black_gc,
		   x1 + i, y1 + foldh, x1 + i, (foldw == 1) ? y1 :
		   (y1 + (foldh - (foldh * i) / (foldw - 1))));
}


/*************************************/
/*  layers dialog widget routines    */
/*************************************/

static void
layers_dialog_preview_extents ()
{
  GImage *gimage;

  if (! layersD)
    return;

  gimage = layersD->gimage;

  layersD->gimage_width = gimage->width;
  layersD->gimage_height = gimage->height;

  /*  Get the image width and height variables, based on the gimage  */
  if (gimage->width > gimage->height)
    layersD->ratio = (double) preview_size / (double) gimage->width;
  else
    layersD->ratio = (double) preview_size / (double) gimage->height;

  if (preview_size)
    {
      layersD->image_width = (int) (layersD->ratio * gimage->width);
      layersD->image_height = (int) (layersD->ratio * gimage->height);
      if (layersD->image_width < 1) layersD->image_width = 1;
      if (layersD->image_height < 1) layersD->image_height = 1;
    }
  else
    {
      layersD->image_width = layer_width;
      layersD->image_height = layer_height;
    }
}

static void
layers_dialog_set_menu_sensitivity ()
{
  gint fs;         /*  floating sel  */
  gint ac;         /*  active channel  */
  gint lm;         /*  layer mask  */
  gint gimage;     /*  is there a gimage  */
  gint lp;         /*  layers present  */
  gint alpha;      /*  alpha channel present  */
  gint next_alpha;
  GSList *list; 
  GSList *next;
  GSList *prev;
  Layer *layer;

  lp = FALSE;

  if (! layersD)
    return;

  if ((layer =  (layersD->active_layer)) != NULL)
    lm = (layer->mask) ? TRUE : FALSE;
  else
    lm = FALSE;

  fs = (layersD->floating_sel == NULL);
  ac = (layersD->active_channel == NULL);
  gimage = (layersD->gimage != NULL);
  alpha = layer && layer_has_alpha (layer);

  if (gimage)
    lp = (layersD->gimage->layers != NULL);

  list = layersD->gimage->layers;
  prev = NULL;
  next = NULL;
  while (list)
    {
      layer = (Layer *)list->data;
      if (layer == (layersD->active_layer))
	{
	  next = g_slist_next (list);
	  break;
	}
      prev = list;
      list = g_slist_next (list);
    }

  if (next)
    {
      layer = (Layer *)next->data;
      next_alpha = layer_has_alpha (layer);
    }
  else
    next_alpha = FALSE;

  menus_set_sensitive_locale ("<Layers>", N_("/Stack/Previous Layer"),
		       fs && ac && gimage && lp && prev);
  menus_set_sensitive_locale ("<Layers>", N_("/Stack/Next Layer"),
		       fs && ac && gimage && lp && next);

  menus_set_sensitive_locale ("<Layers>", N_("/Stack/Raise Layer"),
		       fs && ac && gimage && lp && alpha && prev);
  gtk_widget_set_sensitive (layers_ops_buttons[1].widget,
			    fs && ac && gimage && lp && alpha && prev);

  menus_set_sensitive_locale ("<Layers>", N_("/Stack/Lower Layer"),
		       fs && ac && gimage && lp && next && next_alpha);
  gtk_widget_set_sensitive (layers_ops_buttons[2].widget,
			    fs && ac && gimage && lp && next && next_alpha);

  menus_set_sensitive_locale ("<Layers>", N_("/Stack/Layer to Top"),
		       fs && ac && gimage && lp && alpha && prev);
  menus_set_sensitive_locale ("<Layers>", N_("/Stack/Layer to Bottom"),
		       fs && ac && gimage && lp && next && next_alpha);

  menus_set_sensitive_locale ("<Layers>", N_("/New Layer"), gimage);
  gtk_widget_set_sensitive (layers_ops_buttons[0].widget, gimage);

  menus_set_sensitive_locale ("<Layers>", N_("/Duplicate Layer"), fs && ac && gimage && lp);
  gtk_widget_set_sensitive (layers_ops_buttons[3].widget,
			    fs && ac && gimage && lp);

  menus_set_sensitive_locale ("<Layers>", N_("/Delete Layer"), ac && gimage && lp);
  gtk_widget_set_sensitive (layers_ops_buttons[4].widget, ac && gimage && lp);

  menus_set_sensitive_locale ("<Layers>", N_("/Anchor Layer"), !fs && ac && gimage && lp);
  gtk_widget_set_sensitive (layers_ops_buttons[5].widget,
			    !fs && ac && gimage && lp);

  menus_set_sensitive_locale ("<Layers>", N_("/Scale Layer"), ac && gimage && lp);
  menus_set_sensitive_locale ("<Layers>", N_("/Resize Layer"), ac && gimage && lp);

  menus_set_sensitive_locale ("<Layers>", N_("/Merge Visible Layers"),
		       fs && ac && gimage && lp);
  menus_set_sensitive_locale ("<Layers>", N_("/Merge Down"), fs && ac && gimage && lp);
  menus_set_sensitive_locale ("<Layers>", N_("/Flatten Image"), fs && ac && gimage && lp);

  menus_set_sensitive_locale ("<Layers>", N_("/Add Layer Mask"),
		       fs && ac && gimage && !lm && lp && alpha);
  menus_set_sensitive_locale ("<Layers>", N_("/Apply Layer Mask"),
		       fs && ac && gimage && lm && lp);
  menus_set_sensitive_locale ("<Layers>", N_("/Alpha to Selection"),
		       fs && ac && gimage && lp && alpha);
  menus_set_sensitive_locale ("<Layers>", N_("/Mask to Selection"),
		       fs && ac && gimage && lm && lp);
  menus_set_sensitive_locale ("<Layers>", N_("/Add Alpha Channel"), !alpha);

  /*  set mode, preserve transparency and opacity to insensitive
   *  if there are no layers
   */
  gtk_widget_set_sensitive (layersD->preserve_trans, lp);
  gtk_widget_set_sensitive (layersD->opacity_box, lp);
  gtk_widget_set_sensitive (layersD->mode_box, lp);
}


static void
layers_dialog_set_active_layer (Layer * layer)
{
  LayerWidget *layer_widget;
  GtkStateType state;
  int index;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  state = layer_widget->list_item->state;
  index = gimage_get_layer_index (layer_widget->gimage, layer);
  if ((index >= 0) && (state != GTK_STATE_SELECTED))
    {
      gtk_object_set_user_data (GTK_OBJECT (layer_widget->list_item), NULL);
      gtk_list_select_item (GTK_LIST (layersD->layer_list), index);
      gtk_object_set_user_data (GTK_OBJECT (layer_widget->list_item), layer_widget);
    }

  suspend_gimage_notify--;
}


static void
layers_dialog_unset_layer (Layer * layer)
{
  LayerWidget *layer_widget;
  GtkStateType state;
  int index;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  state = layer_widget->list_item->state;
  index = gimage_get_layer_index (layer_widget->gimage, layer);
  if ((index >= 0) && (state == GTK_STATE_SELECTED))
    {
      gtk_object_set_user_data (GTK_OBJECT (layer_widget->list_item), NULL);
      gtk_list_unselect_item (GTK_LIST (layersD->layer_list), index);
      gtk_object_set_user_data (GTK_OBJECT (layer_widget->list_item), layer_widget);
    }

  suspend_gimage_notify--;
}


static void
layers_dialog_position_layer (Layer * layer,
			      int     new_index)
{
  LayerWidget *layer_widget;
  GList *list = NULL;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  Remove the layer from the dialog  */
  list = g_list_append (list, layer_widget->list_item);
  gtk_list_remove_items (GTK_LIST (layersD->layer_list), list);
  layersD->layer_widgets = g_slist_remove (layersD->layer_widgets, layer_widget);

  /*  Add it back at the proper index  */
  gtk_list_insert_items (GTK_LIST (layersD->layer_list), list, new_index);
  layersD->layer_widgets = g_slist_insert (layersD->layer_widgets, layer_widget, new_index);

  suspend_gimage_notify--;
}


static void
layers_dialog_add_layer (Layer *layer)
{
  GImage *gimage;
  GList *item_list;
  LayerWidget *layer_widget;
  int position;

  if (!layersD || !layer)
    return;
  if (! (gimage = layersD->gimage))
    return;

  item_list = NULL;

  layer_widget = layer_widget_create (gimage, layer);
  item_list = g_list_append (item_list, layer_widget->list_item);

  position = gimage_get_layer_index (gimage, layer);
  layersD->layer_widgets = g_slist_insert (layersD->layer_widgets, layer_widget, position);
  gtk_list_insert_items (GTK_LIST (layersD->layer_list), item_list, position);
}


static void
layers_dialog_remove_layer (Layer * layer)
{
  LayerWidget *layer_widget;
  GList *list = NULL;

  layer_widget = layer_widget_get_ID (layer);

  if (!layersD || !layer_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  Remove the requested layer from the dialog  */
  list = g_list_append (list, layer_widget->list_item);
  gtk_list_remove_items (GTK_LIST (layersD->layer_list), list);

  /*  Delete layer widget  */
  layer_widget_delete (layer_widget);

  suspend_gimage_notify--;
}


static void
layers_dialog_add_layer_mask (Layer * layer)
{
  LayerWidget *layer_widget;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  if (! GTK_WIDGET_VISIBLE (layer_widget->mask_preview))
    gtk_widget_show (layer_widget->mask_preview);

  layer_widget->active_preview = MASK_PREVIEW;

  gtk_widget_draw (layer_widget->layer_preview, NULL);
}


static void
layers_dialog_remove_layer_mask (Layer * layer)
{
  LayerWidget *layer_widget;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  if (GTK_WIDGET_VISIBLE (layer_widget->mask_preview))
    gtk_widget_hide (layer_widget->mask_preview);

  layer_widget->active_preview = LAYER_PREVIEW;

  gtk_widget_draw (layer_widget->layer_preview, NULL);
}

static gint
paint_mode_menu_get_position (gint mode)
{
  /*  FIXME this is an ugly hack that should stay around only until 
   *  the layers dialog is rewritten 
   */
  int i;

  for (i = 0; option_items[i].label != NULL; i++)
    if (mode == (gint) (option_items[i].user_data))
      return i;

  g_message (_("Unknown layer mode"));
  return 0;
}

static void
paint_mode_menu_callback (GtkWidget *w,
			  gpointer   client_data)
{
  GImage *gimage;
  Layer *layer;
  int mode;

  if (! (gimage = layersD->gimage))
    return;
  if (! (layer = gimage->active_layer))
    return;

  /*  If the layer has an alpha channel, set the transparency and redraw  */
  if (layer_has_alpha (layer))
    {
      mode = (long) client_data;
      if (layer->mode != mode)
	{
	  layer->mode = mode;

	  drawable_update (GIMP_DRAWABLE (layer), 0, 0, GIMP_DRAWABLE (layer)->width, GIMP_DRAWABLE (layer)->height);
	  gdisplays_flush ();
	}
    }
}


static void
opacity_scale_update (GtkAdjustment *adjustment,
		      gpointer       data)
{
  GImage *gimage;
  Layer *layer;
  int opacity;

  if (! (gimage = layersD->gimage))
    return;
  if (! (layer = gimage->active_layer))
    return;

  /*  add the 0.001 to insure there are no subtle rounding errors  */
  opacity = (int) (adjustment->value * 2.55 + 0.001);
  if (layer->opacity != opacity)
    {
      layer->opacity = opacity;

      drawable_update (GIMP_DRAWABLE (layer), 0, 0,
		       GIMP_DRAWABLE (layer)->width,
		       GIMP_DRAWABLE (layer)->height);
      gdisplays_flush ();
    }
}


static void
preserve_trans_update (GtkWidget *w,
		       gpointer   data)
{
  GImage *gimage;
  Layer *layer;

  if (! (gimage = layersD->gimage))
    return;
  if (! (layer = gimage->active_layer))
    return;

  if (GTK_TOGGLE_BUTTON (w)->active)
    layer->preserve_trans = TRUE;
  else
    layer->preserve_trans = FALSE;
}


static gint
layer_list_events (GtkWidget *widget,
		   GdkEvent  *event)
{
  GdkEventKey *kevent;
  GdkEventButton *bevent;
  GtkWidget *event_widget;
  LayerWidget *layer_widget;

  event_widget = gtk_get_event_widget (event);

  if (GTK_IS_LIST_ITEM (event_widget))
    {
      layer_widget =
	(LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (event_widget));

      switch (event->type)
	{
	case GDK_BUTTON_PRESS:
	  bevent = (GdkEventButton *) event;

	  if (bevent->button == 3 || bevent->button == 2)
	    {
	      gtk_menu_popup (GTK_MENU (layersD->ops_menu),
			      NULL, NULL, NULL, NULL,
			      bevent->button, bevent->time);
	      return TRUE;
	    }
	  break;

	case GDK_2BUTTON_PRESS:
	  bevent = (GdkEventButton *) event;
	  layers_dialog_edit_layer_query (layer_widget);
	  return TRUE;

	case GDK_KEY_PRESS:
	  kevent = (GdkEventKey *) event;
	  switch (kevent->keyval)
	    {
	    case GDK_Up:
	      /* printf ("up arrow\n"); */
	      break;
	    case GDK_Down:
	      /* printf ("down arrow\n"); */
	      break;
	    default:
	      return FALSE;
	    }
	  return FALSE;

	default:
	  break;
	}
    }

  return FALSE;
}


/*****************************/
/*  layers dialog callbacks  */
/*****************************/

static void
layers_dialog_map_callback (GtkWidget *w,
			    gpointer   client_data)
{
  if (! layersD)
    return;

  gtk_window_add_accel_group (GTK_WINDOW (lc_dialog->shell),
			      layersD->accel_group);
}

static void
layers_dialog_unmap_callback (GtkWidget *w,
			      gpointer   client_data)
{
  if (! layersD)
    return;

  gtk_window_remove_accel_group (GTK_WINDOW (lc_dialog->shell),
				 layersD->accel_group);
}

void
layers_dialog_previous_layer_callback (GtkWidget *w,
				       gpointer   client_data)
{
  GImage *gimage;
  int current_layer;
  Layer *new_layer;

  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  current_layer =
    gimage_get_layer_index (gimage, gimage->active_layer);

  /*  FIXME: don't use internal knowledge about layer lists
   *  TODO : implement gimage_get_layer_by_index()
   */
  new_layer =
    (Layer *) g_slist_nth_data (gimage->layers, current_layer - 1);

  if (new_layer)
    {
      gimage_set_active_layer (gimage, new_layer);
      gdisplays_flush ();
    }
}

void
layers_dialog_next_layer_callback (GtkWidget *w,
				   gpointer   client_data)
{
  GImage *gimage;
  int current_layer;
  Layer *new_layer;

  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  current_layer =
    gimage_get_layer_index (gimage, gimage->active_layer);

  /*  FIXME: don't use internal knowledge about layer lists
   *  TODO : implement gimage_get_layer_by_index()
   */
  new_layer =
    (Layer *) g_slist_nth_data (gimage->layers, current_layer + 1);

  if (new_layer)
    {
      gimage_set_active_layer (gimage, new_layer);
      gdisplays_flush ();
    }
}

void
layers_dialog_raise_layer_callback (GtkWidget *w,
				    gpointer   client_data)
{
  GImage *gimage;

  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  gimage_raise_layer (gimage, gimage->active_layer);
  gdisplays_flush ();
}

void
layers_dialog_lower_layer_callback (GtkWidget *w,
				    gpointer   client_data)
{
  GImage *gimage;

  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  gimage_lower_layer (gimage, gimage->active_layer);
  gdisplays_flush ();
}

void
layers_dialog_raise_layer_to_top_callback (GtkWidget *w,
					   gpointer   client_data)
{
  GImage *gimage;

  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  if (NULL != gimage_raise_layer_to_top (gimage, gimage->active_layer))
    {
      /* update, only needed if raise was performed */
      gdisplays_flush ();
    }
}


void
layers_dialog_lower_layer_to_bottom_callback (GtkWidget *w,
					      gpointer   client_data)
{
  GImage *gimage;

  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  if (NULL != gimage_lower_layer_to_bottom (gimage, gimage->active_layer))
    {
      /* update, only needed if lower was performed */
      gdisplays_flush ();
    }
}

void
layers_dialog_new_layer_callback (GtkWidget *w,
				  gpointer   client_data)
{
  GImage *gimage;
  Layer *layer;

  /*  if there is a currently selected gimage, request a new layer
   */
  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  /*  If there is a floating selection, the new command transforms
   *  the current fs into a new layer
   */
  if ((layer = gimage_floating_sel (gimage)))
    {
      floating_sel_to_layer (layer);

      gdisplays_flush ();
    }
  else
    layers_dialog_new_layer_query (layersD->gimage);
}

void
layers_dialog_duplicate_layer_callback (GtkWidget *w,
					gpointer   client_data)
{
  GImage *gimage;
  Layer *active_layer;
  Layer *new_layer;

  /*  if there is a currently selected gimage, request a new layer
   */
  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  /*  Start a group undo  */
  undo_push_group_start (gimage, EDIT_PASTE_UNDO);

  active_layer = gimage_get_active_layer (gimage);
  new_layer = layer_copy (active_layer, TRUE);
  gimage_add_layer (gimage, new_layer, -1);

  /*  end the group undo  */
  undo_push_group_end (gimage);

  gdisplays_flush ();
}

void
layers_dialog_delete_layer_callback (GtkWidget *w,
				     gpointer   client_data)
{
  GImage *gimage;
  Layer *layer;

  /*  if there is a currently selected gimage
   */
  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  if (! (layer = gimage_get_active_layer (gimage)))
    return;

  /*  if the layer is a floating selection, take special care  */
  if (layer_is_floating_sel (layer))
    floating_sel_remove (layer);
  else
    gimage_remove_layer (gimage, gimage->active_layer);

  gdisplays_flush ();
}

void
layers_dialog_scale_layer_callback (GtkWidget *w,
				    gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  layers_dialog_scale_layer_query (gimage, gimage->active_layer);
}

void
layers_dialog_resize_layer_callback (GtkWidget *w,
				     gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  layers_dialog_resize_layer_query (gimage, gimage->active_layer);
}

void
layers_dialog_add_layer_mask_callback (GtkWidget *w,
				       gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  layers_dialog_add_mask_query (gimage->active_layer);
}

void
layers_dialog_apply_layer_mask_callback (GtkWidget *w,
					 gpointer   client_data)
{
  GImage *gimage;
  Layer *layer;

  /*  if there is a currently selected gimage
   */
  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  /*  Make sure there is a layer mask to apply  */
  if ((layer =  gimage->active_layer) != NULL)
    {
      if (layer->mask)
	layers_dialog_apply_mask_query (gimage->active_layer);
    }
}

void
layers_dialog_anchor_layer_callback (GtkWidget *w,
				     gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  floating_sel_anchor (gimage_get_active_layer (gimage));
  gdisplays_flush ();
}

void
layers_dialog_merge_layers_callback (GtkWidget *w,
				     gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  layers_dialog_layer_merge_query (gimage, TRUE);
}

void
layers_dialog_merge_down_callback (GtkWidget *w,
				   gpointer   client_data)
{
  GImage *gimage;

  /* if there is a currently selected gimage
   */
  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;
  
  gimp_image_merge_down (gimage, gimage->active_layer, EXPAND_AS_NECESSARY);
  gdisplays_flush ();
}

void
layers_dialog_flatten_image_callback (GtkWidget *w,
				      gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  gimage_flatten (gimage);
  gdisplays_flush ();
}


void
layers_dialog_alpha_select_callback (GtkWidget *w,
				     gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  gimage_mask_layer_alpha (gimage, gimage->active_layer);
  gdisplays_flush ();
}


void
layers_dialog_mask_select_callback (GtkWidget *w,
				    gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;

  gimage_mask_layer_mask (gimage, gimage->active_layer);
  gdisplays_flush ();
}


void
layers_dialog_add_alpha_channel_callback (GtkWidget *w,
					  gpointer   client_data)
{
  GImage *gimage;
  Layer  *layer;

  /*  if there is a currently selected gimage
   */
  if (! layersD)
    return;
  if (! (gimage = layersD->gimage))
    return;
  if (! (layer = gimage_get_active_layer (gimage)))
    return;

  /*  Add an alpha channel  */
  layer_add_alpha (layer);
  gdisplays_flush ();
}


/****************************/
/*  layer widget functions  */
/****************************/

static LayerWidget *
layer_widget_get_ID (Layer * ID)
{
  LayerWidget *lw;
  GSList *list;

  if (! layersD)
    return NULL;

  list = layersD->layer_widgets;

  while (list)
    {
      lw = (LayerWidget *) list->data;
      if (lw->layer == ID)
	return lw;

      list = g_slist_next(list);
    }

  return NULL;
}

static LayerWidget *
layer_widget_create (GImage *gimage,
		     Layer  *layer)
{
  LayerWidget *layer_widget;
  GtkWidget *list_item;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *alignment;

  list_item = gtk_list_item_new ();

  /*  create the layer widget and add it to the list  */
  layer_widget = (LayerWidget *) g_malloc (sizeof (LayerWidget));
  layer_widget->gimage = gimage;
  layer_widget->layer = layer;
  layer_widget->layer_preview = NULL;
  layer_widget->mask_preview = NULL;
  layer_widget->layer_pixmap = NULL;
  layer_widget->mask_pixmap = NULL;
  layer_widget->list_item = list_item;
  layer_widget->width = -1;
  layer_widget->height = -1;
  layer_widget->layer_mask = (layer->mask != NULL);
  layer_widget->apply_mask = layer->apply_mask;
  layer_widget->edit_mask = layer->edit_mask;
  layer_widget->show_mask = layer->show_mask;
  layer_widget->visited = TRUE;

  if (layer->mask)
    layer_widget->active_preview = (layer->edit_mask) ? MASK_PREVIEW : LAYER_PREVIEW;
  else
    layer_widget->active_preview = LAYER_PREVIEW;

  /*  Need to let the list item know about the layer_widget  */
  gtk_object_set_user_data (GTK_OBJECT (list_item), layer_widget);

  /*  set up the list item observer  */
  gtk_signal_connect (GTK_OBJECT (list_item), "select",
		      (GtkSignalFunc) layer_widget_select_update,
		      layer_widget);
  gtk_signal_connect (GTK_OBJECT (list_item), "deselect",
		      (GtkSignalFunc) layer_widget_select_update,
		      layer_widget);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (list_item), vbox);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);

  /* Create the visibility toggle button */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, TRUE, 2);
  layer_widget->eye_widget = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (layer_widget->eye_widget),
			 eye_width, eye_height);
  gtk_widget_set_events (layer_widget->eye_widget, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (layer_widget->eye_widget), "event",
		      (GtkSignalFunc) layer_widget_button_events,
		      layer_widget);
  gtk_object_set_user_data (GTK_OBJECT (layer_widget->eye_widget), layer_widget);
  gtk_container_add (GTK_CONTAINER (alignment), layer_widget->eye_widget);
  gtk_widget_show (layer_widget->eye_widget);
  gtk_widget_show (alignment);

  /* Create the link toggle button */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, TRUE, 2);
  layer_widget->linked_widget = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (layer_widget->linked_widget),
			 eye_width, eye_height);
  gtk_widget_set_events (layer_widget->linked_widget, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (layer_widget->linked_widget), "event",
		      (GtkSignalFunc) layer_widget_button_events,
		      layer_widget);
  gtk_object_set_user_data (GTK_OBJECT (layer_widget->linked_widget), layer_widget);
  gtk_container_add (GTK_CONTAINER (alignment), layer_widget->linked_widget);
  gtk_widget_show (layer_widget->linked_widget);
  gtk_widget_show (alignment);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 2);
  gtk_widget_show (alignment);

  layer_widget->layer_preview = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (layer_widget->layer_preview),
			 layersD->image_width + 4, layersD->image_height + 4);
  gtk_widget_set_events (layer_widget->layer_preview, PREVIEW_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (layer_widget->layer_preview), "event",
		      (GtkSignalFunc) layer_widget_preview_events,
		      layer_widget);
  gtk_object_set_user_data (GTK_OBJECT (layer_widget->layer_preview), layer_widget);
  gtk_container_add (GTK_CONTAINER (alignment), layer_widget->layer_preview);
  gtk_widget_show (layer_widget->layer_preview);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 2);
  gtk_widget_show (alignment);

  layer_widget->mask_preview = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (layer_widget->mask_preview),
			 layersD->image_width + 4, layersD->image_height + 4);
  gtk_widget_set_events (layer_widget->mask_preview, PREVIEW_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (layer_widget->mask_preview), "event",
		      (GtkSignalFunc) layer_widget_preview_events,
		      layer_widget);
  gtk_object_set_user_data (GTK_OBJECT (layer_widget->mask_preview), layer_widget);
  gtk_container_add (GTK_CONTAINER (alignment), layer_widget->mask_preview);
  if (layer->mask != NULL)
    gtk_widget_show (layer_widget->mask_preview);

  /*  the layer name label */
  if (layer_is_floating_sel (layer))
    layer_widget->label = gtk_label_new (_("Floating Selection"));
  else
    layer_widget->label = gtk_label_new (layer_get_name (layer));
  gtk_box_pack_start (GTK_BOX (hbox), layer_widget->label, FALSE, FALSE, 2);
  gtk_widget_show (layer_widget->label);

  layer_widget->clip_widget = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (layer_widget->clip_widget), 1, 2);
  gtk_widget_set_events (layer_widget->clip_widget, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (layer_widget->clip_widget), "event",
		      (GtkSignalFunc) layer_widget_button_events,
		      layer_widget);
  gtk_object_set_user_data (GTK_OBJECT (layer_widget->clip_widget), layer_widget);
  gtk_box_pack_start (GTK_BOX (vbox), layer_widget->clip_widget,
		      FALSE, FALSE, 0);
  /* gtk_widget_show (layer_widget->clip_widget); */

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (list_item);

  gtk_widget_ref (layer_widget->list_item);

  return layer_widget;
}

static void
layer_widget_delete (LayerWidget *layer_widget)
{
  if (layer_widget->layer_pixmap)
    gdk_pixmap_unref (layer_widget->layer_pixmap);
  if (layer_widget->mask_pixmap)
    gdk_pixmap_unref (layer_widget->mask_pixmap);

  /*  Remove the layer widget from the list  */
  layersD->layer_widgets = g_slist_remove (layersD->layer_widgets, layer_widget);

  /*  Release the widget  */
  gtk_widget_unref (layer_widget->list_item);
  g_free (layer_widget);
}

static void
layer_widget_select_update (GtkWidget *w,
			    gpointer   data)
{
  LayerWidget *layer_widget;

  if (! (layer_widget = (LayerWidget *) data))
    return;

  /*  Is the list item being selected?  */
  if (w->state != GTK_STATE_SELECTED)
    return;

  /*  Only notify the gimage of an active layer change if necessary  */
  if (suspend_gimage_notify == 0)
    {
      /*  set the gimage's active layer to be this layer  */
      gimage_set_active_layer (layer_widget->gimage, layer_widget->layer);

      gdisplays_flush ();
    }
}

static gint
layer_widget_button_events (GtkWidget *widget,
			    GdkEvent  *event)
{
  static int button_down = 0;
  static GtkWidget *click_widget = NULL;
  static int old_state;
  static int exclusive;
  LayerWidget *layer_widget;
  GtkWidget *event_widget;
  GdkEventButton *bevent;
  gint return_val;

  layer_widget = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));
  return_val = FALSE;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (widget == layer_widget->eye_widget)
	layer_widget_eye_redraw (layer_widget);
      else if (widget == layer_widget->linked_widget)
	layer_widget_linked_redraw (layer_widget);
      else if (widget == layer_widget->clip_widget)
	layer_widget_clip_redraw (layer_widget);
      break;

    case GDK_BUTTON_PRESS:
      return_val = TRUE;
      bevent = (GdkEventButton *) event;

      if (bevent->button == 3)
	{
	  gtk_menu_popup (GTK_MENU (layersD->ops_menu),
			  NULL, NULL, NULL, NULL,
			  3, bevent->time);
	  return TRUE;
	}

      button_down = 1;
      click_widget = widget;
      gtk_grab_add (click_widget);
      
      if (widget == layer_widget->eye_widget)
	{
	  old_state = GIMP_DRAWABLE(layer_widget->layer)->visible;

	  /*  If this was a shift-click, make all/none visible  */
	  if (event->button.state & GDK_SHIFT_MASK)
	    {
	      exclusive = TRUE;
	      layer_widget_exclusive_visible (layer_widget);
	    }
	  else
	    {
	      exclusive = FALSE;
	      GIMP_DRAWABLE(layer_widget->layer)->visible = !GIMP_DRAWABLE(layer_widget->layer)->visible;
	      layer_widget_eye_redraw (layer_widget);
	    }
	}
      else if (widget == layer_widget->linked_widget)
	{
	  old_state = layer_widget->layer->linked;
	  layer_widget->layer->linked = !layer_widget->layer->linked;
	  layer_widget_linked_redraw (layer_widget);
	}
      break;

    case GDK_BUTTON_RELEASE:
      return_val = TRUE;

      button_down = 0;
      gtk_grab_remove (click_widget);

      if (widget == layer_widget->eye_widget)
	{
	  if (exclusive)
       	    {
	      gimage_invalidate_preview (layer_widget->gimage);
	      gdisplays_update_area (layer_widget->gimage, 0, 0,
				     layer_widget->gimage->width,
				     layer_widget->gimage->height);
	      gdisplays_flush ();
	    }
	  else if (old_state != GIMP_DRAWABLE(layer_widget->layer)->visible)
	    {
	      /*  Invalidate the gimage preview  */
	      gimage_invalidate_preview (layer_widget->gimage);
	      drawable_update (GIMP_DRAWABLE(layer_widget->layer), 0, 0,
			       GIMP_DRAWABLE(layer_widget->layer)->width,
			       GIMP_DRAWABLE(layer_widget->layer)->height);
	      gdisplays_flush (); 
	    }
	}
      else if ((widget == layer_widget->linked_widget) &&
	       (old_state != layer_widget->layer->linked))
	{
	}
      break;

    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
      event_widget = gtk_get_event_widget (event);

      if (button_down && (event_widget == click_widget))
	{
	  if (widget == layer_widget->eye_widget)
	    {
	      if (exclusive)
		{
		  layer_widget_exclusive_visible (layer_widget);
		}
	      else
		{
		  GIMP_DRAWABLE(layer_widget->layer)->visible = !GIMP_DRAWABLE(layer_widget->layer)->visible;
		  layer_widget_eye_redraw (layer_widget);
		}
	    }
	  else if (widget == layer_widget->linked_widget)
	    {
	      layer_widget->layer->linked = !layer_widget->layer->linked;
	      layer_widget_linked_redraw (layer_widget);
	    }
	}
      break;

    default:
      break;
    }

  return return_val;
}

static gint
layer_widget_preview_events (GtkWidget *widget,
			     GdkEvent  *event)
{
  GdkEventExpose *eevent;
  GdkPixmap **pixmap;
  GdkEventButton *bevent;
  LayerWidget *layer_widget;
  int valid;
  int preview_type;
  int sx, sy, dx, dy, w, h;

  pixmap = NULL;
  valid  = FALSE;

  layer_widget = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  if (widget == layer_widget->layer_preview)
    preview_type = LAYER_PREVIEW;
  else if (widget == layer_widget->mask_preview && GTK_WIDGET_VISIBLE (widget))
    preview_type = MASK_PREVIEW;
  else
    return FALSE;

  switch (preview_type)
    {
    case LAYER_PREVIEW:
      pixmap = &layer_widget->layer_pixmap;
      valid = GIMP_DRAWABLE(layer_widget->layer)->preview_valid;
      break;
    case MASK_PREVIEW:
      pixmap = &layer_widget->mask_pixmap;
      valid = GIMP_DRAWABLE(layer_widget->layer->mask)->preview_valid;
      break;
    }

  if (layer_is_floating_sel (layer_widget->layer))
    preview_type = FS_PREVIEW;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      /*  Control-button press disables the application of the mask  */
      bevent = (GdkEventButton *) event;

      if (bevent->button == 3)
	{
	  gtk_menu_popup (GTK_MENU (layersD->ops_menu),
			  NULL, NULL, NULL, NULL,
			  3, bevent->time);
	  return TRUE;
	}

      if (event->button.state & GDK_CONTROL_MASK)
	{
	  if (preview_type == MASK_PREVIEW)
	    {
	      gimage_set_layer_mask_apply (layer_widget->gimage, layer_widget->layer);
	      gdisplays_flush ();
	    }
	}
      /*  Alt-button press makes the mask visible instead of the layer  */
      else if (event->button.state & GDK_MOD1_MASK)
	{
	  if (preview_type == MASK_PREVIEW)
	    {
	      gimage_set_layer_mask_show (layer_widget->gimage, layer_widget->layer);
	      gdisplays_flush ();
	    }
	}
      else if (layer_widget->active_preview != preview_type)
	{
	  gimage_set_layer_mask_edit (layer_widget->gimage, layer_widget->layer,
				      (preview_type == MASK_PREVIEW) ? 1 : 0);
	  gdisplays_flush ();
	}
      break;

    case GDK_EXPOSE:
      if (!preview_size && preview_type != FS_PREVIEW)
	layer_widget_no_preview_redraw (layer_widget, preview_type);
      else
	{
	  if (!valid || !*pixmap)
	    {
	      layer_widget_preview_redraw (layer_widget, preview_type);

	      gdk_draw_pixmap (widget->window,
			       widget->style->black_gc,
			       *pixmap,
			       0, 0, 2, 2,
			       layersD->image_width,
			       layersD->image_height);
	    }
	  else
	    {
	      eevent = (GdkEventExpose *) event;

	      w = eevent->area.width;
	      h = eevent->area.height;

	      if (eevent->area.x < 2)
		{
		  sx = eevent->area.x;
		  dx = 2;
		  w -= (2 - eevent->area.x);
		}
	      else
		{
		  sx = eevent->area.x - 2;
		  dx = eevent->area.x;
		}

	      if (eevent->area.y < 2)
		{
		  sy = eevent->area.y;
		  dy = 2;
		  h -= (2 - eevent->area.y);
		}
	      else
		{
		  sy = eevent->area.y - 2;
		  dy = eevent->area.y;
		}

	      if ((sx + w) >= layersD->image_width)
		w = layersD->image_width - sx;

	      if ((sy + h) >= layersD->image_height)
		h = layersD->image_height - sy;

	      if ((w > 0) && (h > 0))
		gdk_draw_pixmap (widget->window,
				 widget->style->black_gc,
				 *pixmap,
				 sx, sy, dx, dy, w, h);
	    }
	}

      /*  The boundary indicating whether layer or mask is active  */
      layer_widget_boundary_redraw (layer_widget, preview_type);
      break;

    default:
      break;
    }

  return FALSE;
}

static void
layer_widget_boundary_redraw (LayerWidget *layer_widget,
			      int          preview_type)
{
  GtkWidget *widget;
  GdkGC *gc1, *gc2;
  GtkStateType state;

  if (preview_type == LAYER_PREVIEW)
    widget = layer_widget->layer_preview;
  else if (preview_type == MASK_PREVIEW)
    widget = layer_widget->mask_preview;
  else
    return;

  state = layer_widget->list_item->state;
  if (state == GTK_STATE_SELECTED)
    {
      if (layer_widget->active_preview == preview_type)
	gc1 = layer_widget->layer_preview->style->white_gc;
      else
	gc1 = layer_widget->layer_preview->style->bg_gc[GTK_STATE_SELECTED];
    }
  else
    {
      if (layer_widget->active_preview == preview_type)
	gc1 = layer_widget->layer_preview->style->black_gc;
      else
	gc1 = layer_widget->layer_preview->style->white_gc;
    }

  gc2 = gc1;
  if (preview_type == MASK_PREVIEW)
    {
      if (layersD->green_gc == NULL)
	{
	  GdkColor green;

	  green.pixel = get_color (0, 255, 0);
	  layersD->green_gc = gdk_gc_new (widget->window);
	  gdk_gc_set_foreground (layersD->green_gc, &green);
	}
      if (layersD->red_gc == NULL)
	{
	  GdkColor red;

	  red.pixel = get_color (255, 0, 0);
	  layersD->red_gc = gdk_gc_new (widget->window);
	  gdk_gc_set_foreground (layersD->red_gc, &red);
	}

      if (layer_widget->layer->show_mask)
	gc2 = layersD->green_gc;
      else if (! layer_widget->layer->apply_mask)
	gc2 = layersD->red_gc;
    }

  gdk_draw_rectangle (widget->window,
		      gc1, FALSE, 0, 0,
		      layersD->image_width + 3,
		      layersD->image_height + 3);

  gdk_draw_rectangle (widget->window,
		      gc2, FALSE, 1, 1,
		      layersD->image_width + 1,
		      layersD->image_height + 1);
}

static void
layer_widget_preview_redraw (LayerWidget *layer_widget,
			     int          preview_type)
{
  TempBuf *preview_buf;
  GdkPixmap **pixmap;
  GtkWidget *widget;
  int offx, offy;

  preview_buf = NULL;
  pixmap = NULL;
  widget = NULL;

  switch (preview_type)
    {
    case LAYER_PREVIEW:
    case FS_PREVIEW:
      widget = layer_widget->layer_preview;
      pixmap = &layer_widget->layer_pixmap;
      break;
    case MASK_PREVIEW:
      widget = layer_widget->mask_preview;
      pixmap = &layer_widget->mask_pixmap;
      break;
    }

  /*  allocate the layer widget pixmap  */
  if (! *pixmap)
    *pixmap = gdk_pixmap_new (widget->window,
			      layersD->image_width,
			      layersD->image_height,
			      -1);

  /*  If this is a floating selection preview, draw the preview  */
  if (preview_type == FS_PREVIEW)
    render_fs_preview (widget, *pixmap);
  /*  otherwise, ask the layer or mask for the preview  */
  else
    {
      /*  determine width and height  */
      layer_widget->width = (int) (layersD->ratio * GIMP_DRAWABLE(layer_widget->layer)->width);
      layer_widget->height = (int) (layersD->ratio * GIMP_DRAWABLE(layer_widget->layer)->height);
      if (layer_widget->width < 1) layer_widget->width = 1;
      if (layer_widget->height < 1) layer_widget->height = 1;
      offx = (int) (layersD->ratio * GIMP_DRAWABLE(layer_widget->layer)->offset_x);
      offy = (int) (layersD->ratio * GIMP_DRAWABLE(layer_widget->layer)->offset_y);

      switch (preview_type)
	{
	case LAYER_PREVIEW:
	  preview_buf = layer_preview (layer_widget->layer,
				       layer_widget->width,
				       layer_widget->height);

	  break;
	case MASK_PREVIEW:
	  preview_buf = layer_mask_preview (layer_widget->layer,
					    layer_widget->width,
					    layer_widget->height);
	  break;
	}

      preview_buf->x = offx;
      preview_buf->y = offy;

      render_preview (preview_buf,
		      layersD->layer_preview,
		      layersD->image_width,
		      layersD->image_height,
		      -1);

      gtk_preview_put (GTK_PREVIEW (layersD->layer_preview),
		       *pixmap, widget->style->black_gc,
		       0, 0, 0, 0, layersD->image_width, layersD->image_height);

      /*  make sure the image has been transfered completely to the pixmap before
       *  we use it again...
       */
      gdk_flush ();
    }
}

static void
layer_widget_no_preview_redraw (LayerWidget *layer_widget,
				int          preview_type)
{
  GdkPixmap *pixmap;
  GdkPixmap **pixmap_normal;
  GdkPixmap **pixmap_selected;
  GdkPixmap **pixmap_insensitive;
  GdkColor *color;
  GtkWidget *widget;
  GtkStateType state;
  gchar *bits;
  int width, height;

  pixmap_normal      = NULL;
  pixmap_selected    = NULL;
  pixmap_insensitive = NULL;
  widget             = NULL;
  bits               = NULL;
  width              = 0;
  height             = 0;

  state = layer_widget->list_item->state;

  switch (preview_type)
    {
    case LAYER_PREVIEW:
      widget = layer_widget->layer_preview;
      pixmap_normal = &layer_pixmap[NORMAL];
      pixmap_selected = &layer_pixmap[SELECTED];
      pixmap_insensitive = &layer_pixmap[INSENSITIVE];
      bits = (gchar *) layer_bits;
      width = layer_width;
      height = layer_height;
      break;
    case MASK_PREVIEW:
      widget = layer_widget->mask_preview;
      pixmap_normal = &mask_pixmap[NORMAL];
      pixmap_selected = &mask_pixmap[SELECTED];
      pixmap_insensitive = &mask_pixmap[INSENSITIVE];
      bits = (gchar *) mask_bits;
      width = mask_width;
      height = mask_height;
      break;
    }

  if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	color = &widget->style->bg[GTK_STATE_SELECTED];
      else
	color = &widget->style->white;
    }
  else
    color = &widget->style->bg[GTK_STATE_INSENSITIVE];

  gdk_window_set_background (widget->window, color);

  if (!*pixmap_normal)
    {
      *pixmap_normal =
	gdk_pixmap_create_from_data (widget->window,
				     bits, width, height, -1,
				     &widget->style->fg[GTK_STATE_SELECTED],
				     &widget->style->bg[GTK_STATE_SELECTED]);
      *pixmap_selected =
	gdk_pixmap_create_from_data (widget->window,
				     bits, width, height, -1,
				     &widget->style->fg[GTK_STATE_NORMAL],
				     &widget->style->white);
      *pixmap_insensitive =
	gdk_pixmap_create_from_data (widget->window,
				     bits, width, height, -1,
				     &widget->style->fg[GTK_STATE_INSENSITIVE],
				     &widget->style->bg[GTK_STATE_INSENSITIVE]);
    }

  if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	pixmap = *pixmap_selected;
      else
	pixmap = *pixmap_normal;
    }
  else
    pixmap = *pixmap_insensitive;

  gdk_draw_pixmap (widget->window,
		   widget->style->black_gc,
		   pixmap, 0, 0, 2, 2, width, height);
}

static void
layer_widget_eye_redraw (LayerWidget *layer_widget)
{
  GdkPixmap *pixmap;
  GdkColor *color;
  GtkStateType state;

  state = layer_widget->list_item->state;

  if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	color = &layer_widget->eye_widget->style->bg[GTK_STATE_SELECTED];
      else
	color = &layer_widget->eye_widget->style->white;
    }
  else
    color = &layer_widget->eye_widget->style->bg[GTK_STATE_INSENSITIVE];

  gdk_window_set_background (layer_widget->eye_widget->window, color);

  if (GIMP_DRAWABLE(layer_widget->layer)->visible)
    {
      if (!eye_pixmap[NORMAL])
	{
	  eye_pixmap[NORMAL] =
	    gdk_pixmap_create_from_data (layer_widget->eye_widget->window,
					 (gchar*) eye_bits, eye_width, eye_height, -1,
					 &layer_widget->eye_widget->style->fg[GTK_STATE_NORMAL],
					 &layer_widget->eye_widget->style->white);
	  eye_pixmap[SELECTED] =
	    gdk_pixmap_create_from_data (layer_widget->eye_widget->window,
					 (gchar*) eye_bits, eye_width, eye_height, -1,
					 &layer_widget->eye_widget->style->fg[GTK_STATE_SELECTED],
					 &layer_widget->eye_widget->style->bg[GTK_STATE_SELECTED]);
	  eye_pixmap[INSENSITIVE] =
	    gdk_pixmap_create_from_data (layer_widget->eye_widget->window,
					 (gchar*) eye_bits, eye_width, eye_height, -1,
					 &layer_widget->eye_widget->style->fg[GTK_STATE_INSENSITIVE],
					 &layer_widget->eye_widget->style->bg[GTK_STATE_INSENSITIVE]);
	}

      if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
	{
	  if (state == GTK_STATE_SELECTED)
	    pixmap = eye_pixmap[SELECTED];
	  else
	    pixmap = eye_pixmap[NORMAL];
	}
      else
	pixmap = eye_pixmap[INSENSITIVE];

      gdk_draw_pixmap (layer_widget->eye_widget->window,
		       layer_widget->eye_widget->style->black_gc,
		       pixmap, 0, 0, 0, 0, eye_width, eye_height);
    }
  else
    {
      gdk_window_clear (layer_widget->eye_widget->window);
    }
}

static void
layer_widget_linked_redraw (LayerWidget *layer_widget)
{
  GdkPixmap *pixmap;
  GdkColor *color;
  GtkStateType state;

  state = layer_widget->list_item->state;

  if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	color = &layer_widget->linked_widget->style->bg[GTK_STATE_SELECTED];
      else
	color = &layer_widget->linked_widget->style->white;
    }
  else
    color = &layer_widget->linked_widget->style->bg[GTK_STATE_INSENSITIVE];

  gdk_window_set_background (layer_widget->linked_widget->window, color);

  if (layer_widget->layer->linked)
    {
      if (!linked_pixmap[NORMAL])
	{
	  linked_pixmap[NORMAL] =
	    gdk_pixmap_create_from_data (layer_widget->linked_widget->window,
					 (gchar*) linked_bits, linked_width, linked_height, -1,
					 &layer_widget->linked_widget->style->fg[GTK_STATE_NORMAL],
					 &layer_widget->linked_widget->style->white);
	  linked_pixmap[SELECTED] =
	    gdk_pixmap_create_from_data (layer_widget->linked_widget->window,
					 (gchar*) linked_bits, linked_width, linked_height, -1,
					 &layer_widget->linked_widget->style->fg[GTK_STATE_SELECTED],
					 &layer_widget->linked_widget->style->bg[GTK_STATE_SELECTED]);
	  linked_pixmap[INSENSITIVE] =
	    gdk_pixmap_create_from_data (layer_widget->linked_widget->window,
					 (gchar*) linked_bits, linked_width, linked_height, -1,
					 &layer_widget->linked_widget->style->fg[GTK_STATE_INSENSITIVE],
					 &layer_widget->linked_widget->style->bg[GTK_STATE_INSENSITIVE]);
	}

      if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
	{
	  if (state == GTK_STATE_SELECTED)
	    pixmap = linked_pixmap[SELECTED];
	  else
	    pixmap = linked_pixmap[NORMAL];
	}
      else
	pixmap = linked_pixmap[INSENSITIVE];

      gdk_draw_pixmap (layer_widget->linked_widget->window,
		       layer_widget->linked_widget->style->black_gc,
		       pixmap, 0, 0, 0, 0, linked_width, linked_height);
    }
  else
    {
      gdk_window_clear (layer_widget->linked_widget->window);
    }
}

static void
layer_widget_clip_redraw (LayerWidget *layer_widget)
{
  GdkColor *color;
  GtkStateType state;

  state = layer_widget->list_item->state;
  color = &layer_widget->clip_widget->style->fg[state];

  gdk_window_set_background (layer_widget->clip_widget->window, color);
  gdk_window_clear (layer_widget->clip_widget->window);
}

static void
layer_widget_exclusive_visible (LayerWidget *layer_widget)
{
  GSList *list;
  LayerWidget *lw;
  int visible = FALSE;

  if (!layersD)
    return;

  /*  First determine if _any_ other layer widgets are set to visible  */
  list = layersD->layer_widgets;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      if (lw != layer_widget)
	visible |= GIMP_DRAWABLE(lw->layer)->visible;

      list = g_slist_next (list);
    }

  /*  Now, toggle the visibility for all layers except the specified one  */
  list = layersD->layer_widgets;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      if (lw != layer_widget)
	GIMP_DRAWABLE(lw->layer)->visible = !visible;
      else
	GIMP_DRAWABLE(lw->layer)->visible = TRUE;

      layer_widget_eye_redraw (lw);

      list = g_slist_next (list);
    }
}

static void
layer_widget_layer_flush (GtkWidget *widget,
			  gpointer   client_data)
{
  LayerWidget *layer_widget;
  Layer *layer;
  char *name;
  char *label_name;
  int update_layer_preview = FALSE;
  int update_mask_preview = FALSE;

  layer_widget = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));
  layer = layer_widget->layer;

  /*  Set sensitivity  */

  /*  to false if there is a floating selection, and this aint it  */
  if (! layer_is_floating_sel (layer_widget->layer) && layersD->floating_sel != NULL)
    {
      if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
	gtk_widget_set_sensitive (layer_widget->list_item, FALSE);
    }
  /*  to true if there is a floating selection, and this is it  */
  if (layer_is_floating_sel (layer_widget->layer) && layersD->floating_sel != NULL)
    {
      if (! GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
	gtk_widget_set_sensitive (layer_widget->list_item, TRUE);
    }
  /*  to true if there is not floating selection  */
  else if (layersD->floating_sel == NULL)
    {
      if (! GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
	gtk_widget_set_sensitive (layer_widget->list_item, TRUE);
    }

  /*  if there is an active channel, unselect layer  */
  if (layersD->active_channel != NULL)
    layers_dialog_unset_layer (layer_widget->layer);
  /*  otherwise, if this is the active layer, set  */
  else if (layersD->active_layer == layer_widget->layer)
    {
      layers_dialog_set_active_layer (layersD->active_layer);
      /*  set the data widgets to reflect this layer's values
       *  1)  The opacity slider
       *  2)  The paint mode menu
       *  3)  The preserve trans button
       */
      layersD->opacity_data->value = (gfloat) layer_widget->layer->opacity / 2.55;
      gtk_signal_emit_by_name (GTK_OBJECT (layersD->opacity_data), "value_changed");
      gtk_option_menu_set_history (GTK_OPTION_MENU (layersD->mode_option_menu),
				   paint_mode_menu_get_position (layer_widget->layer->mode));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (layersD->preserve_trans),
				    (layer_widget->layer->preserve_trans) ?
				    GTK_STATE_ACTIVE : GTK_STATE_NORMAL);
    }

  if (layer_is_floating_sel (layer_widget->layer))
    name = _("Floating Selection");
  else
    name = layer_get_name(layer_widget->layer);

  /*  we need to set the name label if necessary  */
  gtk_label_get (GTK_LABEL (layer_widget->label), &label_name);
  if (strcmp (name, label_name))
    gtk_label_set (GTK_LABEL (layer_widget->label), name);

  /*  show the layer mask preview if necessary  */
  if (layer_widget->layer->mask == NULL && layer_widget->layer_mask)
    {
      layer_widget->layer_mask = FALSE;
      layers_dialog_remove_layer_mask (layer_widget->layer);
    }
  else if (layer_widget->layer->mask != NULL && !layer_widget->layer_mask)
    {
      layer_widget->layer_mask = TRUE;
      layers_dialog_add_layer_mask (layer_widget->layer);
    }

  /*  Update the previews  */
  update_layer_preview = (! GIMP_DRAWABLE(layer)->preview_valid);

  if (layer->mask)
    {
      update_mask_preview = (! GIMP_DRAWABLE(layer->mask)->preview_valid);

      if (layer->apply_mask != layer_widget->apply_mask)
	{
	  layer_widget->apply_mask = layer->apply_mask;
	  update_mask_preview = TRUE;
	}
      if (layer->show_mask != layer_widget->show_mask)
	{
	  layer_widget->show_mask = layer->show_mask;
	  update_mask_preview = TRUE;
	}
      if (layer->edit_mask != layer_widget->edit_mask)
	{
	  layer_widget->edit_mask = layer->edit_mask;

	  if (layer->edit_mask == TRUE)
	    layer_widget->active_preview = MASK_PREVIEW;
	  else
	    layer_widget->active_preview = LAYER_PREVIEW;

	  /*  The boundary indicating whether layer or mask is active  */
	  layer_widget_boundary_redraw (layer_widget, LAYER_PREVIEW);
	  layer_widget_boundary_redraw (layer_widget, MASK_PREVIEW);
	}
    }

  if (update_layer_preview)
    gtk_widget_draw (layer_widget->layer_preview, NULL);
  if (update_mask_preview)
    gtk_widget_draw (layer_widget->mask_preview, NULL);
}


/*
 *  The new layer query dialog
 */
typedef struct _NewLayerOptions NewLayerOptions;

struct _NewLayerOptions {
  GtkWidget *query_box;
  GtkWidget *name_entry;
  GtkWidget *size_se;
  gint fill_type;
  gint xsize;
  gint ysize;

  GimpImage* gimage;
};

static gint   fill_type  = TRANSPARENT_FILL;
static gchar *layer_name = NULL;

static void
new_layer_query_ok_callback (GtkWidget *w,
			     gpointer   client_data)
{
  NewLayerOptions *options;
  Layer *layer;
  GImage *gimage;

  options = (NewLayerOptions *) client_data;

  if (layer_name)
    g_free (layer_name);
  layer_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));

  options->xsize = (gint)
    (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (options->size_se), 0) + 0.5);
  options->ysize = (gint)
    (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (options->size_se), 1) + 0.5);

  fill_type = options->fill_type;

  if ((gimage = options->gimage))
    {
      /*  Start a group undo  */
      undo_push_group_start (gimage, EDIT_PASTE_UNDO);

      layer = layer_new (gimage, options->xsize, options->ysize,
			 gimage_base_type_with_alpha (gimage),
			 layer_name, OPAQUE_OPACITY, NORMAL_MODE);
      if (layer) 
	{
	  drawable_fill (GIMP_DRAWABLE(layer), fill_type);
	  gimage_add_layer (gimage, layer, -1);
	  
	  /*  Endx the group undo  */
	  undo_push_group_end (gimage);
	  
	  gdisplays_flush ();
	} 
      else 
	{
	  g_message (_("new_layer_query_ok_callback: could not allocate new layer"));
	}
    }
  
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
new_layer_query_cancel_callback (GtkWidget *w,
				 gpointer   client_data)
{
  NewLayerOptions *options;

  options = (NewLayerOptions *) client_data;

  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static gint
new_layer_query_delete_callback (GtkWidget *w,
				 GdkEvent  *e,
				 gpointer   client_data)
{
  new_layer_query_cancel_callback (w, client_data);

  return TRUE;
}


static void
new_layer_query_fill_type_callback (GtkWidget *w,
				    gpointer   client_data)
{
  NewLayerOptions *options;

  options = (NewLayerOptions *) client_data;

  options->fill_type =
    (int) gtk_object_get_data (GTK_OBJECT (w), "layer_fill_type");
}

static void
layers_dialog_new_layer_query (GimpImage* gimage)
{
  NewLayerOptions *options;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkObject *adjustment;
  GtkWidget *spinbutton;
  GtkWidget *radio_frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GSList *group;
  int     i;

  static gchar *button_names[] =
  {
    N_("Foreground"),
    N_("Background"),
    N_("White"),
    N_("Transparent")
  };
  static gint nbutton_names = sizeof (button_names) / sizeof (button_names[0]);

  static ActionAreaItem action_items[] =
  {
    { N_("OK"), new_layer_query_ok_callback, NULL, NULL },
    { N_("Cancel"), new_layer_query_cancel_callback, NULL, NULL }
  };

  /*  the new options structure  */
  options = (NewLayerOptions *) g_malloc (sizeof (NewLayerOptions));
  options->fill_type = fill_type;
  options->gimage = gimage;

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box),
			  "new_layer_options", "Gimp");
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("New Layer Options"));
  gtk_window_position (GTK_WINDOW (options->query_box), GTK_WIN_POS_MOUSE);

  /*  handle the wm close signal  */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (new_layer_query_delete_callback),
		      options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox,
		      TRUE, TRUE, 0);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  the name label and entry  */
  label = gtk_label_new (_("Layer Name:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 1);
  gtk_widget_show (label);

  options->name_entry = gtk_entry_new ();
  gtk_widget_set_usize (options->name_entry, 75, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), options->name_entry, 1, 2, 0, 1);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry),
		      (layer_name ? layer_name : _("New Layer")));
  gtk_widget_show (options->name_entry);

  /*  the size labels  */
  label = gtk_label_new (_("Layer Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /*  the size sizeentry  */
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 2);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  
  options->size_se = gimp_size_entry_new (1, gimage->unit, "%a",
					  TRUE, TRUE, FALSE, 75,
					  GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (options->size_se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (options->size_se), spinbutton,
                             1, 2, 0, 1);
  gtk_widget_show (spinbutton);
  gtk_table_attach (GTK_TABLE (table), options->size_se, 1, 2, 1, 3,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (options->size_se);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (options->size_se), UNIT_PIXEL);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (options->size_se), 0,
                                  gimage->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (options->size_se), 1,
                                  gimage->yresolution, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (options->size_se), 0,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (options->size_se), 1,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (options->size_se), 0,
			    0, gimage->width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (options->size_se), 1,
			    0, gimage->height);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (options->size_se), 0,
			      gimage->width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (options->size_se), 1,
			      gimage->height);

  gtk_widget_show (table);

  /*  the radio frame and box  */
  radio_frame = gtk_frame_new (_("Layer Fill Type"));
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

  /*  the radio buttons  */
  group = NULL;
  for (i = 0; i < nbutton_names; i++)
    {
      radio_button =
	gtk_radio_button_new_with_label (group, gettext(button_names[i]));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_object_set_data (GTK_OBJECT (radio_button), "layer_fill_type",
			   (gpointer) i);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) new_layer_query_fill_type_callback,
			  options);

      /*  set the correct radio button  */
      if (i == options->fill_type)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_button), TRUE);

      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 2, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}


/*
 *  The edit layer attributes dialog
 */
typedef struct _EditLayerOptions EditLayerOptions;

struct _EditLayerOptions {
  GtkWidget *query_box;
  GtkWidget *name_entry;
  GimpLayer *layer;
  GImage    *gimage;
};

static void
edit_layer_query_ok_callback (GtkWidget *w,
			      gpointer   client_data)
{
  EditLayerOptions *options;
  Layer *layer;

  options = (EditLayerOptions *) client_data;

  if ((layer = options->layer))
    {
      /*  Set the new layer name  */
      if (GIMP_DRAWABLE(layer)->name)
	{
	  /*  If the layer is a floating selection, make it a layer  */
	  if (layer_is_floating_sel (layer))
	    {
	      floating_sel_to_layer (layer);
	    }
	}
      layer_set_name(layer, gtk_entry_get_text (GTK_ENTRY (options->name_entry)));
      gimage_dirty (options->gimage);
    }

  gdisplays_flush ();

  gtk_widget_destroy (options->query_box); 
  
  g_free (options);
}

static void
edit_layer_query_cancel_callback (GtkWidget *w,
				  gpointer   client_data)
{
  EditLayerOptions *options;

  options = (EditLayerOptions *) client_data;
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static gint
edit_layer_query_delete_callback (GtkWidget *w,
				  GdkEvent *e,
				  gpointer client_data)
{
  edit_layer_query_cancel_callback (w, client_data);

  return TRUE;
}

static void
layers_dialog_edit_layer_query (LayerWidget *layer_widget)
{
  EditLayerOptions *options;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;

  static ActionAreaItem action_items[] =
  {
    { N_("OK"), edit_layer_query_ok_callback, NULL, NULL },
    { N_("Cancel"), edit_layer_query_cancel_callback, NULL, NULL }
  };

  /*  the new options structure  */
  options = (EditLayerOptions *) g_malloc (sizeof (EditLayerOptions));
  options->layer = layer_widget->layer;
  options->gimage = layer_widget->gimage;

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "edit_layer_attrributes", "Gimp");
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("Edit Layer Attributes"));
  gtk_window_position (GTK_WINDOW (options->query_box), GTK_WIN_POS_MOUSE);

  /*  handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (edit_layer_query_delete_callback),
		      options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox,
		      TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Layer name: "));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->name_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), options->name_entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry),
		      ((layer_is_floating_sel (layer_widget->layer) ?
			_("Floating Selection") :
			layer_get_name(layer_widget->layer))));
  gtk_widget_show (options->name_entry);

  gtk_widget_show (hbox);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 2, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}


/*
 *  The add mask query dialog
 */

typedef struct _AddMaskOptions AddMaskOptions;

struct _AddMaskOptions {
  GtkWidget *query_box;
  Layer * layer;
  AddMaskType add_mask_type;
};

static void
add_mask_query_ok_callback (GtkWidget *w,
			    gpointer   client_data)
{
  AddMaskOptions *options;
  GImage *gimage;
  LayerMask *mask;
  Layer *layer;

  options = (AddMaskOptions *) client_data;
  if ((layer =  (options->layer)) &&
      (gimage = GIMP_DRAWABLE(layer)->gimage))
    {
      mask = layer_create_mask (layer, options->add_mask_type);
      gimage_add_layer_mask (gimage, layer, mask);
      gdisplays_flush ();
    }

  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
add_mask_query_cancel_callback (GtkWidget *w,
				gpointer   client_data)
{
  AddMaskOptions *options;

  options = (AddMaskOptions *) client_data;
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static gint
add_mask_query_delete_callback (GtkWidget *w,
				GdkEvent  *e,
				gpointer   client_data)
{
  add_mask_query_cancel_callback (w, client_data);

  return TRUE;
}

static void
fill_white_callback (GtkWidget *w,
		     gpointer   client_data)
{
  AddMaskOptions *options;

  options = (AddMaskOptions *) client_data;
  options->add_mask_type = ADD_WHITE_MASK;
}

static void
fill_black_callback (GtkWidget *w,
		     gpointer   client_data)
{
  AddMaskOptions *options;

  options = (AddMaskOptions *) client_data;
  options->add_mask_type = ADD_BLACK_MASK;
}

static void
fill_alpha_callback (GtkWidget *w,
		     gpointer   client_data)
{
  AddMaskOptions *options;

  options = (AddMaskOptions *) client_data;
  options->add_mask_type = ADD_ALPHA_MASK;
}

static void
layers_dialog_add_mask_query (Layer *layer)
{
  static ActionAreaItem action_items[2] =
  {
    { N_("OK"), add_mask_query_ok_callback, NULL, NULL },
    { N_("Cancel"), add_mask_query_cancel_callback, NULL, NULL }
  };
  AddMaskOptions *options;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *radio_frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GSList *group = NULL;
  int i;
  char *button_names[3] =
  {
    N_("White (Full Opacity)"),
    N_("Black (Full Transparency)"),
    N_("Layer's Alpha Channel")
  };
  ActionCallback button_callbacks[3] =
  {
    fill_white_callback,
    fill_black_callback,
    fill_alpha_callback
  };

  /*  the new options structure  */
  options = (AddMaskOptions *) g_malloc (sizeof (AddMaskOptions));
  options->layer = layer;
  options->add_mask_type = ADD_WHITE_MASK;

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "add_mask_options", "Gimp");
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("Add Mask Options"));
  gtk_window_position (GTK_WINDOW (options->query_box), GTK_WIN_POS_MOUSE);

  /*  handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (add_mask_query_delete_callback),
		      options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox,
		      TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  label = gtk_label_new (_("Initialize Layer Mask To:"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the radio frame and box  */
  radio_frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

  /*  the radio buttons  */
  for (i = 0; i < 3; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, gettext(button_names[i]));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) button_callbacks[i],
			  options);
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 2, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}


/*
 *  The apply layer mask dialog
 */

typedef struct _ApplyMaskOptions ApplyMaskOptions;

struct _ApplyMaskOptions {
  GtkWidget *query_box;
  Layer * layer;
};

static void
apply_mask_query_apply_callback (GtkWidget *w,
				 gpointer   client_data)
{
  ApplyMaskOptions *options;

  options = (ApplyMaskOptions *) client_data;

  gimage_remove_layer_mask (drawable_gimage (GIMP_DRAWABLE(options->layer)),
			    options->layer, APPLY);
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
apply_mask_query_discard_callback (GtkWidget *w,
				   gpointer   client_data)
{
  ApplyMaskOptions *options;

  options = (ApplyMaskOptions *) client_data;

  gimage_remove_layer_mask (drawable_gimage (GIMP_DRAWABLE(options->layer)),
			    options->layer, DISCARD);
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
apply_mask_query_cancel_callback (GtkWidget *w,
				  gpointer   client_data)
{
  ApplyMaskOptions *options;

  options = (ApplyMaskOptions *) client_data;
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static gint
apply_mask_query_delete_callback (GtkWidget *w,
				  GdkEvent  *e,
				  gpointer   client_data)
{
  apply_mask_query_cancel_callback (w, client_data);

  return TRUE;
}

static void
layers_dialog_apply_mask_query (Layer *layer)
{
  static ActionAreaItem action_items[3] =
  {
    { N_("Apply"), apply_mask_query_apply_callback, NULL, NULL },
    { N_("Discard"), apply_mask_query_discard_callback, NULL, NULL },
    { N_("Cancel"), apply_mask_query_cancel_callback, NULL, NULL }
  };
  ApplyMaskOptions *options;
  GtkWidget *vbox;
  GtkWidget *label;

  /*  the new options structure  */
  options = (ApplyMaskOptions *) g_malloc (sizeof (ApplyMaskOptions));
  options->layer = layer;

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "layer_mask_options", "Gimp");
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("Layer Mask Options"));
  gtk_window_position (GTK_WINDOW (options->query_box), GTK_WIN_POS_MOUSE);

  /*  handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (apply_mask_query_delete_callback),
		      options);


  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox,
		      TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  label = gtk_label_new (_("Apply layer mask?"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  action_items[2].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 3, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}


/*
 *  The scale layer dialog
 */

typedef struct _ScaleLayerOptions ScaleLayerOptions;

struct _ScaleLayerOptions {
  Layer  *layer;
  Resize *resize;
};


static void
scale_layer_query_ok_callback (GtkWidget *w,
			       gpointer   client_data)
{
  ScaleLayerOptions *options;
  GImage *gimage;
  Layer *layer;

  options = (ScaleLayerOptions *) client_data;

  if (options->resize->width > 0 && options->resize->height > 0 &&
      (layer =  (options->layer)))
    {
      if ((gimage = GIMP_DRAWABLE(layer)->gimage) != NULL)
	{
	  undo_push_group_start (gimage, LAYER_SCALE_UNDO);

	  if (layer_is_floating_sel (layer))
	    floating_sel_relax (layer, TRUE);

	  layer_scale (layer, options->resize->width, options->resize->height, TRUE);

	  if (layer_is_floating_sel (layer))
	    floating_sel_rigor (layer, TRUE);

	  undo_push_group_end (gimage);

	  gdisplays_flush ();
	}

      resize_widget_free (options->resize);
      g_free (options);
    }
  else
    g_message (_("Invalid width or height.  Both must be positive."));
}

static void
scale_layer_query_cancel_callback (GtkWidget *w,
				   gpointer   client_data)
{
  ScaleLayerOptions *options;
  options = (ScaleLayerOptions *) client_data;

  resize_widget_free (options->resize);
  g_free (options);
}

static gint
scale_layer_query_delete_callback (GtkWidget *w,
				   GdkEvent  *e,
				   gpointer   client_data)
{
  scale_layer_query_cancel_callback (w, client_data);

  return TRUE;
}

static void
layers_dialog_scale_layer_query (GImage *gimage,
				 Layer  *layer)
{
  ScaleLayerOptions *options;

  /*  the new options structure  */
  options = (ScaleLayerOptions *) g_malloc (sizeof (ScaleLayerOptions));
  options->layer = layer;
  options->resize = resize_widget_new (ScaleWidget,
				       ResizeLayer,
				       GTK_OBJECT (layer),
				       drawable_width (GIMP_DRAWABLE(layer)),
				       drawable_height (GIMP_DRAWABLE(layer)),
				       gimage->xresolution,
				       gimage->yresolution,
				       gimage->unit,
				       TRUE,
				       scale_layer_query_ok_callback,
				       scale_layer_query_cancel_callback,
				       scale_layer_query_delete_callback,
				       options);

  gtk_widget_show (options->resize->resize_shell);
}


/*
 *  The resize layer dialog
 */

typedef struct _ResizeLayerOptions ResizeLayerOptions;

struct _ResizeLayerOptions {
  Layer *layer;
  Resize *resize;
};

static void
resize_layer_query_ok_callback (GtkWidget *w,
				gpointer   client_data)
{
  ResizeLayerOptions *options;
  GImage *gimage;
  Layer *layer;

  options = (ResizeLayerOptions *) client_data;

  if (options->resize->width > 0 && options->resize->height > 0 &&
      (layer = (options->layer)))
    {
      if ((gimage = GIMP_DRAWABLE(layer)->gimage) != NULL)
	{
	  undo_push_group_start (gimage, LAYER_RESIZE_UNDO);

	  if (layer_is_floating_sel (layer))
	    floating_sel_relax (layer, TRUE);

	  layer_resize (layer,
			options->resize->width, options->resize->height,
			options->resize->offset_x, options->resize->offset_y);

	  if (layer_is_floating_sel (layer))
	    floating_sel_rigor (layer, TRUE);

	  undo_push_group_end (gimage);

	  gdisplays_flush ();
	}

      resize_widget_free (options->resize);
      g_free (options);
    }
  else
    g_message (_("Invalid width or height.  Both must be positive."));
}

static void
resize_layer_query_cancel_callback (GtkWidget *w,
				    gpointer   client_data)
{
  ResizeLayerOptions *options;
  options = (ResizeLayerOptions *) client_data;

  resize_widget_free (options->resize);
  g_free (options);
}

static gint
resize_layer_query_delete_callback (GtkWidget *w,
				    GdkEvent  *e,
				    gpointer   client_data)
{
  resize_layer_query_cancel_callback (w, client_data);

  return TRUE;
}

static void
layers_dialog_resize_layer_query (GImage * gimage,
				  Layer  * layer)
{
  ResizeLayerOptions *options;

  /*  the new options structure  */
  options = (ResizeLayerOptions *) g_malloc (sizeof (ResizeLayerOptions));
  options->layer = layer;
  options->resize = resize_widget_new (ResizeWidget,
				       ResizeLayer,
				       GTK_OBJECT (layer),
				       drawable_width (GIMP_DRAWABLE(layer)),
				       drawable_height (GIMP_DRAWABLE(layer)),
				       gimage->xresolution,
				       gimage->yresolution,
				       gimage->unit,
				       TRUE,
				       resize_layer_query_ok_callback,
				       resize_layer_query_cancel_callback,
				       resize_layer_query_delete_callback,
				       options);

  gtk_widget_show (options->resize->resize_shell);
}


/*
 *  The layer merge dialog
 */

typedef struct _LayerMergeOptions LayerMergeOptions;

struct _LayerMergeOptions
{
  GtkWidget *query_box;
  GimpImage *gimage;
  int        merge_visible;
  MergeType  merge_type;
};

static void
layer_merge_query_ok_callback (GtkWidget *w,
			       gpointer   client_data)
{
  LayerMergeOptions *options;
  GImage *gimage;

  options = (LayerMergeOptions *) client_data;
  if (! (gimage = options->gimage))
    return;

  if (options->merge_visible)
    gimage_merge_visible_layers (gimage, options->merge_type);

  gdisplays_flush ();
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
layer_merge_query_cancel_callback (GtkWidget *w,
				   gpointer   client_data)
{
  LayerMergeOptions *options;

  options = (LayerMergeOptions *) client_data;
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static gint
layer_merge_query_delete_callback (GtkWidget *w,
				   GdkEvent  *e,
				   gpointer   client_data)
{
  layer_merge_query_cancel_callback (w, client_data);
  
  return TRUE;
}

static void
expand_as_necessary_callback (GtkWidget *w,
			      gpointer   client_data)
{
  LayerMergeOptions *options;

  options = (LayerMergeOptions *) client_data;
  options->merge_type = EXPAND_AS_NECESSARY;
}

static void
clip_to_image_callback (GtkWidget *w,
			gpointer   client_data)
{
  LayerMergeOptions *options;

  options = (LayerMergeOptions *) client_data;
  options->merge_type = CLIP_TO_IMAGE;
}

static void
clip_to_bottom_layer_callback (GtkWidget *w,
			       gpointer   client_data)
{
  LayerMergeOptions *options;

  options = (LayerMergeOptions *) client_data;
  options->merge_type = CLIP_TO_BOTTOM_LAYER;
}

void
layers_dialog_layer_merge_query (GImage *gimage,
				 int     merge_visible)  /*  if 0, anchor active layer  */
{
  static ActionAreaItem action_items[2] =
  {
    { N_("OK"), layer_merge_query_ok_callback, NULL, NULL },
    { N_("Cancel"), layer_merge_query_cancel_callback, NULL, NULL }
  };
  LayerMergeOptions *options;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *radio_frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GSList *group = NULL;
  gint i;
  static gchar *button_names[] =
  {
    N_("Expanded as necessary"),
    N_("Clipped to image"),
    N_("Clipped to bottom layer")
  };
  static ActionCallback button_callbacks[] =
  {
    expand_as_necessary_callback,
    clip_to_image_callback,
    clip_to_bottom_layer_callback
  };

  /*  the new options structure  */
  options = (LayerMergeOptions *) g_malloc (sizeof (LayerMergeOptions));
  options->gimage = gimage;
  options->merge_visible = merge_visible;
  options->merge_type = EXPAND_AS_NECESSARY;

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "layer_merge_options", "Gimp");
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("Layer Merge Options"));
  gtk_window_position (GTK_WINDOW (options->query_box), GTK_WIN_POS_MOUSE);

  /* hadle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (layer_merge_query_delete_callback),
		      options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (options->query_box), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox,
		      TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  if (merge_visible)
    label = gtk_label_new (_("Final, merged layer should be:"));
  else
    label = gtk_label_new (_("Final, anchored layer should be:"));

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the radio frame and box  */
  radio_frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

  /*  the radio buttons  */
  for (i = 0; i < 3; i++)
    {
      radio_button =
	gtk_radio_button_new_with_label (group, gettext(button_names[i]));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) button_callbacks[i],
			  options);
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 2, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}
