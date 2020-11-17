### Trabajo Práctico

#### Comandos para correr el programa
- `make qemu` 
- `make qemu-nox` (para evitar que se lance una terminal gráfica).

#### Compilación
Primero se compilan los .o en el directorio obj/kern y luego se generan los archivos: kernel, kernel.asm, kernel.img y kernel.sym.

- kernel es un ejectuable, más precisamente una imagen de un ejecutable (`ELF`).
- kernel.asm es un assembly que se genera a partir del binario kernel. Aquí  podemos ver las direcciones del código de cada instrucción; tenemos un mapeo uno a uno de cada instrucción a la dirección en la que corre. *Este sirve para debuggear el programa porque lo único que sabremos es en qué dirección crasheó el kernel*
- kernel.sym es una lista de las funciones del kernel y las direcciones en las que están.
- kernel.img contiene, en esencia, el bootloader junto con el binario elf (kernel). Es la que se termina cargando en memoria (la levanta qemu). `man dd` si se quiere entender mejor como se genera el kernel.img en el make. Básicamente se copian 10000 bloques de 512 bytes de ceros (`dev/zero`) al inicio del archivo kernel.img. Luego se escribe todo el contenido del bootloader al inicio de `kernel.img` sin truncarlo. Por último, se saltean 512 bytes del output (se supone, entonces, que el bootloader debe ocupar máximo 512 bytes) y se escribe todo el contenido del binario `kernel` en `kernel.img` sin truncarlo (notar que de esta forma se logra que **siempre** la imágen tenga el mismo tamaño, así se puede simular un "disco").
 
¿Qué es `ELF`? Es el formato de ejecutables estándar de plataformas Unix.
  Los formatos de ejecutables existen para que haya un estándar tal que si vos querés correr un binario en Windows, Mac, Linux u Bsd se lo das al SO y el mismo recibe con el ejecutable no solo la secuencia de instrucciones máquina a ejecutar si no ciertos metadatos: este ejecutable es para tal arquitectura, hace uso de estas bibliotecas compartidas, está enlazado en esta dirección, entre otros. Para conseguir que el kernel corra en una máquina necesitamos ayuda de un bootloader que cargue las instrucciones en memoria: el bootloader debe entender el formato en el que le das el kernel para cargarlo. Entonces, no hay ninguna necesidad de que un kernel sea un `ELF` pero da beneficios porque es un formato estándar, hay muchas herramientas que lo entienden, hay bibliotecas que lo pueden parsear, entonces se elige compilarlo en formato `ELF`.

#### Depurado
*"The GDB Remote Serial Protocol (RSP) provides a high level protocol allowing GDB to connect to any target remotely..."*

El que implementa el protocolo es en nuestro caso es `qemu`.

Necesitamos dos terminales: 
- En una se corre `make qemu-gdb`
- En la otra se corre `make gdb`: esta es la que vamos a usar para debuggear

Algunos comandos importantes:
-  `b i386_init()`: dirección de la primera función a ser ejecutada por el kernel.
- `info registers`: información de los registros. 

Si la terminal donde corrimos `make gdb` se cuelga:
- Podemos ir a la ventana donde corrimos `qemu-gdb` y utilizando el keycombo ` ctrl A + C` entramos al   *qemu monitor* que nos provee de información sin necesidad de utilizar gdb (por ejemplo **info registers**).
- Para finalizar la ejecución de qemu podemos utilizar el keycombo `ctrl A + X`.

#### TP0

Se pide implementar una función *backtrace* similar al comando `bt` de GDB; imprime la secuencia de llamados a funciones que condujo al punto actual en donde estamos en la ejecución del programa. Esta función se utiliza cuando se detecta que el kernel está crasheando, antes de morir y terminar se imprime siempre el `backtrace`para dar una información al usuario de "qué estaba pasando" cuando crasheó.

Cada vez que hay una instrucción call se guarda en el tope de la pila la dirección de retorno (`call + instruction bytes quantity`) y se guarda por convención el *framepointer* (que apunta a lo que era el marco de la función anterior). 
**Obs:** No siempre hay framepointer, esto depende del nivel de optimización, de la arquitectura y de los flags del compilador (pero **siempre** se pushea la dirección de retorno).

El *framepointer* es la dirección del tope de la pila cuando se ingresa a la función.

- La función, básicamente, hace un while que va navegando esa "lista enlazada" hasta llegar a la primera función de todas (un marco de referencia 0).

- Para cada función, ebp se refiere al valor del registro `%ebp` (esto es, el valor del stack pointer al entrar a la función), y eip a la instrucción de retorno.

- **Nota**: el **eip** y el **ebp** están juntos en el stack.

```c
struct Eipdebuginfo {
    const char *eip_file;   // Source code filename for EIP
    int eip_line;           // Source code linenumber for EIP

    const char *eip_fn_name;  // Name of function containing EIP
                              //     Note: not null terminated!
    int eip_fn_namelen;       // Length of function name
    uintptr_t eip_fn_addr;    // Address of start of function
    int eip_fn_narg;          // Number of function arguments
};
```

- ¿Cómo se ha implementado `debuginfo_eip()`?
Cuando se compila se añade la familia de opciones (flags) g para añadir al código objeto información que permita mapear instrucciones a líneas de código (esa información puede estar en distintos formatos según el flag). Esta información viene en un formato estándar de tabla de símbolos para que cualquier debugger lo pueda consultar y saber qué está pasando. Por ejemplo, cuando compilamos con `-g` para que cuando gdb corre el ejecutable, se encuentre que el ejecutable ha heredado esa información de los objetos (no la pierde, la sigue teniendo) y la puede usar para decirnos que es lo que ha pasado en términos humanos (de nombres de funciones y de archivos).
  - `-g` es genérico y produce información de depuración en el formato estándar para el sistema operativo.
  - `-ggdb` es específico para gdb.
  - `-gstabs` es el más sencillo, el más antiguo que contiene menos información (este utiliza `jos`).

- ¿Cómo se logra parsear desde adentro mismo de `jos` la tabla de símbolos que viene heredada de los objetos que componen su binario?
Como no puede hacer simplemente "open" del binario que contiene esta metadata, el bootloader se encarga de incluir toda  la información de depuración (por ejemplo `__STAB_BEGIN__` es la dirección de inicio de esta tabla de símbolos y es una macro que se define en el enlazador).

##### Solución mon_backtrace()

> Se pide ampliar la salida de mon_backtrace() para que, usando campos adicionales de Eipdebuginfo, muestre el archivo y número de línea en que se halla la instrucción, así como el offset de eip respecto a la dirección de comienzo de la funció.

En el code segment tenemos:

```c
foo_b:
    (...)
    x: call foo_b //Pushea en eip x + 4
    return_address: (...)
```
Lo que queremos es imprimir return_address - foo_b (es decir el offset de eip con respecto a la dirección de inicio de la función); en el stackframe de la callee `foo_b` mediante `%ebp` + 4 (notar que %ebp apunta al inicio del stackframe de foo_b) conseguimos la dirección *return_address* y, haciendo uso de la función debuginfo_eip, conseguimos la dirección (del codesegment) de la etiqueta `foo_b` (es decir, la dirección de inicio de la función b).
