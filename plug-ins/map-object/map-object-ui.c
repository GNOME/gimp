/**************************************************************/
/* Dialog creation and updaters, callbacks and event-handlers */
/**************************************************************/

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "arcball.h"
#include "map-object-ui.h"
#include "map-object-icons.h"
#include "map-object-image.h"
#include "map-object-apply.h"
#include "map-object-preview.h"
#include "map-object-main.h"

#include "libgimp/stdplugins-intl.h"


GtkWidget          *previewarea       = NULL;

static GtkWidget   *appwin            = NULL;
static GtkNotebook *options_note_book = NULL;

static GtkWidget *pointlightwid;
static GtkWidget *dirlightwid;

static GtkAdjustment *xadj, *yadj, *zadj;

static GtkWidget *sphere_page    = NULL;
static GtkWidget *box_page       = NULL;
static GtkWidget *cylinder_page  = NULL;

static guint left_button_pressed = FALSE;
static guint light_hit           = FALSE;


static gint preview_events             (GtkWidget           *area,
                                        GdkEvent            *event);

static void update_light_pos_entries   (void);

static void update_preview             (GimpProcedureConfig *config);
static void double_adjustment_update   (GtkAdjustment       *adjustment,
                                        gpointer             data);

static void toggle_update              (GtkWidget           *widget,
                                        gpointer             data);

static void lightmenu_callback         (GtkWidget           *widget,
                                        gpointer             data);

static void preview_callback           (GtkWidget           *widget,
                                        gpointer             data);


/******************************************************/
/* Update angle & position (redraw grid if necessary) */
/******************************************************/

static void
update_preview (GimpProcedureConfig *config)
{
  copy_from_config (config);

  if (mapvals.livepreview)
    compute_preview_image ();

  gtk_widget_queue_draw (previewarea);
}

static void
double_adjustment_update (GtkAdjustment *adjustment,
                          gpointer       data)
{
  gimp_double_adjustment_update (adjustment, data);

  if (mapvals.livepreview)
    compute_preview_image ();

  gtk_widget_queue_draw (previewarea);
}

static void
update_light_pos_entries (void)
{
  g_signal_handlers_block_by_func (xadj,
                                   double_adjustment_update,
                                   &mapvals.lightsource.position.x);
  gtk_adjustment_set_value (xadj,
                            mapvals.lightsource.position.x);
  g_signal_handlers_unblock_by_func (xadj,
                                     double_adjustment_update,
                                     &mapvals.lightsource.position.x);

  g_signal_handlers_block_by_func (yadj,
                                   double_adjustment_update,
                                   &mapvals.lightsource.position.y);
  gtk_adjustment_set_value (yadj,
                            mapvals.lightsource.position.y);
  g_signal_handlers_unblock_by_func (yadj,
                                     double_adjustment_update,
                                     &mapvals.lightsource.position.y);

  g_signal_handlers_block_by_func (zadj,
                                   double_adjustment_update,
                                   &mapvals.lightsource.position.z);
  gtk_adjustment_set_value (zadj,
                            mapvals.lightsource.position.z);
  g_signal_handlers_unblock_by_func (zadj,
                                     double_adjustment_update,
                                     &mapvals.lightsource.position.z);
}

/**********************/
/* Std. toggle update */
/**********************/

static void
toggle_update (GtkWidget *widget,
               gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  compute_preview_image ();
  gtk_widget_queue_draw (previewarea);
}

/*****************************************/
/* Main window light type menu callback. */
/*****************************************/

static void
lightmenu_callback (GtkWidget *widget,
                    gpointer   data)
{
  int light_type;
  GimpProcedureConfig *config = (GimpProcedureConfig *) data;

  light_type = gimp_procedure_config_get_choice_id (config, "light-type");

  if (light_type == POINT_LIGHT)
    {
      gtk_widget_set_visible (dirlightwid, FALSE);
      gtk_widget_set_visible (pointlightwid, TRUE);
    }
  else if (light_type == DIRECTIONAL_LIGHT)
    {
      gtk_widget_set_visible (dirlightwid, TRUE);
      gtk_widget_set_visible (pointlightwid, FALSE);
    }
  else
    {
      gtk_widget_set_visible (dirlightwid, FALSE);
      gtk_widget_set_visible (pointlightwid, FALSE);
    }

  if (mapvals.livepreview)
    {
      copy_from_config (config);
      compute_preview_image ();
      gtk_widget_queue_draw (previewarea);
    }
}

/***************************************/
/* Main window map type menu callback. */
/***************************************/

static void
mapmenu_callback (GtkWidget *widget,
                  gpointer   data)
{
  int map_type;
  GimpProcedureConfig *config = (GimpProcedureConfig *) data;

  map_type = gimp_procedure_config_get_choice_id (config, "map-type");

  if (mapvals.livepreview)
    {
      copy_from_config (config);
      compute_preview_image ();
      gtk_widget_queue_draw (previewarea);
    }

  gtk_widget_set_visible (sphere_page, FALSE);
  gtk_widget_set_visible (box_page, FALSE);
  gtk_widget_set_visible (cylinder_page, FALSE);

  if (map_type == MAP_SPHERE)
    gtk_widget_set_visible (sphere_page, TRUE);
  else if (map_type == MAP_BOX)
    gtk_widget_set_visible (box_page, TRUE);
  else if (map_type == MAP_CYLINDER)
    gtk_widget_set_visible (cylinder_page, TRUE);
}

/******************************************/
/* Main window "Preview!" button callback */
/******************************************/

static void
preview_callback (GtkWidget *widget,
                  gpointer   data)
{
  GimpProcedureConfig *config = (GimpProcedureConfig *) data;

  copy_from_config (config);
  compute_preview_image ();

  gtk_widget_queue_draw (previewarea);
}

static void
zoomed_callback (GimpZoomModel *model)
{
  mapvals.zoom = gimp_zoom_model_get_factor (model);

  compute_preview_image ();

  gtk_widget_queue_draw (previewarea);
}

/******************************/
/* Preview area event handler */
/******************************/

static gint
preview_events (GtkWidget *area,
                GdkEvent  *event)
{
  HVect __attribute__((unused))pos;
/*  HMatrix RotMat;
  gdouble a,b,c; */

  switch (event->type)
    {
      case GDK_ENTER_NOTIFY:
        break;

      case GDK_LEAVE_NOTIFY:
        break;

      case GDK_BUTTON_PRESS:
        light_hit = check_light_hit (event->button.x, event->button.y);
        if (light_hit == FALSE)
          {
            pos.x = -(2.0 * (gdouble) event->button.x /
                      (gdouble) PREVIEW_WIDTH - 1.0);
            pos.y = (2.0 * (gdouble) event->button.y /
                     (gdouble) PREVIEW_HEIGHT - 1.0);
            /*ArcBall_Mouse(pos);
            ArcBall_BeginDrag(); */
          }
        left_button_pressed = TRUE;
        break;

      case GDK_BUTTON_RELEASE:
        if (light_hit == TRUE)
          {
            compute_preview_image ();

            gtk_widget_queue_draw (previewarea);
          }
        else
          {
            pos.x = -(2.0 * (gdouble) event->button.x /
                      (gdouble) PREVIEW_WIDTH - 1.0);
            pos.y = (2.0 * (gdouble) event->button.y /
                     (gdouble) PREVIEW_HEIGHT - 1.0);
            /*ArcBall_Mouse(pos);
            ArcBall_EndDrag(); */
          }
        left_button_pressed = FALSE;
        break;

      case GDK_MOTION_NOTIFY:
        if (left_button_pressed == TRUE)
          {
            if (light_hit == TRUE)
              {
                gint live = mapvals.livepreview;

                mapvals.livepreview = FALSE;
                update_light (event->motion.x, event->motion.y);
                update_light_pos_entries ();
                mapvals.livepreview = live;

                gtk_widget_queue_draw (previewarea);
              }
            else
              {
                pos.x = -(2.0 * (gdouble) event->motion.x /
                          (gdouble) PREVIEW_WIDTH - 1.0);
                pos.y = (2.0 * (gdouble) event->motion.y /
                         (gdouble) PREVIEW_HEIGHT - 1.0);
/*                ArcBall_Mouse(pos);
                ArcBall_Update();
                ArcBall_Values(&a,&b,&c);
                Alpha+=RadToDeg(-a);
                Beta+RadToDeg(-b);
                Gamma+=RadToDeg(-c);
                if (Alpha>180) Alpha-=360;
                if (Alpha<-180) Alpha+=360;
                if (Beta>180) Beta-=360;
                if (Beta<-180) Beta+=360;
                if (Gamma>180) Gamma-=360;
                if (Gamma<-180) Gamma+=360;
                      UpdateAngleSliders(); */
              }
          }
        break;

      default:
        break;
    }

  return FALSE;
}

/********************************/
/* Create and show main dialog. */
/********************************/

gboolean
main_dialog (GimpProcedure       *procedure,
             GimpProcedureConfig *config,
             GimpDrawable        *drawable)
{
  GtkWidget     *main_hbox;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *frame;
  GtkWidget     *button;
  GtkWidget     *toggle;
  GimpZoomModel *model;
  GtkWidget     *map_combo;
  GtkWidget     *combo;
  gboolean       run = FALSE;

  gimp_ui_init (PLUG_IN_BINARY);

  appwin = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Map to Object"));

  /* Create the Preview */

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (vbox);

  /* Add preview widget and various buttons to the first part */

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_realize (appwin);
  previewarea = gtk_drawing_area_new ();
  gtk_widget_set_size_request (previewarea, PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_widget_set_events (previewarea, (GDK_EXPOSURE_MASK |
                                       GDK_BUTTON1_MOTION_MASK |
                                       GDK_BUTTON_PRESS_MASK |
                                       GDK_BUTTON_RELEASE_MASK));
  gtk_container_add (GTK_CONTAINER (frame), previewarea);
  gtk_widget_show (previewarea);

  g_signal_connect (previewarea, "event",
                    G_CALLBACK (preview_events),
                    previewarea);

  g_signal_connect (previewarea, "draw",
                    G_CALLBACK (preview_draw),
                    previewarea);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("_Preview!"));
  g_object_set (gtk_bin_get_child (GTK_BIN (button)),
                "margin-start", 2,
                "margin-end",   2,
                NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (preview_callback),
                    config);

  gimp_help_set_help_data (button, _("Recompute preview image"), NULL);

  model = gimp_zoom_model_new ();
  gimp_zoom_model_set_range (model, 0.25, 1.0);
  gimp_zoom_model_zoom (model, GIMP_ZOOM_TO, mapvals.zoom);

  button = gimp_zoom_button_new (model, GIMP_ZOOM_IN, GTK_ICON_SIZE_MENU);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_zoom_button_new (model, GIMP_ZOOM_OUT, GTK_ICON_SIZE_MENU);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (model, "zoomed",
                    G_CALLBACK (zoomed_callback),
                    NULL);

  toggle = gtk_check_button_new_with_mnemonic (_("Show _wireframe"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mapvals.showgrid);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (toggle_update),
                    &mapvals.showgrid);

  toggle = gtk_check_button_new_with_mnemonic (_("Update preview _live"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mapvals.livepreview);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (toggle_update),
                    &mapvals.livepreview);

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "options-tab", _("O_ptions"), FALSE, TRUE);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "light-tab", _("Li_ght"), FALSE, TRUE);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "viewpoint-tab", _("_Viewpoint"), FALSE, TRUE);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "material-tab", _("_Material"), FALSE, TRUE);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "orientation-tab", _("Orient_ation"),
                                   FALSE, TRUE);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "sphere-tab", _("Sp_here"), FALSE, TRUE);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "box-tab", _("_Box"), FALSE, TRUE);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "cylinder-tab", _("C_ylinder"), FALSE, TRUE);

  /* Options Tab */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "general-options", _("General Options"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin), "general-box",
                                  "map-type",
                                  "transparent-background",
                                  "tiled",
                                  "new-image",
                                  "new-layer",
                                  NULL);
  map_combo = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (appwin),
                                                "map-type", G_TYPE_NONE);
  g_signal_connect (map_combo, "value-changed",
                    G_CALLBACK (mapmenu_callback),
                    config);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (appwin),
                                    "options-frame",
                                    "general-options", FALSE,
                                    "general-box");
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "depth", 1.0);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin), "options-box",
                                  "options-frame",
                                  "antialiasing",
                                  "depth",
                                  "threshold",
                                  NULL);

  g_signal_connect (config, "notify::transparent-background",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::tiled",
                    G_CALLBACK (update_preview),
                    config);

  /* Light Tab */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "light-settings-label", _("Light Settings"),
                                   FALSE, FALSE);

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "light-settings-box",
                                  "light-type",
                                  "light-color",
                                  NULL);
  combo = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (appwin),
                                            "light-type", G_TYPE_NONE);
  g_signal_connect (combo, "value-changed",
                    G_CALLBACK (lightmenu_callback),
                    config);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (appwin),
                                    "light-frame",
                                    "light-settings-label", FALSE,
                                    "light-settings-box");

  /* Depending on light settings, only one of these are visible */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "vector-label", _("Direction Vector"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "vector-box",
                                  "light-direction-x",
                                  "light-direction-y",
                                  "light-direction-z",
                                  NULL);
  dirlightwid = gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (appwin),
                                                  "vector-frame",
                                                  "vector-label", FALSE,
                                                  "vector-box");

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "position-label", _("Position"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "position-box",
                                  "light-position-x",
                                  "light-position-y",
                                  "light-position-z",
                                  NULL);
  pointlightwid = gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (appwin),
                                                    "position-frame",
                                                    "position-label", FALSE,
                                                    "position-box");
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin), "light-box",
                                  "light-frame",
                                  "vector-frame",
                                  "position-frame",
                                  NULL);

  g_signal_connect (config, "notify::light-color",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::light-direction-x",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::light-direction-y",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::light-direction-z",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::light-position-x",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::light-position-y",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::light-position-z",
                    G_CALLBACK (update_preview),
                    config);
  lightmenu_callback (combo, config);

  /* Viewpoint Tab */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "viewpoint-position-label", _("Viewpoint Position"),
                                   FALSE, FALSE);
  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                         "viewpoint-position-box",
                                         "viewpoint-x",
                                         "viewpoint-y",
                                         "viewpoint-z",
                                         NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  pointlightwid = gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (appwin),
                                                    "viewpoint-position-frame",
                                                    "viewpoint-position-label", FALSE,
                                                    "viewpoint-position-box");

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "first-axis-x", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "first-axis-y", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "first-axis-z", 1.0);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "first-axis-label", _("First Axis"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "first-axis-box",
                                  "first-axis-x",
                                  "first-axis-y",
                                  "first-axis-z",
                                  NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (appwin),
                                    "first-axis-frame",
                                    "first-axis-label", FALSE,
                                    "first-axis-box");

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "second-axis-x", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "second-axis-y", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "second-axis-z", 1.0);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "second-axis-label", _("Second Axis"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "second-axis-box",
                                  "second-axis-x",
                                  "second-axis-y",
                                  "second-axis-z",
                                  NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (appwin),
                                    "second-axis-frame",
                                    "second-axis-label", FALSE,
                                    "second-axis-box");

  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                         "axis-box",
                                         "first-axis-frame",
                                         "second-axis-frame",
                                         NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "viewpoint-box",
                                  "viewpoint-position-frame",
                                  "axis-box",
                                  NULL);

  g_signal_connect (config, "notify::viewpoint-x",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::viewpoint-y",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::viewpoint-z",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::first-axis-x",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::first-axis-y",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::first-axis-z",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::second-axis-x",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::second-axis-y",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::second-axis-z",
                    G_CALLBACK (update_preview),
                    config);

  /* Material Tab */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "intensity-label", _("Intensity Levels"),
                                   FALSE, FALSE);
  /* TODO: Restore icons */
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "intensity-box",
                                  "ambient-intensity",
                                  "diffuse-intensity",
                                  NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (appwin),
                                    "intensity-frame",
                                    "intensity-label", FALSE,
                                    "intensity-box");

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "reflexivity-label", _("Intensity Levels"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "reflexivity-box",
                                  "diffuse-reflectivity",
                                  "specular-reflectivity",
                                  "highlight",
                                  NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (appwin),
                                    "reflexivity-frame",
                                    "reflexivity-label", FALSE,
                                    "reflexivity-box");

   gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "material-box",
                                  "intensity-frame",
                                  "reflexivity-frame",
                                  NULL);

  g_signal_connect (config, "notify::ambient-intensity",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::diffuse-intensity",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::diffuse-reflectivity",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::specular-reflectivity",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::highlight",
                    G_CALLBACK (update_preview),
                    config);

  /* Orientation Tab */
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "position-x", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "position-y", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "position-z", 1.0);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "orientation-position-label", _("Position"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "orientation-position-box",
                                  "position-x",
                                  "position-y",
                                  "position-z",
                                  NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (appwin),
                                    "orientation-position-frame",
                                    "orientation-position-label", FALSE,
                                    "orientation-position-box");

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "rotation-angle-x", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "rotation-angle-y", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "rotation-angle-z", 1.0);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "rotation-angle-label", _("Rotation"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "rotation-angle-box",
                                  "rotation-angle-x",
                                  "rotation-angle-y",
                                  "rotation-angle-z",
                                  NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (appwin),
                                    "rotation-angle-frame",
                                    "rotation-angle-label", FALSE,
                                    "rotation-angle-box");

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "orientation-box",
                                  "orientation-position-frame",
                                  "rotation-angle-frame",
                                  NULL);

  g_signal_connect (config, "notify::position-x",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::position-y",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::position-z",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::rotation-angle-x",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::rotation-angle-y",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::rotation-angle-z",
                    G_CALLBACK (update_preview),
                    config);

  /* Sphere Tab */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "sphere-label", _("Size"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (appwin),
                                    "sphere-frame",
                                    "sphere-label", FALSE,
                                    "sphere-radius");

  g_signal_connect (config, "notify::sphere-radius",
                    G_CALLBACK (update_preview),
                    config);

  /* Box Tab */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "box-drawable-label", _("Map Images to Box Faces"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_flowbox (GIMP_PROCEDURE_DIALOG (appwin),
                                      "box-drawable-box",
                                      "box-front-drawable",
                                      "box-back-drawable",
                                      "box-top-drawable",
                                      "box-bottom-drawable",
                                      "box-left-drawable",
                                      "box-right-drawable",
                                      NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (appwin),
                                    "box-drawable-frame",
                                    "box-drawable-label", FALSE,
                                    "box-drawable-box");

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "x-scale", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "y-scale", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "z-scale", 1.0);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "box-scale-box",
                                  "x-scale",
                                  "y-scale",
                                  "z-scale",
                                  NULL);

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "box-box",
                                  "box-drawable-frame",
                                  "box-scale-box",
                                  NULL);

  g_signal_connect (config, "notify::box-front-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::box-back-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::box-top-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::box-bottom-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::box-left-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::box-right-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::x-scale",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::y-scale",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::z-scale",
                    G_CALLBACK (update_preview),
                    config);

  /* Cylinder Tab */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "cyl-drawable-label", _("Images for the Cap Faces"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                      "cyl-drawable-box",
                                      "cyl-top-drawable",
                                      "cyl-bottom-drawable",
                                      NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (appwin),
                                    "cyl-drawable-frame",
                                    "cyl-drawable-label", FALSE,
                                    "cyl-drawable-box");

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "cylinder-radius", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (appwin),
                                        "cylinder-length", 1.0);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (appwin),
                                   "cyl-size-label", _("Size"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "cyl-size-box",
                                  "cylinder-radius",
                                  "cylinder-length",
                                  NULL);

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                  "cylinder-box",
                                  "cyl-drawable-frame",
                                  "cyl-size-box",
                                  NULL);

  g_signal_connect (config, "notify::cyl-top-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::cyl-bottom-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::cylinder-radius",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::cylinder-length",
                    G_CALLBACK (update_preview),
                    config);

  options_note_book =
    GTK_NOTEBOOK (gimp_procedure_dialog_fill_notebook (GIMP_PROCEDURE_DIALOG (appwin),
                                                       "main-notebook",
                                                       "options-tab", "options-box",
                                                       "light-tab", "light-box",
                                                       "viewpoint-tab", "viewpoint-box",
                                                       "orientation-tab", "orientation-box",
                                                       "material-tab", "material-box",
                                                       "sphere-tab", "sphere-frame",
                                                       "box-tab", "box-box",
                                                       "cylinder-tab", "cylinder-box",
                                                       NULL));

  /* Save reference to tabs for hiding/showing */
  sphere_page   = gtk_notebook_get_nth_page (options_note_book, 5);
  box_page      = gtk_notebook_get_nth_page (options_note_book, 6);
  cylinder_page = gtk_notebook_get_nth_page (options_note_book, 7);

  mapmenu_callback (map_combo, config);

  /* Create overall layout */
  main_hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (appwin),
                                              "main-hbox",
                                              "main-notebook",
                                              NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (main_hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (main_hbox), vbox, 0);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (appwin), "main-hbox", NULL);

  gtk_widget_show (appwin);

  {
    GdkCursor *cursor;

    cursor = gdk_cursor_new_for_display (gtk_widget_get_display (previewarea),
                                         GDK_HAND2);
    gdk_window_set_cursor (gtk_widget_get_window (previewarea), cursor);
    g_object_unref (cursor);
  }

  copy_from_config (config);
  image_setup (drawable, TRUE, config);

  compute_preview_image ();

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (appwin));

  gtk_widget_destroy (appwin);
  if (preview_rgb_data)
    g_free (preview_rgb_data);
  if (preview_surface)
    cairo_surface_destroy (preview_surface);

  return run;
}
