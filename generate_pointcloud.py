# Software License Agreement (BSD License)
#
# Copyright (c) 2013, Juergen Sturm, TUM
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#  * Neither the name of TUM nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# the resulting .ply file can be viewed for example with meshlab
# sudo apt-get install meshlab
#
# revised by junwon

"""
This script reads a registered pair of color and depth images and generates a
colored 3D point cloud in the PLY format.
"""

import argparse
import sys
import os
import cv2 as cv
from PIL import Image
from numpy import array
focalLength = 0.05
width = 2160
height = 3840
camera = -4.9 # z axis
sensor_width = 0.025
sensor_height = 0.024
centerX = width / 2
centerY = height / 2
x_pitch = sensor_width / width
y_pitch = sensor_height / height

def generate_pointcloud(rgb_file, depth_file, normal_file, ply_file):
    """
    Generate a colored point cloud in PLY format from a color and a depth image.
    
    Input:
    rgb_file -- filename of color image
    depth_file -- filename of depth image
    normal_file -- filename of normal map
    ply_file -- filename of ply file
    
    """
    rgb = cv.imread(rgb_file, -1)
    rgb = cv.cvtColor(rgb, cv.COLOR_BGR2RGB)
    depth = cv.imread(depth_file, -1)
    depth = cv.cvtColor(depth, cv.COLOR_BGR2RGB)
    normal = cv.imread(normal_file, -1)
    normal = cv.cvtColor(normal, cv.COLOR_BGR2RGB)

    #normal = Image.open(normal_file)
    
    if rgb.size != depth.size:
        raise Exception("Color and depth image do not have the same resolution.")
    if depth.size != normal.size :
        raise Exception("Depth image and normal map do not have the same resolution.")
    points = []
    range_grid = []    
    for v in range(width):
        for u in range(height):
            color = rgb[u][v]
            N = normal[u][v]
            Z = depth[u][v][0]
            if Z==0: continue
            X = (u - centerX) * Z / focalLength * x_pitch
            Y = (centerY - v) * Z / focalLength * y_pitch
            Z = Z #+ camera
            
            if abs(Z-4.9) > 3 :
                range_grid.append(False) 
                continue
            points.append("%f %f %f %f %f %f %d %d %d\n"%(X,Y,Z, N[0], N[1], N[2], color[0],color[1],color[2])) # BGR -> RGB
            range_grid.append(True)
    file = open(ply_file,"w")
    file.write('''ply
format ascii 1.0
obj_info is_mesh 0
obj_info num_cols %d
obj_info num_rows %d
element vertex %d
property float x
property float y
property float z
property float nx
property float ny
property float nz
property uchar red
property uchar green
property uchar blue
element range_grid %d
property list uchar int vertex_indices
end_header
%s
'''%(width, height, len(points),len(range_grid),"".join(points)))
    for i, flag in enumerate(range_grid) :
        if flag :
            file.write("%d %d\n"%(1, i))
        else :
            file.write("%d\n"%0)
    file.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='''
    This script reads a registered pair of color and depth images and generates a colored 3D point cloud in the
    PLY format. 
    ''')
    parser.add_argument('rgb_file', help='input color image (format: tiff)')
    parser.add_argument('depth_file', help='input depth image (format: tiff)')
    parser.add_argument('normal_file', help='input depth image (format: tiff)')
    parser.add_argument('ply_file', help='output PLY file (format: ply)')
    args = parser.parse_args()

    generate_pointcloud(args.rgb_file,args.depth_file, args.normal_file, args.ply_file)
    # python generate_pointcloud.py data/diffuse_albedo.png data/dist0.exr data/syn.tif data/output.ply
    # ./mesh_opt data/output.ply -fc emily.fc data/result1.ply