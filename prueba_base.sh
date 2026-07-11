#!/bin/bash
# ======= PRUEBA BASE =======
# Cambiar estas IPs para el deploy en la facu
IP_MEMORIA="127.0.0.1"
IP_SCHEDULER="127.0.0.1"
IP_SWAP="127.0.0.1"
IP_STICKS="127.0.0.1"

./matar.sh

echo "=== Iniciando Prueba Base ==="

cd swap && ./bin/swap swap.config &
cd ..
sleep 1

cd kernel_memory && ./bin/kernel_memory memoria_base.config &
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

cd kernel_scheduler && ./bin/kernel_scheduler sched_base.config PLANI_PRE_0.prc &
cd ..
sleep 2

cd cpu && ./bin/cpu cpu.config 1 &
cd ..

echo "=== Prueba Base iniciada (1 stick 256 bytes) ==="
echo "Esperá a que terminen los procesos."
echo "Después corré de nuevo con MEMORIA_PRE_0.prc"