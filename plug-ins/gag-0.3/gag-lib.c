#include <strings.h>
#include <stdio.h>

#include "gag.h"

#define LIB_DATA	"lib_data"


typedef struct library_entry_struct  LENTRY;
struct library_entry_struct {
  NODE	  	*expression;
  GtkWidget	*list_item;
  GtkWidget	*label;

  char	  	name [30];
  LENTRY  	*Next;
};
static LENTRY	*Library= NULL;
static LENTRY	*CurrentLentry=NULL;

static ENTRY_DIALOG SaveWindow= {NULL, NULL, NULL};
static ENTRY_DIALOG RenameWindow= {NULL, NULL, NULL};

static int	    save_counter=1;

static struct {
  GtkWidget	*window;
  GtkWidget	*list;
  GtkWidget	*popup_menu;
  GtkWidget	*preview;
  GtkWidget	*progress;
  INDIVIDUAL	*ind;
} LibraryWin= {NULL, NULL, NULL, NULL, NULL, NULL};

void gag_destroy_window(GtkWidget *, gpointer);
void gag_cancel_button(GtkWidget *, gpointer);
void gag_hide_window(GtkWidget *widget, gpointer  data);

void gag_add_lentry_2_list( LENTRY *n )
{
  GtkWidget *label;
  GtkWidget *list_item;

  if (LibraryWin.window == NULL) return;

  label= gtk_label_new(n->name);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  list_item= gtk_list_item_new( );
  gtk_container_add(GTK_CONTAINER(list_item),label);
  gtk_container_add(GTK_CONTAINER(LibraryWin.list),
		    list_item);
  gtk_object_set_data( GTK_OBJECT(list_item), LIB_DATA, n);
  n->list_item= list_item;
  n->label= label;
  gtk_widget_show(label);
  gtk_widget_show(list_item);
}

void SaveWindow_callback(GtkWidget *widget, gpointer data)
{
  LENTRY    **ptr= &Library, *n;

  if (SaveWindow.ind->expression == NULL)
    return;

  if (GTK_WIDGET_VISIBLE( SaveWindow.dialog ))
    gtk_widget_hide( SaveWindow.dialog);

  n= (LENTRY *) malloc(sizeof(LENTRY));
  n-> expression= node_copy( SaveWindow.ind->expression );
  strncpy(n->name,GTK_ENTRY( SaveWindow.entry )->text, 29);
  n-> Next= NULL;
    
  while (*ptr != NULL) ptr= &( (*ptr)->Next );
  *ptr= n;

  gag_add_lentry_2_list( n );
}

void gag_show_save_dialog(GtkWidget *w, gpointer data)
{
  if (SaveWindow.dialog == NULL)
    gag_create_entry(&SaveWindow, "Enter name of expression",
		     GTK_SIGNAL_FUNC(SaveWindow_callback));

  if (!(GTK_WIDGET_VISIBLE(SaveWindow.dialog)))
    {
      char buff[20];
      
      sprintf(buff,"Noname%0d",save_counter++);
      gtk_entry_set_text(GTK_ENTRY(SaveWindow.entry),buff);
      SaveWindow.ind= ui.popup_individual;
      gtk_widget_show(SaveWindow.dialog);
    }
}

/******************************

   Main Library window

******************************/
void gag_lib_Copy( GtkWidget *widget, gpointer data)
{
}

void gag_lib_Delete( GtkWidget *widget, gpointer data)
{
  LENTRY **ptr= &Library, *n;

  while (*ptr != NULL)
    {
      if (*ptr == CurrentLentry) break;
      ptr= &( (*ptr)->Next ); 
    }
  if (ptr != NULL)
    {
      GList	*tmp;
      n= *ptr;

      *ptr= n->Next;

      tmp->data= n->list_item;
      tmp->prev= tmp->next= NULL;
      gtk_list_remove_items (GTK_LIST (LibraryWin.list),tmp);

      gtk_widget_destroy(n->list_item);
      free( n );
    }
}

static void RenameWindow_callback( GtkWidget *widget, gpointer *data)
{
  LENTRY *lptr;

  if (GTK_WIDGET_VISIBLE( RenameWindow.dialog ))
    gtk_widget_hide( RenameWindow.dialog);

  lptr= (LENTRY *) (RenameWindow.ind);
  strncpy(lptr->name, GTK_ENTRY( RenameWindow.entry )->text, 29);
  gtk_label_set(GTK_LABEL(lptr->label), lptr->name);
  gtk_widget_draw(lptr->label, NULL);
}

void gag_lib_Rename( GtkWidget *widget, gpointer data)
{
  if (RenameWindow.dialog == NULL)
    gag_create_entry(&RenameWindow, "Enter name of expression",
		     GTK_SIGNAL_FUNC(RenameWindow_callback));

  if (!(GTK_WIDGET_VISIBLE(RenameWindow.dialog)))
    {
      gtk_entry_set_text(GTK_ENTRY(RenameWindow.entry),CurrentLentry->name);
      RenameWindow.ind= (INDIVIDUAL *) CurrentLentry;
      gtk_widget_show(RenameWindow.dialog);
    }
}

void gag_lib_Evoke( GtkWidget *widget, gpointer data)
{
  destroy_node(LibraryWin.ind->expression);
  LibraryWin.ind->expression= node_copy( CurrentLentry->expression );
  gag_repaint_ind( NULL, LibraryWin.ind, 0);

  ui.repaint_preview = TRUE;
}

void gag_lib_render_pic( GtkWidget *widget, gpointer data)
{
  INDIVIDUAL tmp, *tmpptr;

  tmpptr= &tmp;
  tmp.expression=  CurrentLentry->expression;
  gag_render_picture_ptr(NULL, &tmpptr);
}

static void gag_Create_FS(GtkWidget **window, char *label,
			  void *ok_cb)
{
  *window = gtk_file_selection_new ("file selection dialog");
  gtk_window_position (GTK_WINDOW (*window), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (*window), "destroy",
		      (GtkSignalFunc)gag_destroy_window,
		      window);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (*window)->ok_button),
		      "clicked", (GtkSignalFunc ) ok_cb,
		      *window);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (*window)->ok_button),
		      "clicked", (GtkSignalFunc ) gag_hide_window,
		      *window);

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (*window)->cancel_button),
			     "clicked", (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (*window));
}


static void gag_lib_load_cb(GtkWidget *widget, GtkWidget *fs)
{
  DPRINT("Loading Library");
  gag_load_library(
     gtk_file_selection_get_filename(GTK_FILE_SELECTION (fs))
  );
}

static void gag_lib_save_cb(GtkWidget *widget, GtkWidget *fs)
{
  gag_save_library(
      gtk_file_selection_get_filename(GTK_FILE_SELECTION (fs))
  );
}

static void gag_lib_Load( GtkWidget *widget, gpointer data)
{
  static GtkWidget *LoadWin= NULL;

  if (LoadWin==NULL)
    gag_Create_FS(&LoadWin, "Load Library", (void *)gag_lib_load_cb);

  if (!(GTK_WIDGET_VISIBLE(LoadWin)))
      gtk_widget_show(LoadWin);
}

void gag_lib_Save( GtkWidget *widget, gpointer data)
{
  static GtkWidget *SaveWin= NULL;

  if (SaveWin==NULL)
    gag_Create_FS(&SaveWin, "Save Library as", (void *)gag_lib_save_cb);

  if (!(GTK_WIDGET_VISIBLE(SaveWin)))
      gtk_widget_show(SaveWin);
}


int gag_lib_popup (GtkWidget *widget, GdkEventButton *event,
		   gpointer        user_data)
{
  GtkWidget *event_widget;        
  GdkEventButton *bevent;

  event_widget = gtk_get_event_widget ((GdkEvent*) event);  



  bevent= (GdkEventButton *)event;

  switch (event->type)
    {
    case GDK_2BUTTON_PRESS:
      switch (bevent->button)
	{
	case 1:
	  {
	    INDIVIDUAL 	i;
	    LENTRY	*lptr;

	    lptr= gtk_object_get_data( GTK_OBJECT(event_widget),
				       LIB_DATA);

	    i.preview= LibraryWin.preview;
	    i.expression= lptr->expression;

	    gag_repaint_ind( LibraryWin.progress, &i,1);
	    break;
	  }
	default:
	  break;
	}
      break;

    case GDK_BUTTON_PRESS:
      if (bevent->button==3)
	{
	  CurrentLentry= 
	    gtk_object_get_data( GTK_OBJECT(event_widget),
				 LIB_DATA);
	  gtk_menu_popup( GTK_MENU(LibraryWin.popup_menu),NULL, NULL,
			  NULL,NULL, 3, bevent->time);

	}
      break;
    default:
      break;
    }
  return FALSE;
}

void gag_library_show(GtkWidget *widget, gpointer data)
{
  if (LibraryWin.popup_menu == NULL)
    {
      GtkMenu *menu;
      GtkWidget	*menuitem;

      menu= GTK_MENU( gtk_menu_new () );
      
      /* for now this seems needless

	 menuitem= gtk_menu_item_new_with_label("Copy");
	 gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
	 (GtkSignalFunc) gag_lib_Copy, NULL);
	 gtk_menu_append(menu, menuitem);
	 gtk_widget_show( menuitem );
       */

      menuitem= gtk_menu_item_new_with_label("Evoke");      
      gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
			  (GtkSignalFunc) gag_lib_Evoke, NULL);
      gtk_menu_append(menu, menuitem);
      gtk_widget_show( menuitem );

      if (gag_render_picture_ptr != NULL)
	{
	  menuitem= gtk_menu_item_new_with_label("Render picture");
	  gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
			      (GtkSignalFunc) gag_lib_render_pic, 
			      NULL);
	  gtk_menu_append(menu, menuitem);
	  gtk_widget_show( menuitem );
	}

      menuitem= gtk_menu_item_new();
      gtk_menu_append( menu, menuitem);
      gtk_widget_show( menuitem );

      menuitem= gtk_menu_item_new_with_label("Rename");      
      gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
			  (GtkSignalFunc) gag_lib_Rename, NULL);
      gtk_menu_append(menu, menuitem);
      gtk_widget_show( menuitem );

      menuitem= gtk_menu_item_new_with_label("Delete");
      gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
			  (GtkSignalFunc) gag_lib_Delete, NULL);
      gtk_menu_append(menu, menuitem);
      gtk_widget_show( menuitem );

      menuitem= gtk_menu_item_new();
      gtk_menu_append( menu, menuitem);
      gtk_widget_show( menuitem );

      menuitem= gtk_menu_item_new_with_label("Load from file"); 
      gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
			  (GtkSignalFunc) gag_lib_Load, NULL);
      gtk_menu_append(menu, menuitem);
      gtk_widget_show( menuitem );
      
      menuitem= gtk_menu_item_new_with_label("Save to file");
      gtk_signal_connect( GTK_OBJECT(menuitem), "activate",
			  (GtkSignalFunc) gag_lib_Save, NULL);
      gtk_menu_append(menu, menuitem);
      gtk_widget_show( menuitem );

      LibraryWin.popup_menu= GTK_WIDGET(menu);
    }

  if (LibraryWin.window == NULL)
    {
      GtkWidget *window;
      GtkWidget	*hbox, *vbox;
      GtkWidget	*list;
      GtkWidget	*preview;
      GtkWidget	*scwin;
      GtkWidget	*frame;
      GtkWidget	*button;
      GtkWidget	*progress;

      LENTRY	*lptr;

      gtk_widget_push_visual( gtk_preview_get_visual ());
      gtk_widget_push_colormap (gtk_preview_get_cmap());

      window= gtk_window_new( GTK_WINDOW_TOPLEVEL );
      gtk_signal_connect( GTK_OBJECT(window), "destroy",
			  (GtkSignalFunc) gag_destroy_window,
			  &(LibraryWin.window));
      gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, TRUE);
      gtk_window_set_title( GTK_WINDOW(window),"GAG Library");
      LibraryWin.window= window;

      hbox= gtk_hbox_new(FALSE, 10);
      gtk_container_border_width( GTK_CONTAINER(hbox),10);
      gtk_container_add(GTK_CONTAINER(window), hbox);

      scwin= gtk_scrolled_window_new( NULL, NULL );
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scwin),
				     GTK_POLICY_AUTOMATIC,
				     GTK_POLICY_AUTOMATIC);

      gtk_widget_set_usize( scwin, 300, 150);
      gtk_box_pack_start( GTK_BOX(hbox), scwin, TRUE, TRUE, TRUE);

      LibraryWin.list= list= gtk_list_new();
      gtk_list_set_selection_mode (GTK_LIST (list),	
				   GTK_SELECTION_BROWSE);
      gtk_container_add( GTK_CONTAINER(scwin), list);
      gtk_signal_connect(GTK_OBJECT( list ), "button_press_event",
			 (GtkSignalFunc)gag_lib_popup, NULL);
      gtk_widget_show( list );
      gtk_widget_show(scwin);


      vbox= gtk_vbox_new(FALSE, 5);
      gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE,0);

      frame= gtk_frame_new(NULL);
      gtk_frame_set_shadow_type(GTK_FRAME(frame),
				GTK_SHADOW_IN);
      gtk_box_pack_start( GTK_BOX(vbox), frame,
			  FALSE, FALSE, FALSE);      

      LibraryWin.preview= preview= gtk_preview_new( PREVIEW_MODE );
      gtk_preview_size( GTK_PREVIEW(preview), 
			PREVIEW_WIDTH, PREVIEW_HEIGHT);
      gtk_container_add( GTK_CONTAINER(frame), preview );
      gtk_widget_show( preview );
      gtk_widget_show( frame );

      button= gtk_button_new_with_label("Close");
      gtk_box_pack_end(GTK_BOX(vbox), button, FALSE, FALSE,  0);
      gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
				(GtkSignalFunc) gag_cancel_button,(gpointer)window);
      gtk_widget_show(button);

      progress= gtk_progress_bar_new();
      gtk_widget_set_usize(progress, 80, 20);
      gtk_box_pack_end(GTK_BOX(vbox), progress, 
		       FALSE, FALSE, 0);
      LibraryWin.progress=progress;
      gtk_widget_show( progress );
      
      gtk_widget_show( vbox );
      for (lptr=Library; lptr != NULL; lptr= lptr->Next)
	gag_add_lentry_2_list( lptr );
	  
      gtk_widget_show( hbox );
    }

  if (!(GTK_WIDGET_VISIBLE(LibraryWin.window)))
    {	
      gtk_widget_show(LibraryWin.window);
    }
  LibraryWin.ind= ui.popup_individual;
}

/******************************

  GAG LIB LOAD/SAVE functions

******************************/

void translate_name2esc(char *s, char *d)
{
  while ( (*d=*s) != '\000')
    {
      if (*d == '\\'){          d++; *d='\\'; }
      if (*d == '"') { *d='\\'; d++; *d='"'; }
      d++;
      s++;
    }
}

void gag_save_library(char *file)
{
  FILE 		*f;
  LENTRY	*lptr;
  char 		buff[100];

  lptr= Library;
  
  f= fopen( file, "wt");

  fprintf(f,";;; GAG Library file\n\n");

  while  (lptr != NULL)
    {
      translate_name2esc(lptr->name, buff);
      fprintf(f, "(insert-to-library \"%s\" \n", buff);
      expr_fprint(f, 2, lptr->expression);
      fprintf(f, ") ;;; end of function\n\n");
      lptr= lptr->Next;
    }
  fclose(f);
}

static void string_convert(char *line, char *pline, char *name)
{
  int 		state=0;  /* 0 - ok, 1 - inside string */
  guchar	prevchar=0;

  while ((*pline= *line) != 0)
    {
      if ((state == 0)&&(*line==';'))
	{ *pline = '\000'; return; }

      pline++;

      if ((state==0)&&(*line=='"'))
	{ state=1; line++; continue; }

      if ((state==1)&&(prevchar!='\\')&&(*line=='"'))
	{ state=0; *name= '\000'; line++; continue; }

      if (state==1)
	{
	  switch (prevchar)
	    {
	    case '\\':
	      switch (*line)
		{
		case '"': case '\\':
		  *(name-1)= *line;
		  break;
		default:
		  printf("Warning: Unknown prefix");
		  break;
		}
	      prevchar='\000';
	      break;
	    default:
	      *name= *line; ++name; break;
	    }
	}
      prevchar= *line;
      line++;
    }

  if (state!=0)
    {
      printf("Error: Unterminated string constant: %s\n",line);
    }
}

static void readln(FILE *f, guchar *str, int max)
{
  int z;

  do {
    if ((z= fgetc(f))==EOF) break;

    if ((z == '\n') || (z=='\r') || (z=='\f') || (z=='\v'))
      break;
    else
      *str= (guchar) z;
    str++;
    max--;
  } while (max);
  *str='\000';
}

void gag_load_library(char *file)
{
  FILE		*f;
  char 		*buff, *s;
  
  char		line[500];
  char		smbuff[500];
  char		name[30], tmp[100];
  int		state=0;           /* 0 - waiting, 1 - recieving */
  int		ll;

  NODE	 *n;
  LENTRY *v, **head;

  DPRINT("Loading library...");

  head= &Library;
  while (*head != NULL) head= &((*head)->Next);

  if ((f= fopen(file, "rt"))==NULL) return;

  buff= malloc(200*1000);        /* oh - we are lazy programmers !!!) */
  
  while (! (feof(f)))
    {
      readln(f, (guchar *) line, 499);

      string_convert(line, smbuff, tmp);
      if (smbuff[0]=='\000') continue;

      if (state == 0)
	{
	  if (strncmp(smbuff,"(insert-to-library ",18 )==0)
	    {
	      strncpy(name, tmp, 30);
	      state=1;

	      s= buff;
	      memset(buff, ' ', 200*1000);
	      
	      continue;
	    }
	  printf("Error: unexpected line %s\n", line);
	  continue;
	}
      else
	{
	  if (strncmp(line, ") ;;; end of function",20)==0)
	      {
		s= buff;
		n= parse_prefix_expression(&s);
		if (n==NULL) goto end_of_loading;
		v= (LENTRY *) malloc(sizeof(LENTRY));
		
		v->expression= n;
		v->Next=NULL;
		v->list_item=NULL;

		strncpy(v->name, name, 30);
		gag_add_lentry_2_list( v );
		
		*head= v;
		head= & (v->Next);
		
		state=0;
		continue;
	      }
	  *(s++)= ' ';
	  ll= strlen (smbuff);
	  strcpy(s, smbuff);
	  s+= ll;
	}
    } /* end of while */

  if (state == 1)
    {
      printf("Error: corrupted library\n");
    }

end_of_loading:
  if (f!= NULL) fclose( f);
  if (buff != NULL) free (buff);
}

/******************************

   MISC FUNCTIONS

******************************/

void gag_hide_window(GtkWidget *widget, gpointer  data)
{
  GtkWidget	*tmp;

  tmp= (GtkWidget *) data;
  if (GTK_WIDGET_VISIBLE(tmp))
    gtk_widget_hide(tmp);
}

void gag_destroy_window(GtkWidget *widget, gpointer  data)
{
  GtkWidget	**tmp;

  tmp= (GtkWidget **) data;
  *tmp= NULL;
}

void gag_cancel_button(GtkWidget *widget, gpointer data)
{
  if (GTK_WIDGET_VISIBLE (GTK_WIDGET(data)))
    gtk_widget_hide (GTK_WIDGET(data));
}

void gag_clear_entry(GtkWidget *widget, GtkEntry *entry)
{
  gtk_entry_set_text(entry, "");
}

void gag_create_entry( ENTRY_DIALOG *ed, char *title, GtkSignalFunc OkFunc )
{
  GtkWidget 	*dialog;
  GtkWidget	*button;
  GtkWidget	*entry;
  GtkWidget	*vb;

  dialog= gtk_dialog_new();
  ed->dialog= dialog;
  gtk_signal_connect( GTK_OBJECT(dialog), "destroy",
		      (GtkSignalFunc)gag_destroy_window,
		      &(ed->dialog));
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width (GTK_CONTAINER (dialog), 0);
  gtk_window_set_title(GTK_WINDOW(dialog), title);

  vb= gtk_vbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), vb,
		     TRUE, TRUE,0);
  gtk_container_border_width( GTK_CONTAINER(vb), 10);

  entry= gtk_entry_new();
  gtk_widget_set_usize( entry, 400, 0);
  gtk_box_pack_start( GTK_BOX(vb), entry, TRUE, TRUE,0);
  ed->entry=entry;
  gtk_widget_show(entry);

  gtk_widget_show( vb );

  button= gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_box_pack_start( GTK_BOX(GTK_DIALOG(dialog)->action_area),
		      button, TRUE, TRUE,0);
  gtk_signal_connect( GTK_OBJECT(button), "clicked",
		      (GtkSignalFunc) OkFunc,
		      NULL);
  gtk_widget_grab_default (button);
  gtk_widget_show(button);

  button= gtk_button_new_with_label("Clear");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_box_pack_start( GTK_BOX(GTK_DIALOG(dialog)->action_area),
		      button, TRUE, TRUE,0);
  gtk_signal_connect( GTK_OBJECT(button), "clicked",
		      (GtkSignalFunc) gag_clear_entry,
		      entry);
  gtk_widget_show(button);

  button= gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect( GTK_OBJECT(button), "clicked",
		      (GtkSignalFunc) gag_cancel_button,
		      GTK_OBJECT(dialog));
  gtk_box_pack_start( GTK_BOX(GTK_DIALOG(dialog)->action_area),
		      button, TRUE, TRUE,0);
  gtk_widget_show(button);
}

