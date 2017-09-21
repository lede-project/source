#!/bin/sh

# switching to /tmp folder
cd /tmp

# creating certificate.pem file
openssl genrsa -out server.key 1024
(echo IT; echo Italy; echo Strambino; echo Arduino Srl; echo SoftwareEngineering; echo Arduino Srl; echo info@arduino.org; echo .; echo .) | openssl req -new -key server.key -out server.csr
cp server.key server.key.org
openssl rsa -in server.key.org -out server.key
openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt
cat server.crt server.key > /etc/ssl/certs/certificate.pem

#removing files
rm server.key server.crt
