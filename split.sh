#python generate_pointcloud.py data/diffuse_albedo.png data/dist0.exr data/syn.tif before_correction_overlap/output 10

SET=$(seq 0 4)

for i in $SET
    do
        for j in $SET
        do
        echo "Running loop seq "$i $j
        #./mesh_opt "before_correction_overlap/output$i $j.ply" -noopt -lambda 0.01 -blambda 1 "norm:no_correction/result$i $j.ply"
        # lambda : default 0.1, weight for corrected normal.
    done
done

#python mesh_merge.py after_correction/result final_binary_optimized.ply

python mesh_merge.py no_correction/result test.ply 10