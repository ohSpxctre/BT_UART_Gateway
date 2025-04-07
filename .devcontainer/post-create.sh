#!/bin/bash

# Run doc generation script
cp /workspaces/BT_UART_Gateway/generate_docs.sh /usr/local/bin/generate_docs.sh
chmod +x /usr/local/bin/generate_docs.sh
generate_docs.sh
