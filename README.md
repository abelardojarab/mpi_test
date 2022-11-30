# Parallel Join

## Background

Single Program Multiple Data (SPMD) parallelism paradigm:
The same program runs on all processors but on different chunks of data.
It is different than client/server and master/executor paradigms in that
all processors are usually "equal" and can communicate with each other.

## Simple Parallel Join Problem

The goal is to write a parallel join implementation in C++.
This function is a simplified database-style join, similar to "merge()"
function in Pandas. The main difference is that left table input does not have
data columns and only has a key column.
The function takes key arrays (int32 keys) for the left and right tables,
and two data columns (an int and a double column) for the right table as input.
Left table doesn't have data arrays for simplicity.
All of this data is distributed (each process has a random chunk of global data for each array).
It returns the data arrays of the joined table (keys are ignored).
Output arrays are distributed as well, but chunk sizes may not be equal among processors.
Order of output data doesn't matter.

For example, let's assume we have two MPI processes and right table has two data arrays:

Process 0:

    left table   right table                 joined table
    | keys |   | keys | data 0 | data 1 |  | keys | data 0 | data 1 |
    | 0    |   | 1    | 1.0    | 4      |  | 0    | 2.0    | 1      |
               | 1    | 2.0    | -2     |
    | 1    |   | 0    | 2.0    | 1      |  | 0    | 2.0    | 1      |
    | 1    |   | 4    | 3.0    | 2      |  | 2    | 4.0    | 3      |

Process 1:

    left table   right table                 joined table
    | keys |   | keys | data 0 | data 1 | | keys | data 0 | data 1 |
    | 2    |   | 2    | 4.0    | 3      | | 1    | 1.0    | 4      |
    | 1    |   | 5    | 5.0    | 0      | | 1    | 1.0    | 4      |
    | 0    |   | 3    | 6.0    | 5      | | 1    | 1.0    | 4      |
                                          | 1    | 2.0    | -2     |
                                          | 1    | 2.0    | -2     |
                                          | 1    | 2.0    | -2     |


The exercise has two parts:

- The first part is writing a performant local join implementation. For this, you need to
  implement the `local_join_impl` function in `main.cpp`. The inputs are the same as
  those described above. A simple yet performant local join implementation is important
  for the performance of a parallel join.
- The second part is writing a parallel join implementation, by shuffling the data
  and then performing a local join on all ranks. The implementation should go in the
  `parallel_join_impl` function in `main.cpp`.

For implementation:
- Use simple hash partitioning (`key % num_processes`) to shuffle the data, and then perform local join.
- MPI_Alltoallv() should be used for shuffling data, so keys of input tables can be matched.
  - Useful reference: https://www.eecis.udel.edu/~pollock/372/f06/alltoallv.ppt
  - You can use the helper functions `alltoallv_int` and `alltoallv_double` (see `helpers.hpp`)
    in your implementation.
- Gathering data on a single process (e.g. MPI_Gatherv) is not acceptable.
- Optimizations such as avoiding communication to the same processor are not required. Keep the code as simple as possible.
- Helpers for several MPI collective operations are provided in `helpers.hpp` for your convenience.

## Testing

For convenience, the `helpers.hpp` file contains two input generation functions, `generate_example_inputs`
and `generate_random_inputs`. The former generates the example described above for easy validation,
and the latter generates random input tables for further testing. You are encouraged
to use these as a reference to add your own input generators (fixed or otherwise) for testing.

You can use the `run.sh` script to compile and run your code. By default, the code is run
with 1 MPI process. Use this to test the correctness of the first part of this exercise,
i.e. the local join implementation, and as a sanity check for the second part of this
exercise, i.e. the parallel join implementation.

To run it with `N` processes, execute `./run.sh <N>`. Use this to exhaustively test
your parallel join implementation.

Once your implementation is finished, running your function (both local and parallel join) 
on a single rank (`./run.sh`) should produce an output similar to::

    Rank 0, input:
    | keys1 |  | keys2 |   data0   | data1 |
    |     0 |  |     1 |  1.000000 |     4 |
    |     1 |  |     0 |  2.000000 |     1 |
    |     1 |  |     4 |  3.000000 |     2 |
    |     2 |  |     2 |  4.000000 |     3 |
    |     1 |  |     5 |  5.000000 |     0 |
    |     0 |  |     3 |  6.000000 |     5 |
    Rank 0, output:
    | key |   data0   | data1 |
    |   1 |  1.000000 |     4 |
    |   1 |  1.000000 |     4 |
    |   1 |  1.000000 |     4 |
    |   0 |  2.000000 |     1 |
    |   0 |  2.000000 |     1 |
    |   2 |  4.000000 |     3 |

Running the parallel join algorithm with 2 processes (`./run.sh 2`), should produce the following ouput,
with keys and data properly grouped by rank according to simple hash partitioning::

    Rank 0, input:
    | keys1 |  | keys2 |   data0   | data1 |
    |     0 |  |     1 |  1.000000 |     4 |
    |     1 |  |     0 |  2.000000 |     1 |
    |     1 |  |     4 |  3.000000 |     2 |
    Rank 1, input:
    | keys1 |  | keys2 |   data0   | data1 |
    |     2 |  |     2 |  4.000000 |     3 |
    |     1 |  |     5 |  5.000000 |     0 |
    |     0 |  |     3 |  6.000000 |     5 |
    Rank 0, output:
    | key |   data0   | data1 |
    |   0 |  2.000000 |     1 |
    |   0 |  2.000000 |     1 |
    |   2 |  4.000000 |     3 |
    Rank 1, output:
    | key |   data0   | data1 |
    |   1 |  1.000000 |     4 |
    |   1 |  1.000000 |     4 |
    |   1 |  1.000000 |     4 |
