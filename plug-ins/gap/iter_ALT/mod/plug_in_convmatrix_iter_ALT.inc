/* ----------------------------------------------------------------------
 * p_plug_in_convmatrix_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_convmatrix_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
     typedef enum {
	     EXTEND,
	     WRAP,
	     CLEAR,
	     MIRROR
     }t_convmatrix_BorderMode;

    typedef struct t_plug_in_convmatrix_Vals 
    {
	gfloat matrix[5][5];
	gfloat divisor;
	gfloat offset;
	gint alpha_alg;
	t_convmatrix_BorderMode bmode;
	gint channels[5];
	gint autoset;
    } t_plug_in_convmatrix_Vals; 

    t_plug_in_convmatrix_Vals  buf, *buf_from, *buf_to; 
     int l_x, l_y;

    if(len_struct != sizeof(t_plug_in_convmatrix_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_convmatrix_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_convmatrix_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_convmatrix_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_convmatrix_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_convmatrix_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_convmatrix_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));


    for(l_x=0; l_x < 5; l_x++)
    {
      for(l_y=0; l_y < 5; l_y++)
      {
         p_delta_gfloat(&buf.matrix[l_x][l_y], buf_from->matrix[l_x][l_y], buf_to->matrix[l_x][l_y], total_steps, current_step);
        
      }
    }
    p_delta_gfloat(&buf.divisor, buf_from->divisor, buf_to->divisor, total_steps, current_step);
    p_delta_gfloat(&buf.offset, buf_from->offset, buf_to->offset, total_steps, current_step);



    gimp_set_data("plug_in_convmatrix", &buf, sizeof(buf)); 

    return 0; /* OK */
}
