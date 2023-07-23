## Kaska: editor/suscriptor con un esquema de tipo streams ##
Se trata de un proyecto práctico de carácter individual cuyo plazo de entrega se extiende hasta el final del 2 de junio en la convocatoria ordinaria y hasta el final del 5 de julio en la extraordinaria. La práctica se puede desarrollar en cualquier sistema Linux que tenga instalado el entorno de compilación de C. Puede usarse una máquina personal o cualquiera del conjunto de 4 máquinas asociadas al nombre triqui.fi.upm.es. gestionadas por el centro de cálculo de la escuela. En cualquier caso, como se explicará más adelante, hay que entregar la práctica en triqui.fi.upm.es.
AVISO IMPORTANTE
Como parte de la evaluación de la práctica, se realizará un control de plagio aplicando la normativa vigente para tal circunstancia tanto a los alumnos que han entregado una práctica que no han desarrollado como a los que han dejado su práctica a otros alumnos. Para evitar este tipo de incidencias desagradables para todos, querría que tuvierais en cuenta este par de consideraciones:

Para aquellos que se sientan tentados de pedir prestada la práctica porque, por la razón que sea, no se ven capacitados para abordarla, el enunciado especifica con todo detalle qué pasos hay que realizar y cómo ir modificando el material de apoyo inicial para ir incorporando la funcionalidad pedida. Además, cuentan con mi ayuda para cualquier duda, por elemental que parezca. Os garantizo que con un pequeño esfuerzo por vuestra parte y contactando conmigo por correo todas las veces que haga falta seréis capaz de superar esta práctica.
Para aquellos que se vean presionados para prestar su práctica, exponiéndose a ser penalizados, deberían tener en cuenta la consideración anterior.
Objetivo de la práctica
Como se ha estudiado en la parte teórica de la asignatura, los sistemas editor/suscriptor con un modo de operación pull y un almacenamiento persistente de eventos, es decir, con un esquema de streaming, proporcionan una arquitectura apropiada para numerosos escenarios distribuidos. Dentro de este tipo de arquitecturas, Apache Kafka es la principal plataforma y es en el sistema en el que se centra este proyecto práctico, cuyo nombre homenajea a dicha popular plataforma, además de servir como recordatorio de la falibilidad del software. Evidentemente, se trata de una versión muy reducida de este complejo componente software dejando fuera muchas de sus funcionalidades (el uso de múltiples brokers con replicación y particiones, el empaquetamiento de peticiones y respuestas, el uso de claves, la persistencia de los mensajes, los grupos de consumidores, la recuperación de mensajes por timestamp, etc.), pero consideramos que permite apreciar qué tipo de funcionalidad tienen esta clase de sistemas y entender mejor cuál es su modo de operación interno.
A continuación, se incluyen algunos apartados que describen la funcionalidad y requisitos que debe satisfacer la práctica para que sean consultados cuando se necesite. En cualquier caso, puede ir directamente a la sección que explica paso a paso cómo desarrollar la práctica.

Repaso del modo de operación editor/suscriptor con streaming
Aunque ya lo hemos estudiado en la parte teórica de la asignatura, vamos a recordar los puntos más importantes de este tipo de sistemas suponiendo un modo de operación similar al de Kafka:
El broker (brokers en el caso de Kafka) almacena los mensajes/eventos enviados por los productores/editores a los distintos temas.
En Kafka un consumidor/suscriptor guarda a qué temas está suscrito y cuál fue el último mensaje/evento que leyó de cada uno de esos temas (el offset). Por tanto, esa parte del estado del sistema no se almacena en el servidor/broker sino en la biblioteca del cliente.
Tiene un modo de operación pull: un consumidor/suscriptor pide al broker un nuevo mensaje indicándole a qué temas está suscrito y cuál es su offset para cada uno de esos temas.
Cuando un consumidor/suscriptor se suscribe a un tema, su offset inicial será tal que solo podrá ver los mensajes que se envíen al mismo a partir de ese momento.
Un consumidor/suscriptor puede modificar su offset en un tema para poder recibir mensajes anteriores a su suscripción o volver a recibir un mensaje nuevamente.
Para permitir que un consumidor/suscriptor no tenga que estar siempre activo y pueda retomar el trabajo donde lo dejó al volver a ejecutarse, se pueden guardar de forma persistente sus offsets en el broker y recuperarlos al reiniciarse.
Requisitos de la práctica
A continuación, se especifican una serie de requisitos que deben ser obligatoriamente satisfechos por la práctica desarrollada:
La práctica debe funcionar tanto en local como en remoto.
En cuanto a las tecnologías de comunicación usadas en la práctica, se programará en C, se utilizarán sockets de tipo stream y se supondrá un entorno de máquinas heterogéneas.
Se utilizará un esquema con un único proceso que actúa como broker proporcionando el desacoplamiento espacial y temporal entre los editores (productores en terminología Kafka) y los suscriptores (consumidores en terminología Kafka). Al usar un esquema de tipo streaming, el broker se encargará de almacenar los eventos.
El broker dará un servicio concurrente basado en la creación dinámica de threads encargándose cada thread de servir todas las peticiones que llegan por una conexión.
Un proceso editor y/o suscriptor (recuerde que un proceso puede ejercer ambos roles) mantendrá una conexión persistente con el broker durante toda su interacción. Dado ese posible doble rol, en el resto del documento vamos a denominar clientes a este tipo de procesos.
Como en el protocolo Kafka, el nombre de un tema es un string con un tamaño máximo de 216-1 bytes incluyendo el carácter nulo final.
Los mensajes/eventos enviados pueden tener un contenido binario (por ejemplo, pueden corresponder a una imagen o a texto cifrado). Por tanto, no pueden tratarse como strings y es necesario conocer explícitamente su tamaño. Nótese que las aplicaciones que usan este sistema usarán el esquema de serialización que consideren oportuno para enviar la información que manejan,
El diseño del sistema no debe establecer ninguna limitación en el número de temas y clientes existentes en el sistema ni en el número de mensajes almacenados en el broker.
Recuerde que debe manejar adecuadamente las cadenas de caracteres recibidas asegurándose de que terminan en un carácter nulo.
Se debe garantizar un comportamiento zerocopy tanto en la gestión de los nombres de tema como en el contenido de los mensajes:
No se pueden realizar copias de estos campos en el broker.
No se pueden realizar copias de estos campos en la biblioteca de cliente, excepto cuando el enunciado lo indique.
Para evitar la fragmentación en la transmisión, tanto los clientes como el broker mandarán toda la información de una petición o de una respuesta, respectivamente, con un único envío.
Debe optimizarse el uso de ancho de banda de manera que el tamaño de la información enviada sea solo ligeramente mayor que la suma del tamaño de los campos que deben enviarse.
Para facilitar el desarrollo de la práctica, se proporciona una implementación de un tipo de datos que actúa como un mapa iterable (map), permitiendo asociar un valor con una clave, y un tipo que gestiona una cola append-only (queue). Se deben usar obligatoriamente estos tipos de datos a la hora de implementar la práctica.
API ofrecida a las aplicaciones
En esta sección, se describen las operaciones que se les proporcionan a las aplicaciones, que están declaradas en el fichero libkaska/kaska.h al formar parte de la biblioteca de servicio a los clientes. Lógicamente, esta API se basa en la del propio Kafka, pero simplificada y sin empaquetamiento de parámetros. Así, por ejemplo, la función create_topics del API de Kafka para Python permite crear simultáneamente varios temas, estando el protocolo también diseñado para enviarlos en una sola petición. Sin embargo, en nuestra API con esa función solo se puede crear un único tema (create_topic) y lo mismo sucede con el resto de funciones.
A continuación, se describen las operaciones del API, que devolverán un valor negativo en caso de error en la comunicación, cuya funcionalidad más detallada será explicada de forma incremental, según se vayan especificando los sucesivos pasos en el desarrollo de la práctica.

Crea el tema especificado, devolviendo 0 si la operación es correcta y un valor negativo en caso de error porque el tema ya exista.
int create_topic(char *topic);
Devuelve cuántos temas existen en el sistema.
int ntopics(void);
Envía el mensaje al tema especificado. Nótese la necesidad de indicar el tamaño del mensaje ya que puede tener un contenido de tipo binario. Devuelve el offset donde queda almacenado el mensaje en la cola asociada al tema y un valor negativo en caso de error debido a que el tema no exista. Si el segundo parámetro vale 0, la operación no realizará ninguna labor, pero no se se considerará que se trata de un error.
int send_msg(char *topic, int msg_size, void *msg);
Devuelve la longitud del mensaje almacenado en ese offset del tema indicado y un valor negativo en caso de error debido a que el tema no existe. Si no hay mensajes en ese offset, se debe devolver un 0.
int msg_length(char *topic, int offset);
Obtiene el último offset asociado a un tema en el broker, que corresponde al del último mensaje enviado más uno y, dado que los mensajes se numeran desde 0, coincide con el número de mensajes asociados a ese tema. Devuelve un valor negativo en caso de error debido a que el tema no existe.
int end_offset(char *topic);
Se suscribe al conjunto de temas recibidos. No permite suscripción incremental: hay que especificar todos los temas de una vez. Si un tema no existe o está repetido en la lista simplemente se ignora. Devuelve el número de temas a los que realmente se ha suscrito y un valor negativo solo si ya estaba suscrito a algún tema.
int subscribe(int ntopics, char **topics);
Se da de baja de todos los temas suscritos. Devuelve un valor negativo si no había suscripciones activas. Nótese que se trata de una operación local.
int unsubscribe(void);
Devuelve el offset del cliente para ese tema y un número negativo en caso de error porque no esté suscrito a ese tema. Nótese que se trata de una operación local.
int position(char *topic);
Modifica el offset del cliente para ese tema devolviendo un error si no está suscrito a ese tema. Nótese que se trata de una operación local.
int seek(char *topic, int offset);
Obtiene el siguiente mensaje destinado a este cliente. Los dos parámetros son de salida: a qué tema corresponde y el mensaje recibido. La propia función poll se encarga de reservar en memoria dinámica espacio para el tema y el mensaje. Es responsabilidad de la aplicación liberar ese espacio. Si no hay ningún mensaje, la operación retornará un 0. Devolverá un valor negativo si no está suscrito a ningún tema. Si en cualquiera de los parámetros se recibe un valor nulo, se procederá con la operación normal pero, evidentemente, no se asignará un valor a ese parámetro.
int poll(char **topic, void **msg);
Almacena de forma persistente en el broker el offset especificado para el tema indicado asociándolo con ese cliente. Devuelve un error si el tema no existe. No se requiere estar suscrito al tema.
int commit(char *client, char *topic, int offset);
Recupera del broker el offset almacenado para ese tema correspondiente a ese cliente. Devuelve un error si el tema o el cliente no existen. No se requiere estar suscrito al tema.
int commited(char *client, char *topic);
