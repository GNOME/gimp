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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "gui/gui-types.h"

#include "base/base-config.c"

#include "core/gimp.h"
#include "core/gimpcoreconfig.h"

#include "gui/menus.h"

#include "app_procs.h"
#include "general.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


#define ERROR      0
#define DONE       1
#define OK         2


#define DEFAULT_IMAGE_TITLE_FORMAT  "%f-%p.%i (%t)"
#define DEFAULT_IMAGE_STATUS_FORMAT DEFAULT_IMAGE_TITLE_FORMAT
#define DEFAULT_COMMENT             "Created with The GIMP"


typedef enum
{
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
  TT_XPREVSIZE,
  TT_XUNIT,
  TT_XNAVPREVSIZE,
  TT_XTHUMBSIZE,
  TT_XHELPBROWSER,
  TT_XCURSORMODE,
  TT_XCOMMENT
} TokenType;


typedef struct _ParseFunc ParseFunc;

struct _ParseFunc
{
  gchar     *name;
  TokenType  type;
  gpointer   val1p;
  gpointer   val2p;
};

typedef struct _UnknownToken UnknownToken;

struct _UnknownToken
{
  gchar *token;
  gchar *value;
};


static gint           get_next_token            (void);
static gint           peek_next_token           (void);
static gint           parse_statement           (void);

static gint           parse_string              (gpointer val1p, gpointer val2p);
static gint           parse_path                (gpointer val1p, gpointer val2p);
static gint           parse_double              (gpointer val1p, gpointer val2p);
static gint           parse_float               (gpointer val1p, gpointer val2p);
static gint           parse_int                 (gpointer val1p, gpointer val2p);
static gint           parse_boolean             (gpointer val1p, gpointer val2p);
static gint           parse_position            (gpointer val1p, gpointer val2p);
static gint           parse_mem_size            (gpointer val1p, gpointer val2p);
static gint           parse_image_type          (gpointer val1p, gpointer val2p);
static gint           parse_interpolation_type  (gpointer val1p, gpointer val2p);
static gint           parse_preview_size        (gpointer val1p, gpointer val2p);
static gint           parse_nav_preview_size    (gpointer val1p, gpointer val2p);
static gint           parse_thumbnail_size      (gpointer val1p, gpointer val2p);
static gint           parse_units               (gpointer val1p, gpointer val2p);
static gint           parse_help_browser        (gpointer val1p, gpointer val2p);
static gint           parse_cursor_mode         (gpointer val1p, gpointer val2p);

static gint           parse_unknown             (gchar          *token_sym);

static inline gchar * string_to_str             (gpointer val1p, gpointer val2p);
static inline gchar * path_to_str               (gpointer val1p, gpointer val2p);
static inline gchar * double_to_str             (gpointer val1p, gpointer val2p);
static inline gchar * int_to_str                (gpointer val1p, gpointer val2p);
static inline gchar * boolean_to_str            (gpointer val1p, gpointer val2p);
static inline gchar * position_to_str           (gpointer val1p, gpointer val2p);
static inline gchar * mem_size_to_str           (gpointer val1p, gpointer val2p);
static inline gchar * image_type_to_str         (gpointer val1p, gpointer val2p);
static inline gchar * interpolation_type_to_str (gpointer val1p, gpointer val2p);
static inline gchar * preview_size_to_str       (gpointer val1p, gpointer val2p);
static inline gchar * nav_preview_size_to_str   (gpointer val1p, gpointer val2p);
static inline gchar * thumbnail_size_to_str     (gpointer val1p, gpointer val2p);
static inline gchar * units_to_str              (gpointer val1p, gpointer val2p);
static inline gchar * help_browser_to_str       (gpointer val1p, gpointer val2p);
static inline gchar * cursor_mode_to_str        (gpointer val1p, gpointer val2p);
static inline gchar * comment_to_str            (gpointer val1p, gpointer val2p);

static gchar        * transform_path            (gchar        *path, 
						 gboolean      destroy);
static void           gimprc_set_token          (const gchar  *token,
						 gchar        *value);
static void           add_gimp_directory_token  (const gchar  *gimp_dir);
#ifdef __EMX__
static void           add_x11root_token         (gchar        *x11root);
#endif
static gchar        * open_backup_file          (gchar        *filename,
						 gchar        *secondary_filename,
						 gchar       **name_used,
						 FILE        **fp_new,
						 FILE        **fp_old);


/*  global gimprc variables  */
GimpRc gimprc =
{
  /* marching_speed            */  300,       /* 300 ms */
  /* last_opened_size          */  4,
  /* gamma_val                 */  1.0,
  /* transparency_type         */  1,     /* Mid-Tone Checks */
  /* perfectmouse              */  FALSE, /* off (fast and sloppy) */
  /* transparency_size         */  1,     /* Medium sized */
  /* min_colors                */  144,   /* 6*6*4 */
  /* install_cmap              */  FALSE,
  /* cycled_marching_ants      */  0,
  /* default_threshold         */  15,
  /* resize_windows_on_zoom    */  FALSE,
  /* resize_windows_on_resize  */  FALSE,
  /* no_cursor_updating        */  FALSE,
  /* preview_size              */  GIMP_PREVIEW_SIZE_SMALL,
  /* nav_preview_size          */  GIMP_PREVIEW_SIZE_HUGE,
  /* show_rulers               */  TRUE,
  /* show_statusbar            */  TRUE,
  /* auto_save                 */  TRUE,
  /* confirm_on_close          */  TRUE,
  /* default_dot_for_dot       */  TRUE,
  /* save_session_info         */  TRUE,
  /* save_device_status        */  FALSE,
  /* always_restore_session    */  TRUE,
  /* show_tips                 */  TRUE,
  /* last_tip                  */  -1,
  /* show_tool_tips            */  TRUE,
  /* monitor_xres              */  72.0,
  /* monitor_yres              */  72.0,
  /* using_xserver_resolution  */  FALSE,
  /* image_title_format        */  NULL,
  /* image_status_format       */  NULL,
  /* max_new_image_size        */  33554432,  /* 32 MB */
  /* trust_dirty_flag          */  FALSE,
  /* use_help                  */  TRUE,
  /* info_window_follows_mouse */  TRUE,
  /* help_browser              */  GIMP_HELP_BROWSER_GIMP,
  /* cursor_mode               */  GIMP_CURSOR_MODE_TOOL_ICON,
  /* disable_tearoff_menus     */  FALSE,
  /* theme_path                */  NULL,
  /* theme                     */  NULL
};

static GHashTable *parse_func_hash = NULL;


static ParseFunc funcs[] =
{
  { "gamma-correction",              TT_DOUBLE,        &gimprc.gamma_val, NULL                 },
  { "marching-ants-speed",           TT_INT,           &gimprc.marching_speed, NULL            },
  { "last-opened-size",              TT_INT,           &gimprc.last_opened_size, NULL          },
  { "transparency-type",             TT_INT,           &gimprc.transparency_type, NULL         },
  { "perfect-mouse",                 TT_BOOLEAN,       &gimprc.perfectmouse, NULL              },
  { "transparency-size",             TT_INT,           &gimprc.transparency_size, NULL         },
  { "min-colors",                    TT_INT,           &gimprc.min_colors, NULL                },
  { "install-colormap",              TT_BOOLEAN,       &gimprc.install_cmap, NULL              },
  { "colormap-cycling",              TT_BOOLEAN,       &gimprc.cycled_marching_ants, NULL      },
  { "default-threshold",             TT_INT,           &gimprc.default_threshold, NULL         },
  { "resize-windows-on-zoom",        TT_BOOLEAN,       &gimprc.resize_windows_on_zoom, NULL    },
  { "dont-resize-windows-on-zoom",   TT_BOOLEAN,       NULL, &gimprc.resize_windows_on_zoom    },
  { "resize-windows-on-resize",      TT_BOOLEAN,       &gimprc.resize_windows_on_resize, NULL  },
  { "dont-resize-windows-on-resize", TT_BOOLEAN,       NULL, &gimprc.resize_windows_on_resize  },
  { "cursor-updating",               TT_BOOLEAN,       NULL, &gimprc.no_cursor_updating        },
  { "no-cursor-updating",            TT_BOOLEAN,       &gimprc.no_cursor_updating, NULL        },
  { "preview-size",                  TT_XPREVSIZE,     &gimprc.preview_size, NULL              },
  { "nav-preview-size",              TT_XNAVPREVSIZE,  &gimprc.nav_preview_size, NULL          },
  { "show-rulers",                   TT_BOOLEAN,       &gimprc.show_rulers, NULL               },
  { "dont-show-rulers",              TT_BOOLEAN,       NULL, &gimprc.show_rulers               },
  { "show-statusbar",                TT_BOOLEAN,       &gimprc.show_statusbar, NULL            },
  { "dont-show-statusbar",           TT_BOOLEAN,       NULL, &gimprc.show_statusbar            },
  { "auto-save",                     TT_BOOLEAN,       &gimprc.auto_save, NULL                 },
  { "dont-auto-save",                TT_BOOLEAN,       NULL, &gimprc.auto_save                 },
  { "confirm-on-close",              TT_BOOLEAN,       &gimprc.confirm_on_close, NULL          },
  { "dont-confirm-on-close",         TT_BOOLEAN,       NULL, &gimprc.confirm_on_close          },
  { "save-session-info",             TT_BOOLEAN,       &gimprc.save_session_info, NULL         },
  { "dont-save-session-info",        TT_BOOLEAN,       NULL, &gimprc.save_session_info         },
  { "save-device-status",            TT_BOOLEAN,       &gimprc.save_device_status, NULL        },
  { "dont-save-device-status",       TT_BOOLEAN,       NULL, &gimprc.save_device_status        },
  { "always-restore-session",        TT_BOOLEAN,       &gimprc.always_restore_session , NULL   },
  { "show-tips",                     TT_BOOLEAN,       &gimprc.show_tips, NULL                 },
  { "dont-show-tips",                TT_BOOLEAN,       NULL, &gimprc.show_tips                 },
  { "show-tool-tips",                TT_BOOLEAN,       &gimprc.show_tool_tips, NULL            },
  { "dont-show-tool-tips",           TT_BOOLEAN,       NULL, &gimprc.show_tool_tips            },
  { "default-dot-for-dot",           TT_BOOLEAN,       &gimprc.default_dot_for_dot, NULL       },
  { "monitor-xresolution",           TT_DOUBLE,        &gimprc.monitor_xres, NULL              },
  { "monitor-yresolution",           TT_DOUBLE,        &gimprc.monitor_yres, NULL              },
  { "image-title-format",            TT_STRING,        &gimprc.image_title_format, NULL        },
  { "image-status-format",           TT_STRING,        &gimprc.image_status_format, NULL       },
  { "max-new-image-size",            TT_MEMSIZE,       &gimprc.max_new_image_size, NULL        },
  { "trust-dirty-flag",		     TT_BOOLEAN,       &gimprc.trust_dirty_flag, NULL          },
  { "dont-trust-dirty-flag",         TT_BOOLEAN,       NULL, &gimprc.trust_dirty_flag          },
  { "use-help",                      TT_BOOLEAN,       &gimprc.use_help, NULL                  },
  { "dont-use-help",                 TT_BOOLEAN,       NULL, &gimprc.use_help                  },
  { "info-window-follows-mouse",     TT_BOOLEAN,       &gimprc.info_window_follows_mouse, NULL },
  { "info-window-per-display",       TT_BOOLEAN,       NULL, &gimprc.info_window_follows_mouse },
  { "help-browser",                  TT_XHELPBROWSER,  &gimprc.help_browser, NULL              },
  { "cursor-mode",                   TT_XCURSORMODE,   &gimprc.cursor_mode, NULL               },
  { "disable-tearoff-menus",         TT_BOOLEAN,       &gimprc.disable_tearoff_menus, NULL     },
  { "theme-path",                    TT_PATH,          &gimprc.theme_path, NULL                },
  { "theme",                         TT_STRING,        &gimprc.theme, NULL                     }
};


static ParseInfo  parse_info = { NULL };

static GList     *unknown_tokens = NULL;

static gint       cur_token;
static gint       next_token;


static gchar *
gimp_system_rc_file (void)
{
  static gchar *value = NULL;

  if (! value)
    {
      value = g_build_filename (gimp_sysconf_directory (), "gimprc", NULL);
    }

  return value;
}

gboolean
gimprc_init (Gimp *gimp)
{
  if (! parse_info.buffer)
    {
      gint i;

      static ParseFunc base_funcs[] =
      {
	{ "temp-path",          TT_PATH,    NULL, NULL },
	{ "swap-path",          TT_PATH,    NULL, NULL },
	{ "tile-cache-size",    TT_MEMSIZE, NULL, NULL },
	{ "stingy-memory-use",  TT_BOOLEAN, NULL, NULL },
	{ "num-processors",     TT_INT,     NULL, NULL }
      };

      static ParseFunc core_funcs[] =
      {
	{ "interpolation-type",       TT_INTERP,    NULL, NULL },
	{ "plug-in-path",             TT_PATH,      NULL, NULL },
	{ "module-path",              TT_PATH,      NULL, NULL },
	{ "brush-path",               TT_PATH,      NULL, NULL },
	{ "pattern-path",             TT_PATH,      NULL, NULL },
	{ "palette-path",             TT_PATH,      NULL, NULL },
	{ "gradient-path",            TT_PATH,      NULL, NULL },
	{ "default-brush",            TT_STRING,    NULL, NULL },
	{ "default-pattern",          TT_STRING,    NULL, NULL },
	{ "default-palette",          TT_STRING,    NULL, NULL },
	{ "default-gradient",         TT_STRING,    NULL, NULL },
	{ "default-comment",          TT_STRING,    NULL, NULL },
	{ "default-image-type",       TT_IMAGETYPE, NULL, NULL },
	{ "default-image-size",       TT_POSITION,  NULL, NULL },
	{ "default-units",            TT_XUNIT,     NULL, NULL },
	{ "default-xresolution",      TT_DOUBLE,    NULL, NULL },
	{ "default-yresolution",      TT_DOUBLE,    NULL, NULL },
	{ "default-resolution-units", TT_XUNIT,     NULL, NULL },
	{ "undo-levels",              TT_INT,       NULL, NULL },
	{ "pluginrc-path",            TT_PATH,      NULL, NULL },
	{ "module-load-inhibit",      TT_PATH,      NULL, NULL },
	{ "thumbnail-size",           TT_XTHUMBSIZE,NULL, NULL },
	{ "tool-plug-in-path",        TT_PATH,      NULL, NULL },
	{ "environ-path",             TT_PATH,      NULL, NULL }
      };

      /*  this hurts badly  */
      base_funcs[0].val1p  = &base_config->temp_path;
      base_funcs[1].val1p  = &base_config->swap_path;
      base_funcs[2].val1p  = &base_config->tile_cache_size;
      base_funcs[3].val1p  = &base_config->stingy_memory_use;
      base_funcs[4].val1p  = &base_config->num_processors;

      core_funcs[0].val1p  = &gimp->config->interpolation_type;
      core_funcs[1].val1p  = &gimp->config->plug_in_path;
      core_funcs[2].val1p  = &gimp->config->module_path;
      core_funcs[3].val1p  = &gimp->config->brush_path;
      core_funcs[4].val1p  = &gimp->config->pattern_path;
      core_funcs[5].val1p  = &gimp->config->palette_path;
      core_funcs[6].val1p  = &gimp->config->gradient_path;
      core_funcs[7].val1p  = &gimp->config->default_brush;
      core_funcs[8].val1p  = &gimp->config->default_pattern;
      core_funcs[9].val1p  = &gimp->config->default_palette;
      core_funcs[10].val1p = &gimp->config->default_gradient;
      core_funcs[11].val1p = &gimp->config->default_comment;
      core_funcs[12].val1p = &gimp->config->default_type;
      core_funcs[13].val1p = &gimp->config->default_width;
      core_funcs[13].val2p = &gimp->config->default_height;
      core_funcs[14].val1p = &gimp->config->default_units;
      core_funcs[15].val1p = &gimp->config->default_xresolution;
      core_funcs[16].val1p = &gimp->config->default_xresolution;
      core_funcs[17].val1p = &gimp->config->default_resolution_units;
      core_funcs[18].val1p = &gimp->config->levels_of_undo;
      core_funcs[19].val1p = &gimp->config->pluginrc_path;
      core_funcs[20].val1p = &gimp->config->module_db_load_inhibit;
      core_funcs[21].val1p = &gimp->config->thumbnail_size;
      core_funcs[22].val1p = &gimp->config->tool_plug_in_path;
      core_funcs[23].val1p = &gimp->config->environ_path;

      parse_func_hash = g_hash_table_new (g_str_hash, g_str_equal);

      for (i = 0; i < G_N_ELEMENTS (base_funcs); i++)
	g_hash_table_insert (parse_func_hash,
			     base_funcs[i].name,
			     &base_funcs[i]);

      for (i = 0; i < G_N_ELEMENTS (core_funcs); i++)
	g_hash_table_insert (parse_func_hash,
			     core_funcs[i].name,
			     &core_funcs[i]);

      for (i = 0; i < G_N_ELEMENTS (funcs); i++)
	g_hash_table_insert (parse_func_hash,
			     funcs[i].name,
			     &funcs[i]);

      parse_info.buffer        = g_new (gchar, 4096);
      parse_info.tokenbuf      = parse_info.buffer + 2048;
      parse_info.buffer_size   = 2048;
      parse_info.tokenbuf_size = 2048;

      return TRUE;
    }

  return FALSE;
}

static GList *
parse_add_directory_tokens (void)
{
  const gchar *gimp_dir;

  gimp_dir = gimp_directory ();

  add_gimp_directory_token (gimp_dir);
#ifdef __EMX__
  add_x11root_token (getenv ("X11ROOT"));
#endif

  /* the real output is unknown_tokens list !  */
  return unknown_tokens;
}

void
gimprc_parse (Gimp        *gimp,
              const gchar *cmdline_system_gimprc,
              const gchar *cmdline_gimprc)
{
  gchar *libfilename;
  gchar *filename;

  parse_add_directory_tokens ();

  if (cmdline_system_gimprc)
    libfilename = g_strdup (cmdline_system_gimprc);
  else
    libfilename = g_strdup (gimp_system_rc_file ());

  if (! gimprc_parse_file (libfilename))
    g_message ("Can't open '%s' for reading.", libfilename);

  if (cmdline_gimprc != NULL) 
    filename = g_strdup (cmdline_gimprc);
  else 
    filename = gimp_personal_rc_file ("gimprc");

  if (strcmp (filename, libfilename))
    gimprc_parse_file (filename);

  g_free (filename);
  g_free (libfilename);
 
  if (! gimprc.image_title_format)
    gimprc.image_title_format = g_strdup (DEFAULT_IMAGE_TITLE_FORMAT);

  if (! gimprc.image_status_format)
    gimprc.image_status_format = g_strdup (DEFAULT_IMAGE_STATUS_FORMAT);

  if (! gimp->config->default_comment)
    gimp->config->default_comment = g_strdup (DEFAULT_COMMENT);
}

static gboolean
parse_absolute_gimprc_file (const gchar *filename)
{
  gint status;

  parse_info.fp = fopen (filename, "rt");
  if (! parse_info.fp)
    return FALSE;

  if (the_gimp->be_verbose)
    g_print (_("parsing \"%s\"\n"), filename);

  cur_token  = -1;
  next_token = -1;

  parse_info.position    = -1;
  parse_info.linenum     = 1;
  parse_info.charnum     = 1;
  parse_info.inc_linenum = FALSE;
  parse_info.inc_charnum = FALSE;

  while ((status = parse_statement ()) == OK);

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
gimprc_parse_file (const gchar *filename)
{
  gchar    *rfilename;
  gboolean  parsed;

  if (! g_path_is_absolute (filename))
    {
      const gchar *home_dir = g_get_home_dir ();

      if (home_dir != NULL)
	{
	  rfilename = g_build_filename (home_dir, filename, NULL);
	  parsed = parse_absolute_gimprc_file (rfilename);
	  g_free (rfilename);
	  return parsed;
	}
    }

  parsed = parse_absolute_gimprc_file (filename);
  return parsed;
}

static GList *
g_list_findstr (GList       *list,
		const gchar *str)
{
  for (; list; list = g_list_next (list))
    {
      if (! strcmp ((gchar *) list->data, str))
        break;
    }

  return list;
}

void
save_gimprc_strings (const gchar *token,
		     const gchar *value)
{
  gchar     timestamp[40];  /* variables for parsing and updating gimprc */
  gchar    *name;
  gchar     tokname[51];
  FILE     *fp_new;
  FILE     *fp_old;
  gchar    *cur_line;
  gchar    *prev_line;
  gchar    *error_msg;
  gboolean  found = FALSE;
  gchar    *personal_gimprc;
  gchar    *str;

  UnknownToken *ut;    /* variables to modify unknown_tokens */
  UnknownToken *tmp;
  GList        *list;

  g_assert (token != NULL);
  g_assert (value != NULL);

  /* get the name of the backup file, and the file pointers.  'name'
   * is reused in another context later, disregard it here
   */
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
  cur_line  = g_new (char, 1024);

  while (! feof (fp_old))
    {
      if (! fgets (cur_line, 1024, fp_old))
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
       * and place that into 'name' if its found
       */
      if (find_token (cur_line, tokname, 50))
	{
	  /* check if that entry should be updated */
	  if (! g_ascii_strcasecmp (token, tokname)) /* if they match */
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
	      str = g_strescape (value, NULL);
              if (!found)
                {
		  fprintf (fp_new, "(%s \"%s\")\n", token, str);
		}
              else 
		fprintf (fp_new, "#- (%s \"%s\")\n", token, str);
	      g_free (str);
	      found = TRUE;
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
    } /* end of while(!feof) */

  g_free (cur_line);
  if (prev_line)
    g_free (prev_line);

  fclose (fp_old);

  /* append the options that were not in the old .gimprc */
  if (! found) 
    {
      fprintf (fp_new, "#- Next line added %s\n",
	       timestamp);
      str = g_strescape (value, NULL);
      fprintf (fp_new, "(%s \"%s\")\n\n", token, str);
      g_free (str);
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
gimprc_save (GList **updated_options,
	     GList **conflicting_options)
{
  gchar  timestamp[40];
  gchar *name;
  gchar  tokname[51];
  FILE  *fp_new;
  FILE  *fp_old;
  GList *option;
  gchar *cur_line;
  gchar *prev_line;
  gchar *str;
  gchar *error_msg;
  gchar *personal_gimprc;

  g_assert (updated_options != NULL);
  g_assert (conflicting_options != NULL);

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
      if (find_token (cur_line, tokname, 50))
	{
	  /* check if that entry should be updated */
	  option = g_list_findstr (*updated_options, tokname);
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
	      str = gimprc_value_to_str (tokname);
	      fprintf (fp_new, "(%s %s)\n", tokname, str);
	      g_free (str);

	      *updated_options = g_list_remove_link (*updated_options, option);
	      *conflicting_options = g_list_concat (*conflicting_options,
						    option);
	      continue;
	    }

	  /* check if that entry should be commented out */
	  option = g_list_findstr (*conflicting_options, tokname);
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
      str = gimprc_value_to_str ((const gchar *) option->data);
      fprintf (fp_new, "(%s %s)\n\n", (char *) option->data, str);
      g_free (str);
      option = option->next;
    }
  fclose (fp_new);
}

static gint
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

static gint
peek_next_token (void)
{
  if (next_token == -1)
    next_token = get_token (&parse_info);

  return next_token;
}

static gint
parse_statement (void)
{
  ParseFunc *func;
  gint       token;

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

  func = g_hash_table_lookup (parse_func_hash, token_sym);

  if (func)
    {
      switch (func->type)
	{
	case TT_STRING:
	  return parse_string (func->val1p, func->val2p);
	case TT_PATH:
	  return parse_path (func->val1p, func->val2p);
	case TT_DOUBLE:
	  return parse_double (func->val1p, func->val2p);
	case TT_FLOAT:
	  return parse_float (func->val1p, func->val2p);
	case TT_INT:
	  return parse_int (func->val1p, func->val2p);
	case TT_BOOLEAN:
	  return parse_boolean (func->val1p, func->val2p);
	case TT_POSITION:
	  return parse_position (func->val1p, func->val2p);
	case TT_MEMSIZE:
	  return parse_mem_size (func->val1p, func->val2p);
	case TT_IMAGETYPE:
	  return parse_image_type (func->val1p, func->val2p);
	case TT_INTERP:
	  return parse_interpolation_type (func->val1p, func->val2p);
	case TT_XPREVSIZE:
	  return parse_preview_size (func->val1p, func->val2p);
	case TT_XNAVPREVSIZE:
	  return parse_nav_preview_size (func->val1p, func->val2p);
	case TT_XTHUMBSIZE:
	  return parse_thumbnail_size (func->val1p, func->val2p);
	case TT_XUNIT:
	  return parse_units (func->val1p, func->val2p);
	case TT_XHELPBROWSER:
	  return parse_help_browser (func->val1p, func->val2p);
	case TT_XCURSORMODE:
	  return parse_cursor_mode (func->val1p, func->val2p);
	case TT_XCOMMENT:
	  return parse_string (func->val1p, func->val2p);
	}
    }

  return parse_unknown (token_sym);
}

static gint
parse_path (gpointer val1p,
	    gpointer val2p)
{
  gint    token;
  gchar **pathp;

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

static gint
parse_string (gpointer val1p,
	      gpointer val2p)
{
  gint    token;
  gchar **strp;

  g_assert (val1p != NULL);
  strp = (gchar **)val1p;

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

static gint
parse_double (gpointer val1p,
	      gpointer val2p)
{
  gint     token;
  gdouble *nump;

  g_assert (val1p != NULL);
  nump = (gdouble *) val1p;

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

static gint
parse_float (gpointer val1p,
	     gpointer val2p)
{
  gint    token;
  gfloat *nump;

  g_assert (val1p != NULL);
  nump = (gfloat *) val1p;

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

static gint
parse_int (gpointer val1p,
	   gpointer val2p)
{
  gint token;
  gint *nump;

  g_assert (val1p != NULL);
  nump = (gint *) val1p;

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

static gint
parse_boolean (gpointer val1p,
	       gpointer val2p)
{
  gint  token;
  gint *boolp;

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
    boolp = (gint *) val1p;
  else
    boolp = (gint *) val2p;

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

static gint
parse_position (gpointer val1p,
		gpointer val2p)
{
  gint  token;
  gint *xp;
  gint *yp;

  g_assert (val1p != NULL && val2p != NULL);
  xp = (gint *) val1p;
  yp = (gint *) val2p;

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

static gint
parse_mem_size (gpointer val1p,
		gpointer val2p)
{
  gint   suffix;
  gint   token;
  guint  mult;
  guint *sizep;

  g_assert (val1p != NULL);
  sizep = (guint *) val1p;

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

static gint
parse_image_type (gpointer val1p,
		  gpointer val2p)
{
  gint token;
  gint *typep;
  
  g_assert (val1p != NULL);
  typep = (gint *) val1p;

  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL))
    return ERROR;
  token = get_next_token ();
 
  if (!strcmp (token_sym, "rgb"))
    *typep = GIMP_RGB;
  else if ((!strcmp (token_sym, "gray")) || (!strcmp (token_sym, "grey")))
    *typep = GIMP_GRAY;
  else
    return ERROR;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static gint
parse_interpolation_type (gpointer val1p,
			  gpointer val2p)
{
  gint                   token;
  GimpInterpolationType *typep;
  
  g_assert (val1p != NULL);
  typep = (GimpInterpolationType *) val1p;

  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL))
    return ERROR;
  token = get_next_token ();
 
  if (strcmp (token_sym, "none") == 0)
    *typep = GIMP_INTERPOLATION_NONE;
  else if (strcmp (token_sym, "linear") == 0)
    *typep = GIMP_INTERPOLATION_LINEAR;
  else if (strcmp (token_sym, "cubic") == 0)
    *typep = GIMP_INTERPOLATION_CUBIC;
  else
    return ERROR;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static gint
parse_preview_size (gpointer val1p,
		    gpointer val2p)
{
  gint token;

  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL && token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  if (token == TOKEN_SYMBOL)
    {
      if (strcmp (token_sym, "none") == 0)
	*((gint *) val1p) = 0;
      else if (strcmp (token_sym, "tiny") == 0)
	*((gint *) val1p) = GIMP_PREVIEW_SIZE_TINY;
      else if (strcmp (token_sym, "extra-small") == 0)
	*((gint *) val1p) = GIMP_PREVIEW_SIZE_EXTRA_SMALL;
      else if (strcmp (token_sym, "small") == 0)
	*((gint *) val1p) = GIMP_PREVIEW_SIZE_SMALL;
      else if (strcmp (token_sym, "medium") == 0)
	*((gint *) val1p) = GIMP_PREVIEW_SIZE_MEDIUM;
      else if (strcmp (token_sym, "large") == 0)
	*((gint *) val1p) = GIMP_PREVIEW_SIZE_LARGE;
      else if (strcmp (token_sym, "extra-large") == 0)
	*((gint *) val1p) = GIMP_PREVIEW_SIZE_EXTRA_LARGE;
      else if (strcmp (token_sym, "huge") == 0)
	*((gint *) val1p) = GIMP_PREVIEW_SIZE_HUGE;
      else if (strcmp (token_sym, "enorous") == 0)
	*((gint *) val1p) = GIMP_PREVIEW_SIZE_ENORMOUS;
      else if (strcmp (token_sym, "gigantic") == 0)
	*((gint *) val1p) = GIMP_PREVIEW_SIZE_GIGANTIC;
      else
	*((gint *) val1p) = 0;
    }
  else if (token == TOKEN_NUMBER)
    *((gint *) val1p) = token_num;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static gint
parse_nav_preview_size (gpointer val1p,
			gpointer val2p)
{
  gint token;

  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL && token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  if (token == TOKEN_SYMBOL)
    {
      if (strcmp (token_sym, "none") == 0)
 	*((gint *) val1p) = 0;
      else if (strcmp (token_sym, "small") == 0)
	*((gint *) val1p) = GIMP_PREVIEW_SIZE_MEDIUM;
      else if (strcmp (token_sym, "medium") == 0)
	*((gint *) val1p) = GIMP_PREVIEW_SIZE_EXTRA_LARGE;
      else if (strcmp (token_sym, "large") == 0)
	*((gint *) val1p) = GIMP_PREVIEW_SIZE_HUGE;
      else
	*((gint *) val1p) = 0;
    }
  else if (token == TOKEN_NUMBER)
    *((gint *) val1p) = token_num;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static gint
parse_thumbnail_size (gpointer val1p,
                      gpointer val2p)
{
  gint token;

  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL && token != TOKEN_NUMBER))
    return ERROR;
  token = get_next_token ();

  if (token == TOKEN_SYMBOL)
    {
      if (strcmp (token_sym, "none") == 0)
 	*((gint *) val1p) = GIMP_THUMBNAIL_SIZE_NONE;
      else if (strcmp (token_sym, "normal") == 0)
	*((gint *) val1p) = GIMP_THUMBNAIL_SIZE_NORMAL;
      else if (strcmp (token_sym, "large") == 0)
	*((gint *) val1p) = GIMP_THUMBNAIL_SIZE_LARGE;
      else
	*((gint *) val1p) = GIMP_THUMBNAIL_SIZE_NONE;
    }
  else if (token == TOKEN_NUMBER)
    *((gint *) val1p) = token_num;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static gint
parse_units (gpointer val1p,
	     gpointer val2p)
{
  gint token;
  gint i;

  g_assert (val1p != NULL);

  token = peek_next_token ();
  if (!token || (token != TOKEN_SYMBOL))
    return ERROR;
  token = get_next_token ();

  *((GimpUnit *) val1p) = GIMP_UNIT_INCH;
  for (i = GIMP_UNIT_INCH; i < gimp_unit_get_number_of_units (); i++)
    if (strcmp (token_sym, gimp_unit_get_identifier (i)) == 0)
      {
	*((GimpUnit *) val1p) = i;
	break;
      }

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static gchar *
transform_path (gchar    *path,
		gboolean  destroy)
{
  gint          length      = 0;
  gchar        *new_path;
  const gchar  *home;
  gchar        *token;
  gchar        *tmp;
  gchar        *tmp2;
  gboolean      substituted = FALSE;
  gboolean      is_env      = FALSE;
  UnknownToken *ut;

  home = g_get_home_dir ();

  tmp = path;

  while (*tmp)
    {
#ifndef G_OS_WIN32
      if (*tmp == '~')
	{
	  length += strlen (home);
	  tmp += 1;
	}
      else
#endif
      if (*tmp == '$')
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

	  tmp2 = (gchar *) gimprc_find_token (token);
	  if (tmp2 == NULL) 
	    {
	      /* maybe token is an environment variable */
	      tmp2 = (gchar *) g_getenv (token);
#ifdef G_OS_WIN32
	      /* The default user gimprc on Windows references
	       * ${TEMP}, but not all Windows installations have that
	       * environment variable, even if it should be kinda
	       * standard. So special-case it.
	       */
	      if (tmp2 == NULL &&
		  (g_ascii_strcasecmp (token, "temp") == 0 ||
		   g_ascii_strcasecmp (token, "tmp") == 0))
		{
		  tmp2 = g_get_tmp_dir ();
		}
#endif
	      if (tmp2 != NULL)
		{
		  is_env = TRUE;
		}
	      else
		{
		  g_error ("gimprc token referenced but not defined: %s", token);
		}
	    }
	  tmp2 = transform_path ((gchar *) tmp2, FALSE);
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
#ifndef G_OS_WIN32
      if (*tmp == '~')
	{
	  *tmp2 = '\0';
	  strcat (tmp2, home);
	  tmp2 += strlen (home);
	  tmp += 1;
	}
      else
#endif
	   if (*tmp == '$')
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
	  token = (gchar *) gimprc_find_token (token);
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

static gint
parse_help_browser (gpointer val1p,
		    gpointer val2p)
{
  gint token;

  token = peek_next_token ();
  if (!token || token != TOKEN_SYMBOL)
    return ERROR;
  token = get_next_token ();

  if (strcmp (token_sym, "gimp") == 0)
    *((gint *) val1p) = GIMP_HELP_BROWSER_GIMP;
  else if (strcmp (token_sym, "netscape") == 0)
    *((gint *) val1p) = GIMP_HELP_BROWSER_NETSCAPE;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static gint
parse_cursor_mode (gpointer val1p,
		   gpointer val2p)
{
  gint token;

  token = peek_next_token ();
  if (!token || token != TOKEN_SYMBOL)
    return ERROR;
  token = get_next_token ();

  if (strcmp (token_sym, "tool-icon") == 0)
    *((GimpCursorMode *) val1p) = GIMP_CURSOR_MODE_TOOL_ICON;
  else if (strcmp (token_sym, "tool-crosshair") == 0)
    *((GimpCursorMode *) val1p) = GIMP_CURSOR_MODE_TOOL_CROSSHAIR;
  else if (strcmp (token_sym, "crosshair") == 0)
    *((GimpCursorMode *) val1p) = GIMP_CURSOR_MODE_CROSSHAIR;

  token = peek_next_token ();
  if (!token || (token != TOKEN_RIGHT_PAREN))
    return ERROR;
  token = get_next_token ();

  return OK;
}

static gint
parse_unknown (gchar *token_sym)
{
  gint          token;
  UnknownToken *ut;
  UnknownToken *tmp;
  GList        *list;

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

gchar *
gimprc_value_to_str (const gchar *name)
{
  ParseFunc *func;

  func = g_hash_table_lookup (parse_func_hash, name);

  if (func)
    {
      switch (func->type)
	{
	case TT_STRING:
	  return string_to_str (func->val1p, func->val2p);
	case TT_PATH:
	  return path_to_str (func->val1p, func->val2p);
	case TT_DOUBLE:
	case TT_FLOAT:
	  return double_to_str (func->val1p, func->val2p);
	case TT_INT:
	  return int_to_str (func->val1p, func->val2p);
	case TT_BOOLEAN:
	  return boolean_to_str (func->val1p, func->val2p);
	case TT_POSITION:
	  return position_to_str (func->val1p, func->val2p);
	case TT_MEMSIZE:
	  return mem_size_to_str (func->val1p, func->val2p);
	case TT_IMAGETYPE:
	  return image_type_to_str (func->val1p, func->val2p);
	case TT_INTERP:
	  return interpolation_type_to_str (func->val1p, func->val2p);
	case TT_XPREVSIZE:
	  return preview_size_to_str (func->val1p, func->val2p);
	case TT_XNAVPREVSIZE:
	  return nav_preview_size_to_str (func->val1p, func->val2p);
	case TT_XTHUMBSIZE:
	  return thumbnail_size_to_str (func->val1p, func->val2p);
	case TT_XUNIT:
	  return units_to_str (func->val1p, func->val2p);
	case TT_XHELPBROWSER:
	  return help_browser_to_str (func->val1p, func->val2p);
	case TT_XCURSORMODE:
	  return cursor_mode_to_str (func->val1p, func->val2p);
	case TT_XCOMMENT:
	  return comment_to_str (func->val1p, func->val2p);
	}
    }

  return NULL;
}

static inline gchar *
string_to_str (gpointer val1p,
	       gpointer val2p)
{
  gchar *str = g_strescape (*((char **)val1p), NULL);
  gchar *retval;

  retval = g_strdup_printf ("%c%s%c", '"', str, '"');
  g_free (str);

  return retval;
}

static inline gchar *
path_to_str (gpointer val1p,
	     gpointer val2p)
{
  return string_to_str (val1p, val2p);
}

static inline gchar *
double_to_str (gpointer val1p,
	       gpointer val2p)
{
  gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

  g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", *((gdouble *)val1p));
  return g_strdup (buf);
}

static inline gchar *
int_to_str (gpointer val1p,
	    gpointer val2p)
{
  return g_strdup_printf ("%d", *((gint *)val1p));
}

static inline gchar *
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

static inline gchar *
position_to_str (gpointer val1p,
		 gpointer val2p)
{
  return g_strdup_printf ("%d %d", *((int *)val1p), *((int *)val2p));
}

static inline gchar *
mem_size_to_str (gpointer val1p,
		 gpointer val2p)
{
  guint size;

  size = *((guint *)val1p);
  if (size % 1048576 == 0)
    return g_strdup_printf ("%dM", size / 1048576);
  else if (size % 1024 == 0)
    return g_strdup_printf ("%dK", size / 1024);
  else
    return g_strdup_printf ("%dB", size);
}

static inline gchar *
image_type_to_str (gpointer val1p,
		   gpointer val2p)
{
  gint type;

  type = *((gint *)val1p);
  if (type == GIMP_GRAY)
    return g_strdup ("gray");
  else
    return g_strdup ("rgb");
}

static inline gchar *
interpolation_type_to_str (gpointer val1p,
			   gpointer val2p)
{
  GimpInterpolationType type;

  type = *((GimpInterpolationType *)val1p);
  switch (type)
  {
   case GIMP_INTERPOLATION_NONE:
     return g_strdup ("none");
   case GIMP_INTERPOLATION_LINEAR:
     return g_strdup ("linear");
   case GIMP_INTERPOLATION_CUBIC:
     return g_strdup ("cubic");
   default:
     return g_strdup ("bad interpolation type");
  }
}

static inline gchar *
preview_size_to_str (gpointer val1p,
		     gpointer val2p)
{
  gint size;

  size = *((gint *) val1p);

  if (size >= GIMP_PREVIEW_SIZE_GIGANTIC)
    return g_strdup ("gigantic");
  else if (size >= GIMP_PREVIEW_SIZE_ENORMOUS)
    return g_strdup ("enormous");
  else if (size >= GIMP_PREVIEW_SIZE_HUGE)
    return g_strdup ("huge");
  else if (size >= GIMP_PREVIEW_SIZE_EXTRA_LARGE)
    return g_strdup ("extra-large");
  else if (size >= GIMP_PREVIEW_SIZE_LARGE)
    return g_strdup ("large");
  else if (size >= GIMP_PREVIEW_SIZE_MEDIUM)
    return g_strdup ("medium");
  else if (size >= GIMP_PREVIEW_SIZE_SMALL)
    return g_strdup ("small");
  else if (size >= GIMP_PREVIEW_SIZE_EXTRA_SMALL)
    return g_strdup ("extra-small");
  else if (size >= GIMP_PREVIEW_SIZE_TINY)
    return g_strdup ("tiny");
  else
    return g_strdup ("none");
}

static inline gchar *
nav_preview_size_to_str (gpointer val1p,
			 gpointer val2p)
{
  gint size;

  size = *((gint *) val1p);

  if (size >= GIMP_PREVIEW_SIZE_HUGE)
    return g_strdup ("large");
  else if (size >= GIMP_PREVIEW_SIZE_EXTRA_LARGE)
    return g_strdup ("medium");
  else if (size >= GIMP_PREVIEW_SIZE_MEDIUM)
    return g_strdup ("small");
  else
    return g_strdup ("none");
}

static inline gchar *
thumbnail_size_to_str (gpointer val1p,
                       gpointer val2p)
{
  gint size;

  size = *((gint *) val1p);

  if (size >= GIMP_THUMBNAIL_SIZE_LARGE)
    return g_strdup ("large");
  else if (size >= GIMP_THUMBNAIL_SIZE_NORMAL)
    return g_strdup ("normal");
  else
    return g_strdup ("none");
}


static inline gchar *
units_to_str (gpointer val1p,
	      gpointer val2p)
{
  return g_strdup (gimp_unit_get_identifier (*((GimpUnit *) val1p)));
}

static inline gchar *
help_browser_to_str (gpointer val1p,
		     gpointer val2p)
{
  gint browser;

  browser = *((gint *) val1p);

  if (browser == GIMP_HELP_BROWSER_NETSCAPE)
    return g_strdup ("netscape");
  else
    return g_strdup ("gimp");
}

static inline gchar *
cursor_mode_to_str (gpointer val1p,
		    gpointer val2p)
{
  GimpCursorMode mode;

  mode = *((GimpCursorMode *) val1p);

  if (mode == GIMP_CURSOR_MODE_TOOL_ICON)
    return g_strdup ("tool-icon");
  else if (mode == GIMP_CURSOR_MODE_TOOL_CROSSHAIR)
    return g_strdup ("tool-crosshair");
  else
    return g_strdup ("crosshair");
}

static inline gchar *
comment_to_str (gpointer val1p,
		gpointer val2p)
{
  gchar **str_array;
  gchar  *retval;
  gchar  *str = g_strescape (*((gchar **) val1p), NULL);

  str_array = g_strsplit (str, "\n", 0);
  g_free (str);
  str = g_strjoinv ("\\n", str_array);
  g_strfreev (str_array);
  retval = g_strdup_printf ("%c%s%c", '"', str, '"');
  g_free (str);

  return retval;
}

static void
add_gimp_directory_token (const gchar *gimp_dir)
{
  UnknownToken *ut;

  /* The token holds data from a static buffer which is initialized
   * once.  There should be no need to change an already-existing
   * value.
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
  ut->value = (gchar *) gimp_data_directory ();

  unknown_tokens = g_list_append (unknown_tokens, ut);
}

#ifdef __EMX__
static void
add_x11root_token (gchar *x11root)
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
static gchar *
open_backup_file (gchar  *filename,
		  gchar  *secondary_filename,
		  gchar **name_used,
                  FILE  **fp_new,
                  FILE  **fp_old)
{
  gchar *oldfilename = NULL;

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

const gchar *
gimprc_find_token (const gchar *token)
{
  GList        *list;
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
gimprc_set_token (const gchar *token,
		  gchar       *value)
{
  GList        *list;
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
