# POC Keylogger - Windows Hook Research

## Advertencia
Este proyecto es exclusivamente para fines educativos
y de investigación en entornos controlados.
No usar en sistemas sin autorización explícita.

## Descripción
Prueba de concepto de keylogger usando WH_KEYBOARD_LL
hooks nativos de Windows.

## Entorno de prueba
Windows 10

## Compilación
x86_64-w64-mingw32-g++ -o keylogger.exe src/keylogger.cpp -lkernel32 -luser32 -static-libgcc -static-libstdc++ -Wl,--subsystem,windows -m64
