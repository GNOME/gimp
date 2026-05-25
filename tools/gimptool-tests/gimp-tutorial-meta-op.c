#ifdef GEGL_PROPERTIES

  /* none yet */

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     gimp_tutorial_meta_op
#define GEGL_OP_C_SOURCE gimp-tutorial-meta-op.c

#include "gegl-op.h"

static void
attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglNode *input;
  GeglNode *output;

  input  = gegl_node_get_input_proxy  (gegl, "input");
  output = gegl_node_get_output_proxy (gegl, "output");

  /* construct your GEGL graph here */

  gegl_node_link_many (input, output, NULL);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
                                 "title",       "Hello World Meta",
                                 "name",        "zemarmot:hello-world-meta",
                                 "categories",  "Artistic",
                                 "description", "Hello World as a Meta filter for official GIMP tutorial",
                                 NULL);
}

#endif