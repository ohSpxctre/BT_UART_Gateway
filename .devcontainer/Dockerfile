ARG DOCKER_TAG=v5.4.1
FROM espressif/idf:${DOCKER_TAG}

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

# Setup the ESP-IDF environment
RUN apt-get update -y && apt-get install -y \
    udev \
    doxygen \
    graphviz

# Source the ESP-IDF environment on shell startup
RUN echo "source /opt/esp/idf/export.sh > /dev/null 2>&1" >> ~/.bashrc

# Set Git config
RUN git config --global core.autocrlf true

# Entry point for ESP-IDF container
ENTRYPOINT [ "/opt/esp/entrypoint.sh" ]
CMD ["/bin/bash", "-c"]
