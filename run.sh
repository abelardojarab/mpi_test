set -e

NUM_CORES=${1:-1}

# Compile and link
# g++ -fPIC -std=c++11 -I$CONDA_PREFIX/include helpers.hpp main.cpp -L$CONDA_PREFIX/lib -lmpi -o main.out
mpic++ -fPIC -std=c++11 main.cpp -o main.out

# Run
# mpiexec -n $NUM_CORES ./main.out

