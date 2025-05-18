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

        // Preparar as variáveis para receber os dados dos demais processos
        int *buffer_recvs = malloc((num_procs - 1) * sizeof(int));
        MPI_Request *requests = malloc((num_procs - 1) * sizeof(MPI_Request));

        // Iniciar recepções não bloqueantes
        for (int src = 1; src < num_procs; src++) {
            MPI_Irecv(&buffer_recvs[src - 1], 1, MPI_INT, src, 0, MPI_COMM_WORLD, &requests[src - 1]);
        }

        // Esperar todas as mensagens chegarem
        MPI_Waitall(num_procs - 1, requests, MPI_STATUSES_IGNORE);

        // Somar os valores recebidos
        for (int i = 0; i < num_procs - 1; i++) {
            total += buffer_recvs[i];
        }

        free(buffer_recvs);
        free(requests);
    } else {
        // Envio síncrono: espera até o processo 0 postar o Irecv correspondente
        MPI_Ssend(&cont, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    t_final = MPI_Wtime();

    if (meu_ranque == 0) {
        total += 1; // inclui o 2, que também é primo
        printf("Quant. de primos entre 1 e %ld: %d \n", n, total);
        printf("Tempo de execucao: %0.9f s\n", t_final - t_inicial);
    }

    MPI_Finalize();
    return 0;
}
