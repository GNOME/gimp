#ifndef _GAG_H_
#define _GAG_H_

#include "expressions.h"
#include "gtk/gtk.h"

typedef struct individual_struct INDIVIDUAL;

#define PREVIEW_WIDTH	100
#define PREVIEW_HEIGHT	100

#define PREVIEW_UPDATE  (gint) (PREVIEW_HEIGHT / 100)

/*
#define PREVIEW_BPP  1
#define PREVIEW_MODE GTK_PREVIEW_GRAYSCALE
*/
#define PREVIEW_BPP  3
#define PREVIEW_MODE GTK_PREVIEW_COLOR

struct individual_struct {
  GtkAdjustment *adjustment;
  GtkWidget	*preview;
  NODE  	*expression;
};
extern INDIVIDUAL *gag_popup_ind;

typedef struct entry_dialog_struct ENTRY_DIALOG;
struct entry_dialog_struct {
  GtkWidget	*dialog;
  GtkWidget	*entry;
  INDIVIDUAL	*ind;
};

typedef struct {
  GtkWidget  *window;
  GtkWidget  *notebook;
  GtkWidget	 *previews_table;
  GtkWidget  *closeup_preview;
  GtkWidget  *popup_menu;
  GtkWidget  *progress_bar;

  INDIVIDUAL *popup_individual;
  INDIVIDUAL *closeup_individual;

  guchar     *preview_buffer;
  gint        repaint_mode;
  gint        repaint_quality;
  gint        repaint_preview;
} GAG_UI;

extern GAG_UI ui;

void gag_show_save_dialog(GtkWidget *, gpointer );

void gag_create_entry( ENTRY_DIALOG *, char *, GtkSignalFunc );
void gag_library_show(GtkWidget *, gpointer);
void gag_repaint_ind( GtkWidget *, INDIVIDUAL *, int);

void gag_load_library(char *);
void gag_save_library(char *);

/******************************

   Common things for plug-in 
    and standalone program

******************************/

extern char *gag_library_file;
extern void (*gag_render_picture_ptr)(GtkWidget *, gpointer);

void gag_create_ui(void);

#endif



