# Kaska: editor/suscriptor con un esquema de tipo streams #

## Información del proyecto ##
Un sistemas editor/suscriptor con un modo de operación pull y un almacenamiento persistente de eventos, es decir, con un esquema de streaming, proporcionan una arquitectura apropiada para numerosos escenarios distribuidos. Dentro de este tipo de arquitecturas, Apache Kafka es la principal plataforma y es en el sistema en el que se centra este proyecto, cuyo nombre homenajea a dicha popular plataforma, además de servir como recordatorio de la falibilidad del software.

Esta proyecto se trata de una versión muy reducida de este complejo componente software dejando fuera muchas de sus funcionalidades como el uso de múltiples brokers con replicación y particiones, el empaquetamiento de peticiones y respuestas, el uso de claves, la persistencia de los mensajes, los grupos de consumidores, la recuperación de mensajes por timestamp, etc.

## Modo de funcionamiento ##
El broker almacena los mensajes/eventos enviados por los productores/editores a los distintos temas.

En Kafka un consumidor/suscriptor guarda a qué temas está suscrito y cuál fue el último mensaje/evento que leyó de cada uno de esos temas (el offset). Por tanto, esa parte del estado del sistema no se almacena en el servidor/broker sino en la biblioteca del cliente.

Tiene un modo de operación pull: un consumidor/suscriptor pide al broker un nuevo mensaje indicándole a qué temas está suscrito y cuál es su offset para cada uno de esos temas.

Cuando un consumidor/suscriptor se suscribe a un tema, su offset inicial será tal que solo podrá ver los mensajes que se envíen al mismo a partir de ese momento.

Un consumidor/suscriptor puede modificar su offset en un tema para poder recibir mensajes anteriores a su suscripción o volver a recibir un mensaje nuevamente.

Para permitir que un consumidor/suscriptor no tenga que estar siempre activo y pueda retomar el trabajo donde lo dejó al volver a ejecutarse, se pueden guardar de forma persistente sus offsets en el broker y recuperarlos al reiniciarse.

## Requisitos que cumple el proyecto ##
Como se ha indicado anteriormente, este proyecto cumple sólo algunas de las funcionalidades de Apache Kafka. Estas son las siguintes:

- El proyecto funciona tanto en local como en remoto.
- En cuanto a las tecnologías de comunicación usadas en el proyecto , está programado en C, y se ha utilizado sockets de tipo stream y se supone un entorno de máquinas heterogéneas.
- Se utiliza un esquema con un único proceso que actúa como broker proporcionando el desacoplamiento espacial y temporal entre los editores (productores en terminología Kafka) y los suscriptores (consumidores en terminología Kafka). Al usar un esquema de tipo streaming, el broker se encargará de almacenar los eventos.
- El broker dará un servicio concurrente basado en la creación dinámica de threads encargándose cada thread de servir todas las peticiones que llegan por una conexión.
-Un proceso editor y/o suscriptor mantendrá una conexión persistente con el broker durante toda su interacción. Dado ese posible doble rol, en el resto del documento voy a denominar clientes a este tipo de procesos.
- Como en el protocolo Kafka, el nombre de un tema es un string con un tamaño máximo de 216-1 bytes incluyendo el carácter nulo final.
- Los mensajes/eventos enviados pueden tener un contenido binario (por ejemplo, pueden corresponder a una imagen o a texto cifrado). 
- El diseño del sistema no establece ninguna limitación en el número de temas y clientes existentes en el sistema ni en el número de mensajes almacenados en el broker.
- El comportamiento es zerocopy tanto en la gestión de los nombres de tema como en el contenido de los mensajes.
- Para evitar la fragmentación en la transmisión, tanto los clientes como el broker mandan toda la información de una petición o de una respuesta, respectivamente, con un único envío.


