/* gap_navigator_dialog.c
 *  by hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains the GAP Video Navigator dialog Window 
 * that provides a VCR-GUI for the GAP basic navigation functions.
 *
 */
/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/* TODO:
 * - BUG  X11 deadlock if GAP Video Navigator runs another plugin
 *          from the double-click callback procedure in the frame listbox,
 *          and the other plugin opens a new gtk dialog.
 *          (the new dialog does not get the focus)
 *       Current workaround:
 *          save frames of other types than xcf before the
 *          listbox is filled.
 *          (this forces the GAP p_decide_save_as Dialog
 *           when the Navigator is opened or an image != xcf is
 *           selected in the image optionmenu, and sets
 *           the fileformat specific save parameters)
 *         
 * - BUGFIX or workaround needed: list widget can't handle large lists
 *          (test failed at 1093 items maybe there is a limit of 1092 ??)
 *
 * - start of a 2.nd navigator should pop up the 1.st one and exit.
 * x- scroll the listbox (active image should always be in the visible (exposed) area
 *     problem: 1092 limit !
 * x- implement the unfinished callback procedures
 * x- Updatde Button (to create all missing and out of date thumbnails)
 * x- tooltips
 * x- multiple selections
 * x- timezoom and framerate should be stored in a video info file
 * x- calculate & update for frame timing labels
 * x- Render Preview defaultIcon for images without thumbnail
 *          and for preview_size 0 (off)
 *
 * Events that sould be handled:
 * - changes of the active_image (in ram)  :: update of one frame_widget
 * x- change of preview_size :: update full frame_widgets list
 * x- changes of the active_image (on disk) :: update of image_menu + full frame_widgets list
 *   (maybe i'll set a polling timer event to watch the diskfile)
 * x- close of the active_image (in ram)  :: update of image_menu + full frame_widgets list
 *
 * - drag & drop 
 *    (Problem: gimage struct is not available for plugins,
 *                         need a Drag&Drop type that operates on image_id)
 * - preferences should have additional video_preview_size
 *   (tiny,small,normal,large,huge)
 *
 */


/* revision history:
 * version 1.1.14a; 2000.01.08   hof: 1.st release
 */

static char *gap_navigator_version = "1.1.14a; 2000/01/08";


/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

/* GIMP includes */
#include <gtk/gtk.h>
#include "config.h"
#include <libgimp/stdplugins-intl.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <pixmaps/update.xpm>
#include <pixmaps/play.xpm>
#include <pixmaps/duplicate.xpm>
#include <pixmaps/delete.xpm>

#include <pixmaps/prev.xpm>
#include <pixmaps/next.xpm>
#include <pixmaps/first.xpm>
#include <pixmaps/last.xpm>

/*
 *   OpsButton  is not available for plugins in libgimp 1.1.14
 *   workaround: include gimp-1.1.14/app/ops_buttons.h /.c
 */
 
/* ---------------------------------------  start copy of gimp-1.1.14/app/ops_buttons.h */
#ifndef __OPS_BUTTONS_H__
#define __OPS_BUTTONS_H__

typedef enum
{
  OPS_BUTTON_MODIFIER_NONE,
  OPS_BUTTON_MODIFIER_SHIFT,
  OPS_BUTTON_MODIFIER_CTRL,
  OPS_BUTTON_MODIFIER_ALT,
  OPS_BUTTON_MODIFIER_SHIFT_CTRL,
  OPS_BUTTON_MODIFIER_LAST
} OpsButtonModifier;

typedef enum
{
  OPS_BUTTON_NORMAL,
  OPS_BUTTON_RADIO
} OpsButtonType;

typedef struct _OpsButton OpsButton;

struct _OpsButton 
{
  gchar         **xpm_data;       /*  xpm data for the button  */
  GtkSignalFunc   callback;       /*  callback function        */
  GtkSignalFunc  *ext_callbacks;  /*  callback functions when
				   *  modifiers are pressed    */
  gchar          *tooltip;
  gchar          *private_tip;
  GtkWidget      *widget;         /*  the button widget        */
  gint            modifier;
};

/* Function declarations */

GtkWidget * ops_button_box_new (GtkWidget     *parent,
				OpsButton     *ops_button,
				OpsButtonType  ops_type);

#endif /* __OPS_BUTTONS_H__ */
/* ---------------------------------------  end copy of gimp-1.1.14/app/ops_buttons.h */


/* GAP includes */
#include "gap_lib.h"
#include "gap_pdb_calls.h"

/* Note:
 *  the PDB call of gimp_file_load_thumbnail has a bug in gimp-1.1.14.
 *  As workaround i use a copy of gimp-1.1.14/app/fileops.c:readXVThumb
 *  to read .xvpics correct directly from file (this is also the faster way)
 */
guchar* readXVThumb (const gchar  *fnam,
	     gint         *w,
	     gint         *h,
	     gchar       **imginfo /* caller frees if != NULL */);

/*  some definitions used in all dialogs  */
#define PREVIEW_EVENT_MASK (GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | \
                            GDK_ENTER_NOTIFY_MASK)
#define BUTTON_EVENT_MASK  (GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK | \
                            GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | \
                            GDK_BUTTON_RELEASE_MASK)

#define MAX_HEIGHT_GTK_SCROLLED_WIN 32767
#define PLUGIN_NAME "plug_in_gap_navigator"
#define LIST_WIDTH  200
#define LIST_HEIGHT 150
#define PREVIEW_BPP 3
#define THUMBNAIL_BPP 3
#define MIX_CHANNEL(b, a, m)  (((a * m) + (b * (255 - m))) / 255)
#define PREVIEW_BG_GRAY1 80
#define PREVIEW_BG_GRAY2 180

#define NUPD_IMAGE_MENU             1
#define NUPD_THUMBFILE_TIMESTAMP    2
#define NUPD_FRAME_NR_CHANGED       4
#define NUPD_PREV_LIST              8
#define NUPD_PREV_LIST_ICONS        16
#define NUPD_ALL                   0xffffffff;

typedef struct _OpenFrameImages OpenFrameImages;
typedef struct _NaviDialog NaviDialog;
typedef struct _FrameWidget FrameWidget;
typedef struct _SelectedRange SelectedRange;

struct _OpenFrameImages{
  gint32 image_id;
  gint32 frame_nr;
  OpenFrameImages *next;
};


struct _NaviDialog
{
  gint          tooltip_on;
  GtkWidget     *shell;
  GtkWidget     *vbox;
  GtkWidget     *mode_option_menu;
  GtkWidget     *frame_list;
  GtkWidget     *scrolled_win;
  GtkWidget     *preserve_trans;
  GtkWidget     *framerate_box;
  GtkWidget     *timezoom_box;
  GtkWidget     *image_option_menu;
  GtkWidget     *image_menu;
  GtkAccelGroup *accel_group;
  GtkAdjustment *framerate_data;
  GtkAdjustment *timezoom_data;
  GtkWidget     *frame_preview;
  GtkWidget     *framerange_number_label;
  gint           waiting_cursor;
  GdkCursor     *cursor_wait;
  GdkCursor     *cursor_acitve;

  gdouble ratio;
  gdouble preview_size;
  gint    image_width, image_height;
  gint    gimage_width, gimage_height;

  /* state information  */
  gint32        active_imageid;
  gint32        any_imageid;
  t_anim_info  *ainfo_ptr;
  t_video_info *vin_ptr;
  GSList       *frame_widgets;
  int           timer;
  int           cycle_time;
  OpenFrameImages *OpenFrameImagesList;
  int              OpenFrameImagesCount;
  gint32       item_height;
};


struct _FrameWidget
{
  GtkWidget *clip_widget;
  GtkWidget *number_label;
  GtkWidget *time_label;
  GtkWidget *frame_preview;
  GtkWidget *list_item;
  GtkWidget *label;

  gint32    image_id;
  gint32    frame_nr;
  GdkPixmap *frame_pixmap;
  gint       width, height;

  /*  state information  */
  gboolean  visited;
  time_t    thumb_timestamp;
  char      *thumb_filename;
  /* GimpDropType drop_type; */
};


struct _SelectedRange {
   gint32 from;
   gint32 to;
   SelectedRange *next;
};

/* -----------------------
 * procedure declarations
 * -----------------------
 */
int  gap_navigator(gint32 image_id);
static void navi_preview_extents (void);
static void frames_dialog_flush (void);
static void frames_dialog_update (gint32 image_id);
static void frame_widget_preview_redraw      (FrameWidget *);

static gint navi_images_menu_constrain (gint32 image_id, gint32 drawable_id, gpointer data);
static void navi_images_menu_callback  (gint32 id, gpointer data);

static void navi_dialog_thumb_update_callback(GtkWidget *w, gpointer   data);
static void navi_dialog_thumb_updateall_callback(GtkWidget *w, gpointer   data);
static void navi_dialog_vcr_play_callback(GtkWidget *w, gpointer   data);
static void navi_dialog_vcr_play_optim_callback(GtkWidget *w, gpointer   data);
static void navi_dialog_frames_duplicate_frame_callback(GtkWidget *w, gpointer   data);
static void navi_dialog_frames_delete_frame_callback(GtkWidget *w, gpointer   data);

static void navi_dialog_vcr_goto_first_callback(GtkWidget *w, gpointer   data);
static void navi_dialog_vcr_goto_prev_callback(GtkWidget *w, gpointer   data);
static void navi_dialog_vcr_goto_prevblock_callback(GtkWidget *w, gpointer   data);
static void navi_dialog_vcr_goto_next_callback(GtkWidget *w, gpointer   data);
static void navi_dialog_vcr_goto_nextblock_callback(GtkWidget *w, gpointer   data);
static void navi_dialog_vcr_goto_last_callback(GtkWidget *w, gpointer   data);

static void navi_framerate_scale_update(GtkAdjustment *w, gpointer   data);
static void navi_timezoom_scale_update(GtkAdjustment *w, gpointer   data);


static gint frame_list_events                   (GtkWidget *, GdkEvent *, gpointer);

static void frame_widget_select_update       (GtkWidget *, gpointer);
static gint frame_widget_button_events       (GtkWidget *, GdkEvent *);
static gint frame_widget_preview_events      (GtkWidget *, GdkEvent *);

static gint navi_dialog_poll(GtkWidget *w, gpointer   data);
static void navi_dialog_update(gint32 update_flag);
static void navi_scroll_to_current_frame_nr(void);

static FrameWidget * frame_widget_create (gint32 image_id, gint32 frame_nr);
static void          frame_widget_delete (FrameWidget *fw);
static gint32        navi_get_preview_size(void);
static void          frames_timing_update (void);
static void          frame_widget_time_label_update(FrameWidget *fw);
static void          navi_dialog_tooltips(void);
static void          navi_set_waiting_cursor(void);
static void          navi_set_active_cursor(void);

/* -----------------------
 * Local data 
 * -----------------------
 */

static NaviDialog *naviD = NULL;
static gint suspend_gimage_notify = 0;
static gint32  global_old_active_imageid = -1;

/*  the ops buttons  */
static GtkSignalFunc navi_dialog_update_ext_callbacks[] =
{
  navi_dialog_thumb_updateall_callback, NULL, NULL, NULL
};
static GtkSignalFunc navi_dialog_vcr_play_ext_callbacks[] = 
{
  navi_dialog_vcr_play_optim_callback, NULL, NULL, NULL
};
static GtkSignalFunc navi_dialog_vcr_goto_prev_ext_callbacks[] = 
{
  navi_dialog_vcr_goto_prevblock_callback, NULL, NULL, NULL
};
static GtkSignalFunc navi_dialog_vcr_goto_next_ext_callbacks[] = 
{
  navi_dialog_vcr_goto_nextblock_callback, NULL, NULL, NULL
};

static OpsButton frames_ops_buttons[] =
{
  { play_xpm, navi_dialog_vcr_play_callback, navi_dialog_vcr_play_ext_callbacks,
    N_("Playback         \n"
       "<Shift> optimized"),
    "#playback",
    NULL, 0 },
  { update_xpm, navi_dialog_thumb_update_callback, navi_dialog_update_ext_callbacks,
    N_("Smart Update .xvpics\n"
       "<Shift> forced upd"),
    "#update",
    NULL, 0 },
  { duplicate_xpm, navi_dialog_frames_duplicate_frame_callback, NULL,
    N_("Duplicate selected Frames"),
    "#duplicate",
    NULL, 0 },
  { delete_xpm, navi_dialog_frames_delete_frame_callback, NULL,
    N_("Delete selected Frames"),
    "#delete",
    NULL, 0 },
  { NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

static OpsButton vcr_ops_buttons[] =
{
  { first_xpm, navi_dialog_vcr_goto_first_callback, NULL,
    N_("Goto 1.st Frame"),
    "#goto_first",
    NULL, 0 },
  { prev_xpm, navi_dialog_vcr_goto_prev_callback, navi_dialog_vcr_goto_prev_ext_callbacks,
    N_("Goto prev Frame\n"
       "<Shift> use timezoom stepsize"),
    "#goto_previous",
    NULL, 0 },
  { next_xpm, navi_dialog_vcr_goto_next_callback, navi_dialog_vcr_goto_next_ext_callbacks,
    N_("Goto next Frame\n"
       "<Shift> use timezoom stepsize"),
    "#goto_next",
    NULL, 0 },
  { last_xpm, navi_dialog_vcr_goto_last_callback, NULL,
    N_("Goto last Frame"),
    "#goto_last",
    NULL, 0 },

  { NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

/* ------------------------
 * gap DEBUG switch
 * ------------------------
 */

/* int gap_debug = 1; */    /* print debug infos */
/* int gap_debug = 0; */    /* 0: dont print debug infos */

int gap_debug = 0;


static void query(void);
static void run(char *name, int nparam, GParam *param,
                int *nretvals, GParam **retvals);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};

/* ------------------------
 * MAIN, query & run
 * ------------------------
 */

MAIN ()

static void
query ()
{
  static GParamDef args_navigator[] =
  {
    {PARAM_INT32, "run_mode", "Interactive"},
    {PARAM_IMAGE, "image", "(unused)"},
    {PARAM_DRAWABLE, "drawable", "(unused)"},
  };
  static int nargs_navigator = sizeof(args_navigator) / sizeof(args_navigator[0]);

  static GParamDef *return_vals = NULL;
  static int nreturn_vals = 0;


  gimp_install_procedure(PLUGIN_NAME,
			 "GAP video navigator dialog",
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_navigator_version,
			 N_("<Image>/Video/VCR Navigator..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_navigator, nreturn_vals,
			 args_navigator, return_vals);
}	/* end query */



static void
run (char    *name,
     int      n_params,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  gint32 l_active_image;
  char       *l_env;
  
  static GParam values[2];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gint32     nr;
  pid_t  l_navid_pid;
   
  gint32     l_rc;

  *nreturn_vals = 1;
  *return_vals = values;
  nr = 0;
  l_rc = 0;


  l_env = g_getenv("GAP_DEBUG");
  if(l_env != NULL)
  {
    if((*l_env != 'n') && (*l_env != 'N')) gap_debug = 1;
  }

  run_mode = param[0].data.d_int32;
  l_active_image = -1;

  if(gap_debug) printf("\n\ngap_navigator: debug name = %s\n", name);
  
  l_active_image = param[1].data.d_image;
  
  if (run_mode == RUN_NONINTERACTIVE) {
    INIT_I18N();
  } else {
    INIT_I18N_UI();
  }

  /* check for other Video navigator Dialog Process */
  if (sizeof(pid_t) == gimp_get_data_size(PLUGIN_NAME))
  {
    gimp_get_data(PLUGIN_NAME, &l_navid_pid);
    if(l_navid_pid != 0)
    {
       /* kill  with signal 0 checks only if the process is alive (no signal is sent)
        *       returns 0 if alive, 1 if no process with given pid found.
        */
      if (0 == kill(l_navid_pid, 0))
      {
         p_msg_win(RUN_INTERACTIVE, "Cant open 2 or more Video Navigator Windows");
         l_rc = -1;    
      }
    }
  }

  if(l_rc == 0)
  {
    /* set pid data when navigator is running */
    l_navid_pid = getpid();
    gimp_set_data(PLUGIN_NAME, &l_navid_pid, sizeof(pid_t));
    if (strcmp (name, PLUGIN_NAME) == 0)
    {
	if (run_mode != RUN_INTERACTIVE)
	{
            status = STATUS_CALLING_ERROR;
	}

	if (status == STATUS_SUCCESS)
	{
          l_rc = gap_navigator(l_active_image);
	}
    }
    /* set pid data to 0 when navigator stops */
    l_navid_pid = 0;
    gimp_set_data(PLUGIN_NAME, &l_navid_pid, sizeof(pid_t));
  }

  /* ---------- return handling --------- */

 if(l_rc < 0)
 {
    status = STATUS_EXECUTION_ERROR;
 }
 
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}


/* ---------------------------------
 * the navigator callback procedures
 * ---------------------------------
 */
static gint32
navi_get_preview_size(void)
{
  char *value_string;
  gint32 preview_size;

  preview_size = 32;  /* default preview size if nothing is configured */
  value_string = p_gimp_gimprc_query("video-preview-size");
  if(value_string == NULL)
  {
    value_string = p_gimp_gimprc_query("preview-size");
  }
  
  if(value_string)
  {
    if(gap_debug) printf("navi_get_preview_size value_str:%s:\n", value_string);

     if (strcmp (value_string, "none") == 0)
	preview_size = 0;
      else if (strcmp (value_string, "tiny") == 0)
	preview_size = 24;
      else if (strcmp (value_string, "small") == 0)
	preview_size = 32;
      else if (strcmp (value_string, "medium") == 0)
	preview_size = 48;
      else if (strcmp (value_string, "large") == 0)
	preview_size = 64;
      else if (strcmp (value_string, "huge") == 0)
	preview_size = 128;
      else
	preview_size = atol(value_string);
	
    g_free(value_string);
  }
  else
  {
    if(gap_debug) printf("navi_get_preview_size value_str is NULL\n");
  }
  
  return (preview_size);
}
 
static gint
navi_check_exist_first_and_last(t_anim_info *ainfo_ptr)
{
  char *l_fname;

  l_fname = p_alloc_fname(ainfo_ptr->basename,
                          ainfo_ptr->last_frame_nr, 
			  ainfo_ptr->extension);
  if (!p_file_exists(l_fname))
  { 
     g_free(l_fname);
     return(FALSE);
  }
  g_free(l_fname);
  
  l_fname = p_alloc_fname(ainfo_ptr->basename,
                          ainfo_ptr->first_frame_nr, 
			  ainfo_ptr->extension);
  if (!p_file_exists(l_fname))
  { 
     g_free(l_fname);
     return(FALSE);
  }
  g_free(l_fname);

  l_fname = p_alloc_fname(ainfo_ptr->basename,
                          ainfo_ptr->last_frame_nr+1, 
			  ainfo_ptr->extension);
  if (p_file_exists(l_fname))
  { 
     g_free(l_fname);
     return(FALSE);
  }
  g_free(l_fname);
  
  l_fname = p_alloc_fname(ainfo_ptr->basename,
                          ainfo_ptr->first_frame_nr-1, 
			  ainfo_ptr->extension);
  if (p_file_exists(l_fname))
  { 
     g_free(l_fname);
     return(FALSE);
  }
  g_free(l_fname);

  return(TRUE);			  
}

t_anim_info *
navi_get_ainfo(gint32 image_id, t_anim_info *old_ainfo_ptr)
{
  t_anim_info *ainfo_ptr;
  ainfo_ptr = p_alloc_ainfo(image_id, RUN_NONINTERACTIVE);
  if(ainfo_ptr)
  {
     if(old_ainfo_ptr)
     {
        if((old_ainfo_ptr->image_id == image_id)
	&& (strcmp(old_ainfo_ptr->basename, ainfo_ptr->basename) == 0))
	{
	   if(navi_check_exist_first_and_last(old_ainfo_ptr))
	   {
	      /* - image_id and name have not changed,
	       * - first and last frame still exist
	       *   and are still first and last frame
	       * In that case we can reuse first and last frame_nr
	       * without scanning the directory for frames
	       */
	      ainfo_ptr->first_frame_nr = old_ainfo_ptr->first_frame_nr;
	      ainfo_ptr->last_frame_nr  = old_ainfo_ptr->last_frame_nr;
	      ainfo_ptr->frame_cnt      = old_ainfo_ptr->frame_cnt;
              return(ainfo_ptr);
	   }
	}
     }
     p_dir_ainfo(ainfo_ptr);
  }
  return(ainfo_ptr);
}

void navi_reload_ainfo_force(gint32 image_id)
{
  t_anim_info *old_ainfo_ptr;
  char frame_nr_to_char[20];
  
  if(gap_debug) printf("navi_reload_ainfo_force image_id:%d\n", (int)image_id);
  old_ainfo_ptr = naviD->ainfo_ptr;
  naviD->active_imageid = image_id;
  naviD->ainfo_ptr = navi_get_ainfo(image_id, old_ainfo_ptr);
  
  if((strcmp(naviD->ainfo_ptr->extension, ".xcf") != 0)
  && (strcmp(naviD->ainfo_ptr->extension, ".xcfgz") != 0)
  && (global_old_active_imageid != image_id))
  {
     /* for other frameformats than xcf
      * save the frame a 1.st time (to set filetype specific save paramters)
      * this also is a workaround for a BUG that causes an X11 deadlock
      * when the save dialog pops up from the double-click callback in the frames listbox
      */
     suspend_gimage_notify++;
     p_save_named_frame(image_id, naviD->ainfo_ptr->old_filename);
     suspend_gimage_notify--;
  }
  global_old_active_imageid = image_id;

  if(naviD->framerange_number_label)
  {
     sprintf(frame_nr_to_char, "%04d - %04d"
            , (int)naviD->ainfo_ptr->first_frame_nr
	    , (int)naviD->ainfo_ptr->last_frame_nr );
     gtk_label_set (GTK_LABEL (naviD->framerange_number_label), frame_nr_to_char);
  }

  if(old_ainfo_ptr) p_free_ainfo(&old_ainfo_ptr);
}

void navi_reload_ainfo(gint32 image_id)
{
  if(image_id < 0) navi_reload_ainfo_force(naviD->any_imageid);
  else             navi_reload_ainfo_force(image_id);

  if(naviD->ainfo_ptr)
  {
    if(naviD->vin_ptr) g_free(naviD->vin_ptr);
    naviD->vin_ptr = p_get_video_info(naviD->ainfo_ptr->basename);
    gtk_adjustment_set_value(naviD->framerate_data, (gfloat)naviD->vin_ptr->framerate);
    gtk_adjustment_set_value(naviD->timezoom_data, (gfloat)naviD->vin_ptr->timezoom);

  }
}



static gint
navi_images_menu_constrain(gint32 image_id, gint32 drawable_id, gpointer data)
{
  if(gap_debug) printf("navi_images_menu_constrain PROCEDURE imageID:%d\n", (int)image_id);

  if(p_get_frame_nr(image_id) < 0)
  {
      return(FALSE);  /* reject images without frame number */
  }
  if(naviD)
  {
    if(naviD->active_imageid < 0)
    {
      /* if there is no valid active_imageid
       * we nominate the first one that comes along
       */
      naviD->any_imageid = image_id;
    }
  }
  return(TRUE);
} /* end  navi_images_menu_constrain */


static void
navi_images_menu_callback  (gint32 image_id, gpointer data)
{
  if(gap_debug) printf("navi_images_menu_callback PROCEDURE imageID:%d\n", (int)image_id);

  if(naviD)
  {
    if(naviD->active_imageid != image_id)
    {
      navi_reload_ainfo(image_id);
      navi_dialog_update(NUPD_FRAME_NR_CHANGED | NUPD_PREV_LIST);
      navi_scroll_to_current_frame_nr();
    }

  }
}


static void
navid_update_exposed_previews(void)
{
  if(naviD == NULL) return;

  /* I've tried  to send an "expose_event"  to frame_preview widgets
   * using gtk_signal_emit_by_name , but it did not work.
   * So i decided to use the trick widget_hide and widget_show
   * to force gtk to (re)expose the widgets in the listbox 
   * That way all visible items in the listbox are refreshed (redraw)
   */
  gtk_widget_hide(naviD->scrolled_win);
  gtk_widget_show(naviD->scrolled_win);
}


static void
navi_set_waiting_cursor(void)
{
  if(naviD == NULL) return;
  if(naviD->waiting_cursor) return;

  naviD->waiting_cursor = TRUE;
  gdk_window_set_cursor(GTK_WIDGET(naviD->shell)->window, naviD->cursor_wait);
}
static void
navi_set_active_cursor(void)
{
  if(naviD == NULL) return;
  if(!naviD->waiting_cursor) return;
  
  naviD->waiting_cursor = FALSE;
  gdk_window_set_cursor(GTK_WIDGET(naviD->shell)->window, naviD->cursor_acitve);
}

static void
navi_scroll_to_current_frame_nr(void)
{
  GtkAdjustment *adj;
  gfloat adj_val;
  gint index;
  gint page_size;

  if(naviD == NULL) return;
  if(naviD->ainfo_ptr == NULL) return;
  if(naviD->vin_ptr == NULL) return;
  if(naviD->scrolled_win == NULL) return;


  if(gap_debug) printf("navi_scroll_to_current_frame_nr: BEGIN timezoom:%d, item_height:%d\n", (int)naviD->vin_ptr->timezoom, (int)naviD->item_height);

  adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (naviD->scrolled_win));
  adj_val = adj->value;
  page_size = MAX(adj->page_size, naviD->item_height);
  index = 0;
  if(naviD->vin_ptr->timezoom)
  {
    index = (naviD->ainfo_ptr->curr_frame_nr - naviD->ainfo_ptr->first_frame_nr) / naviD->vin_ptr->timezoom;
  }
  
  if (index * naviD->item_height < adj->value)
  {
      adj_val = index * naviD->item_height;
  }
  else if ((index + 1) * naviD->item_height > adj->value + page_size)
  {
      adj_val = (index + 1) * naviD->item_height - page_size;
  }

  if(adj_val > MAX_HEIGHT_GTK_SCROLLED_WIN - page_size)
  {
    adj_val = MAX_HEIGHT_GTK_SCROLLED_WIN - page_size;
  }
  if(adj_val != adj->value)
  {
    if(gap_debug) printf("navi_scroll_to_current_frame_nr: OLD: %d  should be set to adj_val: %d  index:%d\n", (int)adj->value, (int)adj_val, (int)index);
    if(gap_debug) printf("navi_scroll_to_current_frame_nr: adj->lower :%d adj->upper :%d\n", (int)adj->lower, (int)adj->upper);
    /* gtk_adjustment_set_value(adj, adj_val); */
    adj->value = adj_val;
    gtk_adjustment_value_changed (adj);
    if(gap_debug) printf("navi_scroll_to_current_frame_nr: NEW : %d should be equal to adj_val: %d adj->page_size:%d\n", (int)adj->value, (int)adj_val, (int)adj->page_size);  
  }
}

static void
navi_dialog_tooltips(void)
{
  char *value_string;
  gint tooltip_on;
   
  if(naviD == NULL) return;

  tooltip_on = TRUE;
  value_string = p_gimp_gimprc_query("show-tool-tips");
  
  if(value_string != NULL)
  {
    if (strcmp(value_string, "no") == 0)
    {
      tooltip_on = FALSE;
    }
  }
  
  if(naviD->tooltip_on != tooltip_on)
  {
     naviD->tooltip_on = tooltip_on;
     
     if(tooltip_on)
     {
       gimp_help_enable_tooltips ();
     }
     else
     {
       gimp_help_disable_tooltips ();
     }
  }
}


static gint
navi_find_OpenFrameList(OpenFrameImages *search_item)
{
  OpenFrameImages *item_list; 

  item_list = naviD->OpenFrameImagesList;
  while(item_list)
  {
     if((item_list->image_id == search_item->image_id)
     && (item_list->frame_nr == search_item->frame_nr))
     {
       return(TRUE);  /* item found in the list */
     }   
    item_list = (OpenFrameImages *)item_list->next;
  }
  return(FALSE);
}

static void
navi_free_OpenFrameList(OpenFrameImages *list)
{
  OpenFrameImages *item_list;
  OpenFrameImages *item_next;
  
  item_list = list;
  while(item_list)
  {
    item_next = (OpenFrameImages *)item_list->next;
    g_free(item_list);
    item_list = item_next;
  }
}

static gint
navi_check_image_menu_changes()
{
  int nimages;
  gint32 *images;
  gint32  frame_nr;
  int  i;
  gint l_rc;
  int  item_count;
  OpenFrameImages *item_list; 
  OpenFrameImages *new_item;

  l_rc = TRUE;
  if(naviD->OpenFrameImagesList == NULL)
  {
    l_rc = FALSE;
  }
  item_list = NULL;
  item_count = 0;
  images = gimp_query_images (&nimages);
  for (i = 0; i < nimages; i++)
  {
     frame_nr = p_get_frame_nr(images[i]);  /* check for anim frame */
     if(frame_nr >= 0)
     {
        item_count++;
        new_item = g_malloc(sizeof(OpenFrameImages));
	new_item->image_id = images[i];
	new_item->frame_nr = frame_nr;
        new_item->next = item_list;
	item_list = new_item;
	if(!navi_find_OpenFrameList(new_item))
	{
          l_rc = FALSE;
	}
     }
  }
  g_free(images);
  
  if(item_count != naviD->OpenFrameImagesCount)
  {
    l_rc = FALSE;
  }
  
  if(l_rc == TRUE)
  {
    navi_free_OpenFrameList(item_list);
  }
  else
  {
    navi_free_OpenFrameList(naviD->OpenFrameImagesList);
    naviD->OpenFrameImagesCount = item_count;
    naviD->OpenFrameImagesList = item_list;
  }
  return(l_rc);
}

static gint
navi_refresh_image_menu()
{
  if(naviD)
  {
    if(!navi_check_image_menu_changes())
    {
      if(gap_debug) printf("navi_refresh_image_menu ** BEGIN REFRESH\n");
      if(naviD->OpenFrameImagesCount != 0) 
      { 
        gtk_widget_set_sensitive(naviD->vbox, TRUE); 
      }
      
      naviD->image_menu = gimp_image_menu_new(navi_images_menu_constrain,
                                              navi_images_menu_callback,
					      naviD,
					      naviD->active_imageid
                                              );
      gtk_option_menu_set_menu(GTK_OPTION_MENU(naviD->image_option_menu), naviD->image_menu);
      gtk_widget_show (naviD->image_menu);
      if(naviD->OpenFrameImagesCount == 0) 
      { 
        gtk_widget_set_sensitive(naviD->vbox, FALSE); 
      }
      return(TRUE);
    }
  }
  return(FALSE);
}

void navi_update_after_goto(void)
{
   if(naviD)
   {
      navi_dialog_update(NUPD_IMAGE_MENU | NUPD_FRAME_NR_CHANGED);
      navi_scroll_to_current_frame_nr();
   }
   gimp_displays_flush();
   navi_set_active_cursor();
}


static SelectedRange *
navi_get_selected_ranges(void)
{
  FrameWidget *fw;
  GSList      *list;
  GtkStateType  state;

  SelectedRange *new_range;
  SelectedRange *range_list;

  range_list = NULL;
  new_range  = NULL;
  list = naviD->frame_widgets;
  while (list)
  {
      fw = (FrameWidget *) list->data;
      list = g_slist_next (list);
      state = fw->list_item->state;
      if(state == GTK_STATE_SELECTED)
      {
         if(new_range == NULL)
	 {
	    new_range = g_malloc(sizeof(SelectedRange));
	    new_range->next = range_list;
	    new_range->from = fw->frame_nr;
	    new_range->to   = fw->frame_nr;
	    range_list = new_range;
	 }
	 else
	 {
	    new_range->to = fw->frame_nr;
	 }
      }
      else
      {
         new_range  = NULL;
      }
  }
  
  return(range_list);
}

static void
navi_thumb_update(gint update_all)
{
  gint32 l_frame_nr;
  gint32 l_image_id;
  gint   l_upd_flag;
  gint   l_any_upd_flag;
  char  *l_image_filename;
  char  *l_thumb_filename;
  struct stat  l_stat_thumb;
  struct stat  l_stat_image;
  
  if(naviD == NULL) return;
  if(naviD->ainfo_ptr == NULL) return;

  l_any_upd_flag = FALSE;
  for(l_frame_nr = naviD->ainfo_ptr->first_frame_nr;
      l_frame_nr <= naviD->ainfo_ptr->last_frame_nr;
      l_frame_nr++)
  {
     l_upd_flag = TRUE;
     l_image_filename = p_alloc_fname(naviD->ainfo_ptr->basename, l_frame_nr, naviD->ainfo_ptr->extension);
     l_thumb_filename = p_alloc_fname_thumbnail(l_image_filename);
     
     
     if(!update_all)
     {
       if (0 == stat(l_thumb_filename, &l_stat_thumb))
       {
          /* thumbnail filename exists */
         if(S_ISREG(l_stat_thumb.st_mode))
         {
	    /* and is a regular file */
            if (0 == stat(l_image_filename, &l_stat_image))
	    {
	       if(l_stat_image.st_mtime < l_stat_thumb.st_mtime)
	       {
	         /* time of last modification of image is older (less)
		  * than last modification of the thumbnail
		  * So we can skip the thumbnail Update for this frame
		  */
                 l_upd_flag = FALSE;
	       }
	    }
	 }
       
       }
     }

     if(l_upd_flag)
     {
        l_any_upd_flag = TRUE;
        if(gap_debug) printf("navi_thumb_update frame_nr:%d\n", (int)l_frame_nr);
        l_image_id = p_load_image(l_image_filename);
	p_gimp_file_save_thumbnail(l_image_id, l_image_filename);
        gimp_image_delete(l_image_id);
     }
     
     if(l_image_filename) g_free(l_image_filename);
     if(l_thumb_filename) g_free(l_thumb_filename);
  }
  
  if(l_any_upd_flag  )
  {
    navid_update_exposed_previews();
  }
}

static void navi_dialog_thumb_update_callback(GtkWidget *w, gpointer   data)
{
  if(gap_debug) printf("navi_dialog_thumb_update_callback\n");
  navi_thumb_update(FALSE);
}

static void navi_dialog_thumb_updateall_callback(GtkWidget *w, gpointer   data)
{
  if(gap_debug) printf("navi_dialog_thumb_updateall_callback\n");  
  navi_thumb_update(TRUE);
}


static void navi_playback(gint32 optimize)
{
   SelectedRange *range_list;
   SelectedRange *range_item;
   gint32         l_from;
   gint32         l_to;

   gint32         l_new_image_id;
   GParam          *return_vals;
   int              nreturn_vals;
   char           l_frame_name[50];
   int            l_frame_delay;
  

  if(gap_debug) printf("navi_dialog_vcr_play_callback\n");
  
  strcpy(l_frame_name, "frame_[####]");
  if(naviD->vin_ptr)
  {
    if(naviD->vin_ptr->framerate > 0)
    {
       l_frame_delay = 1000 / naviD->vin_ptr->framerate;
       sprintf(l_frame_name, "frame_[####] (%dms)", (int)l_frame_delay);
    }
  }

  l_from = naviD->ainfo_ptr->first_frame_nr;
  l_to   = naviD->ainfo_ptr->last_frame_nr;
  
  range_list = navi_get_selected_ranges();
  if(range_list)
  {
     l_to   = naviD->ainfo_ptr->first_frame_nr;
     l_from = naviD->ainfo_ptr->last_frame_nr;
     
     while(range_list)
     {
        l_from = MIN(l_from, range_list->from);
        l_to   = MAX(l_to,   range_list->to);
	range_item = range_list;
	range_list = range_list->next;
	g_free(range_item);
     }
  }

  return_vals = gimp_run_procedure ("plug_in_gap_range_to_multilayer",
                                    &nreturn_vals,
	                            PARAM_INT32,    RUN_NONINTERACTIVE,
				    PARAM_IMAGE,    naviD->active_imageid,
				    PARAM_DRAWABLE, -1,  /* dummy */
	                            PARAM_INT32,    l_from,
	                            PARAM_INT32,    l_to,
	                            PARAM_INT32,    3,     /* flatten image */
	                            PARAM_INT32,    1,     /* BG_VISIBLE */
	                            PARAM_INT32,    (gint32)naviD->vin_ptr->framerate,
				    PARAM_STRING,   l_frame_name,
	                            PARAM_INT32,    6,     /* use all visible layers */
	                            PARAM_INT32,    0,     /* ignore case */
	                            PARAM_INT32,    0,     /* normal selection (no invert) */
 				    PARAM_STRING,   "0",   /* select string (ignored) */
                                   PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
  {
         l_new_image_id = return_vals[1].data.d_image;

         if(optimize)
	 {
             return_vals = gimp_run_procedure ("plug_in_animationoptimize",
                                    &nreturn_vals,
	                            PARAM_INT32,    RUN_NONINTERACTIVE,
				    PARAM_IMAGE,    l_new_image_id,
				    PARAM_DRAWABLE, -1,  /* dummy */
                                   PARAM_END);
             if (return_vals[0].data.d_status == STATUS_SUCCESS)
             {
#ifdef COMMENT_BLOCK		
                 /* sorry, plug_in_animationoptimize does create 
		  * a new anim-optimized image, but does not return the id
		  * of the created image so we can't play the optimized
		  * image automatically.
		  */
                
	        /* destroy the tmp image */
                gimp_image_delete(l_new_image_id);

                l_new_image_id = return_vals[1].data.d_image;
#endif		
             }
	 }

         /* TODO: here we should start a thread for the playback, 
	  * so the navigator is not blocked until playback exits
	  */
      return_vals = gimp_run_procedure ("plug_in_animationplay",
                                    &nreturn_vals,
	                            PARAM_INT32,    RUN_NONINTERACTIVE,
				    PARAM_IMAGE,    l_new_image_id,
				    PARAM_DRAWABLE, -1,  /* dummy */
                                   PARAM_END);
  }
}


static void navi_dialog_vcr_play_callback(GtkWidget *w, gpointer   data)
{
  if(gap_debug) printf("navi_dialog_vcr_play_callback\n");
  navi_playback(FALSE /* dont not optimize */);
}

static void navi_dialog_vcr_play_optim_callback(GtkWidget *w, gpointer   data)
{
  if(gap_debug) printf("navi_dialog_vcr_play_optim_callback\n");
  navi_playback(TRUE /* optimize */);
}

void navi_dialog_frames_duplicate_frame_callback(GtkWidget *w, gpointer   data)
{
   SelectedRange *range_list;
   SelectedRange *range_item;
   GParam          *return_vals;
   int              nreturn_vals;
  

  if(gap_debug) printf("navi_dialog_frames_duplicate_frame_callback\n");
    
  range_list = navi_get_selected_ranges();
  if(range_list)
  {
    navi_set_waiting_cursor();    
    while(range_list)
    {
       /* Note: process the ranges from high frame_nummers
	*       downto low frame_numbers.
	*       (the range_list was created in a way that
	*        the highest range is added as 1st list element)
	*/
       if(gap_debug) printf("Duplicate Range from:%d  to:%d\n"
	             ,(int)range_list->from ,(int)range_list->to );
		     
       return_vals = gimp_run_procedure ("plug_in_gap_goto",
                                    &nreturn_vals,
	                            PARAM_INT32,    RUN_NONINTERACTIVE,
				    PARAM_IMAGE,    naviD->active_imageid,
				    PARAM_DRAWABLE, -1,  /* dummy */
	                            PARAM_INT32,    range_list->from,
                                    PARAM_END);
       if (return_vals[0].data.d_status == STATUS_SUCCESS)
       {
	  return_vals = gimp_run_procedure ("plug_in_gap_dup",
                                   &nreturn_vals,
	                           PARAM_INT32,    RUN_NONINTERACTIVE,
				   PARAM_IMAGE,    naviD->active_imageid,
				   PARAM_DRAWABLE, -1,  /* dummy */
	                           PARAM_INT32,    1,     /* copy block 1 times */
	                           PARAM_INT32,    range_list->from,
	                           PARAM_INT32,    range_list->to,
                                  PARAM_END);
       }
       range_item = range_list;
       range_list = range_list->next;
       g_free(range_item);
    }
    navi_update_after_goto();                                 
  }
}	/* end navi_dialog_frames_duplicate_frame_callback */

void navi_dialog_frames_delete_frame_callback(GtkWidget *w, gpointer   data)
{
   SelectedRange *range_list;
   SelectedRange *range_item;
   GParam          *return_vals;
   int              nreturn_vals;
  

  if(gap_debug) printf("navi_dialog_frames_delete_frame_callback\n");
    
  range_list = navi_get_selected_ranges();
  if(range_list)
  {
    navi_set_waiting_cursor();    
    while(range_list)
    {
       /* Note: process the ranges from high frame_nummers
	*       downto low frame_numbers.
	*       (the range_list was created in a way that
	*        the highest range is added as 1st list element)
	*/
       if(gap_debug) printf("Delete Range from:%d  to:%d\n"
	             ,(int)range_list->from ,(int)range_list->to );
		     
       return_vals = gimp_run_procedure ("plug_in_gap_goto",
                                    &nreturn_vals,
	                            PARAM_INT32,    RUN_NONINTERACTIVE,
				    PARAM_IMAGE,    naviD->active_imageid,
				    PARAM_DRAWABLE, -1,  /* dummy */
	                            PARAM_INT32,    range_list->from,
                                    PARAM_END);
       if (return_vals[0].data.d_status == STATUS_SUCCESS)
       {
 	  return_vals = gimp_run_procedure ("plug_in_gap_del",
                                   &nreturn_vals,
	                           PARAM_INT32,    RUN_NONINTERACTIVE,
				   PARAM_IMAGE,    naviD->active_imageid,
				   PARAM_DRAWABLE, -1,  /* dummy */
	                           PARAM_INT32,    1 + (range_list->to - range_list->from), /* number of frames to delete */
                                  PARAM_END);
       }
       range_item = range_list;
       range_list = range_list->next;
       g_free(range_item);
    }
    navi_update_after_goto();                                 
  }
}	/* end navi_dialog_frames_delete_frame_callback */

void navi_dialog_goto_callback(gint32 dst_framenr)
{
   GParam          *return_vals;
   int              nreturn_vals;

   if(gap_debug) printf("navi_dialog_goto_callback\n");
   navi_set_waiting_cursor();       
   return_vals = gimp_run_procedure ("plug_in_gap_goto",
                                    &nreturn_vals,
	                            PARAM_INT32,    RUN_NONINTERACTIVE,
				    PARAM_IMAGE,    naviD->active_imageid,
				    PARAM_DRAWABLE, -1,  /* dummy */
	                            PARAM_INT32,    dst_framenr,
                                    PARAM_END);
   navi_update_after_goto();                                 
}

void navi_dialog_vcr_goto_first_callback(GtkWidget *w, gpointer   data)
{
   GParam          *return_vals;
   int              nreturn_vals;

   if(gap_debug) printf("navi_dialog_vcr_goto_first_callback\n");
   navi_set_waiting_cursor();
   return_vals = gimp_run_procedure ("plug_in_gap_first",
                                    &nreturn_vals,
	                            PARAM_INT32,    RUN_NONINTERACTIVE,
				    PARAM_IMAGE,    naviD->active_imageid,
				    PARAM_DRAWABLE, -1,  /* dummy */
                                    PARAM_END);
   navi_update_after_goto();                                 
}

void navi_dialog_vcr_goto_prev_callback(GtkWidget *w, gpointer   data)
{
   GParam          *return_vals;
   int              nreturn_vals;

   if(gap_debug) printf("navi_dialog_vcr_goto_prev_callback\n");
   navi_set_waiting_cursor();       
   return_vals = gimp_run_procedure ("plug_in_gap_prev",
                                    &nreturn_vals,
	                            PARAM_INT32,    RUN_NONINTERACTIVE,
				    PARAM_IMAGE,    naviD->active_imageid,
				    PARAM_DRAWABLE, -1,  /* dummy */
                                    PARAM_END);
   navi_update_after_goto();                                 
}

void navi_dialog_vcr_goto_prevblock_callback(GtkWidget *w, gpointer   data)
{
   gint32           dst_framenr;

   if(gap_debug) printf("navi_dialog_vcr_goto_prevblock_callback\n");
   if(naviD->ainfo_ptr == NULL) navi_reload_ainfo(naviD->active_imageid);
   if(naviD->ainfo_ptr == NULL) return;
   if(naviD->vin_ptr == NULL) return;
   dst_framenr = MAX(naviD->ainfo_ptr->curr_frame_nr - naviD->vin_ptr->timezoom, 
                     naviD->ainfo_ptr->first_frame_nr);

   navi_dialog_goto_callback(dst_framenr);
}

void navi_dialog_vcr_goto_next_callback(GtkWidget *w, gpointer   data)
{
   GParam          *return_vals;
   int              nreturn_vals;

   if(gap_debug) printf("navi_dialog_vcr_goto_next_callback\n");
   navi_set_waiting_cursor();       
   return_vals = gimp_run_procedure ("plug_in_gap_next",
                                    &nreturn_vals,
	                            PARAM_INT32,    RUN_NONINTERACTIVE,
				    PARAM_IMAGE,    naviD->active_imageid,
				    PARAM_DRAWABLE, -1,  /* dummy */
                                    PARAM_END);
   navi_update_after_goto();                                 
}
void navi_dialog_vcr_goto_nextblock_callback(GtkWidget *w, gpointer   data)
{
   gint32           dst_framenr;

   if(gap_debug) printf("navi_dialog_vcr_goto_nextblock_callback\n");
   if(naviD->ainfo_ptr == NULL) navi_reload_ainfo(naviD->active_imageid);
   if(naviD->ainfo_ptr == NULL) return;
   if(naviD->vin_ptr == NULL) return;
   dst_framenr = MIN(naviD->ainfo_ptr->curr_frame_nr + naviD->vin_ptr->timezoom, 
                     naviD->ainfo_ptr->last_frame_nr);
       
   navi_dialog_goto_callback(dst_framenr);
}

void navi_dialog_vcr_goto_last_callback(GtkWidget *w, gpointer   data)
{
   GParam          *return_vals;
   int              nreturn_vals;

   if(gap_debug) printf("navi_dialog_vcr_goto_last_callback\n");
   navi_set_waiting_cursor();       
   return_vals = gimp_run_procedure ("plug_in_gap_last",
                                    &nreturn_vals,
	                            PARAM_INT32,    RUN_NONINTERACTIVE,
				    PARAM_IMAGE,    naviD->active_imageid,
				    PARAM_DRAWABLE, -1,  /* dummy */
                                    PARAM_END);
   navi_update_after_goto();                                 
}

static void
frames_timing_update (void)
{
  FrameWidget *fw;
  GSList      *list;

  list = naviD->frame_widgets;
  while (list)
  {
      fw = (FrameWidget *) list->data;
      list = g_slist_next (list);
      frame_widget_time_label_update(fw);
  }
}

void 
navi_framerate_scale_update(GtkAdjustment *adjustment,
		      gpointer       data)
{
  gdouble    framerate;

  if(naviD == NULL) return;
  if(naviD->vin_ptr == NULL) return;

  framerate = (gdouble) (adjustment->value);
  if(framerate != naviD->vin_ptr->framerate)
  {
     naviD->vin_ptr->framerate = framerate;
     if(naviD->ainfo_ptr)
     {
       /* write new framerate to video info file */
       p_set_video_info(naviD->vin_ptr, naviD->ainfo_ptr->basename);
     }
     frames_timing_update();
  }
  if(gap_debug) printf("navi_framerate_scale_update :%d\n", (int)naviD->vin_ptr->framerate);
}

void
navi_timezoom_scale_update(GtkAdjustment *adjustment,
		      gpointer       data)
{
  gint    timezoom;
  GtkAdjustment *adj;
  gfloat adj_val;

  if(naviD == NULL) return;
  if(naviD->vin_ptr == NULL) return;

  timezoom = (int) (adjustment->value);
  if(timezoom != naviD->vin_ptr->timezoom)
  {
    adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (naviD->scrolled_win));

    if(timezoom != 0)
    {
      adj_val = (adj->value * (gfloat)naviD->vin_ptr->timezoom) / (gfloat)timezoom;
      if(adj_val > MAX_HEIGHT_GTK_SCROLLED_WIN)
      {
        adj_val = MAX_HEIGHT_GTK_SCROLLED_WIN - adj->page_size;
      }
    }
    else
    {
      adj_val = adj->value;
    }
    naviD->vin_ptr->timezoom = timezoom;     
     if(naviD->ainfo_ptr)
     {
       /* write new timezoom to video info file */
       p_set_video_info(naviD->vin_ptr, naviD->ainfo_ptr->basename);
     }
     frames_dialog_flush();
     gtk_adjustment_set_value(adjustment, (gfloat)timezoom);
     adj->value = adj_val;
     gtk_adjustment_value_changed (adj);
  }
  if(gap_debug) printf("navi_timezoom_scale_update :%d\n", (int)naviD->vin_ptr->timezoom);
}

/********************************/
/*  frame list events callback  */
/********************************/

static gint
frame_list_events (GtkWidget *widget,
		   GdkEvent  *event,
		   gpointer   data)
{
  GdkEventButton *bevent;
  GtkWidget      *event_widget;
  FrameWidget    *frame_widget;

  event_widget = gtk_get_event_widget (event);    

  if (GTK_IS_LIST_ITEM (event_widget))
  {
      frame_widget =
	(FrameWidget *) gtk_object_get_user_data (GTK_OBJECT (event_widget));

      switch (event->type)
      {
 	case GDK_BUTTON_PRESS:
	  bevent = (GdkEventButton *) event;

	  if (bevent->button == 3)
	  {
	      if(gap_debug) printf("frame_list_events:nothing implemented on GDK_BUTTON_PRESS event\n");
	      return TRUE;
	  }
	  break;
	case GDK_2BUTTON_PRESS:
	  bevent = (GdkEventButton *) event;
	  if(gap_debug) printf("frame_list_events: GDK_2BUTTON_PRESS event\n");
	  
	  navi_dialog_goto_callback(frame_widget->frame_nr);
	  return TRUE;

	default:
	  break;
      }
  }
  return FALSE;
}


void
frames_dialog_flush (void)
{
  if(gap_debug) printf("frames_dialog_flush\n");
  if(naviD)
  {
    frames_dialog_update(naviD->active_imageid);
  }
}

void
frames_dialog_update (gint32 image_id)
{
  FrameWidget *fw;
  GSList      *list;
  GList       *item_list;
  gint32       l_frame_nr;
  gint32       l_count;
  gint32       l_warning_flag;
  gint32       scrolled_window_height;
  gint32       scrolled_window_height_ok;
  GtkAdjustment *adj;
  gint          l_waiting_cursor;

  if(gap_debug) printf("frames_dialog_update image_id:%d\n", (int)image_id);

  if (! naviD)   /* || (naviD->active_imageid == image_id) */
  {
    return;
  }
  l_waiting_cursor = naviD->waiting_cursor;
  if(!naviD->waiting_cursor) navi_set_waiting_cursor();

  navi_reload_ainfo(image_id);

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;
  scrolled_window_height_ok = 0;

  /*  Free all elements in the frames listbox  */
  gtk_list_clear_items (GTK_LIST (naviD->frame_list), 0, -1);

  list = naviD->frame_widgets;
  while (list)
  {
      fw = (FrameWidget *) list->data;
      list = g_slist_next (list);
      frame_widget_delete (fw);
  }

  if (naviD->frame_widgets)
    g_warning ("frames_dialog_update(): naviD->frame_widgets not empty!");
  naviD->frame_widgets = NULL;

  /*  Find the preview extents  */
  navi_preview_extents ();

  
/*  naviD->active_layer = NULL;
 */

  l_count = 0;
  l_warning_flag = 0;
  scrolled_window_height = naviD->item_height;
  item_list = NULL;
  for (l_frame_nr = naviD->ainfo_ptr->first_frame_nr;
       l_frame_nr <= naviD->ainfo_ptr->last_frame_nr;
       l_frame_nr++)
  {
      /*  create a frame list item  */
      if((l_frame_nr == naviD->ainfo_ptr->first_frame_nr)
      || (l_frame_nr == naviD->ainfo_ptr->last_frame_nr)
      || (((l_frame_nr - naviD->ainfo_ptr->first_frame_nr) % naviD->vin_ptr->timezoom) == 0)  )
      {

        if(scrolled_window_height < MAX_HEIGHT_GTK_SCROLLED_WIN)
	{
          fw = frame_widget_create (image_id, l_frame_nr);
	  if(fw)
	  {
            naviD->frame_widgets = g_slist_append (naviD->frame_widgets, fw);
            item_list = g_list_append (item_list, fw->list_item);
	    scrolled_window_height_ok = scrolled_window_height;
	  }
	}
	else
	{
	   if(l_warning_flag == 0)
	   {
             printf("WARNING: GTK listbox can't handle more than %d items of %d pixels height\n"
	            ,(int)l_count, (int)naviD->item_height);
             printf("         can't display frames upto %d\n", (int)naviD->ainfo_ptr->last_frame_nr);
	     l_warning_flag = 1;
	   }
	}
        l_count++;
	scrolled_window_height += naviD->item_height;
      }
  }

  if(gap_debug) printf("LIST_CREATED: first:%d, last:%d\n, count:%d\n", 
                (int)naviD->ainfo_ptr->first_frame_nr,
		(int)naviD->ainfo_ptr->last_frame_nr,
		(int)l_count );
  

  /*  get the index of the active frame  */
  if (item_list)
  {
    gtk_list_insert_items (GTK_LIST (naviD->frame_list), item_list, 0);
    adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (naviD->scrolled_win));
    adj->lower = 0;
    adj->upper = scrolled_window_height_ok;
  }
  suspend_gimage_notify--;
  if(!l_waiting_cursor) navi_set_active_cursor();
}

static void
frame_widget_select_update (GtkWidget *widget,
			    gpointer   data)
{
  FrameWidget *frame_widget;

  if (! (frame_widget = (FrameWidget *) data))
    return;

  /*  Is the list item being selected?  */
  if (widget->state != GTK_STATE_SELECTED)
    return;

  /*  Only notify the gimage of an active layer change if necessary  */
  if (suspend_gimage_notify == 0)
  {
     if(gap_debug) printf("frame_widget_select_update NOT IMPLEMENTED (maybe not needed)\n");
  }
}

static gint
frame_widget_button_events (GtkWidget *widget,
			    GdkEvent  *event)
{
  if(gap_debug) printf("frame_widget_button_events NOT IMPLEMENTED\n");
  return -1;
}

/***********************/
/*  Preview functions  */
/***********************/
static gint
render_current_preview (GtkWidget *preview_widget, FrameWidget *fw)
{
   guchar *l_rowbuf;
   guchar *l_ptr;
   gint32  l_x, l_y;
   gint32  l_ofx, l_ofy;
   gint32  l_ofs_thpixel;
   guchar *l_thumbdata_ptr;
   gint32  l_thumbdata_count;
   gint32  l_th_width;
   gint32  l_th_height;
   gint32  l_th_bpp;
   gint32  l_thumb_rowstride;
   gint32  l_pv_width;
   gint32  l_pv_height;
   gdouble l_xscale;
   gdouble l_yscale;
   gdouble l_ofd;
   guchar  l_alpha;
   gint    l_1;
   gint    l_offset;
   gint    l_rc;


   if(gap_debug) printf("render_current_preview START\n");

   l_rc = FALSE;
   if(naviD == NULL) return (l_rc);
   if(naviD->ainfo_ptr == NULL) return(l_rc);

   l_thumbdata_ptr = NULL;
   l_pv_width = naviD->image_width;
   l_pv_height = naviD->image_height;

   if(gap_debug) printf("render_current_preview BEFORE p_gimp_image_thumbnail w:%d h:%d,\n"
                         ,(int)l_pv_width, (int)l_pv_height);
   p_gimp_image_thumbnail(naviD->active_imageid, l_pv_width, l_pv_height,
                          &l_th_width, &l_th_height, &l_th_bpp,
			  &l_thumbdata_count, &l_thumbdata_ptr);
   if(gap_debug) printf("render_current_preview AFTER p_gimp_image_thumbnail th_w:%d th_h:%d, th_bpp:%d, count:%d\n"
                         ,(int)l_th_width, (int)l_th_height, (int)l_th_bpp, (int)l_thumbdata_count);

   if(l_th_bpp < 3) l_1 = 0;
   else             l_1 = 1;		 

   l_rowbuf = g_malloc(PREVIEW_BPP * l_pv_width);
   if(l_rowbuf == NULL) return(l_rc);

   if(l_thumbdata_ptr != NULL)
   {
     l_xscale = (gdouble)l_th_width / (gdouble)l_pv_width;
     l_yscale = (gdouble)l_th_height / (gdouble)l_pv_height;
     l_thumb_rowstride = l_th_bpp * l_th_width;

    /* render preview */
    for(l_y = 0; l_y < l_pv_height; l_y++)
    {
       l_ptr = l_rowbuf;
       l_ofd = (l_yscale * (gdouble)l_y) + 0.5;
       l_ofy = (gint32)(l_ofd) * l_thumb_rowstride;

       if(l_y & 0x4) l_offset = 4;
       else          l_offset = 0;
       
       if((l_th_bpp > 3) || (l_th_bpp == 2))
       {
         /* thumbdata has alpha channel */
	 for(l_x = 0; l_x < l_pv_width; l_x++)
	 {
	     l_ofd = (l_xscale * (gdouble)l_x) + 0.5;
	     l_ofx = (gint32)(l_ofd) * l_th_bpp;
	     l_ofs_thpixel =  l_ofy + l_ofx;

             l_alpha = l_thumbdata_ptr[l_ofs_thpixel + (l_th_bpp -1)];

             if((l_x + l_offset) & 0x4)
	     {
              *l_ptr    = MIX_CHANNEL (PREVIEW_BG_GRAY2, l_thumbdata_ptr[l_ofs_thpixel], l_alpha);
	       l_ptr[1] = MIX_CHANNEL (PREVIEW_BG_GRAY2, l_thumbdata_ptr[l_ofs_thpixel + l_1], l_alpha);
	       l_ptr[2] = MIX_CHANNEL (PREVIEW_BG_GRAY2, l_thumbdata_ptr[l_ofs_thpixel + l_1 + l_1], l_alpha);
	     }
	     else
	     {
              *l_ptr    = MIX_CHANNEL (PREVIEW_BG_GRAY1, l_thumbdata_ptr[l_ofs_thpixel], l_alpha);
	       l_ptr[1] = MIX_CHANNEL (PREVIEW_BG_GRAY1, l_thumbdata_ptr[l_ofs_thpixel + l_1], l_alpha);
	       l_ptr[2] = MIX_CHANNEL (PREVIEW_BG_GRAY1, l_thumbdata_ptr[l_ofs_thpixel + l_1 + l_1], l_alpha);
	     }

            l_ptr += PREVIEW_BPP;
	 }
       }
       else
       {
	 for(l_x = 0; l_x < l_pv_width; l_x++)
	 {
	     l_ofd = (l_xscale * (gdouble)l_x) + 0.5;
	     l_ofx = (gint32)(l_ofd) * l_th_bpp;
	     l_ofs_thpixel =  l_ofy + l_ofx;

            *l_ptr    = l_thumbdata_ptr[l_ofs_thpixel];
	     l_ptr[1] = l_thumbdata_ptr[l_ofs_thpixel + l_1];
	     l_ptr[2] = l_thumbdata_ptr[l_ofs_thpixel + l_1 + l_1];

            l_ptr += PREVIEW_BPP;
	 }
       }
       gtk_preview_draw_row(GTK_PREVIEW(preview_widget), l_rowbuf, 0, l_y, l_pv_width);
    }
    l_rc = TRUE;
  }

  if(l_rowbuf)          g_free(l_rowbuf);
  if(l_thumbdata_ptr)   g_free(l_thumbdata_ptr);

  if(gap_debug) printf("render_current_preview END\n");
 
  return (l_rc);
			  
}	/* end render_current_preview */

static gint
render_preview (GtkWidget *preview_widget, FrameWidget *fw)
{
   guchar *l_rowbuf;
   guchar *l_ptr;
   gint32  l_x, l_y;
   gint32  l_ofx, l_ofy;
   gint32  l_ofs_thpixel;
   gint32  l_th_width;
   gint32  l_th_height;
   gint32  l_thumb_rowstride;
   gint32  l_pv_width;
   gint32  l_pv_height;
   gdouble l_xscale;
   gdouble l_yscale;
   gdouble l_ofd;
   char   *l_filename;
   gint    l_rc;
   guchar *l_raw_thumb;
   gchar  *l_tname;
   gchar   *imginfo = NULL;
   struct stat  l_stat_thumb;


   if(gap_debug) printf("render_preview (quick)\n");

   l_rc = FALSE;
   l_raw_thumb = NULL;
   if(naviD == NULL) return (l_rc);
   if(naviD->ainfo_ptr == NULL) return(l_rc);
   if(naviD->ainfo_ptr->curr_frame_nr == fw->frame_nr)
   {
     /* if this is the currently open image
      * we first write the thumbnail to disc
      * so we get an up to date version in the following readXVThumb call
      */
      l_rc = render_current_preview(preview_widget, fw);
      return(l_rc);
   }
   
   l_filename = p_alloc_fname(naviD->ainfo_ptr->basename, fw->frame_nr, naviD->ainfo_ptr->extension);
   l_tname = p_alloc_fname_thumbnail(l_filename);

    
   if (0 == stat(l_tname, &l_stat_thumb))
   {
     fw->thumb_timestamp  = l_stat_thumb.st_mtime;
     l_raw_thumb = readXVThumb (l_tname, &l_th_width, &l_th_height, &imginfo);
     if(fw->thumb_filename) g_free(fw->thumb_filename);
     fw->thumb_filename = l_tname;    
   }
   else
   {
     g_free(l_tname);
   }

   l_pv_width = naviD->image_width;
   l_pv_height = naviD->image_height;

   l_rowbuf = g_malloc(PREVIEW_BPP * l_pv_width);
   if(l_rowbuf == NULL) return(l_rc);
      
   if(l_raw_thumb != NULL)
   {
     l_xscale = (gdouble)l_th_width / (gdouble)l_pv_width;
     l_yscale = (gdouble)l_th_height / (gdouble)l_pv_height;
     l_thumb_rowstride = l_th_width; /* raw thumb data BPP is 1 */

    /* render preview */
    for(l_y = 0; l_y < l_pv_height; l_y++)
    {
       l_ptr = l_rowbuf;
       l_ofd = (l_yscale * (gdouble)l_y) + 0.5;
       l_ofy = (gint32)(l_ofd) * l_thumb_rowstride;   /* check if > thumbnail height !!! */

       for(l_x = 0; l_x < l_pv_width; l_x++)
       {
	   l_ofd = (l_xscale * (gdouble)l_x) + 0.5;
	   l_ofx = (gint32)(l_ofd);  /* raw thumb data BPP is 1 */
	   l_ofs_thpixel =  l_ofy + l_ofx;

          *l_ptr    = ((l_raw_thumb[l_ofs_thpixel]>>5)*255)/7;
	   l_ptr[1] = (((l_raw_thumb[l_ofs_thpixel]>>2)&7)*255)/7;
	   l_ptr[2] = (((l_raw_thumb[l_ofs_thpixel])&3)*255)/3;

          l_ptr += PREVIEW_BPP;
       }
       gtk_preview_draw_row(GTK_PREVIEW(preview_widget), l_rowbuf, 0, l_y, l_pv_width);
    }
    l_rc = TRUE;
  }

  if(l_rowbuf)          g_free(l_rowbuf);
  if(l_raw_thumb)       g_free(l_raw_thumb);
  if(l_filename)        g_free(l_filename);
  
  return (l_rc);
}	/* end render_preview */


void
render_no_preview (GtkWidget *widget,
		   GdkPixmap *pixmap)
{
  int w, h;
  int x1, y1, x2, y2;
  GdkPoint poly[6];
  int foldh, foldw;
  int i;

  gdk_window_get_size (pixmap, &w, &h);

  x1 = 2;
  y1 = h / 8 + 2;
  x2 = w - w / 8 - 2;
  y2 = h - 2;
  gdk_draw_rectangle (pixmap, widget->style->bg_gc[GTK_STATE_NORMAL], 1,
		      0, 0, w, h);
/*
  gdk_draw_rectangle (pixmap, widget->style->black_gc, 0,
		      x1, y1, (x2 - x1), (y2 - y1));
*/

  foldw = w / 4;
  foldh = h / 4;
  x1 = w / 8 + 2;
  y1 = 2;
  x2 = w - 2;
  y2 = h - h / 8 - 2;

  poly[0].x = x1 + foldw; poly[0].y = y1;
  poly[1].x = x1 + foldw; poly[1].y = y1 + foldh;
  poly[2].x = x1; poly[2].y = y1 + foldh;
  poly[3].x = x1; poly[3].y = y2;
  poly[4].x = x2; poly[4].y = y2;
  poly[5].x = x2; poly[5].y = y1;
  gdk_draw_polygon (pixmap, widget->style->white_gc, 1, poly, 6);

  gdk_draw_line (pixmap, widget->style->black_gc,
		 x1, y1 + foldh, x1, y2);
  gdk_draw_line (pixmap, widget->style->black_gc,
		 x1, y2, x2, y2);
  gdk_draw_line (pixmap, widget->style->black_gc,
		 x2, y2, x2, y1);
  gdk_draw_line (pixmap, widget->style->black_gc,
		 x1 + foldw, y1, x2, y1);

  for (i = 0; i < foldw; i++)
  {
    gdk_draw_line (pixmap, widget->style->black_gc,
		   x1 + i, y1 + foldh, x1 + i, (foldw == 1) ? y1 :
		   (y1 + (foldh - (foldh * i) / (foldw - 1))));
  }
}


static void
frame_widget_preview_redraw (FrameWidget *frame_widget)
{
  GdkPixmap **pixmap;
  GtkWidget *widget;

  widget = frame_widget->frame_preview;
  pixmap = &frame_widget->frame_pixmap;

  if(gap_debug) printf("frame_widget_preview_redraw image_id: %d, frame_nr:%d\n"
                       ,(int)frame_widget->image_id
                       ,(int)frame_widget->frame_nr);

  /*  determine width and height  */
  if((frame_widget->width != naviD->image_width)
  || (frame_widget->height != naviD->image_height))
  {
    frame_widget->width = naviD->image_width;
    frame_widget->height = naviD->image_height;
    if (frame_widget->width < 1) frame_widget->width = 1;
    if (frame_widget->height < 1) frame_widget->height = 1;
    gtk_drawing_area_size (GTK_DRAWING_AREA (frame_widget->frame_preview),
			 naviD->image_width + 4, naviD->image_height + 4);
    if(*pixmap)
    {
       gdk_pixmap_unref(*pixmap);
       *pixmap = NULL;
    }
  }
  /*  allocate the layer widget pixmap  */
  if (! *pixmap)
  {
    *pixmap = gdk_pixmap_new (widget->window,
			      naviD->image_width,
			      naviD->image_height,
			      -1);
  }


  if(naviD->preview_size < 1)
  {
    /* preview_size is none, render a default icon */
    render_no_preview (widget, *pixmap);
  }
  else
  {
    if(TRUE == render_preview (naviD->frame_preview, frame_widget))
    {
      gtk_preview_put (GTK_PREVIEW (naviD->frame_preview),
		   *pixmap, widget->style->black_gc,
		   0, 0, 0, 0, naviD->image_width, naviD->image_height);
    }
    else
    {
      /* frame has no thumbnail (.xvpics), render a default icon */
      render_no_preview (widget, *pixmap);
    }
  }
  
  /*  make sure the image has been transfered completely to the pixmap before
   *  we use it again...
   */
  gdk_flush ();

}	/* end frame_widget_preview_redraw */


static gint
frame_widget_preview_events (GtkWidget *widget,
			     GdkEvent  *event)
{
  GdkEventExpose *eevent;
  GdkPixmap **pixmap;
  GdkEventButton *bevent;
  FrameWidget *frame_widget;
  int valid;
  int sx, sy, dx, dy, w, h;

  pixmap = NULL;
  valid  = FALSE;

  frame_widget = (FrameWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));
  if (frame_widget->frame_nr < 0)
  {
    return FALSE;
  }

  pixmap = &frame_widget->frame_pixmap;
 /*   valid = GIMP_DRAWABLE(frame_widget->layer)->preview_valid; */  /* ?? */

  switch (event->type)
  {
    case GDK_2BUTTON_PRESS:
	  navi_dialog_goto_callback(frame_widget->frame_nr);
	  return TRUE;
    case GDK_BUTTON_PRESS:
      /*  Control-button press disables the application of the mask  */
      bevent = (GdkEventButton *) event;

      if(gap_debug) printf("frame_widget_preview_events GDK_BUTTON_PRESS button:%d NOT IMPLEMENTED\n"
                            , (int)bevent->button);

      if (bevent->button == 3)
      {
/*
 * 	  gtk_menu_popup (GTK_MENU (naviD->ops_menu),
 * 			  NULL, NULL, NULL, NULL,
 * 			  3, bevent->time);
 */
	  return TRUE;
      }

      if (event->button.state & GDK_CONTROL_MASK)
      {
      }
      /*  Alt-button press makes the mask visible instead of the layer  */
      else if (event->button.state & GDK_MOD1_MASK)
      {
      }
      break;

    case GDK_EXPOSE:
      if(gap_debug) printf("frame_widget_preview_events GDK_EXPOSE\n");

      navi_preview_extents();
      frame_widget_time_label_update(frame_widget);

      if (!valid || !*pixmap)
      {
	  frame_widget_preview_redraw (frame_widget);

	  gdk_draw_pixmap (widget->window,
			   widget->style->black_gc,
			   *pixmap,
			   0, 0, 2, 2,
			   naviD->image_width,
			   naviD->image_height);
      }
      else
      {
	  eevent = (GdkEventExpose *) event;

	  w = eevent->area.width;
	  h = eevent->area.height;

	  if (eevent->area.x < 2)
	  {
	      sx = eevent->area.x;
	      dx = 2;
	      w -= (2 - eevent->area.x);
	  }
	  else
	  {
	      sx = eevent->area.x - 2;
	      dx = eevent->area.x;
	  }

	  if (eevent->area.y < 2)
	  {
	      sy = eevent->area.y;
	      dy = 2;
	      h -= (2 - eevent->area.y);
	  }
	  else
	  {
	      sy = eevent->area.y - 2;
	      dy = eevent->area.y;
	  }

	  if ((sx + w) >= naviD->image_width)
	  {
	    w = naviD->image_width - sx;
          }

	  if ((sy + h) >= naviD->image_height)
	  {
	    h = naviD->image_height - sy;
          }

	  if ((w > 0) && (h > 0))
	  {
	    gdk_draw_pixmap (widget->window,
			     widget->style->black_gc,
			     *pixmap,
			     sx, sy, dx, dy, w, h);
          }
      }

      /*  The boundary indicating whether layer or mask is active  */
      /* frames do not have boundary */
      /* frame_widget_boundary_redraw (frame_widget); */
      break;

    default:
      break;
  }

  return FALSE;
}	/* end  frame_widget_preview_events */


static gint
navi_dialog_poll(GtkWidget *w, gpointer   data)
{
   gint32 frame_nr;
   gint32 update_flag;
   gint32 video_preview_size;
  
   if(gap_debug) printf("navi_dialog_poll  TIMER POLL\n");
   
   if(naviD)
   {
      if(suspend_gimage_notify == 0)
      {
	 update_flag = NUPD_IMAGE_MENU;
	 video_preview_size = navi_get_preview_size();
	 if(naviD->preview_size != video_preview_size)
	 {
            naviD->preview_size = video_preview_size;
            update_flag = NUPD_ALL;
	 }

	 /* check and enable/disable tooltips */      
	 navi_dialog_tooltips ();

	 frame_nr = p_get_frame_nr(naviD->active_imageid);
	 if(frame_nr < 0 )
	 {
            /* no valid frame number, maybe frame was closed
	     */
	    naviD->active_imageid = -1;
            update_flag = NUPD_ALL;	 
	 }
	 else
	 {
	   if(naviD->ainfo_ptr)
	   {
              update_flag = NUPD_IMAGE_MENU | NUPD_FRAME_NR_CHANGED;
	   }
	   else
	   {
              update_flag = NUPD_ALL;	 
	   }
	 }
	 navi_dialog_update(update_flag);
      }
      
      /* restart timer */
      naviD->timer = gtk_timeout_add(naviD->cycle_time,
                                    (GtkFunction)navi_dialog_poll, NULL);
   }
   return FALSE;
}

static void
navid_thumb_timestamp_check(void)
{
  struct stat  l_stat_thumb;
  FrameWidget *fw;
  GSList      *list;
  GtkAdjustment *adj;
  gint item_count;

   if(naviD == NULL) return;
   if(naviD->ainfo_ptr == NULL) return;

  adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (naviD->scrolled_win));

  if(gap_debug) printf("navid_thumb_timestamp_check: adj->value:%d  adj->page_size:%d\n", (int)adj->value, (int)adj->page_size);

   list = naviD->frame_widgets;
   item_count = -1;
   while (list)
   {
      item_count++;
      fw = (FrameWidget *) list->data;
      list = g_slist_next (list);

      if((item_count * naviD->item_height) < (adj->value - naviD->image_height))
      {
        continue;  /* WIDGET IS NOT EXPOSED */
      }
      if((item_count * naviD->item_height) > (adj->value + adj->page_size))
      {
        return;    /* WIDGET (and all further widgets) ARE NOT EXPOSED */
      }
      
      if(gap_debug) printf("navid_thumb_timestamp_check: Widget IS EXPOSED frame_nr:%04d\n", (int)fw->frame_nr);
      
      if(fw->frame_nr == naviD->ainfo_ptr->curr_frame_nr)
      {
        /* always redraw the currently opened frame
	 * (the render procedure uses gimp's internal chached thumbnaildata
	 * so the timestamp of the .xvpics thumbnailfile does not matter here)
	 */
        gtk_widget_draw(fw->frame_preview, NULL);
        continue;
      }

      /* for the other exposed items we check the tumbnail timestamp */      
      if(fw->thumb_filename)
      {
	if (0 == stat(fw->thumb_filename, &l_stat_thumb))
	{
	   if(fw->thumb_timestamp < l_stat_thumb.st_mtime)
	   {
              gtk_widget_draw(fw->frame_preview, NULL);
	   }
	}
      }
   }
}


static void
navi_dialog_update(gint32 update_flag)
{
  gint32 l_first, l_last;
  gint   l_image_menu_was_changed;

  l_image_menu_was_changed = FALSE;
  if(update_flag & NUPD_IMAGE_MENU)
  {
    l_image_menu_was_changed = navi_refresh_image_menu();
  }
  if(update_flag & NUPD_FRAME_NR_CHANGED)
  {
     l_first = -1;
     l_last = -1;
     
     if(naviD->ainfo_ptr)
     {
       l_first = naviD->ainfo_ptr->first_frame_nr;
       l_last =  naviD->ainfo_ptr->last_frame_nr;
     }
     navi_reload_ainfo(naviD->active_imageid);
     navi_preview_extents();

     /* we must force a rebuild of the list of frame_widgets
      * if any frames were deleteted or are created 
      * (outside of the naviagator)
      */
     if(naviD->ainfo_ptr)
     {
        if((l_first != naviD->ainfo_ptr->first_frame_nr)
        || (l_last  != naviD->ainfo_ptr->last_frame_nr))
	{
          update_flag |= NUPD_PREV_LIST;
	}
     }
  }
  if(update_flag & NUPD_PREV_LIST)
  {
     frames_dialog_flush();
  }
  else
  {
     navid_thumb_timestamp_check();
  }
}


/* ------------------------
 * dialog helper procedures
 * ------------------------ 
 */
static void
navi_preview_extents (void)
{
  gint32 width, height;
  if (!naviD)
    return;

  naviD->gimage_width = gimp_image_width(naviD->active_imageid);
  naviD->gimage_height = gimp_image_height(naviD->active_imageid);

  /*  Get the image width and height variables, based on the gimage  */
  if (naviD->gimage_width > naviD->gimage_height)
  {
    naviD->ratio = (double) naviD->preview_size / (double) naviD->gimage_width;
  }
  else
  {
    naviD->ratio = (double) naviD->preview_size / (double) naviD->gimage_height;
  }

  if (naviD->preview_size > 0)
  {
    width = (int) (naviD->ratio * naviD->gimage_width);
    height = (int) (naviD->ratio * naviD->gimage_height);
    if (width < 1) width = 1;
    if (height < 1) height = 1;
  }
  else
  {
      width = 16;
      height = 10;
  }
  
  if((naviD->image_width != width)
  || (naviD->image_height != height))
  {
    naviD->image_width = width;
    naviD->image_height = height;
    gtk_preview_size (GTK_PREVIEW (naviD->frame_preview),
		     naviD->image_width, naviD->image_height);
  }

  naviD->item_height = 6 + naviD->image_height;
  
  if(gap_debug) printf("navi_preview_extents w: %d h:%d\n", (int)naviD->image_width, (int)naviD->image_height);
}

static void
navi_calc_frametiming(gint32 frame_nr, char *buf)
{
  gint32 first;
  gdouble msec_per_frame;
  gint32 tmsec;
  gint32 tms;
  gint32 tsec;
  gint32 tmin;
  
  first = frame_nr;
  if(naviD->ainfo_ptr)
  {
    first = naviD->ainfo_ptr->first_frame_nr;
  }

  if(naviD->vin_ptr == NULL)
  {
    sprintf(buf, "min:sec:msec");
    return;
  }
  
  if(naviD->vin_ptr->framerate < 1)
  {
    sprintf(buf, "min:sec:msec");
    return;
  }
  
  msec_per_frame = 1000.0 / naviD->vin_ptr->framerate;
  tmsec = (frame_nr - first) * msec_per_frame;
  
  tms = tmsec % 1000;
  tsec = (tmsec / 1000) % 60;
  tmin = tmsec / 60000;
  
  sprintf(buf, "%02d:%02d:%03d", (int)tmin, (int)tsec, (int)tms);
}

static void
frame_widget_time_label_update(FrameWidget *fw)
{
  char frame_nr_to_time[20];
  
  navi_calc_frametiming(fw->frame_nr, frame_nr_to_time);

  gtk_label_set (GTK_LABEL (fw->time_label), frame_nr_to_time);
}

static FrameWidget *
frame_widget_create (gint32 image_id, gint32 frame_nr)
{
  FrameWidget *frame_widget;
  GtkWidget *list_item;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *alignment;
  
  char frame_nr_to_char[20];
  char frame_nr_to_time[20];


  if(gap_debug) printf("frame_widget_create CREATE image_id:%d, nr:%d\n", (int)image_id, (int)frame_nr);

  list_item = gtk_list_item_new ();

  /*  create the frame widget and add it to the list  */
  frame_widget = g_new (FrameWidget, 1);
  frame_widget->image_id      = image_id;
  frame_widget->frame_nr      = frame_nr;
  frame_widget->frame_preview = NULL;
  frame_widget->frame_pixmap  = NULL;
  frame_widget->list_item     = list_item;
  frame_widget->width         = -1;
  frame_widget->height        = -1;
  frame_widget->thumb_timestamp = 0;
  frame_widget->thumb_filename = NULL;
  frame_widget->visited       = TRUE;
  /* frame_widget->drop_type     = GIMP_DROP_NONE; */
  
  sprintf(frame_nr_to_char, "%04d", (int)frame_nr);
  navi_calc_frametiming(frame_nr, frame_nr_to_time);
  

  /*  Need to let the list item know about the frame_widget  */
  gtk_object_set_user_data (GTK_OBJECT (list_item), frame_widget);

  /*  set up the list item observer  */
  gtk_signal_connect (GTK_OBJECT (list_item), "select",
		      (GtkSignalFunc) frame_widget_select_update,
		      frame_widget);
  gtk_signal_connect (GTK_OBJECT (list_item), "deselect",
		      (GtkSignalFunc) frame_widget_select_update,
		      frame_widget);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (list_item), vbox);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);

  /*  the frame number label */
  frame_widget->number_label = gtk_label_new (frame_nr_to_char);
  gtk_box_pack_start (GTK_BOX (hbox), frame_widget->number_label, FALSE, FALSE, 2);
  gtk_widget_show (frame_widget->number_label);

  /*  The frame preview  */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 2);
  gtk_widget_show (alignment);

  frame_widget->frame_preview = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (frame_widget->frame_preview),
			 naviD->image_width + 4, naviD->image_height + 4);
  gtk_widget_set_events (frame_widget->frame_preview, PREVIEW_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (frame_widget->frame_preview), "event",
		      (GtkSignalFunc) frame_widget_preview_events,
		      frame_widget);
  gtk_object_set_user_data (GTK_OBJECT (frame_widget->frame_preview), frame_widget);
  gtk_container_add (GTK_CONTAINER (alignment), frame_widget->frame_preview);
  gtk_widget_show (frame_widget->frame_preview);

  /*  the frame timing label */
  frame_widget->time_label = gtk_label_new (frame_nr_to_time);
  gtk_box_pack_start (GTK_BOX (hbox), frame_widget->time_label, FALSE, FALSE, 2);
  gtk_widget_show (frame_widget->time_label);

  frame_widget->clip_widget = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (frame_widget->clip_widget), 1, 2);
  gtk_widget_set_events (frame_widget->clip_widget, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (frame_widget->clip_widget), "event",
		      (GtkSignalFunc) frame_widget_button_events,
		      frame_widget);
  gtk_object_set_user_data (GTK_OBJECT (frame_widget->clip_widget), frame_widget);
  gtk_box_pack_start (GTK_BOX (vbox), frame_widget->clip_widget,
		      FALSE, FALSE, 0);
  /* gtk_widget_show (frame_widget->clip_widget); */


  gtk_object_set_data (GTK_OBJECT (list_item), "gap_framenumber", (gpointer)frame_nr );

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (list_item);

  gtk_widget_ref (frame_widget->list_item);

  if(gap_debug) printf("frame_widget_create END image_id:%d, nr:%d\n", (int)image_id, (int)frame_nr);

  return frame_widget;
}

static void
frame_widget_delete (FrameWidget *fw)
{
   if(gap_debug) printf("frame_widget_delete image_id:%d frame_nr:%d\n"
                        ,(int)fw->image_id
                        ,(int)fw->frame_nr
                        );
   if(fw->frame_pixmap)
   {
      gdk_pixmap_unref(fw->frame_pixmap);
   }

   if(fw->thumb_filename) g_free(fw->thumb_filename);
   
   /*  Remove the layer widget from the list  */
   naviD->frame_widgets = g_slist_remove (naviD->frame_widgets, fw);
 
   /*  Release the widget  */
   gtk_widget_unref(fw->list_item);
   g_free(fw);
}



GtkWidget *
navi_dialog_create (GtkWidget* shell, gint32 image_id)
{
  GtkWidget *vbox;
  GtkWidget *util_box;
  GtkWidget *button_box;
  GtkWidget *label;
  GtkWidget *slider;
  char  *l_basename;
  char frame_nr_to_char[20];

  if(gap_debug) printf("navi_dialog_create\n");
 
  if (naviD)
  {
    return naviD->vbox;
  }
  l_basename = NULL;
  sprintf(frame_nr_to_char, "0000 - 0000");
  naviD = g_new (NaviDialog, 1);
  naviD->waiting_cursor = FALSE;
  naviD->cursor_wait = gdk_cursor_new (GDK_WATCH);
  naviD->cursor_acitve = gdk_cursor_new (GDK_TOP_LEFT_ARROW);
  naviD->shell = shell;
  naviD->frame_preview  = NULL;
  naviD->OpenFrameImagesList  = NULL;
  naviD->OpenFrameImagesCount  = 0;
  naviD->any_imageid = -1;
  naviD->frame_widgets  = NULL;
  naviD->cycle_time   = 1000;  /* polling cylcle of 1 sec */
  naviD->timer        = 0;
  naviD->active_imageid = image_id;
/*  naviD->ainfo_ptr  = navi_get_ainfo(naviD->active_imageid, NULL); */
  naviD->ainfo_ptr  = NULL;
  naviD->framerange_number_label = NULL;
  navi_reload_ainfo_force(image_id);
  if(naviD->ainfo_ptr != NULL)
  {
    sprintf(frame_nr_to_char, "%04d - %04d"
            , (int)naviD->ainfo_ptr->first_frame_nr
            , (int)naviD->ainfo_ptr->last_frame_nr);
    l_basename = naviD->ainfo_ptr->basename;
  }
  naviD->vin_ptr  = p_get_video_info(l_basename);
  naviD->image_width = 0;
  naviD->image_height = 0;
  naviD->preview_size = navi_get_preview_size();

  if (naviD->preview_size > 0)
  {
      naviD->frame_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
      navi_preview_extents ();
  }

  /* creates tooltips */
  gimp_help_init ();
  
  /*  The main vbox  */
  naviD->vbox = gtk_event_box_new ();

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (naviD->vbox), vbox);

  /*  The image menu  */
  util_box = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), util_box, FALSE, FALSE, 0);

  naviD->image_option_menu = gtk_option_menu_new();
  naviD->image_menu = NULL;
  navi_refresh_image_menu();
  gtk_box_pack_start (GTK_BOX (util_box), naviD->image_option_menu, FALSE, FALSE, 0);
  gtk_widget_show (naviD->image_option_menu);
  gtk_widget_show (naviD->image_menu);
  gtk_widget_show (util_box);

  /*  the Framerange label */
  util_box = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), util_box, FALSE, FALSE, 0);
  
  label = gtk_label_new (_("Videoframes:"));
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  /*  the max frame number label */
  naviD->framerange_number_label = gtk_label_new (frame_nr_to_char);
  gtk_box_pack_start (GTK_BOX (util_box), naviD->framerange_number_label, FALSE, FALSE, 2);
  gtk_widget_show (naviD->framerange_number_label);
  gtk_widget_show (util_box);


  /*  framerate scale  */
  naviD->framerate_box = util_box = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), util_box, FALSE, FALSE, 0);

  label = gtk_label_new (_("Framerate:"));
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  naviD->framerate_data =
    GTK_ADJUSTMENT (gtk_adjustment_new (naviD->vin_ptr->framerate, 1.0, 100.0, 1.0, 1.0, 0.0));
  slider = gtk_hscale_new (naviD->framerate_data);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED); /* GTK_UPDATE_CONTINUOUS */
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_RIGHT);
  gtk_box_pack_start (GTK_BOX (util_box), slider, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (naviD->framerate_data), "value_changed",
		      (GtkSignalFunc) navi_framerate_scale_update,
		      naviD);
  gtk_widget_show (slider);

  gimp_help_set_help_data (slider, NULL, "#framerate_scale");

  gtk_widget_show (util_box);

  /*  timezoom scale  */
  naviD->timezoom_box = util_box = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), util_box, FALSE, FALSE, 0);

  label = gtk_label_new (_("Timezoom:"));
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  naviD->timezoom_data =
    GTK_ADJUSTMENT (gtk_adjustment_new ((gdouble)naviD->vin_ptr->timezoom, 1.0, 100.0, 1.0, 1.0, 0.0));
  slider = gtk_hscale_new (naviD->timezoom_data);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_RIGHT);
  gtk_box_pack_start (GTK_BOX (util_box), slider, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (naviD->timezoom_data), "value_changed",
		      (GtkSignalFunc) navi_timezoom_scale_update,
		      naviD);
  gtk_widget_show (slider);

  gimp_help_set_help_data (slider, NULL, "#timezoom_scale");

  gtk_widget_show (util_box);

  /*  The frames listbox  */
  naviD->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (naviD->scrolled_win), 
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_widget_set_usize (naviD->scrolled_win, LIST_WIDTH, LIST_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), naviD->scrolled_win, TRUE, TRUE, 2);

  naviD->frame_list = gtk_list_new ();
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (naviD->scrolled_win),
					 naviD->frame_list);
  gtk_list_set_selection_mode (GTK_LIST (naviD->frame_list),
			       GTK_SELECTION_EXTENDED);  /* GTK_SELECTION_BROWSE */
  gtk_signal_connect (GTK_OBJECT (naviD->frame_list), "event",
		      (GtkSignalFunc) frame_list_events, 
		      naviD);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (naviD->frame_list), 
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (naviD->scrolled_win)));
  GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (naviD->scrolled_win)->vscrollbar,
			  GTK_CAN_FOCUS);
      
  gtk_widget_show (naviD->frame_list);
  gtk_widget_show (naviD->scrolled_win);

  /*  The ops buttons  */
  button_box = ops_button_box_new (naviD->shell,
				   frames_ops_buttons, OPS_BUTTON_NORMAL);
  gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 2);
  gtk_widget_show (button_box);


  /*  The VCR ops buttons  */
  button_box = ops_button_box_new (naviD->shell,
				   vcr_ops_buttons, OPS_BUTTON_NORMAL);
  gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 2);
  gtk_widget_show (button_box);

  gtk_widget_show (vbox);
  gtk_widget_show (naviD->vbox);

  if(gap_debug) printf("navi_dialog_create END\n");

  return naviD->vbox;
}


/* ---------------------------------
 * the navigator MAIN dialog
 * ---------------------------------
 */
int  gap_navigator(gint32 image_id)
{
  GtkWidget     *shell;
  GtkWidget *button;
  GtkWidget *subshell;
  gint argc = 1;
  guchar     *color_cube;
  gchar **argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("gap_navigator");

  if(gap_debug) fprintf(stderr, "\nSTARTing gap_navigator_dialog\n");

  /* Init GTK  */
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gdk_set_use_xshm (gimp_use_xshm ());
  gtk_preview_set_gamma(gimp_gamma());
  gtk_preview_set_install_cmap(gimp_install_cmap());
  color_cube = gimp_color_cube();
  gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2], color_cube[3]);
/*   gtk_widget_set_default_visual (gtk_preview_get_visual ());
 */
  gtk_widget_set_default_colormap(gtk_preview_get_cmap());

  /*  The main shell */
  shell = gimp_dialog_new (_("Video Navigator"), "gap_navigator",
			   gimp_plugin_help_func, "filters/gap_navigator_dialog.html",
			   GTK_WIN_POS_NONE,
			   FALSE, TRUE, FALSE,
			   NULL);
  
  gtk_signal_connect (GTK_OBJECT (shell), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), 
		      NULL);
  
  gtk_signal_connect (GTK_OBJECT (shell), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), 
		      NULL);

  /*  The subshell (toplevel vbox)  */
  subshell = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell)->vbox), subshell);
  
  if(gap_debug) printf("BEFORE  navi_dialog_create\n");  

  /*  The naviD dialog structure  */
  navi_dialog_create (shell, image_id);
  if(gap_debug) printf("AFTER  navi_dialog_create\n");  
 
  gtk_box_pack_start (GTK_BOX (subshell), naviD->vbox, TRUE, TRUE, 0);

  /*  The action area  */
  gtk_container_set_border_width
    (GTK_CONTAINER (GTK_DIALOG (shell)->action_area), 1);

  /*  The close button */
  button = gtk_button_new_with_label (_("Close"));
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (shell));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (shell)->action_area),
                      button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gtk_widget_show (subshell); 
  gtk_widget_show (shell);
  
  frames_dialog_flush();
  navi_scroll_to_current_frame_nr();
  
  naviD->timer = gtk_timeout_add(naviD->cycle_time,
                                (GtkFunction)navi_dialog_poll, NULL);

   
  if(gap_debug) printf("BEFORE  gtk_main\n");  
  gtk_main ();
  gdk_flush ();

  if(gap_debug) printf("END gap_navigator_dialog\n");  
 
  return 0;
}








/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 * Some Code Parts copied from gimp/app directory
 * XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 */


/* ---------------------------------------  start copy of gimp-1.1.14/app/fileops.c readXVThumb */
/* The readXVThumb function source may be re-used under
   the XFree86-style license. <adam@gimp.org> */
guchar*
readXVThumb (const gchar  *fnam,
	     gint         *w,
	     gint         *h,
	     gchar       **imginfo /* caller frees if != NULL */)
{
  FILE *fp;
  const gchar *P7_332 = "P7 332";
  gchar P7_buf[7];
  gchar linebuf[200];
  guchar *buf;
  gint twofivefive;
  void *ptr;

  *w = *h = 0;
  *imginfo = NULL;

  fp = fopen (fnam, "rb");
  if (!fp)
    return (NULL);

  fread (P7_buf, 6, 1, fp);

  if (strncmp(P7_buf, P7_332, 6)!=0)
    {
      g_warning("Thumbnail doesn't have the 'P7 332' header.");
      fclose(fp);
      return(NULL);
    }

  /*newline*/
  fread (P7_buf, 1, 1, fp);

  do
    {
      ptr = fgets(linebuf, 199, fp);
      if ((strncmp(linebuf, "#IMGINFO:", 9) == 0) &&
	  (linebuf[9] != '\0') &&
	  (linebuf[9] != '\n'))
	{
	  if (linebuf[strlen(linebuf)-1] == '\n')
	    linebuf[strlen(linebuf)-1] = '\0';

	  if (linebuf[9] != '\0')
	    {
	      if (*imginfo)
		g_free(*imginfo);
	      *imginfo = g_strdup (&linebuf[9]);
	    }
	}
    }
  while (ptr && linebuf[0]=='#'); /* keep throwing away comment lines */

  if (!ptr)
    {
      /* g_warning("Thumbnail ended - not an image?"); */
      fclose(fp);
      return(NULL);
    }

  sscanf(linebuf, "%d %d %d\n", w, h, &twofivefive);

  if (twofivefive!=255)
    {
      g_warning("Thumbnail is of funky depth.");
      fclose(fp);
      return(NULL);
    }

  if ((*w)<1 || (*h)<1 || (*w)>80 || (*h)>60)
    {
      g_warning ("Thumbnail size bad.  Corrupted?");
      fclose(fp);
      return (NULL);
    }

  buf = g_malloc((*w)*(*h));

  fread(buf, (*w)*(*h), 1, fp);
  
  fclose(fp);
  
  return(buf);
}
/* ---------------------------------------  end copy of  gimp-1.1.14/app/fileops.c readXVThumb */


/* ---------------------------------------  start copy of gimp-1.1.14/app/ops_buttons.c */
static void ops_button_pressed_callback  (GtkWidget*, GdkEventButton*, gpointer);
static void ops_button_extended_callback (GtkWidget*, gpointer);


GtkWidget *
ops_button_box_new (GtkWidget     *parent,
		    OpsButton     *ops_button,
		    OpsButtonType  ops_type)   
{
  GtkWidget *button;
  GtkWidget *button_box;
  GtkWidget *pixmap_widget;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle  *style;
  GSList    *group = NULL;
  
  gtk_widget_realize (parent);
  style = gtk_widget_get_style (parent);

  button_box = gtk_hbox_new (TRUE, 1);

  while (ops_button->xpm_data)
    {
      pixmap = gdk_pixmap_create_from_xpm_d (parent->window,
					     &mask,
					     &style->bg[GTK_STATE_NORMAL],
					     ops_button->xpm_data);
      pixmap_widget = gtk_pixmap_new (pixmap, mask);

      switch (ops_type)
	{
	case OPS_BUTTON_NORMAL :
	  button = gtk_button_new ();
	  break;
	case OPS_BUTTON_RADIO :
	  button = gtk_radio_button_new (group);
	  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
	  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
	  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
	  break;
	default :
	  button = NULL; /*stop compiler complaints */
	  g_error ("ops_button_box_new: unknown type %d\n", ops_type);
	  break;
	}

      gtk_container_add (GTK_CONTAINER (button), pixmap_widget);
      
      if (ops_button->ext_callbacks == NULL)
	{
	  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				     (GtkSignalFunc) ops_button->callback,
				     NULL);
	}
      else
	{
	  gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
			      (GtkSignalFunc) ops_button_pressed_callback,
			      ops_button);	  
	  gtk_signal_connect (GTK_OBJECT (button), "clicked",
			      (GtkSignalFunc) ops_button_extended_callback,
			      ops_button);
	}

      gimp_help_set_help_data (button,
			       gettext (ops_button->tooltip),
			       ops_button->private_tip);

      gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0); 

      gtk_widget_show (pixmap_widget);
      gtk_widget_show (button);

      ops_button->widget = button;
      ops_button->modifier = OPS_BUTTON_MODIFIER_NONE;

      ops_button++;
    }

  return (button_box);
}

static void
ops_button_pressed_callback (GtkWidget      *widget, 
			     GdkEventButton *bevent,
			     gpointer        client_data)
{
  OpsButton *ops_button;

  g_return_if_fail (client_data != NULL);
  ops_button = (OpsButton*)client_data;

  if (bevent->state & GDK_SHIFT_MASK)
    {
      if (bevent->state & GDK_CONTROL_MASK)
	  ops_button->modifier = OPS_BUTTON_MODIFIER_SHIFT_CTRL;
      else 
	ops_button->modifier = OPS_BUTTON_MODIFIER_SHIFT;
    }
  else if (bevent->state & GDK_CONTROL_MASK)
    ops_button->modifier = OPS_BUTTON_MODIFIER_CTRL;
  else if (bevent->state & GDK_MOD1_MASK)
    ops_button->modifier = OPS_BUTTON_MODIFIER_ALT;
  else 
    ops_button->modifier = OPS_BUTTON_MODIFIER_NONE;
}

static void
ops_button_extended_callback (GtkWidget *widget, 
			      gpointer   client_data)
{
  OpsButton *ops_button;

  g_return_if_fail (client_data != NULL);
  ops_button = (OpsButton*)client_data;

  if (ops_button->modifier > OPS_BUTTON_MODIFIER_NONE &&
      ops_button->modifier < OPS_BUTTON_MODIFIER_LAST)
    {
      if (ops_button->ext_callbacks[ops_button->modifier - 1] != NULL)
	(ops_button->ext_callbacks[ops_button->modifier - 1]) (widget, NULL);
      else
	(ops_button->callback) (widget, NULL);
    } 
  else 
    (ops_button->callback) (widget, NULL);

  ops_button->modifier = OPS_BUTTON_MODIFIER_NONE;
}
/* ---------------------------------------  end copy of gimp-1.1.14/app/ops_buttons.c */
