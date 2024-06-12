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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

  ValuePairType  type;
  guint          timeout_id;

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
  GeglColor *color;
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
  GtkWidget       *area;
  cairo_surface_t *surface;

  DesignOp         op;
  gdouble          op_x;
  gdouble          op_y;
  gdouble          op_xcenter;
  gdouble          op_ycenter;
  gdouble          op_center_x;
  gdouble          op_center_y;
  guint            button_state;
  gint             num_selected;
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


struct _GimpIfs
{
  GimpPlugIn parent_instance;

  GtkApplication *app;
  GtkWidget      *dialog;
  GimpDrawable   *drawable;
  gint            response_id;

  GtkWidget      *delete_button;
  GtkWidget      *undo_button;
  GtkWidget      *redo_button;

  GtkBuilder     *builder;
};


#define GIMP_TYPE_IFS (gimp_ifs_get_type ())
G_DECLARE_FINAL_TYPE (GimpIfs, gimp_ifs, GIMP, IFS, GimpPlugIn)

GType                   ifs_get_type         (void) G_GNUC_CONST;

static void             gimp_ifs_finalize    (GObject              *object);

static GList          * ifs_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * ifs_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * ifs_run              (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              gint                  n_drawables,
                                              GimpDrawable        **drawables,
                                              GimpProcedureConfig  *config,
                                              gpointer              run_data);

/*  user interface functions  */
static void           on_app_activate             (GApplication *gapp,
                                                   gpointer      user_data);
static gint           ifs_compose_dialog          (GimpIfs      *ifs,
                                                   GimpDrawable *drawable);
static void           ifs_options_dialog          (void);
static GtkWidget    * ifs_compose_trans_page      (void);
static GtkWidget    * ifs_compose_color_page      (void);
static void           design_op_actions_update    (void);
static void           design_area_create          (GimpIfs      *ifs,
                                                   GtkWidget    *window,
                                                   gint          design_width,
                                                   gint          design_height);

/* functions for drawing design window */
static void update_values                   (void);
static void set_current_element             (gint               index);
static void design_area_realize             (GtkWidget         *widget);
static gint design_area_draw                (GtkWidget         *widget,
                                             cairo_t           *cr);
static gint design_area_button_press        (GtkWidget         *widget,
                                             GdkEventButton    *event,
                                             GimpIfs           *ifs);
static gint design_area_button_release      (GtkWidget         *widget,
                                             GdkEventButton    *event);
static void design_area_select_all_action   (GSimpleAction     *action,
                                             GVariant          *parameter,
                                             gpointer           user_data);
static void design_area_size_allocate       (GtkWidget         *widget,
                                             GtkAllocation     *allocation);
static gint design_area_motion              (GtkWidget         *widget,
                                             GdkEventMotion    *event);
static void design_area_redraw              (void);

/* Undo ring functions */
static void undo_begin    (void);
static void undo_update   (gint             element);
static void undo_exchange (gint             el);
static void undo_action   (GSimpleAction   *action,
                           GVariant        *parameter,
                           gpointer         user_data);
static void redo_action   (GSimpleAction   *action,
                           GVariant        *parameter,
                           gpointer         user_data);

static void recompute_center              (gboolean       save_undo);
static void recompute_center_action       (GSimpleAction *action,
                                           GVariant      *parameter,
                                           gpointer       user_data);

static void ifs_compose                   (GimpDrawable *drawable);

static ColorMap *color_map_create         (const gchar  *name,
                                           GeglColor    *orig_color,
                                           GeglColor    *data,
                                           gboolean      fixed_point);
static void      color_map_free           (ColorMap     *cmap);
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

static void flip_check_button_callback    (GtkWidget *widget, gpointer data);
static gint preview_idle_render           (gpointer   data);

static void transform_change_state        (GSimpleAction   *action,
                                           GVariant        *new_state,
                                           gpointer         user_data);
static void ifs_compose_preview           (void);
static void ifs_compose_set_defaults      (void);
static void ifs_compose_new_action        (GSimpleAction   *action,
                                           GVariant        *parameter,
                                           gpointer         user_data);
static void ifs_compose_delete_action     (GSimpleAction   *action,
                                           GVariant        *parameter,
                                           gpointer         user_data);
static void ifs_compose_options_action    (GSimpleAction   *action,
                                           GVariant        *parameter,
                                           gpointer         user_data);
static void ifs_compose_load              (void);
static void ifs_compose_save              (void);
static void ifs_compose_response          (GtkWidget       *widget,
                                           gint             response_id);

/* GAction helper methods */
static void        window_destroy         (GtkWidget       *widget,
                                           GimpIfs         *ifs);

static GtkWidget * add_tool_button        (GtkWidget       *toolbar,
                                           const char      *action,
                                           const char      *icon,
                                           const char      *label,
                                           const char      *tooltip);
static GtkWidget * add_toggle_button      (GtkWidget       *toolbar,
                                           const char      *action,
                                           const char      *icon,
                                           const char      *label,
                                           const char      *tooltip);
static void        add_tool_separator     (GtkWidget       *toolbar,
                                           gboolean         expand);


G_DEFINE_TYPE (GimpIfs, gimp_ifs, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GIMP_TYPE_IFS)
DEFINE_STD_SET_I18N


static IfsDialog        *ifsD       = NULL;
static IfsOptionsDialog *ifsOptD    = NULL;
static IfsDesignArea    *ifsDesign  = NULL;
static GimpIfs          *ifs        = NULL;

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

static const GActionEntry ACTIONS[] =
{
  { "new", ifs_compose_new_action },
  { "delete", ifs_compose_delete_action },
  { "undo", undo_action },
  { "redo", redo_action },
  { "select-all", design_area_select_all_action },
  { "center", recompute_center_action },
  { "options", ifs_compose_options_action },

  /* RadioButtons - only the default state is shown here. */
  { "transform", transform_change_state, "s", "'move'", NULL },
};


static void
gimp_ifs_class_init (GimpIfsClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);
  GObjectClass    *object_class  = G_OBJECT_CLASS (klass);

  object_class->finalize          = gimp_ifs_finalize;

  plug_in_class->query_procedures = ifs_query_procedures;
  plug_in_class->create_procedure = ifs_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
gimp_ifs_init (GimpIfs *ifs)
{
  ifs->builder = NULL;
}

static void
gimp_ifs_finalize (GObject *object)
{
  GimpIfs *ifs = GIMP_IFS (object);

  G_OBJECT_CLASS (gimp_ifs_parent_class)->finalize (object);

  g_clear_object (&ifs->builder);
}

static GList *
ifs_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
ifs_create_procedure (GimpPlugIn  *plug_in,
                           const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            ifs_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_IFS Fractal..."));
      gimp_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Render/Fractals");

      gimp_procedure_set_documentation
        (procedure,
         _("Create an Iterated Function System (IFS) fractal"),
         "Interactively create an Iterated Function System "
         "fractal. Use the window on the upper left to adjust"
         "the component transformations of the fractal. The "
         "operation that is performed is selected by the "
         "buttons underneath the window, or from a menu "
         "popped up by the right mouse button. The fractal "
         "will be rendered with a transparent background if "
         "the current image has an alpha channel.",
         name);
      gimp_procedure_set_attribution (procedure,
                                      "Owen Taylor",
                                      "Owen Taylor",
                                      "1997");

      gimp_procedure_add_string_aux_argument (procedure, "fractal-str",
                                              "The fractal description serialized as string",
                                              NULL, NULL,
                                              GIMP_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
ifs_run (GimpProcedure        *procedure,
         GimpRunMode           run_mode,
         GimpImage            *image,
         gint                  n_drawables,
         GimpDrawable        **drawables,
         GimpProcedureConfig  *config,
         gpointer              run_data)
{
  GimpDrawable *drawable;
  GimpParasite *parasite = NULL;
  gboolean      found_parasite = FALSE;

  ifs = GIMP_IFS (gimp_procedure_get_plug_in (procedure));
#if GLIB_CHECK_VERSION(2,74,0)
  ifs->app = gtk_application_new (NULL, G_APPLICATION_DEFAULT_FLAGS);
#else
  ifs->app = gtk_application_new (NULL, G_APPLICATION_FLAGS_NONE);
#endif

  ifs->builder = gtk_builder_new_from_resource ("/org/gimp/ifs/ifs-menus.ui");

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   gimp_procedure_get_name (procedure));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data; first look for a parasite -
       *  if not found, fall back to global values
       */
      parasite = gimp_item_get_parasite (GIMP_ITEM (drawable),
                                         PLUG_IN_PARASITE);
      if (parasite)
        {
          gchar   *parasite_data;
          guint32  parasite_size;

          parasite_data = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
          parasite_data = g_strndup (parasite_data, parasite_size);
          found_parasite = ifsvals_parse_string (parasite_data, &ifsvals, &elements);
          gimp_parasite_free (parasite);
          g_free (parasite_data);
        }

      if (! found_parasite)
        {
          gchar *data = NULL;

          g_object_get (config, "fractal-str", &data, NULL);
          if (data != NULL && strlen (data) > 0)
            ifsvals_parse_string (data, &ifsvals, &elements);
          g_free (data);
        }

      /* after ifsvals_parse_string, need to set up naming */
      count_for_naming = ifsvals.num_elements;

      ifs->drawable = drawable;

      /*  First acquire information with a dialog  */
      g_signal_connect (ifs->app, "activate", G_CALLBACK (on_app_activate), ifs);
      g_application_run (G_APPLICATION (ifs->app), 0, NULL);
      g_clear_object (&ifs->app);

      if (! ifscint.run)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               NULL);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
        {
          gchar *data = NULL;

          g_object_get (config, "fractal-str", &data, NULL);
          if (data != NULL && strlen (data) > 0)
            {
              ifsvals_parse_string (data, &ifsvals, &elements);
            }
          else
            {
              /* FIXME: there is a known crash in this code path, because some
               * base structures (which are visibly created as part of the
               * dialog or with ifsvals_parse_string() on a valid serialized
               * fractal) don't exist. This should be fixed so that we can run
               * with last vals even when no last vals exist (which should use
               * defaults).
               */
              ifs_compose_set_defaults ();
            }

          g_free (data);
        }
      break;

    default:
      break;
    }

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gchar        *str;
      GimpParasite *parasite;

      gimp_image_undo_group_start (image);

      /*  run the effect  */
      ifs_compose (drawable);

      /*  Store data for next invocation - both globally and
       *  as a parasite on this layer
       */
      str = ifsvals_stringify (&ifsvals, elements);
      g_object_set (config, "fractal-str", str, NULL);

      parasite = gimp_parasite_new (PLUG_IN_PARASITE,
                                    GIMP_PARASITE_PERSISTENT |
                                    GIMP_PARASITE_UNDOABLE,
                                    strlen (str) + 1, str);
      gimp_item_attach_parasite (GIMP_ITEM (drawable), parasite);
      gimp_parasite_free (parasite);

      g_free (str);

      gimp_image_undo_group_end (image);

      gimp_displays_flush ();
    }
  else
    {
      /*  run the effect  */
      ifs_compose (drawable);
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static GtkWidget *
ifs_compose_trans_page (void)
{
  GtkWidget *vbox;
  GtkWidget *grid;
  GtkWidget *label;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 12);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /* X */

  label = gtk_label_new (_("X:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
                   // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->x_pair = value_pair_create (&ifsD->current_vals.x, 0.0, 1.0, FALSE,
                                    VALUE_PAIR_DOUBLE);
  gtk_grid_attach (GTK_GRID (grid), ifsD->x_pair->spin, 1, 0, 1, 1);
                   // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->x_pair->spin);

  /* Y */

  label = gtk_label_new (_("Y:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->y_pair = value_pair_create (&ifsD->current_vals.y, 0.0, 1.0, FALSE,
                                    VALUE_PAIR_DOUBLE);
  gtk_grid_attach (GTK_GRID (grid), ifsD->y_pair->spin, 1, 1, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->y_pair->spin);

  /* Scale */

  label = gtk_label_new (_("Scale:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 0, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->scale_pair = value_pair_create (&ifsD->current_vals.scale, 0.0, 1.0,
                                        FALSE, VALUE_PAIR_DOUBLE);
  gtk_grid_attach (GTK_GRID (grid), ifsD->scale_pair->spin, 3, 0, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->scale_pair->spin);

  /* Angle */

  label = gtk_label_new (_("Angle:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 1, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->angle_pair = value_pair_create (&ifsD->current_vals.theta, -180, 180,
                                        FALSE, VALUE_PAIR_DOUBLE);
  gtk_grid_attach (GTK_GRID (grid), ifsD->angle_pair->spin, 3, 1, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->angle_pair->spin);

  /* Asym */

  label = gtk_label_new (_("Asymmetry:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 4, 0, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->asym_pair = value_pair_create (&ifsD->current_vals.asym, 0.10, 10.0,
                                       FALSE, VALUE_PAIR_DOUBLE);
  gtk_grid_attach (GTK_GRID (grid), ifsD->asym_pair->spin, 5, 0, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->asym_pair->spin);

  /* Shear */

  label = gtk_label_new (_("Shear:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 4, 1, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->shear_pair = value_pair_create (&ifsD->current_vals.shear, -10.0, 10.0,
                                        FALSE, VALUE_PAIR_DOUBLE);
  gtk_grid_attach (GTK_GRID (grid), ifsD->shear_pair->spin, 5, 1, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->shear_pair->spin);

  /* Flip */

  ifsD->flip_check_button = gtk_check_button_new_with_label (_("Flip"));
  gtk_grid_attach (GTK_GRID (grid), ifsD->flip_check_button, 0, 2, 6, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
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
  GtkWidget *grid;
  GtkWidget *label;
  GSList    *group = NULL;
  GeglColor *color;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /* Simple color control section */

  ifsD->simple_button = gtk_radio_button_new_with_label (group, _("Simple"));
  gtk_grid_attach (GTK_GRID (grid), ifsD->simple_button, 0, 0, 1, 2);
                    // GTK_FILL, GTK_FILL, 0, 0);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (ifsD->simple_button));
  g_signal_connect (ifsD->simple_button, "toggled",
                    G_CALLBACK (simple_color_toggled),
                    NULL);
  gtk_widget_show (ifsD->simple_button);

  ifsD->target_cmap = color_map_create (_("IFS Fractal: Target"), NULL,
                                        ifsD->current_vals.target_color, TRUE);
  gtk_grid_attach (GTK_GRID (grid), ifsD->target_cmap->hbox, 1, 0, 1, 2);
                    // GTK_FILL, 0, 0, 0);
  gtk_widget_show (ifsD->target_cmap->hbox);

  label = gtk_label_new (_("Scale hue by:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 0, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->hue_scale_pair = value_pair_create (&ifsD->current_vals.hue_scale,
                                            0.0, 1.0, TRUE, VALUE_PAIR_DOUBLE);
  gtk_grid_attach (GTK_GRID (grid), ifsD->hue_scale_pair->scale, 3, 0, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->hue_scale_pair->scale);
  gtk_grid_attach (GTK_GRID (grid), ifsD->hue_scale_pair->spin, 4, 0, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->hue_scale_pair->spin);

  label = gtk_label_new (_("Scale value by:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 1, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->value_scale_pair = value_pair_create (&ifsD->current_vals.value_scale,
                                              0.0, 1.0, TRUE, VALUE_PAIR_DOUBLE);
  gtk_grid_attach (GTK_GRID (grid), ifsD->value_scale_pair->scale, 3, 1, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->value_scale_pair->scale);
  gtk_grid_attach (GTK_GRID (grid), ifsD->value_scale_pair->spin, 4, 1, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->value_scale_pair->spin);

  /* Full color control section */

  ifsD->full_button = gtk_radio_button_new_with_label (group, _("Full"));
  gtk_grid_attach (GTK_GRID (grid), ifsD->full_button, 0, 2, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (ifsD->full_button));
  gtk_widget_show (ifsD->full_button);

  color = gegl_color_new ("red");
  ifsD->red_cmap = color_map_create (_("IFS Fractal: Red"), color,
                                     ifsD->current_vals.red_color, FALSE);
  gtk_grid_attach (GTK_GRID (grid), ifsD->red_cmap->hbox, 1, 2, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->red_cmap->hbox);
  g_object_unref (color);

  color = gegl_color_new ("green");
  ifsD->green_cmap = color_map_create (_("IFS Fractal: Green"), color,
                                       ifsD->current_vals.green_color, FALSE);
  gtk_grid_attach (GTK_GRID (grid), ifsD->green_cmap->hbox, 2, 2, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->green_cmap->hbox);
  g_object_unref (color);

  color = gegl_color_new ("blue");
  ifsD->blue_cmap = color_map_create (_("IFS Fractal: Blue"), color,
                                      ifsD->current_vals.blue_color, FALSE);
  gtk_grid_attach (GTK_GRID (grid), ifsD->blue_cmap->hbox, 3, 2, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->blue_cmap->hbox);
  g_object_unref (color);

  color = gegl_color_new ("black");
  ifsD->black_cmap = color_map_create (_("IFS Fractal: Black"), color,
                                       ifsD->current_vals.black_color, FALSE);
  gtk_grid_attach (GTK_GRID (grid), ifsD->black_cmap->hbox, 4, 2, 1, 1);
                    // GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->black_cmap->hbox);
  g_object_unref (color);

  return vbox;
}

static void
on_app_activate (GApplication *gapp,
                 gpointer      user_data)
{
  GimpIfs        *ifs = GIMP_IFS (user_data);

  ifs_compose_dialog (ifs, ifs->drawable);

  gtk_application_set_accels_for_action (ifs->app, "app.transform::move", (const char*[]) { "M", NULL });
  gtk_application_set_accels_for_action (ifs->app, "app.transform::rotate", (const char*[]) { "R", NULL });
  gtk_application_set_accels_for_action (ifs->app, "app.transform::stretch", (const char*[]) { "S", NULL });

  gtk_application_set_accels_for_action (ifs->app, "app.new", (const char*[]) { "<control>N", NULL });
  gtk_application_set_accels_for_action (ifs->app, "app.delete", (const char*[]) { "<control>D", NULL });
  gtk_application_set_accels_for_action (ifs->app, "app.undo", (const char*[]) { "<control>Z", NULL });
  gtk_application_set_accels_for_action (ifs->app, "app.redo", (const char*[]) { "<control>Y", NULL });
  gtk_application_set_accels_for_action (ifs->app, "app.select-all", (const char*[]) { "<control>A", NULL });
  gtk_application_set_accels_for_action (ifs->app, "app.center", (const char*[]) {"<control>C", NULL });

  gtk_application_set_accels_for_action (ifs->app, "app.options", (const char*[]) { NULL });
}

static gint
ifs_compose_dialog (GimpIfs      *ifs,
                    GimpDrawable *drawable)
{
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *main_vbox;
  GtkWidget *toolbar;
  GtkWidget *aspect_frame;
  GtkWidget *notebook;
  GtkWidget *page;
  gint       design_width  = gimp_drawable_get_width  (drawable);
  gint       design_height = gimp_drawable_get_height (drawable);

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

  ifsD->drawable_width  = gimp_drawable_get_width  (drawable);
  ifsD->drawable_height = gimp_drawable_get_height (drawable);
  ifsD->preview_width   = design_width;
  ifsD->preview_height  = design_height;

  gimp_ui_init (PLUG_IN_BINARY);

  ifs->dialog = gimp_dialog_new (_("IFS Fractal"), PLUG_IN_ROLE,
                                 NULL, 0,
                                 gimp_standard_help_func, PLUG_IN_PROC,

                                 _("_Open"),   RESPONSE_OPEN,
                                 _("_Save"),   RESPONSE_SAVE,
                                 _("_Reset"),  RESPONSE_RESET,
                                 _("_Cancel"), GTK_RESPONSE_CANCEL,
                                 _("_OK"),     GTK_RESPONSE_OK,

                                 NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (ifs->dialog),
                                           RESPONSE_OPEN,
                                           RESPONSE_SAVE,
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_application (GTK_WINDOW (ifs->dialog), ifs->app);
  gimp_window_set_transient (GTK_WINDOW (ifs->dialog));

  g_object_add_weak_pointer (G_OBJECT (ifs->dialog), (gpointer) &ifs->dialog);

  g_signal_connect (ifs->dialog, "response",
                    G_CALLBACK (ifs_compose_response),
                    NULL);
  g_signal_connect (GTK_WINDOW (ifs->dialog), "destroy",
                    G_CALLBACK (window_destroy),
                    NULL);

  g_action_map_add_action_entries (G_ACTION_MAP (ifs->app),
                                   ACTIONS, G_N_ELEMENTS (ACTIONS),
                                   ifs);

  /*  The main vbox */
  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (ifs->dialog))),
                      main_vbox, TRUE, TRUE, 0);

  toolbar = gtk_toolbar_new ();
  add_toggle_button (toolbar, "app.transform::move", GIMP_ICON_TOOL_MOVE,
                     NULL, _("Move"));
  add_toggle_button (toolbar, "app.transform::rotate", GIMP_ICON_TOOL_ROTATE,
                     NULL, _("Rotate"));
  add_toggle_button (toolbar, "app.transform::stretch", GIMP_ICON_TOOL_PERSPECTIVE,
                     NULL, _("Stretch"));
  add_tool_separator (toolbar, FALSE);
  add_tool_button (toolbar, "app.new", GIMP_ICON_DOCUMENT_NEW,
                   NULL, _("New"));
  ifs->delete_button = add_tool_button (toolbar, "app.delete", GIMP_ICON_EDIT_DELETE,
                       NULL, _("Delete"));
  ifs->undo_button = add_tool_button (toolbar, "app.undo", GIMP_ICON_EDIT_UNDO,
                       NULL, _("Undo"));
  ifs->redo_button = add_tool_button (toolbar, "app.redo", GIMP_ICON_EDIT_REDO,
                       NULL, _("Redo"));
  add_tool_button (toolbar, "app.select-all", GIMP_ICON_SELECTION_ALL,
                   NULL, _("Select All"));
  add_tool_button (toolbar, "app.center", GIMP_ICON_CENTER,
                   NULL, _("Recompute Center"));
  add_tool_separator (toolbar, FALSE);
  add_tool_button (toolbar, "app.options", GIMP_ICON_PREFERENCES_SYSTEM,
                   NULL, _("Render Options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  design_area_create (ifs, ifs->dialog, design_width, design_height);

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

  gtk_widget_show (GTK_WIDGET (ifs->dialog));

  ifs_compose_preview ();

  return ifscint.run;
}

static void
design_area_create (GimpIfs   *ifs,
                    GtkWidget *window,
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
  g_signal_connect (ifsDesign->area, "draw",
                    G_CALLBACK (design_area_draw),
                    NULL);
  g_signal_connect (ifsDesign->area, "button-press-event",
                    G_CALLBACK (design_area_button_press),
                    ifs);
  g_signal_connect (ifsDesign->area, "button-release-event",
                    G_CALLBACK (design_area_button_release),
                    NULL);
  g_signal_connect (ifsDesign->area, "motion-notify-event",
                    G_CALLBACK (design_area_motion),
                    NULL);
  g_signal_connect (ifsDesign->area, "size-allocate",
                    G_CALLBACK (design_area_size_allocate),
                    NULL);
  gtk_widget_set_events (ifsDesign->area,
                         GDK_EXPOSURE_MASK       |
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_POINTER_MOTION_HINT_MASK);

  design_op_actions_update ();
}

static void
design_op_actions_update (void)
{
  GAction *action;

  action = g_action_map_lookup_action (G_ACTION_MAP (ifs->app), "undo");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                               undo_cur >= 0);

  action = g_action_map_lookup_action (G_ACTION_MAP (ifs->app), "redo");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                               undo_cur != undo_num - 1);

  action = g_action_map_lookup_action (G_ACTION_MAP (ifs->app), "delete");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                              ifsvals.num_elements > 2);
}

static void
ifs_options_dialog (void)
{
  if (!ifsOptD)
    {
      GtkWidget *grid;
      GtkWidget *label;

      ifsOptD = g_new0 (IfsOptionsDialog, 1);

      ifsOptD->dialog =
        gimp_dialog_new (_("IFS Fractal Render Options"), PLUG_IN_ROLE,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         _("_Close"), GTK_RESPONSE_CLOSE,

                         NULL);

      g_signal_connect (ifsOptD->dialog, "response",
                        G_CALLBACK (gtk_widget_hide),
                        NULL);

      /* Grid of options */

      grid = gtk_grid_new ();
      gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (ifsOptD->dialog))),
                          grid, FALSE, FALSE, 0);
      gtk_widget_show (grid);

      label = gtk_label_new (_("Max. memory:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
                        // GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      ifsOptD->memory_pair = value_pair_create (&ifsvals.max_memory,
                                                1, 1000000, FALSE,
                                                VALUE_PAIR_INT);
      gtk_grid_attach (GTK_GRID (grid), ifsOptD->memory_pair->spin, 1, 0, 1, 1);
                        // GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ifsOptD->memory_pair->spin);

      label = gtk_label_new (_("Iterations:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
                        // GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      ifsOptD->iterations_pair = value_pair_create (&ifsvals.iterations,
                                                    1, 10000000, FALSE,
                                                    VALUE_PAIR_INT);
      gtk_grid_attach (GTK_GRID (grid), ifsOptD->iterations_pair->spin,
                       1, 1, 1, 1);
                        // GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ifsOptD->iterations_pair->spin);
      gtk_widget_show (label);

      label = gtk_label_new (_("Subdivide:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
                        // GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      ifsOptD->subdivide_pair = value_pair_create (&ifsvals.subdivide,
                                                   1, 10, FALSE,
                                                   VALUE_PAIR_INT);
      gtk_grid_attach (GTK_GRID (grid), ifsOptD->subdivide_pair->spin,
                       1, 2, 1, 1);
                        // GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ifsOptD->subdivide_pair->spin);

      label = gtk_label_new (_("Spot radius:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);
                        // GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      ifsOptD->radius_pair = value_pair_create (&ifsvals.radius,
                                                0, 5, TRUE,
                                                VALUE_PAIR_DOUBLE);
      gtk_grid_attach (GTK_GRID (grid), ifsOptD->radius_pair->scale,
                       1, 3, 1, 1);
                        // GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ifsOptD->radius_pair->scale);
      gtk_grid_attach (GTK_GRID (grid), ifsOptD->radius_pair->spin, 2, 3, 1, 1);
                        // GTK_FILL, GTK_FILL, 0, 0);
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
ifs_compose (GimpDrawable *drawable)
{
  GeglBuffer *buffer = gimp_drawable_get_shadow_buffer (drawable);
  gint        width  = gimp_drawable_get_width (drawable);
  gint        height = gimp_drawable_get_height (drawable);
  gboolean    alpha  = gimp_drawable_has_alpha (drawable);
  const Babl *format;
  gint        num_bands;
  gint        band_height;
  gint        band_y;
  gint        band_no;
  gint        i, j;
  guchar     *data;
  guchar     *mask = NULL;
  guchar     *nhits;
  guchar      c[3];
  GeglColor  *color;

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

  color = gimp_context_get_background ();
  gegl_color_get_pixel (color, babl_format_with_space ("R'G'B' u8", NULL), c);
  g_object_unref (color);

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
                                       GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);
      roi = &iter->items[0].roi;

      while (gegl_buffer_iterator_next (iter))
        {
          guchar *destrow = iter->items[0].data;

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
                      *dest++ = (mtot * rtot + (255 - mtot) * c[0]) / 255;
                      *dest++ = (mtot * gtot + (255 - mtot) * c[1]) / 255;
                      *dest++ = (mtot * btot + (255 - mtot) * c[2]) / 255;
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

  gimp_drawable_merge_shadow (drawable, TRUE);
  gimp_drawable_update (drawable, 0, 0, width, height);
}

static void
update_values (void)
{
  ifsD->in_update = TRUE;

  g_clear_object (&ifsD->current_vals.red_color);
  g_clear_object (&ifsD->current_vals.green_color);
  g_clear_object (&ifsD->current_vals.blue_color);
  g_clear_object (&ifsD->current_vals.black_color);
  g_clear_object (&ifsD->current_vals.target_color);

  ifsD->current_vals = elements[ifsD->current_element]->v;
  ifsD->current_vals.theta       *= 180/G_PI;
  ifsD->current_vals.red_color    = gegl_color_duplicate (ifsD->current_vals.red_color);
  ifsD->current_vals.green_color  = gegl_color_duplicate (ifsD->current_vals.green_color);
  ifsD->current_vals.blue_color   = gegl_color_duplicate (ifsD->current_vals.blue_color);
  ifsD->current_vals.black_color  = gegl_color_duplicate (ifsD->current_vals.black_color);
  ifsD->current_vals.target_color = gegl_color_duplicate (ifsD->current_vals.target_color);

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
design_area_draw (GtkWidget *widget,
                  cairo_t   *cr)
{
  cairo_t       *design_cr;
  GtkAllocation  allocation;
  PangoLayout   *layout;
  GdkRGBA        black = { 0.0, 0.0, 0.0, 1.0 };
  GdkRGBA        white = { 1.0, 1.0, 1.0, 1.0 };
  gint           i;
  gint           cx, cy;

  gtk_widget_get_allocation (widget, &allocation);

  design_cr = cairo_create (ifsDesign->surface);

  gdk_cairo_set_source_rgba (design_cr, &white);
  cairo_paint (design_cr);

  cairo_set_line_join (design_cr, CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_cap (design_cr, CAIRO_LINE_CAP_ROUND);
  cairo_translate (design_cr, 0.5, 0.5);

  /* draw an indicator for the center */

  cx = ifsvals.center_x * allocation.width;
  cy = ifsvals.center_y * allocation.width;

  cairo_move_to (design_cr, cx - 10, cy);
  cairo_line_to (design_cr, cx + 10, cy);

  cairo_move_to (design_cr, cx, cy - 10);
  cairo_line_to (design_cr, cx, cy + 10);

  gdk_cairo_set_source_rgba (design_cr, &black);
  cairo_set_line_width (design_cr, 1.0);
  cairo_stroke (design_cr);

  layout = gtk_widget_create_pango_layout (widget, NULL);

  for (i = 0; i < ifsvals.num_elements; i++)
    {
      aff_element_draw (elements[i], element_selected[i],
                        allocation.width,
                        allocation.height,
                        design_cr,
                        &black,
                        layout);
    }

  g_object_unref (layout);

  cairo_destroy (design_cr);

  cairo_set_source_surface (cr, ifsDesign->surface, 0.0, 0.0);
  cairo_paint (cr);

  return FALSE;
}

static void
design_area_size_allocate (GtkWidget     *widget,
                           GtkAllocation *allocation)
{
  gint i;

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_compute_trans (elements[i],
                               allocation->width, allocation->height,
                               ifsvals.center_x, ifsvals.center_y);

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_compute_boundary (elements[i],
                                  allocation->width, allocation->height,
                                  elements, ifsvals.num_elements);

  if (ifsDesign->surface)
    cairo_surface_destroy (ifsDesign->surface);

  ifsDesign->surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
                                                   allocation->width,
                                                   allocation->height);
}

static gint
design_area_button_press (GtkWidget      *widget,
                          GdkEventButton *event,
                          GimpIfs        *ifs)
{
  GtkAllocation allocation;
  gint          i;
  gint          old_current;

  gtk_widget_get_allocation (ifsDesign->area, &allocation);

  if (gdk_event_triggers_context_menu ((GdkEvent *) event))
    {
      GtkWidget  *menu;
      GMenuModel *model;

      model = G_MENU_MODEL (gtk_builder_get_object (ifs->builder, "ifs-compose-menu"));
      menu = gtk_menu_new_from_model (model);

      if (GTK_IS_MENU_ITEM (menu))
        menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (menu));

      gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (ifsDesign->area), NULL);
      gtk_menu_popup_at_pointer (GTK_MENU (menu), (GdkEvent *) event);

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
undo_action (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  if (undo_cur >= 0)
    {
      undo_exchange ((undo_start + undo_cur) % UNDO_LEVELS);
      undo_cur--;
    }

  design_op_actions_update ();
}

static void
redo_action (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  if (undo_cur != undo_num - 1)
    {
      undo_cur++;
      undo_exchange ((undo_start + undo_cur) % UNDO_LEVELS);
    }

  design_op_actions_update ();
}

static void
design_area_select_all_action (GSimpleAction *action,
                               GVariant      *parameter,
                               gpointer       user_data)
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

  g_clear_object (&cur->v.red_color);
  g_clear_object (&cur->v.green_color);
  g_clear_object (&cur->v.blue_color);
  g_clear_object (&cur->v.black_color);
  g_clear_object (&cur->v.target_color);
  cur->v = ifsD->current_vals;
  cur->v.red_color    = gegl_color_duplicate (ifsD->red_cmap->color);
  cur->v.green_color  = gegl_color_duplicate (ifsD->green_cmap->color);
  cur->v.blue_color   = gegl_color_duplicate (ifsD->blue_cmap->color);
  cur->v.black_color  = gegl_color_duplicate (ifsD->black_cmap->color);
  cur->v.target_color = gegl_color_duplicate (ifsD->target_cmap->color);
  cur->v.theta       *= G_PI/180.0;
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
                  GeglColor   *orig_color,
                  GeglColor   *data,
                  gboolean     fixed_point)
{
  GtkWidget *frame;
  GtkWidget *arrow;
  ColorMap  *color_map = g_new (ColorMap, 1);

  if (data || orig_color)
    color_map->color = gegl_color_duplicate (data ? data : orig_color);
  else
    color_map->color = gegl_color_new ("black");
  gimp_color_set_alpha (color_map->color, 1.0);
  color_map->fixed_point = fixed_point;
  color_map->hbox        = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (color_map->hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  color_map->orig_preview = gimp_color_area_new (fixed_point ? color_map->color : orig_color,
                                                 GIMP_COLOR_AREA_FLAT, 0);
  gtk_drag_dest_unset (color_map->orig_preview);
  gtk_widget_set_size_request (color_map->orig_preview,
                               COLOR_SAMPLE_SIZE, COLOR_SAMPLE_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), color_map->orig_preview);
  gtk_widget_show (color_map->orig_preview);

  arrow = gtk_image_new_from_icon_name ("pan-end-symbolic",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (color_map->hbox), arrow, FALSE, FALSE, 0);
  gtk_widget_show (arrow);

  color_map->button = gimp_color_button_new (name,
                                             COLOR_SAMPLE_SIZE,
                                             COLOR_SAMPLE_SIZE,
                                             color_map->color,
                                             GIMP_COLOR_AREA_FLAT);
  gtk_box_pack_start (GTK_BOX (color_map->hbox), color_map->button,
                      FALSE, FALSE, 0);
  gtk_widget_show (color_map->button);

  g_signal_connect (color_map->button, "color-changed",
                    G_CALLBACK (color_map_color_changed_cb),
                    color_map);

  return color_map;
}

static void
color_map_free (ColorMap *cmap)
{
  g_object_unref (cmap->color);
  g_free (cmap);
}

static void
color_map_color_changed_cb (GtkWidget *widget,
                            ColorMap  *color_map)
{
  g_clear_object (&color_map->color);
  color_map->color = gimp_color_button_get_color (GIMP_COLOR_BUTTON (widget));

  if (ifsD->in_update)
    return;

  undo_begin ();
  undo_update (ifsD->current_element);

  g_clear_object (&elements[ifsD->current_element]->v.red_color);
  g_clear_object (&elements[ifsD->current_element]->v.green_color);
  g_clear_object (&elements[ifsD->current_element]->v.blue_color);
  g_clear_object (&elements[ifsD->current_element]->v.black_color);
  g_clear_object (&elements[ifsD->current_element]->v.target_color);
  elements[ifsD->current_element]->v = ifsD->current_vals;
  elements[ifsD->current_element]->v.theta       *= G_PI/180.0;
  elements[ifsD->current_element]->v.red_color    = gegl_color_duplicate (ifsD->red_cmap->color);
  elements[ifsD->current_element]->v.green_color  = gegl_color_duplicate (ifsD->green_cmap->color);
  elements[ifsD->current_element]->v.blue_color   = gegl_color_duplicate (ifsD->blue_cmap->color);
  elements[ifsD->current_element]->v.black_color  = gegl_color_duplicate (ifsD->black_cmap->color);
  elements[ifsD->current_element]->v.target_color = gegl_color_duplicate (ifsD->target_cmap->color);

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
    gimp_color_area_set_color (GIMP_COLOR_AREA (color_map->orig_preview), color_map->color);
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

  value_pair->adjustment = gtk_adjustment_new (1.0, lower, upper,
                                               (upper - lower) / 100,
                                               (upper - lower) / 10,
                                               0.0);
  value_pair->spin = gimp_spin_button_new (value_pair->adjustment, 1.0, 3);
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
recompute_center_action (GSimpleAction *action,
                         GVariant      *parameter,
                         gpointer       user_data)
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
  GeglColor *color;
  gint       i;

  color = gimp_context_get_foreground ();

  ifsvals.aspect_ratio =
    (gdouble)ifsD->drawable_height / ifsD->drawable_width;

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_free (elements[i]);
  count_for_naming = 0;

  ifsvals.num_elements = 3;
  elements = g_realloc (elements, ifsvals.num_elements * sizeof(AffElement *));
  element_selected = g_realloc (element_selected,
                                ifsvals.num_elements * sizeof(gboolean));

  elements[0] = aff_element_new (0.3, 0.37 * ifsvals.aspect_ratio, color,
                                 ++count_for_naming);
  element_selected[0] = FALSE;
  elements[1] = aff_element_new (0.7, 0.37 * ifsvals.aspect_ratio, color,
                                 ++count_for_naming);
  element_selected[1] = FALSE;
  elements[2] = aff_element_new (0.5, 0.7 * ifsvals.aspect_ratio, color,
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

  g_object_unref (color);
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
ifs_compose_save (void)
{
  static GtkWidget *dialog = NULL;

  if (! dialog)
    {
      dialog =
        gtk_file_chooser_dialog_new (_("Save as IFS Fractal file"),
                                     NULL,
                                     GTK_FILE_CHOOSER_ACTION_SAVE,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Save"),   GTK_RESPONSE_OK,

                                     NULL);

      gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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
ifs_compose_load (void)
{
  static GtkWidget *dialog = NULL;

  if (! dialog)
    {
      dialog =
        gtk_file_chooser_dialog_new (_("Open IFS Fractal file"),
                                     NULL,
                                     GTK_FILE_CHOOSER_ACTION_OPEN,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Open"),   GTK_RESPONSE_OK,

                                     NULL);

      gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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
ifs_compose_new_action (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
  GtkAllocation   allocation;
  GeglColor      *color;
  gint            i;
  AffElement     *elem;

  gtk_widget_get_allocation (ifsDesign->area, &allocation);

  undo_begin ();

  color = gimp_context_get_foreground ();

  elem = aff_element_new (0.5, 0.5 * allocation.height / allocation.width,
                          color, ++count_for_naming);
  g_object_unref (color);

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
ifs_compose_delete_action (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
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
ifs_compose_options_action (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data)
{
  ifs_options_dialog ();
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

static void transform_change_state (GSimpleAction *action,
                                    GVariant      *new_state,
                                    gpointer       user_data)
{
  gchar *str;

  str = g_strdup_printf ("%s",  g_variant_get_string (new_state, NULL));

  if (! strcmp (str, "move"))
    ifsDesign->op = OP_TRANSLATE;
  else if (! strcmp (str, "rotate"))
    ifsDesign->op = OP_ROTATE;
  else if (! strcmp (str, "stretch"))
    ifsDesign->op = OP_STRETCH;

  g_free (str);

  g_simple_action_set_state (action, new_state);

  /* cursor switch */
  if (gtk_widget_get_realized (ifsDesign->area))
    design_area_realize (ifsDesign->area);
}

static void
ifs_compose_preview (void)
{
  /* Expansion isn't really supported for previews */
  gint       i;
  gint       width  = ifsD->preview_width;
  gint       height = ifsD->preview_height;
  guchar    *ptr;
  GeglColor *color;
  guchar     c[3];

  if (!ifsD->preview_data)
    ifsD->preview_data = g_new (guchar, 3 * width * height);

  color = gimp_context_get_background ();
  gegl_color_get_pixel (color, babl_format_with_space ("R'G'B' u8", NULL), c);
  g_object_unref (color);

  ptr = ifsD->preview_data;
  for (i = 0; i < width * height; i++)
    {
      *ptr++ = c[0];
      *ptr++ = c[1];
      *ptr++ = c[2];
    }

  if (ifsD->preview_iterations == 0)
    g_idle_add (preview_idle_render, NULL);

  ifsD->preview_iterations =
    ifsvals.iterations * ((gdouble) width * height /
                          (ifsD->drawable_width * ifsD->drawable_height));
}

static void
ifs_compose_response (GtkWidget *widget,
                      gint       response_id)
{
  switch (response_id)
    {
    case RESPONSE_OPEN:
      ifs_compose_load ();
      break;

    case RESPONSE_SAVE:
      ifs_compose_save ();
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
      gtk_application_remove_window (ifs->app, GTK_WINDOW (ifs->dialog));
      break;
    }
}

static void
window_destroy (GtkWidget *widget,
                GimpIfs   *ifs)
{
  if (ifsOptD)
    gtk_widget_destroy (ifsOptD->dialog);

  color_map_free (ifsD->red_cmap);
  color_map_free (ifsD->green_cmap);
  color_map_free (ifsD->blue_cmap);
  color_map_free (ifsD->black_cmap);
  color_map_free (ifsD->target_cmap);
  g_free (ifsD);

  gtk_application_remove_window (ifs->app, GTK_WINDOW (ifs->dialog));
}

static GtkWidget *
add_tool_button (GtkWidget  *toolbar,
                 const char *action,
                 const char *icon,
                 const char *label,
                 const char *tooltip)
{
  GtkWidget   *tool_icon;
  GtkToolItem *tool_button;

  tool_icon = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (GTK_WIDGET (tool_icon));
  tool_button = gtk_tool_button_new (tool_icon, label);
  gtk_widget_show (GTK_WIDGET (tool_button));
  gtk_tool_item_set_tooltip_text (tool_button, tooltip);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (tool_button), action);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), tool_button, -1);

  return GTK_WIDGET (tool_button);
}

GtkWidget *
add_toggle_button (GtkWidget  *toolbar,
                   const char *action,
                   const char *icon,
                   const char *label,
                   const char *tooltip)
{
  GtkWidget   *tool_icon;
  GtkToolItem *toggle_tool_button;

  tool_icon = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (GTK_WIDGET (tool_icon));

  toggle_tool_button = gtk_toggle_tool_button_new ();
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (toggle_tool_button),
                                   tool_icon);
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (toggle_tool_button), label);
  gtk_widget_show (GTK_WIDGET (toggle_tool_button));
  gtk_tool_item_set_tooltip_text (toggle_tool_button, tooltip);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (toggle_tool_button), action);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toggle_tool_button, -1);

  return GTK_WIDGET (toggle_tool_button);
}

static void
add_tool_separator (GtkWidget *toolbar,
                    gboolean   expand)
{
  GtkToolItem *item;

  item = gtk_separator_tool_item_new ();
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
  gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
  gtk_tool_item_set_expand (item, expand);
  gtk_widget_show (GTK_WIDGET (item));
}
