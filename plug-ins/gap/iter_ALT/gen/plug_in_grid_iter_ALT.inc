/* ----------------------------------------------------------------------
 * p_plug_in_grid_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_grid_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_grid_Vals 
    {
      long      hwidth;
      long      hspace;
      long      hoffset;
      t_color   hcolor;
      char      hopacity;
      long      vwidth;
      long      vspace;
      long      voffset;
      t_color   vcolor;
      char      vopacity;
      long      iwidth;
      long      ispace;
      long      ioffset;
      t_color   icolor;
      char      iopacity;
    } t_plug_in_grid_Vals; 

    t_plug_in_grid_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_grid_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_grid_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_grid_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_grid_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_grid_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_grid_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_grid_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_long(&buf.hwidth, buf_from->hwidth, buf_to->hwidth, total_steps, current_step);
    p_delta_long(&buf.hspace, buf_from->hspace, buf_to->hspace, total_steps, current_step);
    p_delta_long(&buf.hoffset, buf_from->hoffset, buf_to->hoffset, total_steps, current_step);
    p_delta_color(&buf.hcolor, &buf_from->hcolor, &buf_to->hcolor, total_steps, current_step);
    p_delta_char(&buf.hopacity, buf_from->hopacity, buf_to->hopacity, total_steps, current_step);
    p_delta_long(&buf.vwidth, buf_from->vwidth, buf_to->vwidth, total_steps, current_step);
    p_delta_long(&buf.vspace, buf_from->vspace, buf_to->vspace, total_steps, current_step);
    p_delta_long(&buf.voffset, buf_from->voffset, buf_to->voffset, total_steps, current_step);
    p_delta_color(&buf.vcolor, &buf_from->vcolor, &buf_to->vcolor, total_steps, current_step);
    p_delta_char(&buf.vopacity, buf_from->vopacity, buf_to->vopacity, total_steps, current_step);
    p_delta_long(&buf.iwidth, buf_from->iwidth, buf_to->iwidth, total_steps, current_step);
    p_delta_long(&buf.ispace, buf_from->ispace, buf_to->ispace, total_steps, current_step);
    p_delta_long(&buf.ioffset, buf_from->ioffset, buf_to->ioffset, total_steps, current_step);
    p_delta_color(&buf.icolor, &buf_from->icolor, &buf_to->icolor, total_steps, current_step);
    p_delta_char(&buf.iopacity, buf_from->iopacity, buf_to->iopacity, total_steps, current_step);

    gimp_set_data("plug_in_grid", &buf, sizeof(buf)); 

    return 0; /* OK */
}
