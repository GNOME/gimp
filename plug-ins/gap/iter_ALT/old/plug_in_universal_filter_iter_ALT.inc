/* ----------------------------------------------------------------------
 * p_plug_in_universal_filter_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_universal_filter_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_universal_filter_Vals 
    {
      long      matrix_0_0;
      long      matrix_0_1;
      long      matrix_0_2;
      long      matrix_1_0;
      long      matrix_1_1;
      long      matrix_1_2;
      long      matrix_2_0;
      long      matrix_2_1;
      long      matrix_2_2;
      long      xauto;
      long      norm;
      long      bias;
    } t_plug_in_universal_filter_Vals; 

    t_plug_in_universal_filter_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_universal_filter_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_universal_filter_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_universal_filter_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_universal_filter_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_universal_filter_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_universal_filter_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_universal_filter_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_long(&buf.matrix_0_0, buf_from->matrix_0_0, buf_to->matrix_0_0, total_steps, current_step);
    p_delta_long(&buf.matrix_0_1, buf_from->matrix_0_1, buf_to->matrix_0_1, total_steps, current_step);
    p_delta_long(&buf.matrix_0_2, buf_from->matrix_0_2, buf_to->matrix_0_2, total_steps, current_step);
    p_delta_long(&buf.matrix_1_0, buf_from->matrix_1_0, buf_to->matrix_1_0, total_steps, current_step);
    p_delta_long(&buf.matrix_1_1, buf_from->matrix_1_1, buf_to->matrix_1_1, total_steps, current_step);
    p_delta_long(&buf.matrix_1_2, buf_from->matrix_1_2, buf_to->matrix_1_2, total_steps, current_step);
    p_delta_long(&buf.matrix_2_0, buf_from->matrix_2_0, buf_to->matrix_2_0, total_steps, current_step);
    p_delta_long(&buf.matrix_2_1, buf_from->matrix_2_1, buf_to->matrix_2_1, total_steps, current_step);
    p_delta_long(&buf.matrix_2_2, buf_from->matrix_2_2, buf_to->matrix_2_2, total_steps, current_step);
    p_delta_long(&buf.xauto, buf_from->xauto, buf_to->xauto, total_steps, current_step);
    p_delta_long(&buf.norm, buf_from->norm, buf_to->norm, total_steps, current_step);
    p_delta_long(&buf.bias, buf_from->bias, buf_to->bias, total_steps, current_step);

    gimp_set_data("plug_in_universal_filter", &buf, sizeof(buf)); 

    return 0; /* OK */
}
