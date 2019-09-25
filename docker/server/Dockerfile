# Pull base image.
FROM intellabs/vdms:base

# Setup entry point
RUN echo '#!/bin/bash' > /start.sh && \
    echo 'cd /vdms'   >> /start.sh && \
    echo 'vdms'       >> /start.sh && \
    chmod 755 /start.sh

EXPOSE 55555
CMD ["/start.sh"]
