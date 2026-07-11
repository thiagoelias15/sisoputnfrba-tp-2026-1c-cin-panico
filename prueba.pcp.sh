#!/bin/bash
# ======= PRUEBA PLANIFICACIÓN CORTO PLAZO =======

./matar.sh

echo "=== Iniciando Prueba PCP ==="

cd swap && ./bin/swap swap.config &
cd ..
sleep 1

cd kernel_memory && ./bin/kernel_memory memoria_pcp.config &
cd ..
sleep 1

cd memory_stick && ./bin/memory_stick ms1.config 256 &
cd ..
sleep 2

cd io && ./bin/io io_sleep.config SLEEP &
cd ..
sleep 1
cd io && ./bin/io io_stdin.config STDIN &
cd ..
sleep 1
cd io && ./bin/io io_stdout.config STDOUT &
cd ..
sleep 1

cd kernel_scheduler && ./bin/kernel_scheduler sched_pcp.config PCP.prc &
cd ..
sleep 2

cd cpu && ./bin/cpu cpu.config 1 &
cd ..

echo "=== Prueba PCP iniciada (1 stick 256 bytes) ==="