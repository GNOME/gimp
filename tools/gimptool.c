/* gimptool in C
 * Copyright (C) 2001-2007 Tor Lillqvist
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

/*
 * Gimptool rewritten in C, originally for Win32, where end-users who
 * might want to build and install a plug-in from source don't
 * necessarily have any Bourne-compatible shell to run the gimptool
 * script in. Later fixed up to replace the gimptool script on all
 * platforms.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif


static gboolean     silent  = FALSE;
static gboolean     dry_run = FALSE;
static const gchar *cli_prefix;
static const gchar *cli_exec_prefix;

static gboolean     msvc_syntax = FALSE;
static const gchar *env_cc;
static const gchar *env_cflags;
static const gchar *env_ldflags;
static const gchar *env_libs;


#ifdef G_OS_WIN32
#define EXEEXT ".exe"
#else
#define EXEEXT ""
#endif

#ifdef G_OS_WIN32
#define COPY win32_command ("copy")
#define REMOVE win32_command ("del")
#define REMOVE_DIR win32_command ("rd /s /q")
#else
#define COPY "cp"
#define REMOVE "rm -f"
#define REMOVE_DIR "rm -Rf"
#endif

static struct {
  const gchar *option;
  const gchar *value;
} dirs[] = {
  { "prefix",         PREFIX         },
  { "exec-prefix",    EXEC_PREFIX    },
  { "bindir",         BINDIR         },
  { "sbindir",        SBINDIR        },
  { "libexecdir",     LIBEXECDIR     },
  { "datadir",        DATADIR        },
  { "datarootdir",    DATAROOTDIR    },
  { "sysconfdir",     SYSCONFDIR     },
  { "sharedstatedir", SHAREDSTATEDIR },
  { "localstatedir",  LOCALSTATEDIR  },
  { "libdir",         LIBDIR         },
  { "infodir",        INFODIR        },
  { "mandir",         MANDIR         },
#if 0
  /* For --includedir we want the includedir of the developer package,
   * not an includedir under the runtime installation prefix.
   */
  { "includedir",     INCLUDEDIR     },
#endif
  { "gimpplugindir",  GIMPPLUGINDIR  },
  { "gimpdatadir",    GIMPDATADIR    }
};


static void  usage (int exit_status) G_GNUC_NORETURN;


#ifdef G_OS_WIN32

static const gchar *
win32_command (const gchar *command)
{
  static gchar *cmd     = NULL;
  const gchar  *comspec = getenv ("COMSPEC");

  if (comspec == NULL)
    comspec = "cmd.exe";

  g_free (cmd);
  cmd = g_strdup_printf ("%s /c %s", comspec, command);

  return (const gchar *) cmd;
}

/* Windows shells break with auto-quote. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/6378 */
static gchar *
hollow_g_shell_quote (const gchar *input)
{
  return g_strdup (input);
}

#define g_shell_quote hollow_g_shell_quote
#endif

static gboolean
starts_with_dir (const gchar *string,
                 const gchar *dir)
{
  gchar    *dirslash = g_strconcat (dir, "/", NULL);
  gboolean  retval;

  retval = (g_str_has_prefix (string, dirslash) ||
            g_strcmp0 (string, dir) == 0);
  g_free (dirslash);

  return retval;
}

static gchar *
one_line_output (const gchar *program,
                 const gchar *args)
{
  gchar *command = g_strconcat (program, " ", args, NULL);
  FILE  *pipe    = popen (command, "r");
  gchar  line[4096];

  if (pipe == NULL)
    {
      g_printerr ("Cannot run '%s'\n", command);
      g_free (command);
      exit (EXIT_FAILURE);
    }

  if (fgets (line, sizeof (line), pipe) == NULL)
    line[0] = '\0';

  if (strlen (line) > 0 && line [strlen (line) - 1] == '\n')
    line [strlen (line) - 1] = '\0';
  if (strlen (line) > 0 && line [strlen (line) - 1] == '\r')
    line [strlen (line) - 1] = '\0';

  pclose (pipe);

  if (strlen (line) == 0)
    {
      g_printerr ("No output from '%s'\n", command);
      g_free (command);
      exit (EXIT_FAILURE);
    }
  g_free (command);

  return g_strdup (line);
}

static gchar *
pkg_config (const gchar *args)
{
#ifdef G_OS_WIN32
  if (msvc_syntax)
    return one_line_output ("pkg-config --msvc-syntax", args);
#endif

  return one_line_output ("pkg-config", args);
}

static gchar *
get_runtime_prefix (gchar slash)
{
#ifdef G_OS_WIN32

  /* Don't use the developer package prefix, but deduce the
   * installation-time prefix from where gimp-x.y.exe can be found.
   */

  gchar *path;
  gchar *p, *r;

  path = g_find_program_in_path ("gimp-" GIMP_APP_VERSION ".exe");

  if (path == NULL)
    path = g_find_program_in_path ("gimp.exe");

  if (path != NULL)
    {
      r = strrchr (path, G_DIR_SEPARATOR);
      if (r != NULL)
        {
          *r = '\0';
          if (strlen (path) >= 4 &&
              g_ascii_strcasecmp (r - 4, G_DIR_SEPARATOR_S "bin") == 0)
            {
              r[-4] = '\0';
              if (slash == '/')
                {
                  /* Use forward slashes, less quoting trouble in Makefiles */
                  while ((p = strchr (path, '\\')) != NULL)
                    *p = '/';
                }
              return path;
            }
        }
    }

  g_printerr ("Cannot determine GIMP " GIMP_APP_VERSION " installation location\n");

  exit (EXIT_FAILURE);
#else
  /* On Unix assume the executable package is in the same prefix as the developer stuff */
  return pkg_config ("--variable=prefix gimp-" GIMP_PKGCONFIG_VERSION);
#endif
}

static gchar *
get_exec_prefix (gchar slash)
{
#ifdef G_OS_WIN32
  if (cli_exec_prefix != NULL)
    return g_strdup (cli_exec_prefix);

  /* On Win32, exec_prefix is always same as prefix. Or is it? Maybe not,
   * but at least in tml's prebuilt stuff it is. If somebody else does
   * it another way, feel free to hack this.
   */
  return get_runtime_prefix (slash);
#else
  return g_strdup (EXEC_PREFIX);
#endif
}

static gchar *
expand_and_munge (const gchar *value)
{
  gchar *retval;

  if (starts_with_dir (value, "${prefix}"))
    retval = g_strconcat (PREFIX, value + strlen ("${prefix}"), NULL);
  else if (starts_with_dir (value, "${exec_prefix}"))
    retval = g_strconcat (EXEC_PREFIX, value + strlen ("${exec_prefix}"), NULL);
  else
    retval = g_strdup (value);

  if (starts_with_dir (retval, EXEC_PREFIX))
    {
      gchar *exec_prefix = get_exec_prefix ('/');

      retval = g_strconcat (exec_prefix, retval + strlen (EXEC_PREFIX), NULL);
      g_free (exec_prefix);
    }

  if (starts_with_dir (retval, PREFIX))
    {
      gchar *runtime_prefix = get_runtime_prefix ('/');

      retval = g_strconcat (runtime_prefix, retval + strlen (PREFIX), NULL);
      g_free (runtime_prefix);
    }

  return retval;
}

static void
find_out_env_flags (void)
{
  gchar *p;

  if ((p = getenv ("CC")) != NULL && *p != '\0')
    env_cc = p;
  else if (msvc_syntax)
    env_cc = "cl -MD";
  else
    env_cc = CC;

  if (g_ascii_strncasecmp (env_cc, "cl", 2)    == 0 &&
      g_ascii_strncasecmp (env_cc, "clang", 5) != 0)
    msvc_syntax = TRUE;

  if ((p = getenv ("CFLAGS")) != NULL)
    env_cflags = p;
  else
    env_cflags = "";

  if ((p = getenv ("LDFLAGS")) != NULL)
    env_ldflags = p;
  else
    env_ldflags = "";

  if ((p = getenv ("LIBS")) != NULL && *p != '\0')
    env_libs = p;
  else
    env_libs = "";
}

static void
usage (int exit_status)
{
  g_print ("\
Usage: gimptool-" GIMP_TOOL_VERSION " [OPTION]...\n\
\n\
General options:\n\
  --help                  print this message\n\
  --quiet, --silent       don't echo build commands\n\
  --version               print the version of GIMP associated with this script\n\
  -n, --just-print, --dry-run, --recon\n\
                          don't actually run any commands; just print them\n\
Developer options:\n\
  --cflags                print the compiler flags that are necessary to\n\
                          compile a plug-in\n\
  --libs                  print the linker flags that are necessary to link a\n\
                          plug-in\n\
  --prefix=PREFIX         use PREFIX instead of the installation prefix that\n\
                          GIMP was built when computing the output for --cflags\n\
                          and --libs\n\
  --exec-prefix=PREFIX    use PREFIX instead of the installation exec prefix\n\
                          that GIMP was built when computing the output for\n\
                          --cflags and --libs\n\
  --msvc-syntax           print flags in MSVC syntax\n\
\n\
Installation directory options:\n\
  --prefix --exec-prefix --bindir --sbindir --libexecdir --datadir --sysconfdir\n\
  --sharedstatedir --localstatedir --libdir --infodir --mandir --includedir\n\
  --gimpplugindir --gimpdatadir\n\
\n\
The --cflags and --libs options can be appended with -noui to get appropriate\n\
settings for plug-ins which do not use GTK+.\n\
\n\
User options:\n\
  --build plug-in.c               build a plug-in from a source file\n\
  --install plug-in.c             same as --build, but installs the built\n\
                                  plug-in as well\n\
  --install-bin plug-in           install a compiled plug-in\n\
  --install-script script.scm     install a script-fu script\n\
\n\
  --uninstall-bin plug-in         remove a plug-in again\n\
  --uninstall-script plug-in      remove a script-fu script\n\
\n\
The --install and --uninstall options have \"admin\" counterparts (with\n\
prefix --install-admin instead of --install) that can be used instead to\n\
install/uninstall a plug-in or script in the machine directory instead of a\n\
user directory.\n\
\n\
For plug-ins which do not use GTK+, the --build and --install options can be\n\
appended with -noui for appropriate settings. For plug-ins that use GTK+ but\n\
not libgimpui, append -nogimpui.\n");
  exit (exit_status);
}

static gchar *
get_includedir (void)
{
  return pkg_config ("--variable=includedir gimp-" GIMP_PKGCONFIG_VERSION);
}

static void
do_includedir (void)
{
  gchar *includedir = get_includedir ();

  g_print ("%s\n", includedir);
  g_free (includedir);
}

static gchar *
get_cflags (void)
{
  return pkg_config ("--cflags gimpui-" GIMP_PKGCONFIG_VERSION);
}

static void
do_cflags (void)
{
  gchar *cflags = get_cflags ();

  g_print ("%s\n", cflags);
  g_free (cflags);
}

static gchar *
get_cflags_noui (void)
{
  return pkg_config ("--cflags gimp-" GIMP_PKGCONFIG_VERSION);
}

static void
do_cflags_noui (void)
{
  gchar *cflags = get_cflags_noui ();

  g_print ("%s\n", cflags);
  g_free (cflags);
}

static gchar *
get_cflags_nogimpui (void)
{
  return pkg_config ("--cflags gimp-" GIMP_PKGCONFIG_VERSION " gtk+-3.0");
}

static void
do_cflags_nogimpui (void)
{
  gchar *cflags = get_cflags_nogimpui ();

  g_print ("%s\n", cflags);
  g_free (cflags);
}

static gchar *
get_libs (void)
{
  return pkg_config ("--libs gimpui-" GIMP_PKGCONFIG_VERSION);
}

static void
do_libs (void)
{
  gchar *libs = get_libs ();

  g_print ("%s\n", libs);
  g_free (libs);
}

static gchar *
get_libs_noui (void)
{
  return pkg_config ("--libs gimp-" GIMP_PKGCONFIG_VERSION);
}

static void
do_libs_noui (void)
{
  gchar *libs = get_libs_noui ();

  g_print ("%s\n", libs);
  g_free (libs);
}

static gchar *
get_libs_nogimpui (void)
{
  return pkg_config ("--libs gimp-" GIMP_PKGCONFIG_VERSION " gtk+-3.0");
}

static void
do_libs_nogimpui (void)
{
  gchar *libs = get_libs_nogimpui ();

  g_print ("%s\n", libs);
  g_free (libs);
}

static void
maybe_run (gchar *cmd)
{
  if (!silent)
    g_print ("%s\n", cmd);

  /* system() declared with attribute warn_unused_result.
   * Trick to get rid of the compilation warning without using the result.
   */
  if (dry_run || system (cmd))
    ;
}

static void
do_build_2 (const gchar *cflags,
            const gchar *libs,
            const gchar *install_dir,
            const gchar *what)
{
  const gchar *lang_flag = "";
  const gchar *output_flag;
  const gchar *here_comes_linker_flags = "";
  const gchar *windows_subsystem_flag = "";
  gchar       *cmd;
  gchar       *dest_dir;
  gchar       *dest_exe;
  gchar       *source = g_shell_quote (what);

  gchar       *tmp;
  gchar       *p, *q;

  if (install_dir != NULL)
    dest_dir = g_strconcat (install_dir, "/", NULL);
  else
    dest_dir = g_strdup ("");

  dest_exe = g_strdup (what);

  p = strrchr (dest_exe, '.');
  if (p == NULL ||
      !(strcmp (p, ".c")   == 0 ||
        strcmp (p, ".cc")  == 0 ||
        strcmp (p, ".cpp") == 0))
    {
      /* If the file doesn't have a "standard" C/C++ suffix and:
       * 1) if the compiler is known as a C++ compiler, then treat the file as a
       *    C++ file if possible.
       *    It's known that G++ and Clang++ treat a file as a C file if they are
       *    run with the "-x c++" option.
       * 2) if the compiler is known as a C compiler or a multiple-language
       *    compiler, then treat the file as a C file if possible.
       *    It's known that GCC and Clang treat a file as a C file if they are
       *    run with the "-x c" option.
       * TODO We may want to further support compilation with a source file
       * without a standard suffix in more compilers as far as possible.
       */
      if (strcmp (env_cc, "g++") == 0 ||
          strncmp (env_cc, "g++-", sizeof ("g++-") - 1) == 0 ||
          strcmp (env_cc, "clang++") == 0 ||
          strncmp (env_cc, "clang++-", sizeof ("clang++-") - 1) == 0)
        lang_flag = "-x c++ ";
      else if (strcmp (env_cc, "gcc") == 0 ||
               strncmp (env_cc, "gcc-", sizeof ("gcc-") - 1) == 0)
        {
          /* It's known GCC recognizes .CPP and .cxx, so bypass these suffixes */
          if (p != NULL && strcmp (p, ".CPP") != 0 && strcmp (p, ".cxx") != 0)
            lang_flag = "-x c ";
        }
      else if (strcmp (env_cc, "clang") == 0 ||
               strncmp (env_cc, "clang-", sizeof ("clang-") - 1) == 0)
        {
          /* It's known Clang recognizes .CC, .CPP, .cxx and .CXX,
           * so bypass these suffixes
           */
          if (p != NULL && strcmp (p, ".CC") != 0 && strcmp (p, ".CPP") != 0 &&
              strcmp (p, ".cxx") != 0 && strcmp (p, ".CXX") != 0)
            lang_flag = "-x c ";
        }
      else
        {
          g_printerr ("The source file (%s) doesn't have a \"standard\" C or C++ suffix, "
                      "and the tool failed to confirm the language of the file.\n"
                      "Please be explicit about the language of the file "
                      "by renaming it with one of the suffixes: .c .cc .cpp\n",
                      what);
          exit (EXIT_FAILURE);
        }
    }

  if (p)
    *p = '\0';
  q = strrchr (dest_exe, G_DIR_SEPARATOR);
#ifdef G_OS_WIN32
  {
    gchar *r = strrchr (dest_exe, '/');
    if (r != NULL && (q == NULL || r > q))
      q = r;
  }
#endif
  if (q == NULL)
    q = dest_exe;
  else
    q++;

  if (install_dir)
    {
      tmp      = dest_dir;
      dest_dir = g_strconcat (dest_dir, q, G_DIR_SEPARATOR_S, NULL);
      g_free (tmp);

      g_mkdir_with_parents (dest_dir,
                            S_IRUSR | S_IXUSR | S_IWUSR |
                            S_IRGRP | S_IXGRP |
                            S_IROTH | S_IXOTH);
    }

  tmp = g_strconcat (dest_dir, q, NULL);
  g_free (dest_dir);

  dest_exe = g_shell_quote (tmp);
  g_free (tmp);

  if (msvc_syntax)
    {
      output_flag = "-Fe";
      here_comes_linker_flags = " -link";
      windows_subsystem_flag = " -subsystem:windows";
    }
  else
    {
      output_flag = "-o ";
#ifdef G_OS_WIN32
      windows_subsystem_flag = " -mwindows";
#endif
    }

  cmd = g_strdup_printf ("%s %s%s %s %s%s %s%s %s%s %s %s",
                         env_cc,
                         lang_flag,
                         env_cflags,
                         cflags,
                         output_flag,
                         dest_exe,
                         source,
                         here_comes_linker_flags,
                         env_ldflags,
                         windows_subsystem_flag,
                         libs,
                         env_libs);

  maybe_run (cmd);

  g_free (dest_exe);
  g_free (source);
}

static void
do_build (const gchar *what)
{
  gchar *cflags = get_cflags ();
  gchar *libs   = get_libs ();

  do_build_2 (cflags, libs, NULL, what);

  g_free (cflags);
  g_free (libs);
}

static void
do_build_noui (const gchar *what)
{
  gchar *cflags = get_cflags_noui ();
  gchar *libs   = get_libs_noui ();

  do_build_2 (cflags, libs, NULL, what);

  g_free (cflags);
  g_free (libs);
}

static void
do_build_nogimpui (const gchar *what)
{
  do_build (what);
}

static gchar *
get_user_plugin_dir (void)
{
  return g_build_filename (gimp_directory (),
                           "plug-ins",
                           NULL);
}

static gchar *
get_plugin_dir (const gchar *base_dir,
                const gchar *what)
{
  gchar *separator, *dot, *plugin_name, *plugin_dir;
  gchar *tmp = g_strdup (what);

  separator = strrchr (tmp, G_DIR_SEPARATOR);
#ifdef G_OS_WIN32
    {
      gchar *alt_separator = strrchr (tmp, '/');

      if (alt_separator != NULL &&
          (separator == NULL || alt_separator > separator))
        {
          separator = alt_separator;
        }
    }
#endif

  dot = strrchr (tmp, '.');

  if (separator)
    plugin_name = separator + 1;
  else
    plugin_name = tmp;

  if (dot)
    *dot = '\0';

  plugin_dir = g_strconcat (base_dir,
                            G_DIR_SEPARATOR_S,
                            plugin_name,
                            NULL);

  g_free (tmp);

  return plugin_dir;
}

static void
do_install (const gchar *what)
{
  gchar *cflags = get_cflags ();
  gchar *libs   = get_libs ();
  gchar *dir    = get_user_plugin_dir ();

  do_build_2 (cflags, libs, dir, what);

  g_free (cflags);
  g_free (libs);
  g_free (dir);
}

static void
do_install_noui (const gchar *what)
{
  gchar *cflags = get_cflags_noui ();
  gchar *libs   = get_libs_noui ();
  gchar *dir    = get_user_plugin_dir ();

  do_build_2 (cflags, libs, dir, what);

  g_free (cflags);
  g_free (libs);
  g_free (dir);
}

static void
do_install_nogimpui (const gchar *what)
{
  do_install (what);
}

static gchar *
get_sys_plugin_dir (gboolean forward_slashes)
{
  gchar *dir;

#ifdef G_OS_WIN32
  gchar *rprefix;

  rprefix = get_runtime_prefix (forward_slashes ? '/' : G_DIR_SEPARATOR);

  dir = g_build_path (forward_slashes ? "/" : G_DIR_SEPARATOR_S,
                      rprefix,
                      "lib", "gimp",
                      GIMP_PLUGIN_VERSION,
                      "plug-ins",
                      NULL);
  g_free (rprefix);
#else
  dir = g_build_path (forward_slashes ? "/" : G_DIR_SEPARATOR_S,
                      LIBDIR,
                      "gimp",
                      GIMP_PLUGIN_VERSION,
                      "plug-ins",
                      NULL);
#endif

  return dir;
}

static void
do_install_admin (const gchar *what)
{
  gchar *cflags = get_cflags ();
  gchar *libs   = get_libs ();
  gchar *dir    = get_sys_plugin_dir (TRUE);

  do_build_2 (cflags, libs, dir, what);

  g_free (cflags);
  g_free (libs);
  g_free (dir);
}

static void
do_install_admin_noui (const gchar *what)
{
  gchar *cflags = get_cflags_noui ();
  gchar *libs   = get_libs_noui ();
  gchar *dir    = get_sys_plugin_dir (TRUE);

  do_build_2 (cflags, libs, dir, what);

  g_free (cflags);
  g_free (libs);
  g_free (dir);
}

static void
do_install_admin_nogimpui (const gchar *what)
{
  gchar *cflags = get_cflags ();
  gchar *libs   = get_libs ();
  gchar *dir    = get_sys_plugin_dir (TRUE);

  do_build_2 (cflags, libs, dir, what);

  g_free (cflags);
  g_free (libs);
  g_free (dir);
}

static void
do_install_bin_2 (const gchar *dir,
                  const gchar *what)
{
  gchar *cmd;
  gchar *quoted_src;
  gchar *quoted_dir;

  gchar *dest_dir = g_strconcat (dir, G_DIR_SEPARATOR_S, NULL);

  g_mkdir_with_parents (dest_dir,
                        S_IRUSR | S_IXUSR | S_IWUSR |
                        S_IRGRP | S_IXGRP |
                        S_IROTH | S_IXOTH);

  quoted_src = g_shell_quote (what);
  quoted_dir = g_shell_quote (dest_dir);
  cmd = g_strconcat (COPY, " ", quoted_src, " ", quoted_dir, NULL);
  maybe_run (cmd);

  g_free (dest_dir);
  g_free (cmd);
  g_free (quoted_src);
  g_free (quoted_dir);
}

static void
do_install_bin (const gchar *what)
{
  gchar *dir = get_user_plugin_dir ();
  gchar *plugin_dir = get_plugin_dir (dir, what);

  do_install_bin_2 (plugin_dir, what);

  g_free (plugin_dir);
  g_free (dir);
}

static void
do_install_admin_bin (const gchar *what)
{
  gchar *dir = get_sys_plugin_dir (FALSE);
  gchar *plugin_dir = get_plugin_dir (dir, what);

  do_install_bin_2 (dir, what);

  g_free (plugin_dir);
  g_free (dir);
}

static void
do_uninstall (const gchar *dir)
{
  gchar *cmd;
  gchar *quoted_src;

  quoted_src = g_shell_quote (dir);

  cmd = g_strconcat (REMOVE_DIR, " ", quoted_src, NULL);
  maybe_run (cmd);

  g_free (cmd);
  g_free (quoted_src);
}

static void
do_uninstall_script_2 (const gchar *dir,
                       const gchar *what)
{
  gchar *cmd;
  gchar *quoted_src;
  gchar *src;

  src = g_strconcat (dir, G_DIR_SEPARATOR_S, what, NULL);
  quoted_src = g_shell_quote (src);

  cmd = g_strconcat (REMOVE, " ", quoted_src, NULL);
  maybe_run (cmd);

  g_free (cmd);
  g_free (quoted_src);
  g_free (src);
}

static gchar *
maybe_append_exe (const gchar *what)
{
#ifdef G_OS_WIN32
  gchar *p = strrchr (what, '.');

  if (p == NULL || g_ascii_strcasecmp (p, ".exe") != 0)
    return g_strconcat (what, ".exe", NULL);
#endif

  return g_strdup (what);
}

static void
do_uninstall_bin (const gchar *what)
{
  gchar *dir        = get_user_plugin_dir ();
  gchar *exe        = maybe_append_exe (what);
  gchar *plugin_dir = get_plugin_dir (dir, what);

  do_uninstall (plugin_dir);

  g_free (plugin_dir);
  g_free (dir);
  g_free (exe);
}

static void
do_uninstall_admin_bin (const gchar *what)
{
  gchar *dir        = get_sys_plugin_dir (FALSE);
  gchar *exe        = maybe_append_exe (what);
  gchar *plugin_dir = get_plugin_dir (dir, what);

  do_uninstall (plugin_dir);

  g_free (plugin_dir);
  g_free (dir);
  g_free (exe);
}

static gchar *
get_user_script_dir (void)
{
  return g_build_filename (gimp_directory (),
                           "scripts",
                           NULL);
}

static void
do_install_script (const gchar *what)
{
  gchar *dir = get_user_script_dir ();

  do_install_bin_2 (dir, what);
  g_free (dir);
}

static gchar *
get_sys_script_dir (void)
{
  gchar *dir;
  gchar *prefix = get_runtime_prefix (G_DIR_SEPARATOR);

  dir = g_build_filename (prefix, "share", "gimp",
                          GIMP_PLUGIN_VERSION, "scripts",
                          NULL);
  g_free (prefix);

  return dir;
}

static void
do_install_admin_script (const gchar *what)
{
  gchar *dir = get_sys_script_dir ();

  do_install_bin_2 (dir, what);
  g_free (dir);
}

static void
do_uninstall_script (const gchar *what)
{
  gchar *dir = get_user_script_dir ();

  do_uninstall_script_2 (dir, what);
  g_free (dir);
}

static void
do_uninstall_admin_script (const gchar *what)
{
  gchar *dir = get_sys_script_dir ();

  do_uninstall_script_2 (dir, what);
  g_free (dir);
}

int
main (int    argc,
      char **argv)
{
  gint argi;
  gint i;

  if (argc == 1)
    usage (EXIT_SUCCESS);

  /* First scan for flags that affect our behavior globally, but
   * are still allowed late on the command line.
   */
  argi = 0;
  while (++argi < argc)
    {
      if (strcmp (argv[argi], "-n") == 0 ||
          strcmp (argv[argi], "--just-print") == 0 ||
          strcmp (argv[argi], "--dry-run") == 0 ||
          strcmp (argv[argi], "--recon") == 0)
        {
          dry_run = TRUE;
        }
      else if (strcmp (argv[argi], "--help") == 0)
        {
          usage (EXIT_SUCCESS);
        }
      else if (g_str_has_prefix (argv[argi], "--prefix="))
        {
          cli_prefix = argv[argi] + strlen ("--prefix=");
        }
      else if (g_str_has_prefix (argv[argi], "--exec-prefix="))
        {
          cli_exec_prefix = argv[argi] + strlen ("--exec_prefix=");
        }
      else if (strcmp (argv[argi], "--msvc-syntax") == 0)
        {
#ifdef G_OS_WIN32
          msvc_syntax = TRUE;
#else
          g_printerr ("Ignoring --msvc-syntax\n");
#endif
        }
    }

  find_out_env_flags ();

  /* Second pass, actually do something. */
  argi = 0;
  while (++argi < argc)
    {
      for (i = 0; i < G_N_ELEMENTS (dirs); i++)
        {
          gchar *test = g_strconcat ("--", dirs[i].option, NULL);

          if (strcmp (argv[argi], test) == 0)
            {
              g_free (test);
              break;
            }
          else
            {
              g_free (test);
            }
        }

      if (i < G_N_ELEMENTS (dirs))
        {
          gchar *expanded = expand_and_munge (dirs[i].value);

          g_print ("%s\n", expanded);
          g_free (expanded);
        }
      else if (strcmp (argv[argi], "--quiet") == 0 ||
               strcmp (argv[argi], "--silent") == 0)
        {
          silent = TRUE;
        }
      else if (strcmp (argv[argi], "--version") == 0)
        {
          g_print ("%d.%d.%d\n",
                   GIMP_MAJOR_VERSION, GIMP_MINOR_VERSION, GIMP_MICRO_VERSION);
          exit (EXIT_SUCCESS);
        }
      else if (strcmp (argv[argi], "-n") == 0 ||
               strcmp (argv[argi], "--just-print") == 0 ||
               strcmp (argv[argi], "--dry-run") == 0 ||
               strcmp (argv[argi], "--recon") == 0)
        ; /* Already handled */
      else if (strcmp (argv[argi], "--includedir") == 0)
        do_includedir ();
      else if (strcmp (argv[argi], "--cflags") == 0)
        do_cflags ();
      else if (strcmp (argv[argi], "--cflags-noui") == 0)
        do_cflags_noui ();
      else if (strcmp (argv[argi], "--cflags-nogimpui") == 0)
        do_cflags_nogimpui ();
      else if (strcmp (argv[argi], "--libs") == 0)
        do_libs ();
      else if (strcmp (argv[argi], "--libs-noui") == 0)
        do_libs_noui ();
      else if (strcmp (argv[argi], "--libs-nogimpui") == 0)
        do_libs_nogimpui ();
      else if (g_str_has_prefix (argv[argi], "--prefix="))
        ;
      else if (g_str_has_prefix (argv[argi], "--exec-prefix="))
        ;
      else if (strcmp (argv[argi], "--msvc-syntax") == 0)
        ;
      else if (strcmp (argv[argi], "--build") == 0)
        do_build (argv[++argi]);
      else if (strcmp (argv[argi], "--build-noui") == 0)
        do_build_noui (argv[++argi]);
      else if (strcmp (argv[argi], "--build-nogimpui") == 0)
        do_build_nogimpui (argv[++argi]);
      else if (strcmp (argv[argi], "--install") == 0)
        do_install (argv[++argi]);
      else if (strcmp (argv[argi], "--install-noui") == 0)
        do_install_noui (argv[++argi]);
      else if (strcmp (argv[argi], "--install-nogimpui") == 0)
        do_install_nogimpui (argv[++argi]);
      else if (strcmp (argv[argi], "--install-admin") == 0)
        do_install_admin (argv[++argi]);
      else if (strcmp (argv[argi], "--install-admin-noui") == 0)
        do_install_admin_noui (argv[++argi]);
      else if (strcmp (argv[argi], "--install-admin-nogimpui") == 0)
        do_install_admin_nogimpui (argv[++argi]);
      else if (strcmp (argv[argi], "--install-bin") == 0)
        do_install_bin (argv[++argi]);
      else if (strcmp (argv[argi], "--install-admin-bin") == 0)
        do_install_admin_bin (argv[++argi]);
      else if (strcmp (argv[argi], "--uninstall-bin") == 0)
        do_uninstall_bin (argv[++argi]);
      else if (strcmp (argv[argi], "--uninstall-admin-bin") == 0)
        do_uninstall_admin_bin (argv[++argi]);
      else if (strcmp (argv[argi], "--install-script") == 0)
        do_install_script (argv[++argi]);
      else if (strcmp (argv[argi], "--install-admin-script") == 0)
        do_install_admin_script (argv[++argi]);
      else if (strcmp (argv[argi], "--uninstall-script") == 0)
        do_uninstall_script (argv[++argi]);
      else if (strcmp (argv[argi], "--uninstall-admin-script") == 0)
        do_uninstall_admin_script (argv[++argi]);
      else
        {
          g_printerr ("Unrecognized switch %s\n", argv[argi]);
          usage (EXIT_FAILURE);
        }
    }

  exit (EXIT_SUCCESS);
}
/*
 * Local Variables:
 * mode: c
 * End:
 */
