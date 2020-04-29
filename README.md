# normal_position_combination
Efficiently Combining Positions and Normals for Precise 3D Geometry

Copyright :: [source](http://w3.impa.br/~diego/software/NehEtAl05/)

Combining positions and normals following the article below
 Efficiently Combining Positions and Normals for Precise 3D Geometry

```Nehab, D.; Rusinkiewicz, S.; Davis, J.; Ramamoorthi, R.
ACM Transactions on Graphics - SIGGRAPH 2005
Los Angeles, California, July 2005, Volume 24, Issue 3, pp. 536-543
```

## Reference manual

When invoked without any arguments, mesh_opt prints the following help message:
Usage: mesh_opt infile [options] [outfile]

### Options:

   -fc file.fc     Range grid camera intrinsics (i.e. fx fy cx cy)
   -lambda l       Geometry weight
   -blambda b      Boundary geometry weight
   -fixnorm s[:n]  Fix normals by smoothing n times with sigma=s*edgelength
   -smooth s[:n]   Smooth positions n times with sigma=s*edgelength
   -opt            Run one optimization round
   -noopt          Do not optimize
   -noconf         Remove per-vertex confidence
   -nogrid         Unpack range grid to faces

### infile

The input file can be of any type supported by trimesh2. It must contain position and normal measurements. Connectivity can be either explicit, or given by a grid structure. When run on a range grid, and when camera intrinsics are provided, the position optimization stage uses the range grid formulation (higher quality, better stability). Otherwise, the program uses the arbitrary mesh formulation.

-fc file.fc

This option specifies a file from which the camera intrinsics corresponding to a range grid can be obtained. The file should contain four numbers separated by spaces: fx fy cx cy. The numbers are such that the ray through a point projecting to pixel (x, y) is given by [-(x-cx)Z/fx, -(y-cy)Z/fy, Z].

-lambda l

This is the geometry weight, and ranges from 0 to 1. Large values cause the optimization process to favor the original position measurements. Small values give more importance to the normal estimates instead. The default value is 0.1.

-blambda b

Same as -lambda, but for boundary points. If you don't trust your boundary position estimates, you can lower their confidence. Alternatively, you can use mesh_filter and erode the mesh to eliminate them.

-fixnorm s[:n]

Invokes the normal correction stage. This step is not executed implicitly. The parameter s gives the radius of the smoothing kernel, in multiples of the median edge length. The smoothing process can be repeated n times. After smoothing and merging, the corrected normal field contains the low-frequencies from the geometry and the high-frequencies from the measured normals.

-smooth s[:n]

Smooths the measured positions. The parameter s gives the radius of the smoothing kernel, in multiples of the median edge length. The smoothing process can be repeated n times. Smoothing is optional and can be used to eliminate high-frequency noise from the geometry prior to optimization.

-opt

Explicitly invokes the geometry optimization stage. Optimization is run on the current geometry and normal field, which depend on prior optimization, normal correction and smoothing operations. This stage is executed implicitly unless the option -noopt is used.

-noopt
Prevents the program from running the implicit geometry optimization step. This can be used, for example, if you want to save the results of the normal correction stage without optimizing the geometry. Don't forget the "norm:" prefix to the output file name if you want the results to contain normals.

-noconf

Remove per-vertex confidence values. If confidence values are present, the position optimization stage uses them as multiplicative weights. If removed prior to optimization, the confidence values are naturally ignored.

-nogrid

Triangulates the range grid into explicit triangles. This is provided just for convenience.

[outfile]

The output file name. By default, normals are not saved. To get normals, prefix the file name with 'norm:'. For example, 'norm:output.ply' will save results, including normals, into the file 'output.ply'.