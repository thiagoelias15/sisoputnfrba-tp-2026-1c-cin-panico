#!/bin/bash
BASE=~/tp-2026-1c-cin-panico

./matar.sh
sleep 1
echo "=== Iniciando Prueba Memoria (BEST FIT) ==="

xterm -T "SWAP" -e bash -c "$BASE/swap/bin/swap $BASE/swap/swap.config; echo 'SWAP terminó'; read" &
sleep 1
xterm -T "KERNEL MEMORY" -e bash -c "$BASE/kernel_memory/bin/kernel_memory $BASE/kernel_memory/memoria_memoria_best.config; echo 'KM terminó'; read" &
sleep 1
xterm -T "MEMORY STICK 1 (16)" -e bash -c "$BASE/memory_stick/bin/memory_stick $BASE/memory_stick/ms1.config 16; echo 'MS1 terminó'; read" &
sleep 1
xterm -T "MEMORY STICK 2 (32)" -e bash -c "$BASE/memory_stick/bin/memory_stick $BASE/memory_stick/ms2.config 32; echo 'MS2 terminó'; read" &
sleep 1
xterm -T "MEMORY STICK 3 (64)" -e bash -c "$BASE/memory_stick/bin/memory_stick $BASE/memory_stick/ms3.config 64; echo 'MS3 terminó'; read" &
sleep 1
xterm -T "MEMORY STICK 4 (128)" -e bash -c "$BASE/memory_stick/bin/memory_stick $BASE/memory_stick/ms4.config 128; echo 'MS4 terminó'; read" &
sleep 2
xterm -T "SCHEDULER" -e bash -c "$BASE/kernel_scheduler/bin/kernel_scheduler $BASE/kernel_scheduler/sched_memoria.config PLANI_MEM.prc; echo 'SCHED terminó'; read" &
sleep 2
xterm -T "IO SLEEP" -e bash -c "$BASE/io/bin/io $BASE/io/sleep.config SLEEP; echo 'SLEEP terminó'; read" &
sleep 1
xterm -T "IO STDIN" -e bash -c "$BASE/io/bin/io $BASE/io/stdin.config STDIN; echo 'STDIN terminó'; read" &
sleep 1
xterm -T "IO STDOUT" -e bash -c "$BASE/io/bin/io $BASE/io/stdout.config STDOUT; echo 'STDOUT terminó'; read" &
sleep 1
xterm -T "CPU 1" -e bash -c "$BASE/cpu/bin/cpu $BASE/cpu/cpu.config 1; echo 'CPU terminó'; read" &

echo "=== Prueba Memoria BEST iniciada (4 sticks: 16+32+64+128) ==="