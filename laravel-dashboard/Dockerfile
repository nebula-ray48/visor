FROM dunglas/frankenphp:php8.4

# Octane (FrankenPHP) のシグナル処理に必要な拡張
RUN install-php-extensions pcntl

WORKDIR /app
COPY . /app
