Protocol

Reference: [NMEA Protocol](https://en.wikipedia.org/wiki/NMEA_0183)

# รับ IP ที่จะให้ติดต่อ (UDP)

```
$IRCIP,192.168.1.112*47
        |----------|
          Phone IP
```

# ขอคำสั่ง (ESP-TCP)
```
$IRREQ,00000*41
         |
         --- IR Sent command ID (00000 if no command sent) คำสั่งล่าสุดที่ทำไปแล้ว
```

# ส่งคำสั่ง (จากเครื่องควบคุม) (Mobile-TCP)
```
$IRCMD,1,00FAF,RC5,48260482,12*73
       |   |    |      |     |
       |   |    |      |     --- nbits (ถ้ามีระบุใน IR Command) หรือ address กรณี panasonic
       |   |    |      --- Command HEX code 
       |   |    --- protocol (3 char)
       |   --- Command unique ID (5 alphanumeric)
       --- Remaining command count (1 ยังเหลือ 0 หมดแล้ว)
$IRCMD,0,00001,NEC,A1DE21DE,32*46
```

## Protocol
- RC5
- RC6
- PAN (Panasonic)
- NEC
- SAM (Samsung)


# ไม่มีข้อมูลให้ทำ :v

```
$IREMP*44
```

# Invalid received command
```
$IRIVD*40
```