#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   60

#define LAMP1 2
#define LAMP2 3
#define LAMP3 5
#define FAN 6
#define HEATER 7

// MAC address from Ethernet shield sticker under board
byte mac[] = {
  0x14, 0x8F, 0xC6, 0x9C, 0xF1, 0x2B
};
IPAddress ip(192, 168, 0, 120); // IP address, may need to change depending on network
EthernetServer server(80);  // create a server at port 80
File webFile;               // the web page file on the SD card
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer

boolean DEV_state[5] = {0}; // stores the states of the Devices

void setup()
{
    // Set Output pin Mode
    pinMode(LAMP1, OUTPUT);
    pinMode(LAMP2, OUTPUT);
    pinMode(LAMP3, OUTPUT);
    pinMode(FAN, OUTPUT);
    pinMode(HEATER, OUTPUT);

    // Clear Output pins initially i.e. Switch OFF all the appliances initially
    digitalWrite(LAMP1, LOW);
    digitalWrite(LAMP2, LOW);
    digitalWrite(LAMP3, LOW);
    digitalWrite(FAN, LOW);
    digitalWrite(HEATER, LOW);
    
    // disable Ethernet chip
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);
    
    Serial.begin(9600);       // for debugging
    
    // initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(4)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
    }
    Serial.println("SUCCESS - SD card initialized.");
    // check for index.htm file
    if (!SD.exists("index.htm")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
    }
    Serial.println("SUCCESS - Found index.htm file.");
//    // LEDs
//    pinMode(6, OUTPUT);
    
    Ethernet.begin(mac, ip);  // initialize Ethernet device
    server.begin();           // start to listen for clients
    Serial.println("Server started and listning clients.");
}

void loop()
{
    EthernetClient client = server.available();  // try to get client

    if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // limit the size of the stored received HTTP request
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // save HTTP request character
                    req_index++;
                }
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    // remainder of header follows below, depending on if
                    // web page or XML page is requested
                    // Ajax request - send XML file
                    if (StrContains(HTTP_req, "ajax_inputs")) {
                        // send rest of HTTP header
                        client.println("Content-Type: text/xml");
                        client.println("Connection: keep-alive");
                        client.println();
                        SetDEVs();
                        // send XML file containing input states
                        XML_response(client);
                    }
                    else {  // web page request
                        // send rest of HTTP header
                        client.println("Content-Type: text/html");
                        client.println("Connection: keep-alive");
                        client.println();
                        // send web page
                        webFile = SD.open("index.htm");        // open web page file
                        if (webFile) {
                            while(webFile.available()) {
                                client.write(webFile.read()); // send web page to client
                            }
                            webFile.close();
                        }
                    }
                    // display received HTTP request on serial port
                    Serial.print(HTTP_req);
                    // reset buffer index and all buffer elements to 0
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                } 
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(1);      // give the web browser time to receive the data
        client.stop(); // close the connection
    } // end if (client)
}

// checks if received HTTP request is switching on/off LEDs
// also saves the state of the Devices
void SetDEVs(void)
{   
    if (StrContains(HTTP_req, "DEV1=1")) {
        DEV_state[0] = 1;  // save LED state
        digitalWrite(LAMP1, HIGH);
    }
    else if (StrContains(HTTP_req, "DEV1=0")) {
        DEV_state[0] = 0;  // save LED state
        digitalWrite(LAMP1, LOW);
    }   
     if (StrContains(HTTP_req, "DEV2=1")) {
        DEV_state[1] = 1;  // save LED state
        digitalWrite(LAMP2, HIGH);
    }
    else if (StrContains(HTTP_req, "DEV2=0")) {
        DEV_state[1] = 0;  // save LED state
        digitalWrite(LAMP2, LOW);
    }   

    if (StrContains(HTTP_req, "DEV3=1")) {
        DEV_state[2] = 1;  // save LED state
        digitalWrite(LAMP3, HIGH);
    }
    else if (StrContains(HTTP_req, "DEV3=0")) {
        DEV_state[2] = 0;  // save LED state
        digitalWrite(LAMP3, LOW);
    }   

     if (StrContains(HTTP_req, "DEV4=1")) {
        DEV_state[3] = 1;  // save LED state
        digitalWrite(FAN, HIGH);
    }
    else if (StrContains(HTTP_req, "DEV4=0")) {
        DEV_state[3] = 0;  // save LED state
        digitalWrite(FAN, LOW);
    }   

    if (StrContains(HTTP_req, "DEV5=1")) {
        DEV_state[4] = 1;  // save LED state
        digitalWrite(HEATER, HIGH);
    }
    else if (StrContains(HTTP_req, "DEV5=0")) {
        DEV_state[4] = 0;  // save LED state
        digitalWrite(HEATER, LOW);
    }          
}

// send the XML file with analog values, switch status
//  and LED status
void XML_response(EthernetClient cl)
{  
    cl.print("<?xml version = \"1.0\" ?>");
    cl.print("<inputs>");    

    // Check Box Device status
    // For Lamp 1
    cl.print("<DEV>");
    if (DEV_state[0]) {
        cl.print("checked");
    }
    else {
        cl.print("unchecked");
    }
    cl.println("</DEV>");  

    // For Lamp 2
    cl.print("<DEV>");
    if (DEV_state[1]) {
        cl.print("checked");
    }
    else {
        cl.print("unchecked");
    }
    cl.println("</DEV>");  
    
    // For Lamp 3
    cl.print("<DEV>");
    if (DEV_state[2]) {
        cl.print("checked");
    }
    else {
        cl.print("unchecked");
    }
    cl.println("</DEV>");

    // For FAN
    cl.print("<DEV>");
    if (DEV_state[3]) {
        cl.print("checked");
    }
    else {
        cl.print("unchecked");
    }
    cl.println("</DEV>");  

    // For HEATER
    cl.print("<DEV>");
    if (DEV_state[4]) {
      Serial.println("Am I here Always? ");
        cl.print("checked");
    }
    else {
        cl.print("unchecked");
    }
    cl.println("</DEV>");  
    
    cl.print("</inputs>");
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}
