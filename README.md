# Nombres: Pablo Mansilla Hernández (9129)

# Descripción del Codigo:
En este codigo, se utiliza FreeRTOS para poder hacer uso de varias tareas concurrentes, en este sistema se controla el parpadeo de un LED en dos modos distintos, modo lento y 
modo rapido, ademas de que tambien se monitorea constantemente un boton, se realizan lecturas del ADC, y se utiliza un estado de espera Idle, para asi mostrar el funcionamiento
del planificador de tareas. Ademas de esto, se añadio un temporizador el cual reinicia el ciclo, de este modo, si no se presiona el boton durante en tiempo de espera, se regresa a la
primer tarea para repetir de forma indefinida el ciclo


# Conclusiones:
En esta practica pude comprender el funcionamiento basico de FreeRTOS, junto con funciones como lo son la creacion de tareas, la asignación de prioridades, los retardos, las
suspenciones, y la reanudación de tareas. Ademas de esto, pude ver como es que dentro de un sistema embebido, pueden haber varias tareas trabajando de forma paralela


# Preguntas:

1. ¿Por qué la variable g_sensorActivo debe declararse como volatile? ¿Qué ocurre si se omite?
   R= Porque es utilizada por varias tareas. La palabra "volatile" obliga al compilador a leer siempre el valor actualizado desde memoria. Si se omitiera, entonces el compilador
   podria optimizar el acceso, y hacer que alguna de las tareas trabaje con un valor antiguo, provocando errores en el funcionamiento.

2. ¿En qué momento aparece el mensaje [IDLE]? ¿Cuál es el estado de las tareas?
   R= Aparece cuando el monitor entra en modo espera y no hay tareas listas para ejecutarse.

3. ¿Qué diferencia existe entre vTaskDelay() y vTaskDelayUntil()? ¿Cuál es más apropiada aquí?
   R= "vTaskDelay()" espera un tiempo contado desde el momento en que se llama. "vTaskDelayUntil()" en cambio, mantiene una ejecución periódica exacta.
   En este caso, "vTaskDelayUntil()" es la mas coveniente

4. ¿Por qué vTaskLedRapido tiene menor prioridad que vTaskMonitor? ¿Qué ocurriría si se invierten?
   R= Porque el monitor es el que controla todo el funcionamiento del sistema, si se llegaran a invertir las prioridades el LED podría recibir más tiempo de CPU que la tarea
   de control

5. ¿Qué riesgo existe al leer una variable volatile desde dos tareas sin protección? ¿Qué es una sección crítica?
   R= Podria terminar ocurriendo una condición de carrera, causando que dos tareas accedan a la misma variable al mismo tiempo
