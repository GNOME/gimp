/* align_layers.c -- This is a plug-in for the GIMP (1.0's API)
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Time-stamp: <1999-12-18 05:48:38 yasuhiro>
 * Version:  0.26
 *
 * Copyright (C) 1997-1998 Shuji Narazaki <narazaki@InetQ.or.jp>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimp/gimpmath.h>

#include "libgimp/stdplugins-intl.h"

#define	PLUG_IN_NAME	"plug_in_align_layers"
#define SHORT_NAME	"align_layers"
#define PROGRESS_NAME	"align_layers"
#define	MAIN_FUNCTION	main_function
#define INTERFACE	align_layers_interface
#define	DIALOG		align_layers_dialog
#define VALS		align_layers_vals
#define OK_CALLBACK	align_layers_ok_callback
#define	PREVIEW_UPDATE	_preview_update
#define PROGRESS_UPDATE_NUM	100

/* gtkWrapper functions */ 
#define GTKW_ENTRY_WIDTH	40
#define GTKW_SCALE_WIDTH	100
#define GTKW_PREVIEW_WIDTH	50
#define GTKW_PREVIEW_HEIGHT	256
#define	GTKW_BORDER_WIDTH	5
#define GTKW_FLOAT_MIN_ERROR	0.000001
static gint	**gtkW_gint_wrapper_new (gint index, gint *pointer);
static void	gtkW_toggle_update (GtkWidget *widget, gpointer data);
static void	gtkW_iscale_update (GtkAdjustment *adjustment, gpointer data);
static void	gtkW_ientry_update (GtkWidget *widget, gpointer data);
static void     gtkW_table_add_toggle (GtkWidget	*table,
				       gchar	*name,
				       gint	x,
				       gint	y,
				       GtkSignalFunc update,
				       gint	*value);
static void     gtkW_table_add_iscale_entry (GtkWidget	*table,
					     gchar	*name,
					     gint	x,
					     gint	y,
					     GtkSignalFunc	scale_update,
					     GtkSignalFunc	entry_update,
					     gint	*value,
					     gdouble	min,
					     gdouble	max,
					     gdouble	step,
					     gchar	*buffer);
GtkWidget *gtkW_table_add_button (GtkWidget	*table,
				  gchar	*name,
				  gint	x0,
				  gint	x1,
				  gint	y, 
				  GtkSignalFunc	callback,
				  gpointer value);
typedef struct
{
  gchar *name;
  gpointer data;
} gtkW_menu_item;
GtkWidget *gtkW_table_add_menu (GtkWidget *parent,
				gchar *name,
				int x,
				int y,
				GtkSignalFunc imenu_update,
				int *val,
				gtkW_menu_item *item,
				int item_num);
static void	gtkW_menu_update (GtkWidget *widget, gpointer data);
GtkWidget *gtkW_check_button_new (GtkWidget	*parent,
				  gchar	*name,
				  GtkSignalFunc update,
				  gint	*value);
GtkWidget *gtkW_frame_new (GtkWidget *parent, gchar *name);
GtkWidget *gtkW_table_new (GtkWidget *parent, gint col, gint row);
GtkWidget *gtkW_hbox_new (GtkWidget *parent);
GtkWidget *gtkW_vbox_new (GtkWidget *parent);
/* end of GtkW */

static void	query	(void);
static void	run	(gchar   *name,
			 gint     nparams,
			 GParam  *param,
			 gint    *nreturn_vals,
			 GParam **return_vals);

static GStatusType align_layers (gint32 image_id);
static void align_layers_get_align_offsets (gint32	drawable_id,
					    gint	*x,
					    gint	*y);
static gint DIALOG      (void);
static void OK_CALLBACK (GtkWidget *widget, gpointer   data);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};

/* dialog variables */
gtkW_menu_item h_style_menu [] =
{
#define	H_NONE		0
  { N_("None"), NULL },
#define H_COLLECT	1
  { N_("Collect"), NULL },
#define	LEFT2RIGHT	2
  { N_("Fill (left to right)"), NULL },
#define	RIGHT2LEFT	3
  { N_("Fill (right to left)"), NULL },
#define SNAP2HGRID	4
  { N_("Snap to grid"), NULL }
};

gtkW_menu_item h_base_menu [] =
{
#define H_BASE_LEFT	0
  { N_("Left edge"), NULL },
#define H_BASE_CENTER	1
  { N_("Center"), NULL },
#define	H_BASE_RIGHT	2
  { N_("Right edge"), NULL }
};

gtkW_menu_item v_style_menu [] =
{
#define	V_NONE		0
  { N_("None"), NULL },
#define V_COLLECT	1
  { N_("Collect"), NULL },
#define TOP2BOTTOM	2
  { N_("Fill (top to bottom)"), NULL },
#define BOTTOM2TOP	3
  { N_("Fill (bottom to top)"), NULL },
#define SNAP2VGRID	4
  { N_("Snap to grid"), NULL }
};

gtkW_menu_item v_base_menu [] =
{
#define V_BASE_TOP	0
  { N_("Top edge"), NULL },
#define V_BASE_CENTER	1
  { N_("Center"), NULL },
#define V_BASE_BOTTOM	2
  { N_("Bottom edge"), NULL }
};

typedef struct
{
  gint	h_style;
  gint	h_base;
  gint	v_style;
  gint	v_base;
  gint	ignore_bottom;
  gint	base_is_bottom_layer;
  gint	grid_size;
} ValueType;

static ValueType VALS = 
{
  H_NONE, H_BASE_LEFT, V_NONE, V_BASE_TOP, 1, 0, 10
};

typedef struct 
{
  gint run;
} Interface;

static Interface INTERFACE = { FALSE };

/* gint	link_after_alignment = 0;*/

MAIN ()

static void
query (void)
{
  static GParamDef args [] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    { PARAM_IMAGE, "image", "Input image"},
    { PARAM_DRAWABLE, "drawable", "Input drawable (not used)"},
    { PARAM_INT32, "link-afteer-alignment", "Link the visible layers after alignment"},
    { PARAM_INT32, "use-bottom", "use the bottom layer as the base of alignment"},
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  INIT_I18N();
  
  gimp_install_procedure (PLUG_IN_NAME,
			  _("Align visible layers"),
			  _("align visible layers"),
			  "Shuji Narazaki <narazaki@InetQ.or.jp>",
			  "Shuji Narazaki",
			  "1997",
			  N_("<Image>/Layers/Align Visible Layers..."),
			  "RGB*,GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam	 values[1];
  GStatusType	status = STATUS_EXECUTION_ERROR;
  GRunModeType	run_mode;
  gint		image_id, layer_num;
  
  run_mode = param[0].data.d_int32;
  image_id = param[1].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch ( run_mode )
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_image_get_layers (image_id, &layer_num);
      if (layer_num < 2)
	{
	  g_message (_("Align Visible Layers: there are too few layers."));
	  return;
	}
      gimp_get_data (PLUG_IN_NAME, &VALS);
      if (! DIALOG ())
	return;
      break;
    case RUN_NONINTERACTIVE:
      INIT_I18N();
      break;
    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      gimp_get_data (PLUG_IN_NAME, &VALS);
      break;
    }
  
  status = align_layers (image_id);

  if (run_mode != RUN_NONINTERACTIVE)
    gimp_displays_flush();
  if (run_mode == RUN_INTERACTIVE && status == STATUS_SUCCESS )
    gimp_set_data (PLUG_IN_NAME, &VALS, sizeof (ValueType));

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static GStatusType
align_layers (gint32 image_id)
{
 gint	layer_num = 0;
  gint	visible_layer_num = 1;
  gint	*layers = NULL;
  gint	index, vindex;
  gint	step_x = 0;
  gint	step_y = 0;
  gint	x = 0;
  gint	y = 0;
  gint	orig_x = 0;
  gint	orig_y = 0;
  gint	offset_x = 0;
  gint	offset_y = 0;
  gint	base_x = 0;
  gint	base_y =0;

  layers = gimp_image_get_layers (image_id, &layer_num);
  for (index = 0; index < layer_num; index++)
    if (gimp_layer_get_visible (layers[index]))
      visible_layer_num++;

  if (VALS.ignore_bottom)
    {
      layer_num--;
      visible_layer_num--;
    }

  if (0 < visible_layer_num)
    {
      gint	unintialzied = 1;
      gint	min_x = 0;
      gint	min_y = 0;
      gint	max_x = 0;
      gint	max_y = 0;
      
      /* 0 is the top layer */
      for (index = 0; index < layer_num; index++)
	if (gimp_layer_get_visible (layers[index]))
	  {
	    gimp_drawable_offsets (layers[index], &orig_x, &orig_y);
	    align_layers_get_align_offsets (layers[index], &offset_x, &offset_y);
	    orig_x += offset_x;
	    orig_y += offset_y;

	    if (unintialzied)
	      {
		base_x = min_x = max_y = orig_x;
		base_y = min_y = max_y = orig_y;
		unintialzied = 0;
	      }
	    else
	      {
		if ( orig_x < min_x ) min_x = orig_x;
		if ( max_x < orig_x ) max_x = orig_x;
		if ( orig_y < min_y ) min_y = orig_y;
		if ( max_y < orig_y ) max_y = orig_y;
	      }
	  }
      if (VALS.base_is_bottom_layer)
	{
	  gimp_drawable_offsets (layers[layer_num], &orig_x, &orig_y);
	  align_layers_get_align_offsets (layers[layer_num], &offset_x, &offset_y);
	  orig_x += offset_x;
	  orig_y += offset_y;
	  base_x = min_x = max_y = orig_x;
	  base_y = min_y = max_y = orig_y;
	}
      if (1 < visible_layer_num)
	{
	  step_x = (max_x - min_x) / (visible_layer_num - 1);
	  step_y = (max_y - min_y) / (visible_layer_num - 1);
	}
      if ( (VALS.h_style == LEFT2RIGHT) || (VALS.h_style == RIGHT2LEFT))
	base_x = min_x;
      if ( (VALS.v_style == TOP2BOTTOM) || (VALS.v_style == BOTTOM2TOP))
	base_y = min_y;
    }

  gimp_undo_push_group_start (image_id);

  for (vindex = -1, index = 0; index < layer_num; index++)
    {
      if (gimp_layer_get_visible (layers[index])) 
	vindex++;
      else 
	continue;

      gimp_drawable_offsets (layers[index], &orig_x, &orig_y);
      align_layers_get_align_offsets (layers[index], &offset_x, &offset_y);
      
      switch (VALS.h_style)
	{
	case H_NONE:
	  x = orig_x;
	  break;
	case H_COLLECT:
	  x = base_x - offset_x;
	  break;
	case LEFT2RIGHT:
	  x = (base_x + vindex * step_x) - offset_x;
	  break;
	case RIGHT2LEFT:
	  x = (base_x + (visible_layer_num - vindex) * step_x) - offset_x;
	  break;
	case SNAP2HGRID:
	  x = VALS.grid_size
	    * (int) ((orig_x + offset_x + VALS.grid_size /2) / VALS.grid_size)
	    - offset_x;
	  break;
	}
      switch (VALS.v_style)
	{
	case V_NONE:
	  y = orig_y;
	  break;
	case V_COLLECT:
	  y = base_y - offset_y;
	  break;
	case TOP2BOTTOM:
	  y = (base_y + vindex * step_y) - offset_y;
	  break;
	case BOTTOM2TOP:
	  y = (base_y + (visible_layer_num - vindex) * step_y) - offset_y;
	  break;
	case SNAP2VGRID:
	  y = VALS.grid_size 
	    * (int) ((orig_y + offset_y + VALS.grid_size / 2) / VALS.grid_size)
	    - offset_y;
	  break;
	}
      gimp_layer_set_offsets (layers[index], x, y);
    }
  
  gimp_undo_push_group_end (image_id);

  return STATUS_SUCCESS;
}

static void
align_layers_get_align_offsets (gint32	drawable_id,
			       gint	*x,
			       gint	*y)
{
  GDrawable	*layer = gimp_drawable_get (drawable_id);
  
  switch (VALS.h_base)
    {
    case H_BASE_LEFT:
      *x = 0;
      break;
    case H_BASE_CENTER:
      *x = (gint) (layer->width / 2);
      break;
    case H_BASE_RIGHT:
      *x = layer->width;
      break;
    default:
      *x = 0;
      break;
    }
  switch (VALS.v_base)
    {
    case V_BASE_TOP:
      *y = 0;
      break;
    case V_BASE_CENTER:
      *y = (gint) (layer->height / 2);
      break;
    case V_BASE_BOTTOM:
      *y = layer->height;
      break;
    default:
      *y = 0;
      break;
    }
}

/* dialog stuff */
static int
DIALOG (void)
{
  GtkWidget	*dlg;
  GtkWidget	*frame;
  GtkWidget	*table;
#ifdef OLD
  GtkWidget	*hbox, *vbox;
  GtkWidget	*sep;
  GtkWidget	*toggle_vbox;
  GSList	*group = NULL;
#endif
  int		index = 0;
  gchar	buffer[10];
  gchar	**argv;
  gint	argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup (PLUG_IN_NAME);
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
  
  dlg = gimp_dialog_new (_("Align Visible Layers"), "align_layers",
			 gimp_plugin_help_func, "filters/align_layers.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), OK_CALLBACK,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);
  
#ifdef OLD
  hbox = gtkW_hbox_new ((GTK_DIALOG (dlg)->vbox));
  frame = gtkW_frame_new (hbox, "Horizontal Settings");
  
  toggle_vbox = gtkW_vbox_new (frame);

  group = gtkW_vbox_add_radio_button (toggle_vbox, "None", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[H_NONE]);
  group = gtkW_vbox_add_radio_button (toggle_vbox, "Collect", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[H_COLLECT]);
  group = gtkW_vbox_add_radio_button (toggle_vbox, "Fill (left to right)", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[LEFT2RIGHT]);
  group = gtkW_vbox_add_radio_button (toggle_vbox, "Fill (right to left)", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[RIGHT2LEFT]);
  group = gtkW_vbox_add_radio_button (toggle_vbox, "Snap to grid", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[SNAP2HGRID]);
  /* h base */
  sep = gtk_hseparator_new ();
  gtk_container_add (GTK_CONTAINER (toggle_vbox), sep);
  gtk_widget_show (sep);
  group = NULL;
  group = gtkW_vbox_add_radio_button (toggle_vbox, "Left edge", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[H_BASE_LEFT]);
  group = gtkW_vbox_add_radio_button (toggle_vbox, "Center", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[H_BASE_CENTER]);
  group = gtkW_vbox_add_radio_button (toggle_vbox, "Right edge", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[H_BASE_RIGHT]);

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);
  frame = gtkW_frame_new (hbox, "Vetical Settings");
  toggle_vbox = gtkW_vbox_new (frame);

  /* v style */
  group = NULL;
  group = gtkW_vbox_add_radio_button (toggle_vbox, "None", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[V_NONE]);
  group = gtkW_vbox_add_radio_button (toggle_vbox, "Collect", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[V_COLLECT]);
  group = gtkW_vbox_add_radio_button (toggle_vbox, "Fill (top to bottom)", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[TOP2BOTTOM]);
  group = gtkW_vbox_add_radio_button (toggle_vbox, "Fill (bottom to top)", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[BOTTOM2TOP]);
  group = gtkW_vbox_add_radio_button (toggle_vbox, "Snap to grid", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[SNAP2VGRID]);
  /* v base */
  sep = gtk_hseparator_new ();
  gtk_container_add (GTK_CONTAINER (toggle_vbox), sep);
  gtk_widget_show (sep);
  group = NULL;
  group = gtkW_vbox_add_radio_button (toggle_vbox, "Top edge", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[V_BASE_TOP]);
  group = gtkW_vbox_add_radio_button (toggle_vbox, "Center", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[V_BASE_CENTER]);
  group = gtkW_vbox_add_radio_button (toggle_vbox, "Bottom edge", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &d_var[V_BASE_BOTTOM]);

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);
#else
  frame = gtkW_frame_new (GTK_DIALOG (dlg)->vbox, _("Parameter Settings"));
  table = gtkW_table_new (frame, 7, 2);
  gtkW_table_add_menu (table, _("Horizontal Style:"), 0, index++,
		       (GtkSignalFunc) gtkW_menu_update,
		       &VALS.h_style,
		       h_style_menu,
		       sizeof (h_style_menu) / sizeof (h_style_menu[0]));
  gtkW_table_add_menu (table, _("Horizontal Base:"), 0, index++,
		       (GtkSignalFunc) gtkW_menu_update,
		       &VALS.h_base,
		       h_base_menu,
		       sizeof (h_base_menu) / sizeof (h_base_menu[0]));
  gtkW_table_add_menu (table, _("Vertical Style:"), 0, index++,
		       (GtkSignalFunc) gtkW_menu_update,
		       &VALS.v_style,
		       v_style_menu,
		       sizeof (v_style_menu) / sizeof (v_style_menu[0]));
  gtkW_table_add_menu (table, _("Vertical Base:"), 0, index++,
		       (GtkSignalFunc) gtkW_menu_update,
		       &VALS.v_base,
		       v_base_menu,
		       sizeof (v_base_menu) / sizeof (v_base_menu[0]));
#endif

  gtkW_table_add_toggle (table, _("Ignore the Bottom Layer even if Visible"),
			 0, index++,
			 (GtkSignalFunc) gtkW_toggle_update,
			 &VALS.ignore_bottom);
  gtkW_table_add_toggle (table,
			 _("Use the (Invisible) Bottom Layer as the Base"),
			 0, index++,
			 (GtkSignalFunc) gtkW_toggle_update,
			 &VALS.base_is_bottom_layer);
  gtkW_table_add_iscale_entry (table, _("Grid Size:"),
			       0, index++,
			       (GtkSignalFunc) gtkW_iscale_update,
			       (GtkSignalFunc) gtkW_ientry_update,
			       &VALS.grid_size, 0, 200, 1, buffer);

  gtk_widget_show (table);
  gtk_widget_show (frame);
  gtk_widget_show (dlg);
  
  gtk_main ();
  gdk_flush ();

  return INTERFACE.run;
}

static void
OK_CALLBACK (GtkWidget *widget,
	      gpointer   data)
{
#ifdef OLD
  int	index;
  
  for (index = H_NONE; index <= SNAP2HGRID; index++)
    if (d_var[index])
      {
	VALS.h_style = index;
	break;
      }
  for (index = H_BASE_LEFT; index <= H_BASE_RIGHT; index++)
    if (d_var[index])
      {
	VALS.h_base = index;
	break;
      }
  for (index = V_NONE; index <= SNAP2VGRID; index++)
    if (d_var[index])
      {
	VALS.v_style = index;
	break;
      }
  for (index = V_BASE_TOP; index <= V_BASE_BOTTOM; index++)
    if (d_var[index])
      {
	VALS.v_base = index;
	break;
      }
#endif
  INTERFACE.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
PREVIEW_UPDATE ()
{
}

/* gtkW functions: gtkW is the abbreviation of gtk Wrapper */
static gint **
gtkW_gint_wrapper_new (gint index, gint *pointer)
{
  gint **tmp;
  
   tmp = (gint **)malloc(3 * sizeof (gint *));
   tmp[0] = (gint *) ((*pointer == index) ? TRUE : FALSE);
   tmp[1] = pointer;
   tmp[2] = (gint *) index;
   return tmp;
}

static void
gtkW_toggle_update (GtkWidget *widget,
		    gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
  PREVIEW_UPDATE ();
}

GtkWidget *
gtkW_table_new (GtkWidget *parent, gint col, gint row)
{
  GtkWidget	*table;
  
  table = gtk_table_new (col,row, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (parent), table);
  gtk_widget_show (table);
  
  return table;
}

GtkWidget *
gtkW_hbox_new (GtkWidget *parent)
{
  GtkWidget	*hbox;
  
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (hbox), GTKW_BORDER_WIDTH);
  /* gtk_box_pack_start (GTK_BOX (parent), hbox, FALSE, TRUE, 0); */
  gtk_container_add (GTK_CONTAINER (parent), hbox);
  gtk_widget_show (hbox);

  return hbox;
}

GtkWidget *
gtkW_vbox_new (GtkWidget *parent)
{
  GtkWidget *vbox;
  
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), GTKW_BORDER_WIDTH);
  /* gtk_box_pack_start (GTK_BOX (parent), vbox, TRUE, TRUE, 0); */
  gtk_container_add (GTK_CONTAINER (parent), vbox);
  gtk_widget_show (vbox);

  return vbox;
}

GtkWidget *
gtkW_check_button_new (GtkWidget	*parent,
		       gchar	*name,
		       GtkSignalFunc update,
		       gint	*value)
{
  GtkWidget *toggle;
  
  toggle = gtk_check_button_new_with_label (name);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) update,
		      value);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), *value);
  gtk_container_add (GTK_CONTAINER (parent), toggle);
  gtk_widget_show (toggle);
  return toggle;
}

GtkWidget *
gtkW_frame_new (GtkWidget *parent,
		gchar *name)
{
  GtkWidget *frame;
  
  frame = gtk_frame_new (name);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  if (parent != NULL)
    gtk_box_pack_start (GTK_BOX (parent), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return frame;
}

static void
gtkW_table_add_toggle (GtkWidget	*table,
		       gchar	*name,
		       gint	x,
		       gint	y,
		       GtkSignalFunc update,
		       gint	*value)
{
  GtkWidget *toggle;
  
  toggle = gtk_check_button_new_with_label(name);
  gtk_table_attach (GTK_TABLE (table), toggle, x, x + 2, y, y+1,
		    GTK_FILL|GTK_EXPAND, 0 & GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) update,
		      value);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), *value);
  gtk_widget_show (toggle);
}

static void
gtkW_table_add_iscale_entry (GtkWidget	*table,
			     gchar	*name,
			     gint	x,
			     gint	y,
			     GtkSignalFunc	scale_update,
			     GtkSignalFunc	entry_update,
			     gint	*value,
			     gdouble	min,
			     gdouble	max,
			     gdouble	step,
			     gchar	*buffer)
{
  GtkObject *adjustment;
  GtkWidget *label, *hbox, *scale, *entry;
  
  label = gtk_label_new (name);
  gtk_misc_set_alignment (GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, x, x+1, y, y+1,
		    GTK_FILL|GTK_EXPAND, 0 & GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), hbox, x+1, x+2, y, y+1,
		    GTK_EXPAND | GTK_FILL, 0 &  GTK_FILL, 0, 0);

  adjustment = gtk_adjustment_new (*value, min, max, step, step, 0.0);
  gtk_widget_show (hbox);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_widget_set_usize (scale, GTKW_SCALE_WIDTH, 0);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) scale_update, value);

  entry = gtk_entry_new ();
  gtk_object_set_user_data (GTK_OBJECT (entry), adjustment);
  gtk_object_set_user_data (adjustment, entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize (entry, GTKW_ENTRY_WIDTH, 0);
  sprintf (buffer, "%d", *value);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entry_update, value);

  gtk_widget_show (label);
  gtk_widget_show (scale);
  gtk_widget_show (entry);
}

static void
gtkW_iscale_update (GtkAdjustment *adjustment,
		    gpointer       data)
{
  GtkWidget *entry;
  gchar buffer[32];
  int *val;

  val = data;
  if (*val != (int) adjustment->value)
    {
      *val = adjustment->value;
      entry = gtk_object_get_user_data (GTK_OBJECT (adjustment));
      sprintf (buffer, "%d", (int) adjustment->value);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      PREVIEW_UPDATE ();
    }
}

static void
gtkW_ientry_update (GtkWidget *widget,
		    gpointer   data)
{
  GtkAdjustment *adjustment;
  int new_val;
  int *val;

  val = data;
  new_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));

  if (*val != new_val)
    {
      adjustment = gtk_object_get_user_data (GTK_OBJECT (widget));

      if ((new_val >= adjustment->lower) &&
	  (new_val <= adjustment->upper))
	{
	  *val = new_val;
	  adjustment->value = new_val;
	  gtk_signal_emit_by_name (GTK_OBJECT (adjustment), "value_changed");
	  PREVIEW_UPDATE ();
	}
    }
}

GtkWidget *
gtkW_table_add_button (GtkWidget	*table,
		       gchar	*name,
		       gint	x0,
		       gint	x1,
		       gint	y, 
		       GtkSignalFunc	callback,
		       gpointer value)
{
  GtkWidget *button;
  
  button = gtk_button_new_with_label (name);
  gtk_table_attach (GTK_TABLE(table), button, x0, x1, y, y+1,
		    GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) callback, value);
  gtk_widget_show(button);

  return button;
}

GtkWidget *
gtkW_table_add_menu (GtkWidget *table,
		     gchar *name,
		     int x,
		     int y,
		     GtkSignalFunc menu_update,
		     int *val,
		     gtkW_menu_item *item,
		     int item_num)
{
  GtkWidget *label;
  GtkWidget *menu, *menuitem, *option_menu;
  gchar buf[64];
  gint i;

  label = gtk_label_new (name);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, x, x + 1, y, y + 1,
		    GTK_FILL|GTK_EXPAND, 0 & GTK_FILL, 0, 0);
  gtk_widget_show (label);

  menu = gtk_menu_new ();

  for (i = 0; i < item_num; i++)
    {
      sprintf (buf, gettext(item[i].name));
      menuitem = gtk_menu_item_new_with_label (buf);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  (GtkSignalFunc) menu_update,
			  gtkW_gint_wrapper_new (i, val));
      gtk_widget_show (menuitem);
    }

  option_menu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), *val);
  gtk_table_attach (GTK_TABLE (table), option_menu, x + 1, x + 2, y, y + 1,
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_widget_show (option_menu);

  return option_menu;
}

static void
gtkW_menu_update (GtkWidget *widget,
		  gpointer   data)
{
  gint	**buffer = (gint **) data;

  if (*buffer[1] != (gint) buffer[2])
    {
      *buffer[1] = (gint) buffer[2];
      PREVIEW_UPDATE ();
    }
}
/* end of gtkW functions */
/* end of align_layers.c */
