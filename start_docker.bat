@echo off
echo =========================================================================
echo       KHOI DONG HE THONG IOT DOCKER STACK (MOSQUITTO + NODE-RED)
echo =========================================================================
echo Dang kiem tra trang thai Docker...

docker --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [LOI] May chua cai dat Docker hoac chua them Docker vao PATH!
    pause
    exit /b 1
)

echo Dang khoi chay cac container (Mosquitto MQTT Broker + Node-RED)...
docker compose up -d

if %errorlevel% equ 0 (
    echo.
    echo =========================================================================
    echo [THANH CONG] Toan bo he thong Docker Stack da san sang!
    echo.
    echo   1. Node-RED Flow Editor : http://localhost:1880
    echo   2. Web Dashboard UI     : http://localhost:1880/ui
    echo   3. Mosquitto MQTT Broker: localhost:1883 (TCP) / 9001 (WebSockets)
    echo =========================================================================
) else (
    echo.
    echo [CHU Y] Neu gap loi Docker daemon chua chay, vui long bat Docker Desktop truoc.
)
pause
