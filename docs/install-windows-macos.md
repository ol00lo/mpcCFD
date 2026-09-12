# Установка среды — Windows/macOS

Перед началом работы необходимо установить:

* Git;
* Docker Desktop;
* Visual Studio Code.
## 1. Git
[Установите Git](https://git-scm.com/install/).
После установки откройте PowerShell и проверьте:
```powershell
git --version
```
Если команда выводит версию Git, установка выполнена успешно.
## 2. Docker Desktop
[Установите Docker Desktop](https://docs.docker.com/get-started/get-docker/).
При установке поставьте галочку на Use the WSL 2 based engine. 

---
### WSL 2
Docker Desktop для Windows использует WSL 2 для запуска Linux-контейнеров.

Перед запуском Docker Desktop откройте PowerShell от имени администратора и выполните:
```powershell
wsl --install
```
Windows может попросить перезагрузить компьютер.

---
Проверьте Docker из PowerShell:
```powershell
docker --version
```
## 3. Visual Studio Code
[Установите Visual Studio Code](https://code.visualstudio.com/download).
После установки откройте PowerShell и проверьте:
```powershell
code --version
```
---
После установки можно перейти к [настройке проекта](setup.md).