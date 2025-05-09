# Build Stage 1

FROM node:22-alpine AS build
WORKDIR /app

RUN corepack enable

COPY package.json ./

# Install dependencies
RUN npm i

# Copy the entire project
COPY . ./
RUN echo "VITE_API_URL=\"localhost:8081\"" > ./.env
RUN echo "VITE_API_ENDPOINT_PREFIX=\"api/\"" >> ./.env
RUN echo "VITE_API_PROTOCOL=\"http://\"" >> ./.env
RUN echo "VITE_API_WSPROTOCOL=\"ws://\"" >> ./.env

# Build the project
RUN NODE_OPTIONS=--max-old-space-size=2048 npm run build

# Build Stage 2

FROM node:22-alpine
WORKDIR /app

# Only `.output` folder is needed from the build stage
COPY --from=build /app/.output/ ./

# Change the port and host
ENV PORT 80
ENV HOST 0.0.0.0

EXPOSE 80

CMD ["node", "/app/server/index.mjs"]
