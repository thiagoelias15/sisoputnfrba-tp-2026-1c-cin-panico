#!/bin/bash
# ======= PRUEBA HERENCIA DE PRIORIDADES =======

./matar.sh

echo "=== Iniciando Prueba Herencia ==="

cd swap && ./bin/swap swap.config &
cd ..
sleep 1

cd kernel_memory && ./bin/kernel_memory memoria_pmp.config &
cd ..
sleep 1

cd memory_stick
./bin/memory_stick ms1.config 16 &
sleep 1
./bin/memory_stick ms2.config 16 &
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

cd kernel_scheduler && ./bin/kernel_scheduler sched_herencia.config PHP.prc &
cd ..
sleep 2

cd cpu && ./bin/cpu cpu.config 1 &
cd ..

echo "=== Prueba Herencia iniciada (2 sticks: 16+16 = 32 bytes) ==="