# Sistema de Votacao Distribuida

Projeto academico em C para Debian Linux, com servidor e cliente em terminal.

## Dependencias

- GCC
- make
- OpenSSL
- Newt
- SQLite3
- pthread

## Gerar certificados

Rode o gerador uma vez na maquina/build machine do servidor:

```bash
chmod +x scripts/generate_certs.sh
SERVER_IP=192.168.101.104 ./scripts/generate_certs.sh
```

Isso cria:
- `certs/server.crt`
- `certs/server.key`
- `certs/ca.crt`
- `client/src/network/embedded_ca.h`

O cliente nao precisa copiar `ca.crt`. A CA e embutida no executavel do cliente no momento da compilacao.

Se o cliente conectar por IP, esse IP precisa entrar em `SERVER_IP` na geracao.

## Servidor

```bash
cd server
make
./build/server
```

## Cliente

Depois de gerar os certificados no mesmo checkout:

```bash
cd client
make
./build/client
```

## Fluxo correto

1. Gerar certificados uma vez.
2. Compilar o cliente com `client/src/network/embedded_ca.h` gerado.
3. Distribuir somente o executavel do cliente.
4. O servidor continua usando `certs/server.crt` e `certs/server.key`; o binario resolve isso a partir do proprio executavel como `../certs/server.crt` e `../certs/server.key`.

## Observacao

`server/src/security/validation.c` nao participa do problema de TLS. Ele valida o voto depois que o handshake SSL ja foi estabelecido.
