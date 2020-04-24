from plyfile import PlyData, PlyElement, PlyProperty, PlyListProperty
import numpy as np
import argparse

width = 2160
height = 3840

def getindex(h, w, x, num_samples) :
    w_pitch = width // num_samples 

    if w == num_samples -1 :
        w_pitch += width % num_samples
    
    v = x % w_pitch + w * (width // num_samples)
    u = x // w_pitch +  h * (height // num_samples)
    
    return (u,v)

def merging(infile, outfile, num_samples) :

    points = []
    range_grid = [[-1 for x in range(width)] for y in range(height)]
    
    for h in range(num_samples) :
        for w in range(num_samples) :
            name = infile + "{} {}.ply".format(h,w)
            try:
                with open(name, 'rb') as f:
                    plydata = PlyData.read(f)

                    grid = plydata.elements[1]

                    for i, x in enumerate(grid.data) :
                        (u,v) = getindex(h, w, i, num_samples) 
                        num = x[0]
                        if num.size == 1 :
                            range_grid[u][v] = (num[0] + len(points))
                            
                    vertex = plydata.elements[0]

                    for x in vertex :
                        points.append(tuple(x))
                    print(h, w, "done")
            except IOError:
                print(name + " does not exist")


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
end_header'''%(width, height, len(points), width * height)

    file = open(outfile,"wb")
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
    
    for u in range_grid :
        for j in u :
            if j != -1 :
                file.write(np.dtype('<u1').type(1))
                file.write(np.dtype('<u4').type(j))
            else :
                file.write(np.dtype('<u1').type(0))
    
    file.close()    
   


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='''
    This reunifies divided meshes into one .ply file
    '''
    )

    n_sample = 5
    
    parser.add_argument('input_path', help='input directory path/result{i} {j}.ply')
    parser.add_argument('output_path', help='output file path')
    args = parser.parse_args()

    merging(args.input_path, args.output_path, n_sample)