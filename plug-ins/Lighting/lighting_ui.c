/* Lighting Effects - A plug-in for GIMP
 *
 * Dialog creation and updaters, callbacks and event-handlers
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "lighting_ui.h"
#include "lighting_main.h"
#include "lighting_image.h"
#include "lighting_apply.h"
#include "lighting_preview.h"

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


extern LightingValues mapvals;

static GtkWidget   *appwin            = NULL;
static GtkNotebook *options_note_book = NULL;

/*
static GtkWidget   *bump_page         = NULL;
static GtkWidget   *env_page          = NULL;
*/

GdkGC     *gc          = NULL;
GtkWidget *previewarea = NULL;

static GtkWidget *pointlightwid = NULL;
static GtkWidget *dirlightwid   = NULL;

/*
static gint bump_page_pos = -1;
static gint env_page_pos  = -1;
*/

static void create_main_notebook      (GtkWidget *container);

#ifdef _LIGHTNING_UNUSED_CODE
static void xyzval_update             (GtkEntry *entry);
#endif

static void toggle_update             (GtkWidget *widget,
				       gpointer   data);
/*
static void togglebump_update         (GtkWidget *widget,
				       gpointer   data);

static void toggleenvironment_update  (GtkWidget *widget,
				       gpointer   data);
*/

static void lightmenu_callback        (GtkWidget *widget,
				       gpointer   data);

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
/*
static GtkWidget *create_bump_page        (void);

static GtkWidget *create_environment_page (void);
*/

#ifdef _LIGHTNING_UNUSED_CODE
/**********************************************************/
/* Update entry fields that affect the preview parameters */
/**********************************************************/

static void
xyzval_update (GtkEntry *entry)
{
  gdouble *valueptr;
  gdouble  value;

  valueptr = (gdouble *) g_object_get_data (G_OBJECT (entry), "ValuePtr");
  value = atof (gtk_entry_get_text (entry));

  *valueptr = value;
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

/*
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
				gtk_label_new (_("Bumpmap")));
    }
  else
  {
      gtk_notebook_remove_page (options_note_book, bump_page_pos);
      if (bump_page_pos < env_page_pos)
        env_page_pos--;
      bump_page_pos = 0;
    }
}
*/

/*************************************/
/* Toggle environment mapping update */
/*************************************/

/*
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
				gtk_label_new (_("Environment")));
    }
  else
    {
      gtk_notebook_remove_page (options_note_book, env_page_pos);
      if (env_page_pos < bump_page_pos)
        bump_page_pos--;
      env_page_pos = 0;
    }
}
*/


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

  interactive_preview_callback(NULL);

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
/*
static void
zoomout_callback (GtkWidget *widget)
{
  mapvals.preview_zoom_factor *= 0.5;
  draw_preview_image (TRUE);
}
*/
/*********************************************/
/* Main window "+" (zoom out) button callback */
/*********************************************/
/*
static void
zoomin_callback (GtkWidget *widget)
{
  mapvals.preview_zoom_factor *= 2.0;
  draw_preview_image (TRUE);
}
*/
/**********************************************/
/* Main window "Apply" button callback.       */
/* Render to GIMP image, close down and exit. */
/**********************************************/

static gint
bumpmap_constrain (gint32   image_id,
		   gint32   drawable_id,
		   gpointer data)
{
  if (drawable_id == -1)
    return TRUE;

  return  ((gimp_drawable_width (drawable_id) ==
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
  /*GtkWidget *spinbutton;*/
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
  /*
  toggle = gtk_check_button_new_with_label (_("Use Bump Mapping"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.bump_mapped);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
		    G_CALLBACK (togglebump_update),
		    &mapvals.bump_mapped);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Enable/disable bump-mapping (image depth)"),
			   NULL);

  toggle = gtk_check_button_new_with_label (_("Use Environment Mapping"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.env_mapped);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
		    G_CALLBACK (toggleenvironment_update),
		    &mapvals.env_mapped);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Enable/disable environment mapping (reflection)"),
			   NULL);
  */
  toggle = gtk_check_button_new_with_mnemonic (_("T_ransparent Background"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.transparent_background);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (toggle_update),
                    &mapvals.transparent_background);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Make destination image transparent where bump "
			     "height is zero"),NULL);

  toggle = gtk_check_button_new_with_mnemonic (_("Cre_ate New Image"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.create_new_image);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mapvals.create_new_image);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Create a new image when applying filter"), NULL);

  toggle = gtk_check_button_new_with_mnemonic (_("High _Quality Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.previewquality);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (toggle_update),
                    &mapvals.previewquality);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Enable/disable high quality preview"), NULL);

  /* Antialiasing options */

  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  toggle = gtk_check_button_new_with_mnemonic (_("E_nable Antialiasing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.antialiasing);
  gtk_frame_set_label_widget (GTK_FRAME (frame), toggle);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mapvals.antialiasing);

  table = gtk_table_new (2, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  gtk_widget_set_sensitive (table, mapvals.antialiasing);
  g_object_set_data (G_OBJECT (toggle), "set_sensitive", table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("_Depth:"), 0, 6,
			      mapvals.max_depth, 1.0, 5.0, 0.5, 1.0,
			      1, TRUE, 0, 0,
			      _("Antialiasing quality. Higher is better, "
				"but slower"), NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.max_depth);

 adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("T_hreshold:"), 0, 6,
			      mapvals.pixel_treshold, 0.01, 1000.0, 1.0, 15.0, 2,
			      TRUE, 0, 0,
			      _("Stop when pixel differences are smaller than "
			        "this value"), NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.pixel_treshold);

  /*
  spinbutton = gimp_spin_button_new (&adj, mapvals.pixel_treshold,
				     0.001, 1000, 0.1, 1, 1, 0, 3);
  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (gimp_double_adjustment_update),
		    &mapvals.pixel_treshold);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Threshold:"), 1.0, 1.0,
			     spinbutton, 1, TRUE);

  gimp_help_set_help_data (spinbutton,
			   _("Stop when pixel differences are smaller than "
			     "this value"), NULL);
  */
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

  optionmenu =
    gimp_int_option_menu_new (FALSE,
                              G_CALLBACK (lightmenu_callback),
			      &mapvals.lightsource.type,
			      mapvals.lightsource.type,

			      _("None"),        NO_LIGHT,          NULL,
			      _("Directional"), DIRECTIONAL_LIGHT, NULL,
			      _("Point"),       POINT_LIGHT,       NULL,
			      /* _("Spot"),     SPOT_LIGHT,        NULL, */

			      NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("L_ight Type:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  gimp_help_set_help_data (optionmenu, _("Type of light source to apply"), NULL);

  colorbutton = gimp_color_button_new (_("Select Lightsource Color"),
				       64, 16,
				       &mapvals.lightsource.color,
				       GIMP_COLOR_AREA_FLAT);
  g_signal_connect (colorbutton, "color_changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &mapvals.lightsource.color);
  g_signal_connect (colorbutton, "color_changed",
		    G_CALLBACK (interactive_preview_callback),
		    NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Lig_ht Color:"), 1.0, 0.5,
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

  spin_pos_x = gimp_spin_button_new (&adj, mapvals.lightsource.position.x,
				     -G_MAXFLOAT, G_MAXFLOAT,
				     0.1, 1.0, 1.0, 0.0, 2);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("_X:"), 1.0, 0.5,
			     spin_pos_x, 1, TRUE);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.lightsource.position.x);

  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (interactive_preview_callback),
		    NULL);

  gimp_help_set_help_data ( spin_pos_x,
			   _("Light source X position in XYZ space"), NULL);

  spin_pos_y = gimp_spin_button_new (&adj, mapvals.lightsource.position.y,
				     -G_MAXFLOAT, G_MAXFLOAT,
				     0.1, 1.0, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("_Y:"), 1.0, 0.5,
			      spin_pos_y, 1, TRUE);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.lightsource.position.y);
  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (interactive_preview_callback),
		    NULL);


  gimp_help_set_help_data ( spin_pos_y,
			   _("Light source Y position in XYZ space"), NULL);

  spin_pos_z = gimp_spin_button_new (&adj, mapvals.lightsource.position.z,
				     -G_MAXFLOAT, G_MAXFLOAT,
				     0.1, 1.0, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("_Z:"), 1.0, 0.5,
			      spin_pos_z, 1, TRUE);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.lightsource.position.z);
  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (interactive_preview_callback),
		    NULL);

  gimp_help_set_help_data (  spin_pos_z,
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

  spin_dir_x = gimp_spin_button_new (&adj, mapvals.lightsource.direction.x,
				     -1.0, 1.0, 0.01, 0.1, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("X:"), 1.0, 0.5,
			     spin_dir_x, 1, TRUE);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.lightsource.direction.x);
  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (interactive_preview_callback),
		    NULL);


  gimp_help_set_help_data (spin_dir_x,
			   _("Light source X direction in XYZ space"), NULL);

  spin_dir_y = gimp_spin_button_new (&adj, mapvals.lightsource.direction.y,
				     -1.0, 1.0, 0.01, 0.1, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Y:"), 1.0, 0.5,
			     spin_dir_y, 1, TRUE);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.lightsource.direction.y);
  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (interactive_preview_callback),
		    NULL);

  gimp_help_set_help_data (spin_dir_y,
			   _("Light source Y direction in XYZ space"), NULL);

  spin_dir_z = gimp_spin_button_new (&adj, mapvals.lightsource.direction.z,
				     -1.0, 1.0, 0.01, 0.1, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Z:"), 1.0, 0.5,
			     spin_dir_z, 1, TRUE);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.lightsource.direction.z);
  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (interactive_preview_callback),
		    NULL);

  gimp_help_set_help_data (spin_dir_z,
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
  GtkWidget *label;
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
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
				     _("_Ambient:"), 1.0, 0.5,
				     pixmap, 1, FALSE);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.ambient_int,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.material.ambient_int);
  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (interactive_preview_callback),
		    NULL);

  gtk_widget_show (spinbutton);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

  gimp_help_set_help_data (spinbutton,
			   _("Amount of original color to show where no "
			     "direct light falls"), NULL);

  pixmap = gimp_pixmap_new (amb2_xpm);
  gtk_table_attach (GTK_TABLE (table), pixmap, 3, 4, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (pixmap);

  /* Diffuse intensity */

  pixmap = gimp_pixmap_new (diffint1_xpm);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
				     _("_Diffuse:"), 1.0, 0.5,
				     pixmap, 1, FALSE);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.diffuse_int,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.material.diffuse_int);
  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (interactive_preview_callback),
		    NULL);

  gtk_widget_show (spinbutton);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

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
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
				     _("D_iffuse:"), 1.0, 0.5,
				     pixmap, 1, FALSE);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.diffuse_ref,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.material.diffuse_ref);
  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (interactive_preview_callback),
		    NULL);


  gtk_widget_show (spinbutton);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

  gimp_help_set_help_data (spinbutton,
			   _("Higher values makes the object reflect more "
			     "light (appear lighter)"), NULL);

  pixmap = gimp_pixmap_new (diffref2_xpm);
  gtk_table_attach (GTK_TABLE (table), pixmap, 3, 4, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (pixmap);

  /* Specular reflection */

  pixmap = gimp_pixmap_new (specref1_xpm);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
				     _("_Specular:"), 1.0, 0.5,
				     pixmap, 1, FALSE);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.specular_ref,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.material.specular_ref);
  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (interactive_preview_callback),
		    NULL);


  gtk_widget_show (spinbutton);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

  gimp_help_set_help_data (spinbutton,
			   _("Controls how intense the highlights will be"),
			   NULL);

  pixmap = gimp_pixmap_new (specref2_xpm);
  gtk_table_attach (GTK_TABLE (table), pixmap, 3, 4, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (pixmap);

  /* Highlight */

  pixmap = gimp_pixmap_new (high1_xpm);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
				     _("_Highlight:"), 1.0, 0.5,
				     pixmap, 1, FALSE);

  spinbutton = gimp_spin_button_new (&adj, mapvals.material.highlight,
				     0, G_MAXFLOAT, 0.1, 1.0, 1.0, 0.0, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 2, 3, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.material.highlight);
  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (interactive_preview_callback),
		    NULL);

  gtk_widget_show (spinbutton);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

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

/* Create Bump mapping page */

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

  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  toggle = gtk_check_button_new_with_mnemonic (_("E_nable Bump Mapping"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.bump_mapped);
  gtk_frame_set_label_widget (GTK_FRAME (frame), toggle);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mapvals.bump_mapped);

  gimp_help_set_help_data (toggle,
			   _("Enable/disable bump-mapping (image depth)"),
			   NULL);

  table = gtk_table_new (6, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  gtk_widget_set_sensitive (table, mapvals.bump_mapped);
  g_object_set_data (G_OBJECT (toggle), "set_sensitive", table);

  optionmenu = gtk_option_menu_new ();
  menu = gimp_drawable_menu_new (bumpmap_constrain, bumpmap_drawable_callback,
				 NULL, mapvals.bumpmap_id);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Bumpm_ap Image:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  optionmenu =
    gimp_int_option_menu_new (FALSE,
			      G_CALLBACK (mapmenu2_callback),
			      &mapvals.bumpmaptype, mapvals.bumpmaptype,
			      _("Linear"),      LINEAR_MAP,      NULL,
			      _("Logarithmic"), LOGARITHMIC_MAP, NULL,
			      _("Sinusoidal"),  SINUSOIDAL_MAP,  NULL,
			      _("Spherical"),   SPHERICAL_MAP,   NULL,
			      NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Cu_rve:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  spinbutton = gimp_spin_button_new (&adj, mapvals.bumpmax,
				     0, G_MAXFLOAT, 0.01, 0.1, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Ma_ximum Height:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.bumpmax);

  gimp_help_set_help_data (spinbutton,
			   _("Maximum height for bumps"),
			   NULL);

  spinbutton = gimp_spin_button_new (&adj, mapvals.bumpmin,
				     0, G_MAXFLOAT, 0.01, 0.1, 1.0, 0.0, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("M_inimum Height:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mapvals.bumpmin);

   gimp_help_set_help_data (spinbutton,
			   _("Minimum height for bumps"),
			   NULL);

  toggle = gtk_check_button_new_with_mnemonic (_("Auto_stretch to Fit Value Range"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.bumpstretch);
  gtk_table_attach_defaults (GTK_TABLE (table), toggle, 0, 2, 4, 5);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mapvals.bumpstretch);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Fit into value range"),
			   NULL);

  gtk_widget_show (page);

  return page;
}

static GtkWidget *
create_environment_page (void)
{
  GtkWidget *page;
  GtkWidget *toggle;
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *optionmenu;
  GtkWidget *menu;

  page = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page), 4);

  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  toggle = gtk_check_button_new_with_mnemonic (_("E_nable Environment Mapping"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.env_mapped);
  gtk_frame_set_label_widget (GTK_FRAME (frame), toggle);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mapvals.env_mapped);

  gimp_help_set_help_data (toggle,
			   _("Enable/disable environment-mapping (reflection)"),
			   NULL);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  gtk_widget_set_sensitive (table, mapvals.env_mapped);
  g_object_set_data (G_OBJECT (toggle), "set_sensitive", table);

  optionmenu = gtk_option_menu_new ();
  menu = gimp_drawable_menu_new (envmap_constrain, envmap_drawable_callback,
				 NULL, mapvals.envmap_id);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("En_vironment Image:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  gimp_help_set_help_data (optionmenu,
			   _("Environment image to use"),
			   NULL);

  gtk_widget_show (page);

  return page;
}

/*****************************/
/* Create notebook and pages */
/*****************************/

static void
create_main_notebook (GtkWidget *container)
{
  GtkWidget *page;

  options_note_book = GTK_NOTEBOOK (gtk_notebook_new ());
  gtk_container_add (GTK_CONTAINER (container),
		     GTK_WIDGET (options_note_book));

  page = create_options_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new_with_mnemonic (_("Op_tions")));

  page = create_light_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new_with_mnemonic (_("_Light")));

  page = create_material_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new_with_mnemonic (_("_Material")));

  page = create_bump_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new_with_mnemonic (_("_Bump Map")));

  page = create_environment_page ();
  gtk_notebook_append_page (options_note_book, page,
			    gtk_label_new_with_mnemonic (_("_Environment Map")));

  /*
  if (mapvals.bump_mapped == TRUE)
    {
      bump_page = create_bump_page ();
      bump_page_pos = g_list_length (options_note_book->children);
      gtk_notebook_append_page (options_note_book, bump_page,
				gtk_label_new (_("Bumpmap")));
    }

  if (mapvals.env_mapped == TRUE)
    {
      env_page = create_environment_page ();
      env_page_pos = g_list_length (options_note_book->children);
      gtk_notebook_append_page (options_note_book, env_page,
				gtk_label_new (_("Environment")));
    }
  */
  gtk_widget_show (GTK_WIDGET (options_note_book));
}

/********************************/
/* Create and show main dialog. */
/********************************/

gboolean
main_dialog (GimpDrawable *drawable)
{
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *toggle;
  gboolean   run = FALSE;

  /*
  GtkWidget *image;
  */

  gimp_ui_init ("Lighting", FALSE);

  appwin = gimp_dialog_new (_("Lighting Effects"), "Lighting",
                            NULL, 0,
			    gimp_standard_help_func,
			    "filters/lighting.html",

			    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			    GTK_STOCK_OK,     GTK_RESPONSE_OK,

			    NULL);

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
  gtk_widget_set_size_request (previewarea, PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_widget_set_events (previewarea, (GDK_EXPOSURE_MASK |
				       GDK_BUTTON1_MOTION_MASK |
				       GDK_BUTTON_PRESS_MASK |
				       GDK_BUTTON_RELEASE_MASK));
  g_signal_connect (previewarea, "event",
                    G_CALLBACK (preview_events),
                    previewarea);
  gtk_container_add (GTK_CONTAINER (frame), previewarea);
  gtk_widget_show (previewarea);

  /* create preview options, frame and vbox */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_widget_show (hbox);


  button = gtk_button_new_with_mnemonic (_("_Update"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (preview_callback),
                    NULL);
  gtk_widget_show (button);

  gimp_help_set_help_data (button, _("Recompute preview image"), NULL);

  toggle = gtk_check_button_new_with_mnemonic (_("I_nteractive"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				mapvals.interactive_preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mapvals.interactive_preview);
  g_signal_connect (toggle, "toggled",
		    G_CALLBACK (interactive_preview_callback),
		    NULL);


  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
			   _("Enable/disable real time preview of changes"),
			   NULL);

  frame = gtk_frame_new (_("Preview Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (frame);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  create_main_notebook (main_hbox);

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

  if (preview_rgb_data != NULL)
    g_free (preview_rgb_data);

  gtk_widget_destroy (appwin);

  return run;
}
