#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

CERT_DIR="$ROOT_DIR/certs"
SERVER_DNS="${SERVER_DNS:-}"
SERVER_IP="${SERVER_IP:-}"

echo "Gerando certificados SSL/TLS para o sistema de votacao..."

if [[ -z "$SERVER_IP" ]]; then
    echo "Aviso: SERVER_IP nao foi definido. O certificado do servidor vai ficar valido apenas para localhost e 127.0.0.1."
fi

mkdir -p "$CERT_DIR"

rm -f \
    "$CERT_DIR/ca.key" \
    "$CERT_DIR/ca.crt" \
    "$CERT_DIR/ca.srl" \
    "$CERT_DIR/server.key" \
    "$CERT_DIR/server.crt" \
    "$CERT_DIR/server.csr" \
    "$CERT_DIR/ca.cnf" \
    "$CERT_DIR/server.cnf"

cat > "$CERT_DIR/ca.cnf" <<'EOF'
[req]
distinguished_name = dn
x509_extensions = v3_ca
prompt = no

[dn]
C = BR
ST = SP
L = Sao Paulo
O = SistemaVotacao
OU = CA
CN = SistemaVotacaoCA

[v3_ca]
basicConstraints = critical,CA:TRUE
keyUsage = critical,keyCertSign,cRLSign
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always,issuer
EOF

{
    cat <<'EOF'
[req]
distinguished_name = dn
req_extensions = v3_req
prompt = no

[dn]
C = BR
ST = SP
L = Sao Paulo
O = SistemaVotacao
OU = Server
CN = localhost

[v3_req]
basicConstraints = critical,CA:FALSE
keyUsage = critical,digitalSignature,keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
IP.1 = 127.0.0.1
EOF
    if [[ -n "$SERVER_DNS" && "$SERVER_DNS" != "localhost" ]]; then
        printf 'DNS.2 = %s\n' "$SERVER_DNS"
    fi

    if [[ -n "$SERVER_IP" && "$SERVER_IP" != "127.0.0.1" ]]; then
        printf 'IP.2 = %s\n' "$SERVER_IP"
    fi
} > "$CERT_DIR/server.cnf"

openssl genrsa -out "$CERT_DIR/ca.key" 4096

openssl req \
    -new \
    -x509 \
    -sha256 \
    -days 365 \
    -key "$CERT_DIR/ca.key" \
    -out "$CERT_DIR/ca.crt" \
    -config "$CERT_DIR/ca.cnf"

openssl genrsa -out "$CERT_DIR/server.key" 2048

openssl req \
    -new \
    -key "$CERT_DIR/server.key" \
    -out "$CERT_DIR/server.csr" \
    -config "$CERT_DIR/server.cnf"

openssl x509 \
    -req \
    -sha256 \
    -days 365 \
    -in "$CERT_DIR/server.csr" \
    -CA "$CERT_DIR/ca.crt" \
    -CAkey "$CERT_DIR/ca.key" \
    -CAcreateserial \
    -out "$CERT_DIR/server.crt" \
    -extfile "$CERT_DIR/server.cnf" \
    -extensions v3_req

CLIENT_EMBEDDED_CA_HEADER="$ROOT_DIR/client/src/network/embedded_ca.h"

{
    echo '#ifndef EMBEDDED_CA_H'
    echo '#define EMBEDDED_CA_H'
    echo ''
    echo 'static const char EMBEDDED_CA_PEM[] ='
    awk '
        {
            gsub(/\\/, "\\\\");
            gsub(/"/, "\\\"");
            printf "\"%s\\n\"\n", $0;
        }
    ' "$CERT_DIR/ca.crt"
    echo ';'
    echo ''
    echo '#endif'
} > "$CLIENT_EMBEDDED_CA_HEADER"

rm -f \
    "$CERT_DIR/server.csr" \
    "$CERT_DIR/ca.srl" \
    "$CERT_DIR/ca.cnf" \
    "$CERT_DIR/server.cnf"

chmod 600 "$CERT_DIR/server.key"
chmod 600 "$CERT_DIR/ca.key"
chmod 644 "$CERT_DIR/server.crt"
chmod 644 "$CERT_DIR/ca.crt"

echo "Certificados gerados com sucesso!"
echo "Servidor: certs/server.crt e certs/server.key"
echo "CA confiavel do cliente: compilada em client/src/network/embedded_ca.h"
