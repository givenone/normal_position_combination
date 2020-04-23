from plyfile import PlyData, PlyElement, PlyProperty, PlyListProperty
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
    range_grid = [["0\n"] * width] * height

    for h in range(num_samples) :
        for w in range(num_samples) :
            name = infile + "/result{} {}.ply".format(h,w)
            try:
                with open(name, 'rb') as f:
                    plydata = PlyData.read(f)

                    grid = plydata.elements[1]

                    for i, x in enumerate(grid.data) :
                        (u,v) = getindex(h, w, i, num_samples) 
                        num = x[0]
                        if num.size == 1 :
                            range_grid[u][v] = "%d %d\n"%(1, num[0] + len(points))
                            
                    vertex = plydata.elements[0]

                    for x in vertex :
                        points.append("%f %f %f %f %f %f %d %d %d\n"%tuple(x))

            except IOError:
                print(name + " does not exist")
                
    grid = ""
    for h in range(height) :
            grid += "".join(range_grid[h])    
    file = open(outfile,"w")
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
property uchar diffuse_red
property uchar diffuse_green
property uchar diffuse_blue
element range_grid %d
property list uchar int vertex_indices
end_header
%s
%s
'''%(width, height, len(points), width * height,"".join(points), grid)
    )
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