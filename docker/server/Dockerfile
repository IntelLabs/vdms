# Pull base image.
FROM intellabs/vdms:base

# Setup entry point

RUN echo '#!/bin/bash' > /start.sh && \
    echo 'export LD_LIBRARY_PATH=/vdms/utils:/pmgd/lib:$(find /usr/local/lib/ / -type f -name "*.so" | xargs dirname | sort | uniq | tr "\n" ":")' >> /start.sh && \
    echo 'cd /vdms' >> /start.sh && \
    echo 'rm -rf db' >> /start.sh && \
    echo './vdms' >> /start.sh && \
    chmod 755 /start.sh

EXPOSE 55555
CMD ["/start.sh"]
