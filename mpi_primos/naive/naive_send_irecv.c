#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <math.h>

int primo (long int n) {
    int i;
    for (i = 3; i < (int)(sqrt(n) + 1); i+=2) {
        if(n%i == 0) return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    double t_inicial, t_final;
    int cont = 0, total = 0;
    long int i, n;
    int meu_ranque, num_procs, inicio, salto;

    if (argc < 2) {
        printf("Valor inválido! Entre com um valor do maior inteiro\n");
        return 0;
    } else {
        n = strtol(argv[1], (char **) NULL, 10);
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &meu_ranque);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);    

    t_inicial = MPI_Wtime();
    inicio = 3 + meu_ranque * 2;
    salto = num_procs * 2;

    for (i = inicio; i <= n; i += salto) {    
        if(primo(i) == 1) cont++;
    }

    if (meu_ranque == 0) {
        total = cont;

        int *buffer = (int *)malloc((num_procs - 1) * sizeof(int));
        MPI_Request *reqs = (MPI_Request *)malloc((num_procs - 1) * sizeof(MPI_Request));

        for (int src = 1; src < num_procs; src++) {
            MPI_Irecv(&buffer[src - 1], 1, MPI_INT, src, 0, MPI_COMM_WORLD, &reqs[src - 1]);
        }

        // Espera todas as mensagens chegarem
        MPI_Waitall(num_procs - 1, reqs, MPI_STATUSES_IGNORE);

        for (int i = 0; i < num_procs - 1; i++) {
            total += buffer[i];
        }

        free(buffer);
        free(reqs);
    } else {
        MPI_Send(&cont, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    t_final = MPI_Wtime();

    if (meu_ranque == 0) {
        total += 1; 
        printf("Quant. de primos entre 1 e n: %d \n", total);
        printf("Tempo de execucao: %0.9f s\n", t_final - t_inicial);    
    }

    MPI_Finalize();
    return 0;
}
