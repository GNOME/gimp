/**************************************************************/
/* Dialog creation and updaters, callbacks and event-handlers */
/**************************************************************/

#include "lighting_ui.h"
#include "lighting_pixmaps.h"

extern LightingValues mapvals;

GckApplicationWindow *appwin            = NULL;
GtkWidget            *color_select_diag = NULL;
GtkNotebook          *options_note_book = NULL;
GtkWidget            *bump_page         = NULL;
GtkWidget            *env_page          = NULL;
GtkTooltips          *tooltips          = NULL;
GdkGC                *gc                = NULL;

GtkWidget *previewarea,*pointlightwid,*dirlightwid;
GtkWidget *imagewid,*waveswid,*bump_label,*env_label;
GtkWidget *xentry,*yentry,*zentry;

GckRGB old_light_color;

gint color_dialog_id = -1;
gint bump_page_pos,env_page_pos;

guint left_button_pressed = FALSE, light_hit = FALSE;
guint32 blackpixel,whitepixel;

GckScaleValues sample_scale_vals = {128,3.0,1.0,6.0,1.0,1.0,1.0,GTK_UPDATE_CONTINUOUS,TRUE};

gchar *light_labels[] =
  {
    N_("Point light"),
    N_("Directional light"),
    N_("Spot light"),
    N_("No light"),
     NULL
  };

gchar *maptype_labels[] =
  {
    N_("From image"),
    N_("Waves"),
     NULL
  };

gchar *curvetype_labels[] =
  {
    N_("Linear"),
    N_("Logarithmic"),
    N_("Sinusoidal"),
    N_("Spherical"),
     NULL
  };

/**********/
/* Protos */
/**********/

void create_main_dialog   (void);
void create_main_notebook (GtkWidget *);

/**************************/
/* Callbacks and updaters */
/**************************/

gint preview_events           (GtkWidget *area, GdkEvent *event);
#ifdef _LIGHTNING_UNUSED_CODE
void xyzval_update            (GtkEntry *entry);
#endif
void entry_update             (GtkEntry *entry);
void scale_update             (GtkScale *scale);
void toggle_update            (GtkWidget *button);
void toggleanti_update        (GtkWidget *button);
void toggletips_update        (GtkWidget *button);
void toggletrans_update       (GtkWidget *button);
void togglebump_update        (GtkWidget *button);
void toggleenvironment_update (GtkWidget *button);
void togglerefraction_update  (GtkWidget *button);

void lightmenu_callback       (GtkWidget *widget, gpointer client_data);
void preview_callback         (GtkWidget *widget);
void apply_callback           (GtkWidget *widget);
void exit_callback            (GtkWidget *widget);
void color_ok_callback        (GtkWidget *widget, gpointer client_data);
gint color_delete_callback (GtkWidget *widget, GdkEvent *event, gpointer client_data);
void color_changed_callback   (GtkColorSelection *colorsel, gpointer client_data);
void color_cancel_callback    (GtkWidget *widget, gpointer client_data);
void light_color_callback     (GtkWidget *widget, gpointer client_data);

GtkWidget *create_bump_page        (void);
GtkWidget *create_environment_page (void);

gint bumpmap_constrain         (gint32 image_id, gint32 drawable_id, gpointer data);
void bumpmap_drawable_callback (gint32 id, gpointer data);

gint envmap_constrain         (gint32 image_id, gint32 drawable_id, gpointer data);
void envmap_drawable_callback (gint32 id, gpointer data);

/******************/
/* Implementation */
/******************/

#ifdef _LIGHTNING_UNUSED_CODE
/**********************************************************/
/* Update entry fields that affect the preview parameters */
/**********************************************************/

void xyzval_update(GtkEntry *entry)
{
  gdouble *valueptr;
  gdouble value;

  valueptr=(gdouble *)gtk_object_get_data(GTK_OBJECT(entry),"ValuePtr");
  value = atof(gtk_entry_get_text(entry));

  *valueptr=value;
}
#endif

/*********************/
/* Std. entry update */
/*********************/

void entry_update(GtkEntry *entry)
{
  gdouble *valueptr;
  gdouble value;

  valueptr=(gdouble *)gtk_object_get_data(GTK_OBJECT(entry),"ValuePtr");
  value = atof(gtk_entry_get_text(entry));

  *valueptr=value;
}

/*********************/
/* Std. scale update */
/*********************/

void scale_update(GtkScale *scale)
{
  gdouble *valueptr;
  GtkAdjustment *adjustment;

  valueptr=(gdouble *)gtk_object_get_data(GTK_OBJECT(scale),"ValuePtr");
  adjustment=gtk_range_get_adjustment(GTK_RANGE(scale));

  *valueptr=(gdouble)adjustment->value;
}

/**********************/
/* Std. toggle update */
/**********************/

void toggle_update(GtkWidget *button)
{
  gint *value;

  value=(gint *)gtk_object_get_data(GTK_OBJECT(button),"ValuePtr");
  *value=!(*value);
}

void togglestretch_update(GtkWidget *button)
{
  gint *value;

  value=(gint *)gtk_object_get_data(GTK_OBJECT(button),"ValuePtr");
  *value=!(*value);
}

void togglequality_update(GtkWidget *button)
{
  gint *value;

  value=(gint *)gtk_object_get_data(GTK_OBJECT(button),"ValuePtr");
  *value=!(*value);
  
  draw_preview_image(TRUE);
}

/**********************************/
/* Toggle refractive layer update */
/**********************************/

void togglerefraction_update(GtkWidget *button)
{
  gint *value;

  value=(gint *)gtk_object_get_data(GTK_OBJECT(button),"ValuePtr");
  *value=!(*value);
}

/*****************************/
/* Toggle bumpmapping update */
/*****************************/

void togglebump_update(GtkWidget *button)
{
  gint *value;

  value=(gint *)gtk_object_get_data(GTK_OBJECT(button),"ValuePtr");
  *value=!(*value);
  
  if (mapvals.bump_mapped==TRUE)
    {
      bump_page_pos = g_list_length(options_note_book->children);

      bump_page = create_bump_page();
      bump_label = gtk_label_new( _("Bump"));
      gtk_notebook_append_page(options_note_book,bump_page,bump_label);
    }
  else
    {     
      gtk_notebook_remove_page(options_note_book,bump_page_pos);
      if (bump_page_pos<env_page_pos)
        env_page_pos--;
      bump_page_pos = 0;
    }
}

/*************************************/
/* Toggle environment mapping update */
/*************************************/

void toggleenvironment_update(GtkWidget *button)
{
  gint *value;

  value=(gint *)gtk_object_get_data(GTK_OBJECT(button),"ValuePtr");
  *value=!(*value);

  if (mapvals.env_mapped==TRUE)
    {
      env_page_pos = g_list_length(options_note_book->children);

      env_page = create_environment_page();
      env_label = gtk_label_new( _("Env"));

      gtk_notebook_append_page(options_note_book,env_page,env_label);
    }
  else
    {
      gtk_notebook_remove_page(options_note_book,env_page_pos);
      if (env_page_pos<bump_page_pos)
        bump_page_pos--;
      env_page_pos = 0;
    }
}

/******************************/
/* Antialiasing toggle update */
/******************************/

void toggleanti_update(GtkWidget *button)
{
  gint *value;

  value=(gint *)gtk_object_get_data(GTK_OBJECT(button),"ValuePtr");
  *value=!(*value);
}

/**************************/
/* Tooltips toggle update */
/**************************/

void toggletips_update(GtkWidget *button)
{
  gint *value;

  value=(gint *)gtk_object_get_data(GTK_OBJECT(button),"ValuePtr");
  *value=!(*value);

  if (tooltips!=NULL)
    {
      if (mapvals.tooltips_enabled==TRUE)
        gtk_tooltips_enable(tooltips);
      else
        gtk_tooltips_disable(tooltips);
    }
}

/****************************************/
/* Transparent background toggle update */
/****************************************/

void toggletrans_update(GtkWidget *button)
{
  gint *value;

  value=(gint *)gtk_object_get_data(GTK_OBJECT(button),"ValuePtr");
  *value=!(*value);

  draw_preview_image(TRUE);
}

/*****************************************/
/* Main window light type menu callback. */
/*****************************************/

void lightmenu_callback(GtkWidget *widget, gpointer client_data)
{
  mapvals.lightsource.type=(LightType)gtk_object_get_data(GTK_OBJECT(widget),"_GckOptionMenuItemID");

  if (mapvals.lightsource.type==POINT_LIGHT)
    {
      gtk_widget_hide(dirlightwid);
      gtk_widget_show(pointlightwid);
    }
  else if (mapvals.lightsource.type==DIRECTIONAL_LIGHT)
    {
      gtk_widget_hide(pointlightwid);
      gtk_widget_show(dirlightwid);
    }
  else
    {
      gtk_widget_hide(pointlightwid);
      gtk_widget_unmap(pointlightwid);
      gtk_widget_hide(dirlightwid);
      gtk_widget_unmap(dirlightwid);
    }
}

void mapmenu1_callback(GtkWidget *widget, gpointer client_data)
{
/*  mapvals.bumptype=(gint)gtk_object_get_data(GTK_OBJECT(widget),"_GckOptionMenuItemID");

  if (mapvals.bumptype==IMAGE_BUMP)
    {
      gtk_widget_hide(waveswid);
      gtk_widget_show(imagewid);
    }
  else
    {
      gtk_widget_hide(imagewid);
      gtk_widget_show(waveswid);
    } */
}

void mapmenu2_callback(GtkWidget *widget, gpointer client_data)
{
  mapvals.bumpmaptype=(gint)gtk_object_get_data(GTK_OBJECT(widget),"_GckOptionMenuItemID");
  draw_preview_image(TRUE);
}

/******************************************/
/* Main window "Preview!" button callback */
/******************************************/

void preview_callback(GtkWidget *widget)
{
  draw_preview_image(TRUE);
}

/*********************************************/
/* Main window "-" (zoom in) button callback */
/*********************************************/

void zoomout_callback(GtkWidget *widget)
{
  mapvals.preview_zoom_factor*=0.5;
  draw_preview_image(TRUE);
}

/*********************************************/
/* Main window "+" (zoom out) button callback */
/*********************************************/

void zoomin_callback(GtkWidget *widget)
{
  mapvals.preview_zoom_factor*=2.0;
  draw_preview_image(TRUE);
}

/**********************************************/
/* Main window "Apply" button callback.       */ 
/* Render to GIMP image, close down and exit. */
/**********************************************/

void apply_callback(GtkWidget *widget)
{
  if (preview_rgb_data!=NULL)
    free(preview_rgb_data);
  
  if (image!=NULL)
    gdk_image_destroy(image);

  gtk_object_unref(GTK_OBJECT(tooltips));
  
  gck_application_window_destroy(appwin);
  gdk_flush();

  compute_image();

  gtk_main_quit();
}

/*************************************************************/
/* Main window "Cancel" button callback. Shut down and exit. */
/*************************************************************/

void exit_callback(GtkWidget *widget)
{
  if (preview_rgb_data!=NULL)
    free(preview_rgb_data);

  if (image!=NULL)
    gdk_image_destroy(image);

  if (backbuf.image!=NULL)
    gdk_image_destroy(backbuf.image);
  
  gtk_object_unref(GTK_OBJECT(tooltips));
  
  gck_application_window_destroy(appwin);
  
  gtk_main_quit();
}

/*************************************/
/* Color dialog "Ok" button callback */
/*************************************/

void color_ok_callback(GtkWidget *widget, gpointer client_data)
{
  gtk_widget_destroy(color_select_diag);
  color_select_diag=NULL;
}

void color_changed_callback(GtkColorSelection *colorsel, gpointer client_data)
{
  gdouble color[3];
  
  gtk_color_selection_get_color(colorsel, color);
  mapvals.lightsource.color.r=color[0];
  mapvals.lightsource.color.g=color[1];
  mapvals.lightsource.color.b=color[2];
}

gint color_delete_callback(GtkWidget *widget, GdkEvent *event, gpointer client_data)
{
  color_select_diag=NULL;
  return FALSE;
}

/********************************************/
/* Color dialog "Cancel" button callback.   */
/* Close dialog & restore old color values. */
/********************************************/

void color_cancel_callback(GtkWidget *widget, gpointer client_data)
{
  gtk_widget_destroy(color_select_diag);
  color_select_diag=NULL;
}

void light_color_callback(GtkWidget *widget, gpointer client_data)
{
  GtkColorSelectionDialog *csd;

  if (mapvals.lightsource.type!=NO_LIGHT && color_select_diag==NULL)
    {
      color_select_diag=gtk_color_selection_dialog_new("Select lightsource color");
      gtk_window_position (GTK_WINDOW (color_select_diag), GTK_WIN_POS_MOUSE);
      gtk_widget_show(color_select_diag);
      csd=GTK_COLOR_SELECTION_DIALOG(color_select_diag);
      gtk_signal_connect(GTK_OBJECT(csd),"delete_event",
        (GtkSignalFunc)color_delete_callback,(gpointer)color_select_diag);
      gtk_signal_connect(GTK_OBJECT(csd->ok_button),"clicked",
        (GtkSignalFunc)color_ok_callback,(gpointer)color_select_diag);
      gtk_signal_connect(GTK_OBJECT(csd->cancel_button),"clicked",
        (GtkSignalFunc)color_cancel_callback,(gpointer)color_select_diag);
      gtk_signal_connect(GTK_OBJECT(csd->colorsel),"color_changed",
        (GtkSignalFunc)color_changed_callback,(gpointer)color_select_diag);
    }
}

gint bumpmap_constrain(gint32 image_id, gint32 drawable_id, gpointer data)
{
  if (drawable_id == -1)
    return(TRUE);

  return (gimp_drawable_is_gray(drawable_id) && !gimp_drawable_has_alpha(drawable_id) &&
          gimp_drawable_width(drawable_id)==gimp_drawable_width(mapvals.drawable_id) &&
          gimp_drawable_height(drawable_id)==gimp_drawable_height(mapvals.drawable_id));
}

void bumpmap_drawable_callback(gint32 id, gpointer data)
{
  mapvals.bumpmap_id = id;
}

gint envmap_constrain(gint32 image_id, gint32 drawable_id, gpointer data)
{
  if (drawable_id == -1)
    return(TRUE);

  return (!gimp_drawable_is_gray(drawable_id) && !gimp_drawable_has_alpha(drawable_id));
}

void envmap_drawable_callback(gint32 id, gpointer data)
{
  mapvals.envmap_id = id;
  env_width = gimp_drawable_width(mapvals.envmap_id);
  env_height = gimp_drawable_height(mapvals.envmap_id);
}

/******************************/
/* Preview area event handler */
/******************************/

gint preview_events(GtkWidget *area, GdkEvent  *event)
{
  switch (event->type)
    {
      case GDK_EXPOSE:

        /* Is this the first exposure? */
        /* =========================== */

        if (!gc)
          {
            gc=gdk_gc_new(area->window);
            draw_preview_image(TRUE);
          }
        else
          draw_preview_image(FALSE);
        break; 
      case GDK_ENTER_NOTIFY:
        break;
      case GDK_LEAVE_NOTIFY:
        break;
      case GDK_BUTTON_PRESS:
        light_hit=check_light_hit(event->button.x,event->button.y);
        left_button_pressed=TRUE;
        break;
      case GDK_BUTTON_RELEASE:
        if (light_hit==TRUE)
          draw_preview_image(TRUE);
        left_button_pressed=FALSE;
        break;
      case GDK_MOTION_NOTIFY:
        if (left_button_pressed==TRUE && light_hit==TRUE)
          update_light(event->motion.x,event->motion.y);
        break;
      default:
        break;
    }

  return(FALSE);
}

/***********************/
/* Dialog constructors */
/***********************/

GtkWidget *create_options_page(void)
{
  GtkWidget *page,*frame,*toggletrans,*toggleimage;
  GtkWidget *toggletips,*togglequality,*vbox,*hbox;
  GtkWidget *toggleenvironment,*toggleanti,*togglebump;
  GtkWidget *widget1,*widget2,*label;

  /* General options page */
  /* ==================== */

  page = gck_vbox_new(NULL,FALSE,FALSE,FALSE,0,0,0);

  frame=gck_frame_new(_("General options"),page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,5);
  vbox=gck_vbox_new(frame,FALSE,FALSE,FALSE,0,5,5);

  togglebump=gck_checkbutton_new(_("Use bump mapping"),vbox,mapvals.bump_mapped,
    (GtkSignalFunc)togglebump_update);
  toggleenvironment=gck_checkbutton_new(_("Use environment mapping"),vbox,mapvals.env_mapped,
    (GtkSignalFunc)toggleenvironment_update);
/*  togglerefraction=gck_checkbutton_new("Use refraction mapping",vbox,mapvals.ref_mapped,
    (GtkSignalFunc)togglerefraction_update); */
  toggletrans=gck_checkbutton_new(_("Transparent background"),vbox,mapvals.transparent_background,
    (GtkSignalFunc)toggletrans_update);
  toggleimage=gck_checkbutton_new(_("Create new image"),vbox,mapvals.create_new_image,
    (GtkSignalFunc)toggle_update);
  togglequality=gck_checkbutton_new(_("High preview quality"),vbox,mapvals.previewquality,
    (GtkSignalFunc)togglequality_update);
  toggletips=gck_checkbutton_new(_("Enable tooltips"),vbox,mapvals.tooltips_enabled,
    (GtkSignalFunc)toggletips_update);

  gtk_tooltips_set_tip(tooltips,togglebump,_("Enable/disable bump-mapping (image depth)"),NULL);
  gtk_tooltips_set_tip(tooltips,toggleenvironment,_("Enable/disable environment mapping (reflection)"),NULL);
/*  gtk_tooltips_set_tips(tooltips,togglerefraction,"Enable/disable refractive layer"); */
  gtk_tooltips_set_tip(tooltips,toggletrans, _("Make destination image transparent where bump height is zero"),NULL);
  gtk_tooltips_set_tip(tooltips,toggleimage, _("Create a new image when applying filter"),NULL);
  gtk_tooltips_set_tip(tooltips,togglequality, _("Enable/disable high quality previews"),NULL);
  gtk_tooltips_set_tip(tooltips,toggletips, _("Enable/disable tooltip messages"),NULL); 

  gtk_object_set_data(GTK_OBJECT(togglebump), "ValuePtr",(gpointer)&mapvals.bump_mapped);
  gtk_object_set_data(GTK_OBJECT(toggleenvironment), "ValuePtr",(gpointer)&mapvals.env_mapped);
/*  gtk_object_set_data(GTK_OBJECT(togglerefraction),"ValuePtr",(gpointer)&mapvals.ref_mapped); */
  gtk_object_set_data(GTK_OBJECT(toggletrans),"ValuePtr",(gpointer)&mapvals.transparent_background);
  gtk_object_set_data(GTK_OBJECT(toggleimage),"ValuePtr",(gpointer)&mapvals.create_new_image);
  gtk_object_set_data(GTK_OBJECT(togglequality),"ValuePtr",(gpointer)&mapvals.previewquality);
  gtk_object_set_data(GTK_OBJECT(toggletips),"ValuePtr", (gpointer)&mapvals.tooltips_enabled);

  gtk_widget_show(togglebump);
  gtk_widget_show(toggleenvironment);
  gtk_widget_show(toggletrans);
  gtk_widget_show(toggleimage);
  gtk_widget_show(togglequality);
  gtk_widget_show(toggletips);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  frame=gck_frame_new(_("Antialiasing options"),page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,5);
  vbox=gck_vbox_new(frame,FALSE,FALSE,FALSE,5,5,5);

  toggleanti=gck_checkbutton_new(_("Enable antialiasing"),vbox,mapvals.antialiasing,
    (GtkSignalFunc)toggleanti_update);
  gtk_object_set_data(GTK_OBJECT(toggleanti),"ValuePtr",(gpointer)&mapvals.antialiasing);
  gtk_tooltips_set_tip(tooltips,toggleanti,_("Enable/disable jagged edges removal (antialiasing)"),NULL);
   
  hbox=gck_hbox_new(vbox,FALSE,TRUE,TRUE,0,0,0);
  
  gtk_widget_show(toggleanti);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  vbox=gck_vbox_new(hbox,TRUE,FALSE,TRUE,0,0,0);
  frame=gck_frame_new(NULL,vbox,GTK_SHADOW_NONE,TRUE,TRUE,0,0);
  label=gck_label_aligned_new(_("Depth:"),frame,GCK_ALIGN_RIGHT,GCK_ALIGN_BOTTOM);
  gtk_widget_show(label);
  gtk_widget_show(frame);
  
  frame=gck_frame_new(NULL,vbox,GTK_SHADOW_NONE,TRUE,TRUE,0,0);
  label=gck_label_aligned_new(_("Threshold:"),frame,GCK_ALIGN_RIGHT,GCK_ALIGN_CENTERED);
  gtk_widget_show(label);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  vbox=gck_vbox_new(hbox,TRUE,FALSE,FALSE,5,0,0);

  widget1=gck_hscale_new(NULL,vbox,&sample_scale_vals,(GtkSignalFunc)scale_update);
  widget2=gck_entryfield_new(NULL,vbox,mapvals.pixel_treshold,(GtkSignalFunc)entry_update);
  gtk_object_set_data(GTK_OBJECT(widget1),"ValuePtr",(gpointer)&mapvals.max_depth);
  gtk_object_set_data(GTK_OBJECT(widget2),"ValuePtr",(gpointer)&mapvals.pixel_treshold);

  gtk_tooltips_set_tip(tooltips,widget1,_("Antialiasing quality. Higher is better, but slower"),NULL);
  gtk_tooltips_set_tip(tooltips,widget2,_("Stop when pixel differences are smaller than this value"),NULL);

  gtk_widget_show(widget1);
  gtk_widget_show(widget2);
  gtk_widget_show(vbox);
  gtk_widget_show(hbox);

  gtk_widget_show(page);

  return(page);
}

/******************************/
/* Create light settings page */
/******************************/

GtkWidget *create_light_page(void)
{
  GtkWidget *page,*frame,*vbox;
  GtkWidget *widget1,*widget2,*widget3;
  int i;

  for (i = 0; light_labels[i] != NULL; i++)
    light_labels[i] = g_strdup(gettext(light_labels[i]));


  page=gtk_vbox_new(FALSE,0);
  
  frame=gck_frame_new(_("Light settings"),page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,5);
  vbox=gck_vbox_new(frame,FALSE,TRUE,TRUE,5,0,5);

  gck_auto_show(TRUE);
  widget1=gck_option_menu_new(_("Lightsource type:"),vbox,TRUE,TRUE,0,
    light_labels,(GtkSignalFunc)lightmenu_callback, NULL);
  gtk_option_menu_set_history(GTK_OPTION_MENU(widget1),mapvals.lightsource.type);
  gck_auto_show(FALSE);
  
  widget2=gck_pushbutton_new(_("Lightsource color"),vbox,TRUE,FALSE,0,
    (GtkSignalFunc)light_color_callback);

  gtk_widget_show(widget2);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  gtk_tooltips_set_tip(tooltips,widget1,_("Type of light source to apply"),NULL);
  gtk_tooltips_set_tip(tooltips,widget2,_("Set light source color (white is default)"),NULL);
  
  pointlightwid=gck_frame_new(_("Position"),page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,5);
  vbox=gck_vbox_new(pointlightwid,FALSE,FALSE,FALSE,5,0,5);

  xentry=gck_entryfield_new(_("X:"),vbox,mapvals.lightsource.position.x,(GtkSignalFunc)entry_update);
  yentry=gck_entryfield_new(_("Y:"),vbox,mapvals.lightsource.position.y,(GtkSignalFunc)entry_update);
  zentry=gck_entryfield_new(_("Z:"),vbox,mapvals.lightsource.position.z,(GtkSignalFunc)entry_update);

  gtk_object_set_data(GTK_OBJECT(xentry),"ValuePtr",(gpointer)&mapvals.lightsource.position.x);
  gtk_object_set_data(GTK_OBJECT(yentry),"ValuePtr",(gpointer)&mapvals.lightsource.position.y);
  gtk_object_set_data(GTK_OBJECT(zentry),"ValuePtr",(gpointer)&mapvals.lightsource.position.z);

  gtk_tooltips_set_tip(tooltips,xentry,_("Light source X position in XYZ space"),NULL);
  gtk_tooltips_set_tip(tooltips,yentry,_("Light source Y position in XYZ space"),NULL);
  gtk_tooltips_set_tip(tooltips,zentry,_("Light source Z position in XYZ space"),NULL);

  gtk_widget_show(xentry);
  gtk_widget_show(yentry);
  gtk_widget_show(zentry);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);
  gtk_widget_show(pointlightwid);

  dirlightwid=gck_frame_new(_("Direction vector"),page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,5);
  vbox=gck_vbox_new(dirlightwid,FALSE,FALSE,FALSE,5,0,5);

  widget1=gck_entryfield_new(_("X:"),vbox,mapvals.lightsource.direction.x,(GtkSignalFunc)entry_update);
  widget2=gck_entryfield_new(_("Y:"),vbox,mapvals.lightsource.direction.y,(GtkSignalFunc)entry_update);
  widget3=gck_entryfield_new(_("Z:"),vbox,mapvals.lightsource.direction.z,(GtkSignalFunc)entry_update);

  gtk_tooltips_set_tip(tooltips,widget1,_("Light source X direction in XYZ space"),NULL);
  gtk_tooltips_set_tip(tooltips,widget2,_("Light source Y direction in XYZ space"),NULL);
  gtk_tooltips_set_tip(tooltips,widget3,_("Light source Z direction in XYZ space"),NULL);

  gtk_object_set_data(GTK_OBJECT(widget1),"ValuePtr",(gpointer)&mapvals.lightsource.direction.x);
  gtk_object_set_data(GTK_OBJECT(widget2),"ValuePtr",(gpointer)&mapvals.lightsource.direction.y);
  gtk_object_set_data(GTK_OBJECT(widget3),"ValuePtr",(gpointer)&mapvals.lightsource.direction.z);

  gtk_widget_show(widget1);
  gtk_widget_show(widget2);
  gtk_widget_show(widget3);
  gtk_widget_show(vbox);

  gtk_widget_show(page);

  return page;  
}

/*********************************/
/* Create material settings page */
/*********************************/

GtkWidget *create_material_page(void)
{
  GtkWidget *page,*frame,*table;
  GtkWidget *label1,*label2,*label3;
  GtkWidget *widget1,*widget2,*widget3;
  GdkPixmap *image;
  GdkBitmap *mask;
  GtkStyle  *style;
  GtkWidget *pixmap;
  
  page=gtk_vbox_new(FALSE,0);

  frame=gck_frame_new(_("Intensity levels"),page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,5);

  table=gtk_table_new(2,4,FALSE);
  gtk_container_add(GTK_CONTAINER(frame),table);
  
  label1=gck_label_aligned_new(_("Ambient:"),NULL,GCK_ALIGN_RIGHT,GCK_ALIGN_CENTERED);
  label2=gck_label_aligned_new(_("Diffuse:"),NULL,GCK_ALIGN_RIGHT,GCK_ALIGN_CENTERED);

  gtk_table_attach(GTK_TABLE(table),label1,0,1,0,1, 0,0,0,0);
  gtk_table_attach(GTK_TABLE(table),label2,0,1,1,2, 0,0,0,0);

  widget1=gck_entryfield_new(NULL,NULL,mapvals.material.ambient_int,(GtkSignalFunc)entry_update);
  widget2=gck_entryfield_new(NULL,NULL,mapvals.material.diffuse_int,(GtkSignalFunc)entry_update);

  gtk_table_attach(GTK_TABLE(table),widget1,2,3,0,1, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_table_attach(GTK_TABLE(table),widget2,2,3,1,2, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);

  style=gtk_widget_get_style(table);

  image=gdk_pixmap_create_from_xpm_d(appwin->widget->window,&mask,&style->bg[GTK_STATE_NORMAL],amb1_xpm);
  pixmap=gtk_pixmap_new(image,mask);
  gtk_table_attach(GTK_TABLE(table),pixmap,1,2,0,1, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(pixmap);

  image=gdk_pixmap_create_from_xpm_d(appwin->widget->window,&mask,&style->bg[GTK_STATE_NORMAL],amb2_xpm);
  pixmap=gtk_pixmap_new(image,mask);
  gtk_table_attach(GTK_TABLE(table),pixmap,3,4,0,1, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(pixmap);

  image=gdk_pixmap_create_from_xpm_d(appwin->widget->window,&mask,&style->bg[GTK_STATE_NORMAL],diffint1_xpm);
  pixmap=gtk_pixmap_new(image,mask);
  gtk_table_attach(GTK_TABLE(table),pixmap,1,2,1,2, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(pixmap);

  image=gdk_pixmap_create_from_xpm_d(appwin->widget->window,&mask,&style->bg[GTK_STATE_NORMAL],diffint2_xpm);
  pixmap=gtk_pixmap_new(image,mask);
  gtk_table_attach(GTK_TABLE(table),pixmap,3,4,1,2, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(pixmap);

  gtk_widget_show(label1);
  gtk_widget_show(label2);
  gtk_widget_show(widget1);
  gtk_widget_show(widget2);
  gtk_widget_show(table);
  gtk_widget_show(frame);

  gtk_tooltips_set_tip(tooltips,widget1,_("Amount of original color to show where no direct light falls"),NULL);
  gtk_tooltips_set_tip(tooltips,widget2,_("Intensity of original color when lit by a light source"),NULL);

  gtk_object_set_data(GTK_OBJECT(widget1),"ValuePtr",(gpointer)&mapvals.material.ambient_int);
  gtk_object_set_data(GTK_OBJECT(widget2),"ValuePtr",(gpointer)&mapvals.material.diffuse_int);

  frame=gck_frame_new(_("Reflectivity"),page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,5);

  table=gtk_table_new(3,4,FALSE);
  gtk_container_add(GTK_CONTAINER(frame),table);
  
  label1=gck_label_aligned_new(_("Diffuse:"),NULL,GCK_ALIGN_RIGHT,GCK_ALIGN_CENTERED); 
  label2=gck_label_aligned_new(_("Specular:"),NULL,GCK_ALIGN_RIGHT,GCK_ALIGN_CENTERED);
  label3=gck_label_aligned_new(_("Hightlight:"),NULL,GCK_ALIGN_RIGHT,GCK_ALIGN_CENTERED);

  gtk_table_attach(GTK_TABLE(table),label1,0,1,0,1, 0,0,0,0);
  gtk_table_attach(GTK_TABLE(table),label2,0,1,1,2, 0,0,0,0);
  gtk_table_attach(GTK_TABLE(table),label3,0,1,2,3, 0,0,0,0);

  widget1=gck_entryfield_new(NULL,NULL,mapvals.material.diffuse_ref,(GtkSignalFunc)entry_update);
  widget2=gck_entryfield_new(NULL,NULL,mapvals.material.specular_ref,(GtkSignalFunc)entry_update);
  widget3=gck_entryfield_new(NULL,NULL,mapvals.material.highlight,(GtkSignalFunc)entry_update);

  gtk_table_attach(GTK_TABLE(table),widget1,2,3,0,1, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_table_attach(GTK_TABLE(table),widget2,2,3,1,2, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_table_attach(GTK_TABLE(table),widget3,2,3,2,3, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);

  gtk_tooltips_set_tip(tooltips,widget1,_("Higher values makes the object reflect more light (appear lighter)"),NULL);
  gtk_tooltips_set_tip(tooltips,widget2,_("Controls how intense the highlights will be"),NULL);
  gtk_tooltips_set_tip(tooltips,widget3,_("Higher values makes the highlights more focused"),NULL);

  gtk_object_set_data(GTK_OBJECT(widget1),"ValuePtr",(gpointer)&mapvals.material.diffuse_ref);
  gtk_object_set_data(GTK_OBJECT(widget2),"ValuePtr",(gpointer)&mapvals.material.specular_ref);
  gtk_object_set_data(GTK_OBJECT(widget3),"ValuePtr",(gpointer)&mapvals.material.highlight);

  style=gtk_widget_get_style(table);

  image=gdk_pixmap_create_from_xpm_d(appwin->widget->window,&mask,&style->bg[GTK_STATE_NORMAL],diffref1_xpm);
  pixmap=gtk_pixmap_new(image,mask);
  gtk_table_attach(GTK_TABLE(table),pixmap,1,2,0,1, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(pixmap);

  image=gdk_pixmap_create_from_xpm_d(appwin->widget->window,&mask,&style->bg[GTK_STATE_NORMAL],diffref2_xpm);
  pixmap=gtk_pixmap_new(image,mask);
  gtk_table_attach(GTK_TABLE(table),pixmap,3,4,0,1, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(pixmap);

  image=gdk_pixmap_create_from_xpm_d(appwin->widget->window,&mask,&style->bg[GTK_STATE_NORMAL],specref1_xpm);
  pixmap=gtk_pixmap_new(image,mask);
  gtk_table_attach(GTK_TABLE(table),pixmap,1,2,1,2, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(pixmap);

  image=gdk_pixmap_create_from_xpm_d(appwin->widget->window,&mask,&style->bg[GTK_STATE_NORMAL],specref2_xpm);
  pixmap=gtk_pixmap_new(image,mask);
  gtk_table_attach(GTK_TABLE(table),pixmap,3,4,1,2, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(pixmap);

  image=gdk_pixmap_create_from_xpm_d(appwin->widget->window,&mask,&style->bg[GTK_STATE_NORMAL],high1_xpm);
  pixmap=gtk_pixmap_new(image,mask);
  gtk_table_attach(GTK_TABLE(table),pixmap,1,2,2,3, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(pixmap);

  image=gdk_pixmap_create_from_xpm_d(appwin->widget->window,&mask,&style->bg[GTK_STATE_NORMAL],high2_xpm);
  pixmap=gtk_pixmap_new(image,mask);
  gtk_table_attach(GTK_TABLE(table),pixmap,3,4,2,3, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(pixmap);

  gtk_widget_show(label1);
  gtk_widget_show(label2);
  gtk_widget_show(label3);
  gtk_widget_show(widget1);
  gtk_widget_show(widget2);
  gtk_widget_show(widget3);
  gtk_widget_show(table);
  gtk_widget_show(frame);

  gtk_widget_show(page);
  
  return page;
}

GtkWidget *create_bump_page(void)
{
  GtkWidget *page,*widget1,*widget2,*imagewid;
  GtkWidget *frame,*vbox,*label;
  int i;

  /* Bump mapping page */
  /* ================= */

  for (i = 0; curvetype_labels[i] != NULL; i++)
    curvetype_labels[i] = g_strdup(gettext(curvetype_labels[i]));
  
  page=gtk_vbox_new(FALSE,0);

  frame=gck_frame_new(_("Bumpmap settings"),page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,5);
  vbox=gck_vbox_new(frame,FALSE,TRUE,TRUE,5,0,5);

  imagewid=gck_vbox_new(vbox,FALSE,TRUE,TRUE,5,0,5);

  widget1=gck_hbox_new(imagewid,FALSE,TRUE,TRUE,5,0,3);
  label = gck_label_new(_("Bumpmap image:"),widget1);

  gtk_widget_show(label);
  gtk_widget_show(widget1);

  widget2=gtk_option_menu_new();
  gtk_box_pack_end(GTK_BOX(widget1),widget2,TRUE,TRUE,0);
  gtk_widget_show(widget2);

  widget1 = gimp_drawable_menu_new (bumpmap_constrain, bumpmap_drawable_callback,
    NULL, mapvals.bumpmap_id);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(widget2), widget1);

  gck_auto_show(TRUE);
  widget1=gck_option_menu_new(_("Curve:"),imagewid,TRUE,TRUE,0,curvetype_labels,
    (GtkSignalFunc)mapmenu2_callback,NULL);
  gtk_option_menu_set_history(GTK_OPTION_MENU(widget1),mapvals.bumpmaptype);
  gck_auto_show(FALSE);

  widget1=gck_entryfield_new(_("Minimum height:"),imagewid,mapvals.bumpmin,
    (GtkSignalFunc)entry_update);
  widget2=gck_entryfield_new(_("Maximum height:"),imagewid,mapvals.bumpmax,
    (GtkSignalFunc)entry_update);

  gtk_object_set_data(GTK_OBJECT(widget1),"ValuePtr",(gpointer)&mapvals.bumpmin);
  gtk_object_set_data(GTK_OBJECT(widget2),"ValuePtr",(gpointer)&mapvals.bumpmax);
  gtk_widget_show(widget1);
  gtk_widget_show(widget2);

  widget1=gck_checkbutton_new(_("Autostretch to fit value range"),imagewid,mapvals.bumpstretch,
    (GtkSignalFunc)togglestretch_update);
  gtk_object_set_data(GTK_OBJECT(widget1),"ValuePtr",(gpointer)&mapvals.bumpstretch);
  gtk_widget_show(widget1);

  gtk_widget_show(imagewid);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);
  gtk_widget_show(page);

  return(page);
}

GtkWidget *create_environment_page(void)
{
  GtkWidget *page,*frame,*vbox,*imagewid;
  GtkWidget *widget1,*widget2;

  /* Environment mapping page */
  /* ======================== */
  
  page=gck_vbox_new(NULL,FALSE,FALSE,FALSE,0,0,0);

  frame=gck_frame_new(_("Environment settings"),page,
    GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,5);
  vbox=gck_vbox_new(frame,FALSE,TRUE,TRUE,5,0,5);

  imagewid=gck_vbox_new(vbox,FALSE,TRUE,TRUE,5,0,5);

  widget1=gck_hbox_new(imagewid,FALSE,TRUE,TRUE,5,0,3);
  gck_label_new(_("Environment image:"),widget1);

  widget2=gtk_option_menu_new();
  gtk_box_pack_end(GTK_BOX(widget1),widget2,TRUE,TRUE,0);

  gtk_widget_show(widget1);

  widget1 = gimp_drawable_menu_new (envmap_constrain, envmap_drawable_callback,
    NULL, mapvals.envmap_id);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(widget2), widget1);

  gtk_widget_show(widget2);
  gtk_widget_show(imagewid);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);
  gtk_widget_show(page);

  return(page);  
}

/****************************/
/* Create notbook and pages */
/****************************/

void create_main_notebook(GtkWidget *container)
{
  GtkWidget *page,*label;

  gck_auto_show(FALSE);

  options_note_book=GTK_NOTEBOOK(gtk_notebook_new());
  gtk_container_add(GTK_CONTAINER(container),GTK_WIDGET(options_note_book));

  page = create_options_page();
  label=gtk_label_new(_("Options"));
  gtk_widget_show(label);

  gtk_notebook_append_page(options_note_book,page,label);
  
  page = create_light_page();
  label=gtk_label_new(_("Light"));
  gtk_widget_show(label);

  gtk_notebook_append_page(options_note_book,page,label);
  
  page = create_material_page();
  label=gtk_label_new(_("Material"));
  gtk_widget_show(label);

  gtk_notebook_append_page(options_note_book,page,label);

  if (mapvals.bump_mapped==TRUE)
    {
      bump_page = create_bump_page();
      bump_label=gtk_label_new(_("Bump"));
      gtk_widget_show(bump_label);
      bump_page_pos = g_list_length(options_note_book->children);
      gtk_notebook_append_page(options_note_book,bump_page,bump_label);      
    }

  if (mapvals.env_mapped==TRUE)
    {
      env_page = create_environment_page();
      env_label=gtk_label_new(_("Env"));
      gtk_widget_show(env_label);
      env_page_pos = g_list_length(options_note_book->children);
      gtk_notebook_append_page(options_note_book,env_page,env_label);
    }
  
  gtk_widget_show(GTK_WIDGET(options_note_book));

  gck_auto_show(TRUE);
}

/*****************************************************/
/* Create and show main dialog. Uses the plugin_ui.c */
/* routines when possible, gtk itself when not.      */
/*****************************************************/

void create_main_dialog(void)
{
  GtkWidget *main_vbox,*main_workbox,*actionbox,*workbox1,*workbox1b,*workbox2;
  GtkWidget *frame,*applybutton,*cancelbutton,*helpbutton,*hbox,*wid;

  appwin = gck_application_window_new(_("Lighting effects"));
  gtk_widget_realize(appwin->widget);

  tooltips=gtk_tooltips_new();

  /* Main manager widget */
  /* =================== */

  main_vbox=gck_vbox_new(appwin->widget,FALSE,FALSE,FALSE,8,0,0);

  /* Work area manager widget */
  /* ======================== */

  main_workbox=gck_hbox_new(main_vbox,FALSE,FALSE,FALSE,5,0,5);

  /* Action area manager widget */
  /* ========================== */  

  gck_hseparator_new(main_vbox);
  actionbox=gck_hbox_new(main_vbox,FALSE,FALSE,FALSE,5,0,5);

  /* Add Ok, Cancel and Help buttons to the action area */
  /* ================================================== */

  applybutton=gck_pushbutton_new(_("Apply"),actionbox,FALSE,FALSE,5,(GtkSignalFunc)apply_callback);
  cancelbutton=gck_pushbutton_new(_("Cancel"),actionbox,FALSE,FALSE,5,(GtkSignalFunc)exit_callback);
  helpbutton=gck_pushbutton_new(_("Help"),actionbox,FALSE,FALSE,5,NULL);

  GTK_WIDGET_SET_FLAGS (applybutton, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (cancelbutton, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (helpbutton, GTK_CAN_DEFAULT);

  gtk_widget_grab_default (applybutton);
  gtk_widget_set_sensitive(helpbutton,FALSE);
  
  gtk_tooltips_set_tip(tooltips,applybutton,_("Apply filter with current settings"),NULL);
  gtk_tooltips_set_tip(tooltips,cancelbutton,_("Close filter without doing anything"),NULL);

  /* Split the workarea in two */
  /* ========================= */

  frame=gck_frame_new(NULL,main_workbox,GTK_SHADOW_ETCHED_IN,TRUE,TRUE,0,0);
  workbox1=gck_vbox_new(frame,FALSE,TRUE,TRUE,5,0,5);
  workbox2=gck_vbox_new(main_workbox,FALSE,FALSE,FALSE,0,0,0);

  /* Add preview widget and various buttons to the first part */
  /* ======================================================== */

  frame=gck_frame_new(NULL,workbox1,GTK_SHADOW_IN,FALSE,FALSE,0,0);
  previewarea = gck_drawing_area_new(frame, PREVIEW_WIDTH, PREVIEW_HEIGHT,
    GDK_EXPOSURE_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_PRESS_MASK | 
    GDK_BUTTON_RELEASE_MASK, (GtkSignalFunc)preview_events);

  workbox1b=gck_vbox_new(workbox1,FALSE,FALSE,FALSE,0,0,0);

  hbox=gck_hbox_new(workbox1b,FALSE,FALSE,FALSE,5,0,0);
  wid=gck_pushbutton_new(_("  Preview!  "),hbox,FALSE,FALSE,0,(GtkSignalFunc)preview_callback);
  gtk_tooltips_set_tip(tooltips,wid,_("Recompute preview image"),NULL);
  hbox=gck_hbox_new(hbox,TRUE,FALSE,FALSE,0,0,0);
  wid=gck_pushbutton_new(" + ",hbox,FALSE,FALSE,0,(GtkSignalFunc)zoomin_callback);
  gtk_tooltips_set_tip(tooltips,wid,_("Zoom in (make image bigger)"),NULL);
  wid=gck_pushbutton_new(" - ",hbox,FALSE,FALSE,0,(GtkSignalFunc)zoomout_callback);
  gtk_tooltips_set_tip(tooltips,wid,_("Zoom out (make image smaller)"),NULL);

  create_main_notebook(workbox2);

  /* Now lets check out the result of this mess */
  /*=========================================== */

  gtk_widget_show(appwin->widget);

  gck_cursor_set(previewarea->window,GDK_HAND2);
  gtk_tooltips_set_colors(tooltips,
    gck_rgb_to_gdkcolor(appwin->visinfo,255,255,220),
    gck_rgb_to_gdkcolor(appwin->visinfo,0,0,0));
 
  if (mapvals.tooltips_enabled==FALSE)
    gtk_tooltips_disable(tooltips);
}
