#include "pmg_gtk_objects.h"

PmgButtonLabel *pmg_button_with_label_new()
{
  PmgButtonLabel *structure;

  structure = g_new(PmgButtonLabel,1);
  
  return structure;
}

PmgButtonLabelXpm *
pmg_button_label_xpm_new(GtkWidget      *parent,
			 gchar          *xpm_filename, 
			 gchar          *label_text,
			 gchar          *additional_label_text,
			 gint           v_or_h,
			 GtkSignalFunc  *click_function,
			 gpointer       *click_data)
{
  PmgButtonLabelXpm *xpm_button;
  GtkWidget *label_box;

  xpm_button = g_new(PmgButtonLabelXpm, 1);
  
  switch (v_or_h) {
  case 'v':
    xpm_button->box = gtk_vbox_new (FALSE, 0);
    break;
  case 'h':
    xpm_button->box = gtk_hbox_new (FALSE, 0);
    break;
  }

  gtk_container_border_width(GTK_CONTAINER(xpm_button->box), 2);

  gtk_widget_show(xpm_button->box);
    
  xpm_button->parent = parent;

 { 
   label_box = gtk_vbox_new(FALSE,0);
   gtk_widget_show(label_box);
   
   xpm_button->label = gtk_label_new (label_text);
   gtk_widget_show(xpm_button->label);
   gtk_box_pack_start (GTK_BOX (label_box), xpm_button->label,
		        TRUE,FALSE,0);
  
   if (strcmp("",additional_label_text)) {
     xpm_button->additional_label = gtk_label_new (additional_label_text);
     gtk_widget_show(xpm_button->additional_label);
     gtk_box_pack_start (GTK_BOX (label_box), xpm_button->additional_label,
			 TRUE,FALSE,0);
   }
 }
  xpm_button->pixmap_id = NULL;
  pmg_set_xpm_in_button_label(xpm_button, xpm_filename);
  gtk_widget_show(xpm_button->pixmap_id);


  gtk_box_pack_start (GTK_BOX (xpm_button->box), xpm_button->pixmap_id,
		      FALSE, FALSE, 3);
  gtk_box_pack_start (GTK_BOX (xpm_button->box), label_box,
		      FALSE, FALSE, 3);
  
  xpm_button->button = gtk_button_new();
  gtk_widget_show(xpm_button->button);

  gtk_signal_connect (GTK_OBJECT (xpm_button->button), "clicked",
                      (GtkSignalFunc) click_function,
                      click_data);
  
  gtk_container_add(GTK_CONTAINER(xpm_button->button),xpm_button->box);

  return xpm_button;
}

void
pmg_set_xpm_in_button_label(PmgButtonLabelXpm *xpm_button,
			    gchar             *xpm_filename)
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle  *style;

  style = gtk_widget_get_style(xpm_button->parent);
  pixmap = gdk_pixmap_create_from_xpm (xpm_button->parent->window, &mask,
				       &style->bg[GTK_STATE_NORMAL],
				       xpm_filename);
 if (!xpm_button->pixmap_id)
  xpm_button->pixmap_id = gtk_pixmap_new (pixmap, mask);
 else 
   gtk_pixmap_set(GTK_PIXMAP(xpm_button->pixmap_id), pixmap, mask);
}
