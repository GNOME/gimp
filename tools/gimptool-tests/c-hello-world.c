/* Always include this in all plug-ins */
#include <libgimp/gimp.h>
/* The name of my PDB procedure */
#define PLUG_IN_PROC "plug-in-zemarmot-c-demo-hello-world"
/* Our custom class HelloWorld is derived from GimpPlugIn. */
struct _HelloWorld
{
  GimpPlugIn parent_instance;
};
#define HELLO_WORLD_TYPE (hello_world_get_type())
G_DECLARE_FINAL_TYPE (HelloWorld, hello_world, HELLO_WORLD,, GimpPlugIn)

/* Declarations */
static GList          * hello_world_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * hello_world_create_procedure (GimpPlugIn           *plug_in,
                                                      const gchar          *name);
static GimpValueArray * hello_world_run              (GimpProcedure        *procedure,
                                                      GimpRunMode           run_mode,
                                                      GimpImage            *image,
                                                      GimpDrawable        **drawables,
                                                      GimpProcedureConfig  *config,
                                                      gpointer              run_data);

G_DEFINE_TYPE (HelloWorld, hello_world, GIMP_TYPE_PLUG_IN)
static void
hello_world_class_init (HelloWorldClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);
  plug_in_class->query_procedures = hello_world_query_procedures;
  plug_in_class->create_procedure = hello_world_create_procedure;
}
static void
hello_world_init (HelloWorld *hello_world)
{
}
static GList *
hello_world_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}
static GimpProcedure *
hello_world_create_procedure (GimpPlugIn  *plug_in,
                              const gchar *name)
{
  GimpProcedure *procedure = NULL;
  if (g_strcmp0 (name, PLUG_IN_PROC) == 0)
    procedure = gimp_image_procedure_new (plug_in, name,
                                          GIMP_PDB_PROC_TYPE_PLUGIN,
                                          hello_world_run, NULL, NULL);
  return procedure;
}
static GimpValueArray *
hello_world_run (GimpProcedure        *procedure,
                 GimpRunMode           run_mode,
                 GimpImage            *image,
                 GimpDrawable        **drawables,
                 GimpProcedureConfig  *config,
                 gpointer              run_data)
{
  gimp_message ("Hello World!");
  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}
/* Generate needed main() code */
GIMP_MAIN (HELLO_WORLD_TYPE)
