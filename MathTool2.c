#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

// Helper function to print a vector with a label
// This function takes a label and prints the vector values in a formatted way
void print_vector(const char *label, double *vector, int n) {
    printf("[");
    for (int i = 0; i < n; i++) {
        printf("%.4f%s", vector[i], (i < n - 1) ? ", " : "");
    }
    printf("]\n");
}

int main(int argc, char *argv[]) {
    int continue_program = 1;

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    while (continue_program) {
        // Declare variables for rank, size, vector size (n), and local partition sizes
        int rank, size, n, local_n, remainder;
        double *X = NULL, *local_X = NULL;
        double local_min, local_max, global_min, global_max;
        double local_sum = 0.0, global_sum;
        double mean, variance, local_variance = 0.0, global_variance;
        double range;

        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        // The root process (rank 0) reads input and distributes work
        if (rank == 0) {
            printf("Enter the size of the vector (n): ");
            scanf("%d", &n);

            // Allocate memory for the vector and read its elements
            X = (double *)malloc(n * sizeof(double));
            printf("Enter the vector elements:\n");
            for (int i = 0; i < n; i++) {
                printf("X[%d]: ", i);
                scanf("%lf", &X[i]);
            }
        }

        // Broadcast the size of the vector to all processes
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // Calculate the size of the local partition for each process and the remainder
        local_n = n / size;
        remainder = n % size;

        // The last process receives the remainder of the elements
        if (rank == size - 1) {
            local_n += remainder;
        }
        local_X = (double *)malloc(local_n * sizeof(double));

        // Prepare arrays to store counts and displacements for scattering data unevenly
        int *counts = NULL, *displs = NULL;
        if (rank == 0) {
            counts = (int *)malloc(size * sizeof(int));
            displs = (int *)malloc(size * sizeof(int));
            for (int i = 0; i < size; i++) {
                counts[i] = n / size + (i == size - 1 ? remainder : 0);
                displs[i] = i * (n / size);
            }
        }

        // Scatter the vector to all processes using MPI_Scatterv
        MPI_Scatterv(X, counts, displs, MPI_DOUBLE, local_X, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // Calculate local sum, minimum, and maximum
        local_min = DBL_MAX;
        local_max = -DBL_MAX;
        for (int i = 0; i < local_n; i++) {
            local_sum += local_X[i];
            if (local_X[i] < local_min) local_min = local_X[i];
            if (local_X[i] > local_max) local_max = local_X[i];
        }

        // Reduce to compute global sum, minimum, and maximum across all processes
        MPI_Reduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&local_min, &global_min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
        MPI_Reduce(&local_max, &global_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        // Calculate the mean of the vector on the root process
        if (rank == 0) {
            mean = global_sum / n;
        }

        // Broadcast the calculated mean, minimum, and maximum to all processes
        MPI_Bcast(&mean, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Bcast(&global_min, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Bcast(&global_max, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // Calculate local variance contributions
        for (int i = 0; i < local_n; i++) {
            local_variance += (local_X[i] - mean)*(local_X[i] - mean);
        }

        // Reduce to compute global variance
        MPI_Reduce(&local_variance, &global_variance, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        // Calculate range and construct the local portion of the delta vector
        range = global_max - global_min;
        double *local_D = (double *)malloc(local_n * sizeof(double));
        for (int i = 0; i < local_n; i++) {
            local_D[i] = ((local_X[i] - global_min) / range) * 100.0;
        }

        // Gather the delta vector on the root process using MPI_Gatherv
        double *D = NULL;
        if (rank == 0) {
            D = (double *)malloc(n * sizeof(double));
        }
        MPI_Gatherv(local_D, local_n, MPI_DOUBLE, D, counts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // Find the maximum value in the delta vector and its index
        double local_max_D = -DBL_MAX;
        int local_max_index = -1;
        for (int i = 0; i < local_n; i++) {
            if (local_D[i] > local_max_D) {
                local_max_D = local_D[i];
                local_max_index = rank * (n / size) + i;
            }
        }

        // Structure to store value and index for reduction
        struct {
            double value;
            int index;
        } local_max_info, global_max_info;

        local_max_info.value = local_max_D;
        local_max_info.index = local_max_index;

        // Reduce to find the global maximum delta and its index
        MPI_Reduce(&local_max_info, &global_max_info, 1, MPI_DOUBLE_INT, MPI_MAXLOC, 0, MPI_COMM_WORLD);

        // Calculate prefix sums using MPI_Scan
        double local_prefix_sum = 0.0;
        double *local_prefix = (double *)malloc(local_n * sizeof(double));
        for (int i = 0; i < local_n; i++) {
            local_prefix_sum += local_X[i];
            local_prefix[i] = local_prefix_sum;
        }

        double prefix_offset;
        MPI_Scan(&local_prefix_sum, &prefix_offset, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        prefix_offset -= local_prefix_sum;
        for (int i = 0; i < local_n; i++) {
            local_prefix[i] += prefix_offset;
        }

        // Gather the prefix sums on the root process using MPI_Gatherv
        double *prefix_sums = NULL;
        if (rank == 0) {
            prefix_sums = (double *)malloc(n * sizeof(double));
        }
        MPI_Gatherv(local_prefix, local_n, MPI_DOUBLE, prefix_sums, counts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // Print the results on the root process
        if (rank == 0) {
            int below_avg = 0, above_avg = 0;
            for (int i = 0; i < n; i++) {
                if (X[i] < mean) below_avg++;
                else if (X[i] > mean) above_avg++;
            }

            printf("X:               ");
            print_vector("", X, n);
            printf("Average:         %.4f\n", mean);
            printf("Xmin:            %.4f\n", global_min);
            printf("Xmax:            %.4f\n", global_max);
            printf("Below Average:   %d\n", below_avg);
            printf("Above Average:   %d\n", above_avg);
            printf("Variance:        %.4f\n", global_variance / n);
            printf("D:               ");
            print_vector("", D, n);
            printf("Dmax:            %.4f\n", global_max_info.value);
            printf("Dmaxloc:         %d\n", global_max_info.index);
            printf("Prefix Sums:     ");
            print_vector("", prefix_sums, n);

            // Ask the user if they want to continue
            printf("\nDo you want to run the program again? (1 for Yes, 0 for No): ");
            scanf("%d", &continue_program);
        }

        // Clean up allocated memory
        free(local_X);
        free(local_D);
        free(local_prefix);
        if (rank == 0) {
            free(X);
            free(D);
            free(prefix_sums);
            free(counts);
            free(displs);
        }

        // Broadcast the decision to continue to all processes
        MPI_Bcast(&continue_program, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }

    // Finalize the MPI environment
    MPI_Finalize();
    return 0;
}

