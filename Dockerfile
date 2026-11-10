
FROM ghcr.io/ps2dev/ps2dev:latest

WORKDIR /src

CMD ["make", "clean", "all"]
