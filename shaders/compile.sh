printf "Starting shader compile task\n"
for filepath in $1/*.{'frag','vert','tesc','tese','geom','comp'}; do
    [ -f $filepath ] || continue
    file=$(basename "$filepath")
    if [ -f $file.md5 ] && ! md5sum --check $file.md5 --status; then
        md5sum $filepath > $file.md5
        printf "[Compiling]: $file \n"
        glslc $filepath -o $file.spv
    fi
    
done
