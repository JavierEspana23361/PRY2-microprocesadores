#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

void GeneradorTxt(int cantidad) {
    // Reservar memoria para almacenar los datos de los activos
    char **datos = malloc(cantidad * sizeof(char *));
    if (datos == NULL) {
        printf("Error al reservar memoria.\n");
        return;
    }
    for (int i = 0; i < cantidad; i++) {
        datos[i] = malloc(50 * sizeof(char));
        if (datos[i] == NULL) {
            printf("Error al reservar memoria para el activo %d.\n", i + 1);
            return;
        }
    }

    srand(time(0)); // Inicializar la semilla para los números aleatorios

    // Generar los datos en paralelo
    #pragma omp parallel
    {
        int id = omp_get_thread_num();
        unsigned int seed = time(0) + id * 37; // Semilla única para cada hilo

        #pragma omp for
        for (int i = 0; i < cantidad; i++) {
            seed = seed * 1103515245 + 12345; // Algoritmo para generar una nueva semilla
            double amount = (seed % 1001) + 1000; // Generar un número entre 1000 y 2000
            seed = seed * 1103515245 + 12345; // Nueva semilla
            double return_rate = ((seed % 100) / 1000.0) + 0.01; // Generar un número entre 0.01 y 0.1
            seed = seed * 1103515245 + 12345; // Nueva semilla
            double risk_level = ((seed % 200) / 1000.0) + 0.1; // Generar un número entre 0.1 y 0.3

            // Almacenar en el buffer local de datos
            sprintf(datos[i], "Activo%d %.2f %.2f %.2f\n", i + 1, amount, return_rate, risk_level);
        }
    }

    // Escribir todos los datos generados en el archivo
    FILE *file = fopen("datos.txt", "w");
    if (file == NULL) {
        printf("Error al abrir el archivo.\n");
        return;
    }

    fprintf(file, "%d\n", cantidad);

    for (int i = 0; i < cantidad; i++) {
        fprintf(file, "%s", datos[i]);
    }

    fclose(file);
    printf("Archivo generado con éxito.\n");

    // Liberar memoria
    for (int i = 0; i < cantidad; i++) {
        free(datos[i]);
    }
    free(datos);
}

int main() {
    int cantidad = 500;
    GeneradorTxt(cantidad);
    return 0;
}
