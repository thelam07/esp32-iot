#!/bin/bash
# Nap code vao ESP32.  Cach dung:  ./nap.sh 01_blink
# Sau khi nap xong tu dong mo man hinh Serial (Ctrl+C de thoat).
set -e

FQBN=esp32:esp32:esp32doit-devkit-v1
THUMUC="$1"

if [ -z "$THUMUC" ]; then
    echo "Thieu ten thu muc. Vi du:  ./nap.sh 01_blink"
    echo "Co san: 01_blink  02_dht_test  03_wifi_test  04_webserver"
    exit 1
fi

CONG=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -1)
if [ -z "$CONG" ]; then
    echo "KHONG TIM THAY CONG SERIAL."
    echo "  -> ESP32 chua cam, hoac cap chi sac khong truyen data."
    echo "  -> Chay ./test-cap.sh de kiem tra cap."
    exit 1
fi
echo ">>> Dung cong: $CONG"

echo ">>> Dang bien dich..."
arduino-cli compile -b "$FQBN" "$THUMUC"

echo ">>> Dang nap vao board..."
arduino-cli upload -b "$FQBN" -p "$CONG" "$THUMUC"

echo ""
echo ">>> NAP XONG. Mo man hinh Serial (Ctrl+C de thoat):"
echo "-----------------------------------------------------"
arduino-cli monitor -p "$CONG" -c baudrate=115200
