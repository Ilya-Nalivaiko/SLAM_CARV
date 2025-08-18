ulimit -n 4096
valgrind --leak-check=full --show-leak-kinds=all \
         --track-origins=yes --num-callers=50 --error-limit=no \
         --trace-children=yes \
         --log-file=valgrind_orbcarv.log \
         --tool=memcheck \
         rosrun ORB_CARV_Pub Mono Vocabulary/ORBvoc.txt config_files/Logitech_c270_HD720p.yaml 192.168.1.133 8080 192.168.1.133 5555 camera/image_raw
