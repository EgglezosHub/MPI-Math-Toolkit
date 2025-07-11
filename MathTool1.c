#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define TAG_T           1
#define TAG_N           2
#define TAG_BUFSIZE     3
#define TAG_MENU        4
#define TAG_SORTED      5
#define TAG_VAL         6
#define TAG_PREV        7

int
main(int argc, char *argv[])
{
        MPI_Status status;
        int nproc, rank, rc;
        int *t, n, prev;
        int bufsize, starterbuf, offset, remaining;
        int val, sorted;        /* results from each process */
        int f_val, f_sorted;    /* final results */
        int found = 0;          /* indicates that the first unsorted element has been found */
        int i, ch = 1;

        /* just in case an error occurs during initialization */
        if ((rc = MPI_Init(&argc, &argv)) != 0) {
                fprintf(stderr, "%s: cannot intialize MPI.\n", argv[0]);
                MPI_Abort(MPI_COMM_WORLD, rc);
        }

        /* count how many processes we have running */
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &nproc);

        /* main loop */
        while (ch != 2) {
                if (rank == 0) {
                        printf("Enter N: ");
                        scanf("%d", &n);
                        getchar();

                        t = malloc(n * sizeof(int));

                        /* read whole array */
                        for (i = 0; i < n; i++) {
                                printf("t[%d]: ", i);
                                scanf("%d", &t[i]);
                        }
                        getchar();
                        
                        /* reset flag */
                        sorted = 1;
                        /* see if there are any remaining elements */
                        remaining = n % nproc;
                        
                        /* calculate offsets and pass sub-array to each processor */
                        for (i = 1; i < nproc; i++) {
                                /* calculate bufsize as if there are remaining == 0 */
                                starterbuf = n / nproc;
                                bufsize = n / nproc;
                                /*
                                 * calculate which part of the array each process will get.
                                 * the last parts makes sure that if there are any
                                 * remaining elements, the offset will adapt properly
                                 */
                                offset = i * bufsize + (i - 1 < remaining ? i : remaining);
                                /* if there are any remaining elements, increment bufsize */
                                if (i < remaining)
                                        bufsize++;

                                /* each process gets the last element of the previous one */
                                prev = t[offset - 1];

                                /* send sub-array along with its size and sorted flag */
                                MPI_Send(&bufsize, 1, MPI_INT, i, TAG_BUFSIZE, MPI_COMM_WORLD);
                                MPI_Send(&starterbuf, 1, MPI_INT, i, TAG_BUFSIZE, MPI_COMM_WORLD);
                                MPI_Send(t + offset, bufsize, MPI_INT, i, TAG_T, MPI_COMM_WORLD);
                                MPI_Send(&prev, 1, MPI_INT, i, TAG_PREV, MPI_COMM_WORLD);
                                MPI_Send(&sorted, 1, MPI_INT, i, TAG_SORTED, MPI_COMM_WORLD);
                                MPI_Send(&n, 1, MPI_INT, i, TAG_N, MPI_COMM_WORLD);
                        }

                        /*
                         * calculate bufsize for proc 0 in and increment
                         * in case there are still remaining elements
                         */
                        bufsize = n / nproc;
                        if (remaining != 0)
                                bufsize++;
                } else {
                        /*
                         * receive sub-arrays and sorted flag and allocate memory for each
                         * process' array
                         */
                        MPI_Recv(&bufsize, 1, MPI_INT, 0, TAG_BUFSIZE, MPI_COMM_WORLD, &status);
                        t = malloc(bufsize * sizeof(int));
                        /*
                        * We will use the starterbuf to calculate the position of the elements because the buffsize
                        * maybe changed due to uneven allocation
                        */
                        MPI_Recv(&starterbuf, 1, MPI_INT, 0, TAG_BUFSIZE, MPI_COMM_WORLD, &status);
                        MPI_Recv(t, bufsize, MPI_INT, 0, TAG_T, MPI_COMM_WORLD, &status);
                        MPI_Recv(&prev, 1, MPI_INT, 0, TAG_PREV, MPI_COMM_WORLD, &status);
                        MPI_Recv(&sorted, 1, MPI_INT, 0, TAG_SORTED, MPI_COMM_WORLD, &status);
                        MPI_Recv(&n, 1, MPI_INT, 0, TAG_N, MPI_COMM_WORLD, &status);
                }
                /* check if array is unsorted */
                for (i = 0; i < bufsize; i++) {
                        
                        /* 
                         * we check if the last element of the previous process
                         * is greater than t[i] so that each process doesn't compare
                         * only against its own elements. we make sure that the rank
                         * is not 0, since it doesn't have a previous rank.
                         */
                        if (i == 0 && (t[i] < prev && rank != 0)) {
                                /*
                                 * calculates the starting position of the process and
                                 * subtracts 1 to find the position of the unsorted element
                                 */
                                val = rank * starterbuf + (rank - 1 < (n % nproc) ? rank : (n % nproc)) - 1;
                                sorted = 0;
                                break;
                        }else if (i == 0 && (t[i] > t[i + 1])){
                                /*
                                 * calculates the starting position of the process and
                                 * adds i to find the position of the unsorted element
                                 */
                        	val = rank * starterbuf + (rank - 1 < (n % nproc) ? rank : (n % nproc)) + i;
                                sorted = 0;
                                break;
                        }else if (i == bufsize - 1 ){
		                /*  
		                 * If we put i < bufsize - 1 in the for loop we risk that if the bufsize = 1
		                 * it wont run. So we put an extra if statement when is the last element of 
		                 * the table to just skip the last loop so we dont have the problem 
		                 * of t[bufsize-1] > t[bufsize](doesnt exist)
		                 */ 
                        	continue;
                        }else if (t[i] > t[i + 1]) {
                                /*
                                 * calculates the starting position of the process and
                                 * adds i+1 to find the position of the unsorted element
                                 * since it would be the next element (right) of the position
                                 * of i we are checking
                                 */
                                val = rank * starterbuf + (rank - 1 < (n % nproc) ? rank : (n % nproc)) + (i+1);
                                sorted = 0;
                                break;
                        }
                }

                /* we're done with t, free everything */
                free(t);

                if (rank == 0) {
                        /* collect results from proc 0 first */
                        f_sorted = sorted;
                        f_val = val;
                        /* if f_sorted is false already, don't bother searching below */
                        found = f_sorted == 0;

                        /* receive results from the rest */
                        for (i = 1; i < nproc; i++) {
                                MPI_Recv(&val, 1, MPI_INT, i, TAG_VAL, MPI_COMM_WORLD, &status);
                                MPI_Recv(&sorted, 1, MPI_INT, i, TAG_SORTED, MPI_COMM_WORLD, &status);
                                /* 
                                 * AND all flags to determine whether the array is sorted.
                                 * if one of the flags is 0, res will be 0 even if the
                                 * rest of the flags are 1
                                 */
                                f_sorted &= sorted;
                                /*
                                 * get only the first unsorted element, we don't care about
                                 * the rest, if any
                                 */
                                if (!sorted && !found) {
                                        f_val = val;
                                        found = 1;
                                }
                        }

                        if (f_sorted)
                                puts("Array is sorted.");
                        else
                                printf("Array is not sorted:\n-> Position of first unsorted element is: %d\n", f_val);

                        puts("Press [ENTER] to continue. . .");
                        getchar();
                } else {
                        /* send results to processor 0 */
                        MPI_Send(&val, 1, MPI_INT, 0, TAG_VAL, MPI_COMM_WORLD);
                        MPI_Send(&sorted, 1, MPI_INT, 0, TAG_SORTED, MPI_COMM_WORLD);
                }

                /* menu */
                if (rank == 0) {
                        system("clear || cls");
                        printf("1. Continue\n2. Exit\nYour choice: \n\n");
                        scanf("%d", &ch);
                        /* everyone has to know what the choice is */
                        for (i = 1; i < nproc; i++)
                                MPI_Send(&ch, 1, MPI_INT, i, TAG_MENU, MPI_COMM_WORLD);
                } else
                        MPI_Recv(&ch, 1, MPI_INT, 0, TAG_MENU, MPI_COMM_WORLD, &status);
        }

        MPI_Finalize();

        return 0;
}
