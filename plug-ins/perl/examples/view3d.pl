#!/usr/bin/perl -w

BEGIN { $^W=1 }
use strict;

use Gimp;
use Gimp::Fu;
use Gimp::PDL;

use PDL;
use PDL::Core;
use PDL::Math;
use PDL::Graphics::TriD;

register (
    'view3d',

    'View grayscale drawable in 3D',

    'This script uses PDL::Graphics:TriD to view a grayscale drawable in 3D. You can choose a Cartesian (default) or Polar projection, toggle the drawing of lines, and toggle normal smoothing.',

    'Tom Rathborne', 'GPLv2', 'v0.0:1998-11-28',

    '<Image>/View/3D Surface',

    'GRAY', [
        [ PF_BOOL, 'Polar', 'Radial view', 0],
        [ PF_BOOL, 'Lines', 'Draw grid lines', 0],
        [ PF_BOOL, 'Smooth', 'Smooth surface normals', 1]
    ], [ ],

sub {
    my ($img, $dwb, $polar, $lines, $smooth) = @_;
   
    my $w = $dwb->width;
    my $h = $dwb->height;
  
    my $gdwb = $dwb->get;
    my $regn = $gdwb->pixel_rgn (0, 0, $w, $h, 0, 0);
    my $rect = $regn->get_rect (0, 0, $w, $h);
    my $surf = $rect->slice('(0)');

    imag3d [ $polar ? 'POLAR2D' : 'SURF2D', $surf ],
           { 'Lines' => $lines, 'Smooth' => $smooth };

    0;
}

);

exit main;

