This virtual com port loopback is based on bulk loopback
firmware example. Loops back EP2OUT to EP6IN.
The descriptors are changed so that standard USB serial driver
is loaded automatically on windows and linux. The watchdog timer
keeps track of the SOF interrupts and triggers reconnection if
they are lost.

Building this example requires the full version of the Keil Tools.
