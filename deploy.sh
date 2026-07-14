#!/bin/bash
cd ~
git clone https://github.com/sisoputnfrba/so-deploy.git 2>/dev/null
cd so-deploy
./deploy.sh -r=release -p=utils -p=kernel_scheduler -p=kernel_memory -p=cpu -p=memory_stick -p=swap -p=io "tp-2026-1c-cin-panico"
echo "Deploy listo"