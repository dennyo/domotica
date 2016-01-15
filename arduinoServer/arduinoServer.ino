// Click-on click-off controler 
// example with Action device, old model; system code = 31, device = 'A'
// By Sibbele Oosterhaven, Computer Science NHL, Leeuwarden
// V1.0, 13/12/2015
// Hardware: Arduino Uno, Ethernet shield W5100; RF transmitter on RFpin; debug LED for serverconnection on ledPin
// The Ethernet shield uses pin 10, 11, 12 and 13
// IP address of server is based on DHCP. No fallback to static IP; use a wireless router
// Arduino server and smartphone should be in the same network segment (192.168.1.x)
// 
// Based on https://github.com/evothings/evothings-examples/blob/master/resources/arduino/arduinoethernet/arduinoethernet.ino.
//
// Click-on click-off, Action, new model, codes based on Voorbeelden -> RCSwitch-2-> ReceiveDemo_Simple
//   on      off       
// 1 9321647 9321646
// 2 9321645 9321644
// 3 9321643 9321642

// Include files.
#include <SPI.h>                  // Ethernet shield uses SPI-interface
#include <Ethernet.h>             // Ethernet library
#include <RCSwitch.h>             // Remote Control (Action, new model)

//Set Ethernet Shield MAC address  (check yours)
byte mac[] = { 0x40, 0x6c, 0x8f, 0x36, 0x84, 0x8a }; // Ethernet adapter shield S. Oosterhaven
int ethPort = 3300;                                  // Take a free port (check your router)

#define RFPin        3  // output, pin to control the RF-sender (and Click-On Click-Off-device)
#define lowPin       5  // output, always LOW
#define highPin      6  // output, always HIGH
#define switchPin    7  // input, connected to some kind of inputswitch
#define ledPin       8  // output, led used for "connect state": blinking = searching; continuously = connected
#define infoPin      9  // output, more information
#define analogPin    4  // sensor light value
#define analogPin    3  // sensor temp value

EthernetServer server(ethPort);              // EthernetServer instance (listening on port <ethPort>).
bool pinState1 = false;                   // Stores pinstate of switcht 1
bool pinState2 = false;                   // Stores pinstate of switcht 2
bool pinState3 = false;                   // Stores pinstate of switcht 3
bool pinChange = false;                   // Variable to store actual pin change
int  sensorLightValue = 0;                // Variable to store actual sensor light value
int sensorTempValue = 0;                  // Variable to store actual sensor temp value
RCSwitch mySwitch = RCSwitch();           // declaratie van mySwitch
int activeSwitch = 0;                     // Stores which "stopcontact" to change
bool groupmode = false;                   // Stores if groupmode is active or inactive                  
byte thresholdValue = 80;                  // Tresholdvalue lightsensor (groupassignment)
bool isSwitchOn = false;                  // Checks if switch is on (groupassignment)
bool isDS1 = false;                       // When first in groupassignment
bool playMusic = false;                   // Checks if switch for music is on
bool lightSensor = true;                  // Stores the state of the light sensor(on/off)
bool tempSensor = true;                   // Stores the state of the temp sensor(on/off)
int BPM = 120;                            // The speed of the song
int thisNote = 0;                         // Stores how far the song is
int maxNotes = 0;                         // Stores how many notes are in the chosen song
int melody[] = {                          // The hight of the tone
1319,    1319,    1319,    1319,    1319,    1319,    1319,    1568,    1047,    1175,
1319,    1397,    1397,    1397,    1397,    1397,    1319,    1319,    1319,    1319,
1175,    1175,    1319,    1175,    1568,    1319,    1319,    1319,    1319,    1319,
1319,    1319,    1568,    1047,    1175,    1319,    1397,    1397,    1397,    1397,
1397,    1319,    1319,    1319,    1568,    1568,    1319,    1175,    1047};
int noteDurations[] = {                   // note durations: 4 = quarter note, 8 = eighth note, etc. (duration of each tone):
4, 4, 2, 4, 4, 2, 4, 4, 4, 4, 1, 4, 
4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 
2, 4, 4, 2, 4, 4, 2, 4, 4, 4, 4, 1, 
4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1};
int melody2 [] = {
220, 220, 220, 175, 262, 220, 175, 262, 220, 330, 330, 330, 349, 262,
208, 175, 262, 220, 440, 220, 220, 440, 415, 392, 370, 330, 349, 233,
311, 294, 277, 262, 247, 262, 175, 208, 175, 262, 220, 175, 262, 220};
int noteDurations2 [] = {
4,4,4,5,16,4,5,16,2,4,4,4,5,16,4,5,16,2,4,5,16,4,
5,16,16,16,8,8,4,5,16,16,16,8,8,4,5,16,4,5,16,2};

void setup()
{ 
  mySwitch.enableTransmit(3);

   //Init I/O-pins
   pinMode(switchPin, INPUT);            // hardware switch, for changing pin state
   pinMode(lowPin, OUTPUT);              
   pinMode(highPin, OUTPUT);
   pinMode(RFPin, OUTPUT);
   pinMode(ledPin, OUTPUT);
   pinMode(infoPin, OUTPUT);
   
   //Default states
   digitalWrite(switchPin, HIGH);        // Activate pullup resistors (needed for input pin)
   digitalWrite(lowPin, LOW);
   digitalWrite(highPin, HIGH);
   digitalWrite(RFPin, LOW);
   digitalWrite(ledPin, LOW);
   digitalWrite(infoPin, LOW);

   Serial.begin(9600);

   //Try to get an IP address from the DHCP server.
   if (Ethernet.begin(mac) == 0)
   {
      Serial.println("Could not obtain IP-address");
      while (true){     // no point in carrying on, so do nothing forevermore; check your router
      }
   }

//   Serial.println("Domotica project, Arduino server");
//   Serial.print("RF-transmitter (click-on click-off Device) on pin "); Serial.println(RFPin);
//   Serial.print("LED (for connect-state and pin-state) on pin "); Serial.println(ledPin);
//   Serial.print("Input switch on pin "); Serial.println(switchPin);
//   Serial.println("Ethernetboard connected (pins 10, 11, 12, 13 and SPI)");
//   Serial.println("Connect to DHCP source in local network (blinking led -> waiting for connection)");
   
   //Start the ethernet server.
   server.begin();

   // Print IP -                    address and led indication of server state
   Serial.println("IP address: ");
   Serial.print(Ethernet.localIP());
   
   // for hardware debug: LED indication of server state: blinking = waiting for connection
   int offset = 0; 
   if (getIPClassB(Ethernet.localIP()) == 1) offset = 100;             // router S. Oosterhaven
   int IPnr = getIPComputerNumberOffset(Ethernet.localIP(), offset);   // Get computernumber in local network 192.168.1.105 -> 5)
   Serial.print(" ["); Serial.print(IPnr); Serial.print("] "); 
//   Serial.print("  [Testcase: telnet "); Serial.print(Ethernet.localIP()); Serial.print(" "); Serial.print(ethPort); Serial.println("]");
   signalNumber(ledPin, IPnr);
}

void loop()
{
   // Listen for incomming connection (app)
   EthernetClient ethernetClient = server.available();
   if (!ethernetClient) {
      blink(ledPin);
      return; // wait for connection and blink LED
   }
   Serial.println("Application connected");
   digitalWrite(ledPin, LOW);

   // Do what needs to be done while the socket is connected.
   while (ethernetClient.connected()) 
   {
      checkEvent(switchPin, pinState1);
      checkEvent(switchPin, pinState2);
      checkEvent(switchPin, pinState3);
      if(lightSensor){ sensorLightValue = readSensor(4,100); }
      delay(100);
      if(tempSensor) {int val=analogRead(3); sensorTempValue=((1023 - val) /27.3); }
        
      // Activate pin based op pinState
      if (pinChange) {
        if(activeSwitch==0)
        {
          digitalWrite(ledPin, LOW);
        }   
      else if(activeSwitch==1)
        {
          if (pinState1)mySwitch.send(9321647, 24); // Turn RF-Switch 1 on
          else  mySwitch.send(9321646, 24);  // Turn RF-Switch 1 off
        }
      else if(activeSwitch==2)
        {
          if (pinState2)  mySwitch.send(9321645, 24); // Turn RF-Switch 2 on
          else  mySwitch.send(9321644, 24);  // Turn RF-Switch 2 off
        }
      else if(activeSwitch==3)
        {
          if (pinState3)  mySwitch.send(9321643, 24);  // Turn RF-Switch 3 on
          else  mySwitch.send(9321642, 24);  // Turn RF-Switch 3 off
        }
         activeSwitch = 0;
         pinChange = false;
         delay(100); // delay depends on device
      }

      
    if (groupmode == true)
    {
      if(sensorLightValue >= thresholdValue)  // if sensorlightvalue is higher then the threshold
      {    
        if(!isSwitchOn && isDS1 == false)
        {
          Serial.println("Turn switch ON");
          mySwitch.send(9321647, 24);           // if "stopcontact is not on change to on
          isSwitchOn = true;
        }
        if(isSwitchOn && isDS1 == true) 
        {
          Serial.println("Turn switch OFF");
          mySwitch.send(9321646, 24);           // if "stopcontact is on change to not on
          isSwitchOn = false;
        }
      }
  
      else if(sensorLightValue <= thresholdValue) // if sensorlightvalue is lower then the threshold
      {
        if(isSwitchOn && isDS1 == false)
        {
          Serial.println("Turn switch OFF");
          mySwitch.send(9321646, 24);             // if "stopcontact is on change to not on
          isSwitchOn = false;
        }
      }
      delay(1000);
    }
    if(playMusic)         // if switch music is on
    {
      if(thisNote >= maxNotes)  // if thisNote is higher then the last note, reset.
      {
        thisNote = 0;
        playMusic=false;
      }
    
      else                // else note continues
      { 
        if(maxNotes == 42)
        { play( melody, noteDurations );}
        else if(maxNotes == 49)
        { play( melody2, noteDurations2 ); }
      }
    }
   
    // Execute when byte is received.
    while (ethernetClient.available())
    {
      char inByte = ethernetClient.read();   // Get byte from the client.
      executeCommand(inByte);                // Wait for command to execute
      inByte = NULL;                         // Reset the read byte.
    } 
  }  
  delay(1000);
  Serial.println("App disconnected");
}



// Implementation of (simple) protocol between app and Arduino
// Request (from app) is single char ('a', 's', etc.)
// Response (to app) is 4 chars  (not all commands demand a response)
void executeCommand(char cmd)
{     
         char buf[4] = {'\0', '\0', '\0', '\0'};

         // Command protocol
         Serial.print("["); Serial.print(cmd); Serial.print("] -> ");
         switch (cmd) {
         case '1': // Toggle state of Switch 1; If state is already ON then turn it OFF
            if (pinState1) 
            { 
              isSwitchOn = false; pinState1 = false; Serial.println("Pin 1 state OFF");
              if(groupmode == true){ isDS1=false;}
            }
            else 
            { 
              isSwitchOn = true; pinState1 = true; Serial.println("Pin 1 state ON");
              if(groupmode == true) { isDS1=true;}
            }  
            pinChange = true; 
            activeSwitch = 1;
            break;
         case '2': // Switch 2
            if (pinState2) { pinState2 = false; Serial.println("Pin 2 state OFF"); }
            else { pinState2 = true; Serial.println("Pin 2 state ON"); }  
            pinChange = true; 
            activeSwitch = 2;
            break;
         case '3': // Switch 3
            if (pinState3) { pinState3 = false; Serial.println("Pin 3 state OFF"); }
            else { pinState3 = true; Serial.println("Pin 3 state ON"); }  
            pinChange = true; 
            activeSwitch = 3;
            break;
         case 'a': // Report sensor light value to the app  
            if(lightSensor)
            {
            intToCharBuf(sensorLightValue, buf, 4);                // convert to charbuffer
            server.write(buf, 4);                                  // response is always 4 chars (\n included)           
            }
            else if(!lightSensor) 
            {
              buf[0] = '-';
              server.write(buf,4);
            }
            Serial.print("Sensor: "); Serial.println(buf);
            break;
         case 'b': // Report sensor temp value to the app  
            if(tempSensor)
            {
            intToCharBuf(sensorTempValue, buf, 4);                // convert to charbuffer
            server.write(buf, 4);                                 // response is always 4 chars (\n included)
            }
            else if(!tempSensor) 
            {
              buf[0] = '-';
              server.write(buf,4);
            }     
            Serial.print("Sensor: "); Serial.println(buf);
            break;
         case 'g':
            if (groupmode==true) { groupmode = false; Serial.println("Groupmode OFF"); }
            else { groupmode = true; Serial.println("Groupmode ON"); }  
            break;
         case 'i':    
            digitalWrite(infoPin, HIGH);
            break;
         case 'l':
            if(lightSensor) { lightSensor = false; Serial.println("Lightsensor off"); }
            else { lightSensor = true; Serial.println("Lightsensor on"); }
            break;
         case 'm':
            playMusic = true;
            Serial.println("Start music");
            maxNotes = 49;            
            thisNote = 0;
            break;
         case 'n':
            playMusic = false;
            Serial.println("Stop music");
            thisNote = 0;
            break;
         case 'o':
            playMusic = true;
            Serial.println("Start music");
            thisNote = 0;
            maxNotes = 42;
            break;
         case 'p':
            playMusic = false;
            Serial.println("Stop music");
            thisNote = 0;
            break;
         case 't':
            if(tempSensor) { tempSensor = false; Serial.println("Tempsensor off"); }
            else { tempSensor = true; Serial.println("Tempsensor on"); }
            break;
           default:
           digitalWrite(infoPin, LOW);
         }
}

// read value from pin pn, return value is mapped between 0 and mx-1
int readSensor(int pn, int mx)
{
  return map(analogRead(pn), 0, 1023, 0, mx-1);    
}

// Convert int <val> char buffer with length <len>
void intToCharBuf(int val, char buf[], int len)
{
   String s;
   s = String(val);                        // convert tot string
   if (s.length() == 1) s = "0" + s;       // prefix redundant "0" 
   if (s.length() == 2) s = "0" + s;  
   s = s + "\n";                           // add newline
   s.toCharArray(buf, len);                // convert string to char-buffer
}

// Check switch level and determine if an event has happend
// event: low -> high or high -> low
void checkEvent(int p, bool &state)
{
   static bool swLevel = false;       // Variable to store the switch level (Low or High)
   static bool prevswLevel = false;   // Variable to store the previous switch level

   swLevel = digitalRead(p);
   if (swLevel)
      if (prevswLevel) delay(1);
      else {               
         prevswLevel = true;   // Low -> High transition
         state = true;
         pinChange = true;
      } 
   else // swLevel == Low
      if (!prevswLevel) delay(1);
      else {
         prevswLevel = false;  // High -> Low transition
         state = false;
         pinChange = true;
      }
}

// blink led on pin <pn>
void blink(int pn)
{
  digitalWrite(pn, HIGH); delay(100); digitalWrite(pn, LOW); delay(100);
}

// Visual feedback on pin, based on IP number
// Blink ledpin for a short burst, then blink N times, where N is (related to) IP-number
void signalNumber(int pin, int n)
{
   int i;
   for (i = 0; i < 30; i++)
       { digitalWrite(pin, HIGH); delay(20); digitalWrite(pin, LOW); delay(20); }
   delay(1000);
   for (i = 0; i < n; i++)
       { digitalWrite(pin, HIGH); delay(300); digitalWrite(pin, LOW); delay(300); }
    delay(1000);
}

// Convert IPAddress tot String (e.g. "192.168.1.105")
String IPAddressToString(IPAddress address)
{
    return String(address[0]) + "." + 
           String(address[1]) + "." + 
           String(address[2]) + "." + 
           String(address[3]);
}

// Returns B-class network-id: 192.168.1.105 -> 1)
int getIPClassB(IPAddress address)
{
    return address[2];
}

// Returns computernumber in local network: 192.168.1.105 -> 105)
int getIPComputerNumber(IPAddress address)
{
    return address[3];
}

// Returns computernumber in local network: 192.168.1.105 -> 5)
int getIPComputerNumberOffset(IPAddress address, int offset)
{
    return getIPComputerNumber(address) - offset;
}

void play(int notes[], int durations[]) 
{
    int noteDuration = (1000*(60*4/BPM)) / durations[thisNote];
    tone(7, notes[thisNote],noteDuration);  // pin, which melody note, which duration of the note
    int pause = noteDuration*1.30;
    delay(pause);       //wait until the note is finished
    noTone(7);
    thisNote++;         // Counts a new note
}


