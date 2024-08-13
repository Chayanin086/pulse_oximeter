#include <Timer.h> // ดึงข้อมูลจาก Library ของ Timer มาใช้งาน

#include <MAX30105.h> // ดึงข้อมูลจาก Library ของ MAX30105 มาใช้งาน
#include <heartRate.h> ดึงข้อมูลจาก Library ของ HeartRate มาใช้งาน

//เรียกใช้งาน library wifi
#include <ESP8266WiFi.h> // ดึงข้อมูลจาก Board ของ ESP8266WiFi มาใช้งาน
#include <ESP8266HTTPClient.h>// ดึงข้อมูลจาก Board ของ ESP8266HTTPClient มาใช้งาน

char ssid[] = "Tayuguwa" ; // ตั้งค่า User ของ WiFi
char pass[] = "1234567890" ; // ตั้งค่า Password ของ WiFi

MAX30105 sensorOximeter ; // ดึงฟังก์ชันของ MAX30105 ออกมาใช้ โดยตั้งชื่อเป็น seneorOximeter

//สร้างตัวแปรต่างๆเพื่อจัดเก็บข้อมูล
const byte RATE_SIZE = 4 ;
byte rates[RATE_SIZE] ;
byte rateSpot = 0 ;
long lastBeat = 0 ;

float beatPerMinute ;
int beatAvg ; //avg 2 TimePerMinute

//สร้างตัวแปรเวลา t
Timer t ;

//เตรียมตัวแปรสำหรับจัดเก็บข้อมูลที่ใช้สำหรับการส่งข้อมุล
String datasent ;

//สร้างตัวแปรจัดเก็บ IP Address
const char* server = "192.168.122.148" ;

void setup() {

  Serial.begin(115200) ;

  // ตั้งค่าหาก MAX30105 กำลังทำงานอยู่
	// หาก MAX30105 ไม่ทำงาน
  if (!sensorOximeter.begin(Wire , I2C_SPEED_FAST))
  {
    // ให้ปริ้น MAX30105 detect nothing , Check circuit pls.
    Serial.println("MAX30105 detect nothing , Check circuit pls.") ;
    // จากนั้นทำการวนลูปเพื่อการตรวจสอบการทำงาน
    while(1) ; //วนลูปเพื่อทำการเช็คตลอดเวลาว่ามีนิ้วกดอยู่รึเปล่า
  }

  // ถ้า MAX30105 ทำงาน ให้ปริ้น Please place your finger lightly on the sensor.
  Serial.println("Please place your finger lightly on the sensor.") ;

  // ทำการตั้งค่า Sensor Oximeter
  sensorOximeter.setup() ;
  sensorOximeter.setPulseAmplitudeRed(0x0A) ; //Activate Red LED
  sensorOximeter.setPulseAmplitudeGreen(0) ; //Turn off Green LED
  sensorOximeter.enableDIETEMPRDY(); //Enable tmp

  // ตั้งค่าการเชื่อมต่อ WiFi
  WiFi.begin(ssid,pass) ;
  
  //หากไม่สามารถเชื่อมต่อ WiFi ได้
  while(WiFi.status() != WL_CONNECTED)
  {
    // ปริ้น . ทุกๆ 0.5 วินาที จนกว่าจะเชื่อมต่อ
    Serial.print(".") ;
    delay(500) ;
  }

  //หากเชื่อมต่อแล้ว ให้ปริ้น Connected to WiFi
  Serial.println("Connected to WiFi") ;

  // ส่งข้อมูลใหม่ไปทุก 2 วินาที
  t.every(2000 , sentdata) ;

}

//สร้างฟังก์ชันสำหรับการส่งข้อมูล
void sentdata()
  {
    //สร้างตัวแปร client เพื่อเชื่อมต่อกับ Server
    WiFiClient client ;
    int port = 80 ;

    //เช็คการเชื่อต่อกับ webserver
    if(!client.connect(server,port))
    {
      //ถ้าเชื่อมต่อไม่สำเร็จ ให้ปริ้น Connection Failed
      Serial.println("Connection Failed") ;
      return ;
    }

    //ถ้าสามารถเชื่อมต่อได้

    //ส่งข้อมูลไปยัง Data Base
    String Link ;
    HTTPClient http ;

    //เติมตัวแปรสำหรับที่อยู่ของฐานข้อมูลลงใน Webserver 
    Link = "http://" + String(server) + "/webjantung/sentdata.php?data=" + datasent ;

    //ใช้งานตัวแปร Link
    http.begin(Link) ;

    //โหมด GET
    http.GET() ;

    //เช็คการตอบกลับของ Webserver
    String respon = http.getString() ;
    Serial.println(respon) ;
    http.end() ; 

  }

void loop() {
  
  //อัพเดตตัวแปร timer
  t.update() ;

  //read tmp
  float temperature = particleSensor.readTemperature();

  //อ่าน Sensor อินฟาเรดเพื่อตรวจจับนิ้ว 
  long irValue = sensorOximeter.getIR() ;
  if(checkForBeat(irValue) == true )
  {

    //ใช้งาน millis
    long delta = millis() - lastBeat ;
    lastBeat = millis() ;

    //นับจังหวะต่อ 1 นาที (BPM)
    beatPerMinute = 60 / ( delta / 1000.0 ) ;

    //ทดสอบ BPM
    if(beatPerMinute < 255 && beatPerMinute > 20 ) 
    {
      rates[rateSpot++] = (byte)beatPerMinute ;
      rateSpot %= RATE_SIZE ; //กำจัดตัวแปรทิ้ง

      //อ่านค่าเฉลี่ย 2 bpm
      beatAvg = 0 ;
      for(byte x = 0 ; x < RATE_SIZE ; x++ )
      {
        //รวมค่า bpm ทั้งหมดไว้ใน array
        //beatAvg += beatAvg + rates[x] ;
        beatAvg += rates[x] ;

      }

      beatAvg /= RATE_SIZE ;

    }
  }

  //แสดงค่า Sensor ออกมาที่ Serial Monitor
  Serial.print("IR = " + String(irValue)) ;
  Serial.print(", BPM " + String(beatPerMinute)) ;
  Serial.print(", AVG BPM = " + String(beatAvg)) ;
  Serial.print("temperatureC=");
  Serial.print(temperature, 4);

  //เช็คนิ้วบน Sensor ถ้าหากมีค่าน้อยกว่า 50,000 ให้ปริ้น , No Finger
  if(irValue < 50000 ) 
  {
    Serial.print(", No Finger") ;
    datasent = "No Finger" ;
    beatPerMinute = 0 ;
    beatAvg = 0 ;
  }
  else //ถ้ามากกว่า 50,000 จะเริ่มทำการนับจังหวะ
  {
    datasent = String(beatAvg) ;
  }

  Serial.println() ;

}
