#!/bin/bash

docker_commands=(
  "sudo docker buildx build tia-pre-processing-service ."

  "sudo docker run --rm --add-host=host.docker.internal:host-gateway tia-pre-processing-service"
)

for i in "${!docker_commands[@]}"; do
  ${docker_commands[i]}
done
