#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "fp.h"

/* These values are translated for the GUI but also used internally
   to figure out which button the user pushed, etc.
   Not my design, please don't blame me -- njl */

static const char *hue_red=	N_("Red:");
static const char *hue_green=	N_("Green:");
static const char *hue_blue=	N_("Blue:");
static const char *hue_cyan=	N_("Cyan:");
static const char *hue_yellow=	N_("Yellow:");
static const char *hue_magenta=	N_("Magenta:");

static const char *val_darker=	N_("Darker:");
static const char *val_lighter=	N_("Lighter:");

static const char *sat_more=	N_("More Sat:");
static const char *sat_less=	N_("Less Sat:");

static const char *current_val= N_("Current:");

AdvancedWindow AW = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };

extern FP_Params Current;

extern GDrawable *drawable, *mask;

FP_Intensity ShMidHi[]   = { SHADOWS, MIDTONES, HIGHLIGHTS };
int          HueSatVal[] = { BY_HUE, BY_SAT, BY_VAL };

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

GtkWidget *
fp_create_bna (void)
{
  GtkWidget *frame, *blabel, *alabel, *bframe, *aframe, *table;
  
  Create_A_Preview (&origPreview, &bframe, reduced->width, reduced->height);
  Create_A_Preview (&curPreview, &aframe, reduced->width, reduced->height);
  
  frame = gtk_frame_new (_("Before and After"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);

  /* All the previews */
  alabel = gtk_label_new (_("Current:"));
  gtk_widget_show (alabel);
  gtk_misc_set_alignment (GTK_MISC (alabel), 0.0, 0.5);

  blabel = gtk_label_new (_("Original:"));
  gtk_widget_show (blabel);
  gtk_misc_set_alignment (GTK_MISC (blabel), 0.0, 0.5);

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);

  gtk_container_add (GTK_CONTAINER (frame), table);
  
  gtk_table_attach (GTK_TABLE (table), blabel, 0, 1, 0, 1,
		    0, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), alabel, 1, 2, 0, 1,
		    0, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), bframe, 0, 1, 1, 2,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);

  gtk_table_attach (GTK_TABLE (table), aframe, 1, 2, 1, 2,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  return frame;
}

/* close a sub dialog (from window manager) by simulating toggle click */
void
sub_dialog_destroy (GtkWidget *dialog,
		    GdkEvent  *ev,
		    gpointer   dummy)
{
  GtkWidget *button =
    GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (dialog), "ctrlButton"));

  gtk_signal_emit_by_name (GTK_OBJECT (button), "clicked", dialog);
}

GtkWidget *
fp_create_circle_palette (void)
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
  gtk_container_border_width (GTK_CONTAINER (frame), 4);
  gtk_widget_show (frame);

  table = gtk_table_new (11, 11, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_widget_show (table);

  gtk_container_add (GTK_CONTAINER (frame), table);

  Create_A_Preview (&rPreview, &rFrame, reduced->width, reduced->height);
  Create_A_Preview (&gPreview, &gFrame, reduced->width, reduced->height);
  Create_A_Preview (&bPreview, &bFrame, reduced->width, reduced->height);
  Create_A_Preview (&cPreview, &cFrame, reduced->width, reduced->height);
  Create_A_Preview (&yPreview, &yFrame, reduced->width, reduced->height);
  Create_A_Preview (&mPreview, &mFrame, reduced->width, reduced->height);
  Create_A_Preview (&centerPreview, &centerFrame,
		    reduced->width, reduced->height);

  Create_A_Table_Entry (&rVbox, rFrame, hue_red);
  Create_A_Table_Entry (&gVbox, gFrame, hue_green);
  Create_A_Table_Entry (&bVbox, bFrame, hue_blue);
  Create_A_Table_Entry (&cVbox, cFrame, hue_cyan);
  Create_A_Table_Entry (&yVbox, yFrame, hue_yellow);
  Create_A_Table_Entry (&mVbox, mFrame, hue_magenta);
  Create_A_Table_Entry (&centerVbox, centerFrame, current_val);
  
  gtk_table_attach (GTK_TABLE (table), rVbox, 8, 11 ,4 , 7,
		    GTK_EXPAND , GTK_EXPAND, 0 ,0);
  gtk_table_attach (GTK_TABLE (table), gVbox, 2, 5, 0, 3,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);
  gtk_table_attach (GTK_TABLE (table), bVbox, 2, 5, 8, 11,
		    GTK_EXPAND, GTK_EXPAND,0,0);
  gtk_table_attach (GTK_TABLE (table), cVbox, 0, 3, 4, 7,
		    GTK_EXPAND, GTK_EXPAND, 0 ,0);
  gtk_table_attach (GTK_TABLE (table), yVbox, 6, 9, 0, 3,
		    GTK_EXPAND, GTK_EXPAND, 0 ,0);
  gtk_table_attach (GTK_TABLE (table), mVbox, 6, 9, 8, 11,
		    GTK_EXPAND, GTK_EXPAND, 0 ,0);
  gtk_table_attach (GTK_TABLE (table), centerVbox, 4, 7, 4, 7,
		    GTK_EXPAND, GTK_EXPAND, 0 ,0);

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gimp_help_connect_help_accel (win, gimp_standard_help_func, "filters/fp.html");

  gtk_window_set_title (GTK_WINDOW (win), _("Hue Variations"));
  gtk_container_add (GTK_CONTAINER (win), frame);
  gtk_signal_connect (GTK_OBJECT (win), "delete_event",
		      GTK_SIGNAL_FUNC (sub_dialog_destroy),
		      NULL);

  return win;
}

GtkWidget *
fp_create_rough (void)
{
  GtkWidget *frame, *scale, *vbox;
  GtkObject *data;

  frame = gtk_frame_new (_("Roughness"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_widget_show (frame);

  data = gtk_adjustment_new (Current.Rough, 0, 1.0, 0.05, 0.01, 0.0);
  Current.roughnessScale = scale = gtk_hscale_new (GTK_ADJUSTMENT (data));

  gtk_widget_set_usize (scale, 60, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      GTK_SIGNAL_FUNC (fp_scale_update),
		      &Current.Rough);      
  gtk_widget_show (scale);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER( frame), vbox);
  
  return frame;
}

GtkWidget *
fp_create_range (void)
{
  GtkWidget *frame, *vbox;
  GSList *group=NULL;

  frame = gtk_frame_new (_("Affected Range"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
   
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);  
  gtk_widget_show (vbox);

  group = Button_In_A_Box (vbox, group, _("Shadows"), 
			   GTK_SIGNAL_FUNC (fp_change_current_range),
			   ShMidHi + SHADOWS,
			   Current.Range == SHADOWS);
  group = Button_In_A_Box (vbox, group, _("Midtones"), 
			   GTK_SIGNAL_FUNC (fp_change_current_range),
			   ShMidHi + MIDTONES,
			   Current.Range == MIDTONES);
  group = Button_In_A_Box (vbox, group, _("Highlights"), 
			 GTK_SIGNAL_FUNC (fp_change_current_range),
			 ShMidHi + HIGHLIGHTS,
			 Current.Range == HIGHLIGHTS);

  return frame;
}

GtkWidget *
fp_create_control (void)
{
  GtkWidget *frame, *box;

  frame = gtk_frame_new (_("Windows"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
   
  gtk_widget_show (frame);

  box = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_container_set_border_width (GTK_CONTAINER (box), 4);  
  gtk_widget_show (box);

  Frames_Check_Button_In_A_Box (box, _("Hue"),
				GTK_SIGNAL_FUNC (fp_show_hide_frame),
				fpFrames.palette,
				Current.VisibleFrames & HUE);
  Frames_Check_Button_In_A_Box (box, _("Saturation"),
				GTK_SIGNAL_FUNC (fp_show_hide_frame),
				fpFrames.satur,
				Current.VisibleFrames & SATURATION);
  Frames_Check_Button_In_A_Box (box, _("Value"),
				GTK_SIGNAL_FUNC (fp_show_hide_frame),
				fpFrames.lnd,
				Current.VisibleFrames & VALUE);
  Frames_Check_Button_In_A_Box (box, _("Advanced"),
				GTK_SIGNAL_FUNC (fp_show_hide_frame),
				AW.window,
				FALSE); 

  return frame;
}

GtkWidget *
fp_create_lnd (void)
{
  GtkWidget *frame, *table, *lighterFrame, *middleFrame, *darkerFrame;
  GtkWidget *lighterVbox, *middleVbox, *darkerVbox;
  GtkWidget *win;

  Create_A_Preview (&lighterPreview, &lighterFrame,
		    reduced->width, reduced->height);
  Create_A_Preview (&middlePreview, &middleFrame,
		    reduced->width, reduced->height);
  Create_A_Preview (&darkerPreview, &darkerFrame,
		    reduced->width, reduced->height);
 
  Create_A_Table_Entry (&lighterVbox, lighterFrame, val_lighter);
  Create_A_Table_Entry (&middleVbox, middleFrame, current_val);
  Create_A_Table_Entry (&darkerVbox, darkerFrame, val_darker);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 6);
  gtk_widget_show (frame);

  table = gtk_table_new (1, 11, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_widget_show (table);

    
  gtk_table_attach (GTK_TABLE (table), lighterVbox, 0, 3, 0, 1,
		    GTK_EXPAND , GTK_EXPAND, 0, 0);
  gtk_table_attach (GTK_TABLE (table), middleVbox, 4, 7, 0, 1,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);
  gtk_table_attach (GTK_TABLE (table), darkerVbox, 8, 11, 0, 1,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);
  gtk_container_add (GTK_CONTAINER (frame), table);

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gimp_help_connect_help_accel (win, gimp_standard_help_func, "filters/fp.html");

  gtk_window_set_title (GTK_WINDOW (win), _("Value Variations"));
  gtk_container_add (GTK_CONTAINER (win), frame);
  gtk_signal_connect (GTK_OBJECT (win), "delete_event",
		      GTK_SIGNAL_FUNC (sub_dialog_destroy),
		      NULL);
 
  return win;
}

GtkWidget *
fp_create_msnls (void)
{
  GtkWidget *frame, *table, *lessFrame, *middleFrame, *moreFrame;
  GtkWidget *lessVbox, *middleVbox, *moreVbox;
  GtkWidget *win;
 
  Create_A_Preview (&minusSatPreview, &lessFrame,
		    reduced->width, reduced->height);
  Create_A_Preview (&SatPreview, &middleFrame,
		    reduced->width, reduced->height);
  Create_A_Preview (&plusSatPreview, &moreFrame,
		    reduced->width, reduced->height);
 
  Create_A_Table_Entry (&moreVbox, moreFrame, sat_more);
  Create_A_Table_Entry (&middleVbox, middleFrame, current_val);
  Create_A_Table_Entry (&lessVbox, lessFrame, sat_less);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 4);
  gtk_widget_show (frame);

  table = gtk_table_new (1, 11, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_widget_show (table);
    
  gtk_table_attach (GTK_TABLE (table), moreVbox, 0, 3, 0, 1,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);
  gtk_table_attach (GTK_TABLE (table), middleVbox, 4, 7, 0, 1,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);
  gtk_table_attach (GTK_TABLE (table), lessVbox, 8, 11, 0, 1,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);
  gtk_container_add (GTK_CONTAINER (frame), table);

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gimp_help_connect_help_accel (win, gimp_standard_help_func, "filters/fp.html");

  gtk_window_set_title (GTK_WINDOW (win), _("Saturation Variations"));
  gtk_container_add (GTK_CONTAINER (win), frame);
  gtk_signal_connect (GTK_OBJECT (win), "delete_event",
		      GTK_SIGNAL_FUNC (sub_dialog_destroy),
		      NULL);

  return win;
}

GtkWidget *
fp_create_pixels_select_by (void)
{
  GtkWidget *frame, *vbox;
  GSList *group=NULL;

  frame = gtk_frame_new (_("Select Pixels by"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);  
  gtk_widget_show (vbox);

  group = Button_In_A_Box (vbox, group, _("Hue"), 
			   GTK_SIGNAL_FUNC (fp_change_current_pixels_by),
			   HueSatVal + 0,
			   Current.ValueBy == BY_HUE);
  group = Button_In_A_Box (vbox,group, _("Saturation"), 
			   GTK_SIGNAL_FUNC (fp_change_current_pixels_by),
			   HueSatVal + 1,
			   Current.ValueBy == BY_SAT);
  group = Button_In_A_Box (vbox,group, _("Value"), 
			   GTK_SIGNAL_FUNC (fp_change_current_pixels_by),
			   HueSatVal + 2,
			   Current.ValueBy == BY_VAL);
  return frame;
}

GtkWidget *
fp_create_show (void)
{
  GtkWidget *frame, *vbox;
  GSList *group = NULL;

  frame = gtk_frame_new(_("Show"));
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);  
  gtk_widget_show (vbox);

  group = Button_In_A_Box (vbox, group, _("Entire Image"),
			   GTK_SIGNAL_FUNC (fp_entire_image),
			   &Current.SlctnOnly,
			   FALSE);

  group = Button_In_A_Box (vbox, group, _("Selection Only"),
			   GTK_SIGNAL_FUNC (fp_selection_only),
			   &Current.SlctnOnly,
			   TRUE);

  group = Button_In_A_Box (vbox, group, _("Selection In Context"), 
			   GTK_SIGNAL_FUNC (fp_selection_in_context),
			   &Current.SlctnOnly,
			   FALSE);

  return frame;
}

#ifdef __FP_UNUSED_STUFF__
GtkWidget *
fp_create_frame_select (void)
{
  GtkWidget *frame, *box;

  frame = gtk_frame_new (_("Display"));
  gtk_container_border_width (GTK_CONTAINER (frame), 4);
  gtk_widget_show (frame);

  box = gtk_hbox_new (FALSE, 8);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_container_set_border_width (GTK_CONTAINER (box),4);  
  gtk_widget_show (box);

  Check_Button_In_A_Box (box, _("CirclePalette"),
			 GTK_SIGNAL_FUNC (fp_show_hide_frame),
			 fpFrames.palette,TRUE);
  Check_Button_In_A_Box (box, _("Lighter And Darker"),
			 GTK_SIGNAL_FUNC (fp_show_hide_frame),
			 fpFrames.lnd,TRUE);
  Check_Button_In_A_Box (box, _("Saturation"),
			 GTK_SIGNAL_FUNC (fp_show_hide_frame),
			 fpFrames.satur,FALSE);

  return frame;
}
#endif

void Create_A_Preview (GtkWidget **preview,
		       GtkWidget **frame,
		       int         previewWidth,
		       int         previewHeight)
{
  *frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (*frame), GTK_SHADOW_IN);
  gtk_widget_show (*frame);

  *preview = gtk_preview_new (Current.Color ?
			      GTK_PREVIEW_COLOR : GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (*preview), previewWidth, previewHeight);
  gtk_widget_show (*preview);
  gtk_container_add (GTK_CONTAINER (*frame), *preview);
}

GSList *
Button_In_A_Box (GtkWidget     *vbox,
		 GSList        *group,
		 guchar        *label,
		 GtkSignalFunc  function,
		 gpointer       data,
		 gboolean       clicked)
{
  GtkWidget *button;

  button = gtk_radio_button_new_with_label (group, label);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (function),
		      data);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  if (clicked)
    gtk_button_clicked (GTK_BUTTON (button));

  return gtk_radio_button_group (GTK_RADIO_BUTTON (button));
}

void
Check_Button_In_A_Box (GtkWidget     *vbox,
		       guchar        *label,
		       GtkSignalFunc  function,
		       gpointer       data,
		       gboolean       clicked)
{
  GtkWidget *button;

  button = gtk_check_button_new_with_label (label);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (function),
		      data);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clicked);

  gtk_widget_show (button);
}

void
Frames_Check_Button_In_A_Box (GtkWidget     *vbox,
			      guchar        *label,
			      GtkSignalFunc  function,
			      GtkWidget     *frame,
			      gboolean       clicked)
{
  GtkWidget *button;

  button = gtk_check_button_new_with_label (label);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (function),
		      frame);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clicked);
  gtk_object_set_data (GTK_OBJECT (frame), "ctrlButton", (gpointer) button);

  gtk_widget_show (button);
}

void
Create_A_Table_Entry (GtkWidget **box,
		      GtkWidget *SmallerFrame,
		      const gchar *description)
{
  GtkWidget *label, *button, *table;

  *box = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (*box), PR_BX_BRDR);
  gtk_widget_show (*box);

  /* Delayed translation applied here */
  label = gtk_label_new (gettext(description));

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);

  table = gtk_table_new (2, 1, FALSE);
  gtk_widget_show (table);

  gtk_box_pack_start (GTK_BOX (*box), table, TRUE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    0, 0, 0, 0);

  if (description != current_val)
    {
      button = gtk_button_new ();
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (selectionMade),
			  (gchar *) description);

    gtk_container_add (GTK_CONTAINER (button), SmallerFrame);
    gtk_widget_show (button);
    gtk_table_attach (GTK_TABLE (table), button, 0, 1, 1, 2,
		      0, 0, 0, 4);
    }
  else
    gtk_table_attach (GTK_TABLE (table), SmallerFrame, 0, 1, 1, 2,
		      0, 0, 0, 4);
}

void
fp_redraw_all_windows (void)
{
  reduced = Reduce_The_Image (drawable,mask,
			      Current.PreviewSize,
			      Current.SlctnOnly);
  Adjust_Preview_Sizes (reduced->width, reduced->height);

  /*gtk_container_check_resize(GTK_CONTAINER(fpFrames.palette), NULL);*/
  gtk_widget_draw (fpFrames.palette, NULL);

  /*gtk_container_check_resize(GTK_CONTAINER(fpFrames.satur), NULL);*/
  gtk_widget_draw (fpFrames.satur, NULL);

  /*gtk_container_check_resize(GTK_CONTAINER(fpFrames.lnd), NULL);*/
  gtk_widget_draw (fpFrames.lnd, NULL);

  /*gtk_container_check_resize(GTK_CONTAINER(dlg), NULL);*/
  gtk_widget_draw (dlg, NULL);

  refreshPreviews (Current.VisibleFrames);
}

void
fp_entire_image (GtkWidget *button,
		 gpointer   data)
{
  if (!GTK_TOGGLE_BUTTON (button)->active)
    return;
  Current.SlctnOnly = 0;
  fp_redraw_all_windows ();
} 

void
fp_selection_only (GtkWidget *button,
		   gpointer   data)
{
  static int notFirstTime = 0;

  if (!(notFirstTime++))
    return;

  if (!GTK_TOGGLE_BUTTON (button)->active)
    return;
  Current.SlctnOnly = 1;
  fp_redraw_all_windows ();
}

void
fp_selection_in_context (GtkWidget *button,
			 gpointer   data)
{
  if (!GTK_TOGGLE_BUTTON (button)->active)
    return;

  Current.SlctnOnly = 2;
  fp_redraw_all_windows ();
}

void
fp_show_hide_frame (GtkWidget *button, 
		    GtkWidget *frame)
{
  int prev = Current.VisibleFrames;

  if (frame == NULL)
    return;

  if (GTK_TOGGLE_BUTTON (button)->active)
    {
      if (!GTK_WIDGET_VISIBLE (frame))
	{
	  gtk_widget_show (frame);

	  if (frame==fpFrames.palette) 
	    Current.VisibleFrames |= HUE;
	  else if (frame==fpFrames.satur)
	    Current.VisibleFrames |= SATURATION;
	  else if (frame==fpFrames.lnd)
	    Current.VisibleFrames |= VALUE;

	  refreshPreviews (Current.VisibleFrames & ~prev);
	}
    }    
  else
    {
      if (GTK_WIDGET_VISIBLE (frame))
	{
	  gtk_widget_hide (frame);

	  if (frame==fpFrames.palette) 
	    Current.VisibleFrames &= ~HUE;
	  else if (frame==fpFrames.satur)
	    Current.VisibleFrames &= ~SATURATION;
	  else if (frame==fpFrames.lnd)
	    Current.VisibleFrames &= ~VALUE;
	}
    }
}

void
Adjust_Preview_Sizes (gint width,
		      gint height)
{
  gtk_preview_size (GTK_PREVIEW (origPreview),     width, height);
  gtk_preview_size (GTK_PREVIEW (curPreview),      width, height);
  gtk_preview_size (GTK_PREVIEW (rPreview),        width, height);
  gtk_preview_size (GTK_PREVIEW (gPreview),        width, height);
  gtk_preview_size (GTK_PREVIEW (bPreview),        width, height);
  gtk_preview_size (GTK_PREVIEW (cPreview),        width, height);
  gtk_preview_size (GTK_PREVIEW (yPreview),        width, height);
  gtk_preview_size (GTK_PREVIEW (mPreview),        width, height);
  gtk_preview_size (GTK_PREVIEW (centerPreview),   width, height);
  gtk_preview_size (GTK_PREVIEW (lighterPreview),  width, height);
  gtk_preview_size (GTK_PREVIEW (darkerPreview),   width, height);
  gtk_preview_size (GTK_PREVIEW (middlePreview),   width, height);
  gtk_preview_size (GTK_PREVIEW (minusSatPreview), width, height);
  gtk_preview_size (GTK_PREVIEW (SatPreview),      width, height);
  gtk_preview_size (GTK_PREVIEW (plusSatPreview),  width, height);
}

void      
selectionMade (GtkWidget *widget,
	       gpointer   data)
{
  Current.Touched[Current.ValueBy] = 1;
  
  if (data == (gpointer) hue_red) {
    Update_Current_FP (HUE, RED);
  } else if (data == (gpointer) hue_green) {
    Update_Current_FP (HUE, GREEN);
  } else if (data == (gpointer) hue_blue) {
    Update_Current_FP (HUE, BLUE);
  } else if (data == (gpointer) hue_cyan) {
    Update_Current_FP (HUE, CYAN);
  } else if (data == (gpointer) hue_yellow) {
    Update_Current_FP (HUE, YELLOW);
  } else if (data == (gpointer) hue_magenta) {
    Update_Current_FP (HUE, MAGENTA);
  } else if (data == (gpointer) val_darker) {
    Update_Current_FP (VALUE, DOWN);
  } else if (data == (gpointer) val_lighter) {
    Update_Current_FP (VALUE, UP);
  } else if (data == (gpointer) sat_more) {
    Update_Current_FP (SATURATION, UP);
  } else if (data == (gpointer) sat_less) {
    Update_Current_FP (SATURATION, DOWN);
  }

  refreshPreviews (Current.VisibleFrames);
}

void
refreshPreviews (gint which)
{
  fp_Create_Nudge (nudgeArray);
  fp_render_preview     (origPreview,     NONEATALL,  0);
  fp_render_preview     (curPreview,      CURRENT,    0);
  if (which & HUE)
    {
      fp_render_preview (rPreview,        HUE,        RED);
      fp_render_preview (gPreview,        HUE,        GREEN);
      fp_render_preview (bPreview,        HUE,        BLUE);
      fp_render_preview (cPreview,        HUE,        CYAN);
      fp_render_preview (yPreview,        HUE,        YELLOW);
      fp_render_preview (mPreview,        HUE,        MAGENTA);
      fp_render_preview (centerPreview,   CURRENT,    0);
    }
  if (which & VALUE)
    {
      fp_render_preview (lighterPreview,  VALUE,      UP);
      fp_render_preview (middlePreview,   CURRENT,    0);
      fp_render_preview (darkerPreview,   VALUE,      DOWN);
    }
  if (which & SATURATION)
    {
      fp_render_preview (plusSatPreview,  SATURATION, UP);
      fp_render_preview (SatPreview,      CURRENT,    0);
      fp_render_preview (minusSatPreview, SATURATION, DOWN);
    }
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

  if (prevValue != adjustment->value)
    {
      fp_Create_Nudge (nudgeArray);
      refreshPreviews (Current.VisibleFrames);
      if (AW.window != NULL && GTK_WIDGET_VISIBLE (AW.window))
	fp_create_smoothness_graph (AW.aliasingPreview);
      prevValue = adjustment->value;
    }
}

void
fp_change_current_range (GtkAdjustment *button,
			 gint          *Intensity)
{
  static FP_Intensity prevValue=MIDTONES;
  static int notFirstTime=0;
  
  if (!(notFirstTime++))  return;
  if (!GTK_TOGGLE_BUTTON (button)->active)  return;
  if (*Intensity == prevValue) return;
 
  Current.Range = *Intensity;
  refreshPreviews (Current.VisibleFrames);
  if (AW.window != NULL && GTK_WIDGET_VISIBLE (AW.window))
    fp_create_smoothness_graph (AW.aliasingPreview);
  prevValue = *Intensity;
}

void
fp_change_current_pixels_by (GtkWidget *button,
			     gint      *valueBy)
{
  int prevValue=VALUE;
  static int notFirstTime=0;
  
  if (!(notFirstTime++))  return;
  if (!GTK_TOGGLE_BUTTON(button)->active)  return;
  if (*valueBy == prevValue) return;
 
  Current.ValueBy = *valueBy;
  refreshPreviews (Current.VisibleFrames);
  if (AW.window != NULL &&
      GTK_WIDGET_VISIBLE (AW.window) &&
      AW.rangePreview != NULL)
    fp_range_preview_spill (AW.rangePreview,Current.ValueBy);

  prevValue = *valueBy;
}

void
fp_advanced_call (void)
{ 
  if (AW.window!=NULL) 
    if (GTK_WIDGET_VISIBLE (AW.window))
      gtk_widget_hide (AW.window);
    else 
      gtk_widget_show (AW.window);
  else 
    fp_advanced_dialog ();
}

int
fp_dialog (void)
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

  reduced = Reduce_The_Image (drawable,mask,
			      Current.PreviewSize,
			      Current.SlctnOnly);

  gimp_ui_init ("fp", TRUE);

  /********************************************************************/
  /************************* All the Standard Stuff *******************/
  dlg = gimp_dialog_new (_("Filter Pack Simulation"), "fp",
			 gimp_standard_help_func, "filters/fp.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), fp_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Reset"), resetFilterPacks,
			 NULL, NULL, NULL, FALSE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /********************************************************************/
  
  fp_advanced_dialog ();

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
  
  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);

  gtk_table_attach (GTK_TABLE (table), bna, 0, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), control, 1, 2, 1, 3,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), rough, 1, 2, 3, 4,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), show, 0, 1, 1, 2,
		    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), range, 0, 1, 2, 3,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), pixelsBy, 0, 1, 3, 4,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
		  
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  refreshPreviews (Current.VisibleFrames);

  gtk_main ();
  gdk_flush ();
  
  return FPint.run;
}

/***********************************************************/
/************   Advanced Options Window   ******************/
/***********************************************************/

void
fp_advanced_ok (void)
{
  gtk_widget_hide (AW.window);
}

void     
As_You_Drag (GtkWidget *button)
{
  static gboolean notFirstTime = FALSE;

  if (! notFirstTime)
    return;

  notFirstTime = TRUE;

  if (GTK_TOGGLE_BUTTON (button)->active)
    {
      Current.RealTime=TRUE;
      gtk_range_set_update_policy (GTK_RANGE (Current.roughnessScale),0);
      gtk_range_set_update_policy (GTK_RANGE (Current.aliasingScale),0);
      gtk_range_set_update_policy (GTK_RANGE (Current.previewSizeScale),0);
    }
  else
    {
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

gint
fp_advanced_dialog (void)
{
  guchar *rangeNames[] = { N_("Shadows:"), N_("Midtones:"), N_("Highlights:")};
  GtkWidget *frame, *mainvbox;
  GtkObject *smoothnessData;
  GtkWidget *graphFrame, *table, *scale;
  GtkWidget *vbox, *label, *labelTable;
  GtkWidget *optionsFrame;
  gint i;

  AW.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gimp_help_connect_help_accel (AW.window, gimp_standard_help_func,
				"filters/fp.html");

  gtk_window_set_title (GTK_WINDOW (AW.window),
			_("Advanced Filter Pack Options"));
  gtk_signal_connect (GTK_OBJECT (AW.window), "delete_event",
		      GTK_SIGNAL_FUNC (sub_dialog_destroy),
		      NULL);

  mainvbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (mainvbox), 6);
  gtk_container_add (GTK_CONTAINER (AW.window), mainvbox);
  gtk_widget_show (mainvbox);

  frame = gtk_frame_new (_("Smoothness of Aliasing"));
  gtk_box_pack_start (GTK_BOX (mainvbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 1, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);
 
  graphFrame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (graphFrame), GTK_SHADOW_IN);
  gtk_container_border_width (GTK_CONTAINER (graphFrame),0);
  gtk_widget_show (graphFrame);
  gtk_table_attach (GTK_TABLE (table), graphFrame, 0, 1, 0, 1,
		    GTK_EXPAND, 0, 0, 0);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (graphFrame), vbox);
  gtk_widget_show (vbox);

  AW.aliasingPreview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (AW.aliasingPreview), 256, MAX_ROUGHNESS);
  gtk_box_pack_start (GTK_BOX (vbox),AW.aliasingPreview, TRUE, TRUE, 0);
  gtk_widget_show (AW.aliasingPreview);

  fp_create_smoothness_graph (AW.aliasingPreview);

  AW.rangePreview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (AW.rangePreview), 256, RANGE_HEIGHT);
  gtk_box_pack_start(GTK_BOX(vbox),AW.rangePreview, TRUE, TRUE, 0);
  gtk_widget_show (AW.rangePreview);

  fp_range_preview_spill (AW.rangePreview, Current.ValueBy);

  labelTable = gtk_table_new (3, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (labelTable), 4);
  gtk_table_set_row_spacings (GTK_TABLE (labelTable), 2);
  gtk_widget_show (labelTable);
  gtk_table_attach (GTK_TABLE (table), labelTable, 0, 1, 1, 2,
		    GTK_EXPAND, 0, 0, 0);

  for (i = 0; i < 12; i++)
    {
      label = Current.rangeLabels[i] = gtk_label_new ("-");
      if (!(i % 4))
	{
	  gtk_label_set_text (GTK_LABEL(label), gettext (rangeNames[i/4]));
	  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
	}
      gtk_widget_show (label);
      gtk_table_attach (GTK_TABLE (labelTable), label, i%4, i%4+1, i/4, i/4+1,
			GTK_EXPAND | GTK_FILL, 0, 0, 0);
    }

  /************************************************************/
  
  AW.aliasingGraph = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (AW.aliasingGraph),
	  		 2 * MARGIN + 256,
	 		 RANGE_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), AW.aliasingGraph, TRUE, TRUE, 0);
  gtk_widget_show (AW.aliasingGraph);
  gtk_widget_set_events (AW.aliasingGraph, RANGE_ADJUST_MASK);
  gtk_signal_connect (GTK_OBJECT (AW.aliasingGraph),"event",
		      GTK_SIGNAL_FUNC (FP_Range_Change_Events),
		     &Current);

  /************************************************************/

  smoothnessData = gtk_adjustment_new (Current.Alias, 0, 1.0, 0.05, 0.01, 0.0);

  Current.aliasingScale = scale =
    gtk_hscale_new (GTK_ADJUSTMENT (smoothnessData));
  gtk_widget_set_usize (scale, 200, 0);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), 0);
  gtk_signal_connect (GTK_OBJECT (smoothnessData), "value_changed",
		      GTK_SIGNAL_FUNC (fp_scale_update),
		      &Current.Alias);
  gtk_widget_show (scale);
  gtk_table_attach (GTK_TABLE (table), scale, 0, 1, 2, 3,
		    0, 0, 0, 0);

  /******************* MISC OPTIONS ***************************/

  optionsFrame = gtk_frame_new (_("Miscellaneous Options"));
  gtk_widget_show (optionsFrame);

  gtk_box_pack_start (GTK_BOX (mainvbox), optionsFrame, TRUE, TRUE, 0);

 
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (optionsFrame), vbox);
  gtk_widget_show (vbox);
 
  Check_Button_In_A_Box (vbox, _("Preview as You Drag"),
			 GTK_SIGNAL_FUNC (As_You_Drag),
			 NULL, TRUE);

  frame = gtk_frame_new (_("Preview Size"));
  gtk_widget_show (frame);

  smoothnessData = gtk_adjustment_new (Current.PreviewSize, 
				       50, MAX_PREVIEW_SIZE, 
				       5, 5, 0.0);
      
  Current.previewSizeScale = scale =
    gtk_hscale_new (GTK_ADJUSTMENT (smoothnessData));
  gtk_container_add (GTK_CONTAINER (frame),scale);
  gtk_widget_set_usize (scale, 100, 0);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), 0);
  gtk_signal_connect (GTK_OBJECT (smoothnessData), "value_changed",
		      GTK_SIGNAL_FUNC (preview_size_scale_update),
		      &Current.PreviewSize);        
  gtk_widget_show (scale);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  return 1;
}
