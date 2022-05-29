/* TinyScheme Extensions
 * (c) 2002 Visual Tools, S.A.
 * Manuel Heras-Gilsanz (manuel@heras-gilsanz.com)
 *
 * This software is subject to the terms stated in the
 * LICENSE file.
 */

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <time.h>

#include <glib.h>

#include "tinyscheme/scheme-private.h"

#undef cons

typedef enum
{
  FILE_TYPE_UNKNOWN = 0, FILE_TYPE_FILE, FILE_TYPE_DIR, FILE_TYPE_LINK
} FileType;

struct
named_constant {
    const char *name;
    FileType    value;
};

struct named_constant
file_type_constants[] = {
    { "FILE-TYPE-UNKNOWN", FILE_TYPE_UNKNOWN },
    { "FILE-TYPE-FILE",    FILE_TYPE_FILE },
    { "FILE-TYPE-DIR",     FILE_TYPE_DIR },
    { "FILE-TYPE-LINK",    FILE_TYPE_LINK },
    { NULL, 0 }
};

pointer foreign_fileexists(scheme *sc, pointer args);
pointer foreign_filetype(scheme *sc, pointer args);
pointer foreign_filesize(scheme *sc, pointer args);
pointer foreign_filedelete(scheme *sc, pointer args);
pointer foreign_diropenstream(scheme *sc, pointer args);
pointer foreign_dirreadentry(scheme *sc, pointer args);
pointer foreign_dirrewind(scheme *sc, pointer args);
pointer foreign_dirclosestream(scheme *sc, pointer args);
pointer foreign_mkdir(scheme *sc, pointer args);

pointer foreign_getenv(scheme *sc, pointer args);
pointer foreign_time(scheme *sc, pointer args);
pointer foreign_gettimeofday(scheme *sc, pointer args);
pointer foreign_usleep(scheme *sc, pointer args);
void    init_ftx (scheme *sc);


pointer foreign_fileexists(scheme *sc, pointer args)
{
  pointer first_arg;
  char   *filename;

  if (args == sc->NIL)
    return sc->F;

  first_arg = sc->vptr->pair_car(args);
  if (!sc->vptr->is_string(first_arg))
    return sc->F;

  filename = sc->vptr->string_value(first_arg);
  filename = g_filename_from_utf8 (filename, -1, NULL, NULL, NULL);
  if (g_file_test(filename, G_FILE_TEST_EXISTS))
    return sc->T;

  return sc->F;
}

pointer foreign_filetype(scheme *sc, pointer args)
{
  pointer first_arg;
  char   *filename;
  int     retcode;

  if (args == sc->NIL)
    return sc->F;

  first_arg = sc->vptr->pair_car(args);
  if (!sc->vptr->is_string(first_arg))
    return sc->F;

  filename = sc->vptr->string_value(first_arg);
  filename = g_filename_from_utf8 (filename, -1, NULL, NULL, NULL);

  if (g_file_test(filename, G_FILE_TEST_IS_SYMLINK))
    retcode =  FILE_TYPE_LINK;
  else if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
    retcode = FILE_TYPE_FILE;
  else if (g_file_test(filename, G_FILE_TEST_IS_DIR))
    retcode = FILE_TYPE_DIR;
  else
    retcode = FILE_TYPE_UNKNOWN;

  return sc->vptr->mk_integer(sc, retcode);
}

pointer foreign_filesize(scheme *sc, pointer args)
{
  pointer first_arg;
  pointer ret;
  struct stat buf;
  char * filename;
  int retcode;

  if (args == sc->NIL)
    return sc->F;

  first_arg = sc->vptr->pair_car(args);
  if (!sc->vptr->is_string(first_arg))
    return sc->F;

  filename = sc->vptr->string_value(first_arg);
  filename = g_filename_from_utf8 (filename, -1, NULL, NULL, NULL);
  retcode = stat(filename, &buf);
  if (retcode == 0)
    ret = sc->vptr->mk_integer(sc,buf.st_size);
  else
    ret = sc->F;
  return ret;
}

pointer foreign_filedelete(scheme *sc, pointer args)
{
  pointer first_arg;
  pointer ret;
  char * filename;
  int retcode;

  if (args == sc->NIL)
    return sc->F;

  first_arg = sc->vptr->pair_car(args);
  if (!sc->vptr->is_string(first_arg)) {
    return sc->F;
  }

  filename = sc->vptr->string_value(first_arg);
  filename = g_filename_from_utf8 (filename, -1, NULL, NULL, NULL);
  retcode = unlink(filename);
  if (retcode == 0)
    ret = sc->T;
  else
    ret = sc->F;
  return ret;
}

pointer foreign_diropenstream(scheme *sc, pointer args)
{
  pointer first_arg;
  char   *dirpath;
  GDir   *dir;

  if (args == sc->NIL)
    return sc->F;

  first_arg = sc->vptr->pair_car(args);
  if (!sc->vptr->is_string(first_arg))
    return sc->F;

  dirpath = sc->vptr->string_value(first_arg);
  dirpath = g_filename_from_utf8 (dirpath, -1, NULL, NULL, NULL);

  dir = g_dir_open(dirpath, 0, NULL);
  if (dir == NULL)
    return sc->F;

  /* Stuffing a pointer in a long may not always be portable ~~~~~ */
  return (sc->vptr->mk_integer(sc, (long) dir));
}

pointer foreign_dirreadentry(scheme *sc, pointer args)
{
  pointer first_arg;
  GDir   *dir;
  gchar  *entry;

  if (args == sc->NIL)
    return sc->F;

  first_arg = sc->vptr->pair_car(args);
  if (!sc->vptr->is_integer(first_arg))
    return sc->F;

  dir = (GDir *) sc->vptr->ivalue(first_arg);
  if (dir == NULL)
    return sc->F;

  entry = (gchar *)g_dir_read_name(dir);
  if (entry == NULL)
    return sc->EOF_OBJ;

  entry = g_filename_to_utf8 (entry, -1, NULL, NULL, NULL);
  return (sc->vptr->mk_string(sc, entry));
}

pointer foreign_dirrewind(scheme *sc, pointer args)
{
  pointer first_arg;
  GDir   *dir;

  if (args == sc->NIL)
    return sc->F;

  first_arg = sc->vptr->pair_car(args);
  if (!sc->vptr->is_integer(first_arg))
    return sc->F;

  dir = (GDir *) sc->vptr->ivalue(first_arg);
  if (dir == NULL)
    return sc->F;

  g_dir_rewind(dir);
  return sc->T;
}

pointer foreign_dirclosestream(scheme *sc, pointer args)
{
  pointer first_arg;
  GDir   *dir;

  if (args == sc->NIL)
    return sc->F;

  first_arg = sc->vptr->pair_car(args);
  if (!sc->vptr->is_integer(first_arg))
    return sc->F;

  dir = (GDir *) sc->vptr->ivalue(first_arg);
  if (dir == NULL)
    return sc->F;

  g_dir_close(dir);
  return sc->T;
}

pointer foreign_mkdir(scheme *sc, pointer args)
{
  pointer     first_arg;
  pointer     rest;
  pointer     second_arg;
  char       *dirname;
  mode_t      mode;
  int         retcode;

  if (args == sc->NIL)
    return sc->F;

  first_arg = sc->vptr->pair_car(args);
  if (!sc->vptr->is_string(first_arg))
    return sc->F;
  dirname = sc->vptr->string_value(first_arg);
  dirname = g_filename_from_utf8 (dirname, -1, NULL, NULL, NULL);

  rest = sc->vptr->pair_cdr(args);
  if (sc->vptr->is_pair(rest)) /* optional mode argument */
    {
      second_arg = sc->vptr->pair_car(rest);
      if (!sc->vptr->is_integer(second_arg))
        return sc->F;
      mode = sc->vptr->ivalue(second_arg);
    }
  else
    mode = 0777;

  retcode = g_mkdir(dirname, (mode_t)mode);
  if (retcode == 0)
    return sc->T;
  else
    return sc->F;
}

pointer foreign_getenv(scheme *sc, pointer args)
{
  pointer     first_arg;
  pointer     ret;
  char       *varname;
  const char *value;

  if (args == sc->NIL)
    return sc->F;

  first_arg = sc->vptr->pair_car(args);

  if (!sc->vptr->is_string(first_arg))
    return sc->F;

  varname = sc->vptr->string_value(first_arg);
  value = g_getenv(varname);
  if (value == NULL)
    ret = sc->F;
  else
    ret = sc->vptr->mk_string(sc,value);

  return ret;
}

pointer foreign_time(scheme *sc, pointer args)
{
  time_t now;
  struct tm *now_tm;
  pointer ret;

  if (args != sc->NIL)
    return sc->F;

  time(&now);
  now_tm = localtime(&now);

  ret = sc->vptr->cons(sc, sc->vptr->mk_integer(sc,(long) now_tm->tm_year),
         sc->vptr->cons(sc, sc->vptr->mk_integer(sc,(long) now_tm->tm_mon),
          sc->vptr->cons(sc, sc->vptr->mk_integer(sc,(long) now_tm->tm_mday),
           sc->vptr->cons(sc, sc->vptr->mk_integer(sc,(long) now_tm->tm_hour),
            sc->vptr->cons(sc, sc->vptr->mk_integer(sc,(long) now_tm->tm_min),
             sc->vptr->cons(sc, sc->vptr->mk_integer(sc,(long) now_tm->tm_sec),sc->NIL))))));

  return ret;
}

pointer foreign_gettimeofday(scheme *sc, pointer args)
{
  pointer ret;
  gint64  time;

  time = g_get_real_time ();

  ret = sc->vptr->cons(sc, sc->vptr->mk_integer(sc,(long) time / G_USEC_PER_SEC),
         sc->vptr->cons(sc, sc->vptr->mk_integer(sc,(long) time % G_USEC_PER_SEC),
          sc->NIL));

  return ret;
}

pointer foreign_usleep(scheme *sc, pointer args)
{
  pointer first_arg;
  long usec;

  if (args == sc->NIL)
    return sc->F;

  first_arg = sc->vptr->pair_car(args);
  if (!sc->vptr->is_integer(first_arg))
    return sc->F;

  usec = sc->vptr->ivalue(first_arg);
  g_usleep(usec);

  return sc->T;
}

/* This function gets called when TinyScheme is loading the extension */
void init_ftx (scheme *sc)
{
  int i;

  sc->vptr->scheme_define(sc,sc->global_env,
                               sc->vptr->mk_symbol(sc,"getenv"),
                               sc->vptr->mk_foreign_func(sc, foreign_getenv));
  sc->vptr->scheme_define(sc, sc->global_env,
                               sc->vptr->mk_symbol(sc,"time"),
                               sc->vptr->mk_foreign_func(sc, foreign_time));
  sc->vptr->scheme_define(sc, sc->global_env,
                               sc->vptr->mk_symbol(sc,"gettimeofday"),
                               sc->vptr->mk_foreign_func(sc, foreign_gettimeofday));
  sc->vptr->scheme_define(sc, sc->global_env,
                               sc->vptr->mk_symbol(sc,"usleep"),
                               sc->vptr->mk_foreign_func(sc, foreign_usleep));

  sc->vptr->scheme_define(sc, sc->global_env,
                               sc->vptr->mk_symbol(sc,"file-exists?"),
                               sc->vptr->mk_foreign_func(sc, foreign_fileexists));
  sc->vptr->scheme_define(sc, sc->global_env,
                               sc->vptr->mk_symbol(sc,"file-type"),
                               sc->vptr->mk_foreign_func(sc, foreign_filetype));
  sc->vptr->scheme_define(sc, sc->global_env,
                               sc->vptr->mk_symbol(sc,"file-size"),
                               sc->vptr->mk_foreign_func(sc, foreign_filesize));
  sc->vptr->scheme_define(sc, sc->global_env,
                               sc->vptr->mk_symbol(sc,"file-delete"),
                               sc->vptr->mk_foreign_func(sc, foreign_filedelete));
  sc->vptr->scheme_define(sc, sc->global_env,
                               sc->vptr->mk_symbol(sc,"dir-open-stream"),
                               sc->vptr->mk_foreign_func(sc, foreign_diropenstream));
  sc->vptr->scheme_define(sc, sc->global_env,
                               sc->vptr->mk_symbol(sc,"dir-read-entry"),
                               sc->vptr->mk_foreign_func(sc, foreign_dirreadentry));
  sc->vptr->scheme_define(sc, sc->global_env,
                               sc->vptr->mk_symbol(sc,"dir-rewind"),
                               sc->vptr->mk_foreign_func(sc, foreign_dirrewind));
  sc->vptr->scheme_define(sc, sc->global_env,
                               sc->vptr->mk_symbol(sc,"dir-close-stream"),
                               sc->vptr->mk_foreign_func(sc, foreign_dirclosestream));
  sc->vptr->scheme_define(sc, sc->global_env,
                               sc->vptr->mk_symbol(sc,"dir-make"),
                               sc->vptr->mk_foreign_func(sc, foreign_mkdir));

  for (i = 0; file_type_constants[i].name != NULL; ++i)
  {
      sc->vptr->scheme_define(sc, sc->global_env,
            sc->vptr->mk_symbol(sc, file_type_constants[i].name),
            sc->vptr->mk_integer(sc, file_type_constants[i].value));
  }
}
