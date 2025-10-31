/******************************************************************************

* Materia Sistemas Operativos.
* Pontificia Universidad Javeriana.
* Docente John Corredor.
* Autor Karol Dayan Torres Vides.
* Taller 02 Fork
* Descripción: 

*******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

/* Función para verificar que la cantidad de elementos de los arreglos presentes en los ficheros es adecuada */

static void verificarN_ele(const char *path, int *arr, size_t n) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "No se pudo abrir '%s': %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < n; ++i) {
        if (fscanf(f, " %d", &arr[i]) != 1) {  // " %d" salta espacios/lineas
            fprintf(stderr, "Error: el archivo '%s' tiene menos de %zu enteros.\n", path, n);
            fclose(f);
            exit(EXIT_FAILURE);
        }
    }
    /* Verificar que no sobren enteros */
    int final;
    if (fscanf(f, " %d", &final) == 1) {
        fprintf(stderr, "Error: el archivo '%s' tiene más de %zu enteros (excede N declarado).\n", path, n);
        fclose(f);
        exit(EXIT_FAILURE);
    }
    fclose(f);
}

/* */
/* Para verificar que los N1/N2 sean válidos y no cosas como ´abc´ por ejemplo. Se usa
   strtol para pasar de string (texto) a long (numero)*/
static long parsearlong(const char *s, const char *nombre) {
    char *fin = NULL;
    errno = 0;
    long v = strtol(s, &fin, 10);
    if (errno != 0 || fin == s || *fin != '\0' || v <= 0) {
        fprintf(stderr, "Error: %s inválido: '%s'\n", nombre, s);
        exit(EXIT_FAILURE);
    }
    return v;
}

/* Función que se usará para realizar la suma entre los elementos de los arrays, se usará long para almacenar grandes cantidades. */

static long suma_array(const int *arreglo, size_t n) {
    long suma = 0;
    for (size_t i = 0; i < n; ++i) suma += arreglo[i];
    return suma;
}



/* Programa Principal */
int main(int argc, char *argv[])
{

    /* Verificar los parametros que recibe el programa principal */
    printf("Cantidad de elementos del fichero 00: %s\n", argv[1]);
    printf("Nombre primer fichero 00: %s\n", argv[2]);

    printf("Cantidad de elementos del fichero 01: %s\n", argv[3]);
    printf("Nombre primer fichero 01: %s\n", argv[4]);

    /* Verfifcar el numero de argumentos que recibe el programa principal*/
    printf("La cantidad de argumentos ingresada contando el nombre del programa fué: %d\n", argc);

    if ((argc< 5) || (argc > 5)){
       printf("El número de argumentos recibido es inválido %d\n ", argc);
       return 1;
    }
    const char *sN1      = argv[1];   /* cadena con N1 */   
    const char *sN2      = argv[3];   /* cadena con N2 */
    const char *fichero00 = argv[2];   /* cadena con nombre del fichero00 */
    const char *fichero01 = argv[4];   /* cadena con nombre del fichero01 N1 */

    /* 3) Convertir N1 y N2 a enteros  */
    long N1 = parsearlong(sN1, "N1");
    long N2 = parsearlong(sN2, "N2");

    /* Reservar Memoria con malloc() */
    int *A = (int *)malloc((size_t)N1 * sizeof(int));
    int *B = (int *)malloc((size_t)N2 * sizeof(int));
    if (!A || !B) {
    	fprintf(stderr, "Fallo al reservar memoria para A o B.\n");
    	return EXIT_FAILURE;
    }
    /* Se hace llamda de la función que nos verifica tamaños de cada uno de los array */
    verificarN_ele(fichero00, A, (size_t)N1);
    verificarN_ele(fichero01, B, (size_t)N2);

    /* Creamos los PIPE para cada resultado */
    int PA[2], PB[2], PT[2];
    if (pipe(PA) == -1 || pipe(PB) == -1 || pipe(PT) == -1) {
        perror("pipe");
        free(A); free(B);
        return EXIT_FAILURE;
    }

    /* Esta sección corresponde al 1er HIJO */
    pid_t pid1 = fork();
    if (pid1 < 0) { perror("fork primer hijo"); free(A); free(B); return EXIT_FAILURE; }

    if (pid1 == 0) {
        /* 1er HIJO, se cierran los demás PIPE */
        close(PA[0]); close(PA[1]);     
        close(PB[0]); close(PB[1]);  
        close(PT[0]);                   
        long sumaTotal = suma_array(A, (size_t)N1) + suma_array(B, (size_t)N2);
        if (write(PT[1], &sumaTotal, sizeof sumaTotal) != sizeof sumaTotal) {
            perror("write sumaTotal"); _exit(EXIT_FAILURE);
        }
        close(PT[1]);
        _exit(EXIT_SUCCESS);
    }

    /* Sección del 2do HIJO el cual crea al nieto que se encarga de sumar elemenos de array A*/
    pid_t pid2 = fork();
    if (pid2 < 0) { perror("fork segundo hijo"); free(A); free(B); return EXIT_FAILURE; }

    if (pid2 == 0) {
        /* 2do Hijo, se cierra los que no se use */
        close(PA[0]);                   
        /* Se tendrá PA[1] abierto hasta después del fork del nieto */
        close(PB[0]);                   
        close(PT[0]); close(PT[1]);     // Esto se cierra porque no se usa aquí 
        /* Se crea el nieto */
        pid_t pidg = fork();            
        if (pidg < 0) { perror("fork nieto"); _exit(EXIT_FAILURE); }

        if (pidg == 0) {
            /* Sección de code del nieto */
            // Se escribe sumaA en PA[1]
            close(PB[1]);               // Se cierra lo que no se usa
            long sumaA = suma_array(A, (size_t)N1);
            if (write(PA[1], &sumaA, sizeof sumaA) != sizeof sumaA) {
                perror("write sumaA"); _exit(EXIT_FAILURE);
            }
            close(PA[1]);
            _exit(EXIT_SUCCESS);
        }

        /* El programa se devuelve al hijo 2 */
       
        close(PA[1]);

        long sumaB = suma_array(B, (size_t)N2);
        if (write(PB[1], &sumaB, sizeof sumaB) != sizeof sumaB) {
            perror("write sumaB"); _exit(EXIT_FAILURE);
        }
        close(PB[1]);

        // Esperar al nieto para evitar errores
        int stg = 0; waitpid(pidg, &stg, 0);
        _exit(EXIT_SUCCESS);
    }

    /* Sección de code del Padre, como éste solo lee, hay que cerrar la escritura */
    
    close(PA[1]); close(PB[1]); close(PT[1]);

    long rA=0, rB=0, rT=0;
    if (read(PA[0], &rA, sizeof rA) != sizeof rA) { perror("read sumaA"); }
    if (read(PB[0], &rB, sizeof rB) != sizeof rB) { perror("read sumaB"); }
    if (read(PT[0], &rT, sizeof rT) != sizeof rT) { perror("read sumaTotal"); }

    close(PA[0]); close(PB[0]); close(PT[0]);

    /* Esperar a los dos hijos y no al nieto porque a éste lo esperó su propio padre */
    int st1=0, st2=0;
    waitpid(pid1, &st1, 0);
    waitpid(pid2, &st2, 0);

    /* Es preciso mostrar los resultados de las tres sumas */
    printf("sumaA (fichero00) = %ld\n", rA);
    printf("sumaB (fichero01) = %ld\n", rB);
    printf("sumaTotal (A+B)   = %ld\n", rT);

    /* Liberamos memoria */
    free(A); free(B);

    return 0;
}
