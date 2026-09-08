# Git
## 1. Получить проект
Клонировать репозиторий:
```bash
git clone <repository-url>
cd <project>
```
Посмотреть состояние:
```bash
git status
```
---
## 2. Ветки
Посмотреть ветки:
```bash
git branch
```
Создать новую ветку:

```bash
git branch <branch-name>
```
Переключиться на ветку:
```bash
git switch <branch-name>
```
Удалить локальную ветку:
```bash
git branch -d <branch-name>
```
---
## 3. Работа с изменениями
Посмотреть изменения:
```bash
git diff
```
Добавить файл в индекс:
```bash
git add <file>
```
Добавить все изменения:
```bash
git add .
```
Создать коммит:
```bash
git commit -m "Описание изменений"
```
---
## 4. Синхронизация
Получить изменения с удалённого репозитория:
```bash
git pull
```
Отправить свои коммиты:
```bash
git push
```