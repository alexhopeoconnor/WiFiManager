ARG PLAYWRIGHT_VERSION=1.63.0
FROM mcr.microsoft.com/playwright:v${PLAYWRIGHT_VERSION}-noble AS portal-harness

ARG PLAYWRIGHT_VERSION
ENV PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD=1
WORKDIR /work
COPY package.json package-lock.json ./
RUN npm ci --ignore-scripts && \
    [ "$(node -p "require('@playwright/test/package.json').version")" = "$PLAYWRIGHT_VERSION" ]
COPY . ./
RUN chmod 755 /work/run-portal-harness.sh /work/render-readme-media.sh
CMD ["/work/run-portal-harness.sh"]

FROM portal-harness AS media
USER root
RUN apt-get update && \
    apt-get install -y --no-install-recommends ffmpeg && \
    rm -rf /var/lib/apt/lists/*
USER pwuser
