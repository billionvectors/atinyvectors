#!/bin/bash
sudo apt-get install -y libomp-dev
# wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg && \
#     echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main" | \
#     sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
#     sudo apt-get update && \
#     sudo apt-get install -y cmake