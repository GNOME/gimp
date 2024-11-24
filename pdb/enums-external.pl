    GeglDistanceMetric =>
	{ contig => 1,
	  header => '',
	  external => 1,
	  symbols => [ qw(GEGL_DISTANCE_METRIC_EUCLIDEAN
			  GEGL_DISTANCE_METRIC_MANHATTAN
			  GEGL_DISTANCE_METRIC_CHEBYSHEV) ],
	  mapping => { GEGL_DISTANCE_METRIC_EUCLIDEAN => '0',
		       GEGL_DISTANCE_METRIC_MANHATTAN => '1',
		       GEGL_DISTANCE_METRIC_CHEBYSHEV => '2' }
	},
