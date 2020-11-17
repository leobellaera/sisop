TP1: Memoria virtual en JOS
===========================

## boot_alloc_pos
--------------

#### a. Un cálculo manual de la primera dirección de memoria que devolverá boot_alloc() tras el arranque. Se puede calcular a partir del binario compilado (obj/kern/kernel), usando los comandos readelf y/o nm y operaciones matemáticas.

Previo a la  primera ejecución de ```boot_alloc()```, el símbolo `end` tiene la dirección virtual ```0xf0118950``` (podemos conocerla ejecutando `nm obj/kern/kernel | grep end` en el directorio donde se alojan los fuentes de JOS). Como en el primer llamado a ```boot_alloc()``` la variable `nextfree` es ```NULL```, se alinea la dirección virtual de `end` -redondeándola hacia arriba- a 12 bits, siendo la primera dirección virtual que devolverá ```boot_alloc()``` tras el arranque. Luego, el cálculo a realizar para obtener esta dirección es ```(0xf0118950 + 0x1000 - 0x1) - (0xf0118950 + 0x1000 - 0x1) % 0x1000```. 

```
>>> (0xf0118950 + 0x1000 - 0x1) - (0xf0118950 + 0x1000 - 0x1) % 0x1000
>>> '0xf0119000'
```

Luego, la dirección que devolverá ```boot_alloc()``` tras el arranque será ```0xf0119000```.

#### b. Una sesión de GDB en la que, poniendo un breakpoint en la función  boot_alloc(), se muestre el valor de end y nextfree al comienzo y fin de esa primera llamada a boot_alloc().

* Comienzo
```
(gdb) p nextfree
$1 = 0x0

(gdb) p/x &end
$2 = 0xf0118950
```

* Fin
```
(gdb) p nextfree
$3 = 0xf0119000

(gdb) p/x &end
$4 = 0xf0118950
```

## page_alloc
----------
#### ¿En qué se diferencia page2pa() de page2kva()?

* ```page2pa``` Devuelve la dirección física de la página a la que referencia 
el argumento pp (siendo pp un puntero a `struct PageInfo`).

* ```page2kva``` Devuelve la dirección virtual de la página a la que 
referencia el argumento pp (siendo pp un puntero a `struct PageInfo`).

El arreglo `pages` se utiliza como una abstracción de las páginas de memoria 
real que disponemos (utilizando elementos del tipo `struct PageInfo`para 
representarlas), siendo el elemento i-ésimo del mismo una referencia a la 
página  de memoria i. Si restamos la dirección de uno de los elementos del 
arreglo con la dirección base del mismo, obtenemos el índice en el cual se 
encuentra este elemento en el arreglo; luego, shifteando este índice 12 
posiciones hacia la izquierda (pues, al ser nuestras páginas de 4KB, los 
primeros 12 bits de una dirección física son el offset sobre la página a la 
cual se referencia con los primeros 20 bits de la dirección) obtenemos la 
dirección *física* de la página a la cual estamos referenciando. Además, 
sumando `KERNBASE` a la dirección física obtenida podemos obtener la dirección 
virtual de la página en cuestión.

## map_region_large
----------

#### ¿Cuánta memoria se ahorró de este modo? ¿Es una cantidad fija, o depende de la memoria física de la computadora?
