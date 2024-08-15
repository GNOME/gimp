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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* Win32 language lookup table:
 * Copyright (C) 2007-2008 Dieter Verfaillie <dieterv@optionexplicit.be>
 */

#include "config.h"

#ifdef HAVE__NL_IDENTIFICATION_LANGUAGE
#include <langinfo.h>
#endif
#include <locale.h>

#include <glib.h>

#ifdef G_OS_WIN32
#include <windows.h>
#include <winnls.h>
#endif
#ifdef PLATFORM_OSX
#include <Foundation/NSLocale.h>
#endif

#include "language.h"

#include "gimp-intl.h"


static gchar * language_get_system_lang_id (void);


const gchar *
language_init (const gchar  *language,
               const gchar **system_lang_l10n)
{
  static gchar *actual_language = NULL;
  static gchar *system_langstr  = NULL;

  if (actual_language != NULL)
    {
      /* Already initialized. */

      g_return_val_if_fail (system_langstr != NULL, actual_language);

      if (system_lang_l10n)
        *system_lang_l10n = system_langstr;

      return actual_language;
    }

  /* This must be localized with the system language itself, before we
   * apply any other language.
   */
  system_langstr = _("System Language");
  if (system_lang_l10n)
    *system_lang_l10n = system_langstr;


#ifdef G_OS_WIN32
  if ((! language || strlen (language) == 0) &&
      g_getenv ("LANG")        == NULL       &&
      g_getenv ("LC_MESSAGES") == NULL       &&
      g_getenv ("LC_ALL")      == NULL       &&
      g_getenv ("LANGUAGE")    == NULL)
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
          language = "af";          /* Afrikaans - South Africa */
          break;
        case 1052:
          language = "sq";          /* Albanian - Albania */
          break;
        case 1118:
          language = "am";          /* Amharic - Ethiopia */
          break;
        case 1025:
          language = "ar_SA";       /* Arabic - Saudi Arabia */
          break;
        case 5121:
          language = "ar_DZ";       /* Arabic - Algeria */
          break;
        case 15361:
          language = "ar_BH";       /* Arabic - Bahrain */
          break;
        case 3073:
          language = "ar_EG";       /* Arabic - Egypt */
          break;
        case 2049:
          language = "ar_IQ";       /* Arabic - Iraq */
          break;
        case 11265:
          language = "ar_JO";       /* Arabic - Jordan */
          break;
        case 13313:
          language = "ar_KW";       /* Arabic - Kuwait */
          break;
        case 12289:
          language = "ar_LB";       /* Arabic - Lebanon */
          break;
        case 4097:
          language = "ar_LY";       /* Arabic - Libya */
          break;
        case 6145:
          language = "ar_MO";       /* Arabic - Morocco */
          break;
        case 8193:
          language = "ar_OM";       /* Arabic - Oman */
          break;
        case 16385:
          language = "ar_QA";       /* Arabic - Qatar */
          break;
        case 10241:
          language = "ar_SY";       /* Arabic - Syria */
          break;
        case 7169:
          language = "ar_TN";       /* Arabic - Tunisia */
          break;
        case 14337:
          language = "ar_AE";       /* Arabic - U.A.E. */
          break;
        case 9217:
          language = "ar_YE";       /* Arabic - Yemen */
          break;
        case 1067:
          language = "hy";          /* Armenian - Armenia */
          break;
        case 1101:
          language = "as";          /* Assamese */
          break;
        case 2092:
          language = NULL;          /* Azeri (Cyrillic) */
          break;
        case 1068:
          language = NULL;          /* Azeri (Latin) */
          break;
        case 1069:
          language = "eu";          /* Basque */
          break;
        case 1059:
          language = "be";          /* Belarusian */
          break;
        case 1093:
          language = "bn_IN";       /* Bengali (India) */
          break;
        case 2117:
          language = "bn_BD";       /* Bengali (Bangladesh) */
          break;
        case 5146:
          language = "bs";          /* Bosnian (Bosnia/Herzegovina) */
          break;
        case 1026:
          language = "bg";          /* Bulgarian */
          break;
        case 1109:
          language = "my";          /* Burmese */
          break;
        case 1027:
          language = "ca";          /* Catalan */
          break;
        case 1116:
          language = NULL;          /* Cherokee - United States */
          break;
        case 2052:
          language = "zh_CN";       /* Chinese - People"s Republic of China */
          break;
        case 4100:
          language = "zh_SG";       /* Chinese - Singapore */
          break;
        case 1028:
          language = "zh_TW";       /* Chinese - Taiwan */
          break;
        case 3076:
          language = "zh_HK";       /* Chinese - Hong Kong SAR */
          break;
        case 5124:
          language = "zh_MO";       /* Chinese - Macao SAR */
          break;
        case 1050:
          language = "hr_HR";       /* Croatian */
          break;
        case 4122:
          language = "hr_BA";       /* Croatian (Bosnia/Herzegovina) */
          break;
        case 1029:
          language = "cs";          /* Czech */
          break;
        case 1030:
          language = "da";          /* Danish */
          break;
        case 1125:
          language = "dv";          /* Divehi */
          break;
        case 1043:
          language = "nl_NL";       /* Dutch - Netherlands */
          break;
        case 2067:
          language = "nl_BE";       /* Dutch - Belgium */
          break;
        case 1126:
          language = NULL;          /* Edo */
          break;
        case 1033:
          language = "en_US";       /* English - United States */
          break;
        case 2057:
          language = "en_UK";       /* English - United Kingdom */
          break;
        case 3081:
          language = "en_AU";       /* English - Australia */
          break;
        case 10249:
          language = "en_BZ";       /* English - Belize */
          break;
        case 4105:
          language = "en_CA";       /* English - Canada */
          break;
        case 9225:
          language = "en";          /* English - Caribbean */
          break;
        case 15369:
          language = "en_HK";       /* English - Hong Kong SAR */
          break;
        case 16393:
          language = "en_IN";       /* English - India */
          break;
        case 14345:
          language = "en_ID";       /* English - Indonesia */
          break;
        case 6153:
          language = "en_IR";       /* English - Ireland */
          break;
        case 8201:
          language = "en_JM";       /* English - Jamaica */
          break;
        case 17417:
          language = "en_MW";       /* English - Malaysia */
          break;
        case 5129:
          language = "en_NZ";       /* English - New Zealand */
          break;
        case 13321:
          language = "en_PH";       /* English - Philippines */
          break;
        case 18441:
          language = "en_SG";       /* English - Singapore */
          break;
        case 7177:
          language = "en_ZA";       /* English - South Africa */
          break;
        case 11273:
          language = "en_TT";       /* English - Trinidad */
          break;
        case 12297:
          language = "en_ZW";       /* English - Zimbabwe */
          break;
        case 1061:
          language = "et";          /* Estonian */
          break;
        case 1080:
          language = "fo";          /* Faroese */
          break;
        case 1065:
          language = "fa";          /* Farsi */
          break;
        case 1124:
          language = NULL;          /* Filipino */
          break;
        case 1035:
          language = "fi";          /* Finnish */
          break;
        case 1036:
          language = "fr_FR";       /* French - France */
          break;
        case 2060:
          language = "fr_BE";       /* French - Belgium */
          break;
        case 11276:
          language = "fr_CM";       /* French - Cameroon */
          break;
        case 3084:
          language = "fr_CA";       /* French - Canada */
          break;
        case 9228:
          language = "fr_CD";       /* French - Democratic Rep. of Congo */
          break;
        case 12300:
          language = "fr_CI";       /* French - Cote d"Ivoire */
          break;
        case 15372:
          language = "fr_HT";       /* French - Haiti */
          break;
        case 5132:
          language = "fr_LU";       /* French - Luxembourg */
          break;
        case 13324:
          language = "fr_ML";       /* French - Mali */
          break;
        case 6156:
          language = "fr_MC";       /* French - Monaco */
          break;
        case 14348:
          language = "fr_MA";       /* French - Morocco */
          break;
        case 58380:
          language = "fr";          /* French - North Africa */
          break;
        case 8204:
          language = "fr_RE";       /* French - Reunion */
          break;
        case 10252:
          language = "fr_SN";       /* French - Senegal */
          break;
        case 4108:
          language = "fr_CH";       /* French - Switzerland */
          break;
        case 7180:
          language = "fr";          /* French - West Indies */
          break;
        case 1122:
          language = "fy";          /* Frisian - Netherlands */
          break;
        case 1127:
          language = NULL;          /* Fulfulde - Nigeria */
          break;
        case 1071:
          language = "mk";          /* FYRO Macedonian */
          break;
        case 2108:
          language = "ga";          /* Gaelic (Ireland) */
          break;
        case 1084:
          language = "gd";          /* Gaelic (Scotland) */
          break;
        case 1110:
          language = "gl";          /* Galician */
          break;
        case 1079:
          language = "ka";          /* Georgian */
          break;
        case 1031:
          language = "de_DE";       /* German - Germany */
          break;
        case 3079:
          language = "de_AT";       /* German - Austria */
          break;
        case 5127:
          language = "de_LI";       /* German - Liechtenstein */
          break;
        case 4103:
          language = "de_LU";       /* German - Luxembourg */
          break;
        case 2055:
          language = "de_CH";       /* German - Switzerland */
          break;
        case 1032:
          language = "el";          /* Greek */
          break;
        case 1140:
          language = "gn";          /* Guarani - Paraguay */
          break;
        case 1095:
          language = "gu";          /* Gujarati */
          break;
        case 1128:
          language = "ha";          /* Hausa - Nigeria */
          break;
        case 1141:
          language = NULL;          /* Hawaiian - United States */
          break;
        case 1037:
          language = "he";          /* Hebrew */
          break;
        case 1081:
          language = "hi";          /* Hindi */
          break;
        case 1038:
          language = "hu";          /* Hungarian */
          break;
        case 1129:
          language = NULL;          /* Ibibio - Nigeria */
          break;
        case 1039:
          language = "is";          /* Icelandic */
          break;
        case 1136:
          language = "ig";          /* Igbo - Nigeria */
          break;
        case 1057:
          language = "id";          /* Indonesian */
          break;
        case 1117:
          language = "iu";          /* Inuktitut */
          break;
        case 1040:
          language = "it_IT";       /* Italian - Italy */
          break;
        case 2064:
          language = "it_CH";       /* Italian - Switzerland */
          break;
        case 1041:
          language = "ja";          /* Japanese */
          break;
        case 1099:
          language = "kn";          /* Kannada */
          break;
        case 1137:
          language = "kr";          /* Kanuri - Nigeria */
          break;
        case 2144:
          language = "ks";          /* Kashmiri */
          break;
        case 1120:
          language = "ks";          /* Kashmiri (Arabic) */
          break;
        case 1087:
          language = "kk";          /* Kazakh */
          break;
        case 1107:
          language = "km";          /* Khmer */
          break;
        case 1111:
          language = NULL;          /* Konkani */
          break;
        case 1042:
          language = "ko";          /* Korean */
          break;
        case 1088:
          language = "ky";          /* Kyrgyz (Cyrillic) */
          break;
        case 1108:
          language = "lo";          /* Lao */
          break;
        case 1142:
          language = "la";          /* Latin */
          break;
        case 1062:
          language = "lv";          /* Latvian */
          break;
        case 1063:
          language = "lt";          /* Lithuanian */
          break;
        case 1086:
          language = "ms_MY";       /* Malay - Malaysia */
          break;
        case 2110:
          language = "ms_BN";       /* Malay - Brunei Darussalam */
          break;
        case 1100:
          language = "ml";          /* Malayalam */
          break;
        case 1082:
          language = "mt";          /* Maltese */
          break;
        case 1112:
          language = NULL;          /* Manipuri */
          break;
        case 1153:
          language = "mi";          /* Maori - New Zealand */
          break;
        case 1102:
          language = "mr";          /* Marathi */
          break;
        case 1104:
          language = "mn";          /* Mongolian (Cyrillic) */
          break;
        case 2128:
          language = "mn";          /* Mongolian (Mongolian) */
          break;
        case 1121:
          language = "ne_NP";       /* Nepali */
          break;
        case 2145:
          language = "ne_IN";       /* Nepali - India */
          break;
        case 1044:
          language = "no";          /* Norwegian (Bokmￃﾥl) */
          break;
        case 2068:
          language = "no";          /* Norwegian (Nynorsk) */
          break;
        case 1096:
          language = "or";          /* Oriya */
          break;
        case 1138:
          language = "om";          /* Oromo */
          break;
        case 1145:
          language = NULL;          /* Papiamentu */
          break;
        case 1123:
          language = "ps";          /* Pashto */
          break;
        case 1045:
          language = "pl";          /* Polish */
          break;
        case 1046:
          language = "pt_BR";       /* Portuguese - Brazil */
          break;
        case 2070:
          language = "pt_PT";       /* Portuguese - Portugal */
          break;
        case 1094:
          language = "pa";          /* Punjabi */
          break;
        case 2118:
          language = "pa_PK";       /* Punjabi (Pakistan) */
          break;
        case 1131:
          language = "qu_BO";       /* Quecha - Bolivia */
          break;
        case 2155:
          language = "qu_EC";       /* Quecha - Ecuador */
          break;
        case 3179:
          language = "qu_PE";       /* Quecha - Peru */
          break;
        case 1047:
          language = "rm";          /* Rhaeto-Romanic */
          break;
        case 1048:
          language = "ro_RO";       /* Romanian */
          break;
        case 2072:
          language = "ro_MD";       /* Romanian - Moldava */
          break;
        case 1049:
          language = "ru_RU";       /* Russian */
          break;
        case 2073:
          language = "ru_MD";       /* Russian - Moldava */
          break;
        case 1083:
          language = NULL;          /* Sami (Lappish) */
          break;
        case 1103:
          language = "sa";          /* Sanskrit */
          break;
        case 1132:
          language = NULL;          /* Sepedi */
          break;
        case 3098:
          language = "sr";          /* Serbian (Cyrillic) */
          break;
        case 2074:
          language = "sr@latin";    /* Serbian (Latin) */
          break;
        case 1113:
          language = "sd_IN";       /* Sindhi - India */
          break;
        case 2137:
          language = "sd_PK";       /* Sindhi - Pakistan */
          break;
        case 1115:
          language = "si";          /* Sinhalese - Sri Lanka */
          break;
        case 1051:
          language = "sk";          /* Slovak */
          break;
        case 1060:
          language = "sl";          /* Slovenian */
          break;
        case 1143:
          language = "so";          /* Somali */
          break;
        case 1070:
          language = NULL;          /* Sorbian */
          break;
        case 3082:
          language = "es";          /* Spanish - Spain (Modern Sort) */
          break;
        case 1034:
          language = "es";          /* Spanish - Spain (Traditional Sort) */
          break;
        case 11274:
          language = "es_AR";       /* Spanish - Argentina */
          break;
        case 16394:
          language = "es_BO";       /* Spanish - Bolivia */
          break;
        case 13322:
          language = "es_CL";       /* Spanish - Chile */
          break;
        case 9226:
          language = "es_CO";       /* Spanish - Colombia */
          break;
        case 5130:
          language = "es_CR";       /* Spanish - Costa Rica */
          break;
        case 7178:
          language = "es_DO";       /* Spanish - Dominican Republic */
          break;
        case 12298:
          language = "es_EC";       /* Spanish - Ecuador */
          break;
        case 17418:
          language = "es_SV";       /* Spanish - El Salvador */
          break;
        case 4106:
          language = "es_GT";       /* Spanish - Guatemala */
          break;
        case 18442:
          language = "es_HN";       /* Spanish - Honduras */
          break;
        case 58378:
          language = "es";          /* Spanish - Latin America */
          break;
        case 2058:
          language = "es_MX";       /* Spanish - Mexico */
          break;
        case 19466:
          language = "es_NI";       /* Spanish - Nicaragua */
          break;
        case 6154:
          language = "es_PA";       /* Spanish - Panama */
          break;
        case 15370:
          language = "es_PY";       /* Spanish - Paraguay */
          break;
        case 10250:
          language = "es_PE";       /* Spanish - Peru */
          break;
        case 20490:
          language = "es_PR";       /* Spanish - Puerto Rico */
          break;
        case 21514:
          language = "es_US";       /* Spanish - United States */
          break;
        case 14346:
          language = "es_UY";       /* Spanish - Uruguay */
          break;
        case 8202:
          language = "es_VE";       /* Spanish - Venezuela */
          break;
        case 1072:
          language = NULL;          /* Sutu */
          break;
        case 1089:
          language = "sw";          /* Swahili */
          break;
        case 1053:
          language = "sv_SE";       /* Swedish */
          break;
        case 2077:
          language = "sv_FI";       /* Swedish - Finland */
          break;
        case 1114:
          language = NULL;          /* Syriac */
          break;
        case 1064:
          language = "tg";          /* Tajik */
          break;
        case 1119:
          language = NULL;          /* Tamazight (Arabic) */
          break;
        case 2143:
          language = NULL;          /* Tamazight (Latin) */
          break;
        case 1097:
          language = "ta";          /* Tamil */
          break;
        case 1092:
          language = "tt";          /* Tatar */
          break;
        case 1098:
          language = "te";          /* Telugu */
          break;
        case 1054:
          language = "th";          /* Thai */
          break;
        case 2129:
          language = "bo_BT";       /* Tibetan - Bhutan */
          break;
        case 1105:
          language = "bo_CN";       /* Tibetan - People"s Republic of China */
          break;
        case 2163:
          language = "ti_ER";       /* Tigrigna - Eritrea */
          break;
        case 1139:
          language = "ti_ET";       /* Tigrigna - Ethiopia */
          break;
        case 1073:
          language = "ts";          /* Tsonga */
          break;
        case 1074:
          language = "tn";          /* Tswana */
          break;
        case 1055:
          language = "tr";          /* Turkish */
          break;
        case 1090:
          language = "tk";          /* Turkmen */
          break;
        case 1152:
          language = "ug";          /* Uighur - China */
          break;
        case 1058:
          language = "uk";          /* Ukrainian */
          break;
        case 1056:
          language = "ur";          /* Urdu */
          break;
        case 2080:
          language = "ur_IN";       /* Urdu - India */
          break;
        case 2115:
          language = "uz";          /* Uzbek (Cyrillic) */
          break;
        case 1091:
          language = "uz@latin";    /* Uzbek (Latin) */
          break;
        case 1075:
          language = "ve";          /* Venda */
          break;
        case 1066:
          language = "vi";          /* Vietnamese */
          break;
        case 1106:
          language = "cy";          /* Welsh */
          break;
        case 1076:
          language = "xh";          /* Xhosa */
          break;
        case 1144:
          language = NULL;          /* Yi */
          break;
        case 1085:
          language = "yi";          /* Yiddish */
          break;
        case 1130:
          language = "yo";          /* Yoruba */
          break;
        case 1077:
          language = "zu";          /* Zulu */
          break;
        default:
          language = NULL;
        }
    }
#endif

  /*  We already set the locale according to the environment, so just
   *  return early if no language is set in gimprc.
   */
  if (! language || strlen (language) == 0)
    {
      actual_language = language_get_system_lang_id ();
    }
  else
    {
      g_setenv ("LANGUAGE", language, TRUE);
      g_setenv ("LANG", language, TRUE);
      setlocale (LC_ALL, "");

      actual_language = g_strdup (language);
    }

  return actual_language;
}

static gchar *
language_get_system_lang_id (void)
{
  const gchar *syslang = NULL;

  /* Using system language. It doesn't matter too much that the string
   * format is different when using system or preference-set language,
   * because this string is only used for comparison. As long as 2
   * similar run have the same settings, the strings will be
   * identical.
   */
#if defined G_OS_WIN32
  return g_strdup_printf ("LANGID-%d", GetUserDefaultUILanguage());
#elif defined PLATFORM_OSX
  NSString *langs;

  /* In macOS, the user sets a list of prefered languages and the
   * software respects this preference order. I.e. that just storing the
   * top-prefered lang would not be enough. What if GIMP didn't have
   * translations for it, then it would fallback to the second lang. If
   * this second lang changed, GIMP localization would change but we
   * would not be aware of it. Instead, let's use the whole list as our
   * language identifier. If this list changes in any way, we consider
   * the lang may have potentially changed.
   */
  langs = [[NSLocale preferredLanguages] componentsJoinedByString:@","];

  return g_strdup_printf ("%s", [langs UTF8String]);
#elif defined HAVE__NL_IDENTIFICATION_LANGUAGE
  syslang = nl_langinfo (_NL_IDENTIFICATION_LANGUAGE);
#endif

  if (syslang == NULL || strlen (syslang) == 0)
    /* This should return an opaque string which represents the whole
     * locale configuration on this system.
     */
    syslang = setlocale (LC_ALL, NULL);

  /* I don't think we'd ever get here but just in case, as a last
   * resort, if none of the previous methods returned a valid result,
   * let's just check environment variables ourselves.
   * This is the proper order of priority.
   */
  if (syslang == NULL || strlen (syslang) == 0)
    syslang = g_getenv ("LANGUAGE");
  if (syslang == NULL || strlen (syslang) == 0)
    syslang = g_getenv ("LC_ALL");
  if (syslang == NULL || strlen (syslang) == 0)
    syslang = g_getenv ("LC_MESSAGES");
  if (syslang == NULL || strlen (syslang) == 0)
    syslang = g_getenv ("LANG");

  return syslang && strlen (syslang) > 0 ? g_strdup (syslang) : NULL;
}
