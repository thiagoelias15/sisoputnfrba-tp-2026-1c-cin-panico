#!/bin/bash
# Uso: ./cambiar_ips.sh IP_MEMORIA IP_SCHEDULER IP_SWAP IP_STICK

IP_MEM=$1
IP_SCHED=$2
IP_SWAP=$3
IP_STICK=$4

echo "Configurando IPs: MEM=$IP_MEM SCHED=$IP_SCHED SWAP=$IP_SWAP STICK=$IP_STICK"

# Configs del scheduler (IP_MEMORIA)
sed -i "s/IP_MEMORIA=.*/IP_MEMORIA=$IP_MEM/" kernel_scheduler/sched_*.config

# Configs de kernel_memory (IP_SWAP)
sed -i "s/IP_SWAP=.*/IP_SWAP=$IP_SWAP/" kernel_memory/memoria_*.config

# Configs de memory sticks (IP_MEMORIA e IP_ESCUCHA)
sed -i "s/IP_MEMORIA=.*/IP_MEMORIA=$IP_MEM/" memory_stick/ms*.config
sed -i "s/IP_ESCUCHA=.*/IP_ESCUCHA=$IP_STICK/" memory_stick/ms*.config

# Config de CPU (IP_MEMORIA e IP_SCHEDULER)
sed -i "s/IP_MEMORIA=.*/IP_MEMORIA=$IP_MEM/" cpu/cpu.config
sed -i "s/IP_SCHEDULER=.*/IP_SCHEDULER=$IP_SCHED/" cpu/cpu.config

# Config de IO (IP_SCHEDULER)
sed -i "s/IP_SCHEDULER=.*/IP_SCHEDULER=$IP_SCHED/" io/io_*.config

echo "Listo! Todas las IPs actualizadas"