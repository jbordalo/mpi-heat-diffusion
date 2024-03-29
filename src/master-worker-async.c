/*
 * Based on materials from:
 * https://github.com/csc-training/openacc/tree/master/exercises/heat
 * https://enccs.github.io/OpenACC-CUDA-beginners/2.02_cuda-heat-equation/
 * changed 23 nov 2022 - vad@fct.unl.pt
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi/mpi.h>

#ifdef PNG
#include "pngwriter.h"
#endif

#define MASTER rank == 0
#define WORKER rank != 0

/* Convert 2D index layout to unrolled 1D layout
 * \param[in] i      Row index
 * \param[in] j      Column index
 * \param[in] width  The width of the area
 * \returns An index in the unrolled 1D array.
 */
int getIndex(const int i, const int j, const int width) {
    return i * width + j;
}

void initTemp(float *T, int h, int w) {
    // Initializing the data with heat from top side
    // all other points at zero
    for (int i = 0; i < w; i++) {
        T[i] = 100.0;
    }
}

/* write_pgm - write a PGM image ascii file
 */
void write_pgm(FILE *f, float *img, int width, int height, int maxcolors) {
    // header
    fprintf(f, "P2\n%d %d %d\n", width, height, maxcolors);
    // data
    for (int l = 0; l < height; l++) {
        for (int c = 0; c < width; c++) {
            int p = (l * width + c);
            fprintf(f, "%d ", (int) (img[p]));
        }
        putc('\n', f);
    }
}


/* write heat map image
*/
void writeTemp(float *T, int h, int w, int n) {
    char filename[64];
#ifdef PNG
    sprintf(filename, "heat_%06d.png", n);
    save_png(T, h, w, filename, 'c');
#else
    sprintf(filename, "heat_%06d.pgm", n);
    FILE *f = fopen(filename, "w");
    write_pgm(f, T, w, h, 100);
    fclose(f);
#endif
}

double timedif(struct timespec *t, struct timespec *t0) {
    return (t->tv_sec - t0->tv_sec) + 1.0e-9 * (double) (t->tv_nsec - t0->tv_nsec);
}

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size < 2) {
        printf("Need at least two nodes for a master-worker pattern.");
        MPI_Finalize();
        exit(0);
    }

    const int nx = 200;   // Width of the area
    const int ny = 200;   // Height of the area

    const float a = 0.5;     // Diffusion constant

    const float h = 0.005;   // h=dx=dy  grid spacing

    const float h2 = h * h;

    const float dt = h2 / (4.0 * a); // Largest stable time step
    const int numSteps = 100000;      // Number of time steps to simulate (time=numSteps*dt)
    const int outputEvery = 100000;   // How frequently to write output image

    int numElements = nx * ny;

    // Allocate two sets of data for current and next timesteps
    float *Tn = (float *) calloc(numElements, sizeof(float));
    float *Tnp1 = (float *) calloc(numElements, sizeof(float));

    // Initializing the data for T0
    initTemp(Tn, nx, ny);

    // Fill in the data on the next step to ensure that the boundaries are identical.
    memcpy(Tnp1, Tn, numElements * sizeof(float));

    struct timespec t0, t;
    if (MASTER) {
        printf("Simulated time: %g (%d steps of %g)\n", numSteps * dt, numSteps, dt);
        printf("Simulated surface: %gx%g (in %dx%g divisions)\n", nx * h, ny * h, nx, h);
        writeTemp(Tn, nx, ny, 0);

        /*start*/
        clock_gettime(CLOCK_MONOTONIC, &t0);
    }

    int iterPerNode = nx / (size-1);
    int extra = nx % (size-1);

    int *recvcounts = malloc(size * sizeof(int));
    int *displs = malloc(size * sizeof(int));

    recvcounts[0] = 0;
    displs[0] = 0;

    int step = ny * iterPerNode;
    for (int i = 2; i < size; i++) {
        recvcounts[i] = step;
        displs[i] = (i-1) * step + extra * ny;
    }

    recvcounts[1] = step + ny*extra;
    displs[1] = 0;

    if (rank == 1) iterPerNode += extra;

    MPI_Request request = MPI_REQUEST_NULL;

    const int offset = 1 + (rank-1) * iterPerNode + (rank==1 ? 0 : extra);

    if (WORKER) printf("Process %d doing from [%d to %d[.\n", rank, offset, offset + iterPerNode);

    // Main loop
    for (int n = 0; n <= numSteps; n++) {
        // Going through the entire area for one step
        // (borders stay at the same fixed temperatures)
        if (WORKER) {

            int i;
            for (i = offset; i < offset + iterPerNode && i < nx - 1; i++) {
                for (int j = 1; j < ny - 1; j++) {
                    const int index = getIndex(i, j, ny);
                    float tij = Tn[index];
                    float tim1j = Tn[getIndex(i - 1, j, ny)];
                    float tijm1 = Tn[getIndex(i, j - 1, ny)];
                    float tip1j = Tn[getIndex(i + 1, j, ny)];
                    float tijp1 = Tn[getIndex(i, j + 1, ny)];

                    Tnp1[index] = tij + a * dt * ((tim1j + tip1j + tijm1 + tijp1 - 4.0 * tij) / h2);
                }
            }

            // Send top border to the top
            if (rank != 1) {
                MPI_Send(&Tnp1[getIndex(offset, 0, ny)], ny, MPI_FLOAT, rank - 1, 1, MPI_COMM_WORLD);
                MPI_Recv(&Tnp1[getIndex(offset - 1, 0, ny)], ny, MPI_FLOAT, rank - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }

            // Send bottom border to the bottom
            if (rank != size - 1) {
                MPI_Send(&Tnp1[getIndex(i - 1, 0, ny)], ny, MPI_FLOAT, rank + 1, 1, MPI_COMM_WORLD);
                MPI_Recv(&Tnp1[getIndex(i, 0, ny)], ny, MPI_FLOAT, rank + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }

            MPI_Wait(&request, MPI_STATUS_IGNORE);

            // Swapping the pointers for the next timestep
            float *temp = Tn;
            Tn = Tnp1;
            Tnp1 = temp;
        }

        // Write the output if needed
        if ((n + 1) % outputEvery == 0) {
            MPI_Igatherv(&Tn[getIndex(offset - 1, 0, ny)], MASTER ? 0 : ny * iterPerNode, MPI_FLOAT,
                        &Tn[0], recvcounts, displs, MPI_FLOAT, 0, MPI_COMM_WORLD, &request);

            if (MASTER) {
                MPI_Wait(&request, MPI_STATUS_IGNORE);
                writeTemp(Tn, nx, ny, n + 1);
            }
        }

    }

    if (MASTER) {
        /*end*/
        clock_gettime(CLOCK_MONOTONIC, &t);
        printf("It took %f seconds\n", timedif(&t, &t0));
    }

    MPI_Finalize();

    // Release the memory
    free(Tn);
    free(Tnp1);

    return 0;
}
