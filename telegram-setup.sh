#!/bin/bash
# Lay Chat ID tu bot Telegram va ghi ra file bi_mat.h cho sketch 07.
# Token chi nam tren may ban, khong gui di dau ngoai api.telegram.org.

DICH=/home/lam/iot/07_telegram/bi_mat.h

echo "=============================================="
echo " THIET LAP TELEGRAM CHO ESP32"
echo "=============================================="
echo
read -rp "1. Dan TOKEN cua BotFather vao day: " TOKEN
TOKEN=$(echo "$TOKEN" | tr -d '[:space:]')

if [ -z "$TOKEN" ]; then echo "Chua nhap gi. Dung lai."; exit 1; fi

echo
echo ">>> Dang kiem tra token..."
KQ=$(curl -s -4 --max-time 15 "https://api.telegram.org/bot$TOKEN/getMe")

TEN=$(echo "$KQ" | python3 -c '
import sys, json
try:
    d = json.load(sys.stdin)
except Exception:
    sys.exit(1)
if not d.get("ok"):
    sys.exit(1)
print(d["result"]["username"])
' 2>/dev/null)

if [ -z "$TEN" ]; then
    echo "!!! TOKEN SAI hoac khong goi duoc Telegram."
    echo "    Kiem tra lai: copy du ca phan truoc va sau dau hai cham."
    exit 1
fi
echo "    OK - bot cua ban la: @$TEN"

echo
echo "2. Bay gio mo Telegram, vao bot @$TEN va nhan /start"
echo "   (neu da nhan roi thi cu nhan them mot tin bat ky, vi du: xin chao)"
read -rp "   Nhan xong roi thi go Enter... " _

echo
echo ">>> Dang tim Chat ID..."
CHAT=$(curl -s -4 --max-time 15 "https://api.telegram.org/bot$TOKEN/getUpdates" | python3 -c '
import sys, json
try:
    d = json.load(sys.stdin)
except Exception:
    sys.exit(1)
ids = []
for u in d.get("result", []):
    for k in ("message", "edited_message", "channel_post"):
        if k in u:
            ids.append(str(u[k]["chat"]["id"]))
print(ids[-1] if ids else "")
' 2>/dev/null)

if [ -z "$CHAT" ]; then
    echo "!!! KHONG TIM THAY TIN NHAN NAO."
    echo "    -> Ban da nhan /start cho dung bot @$TEN chua?"
    echo "    -> Nhan mot tin bat ky roi chay lai script nay."
    exit 1
fi
echo "    OK - Chat ID cua ban: $CHAT"

cat > "$DICH" <<HEADER
// File nay chua BI MAT. Duoc tao tu dong boi telegram-setup.sh
// KHONG chia se file nay, khong dua len GitHub.
#pragma once
#define TELEGRAM_TOKEN   "$TOKEN"
#define TELEGRAM_CHAT_ID "$CHAT"
HEADER
chmod 600 "$DICH"

echo
echo ">>> Da ghi vao: $DICH"
echo
echo ">>> Gui thu mot tin nhan..."
curl -s -4 --max-time 15 -X POST \
     "https://api.telegram.org/bot$TOKEN/sendMessage" \
     -d "chat_id=$CHAT" \
     --data-urlencode "text=Ket noi thanh cong! ESP32 se gui canh bao vao day." \
     -o /dev/null -w "    Telegram tra ve HTTP %{http_code}\n"
echo
echo "=============================================="
echo " XONG. Kiem tra Telegram xem co tin nhan chua."
echo "=============================================="
