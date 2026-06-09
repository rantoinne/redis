# Build stage (Ubuntu)
FROM ubuntu:24.04 AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY Makefile ./
COPY src ./src/

RUN make clean all

# Runtime stage (Ubuntu)
FROM ubuntu:24.04

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /app/build/redis-scratch /usr/local/bin/redis-scratch

ENTRYPOINT ["redis-scratch"]
CMD ["--port", "6379"]
