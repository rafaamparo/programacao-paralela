#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int rank, size;
    int m = 8, k = 3, n = 5; // A(m x k) * B(k x n) => C(m x n)
    int *a = NULL, *b = NULL, *c = NULL;
    int *particao_a, *submatriz_c;
    int linhas_por_proc;
    double tempo_inicial, tempo_final;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    tempo_inicial = MPI_Wtime();
    if (m % size != 0 || m < size) {
        if (rank == 0) {
            printf("O número de linhas da matriz A não é divisível pelo número de processos.\n");
        }
        MPI_Finalize();
        return 1;
    }
    linhas_por_proc = m / size;

    // essa verificação serve para garantir que o numero de linhas da matriz A (m) pode ser dividido de forma exata entre os processos;
    // se não for divisivel, ou se tivermos mais processos do que linhas, o programa avisa (apenas em p0, para não imprimir mensagens duplicadas) e encerra o programa;
    // isso ocorre porque o Scatter exige que cada processo receba exatamente a mesma quantidade de elementos;
    // se passou pela verificação, entao podemos calcular quantas linhas cada processo vai receber para trabalhar.

    particao_a = (int *)malloc(linhas_por_proc * k * sizeof(int));
    // espaço reservado para a parte da matriz A que será enviada a cada processo;
    // cada processo vai receber (linhas_por_proc) linhas da matriz A original;
    // como cada linha de A tem (k) colunas, precisamos de  espaço para (linhas_por_proc * k) elementos;
    
    b = (int *)malloc(k * n * sizeof(int)); // matriz B, disponivel para todos os processos
    submatriz_c = (int *)malloc(linhas_por_proc * n * sizeof(int)); 
    // espaço reservado para a parte da matriz C que cada processo vai calcular;
    // cada processo vai gerar (linhas_por_proc) linhas de C, cada uma com (n) colunas;
    // a submatriz_c guarda só o resultado parcial, que depois é enviado para p0 para formar a matriz C completa.

    if (rank == 0) {
        a = (int *)malloc(m * k * sizeof(int));
        c = (int *)malloc(m * n * sizeof(int));
        for (int i = 0; i < m * k; i++)
            a[i] = i + 1;
        for (int i = 0; i < k; i++) { // matriz B é identidade, para fins de teste
            for (int j = 0; j < n; j++) {
                if (i == j)
                    b[i * n + j] = 1;
                else
                    b[i * n + j] = 0;
            }
        }
    }

    // broadcast de B para todos os processos
    MPI_Bcast(b, k * n, MPI_INT, 0, MPI_COMM_WORLD);

    // scatter das linhas de A
    MPI_Scatter(a, linhas_por_proc * k, MPI_INT,
                particao_a, linhas_por_proc * k, MPI_INT,
                0, MPI_COMM_WORLD);
    // o processo com o ranque igual a raiz (no caso, definimos a raiz como 0) distribui pelo scatter o conteúdo de A de maneira UNIFORME entre os processos, inclusive o raiz;
    // cada processo recebe uma partição de A com (linhas_por_proc * k) elementos, que é a parte que ele vai calcular, e armazenará no vetor particao_a;
    // passamos esse valor (linhas_por_proc * k) porque o Scatter não consegue enviar uma "linha" da matriz, ele envia elementos;
    // entao se por exemplo, cada processo deve receber 1 linha e a matriz tem 3 colunas, cada processo deve receber 3 elementos.
    
    for (int proc = 0; proc < size; proc++) { // cada processo vai mostrar o que recebeu e calcula sua subC
        if (rank == proc) {
            printf("Processo %d recebeu a seguinte partição de A:\n", rank); // mostra a partição de A recebida pelo scatter 
            for (int i = 0; i < linhas_por_proc; i++) {
                printf("Partição_de_A[%d]: ", i);
                for (int j = 0; j < k; j++) {
                    printf("%d ", particao_a[i * k + j]);
                }
                printf("\n");
            }
            printf("\n");

            printf("Processo %d calculando e imprimindo sua submatriz de C:\n", rank); // cálculo da submatriz de C para cada processo
            for (int i = 0; i < linhas_por_proc; i++) {
                printf("subC[%d]: ", i);
                for (int j = 0; j < n; j++) {
                    submatriz_c[i * n + j] = 0;
                    for (int l = 0; l < k; l++) {
                        submatriz_c[i * n + j] += particao_a[i * k + l] * b[l * n + j];
                    }
                    printf("%d ", submatriz_c[i * n + j]);
                }
                printf("\n");
            }
            printf("\n");
        }
    MPI_Barrier(MPI_COMM_WORLD);
    // o Barrier sincroniza todos os processos neste ponto, então um processo só avança depois que todos chegarem aqui;
    // essa sincronização é feita apenas só para organizar a ordem dos prints no terminal;
    // lembrando que essa barreira NÃO interfere na paralelização da multiplicação de matrizes;
    // quando o Barrier é chamado para sincronizar os prints, a execução paralela já aconteceu.
}


    // gather das submatrizes de C no processo 0
    MPI_Gather(submatriz_c, linhas_por_proc * n, MPI_INT,
               c, linhas_por_proc * n, MPI_INT,
               0, MPI_COMM_WORLD);
    // o processo raiz (p0) recolhe com o gather as submatrizes de C calculadas por todos os processos, inclusive dele;
    // cada processo envia para o raiz sua submatriz parcial (submatriz_c), contendo (linhas_por_proc * n) elementos (assim como o scatter, o gather trabalha com elementos); 
    // ou seja, a quantidade de números correspondentes as linhas que o processo calculou vezes o número de colunas da matriz C resultante;
    // o processo 0 recebe tudo isso no vetor c, juntando as submatrizes pela ordem do rank dos processos, completando a matriz C no final.

    if (rank == 0) {
        // processo 0 imprime a matriz C completa
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

    MPI_Finalize();
    if (rank == 0) {
        tempo_final = MPI_Wtime();
        printf("Foram gastos %3.6f segundos para calcular a matriz C", tempo_final - tempo_inicial);
    }

    return 0;
}
