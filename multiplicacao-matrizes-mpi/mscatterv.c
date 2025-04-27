#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int rank, size;
    int m = 8, k = 3, n = 5; // A(m x k) * B(k x n) => C(m x n)
    int *a = NULL, *b = NULL, *c = NULL;
    int *particao_a, *submatriz_c;
    int linhas_recebidas;
    int *linhas_enviadas_por_proc, *deslocamento_por_proc;
    int *elementos_enviados_por_proc, *deslocamento_elementos_proc;
    double tempo_inicial, tempo_final;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    tempo_inicial = MPI_Wtime();

    // Alocação dos vetores de configuração
    linhas_enviadas_por_proc = (int *)malloc(size * sizeof(int));
    deslocamento_por_proc = (int *)malloc(size * sizeof(int));

    elementos_enviados_por_proc = (int *)malloc(size * sizeof(int));
    deslocamento_elementos_proc = (int *)malloc(size * sizeof(int));

    // Cálculo da divisão de linhas
    int linhas_base = m / size;
    int linhas_extras = m % size;

    for (int i = 0; i < size; i++) {
        if (i < linhas_extras)
            linhas_enviadas_por_proc[i] = linhas_base + 1;
        else
            linhas_enviadas_por_proc[i] = linhas_base;
    }

    deslocamento_por_proc[0] = 0;
    for (int i = 1; i < size; i++) {
        deslocamento_por_proc[i] = deslocamento_por_proc[i-1] + linhas_enviadas_por_proc[i-1];
    }

    // Agora em elementos (não apenas linhas), para usar no Scatterv
    for (int i = 0; i < size; i++) {
        elementos_enviados_por_proc[i] = linhas_enviadas_por_proc[i] * k;
        deslocamento_elementos_proc[i] = deslocamento_por_proc[i] * k;
    }

    // Cada processo aloca apenas o que vai receber
    linhas_recebidas = linhas_enviadas_por_proc[rank];
    particao_a = (int *)malloc(linhas_recebidas * k * sizeof(int));
    b = (int *)malloc(k * n * sizeof(int));
    submatriz_c = (int *)malloc(linhas_recebidas * n * sizeof(int));

    if (rank == 0) {
        a = (int *)malloc(m * k * sizeof(int));
        c = (int *)malloc(m * n * sizeof(int));

        // Inicializar A
        for (int i = 0; i < m * k; i++)
            a[i] = i + 1;

        // Inicializar B como identidade
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                if (i == j)
                    b[i * n + j] = 1;
                else
                    b[i * n + j] = 0;
            }
        }
    }

    // Broadcast de B
    MPI_Bcast(b, k * n, MPI_INT, 0, MPI_COMM_WORLD);

    // Scatterv de A
    MPI_Scatterv(a, elementos_enviados_por_proc, deslocamento_elementos_proc, MPI_INT,
                 particao_a, linhas_recebidas * k, MPI_INT,
                 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    for (int proc = 0; proc < size; proc++) {
        if (rank == proc && linhas_recebidas > 0) {
            printf("Processo %d recebeu:\n", rank);
            for (int i = 0; i < linhas_recebidas; i++) {
                printf("Linha %d: ", i);
                for (int j = 0; j < k; j++) {
                    printf("%d ", particao_a[i * k + j]);
                }
                printf("\n");
            }
            printf("\n");
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    // Calcula submatriz C
    for (int i = 0; i < linhas_recebidas; i++) {
        for (int j = 0; j < n; j++) {
            submatriz_c[i * n + j] = 0;
            for (int l = 0; l < k; l++) {
                submatriz_c[i * n + j] += particao_a[i * k + l] * b[l * n + j];
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    for (int proc = 0; proc < size; proc++) {
        if (rank == proc && linhas_recebidas > 0) {
            printf("Processo %d calculou submatriz C:\n", rank);
            for (int i = 0; i < linhas_recebidas; i++) {
                printf("Linha %d: ", i);
                for (int j = 0; j < n; j++) {
                    printf("%d ", submatriz_c[i * n + j]);
                }
                printf("\n");
            }
            printf("\n");
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    // Gatherv para juntar as submatrizes em C
    int *elementos_recebidos_por_proc = (int *)malloc(size * sizeof(int));
    int *deslocamento_elementos_c = (int *)malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {
        elementos_recebidos_por_proc[i] = linhas_enviadas_por_proc[i] * n;
        deslocamento_elementos_c[i] = deslocamento_por_proc[i] * n;
    }

    MPI_Gatherv(submatriz_c, linhas_recebidas * n, MPI_INT,
                c, elementos_recebidos_por_proc, deslocamento_elementos_c, MPI_INT,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Matriz C (resultado final):\n");
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                printf("%d ", c[i * n + j]);
            }
            printf("\n");
        }
        free(a);
        free(c);
    }

    free(particao_a);
    free(submatriz_c);
    free(b);
    free(linhas_enviadas_por_proc);
    free(deslocamento_por_proc);
    free(elementos_enviados_por_proc);
    free(deslocamento_elementos_proc);
    free(elementos_recebidos_por_proc);
    free(deslocamento_elementos_c);

    tempo_final = MPI_Wtime();
    if (rank == 0) {
        printf("Tempo de execução: %3.6f segundos\n", tempo_final - tempo_inicial);
    }

    MPI_Finalize();
    return 0;
}
