TP2: Procesos de usuario
========================

## env_alloc

#### 1. ¿Qué identificadores se asignan a los primeros 5 procesos creados? (Usar base hexadecimal.)

A los primeros 5 procesos creados se le asignan un identificador único.
Por lo tanto se le asignara los siguientes valores, segun la cuenta realizada en el código:
```
1. 0x1000
2. 0x1001
3. 0x1002
4. 0x1003
5. 0x1004
```

#### 2. Supongamos que al arrancar el kernel se lanzan NENV procesos a ejecución. A continuación se destruye el proceso asociado a envs[630] y se lanza un proceso que cada segundo muere y se vuelve a lanzar. ¿Qué identificadores tendrá este proceso en sus sus primeras cinco ejecuciones?

Si a la posicion 0x1000 se le suman 630, el valor del identificador sera de ```0x1276```
Al destruir ese proceso el identificador sigue siendo el mismo.
Cuando se vuelva a crear se deberá sumar 4096 (cantidad de envs disponibles)
Y lo mismo ocurrirá en cada ejecucion, es decir:
```
1. 0x1000 + 630 = 0x1276
2. 0x1276 + 4096 = 0x2276
3. 0x2276 + 4096 = 0x3276
4. 0x3276 + 4096 = 0x4276
5. 0x4276 + 4096 = 0x5276
```

## env_init_percpu

#### 1. ¿Cuántos bytes escribe la función lgdt, y dónde? ¿Qué representan esos bytes?
La instrucción `lgdt` recibe como operando la dirección de un struct de tipo `Pseudodesc`, que no es más que un contenedor de una variable de tipo `uint16_t` y una variable de tipo `uint32_t` que se corresponden con la dirección virtual de la `Global Descriptor Table` y `sizeof(gdt) - 1` respectivamente. Este argumento se escribe en el regristro GDTR ("Global Descriptor Table Register"), por lo cual se escriben 6 bytes.

## env_pop_tf

#### 1. Dada la secuencia de instrucciones assembly en la función, describir qué contiene durante su ejecución: 

##### El tope de la pila justo antes popal
   Justo antes de ejecutar la instrucción `popal` la unica instruccion ejecutada es `movl %0, %%esp`. Tras ejecutar a la misma, %esp contendrá la dirección  base del Trapframe en cuestión.
    
##### El tope de la pila justo antes iret
   En este momento, %esp (el tope de la pila) contendrá el valor del atributo tf_eip del Trapframe.
    
##### El tercer elemento de la pila justo antes de iret
   El tercer elemento de la pila, es decir, 8(%eip) contiene el campo tf_eflags del Trapframe.


#### 2. ¿Cómo determina la CPU (en x86) si hay un cambio de ring (nivel de privilegio)? Ayuda: Responder antes en qué lugar exacto guarda x86 el nivel de privilegio actual. ¿Cuántos bits almacenan ese privilegio?

En la función env_alloc se ejecutan las siguientes líneas de código:

  ```c
	e->env_tf.tf_ds = GD_UD | 3;
	e->env_tf.tf_es = GD_UD | 3;
	e->env_tf.tf_ss = GD_UD | 3;
	e->env_tf.tf_esp = USTACKTOP;
	e->env_tf.tf_cs = GD_UT | 3;
  ```

Estas líneas setean los 2 bits más bajos del registro de cada segmento, que equivale al tercer ring (privilegios de usuario) marcando a los mismos con  los valores GD_UD ("Global Descriptor User Data) y GD_UT ("Global Descriptor User Text"). De esta forma, el CPU puede saber si el code segment a ejecutar pertenece a una aplicación de usuario o al kernel. Si pertenece al usuario, entonces iret restaura los registros SS ("Stack Segment") y ESP ("Stack Pointer").

Además, el CPU identifica si hay un cambio de ring a partir de los últimos dos bits del registro %cs. Así, puede saber si se está en user mode (11) o en kernel mode (00)

## kern_idt

#### 1. ¿Cómo decidir si usar TRAPHANDLER o TRAPHANDLER_NOEC? ¿Qué pasaría si se usara solamente la primera?

Se usa uno o la otra dependiendo de si el CPU debe pushear o no el código de error al stack. Si se quiere pushear el codigo de error se usa la macro `TRAPHANDLER` y en caso contrario `TRAPHANDLER_NOEC`.
Si se usara solo la primera, el stack basado en el trapframe quedaria en un formato invalido en los casos de excepciones u interrupciones en las cuales el CPU no pushee el código de error, desencadenando errores indeseados.

#### 2. ¿Qué cambia, en la invocación de handlers, el segundo parámetro (istrap) de la macro SETGATE? ¿Por qué se elegiría un comportamiento u otro durante un syscall?

Lo que define el valor de este parámetro es si el handler permite o no que ocurran interrupciones encadenadas.

#### 3. Leer user/softint.c y ejecutarlo con make run-softint-nox. ¿Qué excepción se genera? Si es diferente a la que invoca el programa… ¿cuál es el mecanismo por el que ocurrió esto, y por qué motivos?

Al ejecutar el comando se obtiene la siguiente salida:

```
[00000000] new env 00001000
Incoming TRAP frame at 0xefffffbc
TRAP frame at 0xf01c0000
  edi  0x00000000
  esi  0x00000000
  ebp  0xeebfdfd0
  oesp 0xefffffdc
  ebx  0x00000000
  edx  0x00000000
  ecx  0x00000000
  eax  0x00000000
  es   0x----0023
  ds   0x----0023
  trap 0x0000000d General Protection
  err  0x00000072
  eip  0x00800036
  cs   0x----001b
  flag 0x00000082
  esp  0xeebfdfd0
  ss   0x----0023
[00001000] free env 00001000
Destroyed the only environment - nothing more to do!
```

Podemos observar en la salida obtenida que el valor asociado a trap es `0x0000000d` que se corresponde con el número decimal 13 y está asociado al trap `General Protection`. Este es diferente a la interrupción invocada en el programa, que es el trap número 14 y está asociado a la interrupción `Page Fault`. Esto se debe a que en el llamado a la interrupción 14 en la aplicación `softinc` se tienen privilegios de usuario, mientras que dicha interrupción fue declarada con un nivel de privilegio 0 en el archivo `trap.c` (mediante el llamado a la macro `SETGATE(idt[T_PGFLT], 0, GD_KT, trap_14, 0)`) lo que significa que solo el kernel puede invocarla y transferir la ejecución a la misma. Si se intenta invocar un `Page Fault` sin estar en el ring 0, ocurre una excepción de tipo `General Protection`, que es la que encontramos en la salida obtenida. 

## gdb_hello


1. Poner un breakpoint en env_pop_tf() y continuar la ejecución hasta allí.

```
(gdb) breakpoint env_pop_tf
Punto de interrupción 1 at 0xf0102ead: file kern/env.c, line 514.
(gdb) continue
Continuando.
Se asume que la arquitectura objetivo es i386
=> 0xf0102ead <env_pop_tf>:	push   %ebp

Breakpoint 1, env_pop_tf (tf=0xf01c0000) at kern/env.c:514
514	{
```

2. En QEMU, entrar en modo monitor (Ctrl-a c), y mostrar las cinco primeras líneas del comando info registers.

```
(qemu) info registers 
EAX=003bc000 EBX=f01c0000 ECX=f03bc000 EDX=0000023d
ESI=00010094 EDI=00000000 EBP=f0118fd8 ESP=f0118fbc
EIP=f0102ead EFL=00000092 [--S-A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
```

3. De vuelta a GDB, imprimir el valor del argumento tf

```
(gdb) p tf
$1 = (struct Trapframe *) 0xf01c0000
```

4. Imprimir, con x/Nx tf tantos enteros como haya en el struct Trapframe donde N = sizeof(Trapframe) / sizeof(int).

Obtuvimos como resultado N = 17.

```
(gdb) x/17x tf
0xf01c0000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c0030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c0040:	0x00000023
```

5. Avanzar hasta justo después del `movl ...,%esp`, usando `si M` para ejecutar tantas instrucciones como sea necesario en un solo paso.
```
(gdb) disas
Dump of assembler code for function env_pop_tf:
=> 0xf0102ead <+0>:	push   %ebp
   0xf0102eae <+1>:	mov    %esp,%ebp
   0xf0102eb0 <+3>:	sub    $0xc,%esp
   0xf0102eb3 <+6>:	mov    0x8(%ebp),%esp
   0xf0102eb6 <+9>:	popa   
   0xf0102eb7 <+10>:	pop    %es
   0xf0102eb8 <+11>:	pop    %ds
   0xf0102eb9 <+12>:	add    $0x8,%esp
   0xf0102ebc <+15>:	iret   
   0xf0102ebd <+16>:	push   $0xf010573c
   0xf0102ec2 <+21>:	push   $0x20c
   0xf0102ec7 <+26>:	push   $0xf0105706
   0xf0102ecc <+31>:	call   0xf01000a9 <_panic>
End of assembler dump.
(gdb) si 4
=> 0xf0102eb6 <env_pop_tf+9>:	popa   
0xf0102eb6	515		asm volatile("\tmovl %0,%%esp\n"
(gdb) disas
Dump of assembler code for function env_pop_tf:
   0xf0102ead <+0>:	push   %ebp
   0xf0102eae <+1>:	mov    %esp,%ebp
   0xf0102eb0 <+3>:	sub    $0xc,%esp
   0xf0102eb3 <+6>:	mov    0x8(%ebp),%esp
=> 0xf0102eb6 <+9>:	popa   
   0xf0102eb7 <+10>:	pop    %es
   0xf0102eb8 <+11>:	pop    %ds
   0xf0102eb9 <+12>:	add    $0x8,%esp
   0xf0102ebc <+15>:	iret   
   0xf0102ebd <+16>:	push   $0xf010573c
   0xf0102ec2 <+21>:	push   $0x20c
   0xf0102ec7 <+26>:	push   $0xf0105706
   0xf0102ecc <+31>:	call   0xf01000a9 <_panic>
End of assembler dump
```
6. Comprobar, con ```x/Nx $sp``` que los contenidos son los mismos que tf (donde N es el tamaño de tf).

```
(gdb) x/17x $sp
0xf01c0000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c0030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c0040:	0x00000023
```

7. Explicar con el mayor detalle posible cada uno de los valores. Para los valores no nulos, se debe indicar dónde se configuró inicialmente el valor, y qué representa.

En esta instancia, el stack tiene cargado en su tope valores que están caracterizados aparecerán en el mismo orden en el que aparecen en la estructura `Trapframe`, que tiene la siguiente definición:
```
struct Trapframe {
	struct PushRegs tf_regs;
	uint16_t tf_es;
	uint16_t tf_padding1;
	uint16_t tf_ds;
	uint16_t tf_padding2;
	uint32_t tf_trapno;
	/* below here defined by x86 hardware */
	uint32_t tf_err;
	uintptr_t tf_eip;
	uint16_t tf_cs;
	uint16_t tf_padding3;
	uint32_t tf_eflags;
	/* below here only when crossing rings, such as from user to kernel */
	uintptr_t tf_esp;
	uint16_t tf_ss;
	uint16_t tf_padding4;
} __attribute__((packed));
```

Con el struct PushRegs definido como:

```
struct PushRegs {
	/* registers as pushed by pusha */
	uint32_t reg_edi;
	uint32_t reg_esi;
	uint32_t reg_ebp;
	uint32_t reg_oesp;	
	uint32_t reg_ebx;
	uint32_t reg_edx;
	uint32_t reg_ecx;
	uint32_t reg_eax;
} __attribute__((packed));
```

Analizamos los campos del struct PushRegs y vemos que todos son nulos, lo cual no nos sorprende, puesto que es la primera vez que este proceso entra en contexto y sus valores son inicializados en 0.

``` 
0xf01c0000:	0x00000000	0x00000000	0x00000000	0x00000000
              reg_edi     reg_esi     reg_ebp     reg_oesp

0xf01c0010:	0x00000000	0x00000000	0x00000000	0x00000000
              reg_ebx     reg_edx     reg_ecx     reg_eax
	      
```

Luego tenemos los campos `tf_es`, `tf_padding1`, `tf_ds` y `tf_padding2`, que se corresponden con los dos primeros enteros de 4 bytes mostrados a continuación, respectivamente. En la función env_alloc() se inicializaron con el valor `GD_UD | 3` (User-Data y Privilegios de Usuario) los campos `es` y `ds`, que se corresponden con los valores hexadecimales `0x00000023` que encontramos. 
Por último, encontramos que los campos `trapno` y `tf_err` tienen un valor 0, lo cual es lógico puesto que a esta altura no se produjeron errores ni se invocaron interrupciones por el proceso.

```
0xf01c0020:	0x00000023	0x00000023	0x00000000	0x00000000
		          pad - es    pad - ds    "trapno"    "tf_err"
```

El valor del campo `tf_eip` es la dirección del entry point del código del proceso (y podemos ver que coincide con el address virtual del `.text` si analizamos el `ELF` mediante el comando `readelf -S obj/user/hello`). En la función `env_alloc` se inicializó con el valor `GD_UT | 3` (User-Text y Privilegios de Usuario) el campo `cs`, que se corresponde con el hexa `0x0000001b`. El valor del campo `esp` encontrado se corresponde a la dirección seteada en la función env_alloc() (definida por la macro `USTACKTOP`).


```
0xf01c0030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
              tf_eip      pad - cs    tf_eflags   tf_esp
```

Finalmente, valor del campo `ss` se corresponde con la inicialización realizada por la función env_alloc().

```
0xf01c0040:	0x00000023
              pad - ss
```


8. Continuar hasta la instrucción iret, sin llegar a ejecutarla. Mostrar en este punto, de nuevo, las cinco primeras líneas de info registers en el monitor de QEMU. Explicar los cambios producidos.

```
(gdb) disas
Dump of assembler code for function env_pop_tf:
   0xf0102ead <+0>:	push   %ebp
   0xf0102eae <+1>:	mov    %esp,%ebp
   0xf0102eb0 <+3>:	sub    $0xc,%esp
   0xf0102eb3 <+6>:	mov    0x8(%ebp),%esp
=> 0xf0102eb6 <+9>:	popa   
   0xf0102eb7 <+10>:	pop    %es
   0xf0102eb8 <+11>:	pop    %ds
   0xf0102eb9 <+12>:	add    $0x8,%esp
   0xf0102ebc <+15>:	iret   
   0xf0102ebd <+16>:	push   $0xf010573c
   0xf0102ec2 <+21>:	push   $0x20c
   0xf0102ec7 <+26>:	push   $0xf0105706
   0xf0102ecc <+31>:	call   0xf01000a9 <_panic>
End of assembler dump.
(gdb) si 4
=> 0xf0102ebc <env_pop_tf+15>:	iret   
0xf0102ebc	515		asm volatile("\tmovl %0,%%esp\n"
(gdb) disas
Dump of assembler code for function env_pop_tf:
   0xf0102ead <+0>:	push   %ebp
   0xf0102eae <+1>:	mov    %esp,%ebp
   0xf0102eb0 <+3>:	sub    $0xc,%esp
   0xf0102eb3 <+6>:	mov    0x8(%ebp),%esp
   0xf0102eb6 <+9>:	popa   
   0xf0102eb7 <+10>:	pop    %es
   0xf0102eb8 <+11>:	pop    %ds
   0xf0102eb9 <+12>:	add    $0x8,%esp
=> 0xf0102ebc <+15>:	iret   
   0xf0102ebd <+16>:	push   $0xf010573c
   0xf0102ec2 <+21>:	push   $0x20c
   0xf0102ec7 <+26>:	push   $0xf0105706
   0xf0102ecc <+31>:	call   0xf01000a9 <_panic>
End of assembler dump.
```
Tenemos los valores que nos arroja ahora `info registers`:
```
(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=f01c0030
EIP=f0102ebc EFL=00000096 [--S-AP-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
```

Se actualizaron los valores de los registros de propósito general a los valores contenidos en el trapframe. Esto cambios se producen al ejecutarse la instrucción popal. Además, se actualizaron los valores del registro ES (al ejecutar la instrucción `popl %%es`) y el registro DS (al ejecutar la instrucción `popl %%ds`).
El instruction pointer (EIP) se vió modificado debido a que se ejecutaron algunas líneas de código.

9. Ejecutar la instrucción iret. En ese momento se ha realizado el cambio de contexto y los símbolos del kernel ya no son válidos. Imprimir el valor del contador de programa con p $pc o p $eip. Cargar los símbolos de hello con symbol-file obj/user/hello. Volver a imprimir el valor del contador de programa. Mostrar una última vez la salida de info registers en QEMU, y explicar los cambios producidos.
```
(gdb) p $pc
$1 = (void (*)()) 0x800020

(gdb) p $pc
$1 = (void (*)()) 0x800020 <_start>
```
```
(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=eebfe000
EIP=00800020 EFL=00000002 [-------] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
```

Se actualizaron los registros `IP`, `CS`, `EFL` y `SS` a los valores que especifica el Trapframe.

10. Poner un breakpoint temporal en la función syscall() y explicar qué ocurre justo tras ejecutar la instrucción int $0x30. Usar, de ser necesario, el monitor de QEMU.
```
(gdb) tbreak syscall
Punto de interrupción temporal 2 at 0x8009ed: file lib/syscall.c, line 23.
(gdb) c
Continuando.
=> 0x8009ed <syscall+17>:	mov    0x8(%ebp),%ecx
```

Luego de poner el breakpoint temporal se volvio a utilizar el comando `info_registers` en el monitor del QEMU para ver el contexto actual. Puede observarse que el valor del registro `%cs` paso nuevamente a terminar en 00 (estamos en modo kernel).

```
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000663
ESI=00000000 EDI=00000000 EBP=00000000 ESP=00000000
EIP=0000e062 EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0000 00000000 0000ffff 00009300
CS =f000 000f0000 0000ffff 00009b00
```

## user_evilhello

#### 1. Ejecutar el siguiente programa y describir qué ocurre:

```c
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
    char *entry = (char *) 0xf010000c;
    char first = *entry;
    sys_cputs(&first, 1);
}
```

Al correr el programa obtenemos la siguiente salida:

```
[00000000] new env 00001000
Incoming TRAP frame at 0xefffffbc
[00001000] user fault va f010000c ip 0080003d
TRAP frame at 0xf01c7000
  edi  0x00000000
  esi  0x00000000
  ebp  0xeebfdfd0
  oesp 0xefffffdc
  ebx  0x00000000
  edx  0x00000000
  ecx  0x00000000
  eax  0x00000000
  es   0x----0023
  ds   0x----0023
  trap 0x0000000e Page Fault
  cr2  0xf010000c
  err  0x00000005 [user, read, protection]
  eip  0x0080003d
  cs   0x----001b
  flag 0x00000082
  esp  0xeebfdfb0
  ss   0x----0023
[00001000] free env 00001000
Destroyed the only environment - nothing more to do!
```
Notamos que ocurrió un `Page Fault` durante la ejecución del programa.

#### 2. ¿En qué se diferencia el código de la versión en evilhello.c mostrada arriba?

A diferencia de la otra versión, previo al llamado a la syscall que imprimirá el entry point del kernel, se intenta desreferenciar el contenido del puntero entry desde la aplicación de usuario y ocurre el error mencionado en el punto anterior.

#### 3. ¿En qué cambia el comportamiento durante la ejecución? ¿Por qué? ¿Cuál es el mecanismo?

Dado que la memoria que se desea acceder en el programa pertenece al kernel y no es accesible por el usuario, la MMU detecta este acceso indebido, ocurre el page fault y el proceso es destruido. En la otra versión del programa, simplemente se le solicita a `sys_cputs` que imprima el contenido de la dirección `0xf010000c` y la misma será accedida en modo kernel dentro del handler de esta syscall; por estos motivos, el programa finaliza correctamente. Concluimos que, como método de protección, es necesario que el kernel valide las direcciones que el usuario introduce como argumento en sus llamados a las syscalls.
