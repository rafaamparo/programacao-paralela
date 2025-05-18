#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <math.h>

int primo (long int n) { /* mpi_primos.c  */
    int i;
    for (i = 3; i < (int)(sqrt(n) + 1); i += 2) {
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
        if (n < 2) {
            printf("Valor inválido! Entre com um valor maior que 2\n");
            return 0;
        }
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &meu_ranque);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    t_inicial = MPI_Wtime();

    inicio = 3 + meu_ranque * 2;
    salto  = num_procs * 2;
    for (i = inicio; i <= n; i += salto) {
        if (primo(i)) cont++;
    }

    if (num_procs > 1) {
        MPI_Barrier(MPI_COMM_WORLD);  /* garante que rank 0 postou os receives */

        if (meu_ranque == 0) {
            total = cont;

            int  *recv_bufs   = malloc((num_procs - 1) * sizeof(int));
            MPI_Request *reqs = malloc((num_procs - 1) * sizeof(MPI_Request));

            /* posta todas as Irecv de uma vez, sem bloquear */
            for (int origem = 1; origem < num_procs; origem++) {
                MPI_Irecv(
                    &recv_bufs[origem - 1],  /* buffer para este rank */
                    1, MPI_INT,
                    origem, 0,
                    MPI_COMM_WORLD,
                    &reqs[origem - 1]        /* request não-bloqueante */
                );
            }

            /* aguarda todas as mensagens chegarem */
            MPI_Waitall(num_procs - 1, reqs, MPI_STATUSES_IGNORE);
            for (int j = 0; j < num_procs - 1; j++) {
                total += recv_bufs[j];
            }

            free(recv_bufs);
            free(reqs);
        }
        else {
            MPI_Rsend(&cont, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }
    else {
        total = cont; /* se só um processo, conta os primos encontrados */
    }

    t_final = MPI_Wtime();

    if (meu_ranque == 0) {
        total += 1;  /* inclui o 2 */
        printf("Quant. de primos entre 1 e %ld: %d\n", n, total);
        printf("Tempo de execucao: %0.9f s\n", t_final - t_inicial);
    }

    MPI_Finalize();
    return 0;
}
