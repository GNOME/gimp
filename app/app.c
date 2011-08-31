/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Win32 language lookup table:
 * Copyright (C) 2007-2008 Dieter Verfaillie <dieterv@optionexplicit.be>
 */


#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gegl.h>

#ifdef G_OS_WIN32
#include <windows.h>
#include <winnls.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core/core-types.h"

#include "config/gimprc.h"

#include "base/base.h"
#include "base/tile-swap.h"

#include "gegl/gimp-gegl.h"

#include "core/gimp.h"
#include "core/gimp-user-install.h"

#include "file/file-open.h"

#ifndef GIMP_CONSOLE_COMPILATION
#include "dialogs/user-install-dialog.h"

#include "gui/gui.h"
#endif

#include "app.h"
#include "batch.h"
#include "errors.h"
#include "units.h"
#include "gimp-debug.h"

#include "gimp-intl.h"


/*  local prototypes  */

static void       app_init_update_noop    (const gchar *text1,
                                           const gchar *text2,
                                           gdouble      percentage);
static void       app_init_language       (const gchar *language);

static gboolean   app_exit_after_callback (Gimp        *gimp,
                                           gboolean     kill_it,
                                           GMainLoop   *loop);


/*  public functions  */

void
app_libs_init (GOptionContext *context,
               gboolean        no_interface)
{
  g_type_init ();

  g_option_context_add_group (context, gegl_get_option_group ());

#ifndef GIMP_CONSOLE_COMPILATION
  if (! no_interface)
    {
      gui_libs_init (context);
    }
#endif
}

void
app_abort (gboolean     no_interface,
           const gchar *abort_message)
{
#ifndef GIMP_CONSOLE_COMPILATION
  if (no_interface)
#endif
    {
      g_print ("%s\n\n", abort_message);
    }
#ifndef GIMP_CONSOLE_COMPILATION
  else
    {
      gui_abort (abort_message);
    }
#endif

  app_exit (EXIT_FAILURE);
}

void
app_exit (gint status)
{
  exit (status);
}

void
app_run (const gchar         *full_prog_name,
         const gchar        **filenames,
         const gchar         *alternate_system_gimprc,
         const gchar         *alternate_gimprc,
         const gchar         *session_name,
         const gchar         *batch_interpreter,
         const gchar        **batch_commands,
         gboolean             as_new,
         gboolean             no_interface,
         gboolean             no_data,
         gboolean             no_fonts,
         gboolean             no_splash,
         gboolean             be_verbose,
         gboolean             use_shm,
         gboolean             use_cpu_accel,
         gboolean             console_messages,
         gboolean             use_debug_handler,
         GimpStackTraceMode   stack_trace_mode,
         GimpPDBCompatMode    pdb_compat_mode)
{
  GimpInitStatusFunc  update_status_func = NULL;
  Gimp               *gimp;
  GimpBaseConfig     *config;
  GMainLoop          *loop;
  gboolean            swap_is_ok;

  /*  Create an instance of the "Gimp" object which is the root of the
   *  core object system
   */
  gimp = gimp_new (full_prog_name,
                   session_name,
                   be_verbose,
                   no_data,
                   no_fonts,
                   no_interface,
                   use_shm,
                   console_messages,
                   stack_trace_mode,
                   pdb_compat_mode);

  errors_init (gimp, full_prog_name, use_debug_handler, stack_trace_mode);

  units_init (gimp);

  /*  Check if the user's gimp_directory exists
   */
  if (! g_file_test (gimp_directory (), G_FILE_TEST_IS_DIR))
    {
      GimpUserInstall *install = gimp_user_install_new (be_verbose);

#ifdef GIMP_CONSOLE_COMPILATION
      gimp_user_install_run (install);
#else
      if (! (no_interface ?
	     gimp_user_install_run (install) :
	     user_install_dialog_run (install)))
	exit (EXIT_FAILURE);
#endif

      gimp_user_install_free (install);
    }

  gimp_load_config (gimp, alternate_system_gimprc, alternate_gimprc);

  config = GIMP_BASE_CONFIG (gimp->config);

  /*  change the locale if a language if specified  */
  app_init_language (gimp->config->language);

  /*  initialize lowlevel stuff  */
  swap_is_ok = base_init (config, be_verbose, use_cpu_accel);

  gimp_gegl_init (gimp);

#ifndef GIMP_CONSOLE_COMPILATION
  if (! no_interface)
    update_status_func = gui_init (gimp, no_splash);
#endif

  if (! update_status_func)
    update_status_func = app_init_update_noop;

  /*  Create all members of the global Gimp instance which need an already
   *  parsed gimprc, e.g. the data factories
   */
  gimp_initialize (gimp, update_status_func);

  /*  Load all data files
   */
  gimp_restore (gimp, update_status_func);

  /* display a warning when no test swap file could be generated */
  if (! swap_is_ok)
    {
      gchar *path = gimp_config_path_expand (config->swap_path, FALSE, NULL);

      g_message (_("Unable to open a test swap file.\n\n"
		   "To avoid data loss, please check the location "
		   "and permissions of the swap directory defined in "
		   "your Preferences (currently \"%s\")."), path);

      g_free (path);
    }

  /*  enable autosave late so we don't autosave when the
   *  monitor resolution is set in gui_init()
   */
  gimp_rc_set_autosave (GIMP_RC (gimp->edit_config), TRUE);

  /*  Load the images given on the command-line.
   */
  if (filenames)
    {
      gint i;

      for (i = 0; filenames[i] != NULL; i++)
        file_open_from_command_line (gimp, filenames[i], as_new);
    }

  batch_run (gimp, batch_interpreter, batch_commands);

  loop = g_main_loop_new (NULL, FALSE);

  g_signal_connect_after (gimp, "exit",
                          G_CALLBACK (app_exit_after_callback),
                          loop);

  gimp_threads_leave (gimp);
  g_main_loop_run (loop);
  gimp_threads_enter (gimp);

  g_main_loop_unref (loop);

  g_object_unref (gimp);

  gimp_debug_instances ();

  errors_exit ();
  gegl_exit ();
  base_exit ();
}


/*  private functions  */

static void
app_init_update_noop (const gchar *text1,
                      const gchar *text2,
                      gdouble      percentage)
{
  /*  deliberately do nothing  */
}

static void
app_init_language (const gchar *language)
{
#ifdef G_OS_WIN32
  if (! language)
    {
      /* FIXME: This is a hack. gettext doesn't pick the right language
       * by default on Windows, so we enforce the right one. The
       * following code is an adaptation of Python code from
       * pynicotine. For reasons why this approach is needed, and why
       * the GetLocaleInfo() approach in other libs falls flat, see:
       * http://blogs.msdn.com/b/michkap/archive/2007/04/15/2146890.aspx
       */

      switch (GetUserDefaultUILanguage())
        {
        case 1078:
          language = "af";    /* Afrikaans - South Africa */
          break;
        case 1052:
          language = "sq";    /* Albanian - Albania */
          break;
        case 1118:
          language = "am";    /* Amharic - Ethiopia */
          break;
        case 1025:
          language = "ar";    /* Arabic - Saudi Arabia */
          break;
        case 5121:
          language = "ar";    /* Arabic - Algeria */
          break;
        case 15361:
          language = "ar";    /* Arabic - Bahrain */
          break;
        case 3073:
          language = "ar";    /* Arabic - Egypt */
          break;
        case 2049:
          language = "ar";    /* Arabic - Iraq */
          break;
        case 11265:
          language = "ar";    /* Arabic - Jordan */
          break;
        case 13313:
          language = "ar";    /* Arabic - Kuwait */
          break;
        case 12289:
          language = "ar";    /* Arabic - Lebanon */
          break;
        case 4097:
          language = "ar";    /* Arabic - Libya */
          break;
        case 6145:
          language = "ar";    /* Arabic - Morocco */
          break;
        case 8193:
          language = "ar";    /* Arabic - Oman */
          break;
        case 16385:
          language = "ar";    /* Arabic - Qatar */
          break;
        case 10241:
          language = "ar";    /* Arabic - Syria */
          break;
        case 7169:
          language = "ar";    /* Arabic - Tunisia */
          break;
        case 14337:
          language = "ar";    /* Arabic - U.A.E. */
          break;
        case 9217:
          language = "ar";    /* Arabic - Yemen */
          break;
        case 1067:
          language = "hy";    /* Armenian - Armenia */
          break;
        case 1101:
          language = "as";    /* Assamese */
          break;
        case 2092:
          language = NULL;    /* Azeri (Cyrillic) */
          break;
        case 1068:
          language = NULL;    /* Azeri (Latin) */
          break;
        case 1069:
          language = "eu";    /* Basque */
          break;
        case 1059:
          language = "be";    /* Belarusian */
          break;
        case 1093:
          language = "bn";    /* Bengali (India) */
          break;
        case 2117:
          language = "bn";    /* Bengali (Bangladesh) */
          break;
        case 5146:
          language = "bs";    /* Bosnian (Bosnia/Herzegovina) */
          break;
        case 1026:
          language = "bg";    /* Bulgarian */
          break;
        case 1109:
          language = "my";    /* Burmese */
          break;
        case 1027:
          language = "ca";    /* Catalan */
          break;
        case 1116:
          language = NULL;    /* Cherokee - United States */
          break;
        case 2052:
          language = "zh";    /* Chinese - People"s Republic of China */
          break;
        case 4100:
          language = "zh";    /* Chinese - Singapore */
          break;
        case 1028:
          language = "zh";    /* Chinese - Taiwan */
          break;
        case 3076:
          language = "zh";    /* Chinese - Hong Kong SAR */
          break;
        case 5124:
          language = "zh";    /* Chinese - Macao SAR */
          break;
        case 1050:
          language = "hr";    /* Croatian */
          break;
        case 4122:
          language = "hr";    /* Croatian (Bosnia/Herzegovina) */
          break;
        case 1029:
          language = "cs";    /* Czech */
          break;
        case 1030:
          language = "da";    /* Danish */
          break;
        case 1125:
          language = "dv";    /* Divehi */
          break;
        case 1043:
          language = "nl";    /* Dutch - Netherlands */
          break;
        case 2067:
          language = "nl";    /* Dutch - Belgium */
          break;
        case 1126:
          language = NULL;    /* Edo */
          break;
        case 1033:
          language = "en";    /* English - United States */
          break;
        case 2057:
          language = "en";    /* English - United Kingdom */
          break;
        case 3081:
          language = "en";    /* English - Australia */
          break;
        case 10249:
          language = "en";    /* English - Belize */
          break;
        case 4105:
          language = "en";    /* English - Canada */
          break;
        case 9225:
          language = "en";    /* English - Caribbean */
          break;
        case 15369:
          language = "en";    /* English - Hong Kong SAR */
          break;
        case 16393:
          language = "en";    /* English - India */
          break;
        case 14345:
          language = "en";    /* English - Indonesia */
          break;
        case 6153:
          language = "en";    /* English - Ireland */
          break;
        case 8201:
          language = "en";    /* English - Jamaica */
          break;
        case 17417:
          language = "en";    /* English - Malaysia */
          break;
        case 5129:
          language = "en";    /* English - New Zealand */
          break;
        case 13321:
          language = "en";    /* English - Philippines */
          break;
        case 18441:
          language = "en";    /* English - Singapore */
          break;
        case 7177:
          language = "en";    /* English - South Africa */
          break;
        case 11273:
          language = "en";    /* English - Trinidad */
          break;
        case 12297:
          language = "en";    /* English - Zimbabwe */
          break;
        case 1061:
          language = "et";    /* Estonian */
          break;
        case 1080:
          language = "fo";    /* Faroese */
          break;
        case 1065:
          language = NULL;    /* Farsi */
          break;
        case 1124:
          language = NULL;    /* Filipino */
          break;
        case 1035:
          language = "fi";    /* Finnish */
          break;
        case 1036:
          language = "fr";    /* French - France */
          break;
        case 2060:
          language = "fr";    /* French - Belgium */
          break;
        case 11276:
          language = "fr";    /* French - Cameroon */
          break;
        case 3084:
          language = "fr";    /* French - Canada */
          break;
        case 9228:
          language = "fr";    /* French - Democratic Rep. of Congo */
          break;
        case 12300:
          language = "fr";    /* French - Cote d"Ivoire */
          break;
        case 15372:
          language = "fr";    /* French - Haiti */
          break;
        case 5132:
          language = "fr";    /* French - Luxembourg */
          break;
        case 13324:
          language = "fr";    /* French - Mali */
          break;
        case 6156:
          language = "fr";    /* French - Monaco */
          break;
        case 14348:
          language = "fr";    /* French - Morocco */
          break;
        case 58380:
          language = "fr";    /* French - North Africa */
          break;
        case 8204:
          language = "fr";    /* French - Reunion */
          break;
        case 10252:
          language = "fr";    /* French - Senegal */
          break;
        case 4108:
          language = "fr";    /* French - Switzerland */
          break;
        case 7180:
          language = "fr";    /* French - West Indies */
          break;
        case 1122:
          language = "fy";    /* Frisian - Netherlands */
          break;
        case 1127:
          language = NULL;    /* Fulfulde - Nigeria */
          break;
        case 1071:
          language = "mk";    /* FYRO Macedonian */
          break;
        case 2108:
          language = "ga";    /* Gaelic (Ireland) */
          break;
        case 1084:
          language = "gd";    /* Gaelic (Scotland) */
          break;
        case 1110:
          language = "gl";    /* Galician */
          break;
        case 1079:
          language = "ka";    /* Georgian */
          break;
        case 1031:
          language = "de";    /* German - Germany */
          break;
        case 3079:
          language = "de";    /* German - Austria */
          break;
        case 5127:
          language = "de";    /* German - Liechtenstein */
          break;
        case 4103:
          language = "de";    /* German - Luxembourg */
          break;
        case 2055:
          language = "de";    /* German - Switzerland */
          break;
        case 1032:
          language = "el";    /* Greek */
          break;
        case 1140:
          language = "gn";    /* Guarani - Paraguay */
          break;
        case 1095:
          language = "gu";    /* Gujarati */
          break;
        case 1128:
          language = "ha";    /* Hausa - Nigeria */
          break;
        case 1141:
          language = NULL;    /* Hawaiian - United States */
          break;
        case 1037:
          language = "he";    /* Hebrew */
          break;
        case 1081:
          language = "hi";    /* Hindi */
          break;
        case 1038:
          language = "hu";    /* Hungarian */
          break;
        case 1129:
          language = NULL;    /* Ibibio - Nigeria */
          break;
        case 1039:
          language = "is";    /* Icelandic */
          break;
        case 1136:
          language = "ig";    /* Igbo - Nigeria */
          break;
        case 1057:
          language = "id";    /* Indonesian */
          break;
        case 1117:
          language = "iu";    /* Inuktitut */
          break;
        case 1040:
          language = "it";    /* Italian - Italy */
          break;
        case 2064:
          language = "it";    /* Italian - Switzerland */
          break;
        case 1041:
          language = "ja";    /* Japanese */
          break;
        case 1099:
          language = "kn";    /* Kannada */
          break;
        case 1137:
          language = "kr";    /* Kanuri - Nigeria */
          break;
        case 2144:
          language = "ks";    /* Kashmiri */
          break;
        case 1120:
          language = "ks";    /* Kashmiri (Arabic) */
          break;
        case 1087:
          language = "kk";    /* Kazakh */
          break;
        case 1107:
          language = "km";    /* Khmer */
          break;
        case 1111:
          language = NULL;    /* Konkani */
          break;
        case 1042:
          language = "ko";    /* Korean */
          break;
        case 1088:
          language = "ky";    /* Kyrgyz (Cyrillic) */
          break;
        case 1108:
          language = "lo";    /* Lao */
          break;
        case 1142:
          language = "la";    /* Latin */
          break;
        case 1062:
          language = "lv";    /* Latvian */
          break;
        case 1063:
          language = "lt";    /* Lithuanian */
          break;
        case 1086:
          language = "ms";    /* Malay - Malaysia */
          break;
        case 2110:
          language = "ms";    /* Malay - Brunei Darussalam */
          break;
        case 1100:
          language = "ml";    /* Malayalam */
          break;
        case 1082:
          language = "mt";    /* Maltese */
          break;
        case 1112:
          language = NULL;    /* Manipuri */
          break;
        case 1153:
          language = "mi";    /* Maori - New Zealand */
          break;
        case 1102:
          language = "mr";    /* Marathi */
          break;
        case 1104:
          language = "mn";    /* Mongolian (Cyrillic) */
          break;
        case 2128:
          language = "mn";    /* Mongolian (Mongolian) */
          break;
        case 1121:
          language = "ne";    /* Nepali */
          break;
        case 2145:
          language = "ne";    /* Nepali - India */
          break;
        case 1044:
          language = "no";    /* Norwegian (Bokmￃﾥl) */
          break;
        case 2068:
          language = "no";    /* Norwegian (Nynorsk) */
          break;
        case 1096:
          language = "or";    /* Oriya */
          break;
        case 1138:
          language = "om";    /* Oromo */
          break;
        case 1145:
          language = NULL;    /* Papiamentu */
          break;
        case 1123:
          language = "ps";    /* Pashto */
          break;
        case 1045:
          language = "pl";    /* Polish */
          break;
        case 1046:
          language = "pt";    /* Portuguese - Brazil */
          break;
        case 2070:
          language = "pt";    /* Portuguese - Portugal */
          break;
        case 1094:
          language = "pa";    /* Punjabi */
          break;
        case 2118:
          language = "pa";    /* Punjabi (Pakistan) */
          break;
        case 1131:
          language = "qu";    /* Quecha - Bolivia */
          break;
        case 2155:
          language = "qu";    /* Quecha - Ecuador */
          break;
        case 3179:
          language = "qu";    /* Quecha - Peru */
          break;
        case 1047:
          language = "rm";    /* Rhaeto-Romanic */
          break;
        case 1048:
          language = "ro";    /* Romanian */
          break;
        case 2072:
          language = "ro";    /* Romanian - Moldava */
          break;
        case 1049:
          language = "ru";    /* Russian */
          break;
        case 2073:
          language = "ru";    /* Russian - Moldava */
          break;
        case 1083:
          language = NULL;    /* Sami (Lappish) */
          break;
        case 1103:
          language = "sa";    /* Sanskrit */
          break;
        case 1132:
          language = NULL;    /* Sepedi */
          break;
        case 3098:
          language = "sr";    /* Serbian (Cyrillic) */
          break;
        case 2074:
          language = "sr";    /* Serbian (Latin) */
          break;
        case 1113:
          language = "sd";    /* Sindhi - India */
          break;
        case 2137:
          language = "sd";    /* Sindhi - Pakistan */
          break;
        case 1115:
          language = "si";    /* Sinhalese - Sri Lanka */
          break;
        case 1051:
          language = "sk";    /* Slovak */
          break;
        case 1060:
          language = "sl";    /* Slovenian */
          break;
        case 1143:
          language = "so";    /* Somali */
          break;
        case 1070:
          language = NULL;    /* Sorbian */
          break;
        case 3082:
          language = "es";    /* Spanish - Spain (Modern Sort) */
          break;
        case 1034:
          language = "es";    /* Spanish - Spain (Traditional Sort) */
          break;
        case 11274:
          language = "es";    /* Spanish - Argentina */
          break;
        case 16394:
          language = "es";    /* Spanish - Bolivia */
          break;
        case 13322:
          language = "es";    /* Spanish - Chile */
          break;
        case 9226:
          language = "es";    /* Spanish - Colombia */
          break;
        case 5130:
          language = "es";    /* Spanish - Costa Rica */
          break;
        case 7178:
          language = "es";    /* Spanish - Dominican Republic */
          break;
        case 12298:
          language = "es";    /* Spanish - Ecuador */
          break;
        case 17418:
          language = "es";    /* Spanish - El Salvador */
          break;
        case 4106:
          language = "es";    /* Spanish - Guatemala */
          break;
        case 18442:
          language = "es";    /* Spanish - Honduras */
          break;
        case 58378:
          language = "es";    /* Spanish - Latin America */
          break;
        case 2058:
          language = "es";    /* Spanish - Mexico */
          break;
        case 19466:
          language = "es";    /* Spanish - Nicaragua */
          break;
        case 6154:
          language = "es";    /* Spanish - Panama */
          break;
        case 15370:
          language = "es";    /* Spanish - Paraguay */
          break;
        case 10250:
          language = "es";    /* Spanish - Peru */
          break;
        case 20490:
          language = "es";    /* Spanish - Puerto Rico */
          break;
        case 21514:
          language = "es";    /* Spanish - United States */
          break;
        case 14346:
          language = "es";    /* Spanish - Uruguay */
          break;
        case 8202:
          language = "es";    /* Spanish - Venezuela */
          break;
        case 1072:
          language = NULL;    /* Sutu */
          break;
        case 1089:
          language = "sw";    /* Swahili */
          break;
        case 1053:
          language = "sv";    /* Swedish */
          break;
        case 2077:
          language = "sv";    /* Swedish - Finland */
          break;
        case 1114:
          language = NULL;    /* Syriac */
          break;
        case 1064:
          language = "tg";    /* Tajik */
          break;
        case 1119:
          language = NULL;    /* Tamazight (Arabic) */
          break;
        case 2143:
          language = NULL;    /* Tamazight (Latin) */
          break;
        case 1097:
          language = "ta";    /* Tamil */
          break;
        case 1092:
          language = "tt";    /* Tatar */
          break;
        case 1098:
          language = "te";    /* Telugu */
          break;
        case 1054:
          language = "th";    /* Thai */
          break;
        case 2129:
          language = "bo";    /* Tibetan - Bhutan */
          break;
        case 1105:
          language = "bo";    /* Tibetan - People"s Republic of China */
          break;
        case 2163:
          language = "ti";    /* Tigrigna - Eritrea */
          break;
        case 1139:
          language = "ti";    /* Tigrigna - Ethiopia */
          break;
        case 1073:
          language = "ts";    /* Tsonga */
          break;
        case 1074:
          language = "tn";    /* Tswana */
          break;
        case 1055:
          language = "tr";    /* Turkish */
          break;
        case 1090:
          language = "tk";    /* Turkmen */
          break;
        case 1152:
          language = "ug";    /* Uighur - China */
          break;
        case 1058:
          language = "uk";    /* Ukrainian */
          break;
        case 1056:
          language = "ur";    /* Urdu */
          break;
        case 2080:
          language = "ur";    /* Urdu - India */
          break;
        case 2115:
          language = "uz";    /* Uzbek (Cyrillic) */
          break;
        case 1091:
          language = "uz";    /* Uzbek (Latin) */
          break;
        case 1075:
          language = "ve";    /* Venda */
          break;
        case 1066:
          language = "vi";    /* Vietnamese */
          break;
        case 1106:
          language = "cy";    /* Welsh */
          break;
        case 1076:
          language = "xh";    /* Xhosa */
          break;
        case 1144:
          language = NULL;    /* Yi */
          break;
        case 1085:
          language = "yi";    /* Yiddish */
          break;
        case 1130:
          language = "yo";    /* Yoruba */
          break;
        case 1077:
          language = "zu";    /* Zulu */
          break;
        default:
          language = NULL;
        }
    }
#endif

  /*  We already set the locale according to the environment, so just
   *  return early if no language is set in gimprc.
   */
  if (! language)
    return;

  g_setenv ("LANGUAGE", language, TRUE);
  setlocale (LC_ALL, "");
}

static gboolean
app_exit_after_callback (Gimp      *gimp,
                         gboolean   kill_it,
                         GMainLoop *loop)
{
  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  /*
   *  In stable releases, we simply call exit() here. This speeds up
   *  the process of quitting GIMP and also works around the problem
   *  that plug-ins might still be running.
   *
   *  In unstable releases, we shut down GIMP properly in an attempt
   *  to catch possible problems in our finalizers.
   */

#ifdef GIMP_UNSTABLE

  g_main_loop_quit (loop);

#else

  gegl_exit ();

  /*  make sure that the swap files are removed before we quit */
  tile_swap_exit ();

  exit (EXIT_SUCCESS);

#endif

  return FALSE;
}
