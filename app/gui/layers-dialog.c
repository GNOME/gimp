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
#include <gdk/gdkkeysyms.h>

#include "appenv.h"
#include "colormaps.h"
#include "drawable.h"
#include "errors.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "gimpdnd.h"
#include "gimprc.h"
#include "gimpui.h"
#include "image_render.h"
#include "layers_dialog.h"
#include "layers_dialogP.h"
#include "lc_dialogP.h"
#include "menus.h"
#include "ops_buttons.h"
#include "resize.h"
#include "undo.h"

#include "config.h"
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
  GtkWidget     *ops_menu;
  GtkAccelGroup *accel_group;
  GtkAdjustment *opacity_data;
  GdkGC         *red_gc;     /*  for non-applied layer masks  */
  GdkGC         *green_gc;   /*  for visible layer masks      */
  GtkWidget     *layer_preview;

  gdouble ratio;
  gint    image_width, image_height;
  gint    gimage_width, gimage_height;

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

  GImage    *gimage;
  Layer     *layer;
  GdkPixmap *layer_pixmap;
  GdkPixmap *mask_pixmap;
  gint       active_preview;
  gint       width, height;

  /*  state information  */
  gboolean  layer_mask;
  gboolean  apply_mask;
  gboolean  edit_mask;
  gboolean  show_mask;
  gboolean  visited;

  GimpDropType drop_type;
};

/*  layers dialog widget routines  */
static void layers_dialog_preview_extents       (void);
static void layers_dialog_set_menu_sensitivity  (void);
static void layers_dialog_scroll_index          (gint index);
static void layers_dialog_set_active_layer      (Layer *);
static void layers_dialog_unset_layer           (Layer *);
static void layers_dialog_position_layer        (Layer *, gint);
static void layers_dialog_add_layer             (Layer *);
static void layers_dialog_remove_layer          (Layer *);
static void layers_dialog_add_layer_mask        (Layer *);
static void layers_dialog_remove_layer_mask     (Layer *);

static void paint_mode_menu_callback            (GtkWidget *, gpointer);
static void opacity_scale_update                (GtkAdjustment *, gpointer);
static void preserve_trans_update               (GtkWidget *, gpointer);
static gint layer_list_events                   (GtkWidget *, GdkEvent *, gpointer);

/*  for (un)installing the menu accelarators  */
static void layers_dialog_map_callback          (GtkWidget *, gpointer);
static void layers_dialog_unmap_callback        (GtkWidget *, gpointer);

/*  ops buttons dnd callbacks  */
static gboolean layers_dialog_drag_new_layer_callback       (GtkWidget *,
							     GdkDragContext *,
							     gint, gint, guint);
static gboolean layers_dialog_drag_duplicate_layer_callback (GtkWidget *,
							     GdkDragContext *,
							     gint, gint, guint);
static gboolean layers_dialog_drag_trashcan_callback        (GtkWidget *,
							     GdkDragContext *,
							     gint, gint, guint);

/*  layer widget function prototypes  */
static LayerWidget *layer_widget_get_ID (Layer *);
static LayerWidget *layer_widget_create (GImage *, Layer *);

static gboolean layer_widget_drag_motion_callback (GtkWidget *,
						   GdkDragContext *,
						   gint, gint, guint);
static gboolean layer_widget_drag_drop_callback   (GtkWidget *,
						   GdkDragContext *,
						   gint, gint, guint);
static void layer_widget_drag_begin_callback      (GtkWidget *,
						   GdkDragContext *);
static void layer_mask_drag_begin_callback        (GtkWidget *,
						   GdkDragContext *);
static void layer_widget_drag_leave_callback      (GtkWidget *,
						   GdkDragContext *,
						   guint);
static void layer_widget_drag_indicator_callback  (GtkWidget *, gpointer);

static void layer_widget_draw_drop_indicator (LayerWidget *, GimpDropType);
static void layer_widget_delete              (LayerWidget *);
static void layer_widget_select_update       (GtkWidget *, gpointer);
static gint layer_widget_button_events       (GtkWidget *, GdkEvent *);
static gint layer_widget_preview_events      (GtkWidget *, GdkEvent *);
static void layer_widget_boundary_redraw     (LayerWidget *, int);
static void layer_widget_preview_redraw      (LayerWidget *, int);
static void layer_widget_no_preview_redraw   (LayerWidget *, int);
static void layer_widget_eye_redraw          (LayerWidget *);
static void layer_widget_linked_redraw       (LayerWidget *);
static void layer_widget_clip_redraw         (LayerWidget *);
static void layer_widget_exclusive_visible   (LayerWidget *);
static void layer_widget_layer_flush         (GtkWidget *, gpointer);

/*  assorted query dialogs  */
static void layers_dialog_new_layer_query    (GimpImage*);
static void layers_dialog_edit_layer_query   (LayerWidget *);
static void layers_dialog_add_mask_query     (Layer *);
static void layers_dialog_scale_layer_query  (GImage *, Layer *);
static void layers_dialog_resize_layer_query (GImage *, Layer *);
void        layers_dialog_layer_merge_query  (GImage *, gboolean);

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
  GtkWidget *vbox;
  GtkWidget *util_box;
  GtkWidget *button_box;
  GtkWidget *label;
  GtkWidget *slider;
 
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
  menus_get_layers_menu (&layersD->ops_menu, &layersD->accel_group);

  /*  The Mode option menu, and the preserve transparency  */
  layersD->mode_box = util_box = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), util_box, FALSE, FALSE, 0);

  label = gtk_label_new (_("Mode:"));
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  layersD->mode_option_menu = gimp_option_menu_new2
    (FALSE, paint_mode_menu_callback,
     NULL, (gpointer) NORMAL_MODE,

     _("Normal"),          (gpointer) NORMAL_MODE, NULL,
     _("Dissolve"),        (gpointer) DISSOLVE_MODE, NULL,
     _("Multiply (Burn)"), (gpointer) MULTIPLY_MODE, NULL,
     _("Divide (Dodge)"),  (gpointer) DIVIDE_MODE, NULL,
     _("Screen"),          (gpointer) SCREEN_MODE, NULL,
     _("Overlay"),         (gpointer) OVERLAY_MODE, NULL,
     _("Difference"),      (gpointer) DIFFERENCE_MODE, NULL,
     _("Addition"),        (gpointer) ADDITION_MODE, NULL,
     _("Subtract"),        (gpointer) SUBTRACT_MODE, NULL,
     _("Darken Only"),     (gpointer) DARKEN_ONLY_MODE, NULL,
     _("Lighten Only"),    (gpointer) LIGHTEN_ONLY_MODE, NULL,
     _("Hue"),             (gpointer) HUE_MODE, NULL,
     _("Saturation"),      (gpointer) SATURATION_MODE, NULL,
     _("Color"),           (gpointer) COLOR_MODE, NULL,
     _("Value"),           (gpointer) VALUE_MODE, NULL,

    NULL);
  gtk_box_pack_start (GTK_BOX (util_box), layersD->mode_option_menu,
		      FALSE, FALSE, 2);
  gtk_widget_show (layersD->mode_option_menu);

  gimp_help_set_help_data (layersD->mode_option_menu, NULL, "#paint_mode_menu");

  layersD->preserve_trans =
    gtk_check_button_new_with_label (_("Keep Trans."));
  gtk_box_pack_start (GTK_BOX (util_box), layersD->preserve_trans,
		      FALSE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (layersD->preserve_trans), "toggled",
		      (GtkSignalFunc) preserve_trans_update,
		      layersD);
  gtk_widget_show (layersD->preserve_trans);

  gimp_help_set_help_data (layersD->preserve_trans, NULL, "#keep_trans_button");

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

  /*  Drop to trahcan  */
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
  GSList *list;

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
layers_dialog_invalidate_previews (GimpImage *gimage)
{
  GSList *list = gimage->layers;
  Layer  *layer;

  /*  Invalidate all previews ...
   *  This is called during loading the image
   */
  while (list)
    {
      layer = (Layer *) list->data;
      GIMP_DRAWABLE (layer)->preview_valid = FALSE;
      list = g_slist_next (list);
    }
}

void
layers_dialog_update (GimpImage* gimage)
{
  Layer       *layer;
  LayerWidget *lw;
  GSList      *list;
  GList       *item_list;

  if (! layersD || layersD->gimage == gimage)
    return;

  layersD->gimage = gimage;

  /*  Make sure the gimage is not notified of this change  */
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
    g_warning ("layers_dialog_update(): layersD->layer_widgets not empty!");
  layersD->layer_widgets = NULL;

  /*  Find the preview extents  */
  layers_dialog_preview_extents ();

  layersD->active_layer = NULL;
  layersD->active_channel = NULL;
  layersD->floating_sel = NULL;

  item_list = NULL;
  for (list = gimage->layers; list; list = g_slist_next (list))
    {
      /*  create a layer list item  */
      layer = (Layer *) list->data;
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
  GImage      *gimage;
  Layer       *layer;
  LayerWidget *lw;
  GSList      *list;
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
      for (list = layersD->layer_widgets; list; list = g_slist_next (list))
        {
          lw = (LayerWidget *) list->data;
          lw->visited = FALSE;
        }

      /*  Add any missing layers  */
      for (list = gimage->layers; list; list = g_slist_next (list))
        {
          layer = (Layer *) list->data;
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
      list = layersD->layer_widgets;
      while (list)
        {
          lw = (LayerWidget *) list->data;
          list = g_slist_next (list);
          if (lw->visited == FALSE)
	    layers_dialog_remove_layer (lw->layer);
        }

      /*  Switch positions of items if necessary  */
      pos = 0;
      for (list = gimage->layers; list; list = g_slist_next (list))
        {
          layer = (Layer *) list->data;
          layers_dialog_position_layer (layer, pos++);
        }

      /*  Set the active layer  */
      if (layersD->active_layer != gimage->active_layer)
        layersD->active_layer = gimage->active_layer;

      /*  Set the active channel  */
      if (layersD->active_channel != gimage->active_channel)
        layersD->active_channel = gimage->active_channel;

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
  if (! layersD)
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
layers_dialog_preview_extents (void)
{
  GImage *gimage;

  if (!layersD)
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
layers_dialog_set_menu_sensitivity (void)
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

  if ((layer = (layersD->active_layer)) != NULL)
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
  SET_SENSITIVE ("Merge Down", fs && ac && gimage && lp);
  SET_SENSITIVE ("Flatten Image", fs && ac && gimage && lp);

  SET_SENSITIVE ("Add Layer Mask...", fs && ac && gimage && !lm && lp && alpha);
  SET_SENSITIVE ("Apply Layer Mask...", fs && ac && gimage && lm && lp);
  SET_SENSITIVE ("Delete Layer Mask...", fs && ac && gimage && lm && lp);
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
  GtkAdjustment *adj;
  gint item_height;

  item_height = 6 + (preview_size ? preview_size : layer_height);
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

static void
layers_dialog_set_active_layer (Layer *layer)
{
  LayerWidget  *layer_widget;
  GtkStateType  state;
  gint index;

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
 
      layers_dialog_scroll_index (index);
    }

  suspend_gimage_notify--;
}

static void
layers_dialog_unset_layer (Layer *layer)
{
  LayerWidget  *layer_widget;
  GtkStateType  state;
  gint index;

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
layers_dialog_position_layer (Layer *layer,
			      gint   new_index)
{
  LayerWidget  *layer_widget;
  GList *list = NULL;

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
  layers_dialog_scroll_index (new_index > 0 ? new_index + 1: 0);

  suspend_gimage_notify--;
}

static void
layers_dialog_add_layer (Layer *layer)
{
  LayerWidget *layer_widget;
  GImage *gimage;
  GList  *item_list;
  gint    position;

  if (!layersD || !layer || !(gimage = layersD->gimage))
    return;

  item_list = NULL;

  layer_widget = layer_widget_create (gimage, layer);
  item_list = g_list_append (item_list, layer_widget->list_item);

  position = gimage_get_layer_index (gimage, layer);
  layersD->layer_widgets = g_slist_insert (layersD->layer_widgets, layer_widget, position);
  gtk_list_insert_items (GTK_LIST (layersD->layer_list), item_list, position);
}

static void
layers_dialog_remove_layer (Layer *layer)
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
layers_dialog_add_layer_mask (Layer *layer)
{
  LayerWidget *layer_widget;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  if (! GTK_WIDGET_VISIBLE (layer_widget->mask_preview))
    {
      gtk_object_set_data (GTK_OBJECT (layer_widget->mask_preview),
			   "gimp_layer_mask", layer_get_mask (layer));
      gtk_widget_show (layer_widget->mask_preview);
    }

  layer_widget->active_preview = MASK_PREVIEW;

  gtk_widget_queue_draw (layer_widget->layer_preview);
}

static void
layers_dialog_remove_layer_mask (Layer *layer)
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
}

/*****************************************************/
/*  paint mode, opacity & preserve trans. functions  */
/*****************************************************/

static void
paint_mode_menu_callback (GtkWidget *widget,
			  gpointer   data)
{
  GImage           *gimage;
  Layer            *layer;
  LayerModeEffects  mode;

  if (!(gimage = layersD->gimage) || !(layer = gimage->active_layer))
    return;

  /*  If the layer has an alpha channel, set the transparency and redraw  */
  if (layer_has_alpha (layer))
    {
      mode = (LayerModeEffects) gtk_object_get_user_data (GTK_OBJECT (widget));

      if (layer->mode != mode)
	{
	  layer->mode = mode;

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
  GImage *gimage;
  Layer  *layer;
  gint    opacity;

  if (!(gimage = layersD->gimage) || !(layer = gimage->active_layer))
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
preserve_trans_update (GtkWidget *widget,
		       gpointer   data)
{
  GImage *gimage;
  Layer  *layer;

  if (!(gimage = layersD->gimage) || !(layer = gimage->active_layer))
    return;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    layer->preserve_trans = TRUE;
  else
    layer->preserve_trans = FALSE;
}


/********************************/
/*  layer list events callback  */
/********************************/

static gint
layer_list_events (GtkWidget *widget,
		   GdkEvent  *event,
		   gpointer   data)
{
  GdkEventButton *bevent;
  GtkWidget      *event_widget;
  LayerWidget    *layer_widget;

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
layers_dialog_map_callback (GtkWidget *widget,
			    gpointer   data)
{
  if (! layersD)
    return;

  gtk_window_add_accel_group (GTK_WINDOW (lc_dialog->shell),
			      layersD->accel_group);
}

static void
layers_dialog_unmap_callback (GtkWidget *widget,
			      gpointer   data)
{
  if (! layersD)
    return;

  gtk_window_remove_accel_group (GTK_WINDOW (lc_dialog->shell),
				 layersD->accel_group);
}

/***********************************/
/*  callbacks exported to menus.c  */
/***********************************/

void
layers_dialog_previous_layer_callback (GtkWidget *widget,
				       gpointer   data)
{
  GImage *gimage;
  Layer  *new_layer;
  gint    current_layer;

  if (!layersD || !(gimage = layersD->gimage))
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
layers_dialog_next_layer_callback (GtkWidget *widget,
				   gpointer   data)
{
  GImage *gimage;
  Layer  *new_layer;
  gint    current_layer;

  if (!layersD || !(gimage = layersD->gimage))
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
layers_dialog_raise_layer_callback (GtkWidget *widget,
				    gpointer   data)
{
  GImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  gimage_raise_layer (gimage, gimage->active_layer);
  gdisplays_flush ();
}

void
layers_dialog_lower_layer_callback (GtkWidget *widget,
				    gpointer   data)
{
  GImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  gimage_lower_layer (gimage, gimage->active_layer);
  gdisplays_flush ();
}

void
layers_dialog_raise_layer_to_top_callback (GtkWidget *widget,
					   gpointer   data)
{
  GImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  gimage_raise_layer_to_top (gimage, gimage->active_layer);
  gdisplays_flush ();
}

void
layers_dialog_lower_layer_to_bottom_callback (GtkWidget *widget,
					      gpointer   data)
{
  GImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  gimage_lower_layer_to_bottom (gimage, gimage->active_layer);
  gdisplays_flush ();
}

void
layers_dialog_new_layer_callback (GtkWidget *widget,
				  gpointer   data)
{
  GImage *gimage;
  Layer  *layer;

  if (!layersD || !(gimage = layersD->gimage))
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
layers_dialog_duplicate_layer_callback (GtkWidget *widget,
					gpointer   data)
{
  GImage *gimage;
  Layer  *active_layer;
  Layer  *new_layer;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  /*  Start a group undo  */
  undo_push_group_start (gimage, LAYER_ADD_UNDO);

  active_layer = gimage_get_active_layer (gimage);
  new_layer = layer_copy (active_layer, TRUE);
  gimage_add_layer (gimage, new_layer, -1);

  /*  End the group undo  */
  undo_push_group_end (gimage);

  gdisplays_flush ();
}

void
layers_dialog_delete_layer_callback (GtkWidget *widget,
				     gpointer   data)
{
  GImage *gimage;
  Layer  *layer;

  if (!layersD ||
      !(gimage = layersD->gimage) ||
      !(layer = gimage_get_active_layer (gimage)))
    return;

  /*  if the layer is a floating selection, take special care  */
  if (layer_is_floating_sel (layer))
    floating_sel_remove (layer);
  else
    gimage_remove_layer (gimage, gimage->active_layer);

  gdisplays_flush ();
}

void
layers_dialog_scale_layer_callback (GtkWidget *widget,
				    gpointer   data)
{
  GImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  layers_dialog_scale_layer_query (gimage, gimage->active_layer);
}

void
layers_dialog_resize_layer_callback (GtkWidget *widget,
				     gpointer   data)
{
  GImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  layers_dialog_resize_layer_query (gimage, gimage->active_layer);
}

void
layers_dialog_resize_to_image_callback (GtkWidget *widget,
					gpointer   data)
{
  GImage *gimage;
  
  if (!layersD || !(gimage = layersD->gimage))
    return;
  
  layer_resize_to_image (gimage->active_layer);

  gdisplays_flush ();
}

void
layers_dialog_add_layer_mask_callback (GtkWidget *widget,
				       gpointer   data)
{
  GImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  layers_dialog_add_mask_query (gimage->active_layer);
}

void
layers_dialog_apply_layer_mask_callback (GtkWidget *widget,
					 gpointer   data)
{
  GImage *gimage;
  Layer  *layer;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  /*  Make sure there is a layer mask to apply  */
  if ((layer =  gimage->active_layer) != NULL)
    {
      if (layer->mask)
	gimage_remove_layer_mask (drawable_gimage (GIMP_DRAWABLE (layer)), layer, APPLY);
    }
}

void
layers_dialog_delete_layer_mask_callback (GtkWidget *widget,
					 gpointer   data)
{
  GImage *gimage;
  Layer  *layer;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  /*  Make sure there is a layer mask to apply  */
  if ((layer =  gimage->active_layer) != NULL)
    {
      if (layer->mask)
	gimage_remove_layer_mask (drawable_gimage (GIMP_DRAWABLE (layer)), layer, DISCARD);
    }
}

void
layers_dialog_anchor_layer_callback (GtkWidget *widget,
				     gpointer   data)
{
  GImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  floating_sel_anchor (gimage_get_active_layer (gimage));
  gdisplays_flush ();
}

void
layers_dialog_merge_layers_callback (GtkWidget *widget,
				     gpointer   data)
{
  GImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  layers_dialog_layer_merge_query (gimage, TRUE);
}

void
layers_dialog_merge_down_callback (GtkWidget *widget,
				   gpointer   data)
{
  GImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;
  
  gimp_image_merge_down (gimage, gimage->active_layer, EXPAND_AS_NECESSARY);
  gdisplays_flush ();
}

void
layers_dialog_flatten_image_callback (GtkWidget *widget,
				      gpointer   data)
{
  GImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  gimage_flatten (gimage);
  gdisplays_flush ();
}

void
layers_dialog_alpha_select_callback (GtkWidget *widget,
				     gpointer   data)
{
  GImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  gimage_mask_layer_alpha (gimage, gimage->active_layer);
  gdisplays_flush ();
}

void
layers_dialog_mask_select_callback (GtkWidget *widget,
				    gpointer   data)
{
  GImage *gimage;

  if (!layersD || !(gimage = layersD->gimage))
    return;

  gimage_mask_layer_mask (gimage, gimage->active_layer);
  gdisplays_flush ();
}

void
layers_dialog_add_alpha_channel_callback (GtkWidget *widget,
					  gpointer   data)
{
  GImage *gimage;
  Layer  *layer;

  if (!layersD ||
      !(gimage = layersD->gimage) ||
      !(layer = gimage_get_active_layer (gimage)))
    return;

  layer_add_alpha (layer);
  gdisplays_flush ();
}

void
layers_dialog_edit_layer_attributes_callback (GtkWidget *widget,
					      gpointer   data)
{
  if (layersD && layersD->active_layer)
    {
      LayerWidget *layer_widget;

      layer_widget = layer_widget_get_ID (layersD->active_layer);

      layers_dialog_edit_layer_query (layer_widget);
    }
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
  GtkWidget   *src_widget;
  gboolean return_val = FALSE;

  if ((src_widget = gtk_drag_get_source_widget (context)))
    {
      Layer *layer = NULL;

      layer = (Layer *) gtk_object_get_data (GTK_OBJECT (src_widget),
					     "gimp_layer");

      if (layer &&
	  layer == layersD->active_layer)
	{
	  Layer  *new_layer;
	  GImage *gimage;
	  gint width, height;
	  gint off_x, off_y;

	  gimage = layersD->gimage;

	  width  = gimp_drawable_width  (GIMP_DRAWABLE (layer));
	  height = gimp_drawable_height (GIMP_DRAWABLE (layer));
	  gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

	  /*  Start a group undo  */
	  undo_push_group_start (gimage, EDIT_PASTE_UNDO);

	  new_layer = layer_new (gimage, width, height,
				 gimage_base_type_with_alpha (gimage),
				 _("Empty Layer Copy"),
				 layer->opacity,
				 layer->mode);
	  if (new_layer)
	    {
	      drawable_fill (GIMP_DRAWABLE (new_layer), TRANSPARENT_FILL);
	      layer_translate (new_layer, off_x, off_y);
	      gimage_add_layer (gimage, new_layer, -1);

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
  gboolean return_val = FALSE;

  if ((src_widget = gtk_drag_get_source_widget (context)))
    {
      Layer *layer;

      layer = (Layer *) gtk_object_get_data (GTK_OBJECT (src_widget),
					     "gimp_layer");

      if (layer &&
	  layer == layersD->active_layer &&
	  ! layer_is_floating_sel (layer))
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
  gboolean return_val = FALSE;

  if ((src_widget = gtk_drag_get_source_widget (context)))
    {
      Layer *layer;
      LayerMask *layer_mask;

      layer = (Layer *) gtk_object_get_data (GTK_OBJECT (src_widget),
					     "gimp_layer");
      layer_mask = (LayerMask *) gtk_object_get_data (GTK_OBJECT (src_widget),
						      "gimp_layer_mask");

      if (layer &&
	  layer == layersD->active_layer)
	{
	  layers_dialog_delete_layer_callback (widget, NULL);

	  return_val = TRUE;
	}
      else if (layer_mask &&
	       layer_mask_get_layer (layer_mask) == layersD->active_layer)
	{
	  layers_dialog_apply_layer_mask_callback (widget, NULL);

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
layer_widget_get_ID (Layer *ID)
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

      list = g_slist_next (list);
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
  layer_widget = g_new (LayerWidget, 1);
  layer_widget->gimage        = gimage;
  layer_widget->layer         = layer;
  layer_widget->layer_preview = NULL;
  layer_widget->mask_preview  = NULL;
  layer_widget->layer_pixmap  = NULL;
  layer_widget->mask_pixmap   = NULL;
  layer_widget->list_item     = list_item;
  layer_widget->width         = -1;
  layer_widget->height        = -1;
  layer_widget->layer_mask    = (layer->mask != NULL);
  layer_widget->apply_mask    = layer->apply_mask;
  layer_widget->edit_mask     = layer->edit_mask;
  layer_widget->show_mask     = layer->show_mask;
  layer_widget->visited       = TRUE;
  layer_widget->drop_type     = GIMP_DROP_NONE;

  if (layer->mask)
    layer_widget->active_preview =
      (layer->edit_mask) ? MASK_PREVIEW : LAYER_PREVIEW;
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

  /*  The layer preview  */
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

  /*  The layer mask preview  */
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
  if (layer_get_mask (layer) != NULL)
    {
      gtk_object_set_data (GTK_OBJECT (layer_widget->mask_preview),
			   "gimp_layer_mask", layer_get_mask (layer));
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

  return layer_widget;
}

static gboolean
layer_widget_drag_motion_callback (GtkWidget      *widget,
				   GdkDragContext *context,
				   gint            x,
				   gint            y,
				   guint           time)
{
  LayerWidget *dest;
  gint         dest_index;
  GtkWidget   *src_widget;
  LayerWidget *src;
  gint         src_index;
  gint         difference;

  GimpDropType  drop_type   = GIMP_DROP_NONE;
  GdkDragAction drag_action = GDK_ACTION_DEFAULT;
  gboolean      return_val  = FALSE;

  dest = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  if (dest &&
      layer_has_alpha (dest->layer) &&
      (src_widget = gtk_drag_get_source_widget (context)))
    {
      src
	= (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (src_widget));

      if (src &&
	  layer_has_alpha (src->layer) &&
	  ! layer_is_floating_sel (src->layer) &&
	  src->layer == layersD->active_layer)
	{
	  src_index  = gimage_get_layer_index (layersD->gimage, src->layer);
	  dest_index = gimage_get_layer_index (layersD->gimage, dest->layer);

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

  if (drop_type != dest->drop_type)
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

  layer_widget = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  gimp_dnd_set_drawable_preview_icon
    (widget, context,
     GIMP_DRAWABLE (layer_widget->layer),
     layer_widget->layer_preview->style->black_gc);
}

static void
layer_mask_drag_begin_callback (GtkWidget      *widget,
				GdkDragContext *context)
{
  LayerWidget *layer_widget;

  layer_widget = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  gimp_dnd_set_drawable_preview_icon
    (widget, context,
     GIMP_DRAWABLE (layer_get_mask (layer_widget->layer)),
     layer_widget->mask_preview->style->black_gc);
}

static gboolean
layer_widget_drag_drop_callback (GtkWidget      *widget,
				 GdkDragContext *context,
				 gint            x,
				 gint            y,
				 guint           time)
{
  LayerWidget *dest;
  gint         dest_index;
  GtkWidget   *src_widget;
  LayerWidget *src;
  gint         src_index;
  gint         difference;

  GimpDropType drop_type  = GIMP_DROP_NONE;
  gboolean     return_val = FALSE;

  dest = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  if (dest &&
      layer_has_alpha (dest->layer) &&
      (src_widget = gtk_drag_get_source_widget (context)))
    {
      src
	= (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (src_widget));

      if (src &&
	  layer_has_alpha (src->layer) &&
	  ! layer_is_floating_sel (src->layer) &&
	  src->layer == layersD->active_layer)
	{
	  src_index  = gimage_get_layer_index (layersD->gimage, src->layer);
	  dest_index = gimage_get_layer_index (layersD->gimage, dest->layer);

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
	      gimp_image_position_layer (layersD->gimage, src->layer,
					 dest_index, TRUE);
	      gdisplays_flush ();

	      return_val = TRUE;
	    }
	}
    }

  layer_widget_draw_drop_indicator (dest, dest->drop_type);
  dest->drop_type = GIMP_DROP_NONE;

  gtk_drag_finish (context, return_val, FALSE, time);

  return return_val;
}

static void
layer_widget_drag_leave_callback (GtkWidget      *widget,
				  GdkDragContext *context,
				  guint           time)
{
  LayerWidget *layer_widget;

  layer_widget = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  layer_widget->drop_type = GIMP_DROP_NONE;
}

static void
layer_widget_drag_indicator_callback (GtkWidget *widget,
				      gpointer   data)
{
  LayerWidget *layer_widget;

  layer_widget = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  layer_widget_draw_drop_indicator (layer_widget, layer_widget->drop_type);
}

static void
layer_widget_draw_drop_indicator (LayerWidget   *layer_widget,
				  GimpDropType   drop_type)
{
  static GdkGC *gc = NULL;
  gint y = 0;

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
      gimage_set_active_layer (layer_widget->gimage, layer_widget->layer);

      gdisplays_flush ();
    }
}

static gint
layer_widget_button_events (GtkWidget *widget,
			    GdkEvent  *event)
{
  LayerWidget *layer_widget;
  GtkWidget *event_widget;
  GdkEventButton *bevent;
  gint return_val;

  static gboolean button_down = FALSE;
  static GtkWidget *click_widget = NULL;
  static gint old_state;
  static gint exclusive;

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

      button_down = TRUE;
      click_widget = widget;
      gtk_grab_add (click_widget);
      
      if (widget == layer_widget->eye_widget)
	{
	  old_state = GIMP_DRAWABLE (layer_widget->layer)->visible;

	  /*  If this was a shift-click, make all/none visible  */
	  if (event->button.state & GDK_SHIFT_MASK)
	    {
	      exclusive = TRUE;
	      layer_widget_exclusive_visible (layer_widget);
	    }
	  else
	    {
	      exclusive = FALSE;
	      GIMP_DRAWABLE (layer_widget->layer)->visible =
		!GIMP_DRAWABLE (layer_widget->layer)->visible;
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
	      gimage_invalidate_preview (layer_widget->gimage);
	      gdisplays_update_area (layer_widget->gimage, 0, 0,
				     layer_widget->gimage->width,
				     layer_widget->gimage->height);
	      gdisplays_flush ();
	    }
	  else if (old_state != GIMP_DRAWABLE (layer_widget->layer)->visible)
	    {
	      /*  Invalidate the gimage preview  */
	      gimage_invalidate_preview (layer_widget->gimage);
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
		  GIMP_DRAWABLE (layer_widget->layer)->visible =
		    !GIMP_DRAWABLE (layer_widget->layer)->visible;
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
  lc_dialog_menu_preview_dirty(GTK_OBJECT(gimp_drawable_gimage(GIMP_DRAWABLE(layer_widget->layer))),NULL);
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
	visible |= GIMP_DRAWABLE (lw->layer)->visible;

      list = g_slist_next (list);
    }

  /*  Now, toggle the visibility for all layers except the specified one  */
  list = layersD->layer_widgets;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      if (lw != layer_widget)
	GIMP_DRAWABLE (lw->layer)->visible = !visible;
      else
	GIMP_DRAWABLE (lw->layer)->visible = TRUE;

      layer_widget_eye_redraw (lw);

      list = g_slist_next (list);
    }
}

static void
layer_widget_layer_flush (GtkWidget *widget,
			  gpointer   data)
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
      gtk_adjustment_set_value (GTK_ADJUSTMENT (layersD->opacity_data),
				(gfloat) layer_widget->layer->opacity / 2.55);
      gimp_option_menu_set_history (GTK_OPTION_MENU (layersD->mode_option_menu),
				    (gpointer) layer_widget->layer->mode);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (layersD->preserve_trans),
				    (layer_widget->layer->preserve_trans) ?
				    GTK_STATE_ACTIVE : GTK_STATE_NORMAL);
    }

  if (layer_is_floating_sel (layer_widget->layer))
    name = _("Floating Selection");
  else
    name = layer_get_name (layer_widget->layer);

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
    gtk_widget_queue_draw (layer_widget->layer_preview);
  if (update_mask_preview)
    gtk_widget_queue_draw (layer_widget->mask_preview);
}

/********************************/
/*  The new layer query dialog  */
/********************************/

typedef struct _NewLayerOptions NewLayerOptions;

struct _NewLayerOptions
{
  GtkWidget    *query_box;
  GtkWidget    *name_entry;
  GtkWidget    *size_se;

  GimpFillType  fill_type;
  gint          xsize;
  gint          ysize;

  GimpImage    *gimage;
};

static GimpFillType  fill_type  = TRANSPARENT_FILL;
static gchar        *layer_name = NULL;

static void
new_layer_query_ok_callback (GtkWidget *widget,
			     gpointer   data)
{
  NewLayerOptions *options;
  Layer  *layer;
  GImage *gimage;

  options = (NewLayerOptions *) data;

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
      undo_push_group_start (gimage, LAYER_ADD_UNDO);

      layer = layer_new (gimage, options->xsize, options->ysize,
			 gimage_base_type_with_alpha (gimage),
			 layer_name, OPAQUE_OPACITY, NORMAL_MODE);
      if (layer) 
	{
	  drawable_fill (GIMP_DRAWABLE (layer), fill_type);
	  gimage_add_layer (gimage, layer, -1);
	  
	  /*  End the group undo  */
	  undo_push_group_end (gimage);
	  
	  gdisplays_flush ();
	} 
      else 
	{
	  g_message ("new_layer_query_ok_callback():\n"
		     "could not allocate new layer");
	}
    }

  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
new_layer_query_cancel_callback (GtkWidget *widget,
				 gpointer   data)
{
  NewLayerOptions *options;

  options = (NewLayerOptions *) data;

  gtk_widget_destroy (options->query_box);
  g_free (options);
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
  GtkWidget *frame;

  /*  The new options structure  */
  options = g_new (NewLayerOptions, 1);
  options->fill_type = fill_type;
  options->gimage    = gimage;

  /*  The dialog  */
  options->query_box =
    gimp_dialog_new (_("New Layer Options"), "new_layer_options",
		     gimp_standard_help_func,
		     "dialogs/layers/new_layer.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("OK"), new_layer_query_ok_callback,
		     options, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), new_layer_query_cancel_callback,
		     options, NULL, NULL, FALSE, TRUE,

		     NULL);

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     vbox);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  The name label and entry  */
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

  /*  The size labels  */
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

  /*  The size sizeentry  */
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

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (options->size_se), GIMP_UNIT_PIXEL);

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

  /*  The radio frame  */
  frame =
    gimp_radio_group_new2 (TRUE, _("Layer Fill Type"),
			   gimp_radio_button_update,
			   &options->fill_type, (gpointer) options->fill_type,

			   _("Foreground"),  (gpointer) FOREGROUND_FILL, NULL,
			   _("Background"),  (gpointer) BACKGROUND_FILL, NULL,
			   _("White"),       (gpointer) WHITE_FILL, NULL,
			   _("Transparent"), (gpointer) TRANSPARENT_FILL, NULL,

			   NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}

/**************************************/
/*  The edit layer attributes dialog  */
/**************************************/

typedef struct _EditLayerOptions EditLayerOptions;

struct _EditLayerOptions
{
  GtkWidget *query_box;
  GtkWidget *name_entry;
  GimpLayer *layer;
  GImage    *gimage;
};

static void
edit_layer_query_ok_callback (GtkWidget *widget,
			      gpointer   data)
{
  EditLayerOptions *options;
  Layer *layer;

  options = (EditLayerOptions *) data;

  if ((layer = options->layer))
    {
      /*  Set the new layer name  */
      if (GIMP_DRAWABLE (layer)->name && layer_is_floating_sel (layer))
	{
	  /*  If the layer is a floating selection, make it a layer  */
	  floating_sel_to_layer (layer);
	}
      else
        {
	  /* We're doing a plain rename */
          undo_push_layer_rename (options->gimage, layer);
        }

      layer_set_name (layer,
		      gtk_entry_get_text (GTK_ENTRY (options->name_entry)));
    }

  gdisplays_flush ();

  gtk_widget_destroy (options->query_box); 
  
  g_free (options);
}

static void
edit_layer_query_cancel_callback (GtkWidget *widget,
				  gpointer   data)
{
  EditLayerOptions *options;

  options = (EditLayerOptions *) data;
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
layers_dialog_edit_layer_query (LayerWidget *layer_widget)
{
  EditLayerOptions *options;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;

  /*  The new options structure  */
  options = g_new (EditLayerOptions, 1);
  options->layer  = layer_widget->layer;
  options->gimage = layer_widget->gimage;

  /*  The dialog  */
  options->query_box =
    gimp_dialog_new (_("Edit Layer Attributes"), "edit_layer_attributes",
		     gimp_standard_help_func,
		     "dialogs/layers/edit_layer_attributes.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("OK"), edit_layer_query_ok_callback,
		     options, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), edit_layer_query_cancel_callback,
		     options, NULL, NULL, FALSE, TRUE,

		     NULL);

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     vbox);

  /*  The name hbox, label and entry  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Layer name:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->name_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), options->name_entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry),
		      ((layer_is_floating_sel (layer_widget->layer) ?
			_("Floating Selection") :
			layer_get_name (layer_widget->layer))));
  gtk_widget_show (options->name_entry);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}

/*******************************/
/*  The add mask query dialog  */
/*******************************/

typedef struct _AddMaskOptions AddMaskOptions;

struct _AddMaskOptions
{
  GtkWidget   *query_box;
  Layer       *layer;
  AddMaskType  add_mask_type;
};

static void
add_mask_query_ok_callback (GtkWidget *widget,
			    gpointer   data)
{
  AddMaskOptions *options;
  GImage    *gimage;
  LayerMask *mask;
  Layer     *layer;

  options = (AddMaskOptions *) data;
  if ((layer = (options->layer)) &&
      (gimage = GIMP_DRAWABLE (layer)->gimage))
    {
      mask = layer_create_mask (layer, options->add_mask_type);
      gimage_add_layer_mask (gimage, layer, mask);
      gdisplays_flush ();
    }

  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
add_mask_query_cancel_callback (GtkWidget *widget,
				gpointer   data)
{
  AddMaskOptions *options;

  options = (AddMaskOptions *) data;
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
layers_dialog_add_mask_query (Layer *layer)
{
  AddMaskOptions *options;
  GtkWidget *frame;

  /*  The new options structure  */
  options = g_new (AddMaskOptions, 1);
  options->layer         = layer;
  options->add_mask_type = ADD_WHITE_MASK;

  /*  The dialog  */
  options->query_box =
    gimp_dialog_new (_("Add Mask Options"), "add_mask_options",
		     gimp_standard_help_func,
		     "dialogs/layers/add_mask.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("OK"), add_mask_query_ok_callback,
		     options, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), add_mask_query_cancel_callback,
		     options, NULL, NULL, FALSE, TRUE,

		     NULL);


  /*  The radio frame and box  */
  frame = gimp_radio_group_new2 (TRUE, _("Initialize Layer Mask to:"),
				 gimp_radio_button_update,
				 &options->add_mask_type,
				 (gpointer) options->add_mask_type,

				 _("White (Full Opacity)"),
				 (gpointer) ADD_WHITE_MASK, NULL,
				 _("Black (Full Transparency)"),
				 (gpointer) ADD_BLACK_MASK, NULL,
				 _("Layer's Alpha Channel"),
				 (gpointer) ADD_ALPHA_MASK, NULL,
				 NULL);

  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     frame);
  gtk_widget_show (frame);

  gtk_widget_show (options->query_box);
}


/****************************/
/*  The scale layer dialog  */
/****************************/

typedef struct _ScaleLayerOptions ScaleLayerOptions;

struct _ScaleLayerOptions
{
  Layer  *layer;
  Resize *resize;
};

static void
scale_layer_query_ok_callback (GtkWidget *widget,
			       gpointer   data)
{
  ScaleLayerOptions *options;
  GImage *gimage;
  Layer  *layer;

  options = (ScaleLayerOptions *) data;

  if (options->resize->width > 0 && options->resize->height > 0 &&
      (layer =  (options->layer)))
    {
      if ((gimage = GIMP_DRAWABLE (layer)->gimage) != NULL)
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
    g_message (_("Invalid width or height.\n"
		 "Both must be positive."));
}

static void
scale_layer_query_cancel_callback (GtkWidget *widget,
				   gpointer   data)
{
  ScaleLayerOptions *options;
  options = (ScaleLayerOptions *) data;

  resize_widget_free (options->resize);
  g_free (options);
}

static void
layers_dialog_scale_layer_query (GImage *gimage,
				 Layer  *layer)
{
  ScaleLayerOptions *options;

  /*  the new options structure  */
  options = g_new (ScaleLayerOptions, 1);
  options->layer  = layer;
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
				       options);

  gtk_widget_show (options->resize->resize_shell);
}

/*****************************/
/*  The resize layer dialog  */
/*****************************/

typedef struct _ResizeLayerOptions ResizeLayerOptions;

struct _ResizeLayerOptions
{
  Layer  *layer;
  Resize *resize;
};

static void
resize_layer_query_ok_callback (GtkWidget *widget,
				gpointer   data)
{
  ResizeLayerOptions *options;
  GImage *gimage;
  Layer  *layer;

  options = (ResizeLayerOptions *) data;

  if (options->resize->width > 0 && options->resize->height > 0 &&
      (layer = (options->layer)))
    {
      if ((gimage = GIMP_DRAWABLE (layer)->gimage) != NULL)
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
    g_message (_("Invalid width or height.\n"
		 "Both must be positive."));
}

static void
resize_layer_query_cancel_callback (GtkWidget *widget,
				    gpointer   data)
{
  ResizeLayerOptions *options;
  options = (ResizeLayerOptions *) data;

  resize_widget_free (options->resize);
  g_free (options);
}

static void
layers_dialog_resize_layer_query (GImage *gimage,
				  Layer  *layer)
{
  ResizeLayerOptions *options;

  /*  the new options structure  */
  options = g_new (ResizeLayerOptions, 1);
  options->layer  = layer;
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
				       options);

  gtk_widget_show (options->resize->resize_shell);
}

/****************************/
/*  The layer merge dialog  */
/****************************/

typedef struct _LayerMergeOptions LayerMergeOptions;

struct _LayerMergeOptions
{
  GtkWidget *query_box;
  GimpImage *gimage;
  gboolean   merge_visible;
  MergeType  merge_type;
};

static void
layer_merge_query_ok_callback (GtkWidget *widget,
			       gpointer   data)
{
  LayerMergeOptions *options;
  GImage *gimage;

  options = (LayerMergeOptions *) data;
  if (! (gimage = options->gimage))
    return;

  if (options->merge_visible)
    gimage_merge_visible_layers (gimage, options->merge_type);

  gdisplays_flush ();
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
layer_merge_query_cancel_callback (GtkWidget *widget,
				   gpointer   data)
{
  LayerMergeOptions *options;

  options = (LayerMergeOptions *) data;
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

void
layers_dialog_layer_merge_query (GImage   *gimage,
				 /*  if FALSE, anchor active layer  */
				 gboolean  merge_visible)
{
  LayerMergeOptions *options;
  GtkWidget *vbox;
  GtkWidget *frame;

  /*  The new options structure  */
  options = g_new (LayerMergeOptions, 1);
  options->gimage        = gimage;
  options->merge_visible = merge_visible;
  options->merge_type    = EXPAND_AS_NECESSARY;

  /* The dialog  */
  options->query_box =
    gimp_dialog_new (_("Layer Merge Options"), "layer_merge_options",
		     gimp_standard_help_func,
		     "dialogs/layers/merge_visible_layers.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("OK"), layer_merge_query_ok_callback,
		     options, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), layer_merge_query_cancel_callback,
		     options, NULL, NULL, FALSE, TRUE,

		     NULL);

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     vbox);

  frame = gimp_radio_group_new2 (TRUE,
				 merge_visible ?
				 _("Final, Merged Layer should be:") :
				 _("Final, Anchored Layer should be:"),
				 gimp_radio_button_update,
				 &options->merge_type,
				 (gpointer) options->merge_type,

				 _("Expanded as necessary"),
				 (gpointer) EXPAND_AS_NECESSARY, NULL,
				 _("Clipped to image"),
				 (gpointer) CLIP_TO_IMAGE, NULL,
				 _("Clipped to bottom layer"),
				 (gpointer) CLIP_TO_BOTTOM_LAYER, NULL,

				 NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}
