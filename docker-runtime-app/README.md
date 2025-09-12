# Upload runtime to Docker Hub

Copy binaries to ./bin

Build the Docker Image

```docker build -t agneskls/agg-service:latest .```

```docker login -u agneskls```

```docker push agneskls/agg-service:latest```


