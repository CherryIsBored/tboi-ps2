
FROM ghcr.io/ps2dev/ps2dev:latest

RUN apk add --no-cache make

WORKDIR /src

CMD ["make", "clean", "all"]
