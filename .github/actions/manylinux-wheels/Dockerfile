FROM quay.io/pypa/manylinux1_x86_64

RUN ls
RUN pwd
RUN chmod +x python/build-wheels.sh

ENTRYPOINT ["/python/build-wheels.sh"]
