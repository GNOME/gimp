/**************************************************************/
/* Dialog creation and updaters, callbacks and event-handlers */
/**************************************************************/

#include "mapobject_ui.h"
#include "mapobject_pixmaps.h"

extern MapObjectValues mapvals;

GckApplicationWindow *appwin            = NULL;
GtkWidget            *color_select_diag = NULL;
GtkNotebook          *options_note_book = NULL;
GtkTooltips          *tooltips          = NULL;

GdkGC *gc = NULL;
GtkWidget *previewarea,*pointlightwid,*dirlightwid;
GtkWidget *xentry,*yentry,*zentry, *scale_table;
GtkWidget *images_page = NULL;

GckRGB old_light_color;

gint color_dialog_id = -1;

guint left_button_pressed = FALSE, light_hit = FALSE;
guint32 blackpixel,whitepixel;

GckScaleValues angle_scale_vals =  { 180, 0.0, -180.0, 180.0, 0.1, 1.0, 1.0, GTK_UPDATE_CONTINUOUS,TRUE };
GckScaleValues scale_scale_vals =  { 180, 0.5,   0.01,   5.0, 0.1, 0.1, 0.1,     GTK_UPDATE_CONTINUOUS,TRUE };
GckScaleValues sample_scale_vals = { 128, 3.0,    1.0,   6.0, 1.0, 1.0, 1.0, GTK_UPDATE_CONTINUOUS,TRUE };

gchar *light_labels[] =
  {
    "Point light",
    "Directional light",
    "No light",
     NULL
  };

gchar *map_labels[] =
  {
    "Plane",
    "Sphere",
    "Box",
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

gint preview_events (GtkWidget *area, GdkEvent *event);

void update_slider            (void);
void update_angle_sliders     (void);
void update_light_pos_entries (void);

void xyzval_update      (GtkWidget *widget, GtkEntry *entry);
void entry_update       (GtkWidget *widget, GtkEntry *entry);
void angle_update       (GtkWidget *widget, GtkScale *scale);
void scale_update       (GtkWidget *widget, GtkScale *scale);
void toggle_update      (GtkWidget *widget, GtkCheckButton *button);
void togglegrid_update  (GtkWidget *widget, GtkCheckButton *button);
void toggletile_update  (GtkWidget *widget, GtkCheckButton *button);
void toggleanti_update  (GtkWidget *widget, GtkCheckButton *button);
void toggletips_update  (GtkWidget *widget, GtkCheckButton *button);
void toggletrans_update (GtkWidget *widget, GtkCheckButton *button);

void lightmenu_callback    (GtkWidget *widget, gpointer client_data);
void preview_callback      (GtkWidget *widget, gpointer client_data);
void apply_callback        (GtkWidget *widget, gpointer client_data);
void exit_callback         (GtkWidget *widget, gpointer client_data);
void color_ok_callback     (GtkWidget *widget, gpointer client_data);
void color_cancel_callback (GtkWidget *widget, gpointer client_data);
void light_color_callback  (GtkWidget *widget, gpointer client_data);
gint color_delete_callback (GtkWidget *widget, GdkEvent *event, gpointer client_data);
void color_changed_callback (GtkColorSelection *colorsel, gpointer client_data);

gint box_constrain         (gint32 image_id, gint32 drawable_id, gpointer data);
void box_drawable_callback (gint32 id, gpointer data);

GtkWidget *create_options_page     (void);
GtkWidget *create_light_page       (void);
GtkWidget *create_material_page    (void);
GtkWidget *create_orientation_page (void);
GtkWidget *create_images_page      (void);

/******************/
/* Implementation */
/******************/

/**********************************************************/
/* Update entry fields that affect the preview parameters */
/**********************************************************/

void xyzval_update(GtkWidget *widget, GtkEntry *entry)
{
  gdouble *valueptr;
  gdouble value;

  valueptr=(gdouble *)gtk_object_get_data(GTK_OBJECT(widget),"ValuePtr");
  value = atof(gtk_entry_get_text(entry));

  *valueptr=value;

  if (mapvals.showgrid==TRUE)
    draw_preview_wireframe();
}

/*********************/
/* Std. entry update */
/*********************/

void entry_update(GtkWidget *widget, GtkEntry *entry)
{
  gdouble *valueptr;
  gdouble value;

  valueptr=(gdouble *)gtk_object_get_data(GTK_OBJECT(widget),"ValuePtr");
  value = atof(gtk_entry_get_text(entry));

  *valueptr=value;
}

/***************************************************/
/* Update angle sliders (redraw grid if necessary) */
/***************************************************/

void angle_update(GtkWidget *widget, GtkScale *scale)
{
  gdouble *valueptr;
  GtkAdjustment *adjustment;

  valueptr=(gdouble *)gtk_object_get_data(GTK_OBJECT(widget),"ValuePtr");
  adjustment=gtk_range_get_adjustment(GTK_RANGE(scale));
  
  *valueptr=(gdouble)adjustment->value;

  if (mapvals.showgrid==TRUE)
    draw_preview_wireframe();
}

/***************************************************/
/* Update scale sliders (redraw grid if necessary) */
/***************************************************/

void boxscale_update(GtkWidget *widget, GtkScale *scale)
{
  gdouble *valueptr;
  GtkAdjustment *adjustment;

  valueptr=(gdouble *)gtk_object_get_data(GTK_OBJECT(widget),"ValuePtr");
  adjustment=gtk_range_get_adjustment(GTK_RANGE(scale));
  
  *valueptr=(gdouble)adjustment->value;

  if (mapvals.showgrid==TRUE)
    draw_preview_wireframe();
}

void update_light_pos_entries(void)
{
  gchar entrytext[64];
  
  sprintf(entrytext,"%f",mapvals.lightsource.position.x);
  gtk_entry_set_text(GTK_ENTRY(xentry),entrytext);
  sprintf(entrytext,"%f",mapvals.lightsource.position.y);
  gtk_entry_set_text(GTK_ENTRY(yentry),entrytext);
  sprintf(entrytext,"%f",mapvals.lightsource.position.z);
  gtk_entry_set_text(GTK_ENTRY(zentry),entrytext);
}

void update_slider(void)
{
}

void update_angle_sliders(void)
{
}

/*********************/
/* Std. scale update */
/*********************/

void scale_update(GtkWidget *widget,GtkScale *scale)
{
  gdouble *valueptr;
  GtkAdjustment *adjustment;

  valueptr=(gdouble *)gtk_object_get_data(GTK_OBJECT(widget),"ValuePtr");
  adjustment=gtk_range_get_adjustment(GTK_RANGE(scale));

  *valueptr=(gdouble)adjustment->value;
}

/**********************/
/* Std. toggle update */
/**********************/

void toggle_update(GtkWidget *widget, GtkCheckButton *button)
{
  gint *value;

  value=(gint *)gtk_object_get_data(GTK_OBJECT(button),"ValuePtr");
  *value=!(*value);
}

/***************************/
/* Show grid toggle update */
/***************************/

void togglegrid_update(GtkWidget *widget, GtkCheckButton *button)
{
  gint *value;

  value=(gint *)gtk_object_get_data(GTK_OBJECT(button),"ValuePtr");
  *value=!(*value);

  if (mapvals.showgrid==TRUE && linetab[0].x1==-1)
    draw_preview_wireframe();
  else if (mapvals.showgrid==FALSE && linetab[0].x1!=-1)
    {
      gck_gc_set_foreground(appwin->visinfo,gc,255,255,255);
      gck_gc_set_background(appwin->visinfo,gc,0,0,0);
  
      gdk_gc_set_function(gc,GDK_INVERT);
  
      clear_wireframe();
      linetab[0].x1=-1;
    }
}

/****************************/
/* Tile image toggle update */
/****************************/

void toggletile_update(GtkWidget *widget, GtkCheckButton *button)
{
  gint *value;

  value=(gint *)gtk_object_get_data(GTK_OBJECT(button),"ValuePtr");
  *value=!(*value);

  draw_preview_image(TRUE);
  linetab[0].x1=-1;
}

/******************************/
/* Antialiasing toggle update */
/******************************/

void toggleanti_update(GtkWidget *widget, GtkCheckButton *button)
{
  gint *value;

  value=(gint *)gtk_object_get_data(GTK_OBJECT(button),"ValuePtr");
  *value=!(*value);
}

/**************************/
/* Tooltips toggle update */
/**************************/

void toggletips_update(GtkWidget *widget, GtkCheckButton *button)
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

void toggletrans_update(GtkWidget *widget, GtkCheckButton *button)
{
  gint *value;

  value=(gint *)gtk_object_get_data(GTK_OBJECT(button),"ValuePtr");
  *value=!(*value);

  draw_preview_image(TRUE);
  linetab[0].x1=-1;
}

/*****************************************/
/* Main window light type menu callback. */
/*****************************************/

void lightmenu_callback(GtkWidget *widget, gpointer client_data)
{
  mapvals.lightsource.type=(gint)gtk_object_get_data(GTK_OBJECT(widget),"_GckOptionMenuItemID");

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
      gtk_widget_hide(dirlightwid);
    }
}

/***************************************/
/* Main window map type menu callback. */
/***************************************/

void mapmenu_callback(GtkWidget *widget, gpointer client_data)
{
  GtkWidget *label;

  mapvals.maptype=(MapType)gtk_object_get_data(GTK_OBJECT(widget),"_GckOptionMenuItemID");

  draw_preview_image(TRUE);

  if (mapvals.showgrid==TRUE && linetab[0].x1==-1)
    draw_preview_wireframe();
  else if (mapvals.showgrid==FALSE && linetab[0].x1!=-1)
    {
      gck_gc_set_foreground(appwin->visinfo,gc,255,255,255);
      gck_gc_set_background(appwin->visinfo,gc,0,0,0);
  
      gdk_gc_set_function(gc,GDK_INVERT);
  
      clear_wireframe();
      linetab[0].x1=-1;
    }
  
  if (mapvals.maptype==MAP_BOX)
    {
      gtk_widget_show(scale_table);
      
      if (images_page==NULL)
        {
          images_page = create_images_page();
          label=gtk_label_new("Face images");
          gtk_widget_show(label);
    
          gtk_notebook_append_page(options_note_book,images_page,label);
        }
    }
  else
    {
      if (images_page!=NULL)
        {
          gtk_notebook_remove_page(options_note_book, 
            g_list_length(options_note_book->children)-1);
          images_page = NULL;
        }

      gtk_widget_hide(scale_table);
    }
}

/******************************************/
/* Main window "Preview!" button callback */
/******************************************/

void preview_callback(GtkWidget *widget, gpointer client_data)
{
  draw_preview_image(TRUE);
  linetab[0].x1=-1;
}

/*********************************************/
/* Main window "-" (zoom in) button callback */
/*********************************************/

void zoomout_callback(GtkWidget *widget, gpointer client_data)
{
  if (mapvals.preview_zoom_factor<2)
    {
      mapvals.preview_zoom_factor++;
      if (linetab[0].x1!=-1)
        clear_wireframe();
      draw_preview_image(TRUE);
    }
}

/*********************************************/
/* Main window "+" (zoom out) button callback */
/*********************************************/

void zoomin_callback(GtkWidget *widget, gpointer client_data)
{
  if (mapvals.preview_zoom_factor>0)
    {
      mapvals.preview_zoom_factor--;
      if (linetab[0].x1!=-1)
        clear_wireframe();
      draw_preview_image(TRUE);
    }
}

/**********************************************/
/* Main window "Apply" button callback.       */ 
/* Render to GIMP image, close down and exit. */
/**********************************************/

void apply_callback(GtkWidget *widget, gpointer client_data)
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

void exit_callback(GtkWidget *widget, gpointer client_data)
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

/********************************************/
/* Color dialog "Cancel" button callback.   */
/* Close dialog & restore old color values. */
/********************************************/

void color_changed_callback  (GtkColorSelection *colorsel, gpointer client_data)
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

gint box_constrain(gint32 image_id, gint32 drawable_id, gpointer data)
{
  if (drawable_id == -1)
    return(TRUE);

  return (gimp_drawable_color(drawable_id) && !gimp_drawable_indexed(drawable_id));
}

void box_drawable_callback(gint32 id, gpointer data)
{
  gint i;
  
  i = (gint)gtk_object_get_data(GTK_OBJECT(data),"_mapwid_id");

  mapvals.boxmap_id[i] = id;
}

/******************************/
/* Preview area event handler */
/******************************/

gint preview_events(GtkWidget *area, GdkEvent  *event)
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
            gc=gdk_gc_new(area->window);
            draw_preview_image(TRUE);
          }
        else
          {
            draw_preview_image(FALSE);
            if (mapvals.showgrid==1 && linetab[0].x1!=-1)
              draw_preview_wireframe();
          }
        break; 
      case GDK_ENTER_NOTIFY:
        break;
      case GDK_LEAVE_NOTIFY:
        break;
      case GDK_BUTTON_PRESS:
        light_hit=check_light_hit(event->button.x,event->button.y);
        if (light_hit==FALSE)
          {
            pos.x=-(2.0*(gdouble)event->button.x/(gdouble)PREVIEW_WIDTH-1.0);
            pos.y=2.0*(gdouble)event->button.y/(gdouble)PREVIEW_HEIGHT-1.0;
            /*ArcBall_Mouse(pos);
            ArcBall_BeginDrag(); */
          }
        left_button_pressed=TRUE;
        break;
      case GDK_BUTTON_RELEASE:
        if (light_hit==TRUE)
          draw_preview_image(TRUE);
        else
          {
            pos.x=-(2.0*(gdouble)event->button.x/(gdouble)PREVIEW_WIDTH-1.0);
            pos.y=2.0*(gdouble)event->button.y/(gdouble)PREVIEW_HEIGHT-1.0;
            /*ArcBall_Mouse(pos);
            ArcBall_EndDrag(); */
          }
        left_button_pressed=FALSE;
        break;
      case GDK_MOTION_NOTIFY:
        if (left_button_pressed==TRUE)
          {
            if (light_hit==TRUE)
              {
                update_light(event->motion.x,event->motion.y);
                update_light_pos_entries();
              }
            else
              {
            	pos.x=-(2.0*(gdouble)event->motion.x/(gdouble)PREVIEW_WIDTH-1.0);
                pos.y=2.0*(gdouble)event->motion.y/(gdouble)PREVIEW_HEIGHT-1.0;
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
  return(FALSE);
}

/*******************************/
/* Create general options page */
/*******************************/

GtkWidget *create_options_page(void)
{
  GtkWidget *page,*frame,*vbox,*hbox,*label;
  GtkWidget *toggletile,*toggleanti;
  GtkWidget *toggletrans,*toggleimage,*toggletips;
  GtkWidget *widget1,*widget2;

  page=gck_vbox_new(NULL,FALSE,FALSE,FALSE,0,0,0);

  frame=gck_frame_new("General options",page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,2);
  vbox=gck_vbox_new(frame,FALSE,FALSE,FALSE,0,2,2);
  
  gck_auto_show(TRUE);
  widget1=gck_option_menu_new("Map to:",vbox,TRUE,TRUE,0,map_labels,
    (GtkSignalFunc)mapmenu_callback, NULL);
  gck_auto_show(FALSE);
  gtk_widget_show(vbox);

  gtk_option_menu_set_history(GTK_OPTION_MENU(widget1),mapvals.maptype);
  gtk_tooltips_set_tip(tooltips,widget1,"Type of object to map to",NULL);

  vbox=gck_vbox_new(vbox,FALSE,FALSE,FALSE,0,0,0);
  toggletrans=gck_checkbutton_new("Transparent background",vbox,mapvals.transparent_background,
    (GtkSignalFunc)toggletrans_update);
  toggletile=gck_checkbutton_new("Tile source image",vbox,mapvals.tiled,
    (GtkSignalFunc)toggletile_update);
  toggleimage=gck_checkbutton_new("Create new image",vbox,mapvals.create_new_image,
    (GtkSignalFunc)toggle_update);
  toggletips=gck_checkbutton_new("Enable tooltips",vbox,mapvals.tooltips_enabled,
    (GtkSignalFunc)toggletips_update);

  gtk_tooltips_set_tip(tooltips,toggletrans,"Make image transparent outside object",NULL);
  gtk_tooltips_set_tip(tooltips,toggletile,"Tile source image: useful for infinite planes",NULL);
  gtk_tooltips_set_tip(tooltips,toggleimage,"Create a new image when applying filter",NULL);
  gtk_tooltips_set_tip(tooltips,toggletips,"Enable/disable tooltip messages",NULL); 

  gtk_object_set_data(GTK_OBJECT(toggletrans),"ValuePtr",(gpointer)&mapvals.transparent_background);
  gtk_object_set_data(GTK_OBJECT(toggletile),"ValuePtr",(gpointer)&mapvals.tiled);
  gtk_object_set_data(GTK_OBJECT(toggleimage),"ValuePtr",(gpointer)&mapvals.create_new_image);
  gtk_object_set_data(GTK_OBJECT(toggletips),"ValuePtr", (gpointer)&mapvals.tooltips_enabled);

  gtk_widget_show(toggletrans);
  gtk_widget_show(toggletile);
  gtk_widget_show(toggleimage);
  gtk_widget_show(toggletips);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  frame=gck_frame_new("Antialiasing options",page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,2);
  vbox=gck_vbox_new(frame,FALSE,FALSE,FALSE,0,2,2);

  toggleanti=gck_checkbutton_new("Enable antialiasing",vbox,mapvals.antialiasing,
    (GtkSignalFunc)toggleanti_update);
  gtk_object_set_data(GTK_OBJECT(toggleanti),"ValuePtr",(gpointer)&mapvals.antialiasing);
  gtk_tooltips_set_tip(tooltips,toggleanti,"Enable/disable jagged edges removal (antialiasing)",NULL);
   
  hbox=gck_hbox_new(vbox,FALSE,TRUE,TRUE,0,0,0);

  gtk_widget_show(toggleanti);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  vbox=gck_vbox_new(hbox,TRUE,FALSE,TRUE,0,0,0);

  frame=gck_frame_new(NULL,vbox,GTK_SHADOW_NONE,TRUE,TRUE,0,0);
  label=gck_label_aligned_new("Depth:",frame,GCK_ALIGN_RIGHT,GCK_ALIGN_BOTTOM);

  gtk_widget_show(label);
  gtk_widget_show(frame);

  frame=gck_frame_new(NULL,vbox,GTK_SHADOW_NONE,TRUE,TRUE,0,0);
  label=gck_label_aligned_new("Treshold:",frame,GCK_ALIGN_RIGHT,GCK_ALIGN_CENTERED);

  gtk_widget_show(label);
  gtk_widget_show(frame);
  gtk_widget_show(vbox);

  vbox=gck_vbox_new(hbox,TRUE,FALSE,FALSE,5,0,0);

  widget1=gck_hscale_new(NULL,vbox,&sample_scale_vals,(GtkSignalFunc)scale_update);
  widget2=gck_entryfield_new(NULL,vbox,mapvals.pixeltreshold,(GtkSignalFunc)entry_update);
  gtk_object_set_data(GTK_OBJECT(widget1),"ValuePtr",(gpointer)&mapvals.maxdepth);
  gtk_object_set_data(GTK_OBJECT(widget2),"ValuePtr",(gpointer)&mapvals.pixeltreshold);

  gtk_tooltips_set_tip(tooltips,widget1,"Antialiasing quality. Higher is better, but slower",NULL);
  gtk_tooltips_set_tip(tooltips,widget2,"Stop when pixel differences are smaller than this value",NULL);

  gtk_widget_show(widget1);
  gtk_widget_show(widget2);
  gtk_widget_show(vbox);
  gtk_widget_show(hbox);

  gtk_widget_show(page);

  return page;
}

/******************************/
/* Create light settings page */
/******************************/

GtkWidget *create_light_page(void)
{
  GtkWidget *page,*frame,*vbox;
  GtkWidget *widget1,*widget2,*widget3;

  page=gtk_vbox_new(FALSE,0);
  
  frame=gck_frame_new("Light settings",page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,5);
  vbox=gck_vbox_new(frame,FALSE,TRUE,TRUE,5,0,5);

  gck_auto_show(TRUE);
  widget1=gck_option_menu_new("Lightsource type:",vbox,TRUE,TRUE,0,
    light_labels,(GtkSignalFunc)lightmenu_callback, NULL);
  gtk_option_menu_set_history(GTK_OPTION_MENU(widget1),mapvals.lightsource.type);
  gck_auto_show(FALSE);
  
  widget2=gck_pushbutton_new("Lightsource color",vbox,TRUE,FALSE,0,
    (GtkSignalFunc)light_color_callback);

  gtk_widget_show(widget2);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  gtk_tooltips_set_tip(tooltips,widget1,"Type of light source to apply",NULL);
  gtk_tooltips_set_tip(tooltips,widget2,"Set light source color (white is default)",NULL);
  
  pointlightwid=gck_frame_new("Position",page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,5);
  vbox=gck_vbox_new(pointlightwid,FALSE,FALSE,FALSE,5,0,5);

  xentry=gck_entryfield_new("X:",vbox,mapvals.lightsource.position.x,(GtkSignalFunc)entry_update);
  yentry=gck_entryfield_new("Y:",vbox,mapvals.lightsource.position.y,(GtkSignalFunc)entry_update);
  zentry=gck_entryfield_new("Z:",vbox,mapvals.lightsource.position.z,(GtkSignalFunc)entry_update);

  gtk_object_set_data(GTK_OBJECT(xentry),"ValuePtr",(gpointer)&mapvals.lightsource.position.x);
  gtk_object_set_data(GTK_OBJECT(yentry),"ValuePtr",(gpointer)&mapvals.lightsource.position.y);
  gtk_object_set_data(GTK_OBJECT(zentry),"ValuePtr",(gpointer)&mapvals.lightsource.position.z);

  gtk_tooltips_set_tip(tooltips,xentry,"Light source X position in XYZ space",NULL);
  gtk_tooltips_set_tip(tooltips,yentry,"Light source Y position in XYZ space",NULL);
  gtk_tooltips_set_tip(tooltips,zentry,"Light source Z position in XYZ space",NULL);

  gtk_widget_show(xentry);
  gtk_widget_show(yentry);
  gtk_widget_show(zentry);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);
  gtk_widget_show(pointlightwid);

  dirlightwid=gck_frame_new("Direction vector",page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,5);
  vbox=gck_vbox_new(dirlightwid,FALSE,FALSE,FALSE,5,0,5);

  widget1=gck_entryfield_new("X:",vbox,mapvals.lightsource.direction.x,(GtkSignalFunc)entry_update);
  widget2=gck_entryfield_new("Y:",vbox,mapvals.lightsource.direction.y,(GtkSignalFunc)entry_update);
  widget3=gck_entryfield_new("Z:",vbox,mapvals.lightsource.direction.z,(GtkSignalFunc)entry_update);

  gtk_tooltips_set_tip(tooltips,widget1,"Light source X direction in XYZ space",NULL);
  gtk_tooltips_set_tip(tooltips,widget2,"Light source Y direction in XYZ space",NULL);
  gtk_tooltips_set_tip(tooltips,widget3,"Light source Z direction in XYZ space",NULL);

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
  
  page=gck_vbox_new(NULL,FALSE,FALSE,FALSE,0,0,0);

  frame=gck_frame_new("Intensity levels",page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,5);

  table=gtk_table_new(2,4,FALSE);
  gtk_container_add(GTK_CONTAINER(frame),table);
  
  label1=gck_label_aligned_new("Ambient:",NULL,GCK_ALIGN_RIGHT,GCK_ALIGN_CENTERED);
  label2=gck_label_aligned_new("Diffuse:",NULL,GCK_ALIGN_RIGHT,GCK_ALIGN_CENTERED);

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

  gtk_tooltips_set_tip(tooltips,widget1,"Amount of original color to show where no direct light falls",NULL);
  gtk_tooltips_set_tip(tooltips,widget2,"Intensity of original color when lit by a light source",NULL);

  gtk_object_set_data(GTK_OBJECT(widget1),"ValuePtr",(gpointer)&mapvals.material.ambient_int);
  gtk_object_set_data(GTK_OBJECT(widget2),"ValuePtr",(gpointer)&mapvals.material.diffuse_int);

  frame=gck_frame_new("Reflectivity",page,GTK_SHADOW_ETCHED_IN,FALSE,FALSE,0,5);

  table=gtk_table_new(3,4,FALSE);
  gtk_container_add(GTK_CONTAINER(frame),table);
  
  label1=gck_label_aligned_new("Diffuse:",NULL,GCK_ALIGN_RIGHT,GCK_ALIGN_CENTERED); 
  label2=gck_label_aligned_new("Specular:",NULL,GCK_ALIGN_RIGHT,GCK_ALIGN_CENTERED);
  label3=gck_label_aligned_new("Hightlight:",NULL,GCK_ALIGN_RIGHT,GCK_ALIGN_CENTERED);

  gtk_table_attach(GTK_TABLE(table),label1,0,1,0,1, 0,0,0,0);
  gtk_table_attach(GTK_TABLE(table),label2,0,1,1,2, 0,0,0,0);
  gtk_table_attach(GTK_TABLE(table),label3,0,1,2,3, 0,0,0,0);

  widget1=gck_entryfield_new(NULL,NULL,mapvals.material.diffuse_ref,(GtkSignalFunc)entry_update);
  widget2=gck_entryfield_new(NULL,NULL,mapvals.material.specular_ref,(GtkSignalFunc)entry_update);
  widget3=gck_entryfield_new(NULL,NULL,mapvals.material.highlight,(GtkSignalFunc)entry_update);

  gtk_table_attach(GTK_TABLE(table),widget1,2,3,0,1, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_table_attach(GTK_TABLE(table),widget2,2,3,1,2, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_table_attach(GTK_TABLE(table),widget3,2,3,2,3, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);

  gtk_tooltips_set_tip(tooltips,widget1,"Higher values makes the object reflect more light (appear lighter)",NULL);
  gtk_tooltips_set_tip(tooltips,widget2,"Controls how intense the highlights will be",NULL);
  gtk_tooltips_set_tip(tooltips,widget3,"Higher values makes the highlights more focused",NULL);

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

/****************************************/
/* Create orientation and position page */
/****************************************/

GtkWidget *create_orientation_page(void)
{
  GtkWidget *page,*frame,*vbox,*label,*table;
  GtkWidget *widget1,*widget2,*widget3;
  
  page=gck_vbox_new(NULL,FALSE,FALSE,FALSE,0,0,0);

  frame=gck_frame_new("Position and orientation",page,GTK_SHADOW_ETCHED_IN,TRUE,TRUE,0,5);
  vbox=gck_vbox_new(frame,FALSE,FALSE,FALSE,0,0,5);

  widget1=gck_entryfield_new("X pos.:",vbox,mapvals.position.x,(GtkSignalFunc)xyzval_update);
  widget2=gck_entryfield_new("Y pos.:",vbox,mapvals.position.y,(GtkSignalFunc)xyzval_update);
  widget3=gck_entryfield_new("Z pos.:",vbox,mapvals.position.z,(GtkSignalFunc)xyzval_update);

  gtk_tooltips_set_tip(tooltips,widget1,"Object X position in XYZ space (0.5 is center)",NULL);
  gtk_tooltips_set_tip(tooltips,widget2,"Object Y position in XYZ space (0.5 is center)",NULL);
  gtk_tooltips_set_tip(tooltips,widget3,"Object Z position in XYZ space (0.5 is center)",NULL);

  gtk_object_set_data(GTK_OBJECT(widget1),"ValuePtr",(gpointer)&mapvals.position.x);
  gtk_object_set_data(GTK_OBJECT(widget2),"ValuePtr",(gpointer)&mapvals.position.y);
  gtk_object_set_data(GTK_OBJECT(widget3),"ValuePtr",(gpointer)&mapvals.position.z);

  gtk_widget_show(widget1);
  gtk_widget_show(widget2);
  gtk_widget_show(widget3);

  /* Rotation scales */
  /* =============== */

  table = gtk_table_new(3,2,FALSE);
  gtk_box_pack_start(GTK_BOX(vbox),table,FALSE,FALSE,5);

  label=gck_label_aligned_new("XRot:",NULL,GCK_ALIGN_RIGHT,0.7);
  gtk_table_attach(GTK_TABLE(table),label,0,1,0,1, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(label);

  label=gck_label_aligned_new("YRot:",NULL,GCK_ALIGN_RIGHT,0.7);
  gtk_table_attach(GTK_TABLE(table),label,0,1,1,2, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(label);

  label=gck_label_aligned_new("ZRot:",NULL,GCK_ALIGN_RIGHT,0.7);
  gtk_table_attach(GTK_TABLE(table),label,0,1,2,3, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(label);
 
  angle_scale_vals.value = mapvals.alpha; 
  widget1=gck_hscale_new(NULL,NULL,&angle_scale_vals,(GtkSignalFunc)angle_update);
  angle_scale_vals.value = mapvals.beta; 
  widget2=gck_hscale_new(NULL,NULL,&angle_scale_vals,(GtkSignalFunc)angle_update);
  angle_scale_vals.value = mapvals.gamma; 
  widget3=gck_hscale_new(NULL,NULL,&angle_scale_vals,(GtkSignalFunc)angle_update);

  gtk_table_attach(GTK_TABLE(table),widget1,1,2,0,1, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_table_attach(GTK_TABLE(table),widget2,1,2,1,2, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_table_attach(GTK_TABLE(table),widget3,1,2,2,3, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);

  gtk_widget_show(widget1);
  gtk_widget_show(widget2);
  gtk_widget_show(widget3);

  gtk_object_set_data(GTK_OBJECT(widget1),"ValuePtr",(gpointer)&mapvals.alpha);
  gtk_object_set_data(GTK_OBJECT(widget2),"ValuePtr",(gpointer)&mapvals.beta);
  gtk_object_set_data(GTK_OBJECT(widget3),"ValuePtr",(gpointer)&mapvals.gamma);

  gtk_tooltips_set_tip(tooltips,widget1,"Rotation angle about X axis",NULL);
  gtk_tooltips_set_tip(tooltips,widget2,"Rotation angle about Y axis",NULL);
  gtk_tooltips_set_tip(tooltips,widget3,"Rotation angle about Z axis",NULL);

  /* Scale scales */
  /* ============ */

  scale_table = gtk_table_new(3,2,FALSE);
  gtk_box_pack_start(GTK_BOX(vbox),scale_table,FALSE,FALSE,5);

  label=gck_label_aligned_new("XScale:",NULL,GCK_ALIGN_RIGHT,0.7);
  gtk_table_attach(GTK_TABLE(scale_table),label,0,1,0,1, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(label);

  label=gck_label_aligned_new("YScale:",NULL,GCK_ALIGN_RIGHT,0.7);
  gtk_table_attach(GTK_TABLE(scale_table),label,0,1,1,2, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(label);

  label=gck_label_aligned_new("ZScale:",NULL,GCK_ALIGN_RIGHT,0.7);
  gtk_table_attach(GTK_TABLE(scale_table),label,0,1,2,3, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_widget_show(label);

  scale_scale_vals.value = mapvals.scale.x; 
  widget1=gck_hscale_new(NULL,NULL,&scale_scale_vals,(GtkSignalFunc)boxscale_update);
  scale_scale_vals.value = mapvals.scale.y; 
  widget2=gck_hscale_new(NULL,NULL,&scale_scale_vals,(GtkSignalFunc)boxscale_update);
  scale_scale_vals.value = mapvals.scale.z; 
  widget3=gck_hscale_new(NULL,NULL,&scale_scale_vals,(GtkSignalFunc)boxscale_update);

  gtk_table_attach(GTK_TABLE(scale_table),widget1,1,2,0,1, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_table_attach(GTK_TABLE(scale_table),widget2,1,2,1,2, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
  gtk_table_attach(GTK_TABLE(scale_table),widget3,1,2,2,3, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);

  gtk_widget_show(widget1);
  gtk_widget_show(widget2);
  gtk_widget_show(widget3);

  gtk_object_set_data(GTK_OBJECT(widget1),"ValuePtr",(gpointer)&mapvals.scale.x);
  gtk_object_set_data(GTK_OBJECT(widget2),"ValuePtr",(gpointer)&mapvals.scale.y);
  gtk_object_set_data(GTK_OBJECT(widget3),"ValuePtr",(gpointer)&mapvals.scale.z);

  gtk_tooltips_set_tip(tooltips,widget1,"X scale (size)",NULL);
  gtk_tooltips_set_tip(tooltips,widget2,"Y scale (size)",NULL);
  gtk_tooltips_set_tip(tooltips,widget3,"Z scale (size)",NULL);

  if (mapvals.maptype==MAP_BOX)
    gtk_widget_show(scale_table);

  gtk_widget_show(table);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  gtk_widget_show(page);

  return page;
}

GtkWidget *create_images_page(void)
{
  GtkWidget *page,*frame,*vbox,*label,*table;
  GtkWidget *widget1,*widget2;
  gint i;
  gchar *labels[6] = {"Front:","Back:","Top:","Bottom:","Left:","Right:"};
  
  page=gck_vbox_new(NULL,FALSE,FALSE,FALSE,0,0,0);

  frame=gck_frame_new("Map images to box faces",page,GTK_SHADOW_ETCHED_IN,TRUE,TRUE,0,5);
  vbox=gck_vbox_new(frame,FALSE,FALSE,FALSE,0,0,5);

  table = gtk_table_new(6,2,FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_box_pack_start(GTK_BOX(vbox),table,FALSE,FALSE,5);

  /* Option menues */
  /* ============= */

  for (i=0;i<6;i++)
    {
      label=gck_label_aligned_new(labels[i],NULL,GCK_ALIGN_RIGHT,0.7);
      gtk_table_attach(GTK_TABLE(table),label,0,1,i,i+1, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
      gtk_widget_show(label);

      widget1=gtk_option_menu_new();
      gtk_table_attach(GTK_TABLE(table),widget1, 1,2, i,i+1, GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL, 0,0);
      gtk_widget_show(widget1);

      gtk_object_set_data(GTK_OBJECT(widget1),"_mapwid_id",(gpointer)i);
      
      widget2 = gimp_drawable_menu_new (box_constrain, box_drawable_callback,
        (gpointer)widget1, mapvals.boxmap_id[i]);
      gtk_option_menu_set_menu(GTK_OPTION_MENU(widget1), widget2);      
    }

  gtk_widget_show(table);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  gtk_widget_show(page);

  return page;
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
  label=gtk_label_new("Options");
  gtk_widget_show(label);

  gtk_notebook_append_page(options_note_book,page,label);
  
  page = create_light_page();
  label=gtk_label_new("Light");
  gtk_widget_show(label);

  gtk_notebook_append_page(options_note_book,page,label);
  
  page = create_material_page();
  label=gtk_label_new("Material");
  gtk_widget_show(label);

  gtk_notebook_append_page(options_note_book,page,label);
  
  page = create_orientation_page();
  label=gtk_label_new("Orientation");
  gtk_widget_show(label);

  gtk_notebook_append_page(options_note_book,page,label);

  if (mapvals.maptype==MAP_BOX)
    {
      images_page = create_images_page();
      label=gtk_label_new("Face images");
      gtk_widget_show(label);
    
      gtk_notebook_append_page(options_note_book,images_page,label);
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
  GtkWidget *main_vbox,*main_workbox,*actionbox,*workbox1,*workbox1b,*workbox2,*vbox;
  GtkWidget *frame,*applybutton,*cancelbutton,*helpbutton,*hbox,*gridtoggle;
  GtkWidget *wid;

  appwin = gck_application_window_new("Map to object");
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
  actionbox=gck_hbox_new(main_vbox,TRUE,TRUE,TRUE,5,0,5); 

  /* Add Ok, Cancel and Help buttons to the action area */
  /* ================================================== */

  applybutton=gck_pushbutton_new("Apply",actionbox,FALSE,TRUE,5,(GtkSignalFunc)apply_callback);
  cancelbutton=gck_pushbutton_new("Cancel",actionbox,FALSE,TRUE,5,(GtkSignalFunc)exit_callback);
  helpbutton=gck_pushbutton_new("Help",actionbox,FALSE,TRUE,5,NULL);

  GTK_WIDGET_SET_FLAGS (applybutton, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (cancelbutton, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (helpbutton, GTK_CAN_DEFAULT);

  gtk_widget_grab_default (applybutton);
  gtk_widget_set_sensitive(helpbutton,FALSE);
  
  gtk_tooltips_set_tip(tooltips,applybutton,"Apply filter with current settings",NULL);
  gtk_tooltips_set_tip(tooltips,cancelbutton,"Close filter without doing anything",NULL);

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

  workbox1b=gck_vbox_new(workbox1,TRUE,FALSE,FALSE,0,0,0);
  hbox=gck_hbox_new(workbox1b,FALSE,FALSE,FALSE,5,0,0);
  wid=gck_pushbutton_new("Preview!",hbox,TRUE,TRUE,0,(GtkSignalFunc)preview_callback);
  gtk_tooltips_set_tip(tooltips,wid,"Recompute preview image",NULL);

  hbox=gck_hbox_new(hbox,FALSE,TRUE,TRUE,0,0,0);
  wid=gck_pushbutton_new("+",hbox,TRUE,TRUE,0,(GtkSignalFunc)zoomin_callback);
  gtk_tooltips_set_tip(tooltips,wid,"Zoom in (make image bigger)",NULL);
  wid=gck_pushbutton_new("-",hbox,TRUE,TRUE,0,(GtkSignalFunc)zoomout_callback);
  gtk_tooltips_set_tip(tooltips,wid,"Zoom out (make image smaller)",NULL);

  vbox = gck_vbox_new(workbox1b, FALSE, FALSE, FALSE, 0, 0, 5);
  gridtoggle=gck_checkbutton_new("Show preview wireframe",vbox,mapvals.showgrid,
    (GtkSignalFunc)togglegrid_update);
  gtk_object_set_data(GTK_OBJECT(gridtoggle),"ValuePtr",&mapvals.showgrid);
  gtk_tooltips_set_tip(tooltips,gridtoggle,"Show/hide preview wireframe",NULL);

  create_main_notebook(workbox2);

  /* Endmarkers for line table */
  /* ========================= */
  
  linetab[0].x1=-1;
  
  /* Phew :) Now lets check out the result of this mess */
  /* ================================================== */

  gtk_widget_show(appwin->widget);

  gck_cursor_set(previewarea->window,GDK_HAND2);
  gtk_tooltips_set_colors(tooltips,
    gck_rgb_to_gdkcolor(appwin->visinfo,255,255,220),
    gck_rgb_to_gdkcolor(appwin->visinfo,0,0,0));

  if (mapvals.tooltips_enabled==FALSE)
    gtk_tooltips_disable(tooltips);
}
