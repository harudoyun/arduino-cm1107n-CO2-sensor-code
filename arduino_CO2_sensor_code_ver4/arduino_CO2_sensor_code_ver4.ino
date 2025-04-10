/*
*점프 선색*
A4 -> orange
A5 -> yellow
GND -> brown
5V -> Red
        
*RTC moduel*
SDA -> A4 yellow
SCL -> A5 white
VCC -> Red
GND -> Black  

*SD card modeul*
GND : brawn
5V : Red
D10 CS -> Blue
D11 MOSI -> yellow
D12 MISO -> orange
D13 SCK -> green

*CO2 sensor with adapter*
SDA white -> A4 orange
SCL green -> A5 yellow
GND -> Black
5V -> Red


DHT22
DATA (정면기준 2번째 발) D3 -> brawn
5V -> red
GND -> black

LCD
GND -> brawn
5V -> Red
SDA -> A4 orange
SCL -> A5 yellow


*/
#include <cm1106_i2c.h>
#include <RtcDS3231.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <DHT.h>
#include <SD.h>

unsigned long t = 0;
unsigned long prevMillis = 0;

#define CM1107

#define TRANSISTOR_PIN 6  // 트랜지스터 베이스 제어 핀

#define dhtpin 3
#define dhttype DHT22
DHT dht(dhtpin, dhttype);

CM1106_I2C cm1106_i2c;

LiquidCrystal_I2C lcd(0x27, 16, 2);

RtcDS3231<TwoWire> Rtc(Wire);

File myFile;

String fileName; // 동적으로 파일 이름을 관리하기 위한 변수
uint16_t prevDay = 0; // 이전 날짜를 저장하기 위한 변수

// LCD에 표시할 형식의 날짜 및 시간 문자열 반환
String GDTS_LCD(const RtcDateTime& dt) {
    char datestring[30];
    snprintf_P(datestring, countof(datestring), PSTR("%02u-%02u %02u:%02u:%02u"), 
               dt.Month(), dt.Day(), dt.Hour(), dt.Minute(), dt.Second());
    return String(datestring);
}

// SD 카드에 저장할 형식의 날짜 및 시간 문자열 반환
String GDTS_SD(const RtcDateTime& dt) {
    char datestring[30];
    snprintf_P(datestring, countof(datestring), PSTR("%04u-%02u-%02u %02u:%02u:%02u"), 
               dt.Year(), dt.Month(), dt.Day(), dt.Hour(), dt.Minute(), dt.Second());
    return String(datestring);
}

String CFN(const RtcDateTime& dt) {
    char fileNameBuffer[13];
    snprintf(fileNameBuffer, sizeof(fileNameBuffer), "%04u%02u%02u.csv", dt.Year(), dt.Month(), dt.Day());
    return String(fileNameBuffer);
}

void DDL(String dateTimeStr, int co2, float humidity, float temperature) {
    // LCD에 현재 날짜와 시간 출력
    lcd.setCursor(0, 0);
    lcd.print(dateTimeStr);

    // LCD 두 번째 줄에 CO2 농도와 온습도 출력
    lcd.setCursor(0, 1);  // 위치 조정 (0, 1)으로 수정
    lcd.print("P:");
    lcd.print("    ");
    lcd.setCursor(2, 1);  // 위치 조정 (2, 1)으로 수정
    lcd.print(co2);

    lcd.setCursor(6, 1);  // 위치 조정 (6, 1)으로 수정
    lcd.print("|");
    lcd.print(humidity);

    lcd.setCursor(11, 1); // 위치 조정 (11, 1)으로 수정
    lcd.print("|");
    lcd.print(temperature);
}

void LDTS(String dateTimeStr, int co2, float humidity, float temperature) {
    if (SD.begin(10)) {
        File myFile = SD.open(fileName.c_str(), FILE_WRITE);
        if (myFile) {
            if (myFile.size() == 0) {
                // 새 파일의 경우 컬럼 헤더 추가
                myFile.println("DateTime,Co2,Rh,T");
            }
            lcd.setCursor(14, 0);
            lcd.print("|O");

            myFile.print(dateTimeStr);
            myFile.print(",");
            myFile.print(co2);
            myFile.print(",");
            myFile.print(humidity);
            myFile.print(",");
            myFile.println(temperature);

            myFile.close();
        } else {
            lcd.setCursor(14, 0);
            lcd.print("|X");
        }
    } else {
        lcd.setCursor(14, 0);
        lcd.print("|X");
    }
}

void printToSerial(String dateTimeStr, int co2, float humidity, float temperature) {
    Serial.print(dateTimeStr);
    Serial.print(",");
    Serial.print(co2);
    Serial.print(",");
    Serial.print(humidity);
    Serial.print(",");
    Serial.println(temperature);  // 마지막 값은 println으로 개행 포함
}


void setup() {
  Serial.begin(115200);
  cm1106_i2c.begin();
  cm1106_i2c.read_serial_number();
  cm1106_i2c.check_sw_version();
  dht.begin();
  lcd.init();
  lcd.backlight();
  Wire.begin();
  Rtc.Begin();
  pinMode(TRANSISTOR_PIN, OUTPUT);
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  Serial.println("DateTime,Co2,Rh,T");
  if (!Rtc.IsDateTimeValid())  {
    Rtc.SetDateTime(compiled);  }
  if (!Rtc.GetIsRunning())  {
    Rtc.SetIsRunning(true);  }
  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)  {
  Rtc.SetDateTime(compiled);  }
  digitalWrite(TRANSISTOR_PIN, HIGH);
  Serial.println("초기 전압이 공급되었습니다.");
  prevDay = now.Day(); // 초기 날짜 설정
  fileName = CFN(now); // 초기 파일 이름 설정
}

void loop() {
    if (millis() - t > 1000) {
        t = millis();
        uint8_t ret = cm1106_i2c.measure_result();
        RtcDateTime now = Rtc.GetDateTime();

        if (now.Day() != prevDay) { 
            // 날짜가 변경되었을 때 파일 이름 갱신
            prevDay = now.Day();
            fileName = CFN(now);
            Serial.println("DateTime,Co2,Rh,T");
        }

//시긴수정은 (now + x)를 통해 현재시간과 RTC시간을 맞추기
        String lcdDateTimeStr = GDTS_LCD(now + 10);
        String sdDateTimeStr = GDTS_SD(now + 10);
//위 코드에서 (now + x)를 통해 현재시간과 RTC시간을 맞추기

        float h = dht.readHumidity();
        float t = dht.readTemperature();

        // LCD 출력
        DDL(lcdDateTimeStr, cm1106_i2c.co2, h, t);

        // SD 카드에 데이터 저장
        LDTS(sdDateTimeStr, cm1106_i2c.co2, h, t);

        // 시리얼 모니터에 데이터 출력
        printToSerial(sdDateTimeStr, cm1106_i2c.co2, h, t);  // 새로 만든 함수를 호출
    }
  if (Serial.available()) {
    char command = Serial.read();
    
    if (command == '1') {
      // 트랜지스터 ON, 전압 공급
      digitalWrite(TRANSISTOR_PIN, HIGH); // NPN 트랜지스터를 활성화하기 위해 LOW 신호를 줌
      Serial.println("전압이 공급되었습니다.");
    } 
    else if (command == '0') {
      // 트랜지스터 OFF, 전압 차단
      digitalWrite(TRANSISTOR_PIN, LOW); // NPN 트랜지스터를 비활성화하기 위해 HIGH 신호를 줌
      Serial.println("전압이 차단되었습니다.");
    }
  }  
}
