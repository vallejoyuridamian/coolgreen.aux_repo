#include <stdio.h>
#include <errno.h>
#include <fcntl.h> 
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>

#define TERMINAL "/dev/ttyACM0"
#define MAX_BUFFER_SIZE 2048

// time constants
int header_mark = 3000;
int header_space = 1700;
int bit_mark = 550 ;
int one_space = 1000;
int zero_space = 350;
int tolerance = 150;

char hint_str[128] = "Mode: Cool, Temp: 17, Fan: Auto, Swing: Off"; 
char protocol_name[100]; 

char binaryToHex(char* binary) {
    int decimal = 0;
    for (int i = 0; i < 4; i++) {
        decimal = decimal * 2 + (binary[i] - '0');
    }
    char hex[4];
    sprintf(hex, "%X", decimal);
    return hex[0];
}

char binaryToHexLSB(char* binary) {
    int decimal = 0;
    for (int i = 3; i >= 0; i--) {
        decimal = decimal * 2 + (binary[i] - '0');
    }
    char hex_lsb[4];
    sprintf(hex_lsb, "%X", decimal);
    return hex_lsb[0]; 
}

int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void set_mincount(int fd, int mcount)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;        /* half second timer */

    if (tcsetattr(fd, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}

void set_blocking (int fd, int should_block){
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tggetattr", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                printf ("error %d setting term attributes", errno);
}


unsigned long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL);
    unsigned long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;
    return milliseconds;
}


int main() {


    char *portname = TERMINAL;
    int fd;
    int wlen;
    char *xstr = "Hello!\n";
    int xlen = strlen(xstr);

    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }
    
    // Set the baud rate and other serial port attributes
    set_interface_attribs(fd, B9600);
    set_blocking (fd, 0);                // set no blocking

    char buffer[MAX_BUFFER_SIZE];
    int buffer_pos = 0;

    unsigned long long last_received_time = 0;
    unsigned long long timeout_ms = 1000; // 3 seconds timeout

    
    while (1) {

            unsigned char buf[80];
            int rdlen;
            
            /*This is just to see the input, so we will remove it or make a flag for it*/
            bool message_to_print = false;
            // Actual program
            rdlen = read(fd, buf, sizeof(buf) - 1);
            if (rdlen > 0) {
                last_received_time = current_timestamp(); // Update the last received time

                for (int i = 0; i < rdlen; i++) {
                    /*if (buf[i] == '\n') {
                        buffer[buffer_pos] = '\0'; // Null-terminate the string
                        buffer_pos = 0; // Reset the buffer position
                    } else {
                      */
                    if (buffer_pos < MAX_BUFFER_SIZE - 1) {
                          buffer[buffer_pos] = buf[i];
                          buffer_pos++;
                    } else {
                          // Handle the case where the buffer is full
                    }                
                }
                //printf("rdlen: %d: \n", rdlen);
            } else if (rdlen < 0) {
                printf("Error from read: %d: %s\n", rdlen, strerror(errno));
            } else {  /* rdlen == 0 */
                //printf("rdlen: %d: \n", rdlen);

                // Check for timeout and print the accumulated data
                unsigned long long current_time = current_timestamp();
                //printf("Current time: \"%lld\"\n", current_time);
                //printf("Time since last message: \"%lld\", buffer_pos: %d\n", current_time - last_received_time,buffer_pos );
                if (current_time - last_received_time >= timeout_ms && buffer_pos > 0) {
                    buffer[buffer_pos] = '\0'; // Null-terminate the string
                    //printf("Received:\n \"%s\"\n", buffer);
                    //fflush(stdout); // Flush the output
                
                    // OK so here I would actually process it so that I get the command and print it as in the learner program
                    char line[1024];
                    int lineNumber = 1;
                    bool message_to_print = false;
                    char hexString[256] = ""; 
                    char swappedHexString[256] = ""; 
                    char binaryString[512] = ""; 
	    	    int numbers[8] = {0};
                    char* ptr = buffer;
                
                    while (*ptr != '\0') {
                        //printf("hey %d \n",*ptr);
                        if (*ptr == ' ' && *(ptr + 1) == '+') {
                            //char* ptr = buffer + 2; // Start after ' ' and '+' (or '  +')
                            ptr++;
                            ptr++;
                            for (int i = 0; i < 8; i++) {
                                numbers[i] = 0;
			    }
	                
                            int scanned = 0; // To track the number of values scanned

                            while (*ptr != '\n' && scanned < 8) {
                                if (*ptr >= '0' && *ptr <= '9') {
                 	            // Found a digit, extract the number
                                    int value = 0;
	      	                    while (*ptr >= '0' && *ptr <= '9') {
	                                value = value * 10 + (*ptr - '0');
                                        ptr++;
	                            }
	                            numbers[scanned] = value;
	                            scanned++;
	                        } else {
                                    // Skip other characters
                                    ptr++;
                                }

	                        // Check for a comma to continue scanning numbers
	                        if (*ptr == ',') {
	                            ptr++;
	                        }
	                    }

                            // Check if it is start or end
	                    if ((numbers[0] > header_mark - tolerance && numbers[0] < header_mark + tolerance) &&
	                        (numbers[1] > header_space - tolerance && numbers[1] < header_space + tolerance)) {
	                        //printf("START\n");
	                        hexString[0] ='\0';
                                swappedHexString[0] ='\0';	                    
                                binaryString[0] = '\0';
	                        message_to_print = false;
	                    } else if ((numbers[0] > bit_mark - tolerance && numbers[0] < bit_mark + tolerance) &&
	                        (numbers[1] == 0)) {
	                        //printf("END\n");
         	                message_to_print = true;
                            } else {
	                        char binary[5]; // Store the binary values
	                        binary[4] = '\0'; // Null-terminate the binary string
	    
 	                        for (int i = 0; i < 4; i++) {
	                            int valid_digit = false;
	                            int is_space = false;
	                            int is_mark = false;
	                            if (numbers[2 * i] > bit_mark - tolerance && numbers[2 * i] < bit_mark + tolerance) {
	                                valid_digit = true;
	                            } 
                                    if (numbers[2 * i + 1] > one_space - tolerance && numbers[2 * i + 1] < one_space + tolerance) {
	                                if (valid_digit) {
	                                    is_mark = true;
	                                }
	                            } else if (numbers[2 * i + 1] > zero_space - tolerance && numbers[2 * i + 1] < zero_space + tolerance) {
	                                if (valid_digit) {
	                                    is_space = true;
	                                }
	                            }
	
                                    // Build the binary representation
	                            if (is_mark) {
	                                binary[i] = '1';
	                            } else if (is_space) {
	                                binary[i] = '0';
	                            } else {
	                                binary[i] = 'w'; // Use 'w' for invalid values
	                            }
                                }
	
	                        // Convert binary to HEX and print
	                        char hexValue[2];
	                        hexValue[0] = binaryToHex(binary);
	                        hexValue[1] = '\0';
  	                        strcat(hexString, hexValue);
                        
  	                        char swappedHexValue[2];
  	                        swappedHexValue[0] = binaryToHexLSB(binary);
  	                        swappedHexValue[1] = '\0'; 
                                strcat(swappedHexString, swappedHexValue);
                                strcat(binaryString, binary);
 	                    
	                    }
                        } else if(message_to_print){// if line starts with " +"
	                    // if it doesn't we should check if there is a completed message to print out
	                    // Print the accumulated hexadecimal string
	                    printf("Buffer content: %s\n", buffer);          
	                    //printf("Command received: %s\n", hexString);
	                    //printf("Binary command: %s\n", binaryString);
	                    //strcat(values_string, "'");
                            //strcat(values_string, hexString);
                            //strcat(values_string, binaryString);
                            //strcat(values_string, "'");
	                    message_to_print = false;
	                    for (int i = 0; i < 8; i++) {
                                numbers[i] = 0;
			    }
                            buffer[0] = '\0';
			    buffer_pos = 0; // Reset the buffer position
                        }
	                // here I need to skip the line
	                ptr++; // guess that just increasing the pointer will do
	            } // finished reading the buffer
               } // timeout with something on the buffer
            } // nothing to read from serial
        } // this is the while where we jump from keyboard to serial 

   
    close(fd);
    return 0;
}
