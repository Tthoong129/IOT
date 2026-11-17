@echo off
docker compose up -d
if %errorlevel% equ 0 (
    echo [OK] Containers started.
    echo Dashboard: http://localhost:1880/ui
    echo Node-RED:  http://localhost:1880
)
