#include <gtk/gtk.h>
#include <strings.h>

#include "gas.h"
#include "gag.h"


#define	MAX_ROW	3
#define MAX_COL	3
#define MAX_I	MAX_ROW*MAX_COL

/* 
   these are temporary constants - once it will be possible
   to specify size of preview widget :-) 
*/


#define CLOSEUP_WIDTH   PREVIEW_WIDTH * MAX_COL
#define CLOSEUP_HEIGHT  PREVIEW_HEIGHT * MAX_ROW

#define CLOSEUP_UPDATE  (gint) (CLOSEUP_HEIGHT / 100)

#define REPAINT_PREVIEW 0
#define REPAINT_CLOSEUP 1

#define REPAINT_SCANLINE        0
#define REPAINT_MOSAIC          1
#define REPAINT_ADAPTIVE_MOSAIC 2

INDIVIDUAL 	population[MAX_I];

ENTRY_DIALOG  entry_data = { NULL, NULL, NULL };

GAG_UI ui = { NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, REPAINT_PREVIEW, REPAINT_SCANLINE, FALSE };


int gag_mosaic_sequence[]= {16,8,4,1,0};

/* options */
int	gag_mate_one_to_one=0;
int	gag_do_mating=1;
int	gag_do_mutating=1;

void gag_destroy(GtkWidget *, gpointer *);
void gag_mate(GtkWidget *widget, gpointer *data);
void gag_reset_scales(GtkWidget *widget, gpointer *data);

void gag_random(GtkWidget *widget, gpointer *data);
void gag_draw_picture(GtkWidget *widget, gpointer *data);
void gag_magnify_picture(GtkWidget *widget, gpointer *data);

void gag_print(GtkWidget *widget, gpointer *data);
void gag_mutate(GtkWidget *widget, gpointer *data);
void gag_previews(GtkWidget *widget, gpointer *data);
void gag_toggle_mosaic(GtkWidget *widget, gpointer *data);

void gag_edit_by_hand_button(GtkWidget *widget, gpointer  data);
void gag_edit_by_hand(GtkWidget *widget, gpointer  data);


/* 
      INITIALISATION
*/

void gag_create_menu (void)
{
  GtkWidget *menu;
  GtkWidget *menuitem;

  menu = gtk_menu_new ();
  
  menuitem= gtk_menu_item_new_with_label("Random");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
		      (GtkSignalFunc) gag_random, NULL);
  gtk_widget_show( menuitem );

  menuitem= gtk_menu_item_new_with_label("Mutate");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
		      (GtkSignalFunc) gag_mutate, NULL);
  gtk_widget_show( menuitem );

  menuitem= gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_widget_show( menuitem );

  menuitem= gtk_menu_item_new_with_label("Print to stdout");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
		      (GtkSignalFunc) gag_print, NULL);
  gtk_widget_show( menuitem );

  menuitem= gtk_menu_item_new_with_label("Edit by hand");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
		      (GtkSignalFunc) gag_edit_by_hand, NULL);
  gtk_widget_show( menuitem );

  menuitem= gtk_menu_item_new_with_label("Save to library");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
		      (GtkSignalFunc) gag_show_save_dialog, NULL);
  gtk_widget_show( menuitem );

  menuitem= gtk_menu_item_new_with_label("Load from library");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
		      (GtkSignalFunc) gag_library_show, NULL);
  gtk_widget_show( menuitem );

  menuitem= gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_widget_show( menuitem );

  if (gag_render_picture_ptr != NULL)
    {
      menuitem= gtk_menu_item_new_with_label("Render picture");
      gtk_menu_append(GTK_MENU(menu), menuitem);
      gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
			  (GtkSignalFunc) gag_render_picture_ptr, 
			  &(ui.popup_individual));
      gtk_widget_show( menuitem );
    }

  menuitem= gtk_menu_item_new_with_label("Magnify picture");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
		      (GtkSignalFunc) gag_magnify_picture, NULL);
  gtk_widget_show( menuitem );

  ui.popup_menu= menu;
}

int gag_show_menu(GtkWidget *preview, GdkEvent *event)
{
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      {
	GdkEventButton *bevent;

	bevent= (GdkEventButton *)event;
	if (bevent->button==3)
	  {
	    ui.popup_individual= gtk_object_get_data(GTK_OBJECT(preview), "individual");
	    gtk_menu_popup( GTK_MENU(ui.popup_menu),NULL, NULL,
			    NULL,NULL, 3, bevent->time);
	  }
	break;
      }
    default:
      break;
    }
  return (FALSE);
}

void gag_create_ui(void)
{
  GtkWidget     *top_hbox;

  GtkWidget	*preview;
  GtkObject	*adjustment;
  GtkWidget	*scale;

  GtkWidget     *alignment;
  GtkWidget	*vbox;
  GtkWidget	*frame;
  GtkWidget	*button;
  GtkWidget     *label;
  GtkWidget	*check;

  int		i,j;

  gag_create_menu();

  gtk_widget_push_visual( gtk_preview_get_visual ());
  gtk_widget_push_colormap (gtk_preview_get_cmap());

  ui.window= gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_signal_connect (GTK_OBJECT (ui.window), "destroy",
		      GTK_SIGNAL_FUNC (gag_destroy), NULL);
  gtk_window_set_policy(GTK_WINDOW(ui.window), FALSE, FALSE, TRUE);

  gtk_window_set_title( GTK_WINDOW(ui.window), "GAG");
  
  top_hbox = gtk_hbox_new(FALSE, 10);
  gtk_container_add(GTK_CONTAINER (ui.window), top_hbox);

  ui.notebook = gtk_notebook_new();
  gtk_notebook_set_show_tabs( GTK_NOTEBOOK (ui.notebook), FALSE);
  gtk_box_pack_start(GTK_BOX (top_hbox), ui.notebook, FALSE, FALSE, 0);

  /* Previews page */

  ui.previews_table = gtk_table_new( MAX_ROW*2, MAX_COL, FALSE );
  label = gtk_label_new("Previews");
  gtk_notebook_append_page(GTK_NOTEBOOK (ui.notebook), ui.previews_table, label);

  gtk_container_border_width (GTK_CONTAINER (top_hbox), 10);
  gtk_table_set_row_spacings (GTK_TABLE (ui.previews_table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (ui.previews_table), 2);

  for (i=0; i < MAX_COL; i++)
    for (j=0; j < MAX_ROW; j++)
      {
	frame= gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

	gtk_table_attach( GTK_TABLE(ui.previews_table), frame, 
			  j,j+1,
			  2*i, 2*i+1, 
			  GTK_FILL, GTK_FILL,
			  0,0);
	
	preview = gtk_preview_new( PREVIEW_MODE );
	gtk_preview_size( GTK_PREVIEW(preview), PREVIEW_WIDTH, PREVIEW_HEIGHT );
	gtk_widget_set_events( preview, GDK_BUTTON_PRESS_MASK);

	gtk_signal_connect( GTK_OBJECT(preview), "event",
			    (GtkSignalFunc) gag_show_menu,NULL);

	gtk_object_set_data( GTK_OBJECT(preview),"individual",
			     &(population[i*MAX_ROW+j]));

	gtk_container_add(GTK_CONTAINER(frame),preview);

	adjustment= gtk_adjustment_new(0.0, 0.0, 10.0,
				       0.1, 1.0, 1.0);
	scale= gtk_hscale_new( GTK_ADJUSTMENT(adjustment));

	gtk_scale_set_digits(GTK_SCALE(scale),2);
	gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
	gtk_range_set_update_policy(GTK_RANGE(scale), 
				    GTK_UPDATE_DELAYED);

	gtk_table_attach( GTK_TABLE(ui.previews_table), scale,
			  j, j+1,
			  2*i+1, 2*i+2,
			  GTK_FILL, GTK_FILL,
			  0,0);

	population[i*MAX_ROW+j].preview= preview;
	population[i*MAX_ROW+j].adjustment= GTK_ADJUSTMENT(adjustment);
	population[i*MAX_ROW+j].expression=NULL;

	gtk_widget_show( preview );
	gtk_widget_show( scale );
	gtk_widget_show( frame );
      }

  /* Close Up Page */

  alignment = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_widget_show( alignment );  

  label = gtk_label_new("Close Up");
  gtk_notebook_append_page(GTK_NOTEBOOK (ui.notebook), alignment, label);

  vbox = gtk_vbox_new(FALSE, 5);
  gtk_widget_show( vbox );

  gtk_container_add( GTK_CONTAINER (alignment), vbox);

  alignment = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_widget_show( alignment );

  gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_show( frame );

  gtk_container_add( GTK_CONTAINER (alignment), frame);

  ui.closeup_preview = gtk_preview_new( PREVIEW_MODE );
  gtk_preview_size( GTK_PREVIEW (ui.closeup_preview),
		    CLOSEUP_WIDTH, CLOSEUP_HEIGHT);
  gtk_widget_set_events( ui.closeup_preview, GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect( GTK_OBJECT(ui.closeup_preview), "event",
		      (GtkSignalFunc) gag_show_menu, NULL);
  gtk_widget_show( ui.closeup_preview );

  gtk_container_add( GTK_CONTAINER(frame), ui.closeup_preview);

  alignment = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_widget_show( alignment );

  gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);

  button = gtk_button_new_with_label("Finished");
  gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
			     (GtkSignalFunc) gag_previews, NULL);
  gtk_widget_show( button );

  gtk_container_add( GTK_CONTAINER(alignment), button);

  /* Action buttons */

  vbox= gtk_vbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(top_hbox), vbox, FALSE, FALSE, 0);

  button= gtk_button_new_with_label("New generation");
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE,  0);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			    (GtkSignalFunc) gag_mate,NULL);

  gtk_widget_show(button);

  button= gtk_button_new_with_label("Reset scales");
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE,  0);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			    (GtkSignalFunc) gag_reset_scales,NULL);

  gtk_widget_show(button);

  button= gtk_button_new_with_label("Options");
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE,  0);
  gtk_widget_show(button);

  button= gtk_button_new_with_label("Help");
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE,  0);
  gtk_widget_show(button);

  check = gtk_check_button_new_with_label("Mosaic");
  gtk_box_pack_start(GTK_BOX(vbox), check, FALSE, FALSE,  0);
  gtk_signal_connect_object(GTK_OBJECT(check), "toggled",
			    (GtkSignalFunc) gag_toggle_mosaic, NULL);
  gtk_widget_show(check);

  button= gtk_button_new_with_label("Close");
  gtk_box_pack_end(GTK_BOX(vbox), button, FALSE, FALSE,  0);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			    (GtkSignalFunc) gag_destroy,NULL);

  ui.progress_bar= gtk_progress_bar_new();
  gtk_widget_set_usize(ui.progress_bar, 80, 20);
  gtk_box_pack_end(GTK_BOX(vbox), ui.progress_bar, 
			FALSE, FALSE, 0);

  gtk_widget_show( ui.progress_bar );
			    
  gtk_widget_show(button);
  gtk_widget_show(vbox);

  gtk_widget_show(ui.previews_table);
  gtk_widget_show(ui.notebook);
  gtk_widget_show(top_hbox);
  gtk_widget_show(ui.window);

  ui.preview_buffer = g_new(guchar, (CLOSEUP_WIDTH + 20)* PREVIEW_BPP);
  /* I allocate space for another 20 pixels - because of mosaic preview... 
     (closeup width is not multiple of 16) */
}

/*
       USEFULL Functions
*/

void gag_repaint_ind( GtkWidget *progress, INDIVIDUAL *i, int local)
{
  NODE		*n;
  GtkWidget 	*preview;

  guchar 	*buffer;
  int		xx,yy,pp;
  DBL		x,y,dx,dy;  
  int 		width, height, update;
  DBL 		result[4];
  int 		maxm, *mm;
  guchar 	c[PREVIEW_BPP];

  buffer = ui.preview_buffer;

  width = PREVIEW_WIDTH;
  height = PREVIEW_HEIGHT;
  update = PREVIEW_UPDATE;
  preview= i->preview;

  if (!local)
    if (ui.repaint_mode == REPAINT_CLOSEUP)
      {
	width = CLOSEUP_WIDTH;
	height = CLOSEUP_HEIGHT;
	update = CLOSEUP_UPDATE;
	preview= ui.closeup_preview;
      }

  if (progress==NULL)
    progress= ui.progress_bar;

  if ((n= i->expression)== NULL)
    {
      memset(buffer,128,width*PREVIEW_BPP);
      for (yy=0; yy < height; yy++)
	gtk_preview_draw_row( GTK_PREVIEW(preview),
			      &(buffer[0 + 0]),0,yy,width);
      return;
    }

  prepare_node( n );

  if (ui.repaint_quality == 0) /* the best quality */
    {
      
      dx= 2.0 / (DBL) width;
      dy= 2.0 / (DBL) height;

      y= 1.0;
      for ( yy=0; yy < height; yy++, y-=dy)
	{
	  for (xx=0, x=-1.0; xx < width * PREVIEW_BPP; x+=dx)
	    {
		  eval_xy(result, x,y );
		  

		  for (pp = 0; pp < PREVIEW_BPP; pp++)
		  {
		     buffer[xx++] = wrap_func(result[pp]);
		  }
	    }
	  gtk_preview_draw_row( GTK_PREVIEW(preview), 
				buffer, 0, yy, width);

	  if ((yy & update) == 0)
	    {
	       if ((yy & update * 10) == 0)
		  gtk_widget_draw( preview, NULL);

	       gtk_progress_bar_update(GTK_PROGRESS_BAR(progress),
				       (1.0-y)/2.0);
	    }
	}
    }
   else if (ui.repaint_quality == 1)
   {
      /* mosaic preview */

      gtk_progress_bar_update(GTK_PROGRESS_BAR(progress), 0.0);

      maxm = MAX(width, height);

      for ( mm = gag_mosaic_sequence; *mm > 0; mm++ )
      {
	 dx = (DBL) (*mm) * (2.0 / (DBL) width);
	 dy = (DBL) (*mm) * (2.0 / (DBL) height);
	 y = 1.0;

	 for ( yy = 0; yy < height; y -= dy)
	 {
	    for (xx = 0, x = -1.0; xx < width * PREVIEW_BPP; x += dx)
	    {
	       eval_xy(result, x,y );
	       
	       for (pp = 0; pp < PREVIEW_BPP; pp++)
		  c[pp] = wrap_func(result[pp]);

	       for (pp = 0; pp < PREVIEW_BPP * (*mm); pp++)
		  buffer[xx++] = c[pp % PREVIEW_BPP];	       
	    }

	    for (pp = 0; pp < (*mm); pp++)
	       gtk_preview_draw_row( GTK_PREVIEW(preview), 
				     buffer, 0, yy++, width);

	    if ((yy & update) == 0)
	    {
	       if ((yy & update * 10) == 0)
		  gtk_widget_draw( preview, NULL);

	       gtk_progress_bar_update(GTK_PROGRESS_BAR(progress),
				       (1.0 - y) / 2.0);
	    }
	 }
      }
      gtk_widget_draw( preview, NULL);
   }
  else
    {
      printf("Error: not supported quality...\n");
    }
  gtk_widget_draw( preview, NULL);
  gtk_progress_bar_update(GTK_PROGRESS_BAR(progress), 0.0);
}

/*
       CALLBACKS
*/

void gag_destroy (GtkWidget *widget, gpointer *data)
{
  DPRINT("Saving expression library...");
  gag_save_library(gag_library_file);
  gtk_main_quit ();
}

void gag_print(GtkWidget *widget, gpointer *data)
{
  if (ui.popup_individual->expression != NULL)
    expr_fprint( stdout,  0, ui.popup_individual->expression ); 
}

void gag_mutate(GtkWidget *widget, gpointer *data)
{
  NODE *n;
  
  if ((n= ui.popup_individual->expression)==NULL) return;
  
  ui.popup_individual->expression= gas_mutate(n,n,1);
  gag_repaint_ind( ui.progress_bar, ui.popup_individual, 0);
  ui.repaint_preview = TRUE;
}

void gag_random(GtkWidget *widget, gpointer *data)
{
  destroy_node( ui.popup_individual->expression );
  init_random();
  ui.popup_individual->expression= gas_random( 1, expression_max_depth );

  /*  expr_print( 0, ui.popup_individual->expression ); */
  gag_repaint_ind( NULL, ui.popup_individual, 0);
  ui.repaint_preview = TRUE;
}

int gag_choose_individual()
{
  DBL m,p;
  int i;

  m= 0.0;
  for (i=0; i < MAX_I; i++)
    if (population[i].expression != NULL)
      m+= population[i].adjustment->value;

  if (m==0.0) 
    return -1;

  p= m * dblrand();
  m= 0.0;
  for (i=0; i < MAX_I; i++)
    {
      if (population[i].expression != NULL)
	m+= population[i].adjustment->value;
      if (p < m)
	break;
    }
  return i;
}

/******************************

  M A T E

******************************/
void gag_mate(GtkWidget *widget, gpointer *data)
{
  int 	i,j,k;
  NODE	*(n[MAX_I]), *tmp;
  DBL	w;

  for (i=0; i < MAX_I; i++) n[i]=NULL;

  if (gag_do_mating)  
    for (i=0; i < MAX_I; i++)
      {
	
	j= gag_choose_individual();
	k= gag_choose_individual();

	if (j < 0) break;

	if (gag_mate_one_to_one)
	  w=0.5;
	else
	  w= population[j].adjustment->value / 
	    ( population[j].adjustment->value +
	      population[k].adjustment->value);

	DMPRINT("%d is child of daddy %d and mummy %d\n",i,j,k);
	n[i]= gas_mate(w, population[j].expression,
		       population[k].expression);
      }
  
  for (i=0; i < MAX_I; i++)
    {
      if (n[i]!=NULL)
	{
	  destroy_node( population[i].expression );
	  population[i].expression= n[i];
	}

      tmp= population[i].expression;
      if ((tmp != NULL)&&(gag_do_mutating))
	population[i].expression= gas_mutate( tmp,tmp,0);
    }
  for (i=0; i < MAX_I; i++)
    gag_repaint_ind( NULL, &(population[i]), 0);
}

/******************************

  Edit by hand

******************************/

void gag_edit_by_hand_button(GtkWidget *widget, gpointer data)
{
  NODE 	*n;
  char	tmp[20000];
  char 	*ptr;

  if (GTK_WIDGET_VISIBLE (entry_data.dialog))
    gtk_widget_hide (entry_data.dialog);

  strcpy(tmp, GTK_ENTRY(entry_data.entry)->text);

  ptr= tmp;
  n= parse_prefix_expression(&ptr);
  if (n!=NULL)
    {
      destroy_node(entry_data.ind->expression);
      entry_data.ind->expression= n;
      gag_repaint_ind(NULL, entry_data.ind, 0);
      ui.repaint_preview = TRUE;
    }
}

void gag_edit_by_hand(GtkWidget *widget, gpointer data)
{
  if (entry_data.dialog==NULL)
    gag_create_entry( &entry_data, "Enter expression", 
		     GTK_SIGNAL_FUNC(gag_edit_by_hand_button));

  if (!(GTK_WIDGET_VISIBLE(entry_data.dialog)))
    {
      char	temp[20000]="(x)";	

      if (ui.popup_individual->expression != NULL)
	expr_sprint(temp, ui.popup_individual->expression);
      gtk_entry_set_text(GTK_ENTRY(entry_data.entry), temp);
			 
      entry_data.ind= ui.popup_individual;
      gtk_widget_show(entry_data.dialog);
    }
}

void gag_load_from_lib(GtkWidget *widget, gpointer *data)
{
  printf("load: %d\n", ui.popup_individual - population);
}

void gag_magnify_picture(GtkWidget *widget, gpointer *data)
{
   NODE *n;
  
   printf("magnify: %d\n", ui.popup_individual - population);
   
   if ((n = ui.popup_individual->expression) == NULL) return;
      
   gtk_notebook_set_page( GTK_NOTEBOOK (ui.notebook), 1);
   
   ui.repaint_mode = REPAINT_CLOSEUP;
   ui.closeup_individual = ui.popup_individual;
   
   gtk_object_set_data( GTK_OBJECT(ui.closeup_preview),
			"individual", ui.closeup_individual);
   
   gag_repaint_ind(ui.progress_bar, ui.closeup_individual, 0);

   ui.repaint_preview = FALSE;
}

void gag_previews(GtkWidget *widget, gpointer *data)
{
   gtk_notebook_set_page( GTK_NOTEBOOK (ui.notebook), 0);

   ui.repaint_mode = REPAINT_PREVIEW;

   if (ui.repaint_preview == TRUE)
   {
      gag_repaint_ind(NULL, ui.closeup_individual, 0);
      ui.repaint_preview = FALSE;
   }
}

void gag_reset_scales(GtkWidget *widget, gpointer *data)
{
  int i;
  for (i=0; i<MAX_I; i++)
    {
      population[i].adjustment->value= 0.0;
      gtk_signal_emit_by_name(
        GTK_OBJECT(population[i].adjustment), "value_changed");
    }
}

void gag_toggle_mosaic(GtkWidget *widget, gpointer *data)
{
   if (ui.repaint_quality == REPAINT_MOSAIC)
      ui.repaint_quality = REPAINT_SCANLINE;
   else ui.repaint_quality = REPAINT_MOSAIC;
}
