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
#include <RemoteTransmitter.h>    // Remote Control (Action, old model)
#include <RCSwitch.h>             // Remote Control (Action, new model)

//Set Ethernet Shield MAC address  (check yours)
byte mac[] = { 0x40, 0x6c, 0x8f, 0x36, 0x84, 0x8a }; // Ethernet adapter shield S. Oosterhaven
int ethPort = 3300;                                  // Take a free port (check your router)

#define RFPin        4  // output, pin to control the RF-sender (and Click-On Click-Off-device)
#define lowPin       5  // output, always LOW
#define highPin      6  // output, always HIGH
#define switchPin    7  // input, connected to some kind of inputswitch
#define ledPin       8  // output, led used for "connect state": blinking = searching; continuously = connected
#define infoPin      9  // output, more information
#define analogPin    0  // sensor value

EthernetServer server(ethPort);              // EthernetServer instance (listening on port <ethPort>).
ActionTransmitter actionTransmitter(RFPin);  // Intantiate a new ActionTransmitter remote, old model, use pin <RFPin>

bool action1on = false;
bool action2on = false;
bool action3on = false;
bool pinState = false;                   // Variable to store actual pin state
bool pinChange = false;                  // Variable to store actual pin change
int  sensorValue = 0;                    // Variable to store actual sensor value
RCSwitch mySwitch = RCSwitch(); // declaratie van mySwitch
int activeSwitch = 0;


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
      Serial.println("Could not obtain IP-address from DHCP -> do nothing");
      while (true){     // no point in carrying on, so do nothing forevermore; check your router
      }
   }

   Serial.println("Domotica project, Arduino server");
   Serial.print("RF-transmitter (click-on click-off Device) on pin "); Serial.println(RFPin);
   Serial.print("LED (for connect-state and pin-state) on pin "); Serial.println(ledPin);
   Serial.print("Input switch on pin "); Serial.println(switchPin);
   Serial.println("Ethernetboard connected (pins 10, 11, 12, 13 and SPI)");
   Serial.println("Connect to DHCP source in local network (blinking led -> waiting for connection)");
   
   //Start the ethernet server.
   server.begin();

   // Print IP-address and led indication of server state
   Serial.print("Listening address: ");
   Serial.print(Ethernet.localIP());
   
   // for hardware debug: LED indication of server state: blinking = waiting for connection
   int offset = 0; 
   if (getIPClassB(Ethernet.localIP()) == 1) offset = 100;             // router S. Oosterhaven
   int IPnr = getIPComputerNumberOffset(Ethernet.localIP(), offset);   // Get computernumber in local network 192.168.1.105 -> 5)
   Serial.print(" ["); Serial.print(IPnr); Serial.print("] "); 
   Serial.print("  [Testcase: telnet "); Serial.print(Ethernet.localIP()); Serial.print(" "); Serial.print(ethPort); Serial.println("]");
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
      checkEvent(switchPin, pinState);
      sensorValue = readSensor(0, 100); 
        
      // Activate pin based op pinState
      if (pinChange) {
        if(activeSwitch==1)
        {
          if (pinState) { digitalWrite(ledPin, HIGH); mySwitch.send(9321647, 24); action1on = true; } // Turn RF-Switch 1 on
          else { digitalWrite(ledPin, LOW); mySwitch.send(9321646, 24); action1on = false; }  // Turn RF-Switch 1 off
        }
      else if(activeSwitch==2)
        {
          if (pinState) { digitalWrite(ledPin, HIGH); mySwitch.send(9321645, 24); action2on = true; } // Turn RF-Switch 2 on
          else { digitalWrite(ledPin, LOW); mySwitch.send(9321644, 24); action2on = false; }  // Turn RF-Switch 2 off
        }
      else if(activeSwitch==3)
        {
          if (pinState) { digitalWrite(ledPin, HIGH); mySwitch.send(9321643, 24); action3on = true; } // Turn RF-Switch 3 on
          else { digitalWrite(ledPin, LOW); mySwitch.send(9321642, 24); action3on = false; }  // Turn RF-Switch 3 off
        }
         pinChange = false;
         activeSwitch = 0;
         delay(100); // delay depends on device
      }
   
      // Execute when byte is received.
      while (ethernetClient.available())
      {
         char inByte = ethernetClient.read();   // Get byte from the client.
         executeCommand(inByte);                // Wait for command to execute
         inByte = NULL;                         // Reset the read byte.
      } 
   }
   Serial.println("Application disonnected");
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
         case 'a': // Report sensor value to the app  
            intToCharBuf(sensorValue, buf, 4);                // convert to charbuffer
            server.write(buf, 4);                             // response is always 4 chars (\n included)
            Serial.print("Sensor: "); Serial.println(buf);
            break;
         case 's': // Report switch state to the app
            if (pinState) { server.write(" ON\n"); Serial.println("Pin state is ON"); }  // always send 4 chars
            else { server.write("OFF\n"); Serial.println("Pin state is OFF"); }
            break;
         case 'x': // Toggle state of Switch 1; If state is already ON then turn it OFF
            if (pinState) { pinState = false; Serial.println("Set pin 1 state to \"OFF\""); }
            else { pinState = true; Serial.println("Set pin 1 state to \"ON\""); }  
            pinChange = true; 
            activeSwitch = 1;
            break;
         case 'y': // Switch 2
            if (pinState) { pinState = false; Serial.println("Set pin 2 state to \"OFF\""); }
            else { pinState = true; Serial.println("Set pin 2 state to \"ON\""); }  
            pinChange = true; 
            activeSwitch = 2;
            break;
         case 'z': // Switch 3
            if (pinState) { pinState = false; Serial.println("Set pin 3 state to \"OFF\""); }
            else { pinState = true; Serial.println("Set pin 3 state to \"ON\""); }  
            pinChange = true; 
            activeSwitch = 3;
            break;
         case 'i':    
            digitalWrite(infoPin, HIGH);
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



