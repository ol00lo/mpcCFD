# Установка среды — Linux

Перед началом работы необходимо установить:

* Git;
* Docker;
* Visual Studio Code.

## 1. Git
Проверьте, установлен ли Git:
```bash
git --version
```
Если Git не установлен, установите его через пакетный менеджер вашей системы.
Для Ubuntu:
```bash
sudo apt update
sudo apt install git
```
Проверьте установку:
```bash
git --version
```
## 2. Docker
Для установки Docker используйте [официальную документацию Docker](https://docs.docker.com/engine/install/#installation-procedures-for-supported-platforms) для вашей версии Linux.

Проверьте, установлен ли Docker:
```bash
docker --version
```
Также необходимо проверить, что Docker Engine запущен:
```bash
docker info
```
Если команда выполняется без ошибок, Docker готов к работе.

## 3. Visual Studio Code
[Установите Visual Studio Code](https://code.visualstudio.com/download).
После установки проверьте, что редактор запускается корректно.
---
После установки можно перейти к [настройке проекта](setup.md).