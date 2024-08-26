#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <omp.h>


// Estructura para almacenar los datos de un activo
typedef struct {
    char nombre[50];
    double valor_actual;
    double tasa_rendimiento;
    double riesgo; // Volatilidad
} Activo;

// Función para leer el archivo TXT
int leerArchivoTXT(const char* nombreArchivo, Activo** cartera, int* numActivos) {
    FILE* archivo = fopen(nombreArchivo, "r");
    if (!archivo) {
        printf("No se pudo abrir el archivo: %s\n", nombreArchivo);
        return 0;
    }

    fscanf(archivo, "%d", numActivos);
    *cartera = (Activo*)malloc((*numActivos) * sizeof(Activo));
    
    for (int i = 0; i < *numActivos; i++) {
        fscanf(archivo, "%s %lf %lf %lf", (*cartera)[i].nombre, &(*cartera)[i].valor_actual, &(*cartera)[i].tasa_rendimiento, &(*cartera)[i].riesgo);
    }

    fclose(archivo);
    return 1;
}

// Función para validar los datos de los activos
int validarDatos(Activo* cartera, int numActivos) {
    #pragma omp parallel for
    for (int i = 0; i < numActivos; i++) {
        if (cartera[i].valor_actual <= 0 || cartera[i].riesgo <= 0) {
            printf("Datos no válidos en el activo: %s\n", cartera[i].nombre);
            return 0;
        }
    }
    return 1;
}


// Función para generar un número aleatorio con distribución normal
//Aun no funciona, las funciones de gsl no están definidas, porque no funcionan
/*
double generarDistribucionNormal(double media, double desviacion, gsl_rng* r) {
    return gsl_ran_gaussian(r, desviacion) + media;
} */


// Función para simular precios utilizando distribución log-normal
//Aun no funciona, las funciones de gsl no están definidas, porque no funcionan
/*
double simularPrecioLogNormal(double precio_inicial, double tasa_crecimiento, double volatilidad, double tiempo, gsl_rng* r) {
    double drift = (tasa_crecimiento - 0.5 * volatilidad * volatilidad) * tiempo;
    double shock = volatilidad * sqrt(tiempo) * generarDistribucionNormal(0, 1, r);
    return precio_inicial * exp(drift + shock);
} */

// Función para generar una matriz de covarianza (en este ejemplo, se usa una identidad simple)
double** generarMatrizCovarianza(int numActivos) {
    double** matriz = (double**)malloc(numActivos * sizeof(double*));
    for (int i = 0; i < numActivos; i++) {
        matriz[i] = (double*)malloc(numActivos * sizeof(double));
        for (int j = 0; j < numActivos; j++) {
            matriz[i][j] = (i == j) ? 1.0 : 0.0; // Identidad simple
        }
    }
    return matriz;
}


// Función para simular escenarios con correlación entre activos
//Aun no funciona, las funciones de gsl no están definidas, porque no funcionan
/*
void simularEscenariosCorrelacionados(Activo* cartera, int numActivos, int numEscenarios, double** matrizCovarianza) {
    #pragma omp parallel for
    for (int i = 0; i < numEscenarios; i++) {
        gsl_rng * r;
        const gsl_rng_type * T;
        gsl_rng_env_setup();
        T = gsl_rng_default;
        r = gsl_rng_alloc(T);

        printf("Simulación %d:\n", i + 1);
        for (int j = 0; j < numActivos; j++) {
            double nuevo_valor = simularPrecioLogNormal(cartera[j].valor_actual, cartera[j].tasa_rendimiento, cartera[j].riesgo, 1, r);
            printf("  Activo: %s, Valor ajustado: %.2f\n", cartera[j].nombre, nuevo_valor);
        }

        gsl_rng_free(r);
    }
} */


// Función para calcular pérdidas simuladas (necesaria para el cálculo de VaR)
//Aun no funciona, las funciones de gsl no están definidas, porque no funcionan
/*
double* calcularPerdidasSimuladas(Activo* cartera, int numActivos, int numEscenarios) {
    double* perdidas = (double*)malloc(numEscenarios * sizeof(double));

    #pragma omp parallel for
    for (int i = 0; i < numEscenarios; i++) {
        gsl_rng * r;
        const gsl_rng_type * T;
        gsl_rng_env_setup();
        T = gsl_rng_default;
        r = gsl_rng_alloc(T);

        perdidas[i] = 0;
        for (int j = 0; j < numActivos; j++) {
            double nuevo_valor = simularPrecioLogNormal(cartera[j].valor_actual, cartera[j].tasa_rendimiento, cartera[j].riesgo, 1, r);
            #pragma omp atomic
            perdidas[i] += cartera[j].valor_actual - nuevo_valor;
        }

        gsl_rng_free(r);
    }
    return perdidas;
} */


// Función de comparación para qsort (utilizada en el cálculo de VaR)
int comparar(const void* a, const void* b) {
    double arg1 = *(const double*)a;
    double arg2 = *(const double*)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

// Función para calcular el VaR utilizando percentiles
double calcularVaRPercentil(double* perdidas, int numEscenarios, double confianza) {
    int indice = (int)(numEscenarios * (1 - confianza));
    qsort(perdidas, numEscenarios, sizeof(double), comparar);
    return perdidas[indice];
}

// Función para generar el reporte final
void generarReporte(Activo* cartera, int numActivos, int numEscenarios) {
    printf("\n--- Reporte Final ---\n");
    printf("Número de Activos: %d\n", numActivos);
    printf("Número de Escenarios: %d\n", numEscenarios);
    printf("Reporte generado correctamente.\n");
}

// Función principal
int main() {
    Activo* cartera;
    int numActivos;
    const char* nombreArchivo = "datos.txt";

    // Lectura del archivo
    if (!leerArchivoTXT(nombreArchivo, &cartera, &numActivos)) {
        return 1;
    }

    // Validación de datos
    if (!validarDatos(cartera, numActivos)) {
        free(cartera);
        return 1;
    }

    // Definir la matriz de covarianza (para la correlación entre activos)
    double** matrizCovarianza = generarMatrizCovarianza(numActivos);

    // Simulación de escenarios usando la nueva función
    int numEscenarios = 1000; // Número de escenarios a simular
    simularEscenariosCorrelacionados(cartera, numActivos, numEscenarios, matrizCovarianza);

    // Generar pérdidas simuladas para calcular VaR
    double* perdidas = calcularPerdidasSimuladas(cartera, numActivos, numEscenarios);

    // Cálculo del VaR usando la función mejorada
    double var = calcularVaRPercentil(perdidas, numEscenarios, 0.95);
    printf("Valor en Riesgo (VaR) de la cartera: %.2f\n", var);

    // Generación de reporte con las nuevas funciones
    generarReporte(cartera, numActivos, numEscenarios);

    free(cartera); // Liberar la memoria asignada
    return 0;
}
