FROM quay.io/pypa/manylinux1_x86_64

COPY python/build-wheels.sh /build-wheels.sh

RUN pwd
RUN ls
RUN ls -R /home
RUN chmod +x /build-wheels.sh

ENTRYPOINT ["/build-wheels.sh"]
