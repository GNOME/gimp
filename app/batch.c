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


static int  batch_is_cmd   (char              *cmd);
static void batch_run_cmd  (char              *cmd);
static void batch_run_cmds (FILE              *fp);
static void batch_read     (gpointer           data,
			    gint               source,
			    GdkInputCondition  condition);


void
batch_init ()
{
  extern char **batch_cmds;

  FILE *fp;
  int read_from_stdin;
  int i;

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
      else if (batch_is_cmd (batch_cmds[i]))
	{
	  batch_run_cmd (batch_cmds[i]);
	}
      else
	{
	  fp = fopen (batch_cmds[i], "r");
	  if (!fp)
	    g_print ("unable to open batch file: \"%s\"\n", batch_cmds[i]);

	  batch_run_cmds (fp);
	  fclose (fp);
	}
    }
}


static int
batch_is_cmd (char *cmd)
{
  int paren_level;

  if (!cmd)
    return FALSE;

  cmd = strchr (cmd, '(');
  if (!cmd)
    return FALSE;
  cmd += 1;

  paren_level = 1;

  while (*cmd)
    {
      if (*cmd == ')')
	paren_level -= 1;
      else if (*cmd == '(')
	paren_level += 1;
      cmd++;
    }

  return (paren_level == 0);
}

char *
get_tok(char **rest)
{
  char *tok_start, *tok;
  int i,j,len,escapes;

  /* Skip delimiters */
  while((**rest != 0) && 
	(**rest == ' ' || **rest == '\t'))
    (*rest)++;

  /* token starts here */
  tok_start = *rest;

  if(**rest == '"'){
    /* Handle string */

    /* Skip quote */
    (*rest)++;
    tok_start++;
    len = 0;
    escapes = 0;

    /* Scan to end while skipping escaped quotes */
    while((**rest != 0) && 
	  (**rest != '"')){
      if(**rest == '\\'){
	(*rest)++;
	escapes++;
      }
      (*rest)++;
      len++;
    }
    if(**rest == '"'){
      (*rest)++;
      tok = g_malloc(len+1);
    }
    else{
      g_print("String not properly terminated.");
      return NULL;
    }

    /* Copy the string while converting the escaped characters. */
    /* Only double quote and backspace is accepted other escapes are ignored. */
    j = 0;
    for (i=0;i < len + escapes;i++){
      if(tok_start[i] != '\\')
	tok[j++] = tok_start[i];
      else{
	i++;
	if(tok_start[i] == '"' || tok_start[i] == '\\')
	  tok[j++] = tok_start[i];
      }
    }
    tok[j] = 0;
  }
  else{
    /* Handle number or identifier */
    while((**rest != 0) && 
	  ((**rest >= 'a' && **rest <= 'z') || 
	   (**rest >= 'A' && **rest <= 'Z') || 
	   (**rest >= '0' && **rest <= '9') ||
	   (**rest == '-') ||
	   (**rest == '_')))
      (*rest)++;
    if (*rest != tok_start){
      len = *rest - tok_start;
      tok = g_malloc(len+1);
      strncpy(tok,tok_start,len);
      tok[len]=0;
    }
    else{
      if(**rest == 0){
	g_print("Unexpected end of command argument.");
        return NULL;
      }
      /* One character token - normally "(" or ")" */
      tok = g_malloc(2);
      tok[0] = *rest[0];
      tok[1] = 0;
      (*rest)++;
    }
  }
  return tok;
}

static void
batch_run_cmd (char *cmd)
{
  ProcRecord *proc;
  Argument *args;
  Argument *vals;
  char *rest;
  char *cmdname;
  char *t;
  int i;

  rest = cmd;
  t = get_tok(&rest);
  if (!t || t[0] != '(')
    return;
  g_free(t);

  cmdname = get_tok (&rest);
  if (!cmdname)
    return;

  proc = procedural_db_lookup (cmdname);
  if (!proc)
    {
      g_print ("could not find procedure: \"%s\"\n", cmdname);
      return;
    }

  /* (gimp-procedural-db-dump "/tmp/pdb_dump") */
  args = g_new (Argument, proc->num_args);

  for (i = 0; i < proc->num_args; i++)
    {
      args[i].arg_type = proc->args[i].arg_type;

      switch (proc->args[i].arg_type)
	{
	case PDB_INT32:
	case PDB_INT16:
	case PDB_INT8:
	  t = get_tok (&rest);
	  if (!t)
	    goto error;

	  args[i].value.pdb_int = atoi (t);
	  g_free(t);
	  break;
	case PDB_FLOAT:
	  t = get_tok (&rest);
	  if (!t)
	    goto error;

	  args[i].value.pdb_float = atof (t);
	  g_free(t);
	  break;
	case PDB_STRING:
	  t = get_tok (&rest);
	  if (!t)
	    goto error;

	  args[i].value.pdb_pointer = g_strdup (t);
	  g_free(t);
	  break;
	case PDB_INT32ARRAY:
	case PDB_INT16ARRAY:
	case PDB_INT8ARRAY:
	case PDB_FLOATARRAY:
	case PDB_STRINGARRAY:
	  g_print ("procedures taking array arguments are currently not supported as batch operations\n");
	  goto error;
	case PDB_COLOR:
	  g_print ("procedures taking color arguments are currently not supported as batch operations\n");
	  goto error;
	  break;
	case PDB_REGION:
	  g_print ("procedures taking region arguments are currently not supported as batch operations\n");
	  goto error;
	  break;
	case PDB_DISPLAY:
	  g_print ("procedures taking display arguments are currently not supported as batch operations\n");
	  goto error;
	  break;
	case PDB_IMAGE:
	  g_print ("procedures taking image arguments are currently not supported as batch operations\n");
	  goto error;
	  break;
	case PDB_LAYER:
	  g_print ("procedures taking layer arguments are currently not supported as batch operations\n");
	  goto error;
	  break;
	case PDB_CHANNEL:
	  g_print ("procedures taking channel arguments are currently not supported as batch operations\n");
	  goto error;
	  break;
	case PDB_DRAWABLE:
	  g_print ("procedures taking drawable arguments are currently not supported as batch operations\n");
	  goto error;
	  break;
	case PDB_SELECTION:
	  g_print ("procedures taking selection arguments are currently not supported as batch operations\n");
	  goto error;
	  break;
	case PDB_BOUNDARY:
	  g_print ("procedures taking boundary arguments are currently not supported as batch operations\n");
	  goto error;
	  break;
	case PDB_PATH:
	  g_print ("procedures taking path arguments are currently not supported as batch operations\n");
	  goto error;
	  break;
	case PDB_STATUS:
	  g_print ("procedures taking status arguments are currently not supported as batch operations\n");
	  goto error;
	  break;
	case PDB_END:
	  break;
	}
    }

  vals = procedural_db_execute (proc->name, args);
  switch (vals[0].value.pdb_int)
    {
    case PDB_EXECUTION_ERROR:
      g_print ("batch command: %s experienced an execution error.\n", cmdname);
      break;
    case PDB_CALLING_ERROR:
      g_print ("batch command: %s experienced a calling error.\n", cmdname);
      break;
    case PDB_SUCCESS:
      g_print ("batch command: %s executed successfully.\n", cmdname);
      break;
    }

  g_free(cmdname);
  return;

error:
  g_print ("Unable to run batch command: %s because of bad arguments.\n", cmdname);
  g_free(cmdname);
}

static void
batch_run_cmds (FILE *fp)
{
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
	    done = TRUE;
	  t++;
	}

      if (done && batch_is_cmd (string->str))
	{
	  batch_run_cmd (string->str);
	  g_string_truncate (string, 0);
	}
    }
}
