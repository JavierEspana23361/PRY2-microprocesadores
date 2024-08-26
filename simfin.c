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

    // Lee el número de activos
    if (fscanf(archivo, "%d", numActivos) != 1) {
        printf("Error al leer el número de activos.\n");
        fclose(archivo);
        return 0;
    }

    *cartera = (Activo*)malloc((*numActivos) * sizeof(Activo));
    if (*cartera == NULL) {
        printf("Error al asignar memoria para la cartera.\n");
        fclose(archivo);
        return 0;
    }

    // Lee cada activo
    for (int i = 0; i < *numActivos; i++) {
        if (fscanf(archivo, "%s %lf %lf %lf", (*cartera)[i].nombre, &(*cartera)[i].valor_actual, &(*cartera)[i].tasa_rendimiento, &(*cartera)[i].riesgo) != 4) {
            printf("Error al leer los datos del activo %d.\n", i + 1);
            free(*cartera);
            fclose(archivo);
            return 0;
        }
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
        
        // Valor Inicial
        printf("  Valor Inicial: %.2f\n", cartera[i].valor_actual);
        printf("  -> Este es el valor con el que se empieza a trabajar para este activo. Representa el precio o valor actual en el mercado.\n");
        if (cartera[i].valor_actual > 1000) {
            printf("  -> Interpretación: El valor inicial es alto, lo que puede ser una señal positiva de la calidad o estabilidad del activo.\n");
        } else {
            printf("  -> Interpretación: El valor inicial es bajo, lo que podría indicar un activo de menor calidad o uno que está subvalorado.\n");
        }
        
        // Tasa de Rendimiento
        printf("  Tasa de Rendimiento: %.2f\n", cartera[i].tasa_rendimiento);
        printf("  -> La tasa de rendimiento es el retorno esperado del activo, expresado como un porcentaje. Una tasa más alta suele ser positiva, pero puede venir acompañada de mayor riesgo.\n");
        if (cartera[i].tasa_rendimiento > 0.05) {
            printf("  -> Interpretación: La tasa de rendimiento es alta, lo que es favorable para las ganancias esperadas, pero revisa el riesgo asociado.\n");
        } else if (cartera[i].tasa_rendimiento > 0.02) {
            printf("  -> Interpretación: La tasa de rendimiento es moderada, lo que sugiere un balance entre riesgo y retorno.\n");
        } else {
            printf("  -> Interpretación: La tasa de rendimiento es baja, lo que indica un retorno esperado limitado. Esto podría ser menos favorable si el riesgo es alto.\n");
        }

        // Riesgo (Volatilidad)
        printf("  Riesgo (Volatilidad): %.2f\n", cartera[i].riesgo);
        printf("  -> El riesgo, también conocido como volatilidad, mide la variabilidad del valor del activo. Un valor de riesgo alto implica mayor incertidumbre en los resultados.\n");
        if (cartera[i].riesgo < 0.1) {
            printf("  -> Interpretación: El riesgo es bajo, lo cual es positivo para la estabilidad del activo, pero podría limitar el potencial de ganancias.\n");
        } else if (cartera[i].riesgo < 0.3) {
            printf("  -> Interpretación: El riesgo es moderado, sugiriendo un balance entre estabilidad y potencial de crecimiento.\n");
        } else {
            printf("  -> Interpretación: El riesgo es alto, lo que indica una alta volatilidad. Esto puede llevar a grandes pérdidas o ganancias, por lo que se debe manejar con precaución.\n");
        }
        
        printf("\n");
    }


    printf("\nReporte generado correctamente.\n");
}



// Función principal
int main() {
    Activo* cartera;
    int numActivos;
    const char* nombreArchivo = "datos.txt";

    printf("Simulación Financiera\n");
    printf("Este programa simula escenarios financieros y calcula el Valor en Riesgo (VaR) de una cartera de activos.\n\n");
    printf("si aun no posee un archivo de datos, por favor cree uno con el nombre 'datos.txt' en el directorio actual.\n");
    printf("Asegurese de que el archivo tenga el siguiente formato:\n");
    printf("Numero de activos en primera fila del archivo, únicamente incluir el número de activos\n");
    printf("Nombre del activo, valor actual, tasa de rendimiento, riesgo (volatilidad) en cada fila\n\n");
    printf("Ejemplo:\n\n");
    printf("1\n");
    printf("Activo1 100.0 0.05 0.1\n\n");
    printf("presione cualquier tecla para continuar\n\n");
    getchar();



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

    // Liberar memoria
    free(cartera); // Liberar la memoria asignada
    for (int i = 0; i < numActivos; i++) {
        free(matrizCovarianza[i]);
    }
    free(matrizCovarianza);
    free(perdidas);

    return 0;
}
