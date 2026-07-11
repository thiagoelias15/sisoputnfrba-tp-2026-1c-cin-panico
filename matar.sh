#!/bin/bash
killall -q kernel_memory swap memory_stick kernel_scheduler cpu io 2>/dev/null
sleep 1
echo "Todos los módulos detenidos"