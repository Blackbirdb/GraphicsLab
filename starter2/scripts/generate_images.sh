# This is the script we will test your submission with.

SIZE1=800
SIZE2=800
BIN=../build/a2

# Basic tests
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene01_plane.txt  -output out/basics/a01.png -normals out/basics/a01n.png -depth 8 18 out/basics/a01d.png
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene02_cube.txt   -output out/basics/a02.png -normals out/basics/a02n.png -depth 8 18 out/basics/a02d.png
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene03_sphere.txt -output out/basics/a03.png -normals out/basics/a03n.png -depth 8 18 out/basics/a03d.png
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene04_axes.txt   -output out/basics/a04.png -normals out/basics/a04n.png -depth 8 18 out/basics/a04d.png
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene05_bunny_200.txt -output out/basics/a05.png -normals out/basics/a05n.png -depth 0.8 1.0 out/basics/a05d.png
${BIN} -size ${SIZE1} ${SIZE2} -input ../data/scene06_bunny_1k.txt -bounces 4 -output out/basics/a06.png -normals out/basics/a06n.png -depth 8 18 out/basics/a06d.png
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene07_arch.txt -bounces 4 -shadows -output out/basics/a07.png -normals out/basics/a07n.png -depth 8 18 out/basics/a07d.png

# jitter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene01_plane.txt  -output out/jitter/a01.png -normals out/jitter/a01n.png -depth 8 18 out/jitter/a01d.png -jitter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene02_cube.txt   -output out/jitter/a02.png -normals out/jitter/a02n.png -depth 8 18 out/jitter/a02d.png -jitter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene03_sphere.txt -output out/jitter/a03.png -normals out/jitter/a03n.png -depth 8 18 out/jitter/a03d.png -jitter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene04_axes.txt   -output out/jitter/a04.png -normals out/jitter/a04n.png -depth 8 18 out/jitter/a04d.png -jitter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene05_bunny_200.txt -output out/jitter/a05.png -normals out/jitter/a05n.png -depth 0.8 1.0 out/jitter/a05d.png -jitter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene06_bunny_1k.txt -bounces 4 -output out/jitter/a06.png -normals out/jitter/a06n.png -depth 8 18 out/jitter/a06d.png -jitter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene07_arch.txt -bounces 4 -shadows -output out/jitter/a07.png -normals out/jitter/a07n.png -depth 8 18 out/jitter/a07d.png -jitter

# filter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene01_plane.txt  -output out/filter/a01.png -normals out/filter/a01n.png -depth 8 18 out/filter/a01d.png -filter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene02_cube.txt   -output out/filter/a02.png -normals out/filter/a02n.png -depth 8 18 out/filter/a02d.png -filter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene03_sphere.txt -output out/filter/a03.png -normals out/filter/a03n.png -depth 8 18 out/filter/a03d.png -filter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene04_axes.txt   -output out/filter/a04.png -normals out/filter/a04n.png -depth 8 18 out/filter/a04d.png -filter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene05_bunny_200.txt -output out/filter/a05.png -normals out/filter/a05n.png -depth 0.8 1.0 out/filter/a05d.png -filter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene06_bunny_1k.txt -bounces 4 -output out/filter/a06.png -normals out/filter/a06n.png -depth 8 18 out/filter/a06d.png -filter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene07_arch.txt -bounces 4 -shadows -output out/filter/a07.png -normals out/filter/a07n.png -depth 8 18 out/filter/a07d.png -filter

# jitter + filter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene01_plane.txt  -output out/jf/a01.png -normals out/jf/a01n.png -depth 8 18 out/jf/a01d.png -jitter -filter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene02_cube.txt   -output out/jf/a02.png -normals out/jf/a02n.png -depth 8 18 out/jf/a02d.png -jitter -filter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene03_sphere.txt -output out/jf/a03.png -normals out/jf/a03n.png -depth 8 18 out/jf/a03d.png -jitter -filter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene04_axes.txt   -output out/jf/a04.png -normals out/jf/a04n.png -depth 8 18 out/jf/a04d.png -jitter -filter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene05_bunny_200.txt -output out/jf/a05.png -normals out/jf/a05n.png -depth 0.8 1.0 out/jf/a05d.png -jitter -filter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene06_bunny_1k.txt -bounces 4 -output out/jf/a06.png -normals out/jf/a06n.png -depth 8 18 out/jf/a06d.png -jitter -filter
# ${BIN} -size ${SIZE1} ${SIZE2} -input data/scene07_arch.txt -bounces 4 -shadows -output out/jf/a07.png -normals out/jf/a07n.png -depth 8 18 out/jf/a07d.png -jitter -filter