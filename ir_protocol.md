Protocol

Reference: [NMEA Protocol](https://en.wikipedia.org/wiki/NMEA_0183)

# รับ IP ที่จะให้ติดต่อ

```
$IRCIP,192,168,1,112*47
        |----------|
          Phone IP
```

# ขอคำสั่ง
```
$IRREQ,00000*41
         |
         --- IR Sent command ID (00000 if no command sent)
```

# ส่งคำสั่ง (จากเครื่องควบคุม)
```
$IRCMD,3,00FAF,RC5,48260482*73
       |   |    |      |
       |   |    |      --- Command HEX code 
       |   |    --- protocol (3 char)
       |   --- Command unique ID (5 alphanumeric)
       --- Remaining command count
```

# Invalid received command
```
$IRIVD*40
```