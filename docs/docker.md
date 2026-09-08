# Docker

## 1. Создание и запуск контейнера
Создать image:
```bash
docker build -t <image-name> .
```
Запустить
```bash
docker run <image-name>
```
Посмотреть запущенные контейнеры:
```bash
docker ps
```
Посмотреть все контейнеры:
```bash
docker ps -a
```
---

## 2. Работа с контейнером
Остановить контейнер:
```bash
docker stop <container>
```
Запустить остановленный контейнер:
```bash
docker start <container>
```
Удалить контейнер:
```bash
docker rm <container>
```
Зайти внутрь контейнера:
```bash
docker exec -it <container> bash
```
---
## 7. Volumes
Создать volume:
```bash
docker volume create <volume-name>
```
Подключить volume к контейнеру:
```bash
docker run -v <volume-name>:/path/in/container <image>
```
Volume используется для хранения данных независимо от жизненного цикла контейнера.
---
## 8. Docker Compose
Для запуска нескольких связанных контейнеров используется `compose.yaml`.
Запустить в фоновом режиме:
```bash
docker compose up
```
Остановить и удалить созданные Compose-контейнеры:
```bash
docker compose down
```
