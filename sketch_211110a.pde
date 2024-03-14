import processing.serial.*;
import cc.arduino.*;


Arduino arduino;

PImage img;
PImage onimg;
PImage offimg;




int sampleWindow = 50; // 샘플링 시간(50 mS = 20Hz)
int sample;

int BluePin = 9;
int RedPin = 11;
int GreenPin = 10;

int onePin=2;
int twoPin=3;
int threePin=4;

int i=0;
int ledPin01=13;
int ledPin02=7;
int ledPin03=6;
int ledPin04=8;
int ledPin05=5;
int ledPin06=12;
boolean cnt01 = false;
boolean cnt02 = false;
boolean cnt03 = false;
boolean cnt04 = false;
boolean cnt05 = false;
boolean cnt06 = false;
void setup() {  // this is run once.   
     delay(1000);
     println(Arduino.list());   
     arduino = new Arduino(this, Arduino.list()[0], 57600);
     arduino.pinMode(ledPin01, Arduino.OUTPUT);
     arduino.pinMode(ledPin02, Arduino.OUTPUT);
     arduino.pinMode(ledPin03, Arduino.OUTPUT);
     arduino.pinMode(ledPin04, Arduino.OUTPUT);
     arduino.pinMode(ledPin05, Arduino.OUTPUT);
     arduino.pinMode(ledPin06, Arduino.OUTPUT);
     arduino.pinMode(RedPin, Arduino.OUTPUT); 
     arduino.pinMode(GreenPin, Arduino.OUTPUT); 
     arduino.pinMode(BluePin, Arduino.OUTPUT); 
     arduino.pinMode(onePin, Arduino.OUTPUT);
     arduino.pinMode(twoPin, Arduino.OUTPUT);
     arduino.pinMode(threePin, Arduino.OUTPUT);

    // set the background color
    background(255);
    
    // canvas size (Integers only, please.)
    size(1280, 720); 
    img = loadImage("fix_interface.png");
    onimg = loadImage("onimg.png");
    offimg = loadImage("offimg.png");

  textSize(30);//debug
  fill(255,255,255);//debug
    
} 

void draw() {  // this is run repeatedly.  
    image(img, 0, 0, 1280, 720);
    image(offimg, 460, 175, 50, 20);
    image(offimg, 460, 315, 50, 20);
    image(offimg, 460, 455, 50, 20);
    image(offimg, 460, 595, 50, 20);
    image(offimg, 900, 35, 50, 20);
    image(offimg, 900, 175, 50, 20);
    image(offimg, 900, 315, 50, 20);
    image(offimg, 900, 455, 50, 20);
    image(offimg, 900, 595, 50, 20);
    //arduino.digitalWrite(onePin,Arduino.HIGH);
   long startMillis= millis();  // 샘플링 시작
   int peakToPeak = 0;   // 음성신호의 진폭
   int signalMax = 0;    // 최대크기 초기값 0
   int signalMin = 1024; // 최소크기 초기값 1024
   
   int displayPeak = 0;
   print("%d",sample);
    while (millis() - startMillis < sampleWindow) // 50ms 마다 데이터 수집
   {
      sample = arduino.analogRead(0);  // 마이크 증폭모듈로부터 받아오는 아날로그 값 저장
      if (sample < 1024)  // 값이 1024 보다 작을때
      {
         if (sample > signalMax) // 0보다 크면 
         {
            signalMax = sample;  // signalMax에 저장
         }
         else if (sample < signalMin) // 1024보다 작으면
         {
            signalMin = sample;  // signalMin에 저장
         }
      }
   }

   
   peakToPeak = signalMax - signalMin;  // 최대- 최소 = 진폭값
   double volts = (peakToPeak * 5.0) / 1024;  // // 전압 단위로 변환 = 소리 크기로 변환

   /*rial.print("\t smaple : ");
   Serial.print(sample);
   Serial.print("\t volts  : ");
   Serial.println(volts);
   Serial.print("i : ");
   Serial.print(i);*/
   
   
     
    if(cnt01==true){
      image(onimg, 240, 35, 50, 20);//on image
      arduino.digitalWrite(ledPin01, Arduino.HIGH);
      if (sample >= 0 && sample <200){//main led
        //arduino.digitalWrite(GreenPin, Arduino.HIGH);
        arduino.digitalWrite(BluePin, Arduino.HIGH);
        arduino.digitalWrite(RedPin, Arduino.LOW);
   }

   /*else if(sample >= 180 && sample <200){
     arduino.digitalWrite(GreenPin, Arduino.LOW);
     arduino.digitalWrite(BluePin, Arduino.HIGH);
     arduino.digitalWrite(RedPin, Arduino.LOW);
    //delay(500);
   }*/
   else if( sample >= 200){
     //arduino.digitalWrite(GreenPin, Arduino.LOW);
     arduino.digitalWrite(BluePin, Arduino.LOW);
     arduino.digitalWrite(RedPin, Arduino.HIGH);
   i=i+1;
   delay(2000);
   }
      if (i==1){//counting led
    arduino.digitalWrite(onePin, Arduino.HIGH);
    ellipse(200,77,20,20);
    fill(255,0,0);
   }
   else if(i==2){
    arduino.digitalWrite(twoPin, Arduino.HIGH);
    ellipse(200,77,20,20);
    fill(255,0,0);
    ellipse(230,77,20,20);
    fill(255,0,0);
   }
   else if (i>=3){
    arduino.digitalWrite(threePin, Arduino.HIGH);
    ellipse(200,77,20,20);
    fill(255,0,0);
    ellipse(230,77,20,20);
    fill(255,0,0);
    ellipse(260,77,20,20);
    fill(255,0,0);
   }
}
    if(cnt02==true){
      image(onimg, 240, 175, 50, 20);
      arduino.digitalWrite(ledPin02, Arduino.HIGH);
}
    if(cnt03==true){
      image(onimg, 240, 315, 50, 20);
      arduino.digitalWrite(ledPin03, Arduino.HIGH);
}
    if(cnt04==true){
      image(onimg, 240, 455, 50, 20);
      arduino.digitalWrite(ledPin04, Arduino.HIGH);
}
    if(cnt05==true){
      image(onimg, 240, 595, 50, 20);
      arduino.digitalWrite(ledPin05, Arduino.HIGH);
}
    if(cnt06==true){
      image(onimg, 460, 35, 50, 20);
      arduino.digitalWrite(ledPin06, Arduino.HIGH);
}
    if(cnt01==false){
      image(offimg, 240, 35, 50, 20);
      arduino.digitalWrite(ledPin01, Arduino.LOW);
      arduino.digitalWrite(RedPin, Arduino.LOW);
      arduino.digitalWrite(GreenPin, Arduino.LOW);
      arduino.digitalWrite(BluePin, Arduino.LOW);
      arduino.digitalWrite(onePin, Arduino.LOW);
      arduino.digitalWrite(twoPin, Arduino.LOW);
      arduino.digitalWrite(threePin, Arduino.LOW);
      i=0;
}
    if(cnt02==false){
      image(offimg, 240, 175, 50, 20);
      arduino.digitalWrite(ledPin02, Arduino.LOW);
}
    if(cnt03==false){
      image(offimg, 240, 315, 50, 20);
      arduino.digitalWrite(ledPin03, Arduino.LOW);
}
    if(cnt04==false){
      image(offimg, 240, 455, 50, 20);
      arduino.digitalWrite(ledPin04, Arduino.LOW);
}
    if(cnt05==false){
      image(offimg, 240, 595, 50, 20);
      arduino.digitalWrite(ledPin05, Arduino.LOW);
}
    if(cnt06==false){
      image(offimg, 460, 35, 50, 20);
      arduino.digitalWrite(ledPin06, Arduino.LOW);
}
}

void mousePressed() {
  //text(mouseX,10,550);//debug
  //text(mouseY,80,550);//debug
  
if (mouseX>140 && mouseX<190 && mouseY>100 && mouseY<125) {
  //digitalWrite(ledPin01,HIGH);
  cnt01 = true;
}// 01start
if (mouseX>220 && mouseX<270 && mouseY>100 && mouseY<125){
  //digitalWrite(ledPin01,LOW);
  cnt01 = false;
}// 01end

   

if (mouseX>140 && mouseX<190 && mouseY>240 && mouseY<265){
  //digitalWrite(ledPin02,HIGH);
  cnt02 = true;
}// 02start
if (mouseX>220 && mouseX<270 && mouseY>240 && mouseY<265){
  //digitalWrite(ledPin02,LOW);
  cnt02 = false;
}// 02end

if (mouseX>140 && mouseX<190 && mouseY>380 && mouseY<405){
  //digitalWrite(ledPin03,HIGH);
  cnt03 = true;
}// 03start
if (mouseX>220 && mouseX<270 && mouseY>380 && mouseY<405){
  //digitalWrite(ledPin03,LOW);
  cnt03 = false;
}// 03end


if (mouseX>140 && mouseX<190 && mouseY>520 && mouseY<545){
  //digitalWrite(ledPin04,HIGH);
  cnt04 = true;
}// 04start
if (mouseX>220 && mouseX<270 && mouseY>520 && mouseY<545){
  //digitalWrite(ledPin04,LOW);
  cnt04 = false;
}// 04end

if (mouseX>140 && mouseX<190 && mouseY>660 && mouseY<685){
  //digitalWrite(ledPin05,HIGH);
  cnt05 = true;
}// 05start
if (mouseX>220 && mouseX<270 && mouseY>660 && mouseY<685){
  //digitalWrite(ledPin05,LOW);
  cnt05 = false;
}// 05end


if (mouseX>360 && mouseX<410 && mouseY>100 && mouseY<125){
  //digitalWrite(ledPin06,HIGH);
  cnt06 = true;
}// 06start
if (mouseX>440 && mouseX<490 && mouseY>100 && mouseY<125){
  //digitalWrite(ledPin06,LOW);
  cnt06 = false;
}// 06end

}
