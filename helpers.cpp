#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include "unistd.h"
#include <tuple>
#include "mpi.h"

// MPI HELPER FUNCTIONS

/**
 * @brief Get the start index for this rank in an array with `total` elements.
 *
 * @param total The length of the array
 * @param num_pes Total number of MPI ranks
 * @param node_id This process' rank
 * @return int64_t Starting index for this rank
 */
static int64_t get_start(int64_t total, int num_pes, int node_id)
{
    int64_t div_chunk = (int64_t)ceil(total / ((double)num_pes));
    int64_t start = std::min(total, node_id * div_chunk);
    return start;
}

/**
 * @brief Get the end index for this rank in an array with `total` elements.
 *
 * @param total The length of the array
 * @param num_pes Total number of MPI ranks
 * @param node_id This process' rank
 * @return int64_t End index for this rank
 */
static int64_t get_end(int64_t total, int num_pes, int node_id)
{
    int64_t div_chunk = (int64_t)ceil(total / ((double)num_pes));
    int64_t end = std::min(total, (node_id + 1) * div_chunk);
    return end;
}

/**
 * @brief Get the size of the chunk for this rank in an array with `total` elements.
 *
 * @param total The length of the array
 * @param num_pes Total number of MPI ranks
 * @param node_id This process' rank
 * @return int64_t Chunk size for this rank
 */
static int64_t get_node_portion(int64_t total, int num_pes, int node_id)
{
    return get_end(total, num_pes, node_id) -
           get_start(total, num_pes, node_id);
}

/**
 * @brief Generate a random vector of doubles of length `size`.
 * Note that `size` is the length of the vector across all
 * ranks, so this rank will only receive a chunk of it.
 *
 * @param size Length of random vector to create
 * @return std::vector<double> Random vector chunk for this rank
 */
std::vector<double> _gen_random(int64_t size, int rank, int n_pes)
{
    size = get_node_portion(size, n_pes, rank);
    std::random_device rd;
    std::default_random_engine r_engine(rd());
    std::uniform_real_distribution<> r_dist(0, 1);
    std::vector<double> data(size);
    std::generate(data.begin(), data.end(), [&r_engine, &r_dist]()
                  { return r_dist(r_engine); });
    return data;
}

/**
 * @brief Similar to _gen_random, except the output is an int32 array.
 *
 * @param high Upper-bound on the integers in the array.
 * @param size Length of random vector to create
 * @return std::vector<int> Random vector chunk for this rank
 */
std::vector<int> _gen_random_int32(int64_t high, int64_t size, int rank, int n_pes)
{
    size = get_node_portion(size, n_pes, rank);
    std::random_device rd;
    std::default_random_engine r_engine(rd());
    std::uniform_int_distribution<> r_dist(0, high);
    std::vector<int> data(size);
    std::generate(data.begin(), data.end(), [&r_engine, &r_dist]()
                  { return r_dist(r_engine); });
    return data;
}

/**
 * @brief Create a vector of ints from 0 to size-1.
 * Note that this is a distributed vector, so this
 * function only returns this rank's chunk.
 *
 * @param size Length of vector to create
 * @return std::vector<int> This rank's chunk
 */
std::vector<int> _gen_range32(int64_t size, int rank, int n_pes)
{
    int64_t start = get_start(size, n_pes, rank);
    int64_t end = get_end(size, n_pes, rank);
    size = get_node_portion(size, n_pes, rank);
    std::vector<int> data(size);
    for (uint64_t i = start; i < end; i++)
        data[i - start] = static_cast<int>(i);
    return data;
}

/**
 * @brief Helper function around the MPI_Allreduce API.
 * This takes a value on each rank, computes the global
 * sum, and returns that on each rank.
 *
 * @param val This rank's value
 * @return int Global sum of `value`s from all ranks
 */
int allreduce_sum_scalar(int val)
{
    int ret;
    MPI_Allreduce(&val, &ret, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    return ret;
}

/**
 * @brief Same as allreduce_sum_scalar, except for int64_t.
 *
 * @param val This rank's value
 * @return int64_t Global sum of `value`s from all ranks
 */
int64_t allreduce_sum_scalar(int64_t val)
{
    int64_t ret;
    MPI_Allreduce(&val, &ret, 1, MPI_LONG_LONG_INT, MPI_SUM, MPI_COMM_WORLD);
    return ret;
}

/**
 * @brief Same as allreduce_sum_scalar, except for double.
 *
 * @param val This rank's value
 * @return double Global sum of `value`s from all ranks
 */
double allreduce_sum_scalar(double val)
{
    double ret;
    MPI_Allreduce(&val, &ret, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    return ret;
}

/**
 * @brief Helper function to send an int from this rank to every other rank
 * using MPI_Alltoall
 *
 * @param send_buffer Integer buffer with data to send (as long as the number of ranks)
 * @param[out] recv_buffer Integer buffer where the received data from each rank will be stored.
 */
void alltoall_single_int(int *send_buffer, int *recv_buffer)
{
    MPI_Alltoall(send_buffer, 1, MPI_INT, recv_buffer, 1, MPI_INT, MPI_COMM_WORLD);
}

/**
 * @brief Helper function for sending variable sized integer buffers between all ranks using
 * MPI's Alltoallv collective operation.
 *
 * @param send_buffer Buffer with data to send to all ranks, ordered by the rank to send
 * the data to. The data to send to rank k should start at send_disp_buffer[k]'th index.
 * @param send_counts Vector of integers containing the number of elements to send to
 * each rank (ordered by the rank). Should be of length `n_pes`.
 * @param send_disp Vector of integers containing the displacements for the data to
 * send to each rank in send_buffer. Should be of length `n_pes`.
 * @param[out] recv_buffer Buffer to copy the data into. Make sure that it is appropriately
 * sized.
 * @param recv_counts Vector of integers containing the number og elements to receive from
 * each rank (ordered by the rank). Should be of length `n_pes`.
 * @param recv_disp Vector of integers containing the displacement in `recv_buffer` where
 * data from each rank should be put. Should be of length n_pes.
 */
void alltoallv_int(int *send_buffer, std::vector<int> &send_counts, std::vector<int> &send_disp,
                   int *recv_buffer, std::vector<int> &recv_counts, std::vector<int> &recv_disp)
{
    MPI_Alltoallv(send_buffer, send_counts.data(), send_disp.data(), MPI_INT,
                  recv_buffer, recv_counts.data(), recv_disp.data(), MPI_INT, MPI_COMM_WORLD);
}

/**
 * @brief Same as alltoallv_int, except for sending and receiving doubles.
 */
void alltoallv_double(double *send_buffer, std::vector<int> &send_counts, std::vector<int> &send_disp,
                      double *recv_buffer, std::vector<int> &recv_counts, std::vector<int> &recv_disp)
{
    MPI_Alltoallv(send_buffer, send_counts.data(), send_disp.data(), MPI_DOUBLE,
                  recv_buffer, recv_counts.data(), recv_disp.data(), MPI_DOUBLE, MPI_COMM_WORLD);
}


// INPUT GENERATORS

/**
 * @brief Generate random inputs for the two tables.
 */
void generate_random_inputs(std::vector<int>& keys1, std::vector<int>& keys2, std::vector<double>& data0, std::vector<int>& data1, int rank, int n_pes) {
    int n = 10;
    // Generate random keys
    keys1 = _gen_random_int32(6, n, rank, n_pes);
    keys2 = _gen_random_int32(6, n, rank, n_pes);
    // Generate random data
    data0 = _gen_random(n, rank, n_pes);
    data1 = _gen_range32(n, rank, n_pes);
}

/**
 * @brief Generate inputs from the example in README for the two tables.
 */
void generate_example_inputs(std::vector<int>& keys1, std::vector<int>& keys2, std::vector<double>& data0, std::vector<int>& data1, int rank, int n_pes) {
    std::vector<int> keys1_global = { 0, 1, 1, 2, 1, 0 };
    size_t left_table_size = keys1_global.size();
    keys1 = {keys1_global.begin() + get_start(left_table_size, n_pes, rank), keys1_global.begin() + get_end(left_table_size, n_pes, rank)};
    std::vector<int> keys2_global = { 1, 0, 4, 2, 5, 3 };
    size_t right_table_size = keys2_global.size();
    keys2 = {keys2_global.begin() + get_start(right_table_size, n_pes, rank), keys2_global.begin() + get_end(right_table_size, n_pes, rank)};
    std::vector<double> data0_global = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
    data0 = {data0_global.begin() + get_start(right_table_size, n_pes, rank), data0_global.begin() + get_end(right_table_size, n_pes, rank)};
    std::vector<int> data1_global = { 4, 1, 2, 3, 0, 5 };
    data1 = {data1_global.begin() + get_start(right_table_size, n_pes, rank), data1_global.begin() + get_end(right_table_size, n_pes, rank)};
}
