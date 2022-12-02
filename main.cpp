#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include "unistd.h"
#include <mpi.h>
#include "helpers.cpp"

int rank;
int n_pes;

// JOIN IMPLEMENTATIONS

/**
 * @brief Performant implementation of local join.
 * The left table only has the key column (keys1) which needs
 * to be joined with the first column (keys2) in the right table.
 * data0 and data1 are the 2nd and 3rd columns in the right table.
 *
 * @param keys1 Join key column in the first table
 * @param keys2 Join key column in the second table
 * @param data0 First data column in the second table
 * @param data1 Second data data column in the second table
 * @return std::tuple<std::vector<int>, std::vector<double>, std::vector<int>>
 *  Resulting distributed output (key and data columns from the right table)
 */
std::tuple<std::vector<int>, std::vector<double>, std::vector<int>> local_join_impl(
    std::vector<int> &keys1, 
    std::vector<int> &keys2, std::vector<double> &data0, std::vector<int> &data1)
{

    // TODO Add your implementation here...
    std::vector<int> keys_result;
    std::vector<double> data0_result;
    std::vector<int> data1_result;

    for(const auto& k : keys1) {
        auto p = std::find(keys2.begin(), keys2.end(), k);
        if (p != keys2.end()) {
            int index = p - keys2.begin();
            keys_result.push_back(*p);
            data0_result.push_back(data0[index]);
            data1_result.push_back(data1[index]);
        }
    }

    // This is a dummy return for compilation purposes, replace this with your implementation
    return std::make_tuple(keys_result, data0_result, data1_result);
}

/**
 * @brief Distributed join implementation for joining two tables
 * on an integer column.
 *
 * @param keys1 Join key column in the first table (chunk on this rank)
 * @param keys2 Join key column in the second table (chunk on this rank)
 * @param data0 First data column in the second table (chunk on this rank)
 * @param data1 Second data data column in the second table (chunk on this rank)
 * @return std::tuple<std::vector<int>, std::vector<double>, std::vector<int>>
 *  Resulting distributed output (key and data columns from the right table)
 */

//void alltoallv_int(int *send_buffer, std::vector<int> &send_counts, std::vector<int> &send_disp,
//                   int *recv_buffer, std::vector<int> &recv_counts, std::vector<int> &recv_disp)

std::tuple<std::vector<int>, std::vector<double>, std::vector<int>> parallel_join_impl(
    std::vector<int> &keys1, std::vector<int> &keys2, std::vector<double> &data0, std::vector<int> &data1)
{
    return local_join_impl(keys1, keys2, data0, data1);
}

// DRIVER FUNCTION

int main()
{
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &n_pes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Join data buffers
    std::vector<int> k2;
    std::vector<double> d1;
    std::vector<int> d2;
    
    // Join keys
    std::vector<int> k1;

    // Use one of the following for testing
    // (and comment out the other):

    // Use for testing with given example;
    generate_example_inputs(k1, k2, d1, d2, rank, n_pes);

    // Use for testing with random inputs
    // generate_random_inputs(k1, k2, d1, d2, rank, n_pes);

    MPI_Barrier(MPI_COMM_WORLD);

    // Sleep for clearer stdout
    sleep(rank);
    std::cout << "Rank " << rank << ", input:" << std::endl;
    std::cout << "| keys1 |  | keys2 |   data0   | data1 |" << std::endl;
    size_t max_keys = std::max(k1.size(), k2.size());
    for (int i = 0; i < max_keys; i++) {
        if (i < k1.size()) {
            printf("| %5s |  ", std::to_string(k1[i]).c_str());
        } else {
            printf("           ");
        }
        if (i < k2.size()) {
            printf("| %5s | %9s | %5s |\n", std::to_string(k2[i]).c_str(), std::to_string(d1[i]).c_str(), std::to_string(d2[i]).c_str());
        } else {
            printf("\n");
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // Join ouput buffers
    std::vector<int> o_keys;
    std::vector<double> o1;
    std::vector<int> o2;

    // Perform join
    std::tuple<std::vector<int>, std::vector<double>, std::vector<int>> join_output = parallel_join_impl(k1, k2, d1, d2);

    o_keys = std::get<0>(join_output);
    o1 = std::get<1>(join_output);
    o2 = std::get<2>(join_output);

    // Sleep for clearer stdout
    sleep(rank);
    std::cout << "Rank " << rank << ", output:" << std::endl;
    std::cout << "| key |   data0   | data1 |" << std::endl;
    for (int i = 0; i < o_keys.size(); i++) {
        printf("| %3s | %9s | %5s |\n", std::to_string(o_keys[i]).c_str(), std::to_string(o1[i]).c_str(), std::to_string(o2[i]).c_str());
    }
    MPI_Barrier(MPI_COMM_WORLD);
}
