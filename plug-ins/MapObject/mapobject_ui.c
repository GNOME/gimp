/**************************************************************/
/* Dialog creation and updaters, callbacks and event-handlers */
/**************************************************************/

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <gck/gck.h>

#include "arcball.h"
#include "mapobject_ui.h"
#include "mapobject_image.h"
#include "mapobject_apply.h"
#include "mapobject_preview.h"
#include "mapobject_main.h"

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


GckVisualInfo *visinfo     = NULL;
GdkGC         *gc          = NULL;
GtkWidget     *previewarea = NULL;

static GtkWidget   *appwin            = NULL;
static GtkNotebook *options_note_book = NULL;

static GtkWidget *pointlightwid;
static GtkWidget *dirlightwid;

static GtkObject *xadj, *yadj, *zadj;

static GtkWidget *box_page      = NULL;
static GtkWidget *cylinder_page = NULL;

static guint left_button_pressed = FALSE;
static guint light_hit           = FALSE;

static gboolean run = FALSE;


static void create_main_notebook       (GtkWidget     *container);

static gint preview_events             (GtkWidget     *area,
					GdkEvent      *event);

static void update_light_pos_entries   (void);

static void double_adjustment_update   (GtkAdjustment *adjustment,
					gpointer       data);

static void toggle_update              (GtkWidget     *widget,
					gpointer       data);

static void togglegrid_update          (GtkWidget     *widget,
					gpointer       data);
static void toggletips_update          (GtkWidget     *widget,
					gpointer       data);

static void lightmenu_callback         (GtkWidget     *widget,
					gpointer       data);

static void preview_callback           (GtkWidget     *widget,
					gpointer       data);
static void apply_callback             (GtkWidget     *widget,
					gpointer       data);

static gint box_constrain              (gint32         image_id,
					gint32         drawable_id,
					gpointer       data);
static void box_drawable_callback      (gint32         id,
					gpointer       data);

static gint cylinder_constrain         (gint32   image_id,
					gint32   drawable_id,
					gpointer data);
static void cylinder_drawable_callback (gint32   id,
					gpointer data);

static GtkWidget * create_options_page     (void);
static GtkWidget * create_light_page       (void);
static GtkWidget * create_material_page    (void);
static GtkWidget * create_orientation_page (void);
static GtkWidget * create_box_page         (void);
static GtkWidget * create_cylinder_page    (void);


/******************************************************/
/* Update angle & position (redraw grid if necessary) */
/******************************************************/

static void
double_adjustment_update (GtkAdjustment *adjustment,
			  gpointer       data)
{
  gimp_double_adjustment_update (adjustment, data);

  if (mapvals.showgrid)
    draw_preview_wireframe ();
}

static void
update_light_pos_entries (void)
{
  gtk_signal_handler_block_by_data (GTK_OBJECT (xadj),
				    &mapvals.lightsource.position.x);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (xadj),
			    mapvals.lightsource.position.x);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (xadj),
				      &mapvals.lightsource.position.x);
  
  gtk_signal_handler_block_by_data (GTK_OBJECT (yadj),
				    &mapvals.lightsource.position.y);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (yadj),
			    mapvals.lightsource.position.x);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (yadj),
				      &mapvals.lightsource.position.y);
  
  gtk_signal_handler_block_by_data (GTK_OBJECT (zadj),
				    &mapvals.lightsource.position.z);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (zadj),
			    mapvals.lightsource.position.z);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (zadj),
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

  draw_preview_image (TRUE);
  linetab[0].x1 = -1;
}

/***************************/
/* Show grid toggle update */
/***************************/

static void
togglegrid_update (GtkWidget *widget,
		   gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  if (mapvals.showgrid && linetab[0].x1 == -1)
    draw_preview_wireframe ();
  else if (!mapvals.showgrid && linetab[0].x1 != -1)
    {
      gck_gc_set_foreground (visinfo, gc, 255, 255, 255);
      gck_gc_set_background (visinfo, gc, 0, 0, 0);

      gdk_gc_set_function (gc, GDK_INVERT);

      clear_wireframe ();
      linetab[0].x1 = -1;
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

/***************************************/
/* Main window map type menu callback. */
/***************************************/

static void
mapmenu_callback (GtkWidget *widget,
		  gpointer   data)
{
  gimp_menu_item_update (widget, data);

  draw_preview_image (TRUE);

  if (mapvals.showgrid && linetab[0].x1 == -1)
    draw_preview_wireframe ();
  else if (!mapvals.showgrid && linetab[0].x1 != -1)
    {
      gck_gc_set_foreground (visinfo, gc, 255, 255, 255);
      gck_gc_set_background (visinfo, gc, 0, 0, 0);

      gdk_gc_set_function (gc, GDK_INVERT);
  
      clear_wireframe ();
      linetab[0].x1 = -1;
    }

  if (mapvals.maptype == MAP_BOX)
    {
      if (cylinder_page != NULL)
        {
          gtk_notebook_remove_page
	    (options_note_book, 
	     g_list_length (options_note_book->children) - 1);
          cylinder_page = NULL;
        }

      if (box_page == NULL)
        {
          box_page = create_box_page ();
          gtk_notebook_append_page (options_note_book,
				    box_page,
				    gtk_label_new (_("Box")));
        }
    }
  else if (mapvals.maptype == MAP_CYLINDER)
    {
      if (box_page != NULL)
        {
          gtk_notebook_remove_page
	    (options_note_book,
	     g_list_length (options_note_book->children) - 1);
          box_page = NULL;
        }

      if (cylinder_page == NULL)
	{
	  cylinder_page = create_cylinder_page ();
	  gtk_notebook_append_page (options_note_book,
				    cylinder_page,
				    gtk_label_new (_("Cylinder")));
	}
    }
  else
    {
      if (box_page != NULL)
        {
          gtk_notebook_remove_page
	    (options_note_book, 
	     g_list_length (options_note_book->children) - 1);
        }

      if (cylinder_page != NULL)
        {
          gtk_notebook_remove_page
	    (options_note_book, 
	     g_list_length (options_note_book->children) - 1);
        }

      box_page = NULL;
      cylinder_page = NULL;
    }
}

/******************************************/
/* Main window "Preview!" button callback */
/******************************************/

static void
preview_callback (GtkWidget *widget,
		  gpointer   data)
{
  draw_preview_image (TRUE);
  linetab[0].x1 = -1;
}

/*********************************************/
/* Main window "-" (zoom in) button callback */
/*********************************************/

static void
zoomout_callback (GtkWidget *widget,
		  gpointer   data)
{
  if (mapvals.preview_zoom_factor < 2)
    {
      mapvals.preview_zoom_factor++;
      if (linetab[0].x1 != -1)
        clear_wireframe ();
      draw_preview_image (TRUE);
    }
}

/**********************************************/
/* Main window "+" (zoom out) button callback */
/**********************************************/

static void
zoomin_callback (GtkWidget *widget,
		 gpointer   data)
{
  if (mapvals.preview_zoom_factor > 0)
    {
      mapvals.preview_zoom_factor--;
      if (linetab[0].x1 != -1)
        clear_wireframe ();
      draw_preview_image (TRUE);
    }
}

/**********************************************/
/* Main window "Apply" button callback.       */ 
/* Render to GIMP image, close down and exit. */
/**********************************************/

static void
apply_callback (GtkWidget *widget,
		gpointer   data)
{
  run = TRUE;

  gtk_main_quit ();
}

static gint
box_constrain (gint32   image_id,
	       gint32   drawable_id,
	       gpointer data)
{
  if (drawable_id == -1)
    return TRUE;

  return (gimp_drawable_is_rgb (drawable_id) &&
	  !gimp_drawable_is_indexed (drawable_id));
}

static void
box_drawable_callback (gint32   id,
		       gpointer data)
{
  gint i;
  
  i = (gint) gtk_object_get_data (GTK_OBJECT (data), "_mapwid_id");

  mapvals.boxmap_id[i] = id;
}

static gint
cylinder_constrain (gint32   image_id,
		    gint32   drawable_id,
		    gpointer data)
{
  if (drawable_id == -1)
    return TRUE;

  return (gimp_drawable_is_rgb (drawable_id) &&
	  !gimp_drawable_is_indexed (drawable_id));
}

static void
cylinder_drawable_callback (gint32   id,
			    gpointer data)
{
  gint i;
  
  i = (gint) gtk_object_get_data (GTK_OBJECT (data), "_mapwid_id");

  mapvals.cylindermap_id[i] = id;
}

/******************************/
/* Preview area event handler */
/******************************/

static gint
preview_events (GtkWidget *area,
		GdkEvent  *event)
{
  HVect pos;
/*  HMatrix RotMat;
  gdouble a,b,c; */

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
          {
            draw_preview_image (FALSE);
            if (mapvals.showgrid == 1 && linetab[0].x1 != -1)
              draw_preview_wireframe ();
          }
        break; 
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
          draw_preview_image (TRUE);
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
                update_light (event->motion.x, event->motion.y);
                update_light_pos_entries ();
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

/*******************************/
/* Create general options page */
/*******************************/

static GtkWidget *
create_options_page (void)
{
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *optionmenu;
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

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Map to:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  optionmenu =
    gimp_option_menu_new2 (FALSE, mapmenu_callback,
			   &mapvals.maptype,
			   (gpointer) mapvals.maptype,

			   _("Plane"),    (gpointer) MAP_PLANE, NULL,
			   _("Sphere"),   (gpointer) MAP_SPHERE, NULL,
			   _("Box"),      (gpointer) MAP_BOX, NULL,
			   _("Cylinder"), (gpointer) MAP_CYLINDER, NULL,

			   NULL);
  gtk_box_pack_start (GTK_BOX (hbox), optionmenu, FALSE, FALSE, 0);
  gtk_widget_show (optionmenu);

  gimp_help_set_help_data (optionmenu, _("Type of object to map to"), NULL);

  toggle = gtk_check_button_new_with_label (_("Transpararent Background"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.transparent_background);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (toggle_update),
		      &mapvals.transparent_background);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Make image transparent outside object"), NULL);

  toggle = gtk_check_button_new_with_label (_("Tile Source Image"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.tiled);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (toggle_update),
		      &mapvals.tiled);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Tile source image: useful for infinite planes"),
			   NULL);

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

  toggle = gtk_check_button_new_with_label (_("Enable Tooltips"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.tooltips_enabled);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (toggletips_update),
		      &mapvals.tooltips_enabled);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle, _("Enable/disable tooltip messages"), NULL); 

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
			      mapvals.maxdepth, 1.0, 5.0, 0.1, 1.0,
			      1, TRUE, 0, 0,
			      _("Antialiasing quality. Higher is better, "
			       "but slower"), NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.maxdepth);

  spinbutton = gimp_spin_button_new (&adj, mapvals.pixeltreshold,
				     0.001, 1000, 0.1, 1, 1, 0, 3);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.pixeltreshold);
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
  gtk_container_add (GTK_CONTAINER (frame), table);  gtk_widget_show (table);

  optionmenu = gimp_option_menu_new2 (FALSE, lightmenu_callback,
				      &mapvals.lightsource.type,
				      (gpointer) mapvals.lightsource.type,

				      _("Point Light"),
				      (gpointer) POINT_LIGHT, NULL,
				      _("Directional Light"),
				      (gpointer) DIRECTIONAL_LIGHT, NULL,
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

  spinbutton = gimp_spin_button_new (&xadj, mapvals.lightsource.position.x,
				     -G_MAXDOUBLE, G_MAXDOUBLE,
				     0.1, 1.0, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("X:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (xadj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.lightsource.position.x);

  gimp_help_set_help_data (spinbutton,
			   _("Light source X position in XYZ space"), NULL);

  spinbutton = gimp_spin_button_new (&yadj, mapvals.lightsource.position.y,
				     -G_MAXDOUBLE, G_MAXDOUBLE,
				     0.1, 1.0, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Y:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (yadj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &mapvals.lightsource.position.y);

  gimp_help_set_help_data (spinbutton,
			   _("Light source Y position in XYZ space"), NULL);

  spinbutton = gimp_spin_button_new (&zadj, mapvals.lightsource.position.z,
				     -G_MAXDOUBLE, G_MAXDOUBLE,
				     0.1, 1.0, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Z:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (zadj), "value_changed",
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
				     0, G_MAXDOUBLE, 0.1, 1.0, 1.0, 0.0, 2);
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
				     0, G_MAXDOUBLE, 0.1, 1.0, 1.0, 0.0, 2);
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
				     0, G_MAXDOUBLE, 0.1, 1.0, 1.0, 0.0, 2);
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
				     0, G_MAXDOUBLE, 0.1, 1.0, 1.0, 0.0, 2);
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
				     0, G_MAXDOUBLE, 0.1, 1.0, 1.0, 0.0, 2);
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

/****************************************/
/* Create orientation and position page */
/****************************************/

static GtkWidget *
create_orientation_page (void)
{
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *table;
  GtkObject *adj;

  page = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page), 4);

  frame = gtk_frame_new (_("Position"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("X:"), 0, 0,
			      mapvals.position.x, -1.0, 2.0, 0.01, 0.1, 5,
			      TRUE, 0, 0,
			      _("Object X position in XYZ space"), NULL);
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.01, 5);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (double_adjustment_update),
		      &mapvals.position.x);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Y:"), 0, 0,
			      mapvals.position.y, -1.0, 2.0, 0.01, 0.1, 5,
			      TRUE, 0, 0,
			      _("Object Y position in XYZ space"), NULL);
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.01, 5);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (double_adjustment_update),
		      &mapvals.position.y);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Z:"), 0, 0,
			      mapvals.position.z, -1.0, 2.0, 0.01, 0.1, 5,
			      TRUE, 0, 0,
			      _("Object Z position in XYZ space"), NULL);
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.01, 5);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (double_adjustment_update),
		      &mapvals.position.z);

  frame = gtk_frame_new (_("Rotation"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("X:"), 0, 0,
			      mapvals.alpha, -180.0, 180.0, 1.0, 15.0, 1,
			      TRUE, 0, 0,
			      _("Rotation angle about X axis"), NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (double_adjustment_update),
		      &mapvals.alpha);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Y:"), 0, 0,
			      mapvals.beta, -180.0, 180.0, 1.0, 15.0, 1,
			      TRUE, 0, 0,
			      _("Rotation angle about Y axis"), NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (double_adjustment_update),
		      &mapvals.beta);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Z:"), 0, 0,
			      mapvals.gamma, -180.0, 180.0, 1.0, 15.0, 1,
			      TRUE, 0, 0,
			      _("Rotation angle about Z axis"), NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (double_adjustment_update),
		      &mapvals.gamma);

  gtk_widget_show (page);

  return page;
}

static GtkWidget *
create_box_page (void)
{
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *optionmenu;
  GtkWidget *menu;
  GtkObject *adj;
  gint       i;

  static gchar *labels[] =
  {
    N_("Front:"), N_("Back:"),
    N_("Top:"),   N_("Bottom:"),
    N_("Left:"),  N_("Right:")
  };

  page = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page), 4);

  frame = gtk_frame_new (_("Map Images to Box Faces"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (6, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE(table), 2);
  gtk_table_set_col_spacings (GTK_TABLE(table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 5);
  gtk_widget_show (table);

  /* Option menues */

  for (i = 0; i < 6; i++)
    {
      optionmenu = gtk_option_menu_new ();
      gtk_object_set_data (GTK_OBJECT (optionmenu), "_mapwid_id",
			   (gpointer) i);
      menu = gimp_drawable_menu_new (box_constrain, box_drawable_callback,
				     (gpointer) optionmenu,
				     mapvals.boxmap_id[i]);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);      

      gimp_table_attach_aligned (GTK_TABLE (table), 0, i,
				 gettext (labels[i]), 1.0, 0.5,
				 optionmenu, 1, FALSE);
    }

  /* Scale scales */

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE(table), 2);
  gtk_table_set_col_spacings (GTK_TABLE(table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Scale X:"), 0, 0,
			      mapvals.scale.x, 0.0, 5.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      _("X scale (size)"), NULL);
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.1, 2);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (double_adjustment_update),
		      &mapvals.scale.x);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Y:"), 0, 0,
			      mapvals.scale.y, 0.0, 5.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      _("Y scale (size)"), NULL);
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.1, 2);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (double_adjustment_update),
		      &mapvals.scale.y);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Z:"), 0, 0,
			      mapvals.scale.z, 0.0, 5.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      _("Z scale (size)"), NULL);
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.1, 2);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (double_adjustment_update),
		      &mapvals.scale.z);

  gtk_widget_show (page);

  return page;
}

static GtkWidget *
create_cylinder_page (void)
{
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *optionmenu;
  GtkWidget *menu;
  GtkObject *adj;
  gint       i;

  static gchar *labels[] = { N_("Top:"), N_("Bottom:") };

  page = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page), 4);

  frame = gtk_frame_new (_("Images for the Cap Faces"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  /* Option menus */

  for (i = 0; i < 2; i++)
    {
      optionmenu = gtk_option_menu_new ();
      gtk_object_set_data (GTK_OBJECT (optionmenu), "_mapwid_id",
			   (gpointer) i);
      menu = gimp_drawable_menu_new (cylinder_constrain,
				     cylinder_drawable_callback,
				     (gpointer) optionmenu,
				     mapvals.cylindermap_id[i]);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, i,
				 gettext (labels[i]), 1.0, 0.5,
				 optionmenu, 1, FALSE);
    }

  frame = gtk_frame_new (_("Size"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Radius:"), 0, 0,
			      mapvals.cylinder_radius,
			      0.0, 2.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      _("Cylinder radius"), NULL);
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.1, 2);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (double_adjustment_update),
		      &mapvals.cylinder_radius);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Length:"), 0, 0,
			      mapvals.cylinder_length,
			      0.0, 2.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      _("Cylinder length"), NULL);
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.1, 2);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (double_adjustment_update),
		      &mapvals.cylinder_length);

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
  gtk_container_add (GTK_CONTAINER (container), GTK_WIDGET (options_note_book));

  page = create_options_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new (_("Options")));
  
  page = create_light_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new (_("Light")));
  
  page = create_material_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new (_("Material")));
  
  page = create_orientation_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new (_("Orientation")));

  if (mapvals.maptype == MAP_BOX)
    {
      box_page = create_box_page ();
      gtk_notebook_append_page (options_note_book, box_page,
				gtk_label_new (_("Box")));
    }
  else if (mapvals.maptype == MAP_CYLINDER)
    {
      cylinder_page = create_cylinder_page ();
      gtk_notebook_append_page (options_note_book, cylinder_page,
				gtk_label_new (_("Cylinder")));
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
  GtkWidget *toggle;
  gchar **argv;
  gint    argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("MapObject");

  gdk_set_use_xshm (gimp_use_xshm ());

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  /* Set up ArcBall stuff */
  /* ==================== */

  /*ArcBall_Init(); */

  visinfo = gck_visualinfo_new ();

  appwin = gimp_dialog_new (_("Map to Object"), "MapObject",
			    gimp_plugin_help_func,
			    "filters/mapobject.html",
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

  frame = gtk_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* Add preview widget and various buttons to the first part */

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

  toggle = gtk_check_button_new_with_label (_("Show Preview Wireframe"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mapvals.showgrid);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (togglegrid_update),
		      &mapvals.showgrid);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle, _("Show/hide preview wireframe"), NULL);

  create_main_notebook (main_hbox);

  /* Endmarkers for line table */
  
  linetab[0].x1 = -1;

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
  gtk_widget_destroy(appwin);

  gimp_help_free ();

  gdk_flush ();

  return run;
}
