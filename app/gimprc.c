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
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <glib.h>

#include "app_procs.h"
#include "appenv.h"
#include "devices.h"
#include "errors.h"
#include "fileops.h"
#include "general.h"
#include "gimprc.h"
#include "menus.h"
#include "plug_in.h"
#include "gimage.h"
#include "session.h"
#include "tools.h"
#include "gimpparasite.h"

#include "libgimp/parasite.h"
#include "libgimp/gimpintl.h"
#include "libgimp/gimpenv.h"

#define ERROR  0
#define DONE   1
#define OK     2

typedef enum {
  TT_STRING,
  TT_PATH,
  TT_DOUBLE,
  TT_FLOAT,
  TT_INT,
  TT_BOOLEAN,
  TT_POSITION,
  TT_MEMSIZE,
  TT_IMAGETYPE,
  TT_INTERP,
  TT_XCOLORCUBE,
  TT_XPREVSIZE,
  TT_XUNIT,
  TT_XPLUGIN,
  TT_XPLUGINDEF,
  TT_XMENUPATH,
  TT_XDEVICE,
  TT_XSESSIONINFO,
  TT_XUNITINFO,
  TT_XPARASITE,
  TT_XNAVPREVSIZE
} TokenType;

typedef struct _ParseFunc ParseFunc;

struct _ParseFunc
{
  char *name;
  TokenType type;
  gpointer val1p;
  gpointer val2p;
};

typedef struct _UnknownToken UnknownToken;

struct _UnknownToken
{
  char *token;
  char *value;
};


/*  global gimprc variables  */
char *    plug_in_path = NULL;
char *    temp_path = NULL;
char *    swap_path = NULL;
char *    brush_path = NULL;
char *    brush_vbr_path = NULL;
char *    default_brush = NULL;
char *    pattern_path = NULL;
char *    default_pattern = NULL;
char *    palette_path = NULL;
char *    default_palette = NULL;
char *    gradient_path = NULL;
char *    default_gradient = NULL;
char *    pluginrc_path = NULL;
char *    module_path = NULL;
int       tile_cache_size = 10485760;  /* 10 MB */
int       marching_speed = 300;   /* 300 ms */
double    gamma_val = 1.0;
int       transparency_type = 1;  /* Mid-Tone Checks */
int       perfectmouse = 0;       /* off (fast and sloppy) */
int       transparency_size = 1;  /* Medium sized */
int       levels_of_undo = 2;     /* 2 levels of undo default */
int       last_opened_size = 4;   /* 4 documents in the MRU list */
int       color_cube_shades[4] = {6, 7, 4, 24};
int       install_cmap = 0;
int       cycled_marching_ants = 0;
int       default_threshold = 15;
int       stingy_memory_use = 0;
int       allow_resize_windows = 0;
int       no_cursor_updating = 0;
int       preview_size = 32;
int       nav_preview_size = 112;
int       show_rulers = TRUE;
int       show_statusbar = TRUE;
GUnit     default_units = UNIT_INCH;
int       auto_save = TRUE;
InterpolationType interpolation_type = LINEAR_INTERPOLATION;
int       confirm_on_close = TRUE;
int       save_session_info = TRUE;
int       save_device_status = FALSE;
int       always_restore_session = FALSE;
int       default_width = 983;
int       default_height = 857;
/* temporary default size increase to stress test stuff.  -- austin 7/11/99 */
int       default_type = RGB;
double    default_xresolution = 72.0;
double    default_yresolution = 72.0;
GUnit     default_resolution_units = UNIT_INCH;
int       show_tips = TRUE;
int       last_tip = -1;
int       show_tool_tips = TRUE;
double    monitor_xres = 72.0;
double    monitor_yres = 72.0;
int       using_xserver_resolution = FALSE;
int       num_processors = 1;
char *    image_title_format = NULL;
int       global_paint_options = TRUE;
int       show_indicators = TRUE;
int       max_new_image_size = 33554432;  /* 32 MB */
int       thumbnail_mode = 1;
int	  trust_dirty_flag = FALSE;
int       use_help = TRUE;

extern char * module_db_load_inhibit;

static int get_next_token (void);
static int peek_next_token (void);
static int parse_statement (void);

static int parse_string (gpointer val1p, gpointer val2p);
static int parse_path (gpointer val1p, gpointer val2p);
static int parse_double (gpointer val1p, gpointer val2p);
static int parse_float (gpointer val1p, gpointer val2p);
static int parse_int (gpointer val1p, gpointer val2p);
static int parse_boolean (gpointer val1p, gpointer val2p);
static int parse_position (gpointer val1p, gpointer val2p);
static int parse_mem_size (gpointer val1p, gpointer val2p);
static int parse_image_type (gpointer val1p, gpointer val2p);
static int parse_interpolation_type (gpointer val1p, gpointer val2p);
static int parse_color_cube (gpointer val1p, gpointer val2p);
static int parse_preview_size (gpointer val1p, gpointer val2p);
static int parse_nav_preview_size (gpointer val1p, gpointer val2p);
static int parse_units (gpointer val1p, gpointer val2p);
static int parse_plug_in (gpointer val1p, gpointer val2p);
static int parse_plug_in_def (gpointer val1p, gpointer val2p);
static int parse_device (gpointer val1p, gpointer val2p);
static int parse_menu_path (gpointer val1p, gpointer val2p);
static int parse_session_info (gpointer val1p, gpointer val2p);
static int parse_unit_info (gpointer val1p, gpointer val2p);
static int parse_parasite (gpointer val1p, gpointer val2p);

static int parse_proc_def (PlugInProcDef **proc_def);
static int parse_proc_arg (ProcArg *arg);
static int parse_unknown (char *token_sym);

char* gimprc_value_to_str (char *name);
static char* value_to_str (char *name);

static inline char* string_to_str (gpointer val1p, gpointer val2p);
static inline char* path_to_str (gpointer val1p, gpointer val2p);
static inline char* double_to_str (gpointer val1p, gpointer val2p);
static inline char* float_to_str (gpointer val1p, gpointer val2p);
static inline char* int_to_str (gpointer val1p, gpointer val2p);
static inline char* boolean_to_str (gpointer val1p, gpointer val2p);
static inline char* position_to_str (gpointer val1p, gpointer val2p);
static inline char* mem_size_to_str (gpointer val1p, gpointer val2p);
static inline char* image_type_to_str (gpointer val1p, gpointer val2p);
static inline char* interpolation_type_to_str (gpointer val1p, gpointer val2p);
static inline char* color_cube_to_str (gpointer val1p, gpointer val2p);
static inline char* preview_size_to_str (gpointer val1p, gpointer val2p);
static inline char* nav_preview_size_to_str (gpointer val1p, gpointer val2p);
static inline char* units_to_str (gpointer val1p, gpointer val2p);

static char* transform_path (char *path, int destroy);
static void gimprc_set_token (char *token, char *value);
static void add_gimp_directory_token (char *gimp_dir);
#ifdef __EMX__
static void add_x11root_token (char *x11root);
#endif
static char* open_backup_file (char *filename,
			       char *secondary_filename,
			       char **name_used,
			       FILE **fp_new,
			       FILE **fp_old);

static ParseInfo parse_info;

static GList *unknown_tokens = NULL;

static int cur_token;
static int next_token;
static int done;

static ParseFunc funcs[] =
{
  { "temp-path",                 TT_PATH,       &temp_path, NULL },
  { "swap-path",                 TT_PATH,       &swap_path, NULL },
  { "brush-path",                TT_PATH,       &brush_path, NULL },
  { "brush-vbr-path",            TT_PATH,       &brush_vbr_path, NULL },
  { "pattern-path",              TT_PATH,       &pattern_path, NULL },
  { "plug-in-path",              TT_PATH,       &plug_in_path, NULL },
  { "palette-path",              TT_PATH,       &palette_path, NULL },
  { "gradient-path",             TT_PATH,       &gradient_path, NULL },
  { "pluginrc-path",             TT_PATH,       &pluginrc_path, NULL },
  { "module-path",               TT_PATH,       &module_path, NULL },
  { "default-brush",             TT_STRING,     &default_brush, NULL },
  { "default-pattern",           TT_STRING,     &default_pattern, NULL },
  { "default-palette",           TT_STRING,     &default_palette, NULL },
  { "default-gradient",          TT_STRING,     &default_gradient, NULL },
  { "gamma-correction",          TT_DOUBLE,     &gamma_val, NULL },
  { "color-cube",                TT_XCOLORCUBE, NULL, NULL },
  { "tile-cache-size",           TT_MEMSIZE,    &tile_cache_size, NULL },
  { "marching-ants-speed",       TT_INT,        &marching_speed, NULL },
  { "last-opened-size",          TT_INT,        &last_opened_size, NULL },
  { "undo-levels",               TT_INT,        &levels_of_undo, NULL },
  { "transparency-type",         TT_INT,        &transparency_type, NULL },
  { "perfect-mouse",             TT_BOOLEAN,    &perfectmouse, NULL },
  { "transparency-size",         TT_INT,        &transparency_size, NULL },
  { "install-colormap",          TT_BOOLEAN,    &install_cmap, NULL },
  { "colormap-cycling",          TT_BOOLEAN,    &cycled_marching_ants, NULL },
  { "default-threshold",         TT_INT,        &default_threshold, NULL },
  { "stingy-memory-use",         TT_BOOLEAN,    &stingy_memory_use, NULL },
  { "allow-resize-windows",      TT_BOOLEAN,    &allow_resize_windows, NULL },
  { "dont-allow-resize-windows", TT_BOOLEAN,    NULL, &allow_resize_windows },
  { "cursor-updating",           TT_BOOLEAN,    NULL, &no_cursor_updating },
  { "no-cursor-updating",        TT_BOOLEAN,    &no_cursor_updating, NULL },
  { "preview-size",              TT_XPREVSIZE,  NULL, NULL },
  { "nav-preview-size",          TT_XNAVPREVSIZE,  NULL, NULL },
  { "show-rulers",               TT_BOOLEAN,    &show_rulers, NULL },
  { "dont-show-rulers",          TT_BOOLEAN,    NULL, &show_rulers },
  { "show-statusbar",            TT_BOOLEAN,    &show_statusbar, NULL },
  { "dont-show-statusbar",       TT_BOOLEAN,    NULL, &show_statusbar },
  { "default-units",             TT_XUNIT,      &default_units, NULL },
  { "auto-save",                 TT_BOOLEAN,    &auto_save, NULL },
  { "dont-auto-save",            TT_BOOLEAN,    NULL, &auto_save },
  { "interpolation-type",        TT_INTERP,     &interpolation_type, NULL },
  { "confirm-on-close",          TT_BOOLEAN,    &confirm_on_close, NULL },
  { "dont-confirm-on-close",     TT_BOOLEAN,    NULL, &confirm_on_close },
  { "save-session-info",         TT_BOOLEAN,    &save_session_info, NULL },
  { "dont-save-session-info",    TT_BOOLEAN,    NULL, &save_session_info},
  { "save-device-status",        TT_BOOLEAN,    &save_device_status, NULL },
  { "dont-save-device-status",   TT_BOOLEAN,    NULL, &save_device_status},
  { "always-restore-session",    TT_BOOLEAN,    &always_restore_session, NULL },
  { "show-tips",                 TT_BOOLEAN,    &show_tips, NULL },
  { "dont-show-tips",            TT_BOOLEAN,    NULL, &show_tips },
  { "last-tip-shown",            TT_INT,        &last_tip, NULL },
  { "show-tool-tips",            TT_BOOLEAN,    &show_tool_tips, NULL },
  { "dont-show-tool-tips",       TT_BOOLEAN,    NULL, &show_tool_tips },
  { "default-image-size",        TT_POSITION, &default_width, &default_height },
  { "default-image-type",        TT_IMAGETYPE,  &default_type, NULL },
  { "default-xresolution",       TT_DOUBLE,     &default_xresolution, NULL },
  { "default-yresolution",       TT_DOUBLE,     &default_yresolution, NULL },
  { "default-resolution-units",  TT_XUNIT,    &default_resolution_units, NULL },
  { "plug-in",                   TT_XPLUGIN,    NULL, NULL },
  { "plug-in-def",               TT_XPLUGINDEF, NULL, NULL },
  { "menu-path",                 TT_XMENUPATH,  NULL, NULL },
  { "device",                    TT_XDEVICE,    NULL, NULL },
  { "session-info",              TT_XSESSIONINFO, NULL, NULL },
  { "unit-info",                 TT_XUNITINFO,  NULL, NULL },
  { "monitor-xresolution",       TT_DOUBLE,     &monitor_xres, NULL },
  { "monitor-yresolution",       TT_DOUBLE,     &monitor_yres, NULL },
  { "num-processors",            TT_INT,        &num_processors, NULL },
  { "image-title-format",        TT_STRING,     &image_title_format, NULL },
  { "parasite",                  TT_XPARASITE,  NULL, NULL },
  { "global-paint-options",      TT_BOOLEAN,    &global_paint_options, NULL },
  { "show-indicators",           TT_BOOLEAN,    &show_indicators, NULL },
  { "dont-show-indicators",      TT_BOOLEAN,    NULL, &show_indicators },
  { "no-global-paint-options",   TT_BOOLEAN,    NULL, &global_paint_options },
  { "module-load-inhibit",       TT_PATH,       &module_db_load_inhibit, NULL },
  { "max-new-image-size",        TT_MEMSIZE,    &max_new_image_size, NULL },
  { "thumbnail-mode",            TT_INT,        &thumbnail_mode, NULL },
  { "trust-dirty-flag",		 TT_BOOLEAN,	&trust_dirty_flag, NULL },
  { "dont-trust-dirty-flag",     TT_BOOLEAN,	NULL, &trust_dirty_flag },
  { "use-help",                  TT_BOOLEAN,    &use_help, NULL },
  { "dont-use-help",             TT_BOOLEAN,    NULL, &use_help }
};
static int nfuncs = sizeof (funcs) / sizeof (funcs[0]);

static SessionInfo *session_infos[] =
{
  &toolbox_session_info,
  &lc_dialog_session_info,
  &info_dialog_session_info,
  &tool_options_session_info,
  &palette_session_info,
  &brush_select_session_info,
  &pattern_select_session_info,
  &gradient_select_session_info,
  &device_status_session_info,
  &error_console_session_info
};
static int nsession_infos = sizeof (session_infos) / sizeof (session_infos[0]);

extern char* alternate_gimprc;
extern char* alternate_system_gimprc;

#define DEFAULT_IMAGE_TITLE_FORMAT "%f-%p.%i (%t)"

static char *
gimp_system_rc_file (void)
{
  static char *value = NULL;
  if (value != NULL)
    return value;

  value = g_strconcat (gimp_data_directory (), G_DIR_SEPARATOR_S,
		       "gimprc", NULL);
  return value;
}

void
parse_buffers_init (void)
{
  parse_info.buffer = g_new (char, 4096);
  parse_info.tokenbuf = parse_info.buffer + 2048;
  parse_info.buffer_size = 2048;
  parse_info.tokenbuf_size = 2048;
}

static GList *parse_add_directory_tokens (void)
{
  char *gimp_dir;

  gimp_dir = gimp_directory ();
  add_gimp_directory_token (gimp_dir);
#ifdef __EMX__
  add_x11root_token(getenv("X11ROOT"));
#endif
  /* the real output is unknown_tokens list !  */
  return (unknown_tokens); 
}

void
parse_gimprc (void)
{
  char libfilename[MAXPATHLEN];
  char filename[MAXPATHLEN];
  gchar *personal_gimprc;
  
  parse_add_directory_tokens ();

  strcpy (libfilename, gimp_system_rc_file ());
  if (alternate_system_gimprc != NULL) 
    strncpy (libfilename, alternate_system_gimprc, MAXPATHLEN);
  app_init_update_status(_("Resource configuration"), libfilename, -1);
  parse_gimprc_file (libfilename);

  if (alternate_gimprc != NULL) 
    strncpy (filename, alternate_gimprc, MAXPATHLEN);
  else 
    {
      personal_gimprc = gimp_personal_rc_file ("gimprc");
      strncpy (filename, personal_gimprc, MAXPATHLEN);
      g_free (personal_gimprc);
    }

  if (filename[0] != '\0' && strcmp (filename, libfilename) != 0)
    {
      app_init_update_status(NULL, filename, -1);
      parse_gimprc_file (filename);
    }

  if (!image_title_format)
    image_title_format = g_strdup(DEFAULT_IMAGE_TITLE_FORMAT);
}

gboolean
parse_absolute_gimprc_file (char *filename)
{
  int status;

  parse_info.fp = fopen (filename, "rt");
  if (!parse_info.fp)
    return FALSE;

  if ((be_verbose == TRUE) || (no_splash == TRUE))
    g_print (_("parsing \"%s\"\n"), filename);

  cur_token = -1;
  next_token = -1;

  parse_info.position = -1;
  parse_info.linenum = 1;
  parse_info.charnum = 1;
  parse_info.inc_linenum = FALSE;
  parse_info.inc_charnum = FALSE;

  done = FALSE;
  while ((status = parse_statement ()) == OK)
    ;

  fclose (parse_info.fp);

  if (status == ERROR)
    {
      g_print (_("error parsing: \"%s\"\n"), filename);
      g_print (_("  at line %d column %d\n"), parse_info.linenum, parse_info.charnum);
      g_print (_("  unexpected token: %s\n"), token_sym);

      return FALSE;
    }

  return TRUE;
}

gboolean
parse_gimprc_file (char *filename)
{
  char rfilename[MAXPATHLEN];

  if (!g_path_is_absolute (filename))
    {
      if (g_get_home_dir () != NULL)
	{
	  g_snprintf (rfilename, sizeof (rfilename),
		      "%s" G_DIR_SEPARATOR_S "%s",
		      g_get_home_dir (), filename);
	  filename = rfilename;
	}
    }

  return (parse_absolute_gimprc_file (filename));
}

static GList *
g_list_findstr (GList *list,
		char  *str)
{
  for (; list; list = g_list_next (list))
    {
      if (! strcmp ((char *) list->data, str))
        break;
    }

  return list;
}

void
save_gimprc_strings (gchar *token,
		     gchar *value)
{
  gchar timestamp[40];  /* variables for parsing and updating gimprc */
  gchar *name;
  FILE *fp_new;
  FILE *fp_old;
  gchar *cur_line;
  gchar *prev_line;
  gchar *error_msg;
  gboolean found = FALSE;
  gchar *personal_gimprc;
  
  UnknownToken *ut;    /* variables to modify unknown_tokens */
  UnknownToken *tmp;
  GList *list;

  
  g_assert(token != NULL);
  g_assert(value != NULL);
  
  /* get the name of the backup file, and the file pointers.  'name'
     is reused in another context later, disregard it here */
  personal_gimprc = gimp_personal_rc_file ("gimprc");
  error_msg = open_backup_file (personal_gimprc,
				gimp_system_rc_file (),
				&name, &fp_new, &fp_old);
  g_free (personal_gimprc);

  if (error_msg != NULL)
    {
      g_message (error_msg);
      g_free (error_msg);
      return;
    }

  strcpy (timestamp, "by GIMP on ");
  iso_8601_date_format (timestamp + strlen (timestamp), FALSE);

  /* copy the old .gimprc into the new one, modifying it as needed */
  prev_line = NULL;
  cur_line = g_new (char, 1024);
  while (!feof (fp_old))
    {
      if (!fgets (cur_line, 1024, fp_old))
	continue;
      
      /* special case: save lines starting with '#-' (added by GIMP) */
      if ((cur_line[0] == '#') && (cur_line[1] == '-'))
	{
	  if (prev_line != NULL)
	    {
	      fputs (prev_line, fp_new);
	      g_free (prev_line);
	    }
	  prev_line = g_strdup (cur_line);
	  continue;
	}

      /* see if the line contains something that we can use 
         and place that into 'name' if its found */
      if (find_token (cur_line, name, 50))
	{
	  /* check if that entry should be updated */
	  if (!g_strcasecmp(token, name)) /* if they match */
	    {
	      if (prev_line == NULL)
		{
		  fprintf (fp_new, "#- Next line commented out %s\n",
			   timestamp);
		  fprintf (fp_new, "# %s\n", cur_line);
		  fprintf (fp_new, "#- Next line added %s\n",
			   timestamp);
		}
	      else
		{
		  g_free (prev_line);
		  prev_line = NULL;
		  fprintf (fp_new, "#- Next line modified %s\n",
			   timestamp);
		}
              if (!found)
                {
		  fprintf (fp_new, "(%s \"%s\")\n", token, value);
		}
              else 
		fprintf (fp_new, "#- (%s \"%s\")\n", token, value);
	      found = TRUE;
	      continue;
	    } /* end if token and name match */
	} /* end if token is found */
    
      /* all lines that did not match the tests above are simply copied */
      if (prev_line != NULL)
	{
	  fputs (prev_line, fp_new);
	  g_free (prev_line);
	  prev_line = NULL;
	}
      fputs (cur_line, fp_new);
    } /* end of while(!feof) */

  g_free (cur_line);
  if (prev_line != NULL)
    g_free (prev_line);
  fclose (fp_old);

  /* append the options that were not in the old .gimprc */
  if (!found) 
    {
      fprintf (fp_new, "#- Next line added %s\n",
	       timestamp);
      fprintf (fp_new, "(%s \"%s\")\n\n", token, value);
    }

  /* update unknown_tokens to reflect new token value */
  ut = g_new (UnknownToken, 1);
  ut->token = g_strdup (token);
  ut->value = g_strdup (value);

  list = unknown_tokens;
  while (list)
    {
      tmp = (UnknownToken *) list->data;
      list = list->next;

      if (strcmp (tmp->token, ut->token) == 0)
	{
	  unknown_tokens = g_list_remove (unknown_tokens, tmp);
	  g_free (tmp->token);
	  g_free (tmp->value);
	  g_free (tmp);
	}
    }
  unknown_tokens = g_list_append (unknown_tokens, ut);

  fclose (fp_new);
}

void
save_gimprc (GList **updated_options,
	     GList **conflicting_options)
{
  char timestamp[40];
  char *name;
  FILE *fp_new;
  FILE *fp_old;
  GList *option;
  char *cur_line;
  char *prev_line;
  char *str;
  gchar *error_msg;
  gchar *personal_gimprc;

  g_assert(updated_options != NULL);
  g_assert(conflicting_options != NULL);

  personal_gimprc = gimp_personal_rc_file ("gimprc");
  error_msg = open_backup_file (personal_gimprc,
				gimp_system_rc_file (),
				&name, &fp_new, &fp_old);
  g_free (personal_gimprc);

  if (error_msg != NULL)
    {
      g_message (error_msg);
      g_free (error_msg);
      return;
    }

  strcpy (timestamp, "by GIMP on ");
  iso_8601_date_format (timestamp + strlen (timestamp), FALSE);

  /* copy the old .gimprc into the new one, modifying it as needed */
  prev_line = NULL;
  cur_line = g_new (char, 1024);
  while (!feof (fp_old))
    {
      if (!fgets (cur_line, 1024, fp_old))
	continue;
      
      /* special case: save lines starting with '#-' (added by GIMP) */
      if ((cur_line[0] == '#') && (cur_line[1] == '-'))
	{
	  if (prev_line != NULL)
	    {
	      fputs (prev_line, fp_new);
	      g_free (prev_line);
	    }
	  prev_line = g_strdup (cur_line);
	  continue;
	}

      /* see if the line contains something that we can use */
      if (find_token (cur_line, name, 50))
	{
	  /* check if that entry should be updated */
	  option = g_list_findstr (*updated_options, name);
	  if (option != NULL)
	    {
	      if (prev_line == NULL)
		{
		  fprintf (fp_new, "#- Next line commented out %s\n",
			   timestamp);
		  fprintf (fp_new, "# %s\n", cur_line);
		  fprintf (fp_new, "#- Next line added %s\n",
			   timestamp);
		}
	      else
		{
		  g_free (prev_line);
		  prev_line = NULL;
		  fprintf (fp_new, "#- Next line modified %s\n",
			   timestamp);
		}
	      str = value_to_str (name);
	      fprintf (fp_new, "(%s %s)\n", name, str);
	      g_free (str);

	      *updated_options = g_list_remove_link (*updated_options, option);
	      *conflicting_options = g_list_append (*conflicting_options,
						    (gpointer) option->data);
	      g_list_free_1 (option);
	      continue;
	    }

	  /* check if that entry should be commented out */
	  option = g_list_findstr (*conflicting_options, name);
	  if (option != NULL)
	    {
	      if (prev_line != NULL)
		{
		  g_free (prev_line);
		  prev_line = NULL;
		}
	      fprintf (fp_new, "#- Next line commented out %s\n",
		       timestamp);
	      fprintf (fp_new, "# %s\n", cur_line);
	      continue;
	    }
	}

      /* all lines that did not match the tests above are simply copied */
      if (prev_line != NULL)
	{
	  fputs (prev_line, fp_new);
	  g_free (prev_line);
	  prev_line = NULL;
	}
      fputs (cur_line, fp_new);

    }

  g_free (cur_line);
  if (prev_line != NULL)
    g_free (prev_line);
  fclose (fp_old);

  /* append the options that were not in the old .gimprc */
  option = *updated_options;
  while (option)
    {
      fprintf (fp_new, "#- Next line added %s\n",
	       timestamp);
      str = value_to_str ((char *) option->data);
      fprintf (fp_new, "(%s %s)\n\n", (char *) option->data, str);
      g_free (str);
      option = option->next;
    }
  fclose (fp_new);
}

static int
get_next_token (void)
{
  if (next_token != -1)
    {
      cur_token = next_token;
      next_token = -1;
    }
  else
    {
      cur_token = get_token (&parse_info);
    }

  return cur_token;
}

static int
peek_next_token (void)
{
  if (next_token == -1)
    next_token = get_token (&parse_info);

  return next_token;
}

static int
parse_statement (void)
{
  int token;
  int i;

  token = peek_next_token ();
  if (!token)
    return DONE;
  if (token != TOKEN_LEFT_PAREN)
    return ERROR;
  token = get_next_token ();

  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL))
    return ERROR;
  token = get_next_token ();

  for (i = 0; i < nfuncs; i++)
    if (strcmp (funcs[i].name, token_sym) == 0)
      switch (funcs[i].type)
	{
	case TT_STRING:
	  return parse_string (funcs[i].val1p, funcs[i].val2p);
	case TT_PATH:
	  return parse_path (funcs[i].val1p, funcs[i].val2p);
	case TT_DOUBLE:
	  return parse_double (funcs[i].val1p, funcs[i].val2p);
	case TT_FLOAT:
	  return parse_float (funcs[i].val1p, funcs[i].val2p);
	case TT_INT:
	  return parse_int (funcs[i].val1p, funcs[i].val2p);
	case TT_BOOLEAN:
	  return parse_boolean (funcs[i].val1p, funcs[i].val2p);
	case TT_POSITION:
	  return parse_position (funcs[i].val1p, funcs[i].val2p);
	case TT_MEMSIZE:
	  return parse_mem_size (funcs[i].val1p, funcs[i].val2p);
	case TT_IMAGETYPE:
	  return parse_image_type (funcs[i].val1p, funcs[i].val2p);
	case TT_INTERP:
	  return parse_interpolation_type (funcs[i].val1p, funcs[i].val2p);
	case TT_XCOLORCUBE:
	  return parse_color_cube (funcs[i].val1p, funcs[i].val2p);
	case TT_XPREVSIZE:
	  return parse_preview_size (funcs[i].val1p, funcs[i].val2p);
	case TT_XNAVPREVSIZE:
	  return parse_nav_preview_size (funcs[i].val1p, funcs[i].val2p);
	case TT_XUNIT:
	  return parse_units (funcs[i].val1p, funcs[i].val2p);
	case TT_XPLUGIN:
	  return parse_plug_in (funcs[i].val1p, funcs[i].val2p);
	case TT_XPLUGINDEF:
	  return parse_plug_in_def (funcs[i].val1p, funcs[i].val2p);
	case TT_XMENUPATH:
	  return parse_menu_path (funcs[i].val1p, funcs[i].val2p);
	case TT_XDEVICE:
	  return parse_device (funcs[i].val1p, funcs[i].val2p);
	case TT_XSESSIONINFO:
	  return parse_session_info (funcs[i].val1p, funcs[i].val2p);
	case TT_XUNITINFO:
	  return parse_unit_info (funcs[i].val1p, funcs[i].val2p);
	case TT_XPARASITE:
	  return parse_parasite (funcs[i].val1p, funcs[i].val2p);
	}

  return parse_unknown (token_sym);
}

static int
parse_path (gpointer val1p,
	    gpointer val2p)
{
  int token;
  char **pathp;

  g_assert (val1p != NULL);
  pathp = (char **)val1p;

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    return ERROR;
  token = get_next_token ();

  if (*pathp)
    g_free (*pathp);
  *pathp = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    {
      g_free (*pathp);
      *pathp = NULL;
      return ERROR;
    }
  token = get_next_token ();

  *pathp = transform_path (*pathp, TRUE);

  return OK;
}

static int
parse_string (gpointer val1p,
	      gpointer val2p)
{
  int token;
  char **strp;

  g_assert (val1p != NULL);
  strp = (char **)val1p;

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    return ERROR;
  token = get_next_token ();

  if (*strp)
    g_free (*strp);
  *strp = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    {
      g_free (*strp);
      *strp = NULL;
      return ERROR;
    }
  token = get_next_token ();

  return OK;
}

static int
parse_double (gpointer val1p,
	      gpointer val2p)
{
  int token;
  double *nump;

  g_assert (val1p != NULL);
  nump = (double *)val1p;

  token = peek_next_token ();
  if (!token || (token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  *nump = token_num;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static int
parse_float (gpointer val1p,
	     gpointer val2p)
{
  int token;
  float *nump;

  g_assert (val1p != NULL);
  nump = (float *)val1p;

  token = peek_next_token ();
  if (!token || (token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  *nump = token_num;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static int
parse_int (gpointer val1p,
	   gpointer val2p)
{
  int token;
  int *nump;

  g_assert (val1p != NULL);
  nump = (int *)val1p;

  token = peek_next_token ();
  if (!token || (token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  *nump = token_num;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static int
parse_boolean (gpointer val1p,
	       gpointer val2p)
{
  int token;
  int *boolp;

  /* The variable to be set should be passed in the first or second
   * pointer.  If the pointer is in val2p, then the opposite value is
   * stored in the pointer.  This is useful for "dont-xxx" or "no-xxx"
   * type of options.
   * If the expression to be parsed is written as "(option)" instead
   * of "(option yes)" or "(option no)", then the default value is
   * TRUE if the variable is passed in val1p, or FALSE if in val2p.
   */
  g_assert (val1p != NULL || val2p != NULL);
  if (val1p != NULL)
    boolp = (int *)val1p;
  else
    boolp = (int *)val2p;

  token = peek_next_token ();
  if (!token)
    return ERROR;
  switch (token)
    {
    case TOKEN_RIGHT_PAREN:
      *boolp = TRUE;
      break;
    case TOKEN_NUMBER:
      token = get_next_token ();
      *boolp = token_num;
      token = peek_next_token ();
      if (!token || (token != TOKEN_RIGHT_PAREN))
	return ERROR;
      break;
    case TOKEN_SYMBOL:
      token = get_next_token ();
      if (!strcmp (token_sym, "true") || !strcmp (token_sym, "on")
	  || !strcmp (token_sym, "yes"))
	*boolp = TRUE;
      else if (!strcmp (token_sym, "false") || !strcmp (token_sym, "off")
	       || !strcmp (token_sym, "no"))
	*boolp = FALSE;
      else
	return ERROR;
      token = peek_next_token ();
      if (!token || (token != TOKEN_RIGHT_PAREN))
	return ERROR;
      break;
    default:
      return ERROR;
    }

  if (val1p == NULL)
    *boolp = !*boolp;

  token = get_next_token ();

  return OK;
}

static int
parse_position (gpointer val1p,
		gpointer val2p)
{
  int token;
  int *xp;
  int *yp;

  g_assert (val1p != NULL && val2p != NULL);
  xp = (int *)val1p;
  yp = (int *)val2p;

  token = peek_next_token ();
  if (!token || (token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  *xp = token_num;

  token = peek_next_token ();
  if (!token || (token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  *yp = token_num;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static int
parse_mem_size (gpointer val1p,
		gpointer val2p)
{
  int mult;
  int suffix;
  int token;
  int *sizep;

  g_assert (val1p != NULL);
  sizep = (int *)val1p;

  token = peek_next_token ();
  if (!token || ((token != TOKEN_NUMBER) &&
		 (token != TOKEN_SYMBOL)))
    return ERROR;
  token = get_next_token ();

  if (token == TOKEN_NUMBER)
    {
      *sizep = token_num * 1024;
    }
  else
    {
      *sizep = atoi (token_sym);

      suffix = token_sym[strlen (token_sym) - 1];
      if ((suffix == 'm') || (suffix == 'M'))
	mult = 1024 * 1024;
      else if ((suffix == 'k') || (suffix == 'K'))
	mult = 1024;
      else if ((suffix == 'b') || (suffix == 'B'))
	mult = 1;
      else
	return FALSE;

      *sizep *= mult;
    }

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static int
parse_image_type (gpointer val1p,
		  gpointer val2p)
{
  int token;
  int *typep;
  
  g_assert (val1p != NULL);
  typep = (int *)val1p;

  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL))
    return ERROR;
  token = get_next_token ();
 
  if (!strcmp (token_sym, "rgb"))
    *typep = RGB;
  else if ((!strcmp (token_sym, "gray")) || (!strcmp (token_sym, "grey")))
    *typep = GRAY;
  else
    return ERROR;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static int
parse_interpolation_type (gpointer val1p,
			  gpointer val2p)
{
  int token;
  InterpolationType *typep;
  
  g_assert (val1p != NULL);
  typep = (InterpolationType *)val1p;

  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL))
    return ERROR;
  token = get_next_token ();
 
  if (strcmp (token_sym, "nearest-neighbor") == 0)
    *typep = NEAREST_NEIGHBOR_INTERPOLATION;
  else if (strcmp (token_sym, "linear") == 0)
    *typep = LINEAR_INTERPOLATION;
  else if (strcmp (token_sym, "cubic") == 0)
    *typep = CUBIC_INTERPOLATION;
  else
    return ERROR;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static int
parse_color_cube (gpointer val1p,
		  gpointer val2p)
{
  int token;

  token = peek_next_token ();
  if (!token || (token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  color_cube_shades[0] = token_num;

  token = peek_next_token ();
  if (!token || (token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  color_cube_shades[1] = token_num;

  token = peek_next_token ();
  if (!token || (token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  color_cube_shades[2] = token_num;

  token = peek_next_token ();
  if (!token || (token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  color_cube_shades[3] = token_num;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static int
parse_preview_size (gpointer val1p,
		    gpointer val2p)
{
  int token;

  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL && token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  if (token == TOKEN_SYMBOL)
    {
      if (strcmp (token_sym, "none") == 0)
	preview_size = 0;
      else if (strcmp (token_sym, "tiny") == 0)
	preview_size = 24;
      else if (strcmp (token_sym, "small") == 0)
	preview_size = 32;
      else if (strcmp (token_sym, "medium") == 0)
	preview_size = 48;
      else if (strcmp (token_sym, "large") == 0)
	preview_size = 64;
      else if (strcmp (token_sym, "huge") == 0)
	preview_size = 128;
      else
	preview_size = 0;
    }
  else if (token == TOKEN_NUMBER)
    preview_size = token_num;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static int
parse_nav_preview_size (gpointer val1p,
			gpointer val2p)
{
  int token;

  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL && token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  if (token == TOKEN_SYMBOL)
    {
/*       if (strcmp (token_sym, "none") == 0) */
/* 	preview_size = 0; */
/*       else  */
      if (strcmp (token_sym, "small") == 0)
	nav_preview_size = 48;
      else if (strcmp (token_sym, "medium") == 0)
	nav_preview_size = 80;
      else if (strcmp (token_sym, "large") == 0)
	nav_preview_size = 112;
      else
	nav_preview_size = 112;
    }
  else if (token == TOKEN_NUMBER)
    nav_preview_size = token_num;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static int
parse_units (gpointer val1p,
	     gpointer val2p)
{
  int token;
  int i;

  g_assert (val1p != NULL);

  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL))
    return ERROR;
  token = get_next_token ();

  *((GUnit *) val1p) = UNIT_INCH;
  for (i = UNIT_INCH; i < gimp_unit_get_number_of_units (); i++)
    if (strcmp (token_sym, gimp_unit_get_identifier (i)) == 0)
      {
	*((GUnit *) val1p) = i;
	break;
      }

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static int
parse_plug_in (gpointer val1p,
	       gpointer val2p)
{
  char *name;
  char *menu_path;
  char *accelerator;
  int token;

  name = NULL;
  menu_path = NULL;
  accelerator = NULL;

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    return ERROR;
  token = get_next_token ();

  name = g_strdup (token_str);

  token = peek_next_token ();
  if (token == TOKEN_STRING)
    {
      menu_path = g_strdup (token_str);
      token = get_next_token ();
    }

  token = peek_next_token ();
  if (token == TOKEN_STRING)
    {
      accelerator = g_strdup (token_str);
      token = get_next_token ();
    }

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    {
      g_free (name);
      g_free (menu_path);
      g_free (accelerator);
      return ERROR;
    }
  token = get_next_token ();

  if (name && menu_path)
    plug_in_add (name, menu_path, accelerator);

  return OK;
}

static int
parse_plug_in_def (gpointer val1p,
		   gpointer val2p)
{
  PlugInDef *plug_in_def;
  PlugInProcDef *proc_def;
  GSList *tmp_list;
  int token;

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    return ERROR;
  token = get_next_token ();

  plug_in_def = g_new (PlugInDef, 1);
  plug_in_def->prog = g_strdup (token_str);
  plug_in_def->proc_defs = NULL;
  plug_in_def->mtime = 0;
  plug_in_def->query = FALSE;

  token = peek_next_token ();
  if (!token || (token != TOKEN_NUMBER))
    goto error;
  token = get_next_token ();

  plug_in_def->mtime = token_int;

  while (parse_proc_def (&proc_def))
    {
      proc_def->mtime = plug_in_def->mtime;
      proc_def->prog = g_strdup (plug_in_def->prog);
      plug_in_def->proc_defs = g_slist_append (plug_in_def->proc_defs, proc_def);
    }

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    goto error;
  token = get_next_token ();

  plug_in_def_add (plug_in_def);

  return OK;

 error:
  g_message (_("error parsing pluginrc"));
  tmp_list = plug_in_def->proc_defs;
  while (tmp_list)
    {
      g_free (tmp_list->data);
      tmp_list = tmp_list->next;
    }
  g_slist_free (plug_in_def->proc_defs);
  g_free (plug_in_def->prog);
  g_free (plug_in_def);

  return ERROR;
}

static int
parse_proc_def (PlugInProcDef **proc_def)
{
  PlugInProcDef *pd;
  int token;
  int i;

  token = peek_next_token ();
  if (!token || (token != TOKEN_LEFT_PAREN))
    return ERROR;
  token = get_next_token ();

  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL) ||
      (strcmp ("proc-def", token_sym) != 0))
    return ERROR;
  token = get_next_token ();

  pd = g_new0 (PlugInProcDef, 1);

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();

  pd->db_info.name = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_NUMBER))
    goto error;
  token = get_next_token ();

  pd->db_info.proc_type = token_int;

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();

  pd->db_info.blurb = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();

  pd->db_info.help = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();

  pd->db_info.author = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();

  pd->db_info.copyright = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();

  pd->db_info.date = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();

  if (token_str[0] != '\0')
    pd->menu_path = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();

  if (token_str[0] != '\0')
    pd->extensions = g_strdup (token_str);
  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();
  if (token_str[0] != '\0')
    pd->prefixes = g_strdup (token_str);
  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();
  if (token_str[0] != '\0')
    pd->magics = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();

  if (token_str[0] != '\0')
    {
      pd->image_types = g_strdup (token_str);
      pd->image_types_val = plug_in_image_types_parse (token_str);
    }

  token = peek_next_token ();
  if (!token || (token != TOKEN_NUMBER))
    goto error;
  token = get_next_token ();

  pd->db_info.num_args = token_int;
  pd->db_info.args = g_new0 (ProcArg, pd->db_info.num_args);

  token = peek_next_token ();
  if (!token || (token != TOKEN_NUMBER))
    goto error;
  token = get_next_token ();

  pd->db_info.num_values = token_int;
  pd->db_info.values = NULL;
  if (pd->db_info.num_values > 0)
    pd->db_info.values = g_new (ProcArg, pd->db_info.num_values);

  for (i = 0; i < pd->db_info.num_args; i++)
    if (!parse_proc_arg (&pd->db_info.args[i]))
      goto error;

  for (i = 0; i < pd->db_info.num_values; i++)
    if (!parse_proc_arg (&pd->db_info.values[i]))
      goto error;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    goto error;
  token = get_next_token ();

  *proc_def = pd;
  return OK;

 error:
  g_free (pd->db_info.name);
  g_free (pd->db_info.blurb);
  g_free (pd->db_info.help);
  g_free (pd->db_info.author);
  g_free (pd->db_info.copyright);
  g_free (pd->db_info.date);
  g_free (pd->menu_path);
  g_free (pd->extensions);
  g_free (pd->prefixes);
  g_free (pd->magics);
  g_free (pd->image_types);

  for (i = 0; i < pd->db_info.num_args; i++)
    {
      g_free (pd->db_info.args[i].name);
      g_free (pd->db_info.args[i].description);
    }

  for (i = 0; i < pd->db_info.num_values; i++)
    {
      g_free (pd->db_info.values[i].name);
      g_free (pd->db_info.values[i].description);
    }

  g_free (pd->db_info.args);
  g_free (pd->db_info.values);
  g_free (pd);
  return ERROR;
}

static int
parse_proc_arg (ProcArg *arg)
{
  int token;

  arg->arg_type = -1;
  arg->name = NULL;
  arg->description = NULL;

  token = peek_next_token ();
  if (!token || (token != TOKEN_LEFT_PAREN))
    return ERROR;
  token = get_next_token ();

  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL) ||
      (strcmp ("proc-arg", token_sym) != 0))
    return ERROR;
  token = get_next_token ();

  token = peek_next_token ();
  if (!token || (token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  arg->arg_type = token_int;

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    return ERROR;
  token = get_next_token ();

  arg->name = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();

  arg->description = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    goto error;
  token = get_next_token ();

  return OK;

 error:
  g_free (arg->name);
  g_free (arg->description);

  return ERROR;
}

static int
parse_menu_path (gpointer val1p,
		 gpointer val2p)
{
  char *menu_path;
  char *accelerator;
  int token;

  menu_path = NULL;
  accelerator = NULL;

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();

  menu_path = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();

  accelerator = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    goto error;
  token = get_next_token ();

  return OK;

 error:
  g_free (menu_path);
  g_free (accelerator);

  return ERROR;
}

static char*
transform_path (char *path,
		int   destroy)
{
  int length;
  char *new_path;
  char *home;
  char *token;
  char *tmp;
  char *tmp2;
  int  substituted;
  int  is_env;
  UnknownToken *ut;

  home = g_get_home_dir ();
  length = 0;
  substituted = FALSE;
  is_env = FALSE;

  tmp = path;
  while (*tmp)
    {
      if (*tmp == '~')
	{
	  length += strlen (home);
	  tmp += 1;
	}
      else if (*tmp == '$')
	{
	  tmp += 1;
	  if (!*tmp || (*tmp != '{'))
	    return path;
	  tmp += 1;

	  token = tmp;
	  while (*tmp && (*tmp != '}'))
	    tmp += 1;

	  if (!*tmp)
	    return path;

	  *tmp = '\0';

	  tmp2 = gimprc_find_token (token);
	  if (tmp2 == NULL) 
	    {
	      /* maybe token is an environment variable */
	      tmp2 = g_getenv (token);
	      if (tmp2 != NULL)
		{
		  is_env = TRUE;
		}
	      else
		{
		  gimp_terminate (_("transform_path(): gimprc token referenced but not defined: %s"), token);
		}
	    }
	  tmp2 = transform_path (tmp2, FALSE);
	  if (is_env)
	    {
	      /* then add to list of unknown tokens */
	      /* but only if it isn't already in list */
	      if (gimprc_find_token (token) == NULL)
		{
		  ut = g_new (UnknownToken, 1);
		  ut->token = g_strdup (token);
		  ut->value = g_strdup (tmp2);
		  unknown_tokens = g_list_append (unknown_tokens, ut);
		}
	    }
	  else
	    {
	      gimprc_set_token (token, tmp2);
	    }
	  length += strlen (tmp2);

	  *tmp = '}';
	  tmp += 1;

	  substituted = TRUE;
	}
      else
	{
	  length += 1;
	  tmp += 1;
	}
    }

  if ((length == strlen (path)) && !substituted)
    return path;

  new_path = g_new (char, length + 1);

  tmp = path;
  tmp2 = new_path;

  while (*tmp)
    {
      if (*tmp == '~')
	{
	  *tmp2 = '\0';
	  strcat (tmp2, home);
	  tmp2 += strlen (home);
	  tmp += 1;
	}
      else if (*tmp == '$')
	{
	  tmp += 1;
	  if (!*tmp || (*tmp != '{'))
	    {
	      g_free (new_path);
	      return path;
	    }
	  tmp += 1;

	  token = tmp;
	  while (*tmp && (*tmp != '}'))
	    tmp += 1;

	  if (!*tmp)
	    {
	      g_free (new_path);
	      return path;
	    }

	  *tmp = '\0';
	  token = gimprc_find_token (token);
	  *tmp = '}';

	  *tmp2 = '\0';
	  strcat (tmp2, token);
	  tmp2 += strlen (token);
	  tmp += 1;
	}
      else
	{
	  *tmp2++ = *tmp++;
	}
    }

  *tmp2 = '\0';

  if (destroy)
    g_free (path);

  return new_path;
}

/* Copied from gtk_menu_factory_parse_accelerator() */
static void
parse_device_accelerator (const char   *accelerator,
			  GdkDeviceKey *key)
{
  int done;

  g_return_if_fail (accelerator != NULL);
  g_return_if_fail (key != NULL);

  key->modifiers = 0;

  done = FALSE;
  while (!done)
    {
      if (strncmp (accelerator, "<shift>", 7) == 0)
        {
          accelerator += 7;
          key->modifiers |= GDK_SHIFT_MASK;
        }
      else if (strncmp (accelerator, "<alt>", 5) == 0)
        {
          accelerator += 5;
          key->modifiers |= GDK_MOD1_MASK;
        }
      else if (strncmp (accelerator, "<control>", 9) == 0)
        {
          accelerator += 9;
          key->modifiers |= GDK_CONTROL_MASK;
        }
      else
        {
          done = TRUE;
	  /* Tricky, but works... ("" => keyval = 0, or no translation) */
          key->keyval = accelerator[0];
        }
    }
}

static gint 
parse_device (gpointer val1p, 
	      gpointer val2p)
{
  DeviceValues values = 0;
  gint i;
  gint token;
  
  /* The initialized values here are meaningless */
  gchar *name        = NULL;
  GdkInputMode mode  = GDK_MODE_DISABLED;
  gint num_axes      = 0;
  GdkAxisUse *axes   = NULL;
  gint num_keys      = 0;
  GdkDeviceKey *keys = NULL;

  ToolType tool        = RECT_SELECT;
  guchar foreground[3] = { 0, 0, 0 };
  guchar background[3] = { 0, 0, 0 };
  gchar *brush_name    = NULL;
  gchar *pattern_name  = NULL;
  gchar *gradient_name = NULL;

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    goto error;
  token = get_next_token ();

  name = g_strdup (token_str);

  /* Parse options for device */

  while (peek_next_token () == TOKEN_LEFT_PAREN)
    {
      token = get_next_token ();

      token = peek_next_token ();
      if (!token || (token != TOKEN_SYMBOL))
	goto error;
      token = get_next_token ();

      if (!strcmp ("mode", token_sym))
	{
	  values |= DEVICE_MODE;

	  token = peek_next_token ();
	  if (!token || (token != TOKEN_SYMBOL))
	    goto error;
	  token = get_next_token ();

	  if (!strcmp ("disabled", token_sym))
	    mode = GDK_MODE_DISABLED;
	  else if (!strcmp ("window", token_sym))
	    mode = GDK_MODE_WINDOW;
	  else if (!strcmp ("screen", token_sym))
	    mode = GDK_MODE_SCREEN;
	  else
	    goto error;
	}
      else if (!strcmp ("axes", token_sym))
	{
	  values |= DEVICE_AXES;

	  token = peek_next_token ();
	  if (!token || (token != TOKEN_NUMBER))
	    goto error;
	  token = get_next_token ();

	  num_axes = token_int;
	  axes = g_new (GdkAxisUse, num_axes);

	  for (i = 0; i < num_axes; i++)
	    {
	      token = peek_next_token ();
	      if (!token || (token != TOKEN_SYMBOL))
		goto error;
	      token = get_next_token ();

	      if (!strcmp ("ignore", token_sym))
		axes[i] = GDK_AXIS_IGNORE;
	      else if (!strcmp ("x", token_sym))
		axes[i] = GDK_AXIS_X;
	      else if (!strcmp ("y", token_sym))
		axes[i] = GDK_AXIS_Y;
	      else if (!strcmp ("pressure", token_sym))
		axes[i] = GDK_AXIS_PRESSURE;
	      else if (!strcmp ("xtilt", token_sym))
		axes[i] = GDK_AXIS_XTILT;
	      else if (!strcmp ("ytilt", token_sym))
		axes[i] = GDK_AXIS_YTILT;
#ifdef GTK_HAVE_SIX_VALUATORS
              else if (!strcmp ("wheel", token_sym))
               axes[i] = GDK_AXIS_WHEEL;
#endif /* GTK_HAVE_SIX_VALUATORS */
	      else
		goto error;
	    }
	}
      else if (!strcmp ("keys", token_sym))
	{
	  values |= DEVICE_KEYS;

	  token = peek_next_token ();
	  if (!token || (token != TOKEN_NUMBER))
	    goto error;
	  token = get_next_token ();

	  num_keys = token_int;
	  keys = g_new (GdkDeviceKey, num_keys);

	  for (i=0; i<num_keys; i++)
	    {
	      token = peek_next_token ();
	      if (!token || (token != TOKEN_STRING))
		goto error;
	      token = get_next_token ();

	      parse_device_accelerator (token_str, &keys[i]);
	    }
	}
      else if (!strcmp ("tool", token_sym))
	{
	  values |= DEVICE_TOOL;

	  token = peek_next_token ();
	  if (!token || (token != TOKEN_STRING))
	    goto error;
	  token = get_next_token ();

	  /* FIXME: this shouldn't be hard coded like this */
	  for (tool = FIRST_TOOLBOX_TOOL; tool <= LAST_TOOLBOX_TOOL; tool++)
	    {
	      if (!strcmp (tool_info[tool].tool_name, token_str))
		break;
	    }
	  if (tool > LAST_TOOLBOX_TOOL)
	    goto error;
	}
      else if (!strcmp ("foreground", token_sym))
	{
	  values |= DEVICE_FOREGROUND;

	  for (i = 0; i < 3; i++)
	    {
	      token = peek_next_token ();
	      if (!token || (token != TOKEN_NUMBER))
		goto error;
	      token = get_next_token ();

	      foreground[i] = token_int;
	    }
	}
      else if (!strcmp ("background", token_sym))
	{
	  values |= DEVICE_BACKGROUND;

	  for (i = 0; i < 3; i++)
	    {
	      token = peek_next_token ();
	      if (!token || (token != TOKEN_NUMBER))
		goto error;
	      token = get_next_token ();

	      background[i] = token_int;
	    }
	}
      else if (!strcmp ("brush", token_sym))
	{
	  values |= DEVICE_BRUSH;

	  token = peek_next_token ();
	  if (!token || (token != TOKEN_STRING))
	    goto error;
	  token = get_next_token ();

	  brush_name = g_strdup (token_str);
	}
      else if (!strcmp ("pattern", token_sym))
	{
	  values |= DEVICE_PATTERN;

	  token = peek_next_token ();
	  if (!token || (token != TOKEN_STRING))
	    goto error;
	  token = get_next_token ();

	  pattern_name = g_strdup (token_str);
	}
      else if (!strcmp ("gradient", token_sym))
	{
	  values |= DEVICE_GRADIENT;

	  token = peek_next_token ();
	  if (!token || (token != TOKEN_STRING))
	    goto error;
	  token = get_next_token ();

	  gradient_name = g_strdup (token_str);
	}
      else
	goto error;
      
      token = peek_next_token ();
      if (!token || (token != TOKEN_RIGHT_PAREN))
	goto error;
      token = get_next_token ();
    }

  if (!token || (token != TOKEN_RIGHT_PAREN))
    goto error;
  token = get_next_token ();

  devices_rc_update (name, values, mode, num_axes, axes, num_keys, keys,
		     tool,
		     foreground,
		     background,
		     brush_name,
		     pattern_name,
		     gradient_name);

  g_free (brush_name);
  g_free (pattern_name);
  g_free (gradient_name);
  g_free (name);
  g_free (axes);
  g_free (keys);

  return OK;

 error:
  g_free (brush_name);
  g_free (pattern_name);
  g_free (gradient_name);
  g_free (name);
  g_free (axes);
  g_free (keys);

  return ERROR;
}

static int
parse_session_info (gpointer val1p, 
		    gpointer val2p)
{
  int i;
  int token;
  SessionInfo *info = NULL;

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    return ERROR;
  token = get_next_token ();

  for (i = 0; i < nsession_infos; i++)
    { 
      if (strcmp (session_infos[i]->name, token_str) == 0)
	info = session_infos[i];
    }

  if (info == NULL)
    return ERROR;

  /* Parse options for session info */

  while (peek_next_token () == TOKEN_LEFT_PAREN)
    {
      token = get_next_token ();

      token = peek_next_token ();
      if (!token || (token != TOKEN_SYMBOL))
	return ERROR;
      token = get_next_token ();

      if (!strcmp ("position", token_sym))
	{
	  token = peek_next_token ();
	  if (!token || (token != TOKEN_NUMBER))
	    return ERROR;
	  token = get_next_token ();
	  info->x = token_int;

	  token = peek_next_token ();
	  if (!token || (token != TOKEN_NUMBER))
	    return ERROR;
	  token = get_next_token ();
	  info->y = token_int;
	}
      else if (!strcmp ("size", token_sym))
	{
	  token = peek_next_token ();
	  if (!token || (token != TOKEN_NUMBER))
	    return ERROR;
	  token = get_next_token ();
	  info->width = token_int;

	  token = peek_next_token ();
	  if (!token || (token != TOKEN_NUMBER))
	    return ERROR;
	  token = get_next_token ();
	  info->height = token_int;
	}
      else if (!strcmp ("open-on-exit", token_sym))
	  info->open = TRUE;
      else
	return ERROR;
      
      token = peek_next_token ();
      if (!token || (token != TOKEN_RIGHT_PAREN))
	return ERROR;
      token = get_next_token ();
    }

  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  session_info_updates = g_list_append (session_info_updates, info);

  return OK;
}

static int
parse_unit_info (gpointer val1p, 
		 gpointer val2p)
{
  gint token;

  GUnit unit;

  gchar   *identifier   = NULL;
  gdouble  factor       = 1.0;
  gint     digits       = 2.0;
  gchar   *symbol       = NULL;
  gchar   *abbreviation = NULL;
  gchar   *singular     = NULL;
  gchar   *plural       = NULL;

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    return ERROR;
  token = get_next_token ();
  identifier = g_strdup (token_str);

  /* Parse options for unit info */

  while (peek_next_token () == TOKEN_LEFT_PAREN)
    {
      token = get_next_token ();

      token = peek_next_token ();
      if (!token || (token != TOKEN_SYMBOL))
	goto error;
      token = get_next_token ();

      if (!strcmp ("factor", token_sym))
	{
	  token = peek_next_token ();
	  if (!token || (token != TOKEN_NUMBER))
	    goto error;
	  token = get_next_token ();
	  factor = token_num;
	}
      else if (!strcmp ("digits", token_sym))
	{
	  token = peek_next_token ();
	  if (!token || (token != TOKEN_NUMBER))
	    goto error;
	  token = get_next_token ();
	  digits = token_int;
	}
      else if (!strcmp ("symbol", token_sym))
	{
	  token = peek_next_token ();
	  if (!token || (token != TOKEN_STRING))
	    goto error;
	  token = get_next_token ();
	  symbol = g_strdup (token_str);
	}
      else if (!strcmp ("abbreviation", token_sym))
	{
	  token = peek_next_token ();
	  if (!token || (token != TOKEN_STRING))
	    goto error;
	  token = get_next_token ();
	  abbreviation = g_strdup (token_str);
	}
      else if (!strcmp ("singular", token_sym))
	{
	  token = peek_next_token ();
	  if (!token || (token != TOKEN_STRING))
	    goto error;
	  token = get_next_token ();
	  singular = g_strdup (token_str);
	}
      else if (!strcmp ("plural", token_sym))
	{
	  token = peek_next_token ();
	  if (!token || (token != TOKEN_STRING))
	    goto error;
	  token = get_next_token ();
	  plural = g_strdup (token_str);
	}
      else
	goto error;
      
      token = peek_next_token ();
      if (!token || (token != TOKEN_RIGHT_PAREN))
	goto error;
      token = get_next_token ();
    }

  if (!token || (token != TOKEN_RIGHT_PAREN))
    goto error;
  token = get_next_token ();

  unit = gimp_unit_new (identifier, factor, digits,
			symbol, abbreviation, singular, plural);
  /*  make the unit definition persistent  */
  gimp_unit_set_deletion_flag (unit, FALSE);

  g_free (identifier);
  g_free (symbol);
  g_free (abbreviation);
  g_free (singular);
  g_free (plural);

  return OK;

 error:
  g_free (identifier);
  g_free (symbol);
  g_free (abbreviation);
  g_free (singular);
  g_free (plural);

  return ERROR;
}

static int
parse_parasite  (gpointer val1p, 
		 gpointer val2p)
{
  int token;
  int res = ERROR;
  gchar *identifier = NULL;
  gulong flags = 0;

  token = get_next_token ();
  if (token != TOKEN_STRING)
    goto error;

  identifier = g_strdup (token_str);

  token = get_next_token ();

  /* possible future extension: allow flags as symbolic strings.  */
  if (token == TOKEN_NUMBER)
    flags |= token_int;

  token = get_next_token ();
  if (token != TOKEN_STRING)
    goto error;

  gimp_parasite_attach (parasite_new (identifier, flags, token_int, token_str));

  token = get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    goto error;

  res = OK;

error:
  g_free (identifier);
  return res;
}

static int
parse_unknown (char *token_sym)
{
  int token;
  UnknownToken *ut, *tmp;
  GList *list;

  ut = g_new (UnknownToken, 1);
  ut->token = g_strdup (token_sym);

  token = peek_next_token ();
  if (!token || (token != TOKEN_STRING))
    {
      g_free (ut->token);
      g_free (ut);
      return ERROR;
    }
  token = get_next_token ();

  ut->value = g_strdup (token_str);

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    {
      g_free (ut->token);
      g_free (ut->value);
      g_free (ut);
      return ERROR;
    }
  token = get_next_token ();

  /*  search through the list of unknown tokens and replace an existing entry  */
  list = unknown_tokens;
  while (list)
    {
      tmp = (UnknownToken *) list->data;
      list = list->next;

      if (strcmp (tmp->token, ut->token) == 0)
	{
	  unknown_tokens = g_list_remove (unknown_tokens, tmp);
	  g_free (tmp->token);
	  g_free (tmp->value);
	  g_free (tmp);
	}
    }

  ut->value = transform_path (ut->value, TRUE);
  unknown_tokens = g_list_append (unknown_tokens, ut);

  return OK;
}


char* 
gimprc_value_to_str (char *name)
{
  return value_to_str (name); /* had a namespace collision */
}

static char *
value_to_str (char *name)
{
  int i;
  for (i = 0; i < nfuncs; i++)
    if (! strcmp (funcs[i].name, name))
      switch (funcs[i].type)
	{
	case TT_STRING:
	  return string_to_str (funcs[i].val1p, funcs[i].val2p);
	case TT_PATH:
	  return path_to_str (funcs[i].val1p, funcs[i].val2p);
	case TT_DOUBLE:
	  return double_to_str (funcs[i].val1p, funcs[i].val2p);
	case TT_FLOAT:
	  return float_to_str (funcs[i].val1p, funcs[i].val2p);
	case TT_INT:
	  return int_to_str (funcs[i].val1p, funcs[i].val2p);
	case TT_BOOLEAN:
	  return boolean_to_str (funcs[i].val1p, funcs[i].val2p);
	case TT_POSITION:
	  return position_to_str (funcs[i].val1p, funcs[i].val2p);
	case TT_MEMSIZE:
	  return mem_size_to_str (funcs[i].val1p, funcs[i].val2p);
	case TT_IMAGETYPE:
	  return image_type_to_str (funcs[i].val1p, funcs[i].val2p);
	case TT_INTERP:
	  return interpolation_type_to_str (funcs[i].val1p, funcs[i].val2p);
	case TT_XCOLORCUBE:
	  return color_cube_to_str (funcs[i].val1p, funcs[i].val2p);
	case TT_XPREVSIZE:
	  return preview_size_to_str (funcs[i].val1p, funcs[i].val2p);
	case TT_XNAVPREVSIZE:
	  return nav_preview_size_to_str (funcs[i].val1p, funcs[i].val2p);
	case TT_XUNIT:
	  return units_to_str (funcs[i].val1p, funcs[i].val2p);
	case TT_XPLUGIN:
	case TT_XPLUGINDEF:
	case TT_XMENUPATH:
	case TT_XDEVICE:
	case TT_XSESSIONINFO:
	case TT_XUNITINFO:
	case TT_XPARASITE:
	  return NULL;
	}
  return NULL;
}

static inline char *
string_to_str (gpointer val1p,
	       gpointer val2p)
{
#if defined (GLIB_CHECK_VERSION) && GLIB_CHECK_VERSION (1,3,1)
  gchar *str = g_strescape (*((char **)val1p), NULL);
#else
  gchar *str = g_strescape (*((char **)val1p));
#endif
  gchar *retval;

  retval = g_strdup_printf ("%c%s%c", '"', str, '"');
  g_free (str);

  return retval;
}

static inline char *
path_to_str (gpointer val1p,
	     gpointer val2p)
{
  return string_to_str (val1p, val2p);
}

static inline char *
double_to_str (gpointer val1p,
	       gpointer val2p)
{
  return g_strdup_printf ("%f", *((double *)val1p));
}

static inline char *
float_to_str (gpointer val1p,
	      gpointer val2p)
{
  return g_strdup_printf ("%f", (double)(*((float *)val1p)));
}

static inline char *
int_to_str (gpointer val1p,
	    gpointer val2p)
{
  return g_strdup_printf ("%d", *((int *)val1p));
}

static inline char *
boolean_to_str (gpointer val1p,
		gpointer val2p)
{
  int v;

  if (val1p != NULL)
    v = *((int *)val1p);
  else
    v = !*((int *)val2p);
  if (v)
    return g_strdup ("yes");
  else
    return g_strdup ("no");
}

static inline char *
position_to_str (gpointer val1p,
		 gpointer val2p)
{
  return g_strdup_printf ("%d %d", *((int *)val1p), *((int *)val2p));
}

static inline char *
mem_size_to_str (gpointer val1p,
		 gpointer val2p)
{
  int size;

  size = *((int *)val1p);
  if (size % 1048576 == 0)
    return g_strdup_printf ("%dM", size / 1048576);
  else if (size % 1024 == 0)
    return g_strdup_printf ("%dK", size / 1024);
  else
    return g_strdup_printf ("%dB", size);
}

static inline char *
image_type_to_str (gpointer val1p,
		   gpointer val2p)
{
  int type;

  type = *((int *)val1p);
  if (type == GRAY)
    return g_strdup ("gray");
  else
    return g_strdup ("rgb");
}

static inline char *
interpolation_type_to_str (gpointer val1p,
			   gpointer val2p)
{
  InterpolationType type;

  type = *((InterpolationType *)val1p);
  switch (type)
  {
   case LINEAR_INTERPOLATION:
     return g_strdup ("linear");
   case CUBIC_INTERPOLATION:
     return g_strdup ("cubic");
   case NEAREST_NEIGHBOR_INTERPOLATION:
     return g_strdup ("nearest-neighbor");
   default:
     return g_strdup ("bad interpolation type");
  }
}

static inline char *
color_cube_to_str (gpointer val1p,
		   gpointer val2p)
{
  return g_strdup_printf ("%d %d %d  %d",
			   color_cube_shades[0], color_cube_shades[1],
			   color_cube_shades[2], color_cube_shades[3]);
}

static inline char *
preview_size_to_str (gpointer val1p,
		     gpointer val2p)
{
  if (preview_size >= 128)
    return g_strdup ("huge");
  else if (preview_size >= 64)
    return g_strdup ("large");
  else if (preview_size >= 48)
    return g_strdup ("medium");
  else if (preview_size >= 32)
    return g_strdup ("small");
  else if (preview_size >= 24)
    return g_strdup ("tiny");
  else
    return g_strdup ("none");
}

static inline char *
nav_preview_size_to_str (gpointer val1p,
			 gpointer val2p)
{
  if (nav_preview_size >= 112)
    return g_strdup ("large");
  else if (nav_preview_size >= 80)
    return g_strdup ("medium");
  else if (nav_preview_size >= 48)
    return g_strdup ("small");
  else
    return g_strdup ("large");
}


static inline char *
units_to_str (gpointer val1p,
	      gpointer val2p)
{
  return g_strdup (gimp_unit_get_identifier (*((GUnit*)val1p)));
}

static void
add_gimp_directory_token (char *gimp_dir)
{
  UnknownToken *ut;

  /*
    The token holds data from a static buffer which is initialized
    once.  There should be no need to change an already-existing
    value.
  */
  if (NULL != gimprc_find_token ("gimp_dir"))
    return;

  ut = g_new (UnknownToken, 1);
  ut->token = g_strdup ("gimp_dir");
  ut->value = g_strdup (gimp_dir);

  /* Similarly, transforming the path should be silly. */
  unknown_tokens = g_list_append (unknown_tokens, ut);

  /* While we're at it, also add the token gimp_install_dir */
  ut = g_new (UnknownToken, 1);
  ut->token = g_strdup ("gimp_install_dir");
  ut->value = gimp_data_directory ();

  unknown_tokens = g_list_append (unknown_tokens, ut);
}

#ifdef __EMX__
static void
add_x11root_token (char *x11root)
{
  UnknownToken *ut;

  if (x11root == NULL)
    return;
  /*
    The token holds data from a static buffer which is initialized
    once.  There should be no need to change an already-existing
    value.
  */
  if (gimprc_find_token ("x11root") != NULL)
    return;

  ut = g_new (UnknownToken, 1);
  ut->token = g_strdup ("x11root");
  ut->value = g_strdup (x11root);

  unknown_tokens = g_list_append (unknown_tokens, ut);
}
#endif

/* Try to:

   1. Open the personal file for reading.

   1b. If that fails, try to open the system (overriding) file

   2. If we could open the personal file, rename it to file.old.

   3. Open personal file for writing.

   On success, return NULL. On failure, return a string in English
   explaining the problem.

   */
static char *
open_backup_file (char *filename,
		  char *secondary_filename,
		  char **name_used,
                  FILE **fp_new,
                  FILE **fp_old)
{
  char *oldfilename = NULL;

  /* Rename the file to *.old, open it for reading and create the new file. */
  if ((*fp_old = fopen (filename, "rt")) == NULL)
    {
      if ((*fp_old = fopen (secondary_filename, "rt")) == NULL)
	{
	  return g_strdup_printf (_("Can't open %s; %s"),
				  secondary_filename, g_strerror (errno));
	}
      else
	*name_used = secondary_filename;
    }
  else
    {
      *name_used = filename;
      oldfilename = g_strdup_printf ("%s.old", filename);
#if defined(G_OS_WIN32) || defined(__EMX__)
      /* Can't rename open files... */
      fclose (*fp_old);
      /* Also, can't rename to an existing file name */
      unlink (oldfilename);
#endif
      if (rename (filename, oldfilename) < 0)
	{
	  g_free (oldfilename);
	  return g_strdup_printf (_("Can't rename %s to %s.old; %s"),
				    filename, filename, g_strerror (errno));
	}
#if defined(G_OS_WIN32) || defined(__EMX__)
      /* Can't rename open files... */
      if ((*fp_old = fopen (oldfilename, "rt")) == NULL)
	g_error (_("Couldn't reopen %s\n"), oldfilename);
#endif
    }

  if ((*fp_new = fopen (filename, "w")) == NULL)
    {
      if (oldfilename != NULL)
	{
	  /* Undo the renaming... */
	  (void) rename (oldfilename, filename);
	  g_free (oldfilename);
	}
      return g_strdup_printf (_("Can't write to %s; %s"),
			      filename, g_strerror (errno));
    }

  if (oldfilename != NULL)
    g_free (oldfilename);
  return NULL;
}

char*
gimprc_find_token (char *token)
{
  GList *list;
  UnknownToken *ut;

  for (list = unknown_tokens; list; list = g_list_next (list))
    {
      ut = (UnknownToken *) list->data;

      if (strcmp (ut->token, token) == 0)
	return ut->value;
    }

  return NULL;
}

static void
gimprc_set_token (char *token,
		  char *value)
{
  GList *list;
  UnknownToken *ut;

  for (list = unknown_tokens; list; list = g_list_next (list))
    {
      ut = (UnknownToken *) list->data;

      if (strcmp (ut->token, token) == 0)
	{
	  if (ut->value != value)
	    {
	      if (ut->value)
		g_free (ut->value);
	      ut->value = value;
	    }
	  break;
	}
    }
}
