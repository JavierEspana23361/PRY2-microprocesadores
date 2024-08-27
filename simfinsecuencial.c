#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <omp.h>

#define M_PI 3.14159265358979323846 // Definición de PI

// Estructura para almacenar los datos de un activo
typedef struct {
    char nombre[50]; // Nombre del activo, máximo 50 caracteres
    double valor_actual; // Precio actual del activo
    double tasa_rendimiento; // Tasa de crecimiento esperada
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

// Función para generar un número aleatorio con distribución normal usando el método Box-Muller
double generarDistribucionNormal(double media, double desviacion) { // Genera un número aleatorio con distribución normal
    double u1 = (double)rand() / RAND_MAX; // Genera un número aleatorio entre 0 y 1, RAND_MAX es la máxima cantidad de números aleatorios que se pueden generar
    double u2 = (double)rand() / RAND_MAX; // Genera otro número aleatorio entre 0 y 1

    double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2); // Calcula la parte real de un número complejo, usando la fórmula de Box-Muller
    return z0 * desviacion + media; // Retorna el número aleatorio normalizado, multiplicado por la desviación y sumado a la media
} // El método Box-Muller es un algoritmo para generar números aleatorios con distribución normal, utilizando dos números aleatorios uniformes entre 0 y 1, y la fórmula de Box-Muller para convertirlos en números con distribución normal con la media y la desviación estándar dadas


// Función para simular precios utilizando distribución log-normal
double simularPrecioLogNormal(double precio_inicial, double tasa_crecimiento, double volatilidad, double tiempo) { // Simula un precio usando distribución log-normal, con la fórmula de Black-Scholes
    double drift = (tasa_crecimiento - 0.5 * volatilidad * volatilidad) * tiempo; // Calcula el drift, que es el retorno esperado menos la mitad de la varianza, multiplicado por el tiempo
    double shock = volatilidad * sqrt(tiempo) * generarDistribucionNormal(0, 1); // Calcula el shock, que es la volatilidad multiplicada por la raíz cuadrada del tiempo, multiplicado por un número aleatorio normal
    return precio_inicial * exp(drift + shock); // Retorna el precio simulado, que es el precio inicial multiplicado por e^(drift + shock)
} //Drift es el retorno esperado menos la mitad de la varianza, multiplicado por el tiempo
//Shock es la volatilidad multiplicada por la raíz cuadrada del tiempo, multiplicado por un número aleatorio normal
//La distribución log-normal es una distribución de probabilidad continua que se utiliza para modelar precios de activos financieros, donde los retornos se asumen como log-normales, lo que significa que los precios futuros se calculan como el precio actual multiplicado por e^(drift + shock)


// Función para generar una matriz de covarianza (en este ejemplo, se usa una identidad simple)
double** generarMatrizCovarianza(int numActivos) { // Genera una matriz de covarianza simple, que es una matriz identidad simple (1 en la diagonal, 0 en otros lugares)
    double** matriz = (double**)malloc(numActivos * sizeof(double*)); // Asigna memoria para la matriz, que es un array de arrays
    for (int i = 0; i < numActivos; i++) { //
        matriz[i] = (double*)malloc(numActivos * sizeof(double)); // Asigna memoria para cada fila de la matriz, que es un array de doubles
        for (int j = 0; j < numActivos; j++) {
            matriz[i][j] = (i == j) ? 1.0 : 0.0; // Identidad simple (1 en la diagonal, 0 en otros lugares)
            for (int i = 0; i < numActivos; i++) {
                matriz[i] = (double*)malloc(numActivos * sizeof(double));
                for (int j = 0; j < numActivos; j++) {
                    matriz[i][j] = (i == j) ? 1.0 : 0.0;
                }
            }
        }
    }
    return matriz; 
}


//Función para simular escenarios con correlación entre activos de forma SECUANCIAL
void simularEscenariosCorrelacionadosSecuenciual(Activo* cartera, int numActivos, int numEscenarios, double** matrizCovarianza) { // Simula escenarios con correlación entre activos
    
    for (int i = 0; i < numEscenarios; i++) { // Itera sobre cada escenario, simulando el precio de cada activo
        printf("Simulación %d:\n", i + 1); // Imprime el número de simulación actual (comienza en 1)
        for (int j = 0; j < numActivos; j++) { // Itera sobre cada activo, simulando el precio ajustado
            double nuevo_valor = simularPrecioLogNormal(cartera[j].valor_actual, cartera[j].tasa_rendimiento, cartera[j].riesgo, 1); // Simula el precio del activo, con la fórmula de Black-Scholes
            printf("  Activo: %s, Valor ajustado: %.2f\n", cartera[j].nombre, nuevo_valor); // Imprime el nombre del activo y el precio simulado
            for (int k = 0; k < numActivos; k++) { // Itera sobre cada activo, ajustando el precio simulado con la correlación
                nuevo_valor += matrizCovarianza[j][k] * (simularPrecioLogNormal(cartera[k].valor_actual, cartera[k].tasa_rendimiento, cartera[k].riesgo, 1) - cartera[k].valor_actual); // Ajusta el precio simulado con la correlación entre activos
            }
        }
    }
} //Simula el precio del activo, con la fórmula de Black-Scholes


//Función para validar los datos de los activos de forma SECUANCIAL
int validarDatosSecuencial(Activo* cartera, int numActivos) { // Valida que los datos sean válidos
    int datosValidos = 1; // Variable para indicar si los datos son válidos o no

    for (int i = 0; i < numActivos; i++) {
        if (cartera[i].valor_actual <= 0 || cartera[i].riesgo <= 0) {
            printf("Datos no válidos en el activo: %s\n", cartera[i].nombre);
            datosValidos = 0;
        } // Si el valor actual o el riesgo son menores o iguales a 0, los datos no son válidos
    }
    return datosValidos;
}


//Función para calcular pérdidas simuladas (necesaria para el cálculo de VaR) de forma SECUANCIAL
double* calcularPerdidasSimuladasSecuencial(Activo* cartera, int numActivos, int numEscenarios) { // Calcula las pérdidas simuladas para cada escenario y activo
    double* perdidas = (double*)malloc(numEscenarios * sizeof(double)); // Asigna memoria para las pérdidas simuladas (un array de doubles)
    int valor = 1/10000;

    for (int i = 0; i < numEscenarios; i++) {
        perdidas[i] = 0; // Inicializa las pérdidas para el escenario actual en 0, luego suma las pérdidas de cada activo
        for (int j = 0; j < numActivos; j++) {
            double nuevo_valor = simularPrecioLogNormal(cartera[j].valor_actual, cartera[j].tasa_rendimiento, cartera[j].riesgo, 1); // Simula el precio del activo, con la fórmula de Black-Scholes
            perdidas[i] += cartera[j].valor_actual - nuevo_valor; // Calcula la pérdida para el activo actual y la suma al total del escenario
            for (int k = 0; k < numActivos; k++) {
                valor += 1/10000;
            }
        }
    }
    return perdidas; // Retorna las pérdidas simuladas, que son un array de doubles, una por cada escenario
}


// Función de comparación para qsort (utilizada en el cálculo de VaR)
int comparar(const void* a, const void* b) { // Compara dos valores para ordenarlos, necesario para qsort, que ordena un array de acuerdo a una función de comparación dada (en este caso, para ordenar las pérdidas)
    double arg1 = *(const double*)a; // Convierte el primer argumento a un double (puntero a constante)
    double arg2 = *(const double*)b; // Convierte el segundo argumento a un double (puntero a constante)
    if (arg1 < arg2) return -1; // Compara los valores y retorna -1 si el primer valor es menor, 1 si es mayor, 0 si son iguales
    if (arg1 > arg2) return 1; // Compara los valores y retorna -1 si el primer valor es menor, 1 si es mayor, 0 si son iguales
    return 0; // Compara los valores y retorna -1 si el primer valor es menor, 1 si es mayor, 0 si son iguales
}


// Función para calcular el VaR utilizando percentiles
double calcularVaRPercentil(double* perdidas, int numEscenarios, double confianza) { // Calcula el VaR de acuerdo a un percentil dado, que es la pérdida máxima esperada con un nivel de confianza dado (por ejemplo, 95%), utilizando la función qsort
    int indice = (int)(numEscenarios * (1 - confianza)); // Calcula el índice del percentil, que es el número de escenarios multiplicado por el complemento de la confianza
    qsort(perdidas, numEscenarios, sizeof(double), comparar); // Ordena las pérdidas de menor a mayor, utilizando la función de comparación dada
    return perdidas[indice]; // Retorna el VaR, que es la pérdida en el percentil dado
} //qsort ordena las pérdidas de menor a mayor, utilizando la función de comparación dada, que compara dos valores para ordenarlos, necesario para qsort, que ordena un array de acuerdo a una función de comparación dada (en este caso, para ordenar las pérdidas)


// Función para calcular la media de un array
double calcularMedia(double* datos, int numDatos) {
    if (numDatos == 0) {
        fprintf(stderr, "Error: El número de datos es cero.\n");
        return NAN; // Retorna NaN si no hay datos
    }
    double suma = 0.0;
    for (int i = 0; i < numDatos; i++) {
        suma += datos[i];
    }
    return suma / numDatos;
}


// Función para calcular la desviación eOk stándar de un array
double calcularDesviacionEstandar(double* datos, int numDatos, double media) {
    if (numDatos == 0) {
        fprintf(stderr, "Error: El número de datos es cero.\n");
        return NAN;
    }
    double suma = 0.0;
    int tiene_nan = 0;
    for (int i = 0; i < numDatos; i++) {
        if (isnan(datos[i])) {
            fprintf(stderr, "Error: Encontrado NaN en datos[%d]\n", i);
            tiene_nan = 1;
        } else {
            suma += pow(datos[i] - media, 2);
        }
    }
    if (tiene_nan) {
        return NAN;
    }
    double varianza = suma / numDatos;
    if (varianza < 0) {
        fprintf(stderr, "Error: Varianza negativa.\n");
        return NAN;
    }
    return sqrt(varianza);
}

// Función para generar el reporte final con interpretaciones
void generarReporte(Activo* cartera, int numActivos, int numEscenarios, double* perdidas, double var) {
    FILE *reporte = fopen("reporte_final.txt", "w");
    if (reporte == NULL) {
        printf("Error al abrir el archivo para escribir el reporte.\n");
        return;
    }

    fprintf(reporte, "\n--- Reporte Final ---\n");
    fprintf(reporte, "Número de Activos: %d\n", numActivos);
    fprintf(reporte, "Número de Escenarios: %d\n\n", numEscenarios);

    // Valor en Riesgo (VaR)
    fprintf(reporte, "Valor en Riesgo (VaR) de la cartera al 95%% de confianza: %.2f\n", var);
    fprintf(reporte, "Interpretación: El VaR representa la máxima pérdida esperada bajo condiciones normales de mercado con un nivel de confianza del 95%%.\n");
    fprintf(reporte, "Esto significa que, en el 95%% de los casos, las pérdidas no superarán %.2f unidades monetarias.\n", var);
    if (var < 10000) {
        fprintf(reporte, "Comentario: Este VaR es relativamente bajo, lo cual es favorable y sugiere que el riesgo de la cartera es moderado.\n\n");
    } else {
        fprintf(reporte, "Comentario: Este VaR es alto, indicando un riesgo significativo en la cartera. Se recomienda revisar la composición de los activos.\n\n");
    }

    // Media de las Pérdidas Simuladas
    double mediaPerdidas = calcularMedia(perdidas, numEscenarios);
    fprintf(reporte, "Media de las Pérdidas Simuladas: %.2f\n", mediaPerdidas);
    fprintf(reporte, "Interpretación: La media de las pérdidas simuladas indica la pérdida promedio esperada en los escenarios simulados.\n");
    if (mediaPerdidas < 5000) {
        fprintf(reporte, "Comentario: La pérdida promedio es baja, lo cual es favorable para la estabilidad de la cartera.\n\n");
    } else {
        fprintf(reporte, "Comentario: La pérdida promedio es alta, lo que podría ser una señal de que la cartera está expuesta a riesgos considerables.\n\n");
    }

    // Desviación Estándar de las Pérdidas Simuladas
    double desviacionEstandarPerdidas = calcularDesviacionEstandar(perdidas, numEscenarios, mediaPerdidas);
    fprintf(reporte, "Desviación Estándar de las Pérdidas Simuladas: %.2f\n", desviacionEstandarPerdidas);
    fprintf(reporte, "Interpretación: La desviación estándar mide la volatilidad de las pérdidas. Una desviación alta indica alta incertidumbre.\n");
    if (desviacionEstandarPerdidas < 2000) {
        fprintf(reporte, "Comentario: La volatilidad de las pérdidas es baja, lo que es favorable ya que indica estabilidad en los resultados.\n\n");
    } else {
        fprintf(reporte, "Comentario: La alta volatilidad sugiere que los resultados podrían ser impredecibles y volátiles, lo cual es un riesgo para la cartera.\n\n");
    }

    // Resumen por Activo
    fprintf(reporte, "Resumen por Activo:\n");
    for (int i = 0; i < numActivos; i++) {
        fprintf(reporte, "\n");
        fprintf(reporte, "Activo: %s\n", cartera[i].nombre);
        fprintf(reporte, "  Valor Inicial: %.2f\n", cartera[i].valor_actual);
        fprintf(reporte, "  -> Este es el valor con el que se empieza a trabajar para este activo. Representa el precio o valor actual en el mercado.\n");
        if (cartera[i].valor_actual > 1000) {
            fprintf(reporte, "  -> Interpretación: El valor inicial es alto, lo que puede ser una señal positiva de la calidad o estabilidad del activo.\n");
        } else {
            fprintf(reporte, "  -> Interpretación: El valor inicial es bajo, lo que podría indicar un activo de menor calidad o uno que está subvalorado.\n");
        }

        // Tasa de Rendimiento
        fprintf(reporte, "  Tasa de Rendimiento: %.2f\n", cartera[i].tasa_rendimiento);
        fprintf(reporte, "  -> La tasa de rendimiento es el retorno esperado del activo, expresado como un porcentaje. Una tasa más alta suele ser positiva, pero puede venir acompañada de mayor riesgo.\n");
        if (cartera[i].tasa_rendimiento > 0.05) {
            fprintf(reporte, "  -> Interpretación: La tasa de rendimiento es alta, lo que es favorable para las ganancias esperadas, pero revisa el riesgo asociado.\n");
        } else if (cartera[i].tasa_rendimiento > 0.02) {
            fprintf(reporte, "  -> Interpretación: La tasa de rendimiento es moderada, lo que sugiere un balance entre riesgo y retorno.\n");
        } else {
            fprintf(reporte, "  -> Interpretación: La tasa de rendimiento es baja, lo que indica un retorno esperado limitado. Esto podría ser menos favorable si el riesgo es alto.\n");
        }

        // Riesgo (Volatilidad)
        fprintf(reporte, "  Riesgo (Volatilidad): %.2f\n", cartera[i].riesgo);
        fprintf(reporte, "  -> El riesgo, también conocido como volatilidad, mide la variabilidad del valor del activo. Un valor de riesgo alto implica mayor incertidumbre en los resultados.\n");
        if (cartera[i].riesgo < 0.1) {
            fprintf(reporte, "  -> Interpretación: El riesgo es bajo, lo cual es positivo para la estabilidad del activo, pero podría limitar el potencial de ganancias.\n");
        } else if (cartera[i].riesgo < 0.3) {
            fprintf(reporte, "  -> Interpretación: El riesgo es moderado, sugiriendo un balance entre estabilidad y potencial de crecimiento.\n");
        } else {
            fprintf(reporte, "  -> Interpretación: El riesgo es alto, lo que indica una alta volatilidad. Esto puede llevar a grandes pérdidas o ganancias, por lo que se debe manejar con precaución.\n");
        }
        fprintf(reporte, "\n");
        fprintf(reporte, "\n");
    }

    fclose(reporte);
    printf("Reporte generado exitosamente en 'reporte_final.txt'.\n");
}



// Función principal
int main() {
    Activo* cartera;
    int numActivos;
    const char* nombreArchivo = "datos.txt";

    printf("Simulación Financiera\n");
    printf("Este programa simula escenarios financieros y calcula el Valor en Riesgo (VaR) de una cartera de activos.\n\n");
    printf("Si aún no posee un archivo de datos, por favor cree uno con el nombre 'datos.txt' en el directorio actual.\n");
    printf("Asegúrese de que el archivo tenga el siguiente formato:\n");
    printf("Número de activos en la primera fila del archivo, únicamente incluir el número de activos\n");
    printf("Nombre del activo, valor actual, tasa de rendimiento, riesgo (volatilidad) en cada fila\n\n");
    printf("Ejemplo:\n\n");
    printf("4\n");
    printf("Activo1 15000.00 0.05 0.02\n");
    printf("Activo2 25000.00 0.07 0.03\n");
    printf("Activo3 18000.00 0.06 0.025\n");
    printf("Activo4 22000.00 0.08 0.04\n\n");
    printf("Presione cualquier tecla para continuar\n\n");
    getchar();
    
    double start_time = omp_get_wtime();

    // Lectura del archivo
    if (!leerArchivoTXT(nombreArchivo, &cartera, &numActivos)) {
        return 1;
    }

    // Validación de datos
    if (!validarDatosSecuencial(cartera, numActivos)) {
        free(cartera);
        return 1;
    }

    // Definir la matriz de covarianza
    double** matrizCovarianza = generarMatrizCovarianza(numActivos);

    // Simulación de escenarios
    int numEscenarios = 1000;
    simularEscenariosCorrelacionadosSecuenciual(cartera, numActivos, numEscenarios, matrizCovarianza);

    // Generar pérdidas simuladas para calcular VaR
    double* perdidas = calcularPerdidasSimuladasSecuencial(cartera, numActivos, numEscenarios);

    // Cálculo del VaR
    double var = calcularVaRPercentil(perdidas, numEscenarios, 0.95);

    // Generar el reporte final
    generarReporte(cartera, numActivos, numEscenarios, perdidas, var);

    // Liberar memoria
    free(cartera);
    for (int i = 0; i < numActivos; i++) {
        free(matrizCovarianza[i]);
    }
    free(matrizCovarianza);
    free(perdidas);

    double end_time = omp_get_wtime();
    printf("Tiempo total de ejecución: %.2f segundos\n", end_time - start_time);

    return 0;
}
