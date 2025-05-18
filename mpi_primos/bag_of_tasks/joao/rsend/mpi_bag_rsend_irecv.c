#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"

#define TAMANHO 500000

int primo(int n) {
    if (n % 2 == 0) return (n == 2);
    int limite = (int)(sqrt(n) + 1);
    for (int i = 3; i <= limite; i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int meu_ranque, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &meu_ranque);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (argc < 2) {
        if (meu_ranque == 0)
            printf("Rodar mpirun -n <p> %s <n>\n", argv[0]);
        MPI_Finalize();
        return 0;
    }
    if (num_procs < 2) {
        if (meu_ranque == 0)
            printf("Este programa requer pelo menos 2 processos.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    const int raiz = 0;
    int tag, cont, total = 0;
    int inicio, dest, stop = 0;
    double t0, t1;
    MPI_Status estado;
    MPI_Request request; // Request para comunicações não-bloqueantes
    MPI_Status status_isend; // Status para verificar conclusão de operações não-bloqueantes

    int n = strtol(argv[1], NULL, 10);
    if (n < 2 ) {
        printf("Valor inválido! Entre com um valor maior que 2\n");
        MPI_Finalize();
        return 0;
    }

    t0 = MPI_Wtime();
    
    // Barreira de sincronização para garantir que todos os processos estejam prontos
    MPI_Barrier(MPI_COMM_WORLD);

    if (meu_ranque == raiz) {
        // --- Código do processo pai ---
        inicio = 3;   // inicia a busca em 3
        int valor_inicio; // Variável auxiliar para armazenar valores enviados

        // Envio inicial de uma faixa para cada filho
        for (dest = 1; dest < num_procs; dest++) {
            if (inicio < n) {
                tag = 1;
                valor_inicio = inicio; // Armazena o valor atual de inicio
                MPI_Rsend(&valor_inicio, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
                inicio += TAMANHO;
            } else {
                // sem mais trabalho: sinal de parada
                tag = 99;
                valor_inicio = inicio; // Armazena o valor atual de inicio
                MPI_Rsend(&valor_inicio, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
                stop++;
            }
        }

        // Recebe resultados e despacha novas faixas até todos encerrarem
        while (stop < num_procs - 1) {
            // O processo filho sinalizou com o resultado e está pronto para receber
            MPI_Irecv(&cont, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, &estado);
            total += cont;
            dest = estado.MPI_SOURCE;
            if (inicio < n) {
                tag = 1;
                valor_inicio = inicio; // Armazena o valor atual de inicio
                MPI_Rsend(&valor_inicio, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
                inicio += TAMANHO;
            } else {
                tag = 99;
                valor_inicio = inicio; // Armazena o valor atual de inicio
                MPI_Rsend(&valor_inicio, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
                stop++;
            }
        }

        // conta o 2
        total += 1;
        t1 = MPI_Wtime();
        printf("Quant. de primos entre 1 e %d: %d\n", n, total);
        printf("Tempo de execucao: %0.9f s\n", t1 - t0);
    }
    else {
        // --- Código dos processos filhos ---
        while (1) {
            MPI_Irecv(&inicio, 1, MPI_INT, raiz, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, &estado);
            printf("Processo %d: minha mensagem tem tag %d\n", meu_ranque, estado.MPI_TAG);
            printf("Processo %d recebeu uma faixa que começa em %d\n", meu_ranque, inicio);
            if (estado.MPI_TAG == 99) {
                // sem mais trabalho: sai do loop
                break;
            }
            // calcula primos na faixa [inicio, inicio+TAMANHO)
            cont = 0;
            int limite = inicio + TAMANHO;
            for (int i = inicio; i < limite && i < n; i += 2) {
                if (primo(i)) cont++;
            }
            // O envio dos resultados é seguro pois o raiz já chamou MPI_Recv
            MPI_Rsend(&cont, 1, MPI_INT, raiz, 1, MPI_COMM_WORLD);
        }
        t1 = MPI_Wtime();
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
