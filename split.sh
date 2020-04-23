#python generate_pointcloud.py data/diffuse_albedo.png data/dist0.exr data/syn.tif before_correction/output

SET=$(seq 0 4)

for i in $SET
    do
        for j in $SET
        do
        echo "Running loop seq "$i $j
        ./mesh_opt "before_correction/output$i $j.ply" "norm:after_correction/result$i $j.ply"
    done
done