#!/bin/bash

# 1. Atualizar repositórios e instalar dependências
apt update
apt install -y \
	build-essential \
	gcc \
	make \
	libnewt-dev \
	libsqlite3-dev \
	sqlite3 \
	netcat-openbsd \
	libssl-dev

