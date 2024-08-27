# Generador de TXT
# 3 (cantidad de activos)
# Activo1 1000.0 0.05 0.2
# Activo2 1500.0 0.03 0.15
# Activo3 1200.0 0.07 0.25

import random

def GeneradorTxt(cantidad):
    try:
        with open("datos.txt", "w") as f:
            f.write(str(cantidad) + "\n")
            for i in range(cantidad):
                amount = random.uniform(1000, 2000)
                return_rate = random.uniform(0.01, 0.1)
                risk_level = random.uniform(0.1, 0.3)
                f.write(f"Activo{i+1} {amount:.2f} {return_rate:.2f} {risk_level:.2f}\n")
        print("Archivo generado con exito")
    except Exception as e:
        print(f"Error al generar el archivo: {e}")

GeneradorTxt(500)
