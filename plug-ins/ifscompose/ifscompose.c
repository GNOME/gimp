/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * IfsCompose is a interface for creating IFS fractals by
 * direct manipulation.
 * Copyright (C) 1997 Owen Taylor
 *
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

/* TODO
 * ----
 *
 * 1. Run in non-interactive mode (need to figure out useful
 *    way for a script to give the 19N paramters for an image).
 *    Perhaps just support saving parameters to a file, script
 *    passes file name.
 * 2. Save settings on a per-layer basis (long term, needs GIMP
 *    support to do properly). Load/save from affine parameters?
 * 3. Figure out if we need multiple phases for supersampled
 *    brushes.
 * 4. (minor) Make undo work correctly when focus is in entry widget.
 */
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "ifscompose.h"

#define SCALE_WIDTH     150
#define ENTRY_WIDTH 60
#define DESIGN_AREA_MAX_SIZE 256

#define PREVIEW_RENDER_CHUNK 10000

#define UNDO_LEVELS 10

#define IFSCOMPOSE_PARASITE "ifscompose-parasite"
#define IFSCOMPOSE_DATA "plug_in_ifscompose"

typedef enum {
  OP_TRANSLATE,
  OP_ROTATE,			/* or scale */
  OP_STRETCH
} DesignOp;

typedef enum {
  VALUE_PAIR_INT,
  VALUE_PAIR_DOUBLE
} ValuePairType;

typedef struct
{
  GtkObject *adjustment;
  GtkWidget *scale;
  GtkWidget *entry;

  ValuePairType type;

  union {
    gdouble *d;
    gint    *i;
  } data;

  gint entry_handler_id;
} ValuePair;

typedef struct
{
  IfsComposeVals ifsvals;
  AffElement **elements;
  gint *element_selected;
  gint current_element;
} UndoItem;

typedef struct
{
  IfsColor *color;
  gchar *name;
  GtkWidget *hbox;
  GtkWidget *orig_preview;
  GtkWidget *preview;
  GtkWidget *dialog;
  gint fixed_point;

  gint in_change_callback;
} ColorMap;

typedef struct
{
  GtkWidget *dialog;

  ValuePair *iterations_pair;
  ValuePair *subdivide_pair;
  ValuePair *radius_pair;
  ValuePair *memory_pair;
} IfsOptionsDialog;

typedef struct
{
  GtkWidget *area;
  GtkWidget *op_menu;
  GdkPixmap *pixmap;

  DesignOp op;
  gdouble op_x;
  gdouble op_y;
  gdouble op_xcenter;
  gdouble op_ycenter;
  gdouble op_center_x;
  gdouble op_center_y;
  guint button_state;
  gint num_selected;

  GdkGC *selected_gc;
} IfsDesignArea;

typedef struct
{
  ValuePair *prob_pair;
  ValuePair *x_pair;
  ValuePair *y_pair;
  ValuePair *scale_pair;
  ValuePair *angle_pair;
  ValuePair *asym_pair;
  ValuePair *shear_pair;
  GtkWidget *flip_check_button;

  ColorMap  *red_cmap;
  ColorMap  *green_cmap;
  ColorMap  *blue_cmap;
  ColorMap  *black_cmap;
  ColorMap  *target_cmap;
  ValuePair *hue_scale_pair;
  ValuePair *value_scale_pair;
  GtkWidget *simple_button;
  GtkWidget *full_button;
  GtkWidget *current_frame;

  GtkWidget *move_button;
  gint move_handler;
  GtkWidget *rotate_button;
  gint rotate_handler;
  GtkWidget *stretch_button;
  gint stretch_handler;

  GtkWidget *preview;
  guchar *preview_data;
  gint preview_iterations;

  gint drawable_width,drawable_height;

  AffElement *selected_orig;
  gint current_element;
  AffElementVals current_vals;
  gint auto_preview;

  gint in_update;		/* true if we're currently in
				   update_values() - don't do anything
				   on updates */
} IfsDialog;

typedef struct
{
  gint       run;
} IfsComposeInterface;

/* Declare local functions.
 */
static void      query  (void);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

/*  user interface functions  */
static gint      ifs_compose_dialog     (GDrawable *drawable);
static void      ifs_options_dialog      ();
static GtkWidget *ifs_compose_trans_page ();
static GtkWidget *ifs_compose_color_page ();
static void design_op_menu_popup         (gint button, guint32 activate_time);
static void design_op_menu_create        (GtkWidget *window);
static void design_area_create(GtkWidget *window,gint design_width,
			       gint design_height);

/* functions for drawing design window */
static void update_values();
static void set_current_element(gint index);
static gint design_area_expose(GtkWidget *widget,GdkEventExpose *event);
static gint design_area_button_press(GtkWidget *widget,
				     GdkEventButton *event);
static gint design_area_button_release(GtkWidget *widget,
				       GdkEventButton *event);
static void design_area_select_all_callback(GtkWidget *w, gpointer data);
static gint design_area_configure(GtkWidget *widget,
				  GdkEventConfigure *event);
static gint design_area_motion(GtkWidget *widget, GdkEventMotion *event);
static void design_area_redraw(void);

/* Undo ring functions */
static void undo_begin(void);
static void undo_update(gint element);
static void undo_exchange(gint el);
static void undo(void);
static void redo(void);

static void recompute_center(int save_undo);
static void recompute_center_cb(GtkWidget *, gpointer);

static void ifs_compose(GDrawable *drawable);

static void color_map_set_preview_color(GtkWidget *preview,
					IfsColor *color);
static ColorMap *color_map_create(gchar *name,IfsColor *orig_color,
				  IfsColor *data, gint fixed_point);
static void color_map_clicked_callback(GtkWidget *widget,ColorMap *colormap);
static void color_map_destroy_callback(GtkWidget *widget,ColorMap *colormap);
static void color_map_color_changed_cb(GtkWidget *widget,
				       ColorMap *color_map);
static void color_map_update(ColorMap *color_map);

/* interface functions */
static void simple_color_toggled(GtkWidget *widget,gpointer data);
static void simple_color_set_sensitive();
static void val_changed_update ();
static ValuePair *value_pair_create (gpointer data, gdouble lower, gdouble upper,
	      gboolean create_scale, ValuePairType type);
static void value_pair_update(ValuePair *value_pair);
static void value_pair_entry_callback (GtkWidget   *w,
				       ValuePair   *value_pair);
static void value_pair_destroy_callback (GtkWidget   *widget,
					 ValuePair   *value_pair);
static void value_pair_button_release (GtkWidget *widget,
				       GdkEventButton *event,
				       gpointer data);
static void value_pair_scale_callback   (GtkAdjustment *adjustment,
					 ValuePair *value_pair);

static void auto_preview_callback (GtkWidget *widget, gpointer data);
static void design_op_callback (GtkWidget *widget, gpointer data);
static void design_op_update_callback (GtkWidget *widget, gpointer data);
static void flip_check_button_callback (GtkWidget *widget, gpointer data);
static gint preview_idle_render();

static void ifs_options_close_callback ();
static void ifs_compose_set_defaults ();
static void ifs_compose_defaults_callback (GtkWidget *widget,
					   gpointer   data);
static void ifs_compose_new_callback (GtkWidget *widget,
				      gpointer   data);
static void ifs_compose_delete_callback (GtkWidget *widget,
					 gpointer   data);
static void ifs_compose_preview_callback (GtkWidget *widget,
					  GtkWidget *preview);

static void ifs_compose_close_callback (GtkWidget *widget,
					GtkWidget **destroyed_widget);
static void ifs_compose_ok_callback (GtkWidget *widget,
				     GtkWidget *window);


/*
 *  Some static variables
 */

IfsDialog *ifsD = 0;
IfsOptionsDialog *ifsOptD = 0;
IfsDesignArea *ifsDesign = 0;

static AffElement **elements = 0;
static gint *element_selected = 0;
static gint element_count = 0;

static UndoItem undo_ring[UNDO_LEVELS];
static gint undo_cur = -1;
static gint undo_num = 0;
static gint undo_start = 0;

/* num_elements = 0, signals not inited */
static IfsComposeVals ifsvals =
{
  0,				/* num_elements */
  50000,			/* iterations */
  4096,				/* max_memory */
  4,				/* subdivide */
  0.75,				/* radius */
  1.0,				/* aspect ratio */
  0.5,				/* center_x */
  0.5,				/* center_y */
};

static
 IfsComposeInterface ifscint =
{
  FALSE,        /* run */
};

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
  };

  static GParamDef *return_vals = NULL;

  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_ifs_compose",
			  "Create an Iterated Function System Fractal",
   "Interactively create an Iterated Function System fractal."
   "Use the window on the upper left to adjust the component"
   "transformations of the fractal. The operation that is performed"
   "is selected by the buttons underneath the window, or from a"
   "menu popped up by the right mouse button. The fractal will be"
   "rendered with a transparent background if the current image has"
   "a transparent background.",
			  "Owen Taylor",
			  "Owen Taylor",
			  "1997",
			  "<Image>/Filters/Render/IfsCompose",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *active_drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  Parasite *parasite = NULL;
  gboolean found_parasite;

  run_mode = param[0].data.d_int32;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals = values;

  /* kill (getpid(), 19); */
  
  /*  Get the active drawable  */
  active_drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Possibly retrieve data; first look for a parasite -
       *  if not found, fall back to global values
       */
      parasite = gimp_drawable_find_parasite (active_drawable->id,
					      IFSCOMPOSE_PARASITE);
      found_parasite = FALSE;
      if (parasite)
	{
	  found_parasite = ifsvals_parse_string (parasite_data (parasite),
						 &ifsvals, &elements);
	  parasite_free (parasite);
	}

      if (!found_parasite)
	{
	  gint length;
	  gchar *data; 
	  
	  length = gimp_get_data_size (IFSCOMPOSE_DATA);
	  if (length)
	    {
	      data = g_new (gchar, length);
	      gimp_get_data(IFSCOMPOSE_DATA, data);

	      ifsvals_parse_string (data, &ifsvals, &elements);
	      g_free (data);
	    }
	}

      /*  First acquire information with a dialog  */
      if (! ifs_compose_dialog (active_drawable))
	return;
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      status = STATUS_CALLING_ERROR;
/*      gimp_get_data ("plug_in_ifs_compose", &mvals); */
      break;

    default:
      break;
    }

  /*  Render the fractal  */
  if ((status == STATUS_SUCCESS) &&
      (gimp_drawable_color (active_drawable->id) || gimp_drawable_is_gray (active_drawable->id)))
    {
      /*  set the tile cache size so that the operation works well  */
      gimp_tile_cache_ntiles (2 * (MAX (active_drawable->width, active_drawable->height) /
				   gimp_tile_width () + 1));

      /*  run the effect  */
      ifs_compose (active_drawable);

      /*  If the run mode is interactive, flush the displays  */
      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      /*  Store data for next invocation - both globally and
       *  as a parasite on this layer
       */
     if (run_mode == RUN_INTERACTIVE)
       {
	 char *str = ifsvals_stringify (&ifsvals, elements);
	 Parasite *parasite;

	 gimp_set_data (IFSCOMPOSE_DATA, str, strlen(str)+1);
	 parasite = parasite_new (IFSCOMPOSE_PARASITE,
				  PARASITE_PERSISTENT | PARASITE_UNDOABLE,
				  strlen(str)+1, str);
	 gimp_drawable_parasite_attach (active_drawable->id, parasite);
	 parasite_free (parasite);

	 g_free (str);
       }
    }
  else if (status == STATUS_SUCCESS)
    {
      status = STATUS_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (active_drawable);
}

static GtkWidget *
ifs_compose_trans_page ()
{
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(vbox), 4);

  table = gtk_table_new(3, 6, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table),6);
  gtk_container_border_width(GTK_CONTAINER(table), 0);
  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
  gtk_widget_show(table);

  /* X */

  label = gtk_label_new("X");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL,
		   4, 0);
  gtk_widget_show(label);

  ifsD->x_pair = value_pair_create(&ifsD->current_vals.x, 0.0, 1.0, FALSE,
				   VALUE_PAIR_DOUBLE);
  gtk_table_attach(GTK_TABLE(table), ifsD->x_pair->entry,1,2,0,1,
		   GTK_FILL,GTK_FILL,4,0);
  gtk_widget_show (ifsD->x_pair->entry);

  /* Y */

  label = gtk_label_new("Y");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  ifsD->y_pair = value_pair_create(&ifsD->current_vals.y, 0.0, 1.0, FALSE,
				   VALUE_PAIR_DOUBLE);
  gtk_table_attach(GTK_TABLE(table), ifsD->y_pair->entry,1,2,1,2,
		   GTK_FILL,GTK_FILL,4,0);
  gtk_widget_show (ifsD->y_pair->entry);

  /* Scale */

  label = gtk_label_new("Scale");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  ifsD->scale_pair = value_pair_create(&ifsD->current_vals.scale, 0.0,1.0, 
				       FALSE, VALUE_PAIR_DOUBLE);
  gtk_table_attach(GTK_TABLE(table), ifsD->scale_pair->entry,3,4,0,1,
		   GTK_FILL,GTK_FILL,4,0);
  gtk_widget_show (ifsD->scale_pair->entry);

  /* Angle */

  label = gtk_label_new("Angle");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 1, 2,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  ifsD->angle_pair = value_pair_create(&ifsD->current_vals.theta,-180,180,
				       FALSE, VALUE_PAIR_DOUBLE);
  gtk_table_attach(GTK_TABLE(table), ifsD->angle_pair->entry,3,4,1,2,
		   GTK_FILL,GTK_FILL,4,0);
  gtk_widget_show (ifsD->angle_pair->entry);

  /* Asym */

  label = gtk_label_new("Asymmetry");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  gtk_table_attach(GTK_TABLE(table), label, 4, 5, 0, 1,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  ifsD->asym_pair = value_pair_create(&ifsD->current_vals.asym,0.10,10.0,
				      FALSE, VALUE_PAIR_DOUBLE);
  gtk_table_attach(GTK_TABLE(table), ifsD->asym_pair->entry,5,6,0,1,
		   GTK_FILL,GTK_FILL,4,0);
  gtk_widget_show (ifsD->asym_pair->entry);

  /* Shear */

  label = gtk_label_new("Shear");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  gtk_table_attach(GTK_TABLE(table), label, 4, 5, 1, 2,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  ifsD->shear_pair = value_pair_create(&ifsD->current_vals.shear,-10.0,10.0,
				       FALSE, VALUE_PAIR_DOUBLE);
  gtk_table_attach(GTK_TABLE(table), ifsD->shear_pair->entry,5,6,1,2,
		   GTK_FILL,GTK_FILL,4,0);
  gtk_widget_show (ifsD->shear_pair->entry);

  /* Flip */

  ifsD->flip_check_button = gtk_check_button_new_with_label("Flip");
  gtk_table_attach(GTK_TABLE(table), ifsD->flip_check_button,0,1,2,3,
		   GTK_FILL,GTK_FILL,4,0);
  gtk_signal_connect(GTK_OBJECT(ifsD->flip_check_button), "toggled",
		     (GtkSignalFunc)flip_check_button_callback,NULL);
  gtk_widget_show(ifsD->flip_check_button);

  return vbox;
}

static GtkWidget *
ifs_compose_color_page ()
{
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;
  GSList *group = NULL;
  IfsColor color;

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(vbox), 4);

  table = gtk_table_new(3, 5, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table),6);
  gtk_container_border_width(GTK_CONTAINER(table), 0);
  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
  gtk_widget_show(table);

  /* Simple color control section */

  ifsD->simple_button = gtk_radio_button_new_with_label (group, "Simple");
  gtk_table_attach(GTK_TABLE(table), ifsD->simple_button, 0, 1, 0, 2,
		   GTK_FILL, GTK_FILL, 4, 0);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (ifsD->simple_button));
  gtk_signal_connect (GTK_OBJECT (ifsD->simple_button), "toggled",
		      (GtkSignalFunc) simple_color_toggled, NULL);
  gtk_widget_show (ifsD->simple_button);

  color.vals[0] = 1.0;
  color.vals[1] = 0.0;
  color.vals[2] = 0.0;
  ifsD->target_cmap = color_map_create("IfsCompose: Target",NULL,
				       &ifsD->current_vals.target_color,TRUE);
  gtk_table_attach(GTK_TABLE(table), ifsD->target_cmap->hbox, 1, 2, 0, 2,
		   GTK_FILL, 0, 4, 0);
  gtk_widget_show(ifsD->target_cmap->hbox);

  label = gtk_label_new("Scale hue by:");
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  ifsD->hue_scale_pair = value_pair_create(&ifsD->current_vals.hue_scale,
				       0.0,1.0, TRUE, VALUE_PAIR_DOUBLE);
  gtk_table_attach(GTK_TABLE(table), ifsD->hue_scale_pair->scale, 3, 4, 0, 1,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show (ifsD->hue_scale_pair->scale);
  gtk_table_attach(GTK_TABLE(table), ifsD->hue_scale_pair->entry, 4, 5, 0, 1,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show (ifsD->hue_scale_pair->entry);

  label = gtk_label_new("Scale value by:");
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 1, 2,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  ifsD->value_scale_pair = value_pair_create(&ifsD->current_vals.value_scale,
				       0.0,1.0, TRUE, VALUE_PAIR_DOUBLE);
  gtk_table_attach(GTK_TABLE(table), ifsD->value_scale_pair->scale,
		   3, 4, 1, 2, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show (ifsD->value_scale_pair->scale);
  gtk_table_attach(GTK_TABLE(table), ifsD->value_scale_pair->entry,
		   4, 5, 1, 2, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show (ifsD->value_scale_pair->entry);

  /* Full color control section */

  ifsD->full_button = gtk_radio_button_new_with_label (group, "Full");
  gtk_table_attach(GTK_TABLE(table), ifsD->full_button, 0, 1, 2, 3,
		   GTK_FILL, GTK_FILL, 4, 0);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (ifsD->full_button));
  gtk_widget_show (ifsD->full_button);

  color.vals[0] = 1.0;
  color.vals[1] = 0.0;
  color.vals[2] = 0.0;
  ifsD->red_cmap = color_map_create("IfsCompose: Red",&color,
				    &ifsD->current_vals.red_color,FALSE);
  gtk_table_attach(GTK_TABLE(table), ifsD->red_cmap->hbox, 1, 2, 2, 3,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(ifsD->red_cmap->hbox);

  color.vals[0] = 0.0;
  color.vals[1] = 1.0;
  color.vals[2] = 0.0;
  ifsD->green_cmap = color_map_create("IfsCompose: Green",&color,
				    &ifsD->current_vals.green_color,FALSE);
  gtk_table_attach(GTK_TABLE(table), ifsD->green_cmap->hbox, 2, 3, 2, 3,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(ifsD->green_cmap->hbox);

  color.vals[0] = 0.0;
  color.vals[1] = 0.0;
  color.vals[2] = 2.0;
  ifsD->blue_cmap = color_map_create("IfsCompose: Blue",&color,
				    &ifsD->current_vals.blue_color,FALSE);
  gtk_table_attach(GTK_TABLE(table), ifsD->blue_cmap->hbox, 3, 4, 2, 3,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(ifsD->blue_cmap->hbox);

  color.vals[0] = 0.0;
  color.vals[1] = 0.0;
  color.vals[2] = 0.0;
  ifsD->black_cmap = color_map_create("IfsCompose: Black",&color,
				    &ifsD->current_vals.black_color,FALSE);
  gtk_table_attach(GTK_TABLE(table), ifsD->black_cmap->hbox, 4, 5, 2, 3,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(ifsD->black_cmap->hbox);

  return vbox;
}

static gint
ifs_compose_dialog (GDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *check_button;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *util_hbox;
  GtkWidget *main_vbox;
  GtkWidget *alignment;
  GtkWidget *aspect_frame;
  GtkWidget *notebook;
  GtkWidget *page;
  guchar *color_cube;
  gchar **argv;
  gint argc;

  gint design_width, design_height;

  design_width = drawable->width;
  design_height = drawable->height;

  if (design_width > design_height)
    {
      if (design_width > DESIGN_AREA_MAX_SIZE)
	{
	  design_height = design_height * DESIGN_AREA_MAX_SIZE / design_width;
	  design_width = DESIGN_AREA_MAX_SIZE;
	}
    }
  else
    {
      if (design_height > DESIGN_AREA_MAX_SIZE)
	{
	  design_width = design_width * DESIGN_AREA_MAX_SIZE / design_height;
	  design_height = DESIGN_AREA_MAX_SIZE;
	}
    }

  ifsD = g_new(IfsDialog,1);
  ifsD->auto_preview = TRUE;
  ifsD->drawable_width = drawable->width;
  ifsD->drawable_height = drawable->height;

  ifsD->selected_orig = NULL;

  ifsD->preview_data = NULL;
  ifsD->preview_iterations = 0;

  ifsD->in_update = 0;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("ifs_compose");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
			      color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "IfsCompose");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) ifs_compose_close_callback,
		      &dlg);

  /*  Action area  */

  button = gtk_button_new_with_label ("New");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) ifs_compose_new_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Delete");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) ifs_compose_delete_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Defaults");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (ifs_compose_defaults_callback),
                      NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (ifs_compose_ok_callback),
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  The main vbox */
  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox, TRUE, TRUE, 0);

  /*  The design area */
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);

  aspect_frame = gtk_aspect_frame_new(NULL,
				      0.5, 0.5,
				      (gdouble)design_width/design_height,0);
  gtk_frame_set_shadow_type (GTK_FRAME (aspect_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), aspect_frame, TRUE, TRUE, 0);

  design_area_create(dlg,design_width,design_height);
  gtk_container_add (GTK_CONTAINER (aspect_frame), ifsDesign->area);

  gtk_widget_show (ifsDesign->area);
  gtk_widget_show (aspect_frame);

  /* the preview */

  aspect_frame = gtk_aspect_frame_new(NULL,
				      0.5, 0.5,
				      (gdouble)design_width/design_height,0);
  gtk_frame_set_shadow_type (GTK_FRAME (aspect_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), aspect_frame, TRUE, TRUE, 0);

  ifsD->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW(ifsD->preview),design_width,design_height);
  gtk_container_add (GTK_CONTAINER (aspect_frame), ifsD->preview);
  gtk_widget_show (ifsD->preview);

  gtk_widget_show (aspect_frame);

  gtk_widget_show (hbox);

  /* Iterations and preview options */

  hbox = gtk_hbox_new(FALSE,1);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 5);

  util_hbox = gtk_hbox_new(FALSE,5);
  gtk_container_add(GTK_CONTAINER(hbox), util_hbox);

  ifsD->move_button = gtk_toggle_button_new_with_label("Move");
  gtk_box_pack_start (GTK_BOX(util_hbox), ifsD->move_button, TRUE, TRUE, 0);
  gtk_widget_show (ifsD->move_button);
  ifsD->move_handler = gtk_signal_connect(GTK_OBJECT(ifsD->move_button),"toggled",
		     (GtkSignalFunc)design_op_callback,
		     (gpointer)((long)OP_TRANSLATE));

  ifsD->rotate_button = gtk_toggle_button_new_with_label("Rotate/Scale");
  gtk_box_pack_start (GTK_BOX(util_hbox), ifsD->rotate_button, TRUE, TRUE, 0);
  gtk_widget_show (ifsD->rotate_button);
  ifsD->rotate_handler = gtk_signal_connect(GTK_OBJECT(ifsD->rotate_button),
					    "toggled",
					    (GtkSignalFunc)design_op_callback,
					    (gpointer)((long)OP_ROTATE));

  ifsD->stretch_button = gtk_toggle_button_new_with_label("Stretch");
  gtk_box_pack_start (GTK_BOX(util_hbox), ifsD->stretch_button, TRUE, TRUE, 0);
  gtk_widget_show (ifsD->stretch_button);
  ifsD->stretch_handler = gtk_signal_connect(GTK_OBJECT(ifsD->stretch_button),
				     "toggled",
				     (GtkSignalFunc)design_op_callback,
				     (gpointer)((long)OP_STRETCH));

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ifsD->move_button),TRUE);

  gtk_widget_show (util_hbox);

  alignment = gtk_alignment_new(1.0,0.5,0.5,0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);

  util_hbox = gtk_hbox_new(FALSE,5);
  gtk_container_add(GTK_CONTAINER(alignment), util_hbox);

  button = gtk_button_new_with_label ("Render Options");
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) ifs_options_dialog,
			     NULL);
  gtk_box_pack_start (GTK_BOX (util_hbox), button,
		      TRUE, TRUE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Preview");
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) ifs_compose_preview_callback,
			     GTK_OBJECT (ifsD->preview));
  gtk_box_pack_start (GTK_BOX (util_hbox), button,
		      TRUE, TRUE, 0);
  gtk_widget_show (button);

  check_button = gtk_check_button_new_with_label ("Auto");
  gtk_box_pack_start (GTK_BOX (util_hbox), check_button,
		      FALSE, FALSE, 0);
  gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON(check_button) ,
				ifsD->auto_preview );
  gtk_signal_connect ( GTK_OBJECT (check_button), "toggled",
		       (GtkSignalFunc) auto_preview_callback,
		       NULL );
  gtk_widget_show (check_button);

  gtk_widget_show (util_hbox);
  gtk_widget_show (alignment);
  gtk_widget_show (hbox);

  /* The current transformation frame */

  ifsD->current_frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (ifsD->current_frame),
			     GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start(GTK_BOX(main_vbox),ifsD->current_frame,FALSE,FALSE,0);

  vbox = gtk_vbox_new(FALSE,0);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER(ifsD->current_frame), vbox);

  /* The notebook */

  notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
  gtk_box_pack_start(GTK_BOX(vbox),notebook,FALSE,FALSE,5);
  gtk_widget_show(notebook);

  page = ifs_compose_trans_page();
  label = gtk_label_new("Spatial Transformation");
  gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
  gtk_widget_show(page);

  page = ifs_compose_color_page();
  label = gtk_label_new("Color Transformation");
  gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
  gtk_widget_show(page);

  /* The probability entry */

  hbox = gtk_hbox_new(FALSE,5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
  label = gtk_label_new ("Relative Probability:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  ifsD->prob_pair = value_pair_create(&ifsD->current_vals.prob,0.0,5.0, TRUE,
				      VALUE_PAIR_DOUBLE);
  gtk_box_pack_start (GTK_BOX (hbox), ifsD->prob_pair->scale, TRUE, TRUE, 0);
  gtk_widget_show (ifsD->prob_pair->scale);
  gtk_box_pack_start (GTK_BOX (hbox), ifsD->prob_pair->entry, FALSE, TRUE, 0);
  gtk_widget_show (ifsD->prob_pair->entry);

  gtk_widget_show(hbox);
  gtk_widget_show(vbox);
  gtk_widget_show(ifsD->current_frame);

  gtk_widget_show (main_vbox);

  if (ifsvals.num_elements == 0)
    {
      ifs_compose_set_defaults();
      if (ifsD->auto_preview)
	ifs_compose_preview_callback(NULL, ifsD->preview);
    }
  else
    {
      int i;
      gdouble ratio = (gdouble)ifsD->drawable_height/ifsD->drawable_width;

      element_selected = g_new(gint, ifsvals.num_elements);
      element_selected[0] = TRUE;
      for (i=1;i<ifsvals.num_elements;i++)
	element_selected[i] = FALSE;

      if (ratio != ifsvals.aspect_ratio)
	{
	  /* Adjust things so that what fit onto the old image, fits
	     onto the new image */
	  Aff2 t1,t2,t3;
	  gdouble x_offset, y_offset;
	  gdouble center_x, center_y;
	  gdouble scale;

	  if (ratio < ifsvals.aspect_ratio)
	    {
	      scale = ratio/ifsvals.aspect_ratio;
	      x_offset = (1-scale)/2;
	      y_offset = 0;
	    }
	  else
	    {
	      scale = 1;
	      x_offset = 0;
	      y_offset = (ratio - ifsvals.aspect_ratio)/2;
	    }
	  aff2_scale(&t1,scale,0);
	  aff2_translate(&t2,x_offset,y_offset);
	  aff2_compose(&t3,&t2,&t1);
	  aff2_invert(&t1,&t3);

	  aff2_apply(&t3,ifsvals.center_x,ifsvals.center_y,&center_x,
		     &center_y);

	  for (i=0;i<ifsvals.num_elements;i++)
	    {
	      aff_element_compute_trans(elements[i],1,ifsvals.aspect_ratio,
					ifsvals.center_x,ifsvals.center_y);
	      aff2_compose(&t2,&elements[i]->trans,&t1);
	      aff2_compose(&elements[i]->trans,&t3,&t2);
	      aff_element_decompose_trans(elements[i],&elements[i]->trans,
					   1,ifsvals.aspect_ratio,
					   center_x,center_y);
	    }
	  ifsvals.center_x = center_x;
	  ifsvals.center_y = center_y;

	  ifsvals.aspect_ratio = ratio;
	}

      for (i=0;i<ifsvals.num_elements;i++)
	aff_element_compute_color_trans(elements[i]);
      /* boundary and spatial transformations will be computed
	 when the design_area gets a ConfigureNotify event */

      set_current_element(0);
      if (ifsD->auto_preview)
	ifs_compose_preview_callback(NULL, ifsD->preview);

      ifsD->selected_orig = g_new(AffElement,ifsvals.num_elements);
    }

  gtk_widget_show (dlg);
  gtk_main ();

  gtk_object_unref (GTK_OBJECT (ifsDesign->op_menu));
  
  if (dlg)
    gtk_widget_destroy (dlg);

  if (ifsOptD)
    gtk_widget_destroy (ifsOptD->dialog);

  gdk_flush ();

  gdk_gc_destroy(ifsDesign->selected_gc);

  g_free(ifsD);

  return ifscint.run;
}

static void
design_area_create(GtkWidget *window,gint design_width,gint design_height)
{
  ifsDesign = g_new(IfsDesignArea,1);

  ifsDesign->op = OP_TRANSLATE;
  ifsDesign->button_state = 0;
  ifsDesign->pixmap = NULL;
  ifsDesign->selected_gc = NULL;

  ifsDesign->area = gtk_drawing_area_new();
  gtk_drawing_area_size (GTK_DRAWING_AREA(ifsDesign->area),design_width,
					  design_height);

  gtk_signal_connect(GTK_OBJECT(ifsDesign->area),"expose_event",
		     (GtkSignalFunc)design_area_expose,NULL);
  gtk_signal_connect(GTK_OBJECT(ifsDesign->area),"button_press_event",
		     (GtkSignalFunc)design_area_button_press,NULL);
  gtk_signal_connect(GTK_OBJECT(ifsDesign->area),"button_release_event",
		     (GtkSignalFunc)design_area_button_release,NULL);
  gtk_signal_connect(GTK_OBJECT(ifsDesign->area),"motion_notify_event",
		     (GtkSignalFunc)design_area_motion,NULL);
  gtk_signal_connect(GTK_OBJECT(ifsDesign->area),"configure_event",
		     (GtkSignalFunc) design_area_configure, NULL);
  gtk_widget_set_events (ifsDesign->area,
			 GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
			 GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
			 GDK_POINTER_MOTION_HINT_MASK);

  design_op_menu_create(window);
}

static void
design_op_menu_create(GtkWidget *window)
{
  GtkWidget *menu_item;
  GtkAccelGroup *accel_group;

  ifsDesign->op_menu = gtk_menu_new();
  gtk_object_ref (GTK_OBJECT (ifsDesign->op_menu));
  gtk_object_sink (GTK_OBJECT (ifsDesign->op_menu));

  accel_group = gtk_accel_group_new();
  gtk_menu_set_accel_group(GTK_MENU(ifsDesign->op_menu), accel_group);
  gtk_window_add_accel_group(GTK_WINDOW(window),accel_group);

  menu_item = gtk_menu_item_new_with_label("Move");
  gtk_menu_append(GTK_MENU(ifsDesign->op_menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     (GtkSignalFunc)design_op_update_callback,
		     (gpointer)((long)OP_TRANSLATE));
  gtk_widget_add_accelerator(menu_item,
			     "activate",
			     accel_group,
			     'M', 0,
			     GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  menu_item = gtk_menu_item_new_with_label("Rotate/Scale");
  gtk_menu_append(GTK_MENU(ifsDesign->op_menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     (GtkSignalFunc)design_op_update_callback,
		     (gpointer)((long)OP_ROTATE));
  gtk_widget_add_accelerator(menu_item,
			     "activate",
			     accel_group,
			     'R', 0,
			     GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  menu_item = gtk_menu_item_new_with_label("Stretch");
  gtk_menu_append(GTK_MENU(ifsDesign->op_menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     (GtkSignalFunc)design_op_update_callback,
		     (gpointer)((long)OP_STRETCH));
  gtk_widget_add_accelerator(menu_item,
			     "activate",
			     accel_group,
			     'S', 0,
			     GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* A separator */
  menu_item = gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(ifsDesign->op_menu),menu_item);
  gtk_widget_show(menu_item);

  menu_item = gtk_menu_item_new_with_label("Select All");
  gtk_menu_append(GTK_MENU(ifsDesign->op_menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     (GtkSignalFunc)design_area_select_all_callback,
		     NULL);
  gtk_widget_add_accelerator(menu_item,
			     "activate",
			     accel_group,
			     'A', GDK_CONTROL_MASK,
			     GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  menu_item = gtk_menu_item_new_with_label("Recompute Center");
  gtk_menu_append(GTK_MENU(ifsDesign->op_menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     (GtkSignalFunc)recompute_center_cb,
		     NULL);
  gtk_widget_add_accelerator(menu_item,
			     "activate",
			     accel_group,
			     'R', GDK_MOD1_MASK,
			     GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  menu_item = gtk_menu_item_new_with_label("Undo");
  gtk_menu_append(GTK_MENU(ifsDesign->op_menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     (GtkSignalFunc)undo,
		     NULL);
  gtk_widget_add_accelerator(menu_item,
			     "activate",
			     accel_group,
			     'Z', GDK_CONTROL_MASK,
			     GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  menu_item = gtk_menu_item_new_with_label("Redo");
  gtk_menu_append(GTK_MENU(ifsDesign->op_menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     (GtkSignalFunc)redo,
		     NULL);
  gtk_widget_add_accelerator(menu_item,
			     "activate",
			     accel_group,
			     'R', GDK_CONTROL_MASK,
			     GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);
}

static void
design_op_menu_popup(gint button, guint32 activate_time)
{
  gtk_menu_popup(GTK_MENU(ifsDesign->op_menu),NULL,NULL,NULL,NULL,button,activate_time);
}

static void
ifs_options_dialog()
{
  GtkWidget *button;
  GtkWidget *table;
  GtkWidget *label;

  if (!ifsOptD)
    {
      ifsOptD = g_new(IfsOptionsDialog,1);

      ifsOptD->dialog = gtk_dialog_new();
      gtk_window_set_title(GTK_WINDOW(ifsOptD->dialog),"IfsCompose Options");
      gtk_window_position(GTK_WINDOW(ifsOptD->dialog), GTK_WIN_POS_MOUSE);
      gtk_signal_connect (GTK_OBJECT(ifsOptD->dialog),
			   "delete_event",
			   GTK_SIGNAL_FUNC (gtk_widget_hide_on_delete),
			   &ifsOptD->dialog);
      gtk_signal_connect(GTK_OBJECT(ifsOptD->dialog), "destroy",
			 (GtkSignalFunc) ifs_options_close_callback,
			 NULL);
      /* Action area */

      button = gtk_button_new_with_label ("Close");
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) ifs_options_close_callback,
			  NULL);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ifsOptD->dialog)->action_area),
			  button, TRUE, TRUE, 0);
      gtk_widget_show (button);

      /* Table of options */

      table = gtk_table_new(4,3,FALSE);
      gtk_container_border_width(GTK_CONTAINER(table),10);
      gtk_table_set_row_spacings(GTK_TABLE(table), 4);
      gtk_table_set_col_spacings(GTK_TABLE(table), 4);
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ifsOptD->dialog)->vbox), table,
			 FALSE,FALSE,0);
      gtk_widget_show(table);

      label = gtk_label_new("Max. Memory:");
      gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
      gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
		       GTK_FILL, GTK_FILL, 4, 0);
      gtk_widget_show(label);

      ifsOptD->memory_pair = value_pair_create(&ifsvals.max_memory,
					       1,1000000,FALSE,
					       VALUE_PAIR_INT);
      gtk_table_attach(GTK_TABLE(table), ifsOptD->memory_pair->entry,
		       1, 2, 0, 1, GTK_FILL, GTK_FILL, 4, 0);
      gtk_widget_show (ifsOptD->memory_pair->entry);

      label = gtk_label_new("Iterations:");
      gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
      gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
		       GTK_FILL, GTK_FILL, 4, 0);
      gtk_widget_show(label);

      ifsOptD->iterations_pair = value_pair_create(&ifsvals.iterations,1,10000000,FALSE,
						VALUE_PAIR_INT);
      gtk_table_attach(GTK_TABLE(table), ifsOptD->iterations_pair->entry,
		       1, 2, 1, 2, GTK_FILL, GTK_FILL, 4, 0);
      gtk_widget_show (ifsOptD->iterations_pair->entry);
      gtk_widget_show (label);

      label = gtk_label_new("Subdivide:");
      gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
      gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
		       GTK_FILL, GTK_FILL, 4, 0);
      gtk_widget_show(label);

      ifsOptD->subdivide_pair = value_pair_create(&ifsvals.subdivide,1,10,
						  FALSE,
						  VALUE_PAIR_INT);
      gtk_table_attach(GTK_TABLE(table), ifsOptD->subdivide_pair->entry,
		       1, 2, 2, 3, GTK_FILL, GTK_FILL, 4, 0);
      gtk_widget_show (ifsOptD->subdivide_pair->entry);

      label = gtk_label_new("Spot Radius:");
      gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
      gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
		       GTK_FILL, GTK_FILL, 4, 0);
      gtk_widget_show(label);

      ifsOptD->radius_pair = value_pair_create(&ifsvals.radius,0,5,
					       TRUE,
					       VALUE_PAIR_DOUBLE);
      gtk_table_attach(GTK_TABLE(table), ifsOptD->radius_pair->scale,
		       1, 2, 3, 4, GTK_FILL, GTK_FILL, 4, 0);
      gtk_widget_show (ifsOptD->radius_pair->scale);
      gtk_table_attach(GTK_TABLE(table), ifsOptD->radius_pair->entry,
		       2, 3, 3, 4, GTK_FILL, GTK_FILL, 4, 0);
      gtk_widget_show (ifsOptD->radius_pair->entry);

      value_pair_update(ifsOptD->iterations_pair);
      value_pair_update(ifsOptD->subdivide_pair);
      value_pair_update(ifsOptD->memory_pair);
      value_pair_update(ifsOptD->radius_pair);

      gtk_widget_show (ifsOptD->dialog);
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (ifsOptD->dialog))
	gtk_widget_show (ifsOptD->dialog);
    }
}

static void
ifs_compose(GDrawable *drawable)
{
  gint i,j;
  GDrawableType type = gimp_drawable_type(drawable->id);
  gchar buffer[128];

  gint width = drawable->width;
  gint height = drawable->height;
  gint num_bands,band_height,band_y,band_no;
  guchar *data;
  guchar *mask = NULL;
  guchar *nhits;
  guchar rc,gc,bc;

  num_bands = ceil((gdouble)(width*height*SQR(ifsvals.subdivide)*5)
		   / (1024 * ifsvals.max_memory));
  band_height = height / num_bands;
  if (band_height > height)
    band_height = height;

  mask = g_new(guchar,width*band_height*SQR(ifsvals.subdivide));
  data = g_new(guchar,width*band_height*SQR(ifsvals.subdivide)*3);
  nhits = g_new(guchar,width*band_height*SQR(ifsvals.subdivide));
  gimp_palette_get_background ( &rc, &gc, &bc );

  band_y = 0;
  for (band_no = 0; band_no < num_bands; band_no++)
    {
      guchar *ptr;
      guchar *maskptr;
      guchar *dest;
      guchar *destrow;
      guchar maskval;
      GPixelRgn dest_rgn;
      gint progress;
      gint max_progress;

      gpointer pr;

      sprintf(buffer,"Rendering IFS (%d/%d)...",band_no+1,num_bands);
      gimp_progress_init(buffer);

      /* render the band to a buffer */
      if (band_y + band_height > height)
	band_height = height - band_y;

      /* we don't need to clear data since we store nhits */
      memset(mask, 0, width*band_height*SQR(ifsvals.subdivide));
      memset(nhits, 0, width*band_height*SQR(ifsvals.subdivide));

      ifs_render(elements, ifsvals.num_elements, width, height, ifsvals.iterations,
		 &ifsvals, band_y, band_height, data, mask, nhits, FALSE);

      /* transfer the image to the drawable */

      sprintf(buffer,"Copying IFS to image (%d/%d)...",band_no+1,num_bands);
      gimp_progress_init(buffer);

      progress = 0;
      max_progress = band_height * width;

      gimp_pixel_rgn_init (&dest_rgn, drawable, 0, band_y,
			   width, band_height, TRUE, TRUE);

      for (pr = gimp_pixel_rgns_register (1, &dest_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
	{
	  destrow = dest_rgn.data;

	  for (j = dest_rgn.y; j < (dest_rgn.y + dest_rgn.h); j++)
	    {
	      dest = destrow;

	      for (i = dest_rgn.x; i < (dest_rgn.x + dest_rgn.w); i++)
		{
		  /* Accumulate a reduced pixel */

		  gint ii,jj;
		  gint rtot=0;
		  gint btot=0;
		  gint gtot=0;
		  gint mtot=0;
		  for (jj=0;jj<ifsvals.subdivide;jj++)
		    {
		      ptr = data + 3 *
			(((j-band_y)*ifsvals.subdivide+jj)*ifsvals.subdivide*width +
			 i*ifsvals.subdivide);

		      maskptr = mask +
			((j-band_y)*ifsvals.subdivide+jj)*ifsvals.subdivide*width +
			i*ifsvals.subdivide;
		      for (ii=0;ii<ifsvals.subdivide;ii++)
			{
			  maskval = *maskptr++;
			  mtot += maskval;
			  rtot += maskval* *ptr++;
			  gtot += maskval* *ptr++;
			  btot += maskval* *ptr++;
			}
		    }
		  if (mtot)
		    {
		      rtot /= mtot;
		      gtot /= mtot;
		      btot /= mtot;
		      mtot /= SQR(ifsvals.subdivide);
		    }
		  /* and store it */
		  switch (type)
		    {
 		    case GRAY_IMAGE:
		      *dest++ = (mtot*(rtot+btot+gtot)+
				 (255-mtot)*(rc+gc+bc))/(3*255);
		      break;
		    case GRAYA_IMAGE:
		      *dest++ = (rtot+btot+gtot)/3;
		      *dest++ = mtot;
		      break;
		    case RGB_IMAGE:
		      *dest++ = (mtot*rtot + (255-mtot)*rc)/255;
		      *dest++ = (mtot*gtot + (255-mtot)*gc)/255;
		      *dest++ = (mtot*btot + (255-mtot)*bc)/255;
		      break;
		    case RGBA_IMAGE:
		      *dest++ = rtot;
		      *dest++ = gtot;
		      *dest++ = btot;
		      *dest++ = mtot;
		      break;
		    case INDEXED_IMAGE:
		    case INDEXEDA_IMAGE:
		      g_error("Indexed images not supported by IfsCompose");
		      break;
		    }
		}
	      destrow += dest_rgn.rowstride;;
	    }
	  progress += dest_rgn.w * dest_rgn.h;
	  gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
	}
      band_y += band_height;
    }

  g_free(mask);
  g_free(data);
  g_free(nhits);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id,0,0,width,height);
}

static void
update_values()
{
  ifsD->in_update = TRUE;

  ifsD->current_vals = elements[ifsD->current_element]->v;
  ifsD->current_vals.theta *= 180/G_PI;

  value_pair_update(ifsD->prob_pair);
  value_pair_update(ifsD->x_pair);
  value_pair_update(ifsD->y_pair);
  value_pair_update(ifsD->scale_pair);
  value_pair_update(ifsD->angle_pair);
  value_pair_update(ifsD->asym_pair);
  value_pair_update(ifsD->shear_pair);
  color_map_update(ifsD->red_cmap);
  color_map_update(ifsD->green_cmap);
  color_map_update(ifsD->blue_cmap);
  color_map_update(ifsD->black_cmap);
  color_map_update(ifsD->target_cmap);
  value_pair_update(ifsD->hue_scale_pair);
  value_pair_update(ifsD->value_scale_pair);
  if (elements[ifsD->current_element]->v.simple_color)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ifsD->simple_button),
				 TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ifsD->full_button),
				 TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ifsD->flip_check_button),
			      elements[ifsD->current_element]->v.flip);

  ifsD->in_update = FALSE;

  simple_color_set_sensitive();
}

static void
set_current_element(gint index)
{
  ifsD->current_element = index;

  gtk_frame_set_label(GTK_FRAME(ifsD->current_frame),elements[index]->name);

  update_values();
}

static gint
design_area_expose(GtkWidget *widget,GdkEventExpose *event)
{
  gint i;
  gint cx,cy;

  if (!ifsDesign->selected_gc)
    {
      ifsDesign->selected_gc = gdk_gc_new(ifsDesign->area->window);
      gdk_gc_set_line_attributes(ifsDesign->selected_gc,2,
				 GDK_LINE_SOLID,GDK_CAP_ROUND,
				 GDK_JOIN_ROUND);
    }

  gdk_draw_rectangle(ifsDesign->pixmap,
		     widget->style->bg_gc[widget->state],
		     TRUE,
		     event->area.x,
		     event->area.y,
		     event->area.width,event->area.height);

  /* draw an indicator for the center */

  cx = ifsvals.center_x * widget->allocation.width;
  cy = ifsvals.center_y * widget->allocation.width;
  gdk_draw_line(ifsDesign->pixmap,
		widget->style->fg_gc[widget->state],
		cx - 10, cy, cx + 10, cy);
  gdk_draw_line(ifsDesign->pixmap,
		widget->style->fg_gc[widget->state],
		cx, cy - 10, cx, cy + 10);

  for (i=0;i<ifsvals.num_elements;i++)
    {
      aff_element_draw(elements[i], element_selected[i],
		       widget->allocation.width,
		       widget->allocation.height,
		       ifsDesign->pixmap,
		       widget->style->fg_gc[widget->state],
		       ifsDesign->selected_gc,
		       ifsDesign->area->style->font);
    }

  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  ifsDesign->pixmap,
		  event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}

static gint
design_area_configure(GtkWidget *widget, GdkEventConfigure *event)
{
  int i;
  gdouble width = widget->allocation.width;
  gdouble height = widget->allocation.height;

  for (i=0;i<ifsvals.num_elements;i++)
    aff_element_compute_trans(elements[i],width,height,
			      ifsvals.center_x, ifsvals.center_y);
  for (i=0;i<ifsvals.num_elements;i++)
    aff_element_compute_boundary(elements[i],width,height,
				 elements, ifsvals.num_elements);

  if (ifsDesign->pixmap)
    {
      gdk_pixmap_unref(ifsDesign->pixmap);
    }
  ifsDesign->pixmap = gdk_pixmap_new(widget->window,
				     widget->allocation.width,
				     widget->allocation.height,
				     gtk_preview_get_visual()->depth);

  return FALSE;
}

static gint
design_area_button_press(GtkWidget *widget, GdkEventButton *event)
{
  gint i;
  gdouble width = ifsDesign->area->allocation.width;
  gint old_current;

  gtk_widget_grab_focus(widget);

  if (event->button != 1 ||
      (ifsDesign->button_state & GDK_BUTTON1_MASK))
    {
      if (event->button == 3)
	  design_op_menu_popup(event->button, event->time);
      return FALSE;
    }

  old_current = ifsD->current_element;
  ifsD->current_element = -1;

  /* Find out where the button press was */
  for (i=0;i<ifsvals.num_elements;i++)
    {
      if ( ipolygon_contains(elements[i]->click_boundary,event->x,event->y) )
	{
	  set_current_element(i);
	  break;
	}
    }

  /* if the user started manipulating an object, set up a new
     position on the undo ring */
  if (ifsD->current_element >= 0)
    undo_begin();

  if (!(event->state & GDK_SHIFT_MASK)
      && ( (ifsD->current_element<0)
	   || !element_selected[ifsD->current_element] ))
    {
      for (i=0;i<ifsvals.num_elements;i++)
	element_selected[i] = 0;
    }

  if (ifsD->current_element >= 0)
    {
      ifsDesign->button_state |= GDK_BUTTON1_MASK;

      element_selected[ifsD->current_element] = TRUE;

      ifsDesign->num_selected = 0;
      ifsDesign->op_xcenter = 0.0;
      ifsDesign->op_ycenter = 0.0;
      for (i=0;i<ifsvals.num_elements;i++)
	{
	  if (element_selected[i])
	    {
	      ifsD->selected_orig[i] = *elements[i];
	      ifsDesign->op_xcenter += elements[i]->v.x;
	      ifsDesign->op_ycenter += elements[i]->v.y;
	      ifsDesign->num_selected++;
	      undo_update(i);
	    }
	}
      ifsDesign->op_xcenter /= ifsDesign->num_selected;
      ifsDesign->op_ycenter /= ifsDesign->num_selected;
      ifsDesign->op_x = (gdouble)event->x/width;
      ifsDesign->op_y = (gdouble)event->y/width;
      ifsDesign->op_center_x = ifsvals.center_x;
      ifsDesign->op_center_y = ifsvals.center_y;
    }
  else
    {
      ifsD->current_element = old_current;
      element_selected[old_current] = TRUE;
    }

  design_area_redraw();

  return FALSE;
}

static gint
design_area_button_release(GtkWidget *widget, GdkEventButton *event)
{
  if (event->button == 1 &&
      (ifsDesign->button_state & GDK_BUTTON1_MASK))
    {
      ifsDesign->button_state &= ~GDK_BUTTON1_MASK;
      if (ifsD->auto_preview)
	ifs_compose_preview_callback(NULL, ifsD->preview);
    }
  return FALSE;
}

static gint
design_area_motion(GtkWidget *widget, GdkEventMotion *event)
{
  gint i;
  gdouble xo;
  gdouble yo;
  gdouble xn;
  gdouble yn;
  gint px,py;
  gdouble width = ifsDesign->area->allocation.width;
  gdouble height = ifsDesign->area->allocation.height;

  Aff2 trans,t1,t2,t3;

  if (!(ifsDesign->button_state & GDK_BUTTON1_MASK)) return FALSE;

  if (event->is_hint)
    {
      gtk_widget_get_pointer(ifsDesign->area, &px, &py);
      event->x = px;
      event->y = py;
    }

  xo = (ifsDesign->op_x - ifsDesign->op_xcenter);
  yo = (ifsDesign->op_y - ifsDesign->op_ycenter);
  xn = (gdouble)event->x/width - ifsDesign->op_xcenter;
  yn = (gdouble)event->y/width - ifsDesign->op_ycenter;

  /*  for (i=0;i<num_elements;i++)
    {
      gtk_widget_draw(widget,&elements[i]->bounding_box);
    } */

  switch (ifsDesign->op)
    {
    case OP_ROTATE:
      {
	aff2_translate(&t1,-ifsDesign->op_xcenter*width,
		       -ifsDesign->op_ycenter*width);
	aff2_scale(&t2,
		   sqrt((SQR(xn)+SQR(yn))/(SQR(xo)+SQR(yo))),
		   0);
	aff2_compose(&t3, &t2, &t1);
	aff2_rotate(&t1, - atan2(yn,xn) + atan2(yo,xo));
	aff2_compose(&t2, &t1, &t3);
	aff2_translate(&t3,ifsDesign->op_xcenter*width,
		       ifsDesign->op_ycenter*width);
	aff2_compose(&trans, &t3, &t2);
	break;
      }
    case OP_STRETCH:
      {
	aff2_translate(&t1,-ifsDesign->op_xcenter*width,
		       -ifsDesign->op_ycenter*width);
	aff2_compute_stretch(&t2, xo, yo, xn, yn);
	aff2_compose(&t3, &t2, &t1);
	aff2_translate(&t1,ifsDesign->op_xcenter*width,
		       ifsDesign->op_ycenter*width);
	aff2_compose(&trans, &t1, &t3);
	break;
      }
    case OP_TRANSLATE:
      {
	aff2_translate(&trans,(xn-xo)*width,(yn-yo)*width);
	break;
      }
    }

  for (i=0;i<ifsvals.num_elements;i++)
    if (element_selected[i])
      {
	if (ifsDesign->num_selected == ifsvals.num_elements)
	  {
	    gdouble cx,cy;
	    aff2_invert(&t1, &trans);
	    aff2_compose(&t2, &trans, &ifsD->selected_orig[i].trans);
	    aff2_compose(&elements[i]->trans, &t2, &t1);

	    cx = ifsDesign->op_center_x * width;
	    cy = ifsDesign->op_center_y * width;
	    aff2_apply(&trans,cx,cy,&cx,&cy);
	    ifsvals.center_x = cx / width;
	    ifsvals.center_y = cy / width;
	  }
	else
	  {
	    aff2_compose(&elements[i]->trans, &trans,
			 &ifsD->selected_orig[i].trans);
	  }
	aff_element_decompose_trans(elements[i],&elements[i]->trans,
				    width,height, ifsvals.center_x,
				    ifsvals.center_y);
	aff_element_compute_trans(elements[i],width,height,
			      ifsvals.center_x, ifsvals.center_y);
      }

  update_values();
  design_area_redraw();

  return FALSE;
}

static void
design_area_redraw(void)
{
  gint i;
  gdouble width = ifsDesign->area->allocation.width;
  gdouble height = ifsDesign->area->allocation.height;

  for (i=0;i<ifsvals.num_elements;i++)
    {
      aff_element_compute_boundary(elements[i],width,height,
				   elements,ifsvals.num_elements);
    }
  gtk_widget_draw(ifsDesign->area,NULL);
}

/* Undo ring functions */
static void
undo_begin(void)
{
  gint i,j;
  gint to_delete;
  gint new_index;

  if (undo_cur == UNDO_LEVELS-1)
    {
      to_delete = 1;
      undo_start = (undo_start + 1)%UNDO_LEVELS;
    }
  else
    {
      undo_cur++;
      to_delete = undo_num - undo_cur;
    }

  undo_num = undo_num - to_delete + 1;
  new_index = (undo_start+undo_cur)%UNDO_LEVELS;

  /* remove any redo elements or the oldest element if necessary */
  for (j=new_index;to_delete>0;j=(j+1)%UNDO_LEVELS,to_delete--)
    {
      for (i=0;i<undo_ring[j].ifsvals.num_elements;i++)
	if (undo_ring[j].elements[i])
	  aff_element_free(undo_ring[j].elements[i]);
      g_free(undo_ring[j].elements);
      g_free(undo_ring[j].element_selected);
    }

  undo_ring[new_index].ifsvals = ifsvals;
  undo_ring[new_index].elements = g_new(AffElement *,ifsvals.num_elements);
  undo_ring[new_index].element_selected = g_new(gint,ifsvals.num_elements);
  undo_ring[new_index].current_element = ifsD->current_element;

  for (i=0;i<ifsvals.num_elements;i++)
    {
      undo_ring[new_index].elements[i] = NULL;
      undo_ring[new_index].element_selected[i] = element_selected[i];
    }
}

static void
undo_update(gint el)
{
  AffElement *elem;
  /* initialize */
  
  elem = NULL;

  if (!undo_ring[(undo_start+undo_cur)%UNDO_LEVELS].elements[el])
    undo_ring[(undo_start+undo_cur)%UNDO_LEVELS].elements[el]
      = elem = g_new(AffElement,1);

  *elem = *elements[el];
  elem->draw_boundary = NULL;
  elem->click_boundary = NULL;
}

static void
undo_exchange(gint el)
{
  gint i;
  AffElement **telements;
  gint *tselected;
  IfsComposeVals tifsvals;
  gint tcurrent;
  gdouble width = ifsDesign->area->allocation.width;
  gdouble height = ifsDesign->area->allocation.height;

  /* swap the arrays and values*/
  telements = elements;
  elements = undo_ring[el].elements;
  undo_ring[el].elements = telements;

  tifsvals = ifsvals;
  ifsvals = undo_ring[el].ifsvals;
  undo_ring[el].ifsvals = tifsvals;

  tselected = element_selected;
  element_selected = undo_ring[el].element_selected;
  undo_ring[el].element_selected = tselected;

  tcurrent = ifsD->current_element;
  ifsD->current_element = undo_ring[el].current_element;
  undo_ring[el].current_element = tcurrent;

  /* now swap back any unchanged elements */
  for (i=0;i<ifsvals.num_elements;i++)
    if (!elements[i])
      {
	elements[i] = undo_ring[el].elements[i];
	undo_ring[el].elements[i] = 0;
      }
    else
      aff_element_compute_trans(elements[i],width,height,
				ifsvals.center_x,ifsvals.center_y);

  set_current_element(ifsD->current_element);

  design_area_redraw();

  if (ifsD->auto_preview)
    ifs_compose_preview_callback(NULL, ifsD->preview);
}

static void
undo(void)
{
  if (undo_cur >= 0)
    {
      undo_exchange((undo_start+undo_cur)%UNDO_LEVELS);
      undo_cur--;
    }
}

static void
redo(void)
{
  if (undo_cur != undo_num - 1)
    {
      undo_cur++;
      undo_exchange((undo_start+undo_cur)%UNDO_LEVELS);
    }
}

static void
design_area_select_all_callback(GtkWidget *w, gpointer data)
{
  gint i;

  for (i=0;i<ifsvals.num_elements;i++)
    element_selected[i] = TRUE;

  design_area_redraw();
}

/*  Interface functions  */

static void
val_changed_update ()
{
  gdouble width = ifsDesign->area->allocation.width;
  gdouble height = ifsDesign->area->allocation.height;
  AffElement *cur = elements[ifsD->current_element];

  if (ifsD->in_update)
    return;

  undo_begin();
  undo_update(ifsD->current_element);

  cur->v = ifsD->current_vals;
  cur->v.theta *= G_PI/180.0;
  aff_element_compute_trans(cur,width,height,
			      ifsvals.center_x, ifsvals.center_y);
  aff_element_compute_color_trans(cur);

  design_area_redraw();

  if (ifsD->auto_preview)
    ifs_compose_preview_callback(NULL, ifsD->preview);
}

/* Pseudo-widget representing a color mapping */

#define COLOR_SAMPLE_SIZE 30

static void
color_map_set_preview_color(GtkWidget *preview, IfsColor *color)
{
  gint i;
  guchar buf[3*COLOR_SAMPLE_SIZE];

  for (i=0;i<COLOR_SAMPLE_SIZE;i++)
    {
      buf[3*i] = (guint)(255.999*color->vals[0]);
      buf[3*i+1] = (guint)(255.999*color->vals[1]);
      buf[3*i+2] = (guint)(255.999*color->vals[2]);
    }
  for (i=0;i<COLOR_SAMPLE_SIZE;i++)
    gtk_preview_draw_row(GTK_PREVIEW(preview),buf,0,i,COLOR_SAMPLE_SIZE);

  gtk_widget_draw (preview, NULL);
}

static ColorMap *
color_map_create(gchar *name, IfsColor *orig_color, IfsColor *data,
		 gint fixed_point)
{
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *button;

  ColorMap *color_map = g_new(ColorMap,1);
  color_map->name = name;
  color_map->color = data;
  color_map->dialog = NULL;
  color_map->fixed_point = fixed_point;

  color_map->in_change_callback = FALSE;

  color_map->hbox = gtk_hbox_new(FALSE,2);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX(color_map->hbox),frame,FALSE,FALSE,0);
  gtk_widget_show(frame);

  color_map->orig_preview = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW(color_map->orig_preview),
		   COLOR_SAMPLE_SIZE,COLOR_SAMPLE_SIZE);
  gtk_container_add (GTK_CONTAINER(frame),color_map->orig_preview);
  gtk_widget_show(color_map->orig_preview);

  if (fixed_point)
    color_map_set_preview_color(color_map->orig_preview,data);
  else
    color_map_set_preview_color(color_map->orig_preview,orig_color);

  label = gtk_label_new("=>");
  gtk_box_pack_start(GTK_BOX(color_map->hbox),label,FALSE,FALSE,0);
  gtk_widget_show(label);

  button = gtk_button_new();
  gtk_box_pack_start(GTK_BOX(color_map->hbox),button,FALSE,FALSE,0);
  gtk_widget_show(button);

  color_map->preview = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW(color_map->preview),
		   COLOR_SAMPLE_SIZE,COLOR_SAMPLE_SIZE);
  gtk_container_add (GTK_CONTAINER(button),color_map->preview);
  gtk_widget_show(color_map->preview);

  color_map_set_preview_color(color_map->preview,data);

  gtk_signal_connect(GTK_OBJECT(button),"clicked",
		     GTK_SIGNAL_FUNC (color_map_clicked_callback),
		     color_map);

  gtk_signal_connect(GTK_OBJECT(frame),"destroy",
		     GTK_SIGNAL_FUNC (color_map_destroy_callback),
		     color_map);

  return color_map;
}

static void
color_map_clicked_callback(GtkWidget *widget,
			   ColorMap *color_map)
{
  GtkColorSelectionDialog *csd;

  if (!color_map->dialog)
    {
      color_map->dialog = gtk_color_selection_dialog_new(color_map->name);
      csd  = GTK_COLOR_SELECTION_DIALOG(color_map->dialog);
      gtk_color_selection_set_update_policy(
					 GTK_COLOR_SELECTION(csd->colorsel),
					 GTK_UPDATE_DELAYED);

      gtk_widget_destroy ( csd->help_button );
      gtk_widget_destroy ( csd->cancel_button );

      gtk_signal_connect_object( GTK_OBJECT(csd->ok_button),
				 "clicked",
				 (GtkSignalFunc)gtk_widget_hide,
				 GTK_OBJECT(color_map->dialog));
      gtk_signal_connect ( GTK_OBJECT(csd->colorsel),
			   "color_changed",
			   (GtkSignalFunc)color_map_color_changed_cb,
			   color_map );

      gtk_signal_connect ( GTK_OBJECT(csd->colorsel),
			   "destroy",
			   GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			   &color_map->dialog );

      /* call here so the old color is set */
      gtk_color_selection_set_color( GTK_COLOR_SELECTION(csd->colorsel),
				     color_map->color->vals);
    }
  else
    csd  = GTK_COLOR_SELECTION_DIALOG(color_map->dialog);

  undo_begin();
  undo_update(ifsD->current_element);

  gtk_color_selection_set_color(GTK_COLOR_SELECTION(csd->colorsel),
				color_map->color->vals);

  gtk_window_position(GTK_WINDOW(color_map->dialog), GTK_WIN_POS_MOUSE);
  gtk_widget_show(color_map->dialog);
}

static void
color_map_destroy_callback(GtkWidget *widget,
			   ColorMap *color_map)
{
  if (color_map->dialog)
    gtk_widget_destroy (color_map->dialog);
}

static void
color_map_color_changed_cb(GtkWidget *widget,
			   ColorMap *color_map)
{
  color_map->in_change_callback = TRUE;

  gtk_color_selection_get_color(
    GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(color_map->dialog)
       ->colorsel),
    color_map->color->vals);

  elements[ifsD->current_element]->v = ifsD->current_vals;
  elements[ifsD->current_element]->v.theta *= G_PI/180.0;
  aff_element_compute_color_trans(elements[ifsD->current_element]);

  update_values();

  if (ifsD->auto_preview)
    ifs_compose_preview_callback(NULL,ifsD->preview);

  color_map->in_change_callback = FALSE;
}

static void
color_map_update(ColorMap *color_map)
{
  color_map_set_preview_color(color_map->preview,color_map->color);
  if (color_map->fixed_point)
    color_map_set_preview_color(color_map->orig_preview,color_map->color);

  if (color_map->dialog && !color_map->in_change_callback)
    {
      gtk_color_selection_set_color(
         GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(color_map->dialog)
	    ->colorsel),
	 color_map->color->vals);
    }
}

static void
simple_color_set_sensitive()
{
  gint sc = elements[ifsD->current_element]->v.simple_color;

  gtk_widget_set_sensitive(ifsD->target_cmap->hbox,sc);
  gtk_widget_set_sensitive(ifsD->hue_scale_pair->scale,sc);
  gtk_widget_set_sensitive(ifsD->hue_scale_pair->entry,sc);
  gtk_widget_set_sensitive(ifsD->value_scale_pair->scale,sc);
  gtk_widget_set_sensitive(ifsD->value_scale_pair->entry,sc);

  gtk_widget_set_sensitive(ifsD->red_cmap->hbox,!sc);
  gtk_widget_set_sensitive(ifsD->green_cmap->hbox,!sc);
  gtk_widget_set_sensitive(ifsD->blue_cmap->hbox,!sc);
  gtk_widget_set_sensitive(ifsD->black_cmap->hbox,!sc);
}

static void
simple_color_toggled(GtkWidget *widget,gpointer data)
{
  AffElement *cur = elements[ifsD->current_element];
  cur->v.simple_color = GTK_TOGGLE_BUTTON(widget)->active;
  ifsD->current_vals.simple_color = cur->v.simple_color;
  if (cur->v.simple_color)
    {
      aff_element_compute_color_trans(cur);
      val_changed_update();
    }
  simple_color_set_sensitive();
}

/* Generic mechanism for scale/entry combination (possibly without
   scale) */

static ValuePair *
value_pair_create (gpointer data, gdouble lower, gdouble upper,
		   gboolean create_scale, ValuePairType type)
{

  ValuePair *value_pair = g_new(ValuePair,1);
  value_pair->data.d = data;
  value_pair->type = type;

  value_pair->adjustment = gtk_adjustment_new (1.0, lower, upper,
					  (upper-lower)/100, (upper-lower)/10,
					  0.0);
  /* We need to sink the adjustment, since we may not create a scale for
   * it, so nobody will assume the initial refcount
   */
  gtk_object_ref (value_pair->adjustment);
  gtk_object_sink (value_pair->adjustment);
  gtk_signal_connect (GTK_OBJECT (value_pair->adjustment), "value_changed",
		      (GtkSignalFunc) value_pair_scale_callback,
		      value_pair);

  if (create_scale)
    {
      value_pair->scale = gtk_hscale_new(GTK_ADJUSTMENT (value_pair->adjustment));
      gtk_widget_ref (value_pair->scale);

      if (type == VALUE_PAIR_INT)
	  gtk_scale_set_digits (GTK_SCALE (value_pair->scale), 0);
      else
	  gtk_scale_set_digits (GTK_SCALE (value_pair->scale), 2);

      gtk_scale_set_draw_value (GTK_SCALE (value_pair->scale), FALSE);
      gtk_signal_connect (GTK_OBJECT (value_pair->scale),
			  "button_release_event",
			  (GtkSignalFunc) value_pair_button_release,
			  NULL);
    }
  else
    value_pair->scale = NULL;

  /* We destroy the value pair when the entry is destroyed, so
   * we don't need to hold a refcount on the entry
   */

  value_pair->entry = gtk_entry_new ();
  gtk_widget_set_usize (value_pair->entry, ENTRY_WIDTH, 0);
  value_pair->entry_handler_id =
    gtk_signal_connect (GTK_OBJECT (value_pair->entry), "changed",
			(GtkSignalFunc) value_pair_entry_callback, value_pair);
  gtk_signal_connect (GTK_OBJECT (value_pair->entry), "destroy",
		      (GtkSignalFunc) value_pair_destroy_callback, value_pair);

  return value_pair;
}

static void
value_pair_update(ValuePair *value_pair)
{
  gchar buffer[32];

  if (value_pair->type == VALUE_PAIR_INT)
    {
      GTK_ADJUSTMENT(value_pair->adjustment)->value = *value_pair->data.i;
      sprintf (buffer, "%d", *value_pair->data.i);
    }
  else
    {
      GTK_ADJUSTMENT(value_pair->adjustment)->value = *value_pair->data.d;
      sprintf (buffer, "%0.2f", *value_pair->data.d);
    }
  gtk_signal_emit_by_name (value_pair->adjustment, "value_changed");

  gtk_signal_handler_block(GTK_OBJECT(value_pair->entry),
			   value_pair->entry_handler_id);
  gtk_entry_set_text (GTK_ENTRY (value_pair->entry), buffer);
  gtk_signal_handler_unblock(GTK_OBJECT(value_pair->entry),
			     value_pair->entry_handler_id);
}

static void
value_pair_button_release (GtkWidget *widget,
		       GdkEventButton *event,
		       gpointer data)
{
  val_changed_update();
}

static void
value_pair_scale_callback (GtkAdjustment *adjustment,
			   ValuePair *value_pair)
{
  gchar buffer[32];
  gint changed = FALSE;

  if (value_pair->type == VALUE_PAIR_DOUBLE)
    {
      if ((gfloat)*value_pair->data.d != adjustment->value)
	{
	  changed = TRUE;
	  *value_pair->data.d = adjustment->value;
	  sprintf (buffer, "%0.2f", adjustment->value);
	}
    }
  else
    {
      if (*value_pair->data.i != (gint)adjustment->value)
	{
	  changed = TRUE;
	  *value_pair->data.i = adjustment->value;
	  sprintf (buffer, "%d", (gint)adjustment->value);
	}
    }
  if (changed)
    {
      gtk_signal_handler_block(GTK_OBJECT(value_pair->entry),
			       value_pair->entry_handler_id);
      gtk_entry_set_text (GTK_ENTRY (value_pair->entry), buffer);
      gtk_signal_handler_unblock(GTK_OBJECT(value_pair->entry),
				 value_pair->entry_handler_id);
    }
}

static void
value_pair_entry_callback (GtkWidget   *widget,
			   ValuePair   *value_pair)
{
  GtkAdjustment *adjustment = GTK_ADJUSTMENT(value_pair->adjustment);
  gdouble new_value;
  gdouble old_value;

  if (value_pair->type == VALUE_PAIR_INT)
    {
      old_value = *value_pair->data.i;
      new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
    }
  else
    {
      old_value = *value_pair->data.d;
      new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));
    }

  if (floor(0.5+old_value*10000) != floor(0.5+new_value*10000))
    {
      if ((new_value >= adjustment->lower) &&
	  (new_value <= adjustment->upper))
	{
	  if (value_pair->type == VALUE_PAIR_INT)
	    *value_pair->data.i = new_value;
	  else
	    *value_pair->data.d = new_value;
	  adjustment->value = new_value;
	  gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");

	  val_changed_update();
	}
    }
}

static void
value_pair_destroy_callback (GtkWidget   *widget,
			     ValuePair   *value_pair)
{
  if (value_pair->scale)
    gtk_object_unref (GTK_OBJECT (value_pair->scale));
  gtk_object_unref (value_pair->adjustment);
}
 
static void
design_op_callback (GtkWidget *widget, gpointer data)
{
  DesignOp op = (DesignOp)data;

  if (op != ifsDesign->op)
    {
      switch (ifsDesign->op)
	{
	case OP_TRANSLATE:
	  gtk_signal_handler_block(GTK_OBJECT(ifsD->move_button),
				   ifsD->move_handler);
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ifsD->move_button),
				      FALSE);
	  gtk_signal_handler_unblock(GTK_OBJECT(ifsD->move_button),
				     ifsD->move_handler);
	  break;
	case OP_ROTATE:
	  gtk_signal_handler_block(GTK_OBJECT(ifsD->rotate_button),
				   ifsD->rotate_handler);
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ifsD->rotate_button),
				      FALSE);
	  gtk_signal_handler_unblock(GTK_OBJECT(ifsD->rotate_button),
				     ifsD->rotate_handler);
	  break;
	case OP_STRETCH:
	  gtk_signal_handler_block(GTK_OBJECT(ifsD->stretch_button),
				   ifsD->stretch_handler);
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ifsD->stretch_button),
				      FALSE);
	  gtk_signal_handler_unblock(GTK_OBJECT(ifsD->stretch_button),
				     ifsD->stretch_handler);
	  break;
	}
      ifsDesign->op = op;
    }
  else
    {
      GTK_TOGGLE_BUTTON(widget)->active = TRUE;
    }
}

static void
design_op_update_callback (GtkWidget *widget, gpointer data)
{
  DesignOp op = (DesignOp)data;

  if (op != ifsDesign->op)
    {
      switch (op)
	{
	case OP_TRANSLATE:
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ifsD->move_button),
				       TRUE);
	  break;
	case OP_ROTATE:
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ifsD->rotate_button),
				      TRUE);
	  break;
	case OP_STRETCH:
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ifsD->stretch_button),
				      TRUE);
	  break;
	}
    }
}

static void
recompute_center_cb(GtkWidget *w, gpointer data)
{
  recompute_center(TRUE);
}

static void
recompute_center(int save_undo)
{
  int i;
  gdouble x,y;
  gdouble center_x = 0.0;
  gdouble center_y = 0.0;

  gdouble width = ifsDesign->area->allocation.width;
  gdouble height = ifsDesign->area->allocation.height;

  if (save_undo)
    undo_begin();

  for (i=0;i<ifsvals.num_elements;i++)
    {
      if (save_undo)
	undo_update(i);
      aff_element_compute_trans(elements[i],1,ifsvals.aspect_ratio,
				ifsvals.center_x, ifsvals.center_y);
      aff2_fixed_point(&elements[i]->trans,&x,&y);
      center_x += x;
      center_y += y;
    }

  ifsvals.center_x = center_x/ifsvals.num_elements;
  ifsvals.center_y = center_y/ifsvals.num_elements;

  for (i=0;i<ifsvals.num_elements;i++)
    {
	aff_element_decompose_trans(elements[i],&elements[i]->trans,
				    1,ifsvals.aspect_ratio,
				    ifsvals.center_x, ifsvals.center_y);
    }

  if (width > 1 && height > 1)
    {
      for (i=0;i<ifsvals.num_elements;i++)
	aff_element_compute_trans(elements[i],width,height,
				  ifsvals.center_x, ifsvals.center_y);
      design_area_redraw();
      update_values();
    }
}

static void
auto_preview_callback (GtkWidget *widget,
		       gpointer data)
{
  if (ifsD->auto_preview)
    {
      ifsD->auto_preview = 0;
    }
  else
    {
      ifsD->auto_preview = 1;
      ifs_compose_preview_callback(NULL, ifsD->preview);
    }
}

static void
flip_check_button_callback (GtkWidget *widget,
		      gpointer data)
{
  ifsD->current_vals.flip = GTK_TOGGLE_BUTTON(widget)->active;
  val_changed_update();
}

static void
ifs_options_close_callback ()
{
  if (ifsOptD)
    gtk_widget_hide(ifsOptD->dialog);
}

static void
ifs_compose_set_defaults ()
{
  gint i;
  IfsColor color;
  guchar rc,bc,gc;

  gimp_palette_get_foreground (&rc,&gc,&bc);

  color.vals[0] = (gdouble)rc/255;
  color.vals[1] = (gdouble)gc/255;
  color.vals[2] = (gdouble)bc/255;

  ifsvals.aspect_ratio = (gdouble)ifsD->drawable_height/ifsD->drawable_width;

  for (i=0;i<ifsvals.num_elements;i++)
    aff_element_free(elements[i]);

  ifsvals.num_elements = 3;
  elements = g_realloc(elements, ifsvals.num_elements*sizeof(AffElement *));
  element_selected = g_realloc(element_selected,
			       ifsvals.num_elements*sizeof(gint));

  elements[0] = aff_element_new(0.3,0.37*ifsvals.aspect_ratio,color,
				element_count++);
  element_selected[0] = FALSE;
  elements[1] = aff_element_new(0.7,0.37*ifsvals.aspect_ratio,color,
				element_count++);
  element_selected[1] = FALSE;
  elements[2] = aff_element_new(0.5,0.7*ifsvals.aspect_ratio,color,
				element_count++);
  element_selected[2] = FALSE;

  ifsvals.center_x = 0.5;
  ifsvals.center_y = 0.5*ifsvals.aspect_ratio;
  ifsvals.iterations = ifsD->drawable_height*ifsD->drawable_width;

  ifsvals.subdivide = 3;
  ifsvals.max_memory = 4096;

  if (ifsOptD)
    {
      value_pair_update(ifsOptD->iterations_pair);
      value_pair_update(ifsOptD->subdivide_pair);
      value_pair_update(ifsOptD->radius_pair);
      value_pair_update(ifsOptD->memory_pair);
    }

  ifsvals.radius = 0.7;

  set_current_element(0);
  element_selected[0] = TRUE;
  recompute_center(FALSE);

  if (ifsD->selected_orig)
    g_free(ifsD->selected_orig);

  ifsD->selected_orig = g_new(AffElement,ifsvals.num_elements);
}

static void
ifs_compose_defaults_callback (GtkWidget *widget,
			       gpointer data)
{
  gint i;
  gdouble width = ifsDesign->area->allocation.width;
  gdouble height = ifsDesign->area->allocation.height;

  undo_begin();
  for (i=0;i<ifsvals.num_elements;i++)
    undo_update(i);

  ifs_compose_set_defaults();

  if (ifsD->auto_preview)
    ifs_compose_preview_callback(NULL, ifsD->preview);

  for (i=0;i<ifsvals.num_elements;i++)
    aff_element_compute_trans(elements[i],width,height,
			      ifsvals.center_x, ifsvals.center_y);

  design_area_redraw();
}

static void
ifs_compose_new_callback (GtkWidget *widget,
			  gpointer   data)
{
  IfsColor color;
  guchar rc,bc,gc;
  gint i;
  gdouble width = ifsDesign->area->allocation.width;
  gdouble height = ifsDesign->area->allocation.height;
  AffElement *elem;

  undo_begin();

  gimp_palette_get_foreground (&rc,&gc,&bc);

  color.vals[0] = (gdouble)rc/255;
  color.vals[1] = (gdouble)gc/255;
  color.vals[2] = (gdouble)bc/255;

  elem = aff_element_new(0.5, 0.5*height/width,color,
			 element_count++);

  ifsvals.num_elements++;
  elements = g_realloc(elements, ifsvals.num_elements*sizeof(AffElement *));
  element_selected = g_realloc(element_selected,
			       ifsvals.num_elements*sizeof(gint));

  for (i=0;i<ifsvals.num_elements-1;i++)
    element_selected[i] = FALSE;
  element_selected[ifsvals.num_elements-1] = TRUE;

  elements[ifsvals.num_elements-1] = elem;
  set_current_element(ifsvals.num_elements-1);

  ifsD->selected_orig = g_realloc(ifsD->selected_orig,
				  ifsvals.num_elements*sizeof(AffElement));
  aff_element_compute_trans(elem,width,height,
			      ifsvals.center_x, ifsvals.center_y);

  design_area_redraw();

  if (ifsD->auto_preview)
    ifs_compose_preview_callback(NULL, ifsD->preview);
}

static void
ifs_compose_delete_callback (GtkWidget *widget,
			     gpointer   data)
{
  gint i;
  gint new_current;
  if (ifsvals.num_elements <= 2)
    return;

  undo_begin();
  undo_update(ifsD->current_element);

  aff_element_free(elements[ifsD->current_element]);

  if (ifsD->current_element < ifsvals.num_elements-1)
    {
      undo_update(ifsvals.num_elements-1);
      elements[ifsD->current_element] = elements[ifsvals.num_elements-1];
      new_current = ifsD->current_element;
    }
  else
    new_current = ifsvals.num_elements-2;

  ifsvals.num_elements--;

  for (i=0;i<ifsvals.num_elements;i++)
    if (element_selected[i])
      {
	new_current = i;
	break;
      }

  element_selected[new_current] = TRUE;
  set_current_element(new_current);

  design_area_redraw();

  if (ifsD->auto_preview)
    ifs_compose_preview_callback(NULL, ifsD->preview);
}

static void
ifs_compose_close_callback (GtkWidget *widget,
			    GtkWidget **destroyed_widget)
{
  *destroyed_widget = NULL;
  gtk_main_quit ();
}

static gint
preview_idle_render()
{
  gint i;
  gint width = GTK_WIDGET(ifsD->preview)->requisition.width;
  gint height = GTK_WIDGET(ifsD->preview)->requisition.height;

  gint iterations = PREVIEW_RENDER_CHUNK;
  if (iterations > ifsD->preview_iterations)
    iterations = ifsD->preview_iterations;

  for (i=0;i<ifsvals.num_elements;i++)
    aff_element_compute_trans(elements[i], width, height,
			      ifsvals.center_x, ifsvals.center_y);

  ifs_render(elements,ifsvals.num_elements,width,height,
	     iterations,&ifsvals,0,height,
	     ifsD->preview_data,NULL,NULL,TRUE);

  for (i=0;i<ifsvals.num_elements;i++)
    aff_element_compute_trans(elements[i],
			      ifsDesign->area->allocation.width,
			      ifsDesign->area->allocation.height,
			      ifsvals.center_x, ifsvals.center_y);

  ifsD->preview_iterations -= iterations;

  for (i = 0; i < height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (ifsD->preview),
			  ifsD->preview_data + i * width * 3,
			  0, i, width);
  gtk_widget_draw (ifsD->preview, NULL);

  return (ifsD->preview_iterations != 0);
}

static void
ifs_compose_preview_callback (GtkWidget *widget,
			      GtkWidget *preview)
{
  /* Expansion isn't really supported for previews */
  gint i;
  gint width = GTK_WIDGET(ifsD->preview)->requisition.width;
  gint height = GTK_WIDGET(ifsD->preview)->requisition.height;
  guchar rc,gc,bc;
  guchar *ptr;

  if (!ifsD->preview_data)
    ifsD->preview_data = g_new(guchar,3*width*height);

  gimp_palette_get_background ( &rc, &gc, &bc );

  ptr = ifsD->preview_data;
  for (i=0;i<width*height;i++)
    {
      *ptr++ = rc;
      *ptr++ = gc;
      *ptr++ = bc;
    }

  if (ifsD->preview_iterations == 0)
    gtk_idle_add ((GtkFunction)preview_idle_render, NULL);

  ifsD->preview_iterations = ifsvals.iterations*((gdouble)width*height/
				 (ifsD->drawable_width*ifsD->drawable_height));
}

static void
ifs_compose_ok_callback (GtkWidget *widget,
			 GtkWidget *window)
{
  ifscint.run = TRUE;

  gtk_widget_destroy (window);
}
