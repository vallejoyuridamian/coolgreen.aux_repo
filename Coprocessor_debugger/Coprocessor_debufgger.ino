// constants
#define RE_CA_UART_STX  0xCA
#define RE_CA_UART_FIELD_DELIMITER 0x2C

// commands
#define RE_CA_UART_SET_ALL 15
#define RE_CA_UART_ADV_RPRT 16
#define RE_CA_UART_GET_ALL 25
#define RE_CA_UART_ACK  32
#define RE_CA_UART_SEND_TO_NUS 33
#define RE_CA_UART_BLE_CONNECTION 34
#define RE_CA_UART_NUS_SENT 35
#define RE_CA_UART_BLE_DISCOVERY 36
// Status
#define NRF_SUCCESS 0

// config
bool sendLedCtrl = false;

// aux
bool gotConfigCmd = false;
unsigned long lastMessageTime = 0;
const unsigned long messageInterval = 1000; // 1 second interval
int adv_counter = 0;
int MAX_ADV_PRINT = 80;

void setup() {
  Serial.begin(115200); // Console
  Serial1.begin(115200); // UART to NRF
  lastMessageTime = millis(); // Initialize last message time
}

void printByteAsHex(uint8_t byte) {
  // Print the byte in hexadecimal format with leading zeros
  char hexString[5];
  sprintf(hexString, "%02X", byte);
  Serial.print("[");
  Serial.print(hexString);
  Serial.print("]");
}

void sendResponse(const uint8_t* response, uint8_t length) {
  // Send the response back over Serial1
  for (uint8_t i = 0; i < length; ++i) {
    Serial1.write(response[i]);
  }
} 


void decodeMessage(uint8_t* message, uint8_t length) {
  // Placeholder for handling the received message
  /*Serial.print("Got message of length ");
  Serial.print(length);
  Serial.println();
  Serial.print("Message: ");
  for (uint8_t i = 0; i < length; ++i) {
    printByteAsHex(message[i]);
  }
  Serial.println();*/
  
  // OK so the message will start with CA, if not, it is wrong
  if (message[0] != RE_CA_UART_STX){
    // I will ignore leading 0's actually
    while (message[0] == 0) {
      for (int i = 0; i < length - 1;i++){
        message[i] = message[i+1];
      }
      length--;
    }
    
    if ( (length <= 0)||( message[0] != RE_CA_UART_STX)){
      Serial.println("Invalid message");
      return;
    }
  }
  //Serial.println("Correct start of message");
  int msg_len = message[1];
  int cmd = message[2];
 
  /*Serial.print("msg_len: ");
  Serial.println(msg_len);
 
  Serial.print("cmd: ");
  Serial.println(cmd);*/
 
  if (cmd == RE_CA_UART_ACK){
    Serial.println("Got UART ACK message");
    /*Serial.print("Message: ");
    for (uint8_t i = 0; i < length; ++i) {
      printByteAsHex(message[i]);
    }
    Serial.println();*/

    //Message: [CA][04][20][22][2C][01][2C][5B][FF][0A]
    // CA STX, 0A is EOF, 5B FF is CRC, so the meat is 0x22 0x2C 0x01 0x2C 
    // message length doesn't consider CA, itself, command, CRC nor 0A, does consider the delimiters though
    // so let's assume that it is always 4
    if (msg_len != 4){
      Serial.println("Message length for ACK is not 4, come fix pls!!!");  
    }
    // so I will have the command and the state
    int ack_cmd = message[3];
    int ack_status = message[5];

    if (ack_cmd == RE_CA_UART_SEND_TO_NUS){
      Serial.println("ACK from Send to NUS");  
    } else if (ack_cmd == RE_CA_UART_BLE_CONNECTION){
      Serial.println("ACK from BLE connection");  
    } else if (ack_cmd == RE_CA_UART_NUS_SENT){
      Serial.println("ACK from Nus sent");  
    } else if (ack_cmd == RE_CA_UART_SET_ALL){
      Serial.println("ACK from Set all");  
    } else if (ack_cmd == RE_CA_UART_BLE_DISCOVERY){
      Serial.println("ACK from Discovery");  
    } else {
      Serial.print("Uknown ACK pls code or ignore:");
      Serial.println(ack_cmd);
    }

    // status check 
    if (ack_status == NRF_SUCCESS){
      Serial.println("Success!!");  
    } else {
      Serial.print("Faiulre :( status: ");
      Serial.println(ack_status);
    }
  } else if (cmd == RE_CA_UART_ADV_RPRT){
    Serial.print("a");
    adv_counter++;
    if (adv_counter >= MAX_ADV_PRINT) {
        adv_counter = 0;
        Serial.println();
    }
  } else if (cmd == RE_CA_UART_GET_ALL){
    Serial.println("Got get all request");
  } else{
    Serial.println("Got an unknown message");
  }

  // Verify if the received message matches the expected stream
  const uint8_t getConfigCmd[] = {0xCA, 0x00, 0x19, 0x17, 0x9E, 0x0A};
  bool isGetConfigCmd = true;
  // ignore leading 0's
  uint8_t iOffset = 0;
  while (message[iOffset] == 0x00){
    iOffset = iOffset + 1;
  }
  
  for (uint8_t i = 0; i + iOffset < length; ++i) {
    if (message[i + iOffset] != getConfigCmd[i]) {
      isGetConfigCmd = false;
      break;
    }
  }
  


  if (isGetConfigCmd) {
    //Serial.println("Received message matches the expected stream.");
    gotConfigCmd = true;
    const uint8_t response[] = {0xCA, 0x05, 0x0F, 0x99, 0x04, 0x2C, 0x7D, 0x2C, 0x21, 0x61,0x0A};
    // CA is the start, 05 is length, 0F is the command, 0499 is the manufacturer ID (swap endianess), 2C's are delimiters
    // length is 5, doesn't consdier CA, itself, command, CRC and 0A 
    // BC is the boolean flags and 0A is the end of the message
    // flags are six bits filter tag (1), coded phy (0), scan phy (1), extended payload (1), use channels (1, 1 ,1)
    // last two bits unused 
    // 61 21 is the 16 bit CRC for 050F99042C7D2C (https://crccalc.com/ CRC-16/CCITT-FALSE swap endianness, input HEX, output HEX)
    // CRC is calculated on all the message without the CA and the 0A
    sendResponse(response, sizeof(response));


  } 
  // Verify if the received message matches the entire burst
  // TODO: Implement the verification logic
}

void loop() {
  static uint8_t message[128];
  static uint8_t messageLength = 0;
  static bool isNewMessage = false;

  if (Serial1.available()) {
    // Read the incoming byte from Serial1
    uint8_t incomingByte = Serial1.read();

    // Print the byte in hexadecimal format
    // printByteAsHex(incomingByte);

    // Add the byte to the message if it's within the array bounds
    if (messageLength < sizeof(message)) {
      message[messageLength++] = incomingByte;
    }

    // Check if the received byte is the newline character
    if (incomingByte == '\n') {
      // Serial.println();

      // Print the received message if it's a new message
      if (isNewMessage) {
        decodeMessage(message, messageLength);
      }

      // Reset the message length and flag for the next message
      messageLength = 0;
      isNewMessage = false;
    } else {
      // Set the flag to indicate a new message is being received
      isNewMessage = true;
    }
  }

  // Serial communication from the console 
  if (Serial.available()) {
    // Read the incoming byte from Serial1
    uint8_t incomingByte = Serial.read();
    Serial.print("Received command from user: ");
    printByteAsHex(incomingByte);
    Serial.println();
    // prepare the message to send to the other serial
    // OK so it must start with CA and then the length, the command, data, and CRC,
    // The command will be 33, 0x21, RE_CA_UART_CONNECT_TO_NUS, I will send the MACID, and the message content
    // MACID = F2:F5:0E:C4:1E:28
    // 2C is separator
    // MSG_LEN = 5
    // 2C is separator
    // MSG = 0x01,0x02,0x03,0x04,0x05
    // 2C is separator
    // CRC
    // EOL 0x0A
    const uint8_t response_2[] = {0xCA, 0x15, 0x21, 0xF2, 0xF5, 0x0E, 0xC4, 0x1E, 0x28, 0x2C, 0x0B, 0x2C, 0x2A, 0x02, 0x01, 0x04, 0x05,0x06, 0x07,
      0x08, 0x09, 0xFF, 0x0B, 0x2C, 0x03, 0xDA, 0x0A};

    //const uint8_t response[] = {0xCA, 0x05, 0x0F, 0x99, 0x04, 0x2C, 0x7D, 0x2C, 0x21, 0x61, 0x0A};
    // the length of the message is the total length -5, 27 - 5  - 1= 21 = 0x15
    // So to test for the password, 0x0B is the minimum length (11), 0x2A is the command to ask for passwors, 0x01 must be so it is valid
    // the rest is crap
    //handleReceivedMessage(response_2, sizeof(response_2));
    // for the CRC I use 1521F2F50EC41E282C0B2C2A0201040506070809FF0B2C      
    // for the length I don't consider itself or command, so 21 = 0x15 
    // DA 03 is the 16 bit CRC (https://crccalc.com/ CRC-16/CCITT-FALSE swap endianness)
    //sendResponse(response_2, sizeof(response_2));

    // OK, so now I'm going to send the LED blink
    const uint8_t response_3[] = {0xCA, 0x15, 0x21, 0xF2, 0xF5, 0x0E, 0xC4, 0x1E, 0x28, 0x2C, 0x0B, 0x2C, 0x2C, 0x02, 0x01, 0x04, 0x05,0x06, 0x07,
      0x08, 0x09, 0xFF, 0x0B, 0x2C, 0x3D, 0xD1, 0x0A};
    // So CA is the command intiation
    // 0x15 is the length (needs to be recalculated, didn't change it, so it is still 21)
    // The command is send to nus, 0x21
    // MACID = F2:F5:0E:C4:1E:28
    // 2C is separator
    // MSG_LEN = 11 (0x0B)
    // 2C is separator
    // 2C is command again (hopefully not interpreted as separator)
    // MSG = 0x02,0x01, 0x04,0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0x0B (not 0x0A because it is end of line)
    // 2C is separator
    // CRC
    // EOL 0x0A    
    // To calculate CRC  1521F2F50EC41E282C0B2C2C0201040506070809FF0B2C -> 0xD13D
    sendResponse(response_3, sizeof(response_3));
  }

  // Check if it's time to send a message
  unsigned long currentTime = millis();
  if (sendLedCtrl && gotConfigCmd && (currentTime - lastMessageTime >= messageInterval)) {
    lastMessageTime = currentTime; // Update last message time
    Serial.println("Sending LED control");
    const uint8_t led_msg[] = {0xCA, 0x03, 0x0E, 0xDC, 0x05, 0X2C, 0x6A, 0x57, 0x0A};
    sendResponse(led_msg, sizeof(led_msg)); // Exclude null terminator
  }

}

