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

#include "config.h"

#include <string.h> 

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimp/gimplimits.h"

#include "apptypes.h"

#include "paint-funcs/paint-funcs.h"

#include "tools/paint_options.h"

#include "gdisplay.h"
#include "gimpdnd.h"
#include "layers-commands.h"
#include "layers-dialog.h"
#include "menus.h"

#include "appenv.h"
#include "colormaps.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "gimpcontainer.h"
#include "gimplayer.h"
#include "gimplayermask.h"
#include "gimplist.h"
#include "gimprc.h"
#include "gimpui.h"
#include "image_render.h"
#include "lc_dialogP.h"
#include "ops_buttons.h"
#include "resize.h"
#include "temp_buf.h"
#include "undo.h"

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


#define LAYER_PREVIEW 0
#define MASK_PREVIEW  1
#define FS_PREVIEW    2

typedef struct _LayersDialog LayersDialog;

struct _LayersDialog
{
  GtkWidget     *vbox; 
  GtkWidget     *mode_option_menu;
  GtkWidget     *layer_list;
  GtkWidget     *scrolled_win;
  GtkWidget     *preserve_trans;
  GtkWidget     *mode_box;
  GtkWidget     *opacity_box;
  GtkAdjustment *opacity_data;
  GdkGC         *red_gc;     /*  for non-applied layer masks  */
  GdkGC         *green_gc;   /*  for visible layer masks      */
  GtkWidget     *layer_preview;

  GtkItemFactory *ifactory;

  /*  state information  */
  GimpImage     *gimage;
  gint           image_width, image_height;
  gint           gimage_width, gimage_height;
  gdouble        ratio;

  GimpLayer     *active_layer;
  GimpChannel   *active_channel;
  GimpLayer     *floating_sel;
  GSList        *layer_widgets;
};

typedef struct _LayerWidget LayerWidget;

struct _LayerWidget
{
  GtkWidget *list_item;

  GtkWidget *eye_widget;
  GtkWidget *linked_widget;
  GtkWidget *clip_widget;
  GtkWidget *layer_preview;
  GtkWidget *mask_preview;
  GtkWidget *label;

  GdkPixmap *layer_pixmap;
  GdkPixmap *mask_pixmap;

 /*  state information  */
  GimpImage *gimage;
  GimpLayer *layer;
  gint       width, height;

  gint       active_preview;
  gboolean   layer_mask;
  gboolean   apply_mask;
  gboolean   edit_mask;
  gboolean   show_mask;
  gboolean   visited;

  GimpDropType drop_type;

  gboolean   layer_pixmap_valid;
};

/*  layers dialog widget routines  */
static void layers_dialog_preview_extents       (void);
static void layers_dialog_set_menu_sensitivity  (void);
static void layers_dialog_scroll_index          (gint            index);
static void layers_dialog_set_active_layer      (GimpLayer      *layer);
static void layers_dialog_unset_layer           (GimpLayer      *layer);
static void layers_dialog_position_layer        (GimpLayer      *layer,
						 gint            new_index);
static void layers_dialog_add_layer             (GimpLayer      *layer);
static void layers_dialog_remove_layer          (GimpLayer      *layer);
static void layers_dialog_add_layer_mask        (GimpLayer      *layer);
static void layers_dialog_remove_layer_mask     (GimpLayer      *layer);

static void paint_mode_menu_callback            (GtkWidget      *widget,
						 gpointer        data);
static void opacity_scale_update                (GtkAdjustment  *widget,
						 gpointer        data);
static void preserve_trans_update               (GtkWidget      *widget,
						 gpointer        data);
static gint layer_list_events                   (GtkWidget      *widget,
						 GdkEvent       *event,
						 gpointer        data);

/*  for (un)installing the menu accelarators  */
static void layers_dialog_map_callback          (GtkWidget      *widget,
						 gpointer        data);
static void layers_dialog_unmap_callback        (GtkWidget      *widget,
						 gpointer        data);

/*  ops buttons dnd callbacks  */
static gboolean layers_dialog_drag_new_layer_callback
                                                (GtkWidget      *widget,
						 GdkDragContext *context,
						 gint            x,
						 gint            y,
						 guint           time);
static gboolean layers_dialog_drag_duplicate_layer_callback
                                                (GtkWidget      *widget,
						 GdkDragContext *context,
						 gint            x,
						 gint            y,
						 guint           time);
static gboolean layers_dialog_drag_trashcan_callback
                                                (GtkWidget      *widget,
						 GdkDragContext *context,
						 gint            x,
						 gint            y,
						 guint           time);

/*  layer widget function prototypes  */
static LayerWidget * layer_widget_get_ID        (GimpLayer      *layer);
static LayerWidget * layer_widget_create        (GimpImage      *gimage,
					         GimpLayer      *layer);

static gboolean layer_widget_drag_motion_callback
                                                (GtkWidget      *widget,
						 GdkDragContext *context,
						 gint            x,
						 gint            y,
						 guint           time);
static gboolean layer_widget_drag_drop_callback (GtkWidget      *widget,
						 GdkDragContext *context,
						 gint            x,
						 gint            y,
						 guint           time);
static void layer_widget_drag_begin_callback    (GtkWidget      *widget,
						 GdkDragContext *context);
static void layer_mask_drag_begin_callback      (GtkWidget      *widget,
						 GdkDragContext *context);
static void layer_widget_drag_leave_callback    (GtkWidget      *widget,
						 GdkDragContext *context,
						   guint           time);
static void layer_widget_drag_indicator_callback(GtkWidget      *widget,
						 gpointer        data);

static void layer_widget_draw_drop_indicator    (LayerWidget    *layer_widget,
						 GimpDropType    drop_type);
static void layer_widget_delete                 (LayerWidget    *layer_widget);
static void layer_widget_select_update          (GtkWidget      *widget,
						 gpointer        data);
static gint layer_widget_button_events          (GtkWidget      *widget,
						 GdkEvent       *event,
						 gpointer        data);
static gint layer_widget_preview_events         (GtkWidget      *widget,
						 GdkEvent       *event,
						 gpointer        data);
static void layer_widget_boundary_redraw        (LayerWidget    *layer_widget,
						 gint            preview_type);
static void layer_widget_preview_redraw         (LayerWidget    *layer_widget,
						 gint            preview_type);
static void layer_widget_no_preview_redraw      (LayerWidget    *layer_widget,
						 gint            preview_type);
static void layer_widget_eye_redraw             (LayerWidget    *layer_widget);
static void layer_widget_linked_redraw          (LayerWidget    *layer_widget);
static void layer_widget_clip_redraw            (LayerWidget    *layer_widget);
static void layer_widget_exclusive_visible      (LayerWidget    *layer_widget);
static void layer_widget_layer_flush            (GtkWidget      *widget,
						 gpointer        data);

static void layers_dialog_raise_layer_callback           (GtkWidget *widet,
							  gpointer   data);
static void layers_dialog_lower_layer_callback           (GtkWidget *widet,
							  gpointer   data);
static void layers_dialog_raise_layer_to_top_callback    (GtkWidget *widet,
							  gpointer   data);
static void layers_dialog_lower_layer_to_bottom_callback (GtkWidget *widet,
							  gpointer   data);
static void layers_dialog_new_layer_callback             (GtkWidget *widet,
							  gpointer   data);
static void layers_dialog_duplicate_layer_callback       (GtkWidget *widet,
							  gpointer   data);
static void layers_dialog_delete_layer_callback          (GtkWidget *widet,
							  gpointer   data);
static void layers_dialog_delete_layer_mask_callback     (GtkWidget *widget,
							  gpointer   data);
static void layers_dialog_anchor_layer_callback          (GtkWidget *widet,
							  gpointer   data);


/****************/
/*  Local data  */
/****************/

static LayersDialog *layersD = NULL;

static GdkPixmap *eye_pixmap[]    = { NULL, NULL, NULL };
static GdkPixmap *linked_pixmap[] = { NULL, NULL, NULL };
static GdkPixmap *layer_pixmap[]  = { NULL, NULL, NULL };
static GdkPixmap *mask_pixmap[]   = { NULL, NULL, NULL };

static gint suspend_gimage_notify = 0;

/*  the ops buttons  */
static GtkSignalFunc raise_layers_ext_callbacks[] = 
{
  layers_dialog_raise_layer_to_top_callback, NULL, NULL, NULL
};

static GtkSignalFunc lower_layers_ext_callbacks[] = 
{
  layers_dialog_lower_layer_to_bottom_callback, NULL, NULL, NULL
};

static OpsButton layers_ops_buttons[] =
{
  { new_xpm, layers_dialog_new_layer_callback, NULL,
    N_("New Layer"),
    "layers/dialogs/new_layer.html",
    NULL, 0 },
  { raise_xpm, layers_dialog_raise_layer_callback, raise_layers_ext_callbacks,
    N_("Raise Layer    \n"
       "<Shift> To Top"),
    "layers/stack/stack.html#raise_layer",
    NULL, 0 },
  { lower_xpm, layers_dialog_lower_layer_callback, lower_layers_ext_callbacks,
    N_("Lower Layer       \n"
       "<Shift> To Bottom"),
    "layers/stack/stack.html#lower_layer",
    NULL, 0 },
  { duplicate_xpm, layers_dialog_duplicate_layer_callback, NULL,
    N_("Duplicate Layer"),
    "layers/duplicate_layer.html",
    NULL, 0 },
  { anchor_xpm, layers_dialog_anchor_layer_callback, NULL,
    N_("Anchor Layer"),
    "layers/anchor_layer.html",
    NULL, 0 },
  { delete_xpm, layers_dialog_delete_layer_callback, NULL,
    N_("Delete Layer"),
    "layers/delete_layer.html",
    NULL, 0 },
  { NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

/*  dnd structures  */
static GtkTargetEntry layer_target_table[] =
{
  GIMP_TARGET_LAYER
};
static guint n_layer_targets = (sizeof (layer_target_table) /
				sizeof (layer_target_table[0]));

static GtkTargetEntry layer_mask_target_table[] =
{
  GIMP_TARGET_LAYER_MASK
};
static guint n_layer_mask_targets = (sizeof (layer_mask_target_table) /
				     sizeof (layer_mask_target_table[0]));

static GtkTargetEntry trashcan_target_table[] =
{
  GIMP_TARGET_LAYER,
  GIMP_TARGET_LAYER_MASK
};
static guint n_trashcan_targets = (sizeof (trashcan_target_table) /
				   sizeof (trashcan_target_table[0]));


/************************************/
/*  Public layers dialog functions  */
/************************************/

GtkWidget *
layers_dialog_create (void)
{
  GtkWidget      *vbox;
  GtkWidget      *util_box;
  GtkWidget      *button_box;
  GtkWidget      *label;
  GtkWidget      *slider;
 
  if (layersD)
    return layersD->vbox;

  layersD = g_new (LayersDialog, 1);
  layersD->layer_preview  = NULL;
  layersD->gimage         = NULL;
  layersD->active_layer   = NULL;
  layersD->active_channel = NULL;
  layersD->floating_sel   = NULL;
  layersD->layer_widgets  = NULL;
  layersD->green_gc       = NULL;
  layersD->red_gc         = NULL;

  if (preview_size)
    {
      layersD->layer_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_preview_size (GTK_PREVIEW (layersD->layer_preview),
			preview_size, preview_size);
    }

  /*  The main vbox  */
  layersD->vbox = gtk_event_box_new ();

  gimp_help_set_help_data (layersD->vbox, NULL, "dialogs/layers/layers.html");

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (layersD->vbox), vbox);

  /*  The layers commands pulldown menu  */
  layersD->ifactory =menus_get_layers_factory ();

  /*  The Mode option menu, and the preserve transparency  */
  layersD->mode_box = util_box = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), util_box, FALSE, FALSE, 0);

  label = gtk_label_new (_("Mode:"));
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  layersD->mode_option_menu = paint_mode_menu_new (paint_mode_menu_callback,
                                                   NULL,
                                                   FALSE, NORMAL_MODE);

  gtk_box_pack_start (GTK_BOX (util_box), layersD->mode_option_menu,
		      FALSE, FALSE, 2);
  gtk_widget_show (layersD->mode_option_menu);

  gimp_help_set_help_data (layersD->mode_option_menu, 
			   NULL, "#paint_mode_menu");

  layersD->preserve_trans =
    gtk_check_button_new_with_label (_("Keep Trans."));
  gtk_box_pack_start (GTK_BOX (util_box), layersD->preserve_trans,
		      FALSE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (layersD->preserve_trans), "toggled",
		      (GtkSignalFunc) preserve_trans_update,
		      layersD);
  gtk_widget_show (layersD->preserve_trans);

  gimp_help_set_help_data (layersD->preserve_trans,
			   _("Keep Transparency"), "#keep_trans_button");

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
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_CONTINUOUS);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_RIGHT);
  gtk_box_pack_start (GTK_BOX (util_box), slider, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (layersD->opacity_data), "value_changed",
		      (GtkSignalFunc) opacity_scale_update,
		      layersD);
  gtk_widget_show (slider);

  gimp_help_set_help_data (slider, NULL, "#opacity_scale");

  gtk_widget_show (util_box);

  /*  The layers listbox  */
  layersD->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (layersD->scrolled_win), 
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_widget_set_usize (layersD->scrolled_win, LIST_WIDTH, LIST_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), layersD->scrolled_win, TRUE, TRUE, 2);

  layersD->layer_list = gtk_list_new ();
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (layersD->scrolled_win),
					 layersD->layer_list);
  gtk_list_set_selection_mode (GTK_LIST (layersD->layer_list),
			       GTK_SELECTION_BROWSE);
  gtk_signal_connect (GTK_OBJECT (layersD->layer_list), "event",
		      (GtkSignalFunc) layer_list_events, 
		      layersD);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (layersD->layer_list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (layersD->scrolled_win)));
  GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (layersD->scrolled_win)->vscrollbar,
			  GTK_CAN_FOCUS);
      
  gtk_widget_show (layersD->layer_list);
  gtk_widget_show (layersD->scrolled_win);

  /*  The ops buttons  */
  button_box = ops_button_box_new (layers_ops_buttons, OPS_BUTTON_NORMAL);
  gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 2);
  gtk_widget_show (button_box);

  /*  Drop to new  */
  gtk_drag_dest_set (layers_ops_buttons[0].widget,
		     GTK_DEST_DEFAULT_ALL,
                     layer_target_table, n_layer_targets,
                     GDK_ACTION_COPY);
  gtk_signal_connect (GTK_OBJECT (layers_ops_buttons[0].widget), "drag_drop",
                      GTK_SIGNAL_FUNC (layers_dialog_drag_new_layer_callback),
		      NULL);

  /*  Drop to duplicate  */
  gtk_drag_dest_set (layers_ops_buttons[3].widget,
		     GTK_DEST_DEFAULT_ALL,
                     layer_target_table, n_layer_targets,
                     GDK_ACTION_COPY);
  gtk_signal_connect (GTK_OBJECT (layers_ops_buttons[3].widget), "drag_drop",
                      GTK_SIGNAL_FUNC (layers_dialog_drag_duplicate_layer_callback),
		      NULL);

  /*  Drop to trashcan  */
  gtk_drag_dest_set (layers_ops_buttons[5].widget,
		     GTK_DEST_DEFAULT_ALL,
                     trashcan_target_table, n_trashcan_targets,
                     GDK_ACTION_COPY);
  gtk_signal_connect (GTK_OBJECT (layers_ops_buttons[5].widget), "drag_drop",
                      GTK_SIGNAL_FUNC (layers_dialog_drag_trashcan_callback),
		      NULL);

  /*  Set up signals for map/unmap for the accelerators  */
  gtk_signal_connect (GTK_OBJECT (layersD->vbox), "map",
		      (GtkSignalFunc) layers_dialog_map_callback,
		      NULL);
  gtk_signal_connect (GTK_OBJECT (layersD->vbox), "unmap",
		      (GtkSignalFunc) layers_dialog_unmap_callback,
		      NULL);

  gtk_widget_show (vbox);
  gtk_widget_show (layersD->vbox);

  return layersD->vbox;
}

void
layers_dialog_free (void)
{
  LayerWidget *lw;

  if (!layersD)
    return;

  suspend_gimage_notify++;
  /*  Free all elements in the layers listbox  */
  gtk_list_clear_items (GTK_LIST (layersD->layer_list), 0, -1);
  suspend_gimage_notify--;

  while (layersD->layer_widgets)
    {
      lw = (LayerWidget *) layersD->layer_widgets->data;

      layer_widget_delete (lw);
    }

  layersD->layer_widgets  = NULL;
  layersD->active_layer   = NULL;
  layersD->active_channel = NULL;
  layersD->floating_sel   = NULL;

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
layers_dialog_invalidate_previews (GimpImage *gimage)
{
  /*  Invalidate all previews ...
   *  This is called during loading the image
   */
  gimp_container_foreach (gimage->layers, 
			  (GFunc) gimp_viewable_invalidate_preview,
			  NULL);
}

void
layers_dialog_update (GimpImage* gimage)
{
  GimpLayer   *layer;
  LayerWidget *lw;
  GList       *list;
  GList       *item_list;

  if (! layersD || layersD->gimage == gimage)
    return;

  layersD->gimage = gimage;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  Free all elements in the layers listbox  */
  gtk_list_clear_items (GTK_LIST (layersD->layer_list), 0, -1);

  while (layersD->layer_widgets)
    {
      lw = (LayerWidget *) layersD->layer_widgets->data;
      
      layer_widget_delete (lw);
    }

  if (layersD->layer_widgets)
    g_warning ("layers_dialog_update(): layersD->layer_widgets not empty!");
  layersD->layer_widgets = NULL;

  /*  Find the preview extents  */
  layers_dialog_preview_extents ();

  layersD->active_layer   = NULL;
  layersD->active_channel = NULL;
  layersD->floating_sel   = NULL;

  for (list = GIMP_LIST (gimage->layers)->list, item_list = NULL; 
       list; 
       list = g_list_next (list))
    {
      /*  create a layer list item  */
      layer = (GimpLayer *) list->data;

      lw = layer_widget_create (gimage, layer);
      layersD->layer_widgets = g_slist_append (layersD->layer_widgets, lw);
      item_list = g_list_append (item_list, lw->list_item);
    }

  /*  get the index of the active layer  */
  if (item_list)
    gtk_list_insert_items (GTK_LIST (layersD->layer_list), item_list, 0);

  suspend_gimage_notify--;
}

void
layers_dialog_flush (void)
{
  GimpImage   *gimage;
  GimpLayer   *layer;
  LayerWidget *lw;
  GList       *list;
  GSList      *slist;
  gint         pos;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  Check if the gimage extents have changed  */
  if ((gimage->width != layersD->gimage_width) ||
      (gimage->height != layersD->gimage_height))
    {
      layersD->gimage = NULL;
      layers_dialog_update (gimage);
    }
  else
    {
      /*  Set all current layer widgets to visited = FALSE  */
      for (slist = layersD->layer_widgets; 
	   slist;
	   slist = g_slist_next (slist))
        {
          lw = (LayerWidget *) slist->data;

          lw->visited = FALSE;
        }

      /*  Add any missing layers  */
      for (list = GIMP_LIST (gimage->layers)->list; 
	   list; 
	   list = g_list_next (list))
        {
          layer = (GimpLayer *) list->data;
          lw = layer_widget_get_ID (layer);

          /*  If the layer isn't in the layer widget list, add it  */
          if (lw == NULL)
	    {
	      /*  sets visited = TRUE  */
	      layers_dialog_add_layer (layer);
	    }
          else
	    lw->visited = TRUE;
        }

      /*  Remove any extraneous layers  */
      for (slist = layersD->layer_widgets; slist;)
        {
          lw = (LayerWidget *) slist->data;
	  slist = g_slist_next (slist);

          if (lw->visited == FALSE)
	    layers_dialog_remove_layer (lw->layer);
        }

      /*  Switch positions of items if necessary  */
      for (list = GIMP_LIST (gimage->layers)->list, pos = 0; 
	   list;
	   list = g_list_next (list))
        {
          layer = (GimpLayer *) list->data;

          layers_dialog_position_layer (layer, pos++);
        }

      /*  Set the active layer  */
      if (layersD->active_layer != gimp_image_get_active_layer (gimage))
        layersD->active_layer = gimp_image_get_active_layer (gimage);

      /*  Set the active channel  */
      if (layersD->active_channel != gimp_image_get_active_channel (gimage))
        layersD->active_channel = gimp_image_get_active_channel (gimage);

      /*  set the menus if floating sel status has changed  */
      if (layersD->floating_sel != gimage->floating_sel)
        layersD->floating_sel = gimage->floating_sel;

      layers_dialog_set_menu_sensitivity ();

      gtk_container_foreach (GTK_CONTAINER (layersD->layer_list),
			     layer_widget_layer_flush, NULL);
    }

  suspend_gimage_notify--;
}

void
layers_dialog_clear (void)
{
  if (!layersD)
    return;

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
		gint       width,
		gint       height,
		gint       channel)
{
  guchar   *src, *s;
  guchar   *cb;
  guchar   *buf;
  gint      a;
  gint      i, j, b;
  gint      x1, y1, x2, y2;
  gint      rowstride;
  gint      color_buf;
  gint      color;
  gint      alpha;
  gboolean  has_alpha;
  gint      image_bytes;
  gint      offset;

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
  color_buf   = (GTK_PREVIEW (preview_widget)->type == GTK_PREVIEW_COLOR);
  image_bytes = (color_buf) ? 3 : 1;
  has_alpha   = (preview_buf->bytes == 2 || preview_buf->bytes == 4);
  rowstride   = preview_buf->width * preview_buf->bytes;

  /*  Determine if the preview buf supplied is color
   *   Generally, if the bytes == {3, 4}, this is true.
   *   However, if the channel argument supplied is not -1, then
   *   the preview buf is assumed to be gray despite the number of
   *   channels it contains
   */
  color = (channel == -1) &&
          (preview_buf->bytes == 3 || preview_buf->bytes == 4);

  if (has_alpha)
    {
      buf = render_check_buf;
      alpha = ((color) ? ALPHA_PIX :
	       ((channel != -1) ? (preview_buf->bytes - 1) :
		ALPHA_G_PIX));
    }
  else
    buf = render_empty_buf;

  x1 = CLAMP (preview_buf->x, 0, width);
  y1 = CLAMP (preview_buf->y, 0, height);
  x2 = CLAMP (preview_buf->x + preview_buf->width, 0, width);
  y2 = CLAMP (preview_buf->y + preview_buf->height, 0, height);

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

      /*  The interesting stuff between leading & trailing 
	  vertical transparency  */
      if (i >= y1 && i < y2)
	{
	  /*  Handle the leading transparency  */
	  for (j = 0; j < x1; j++)
	    for (b = 0; b < image_bytes; b++)
	      render_temp_buf[j * image_bytes + b] = cb[j * 3 + b];

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
			  render_temp_buf[j * 3 + 0] = 
			    render_blend_dark_check [(a | s[RED_PIX])];
			  render_temp_buf[j * 3 + 1] = 
			    render_blend_dark_check [(a | s[GREEN_PIX])];
			  render_temp_buf[j * 3 + 2] = 
			    render_blend_dark_check [(a | s[BLUE_PIX])];
			}
		      else
			{
			  render_temp_buf[j * 3 + 0] = 
			    render_blend_light_check [(a | s[RED_PIX])];
			  render_temp_buf[j * 3 + 1] = 
			    render_blend_light_check [(a | s[GREEN_PIX])];
			  render_temp_buf[j * 3 + 2] = 
			    render_blend_light_check [(a | s[BLUE_PIX])];
			}
		    }
		  else
		    {
		      render_temp_buf[j * 3 + 0] = s[RED_PIX];
		      render_temp_buf[j * 3 + 1] = s[GREEN_PIX];
		      render_temp_buf[j * 3 + 2] = s[BLUE_PIX];
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
			      render_temp_buf[j * 3 + 0] = 
				render_blend_dark_check [(a | s[GRAY_PIX])];
			      render_temp_buf[j * 3 + 1] = 
				render_blend_dark_check [(a | s[GRAY_PIX])];
			      render_temp_buf[j * 3 + 2] = 
				render_blend_dark_check [(a | s[GRAY_PIX])];
			    }
			  else
			    render_temp_buf[j] = 
			      render_blend_dark_check [(a | s[GRAY_PIX + channel])];
			}
		      else
			{
			  if (color_buf)
			    {
			      render_temp_buf[j * 3 + 0] = 
				render_blend_light_check [(a | s[GRAY_PIX])];
			      render_temp_buf[j * 3 + 1] = 
				render_blend_light_check [(a | s[GRAY_PIX])];
			      render_temp_buf[j * 3 + 2] = 
				render_blend_light_check [(a | s[GRAY_PIX])];
			    }
			  else
			    render_temp_buf[j] = 
			      render_blend_light_check [(a | s[GRAY_PIX + channel])];
			}
		    }
		  else
		    {
		      if (color_buf)
			{
			  render_temp_buf[j * 3 + 0] = s[GRAY_PIX];
			  render_temp_buf[j * 3 + 1] = s[GRAY_PIX];
			  render_temp_buf[j * 3 + 2] = s[GRAY_PIX];
			}
		      else
			render_temp_buf[j] = s[GRAY_PIX + channel];
		    }
		}

	      s += preview_buf->bytes;
	    }

	  /*  Handle the trailing transparency  */
	  for (j = x2; j < width; j++)
	    for (b = 0; b < image_bytes; b++)
	      render_temp_buf[j * image_bytes + b] = cb[j * 3 + b];

	  src += rowstride;
	}
      else
	{
	  for (j = 0; j < width; j++)
	    for (b = 0; b < image_bytes; b++)
	      render_temp_buf[j * image_bytes + b] = cb[j * 3 + b];
	}

      gtk_preview_draw_row (GTK_PREVIEW (preview_widget), 
			    render_temp_buf, 0, i, width);
    }
}

void
render_fs_preview (GtkWidget *widget,
		   GdkPixmap *pixmap)
{
  gint      w, h;
  gint      x1, y1, x2, y2;
  GdkPoint  poly[6];
  gint      foldh, foldw;
  gint      i;

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
  poly[2].x = x1;         poly[2].y = y1 + foldh;
  poly[3].x = x1;         poly[3].y = y2;
  poly[4].x = x2;         poly[4].y = y2;
  poly[5].x = x2;         poly[5].y = y1;

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
layers_dialog_preview_extents (void)
{
  GimpImage *gimage;

  if (!layersD)
    return;

  gimage = layersD->gimage;

  layersD->gimage_width  = gimage->width;
  layersD->gimage_height = gimage->height;

  /*  Get the image width and height variables, based on the gimage  */
  if (gimage->width > gimage->height)
    layersD->ratio = 
      MIN (1.0, (gdouble) preview_size / (gdouble) gimage->width);
  else
    layersD->ratio = 
      MIN (1.0, (gdouble) preview_size / (gdouble) gimage->height);

  if (preview_size)
    {
      layersD->image_width  = RINT (layersD->ratio * (gdouble) gimage->width);
      layersD->image_height = RINT (layersD->ratio * (gdouble) gimage->height);
      
      if (layersD->image_width  < 1) layersD->image_width  = 1;
      if (layersD->image_height < 1) layersD->image_height = 1;
    }
  else
    {
      layersD->image_width  = layer_width;
      layersD->image_height = layer_height;
    }
}

static void
layers_dialog_set_menu_sensitivity (void)
{
  gboolean   fs;         /*  no floating sel        */
  gboolean   ac;         /*  no active channel      */
  gboolean   lm;         /*  layer mask             */
  gboolean   gimage;     /*  is there a gimage      */
  gboolean   lp;         /*  layers present         */
  gboolean   alpha;      /*  alpha channel present  */
  gboolean   indexed;    /*  is indexed             */
  gboolean   next_alpha;
  GList     *list; 
  GList     *next;
  GList     *prev;
  GimpLayer *layer;

  lp      = FALSE;
  indexed = FALSE;
  
  if (! layersD)
    return;

  if ((layer = (layersD->active_layer)) != NULL)
    lm = (gimp_layer_get_mask (layer)) ? TRUE : FALSE;
  else
    lm = FALSE;

  fs = (layersD->floating_sel == NULL);
  ac = (layersD->active_channel == NULL);
  gimage = (layersD->gimage != NULL);
  alpha = layer && gimp_layer_has_alpha (layer);

  if (gimage)
    {
      lp = ! gimp_image_is_empty (layersD->gimage);
      indexed = (gimp_image_base_type (layersD->gimage) == INDEXED);
    } 
     
  prev = NULL;
  next = NULL;

  for (list = GIMP_LIST (layersD->gimage->layers)->list;
       list;
       list = g_list_next (list)) 
    {
      layer = (GimpLayer *)list->data;

      if (layer == (layersD->active_layer))
	{
	  prev = g_list_previous (list);
	  next = g_list_next (list);
	  break;
	}
    }

  if (next)
    next_alpha = gimp_layer_has_alpha (GIMP_LAYER (next->data));
  else
    next_alpha = FALSE;

#define SET_SENSITIVE(menu,condition) \
        menus_set_sensitive ("<Layers>/" menu, (condition) != 0)
#define SET_OPS_SENSITIVE(button,condition) \
        gtk_widget_set_sensitive (layers_ops_buttons[(button)].widget, \
                                 (condition) != 0)

  SET_SENSITIVE ("New Layer...", gimage);
  SET_OPS_SENSITIVE (0, gimage);

  SET_SENSITIVE ("Stack/Raise Layer",
		 fs && ac && gimage && lp && alpha && prev);
  SET_OPS_SENSITIVE (1, fs && ac && gimage && lp && alpha && prev);

  SET_SENSITIVE ("Stack/Lower Layer",
		 fs && ac && gimage && lp && next && next_alpha);
  SET_OPS_SENSITIVE (2, fs && ac && gimage && lp && next && next_alpha);

  SET_SENSITIVE ("Stack/Layer to Top",
		 fs && ac && gimage && lp && alpha && prev);
  SET_SENSITIVE ("Stack/Layer to Bottom",
		 fs && ac && gimage && lp && next && next_alpha);

  SET_SENSITIVE ("Duplicate Layer", fs && ac && gimage && lp);
  SET_OPS_SENSITIVE (3, fs && ac && gimage && lp);

  SET_SENSITIVE ("Anchor Layer", !fs && ac && gimage && lp);
  SET_OPS_SENSITIVE (4, !fs && ac && gimage && lp);

  SET_SENSITIVE ("Delete Layer", ac && gimage && lp);
  SET_OPS_SENSITIVE (5, ac && gimage && lp);

  SET_SENSITIVE ("Layer Boundary Size...", ac && gimage && lp);
  SET_SENSITIVE ("Layer to Imagesize", ac && gimage && lp);
  SET_SENSITIVE ("Scale Layer...", ac && gimage && lp);

  SET_SENSITIVE ("Merge Visible Layers...", fs && ac && gimage && lp);
  SET_SENSITIVE ("Merge Down", fs && ac && gimage && lp && next);
  SET_SENSITIVE ("Flatten Image", fs && ac && gimage && lp);

  SET_SENSITIVE ("Add Layer Mask...", 
		 fs && ac && gimage && !lm && lp && alpha && !indexed);
  SET_SENSITIVE ("Apply Layer Mask", fs && ac && gimage && lm && lp);
  SET_SENSITIVE ("Delete Layer Mask", fs && ac && gimage && lm && lp);
  SET_SENSITIVE ("Mask to Selection", fs && ac && gimage && lm && lp);

  SET_SENSITIVE ("Add Alpha Channel", !alpha);
  SET_SENSITIVE ("Alpha to Selection", fs && ac && gimage && lp && alpha);

  SET_SENSITIVE ("Edit Layer Attributes...", ac && gimage && lp);

#undef SET_OPS_SENSITIVE
#undef SET_SENSITIVE

  /*  set mode, preserve transparency and opacity to insensitive
   *  if there are no layers
   */
  gtk_widget_set_sensitive (layersD->preserve_trans, lp);
  gtk_widget_set_sensitive (layersD->opacity_box, lp);
  gtk_widget_set_sensitive (layersD->mode_box, lp);
}

static void
layers_dialog_scroll_index (gint index)
{
  LayerWidget   *layer_widget;
  GtkAdjustment *adj;
  gint           item_height;

  if (!layersD->layer_widgets)
    return;

  layer_widget = (LayerWidget *) layersD->layer_widgets->data;
  item_height = layer_widget->list_item->allocation.height;

  adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (layersD->scrolled_win));
 
  if (index * item_height < adj->value)
    {
      adj->value = index * item_height;
      gtk_adjustment_value_changed (adj);
    }
  else if ((index + 1) * item_height > adj->value + adj->page_size)
    {
      adj->value = (index + 1) * item_height - adj->page_size;
      gtk_adjustment_value_changed (adj);
    }
}

/* Commented out because this piece of code produced strange segfaults

static gint
layer_dialog_idle_set_active_layer_focus (gpointer data)
{
  gtk_widget_grab_focus (GTK_WIDGET (data));

  return FALSE;
}

*/

static void
layers_dialog_set_active_layer (GimpLayer *layer)
{
  LayerWidget  *layer_widget;
  GtkStateType  state;
  gint          index;

  if (! layer)
    return;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  state = layer_widget->list_item->state;
  index = gimp_image_get_layer_index (layer_widget->gimage, layer);
  if ((index >= 0) && (state != GTK_STATE_SELECTED))
    {
      gtk_object_set_user_data (GTK_OBJECT (layer_widget->list_item), NULL);
      gtk_list_select_item (GTK_LIST (layersD->layer_list), index);
      /*  let dnd finish it's work before setting the focus  */
      /* Commented out because this piece of code produced strange segfaults
      gtk_idle_add ((GtkFunction) layer_dialog_idle_set_active_layer_focus,
		    layer_widget->list_item);
      */
      gtk_object_set_user_data (GTK_OBJECT (layer_widget->list_item),
				layer_widget);

      layers_dialog_scroll_index (index);
    }

  suspend_gimage_notify--;
}

static void
layers_dialog_unset_layer (GimpLayer *layer)
{
  LayerWidget  *layer_widget;
  GtkStateType  state;
  gint          index;

  if (! layer)
    return;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  state = layer_widget->list_item->state;
  index = gimp_image_get_layer_index (layer_widget->gimage, layer);
  if ((index >= 0) && (state == GTK_STATE_SELECTED))
    {
      gtk_object_set_user_data (GTK_OBJECT (layer_widget->list_item), NULL);
      gtk_list_unselect_item (GTK_LIST (layersD->layer_list), index);
      gtk_object_set_user_data (GTK_OBJECT (layer_widget->list_item), 
				layer_widget);
    }

  suspend_gimage_notify--;
}

static void
layers_dialog_position_layer (GimpLayer *layer,
			      gint       new_index)
{
  LayerWidget *layer_widget;
  GList       *list = NULL;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  if (new_index == g_slist_index (layersD->layer_widgets, layer_widget))
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  Remove the layer from the dialog  */
  list = g_list_append (list, layer_widget->list_item);
  gtk_list_remove_items (GTK_LIST (layersD->layer_list), list);
  layersD->layer_widgets = g_slist_remove (layersD->layer_widgets,
					   layer_widget);

  /*  Add it back at the proper index  */
  gtk_list_insert_items (GTK_LIST (layersD->layer_list), list, new_index);
  layersD->layer_widgets = g_slist_insert (layersD->layer_widgets,
					   layer_widget, new_index);

  /*  Adjust the scrollbar so the layer is visible  */
  layers_dialog_scroll_index (new_index > 0 ? new_index + 1 : 0);

  suspend_gimage_notify--;
}

static void
invalidate_preview_callback (GtkWidget   *widget,
			     LayerWidget *layer_widget)
{
  layer_widget->layer_pixmap_valid = FALSE;

  /* synthesize an expose event */
  gtk_widget_queue_draw (layer_widget->layer_preview);
}


static void
layers_dialog_add_layer (GimpLayer *layer)
{
  LayerWidget *layer_widget;
  GimpImage   *gimage;
  GList       *item_list;
  gint         position;

  if (!layersD || !layer || !(gimage = layersD->gimage))
    return;

  item_list = NULL;

  layer_widget = layer_widget_create (gimage, layer);
  item_list = g_list_append (item_list, layer_widget->list_item);

  position = gimp_image_get_layer_index (gimage, layer);
  layersD->layer_widgets =
    g_slist_insert (layersD->layer_widgets, layer_widget, position);
  gtk_list_insert_items (GTK_LIST (layersD->layer_list), item_list, position);
}

static void
layers_dialog_remove_layer (GimpLayer *layer)
{
  LayerWidget *layer_widget;
  GList       *list = NULL;

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
layers_dialog_add_layer_mask (GimpLayer *layer)
{
  LayerWidget *layer_widget;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  if (! GTK_WIDGET_VISIBLE (layer_widget->mask_preview))
    {
      gtk_object_set_data (GTK_OBJECT (layer_widget->mask_preview),
			   "gimp_layer_mask", gimp_layer_get_mask (layer));
      gtk_widget_show (layer_widget->mask_preview);
    }

  layer_widget->active_preview = MASK_PREVIEW;

  gtk_widget_queue_draw (layer_widget->layer_preview);
}

static void
layers_dialog_remove_layer_mask (GimpLayer *layer)
{
  LayerWidget *layer_widget;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  if (GTK_WIDGET_VISIBLE (layer_widget->mask_preview))
    {
      gtk_object_set_data (GTK_OBJECT (layer_widget->mask_preview),
			   "gimp_layer_mask", NULL);
      gtk_widget_hide (layer_widget->mask_preview);
    }

  layer_widget->active_preview = LAYER_PREVIEW;

  gtk_widget_queue_draw (layer_widget->layer_preview);
  gtk_widget_queue_resize (layer_widget->layer_preview->parent);
}

/*****************************************************/
/*  paint mode, opacity & preserve trans. functions  */
/*****************************************************/

static void
paint_mode_menu_callback (GtkWidget *widget,
			  gpointer   data)
{
  GimpImage        *gimage;
  GimpLayer        *layer;
  LayerModeEffects  mode;

  if (! (gimage = layersD->gimage) ||
      ! (layer  = gimp_image_get_active_layer (gimage)))
    return;

  /*  If the layer has an alpha channel, set the transparency and redraw  */
  if (gimp_layer_has_alpha (layer))
    {
      mode = (LayerModeEffects) gtk_object_get_user_data (GTK_OBJECT (widget));

      if (layer->mode != mode)
	{
	  gimp_layer_set_mode (layer, mode);

	  drawable_update (GIMP_DRAWABLE (layer), 0, 0,
			   GIMP_DRAWABLE (layer)->width,
			   GIMP_DRAWABLE (layer)->height);
	  gdisplays_flush ();
	}
    }
}

static void
opacity_scale_update (GtkAdjustment *adjustment,
		      gpointer       data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  gdouble    opacity;

  if (! (gimage = layersD->gimage) ||
      ! (layer  = gimp_image_get_active_layer (gimage)))
    return;

  opacity = adjustment->value / 100.0;

  if (gimp_layer_get_opacity (layer) != opacity)
    {
      gimp_layer_set_opacity (layer, opacity);

      drawable_update (GIMP_DRAWABLE (layer), 0, 0,
		       GIMP_DRAWABLE (layer)->width,
		       GIMP_DRAWABLE (layer)->height);
      gdisplays_flush ();
    }
}

static void
preserve_trans_update (GtkWidget *widget,
		       gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;

  if (! (gimage = layersD->gimage) ||
      ! (layer  = gimp_image_get_active_layer (gimage)))
    return;

  gimp_layer_set_preserve_trans (layer, GTK_TOGGLE_BUTTON (widget)->active);
}


/********************************/
/*  layer list events callback  */
/********************************/

static gint
layer_list_events (GtkWidget *widget,
		   GdkEvent  *event,
		   gpointer   data)
{
  LayerWidget    *layer_widget;
  GdkEventButton *bevent;
  GtkWidget      *event_widget;

  event_widget = gtk_get_event_widget (event);    

  if (GTK_IS_LIST_ITEM (event_widget))
    {
      layer_widget =
	(LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (event_widget));

      switch (event->type)
	{
	case GDK_BUTTON_PRESS:
	  bevent = (GdkEventButton *) event;
	  if (bevent->button == 3)
	    {
	      gint x, y;

	      gimp_menu_position (GTK_MENU (layersD->ifactory->widget), &x, &y);

	      gtk_item_factory_popup_with_data (layersD->ifactory,
						layersD->gimage, NULL,
						x, y,
						bevent->button, bevent->time);
	      return TRUE;
	    }
	  break;

	case GDK_2BUTTON_PRESS:
	  layers_edit_layer_query (layer_widget->layer);
	  return TRUE;

	default:
	  break;
	}
    }

  return FALSE;
}

/*****************************/
/*  layers dialog callbacks  */
/*****************************/

static gboolean
layers_dialog_button_press (GtkWidget    *widget,
			    GdkEvent     *event,
			    LayersDialog *layers_dialog)
{
  gtk_object_set_data (GTK_OBJECT (layers_dialog->ifactory),
		       "gimp-accel-context",
		       layers_dialog->gimage);

  return FALSE;
}

static void
layers_dialog_map_callback (GtkWidget *widget,
			    gpointer   data)
{
  if (! layersD)
    return;

  gtk_signal_connect (GTK_OBJECT (lc_dialog->shell), "key_press_event",
		      GTK_SIGNAL_FUNC (layers_dialog_button_press),
		      layersD);

  gtk_window_add_accel_group (GTK_WINDOW (lc_dialog->shell),
			      layersD->ifactory->accel_group);
}

static void
layers_dialog_unmap_callback (GtkWidget *widget,
			      gpointer   data)
{
  if (! layersD)
    return;

  gtk_signal_disconnect_by_func (GTK_OBJECT (lc_dialog->shell),
				 GTK_SIGNAL_FUNC (layers_dialog_button_press),
				 layersD);

  gtk_window_remove_accel_group (GTK_WINDOW (lc_dialog->shell),
				 layersD->ifactory->accel_group);
}

/***************************/
/*  ops buttons callbacks  */
/***************************/

static void
layers_dialog_raise_layer_callback (GtkWidget *widget,
				    gpointer   data)
{
  GimpImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  gimp_image_raise_layer (gimage, gimp_image_get_active_layer (gimage));
  gdisplays_flush ();
}

static void
layers_dialog_lower_layer_callback (GtkWidget *widget,
				    gpointer   data)
{
  GimpImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  gimp_image_lower_layer (gimage, gimp_image_get_active_layer (gimage));
  gdisplays_flush ();
}

static void
layers_dialog_raise_layer_to_top_callback (GtkWidget *widget,
					   gpointer   data)
{
  GimpImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  gimp_image_raise_layer_to_top (gimage, gimp_image_get_active_layer (gimage));
  gdisplays_flush ();
}

static void
layers_dialog_lower_layer_to_bottom_callback (GtkWidget *widget,
					      gpointer   data)
{
  GimpImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  gimp_image_lower_layer_to_bottom (gimage,
				    gimp_image_get_active_layer (gimage));
  gdisplays_flush ();
}

static void
layers_dialog_new_layer_callback (GtkWidget *widget,
				  gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  /*  If there is a floating selection, the new command transforms
   *  the current fs into a new layer
   */
  if ((layer = gimp_image_floating_sel (gimage)))
    {
      floating_sel_to_layer (layer);

      gdisplays_flush ();
    }
  else
    layers_new_layer_query (layersD->gimage);
}

static void
layers_dialog_duplicate_layer_callback (GtkWidget *widget,
					gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *active_layer;
  GimpLayer *new_layer;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  active_layer = gimp_image_get_active_layer (gimage);
  new_layer = gimp_layer_copy (active_layer, TRUE);
  gimp_image_add_layer (gimage, new_layer, -1);

  gdisplays_flush ();
}

static void
layers_dialog_delete_layer_callback (GtkWidget *widget,
				     gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;

  if (! layersD ||
      ! (gimage = layersD->gimage) ||
      ! (layer = gimp_image_get_active_layer (gimage)))
    return;

  /*  if the layer is a floating selection, take special care  */
  if (gimp_layer_is_floating_sel (layer))
    floating_sel_remove (layer);
  else
    gimp_image_remove_layer (gimage, gimp_image_get_active_layer (gimage));

  gdisplays_flush_now ();
}

static void
layers_dialog_delete_layer_mask_callback (GtkWidget *widget,
                                          gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  /*  Make sure there is a layer mask to apply  */
  if ((layer = gimage->active_layer) != NULL &&
      gimp_layer_get_mask (layer))
    {
      gboolean flush = layer->mask->apply_mask || layer->mask->show_mask;

      gimp_layer_apply_mask (layer, DISCARD, TRUE);

      if (flush)
        {
          gdisplays_flush ();
        }
      else
        {
          LayerWidget *layer_widget = layer_widget_get_ID (layer);

          layer_widget_layer_flush (layer_widget->list_item, NULL);
          layers_dialog_set_menu_sensitivity ();
        }
    }
}

static void
layers_dialog_anchor_layer_callback (GtkWidget *widget,
				     gpointer   data)
{
  GimpImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  floating_sel_anchor (gimp_image_get_active_layer (gimage));
  gdisplays_flush ();
}


/*******************************/
/*  ops buttons dnd callbacks  */
/*******************************/

static gboolean
layers_dialog_drag_new_layer_callback (GtkWidget      *widget,
				       GdkDragContext *context,
				       gint            x,
				       gint            y,
				       guint           time)
{
  GtkWidget *src_widget;
  gboolean   return_val = FALSE;

  if ((src_widget = gtk_drag_get_source_widget (context)))
    {
      GimpLayer *layer;

      layer = (GimpLayer *) gtk_object_get_data (GTK_OBJECT (src_widget),
						 "gimp_layer");

      if (layer &&
	  layer == layersD->active_layer)
	{
	  GimpLayer *new_layer;
	  GimpImage *gimage;
	  gint       width, height;
	  gint       off_x, off_y;

	  gimage = layersD->gimage;

	  width  = gimp_drawable_width  (GIMP_DRAWABLE (layer));
	  height = gimp_drawable_height (GIMP_DRAWABLE (layer));
	  gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

	  /*  Start a group undo  */
	  undo_push_group_start (gimage, EDIT_PASTE_UNDO);

	  new_layer = gimp_layer_new (gimage, width, height,
				      gimp_image_base_type_with_alpha (gimage),
				      _("Empty Layer Copy"),
				      layer->opacity,
				      layer->mode);
	  if (new_layer)
	    {
	      drawable_fill (GIMP_DRAWABLE (new_layer), TRANSPARENT_FILL);
	      gimp_layer_translate (new_layer, off_x, off_y);
	      gimp_image_add_layer (gimage, new_layer, -1);

	      /*  End the group undo  */
	      undo_push_group_end (gimage);
	  
	      gdisplays_flush ();
	    } 
	  else
	    {
	      g_message ("layers_dialog_drop_new_layer_callback():\n"
			   "could not allocate new layer");
	    }

	  return_val = TRUE;
	}
    }

  gtk_drag_finish (context, return_val, FALSE, time);

  return return_val;
}

static gboolean
layers_dialog_drag_duplicate_layer_callback (GtkWidget      *widget,
					     GdkDragContext *context,
					     gint            x,
					     gint            y,
					     guint           time)
{
  GtkWidget *src_widget;
  gboolean   return_val = FALSE;

  if ((src_widget = gtk_drag_get_source_widget (context)))
    {
      GimpLayer *layer;

      layer = (GimpLayer *) gtk_object_get_data (GTK_OBJECT (src_widget),
						 "gimp_layer");

      if (layer &&
	  layer == layersD->active_layer &&
	  ! gimp_layer_is_floating_sel (layer))
	{
	  layers_dialog_duplicate_layer_callback (widget, NULL);

	  return_val = TRUE;
	}
    }

  gtk_drag_finish (context, return_val, FALSE, time);

  return return_val;
}

static gboolean
layers_dialog_drag_trashcan_callback (GtkWidget      *widget,
				      GdkDragContext *context,
				      gint            x,
				      gint            y,
				      guint           time)
{
  GtkWidget *src_widget;
  gboolean   return_val = FALSE;

  if ((src_widget = gtk_drag_get_source_widget (context)))
    {
      GimpLayer     *layer;
      GimpLayerMask *layer_mask;

      layer =
	(GimpLayer *) gtk_object_get_data (GTK_OBJECT (src_widget),
					   "gimp_layer");

      layer_mask =
	(GimpLayerMask *) gtk_object_get_data (GTK_OBJECT (src_widget),
					       "gimp_layer_mask");

      if (layer &&
	  layer == layersD->active_layer)
	{
	  layers_dialog_delete_layer_callback (widget, NULL);

	  return_val = TRUE;
	}
      else if (layer_mask &&
	       gimp_layer_mask_get_layer (layer_mask) == layersD->active_layer)
	{
	  layers_dialog_delete_layer_mask_callback (widget, NULL);

	  return_val = TRUE;
	}
    }

  gtk_drag_finish (context, return_val, FALSE, time);

  return return_val;
}

/****************************/
/*  layer widget functions  */
/****************************/

static LayerWidget *
layer_widget_get_ID (GimpLayer *ID)
{
  LayerWidget *lw;
  GSList      *list;

  if (! layersD)
    return NULL;

  for (list = layersD->layer_widgets; list; list = g_slist_next (list))
    {
      lw = (LayerWidget *) list->data;

      if (lw->layer == ID)
	return lw;
    }

  return NULL;
}

static LayerWidget *
layer_widget_create (GimpImage *gimage,
		     GimpLayer *layer)
{
  LayerWidget *layer_widget;
  GtkWidget   *list_item;
  GtkWidget   *hbox;
  GtkWidget   *vbox;
  GtkWidget   *alignment;

  list_item = gtk_list_item_new ();

  /*  create the layer widget and add it to the list  */
  layer_widget = g_new (LayerWidget, 1);
  layer_widget->gimage             = gimage;
  layer_widget->layer              = layer;
  layer_widget->list_item          = list_item;
  layer_widget->layer_preview      = NULL;
  layer_widget->mask_preview       = NULL;
  layer_widget->layer_pixmap       = NULL;
  layer_widget->mask_pixmap        = NULL;
  layer_widget->width              = -1;
  layer_widget->height             = -1;
  layer_widget->layer_mask         = (gimp_layer_get_mask (layer) != NULL);

  if (layer->mask)
    {
      layer_widget->apply_mask     = layer->mask->apply_mask;
      layer_widget->edit_mask      = layer->mask->edit_mask;
      layer_widget->show_mask      = layer->mask->show_mask;
    }
  else
    {
      layer_widget->apply_mask     = FALSE;
      layer_widget->edit_mask      = FALSE;
      layer_widget->show_mask      = FALSE;
    }

  layer_widget->visited            = TRUE;
  layer_widget->drop_type          = GIMP_DROP_NONE;
  layer_widget->layer_pixmap_valid = FALSE;

  if (gimp_layer_get_mask (layer))
    layer_widget->active_preview =
      (layer->mask->edit_mask) ? MASK_PREVIEW : LAYER_PREVIEW;
  else
    layer_widget->active_preview = LAYER_PREVIEW;

  /*  Need to let the list item know about the layer_widget  */
  gtk_object_set_user_data (GTK_OBJECT (list_item), layer_widget);

  /*  set up the list item observer  */
  gtk_signal_connect (GTK_OBJECT (list_item), "select",
		      GTK_SIGNAL_FUNC (layer_widget_select_update),
		      layer_widget);
  gtk_signal_connect (GTK_OBJECT (list_item), "deselect",
		      GTK_SIGNAL_FUNC (layer_widget_select_update),
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
		      GTK_SIGNAL_FUNC (layer_widget_button_events),
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
		      GTK_SIGNAL_FUNC (layer_widget_button_events),
		      layer_widget);
  gtk_object_set_user_data (GTK_OBJECT (layer_widget->linked_widget),
			    layer_widget);
  gtk_container_add (GTK_CONTAINER (alignment), layer_widget->linked_widget);
  gtk_widget_show (layer_widget->linked_widget);
  gtk_widget_show (alignment);

  /*  The layer preview  */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 2);
  gtk_widget_show (alignment);

  layer_widget->layer_preview = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (layer_widget->layer_preview),
			 layersD->image_width + 4, layersD->image_height + 4);
  gtk_widget_set_events (layer_widget->layer_preview, PREVIEW_EVENT_MASK);
  gtk_signal_connect_while_alive (GTK_OBJECT (layer_widget->layer_preview), 
				  "event",
				  GTK_SIGNAL_FUNC (layer_widget_preview_events),
				  layer_widget,
				  GTK_OBJECT (layer));
  gtk_object_set_user_data (GTK_OBJECT (layer_widget->layer_preview),
			    layer_widget);
  gtk_container_add (GTK_CONTAINER (alignment), layer_widget->layer_preview);
  gtk_widget_show (layer_widget->layer_preview);

  /*  The layer mask preview  */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 2);
  gtk_widget_show (alignment);

  layer_widget->mask_preview = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (layer_widget->mask_preview),
			 layersD->image_width + 4, layersD->image_height + 4);
  gtk_widget_set_events (layer_widget->mask_preview, PREVIEW_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (layer_widget->mask_preview), "event",
		      GTK_SIGNAL_FUNC (layer_widget_preview_events),
		      layer_widget);
  gtk_object_set_user_data (GTK_OBJECT (layer_widget->mask_preview),
			    layer_widget);
  gtk_container_add (GTK_CONTAINER (alignment), layer_widget->mask_preview);
  if (gimp_layer_get_mask (layer) != NULL)
    {
      gtk_object_set_data (GTK_OBJECT (layer_widget->mask_preview),
			   "gimp_layer_mask", gimp_layer_get_mask (layer));
      gtk_widget_show (layer_widget->mask_preview);
    }

  /*  dnd source  */
  gtk_drag_source_set (layer_widget->mask_preview,
		       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                       layer_mask_target_table, n_layer_mask_targets, 
                       GDK_ACTION_MOVE | GDK_ACTION_COPY);

  gtk_signal_connect (GTK_OBJECT (layer_widget->mask_preview), "drag_begin",
                      GTK_SIGNAL_FUNC (layer_mask_drag_begin_callback),
		      NULL);

  /*  the layer name label */
  if (gimp_layer_is_floating_sel (layer))
    layer_widget->label = gtk_label_new (_("Floating Selection"));
  else
    layer_widget->label =
      gtk_label_new (gimp_object_get_name (GIMP_OBJECT (layer)));

  gtk_box_pack_start (GTK_BOX (hbox), layer_widget->label, FALSE, FALSE, 2);
  gtk_widget_show (layer_widget->label);

  layer_widget->clip_widget = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (layer_widget->clip_widget), 1, 2);
  gtk_widget_set_events (layer_widget->clip_widget, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (layer_widget->clip_widget), "event",
		      GTK_SIGNAL_FUNC (layer_widget_button_events),
		      layer_widget);
  gtk_object_set_user_data (GTK_OBJECT (layer_widget->clip_widget), layer_widget);
  gtk_box_pack_start (GTK_BOX (vbox), layer_widget->clip_widget,
		      FALSE, FALSE, 0);
  /* gtk_widget_show (layer_widget->clip_widget); */

  /*  dnd destination  */
  gtk_drag_dest_set (list_item,
		     GTK_DEST_DEFAULT_ALL,
                     layer_target_table, n_layer_targets,
                     GDK_ACTION_MOVE);
  gtk_signal_connect (GTK_OBJECT (list_item), "drag_leave",
                      GTK_SIGNAL_FUNC (layer_widget_drag_leave_callback),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (list_item), "drag_motion",
		      GTK_SIGNAL_FUNC (layer_widget_drag_motion_callback),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (list_item), "drag_drop",
                      GTK_SIGNAL_FUNC (layer_widget_drag_drop_callback),
		      NULL);

  /*  re-paint the drop indicator after drawing the widget  */
  gtk_signal_connect_after (GTK_OBJECT (list_item), "draw",
			    (GtkSignalFunc) layer_widget_drag_indicator_callback,
			    layer_widget);

  /*  dnd source  */
  gtk_drag_source_set (list_item,
		       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                       layer_target_table, n_layer_targets, 
                       GDK_ACTION_MOVE | GDK_ACTION_COPY);

  gtk_signal_connect (GTK_OBJECT (list_item), "drag_begin",
                      GTK_SIGNAL_FUNC (layer_widget_drag_begin_callback),
		      NULL);

  gtk_object_set_data (GTK_OBJECT (list_item), "gimp_layer", (gpointer) layer);

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (list_item);

  gtk_widget_ref (layer_widget->list_item);

  gtk_signal_connect_while_alive (GTK_OBJECT (layer), "invalidate_preview",
				  GTK_SIGNAL_FUNC (invalidate_preview_callback),
				  layer_widget,
				  GTK_OBJECT (layer_widget->list_item));

  return layer_widget;
}

static gboolean
layer_widget_drag_motion_callback (GtkWidget      *widget,
				   GdkDragContext *context,
				   gint            x,
				   gint            y,
				   guint           time)
{
  LayerWidget   *dest;
  gint           dest_index;
  GtkWidget     *src_widget;
  LayerWidget   *src;
  gint           src_index;
  gint           difference;
  GimpDropType   drop_type   = GIMP_DROP_NONE;
  GdkDragAction  drag_action = GDK_ACTION_DEFAULT;
  gboolean       return_val  = FALSE;

  dest = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  if (dest &&
      gimp_layer_has_alpha (dest->layer) &&
      (src_widget = gtk_drag_get_source_widget (context)))
    {
      src
	= (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (src_widget));

      if (src &&
	  gimp_layer_has_alpha (src->layer) &&
	  ! gimp_layer_is_floating_sel (src->layer) &&
	  src->layer == layersD->active_layer)
	{
	  src_index  = gimp_image_get_layer_index (layersD->gimage, 
						   src->layer);
	  dest_index = gimp_image_get_layer_index (layersD->gimage, 
						   dest->layer);

	  difference = dest_index - src_index;

	  drop_type = ((y < widget->allocation.height / 2) ?
		       GIMP_DROP_ABOVE : GIMP_DROP_BELOW);

	  if (difference < 0 &&
	      drop_type == GIMP_DROP_BELOW)
	    {
	      dest_index++;
	    }
	  else if (difference > 0 &&
		   drop_type == GIMP_DROP_ABOVE)
	    {
	      dest_index--;
	    }

	  if (src_index != dest_index)
	    {
	      drag_action = GDK_ACTION_MOVE;
	      return_val = TRUE;
	    }
	  else
	    {
	      drop_type = GIMP_DROP_NONE;
	    }
	}
    }

  gdk_drag_status (context, drag_action, time);

  if (dest && drop_type != dest->drop_type)
    {
      layer_widget_draw_drop_indicator (dest, dest->drop_type);
      layer_widget_draw_drop_indicator (dest, drop_type);
      dest->drop_type = drop_type;
    }

  return return_val;
}

static void
layer_widget_drag_begin_callback (GtkWidget      *widget,
				  GdkDragContext *context)
{
  LayerWidget *layer_widget;

  layer_widget = 
    (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  gimp_dnd_set_drawable_preview_icon (widget, context,
				      GIMP_DRAWABLE (layer_widget->layer));
}

static void
layer_mask_drag_begin_callback (GtkWidget      *widget,
				GdkDragContext *context)
{
  LayerWidget *layer_widget;

  layer_widget = 
    (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  gimp_dnd_set_drawable_preview_icon
    (widget, context,
     GIMP_DRAWABLE (gimp_layer_get_mask (layer_widget->layer)));
}

typedef struct
{
  GimpImage *gimage;
  GimpLayer *layer;
  gint       dest_index;
} LayerDrop;

static gint
layer_widget_idle_drop_layer (gpointer data)
{
  LayerDrop *ld;

  ld = (LayerDrop *) data;

  gimp_image_position_layer (ld->gimage, ld->layer, ld->dest_index, TRUE);
  gdisplays_flush ();

  g_free (ld);

  return FALSE;
}

static gboolean
layer_widget_drag_drop_callback (GtkWidget      *widget,
				 GdkDragContext *context,
				 gint            x,
				 gint            y,
				 guint           time)
{
  LayerWidget  *dest;
  gint          dest_index;
  GtkWidget    *src_widget;
  LayerWidget  *src;
  gint          src_index;
  gint          difference;
  GimpDropType  drop_type  = GIMP_DROP_NONE;
  gboolean      return_val = FALSE;

  dest = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  if (dest &&
      gimp_layer_has_alpha (dest->layer) &&
      (src_widget = gtk_drag_get_source_widget (context)))
    {
      src = 
	(LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (src_widget));

      if (src &&
	  gimp_layer_has_alpha (src->layer) &&
	  ! gimp_layer_is_floating_sel (src->layer) &&
	  src->layer == layersD->active_layer)
	{
	  src_index  = 
	    gimp_image_get_layer_index (layersD->gimage, src->layer);
	  dest_index = 
	    gimp_image_get_layer_index (layersD->gimage, dest->layer);

	  difference = dest_index - src_index;

	  drop_type = ((y < widget->allocation.height / 2) ?
		       GIMP_DROP_ABOVE : GIMP_DROP_BELOW);

	  if (difference < 0 &&
	      drop_type == GIMP_DROP_BELOW)
	    {
	      dest_index++;
	    }
	  else if (difference > 0 &&
		   drop_type == GIMP_DROP_ABOVE)
	    {
	      dest_index--;
	    }

	  if (src_index != dest_index)
	    {
	      LayerDrop *ld;

	      ld = g_new (LayerDrop, 1);
	      ld->gimage     = layersD->gimage;
	      ld->layer      = src->layer;
	      ld->dest_index = dest_index;

	      /*  let dnd finish it's work before changing the widget tree  */
	      gtk_idle_add ((GtkFunction) layer_widget_idle_drop_layer, ld);
	      return_val = TRUE;
	    }
	}
    }

  if (dest)
    {
      if (!return_val)
	layer_widget_draw_drop_indicator (dest, dest->drop_type);

      dest->drop_type = GIMP_DROP_NONE;
    }

  gtk_drag_finish (context, return_val, FALSE, time);

  return return_val;
}

static void
layer_widget_drag_leave_callback (GtkWidget      *widget,
				  GdkDragContext *context,
				  guint           time)
{
  LayerWidget *layer_widget;

  layer_widget = 
    (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  layer_widget->drop_type = GIMP_DROP_NONE;
}

static void
layer_widget_drag_indicator_callback (GtkWidget *widget,
				      gpointer   data)
{
  LayerWidget *layer_widget;

  layer_widget = 
    (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  layer_widget_draw_drop_indicator (layer_widget, layer_widget->drop_type);
}

static void
layer_widget_draw_drop_indicator (LayerWidget  *layer_widget,
				  GimpDropType  drop_type)
{
  static GdkGC *gc = NULL;
  gint          y  = 0;

  if (!gc)
    {
      GdkColor fg, bg;

      gc = gdk_gc_new (layer_widget->list_item->window);

      fg.pixel = 0xFFFFFFFF;
      bg.pixel = 0x00000000;

      gdk_gc_set_function (gc, GDK_INVERT);
      gdk_gc_set_foreground (gc, &fg);
      gdk_gc_set_background (gc, &bg);
      gdk_gc_set_line_attributes (gc, 5, GDK_LINE_SOLID,
				  GDK_CAP_BUTT, GDK_JOIN_MITER);
    }

  if (drop_type != GIMP_DROP_NONE)
    {
      y = ((drop_type == GIMP_DROP_ABOVE) ?
	   3 : layer_widget->list_item->allocation.height - 4);

      gdk_draw_line (layer_widget->list_item->window, gc,
		     2, y, layer_widget->list_item->allocation.width - 3, y);
    }
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
layer_widget_select_update (GtkWidget *widget,
			    gpointer   data)
{
  LayerWidget *layer_widget;

  if (! (layer_widget = (LayerWidget *) data))
    return;

  /*  Is the list item being selected?  */
  if (widget->state != GTK_STATE_SELECTED)
    return;

  /*  Only notify the gimage of an active layer change if necessary  */
  if (suspend_gimage_notify == 0)
    {
      /*  set the gimage's active layer to be this layer  */
      gimp_image_set_active_layer (layer_widget->gimage, layer_widget->layer);

      gdisplays_flush ();
    }
}

static gint
layer_widget_button_events (GtkWidget *widget,
			    GdkEvent  *event,
			    gpointer   data)
{
  LayerWidget      *layer_widget;
  GtkWidget        *event_widget;
  GdkEventButton   *bevent;
  gint              return_val;

  static gboolean   button_down  = FALSE;
  static GtkWidget *click_widget = NULL;
  static gint       old_state;
  static gint       exclusive;

  layer_widget = 
    (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

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
	  gint x, y;

	  gimp_menu_position (GTK_MENU (layersD->ifactory->widget), &x, &y);

	  gtk_item_factory_popup_with_data (layersD->ifactory,
					    layersD->gimage, NULL,
					    x, y,
					    3, bevent->time);
	  return TRUE;
	}

      button_down = TRUE;
      click_widget = widget;
      gtk_grab_add (click_widget);
      
      if (widget == layer_widget->eye_widget)
	{
	  old_state =
            gimp_drawable_get_visible (GIMP_DRAWABLE (layer_widget->layer));

	  /*  If this was a shift-click, make all/none visible  */
	  if (event->button.state & GDK_SHIFT_MASK)
	    {
	      exclusive = TRUE;
	      layer_widget_exclusive_visible (layer_widget);
	    }
	  else
	    {
	      exclusive = FALSE;
	      gimp_drawable_set_visible (GIMP_DRAWABLE (layer_widget->layer),
                                         ! old_state);
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

      button_down = FALSE;
      gtk_grab_remove (click_widget);

      if (widget == layer_widget->eye_widget)
	{
	  if (exclusive)
       	    {
	      gimp_viewable_invalidate_preview
		(GIMP_VIEWABLE (layer_widget->gimage));

	      gdisplays_update_area (layer_widget->gimage, 0, 0,
				     layer_widget->gimage->width,
				     layer_widget->gimage->height);
	      gdisplays_flush ();
	    }
	  else if (old_state != gimp_drawable_get_visible (GIMP_DRAWABLE (layer_widget->layer)))
	    {
	      /*  Invalidate the gimage preview  */
	      gimp_viewable_invalidate_preview
		(GIMP_VIEWABLE (layer_widget->gimage));

	      drawable_update (GIMP_DRAWABLE (layer_widget->layer), 0, 0,
			       GIMP_DRAWABLE (layer_widget->layer)->width,
			       GIMP_DRAWABLE (layer_widget->layer)->height);
	      gdisplays_flush (); 
	    }
	}
      else if ((widget == layer_widget->linked_widget) &&
	       (old_state != layer_widget->layer->linked))
	{
	}
      break;

    case GDK_LEAVE_NOTIFY:
      event_widget = gtk_get_event_widget (event);

     if (button_down && (event_widget == click_widget))
	{
	  /* the user moved the cursor out of the widget before
             releasing the button -> cancel the button_press */ 
	  button_down = FALSE;

	  if (widget == layer_widget->eye_widget)
	    {
	      if (exclusive)
		{
		  layer_widget_exclusive_visible (layer_widget);
		}
	      else
		{
		  gimp_drawable_set_visible (GIMP_DRAWABLE (layer_widget->layer),
                                             ! gimp_drawable_get_visible (GIMP_DRAWABLE (layer_widget->layer)));
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
			     GdkEvent  *event,
			     gpointer   data)
{
  LayerWidget     *layer_widget;
  GdkEventExpose  *eevent;
  GdkEventButton  *bevent;
  GdkPixmap      **pixmap;
  gboolean         valid;
  gint             preview_type;
  gint             sx, sy, dx, dy, w, h;

  pixmap = NULL;
  valid  = FALSE;

  layer_widget = 
    (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  g_return_val_if_fail (layer_widget != NULL, FALSE);
  g_return_val_if_fail (layer_widget->layer != NULL, FALSE);  

  if (!GIMP_IS_DRAWABLE (layer_widget->layer))
    return FALSE;

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
      valid  = GIMP_DRAWABLE (layer_widget->layer)->preview_valid;
      break;
    case MASK_PREVIEW:
      pixmap = &layer_widget->mask_pixmap;
      valid  = 
	GIMP_DRAWABLE (gimp_layer_get_mask (layer_widget->layer))->preview_valid;
      break;
    }

  if (gimp_layer_is_floating_sel (layer_widget->layer))
    preview_type = FS_PREVIEW;

  switch (event->type)
    {
     case GDK_BUTTON_PRESS:
      /*  Control-button press disables the application of the mask  */
      bevent = (GdkEventButton *) event;

      if (bevent->button == 3)
	{
	  gint x, y;

	  gimp_menu_position (GTK_MENU (layersD->ifactory->widget), &x, &y);

	  gtk_item_factory_popup_with_data (layersD->ifactory,
					    layersD->gimage, NULL,
					    x, y,
					    3, bevent->time);
	  return TRUE;
	}

      if (event->button.state & GDK_CONTROL_MASK)
	{
	  if (preview_type == MASK_PREVIEW)
	    {
	      gimp_layer_mask_set_apply (layer_widget->layer->mask,
                                         ! layer_widget->layer->mask->apply_mask);
	      gdisplays_flush ();
	    }
	}
      /*  Alt-button press makes the mask visible instead of the layer  */
      else if (event->button.state & GDK_MOD1_MASK)
	{
	  if (preview_type == MASK_PREVIEW)
	    {
	      gimp_layer_mask_set_show (layer_widget->layer->mask,
                                        ! layer_widget->layer->mask->show_mask);
	      gdisplays_flush ();
	    }
	}
      else if (layer_widget->active_preview != preview_type)
	{
	  if (preview_type == MASK_PREVIEW)
	    {
	      gimp_layer_mask_set_edit (layer_widget->layer->mask, TRUE);
	    }
          else if (layer_widget->layer->mask)
            {
	      gimp_layer_mask_set_edit (layer_widget->layer->mask, FALSE);
            }

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
		{
        /* 
	   Expose events are optimzed away by GTK+ if the widget is not
	   visible. Therefore, previews not visible in the layers_dialog
	   are not redrawn when they invalidate. Later the preview gets
	   validated by the image_preview in lc_dialog but is never
	   propagated to the layer_pixmap. We work around this by using an
	   additional flag "layer_pixmap_valid" so that the pixmap gets
	   updated once the preview scrolls into sight.
	   We should probably do the same for all drawables (masks, 
	   channels), but it is much more difficult to change one of these
	   when it's not visible.
	 */
		  if (preview_type == LAYER_PREVIEW && 
		      ! layer_widget->layer_pixmap_valid)
		    {
		      layer_widget_preview_redraw (layer_widget, preview_type);
		    }

		  gdk_draw_pixmap (widget->window,
				   widget->style->black_gc,
				   *pixmap,
				   sx, sy, dx, dy, w, h);
		}
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
			      gint         preview_type)
{
  GtkWidget    *widget;
  GdkGC        *gc1;
  GdkGC        *gc2;
  GtkStateType  state;

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

      if (layer_widget->layer->mask->show_mask)
	gc2 = layersD->green_gc;
      else if (! layer_widget->layer->mask->apply_mask)
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
			     gint         preview_type)
{
  TempBuf    *preview_buf;
  GdkPixmap **pixmap;
  GtkWidget  *widget;
  gint        offx;
  gint        offy;

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
      layer_widget->width  = 
	RINT (layersD->ratio * GIMP_DRAWABLE (layer_widget->layer)->width);
      layer_widget->height = 
	RINT (layersD->ratio * GIMP_DRAWABLE (layer_widget->layer)->height);

      if (layer_widget->width < 1)  
	layer_widget->width = 1;
      if (layer_widget->height < 1)  
	layer_widget->height = 1;

      offx = RINT (layersD->ratio *
		   GIMP_DRAWABLE (layer_widget->layer)->offset_x);
      offy = RINT (layersD->ratio *
		   GIMP_DRAWABLE (layer_widget->layer)->offset_y);

      switch (preview_type)
	{
	case LAYER_PREVIEW:
	  preview_buf =
	    gimp_viewable_get_preview (GIMP_VIEWABLE (layer_widget->layer),
				       layer_widget->width,
				       layer_widget->height);
	  
	  layer_widget->layer_pixmap_valid = TRUE;
	  break;

	case MASK_PREVIEW:
	  preview_buf =
	    gimp_viewable_get_preview (GIMP_VIEWABLE (layer_widget->layer->mask),
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
		       0, 0, 0, 0, 
		       layersD->image_width, layersD->image_height);

      /*  make sure the image has been transfered completely to the pixmap 
       *  before we use it again...
       */
      gdk_flush ();
    }

  lc_dialog_menu_preview_dirty
    (GTK_OBJECT (gimp_drawable_gimage (GIMP_DRAWABLE (layer_widget->layer))),
     NULL);
}

static void
layer_widget_no_preview_redraw (LayerWidget *layer_widget,
				gint         preview_type)
{
  GdkPixmap     *pixmap;
  GdkPixmap    **pixmap_normal;
  GdkPixmap    **pixmap_selected;
  GdkPixmap    **pixmap_insensitive;
  GdkColor      *color;
  GtkWidget     *widget;
  GtkStateType   state;
  gchar         *bits;
  gint           width, height;

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
  GdkPixmap    *pixmap;
  GdkColor     *color;
  GtkStateType  state;

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

  if (gimp_drawable_get_visible (GIMP_DRAWABLE (layer_widget->layer)))
    {
      if (!eye_pixmap[NORMAL])
	{
	  eye_pixmap[NORMAL] =
	    gdk_pixmap_create_from_data (layer_widget->eye_widget->window,
					 (gchar*) eye_bits,
					 eye_width, eye_height, -1,
					 &layer_widget->eye_widget->style->fg[GTK_STATE_NORMAL],
					 &layer_widget->eye_widget->style->white);
	  eye_pixmap[SELECTED] =
	    gdk_pixmap_create_from_data (layer_widget->eye_widget->window,
					 (gchar*) eye_bits,
					 eye_width, eye_height, -1,
					 &layer_widget->eye_widget->style->fg[GTK_STATE_SELECTED],
					 &layer_widget->eye_widget->style->bg[GTK_STATE_SELECTED]);
	  eye_pixmap[INSENSITIVE] =
	    gdk_pixmap_create_from_data (layer_widget->eye_widget->window,
					 (gchar*) eye_bits,
					 eye_width, eye_height, -1,
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
  GdkPixmap    *pixmap;
  GdkColor     *color;
  GtkStateType  state;

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
  GdkColor     *color;
  GtkStateType  state;

  state = layer_widget->list_item->state;
  color = &layer_widget->clip_widget->style->fg[state];

  gdk_window_set_background (layer_widget->clip_widget->window, color);
  gdk_window_clear (layer_widget->clip_widget->window);
}

static void
layer_widget_exclusive_visible (LayerWidget *layer_widget)
{
  GSList      *list;
  LayerWidget *lw;
  gboolean     visible = FALSE;

  if (!layersD)
    return;

  /*  First determine if _any_ other layer widgets are set to visible  */
  for (list = layersD->layer_widgets; list;  list = g_slist_next (list))
    {
      lw = (LayerWidget *) list->data;

      if (lw != layer_widget)
	visible |= gimp_drawable_get_visible (GIMP_DRAWABLE (lw->layer));
    }

  /*  Now, toggle the visibility for all layers except the specified one  */
  for (list = layersD->layer_widgets; list; list = g_slist_next (list))
    {
      lw = (LayerWidget *) list->data;

      if (lw != layer_widget)
	gimp_drawable_set_visible (GIMP_DRAWABLE (lw->layer), ! visible);
      else
	gimp_drawable_set_visible (GIMP_DRAWABLE (lw->layer), TRUE);

      layer_widget_eye_redraw (lw);
    }
}

static void
layer_widget_layer_flush (GtkWidget *widget,
			  gpointer   data)
{
  LayerWidget *layer_widget;
  GimpLayer   *layer;
  const gchar *name;
  gchar       *label_name;
  gboolean     update_layer_preview = FALSE;
  gboolean     update_mask_preview  = FALSE;

  layer_widget = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));
  layer = layer_widget->layer;

  /*  Set sensitivity  */

  /*  to false if there is a floating selection, and this aint it  */
  if (! gimp_layer_is_floating_sel (layer_widget->layer) &&
      layersD->floating_sel != NULL)
    {
      if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
	gtk_widget_set_sensitive (layer_widget->list_item, FALSE);
    }
  /*  to true if there is a floating selection, and this is it  */
  if (gimp_layer_is_floating_sel (layer_widget->layer) &&
      layersD->floating_sel != NULL)
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
      gtk_signal_handler_block_by_func (GTK_OBJECT (layersD->opacity_data),
                                        opacity_scale_update,
                                        layersD);

      gtk_adjustment_set_value (GTK_ADJUSTMENT (layersD->opacity_data),
				gimp_layer_get_opacity (layer_widget->layer) * 100.0);

      gtk_signal_handler_unblock_by_func (GTK_OBJECT (layersD->opacity_data),
                                          opacity_scale_update,
                                          layersD);

      gimp_option_menu_set_history (GTK_OPTION_MENU (layersD->mode_option_menu),
				    (gpointer) layer_widget->layer->mode);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (layersD->preserve_trans),
				    (layer_widget->layer->preserve_trans) ?
				    GTK_STATE_ACTIVE : GTK_STATE_NORMAL);
    }

  if (gimp_layer_is_floating_sel (layer_widget->layer))
    name = _("Floating Selection");
  else
    name = gimp_object_get_name (GIMP_OBJECT (layer_widget->layer));

  /*  we need to set the name label if necessary  */
  gtk_label_get (GTK_LABEL (layer_widget->label), &label_name);
  if (strcmp (name, label_name))
    gtk_label_set_text (GTK_LABEL (layer_widget->label), name);

  /*  show the layer mask preview if necessary  */
  if (gimp_layer_get_mask (layer_widget->layer) == NULL &&
      layer_widget->layer_mask)
    {
      layer_widget->layer_mask = FALSE;
      layers_dialog_remove_layer_mask (layer_widget->layer);
    }
  else if (gimp_layer_get_mask (layer_widget->layer) != NULL &&
	   !layer_widget->layer_mask)
    {
      layer_widget->layer_mask = TRUE;
      layers_dialog_add_layer_mask (layer_widget->layer);
    }

  /*  Update the previews  */
  update_layer_preview = (! GIMP_DRAWABLE (layer)->preview_valid);

  if (gimp_layer_get_mask (layer))
    {
      update_mask_preview = 
	(! GIMP_DRAWABLE (gimp_layer_get_mask (layer))->preview_valid);

      if (layer->mask->apply_mask != layer_widget->apply_mask)
	{
	  layer_widget->apply_mask = layer->mask->apply_mask;
	  update_mask_preview = TRUE;
	}
      if (layer->mask->show_mask != layer_widget->show_mask)
	{
	  layer_widget->show_mask = layer->mask->show_mask;
	  update_mask_preview = TRUE;
	}
      if (layer->mask->edit_mask != layer_widget->edit_mask)
	{
	  layer_widget->edit_mask = layer->mask->edit_mask;

	  if (layer->mask->edit_mask == TRUE)
	    layer_widget->active_preview = MASK_PREVIEW;
	  else
	    layer_widget->active_preview = LAYER_PREVIEW;

	  /*  The boundary indicating whether layer or mask is active  */
	  layer_widget_boundary_redraw (layer_widget, LAYER_PREVIEW);
	  layer_widget_boundary_redraw (layer_widget, MASK_PREVIEW);
	}
    }

  if (update_layer_preview)
    gtk_widget_queue_draw (layer_widget->layer_preview);
  if (update_mask_preview)
    gtk_widget_queue_draw (layer_widget->mask_preview);
}
