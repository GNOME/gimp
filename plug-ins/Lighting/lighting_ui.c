/**************************************************************/
/* Dialog creation and updaters, callbacks and event-handlers */
/**************************************************************/

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <gck/gck.h>

#include "lighting_ui.h"
#include "lighting_main.h"
#include "lighting_image.h"
#include "lighting_apply.h"
#include "lighting_preview.h"

#include "config.h"
#include "libgimp/stdplugins-intl.h"

#include "amb1.xpm"
#include "amb2.xpm"
#include "diffint1.xpm"
#include "diffint2.xpm"
#include "diffref1.xpm"
#include "diffref2.xpm"
#include "specref1.xpm"
#include "specref2.xpm"
#include "high1.xpm"
#include "high2.xpm"

#include "pixmaps/zoom_in.xpm"
#include "pixmaps/zoom_out.xpm"


extern LightingValues mapvals;

GckVisualInfo *visinfo = NULL;

static GtkWidget   *appwin            = NULL;
static GtkNotebook *options_note_book = NULL;
static GtkWidget   *bump_page         = NULL;
static GtkWidget   *env_page          = NULL;

GdkGC     *gc          = NULL;
GtkWidget *previewarea = NULL;

static GtkWidget *pointlightwid = NULL;
static GtkWidget *dirlightwid   = NULL;

static gint bump_page_pos = -1;
static gint env_page_pos  = -1;

static guint left_button_pressed = FALSE;
static guint light_hit           = FALSE;

static gboolean run = FALSE;


static void create_main_notebook      (GtkWidget *container);

static gint preview_events            (GtkWidget *area,
				       GdkEvent  *event);

#ifdef _LIGHTNING_UNUSED_CODE
static void xyzval_update             (GtkEntry *entry);
#endif

static void toggle_update             (GtkWidget *widget,
				       gpointer   data);

static void togglebump_update         (GtkWidget *widget,
				       gpointer   data);
static void toggleenvironment_update  (GtkWidget *widget,
				       gpointer   data);
static void toggletips_update         (GtkWidget *widget,
				       gpointer   data);

static void lightmenu_callback        (GtkWidget *widget,
				       gpointer   data);

static void preview_callback          (GtkWidget *widget);
static void apply_callback            (GtkWidget *widget);

static gint bumpmap_constrain         (gint32   image_id,
				       gint32   drawable_id,
				       gpointer data);
static void bumpmap_drawable_callback (gint32   id,
				       gpointer data);

static gint envmap_constrain          (gint32   image_id,
				       gint32   drawable_id,
				       gpointer data);
static void envmap_drawable_callback  (gint32   id,
				       gpointer data);

static GtkWidget *create_bump_page        (void);
static GtkWidget *create_environment_page (void);


#ifdef _LIGHTNING_UNUSED_CODE
/**********************************************************/
/* Update entry fields that affect the preview parameters */
/**********************************************************/

static void xyzval_update(GtkEntry *entry)
{
  gdouble *valueptr;
  gdouble value;

  valueptr=(gdouble *)gtk_object_get_data(GTK_OBJECT(entry),"ValuePtr");
  value = atof(gtk_entry_get_text(entry));

  *valueptr=value;
}
#endif

/**********************/
/* Std. toggle update */
/**********************/

static void
toggle_update (GtkWidget *widget,
	       gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  draw_preview_image (TRUE);
}

/*****************************/
/* Toggle bumpmapping update */
/*****************************/

static void
togglebump_update (GtkWidget *widget,
		   gpointer   data)
{
  gimp_toggle_button_update (widget, data);
  
  if (mapvals.bump_mapped)
    {
      bump_page_pos = g_list_length (options_note_book->children);

      bump_page = create_bump_page ();
      gtk_notebook_append_page (options_note_book, bump_page,
				gtk_label_new (_("Bump")));
    }
  else
    {
      gtk_notebook_remove_page (options_note_book, bump_page_pos);
      if (bump_page_pos < env_page_pos)
        env_page_pos--;
      bump_page_pos = 0;
    }
}

/*************************************/
/* Toggle environment mapping update */
/*************************************/

static void
toggleenvironment_update (GtkWidget *widget,
			  gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  if (mapvals.env_mapped)
    {
      env_page_pos = g_list_length (options_note_book->children);

      env_page = create_environment_page ();
      gtk_notebook_append_page (options_note_book, env_page,
				gtk_label_new (_("Env")));
    }
  else
    {
      gtk_notebook_remove_page (options_note_book, env_page_pos);
      if (env_page_pos < bump_page_pos)
        bump_page_pos--;
      env_page_pos = 0;
    }
}

/**************************/
/* Tooltips toggle update */
/**************************/

static void
toggletips_update (GtkWidget *widget,
		   gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  if (mapvals.tooltips_enabled)
    gimp_help_enable_tooltips ();
  else
    gimp_help_disable_tooltips ();
}

/*****************************************/
/* Main window light type menu callback. */
/*****************************************/

static void
lightmenu_callback (GtkWidget *widget,
		    gpointer   data)
{
  gimp_menu_item_update (widget, data);

  if (mapvals.lightsource.type == POINT_LIGHT)
    {
      gtk_widget_hide (dirlightwid);
      gtk_widget_show (pointlightwid);
    }
  else if (mapvals.lightsource.type == DIRECTIONAL_LIGHT)
    {
      gtk_widget_hide (pointlightwid);
      gtk_widget_show (dirlightwid);
    }
  else
    {
      gtk_widget_hide (pointlightwid);
      gtk_widget_hide (dirlightwid);
    }
}

static void
mapmenu2_callback (GtkWidget *widget,
		   gpointer   data)
{
  gimp_menu_item_update (widget, data);

  draw_preview_image (TRUE);
}

/******************************************/
/* Main window "Preview!" button callback */
/******************************************/

static void
preview_callback (GtkWidget *widget)
{
  draw_preview_image (TRUE);
}

/*********************************************/
/* Main window "-" (zoom in) button callback */
/*********************************************/

static void
zoomout_callback (GtkWidget *widget)
{
  mapvals.preview_zoom_factor *= 0.5;
  draw_preview_image (TRUE);
}

/*********************************************/
/* Main window "+" (zoom out) button callback */
/*********************************************/

static void
zoomin_callback (GtkWidget *widget)
{
  mapvals.preview_zoom_factor *= 2.0;
  draw_preview_image (TRUE);
}

/**********************************************/
/* Main window "Apply" button callback.       */ 
/* Render to GIMP image, close down and exit. */
/**********************************************/

static void
apply_callback (GtkWidget *widget)
{
  run = TRUE;

  gtk_main_quit ();
}

static gint
bumpmap_constrain (gint32   image_id,
		   gint32   drawable_id,
		   gpointer data)
{
  if (drawable_id == -1)
    return TRUE;

  return (gimp_drawable_is_gray (drawable_id) &&
	  !gimp_drawable_has_alpha (drawable_id) &&
          (gimp_drawable_width (drawable_id) ==
	   gimp_drawable_width (mapvals.drawable_id)) &&
          (gimp_drawable_height (drawable_id) ==
	   gimp_drawable_height (mapvals.drawable_id)));
}

static void
bumpmap_drawable_callback (gint32   id,
			   gpointer data)
{
  mapvals.bumpmap_id = id;
}

static gint
envmap_constrain (gint32   image_id,
		  gint32   drawable_id,
		  gpointer data)
{
  if (drawable_id == -1)
    return TRUE;

  return (!gimp_drawable_is_gray (drawable_id) &&
	  !gimp_drawable_has_alpha (drawable_id));
}

static void
envmap_drawable_callback (gint32   id,
			  gpointer data)
{
  mapvals.envmap_id = id;
  env_width = gimp_drawable_width (mapvals.envmap_id);
  env_height = gimp_drawable_height (mapvals.envmap_id);
}

/******************************/
/* Preview area event handler */
/******************************/

static gint
preview_events (GtkWidget *area,
		GdkEvent  *event)
{
  switch (event->type)
    {
      case GDK_EXPOSE:

        /* Is this the first exposure? */
        /* =========================== */

        if (!gc)
          {
            gc = gdk_gc_new (area->window);
            draw_preview_image (TRUE);
          }
        else
          draw_preview_image (FALSE);
        break; 
      case GDK_ENTER_NOTIFY:
        break;
      case GDK_LEAVE_NOTIFY:
        break;
      case GDK_BUTTON_PRESS:
        light_hit = check_light_hit (event->button.x, event->button.y);
        left_button_pressed = TRUE;
        break;
      case GDK_BUTTON_RELEASE:
        if (light_hit == TRUE)
          draw_preview_image (TRUE);
        left_button_pressed = FALSE;
        break;
      case GDK_MOTION_NOTIFY:
        if (left_button_pressed == TRUE && light_hit == TRUE)
          update_light (event->motion.x, event->motion.y);
        break;
      default:
        break;
    }

  return FALSE;
}

/***********************/
/* Dialog constructors */
/***********************/

static GtkWidget *
create_options_page (void)
{
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *toggle;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;

  page = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page), 4);

  /* General options */

  frame = gtk_frame_new (_("General Options"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  toggle = gtk_check_button_new_with_label (_("Use Bump Mapping"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.bump_mapped);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (togglebump_update),
		      &mapvals.bump_mapped);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Enable/disable bump-mapping (image depth)"),
			   NULL);

  toggle = gtk_check_button_new_with_label (_("Use Environment Mapping"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.env_mapped);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (toggleenvironment_update),
		      &mapvals.env_mapped);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Enable/disable environment mapping (reflection)"),
			   NULL);

  toggle = gtk_check_button_new_with_label (_("Transparent Background"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.transparent_background);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (toggle_update),
		      &mapvals.transparent_background);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Make destination image transparent where bump "
			     "height is zero"),NULL);

  toggle = gtk_check_button_new_with_label (_("Create New Image"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.create_new_image);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &mapvals.create_new_image);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Create a new image when applying filter"), NULL);

  toggle = gtk_check_button_new_with_label (_("High Preview Quality"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.previewquality);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (toggle_update),
		      &mapvals.previewquality);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Enable/disable high quality previews"), NULL);

  toggle = gtk_check_button_new_with_label (_("Enable Tooltips"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.tooltips_enabled);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (toggletips_update),
		      &mapvals.tooltips_enabled);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Enable/disable tooltip messages"), NULL); 

  /* Antialiasing options */

  frame = gtk_frame_new (_("Antialiasing Options"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  toggle = gtk_check_button_new_with_label (_("Enable Antialiasing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.antialiasing);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &mapvals.antialiasing);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Enable/disable jagged edges removal "
			     "(antialiasing)"), NULL);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  gtk_widget_set_sensitive (table, mapvals.antialiasing);
  gtk_object_set_data (GTK_OBJECT (toggle), "set_sensitive", table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Depth:"), 0, 0,
			      mapvals.max_depth, 1.0, 5.0, 1.0, 1.0,
			      0, TRUE, 0, 0,
			      _("Antialiasing quality. Higher is better, "
				"but slower"), NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &mapvals.max_depth);

  spinbutton = gimp_spin_button_new (&adj, mapvals.pixel_treshold,
				     0.001, 1000, 0.1, 1, 1, 0, 3);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.pixel_treshold);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Treshold:"), 1.0, 1.0,
			     spinbutton, 1, TRUE);

  gimp_help_set_help_data (spinbutton,
			   _("Stop when pixel differences are smaller than "
			     "this value"), NULL);

  gtk_widget_show (page);

  return page;
}

/******************************/
/* Create light settings page */
/******************************/

static GtkWidget *
create_light_page (void)
{
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *optionmenu;
  GtkWidget *colorbutton;
  GtkWidget *spinbutton;
  GtkObject *adj;

  page = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page), 4);

  frame = gtk_frame_new (_("Light Settings"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  optionmenu = gimp_option_menu_new2 (FALSE, lightmenu_callback,
				      &mapvals.lightsource.type,
				      (gpointer) mapvals.lightsource.type,

				      _("Point Light"),
				      (gpointer) POINT_LIGHT, NULL,
				      _("Directional Light"),
				      (gpointer) DIRECTIONAL_LIGHT, NULL,
				      _("Spot Light"),
				      (gpointer) SPOT_LIGHT, NULL,
				      _("No Light"),
				      (gpointer) NO_LIGHT, NULL,

				      NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Lightsource Type:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  gimp_help_set_help_data (optionmenu, _("Type of light source to apply"), NULL);

  colorbutton = gimp_color_button_double_new (_("Select Lightsource Color"),
					      64, 16,
					      &mapvals.lightsource.color.r, 3);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Lightsource Color:"), 1.0, 0.5,
			     colorbutton, 1, TRUE);

  gimp_help_set_help_data (colorbutton,
			   _("Set light source color"), NULL);

  pointlightwid = gtk_frame_new (_("Position"));
  gtk_box_pack_start (GTK_BOX (page), pointlightwid, FALSE, FALSE, 0);

  if (mapvals.lightsource.type == POINT_LIGHT)
    gtk_widget_show (pointlightwid);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (pointlightwid), table);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&adj, mapvals.lightsource.position.x,
				     -G_MAXFLOAT, G_MAXFLOAT,
				     0.1, 1.0, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("X:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.lightsource.position.x);

  gimp_help_set_help_data (spinbutton,
			   _("Light source X position in XYZ space"), NULL);

  spinbutton = gimp_spin_button_new (&adj, mapvals.lightsource.position.y,
				     -G_MAXFLOAT, G_MAXFLOAT,
				     0.1, 1.0, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Y:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.lightsource.position.y);

  gimp_help_set_help_data (spinbutton,
			   _("Light source Y position in XYZ space"), NULL);

  spinbutton = gimp_spin_button_new (&adj, mapvals.lightsource.position.z,
				     -G_MAXFLOAT, G_MAXFLOAT,
				     0.1, 1.0, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Z:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.lightsource.position.z);

  gimp_help_set_help_data (spinbutton,
			   _("Light source Z position in XYZ space"), NULL);


  dirlightwid = gtk_frame_new (_("Direction Vector"));
  gtk_box_pack_start (GTK_BOX (page), dirlightwid, FALSE, FALSE, 0);

  if (mapvals.lightsource.type == DIRECTIONAL_LIGHT)
    gtk_widget_show (dirlightwid);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (dirlightwid), table);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&adj, mapvals.lightsource.direction.x,
				     -1.0, 1.0, 0.01, 0.1, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("X:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.lightsource.direction.x);

  gimp_help_set_help_data (spinbutton,
			   _("Light source X direction in XYZ space"), NULL);

  spinbutton = gimp_spin_button_new (&adj, mapvals.lightsource.direction.y,
				     -1.0, 1.0, 0.01, 0.1, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Y:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.lightsource.direction.y);

  gimp_help_set_help_data (spinbutton,
			   _("Light source Y direction in XYZ space"), NULL);

  spinbutton = gimp_spin_button_new (&adj, mapvals.lightsource.direction.z,
				     -1.0, 1.0, 0.01, 0.1, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Z:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.lightsource.direction.z);

  gimp_help_set_help_data (spinbutton,
			   _("Light source Z direction in XYZ space"), NULL);

  gtk_widget_show (page);

  return page;  
}

/*********************************/
/* Create material settings page */
/*********************************/

static GtkWidget *
create_material_page (void)
{
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *hbox;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *pixmap;

  page = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page), 4);

  frame = gtk_frame_new (_("Intensity Levels"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  table = gtk_table_new (2, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Ambient intensity */

  pixmap = gimp_pixmap_new (amb1_xpm);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Ambient:"), 1.0, 0.5,
			     pixmap, 1, FALSE);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.ambient_int,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.material.ambient_int);
  gtk_widget_show (spinbutton);

  gimp_help_set_help_data (spinbutton,
			   _("Amount of original color to show where no "
			     "direct light falls"), NULL);

  pixmap = gimp_pixmap_new (amb2_xpm);
  gtk_table_attach (GTK_TABLE (table), pixmap, 3, 4, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (pixmap);

  /* Diffuse intensity */

  pixmap = gimp_pixmap_new (diffint1_xpm);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Diffuse:"), 1.0, 0.5,
			     pixmap, 1, FALSE);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.diffuse_int,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.material.diffuse_int);
  gtk_widget_show (spinbutton);

  gimp_help_set_help_data (spinbutton,
			   _("Intensity of original color when lit by a light "
			     "source"), NULL);

  pixmap = gimp_pixmap_new (diffint2_xpm);
  gtk_table_attach (GTK_TABLE (table), pixmap, 3, 4, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (pixmap);

  frame = gtk_frame_new (_("Reflectivity"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  table = gtk_table_new (3, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Diffuse reflection */

  pixmap = gimp_pixmap_new (diffref1_xpm);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Diffuse:"), 1.0, 0.5,
			     pixmap, 1, FALSE);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.diffuse_ref,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.material.diffuse_ref);
  gtk_widget_show (spinbutton);

  gimp_help_set_help_data (spinbutton,
			   _("Higher values makes the object reflect more "
			     "light (appear lighter)"), NULL);

  pixmap = gimp_pixmap_new (diffref2_xpm);
  gtk_table_attach (GTK_TABLE (table), pixmap, 3, 4, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (pixmap);

  /* Specular reflection */

  pixmap = gimp_pixmap_new (specref1_xpm);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Specular:"), 1.0, 0.5,
			     pixmap, 1, FALSE);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.specular_ref,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.material.specular_ref);
  gtk_widget_show (spinbutton);

  gimp_help_set_help_data (spinbutton,
			   _("Controls how intense the highlights will be"),
			   NULL);

  pixmap = gimp_pixmap_new (specref2_xpm);
  gtk_table_attach (GTK_TABLE (table), pixmap, 3, 4, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (pixmap);

  /* Highlight */

  pixmap = gimp_pixmap_new (high1_xpm);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Highlight:"), 1.0, 0.5,
			     pixmap, 1, FALSE);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.highlight,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.material.highlight);
  gtk_widget_show (spinbutton);

  gimp_help_set_help_data (spinbutton,
			   _("Higher values makes the highlights more focused"),
			   NULL);

  pixmap = gimp_pixmap_new (high2_xpm);
  gtk_table_attach (GTK_TABLE (table), pixmap, 3, 4, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (pixmap);

  gtk_widget_show (page);
  
  return page;
}

static GtkWidget *
create_bump_page (void)
{
  GtkWidget *page;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *optionmenu;
  GtkWidget *menu;
  GtkWidget *spinbutton;
  GtkObject *adj;

  page = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page), 4);

  frame = gtk_frame_new (_("Bumpmap Settings"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (5, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  optionmenu = gtk_option_menu_new ();
  menu = gimp_drawable_menu_new (bumpmap_constrain, bumpmap_drawable_callback,
				 NULL, mapvals.bumpmap_id);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Bumpmap Image:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  optionmenu =
    gimp_option_menu_new2 (FALSE, mapmenu2_callback,
			   &mapvals.bumpmaptype,
			   (gpointer) mapvals.bumpmaptype,

			   _("Linear"),      (gpointer) LINEAR_MAP, NULL,
			   _("Logarithmic"), (gpointer) LOGARITHMIC_MAP, NULL,
			   _("Sinusoidal"),  (gpointer) SINUSOIDAL_MAP, NULL,
			   _("Spherical"),   (gpointer) SPHERICAL_MAP, NULL,

			   NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Curve:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  spinbutton = gimp_spin_button_new (&adj, mapvals.bumpmin,
				     0, G_MAXFLOAT, 0.01, 0.1, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Minimum Height:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.bumpmin);

  spinbutton = gimp_spin_button_new (&adj, mapvals.bumpmax,
				     0, G_MAXFLOAT, 0.01, 0.1, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Maximum Height:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.bumpmax);

  toggle = gtk_check_button_new_with_label (_("Autostretch to Fit Value Range"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.bumpstretch);
  gtk_table_attach_defaults (GTK_TABLE (table), toggle, 0, 2, 4, 5);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &mapvals.bumpstretch);
  gtk_widget_show (toggle);

  gtk_widget_show (page);

  return page;
}

static GtkWidget *
create_environment_page (void)
{
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *optionmenu;
  GtkWidget *menu;

  page = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page), 4);

  frame = gtk_frame_new (_("Environment Settings"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Environment Image:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  optionmenu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox), optionmenu, FALSE, FALSE, 0);
  menu = gimp_drawable_menu_new (envmap_constrain, envmap_drawable_callback,
				 NULL, mapvals.envmap_id);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
  gtk_widget_show (optionmenu);

  gtk_widget_show (page);

  return page;
}

/****************************/
/* Create notbook and pages */
/****************************/

static void
create_main_notebook (GtkWidget *container)
{
  GtkWidget *page;

  options_note_book = GTK_NOTEBOOK (gtk_notebook_new ());
  gtk_container_add (GTK_CONTAINER (container),
		     GTK_WIDGET (options_note_book));

  page = create_options_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new (_("Options")));

  page = create_light_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new (_("Light")));

  page = create_material_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new (_("Material")));

  if (mapvals.bump_mapped == TRUE)
    {
      bump_page = create_bump_page ();
      bump_page_pos = g_list_length (options_note_book->children);
      gtk_notebook_append_page (options_note_book, bump_page,
				gtk_label_new (_("Bump")));
    }

  if (mapvals.env_mapped == TRUE)
    {
      env_page = create_environment_page ();
      env_page_pos = g_list_length (options_note_book->children);
      gtk_notebook_append_page (options_note_book, env_page,
				gtk_label_new (_("Env")));
    }

  gtk_widget_show (GTK_WIDGET (options_note_book));
}

/********************************/
/* Create and show main dialog. */
/********************************/

gboolean
main_dialog (GDrawable *drawable)
{
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *button;
  gchar **argv;
  gint    argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("MapObject");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  if (! gimp_use_xshm ())
    gdk_set_use_xshm (FALSE);

  visinfo = gck_visualinfo_new ();

  appwin = gimp_dialog_new (_("Lighting Effects"), "Lighting",
			    gimp_plugin_help_func,
			    "filters/lighting.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    _("OK"), apply_callback,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_main_quit,
			    NULL, NULL, NULL, FALSE, TRUE,

			    NULL);

  gimp_help_init ();

  main_hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (appwin)->vbox), main_hbox,
		      FALSE, FALSE, 0);
  gtk_widget_show (main_hbox);

  /* Create the Preview */
  /* ================== */

  frame = gtk_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* Add preview widget and various buttons to the first part */
  /* ======================================================== */

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_realize (appwin);

  previewarea = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (previewarea),
			 PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_widget_set_events (previewarea, (GDK_EXPOSURE_MASK |
				       GDK_BUTTON1_MOTION_MASK |
				       GDK_BUTTON_PRESS_MASK | 
				       GDK_BUTTON_RELEASE_MASK));
  gtk_signal_connect (GTK_OBJECT (previewarea), "event",
		      GTK_SIGNAL_FUNC (preview_events),
		      (gpointer) previewarea);
  gtk_container_add (GTK_CONTAINER (frame), previewarea);
  gtk_widget_show (previewarea);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_label (_("Preview!"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (preview_callback),
		      NULL);
  gtk_widget_show (button);

  gimp_help_set_help_data (button, _("Recompute preview image"), NULL);

  button = gimp_pixmap_button_new (zoom_out_xpm, NULL);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (zoomout_callback),
		      NULL);
  gtk_widget_show (button);

  gimp_help_set_help_data (button, _("Zoom out (make image smaller)"), NULL);

  button = gimp_pixmap_button_new (zoom_in_xpm, NULL);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (zoomin_callback),
		      NULL);
  gtk_widget_show (button);

  gimp_help_set_help_data (button, _("Zoom in (make image bigger)"), NULL);

  create_main_notebook (main_hbox);

  gtk_widget_show (appwin);

  {
    GdkCursor *newcursor;

    newcursor = gdk_cursor_new (GDK_HAND2);
    gdk_window_set_cursor (previewarea->window, newcursor);
    gdk_cursor_destroy (newcursor);
    gdk_flush ();
  }

  if (!mapvals.tooltips_enabled)
    gimp_help_disable_tooltips ();

  image_setup (drawable, TRUE);

  gtk_main ();

  if (preview_rgb_data != NULL)
    g_free (preview_rgb_data);

  if (image != NULL)
    gdk_image_destroy (image);

  gck_visualinfo_destroy (visinfo);
  gtk_widget_destroy (appwin);

  gimp_help_free ();

  gdk_flush ();

  return run;
}
