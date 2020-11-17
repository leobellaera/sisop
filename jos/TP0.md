TP0: Introducción a JOS
=======================

backtrace_func_names
--------------------

Salida del comando `backtrace`:

```
K> backtrace
  ebp f0110f48  eip f0100a21  args 00000001 f0110f70 00000000 0000000a 00000009
	     kern/monitor.c:122: runcmd+261
  ebp f0110fc8  eip f0100a6a  args 00010094 00010094 f0110ff8 f01000e9 00000000
	     kern/monitor.c:140: monitor+66
  ebp f0110fd8  eip f01000e9  args 00000000 00001aac 00000644 00000000 00000000
	     kern/init.c:43: i386_init+81
  ebp f0110ff8  eip f010003e  args 00112021 00000000 00000000 00000000 00000000
	     kern/entry.S:84: entry+50
```
