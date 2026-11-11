# Linux x64 toolchain, so the compiler and the programs it emits run on any host.
# Build and run with --platform linux/amd64 (see `make docker-test`).
FROM alpine:3.20
RUN apk add --no-cache g++ make nasm binutils
WORKDIR /dhad
COPY . .
CMD ["make", "-s", "test"]
