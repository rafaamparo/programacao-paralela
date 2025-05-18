#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <math.h>

int primo (long int n) { /* mpi_primos.c  */
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


	// argc é a quantidade de argumentos passados na linha de comando
	// argv é um vetor de strings que contém os argumentos passados
	if (argc < 2) {
        	printf("Valor inválido! Entre com um valor do maior inteiro\n");
       	 	return 0;
    	} else {
			// strtol converte uma string em um número inteiro
			// argv[1] é o número passado como argumento

        	n = strtol(argv[1], (char **) NULL, 10);

			// Verifica se o número é menor que 2
			// Se for, imprime uma mensagem de erro e retorna 0
			if (n < 2 ) {
				printf("Valor inválido! Entre com um valor maior que 2\n");
				return 0;
			}
       	}

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &meu_ranque);
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);	
    t_inicial = MPI_Wtime();

    inicio = 3 + meu_ranque*2;
    salto = num_procs*2;

	// Exemplo de como funciona:
	// Rank 0: Coemça em 3 e incremente de 8 em 8
	// Rank 1: Começa em 5 e incremente de 8 em 8
	// Rank 2: Começa em 7 e incremente de 8 em 8
	// Rank 3: Começa em 9 e incremente de 8 em 8

	// Rank 0: 3, 11, 19, 27, ...
	// Rank 1: 5, 13, 21, 29, ...
	// Rank 2: 7, 15, 23, 31, ...
	// Rank 3: 9, 17, 25, 33, ...


	for (i = inicio; i <= n; i += salto) 
	{	
		if(primo(i) == 1) cont++;
	}
		
	if(num_procs > 1) {
		// Sincronização para garantir que o processo 0 tenha postado o MPI_Irecv
		// antes que outros processos chamem MPI_Rsend
		MPI_Barrier(MPI_COMM_WORLD);
		
		if (meu_ranque == 0) {
			total = cont; // Inicializa com os primos encontrados pelo processo 0
			int cont_outros;
			MPI_Request requests[num_procs-1];
			MPI_Status statuses[num_procs-1];
			
			// Inicia todas as recepções não-bloqueantes
			for (int origem = 1; origem < num_procs; origem++) {
				// MPI_Irecv inicia a recepção e retorna imediatamente
				MPI_Irecv(&cont_outros + (origem-1), 1, MPI_INT, origem, 0, 
					MPI_COMM_WORLD, &requests[origem-1]);
			}
			
			// Espera todas as recepções terminarem
			MPI_Waitall(num_procs-1, requests, statuses);
			
			// Soma todas as contagens recebidas
			for (int origem = 1; origem < num_procs; origem++) {
				total += *((&cont_outros) + (origem-1));
			}
		} else {
			// Usando MPI_Rsend que exige que o receptor já tenha postado o MPI_Irecv
			MPI_Rsend(&cont, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
			// Não é necessário MPI_Wait pois MPI_Rsend é bloqueante
		}
	} else {
		total = cont;
	}
	
	t_final = MPI_Wtime();

	if (meu_ranque == 0) {
        total += 1;    /* Acrescenta o dois, que também é primo */
		printf("Quant. de primos entre 1 e %ld: %d \n", n, total);
		printf("Tempo de execucao: %0.9f s\n", t_final - t_inicial);	 
	}
	MPI_Finalize();
	return(0);
}