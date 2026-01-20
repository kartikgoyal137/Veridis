#!/usr/bin/env bash

set -euo pipefail

CGROUP_ROOT="/sys/fs/cgroup/"
CGROUP_NAME="veridis_managed"
CGROUP_PATH="$CGROUP_ROOT/$CGROUP_NAME"

if !(mountpoint -q /sys/fs/cgroup); then
  echo "cgroup v2 is not mounted at $CGROUP_ROOT"
  exit 1
fi

echo "cgroup v2 detected"

AVAILABLE_CONTROLLERS=$(cat "$CGROUP_ROOT/cgroup.controllers")
ENABLED_CONTROLLERS=$(cat "$CGROUP_ROOT/cgroup.subtree_control")

for ctrl in $AVAILABLE_CONTROLLERS; do 
  if ! echo "$ENABLED_CONTROLLERS" | grep -qw "$ctrl"; then 
    echo "+$ctrl" >> "$CGROUP_ROOT/cgroup.subtree_control"
  fi 
done

echo "Subtree controllers enabled"

if [ ! -d "$CGROUP_PATH" ]; then
    echo "Creating cgroup: $CGROUP_NAME"
    mkdir "$CGROUP_PATH"
else
    echo "Cgroup '$CGROUP_NAME' already exists"
fi

echo "Cgroup setup complete: $CGROUP_PATH"

  
