#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "fp.h"
#include "fp_hsv.h"

AdvancedWindow AW={NULL,NULL,NULL,NULL,NULL,NULL,NULL};

extern FP_Params Current;

extern GDrawable *drawable, *mask;

FP_Intensity ShMidHi[]   ={SHADOWS,MIDTONES,HIGHLIGHTS};
int          HueSatVal[] ={BY_HUE,BY_SAT,BY_VAL};

gint nudgeArray[256];

GtkWidget *origPreview, *curPreview;
GtkWidget *rPreview, *gPreview, *bPreview;
GtkWidget *cPreview, *yPreview, *mPreview;
GtkWidget *centerPreview;
GtkWidget *darkerPreview, *lighterPreview, *middlePreview;
GtkWidget *allOrSell, *dlg;
GtkWidget *plusSatPreview, *SatPreview, *minusSatPreview;

struct 
{
  GtkWidget *bna, 
    *palette, 
    *rough, 
    *range, 
    *show, 
    *lnd, 
    *pixelsBy, 
    *frameSelect,
    *satur;
} fpFrames;


fpInterface FPint =
{
  FALSE   /*  run  */
};
ReducedImage *reduced;

/***********************************************************/
/************   Main Dialog Window   ******************/
/***********************************************************/

GtkWidget *fp_create_bna(void)
{
  GtkWidget *frame, *blabel, *alabel, *bframe, *aframe, *table;
  
  Create_A_Preview(&origPreview,&bframe, reduced->width, reduced->height);
  Create_A_Preview(&curPreview,&aframe, reduced->width, reduced->height);
  
  frame = gtk_frame_new ("Before And After");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);

  /* All the previews */
  alabel=gtk_label_new("Current:");
  gtk_widget_show(alabel);
  gtk_misc_set_alignment(GTK_MISC(alabel), 0.0, 0.5);

  blabel=gtk_label_new("Original:");
  gtk_widget_show(blabel);
  gtk_misc_set_alignment(GTK_MISC(blabel), 0.0, 0.5);

  table=gtk_table_new(2,2,FALSE);
  gtk_container_border_width(GTK_CONTAINER(table),10);
  gtk_table_set_row_spacings(GTK_TABLE(table),0);
  gtk_table_set_col_spacings(GTK_TABLE(table),20);
  
  gtk_container_add (GTK_CONTAINER (frame), table);
  
  gtk_table_attach(GTK_TABLE(table),blabel,0,1,0,1,
		   0,
		   GTK_EXPAND|GTK_FILL,
		   0,0);
  gtk_table_attach(GTK_TABLE(table),alabel,1,2,0,1,
		   0,
		   GTK_EXPAND|GTK_FILL,
		   0,0);
  gtk_table_attach(GTK_TABLE(table),bframe,0,1,1,2,
		   GTK_EXPAND,
		   GTK_EXPAND,
		   0,0);
  gtk_table_attach(GTK_TABLE(table),aframe,1,2,1,2,
		   GTK_EXPAND,
		   GTK_EXPAND,
		   0,0);

  gtk_widget_show(table);
  gtk_widget_show (frame);
  return frame;
}

GtkWidget *fp_create_circle_palette(void)
{
  GtkWidget *frame, *table;
  GtkWidget *rVbox, *rFrame;
  GtkWidget *gVbox, *gFrame;
  GtkWidget *bVbox, *bFrame;
  GtkWidget *cVbox, *cFrame;
  GtkWidget *yVbox, *yFrame;
  GtkWidget *mVbox, *mFrame;
  GtkWidget *centerVbox, *centerFrame;

  GtkWidget *win;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_widget_show (frame);

  table = gtk_table_new(11,11,FALSE);
  gtk_container_border_width(GTK_CONTAINER(table),0);
  gtk_table_set_row_spacings(GTK_TABLE(table),0);
  gtk_table_set_col_spacings(GTK_TABLE(table),3);
  gtk_widget_show (table);

  gtk_container_add (GTK_CONTAINER (frame), table);

  Create_A_Preview(&rPreview,&rFrame, reduced->width, reduced->height);
  Create_A_Preview(&gPreview,&gFrame, reduced->width, reduced->height);
  Create_A_Preview(&bPreview,&bFrame, reduced->width, reduced->height);
  Create_A_Preview(&cPreview,&cFrame, reduced->width, reduced->height);
  Create_A_Preview(&yPreview,&yFrame, reduced->width, reduced->height);
  Create_A_Preview(&mPreview,&mFrame, reduced->width, reduced->height);
  Create_A_Preview(&centerPreview,&centerFrame, reduced->width, reduced->height);

  Create_A_Table_Entry(&rVbox,rFrame,"Red:");
  Create_A_Table_Entry(&gVbox,gFrame,"Green:");
  Create_A_Table_Entry(&bVbox,bFrame,"Blue:");
  Create_A_Table_Entry(&cVbox,cFrame,"Cyan:");
  Create_A_Table_Entry(&yVbox,yFrame,"Yellow:");
  Create_A_Table_Entry(&mVbox,mFrame,"Magenta:");
  Create_A_Table_Entry(&centerVbox,centerFrame,"Current:");
  
  gtk_table_attach( GTK_TABLE(table), rVbox,8,11,4,7, 
		    GTK_EXPAND , GTK_EXPAND,0,0);
  gtk_table_attach( GTK_TABLE(table), gVbox,2,5,0,3, 
		    GTK_EXPAND, GTK_EXPAND,0,0);
  gtk_table_attach( GTK_TABLE(table), bVbox,2,5,8,11, 
		    GTK_EXPAND, GTK_EXPAND,0,0);
  gtk_table_attach( GTK_TABLE(table), cVbox,0,3,4,7, 
		    GTK_EXPAND, GTK_EXPAND,0,0);
  gtk_table_attach( GTK_TABLE(table), yVbox,6,9,0,3, 
		    GTK_EXPAND, GTK_EXPAND,0,0);
  gtk_table_attach( GTK_TABLE(table), mVbox,6,9,8,11, 
		    GTK_EXPAND, GTK_EXPAND,0,0);
  gtk_table_attach( GTK_TABLE(table), centerVbox,4,7,4,7, 
		    GTK_EXPAND, GTK_EXPAND,0,0);

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win),"Hue Variations");
  gtk_container_add(GTK_CONTAINER(win),frame);
 
  return win;
}

GtkWidget *fp_create_rough(void)
{
  GtkWidget *frame, *scale, *vbox;
  GtkObject *data;

  frame = gtk_frame_new ("Roughness");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_widget_show (frame);

  data = gtk_adjustment_new (Current.Rough, 
			     0, 1.0, 0.05, 0.01, 0.0);
  Current.roughnessScale=
  scale = gtk_hscale_new (GTK_ADJUSTMENT (data));
   
  gtk_widget_set_usize (scale, 60, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) fp_scale_update,
		      &Current.Rough);      
  gtk_widget_show (scale);

  vbox=gtk_vbox_new(FALSE,5);
  gtk_widget_show(vbox);
  gtk_box_pack_start (GTK_BOX(vbox), scale,TRUE,TRUE,5);
  gtk_container_add (GTK_CONTAINER(frame), vbox);
  
  return frame;
}

GtkWidget *fp_create_range(void)
{
  GtkWidget *frame, *vbox;
  GSList *group=NULL;

  frame = gtk_frame_new ("Affected Range");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
   
  gtk_widget_show (frame);

  /********THE THREE RANGES*************/
  vbox=gtk_vbox_new(FALSE,5);
  gtk_container_add(GTK_CONTAINER(frame),vbox);
  gtk_container_border_width(GTK_CONTAINER(vbox),5);  
  gtk_widget_show(vbox);

  group=Button_In_A_Box(vbox,group,"Shadows", 
			 (GtkSignalFunc) fp_change_current_range,
			 ShMidHi+SHADOWS,
			 Current.Range==SHADOWS);
  group=Button_In_A_Box(vbox,group,"Midtones", 
			 (GtkSignalFunc) fp_change_current_range,
			 ShMidHi+MIDTONES,
			 Current.Range==MIDTONES);
  group=Button_In_A_Box(vbox,group,"Highlights", 
			 (GtkSignalFunc) fp_change_current_range,
			 ShMidHi+HIGHLIGHTS,
			 Current.Range==HIGHLIGHTS);
  return frame;
}

  /********THE THREE RANGES*************/
  /********THE THREE RANGES*************/
  /********THE THREE RANGES*************/
  /********THE THREE RANGES*************/
  /********THE THREE RANGES*************/
GtkWidget *fp_create_control(void)
{
  GtkWidget *frame, *box;

  frame = gtk_frame_new ("Windows");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
   
  gtk_widget_show (frame);

  /********THE THREE RANGES*************/
  box=gtk_vbox_new(FALSE,5);
  gtk_container_add(GTK_CONTAINER(frame),box);
  gtk_container_border_width(GTK_CONTAINER(box),5);  
  gtk_widget_show(box);

  Check_Button_In_A_Box(box,"Hue",
			(GtkSignalFunc) fp_show_hide_frame,
			fpFrames.palette,
			Current.VisibleFrames&HUE);
  Check_Button_In_A_Box(box,"Saturation",
			(GtkSignalFunc) fp_show_hide_frame,
			fpFrames.satur,
			Current.VisibleFrames&SATURATION);
  Check_Button_In_A_Box(box,"Value",
			(GtkSignalFunc) fp_show_hide_frame,
			fpFrames.lnd,
			Current.VisibleFrames&VALUE);
  Check_Button_In_A_Box(box,"Advanced",
			(GtkSignalFunc) fp_show_hide_frame,
			AW.window,
			FALSE); 
			
  return frame;
}

GtkWidget *fp_create_lnd()
{
  GtkWidget *frame, *table, *lighterFrame, *middleFrame, *darkerFrame;
  GtkWidget *lighterVbox, *middleVbox, *darkerVbox;
  GtkWidget *win;

  Create_A_Preview(&lighterPreview,&lighterFrame, reduced->width, reduced->height);
  Create_A_Preview(&middlePreview,&middleFrame, reduced->width, reduced->height);
  Create_A_Preview(&darkerPreview,&darkerFrame, reduced->width, reduced->height);
 
  Create_A_Table_Entry(&lighterVbox,lighterFrame,"Lighter:");
  Create_A_Table_Entry(&middleVbox,middleFrame,"Current:");
  Create_A_Table_Entry(&darkerVbox,darkerFrame,"Darker:");

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_widget_show (frame);

  table = gtk_table_new(1,11,FALSE);
  gtk_container_border_width(GTK_CONTAINER(table),0);
  gtk_table_set_row_spacings(GTK_TABLE(table),0);
  gtk_table_set_col_spacings(GTK_TABLE(table),3);
  gtk_widget_show (table);

    
  gtk_table_attach( GTK_TABLE(table), lighterVbox,0,3,0,1, 
		    GTK_EXPAND , GTK_EXPAND,0,0);
  gtk_table_attach( GTK_TABLE(table), middleVbox,4,7,0,1, 
		    GTK_EXPAND, GTK_EXPAND,0,0);
  gtk_table_attach( GTK_TABLE(table), darkerVbox,8,11,0,1, 
		    GTK_EXPAND, GTK_EXPAND,0,0);
  gtk_container_add (GTK_CONTAINER (frame), table);

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win),"Value Variations");
  gtk_container_add(GTK_CONTAINER(win),frame);
 
  return win;
}

GtkWidget *fp_create_msnls()
{

  GtkWidget *frame, *table, *lessFrame, *middleFrame, *moreFrame;
  GtkWidget *lessVbox, *middleVbox, *moreVbox;
  GtkWidget *win;
 
  Create_A_Preview(&minusSatPreview,&lessFrame, reduced->width, reduced->height);
  Create_A_Preview(&SatPreview,&middleFrame, reduced->width, reduced->height);
  Create_A_Preview(&plusSatPreview,&moreFrame, reduced->width, reduced->height);
 
  Create_A_Table_Entry(&moreVbox,moreFrame,"More Sat:");
  Create_A_Table_Entry(&middleVbox,middleFrame,"Current:");
  Create_A_Table_Entry(&lessVbox,lessFrame,"Less Sat:");

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_widget_show(frame);

  table = gtk_table_new(1,11,FALSE);
  gtk_container_border_width(GTK_CONTAINER(table),0);
  gtk_table_set_row_spacings(GTK_TABLE(table),0);
  gtk_table_set_col_spacings(GTK_TABLE(table),3);
  gtk_widget_show (table);

    
  gtk_table_attach( GTK_TABLE(table), moreVbox,0,3,0,1, 
		    GTK_EXPAND , GTK_EXPAND,0,0);
  gtk_table_attach( GTK_TABLE(table), middleVbox,4,7,0,1, 
		    GTK_EXPAND, GTK_EXPAND,0,0);
  gtk_table_attach( GTK_TABLE(table), lessVbox,8,11,0,1, 
		    GTK_EXPAND, GTK_EXPAND,0,0);
  gtk_container_add (GTK_CONTAINER (frame), table);

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win),"Saturation Variations");
  gtk_container_add(GTK_CONTAINER(win),frame);
 
  return win;
}


GtkWidget *fp_create_pixels_select_by()
{
  GtkWidget *frame, *vbox;
  GSList *group=NULL;

  frame = gtk_frame_new ("Select Pixels By");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);  
  gtk_widget_show(frame);

  vbox=gtk_vbox_new(FALSE,5);
  gtk_container_add(GTK_CONTAINER(frame),vbox);
  gtk_container_border_width(GTK_CONTAINER(vbox),5);  
  gtk_widget_show(vbox);

  group=Button_In_A_Box(vbox,group,"Hue", 
			 (GtkSignalFunc) fp_change_current_pixels_by,
			 HueSatVal+0,
			 Current.ValueBy==BY_HUE);
  group=Button_In_A_Box(vbox,group,"Saturation", 
			 (GtkSignalFunc) fp_change_current_pixels_by,
			 HueSatVal+1,
			 Current.ValueBy==BY_SAT);

  group=Button_In_A_Box(vbox,group,"Value", 
			 (GtkSignalFunc) fp_change_current_pixels_by,
			 HueSatVal+2,
			 Current.ValueBy==BY_VAL);
  return frame;
}

GtkWidget *fp_create_show()
{
  GtkWidget *frame, *vbox;
  GSList *group=NULL;

  frame=gtk_frame_new("Show");
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_widget_show(frame);

  vbox=gtk_vbox_new(FALSE,5);
  gtk_container_add(GTK_CONTAINER(frame),vbox);
  gtk_container_border_width(GTK_CONTAINER(vbox),5);  
  gtk_widget_show(vbox);


  group=Button_In_A_Box(vbox,group,"Entire Image", 
			 (GtkSignalFunc) fp_entire_image,
			 &Current.SlctnOnly,
			 FALSE);

  group=Button_In_A_Box(vbox,group,"Selection Only", 
			 (GtkSignalFunc) fp_selection_only,
			 &Current.SlctnOnly,
			 TRUE);

  group=Button_In_A_Box(vbox,group,"Selection In Context", 
			 (GtkSignalFunc) fp_selection_in_context,
			 &Current.SlctnOnly,
			 FALSE);
  return frame;
}
GtkWidget *fp_create_frame_select()
{
  GtkWidget *frame, *box;

  frame=gtk_frame_new("Display");
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_widget_show(frame);

  box=gtk_hbox_new(FALSE,15);
  gtk_container_add(GTK_CONTAINER(frame),box);
  gtk_container_border_width(GTK_CONTAINER(box),5);  
  gtk_widget_show(box);

  Check_Button_In_A_Box (box,"CirclePalette",
			  (GtkSignalFunc) fp_show_hide_frame,
			  fpFrames.palette,TRUE);
  Check_Button_In_A_Box (box,"Lighter And Darker",
			  (GtkSignalFunc) fp_show_hide_frame,
			  fpFrames.lnd,TRUE);
  Check_Button_In_A_Box (box,"Saturation",
			  (GtkSignalFunc) fp_show_hide_frame,
			  fpFrames.satur,FALSE);
  return frame;
}

void Create_A_Preview (GtkWidget  **preview,
		       GtkWidget  **frame,
		       int        previewWidth,
		       int        previewHeight)
{
  *frame=gtk_frame_new(NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (*frame), GTK_SHADOW_IN);
  gtk_container_border_width (GTK_CONTAINER(*frame),0);
  gtk_widget_show(*frame);

  *preview=gtk_preview_new (Current.Color?GTK_PREVIEW_COLOR
			                 :GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (*preview), previewWidth, previewHeight);
  gtk_widget_show(*preview);
  gtk_container_add(GTK_CONTAINER(*frame),*preview);
}


GSList *  Button_In_A_Box        (GtkWidget  *vbox,
				  GSList     *group,
				  guchar     *label,
				  GtkSignalFunc function,
				  gpointer   data,
				  int        clicked)
{
  GtkWidget *button;
  
  button=gtk_radio_button_new_with_label(group,label);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) function,
		      data);
  gtk_widget_show(button);
  gtk_box_pack_start (GTK_BOX(vbox),button, TRUE, TRUE, 0);
  if (clicked)
    gtk_button_clicked(GTK_BUTTON(button));
  return gtk_radio_button_group(GTK_RADIO_BUTTON(button));
}

void Check_Button_In_A_Box       (GtkWidget  *vbox,
				  guchar     *label,
				  GtkSignalFunc function,
				  gpointer   data,
				  int        clicked)
{
  GtkWidget *button;


  button=gtk_check_button_new_with_label(label);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) function,
		      data);
  gtk_box_pack_start (GTK_BOX(vbox),button, TRUE, TRUE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),clicked);

  gtk_widget_show(button);
}


void Create_A_Table_Entry    (GtkWidget  **box,
				     GtkWidget  *SmallerFrame,
				     char       *description)
{
  GtkWidget *label, *button, *table;
  *box = gtk_vbox_new(FALSE,1);
  gtk_container_border_width (GTK_CONTAINER (*box),PR_BX_BRDR);
  gtk_widget_show (*box);
  label = gtk_label_new(description);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show (label);
 
  table=gtk_table_new(2,1,FALSE);
  gtk_widget_show(table);

  gtk_box_pack_start (GTK_BOX(*box),table, TRUE, TRUE, 0);

  gtk_table_attach(GTK_TABLE(table),label,0,1,0,1,0,0,0,0);

  if (strcmp(description,"Current:")) {
    button = gtk_button_new();
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) selectionMade,
                      description);

    gtk_container_add(GTK_CONTAINER(button),SmallerFrame);
    gtk_container_border_width (GTK_CONTAINER (button),0);
    gtk_widget_show(button);
    gtk_table_attach(GTK_TABLE(table), button, 0,1,1,2,0,0,0,5);
  }
  else
    gtk_table_attach(GTK_TABLE(table), SmallerFrame, 0,1,1,2,0,0,0,5);
}

void
fp_redraw_all_windows()
{
  
  reduced=Reduce_The_Image(drawable,mask,
			   Current.PreviewSize,
			   Current.SlctnOnly);
  Adjust_Preview_Sizes(reduced->width,reduced->height);

  /*gtk_container_check_resize(GTK_CONTAINER(fpFrames.palette), NULL);*/
  gtk_widget_draw(fpFrames.palette,NULL);
  
  /*gtk_container_check_resize(GTK_CONTAINER(fpFrames.satur), NULL);*/
  gtk_widget_draw(fpFrames.satur,NULL);
  
  /*gtk_container_check_resize(GTK_CONTAINER(fpFrames.lnd), NULL);*/
  gtk_widget_draw(fpFrames.lnd,NULL);
  
  /*gtk_container_check_resize(GTK_CONTAINER(dlg), NULL);*/
  gtk_widget_draw(dlg,NULL);
  
  refreshPreviews(Current.VisibleFrames);
}

void
fp_entire_image    (GtkWidget *button,
			gpointer  data   )
{
  if (!GTK_TOGGLE_BUTTON(button)->active)
    return;
  Current.SlctnOnly = 0;
  fp_redraw_all_windows();
} 


void
fp_selection_only  (GtkWidget *button,
		    gpointer  data )
{
  static int notFirstTime=0;
  
  if (!(notFirstTime++))  return;

  if (!GTK_TOGGLE_BUTTON(button)->active)
    return;
  Current.SlctnOnly = 1;
  fp_redraw_all_windows();
}


void
fp_selection_in_context  (GtkWidget *button,
			  gpointer  data )
{
  if (!GTK_TOGGLE_BUTTON(button)->active)
    return;
  Current.SlctnOnly = 2;
  fp_redraw_all_windows();
}

void fp_show_hide_frame (GtkWidget *button, 
			 GtkWidget *frame)
{
  int prev=Current.VisibleFrames;
  if (frame == NULL) return;
  if (GTK_TOGGLE_BUTTON(button)->active)
    if (!GTK_WIDGET_VISIBLE(frame)) {
      gtk_widget_show(frame);
      if (frame==fpFrames.palette) 
	Current.VisibleFrames |= HUE;
      else if (frame==fpFrames.satur)
	Current.VisibleFrames |= SATURATION;
      else if (frame==fpFrames.lnd)
	Current.VisibleFrames |= VALUE;
      refreshPreviews(Current.VisibleFrames & ~prev);
    }
    else ;
    
  else 
    if (GTK_WIDGET_VISIBLE(frame)) {
      gtk_widget_hide(frame);
      if (frame==fpFrames.palette) 
	Current.VisibleFrames &= ~HUE;
      else if (frame==fpFrames.satur)
	Current.VisibleFrames &= ~SATURATION;
      else if (frame==fpFrames.lnd)
	Current.VisibleFrames &= ~VALUE;
    }
}

void Adjust_Preview_Sizes(int width, int height)
{
  gtk_preview_size (GTK_PREVIEW (origPreview),    width, height);
  gtk_preview_size (GTK_PREVIEW (curPreview),     width, height);
  gtk_preview_size (GTK_PREVIEW (rPreview),       width, height);
  gtk_preview_size (GTK_PREVIEW (gPreview),       width, height);
  gtk_preview_size (GTK_PREVIEW (bPreview),       width, height);
  gtk_preview_size (GTK_PREVIEW (cPreview),       width, height);
  gtk_preview_size (GTK_PREVIEW (yPreview),       width, height);
  gtk_preview_size (GTK_PREVIEW (mPreview),       width, height);
  gtk_preview_size (GTK_PREVIEW (centerPreview),  width, height);
  gtk_preview_size (GTK_PREVIEW (lighterPreview), width, height);
  gtk_preview_size (GTK_PREVIEW (darkerPreview),  width, height);
  gtk_preview_size (GTK_PREVIEW (middlePreview),  width, height);
  gtk_preview_size (GTK_PREVIEW (minusSatPreview), width, height);
  gtk_preview_size (GTK_PREVIEW (SatPreview),      width, height);
  gtk_preview_size (GTK_PREVIEW (plusSatPreview),  width, height);
}


void      
selectionMade         (GtkWidget *widget,
		       gpointer   data)
{
  Current.Touched[Current.ValueBy] = 1;
  
  if      (!strcmp("Red:",data))     Update_Current_FP( HUE, RED     );
  else if (!strcmp("Green:",data))   Update_Current_FP( HUE, GREEN   );
  else if (!strcmp("Blue:",data))    Update_Current_FP( HUE, BLUE    );
  else if (!strcmp("Cyan:",data))    Update_Current_FP( HUE, CYAN    );
  else if (!strcmp("Yellow:",data))  Update_Current_FP( HUE, YELLOW  );
  else if (!strcmp("Magenta:",data)) Update_Current_FP( HUE, MAGENTA );
  else if (!strcmp("Darker:",data))  Update_Current_FP( VALUE, DOWN );
  else if (!strcmp("Lighter:",data)) Update_Current_FP( VALUE, UP   );
  else if (!strcmp("More Saturated:",data)) 
    Update_Current_FP( SATURATION, UP );
  else if (!strcmp("Less Saturated:",data)) 
    Update_Current_FP( SATURATION, DOWN );
  
  refreshPreviews(Current.VisibleFrames);
}


void refreshPreviews(gint which)
{
  fp_Create_Nudge(nudgeArray);
  fp_render_preview  ( origPreview,     NONEATALL,  0       );
  fp_render_preview  ( curPreview,      CURRENT,    0       );
  if (which & HUE) {
    fp_render_preview( rPreview,        HUE,        RED     );
    fp_render_preview( gPreview,        HUE,        GREEN   );
    fp_render_preview( bPreview,        HUE,        BLUE    );
    fp_render_preview( cPreview,        HUE,        CYAN    );
    fp_render_preview( yPreview,        HUE,        YELLOW  );
    fp_render_preview( mPreview,        HUE,        MAGENTA );
    fp_render_preview( centerPreview,   CURRENT,    0       );
  }
  if (which & VALUE) {
    fp_render_preview( lighterPreview,  VALUE,      UP      );
    fp_render_preview( middlePreview,   CURRENT,    0       );
    fp_render_preview( darkerPreview,   VALUE,      DOWN    );
  }
  if (which & SATURATION) {
    fp_render_preview( plusSatPreview,  SATURATION, UP      );
    fp_render_preview( SatPreview,      CURRENT,    0       );
    fp_render_preview( minusSatPreview, SATURATION, DOWN    );
  }

}

void
fp_close_callback (GtkWidget *widget,
		      gpointer   data)
{
  gtk_main_quit ();
}

void
fp_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  FPint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}


void
fp_scale_update (GtkAdjustment *adjustment,
		 float         *scale_val)
{
  static float prevValue=.25;
  *scale_val = adjustment->value;
  if (prevValue != adjustment->value) {
    fp_Create_Nudge(nudgeArray);
    refreshPreviews(Current.VisibleFrames);
    if (AW.window != NULL && GTK_WIDGET_VISIBLE (AW.window))
      fp_create_smoothness_graph(AW.aliasingPreview);
    prevValue = adjustment->value;
  }
}

void
fp_change_current_range  (GtkAdjustment *button,
		          gint          *Intensity)
{
  static FP_Intensity prevValue=MIDTONES;
  static int notFirstTime=0;
  
  if (!(notFirstTime++))  return;
  if (!GTK_TOGGLE_BUTTON(button)->active)  return;
  if (*Intensity == prevValue) return;
 
  Current.Range = *Intensity;
  refreshPreviews(Current.VisibleFrames);
  if (AW.window != NULL && GTK_WIDGET_VISIBLE (AW.window))
    fp_create_smoothness_graph(AW.aliasingPreview);
  prevValue = *Intensity;
}

void
fp_change_current_pixels_by  (GtkWidget *button,
		              gint      *valueBy)
{
  int prevValue=VALUE;
  static int notFirstTime=0;
  
  if (!(notFirstTime++))  return;
  if (!GTK_TOGGLE_BUTTON(button)->active)  return;
  if (*valueBy == prevValue) return;
 
  Current.ValueBy = *valueBy;
  refreshPreviews(Current.VisibleFrames);
  if (AW.window != NULL && GTK_WIDGET_VISIBLE (AW.window) && AW.rangePreview != NULL)
    fp_range_preview_spill(AW.rangePreview,Current.ValueBy);

  prevValue = *valueBy;
}

void
fp_advanced_call ()
{ 
  if (AW.window!=NULL) 
    if(GTK_WIDGET_VISIBLE(AW.window))
      gtk_widget_hide(AW.window);
    else 
      gtk_widget_show(AW.window);
  else 
    fp_advanced_dialog();
}

int fp_dialog()
{
  GtkWidget *bna;
  GtkWidget *palette;
  GtkWidget *lnd;
  GtkWidget *show;
  GtkWidget *rough;
  GtkWidget *range;
  GtkWidget *pixelsBy;
  GtkWidget *satur;
  GtkWidget *control;

  GtkWidget *table;
  GtkWidget *OKbutton, *CANCELbutton, *RESETbutton; 
  GtkWidget *buttonTable;

  guchar *color_cube;
  gchar **argv;
  gint argc;
 

  reduced=Reduce_The_Image(drawable,mask,
			   Current.PreviewSize,
			   Current.SlctnOnly);

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("fp");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());  

  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
                              color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  /********************************************************************/
  /************************* All the Standard Stuff *******************/
  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Filter Pack Simulation");
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) fp_close_callback,
		      NULL);


  OKbutton = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (OKbutton, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (OKbutton), "clicked",
                      (GtkSignalFunc) fp_ok_callback,
                      dlg);
  gtk_widget_grab_default (OKbutton);
  gtk_widget_show (OKbutton);

  CANCELbutton = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (CANCELbutton, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (CANCELbutton), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_widget_show (CANCELbutton);


  RESETbutton = gtk_button_new_with_label ("Reset");
  GTK_WIDGET_SET_FLAGS (RESETbutton, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (RESETbutton), "clicked",
			     (GtkSignalFunc) resetFilterPacks,
			     NULL);
  gtk_widget_show (RESETbutton);

  
  buttonTable=gtk_table_new(1,4,TRUE);
  gtk_container_border_width(GTK_CONTAINER(buttonTable),0);
  gtk_table_set_col_spacings(GTK_TABLE(buttonTable),3);
  
   
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), buttonTable, TRUE, TRUE, 0);

  gtk_table_attach( GTK_TABLE(buttonTable), OKbutton,0,1,0,1, 
		    GTK_FILL|GTK_EXPAND,0,0,0);
  gtk_table_attach( GTK_TABLE(buttonTable), CANCELbutton,1,2,0,1, 
		    GTK_FILL|GTK_EXPAND,0,0,0);
  gtk_table_attach( GTK_TABLE(buttonTable), RESETbutton,3,4,0,1, 
		    GTK_FILL|GTK_EXPAND,0,0,0);
		   
  gtk_widget_show (buttonTable);
  
  /********************************************************************/
  
  fp_advanced_dialog();

  fpFrames.bna          = bna          = fp_create_bna();
  fpFrames.rough        = rough        = fp_create_rough();
  fpFrames.range        = range        = fp_create_range();
  fpFrames.palette      = palette      = fp_create_circle_palette();
  fpFrames.lnd          = lnd          = fp_create_lnd();
  fpFrames.show         = show         = fp_create_show();
  fpFrames.satur        = satur        = fp_create_msnls();
  fpFrames.pixelsBy     = pixelsBy     = fp_create_pixels_select_by();
                          control      = fp_create_control();
  /********************************************************************/
  /********************   PUT EVRYTHING TOGETHER     ******************/
  

  table=gtk_table_new(4,2,FALSE);
  gtk_table_attach(GTK_TABLE(table),
		   bna,0,2,0,1,
		   GTK_EXPAND|GTK_FILL,
		   GTK_EXPAND|GTK_FILL,
		   0,0);

  gtk_table_attach(GTK_TABLE(table),
		   control,1,2,1,3,
		   GTK_EXPAND|GTK_FILL,
		   GTK_EXPAND|GTK_FILL,
		   0,0);
    gtk_table_attach(GTK_TABLE(table),
		   rough,1,2,3,4,
		   GTK_EXPAND|GTK_FILL,
		   GTK_EXPAND|GTK_FILL,
		   0,0);
		   

  gtk_table_attach(GTK_TABLE(table),
		   show,0,1,1,2,
		   GTK_FILL,
		   GTK_EXPAND|GTK_FILL,
		   0,0);
  gtk_table_attach(GTK_TABLE(table),
		   range,0,1,2,3,
		   GTK_EXPAND|GTK_FILL,
		   GTK_EXPAND|GTK_FILL,
		   0,0);
  gtk_table_attach(GTK_TABLE(table),
		   pixelsBy,0,1,3,4,
		   GTK_EXPAND|GTK_FILL,
		   GTK_EXPAND|GTK_FILL,
		   0,0);
		  
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG (dlg)->vbox), table, TRUE,TRUE,0);
  gtk_widget_show(table);
  gtk_widget_show (dlg);
  
  refreshPreviews(Current.VisibleFrames);
  gtk_main ();
  gdk_flush ();
  
  return FPint.run;
}

/***********************************************************/
/************   Advanced Options Window   ******************/
/***********************************************************/

void fp_advanced_ok()
{
  gtk_widget_hide(AW.window);
}


void     
As_You_Drag           (GtkWidget *button)
{
  static int notFirstTime=0;
  if (!(notFirstTime++))  return;

  if (GTK_TOGGLE_BUTTON(button)->active) {
    Current.RealTime=TRUE;
    gtk_range_set_update_policy (GTK_RANGE (Current.roughnessScale),0);
    gtk_range_set_update_policy (GTK_RANGE (Current.aliasingScale),0);
    gtk_range_set_update_policy (GTK_RANGE (Current.previewSizeScale),0);
  }
  else {
    Current.RealTime=FALSE;
    gtk_range_set_update_policy (GTK_RANGE (Current.roughnessScale),
				 GTK_UPDATE_DELAYED);
    gtk_range_set_update_policy (GTK_RANGE (Current.aliasingScale),
				 GTK_UPDATE_DELAYED);
    gtk_range_set_update_policy (GTK_RANGE (Current.previewSizeScale),
				 GTK_UPDATE_DELAYED);
  }

  return;
}


void
preview_size_scale_update (GtkAdjustment *adjustment,
			   float         *scale_val)
{
  Current.PreviewSize = adjustment->value;
  fp_redraw_all_windows();
}


gint fp_advanced_dialog()
{
  guchar *rangeNames[]={"Shadows:", "Midtones:", "Highlights:"};
  GtkWidget *frame, *mainvbox;
  GtkObject *smoothnessData;
  GtkWidget *graphFrame, *table, *scale;
  GtkWidget *vbox, *label, *labelTable;
  GtkWidget *optionsFrame;
  int i;

  AW.window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(AW.window),"Advanced Filter Pack Options");

  mainvbox=gtk_hbox_new(FALSE,5);
  gtk_container_add(GTK_CONTAINER(AW.window),mainvbox);
  gtk_widget_show(mainvbox);
  /************************************************************/
  frame = gtk_frame_new("Smoothness of Aliasing");
  gtk_container_border_width(GTK_CONTAINER(frame),10);
  gtk_widget_show(frame);

  gtk_box_pack_start(GTK_BOX (mainvbox),frame, TRUE, TRUE,0);

  table=gtk_table_new(3,1,FALSE);
  gtk_widget_show(table);
  gtk_container_add(GTK_CONTAINER(frame),table);
 
  graphFrame=gtk_frame_new(NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (graphFrame), GTK_SHADOW_IN);
  gtk_container_border_width (GTK_CONTAINER(graphFrame),0);
  gtk_widget_show(graphFrame);
  gtk_table_attach(GTK_TABLE(table),graphFrame,0,1,0,1,
		   GTK_EXPAND,
		   0,
		   10,10);
  
  vbox=gtk_vbox_new(FALSE,0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(graphFrame),vbox);

  AW.aliasingPreview=gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (AW.aliasingPreview), 256, MAX_ROUGHNESS);
  gtk_widget_show(AW.aliasingPreview);
  gtk_box_pack_start(GTK_BOX(vbox),AW.aliasingPreview,TRUE,TRUE,0);
  fp_create_smoothness_graph(AW.aliasingPreview);
  
  AW.rangePreview=gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (AW.rangePreview), 256, RANGE_HEIGHT);
  gtk_widget_show(AW.rangePreview);
  gtk_box_pack_start(GTK_BOX(vbox),AW.rangePreview,TRUE,TRUE,0);
  fp_range_preview_spill(AW.rangePreview,Current.ValueBy);
  

  labelTable=gtk_table_new(3,4,FALSE);
  gtk_widget_show(labelTable);
  gtk_table_attach(GTK_TABLE(table),labelTable,0,1,1,2,
		   GTK_EXPAND,
		   0,
		   0,15);

  for (i=0; i<12; i++)
    {
      label=Current.rangeLabels[i]=gtk_label_new("-");
      if (!(i%4)) {
	gtk_label_set_text(GTK_LABEL(label),rangeNames[i/4]);
	gtk_misc_set_alignment(GTK_MISC(label),0.0,1.0);
      }
      gtk_widget_show(label);
      gtk_table_attach(GTK_TABLE(labelTable),label,i%4, i%4+1,i/4, i/4+1,
		       GTK_EXPAND|GTK_FILL,0,5,0);
    }
  /************************************************************/
  /************************************************************/
  /************************************************************/

  
  AW.aliasingGraph = gtk_drawing_area_new();
  gtk_drawing_area_size (GTK_DRAWING_AREA (AW.aliasingGraph),
	  		 2*MARGIN + 256,
	 		 RANGE_HEIGHT);
  gtk_box_pack_start(GTK_BOX(vbox), AW.aliasingGraph, TRUE, TRUE, 0);
  gtk_widget_show(AW.aliasingGraph);
  gtk_widget_set_events(AW.aliasingGraph,RANGE_ADJUST_MASK);
  gtk_signal_connect(GTK_OBJECT(AW.aliasingGraph),"event",
		     (GtkSignalFunc) FP_Range_Change_Events,
		     &Current);


  /************************************************************/
  /************************************************************/
  /************************************************************/


  smoothnessData = gtk_adjustment_new (Current.Alias, 0, 1.0, 0.05, 0.01, 0.0);

  Current.aliasingScale = 
  scale=gtk_hscale_new (GTK_ADJUSTMENT (smoothnessData));
  gtk_widget_set_usize (scale, 200, 0);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), 0);
  gtk_signal_connect (GTK_OBJECT (smoothnessData), "value_changed",
		      (GtkSignalFunc) fp_scale_update,
		      &Current.Alias);      
  gtk_widget_show (scale);
  gtk_table_attach(GTK_TABLE(table),scale,0,1,2,3,0,0,0,15);

  /************************************************************/
  /******************* MISC OPTIONS ***************************/
  /************************************************************/


  optionsFrame=gtk_frame_new("Miscellaneous Options");
  gtk_container_border_width(GTK_CONTAINER(optionsFrame),10);
  gtk_widget_show(optionsFrame);

  gtk_box_pack_start(GTK_BOX(mainvbox),optionsFrame, TRUE, TRUE,0);
 
  table=gtk_table_new(2,2,FALSE);
  gtk_widget_show(table);
  gtk_container_add(GTK_CONTAINER(optionsFrame),table);
 
  vbox=gtk_vbox_new(FALSE,8);
  gtk_container_border_width(GTK_CONTAINER(vbox),10);
  gtk_widget_show(vbox);
  gtk_table_attach(GTK_TABLE(table),vbox,0,1,0,1,0,0,5,5);
 
  Check_Button_In_A_Box (vbox,"Preview As You Drag",
			  (GtkSignalFunc) As_You_Drag,
			  NULL,TRUE);			

  frame=gtk_frame_new("Preview Size");
  gtk_widget_show(frame);
  gtk_container_border_width(GTK_CONTAINER(frame),10);
 
  smoothnessData = gtk_adjustment_new (Current.PreviewSize, 
				       50, MAX_PREVIEW_SIZE, 
				       5, 5, 0.0);
				      
  Current.previewSizeScale=
  scale=gtk_hscale_new (GTK_ADJUSTMENT (smoothnessData));
  gtk_container_add(GTK_CONTAINER(frame),scale);
  gtk_widget_set_usize (scale, 100, 0);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), 0);
  gtk_signal_connect (GTK_OBJECT (smoothnessData), "value_changed",
		      (GtkSignalFunc) preview_size_scale_update,
		      &Current.PreviewSize);        
  gtk_widget_show (scale);
  gtk_table_attach(GTK_TABLE(table),frame,0,1,1,2,GTK_FILL,0,5,5);

  return 1;
}

