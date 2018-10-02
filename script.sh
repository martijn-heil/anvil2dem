for f in /mnt/c/Users/Martijn/Desktop/lg_server/world/region/*.mca; do
    cat "$f" | ./testproject > ./dump6.gml
done
