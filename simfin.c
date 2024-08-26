#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <omp.h>

#define M_PI 3.14159265358979323846 // Definición de PI

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
    int datosValidos = 1;

    #pragma omp parallel for
    for (int i = 0; i < numActivos; i++) {
        if (cartera[i].valor_actual <= 0 || cartera[i].riesgo <= 0) {
            printf("Datos no válidos en el activo: %s\n", cartera[i].nombre);
            datosValidos = 0;
        }
    }
    return datosValidos;
}

// Función para generar un número aleatorio con distribución normal usando el método Box-Muller
double generarDistribucionNormal(double media, double desviacion) {
    double u1 = (double)rand() / RAND_MAX;
    double u2 = (double)rand() / RAND_MAX;

    double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    return z0 * desviacion + media;
}

// Función para simular precios utilizando distribución log-normal
double simularPrecioLogNormal(double precio_inicial, double tasa_crecimiento, double volatilidad, double tiempo) {
    double drift = (tasa_crecimiento - 0.5 * volatilidad * volatilidad) * tiempo;
    double shock = volatilidad * sqrt(tiempo) * generarDistribucionNormal(0, 1);
    return precio_inicial * exp(drift + shock);
}

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
void simularEscenariosCorrelacionados(Activo* cartera, int numActivos, int numEscenarios, double** matrizCovarianza) {
    #pragma omp parallel for
    for (int i = 0; i < numEscenarios; i++) {
        printf("Simulación %d:\n", i + 1);
        for (int j = 0; j < numActivos; j++) {
            double nuevo_valor = simularPrecioLogNormal(cartera[j].valor_actual, cartera[j].tasa_rendimiento, cartera[j].riesgo, 1);
            printf("  Activo: %s, Valor ajustado: %.2f\n", cartera[j].nombre, nuevo_valor);
        }
    }
}

// Función para calcular pérdidas simuladas (necesaria para el cálculo de VaR)
double* calcularPerdidasSimuladas(Activo* cartera, int numActivos, int numEscenarios) {
    double* perdidas = (double*)malloc(numEscenarios * sizeof(double));

    #pragma omp parallel for
    for (int i = 0; i < numEscenarios; i++) {
        perdidas[i] = 0;
        for (int j = 0; j < numActivos; j++) {
            double nuevo_valor = simularPrecioLogNormal(cartera[j].valor_actual, cartera[j].tasa_rendimiento, cartera[j].riesgo, 1);
            #pragma omp atomic
            perdidas[i] += cartera[j].valor_actual - nuevo_valor;
        }
    }
    return perdidas;
}

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

// Función para calcular la media de un array
double calcularMedia(double* datos, int numDatos) {
    double suma = 0.0;
    for (int i = 0; i < numDatos; i++) {
        suma += datos[i];
    }
    return suma / numDatos;
}

// Función para calcular la desviación estándar de un array
double calcularDesviacionEstandar(double* datos, int numDatos, double media) {
    double suma = 0.0;
    for (int i = 0; i < numDatos; i++) {
        suma += pow(datos[i] - media, 2);
    }
    return sqrt(suma / numDatos);
}

// Función para generar el reporte final con interpretaciones
void generarReporte(Activo* cartera, int numActivos, int numEscenarios, double* perdidas, double var) {
    printf("\n--- Reporte Final ---\n");
    printf("Número de Activos: %d\n", numActivos);
    printf("Número de Escenarios: %d\n\n", numEscenarios);

    // Valor en Riesgo (VaR)
    printf("Valor en Riesgo (VaR) de la cartera al 95%% de confianza: %.2f\n", var);
    printf("Interpretación: El VaR representa la máxima pérdida esperada bajo condiciones normales de mercado con un nivel de confianza del 95%%.\n");
    printf("Esto significa que, en el 95%% de los casos, las pérdidas no superarán %.2f unidades monetarias.\n", var);
    if (var < 10000) {
        printf("Comentario: Este VaR es relativamente bajo, lo cual es favorable y sugiere que el riesgo de la cartera es moderado.\n\n");
    } else {
        printf("Comentario: Este VaR es alto, indicando un riesgo significativo en la cartera. Se recomienda revisar la composición de los activos.\n\n");
    }

    // Media de las Pérdidas Simuladas
    double mediaPerdidas = calcularMedia(perdidas, numEscenarios);
    printf("Media de las Pérdidas Simuladas: %.2f\n", mediaPerdidas);
    printf("Interpretación: La media de las pérdidas simuladas indica la pérdida promedio esperada en los escenarios simulados.\n");
    if (mediaPerdidas < 5000) {
        printf("Comentario: La pérdida promedio es baja, lo cual es favorable para la estabilidad de la cartera.\n\n");
    } else {
        printf("Comentario: La pérdida promedio es alta, lo que podría ser una señal de que la cartera está expuesta a riesgos considerables.\n\n");
    }

    // Desviación Estándar de las Pérdidas Simuladas
    double desviacionEstandarPerdidas = calcularDesviacionEstandar(perdidas, numEscenarios, mediaPerdidas);
    printf("Desviación Estándar de las Pérdidas Simuladas: %.2f\n", desviacionEstandarPerdidas);
    printf("Interpretación: La desviación estándar mide la volatilidad de las pérdidas. Una desviación alta indica alta incertidumbre.\n");
    if (desviacionEstandarPerdidas < 2000) {
        printf("Comentario: La volatilidad de las pérdidas es baja, lo que es favorable ya que indica estabilidad en los resultados.\n\n");
    } else {
        printf("Comentario: La alta volatilidad sugiere que los resultados podrían ser impredecibles y volátiles, lo cual es un riesgo para la cartera.\n\n");
    }

    // Resumen por Activo
    printf("Resumen por Activo:\n");
    for (int i = 0; i < numActivos; i++) {
        printf("Activo: %s\n", cartera[i].nombre);
        printf("  Valor Inicial: %.2f\n", cartera[i].valor_actual);
        // Aquí se puede incluir el valor final promedio o alguna otra métrica relevante
    }

    printf("\nReporte generado correctamente.\n");
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
    generarReporte(cartera, numActivos, numEscenarios, perdidas, var);

    free(cartera); // Liberar la memoria asignada
    return 0;
}
