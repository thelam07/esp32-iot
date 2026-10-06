#!/bin/bash
# Do xem cam ESP32 vao thi may tinh co "thay" no khong
echo "======================================"
echo " DANG DO... hay CAM CAP VAO BAY GIO"
echo " (tu dong dung sau 30 giay)"
echo "======================================"

truoc=$(lsusb | sort)
for i in $(seq 30); do
    sau=$(lsusb | sort)
    if [ "$truoc" != "$sau" ]; then
        moi=$(diff <(echo "$truoc") <(echo "$sau") | grep '^>' | sed 's/^> //')
        echo ""
        echo ">>> PHAT HIEN THIET BI MOI:"
        echo "    $moi"
        sleep 2
        cong=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null)
        if [ -n "$cong" ]; then
            echo ""
            echo "*** THANH CONG! Day la CAP DATA. ***"
            echo "    Cong serial: $cong"
            ls -l $cong
        else
            echo ""
            echo "!!! Thay chip nhung KHONG tao ra cong serial -> thieu driver"
        fi
        exit 0
    fi
    printf "."
    sleep 1
done

echo ""
echo "*** KHONG THAY GI SAU 30 GIAY ***"
echo "    -> Cap nay CHI SAC, khong truyen data. Doi cap khac."
