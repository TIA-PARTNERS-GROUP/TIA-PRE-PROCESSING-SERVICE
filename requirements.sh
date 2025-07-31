#!/bin/bash

libmariadb='sudo pacman -Syu libmariadb-dev'

installs=(
  libmariadb
)

for i in "${!installs[@]}"; do
  ${!installs[i]}
done
