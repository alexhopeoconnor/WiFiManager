ARG PLAYWRIGHT_VERSION=1.63.0
FROM mcr.microsoft.com/playwright:v${PLAYWRIGHT_VERSION}-noble

ARG PLAYWRIGHT_VERSION
ENV PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD=1
WORKDIR /work
COPY tests/portal-contract/package.json tests/portal-contract/package-lock.json ./
RUN npm ci --ignore-scripts && \
    [ "$(node -p "require('@playwright/test/package.json').version")" = "$PLAYWRIGHT_VERSION" ]
COPY tests/portal-contract/ ./
CMD ["npm", "test"]
