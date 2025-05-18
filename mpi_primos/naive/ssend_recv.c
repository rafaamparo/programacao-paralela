#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <math.h>

int primo(long int n) {
    for (int i = 3; i < (int)(sqrt(n) + 1); i += 2) {
        if (n % i == 0) return 0;
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
        n = strtol(argv[1], NULL, 10);
    }

    if (n < 2) {
        printf("Valor inválido! Entre com um valor maior que 2\n");
        return 0;
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &meu_ranque);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    t_inicial = MPI_Wtime();

    inicio = 3 + meu_ranque * 2;
    salto = num_procs * 2;

    for (i = inicio; i <= n; i += salto) {
        if (primo(i)) cont++;
    }

    if (meu_ranque == 0) {
        total = cont;
        for (int src = 1; src < num_procs; src++) {
            int recv_count;
            MPI_Status status;
            MPI_Recv(&recv_count, 1, MPI_INT, src, 0, MPI_COMM_WORLD, &status);
            total += recv_count;
        }
    } else {
        MPI_Ssend(&cont, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); // Envio síncrono
    }

    t_final = MPI_Wtime();

    if (meu_ranque == 0) {
        total += 1;  // inclui o 2
        printf("Quant. de primos entre 1 e %ld: %d \n", n, total);
        printf("Tempo de execucao: %0.9f s\n", t_final - t_inicial);
    }

    MPI_Finalize();
    return 0;
}
