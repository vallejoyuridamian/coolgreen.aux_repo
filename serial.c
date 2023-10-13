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
int header_mark;
int header_space;
int bit_mark;
int one_space;
int zero_space;
int tolerance;

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

    printf("\n\n\n");
    printf("\t\t\t\t\t\t\t\tWelcome to \033[36mCool\033[32mgreen's\033[31m\n");
    printf("\n\n\n");
    printf("\t\t██╗██████╗      ██████╗ ██████╗ ██████╗ ███████╗    ██╗     ███████╗ █████╗ ██████╗ ███╗   ██╗███████╗██████╗ \n");
    printf("\t\t██║██╔══██╗    ██╔════╝██╔═══██╗██╔══██╗██╔════╝    ██║     ██╔════╝██╔══██╗██╔══██╗████╗  ██║██╔════╝██╔══██╗\n");
    printf("\t\t██║██████╔╝    ██║     ██║   ██║██║  ██║█████╗      ██║     █████╗  ███████║██████╔╝██╔██╗ ██║█████╗  ██████╔╝\n");
    printf("\t\t██║██╔══██╗    ██║     ██║   ██║██║  ██║██╔══╝      ██║     ██╔══╝  ██╔══██║██╔══██╗██║╚██╗██║██╔══╝  ██╔══██╗\n");
    printf("\t\t██║██║  ██║    ╚██████╗╚██████╔╝██████╔╝███████╗    ███████╗███████╗██║  ██║██║  ██║██║ ╚████║███████╗██║  ██║\n");
    printf("\t\t╚═╝╚═╝  ╚═╝     ╚═════╝ ╚═════╝ ╚═════╝ ╚══════╝    ╚══════╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝\n");
    printf("\033[0m");


    printf("OK, so in this program we will use the Arduino and remote control to get the IR codes! \U0001F4A1 \n\n");
    printf("We will use this nomenclature to define the state:\nmode = C, H (cooling, heating) \u2744 \U0001F525\n");
    printf("temp = 17, ..., 30 (14 values) \U0001F321 \nfan = A, L, M, H (auto, low, medium, high) \U0001FAAD \nswing = 0, 1 (off, on)\n\n");
    
    printf("So for example, if we want cooling, 17 degrees, fan in auto and swing off it will be C17A0 \n\n");
    
    printf("Now kindly follow the instructions, point the remote to the IR receiver on the Arduino and let's get started!! \n\n");
    printf("Please put the remote in cooling mode, 17 degrees, auto fan, swing off (C17A0).\n");
    printf("When you are ready, put it one key press away from it (e.g. C18A0), press enter (in the keyboard!) and then send");
    printf(" the command when asked.\nNow just press enter once to start the program");

    int enterPressed = 0;
    while (!enterPressed) {
        int c = getchar();
        if (c == '\n' || c == EOF) {
            enterPressed = 1;
        }
    }

    printf("\n\n\nOK, we are going to have to figure out the protocol's time constants first, so just press buttons in the remote");
    printf(" and see them and when you feel ready press enter again and you will be asked to input them. We assume that the");
    printf(" protocol is pulse distance modulation for now, so you will be asked those constants.\n\n\n");
    
    enterPressed = 0;
    while (!enterPressed) {
        int c = getchar();
        if (c == '\n' || c == EOF) {
            enterPressed = 1;
        }
    }

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

     // Variables for the states' logic  //
     
    char modes[] = {'C', 'H'};
    int temperatures[] = {17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};
    char fan_speeds[] = {'A', 'L', 'M', 'H'};
    char swing_states[] = {'0', '1'};

/*    char modes[] = {'C', 'H'};
    int temperatures[] = {17, 18, 19};
    char fan_speeds[] = {'A', 'L'};
    char swing_states[] = {'0', '1'};*/

    int mode_count = sizeof(modes) / sizeof(modes[0]);
    int temp_count = sizeof(temperatures) / sizeof(temperatures[0]);
    int fan_count = sizeof(fan_speeds) / sizeof(fan_speeds[0]);
    int swing_count = sizeof(swing_states) / sizeof(swing_states[0]);

    int mode_idx  =  0;
    int temp_idx  =  0;
    int fan_idx   =  0;
    int swing_idx =  0;

    bool sweep_completed = false;    

    bool mode_completed = false;
    bool temp_completed = false;
    bool fan_completed = false;
    bool swing_completed = false;
    bool reset_fan = false;

    bool mode_going_up = true;
    bool temp_going_up = true;
    bool fan_going_up = true;
    bool swing_going_up = true;
    int fan_states_sweeped = 0;

    bool go_to_highest_fan = true; // this is for the fan that goes cyclical

    int state_idx = 0;

    // SQL stamenent variables //
    char insert_into[] = "INSERT INTO protocols_1 ("; 
    char values[] = " VALUES (";
    char columns_string[2048] = "";
    //char values_string[2048] = "";   
    //char values_string[131072] = "";   
    char *values_string = malloc(131072 * sizeof(char));


    // program states //
    bool checking_protocol = true;
    bool asking_time_constants = false;
    bool sweeping_commands = false;

    // time constants //
    
    
    bool keyboardInput = true;
    
    while (1) {

        // Program in mode reading from keyboard/writing to console
        if (keyboardInput) {

            char current_mode = modes[mode_idx];
            int current_temp = temperatures[temp_idx];
            char current_fan = fan_speeds[fan_idx];
            int current_swing = swing_states[swing_idx];

            char state_string[6]; // Assuming a maximum length of 10 characters for the state string
            snprintf(state_string, sizeof(state_string), "%c%d%c%c%c", current_mode, current_temp, current_fan, current_swing,'\0');

            // first we have to get the time constants
            if(checking_protocol){
                printf("Please press any button in the remote\n");
            } else if (mode_completed) {
                sweep_completed = true;
                printf("Please send the command for OFF (and we are done!)\n");
                strcat(columns_string, "OFF) ");
            } else {
            
            
                if(strcmp(hint_str,"Change mode") == 0){
                    printf("OK, so we know that cool and heat are not one key press away, so take your time \n");  
                    printf("to take the remote to a state that is one key press away from heat (with the same \n");
                    printf("temp, fan speed and swing of the previous state)\n");
                    int enterPressed = 0;
                    while (!enterPressed) {
                        int c = getchar();
                        if (c == '\n' || c == EOF) {
                            enterPressed = 1;
                        }
                    }
                }
            
                printf("Please send the command for %s (%s)\n", state_string, hint_str);
                strcat(columns_string, state_string);
                strcat(columns_string, ", ");
            }
            
            if(sweeping_commands){
                // this will calculate everything for the next state
                if (!swing_completed){
                    if (swing_going_up){
                        if (swing_idx < swing_count - 1) {
                            swing_idx++;  
                            strcpy(hint_str, "Turn on the swing");
                        } else {
                            swing_completed = true;
                            swing_going_up = false;
                        }       
                    } else { // swing going down
                        if (swing_idx > 0) {
                            swing_idx--;  
                            strcpy(hint_str, "Turn off the swing");
                        } else {
                            swing_completed = true;
                            swing_going_up = true;
                        } 
                    }
                }
                // if I completed the swing then I will check the fan
                // thing is that the fan goes only in one direction
                if (swing_completed && !fan_completed){
                    swing_completed = false;
                    if (fan_going_up){ 
                        if (fan_states_sweeped < fan_count - 1) {
                            fan_states_sweeped++;
                            fan_idx++;
                            if(fan_idx < fan_count) {
                                strcpy(hint_str, "Increase fan speed");
                            } else {
                                fan_idx = 0;
                                strcpy(hint_str, "Reset fan speed");                        
                            }                           
                        } else {
                            fan_states_sweeped = 0;
                            fan_completed = true;
                        }       
                    } else { // fan going down /*I'm gonna have to fix this logic too*/
                        if (fan_idx > 0) {
                            fan_idx--;  
                            strcpy(hint_str, "Decrease fan speed");
                        } else {
                            //fan_completed = true;
                            reset_fan = true;
                            //fan_going_up = true; this doesn't change
                        }
                    }
                }
                // if I completed the fan then I will check the temp
                if (fan_completed && !temp_completed){
                    fan_completed = false;
                    if (temp_going_up){
                        if (temp_idx < temp_count - 1) {
                            temp_idx++;  
                            strcpy(hint_str, "Increase temp");
                        } else {
                            temp_completed = true;
                            temp_going_up = false;
                        }       
                    } else { // fan going down
                        if (temp_idx > 0) {
                            temp_idx--;  
                            strcpy(hint_str, "Decrease temp");
                        } else {
                            temp_completed = true;
                            temp_going_up = true;
                        } 
                    }
                }
                // if I completed the temp then I will check the mode
                if (temp_completed && !mode_completed){
                    temp_completed = false;
                    if (mode_going_up){
                        if (mode_idx < mode_count - 1) {
                            mode_idx++;  
                            strcpy(hint_str, "Change mode");
                        } else {
                            mode_completed = true;
                            mode_going_up = false;
                        }       
                    } else { // mode going down
                        if (mode_idx > 0) {
                            mode_idx--;  
                            strcpy(hint_str, "Change mode");
                        } else {
                            mode_completed = true;
                            mode_going_up = true;
                        } 
                    }
                }
            }
            
            keyboardInput = false; // Switch to serial communication



        // Program in mode reading from serial
        } else {

            unsigned char buf[80];
            int rdlen;
            
            /*This is just to see the input, so we will remove it or make a flag for it*/
            bool message_to_print = false;
            while(checking_protocol){
                rdlen = read(fd, buf, sizeof(buf) - 1);
	        if (rdlen > 0) {
	            last_received_time = current_timestamp(); // Update the last received time
	            message_to_print = true;
	            //printf("Received:\n \"%s\"\n", buf);   
	            // copy to the buffer
	            for (int i = 0; i < rdlen; i++) {
                        if (buffer_pos < MAX_BUFFER_SIZE - 1) {
                            buffer[buffer_pos] = buf[i];
                            buffer_pos++;
                        }
	            }
                } else if (rdlen < 0) {
                    printf("Error from read: %d: %s\n", rdlen, strerror(errno));
                } else {  /* rdlen == 0 */
                    //printf("rdlen: %d: \n", rdlen);

                    // Check for timeout and print the accumulated data
                    unsigned long long current_time = current_timestamp();
                    //printf("Current time: \"%lld\"\n", current_time);
                    //printf("Time since last message: \"%lld\", buffer_pos: %d\n", current_time - last_received_time,buffer_pos );
                    if ((current_time - last_received_time >= timeout_ms && buffer_pos > 0) && (message_to_print)) {
                        buffer[buffer_pos] = '\0'; // Null-terminate the string
                        printf("\"%s\"\n", buffer);
                        buffer[0] = '\0';
		        buffer_pos = 0; // Reset the buffer position
                        message_to_print = false;
                        
                        printf("If you got the constants, press \"y\" and enter and you will be asked to input them \n");
                        printf("If you want to keep checking, press \"n\" and enter and you can send a new command \n");
                        
                        bool keyPressed = false;
                        while (!keyPressed) {
                            int c = getchar();
                            if (c == 'y') {
                                asking_time_constants = true;
                                checking_protocol = false;
                                printf("You pressed y, now you will be asked to input the time constants \n");
                            
                                // here I will get the time constants
                                keyPressed = false;
                                printf("Please input the header mark\n");
                                while (!keyPressed) {
                                    if (scanf("%d", &header_mark) == 1) {
                                        keyPressed = true;
                                    }
                                }
                                
                                keyPressed = false;
                                printf("Please input the header space\n");
                                while (!keyPressed) {
                                    if (scanf("%d", &header_space) == 1) {
                                        keyPressed = true;

                                    }
                                }
                                
                                keyPressed = false;                               
                                printf("Please input the bit mark\n");
                                while (!keyPressed) {
                                    if (scanf("%d", &bit_mark) == 1) {
                                        keyPressed = true;

                                    }
                                }
                                
                                keyPressed = false;
                                printf("Please input the one space\n");
                                while (!keyPressed) {
                                    if (scanf("%d", &one_space) == 1) {
                                        keyPressed = true;
                                    }
                                }
                                
                                keyPressed = false;
                                printf("Please input the zero space\n");
                                while (!keyPressed) {
                                    if (scanf("%d", &zero_space) == 1) {
                                        keyPressed = true;
                                    }
                                }                                              


                                keyPressed = false;
                                printf("Please input the tolerance you want the time to have (50-150 is OK in most cases)\n");
                                while (!keyPressed) {
                                    if (scanf("%d", &tolerance) == 1) {
                                        keyPressed = true;
                                    }
                                }                                              

                                keyPressed = false;
                                printf("OK, now please tell me the name we will give to this protocol (no spaces, please)\n");
                                while (!keyPressed) {
                                  scanf("%s", protocol_name);
                                  strcat(columns_string, "protocol_name, ");
                                  strcat(values_string, "'");
                                  strcat(values_string, protocol_name);
                                  strcat(values_string, "', ");
                                  keyPressed = true;
                                }                                              

                                printf("Header mark = %d us\n", header_mark);
                                printf("Header space = %d us\n", header_space);
                                printf("Bit mark = %d us\n", bit_mark);
                                printf("One space = %d us\n", one_space);
                                printf("Zero space = %d us\n", zero_space);
                                printf("Tolerance = %d us\n", tolerance);
                                printf("Protocol name = %s\n", protocol_name);
                                printf("OK! let's fa2fo!!\n\n");


                                printf("Hey! Remember that we will start with C17A0, so try and leave the remote one press away from it.\n");
                                printf("Another enter key press as you do it, take your time.\n");
                                printf("If you start seeing many w's you may want to Ctrl+C and start this over.\n");
                                getchar(); // burn one \n
                               
                                keyPressed = false;
                                while (!keyPressed) {
                                    int c = getchar();
                                    if (c == '\n') {
                                      keyPressed = true;
                                    }
                                }

                                keyboardInput = true;
                                sweeping_commands = true;
                                
                                
                            } else if (c == 'n') {
                                keyPressed = true;
                                checking_protocol = true;
                                printf("You pressed n, you can send a new command \n");
                            }                             
                        }
                    }
                }    
            }
            
            
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
	                    //printf("Buffer content: %s\n", buffer);          
	                    printf("Command received: %s\n", hexString);
	                    printf("Binary command: %s\n", binaryString);
	                    strcat(values_string, "'");
                            //strcat(values_string, hexString);
                            strcat(values_string, binaryString);
                            strcat(values_string, "'");
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
	            keyboardInput = true;
	            if (sweep_completed){
	                strcat(values_string, "); ");
	                break; // we are done
	            } else {
	                strcat(values_string, ", ");
                    }
               } // timeout with something on the buffer
            } // nothing to read from serial
        } // if keyboard
    } // this is the while where we jump from keyboard to serial 


    // Create the complete SQL statement
    char sqlStatement[262144]; // Adjust the size as needed
    
    snprintf(sqlStatement, sizeof(sqlStatement), "%s%s%s%s", insert_into, columns_string, values, values_string);

    free(values_string);
 
    // Print the SQL statement
    printf("\n\n\nSQL Statement:\n%s\n\n\n\n", sqlStatement);

    printf("We are done!! thank you for your patience \U0001F64F Have a good one!\n");
    printf("And remember, if you don't fuck around, you will never find out.\n");
    
    printf("Keep in mind that the commands may have some bits that are always the same, it would be smart\n");
    printf("to remove these bits from the command in the table and put them in the tag's code so we don't have to send\n");
    printf("unnecesary information, so you may want to postprocess this in excel or something, I'm just a c program, I already\n");
    printf("did a lot here, come on, you are on your own, good luck!\n");
    
 
    // Specify the file name and mode (e.g., "w" for write)
    const char* filename = "mysql.txt";
    FILE* file = fopen(filename, "w");

    // Check if the file was opened successfully
    if (file == NULL) {
        printf("Unable to open the file %s for writing.\n", filename);
        return 1; // Return an error code
    }

    // Write the string to the file
    fprintf(file, "%s", sqlStatement);
    // Close the file
    fclose(file);

    printf("PS: the sql statement is saved in %s\n", filename);
    printf("PS2: if you see any w's in the bits, you may have to do this all over again, sorry \n");
    printf("but if you are lucky, it fell into the constant bits so you may be able to deduce it.\n");
    
    close(fd);
    return 0;
}
