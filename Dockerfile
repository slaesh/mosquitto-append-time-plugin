FROM eclipse-mosquitto:2.0.13 AS builder

RUN apk add make git gcc g++
RUN git clone --depth 1 --branch v2.0.9 https://github.com/eclipse/mosquitto.git mosquitto-src

COPY ./src mosquitto-src/plugins/append_timestamp

WORKDIR /mosquitto-src/plugins/append_timestamp

RUN make

FROM eclipse-mosquitto:2.0.13 AS prod

COPY --from=builder /mosquitto-src/plugins/append_timestamp/append_timestamp.so .
