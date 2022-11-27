printf "Starting shader compile task\n"
for filepath in $1/*.{'frag','vert','tesc','tese','geom','comp'}; do
    [ -f $filepath ] || continue
    file=$(basename "$filepath")
    [ -f $file.md5 ] && md5sum --check $file.md5 --status && continue #Check if exists and already hashed
    printf "[Compiling]: $file \n"
    if glslc $filepath -o $file.spv; then
        md5sum $filepath > $file.md5
    else
        read -p prompt
    fi
done