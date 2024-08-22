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
#include <RtcDS3231.h> // DS3231 라이브러리로 변경
#include <LiquidCrystal_I2C.h>
#include <Wire.h>  
#include <DHT.h>
#include <SD.h> // SD 라이브러리 추가

#define CM1107

#define dhtpin 3
#define dhttype DHT22
DHT dht(dhtpin, dhttype);

CM1106_I2C cm1106_i2c;

LiquidCrystal_I2C lcd(0x27, 16, 2);

RtcDS3231<TwoWire> Rtc(Wire); // DS3231 사용을 위한 객체 생성

#define countof(a) (sizeof(a) / sizeof(a[0]))

File myFile; // 파일 객체 생성


String getDateTimeString(const RtcDateTime& dt) {
    char datestring[26];
    snprintf_P(datestring, countof(datestring), PSTR("%02u/%02u %02u:%02u:%02u"), 
    dt.Month(), dt.Day(), dt.Hour(), dt.Minute(), dt.Second());
    return String(datestring);
}


void setup() {
  cm1106_i2c.begin();
  cm1106_i2c.read_serial_number();
  cm1106_i2c.check_sw_version();
  dht.begin();
  Serial.begin(9600);
  lcd.init();        // LCD 초기화
  lcd.backlight();   // 백라이트 켜기
  Wire.begin(); // DS3231을 위한 I2C 통신 시작
  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  Serial.print(getDateTimeString(compiled));
  Serial.println();
  if (!Rtc.IsDateTimeValid())  {
    Rtc.SetDateTime(compiled);  }
  if (!Rtc.GetIsRunning())  {
    Rtc.SetIsRunning(true);  }
  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)  {
  Rtc.SetDateTime(compiled);  }
}

void loop() {
  uint8_t ret = cm1106_i2c.measure_result();
  RtcDateTime now = Rtc.GetDateTime();
  String dateTimeStr = getDateTimeString(now + 8); // 현재 시간을 문자열로 변환합니다.
  Serial.println(dateTimeStr);

  // LCD에 현재 날짜와 시간 출력
  lcd.setCursor(0, 0); // LCD의 첫 번째 줄, 첫 번째 위치로 커서를 설정
  lcd.print(dateTimeStr); // 변환된 날짜와 시간 문자열을 LCD에 출력

  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  lcd.setCursor(0, 1); // 커서 위치 설정  
  lcd.print("P:"); // PPM 레이블 출력
  lcd.print("    ");
  lcd.setCursor(2, 1);
  lcd.print(cm1106_i2c.co2); // PPM 값 출력
  lcd.setCursor(6, 1); // 커서 위치 설정
  lcd.print("|"); // PPM 레이블 출력
  lcd.print(h); // PPM 값 출력
  lcd.setCursor(11, 1); // 커서 위치 설정
  lcd.print("|"); // PPM 레이블 출력  
  lcd.print(t); // PPM 값 출력
 
if (SD.begin(10)) {
    myFile = SD.open("123.csv", FILE_WRITE);
    lcd.setCursor(14, 0); 
    lcd.print("|O");
    myFile.print(dateTimeStr); // 날짜와 시간을 SD 카드에 기록합니다.
    myFile.print(" | PPM : ");
    myFile.print(cm1106_i2c.co2);
    myFile.print(" | RH : ");
    myFile.print(h);
    myFile.print(" | T : ");
    myFile.println(t);
    myFile.close();
} else {
    lcd.setCursor(14, 0); 
    lcd.print("|X");
}
   delay(390);
  }
