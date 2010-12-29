/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * IfsCompose is a interface for creating IFS fractals by
 * direct manipulation.
 * Copyright (C) 1997 Owen Taylor
 *
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* TODO
 * ----
 *
 * 1. Run in non-interactive mode (need to figure out useful way for a
 *    script to give the 19N parameters for an image).  Perhaps just
 *    support saving parameters to a file, script passes file name.
 * 2. Figure out if we need multiple phases for supersampled brushes.
 */

#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "ifs-compose.h"

#include "libgimp/stdplugins-intl.h"


#define RESPONSE_RESET           1
#define RESPONSE_OPEN            2
#define RESPONSE_SAVE            3

#define DESIGN_AREA_MAX_SIZE   300

#define PREVIEW_RENDER_CHUNK 10000

#define UNDO_LEVELS             24

#define PLUG_IN_PARASITE "ifscompose-parasite"
#define PLUG_IN_PROC     "plug-in-ifscompose"
#define PLUG_IN_BINARY   "ifs-compose"
#define PLUG_IN_ROLE     "gimp-ifs-compose"

typedef enum
{
  OP_TRANSLATE,
  OP_ROTATE,        /* or scale */
  OP_STRETCH
} DesignOp;

typedef enum
{
  VALUE_PAIR_INT,
  VALUE_PAIR_DOUBLE
} ValuePairType;

typedef struct
{
  GtkAdjustment *adjustment;
  GtkWidget     *scale;
  GtkWidget     *spin;

  ValuePairType type;
  guint         timeout_id;

  union
  {
    gdouble *d;
    gint    *i;
  } data;
} ValuePair;

typedef struct
{
  IfsComposeVals   ifsvals;
  AffElement     **elements;
  gboolean        *element_selected;
  gint             current_element;
} UndoItem;

typedef struct
{
  GimpRGB   *color;
  GtkWidget *hbox;
  GtkWidget *orig_preview;
  GtkWidget *button;
  gboolean   fixed_point;
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
  GtkWidget    *area;
  GtkUIManager *ui_manager;
  GdkPixmap    *pixmap;

  DesignOp      op;
  gdouble       op_x;
  gdouble       op_y;
  gdouble       op_xcenter;
  gdouble       op_ycenter;
  gdouble       op_center_x;
  gdouble       op_center_y;
  guint         button_state;
  gint          num_selected;
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

  GtkWidget *preview;
  guchar    *preview_data;
  gint       preview_iterations;

  gint       drawable_width;
  gint       drawable_height;
  gint       preview_width;
  gint       preview_height;

  AffElement     *selected_orig;
  gint            current_element;
  AffElementVals  current_vals;

  gboolean   in_update;         /* true if we're currently in
                                   update_values() - don't do anything
                                   on updates */
} IfsDialog;

typedef struct
{
  gboolean   run;
} IfsComposeInterface;

/* Declare local functions.
 */
static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

/*  user interface functions  */
static gint           ifs_compose_dialog          (gint32        drawable_id);
static void           ifs_options_dialog          (GtkWidget    *parent);
static GtkWidget    * ifs_compose_trans_page      (void);
static GtkWidget    * ifs_compose_color_page      (void);
static GtkUIManager * design_op_menu_create       (GtkWidget   *window);
static void           design_op_actions_update    (void);
static void           design_area_create          (GtkWidget   *window,
                                                   gint         design_width,
                                                   gint         design_height);

/* functions for drawing design window */
static void update_values                   (void);
static void set_current_element             (gint               index);
static void design_area_realize             (GtkWidget         *widget);
static gint design_area_expose              (GtkWidget         *widget,
                                             GdkEventExpose    *event);
static gint design_area_button_press        (GtkWidget         *widget,
                                             GdkEventButton    *event);
static gint design_area_button_release      (GtkWidget         *widget,
                                             GdkEventButton    *event);
static void design_area_select_all_callback (GtkWidget         *widget,
                                             gpointer           data);
static gint design_area_configure           (GtkWidget         *widget,
                                             GdkEventConfigure *event);
static gint design_area_motion              (GtkWidget         *widget,
                                             GdkEventMotion    *event);
static void design_area_redraw              (void);

/* Undo ring functions */
static void undo_begin    (void);
static void undo_update   (gint element);
static void undo_exchange (gint el);
static void undo          (void);
static void redo          (void);

static void recompute_center              (gboolean   save_undo);
static void recompute_center_cb           (GtkWidget *widget,
                                           gpointer   data);

static void ifs_compose                   (gint32     drawable_id);

static ColorMap *color_map_create         (const gchar  *name,
                                           GimpRGB      *orig_color,
                                           GimpRGB      *data,
                                           gboolean      fixed_point);
static void color_map_color_changed_cb    (GtkWidget    *widget,
                                           ColorMap     *color_map);
static void color_map_update              (ColorMap     *color_map);

/* interface functions */
static void simple_color_toggled          (GtkWidget *widget, gpointer data);
static void simple_color_set_sensitive    (void);
static void val_changed_update            (void);
static ValuePair *value_pair_create       (gpointer   data,
                                           gdouble    lower,
                                           gdouble    upper,
                                           gboolean   create_scale,
                                           ValuePairType type);
static void value_pair_update             (ValuePair *value_pair);
static void value_pair_scale_callback     (GtkAdjustment *adjustment,
                                           ValuePair *value_pair);

static void design_op_update_callback     (GtkRadioAction *action,
                                           GtkRadioAction *current,
                                           gpointer        data);
static void flip_check_button_callback    (GtkWidget *widget, gpointer data);
static gint preview_idle_render           (gpointer   data);

static void ifs_compose_preview           (void);
static void ifs_compose_set_defaults      (void);
static void ifs_compose_new_callback      (GtkAction *action,
                                           gpointer   data);
static void ifs_compose_delete_callback   (GtkAction *action,
                                           gpointer   data);
static void ifs_compose_options_callback  (GtkAction *action,
                                           gpointer   data);
static void ifs_compose_load              (GtkWidget *parent);
static void ifs_compose_save              (GtkWidget *parent);
static void ifs_compose_response          (GtkWidget *widget,
                                           gint       response_id,
                                           gpointer   data);

/*
 *  Some static variables
 */

static IfsDialog        *ifsD       = NULL;
static IfsOptionsDialog *ifsOptD    = NULL;
static IfsDesignArea    *ifsDesign  = NULL;


static AffElement **elements = NULL;
static gint        *element_selected = NULL;
/* labels are generated by printing this int */
static gint         count_for_naming = 0;

static UndoItem     undo_ring[UNDO_LEVELS];
static gint         undo_cur = -1;
static gint         undo_num = 0;
static gint         undo_start = 0;


/* num_elements = 0, signals not inited */
static IfsComposeVals ifsvals =
{
  0,       /* num_elements */
  50000,   /* iterations   */
  4096,    /* max_memory   */
  4,       /* subdivide    */
  0.75,    /* radius       */
  1.0,     /* aspect ratio */
  0.5,     /* center_x     */
  0.5,     /* center_y     */
};

static IfsComposeInterface ifscint =
{
  FALSE,   /* run          */
};

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
  };

  static const GimpParamDef *return_vals = NULL;
  static int nreturn_vals = 0;

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Create an Iterated Function System (IFS) fractal"),
                          "Interactively create an Iterated Function System "
                          "fractal. Use the window on the upper left to adjust "
                          "the component transformations of the fractal. The "
                          "operation that is performed is selected by the "
                          "buttons underneath the window, or from a menu "
                          "popped up by the right mouse button. The fractal "
                          "will be rendered with a transparent background if "
                          "the current image has an alpha channel.",
                          "Owen Taylor",
                          "Owen Taylor",
                          "1997",
                          N_("_IFS Fractal..."),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), nreturn_vals,
                          args, return_vals);

  gimp_plugin_menu_register (PLUG_IN_PROC,
                             "<Image>/Filters/Render/Fractals");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status   = GIMP_PDB_SUCCESS;
  GimpParasite      *parasite = NULL;
  gint32             image_id;
  gint32             drawable_id;
  gboolean           found_parasite = FALSE;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  image_id    = param[1].data.d_image;
  drawable_id = param[2].data.d_drawable;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data; first look for a parasite -
       *  if not found, fall back to global values
       */
      parasite = gimp_item_get_parasite (drawable_id,
                                         PLUG_IN_PARASITE);
      if (parasite)
        {
          found_parasite = ifsvals_parse_string (gimp_parasite_data (parasite),
                                                 &ifsvals, &elements);
          gimp_parasite_free (parasite);
        }

      if (!found_parasite)
        {
          gint length = gimp_get_data_size (PLUG_IN_PROC);

          if (length > 0)
            {
              gchar *data = g_new (gchar, length);

              gimp_get_data (PLUG_IN_PROC, data);
              ifsvals_parse_string (data, &ifsvals, &elements);
              g_free (data);
            }
        }

      /* after ifsvals_parse_string, need to set up naming */
      count_for_naming = ifsvals.num_elements;

      /*  First acquire information with a dialog  */
      if (! ifs_compose_dialog (drawable_id))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      {
        gint length = gimp_get_data_size (PLUG_IN_PROC);

        if (length > 0)
          {
            gchar *data = g_new (gchar, length);

            gimp_get_data (PLUG_IN_PROC, data);
            ifsvals_parse_string (data, &ifsvals, &elements);
            g_free (data);
          }
        else
          {
            ifs_compose_set_defaults ();
          }
      }
      break;

    default:
      break;
    }

  /*  Render the fractal  */
  if (status == GIMP_PDB_SUCCESS)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          gchar        *str;
          GimpParasite *parasite;

          gimp_image_undo_group_start (image_id);

          /*  run the effect  */
          ifs_compose (drawable_id);

          /*  Store data for next invocation - both globally and
           *  as a parasite on this layer
           */
          str = ifsvals_stringify (&ifsvals, elements);

          gimp_set_data (PLUG_IN_PROC, str, strlen (str) + 1);

          parasite = gimp_parasite_new (PLUG_IN_PARASITE,
                                        GIMP_PARASITE_PERSISTENT |
                                        GIMP_PARASITE_UNDOABLE,
                                        strlen (str) + 1, str);
          gimp_item_attach_parasite (drawable_id, parasite);
          gimp_parasite_free (parasite);

          g_free (str);

          gimp_image_undo_group_end (image_id);

          gimp_displays_flush ();
        }
      else
        {
          /*  run the effect  */
          ifs_compose (drawable_id);
        }
    }

  values[0].data.d_status = status;
}

static GtkWidget *
ifs_compose_trans_page (void)
{
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

  table = gtk_table_new (3, 6, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_table_set_col_spacing (GTK_TABLE (table), 2, 6);
  gtk_table_set_col_spacing (GTK_TABLE (table), 4, 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 12);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* X */

  label = gtk_label_new (_("X:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->x_pair = value_pair_create (&ifsD->current_vals.x, 0.0, 1.0, FALSE,
                                    VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->x_pair->spin, 1, 2, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->x_pair->spin);

  /* Y */

  label = gtk_label_new (_("Y:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->y_pair = value_pair_create (&ifsD->current_vals.y, 0.0, 1.0, FALSE,
                                    VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->y_pair->spin, 1, 2, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->y_pair->spin);

  /* Scale */

  label = gtk_label_new (_("Scale:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->scale_pair = value_pair_create (&ifsD->current_vals.scale, 0.0, 1.0,
                                        FALSE, VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->scale_pair->spin, 3, 4, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->scale_pair->spin);

  /* Angle */

  label = gtk_label_new (_("Angle:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->angle_pair = value_pair_create (&ifsD->current_vals.theta, -180, 180,
                                        FALSE, VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->angle_pair->spin, 3, 4, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->angle_pair->spin);

  /* Asym */

  label = gtk_label_new (_("Asymmetry:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 4, 5, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->asym_pair = value_pair_create (&ifsD->current_vals.asym, 0.10, 10.0,
                                       FALSE, VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->asym_pair->spin, 5, 6, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->asym_pair->spin);

  /* Shear */

  label = gtk_label_new (_("Shear:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 4, 5, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->shear_pair = value_pair_create (&ifsD->current_vals.shear, -10.0, 10.0,
                                        FALSE, VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->shear_pair->spin, 5, 6, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->shear_pair->spin);

  /* Flip */

  ifsD->flip_check_button = gtk_check_button_new_with_label (_("Flip"));
  gtk_table_attach (GTK_TABLE (table), ifsD->flip_check_button, 0, 6, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect (ifsD->flip_check_button, "toggled",
                    G_CALLBACK (flip_check_button_callback),
                    NULL);
  gtk_widget_show (ifsD->flip_check_button);

  return vbox;
}

static GtkWidget *
ifs_compose_color_page (void)
{
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;
  GSList    *group = NULL;
  GimpRGB    color;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

  table = gtk_table_new (3, 5, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Simple color control section */

  ifsD->simple_button = gtk_radio_button_new_with_label (group, _("Simple"));
  gtk_table_attach (GTK_TABLE (table), ifsD->simple_button, 0, 1, 0, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (ifsD->simple_button));
  g_signal_connect (ifsD->simple_button, "toggled",
                    G_CALLBACK (simple_color_toggled),
                    NULL);
  gtk_widget_show (ifsD->simple_button);

  ifsD->target_cmap = color_map_create (_("IFS Fractal: Target"), NULL,
                                        &ifsD->current_vals.target_color, TRUE);
  gtk_table_attach (GTK_TABLE (table), ifsD->target_cmap->hbox, 1, 2, 0, 2,
                    GTK_FILL, 0, 0, 0);
  gtk_widget_show (ifsD->target_cmap->hbox);

  label = gtk_label_new (_("Scale hue by:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->hue_scale_pair = value_pair_create (&ifsD->current_vals.hue_scale,
                                            0.0, 1.0, TRUE, VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->hue_scale_pair->scale, 3, 4, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->hue_scale_pair->scale);
  gtk_table_attach (GTK_TABLE (table), ifsD->hue_scale_pair->spin, 4, 5, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->hue_scale_pair->spin);

  label = gtk_label_new (_("Scale value by:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->value_scale_pair = value_pair_create (&ifsD->current_vals.value_scale,
                                              0.0, 1.0, TRUE, VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->value_scale_pair->scale,
                    3, 4, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->value_scale_pair->scale);
  gtk_table_attach (GTK_TABLE (table), ifsD->value_scale_pair->spin,
                    4, 5, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->value_scale_pair->spin);

  /* Full color control section */

  ifsD->full_button = gtk_radio_button_new_with_label (group, _("Full"));
  gtk_table_attach (GTK_TABLE (table), ifsD->full_button, 0, 1, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (ifsD->full_button));
  gtk_widget_show (ifsD->full_button);

  gimp_rgb_parse_name (&color, "red", -1);
  gimp_rgb_set_alpha (&color, 1.0);
  ifsD->red_cmap = color_map_create (_("IFS Fractal: Red"), &color,
                                     &ifsD->current_vals.red_color, FALSE);
  gtk_table_attach (GTK_TABLE (table), ifsD->red_cmap->hbox, 1, 2, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->red_cmap->hbox);

  gimp_rgb_parse_name (&color, "green", -1);
  gimp_rgb_set_alpha (&color, 1.0);
  ifsD->green_cmap = color_map_create (_("IFS Fractal: Green"), &color,
                                       &ifsD->current_vals.green_color, FALSE);
  gtk_table_attach (GTK_TABLE (table), ifsD->green_cmap->hbox, 2, 3, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->green_cmap->hbox);

  gimp_rgb_parse_name (&color, "blue", -1);
  gimp_rgb_set_alpha (&color, 1.0);
  ifsD->blue_cmap = color_map_create (_("IFS Fractal: Blue"), &color,
                                      &ifsD->current_vals.blue_color, FALSE);
  gtk_table_attach (GTK_TABLE (table), ifsD->blue_cmap->hbox, 3, 4, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->blue_cmap->hbox);

  gimp_rgb_parse_name (&color, "black", -1);
  gimp_rgb_set_alpha (&color, 1.0);
  ifsD->black_cmap = color_map_create (_("IFS Fractal: Black"), &color,
                                       &ifsD->current_vals.black_color, FALSE);
  gtk_table_attach (GTK_TABLE (table), ifsD->black_cmap->hbox, 4, 5, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->black_cmap->hbox);

  return vbox;
}

static gint
ifs_compose_dialog (gint32 drawable_id)
{
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *main_vbox;
  GtkWidget *toolbar;
  GtkWidget *aspect_frame;
  GtkWidget *notebook;
  GtkWidget *page;
  gint       design_width  = gimp_drawable_width (drawable_id);
  gint       design_height = gimp_drawable_height (drawable_id);

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

  ifsD = g_new0 (IfsDialog, 1);

  ifsD->drawable_width  = gimp_drawable_width (drawable_id);
  ifsD->drawable_height = gimp_drawable_height (drawable_id);
  ifsD->preview_width   = design_width;
  ifsD->preview_height  = design_height;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("IFS Fractal"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Open"),   RESPONSE_OPEN,
                            _("_Save"),   RESPONSE_SAVE,
                            _("_Reset"),  RESPONSE_RESET,
                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_OPEN,
                                           RESPONSE_SAVE,
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer) &dialog);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (ifs_compose_response),
                    NULL);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  design_area_create (dialog, design_width, design_height);

  toolbar = gtk_ui_manager_get_widget (ifsDesign->ui_manager,
                                       "/ifs-compose-toolbar");
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  /*  The main vbox */
  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);

  /*  The design area */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  aspect_frame = gtk_aspect_frame_new (NULL,
                                       0.5, 0.5,
                                       (gdouble) design_width / design_height,
                                       0);
  gtk_frame_set_shadow_type (GTK_FRAME (aspect_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), aspect_frame, TRUE, TRUE, 0);
  gtk_widget_show (aspect_frame);

  gtk_container_add (GTK_CONTAINER (aspect_frame), ifsDesign->area);
  gtk_widget_show (ifsDesign->area);

  /*  The Preview  */

  aspect_frame = gtk_aspect_frame_new (NULL,
                                       0.5, 0.5,
                                       (gdouble) design_width / design_height,
                                       0);
  gtk_frame_set_shadow_type (GTK_FRAME (aspect_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), aspect_frame, TRUE, TRUE, 0);

  ifsD->preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (ifsD->preview,
                               ifsD->preview_width,
                               ifsD->preview_height);
  gtk_container_add (GTK_CONTAINER (aspect_frame), ifsD->preview);
  gtk_widget_show (ifsD->preview);

  gtk_widget_show (aspect_frame);

  gtk_widget_show (hbox);

  /* The current transformation frame */

  ifsD->current_frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), ifsD->current_frame,
                      FALSE, FALSE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (ifsD->current_frame), vbox);

  /* The notebook */

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  page = ifs_compose_trans_page ();
  label = gtk_label_new (_("Spatial Transformation"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.5);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);

  page = ifs_compose_color_page ();
  label = gtk_label_new (_("Color Transformation"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.5);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);

  /* The probability entry */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Relative probability:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  ifsD->prob_pair = value_pair_create (&ifsD->current_vals.prob, 0.0, 5.0, TRUE,
                                       VALUE_PAIR_DOUBLE);
  gtk_box_pack_start (GTK_BOX (hbox), ifsD->prob_pair->scale, TRUE, TRUE, 0);
  gtk_widget_show (ifsD->prob_pair->scale);
  gtk_box_pack_start (GTK_BOX (hbox), ifsD->prob_pair->spin, FALSE, TRUE, 0);
  gtk_widget_show (ifsD->prob_pair->spin);

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (ifsD->current_frame);

  gtk_widget_show (main_vbox);

  if (ifsvals.num_elements == 0)
    {
      ifs_compose_set_defaults ();
    }
  else
    {
      gint i;
      gdouble ratio = (gdouble) ifsD->drawable_height / ifsD->drawable_width;

      element_selected = g_new (gint, ifsvals.num_elements);
      element_selected[0] = TRUE;
      for (i = 1; i < ifsvals.num_elements; i++)
        element_selected[i] = FALSE;

      if (ratio != ifsvals.aspect_ratio)
        {
          /* Adjust things so that what fit onto the old image, fits
             onto the new image */
          Aff2 t1, t2, t3;
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
          aff2_scale (&t1, scale, 0);
          aff2_translate (&t2, x_offset, y_offset);
          aff2_compose (&t3, &t2, &t1);
          aff2_invert (&t1, &t3);

          aff2_apply (&t3, ifsvals.center_x, ifsvals.center_y, &center_x,
                      &center_y);

          for (i = 0; i < ifsvals.num_elements; i++)
            {
              aff_element_compute_trans (elements[i],1, ifsvals.aspect_ratio,
                                         ifsvals.center_x, ifsvals.center_y);
              aff2_compose (&t2, &elements[i]->trans, &t1);
              aff2_compose (&elements[i]->trans, &t3, &t2);
              aff_element_decompose_trans (elements[i],&elements[i]->trans,
                                           1, ifsvals.aspect_ratio,
                                           center_x, center_y);
            }
          ifsvals.center_x = center_x;
          ifsvals.center_y = center_y;

          ifsvals.aspect_ratio = ratio;
        }

      for (i = 0; i < ifsvals.num_elements; i++)
        aff_element_compute_color_trans (elements[i]);
      /* boundary and spatial transformations will be computed
         when the design_area gets a ConfigureNotify event */

      set_current_element (0);

      ifsD->selected_orig = g_new (AffElement, ifsvals.num_elements);
    }

  gtk_widget_show (dialog);

  ifs_compose_preview ();

  gtk_main ();

  g_object_unref (ifsDesign->ui_manager);

  if (dialog)
    gtk_widget_destroy (dialog);

  if (ifsOptD)
    gtk_widget_destroy (ifsOptD->dialog);

  gdk_flush ();

  g_free (ifsD);

  return ifscint.run;
}

static void
design_area_create (GtkWidget *window,
                    gint       design_width,
                    gint       design_height)
{
  ifsDesign = g_new0 (IfsDesignArea, 1);

  ifsDesign->op = OP_TRANSLATE;

  ifsDesign->area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (ifsDesign->area, design_width, design_height);

  g_signal_connect (ifsDesign->area, "realize",
                    G_CALLBACK (design_area_realize),
                    NULL);
  g_signal_connect (ifsDesign->area, "expose-event",
                    G_CALLBACK (design_area_expose),
                    NULL);
  g_signal_connect (ifsDesign->area, "button-press-event",
                    G_CALLBACK (design_area_button_press),
                    NULL);
  g_signal_connect (ifsDesign->area, "button-release-event",
                    G_CALLBACK (design_area_button_release),
                    NULL);
  g_signal_connect (ifsDesign->area, "motion-notify-event",
                    G_CALLBACK (design_area_motion),
                    NULL);
  g_signal_connect (ifsDesign->area, "configure-event",
                    G_CALLBACK (design_area_configure),
                    NULL);
  gtk_widget_set_events (ifsDesign->area,
                         GDK_EXPOSURE_MASK       |
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_POINTER_MOTION_HINT_MASK);

  ifsDesign->ui_manager = design_op_menu_create (window);
  design_op_actions_update ();
}

static GtkUIManager *
design_op_menu_create (GtkWidget *window)
{
  static GtkActionEntry actions[] =
  {
    { "ifs-compose-menu", NULL, "IFS Fractal Menu" },

    { "new", GIMP_ICON_DOCUMENT_NEW,
      N_("_New"), "<primary>N", NULL,
      G_CALLBACK (ifs_compose_new_callback) },

    { "delete", GIMP_ICON_EDIT_DELETE,
      N_("_Delete"), "<primary>D", NULL,
      G_CALLBACK (ifs_compose_delete_callback) },

    { "undo", GIMP_ICON_EDIT_UNDO,
      N_("_Undo"), "<primary>Z", NULL,
      G_CALLBACK (undo) },

    { "redo", GIMP_ICON_EDIT_REDO,
      N_("_Redo"), "<primary>Y", NULL,
      G_CALLBACK (redo) },

    { "select-all", GIMP_ICON_SELECTION_ALL,
      N_("Select _All"), "<primary>A", NULL,
      G_CALLBACK (design_area_select_all_callback) },

    { "center", GIMP_ICON_CENTER,
      N_("Re_center"), "<primary>C", N_("Recompute Center"),
      G_CALLBACK (recompute_center_cb) },

    { "options", GIMP_ICON_PREFERENCES_SYSTEM,
      N_("Render Options"), NULL, NULL,
      G_CALLBACK (ifs_compose_options_callback) }
  };
  static GtkRadioActionEntry radio_actions[] =
  {
    { "move", GIMP_ICON_TOOL_MOVE,
      N_("Move"), "M", NULL, OP_TRANSLATE },

    { "rotate", GIMP_ICON_TOOL_ROTATE,
      N_("Rotate"), "R", N_("Rotate / Scale"), OP_ROTATE },

    { "stretch", GIMP_ICON_TOOL_PERSPECTIVE,
      N_("Stretch"), "S", NULL, OP_STRETCH }
  };

  GtkUIManager   *ui_manager = gtk_ui_manager_new ();
  GtkActionGroup *group      = gtk_action_group_new ("Actions");

  gtk_action_group_set_translation_domain (group, NULL);

  gtk_action_group_add_actions (group,
                                actions,
                                G_N_ELEMENTS (actions),
                                window);
  gtk_action_group_add_radio_actions (group,
                                      radio_actions,
                                      G_N_ELEMENTS (radio_actions),
                                      ifsDesign->op,
                                      G_CALLBACK (design_op_update_callback),
                                      window);

  gtk_window_add_accel_group (GTK_WINDOW (window),
                              gtk_ui_manager_get_accel_group (ui_manager));
  gtk_accel_group_lock (gtk_ui_manager_get_accel_group (ui_manager));

  gtk_ui_manager_insert_action_group (ui_manager, group, -1);
  g_object_unref (group);

  gtk_ui_manager_add_ui_from_string (ui_manager,
                                     "<ui>"
                                     "  <menubar name=\"dummy-menubar\">"
                                     "    <menu action=\"ifs-compose-menu\">"
                                     "      <menuitem action=\"move\" />"
                                     "      <menuitem action=\"rotate\" />"
                                     "      <menuitem action=\"stretch\" />"
                                     "      <separator />"
                                     "      <menuitem action=\"new\" />"
                                     "      <menuitem action=\"delete\" />"
                                     "      <menuitem action=\"undo\" />"
                                     "      <menuitem action=\"redo\" />"
                                     "      <menuitem action=\"select-all\" />"
                                     "      <menuitem action=\"center\" />"
                                     "      <separator />"
                                     "      <menuitem action=\"options\" />"
                                     "    </menu>"
                                     "  </menubar>"
                                     "</ui>",
                                     -1, NULL);

  gtk_ui_manager_add_ui_from_string (ui_manager,
                                     "<ui>"
                                     "  <toolbar name=\"ifs-compose-toolbar\">"
                                     "    <toolitem action=\"move\" />"
                                     "    <toolitem action=\"rotate\" />"
                                     "    <toolitem action=\"stretch\" />"
                                     "    <separator />"
                                     "    <toolitem action=\"new\" />"
                                     "    <toolitem action=\"delete\" />"
                                     "    <toolitem action=\"undo\" />"
                                     "    <toolitem action=\"redo\" />"
                                     "    <toolitem action=\"select-all\" />"
                                     "    <toolitem action=\"center\" />"
                                     "    <separator />"
                                     "    <toolitem action=\"options\" />"
                                     "  </toolbar>"
                                     "</ui>",
                                     -1, NULL);

  return ui_manager;
}

static void
design_op_actions_update (void)
{
  GtkAction *act;

  act = gtk_ui_manager_get_action (ifsDesign->ui_manager,
                                   "/ui/dummy-menubar/ifs-compose-menu/undo");
  gtk_action_set_sensitive (act, undo_cur >= 0);

  act = gtk_ui_manager_get_action (ifsDesign->ui_manager,
                                   "/ui/dummy-menubar/ifs-compose-menu/redo");
  gtk_action_set_sensitive (act, undo_cur != undo_num - 1);

  act = gtk_ui_manager_get_action (ifsDesign->ui_manager,
                                   "/ui/dummy-menubar/ifs-compose-menu/delete");
  gtk_action_set_sensitive (act, ifsvals.num_elements > 2);
}

static void
ifs_options_dialog (GtkWidget *parent)
{
  if (!ifsOptD)
    {
      GtkWidget *table;
      GtkWidget *label;

      ifsOptD = g_new0 (IfsOptionsDialog, 1);

      ifsOptD->dialog =
        gimp_dialog_new (_("IFS Fractal Render Options"), PLUG_IN_ROLE,
                         parent, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         _("_Close"), GTK_RESPONSE_CLOSE,

                         NULL);

      g_signal_connect (ifsOptD->dialog, "response",
                        G_CALLBACK (gtk_widget_hide),
                        NULL);

      /* Table of options */

      table = gtk_table_new (4, 3, FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (table), 12);
      gtk_table_set_row_spacings (GTK_TABLE (table), 6);
      gtk_table_set_col_spacings (GTK_TABLE (table), 6);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (ifsOptD->dialog))),
                          table, FALSE, FALSE, 0);
      gtk_widget_show (table);

      label = gtk_label_new (_("Max. memory:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      ifsOptD->memory_pair = value_pair_create (&ifsvals.max_memory,
                                                1, 1000000, FALSE,
                                                VALUE_PAIR_INT);
      gtk_table_attach (GTK_TABLE (table), ifsOptD->memory_pair->spin,
                        1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ifsOptD->memory_pair->spin);

      label = gtk_label_new (_("Iterations:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      ifsOptD->iterations_pair = value_pair_create (&ifsvals.iterations,
                                                    1, 10000000, FALSE,
                                                    VALUE_PAIR_INT);
      gtk_table_attach (GTK_TABLE (table), ifsOptD->iterations_pair->spin,
                        1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ifsOptD->iterations_pair->spin);
      gtk_widget_show (label);

      label = gtk_label_new (_("Subdivide:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      ifsOptD->subdivide_pair = value_pair_create (&ifsvals.subdivide,
                                                   1, 10, FALSE,
                                                   VALUE_PAIR_INT);
      gtk_table_attach (GTK_TABLE (table), ifsOptD->subdivide_pair->spin,
                        1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ifsOptD->subdivide_pair->spin);

      label = gtk_label_new (_("Spot radius:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      ifsOptD->radius_pair = value_pair_create (&ifsvals.radius,
                                                0, 5, TRUE,
                                                VALUE_PAIR_DOUBLE);
      gtk_table_attach (GTK_TABLE (table), ifsOptD->radius_pair->scale,
                        1, 2, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ifsOptD->radius_pair->scale);
      gtk_table_attach (GTK_TABLE (table), ifsOptD->radius_pair->spin,
                        2, 3, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ifsOptD->radius_pair->spin);

      value_pair_update (ifsOptD->iterations_pair);
      value_pair_update (ifsOptD->subdivide_pair);
      value_pair_update (ifsOptD->memory_pair);
      value_pair_update (ifsOptD->radius_pair);

      gtk_widget_show (ifsOptD->dialog);
    }
  else
    {
      gtk_window_present (GTK_WINDOW (ifsOptD->dialog));
    }
}

static void
ifs_compose (gint32 drawable_id)
{
  GeglBuffer *buffer = gimp_drawable_get_shadow_buffer (drawable_id);
  gint        width  = gimp_drawable_width (drawable_id);
  gint        height = gimp_drawable_height (drawable_id);
  gboolean    alpha  = gimp_drawable_has_alpha (drawable_id);
  const Babl *format;
  gint        num_bands;
  gint        band_height;
  gint        band_y;
  gint        band_no;
  gint        i, j;
  guchar     *data;
  guchar     *mask = NULL;
  guchar     *nhits;
  guchar      rc, gc, bc;
  GimpRGB     color;

  if (alpha)
    format = babl_format ("R'G'B'A u8");
  else
    format = babl_format ("R'G'B' u8");

  num_bands = ceil ((gdouble) (width * height * SQR (ifsvals.subdivide) * 5)
                   / (1024 * ifsvals.max_memory));
  band_height = (height + num_bands - 1) / num_bands;

  if (band_height > height)
    band_height = height;

  mask  = g_new (guchar, width * band_height * SQR (ifsvals.subdivide));
  data  = g_new (guchar, width * band_height * SQR (ifsvals.subdivide) * 3);
  nhits = g_new (guchar, width * band_height * SQR (ifsvals.subdivide));

  gimp_context_get_background (&color);
  gimp_rgb_get_uchar (&color, &rc, &gc, &bc);

  for (band_no = 0, band_y = 0; band_no < num_bands; band_no++)
    {
      GeglBufferIterator *iter;
      GeglRectangle      *roi;

      gimp_progress_init_printf (_("Rendering IFS (%d/%d)"),
                                 band_no + 1, num_bands);

      /* render the band to a buffer */
      if (band_y + band_height > height)
        band_height = height - band_y;

      /* we don't need to clear data since we store nhits */
      memset (mask, 0, width * band_height * SQR (ifsvals.subdivide));
      memset (nhits, 0, width * band_height * SQR (ifsvals.subdivide));

      ifs_render (elements,
                  ifsvals.num_elements, width, height, ifsvals.iterations,
                  &ifsvals, band_y, band_height, data, mask, nhits, FALSE);

      /* transfer the image to the drawable */

      iter = gegl_buffer_iterator_new (buffer,
                                       GEGL_RECTANGLE (0, band_y,
                                                       width, band_height), 0,
                                       format,
                                       GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);
      roi = &iter->roi[0];

      while (gegl_buffer_iterator_next (iter))
        {
          guchar *destrow = iter->data[0];

          for (j = roi->y; j < (roi->y + roi->height); j++)
            {
              guchar *dest = destrow;

              for (i = roi->x; i < (roi->x + roi->width); i++)
                {
                  /* Accumulate a reduced pixel */

                  gint rtot = 0;
                  gint btot = 0;
                  gint gtot = 0;
                  gint mtot = 0;
                  gint ii, jj;

                  for (jj = 0; jj < ifsvals.subdivide; jj++)
                    {
                      guchar *ptr;
                      guchar *maskptr;

                      ptr = data +
                        3 * (((j - band_y) * ifsvals.subdivide + jj) *
                             ifsvals.subdivide * width +
                             i * ifsvals.subdivide);

                      maskptr = mask +
                        ((j - band_y) * ifsvals.subdivide + jj) *
                        ifsvals.subdivide * width +
                        i * ifsvals.subdivide;

                      for (ii = 0; ii < ifsvals.subdivide; ii++)
                        {
                          guchar  maskval = *maskptr++;

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
                      mtot /= SQR (ifsvals.subdivide);
                    }

                  if (alpha)
                    {
                      *dest++ = rtot;
                      *dest++ = gtot;
                      *dest++ = btot;
                      *dest++ = mtot;
                    }
                  else
                    {
                      *dest++ = (mtot * rtot + (255 - mtot) * rc) / 255;
                      *dest++ = (mtot * gtot + (255 - mtot) * gc) / 255;
                      *dest++ = (mtot * btot + (255 - mtot) * bc) / 255;
                    }
                }

              if (alpha)
                destrow += roi->width * 4;
              else
                destrow += roi->width * 3;
            }
        }

      band_y += band_height;
    }

  g_free (mask);
  g_free (data);
  g_free (nhits);

  g_object_unref (buffer);

  gimp_drawable_merge_shadow (drawable_id, TRUE);
  gimp_drawable_update (drawable_id, 0, 0, width, height);
}

static void
update_values (void)
{
  ifsD->in_update = TRUE;

  ifsD->current_vals = elements[ifsD->current_element]->v;
  ifsD->current_vals.theta *= 180/G_PI;

  value_pair_update (ifsD->prob_pair);
  value_pair_update (ifsD->x_pair);
  value_pair_update (ifsD->y_pair);
  value_pair_update (ifsD->scale_pair);
  value_pair_update (ifsD->angle_pair);
  value_pair_update (ifsD->asym_pair);
  value_pair_update (ifsD->shear_pair);
  color_map_update (ifsD->red_cmap);
  color_map_update (ifsD->green_cmap);
  color_map_update (ifsD->blue_cmap);
  color_map_update (ifsD->black_cmap);
  color_map_update (ifsD->target_cmap);
  value_pair_update (ifsD->hue_scale_pair);
  value_pair_update (ifsD->value_scale_pair);

  if (elements[ifsD->current_element]->v.simple_color)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ifsD->simple_button),
                                  TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ifsD->full_button),
                                 TRUE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ifsD->flip_check_button),
                                elements[ifsD->current_element]->v.flip);

  ifsD->in_update = FALSE;

  simple_color_set_sensitive ();
}

static void
set_current_element (gint index)
{
  gchar *frame_name = g_strdup_printf (_("Transformation %s"),
                                      elements[index]->name);

  ifsD->current_element = index;

  gtk_frame_set_label (GTK_FRAME (ifsD->current_frame),frame_name);
  g_free (frame_name);

  update_values ();
}

static void
design_area_realize (GtkWidget *widget)
{
  const gint cursors[3] =
  {
    GDK_FLEUR,     /* OP_TRANSLATE */
    GDK_EXCHANGE,  /* OP_ROTATE    */
    GDK_CROSSHAIR  /* OP_SHEAR     */
  };

  GdkDisplay *display = gtk_widget_get_display (widget);
  GdkCursor  *cursor  = gdk_cursor_new_for_display (display,
                                                    cursors[ifsDesign->op]);
  gdk_window_set_cursor (gtk_widget_get_window (widget), cursor);
  g_object_unref (cursor);
}

static gboolean
design_area_expose (GtkWidget      *widget,
                    GdkEventExpose *event)
{
  GtkStyle      *style = gtk_widget_get_style (widget);
  GtkStateType   state = gtk_widget_get_state (widget);
  cairo_t       *cr;
  GtkAllocation  allocation;
  PangoLayout   *layout;
  gint           i;
  gint           cx, cy;

  gtk_widget_get_allocation (widget, &allocation);

  cr = gdk_cairo_create (ifsDesign->pixmap);

  gdk_cairo_set_source_color (cr, &style->bg[state]);
  cairo_paint (cr);

  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_translate (cr, 0.5, 0.5);

  /* draw an indicator for the center */

  cx = ifsvals.center_x * allocation.width;
  cy = ifsvals.center_y * allocation.width;

  cairo_move_to (cr, cx - 10, cy);
  cairo_line_to (cr, cx + 10, cy);

  cairo_move_to (cr, cx, cy - 10);
  cairo_line_to (cr, cx, cy + 10);

  gdk_cairo_set_source_color (cr, &style->fg[state]);
  cairo_set_line_width (cr, 1.0);
  cairo_stroke (cr);

  layout = gtk_widget_create_pango_layout (widget, NULL);

  for (i = 0; i < ifsvals.num_elements; i++)
    {
      aff_element_draw (elements[i], element_selected[i],
                        allocation.width,
                        allocation.height,
                        cr,
                        &style->fg[state],
                        layout);
    }

  g_object_unref (layout);

  cairo_destroy (cr);

  cr = gdk_cairo_create (gtk_widget_get_window (widget));

  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  gdk_cairo_set_source_pixmap (cr, ifsDesign->pixmap, 0.0, 0.0);
  cairo_paint (cr);

  cairo_destroy (cr);

  return FALSE;
}

static gboolean
design_area_configure (GtkWidget         *widget,
                       GdkEventConfigure *event)
{
  GtkAllocation allocation;
  gint          i;

  gtk_widget_get_allocation (widget, &allocation);

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_compute_trans (elements[i],
                               allocation.width, allocation.height,
                               ifsvals.center_x, ifsvals.center_y);

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_compute_boundary (elements[i],
                                  allocation.width, allocation.height,
                                  elements, ifsvals.num_elements);

  if (ifsDesign->pixmap)
    {
      g_object_unref (ifsDesign->pixmap);
    }
  ifsDesign->pixmap = gdk_pixmap_new (gtk_widget_get_window (widget),
                                      allocation.width,
                                      allocation.height,
                                      -1); /* Is this correct? */

  return FALSE;
}

static gint
design_area_button_press (GtkWidget      *widget,
                          GdkEventButton *event)
{
  GtkAllocation allocation;
  gint          i;
  gint          old_current;

  gtk_widget_get_allocation (ifsDesign->area, &allocation);

  gtk_widget_grab_focus (widget);

  if (gdk_event_triggers_context_menu ((GdkEvent *) event))
    {
      GtkWidget *menu =
        gtk_ui_manager_get_widget (ifsDesign->ui_manager,
                                   "/dummy-menubar/ifs-compose-menu");

      if (GTK_IS_MENU_ITEM (menu))
        menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (menu));

      gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));

      gtk_menu_popup (GTK_MENU (menu),
                      NULL, NULL, NULL, NULL,
                      event->button, event->time);

      return FALSE;
    }

  old_current = ifsD->current_element;
  ifsD->current_element = -1;

  /* Find out where the button press was */
  for (i = 0; i < ifsvals.num_elements; i++)
    {
      if (ipolygon_contains (elements[i]->click_boundary, event->x, event->y))
        {
          set_current_element (i);
          break;
        }
    }

  /* if the user started manipulating an object, set up a new
     position on the undo ring */
  if (ifsD->current_element >= 0)
    undo_begin ();

  if (!(event->state & GDK_SHIFT_MASK)
      && ( (ifsD->current_element<0)
           || !element_selected[ifsD->current_element] ))
    {
      for (i = 0; i < ifsvals.num_elements; i++)
        element_selected[i] = FALSE;
    }

  if (ifsD->current_element >= 0)
    {
      ifsDesign->button_state |= GDK_BUTTON1_MASK;

      element_selected[ifsD->current_element] = TRUE;

      ifsDesign->num_selected = 0;
      ifsDesign->op_xcenter = 0.0;
      ifsDesign->op_ycenter = 0.0;
      for (i = 0; i < ifsvals.num_elements; i++)
        {
          if (element_selected[i])
            {
              ifsD->selected_orig[i] = *elements[i];
              ifsDesign->op_xcenter += elements[i]->v.x;
              ifsDesign->op_ycenter += elements[i]->v.y;
              ifsDesign->num_selected++;
              undo_update (i);
            }
        }
      ifsDesign->op_xcenter /= ifsDesign->num_selected;
      ifsDesign->op_ycenter /= ifsDesign->num_selected;
      ifsDesign->op_x = (gdouble)event->x / allocation.width;
      ifsDesign->op_y = (gdouble)event->y / allocation.width;
      ifsDesign->op_center_x = ifsvals.center_x;
      ifsDesign->op_center_y = ifsvals.center_y;
    }
  else
    {
      ifsD->current_element = old_current;
      element_selected[old_current] = TRUE;
    }

  design_area_redraw ();

  return FALSE;
}

static gint
design_area_button_release (GtkWidget      *widget,
                            GdkEventButton *event)
{
  if (event->button == 1 &&
      (ifsDesign->button_state & GDK_BUTTON1_MASK))
    {
      ifsDesign->button_state &= ~GDK_BUTTON1_MASK;
      ifs_compose_preview ();
    }
  return FALSE;
}

static gint
design_area_motion (GtkWidget      *widget,
                    GdkEventMotion *event)
{
  GtkAllocation allocation;
  gint          i;
  gdouble       xo;
  gdouble       yo;
  gdouble       xn;
  gdouble       yn;
  Aff2          trans, t1, t2, t3;

  if (! (ifsDesign->button_state & GDK_BUTTON1_MASK))
    return FALSE;

  gtk_widget_get_allocation (ifsDesign->area, &allocation);

  xo = (ifsDesign->op_x - ifsDesign->op_xcenter);
  yo = (ifsDesign->op_y - ifsDesign->op_ycenter);
  xn = (gdouble) event->x / allocation.width - ifsDesign->op_xcenter;
  yn = (gdouble) event->y / allocation.width - ifsDesign->op_ycenter;

  switch (ifsDesign->op)
    {
    case OP_ROTATE:
      aff2_translate (&t1,-ifsDesign->op_xcenter * allocation.width,
                      -ifsDesign->op_ycenter * allocation.width);
      aff2_scale (&t2,
                  sqrt((SQR(xn)+SQR(yn))/(SQR(xo)+SQR(yo))),
                  0);
      aff2_compose (&t3, &t2, &t1);
      aff2_rotate (&t1, - atan2(yn, xn) + atan2(yo, xo));
      aff2_compose (&t2, &t1, &t3);
      aff2_translate (&t3, ifsDesign->op_xcenter * allocation.width,
                      ifsDesign->op_ycenter * allocation.width);
      aff2_compose (&trans, &t3, &t2);
      break;

    case OP_STRETCH:
      aff2_translate (&t1,-ifsDesign->op_xcenter * allocation.width,
                      -ifsDesign->op_ycenter * allocation.width);
      aff2_compute_stretch (&t2, xo, yo, xn, yn);
      aff2_compose (&t3, &t2, &t1);
      aff2_translate (&t1, ifsDesign->op_xcenter * allocation.width,
                      ifsDesign->op_ycenter * allocation.width);
      aff2_compose (&trans, &t1, &t3);
      break;

    case OP_TRANSLATE:
      aff2_translate (&trans,
                      (xn-xo) * allocation.width,
                      (yn-yo) * allocation.width);
      break;
    }

  for (i = 0; i < ifsvals.num_elements; i++)
    if (element_selected[i])
      {
        if (ifsDesign->num_selected == ifsvals.num_elements)
          {
            gdouble cx, cy;
            aff2_invert (&t1, &trans);
            aff2_compose (&t2, &trans, &ifsD->selected_orig[i].trans);
            aff2_compose (&elements[i]->trans, &t2, &t1);

            cx = ifsDesign->op_center_x * allocation.width;
            cy = ifsDesign->op_center_y * allocation.width;
            aff2_apply (&trans, cx, cy, &cx, &cy);
            ifsvals.center_x = cx / allocation.width;
            ifsvals.center_y = cy / allocation.width;
          }
        else
          {
            aff2_compose (&elements[i]->trans, &trans,
                         &ifsD->selected_orig[i].trans);
          }

        aff_element_decompose_trans (elements[i],&elements[i]->trans,
                                     allocation.width, allocation.height,
                                     ifsvals.center_x, ifsvals.center_y);
        aff_element_compute_trans (elements[i],
                                   allocation.width, allocation.height,
                                   ifsvals.center_x, ifsvals.center_y);
      }

  update_values ();
  design_area_redraw ();

  /* Ask for more motion events in case the event was a hint */
  gdk_event_request_motions (event);

  return FALSE;
}

static void
design_area_redraw (void)
{
  GtkAllocation allocation;
  gint          i;

  gtk_widget_get_allocation (ifsDesign->area, &allocation);

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_compute_boundary (elements[i],
                                  allocation.width, allocation.height,
                                  elements, ifsvals.num_elements);

  gtk_widget_queue_draw (ifsDesign->area);
}

/* Undo ring functions */
static void
undo_begin (void)
{
  gint i, j;
  gint to_delete;
  gint new_index;

  if (undo_cur == UNDO_LEVELS-1)
    {
      to_delete = 1;
      undo_start = (undo_start + 1) % UNDO_LEVELS;
    }
  else
    {
      undo_cur++;
      to_delete = undo_num - undo_cur;
    }

  undo_num = undo_num - to_delete + 1;
  new_index = (undo_start + undo_cur) % UNDO_LEVELS;

  /* remove any redo elements or the oldest element if necessary */
  for (j = new_index; to_delete > 0; j = (j+1) % UNDO_LEVELS, to_delete--)
    {
      for (i = 0; i < undo_ring[j].ifsvals.num_elements; i++)
        if (undo_ring[j].elements[i])
          aff_element_free (undo_ring[j].elements[i]);
      g_free (undo_ring[j].elements);
      g_free (undo_ring[j].element_selected);
    }

  undo_ring[new_index].ifsvals = ifsvals;
  undo_ring[new_index].elements = g_new (AffElement *,ifsvals.num_elements);
  undo_ring[new_index].element_selected = g_new (gboolean,
                                                 ifsvals.num_elements);
  undo_ring[new_index].current_element = ifsD->current_element;

  for (i = 0; i < ifsvals.num_elements; i++)
    {
      undo_ring[new_index].elements[i] = NULL;
      undo_ring[new_index].element_selected[i] = element_selected[i];
    }

  design_op_actions_update ();
}

static void
undo_update (gint el)
{
  AffElement *elem;
  /* initialize */

  elem = NULL;

  if (!undo_ring[(undo_start + undo_cur) % UNDO_LEVELS].elements[el])
    undo_ring[(undo_start + undo_cur) % UNDO_LEVELS].elements[el]
      = elem = g_new (AffElement, 1);

  *elem = *elements[el];
  elem->draw_boundary = NULL;
  elem->click_boundary = NULL;
}

static void
undo_exchange (gint el)
{
  GtkAllocation   allocation;
  gint            i;
  AffElement    **telements;
  gboolean       *tselected;
  IfsComposeVals  tifsvals;
  gint            tcurrent;

  gtk_widget_get_allocation (ifsDesign->area, &allocation);

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
  for (i = 0; i < ifsvals.num_elements; i++)
    if (!elements[i])
      {
        elements[i] = undo_ring[el].elements[i];
        undo_ring[el].elements[i] = NULL;
      }
    else
      aff_element_compute_trans (elements[i],
                                 allocation.width, allocation.height,
                                 ifsvals.center_x, ifsvals.center_y);

  set_current_element (ifsD->current_element);

  design_area_redraw ();

  ifs_compose_preview ();
}

static void
undo (void)
{
  if (undo_cur >= 0)
    {
      undo_exchange ((undo_start + undo_cur) % UNDO_LEVELS);
      undo_cur--;
    }

  design_op_actions_update ();
}

static void
redo (void)
{
  if (undo_cur != undo_num - 1)
    {
      undo_cur++;
      undo_exchange ((undo_start + undo_cur) % UNDO_LEVELS);
    }

  design_op_actions_update ();
}

static void
design_area_select_all_callback (GtkWidget *widget,
                                 gpointer   data)
{
  gint i;

  for (i = 0; i < ifsvals.num_elements; i++)
    element_selected[i] = TRUE;

  design_area_redraw ();
}

/*  Interface functions  */

static void
val_changed_update (void)
{
  GtkAllocation  allocation;
  AffElement    *cur;

  if (ifsD->in_update)
    return;

  gtk_widget_get_allocation (ifsDesign->area, &allocation);

  cur = elements[ifsD->current_element];

  undo_begin ();
  undo_update (ifsD->current_element);

  cur->v = ifsD->current_vals;
  cur->v.theta *= G_PI/180.0;
  aff_element_compute_trans (cur,
                             allocation.width, allocation.height,
                             ifsvals.center_x, ifsvals.center_y);
  aff_element_compute_color_trans (cur);

  design_area_redraw ();

  ifs_compose_preview ();
}

/* Pseudo-widget representing a color mapping */

#define COLOR_SAMPLE_SIZE 30

static ColorMap *
color_map_create (const gchar *name,
                  GimpRGB     *orig_color,
                  GimpRGB     *data,
                  gboolean     fixed_point)
{
  GtkWidget *frame;
  GtkWidget *arrow;
  ColorMap  *color_map = g_new (ColorMap, 1);

  gimp_rgb_set_alpha (data, 1.0);
  color_map->color       = data;
  color_map->fixed_point = fixed_point;
  color_map->hbox        = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (color_map->hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  color_map->orig_preview =
    gimp_color_area_new (fixed_point ? data : orig_color,
                         GIMP_COLOR_AREA_FLAT, 0);
  gtk_drag_dest_unset (color_map->orig_preview);
  gtk_widget_set_size_request (color_map->orig_preview,
                               COLOR_SAMPLE_SIZE, COLOR_SAMPLE_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), color_map->orig_preview);
  gtk_widget_show (color_map->orig_preview);

  arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (color_map->hbox), arrow, FALSE, FALSE, 0);
  gtk_widget_show (arrow);

  color_map->button = gimp_color_button_new (name,
                                             COLOR_SAMPLE_SIZE,
                                             COLOR_SAMPLE_SIZE,
                                             data,
                                             GIMP_COLOR_AREA_FLAT);
  gtk_box_pack_start (GTK_BOX (color_map->hbox), color_map->button,
                      FALSE, FALSE, 0);
  gtk_widget_show (color_map->button);

  g_signal_connect (color_map->button, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    data);

  g_signal_connect (color_map->button, "color-changed",
                    G_CALLBACK (color_map_color_changed_cb),
                    color_map);

  return color_map;
}

static void
color_map_color_changed_cb (GtkWidget *widget,
                            ColorMap  *color_map)
{
  if (ifsD->in_update)
    return;

  undo_begin ();
  undo_update (ifsD->current_element);

  elements[ifsD->current_element]->v = ifsD->current_vals;
  elements[ifsD->current_element]->v.theta *= G_PI/180.0;
  aff_element_compute_color_trans (elements[ifsD->current_element]);

  update_values ();

  ifs_compose_preview ();
}

static void
color_map_update (ColorMap *color_map)
{
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (color_map->button),
                               color_map->color);

  if (color_map->fixed_point)
    gimp_color_area_set_color (GIMP_COLOR_AREA (color_map->orig_preview),
                               color_map->color);
}

static void
simple_color_set_sensitive (void)
{
  gint sc = elements[ifsD->current_element]->v.simple_color;

  gtk_widget_set_sensitive (ifsD->target_cmap->hbox,       sc);
  gtk_widget_set_sensitive (ifsD->hue_scale_pair->scale,   sc);
  gtk_widget_set_sensitive (ifsD->hue_scale_pair->spin,   sc);
  gtk_widget_set_sensitive (ifsD->value_scale_pair->scale, sc);
  gtk_widget_set_sensitive (ifsD->value_scale_pair->spin, sc);

  gtk_widget_set_sensitive (ifsD->red_cmap->hbox,   !sc);
  gtk_widget_set_sensitive (ifsD->green_cmap->hbox, !sc);
  gtk_widget_set_sensitive (ifsD->blue_cmap->hbox,  !sc);
  gtk_widget_set_sensitive (ifsD->black_cmap->hbox, !sc);
}

static void
simple_color_toggled (GtkWidget *widget,
                      gpointer   data)
{
  AffElement *cur = elements[ifsD->current_element];

  cur->v.simple_color = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  ifsD->current_vals.simple_color = cur->v.simple_color;

  if (cur->v.simple_color)
    aff_element_compute_color_trans (cur);

  val_changed_update ();
  simple_color_set_sensitive ();
}

/* Generic mechanism for scale/entry combination (possibly without
   scale) */

static ValuePair *
value_pair_create (gpointer      data,
                   gdouble       lower,
                   gdouble       upper,
                   gboolean      create_scale,
                   ValuePairType type)
{

  ValuePair *value_pair = g_new (ValuePair, 1);

  value_pair->data.d = data;
  value_pair->type   = type;
  value_pair->timeout_id = 0;

  value_pair->adjustment = (GtkAdjustment *)
    gtk_adjustment_new (1.0, lower, upper,
                        (upper - lower) / 100,
                        (upper - lower) / 10,
                        0.0);
  value_pair->spin = gtk_spin_button_new (value_pair->adjustment, 1.0, 3);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (value_pair->spin), TRUE);
  gtk_widget_set_size_request (value_pair->spin, 72, -1);

  g_signal_connect (value_pair->adjustment, "value-changed",
                    G_CALLBACK (value_pair_scale_callback),
                    value_pair);

  if (create_scale)
    {
      value_pair->scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL,
                                         value_pair->adjustment);

      if (type == VALUE_PAIR_INT)
        gtk_scale_set_digits (GTK_SCALE (value_pair->scale), 0);
      else
        gtk_scale_set_digits (GTK_SCALE (value_pair->scale), 3);

      gtk_scale_set_draw_value (GTK_SCALE (value_pair->scale), FALSE);
    }
  else
    {
      value_pair->scale = NULL;
    }

  return value_pair;
}

static void
value_pair_update (ValuePair *value_pair)
{
  if (value_pair->type == VALUE_PAIR_INT)
    gtk_adjustment_set_value (value_pair->adjustment, *value_pair->data.i);
  else
    gtk_adjustment_set_value (value_pair->adjustment, *value_pair->data.d);

}

static gboolean
value_pair_scale_callback_real (gpointer data)
{
  ValuePair *value_pair = data;
  gint changed = FALSE;

  if (value_pair->type == VALUE_PAIR_DOUBLE)
    {
      if ((gdouble) *value_pair->data.d !=
          gtk_adjustment_get_value (value_pair->adjustment))
        {
          changed = TRUE;
          *value_pair->data.d = gtk_adjustment_get_value (value_pair->adjustment);
        }
    }
  else
    {
      if (*value_pair->data.i !=
          (gint) gtk_adjustment_get_value (value_pair->adjustment))
        {
          changed = TRUE;
          *value_pair->data.i = gtk_adjustment_get_value (value_pair->adjustment);
        }
    }

  if (changed)
    val_changed_update ();

  value_pair->timeout_id = 0;

  return FALSE;
}

static void
value_pair_scale_callback (GtkAdjustment *adjustment,
                           ValuePair     *value_pair)
{
  if (value_pair->timeout_id != 0)
    return;

  value_pair->timeout_id = g_timeout_add (500, /* update every half second */
                                          value_pair_scale_callback_real,
                                          value_pair);
}

static void
design_op_update_callback (GtkRadioAction *action,
                           GtkRadioAction *current,
                           gpointer        data)
{
  ifsDesign->op = gtk_radio_action_get_current_value (action);

  /* cursor switch */
  if (gtk_widget_get_realized (ifsDesign->area))
    design_area_realize (ifsDesign->area);
}

static void
recompute_center_cb (GtkWidget *widget,
                     gpointer   data)
{
  recompute_center (TRUE);
}

static void
recompute_center (gboolean save_undo)
{
  GtkAllocation allocation;
  gint          i;
  gdouble       x, y;
  gdouble       center_x = 0.0;
  gdouble       center_y = 0.0;

  gtk_widget_get_allocation (ifsDesign->area, &allocation);

  if (save_undo)
    undo_begin ();

  for (i = 0; i < ifsvals.num_elements; i++)
    {
      if (save_undo)
        undo_update (i);

      aff_element_compute_trans (elements[i],1, ifsvals.aspect_ratio,
                                ifsvals.center_x, ifsvals.center_y);
      aff2_fixed_point (&elements[i]->trans, &x, &y);
      center_x += x;
      center_y += y;
    }

  ifsvals.center_x = center_x/ifsvals.num_elements;
  ifsvals.center_y = center_y/ifsvals.num_elements;

  for (i = 0; i < ifsvals.num_elements; i++)
    {
        aff_element_decompose_trans (elements[i],&elements[i]->trans,
                                    1, ifsvals.aspect_ratio,
                                    ifsvals.center_x, ifsvals.center_y);
    }

  if (allocation.width > 1 && allocation.height > 1)
    {
      for (i = 0; i < ifsvals.num_elements; i++)
        aff_element_compute_trans (elements[i],
                                   allocation.width, allocation.height,
                                   ifsvals.center_x, ifsvals.center_y);
      design_area_redraw ();
      update_values ();
    }
}

static void
flip_check_button_callback (GtkWidget *widget,
                            gpointer   data)
{
  GtkAllocation allocation;
  guint         i;
  gboolean      active;

  if (ifsD->in_update)
    return;

  gtk_widget_get_allocation (ifsDesign->area, &allocation);

  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  undo_begin ();
  for (i = 0; i < ifsvals.num_elements; i++)
    {
      if (element_selected[i])
        {
          undo_update (i);
          elements[i]->v.flip = active;
          aff_element_compute_trans (elements[i],
                                     allocation.width, allocation.height,
                                     ifsvals.center_x, ifsvals.center_y);
        }
    }

  update_values ();
  design_area_redraw ();

  ifs_compose_preview ();
}

static void
ifs_compose_set_defaults (void)
{
  gint     i;
  GimpRGB  color;

  gimp_context_get_foreground (&color);

  ifsvals.aspect_ratio =
    (gdouble)ifsD->drawable_height / ifsD->drawable_width;

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_free (elements[i]);
  count_for_naming = 0;

  ifsvals.num_elements = 3;
  elements = g_realloc (elements, ifsvals.num_elements * sizeof(AffElement *));
  element_selected = g_realloc (element_selected,
                                ifsvals.num_elements * sizeof(gboolean));

  elements[0] = aff_element_new (0.3, 0.37 * ifsvals.aspect_ratio, &color,
                                 ++count_for_naming);
  element_selected[0] = FALSE;
  elements[1] = aff_element_new (0.7, 0.37 * ifsvals.aspect_ratio, &color,
                                 ++count_for_naming);
  element_selected[1] = FALSE;
  elements[2] = aff_element_new (0.5, 0.7 * ifsvals.aspect_ratio, &color,
                                 ++count_for_naming);
  element_selected[2] = FALSE;

  ifsvals.center_x   = 0.5;
  ifsvals.center_y   = 0.5 * ifsvals.aspect_ratio;
  ifsvals.iterations = ifsD->drawable_height * ifsD->drawable_width;
  ifsvals.subdivide  = 3;
  ifsvals.max_memory = 4096;

  if (ifsOptD)
    {
      value_pair_update (ifsOptD->iterations_pair);
      value_pair_update (ifsOptD->subdivide_pair);
      value_pair_update (ifsOptD->radius_pair);
      value_pair_update (ifsOptD->memory_pair);
    }

  ifsvals.radius = 0.7;

  set_current_element (0);
  element_selected[0] = TRUE;
  recompute_center (FALSE);

  if (ifsD->selected_orig)
    g_free (ifsD->selected_orig);

  ifsD->selected_orig = g_new (AffElement, ifsvals.num_elements);
}

/* show a transient message dialog */
static void
ifscompose_message_dialog (GtkMessageType  type,
                           GtkWindow      *parent,
                           const gchar    *title,
                           const gchar    *message)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (parent, 0, type, GTK_BUTTONS_OK,
				   "%s", message);

  if (title)
    gtk_window_set_title (GTK_WINDOW (dialog), title);

  gtk_window_set_role (GTK_WINDOW (dialog), "ifscompose-message");
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/* save an ifs file */
static void
ifsfile_save_response (GtkWidget *dialog,
                       gint       response_id,
                       gpointer   data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *filename;
      gchar *str;
      FILE  *fh;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      str = ifsvals_stringify (&ifsvals, elements);

      fh = g_fopen (filename, "wb");
      if (! fh)
        {
          gchar *message =
            g_strdup_printf (_("Could not open '%s' for writing: %s"),
                             gimp_filename_to_utf8 (filename),
                             g_strerror (errno));

          ifscompose_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dialog),
                                     _("Save failed"), message);

          g_free (message);
          g_free (filename);

          return;
        }

      fputs (str, fh);
      fclose (fh);
    }

  gtk_widget_destroy (dialog);
}

/* replace ifsvals and elements with specified new values
 * recompute and update everything necessary */
static void
ifsfile_replace_ifsvals (IfsComposeVals  *new_ifsvals,
                         AffElement     **new_elements)
{
  GtkAllocation allocation;
  guint         i;

  gtk_widget_get_allocation (ifsDesign->area, &allocation);

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_free (elements[i]);
  g_free (elements);

  ifsvals = *new_ifsvals;
  elements = new_elements;
  for (i = 0; i < ifsvals.num_elements; i++)
    {
      aff_element_compute_trans (elements[i],
                                 allocation.width, allocation.height,
                                 ifsvals.center_x, ifsvals.center_y);
      aff_element_compute_color_trans (elements[i]);
    }

  element_selected = g_realloc (element_selected,
                                ifsvals.num_elements * sizeof(gboolean));
  for (i = 0; i < ifsvals.num_elements; i++)
    element_selected[i] = FALSE;

  if (ifsOptD)
    {
      value_pair_update (ifsOptD->iterations_pair);
      value_pair_update (ifsOptD->subdivide_pair);
      value_pair_update (ifsOptD->radius_pair);
      value_pair_update (ifsOptD->memory_pair);
    }

  set_current_element (0);
  element_selected[0] = TRUE;
  recompute_center (FALSE);

  if (ifsD->selected_orig)
    g_free (ifsD->selected_orig);

  ifsD->selected_orig = g_new (AffElement, ifsvals.num_elements);
}

/* load an ifs file */
static void
ifsfile_load_response (GtkWidget *dialog,
                       gint       response_id,
                       gpointer   data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar           *filename;
      gchar           *buffer;
      AffElement     **new_elements;
      IfsComposeVals   new_ifsvals;
      GError          *error = NULL;
      guint            i;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (! g_file_get_contents (filename, &buffer, NULL, &error))
        {
          ifscompose_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dialog),
                                     _("Open failed"), error->message);
          g_error_free (error);
          g_free (filename);
          return;
        }

      if (! ifsvals_parse_string (buffer, &new_ifsvals, &new_elements))
        {
          gchar *message = g_strdup_printf (_("File '%s' doesn't seem to be "
                                              "an IFS Fractal file."),
                                            gimp_filename_to_utf8 (filename));

          ifscompose_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dialog),
                                     _("Open failed"), message);
          g_free (filename);
          g_free (message);
          g_free (buffer);

          return;
        }

      g_free (buffer);
      g_free (filename);

      undo_begin ();
      for (i = 0; i < ifsvals.num_elements; i++)
        undo_update (i);

      ifsfile_replace_ifsvals (&new_ifsvals, new_elements);

      design_op_actions_update ();

      ifs_compose_preview ();

      design_area_redraw ();
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
ifs_compose_save (GtkWidget *parent)
{
  static GtkWidget *dialog = NULL;

  if (! dialog)
    {
      dialog =
        gtk_file_chooser_dialog_new (_("Save as IFS Fractal file"),
                                     GTK_WINDOW (parent),
                                     GTK_FILE_CHOOSER_ACTION_SAVE,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Save"),   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);
      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                      TRUE);

      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &dialog);
      g_signal_connect (dialog, "response",
                        G_CALLBACK (ifsfile_save_response),
                        NULL);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
ifs_compose_load (GtkWidget *parent)
{
  static GtkWidget *dialog = NULL;

  if (! dialog)
    {
      dialog =
        gtk_file_chooser_dialog_new (_("Open IFS Fractal file"),
                                     GTK_WINDOW (parent),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Open"),   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &dialog);
      g_signal_connect (dialog, "response",
                        G_CALLBACK (ifsfile_load_response),
                        NULL);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
ifs_compose_new_callback (GtkAction *action,
                          gpointer   data)
{
  GtkAllocation  allocation;
  GimpRGB        color;
  gint           i;
  AffElement    *elem;

  gtk_widget_get_allocation (ifsDesign->area, &allocation);

  undo_begin ();

  gimp_context_get_foreground (&color);

  elem = aff_element_new (0.5, 0.5 * allocation.height / allocation.width,
                          &color,
                          ++count_for_naming);

  ifsvals.num_elements++;
  elements = g_realloc (elements, ifsvals.num_elements * sizeof (AffElement *));
  element_selected = g_realloc (element_selected,
                                ifsvals.num_elements * sizeof (gboolean));

  for (i = 0; i < ifsvals.num_elements-1; i++)
    element_selected[i] = FALSE;
  element_selected[ifsvals.num_elements-1] = TRUE;

  elements[ifsvals.num_elements-1] = elem;
  set_current_element (ifsvals.num_elements-1);

  ifsD->selected_orig = g_realloc (ifsD->selected_orig,
                                  ifsvals.num_elements * sizeof(AffElement));
  aff_element_compute_trans (elem,
                             allocation.width, allocation.height,
                             ifsvals.center_x, ifsvals.center_y);

  design_area_redraw ();

  ifs_compose_preview ();

  design_op_actions_update ();
}

static void
ifs_compose_delete_callback (GtkAction *action,
                             gpointer   data)
{
  gint i;
  gint new_current;

  undo_begin ();
  undo_update (ifsD->current_element);

  aff_element_free (elements[ifsD->current_element]);

  if (ifsD->current_element < ifsvals.num_elements-1)
    {
      undo_update (ifsvals.num_elements-1);
      elements[ifsD->current_element] = elements[ifsvals.num_elements-1];
      new_current = ifsD->current_element;
    }
  else
    new_current = ifsvals.num_elements-2;

  ifsvals.num_elements--;

  for (i = 0; i < ifsvals.num_elements; i++)
    if (element_selected[i])
      {
        new_current = i;
        break;
      }

  element_selected[new_current] = TRUE;
  set_current_element (new_current);

  design_area_redraw ();

  ifs_compose_preview ();

  design_op_actions_update ();
}

static void
ifs_compose_options_callback (GtkAction *action,
                              gpointer   data)
{
  ifs_options_dialog (GTK_WIDGET (data));
}

static gint
preview_idle_render (gpointer data)
{
  GtkAllocation allocation;
  gint          iterations = PREVIEW_RENDER_CHUNK;
  gint          i;

  gtk_widget_get_allocation (ifsDesign->area, &allocation);

  if (iterations > ifsD->preview_iterations)
    iterations = ifsD->preview_iterations;

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_compute_trans (elements[i],
                               allocation.width, allocation.height,
                               ifsvals.center_x, ifsvals.center_y);

  ifs_render (elements, ifsvals.num_elements,
              allocation.width, allocation.height,
              iterations,&ifsvals, 0, allocation.height,
              ifsD->preview_data, NULL, NULL, TRUE);

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_compute_trans (elements[i],
                               allocation.width, allocation.height,
                               ifsvals.center_x, ifsvals.center_y);

  ifsD->preview_iterations -= iterations;

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (ifsD->preview),
                          0, 0, allocation.width, allocation.height,
                          GIMP_RGB_IMAGE,
                          ifsD->preview_data,
                          allocation.width * 3);

  return (ifsD->preview_iterations != 0);
}

static void
ifs_compose_preview (void)
{
  /* Expansion isn't really supported for previews */
  gint     i;
  gint     width  = ifsD->preview_width;
  gint     height = ifsD->preview_height;
  guchar   rc, gc, bc;
  guchar  *ptr;
  GimpRGB  color;

  if (!ifsD->preview_data)
    ifsD->preview_data = g_new (guchar, 3 * width * height);

  gimp_context_get_background (&color);
  gimp_rgb_get_uchar (&color, &rc, &gc, &bc);

  ptr = ifsD->preview_data;
  for (i = 0; i < width * height; i++)
    {
      *ptr++ = rc;
      *ptr++ = gc;
      *ptr++ = bc;
    }

  if (ifsD->preview_iterations == 0)
    g_idle_add (preview_idle_render, NULL);

  ifsD->preview_iterations =
    ifsvals.iterations * ((gdouble) width * height /
                          (ifsD->drawable_width * ifsD->drawable_height));
}

static void
ifs_compose_response (GtkWidget *widget,
                      gint       response_id,
                      gpointer   data)
{
  switch (response_id)
    {
    case RESPONSE_OPEN:
      ifs_compose_load (widget);
      break;

    case RESPONSE_SAVE:
      ifs_compose_save (widget);
      break;

    case RESPONSE_RESET:
      {
        GtkAllocation allocation;
        gint          i;

        gtk_widget_get_allocation (ifsDesign->area, &allocation);

        undo_begin ();
        for (i = 0; i < ifsvals.num_elements; i++)
          undo_update (i);

        ifs_compose_set_defaults ();

        ifs_compose_preview ();

        for (i = 0; i < ifsvals.num_elements; i++)
          aff_element_compute_trans (elements[i],
                                     allocation.width, allocation.height,
                                     ifsvals.center_x, ifsvals.center_y);

        design_area_redraw ();
        design_op_actions_update ();
      }
      break;

    case GTK_RESPONSE_OK:
      ifscint.run = TRUE;

    default:
      gtk_widget_destroy (widget);
      break;
    }
}
