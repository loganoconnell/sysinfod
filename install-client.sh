#!/bin/bash
set -e

clang -arch arm64 sysinfod_client.c -o sysinfod-client

echo "Successfully uninstalled sysinfod client"
