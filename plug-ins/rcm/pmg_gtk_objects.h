#ifndef __PMG_GTK_OBJECTS__
#define __PMG_GTK_OBJECTS__

#include <stdio.h>
#include <stdlib.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

typedef struct _PmgButtonLabel PmgButtonLabel;
typedef struct _PmgButtonLabelXpm PmgButtonLabelXpm;

struct _PmgButtonLabel {
  GtkWidget *button;
  GtkWidget *label;
};

struct _PmgButtonLabelXpm {
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *additional_label;
  GtkWidget *box;
  GtkWidget *parent;
  GtkWidget *pixmap_id;
};

PmgButtonLabel *pmg_button_with_label_new();

PmgButtonLabelXpm *
pmg_button_label_xpm_new(GtkWidget      *parent,
			 gchar          *xpm_filename, 
			 gchar          *label_text,
			 gchar          *additional_label_text,
			 gint           v_or_h,
			 GtkSignalFunc  *click_function,
			 gpointer       *click_data);

void
pmg_set_xpm_in_button_label(PmgButtonLabelXpm *xpm_button,
			    gchar             *xpm_filename);

  
#endif
