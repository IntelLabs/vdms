# Pull base image.
FROM intellabs/vdms:base

# Setup entry point

RUN echo '#!/bin/bash' > /start.sh
RUN echo 'export LD_LIBRARY_PATH=/vdms/utils:/pmgd/lib:$(find /usr/local/lib/ / -type f -name "*.so" | xargs dirname | sort | uniq | tr "\n" ":")' >> /start.sh
RUN echo 'cd /vdms' >> /start.sh
RUN echo 'rm -rf db' >> /start.sh
RUN echo './vdms' >> /start.sh
RUN chmod 755 /start.sh

EXPOSE 55555
CMD ["/start.sh"]
