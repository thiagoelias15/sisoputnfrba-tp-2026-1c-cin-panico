#!/bin/bash
BASE=~/so-deploy/tp-2026-1c-cin-panico

IP_MEM=$1
IP_SCHED=$2
IP_SWAP=$3
IP_STICK=$4

echo "Configurando IPs: MEM=$IP_MEM SCHED=$IP_SCHED SWAP=$IP_SWAP STICK=$IP_STICK"

sed -i "s/IP_MEMORIA=.*/IP_MEMORIA=$IP_MEM/" $BASE/kernel_scheduler/sched_*.config
sed -i "s/IP_SWAP=.*/IP_SWAP=$IP_SWAP/" $BASE/kernel_memory/memoria_*.config
sed -i "s/IP_MEMORIA=.*/IP_MEMORIA=$IP_MEM/" $BASE/memory_stick/ms*.config
sed -i "s/IP_ESCUCHA=.*/IP_ESCUCHA=$IP_STICK/" $BASE/memory_stick/ms*.config
sed -i "s/IP_MEMORIA=.*/IP_MEMORIA=$IP_MEM/" $BASE/cpu/cpu.config
sed -i "s/IP_SCHEDULER=.*/IP_SCHEDULER=$IP_SCHED/" $BASE/cpu/cpu.config
sed -i "s/IP_SCHEDULER=.*/IP_SCHEDULER=$IP_SCHED/" $BASE/io/*.config

echo "Listo! Todas las IPs actualizadas"