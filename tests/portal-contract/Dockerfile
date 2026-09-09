ARG PLAYWRIGHT_VERSION=1.63.0
FROM mcr.microsoft.com/playwright:v${PLAYWRIGHT_VERSION}-noble AS portal-contract

ARG PLAYWRIGHT_VERSION
ENV PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD=1
WORKDIR /work
COPY tests/portal-contract/package.json tests/portal-contract/package-lock.json ./
RUN npm ci --ignore-scripts && \
    [ "$(node -p "require('@playwright/test/package.json').version")" = "$PLAYWRIGHT_VERSION" ]
COPY tests/portal-contract/ ./
RUN chmod 755 /work/run-portal-contract.sh /work/render-readme-media.sh
CMD ["/work/run-portal-contract.sh"]

FROM portal-contract AS media
USER root
RUN apt-get update && \
    apt-get install -y --no-install-recommends ffmpeg && \
    rm -rf /var/lib/apt/lists/*
USER pwuser
