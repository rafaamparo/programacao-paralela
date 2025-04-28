#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int rank, size;
    int m = 8, k = 3, n = 5; // A(m x k) * B(k x n) => C(m x n)
    int *a = NULL, *b = NULL, *c = NULL;
    int *particao_a, *sub_c;
    int *quantidade_elementos_para_enviar, *deslocamento_para_enviar;
    int *quantidade_elementos_para_receber, *deslocamento_para_receber;
    int linhas_recebidas;
    double tempo_inicial, tempo_final;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    tempo_inicial = MPI_Wtime();
    b = (int *)malloc(k * n * sizeof(int)); // matriz B, disponivel para todos os processos
    if (rank == 0) {
        a = (int *)malloc(m * k * sizeof(int));
        c = (int *)malloc(m * n * sizeof(int)); // A e C só existem em p0
        quantidade_elementos_para_enviar = (int *)malloc(size * sizeof(int));
        deslocamento_para_enviar = (int *)malloc(size * sizeof(int));
        quantidade_elementos_para_receber = (int *)malloc(size * sizeof(int));
        deslocamento_para_receber = (int *)malloc(size * sizeof(int));

        for (int i = 0; i < m * k; i++)
            a[i] = i + 1;

        // matriz B é matriz identidade, para fins de teste;
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                if (i == j)
                    b[i * n + j] = 1;
                else
                    b[i * n + j] = 0;
            }
        }

        // define como as linhas serão distribuídas entre os processos;
        int linhas_base = m / size;
        int linhas_extras = m % size;

        int deslocamento_linhas = 0;
        for (int i = 0; i < size; i++) {
        // calcula quantas linhas um processo 'i' vai receber
        // cada processo recebe 'linhas_base' linhas e processos com rank menor que 'linhas_extras' recebem 1 linha a mais
            int linhas_para_esse_processo = linhas_base + (i < linhas_extras ? 1 : 0);
            quantidade_elementos_para_enviar[i] = linhas_para_esse_processo * k;
            deslocamento_para_enviar[i] = deslocamento_linhas * k;
            quantidade_elementos_para_receber[i] = linhas_para_esse_processo * n;
            deslocamento_para_receber[i] = deslocamento_linhas * n;
            deslocamento_linhas += linhas_para_esse_processo;
        }
        printf("Matriz A:\n");
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < k; j++) {
                printf("%d ", a[i * k + j]);
            }
            printf("\n");
        }
        printf("\n");
        printf("Matriz B:\n");
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                printf("%d ", b[i * n + j]);
            }
            printf("\n");
        }
        printf("\n");
    }

    // broadcast da matriz B pra todos os processos;
    MPI_Bcast(b, k * n, MPI_INT, 0, MPI_COMM_WORLD);
    // cada processo calcula quantas linhas ele recebe baseado na divisão feita;
    int linhas_base_local = m / size;
    int linhas_extras_local = m % size;
    linhas_recebidas = linhas_base_local + (rank < linhas_extras_local ? 1 : 0);
    particao_a = (int *)malloc(linhas_recebidas * k * sizeof(int)); // espaço reservado para a parte da matriz A que será enviada a cada processo;
    sub_c = (int *)malloc(linhas_recebidas * n * sizeof(int)); // espaço reservado para a parte da matriz C que cada processo vai calcular;

    // distribui as partições de A entre os processos usando Scatterv, que permite tamanhos diferentes de partições;
    MPI_Scatterv(a, quantidade_elementos_para_enviar, deslocamento_para_enviar, MPI_INT,
                 particao_a, linhas_recebidas * k, MPI_INT,
                 0, MPI_COMM_WORLD);
    // o processo com o rank igual a raiz (no caso, definimos a raiz como 0) distribui pelo Scatterv o conteúdo da matriz A de maneira NÃO UNIFORME entre os processos;
    // cada processo recebe uma partição de A de acordo com o número de linhas que ele deve processar; 
    // o número de elementos que cada processo recebe é determinado pelos vetores quantidade_elementos_para_enviar e deslocamento_para_enviar, que indicam:
    // quantos elementos cada processo irá receber e a partir de qual posição da matriz A ele deve começar a receber os dados;
    // exemplo: se o processo 0 deve receber 3 linhas, ele vai receber 3 * k elementos, começando do índice 0;
    // e se o processo 1 deve receber 2 linhas, ele vai receber 2 * k elementos, começando do índice 3 * k.

    // para organizar os prints no terminal, imprimir um processo de cada vez;
    for (int proc = 0; proc < size; proc++) {
        if (rank == proc) {
            printf("=============================================\n");
            printf("Processo %d recebeu a seguinte partição de A:\n", rank);
            for (int i = 0; i < linhas_recebidas; i++) {
                printf("Linha %d: ", i);
                for (int j = 0; j < k; j++) {
                    printf("%d ", particao_a[i * k + j]);
                }
                printf("\n");
            }
            printf("\n");

            printf("Processo %d calculando sua submatriz de C:\n", rank);
            for (int i = 0; i < linhas_recebidas; i++) {
                printf("sub_c[%d]: ", i);
                for (int j = 0; j < n; j++) {
                    sub_c[i * n + j] = 0;
                    for (int l = 0; l < k; l++) {
                        sub_c[i * n + j] += particao_a[i * k + l] * b[l * n + j];
                    }
                    printf("%d ", sub_c[i * n + j]);
                }
                printf("\n");
            }
            printf("=============================================\n\n");
        }
        MPI_Barrier(MPI_COMM_WORLD);
        // a barreira sincroniza os processos para que cada processo imprima na sua vez, sem bagunçar o terminal.
    }

    // recolhe as submatrizes de C de todos os processos usando Gatherv;
    MPI_Gatherv(sub_c, linhas_recebidas * n, MPI_INT,
                c, quantidade_elementos_para_receber, deslocamento_para_receber, MPI_INT,
                0, MPI_COMM_WORLD);
    // o processo com o rank igual a raiz (no caso, definimos a raiz como 0) recolhe pelo Gatherv o conteúdo da matriz C de maneira NÃO UNIFORME entre os processos;
    // cada processo envia uma partição de C de acordo com o número de linhas que ele processou;
    // o número de elementos que cada processo envia é determinado pelos vetores quantidade_elementos_para_receber e deslocamento_para_receber, que indicam:
    // quantos elementos cada processo irá enviar e a partir de qual posição da matriz C ele deve começar a enviar os dados;
    // exemplo: se o processo 0 deve enviar 3 linhas, ele vai enviar 3 * n elementos, começando do índice 0;
    // e se o processo 1 deve enviar 2 linhas, ele vai enviar 2 * n elementos, começando do índice 3 * n.

    if (rank == 0) {
        // processo 0 imprime a matriz C completa após juntar todas as partes;
        printf("========== MATRIZ C (RESULTADO FINAL) ==========\n");
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                printf("%d ", c[i * n + j]);
            }
            printf("\n");
        }
        printf("================================================\n\n");
    }

    free(particao_a);
    free(sub_c);
    free(b);
    // libera a memória das particoes, submatrizes de C e matriz B, que são alocadas em todos os processos;

    if (rank == 0) {
        free(a);
        free(c);
        free(quantidade_elementos_para_enviar);
        free(deslocamento_para_enviar);
        free(quantidade_elementos_para_receber);
        free(deslocamento_para_receber);
        // libera a memória da matriz A, matriz C e dos vetores de controle, que só existem em p0;
    }

    MPI_Finalize();

    if (rank == 0) {
        tempo_final = MPI_Wtime();
        printf("Tempo gasto para calcular a matriz C: %3.6f segundos\n", tempo_final - tempo_inicial);
    }

    return 0;
}
