FROM quay.io/pypa/manylinux1_x86_64

RUN pwd
RUN ls
RUN ls -R /home
RUN ls /github
RUN chmod +x python/build-wheels.sh

ENTRYPOINT ["/python/build-wheels.sh"]
