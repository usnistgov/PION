#!/bin/bash
set -euo pipefail

if ! [[ -f .env ]]; then
  cp sample.env .env
fi

mkdir -p runtime
