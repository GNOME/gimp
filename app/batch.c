#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "appenv.h"
#include "app_procs.h"
#include "batch.h"
#include "procedural_db.h"


static void batch_run_cmd  (char              *cmd);
static void batch_read     (gpointer           data,
			    gint               source,
			    GdkInputCondition  condition);


static ProcRecord *eval_proc;


void
batch_init ()
{
  extern char **batch_cmds;

  int read_from_stdin;
  int i;

  eval_proc = procedural_db_lookup ("extension_script_fu_eval");
  if (!eval_proc)
    {
      g_message ("script-fu not available: batch mode disabled\n");
      return;
    }

  read_from_stdin = FALSE;
  for (i = 0; batch_cmds[i]; i++)
    {
      if (strcmp (batch_cmds[i], "-") == 0)
	{
	  if (!read_from_stdin)
	    {
	      g_print ("reading batch commands from stdin\n");
	      gdk_input_add (STDIN_FILENO, GDK_INPUT_READ, batch_read, NULL);
	      read_from_stdin = TRUE;
	    }
	}
      else
	{
	  batch_run_cmd (batch_cmds[i]);
	}
    }
}


static void
batch_run_cmd (char *cmd)
{
  Argument *args;
  Argument *vals;
  int i;

  if (g_strcasecmp (cmd, "(gimp-quit 0)") == 0)
    {
      app_exit (0);
      exit (0);
    }

  args = g_new0 (Argument, eval_proc->num_args);
  for (i = 0; i < eval_proc->num_args; i++)
    args[i].arg_type = eval_proc->args[i].arg_type;

  args[0].value.pdb_int = 1;
  args[1].value.pdb_pointer = cmd;

  vals = procedural_db_execute ("extension_script_fu_eval", args);
  switch (vals[0].value.pdb_int)
    {
    case PDB_EXECUTION_ERROR:
      g_print ("batch command: experienced an execution error.\n");
      break;
    case PDB_CALLING_ERROR:
      g_print ("batch command: experienced a calling error.\n");
      break;
    case PDB_SUCCESS:
      g_print ("batch command: executed successfully.\n");
      break;
    default:
      break;
    }
  
  procedural_db_destroy_args (vals, eval_proc->num_values);
  g_free(args);

  return;
}


static void
batch_read (gpointer          data,
	    gint              source,
	    GdkInputCondition condition)
{
  static GString *string;
  char buf[32], *t;
  int nread, done;

  if (condition & GDK_INPUT_READ)
    {
      do {
	nread = read (source, &buf, sizeof (char) * 31);
      } while ((nread == -1) && ((errno == EAGAIN) || (errno == EINTR)));

      if ((nread == 0) && (!string || (string->len == 0)))
	app_exit (FALSE);

      buf[nread] = '\0';

      if (!string)
	string = g_string_new ("");

      t = buf;
      if (string->len == 0)
	{
	  while (*t)
	    {
	      if (isspace (*t))
		t++;
	      else
		break;
	    }
	}

      g_string_append (string, t);

      done = FALSE;

      while (*t)
	{
	  if ((*t == '\n') || (*t == '\r'))
	    {
              t = '\0';
	      done = TRUE;
	    }
	  t++;
	}

      if (done)
	{
	  batch_run_cmd (string->str);
	  g_string_truncate (string, 0);
	}
    }
}
