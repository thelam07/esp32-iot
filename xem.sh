#!/bin/bash
# Mo man hinh Serial de xem ESP32 dang in gi. KHONG nap lai code.
# Cach dung:  ./xem.sh        (Ctrl+C de thoat - khong lam sao ca)
CONG=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -1)
if [ -z "$CONG" ]; then
    echo "Khong thay cong serial. ESP32 chua cam vao may tinh."
    exit 1
fi
echo ">>> Dang xem $CONG  (Ctrl+C de thoat)"
echo ">>> Neu man hinh im lang: board da chay qua setup() roi."
echo ">>> Nhan nut EN tren board de khoi dong lai va xem tu dau."
echo "------------------------------------------------------------"
arduino-cli monitor -p "$CONG" -c baudrate=115200
