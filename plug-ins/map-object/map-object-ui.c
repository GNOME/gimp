/**************************************************************/
/* Dialog creation and updaters, callbacks and event-handlers */
/**************************************************************/

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "arcball.h"
#include "map-object-ui.h"
#include "map-object-image.h"
#include "map-object-apply.h"
#include "map-object-preview.h"
#include "map-object-main.h"
#include "map-object-stock.h"

#include "libgimp/stdplugins-intl.h"


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

static void lightmenu_callback         (GtkWidget     *widget,
					gpointer       data);

static void preview_callback           (GtkWidget     *widget,
					gpointer       data);

static gint box_constrain              (gint32         image_id,
					gint32         drawable_id,
					gpointer       data);
static gint cylinder_constrain         (gint32         image_id,
					gint32         drawable_id,
					gpointer       data);

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
  g_signal_handlers_block_by_func (xadj,
                                   gimp_double_adjustment_update,
                                   &mapvals.lightsource.position.x);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (xadj),
			    mapvals.lightsource.position.x);
  g_signal_handlers_unblock_by_func (xadj,
                                     gimp_double_adjustment_update,
                                     &mapvals.lightsource.position.x);

  g_signal_handlers_block_by_func (yadj,
                                   gimp_double_adjustment_update,
                                   &mapvals.lightsource.position.y);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (yadj),
			    mapvals.lightsource.position.x);
  g_signal_handlers_unblock_by_func (yadj,
                                     gimp_double_adjustment_update,
                                     &mapvals.lightsource.position.y);

  g_signal_handlers_block_by_func (zadj,
                                   gimp_double_adjustment_update,
                                   &mapvals.lightsource.position.z);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (zadj),
			    mapvals.lightsource.position.z);
  g_signal_handlers_unblock_by_func (zadj,
                                     gimp_double_adjustment_update,
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
    {
      draw_preview_wireframe ();
    }
  else if (!mapvals.showgrid && linetab[0].x1 != -1)
    {
      GdkColor  color;

      color.red   = 0x0;
      color.green = 0x0;
      color.blue  = 0x0;
      gdk_gc_set_rgb_bg_color (gc, &color);

      color.red   = 0xFFFF;
      color.green = 0xFFFF;
      color.blue  = 0xFFFF;
      gdk_gc_set_rgb_fg_color (gc, &color);

      gdk_gc_set_function (gc, GDK_INVERT);

      clear_wireframe ();
      linetab[0].x1 = -1;
    }
}

/*****************************************/
/* Main window light type menu callback. */
/*****************************************/

static void
lightmenu_callback (GtkWidget *widget,
		    gpointer   data)
{
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), (gint *) data);

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
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), (gint *) data);

  draw_preview_image (TRUE);

  if (mapvals.showgrid && linetab[0].x1 == -1)
    {
      draw_preview_wireframe ();
    }
  else if (!mapvals.showgrid && linetab[0].x1 != -1)
    {
      GdkColor  color;

      color.red   = 0x0;
      color.green = 0x0;
      color.blue  = 0x0;
      gdk_gc_set_rgb_bg_color (gc, &color);

      color.red   = 0xFFFF;
      color.green = 0xFFFF;
      color.blue  = 0xFFFF;
      gdk_gc_set_rgb_fg_color (gc, &color);

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
				    gtk_label_new_with_mnemonic (_("_Box")));
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
				    gtk_label_new_with_mnemonic (_("C_ylinder")));
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

static void
zoomed_callback (GimpZoomModel *model)
{
  mapvals.zoom = gimp_zoom_model_get_factor (model);

  if (linetab[0].x1 != -1)
    clear_wireframe ();

  draw_preview_image (TRUE);
}

/**********************************************/
/* Main window "Apply" button callback.       */
/* Render to GIMP image, close down and exit. */
/**********************************************/

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
	  {
	    draw_preview_image (TRUE);
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
  GtkWidget *combo;
  GtkWidget *toggle;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;

  page = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  /* General options */

  frame = gimp_frame_new (_("General Options"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Map to:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gimp_int_combo_box_new (_("Plane"),    MAP_PLANE,
                                  _("Sphere"),   MAP_SPHERE,
                                  _("Box"),      MAP_BOX,
                                  _("Cylinder"), MAP_CYLINDER,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), mapvals.maptype);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (mapmenu_callback),
                    &mapvals.maptype);

  gimp_help_set_help_data (combo, _("Type of object to map to"), NULL);

  toggle = gtk_check_button_new_with_label (_("Transparent background"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.transparent_background);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (toggle_update),
                    &mapvals.transparent_background);

  gimp_help_set_help_data (toggle,
			   _("Make image transparent outside object"), NULL);

  toggle = gtk_check_button_new_with_label (_("Tile source image"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.tiled);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (toggle_update),
                    &mapvals.tiled);

  gimp_help_set_help_data (toggle,
			   _("Tile source image: useful for infinite planes"),
			   NULL);

  toggle = gtk_check_button_new_with_label (_("Create new image"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.create_new_image);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mapvals.create_new_image);

  gimp_help_set_help_data (toggle,
			   _("Create a new image when applying filter"), NULL);

  /* Antialiasing options */

  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  toggle = gtk_check_button_new_with_mnemonic (_("Enable _antialiasing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.antialiasing);
  gtk_frame_set_label_widget (GTK_FRAME (frame), toggle);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mapvals.antialiasing);

  gimp_help_set_help_data (toggle,
			   _("Enable/disable jagged edges removal "
			     "(antialiasing)"), NULL);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  gtk_widget_set_sensitive (table, mapvals.antialiasing);
  g_object_set_data (G_OBJECT (toggle), "set_sensitive", table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("_Depth:"), 0, 0,
			      mapvals.maxdepth, 1.0, 5.0, 0.1, 1.0,
			      1, TRUE, 0, 0,
			      _("Antialiasing quality. Higher is better, "
			       "but slower"), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.maxdepth);

  spinbutton = gimp_spin_button_new (&adj, mapvals.pixeltreshold,
				     0.001, 1000, 0.1, 1, 1, 0, 3);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("_Threshold:"), 0.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.pixeltreshold);

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
  GtkWidget *combo;
  GtkWidget *colorbutton;
  GtkWidget *spinbutton;
  GtkObject *adj;

  page = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  frame = gimp_frame_new (_("Light Settings"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);  gtk_widget_show (table);

  combo = gimp_int_combo_box_new (_("Point light"),       POINT_LIGHT,
                                  _("Directional light"), DIRECTIONAL_LIGHT,
                                  _("No light"),          NO_LIGHT,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                 mapvals.lightsource.type);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Lightsource type:"), 0.0, 0.5,
			     combo, 1, FALSE);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (lightmenu_callback),
                    &mapvals.lightsource.type);

  gimp_help_set_help_data (combo, _("Type of light source to apply"), NULL);

  colorbutton = gimp_color_button_new (_("Select lightsource color"),
				       64, 16,
				       &mapvals.lightsource.color,
				       GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Lightsource color:"), 0.0, 0.5,
			     colorbutton, 1, FALSE);

  g_signal_connect (colorbutton, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &mapvals.lightsource.color);

  gimp_help_set_help_data (colorbutton,
			   _("Set light source color"), NULL);

  pointlightwid = gimp_frame_new (_("Position"));
  gtk_box_pack_start (GTK_BOX (page), pointlightwid, FALSE, FALSE, 0);

  if (mapvals.lightsource.type == POINT_LIGHT)
    gtk_widget_show (pointlightwid);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (pointlightwid), table);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&xadj, mapvals.lightsource.position.x,
				     -G_MAXFLOAT, G_MAXFLOAT,
				     0.1, 1.0, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("X:"), 0.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (xadj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.lightsource.position.x);

  gimp_help_set_help_data (spinbutton,
			   _("Light source X position in XYZ space"), NULL);

  spinbutton = gimp_spin_button_new (&yadj, mapvals.lightsource.position.y,
				     -G_MAXFLOAT, G_MAXFLOAT,
				     0.1, 1.0, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Y:"), 0.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (yadj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.lightsource.position.y);

  gimp_help_set_help_data (spinbutton,
			   _("Light source Y position in XYZ space"), NULL);

  spinbutton = gimp_spin_button_new (&zadj, mapvals.lightsource.position.z,
				     -G_MAXFLOAT, G_MAXFLOAT,
				     0.1, 1.0, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Z:"), 0.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (zadj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.lightsource.position.z);

  gimp_help_set_help_data (spinbutton,
			   _("Light source Z position in XYZ space"), NULL);


  dirlightwid = gimp_frame_new (_("Direction Vector"));
  gtk_box_pack_start (GTK_BOX (page), dirlightwid, FALSE, FALSE, 0);

  if (mapvals.lightsource.type == DIRECTIONAL_LIGHT)
    gtk_widget_show (dirlightwid);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (dirlightwid), table);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&adj, mapvals.lightsource.direction.x,
				     -1.0, 1.0, 0.01, 0.1, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("X:"), 0.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.lightsource.direction.x);

  gimp_help_set_help_data (spinbutton,
			   _("Light source X direction in XYZ space"), NULL);

  spinbutton = gimp_spin_button_new (&adj, mapvals.lightsource.direction.y,
				     -1.0, 1.0, 0.01, 0.1, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Y:"), 0.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.lightsource.direction.y);

  gimp_help_set_help_data (spinbutton,
			   _("Light source Y direction in XYZ space"), NULL);

  spinbutton = gimp_spin_button_new (&adj, mapvals.lightsource.direction.z,
				     -1.0, 1.0, 0.01, 0.1, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Z:"), 0.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
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
  GtkSizeGroup *group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  GtkWidget    *page;
  GtkWidget    *frame;
  GtkWidget    *table;
  GtkWidget    *label;
  GtkWidget    *hbox;
  GtkWidget    *spinbutton;
  GtkWidget    *image;
  GtkObject    *adj;

  page = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  frame = gimp_frame_new (_("Intensity Levels"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  table = gtk_table_new (2, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Ambient intensity */

  image = gtk_image_new_from_stock (STOCK_INTENSITY_AMBIENT_LOW,
                                    GTK_ICON_SIZE_BUTTON);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                     _("Ambient:"), 0.0, 0.5,
                                     image, 1, FALSE);
  gtk_size_group_add_widget (group, label);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.ambient_int,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.material.ambient_int);

  gimp_help_set_help_data (spinbutton,
			   _("Amount of original color to show where no "
			     "direct light falls"), NULL);

  image = gtk_image_new_from_stock (STOCK_INTENSITY_AMBIENT_HIGH,
                                    GTK_ICON_SIZE_BUTTON);
  gtk_table_attach (GTK_TABLE (table), image, 3, 4, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (image);

  /* Diffuse intensity */

  image = gtk_image_new_from_stock (STOCK_INTENSITY_DIFFUSE_LOW,
                                    GTK_ICON_SIZE_BUTTON);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                                     _("Diffuse:"), 0.0, 0.5,
                                     image, 1, FALSE);
  gtk_size_group_add_widget (group, label);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.diffuse_int,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.material.diffuse_int);

  gimp_help_set_help_data (spinbutton,
			   _("Intensity of original color when lit by a light "
			     "source"), NULL);

  image = gtk_image_new_from_stock (STOCK_INTENSITY_DIFFUSE_HIGH,
                                    GTK_ICON_SIZE_BUTTON);
  gtk_table_attach (GTK_TABLE (table), image, 3, 4, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (image);

  frame = gimp_frame_new (_("Reflectivity"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  table = gtk_table_new (3, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Diffuse reflection */

  image = gtk_image_new_from_stock (STOCK_REFLECTIVITY_DIFFUSE_LOW,
                                    GTK_ICON_SIZE_BUTTON);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                     _("Diffuse:"), 0.0, 0.5,
                                     image, 1, FALSE);
  gtk_size_group_add_widget (group, label);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.diffuse_ref,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.material.diffuse_ref);

  gimp_help_set_help_data (spinbutton,
			   _("Higher values makes the object reflect more "
			     "light (appear lighter)"), NULL);

  image = gtk_image_new_from_stock (STOCK_REFLECTIVITY_DIFFUSE_HIGH,
                                    GTK_ICON_SIZE_BUTTON);
  gtk_table_attach (GTK_TABLE (table), image, 3, 4, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (image);

  /* Specular reflection */

  image = gtk_image_new_from_stock (STOCK_REFLECTIVITY_SPECULAR_LOW,
                                    GTK_ICON_SIZE_BUTTON);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                                     _("Specular:"), 0.0, 0.5,
                                     image, 1, FALSE);
  gtk_size_group_add_widget (group, label);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.specular_ref,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.material.specular_ref);

  gimp_help_set_help_data (spinbutton,
			   _("Controls how intense the highlights will be"),
			   NULL);

  image = gtk_image_new_from_stock (STOCK_REFLECTIVITY_SPECULAR_HIGH,
                                    GTK_ICON_SIZE_BUTTON);
  gtk_table_attach (GTK_TABLE (table), image, 3, 4, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (image);

  /* Highlight */

  image = gtk_image_new_from_stock (STOCK_REFLECTIVITY_HIGHLIGHT_LOW,
                                    GTK_ICON_SIZE_BUTTON);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                                     _("Highlight:"), 0.0, 0.5,
                                     image, 1, FALSE);
  gtk_size_group_add_widget (group, label);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.highlight,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.material.highlight);

  gimp_help_set_help_data (spinbutton,
			   _("Higher values makes the highlights more focused"),
			   NULL);

  image = gtk_image_new_from_stock (STOCK_REFLECTIVITY_HIGHLIGHT_HIGH,
                                    GTK_ICON_SIZE_BUTTON);
  gtk_table_attach (GTK_TABLE (table), image, 3, 4, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (image);

  gtk_widget_show (page);

  g_object_unref (group);

  return page;
}

/****************************************/
/* Create orientation and position page */
/****************************************/

static GtkWidget *
create_orientation_page (void)
{
  GtkSizeGroup *group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  GtkWidget    *page;
  GtkWidget    *frame;
  GtkWidget    *table;
  GtkObject    *adj;

  page = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  frame = gimp_frame_new (_("Position"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("X:"), 0, 0,
			      mapvals.position.x, -1.0, 2.0, 0.01, 0.1, 5,
			      TRUE, 0, 0,
			      _("Object X position in XYZ space"), NULL);
  gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_SPINBUTTON (adj));
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.01, 5);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.position.x);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Y:"), 0, 0,
			      mapvals.position.y, -1.0, 2.0, 0.01, 0.1, 5,
			      TRUE, 0, 0,
			      _("Object Y position in XYZ space"), NULL);
  gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_SPINBUTTON (adj));
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.01, 5);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.position.y);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Z:"), 0, 0,
			      mapvals.position.z, -1.0, 2.0, 0.01, 0.1, 5,
			      TRUE, 0, 0,
			      _("Object Z position in XYZ space"), NULL);
  gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_SPINBUTTON (adj));
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.01, 5);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.position.z);

  frame = gimp_frame_new (_("Rotation"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("X:"), 0, 0,
			      mapvals.alpha, -180.0, 180.0, 1.0, 15.0, 1,
			      TRUE, 0, 0,
			      _("Rotation angle about X axis"), NULL);
  gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_SPINBUTTON (adj));

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.alpha);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Y:"), 0, 0,
			      mapvals.beta, -180.0, 180.0, 1.0, 15.0, 1,
			      TRUE, 0, 0,
			      _("Rotation angle about Y axis"), NULL);
  gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_SPINBUTTON (adj));

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.beta);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Z:"), 0, 0,
			      mapvals.gamma, -180.0, 180.0, 1.0, 15.0, 1,
			      TRUE, 0, 0,
			      _("Rotation angle about Z axis"), NULL);
  gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_SPINBUTTON (adj));

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.gamma);

  gtk_widget_show (page);

  g_object_unref (group);

  return page;
}

static GtkWidget *
create_box_page (void)
{
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkObject *adj;
  gint       i;

  static gchar *labels[] =
  {
    N_("Front:"), N_("Back:"),
    N_("Top:"),   N_("Bottom:"),
    N_("Left:"),  N_("Right:")
  };

  page = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  frame = gimp_frame_new (_("Map Images to Box Faces"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (6, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE(table), 6);
  gtk_table_set_col_spacings (GTK_TABLE(table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 5);
  gtk_widget_show (table);

  for (i = 0; i < 6; i++)
    {
      GtkWidget *combo;

      combo = gimp_drawable_combo_box_new (box_constrain, NULL);
      gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                                  mapvals.boxmap_id[i],
                                  G_CALLBACK (gimp_int_combo_box_get_active),
                                  &mapvals.boxmap_id[i]);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, i,
				 gettext (labels[i]), 0.0, 0.5,
				 combo, 1, FALSE);
    }

  /* Scale scales */

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE(table), 6);
  gtk_table_set_col_spacings (GTK_TABLE(table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Scale X:"), 0, 0,
			      mapvals.scale.x, 0.0, 5.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      _("X scale (size)"), NULL);
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.1, 2);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.scale.x);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Y:"), 0, 0,
			      mapvals.scale.y, 0.0, 5.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      _("Y scale (size)"), NULL);
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.1, 2);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.scale.y);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Z:"), 0, 0,
			      mapvals.scale.z, 0.0, 5.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      _("Z scale (size)"), NULL);
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.1, 2);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.scale.z);

  gtk_widget_show (page);

  return page;
}

static GtkWidget *
create_cylinder_page (void)
{
  GtkSizeGroup *group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  GtkWidget    *page;
  GtkWidget    *frame;
  GtkWidget    *table;
  GtkObject    *adj;
  gint          i;

  static gchar *labels[] = { N_("_Top:"), N_("_Bottom:") };

  page = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  frame = gimp_frame_new (_("Images for the Cap Faces"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  /* Option menus */

  for (i = 0; i < 2; i++)
    {
      GtkWidget *combo;
      GtkWidget *label;

      combo = gimp_drawable_combo_box_new (cylinder_constrain, NULL);
      gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                                  mapvals.cylindermap_id[i],
                                  G_CALLBACK (gimp_int_combo_box_get_active),
                                  &mapvals.cylindermap_id[i]);

      label = gimp_table_attach_aligned (GTK_TABLE (table), 0, i,
                                         gettext (labels[i]), 0.0, 0.5,
                                         combo, 1, FALSE);
      gtk_size_group_add_widget (group, label);
    }

  frame = gimp_frame_new (_("Size"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("R_adius:"), 0, 0,
			      mapvals.cylinder_radius,
			      0.0, 2.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      _("Cylinder radius"), NULL);
  gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.1, 2);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.cylinder_radius);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("L_ength:"), 0, 0,
			      mapvals.cylinder_length,
			      0.0, 2.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      _("Cylinder length"), NULL);
  gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));
  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.1, 2);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.cylinder_length);

  gtk_widget_show (page);

  g_object_unref (group);

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
			    gtk_label_new_with_mnemonic (_("O_ptions")));

  page = create_light_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new_with_mnemonic (_("_Light")));

  page = create_material_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new_with_mnemonic (_("_Material")));

  page = create_orientation_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new_with_mnemonic (_("O_rientation")));

  if (mapvals.maptype == MAP_BOX)
    {
      box_page = create_box_page ();
      gtk_notebook_append_page (options_note_book, box_page,
				gtk_label_new_with_mnemonic (_("_Box")));
    }
  else if (mapvals.maptype == MAP_CYLINDER)
    {
      cylinder_page = create_cylinder_page ();
      gtk_notebook_append_page (options_note_book, cylinder_page,
				gtk_label_new_with_mnemonic (_("C_ylinder")));
    }

  gtk_widget_show (GTK_WIDGET (options_note_book));
}

/********************************/
/* Create and show main dialog. */
/********************************/

gboolean
main_dialog (GimpDrawable *drawable)
{
  GtkWidget     *main_hbox;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *frame;
  GtkWidget     *button;
  GtkWidget     *toggle;
  GimpZoomModel *model;
  gboolean       run = FALSE;

  gimp_ui_init ("map-object", FALSE);

  appwin = gimp_dialog_new (_("Map to Object"), "map-object",
                            NULL, 0,
			    gimp_standard_help_func, "plug-in-map-object",

			    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			    GTK_STOCK_OK,     GTK_RESPONSE_OK,

			    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (appwin),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (appwin));

  main_hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (appwin)->vbox), main_hbox,
		      FALSE, FALSE, 0);
  gtk_widget_show (main_hbox);

  /* Create the Preview */

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);
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

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("_Preview!"));
  gtk_misc_set_padding (GTK_MISC (gtk_bin_get_child (GTK_BIN (button))), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (preview_callback),
                    NULL);

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

  toggle = gtk_check_button_new_with_mnemonic (_("Show preview _wireframe"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mapvals.showgrid);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (togglegrid_update),
                    &mapvals.showgrid);

  create_main_notebook (main_hbox);

  /* Endmarkers for line table */

  linetab[0].x1 = -1;

  gtk_widget_show (appwin);

  {
    GdkCursor *cursor;

    cursor = gdk_cursor_new_for_display (gtk_widget_get_display (previewarea),
                                         GDK_HAND2);
    gdk_window_set_cursor (previewarea->window, cursor);
    gdk_cursor_unref (cursor);
  }

  image_setup (drawable, TRUE);

  if (gimp_dialog_run (GIMP_DIALOG (appwin)) == GTK_RESPONSE_OK)
    run = TRUE;

  gtk_widget_destroy (appwin);
  g_free (preview_rgb_data);

  return run;
}
