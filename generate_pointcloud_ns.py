# the resulting .ply file can be viewed for example with meshlab
# sudo apt-get install meshlab
#
# revised by junwon

"""
This script reads a registered pair of color and depth images and generates a
colored 3D point cloud in the PLY format.
"""
from plyfile import PlyData, PlyElement, PlyProperty, PlyListProperty
import numpy as np
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
    
    for u in range(height) :
        for v in range(width) :
            color = rgb[u][v]
            N = normal[u][v]
            Z = depth[u][v][0]
            if Z==0: continue
            X = (v - centerX) * Z / focalLength * x_pitch
            Y = (centerY - u) * Z / focalLength * y_pitch
            Z = -Z #+ camera
            
            if abs(Z+4.9) > 3 :
                range_grid.append(False) 
                continue
            points.append((X,Y,Z, N[0], N[1], N[2], color[0],color[1],color[2])) # BGR -> RGB
            range_grid.append(True)
            
    header = '''ply
format binary_little_endian 1.0
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
end_header'''%(width, height, len(points),len(range_grid))

    file = open(ply_file,"wb")
    file.write(header.encode('ascii'))
    file.write(b'\n')
    
    for data in points :
        file.write(np.dtype('<f4').type(data[0]))
        file.write(np.dtype('<f4').type(data[1]))
        file.write(np.dtype('<f4').type(data[2]))
        file.write(np.dtype('<f4').type(data[3]))
        file.write(np.dtype('<f4').type(data[4]))
        file.write(np.dtype('<f4').type(data[5]))
        file.write(np.dtype('<u1').type(data[6]))
        file.write(np.dtype('<u1').type(data[7]))
        file.write(np.dtype('<u1').type(data[8]))
    cnt = 0
    for flag in range_grid :
        if flag :
            file.write(np.dtype('<u1').type(1))
            file.write(np.dtype('<u4').type(cnt))
            cnt += 1
        else :
            file.write(np.dtype('<u1').type(0))
    
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
        
    output = args.ply_file
    generate_pointcloud(args.rgb_file,args.depth_file, args.normal_file, args.ply_file)
    # python generate_pointcloud.py data/diffuse_albedo.png data/dist0.exr data/syn.tif data/output.ply
    # ./mesh_opt data/output.ply -fc emily.fc data/result1.ply

     # python generate_pointcloud.py data/diffuse_albedo.png data/dist0.exr data/syn.tif before_correction/output