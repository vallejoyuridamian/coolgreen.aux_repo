#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTRIES 1000 // Adjust this based on your needs
#define MAX_BIT_LENGTH 1000 // Adjust this based on your needs

char *substring;// = "VALUES (";
char *pos;// = strstr(line, substring);


int main() {
    FILE *file = fopen("mysql.txt", "r"); // Replace "input.txt" with your file path
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    char line[65536]; // Adjust the buffer size as needed

    // we ignore the characters up to VALUES (
    while (fgets(line, sizeof(line), file) != NULL) {
        substring = "VALUES (";
        pos = strstr(line, substring);

/*        if (pos != NULL) {
            // "VALUES (" found, print the rest of the line
            printf("Found: %s\n", pos);
        }*/
    }
    // now we look for the next comma and we will have the first value
    //while (fgets(line, sizeof(line), file) != NULL) {
    substring = ",";
    pos = strstr(pos, substring);
    pos = pos + 2; // the , and space

    
    
    //printf("pos before the things = %s\n",pos);
            
    char values[MAX_ENTRIES][MAX_BIT_LENGTH];
    int numValues = 0;

    /*// Tokenize and store values
    char *token = strtok(pos, ",");
            
            
    printf("pos after the token thing = %s\n",pos)*/;
    
    char *startQuote = strchr(pos, '\'');
         
    while (startQuote != NULL){
        char *endQuote = strchr(startQuote + 1, '\'');

        int length = endQuote - startQuote - 1;
        strncpy(values[numValues], startQuote + 1, length);
        values[numValues][length] = '\0';
        numValues++;
        startQuote = strchr(endQuote + 1, '\'');           
    }
            
    /*for (int i = 0; i < numValues; i++){
        printf("Value found %d: %s\n", i + 1, values[i]);
    }*/


    int bitLength = strlen(values[0]);

    char result[bitLength + 1];
    result[bitLength] = '\0';

    int sameCount = 0;
    int varyCount = 0;

    for (int i = 0; i < bitLength; i++) {
        char bitValue = values[0][i];
        int isSame = 1;

        for (int j = 1; j < numValues; j++) {
            if (values[j][i] != bitValue) {
                isSame = 0;
                break;
            }
        }

    /*    if (isSame) {
           printf("Bit %d is always %c\n", i, bitValue);
        } else {
            printf("Bit %d varies\n", i);
        }*/
        
        if (isSame) {
            result[i] = bitValue;
            sameCount++;
        } else {
            result[i] = 'X';
            varyCount++;
        }
        
        
    }

    printf("Resulting string: %s\n", result);
    printf("Bits always the same: %d\n", sameCount);
    printf("Bits that vary: %d\n", varyCount);

    substring = "VALUES (";
    int lengthBefore = strstr(line, substring) - line;
    line[lengthBefore] = '\0';

    strcat(line,substring);

    // Define an array to store modified values for each entry
    char arrayResults[numValues][varyCount];
    memset(arrayResults, 0, sizeof(arrayResults)); // Initialize the arrayResults

    strcat(line,"\'");
    int kBitVary = 0;
    for (int kBit = 0; kBit < bitLength; kBit++) {
        if(result[kBit] == 'X'){
            arrayResults[0][kBitVary] = values[0][kBit];
            kBitVary++;
        }
    }
    strcat(line, arrayResults[0]);
    strcat(line,"\'");
    //    printf("Got here, %s \n", arrayResults[0]);
    
    for (int kValue = 0; kValue < numValues - 1; kValue++){
        strcat(line,", ");        
        strcat(line,"\'");
    
        int kBitVary = 0;
        for (int kBit = 0; kBit < bitLength; kBit++) {
            if(result[kBit] == 'X'){
                arrayResults[kValue + 1][kBitVary] = values[kValue + 1][kBit];
                kBitVary++;
            }
        }
        
        strcat(line, arrayResults[kValue + 1]);
        strcat(line,"\'");
    }
    
    strcat(line,"); ");        

    //printf("New SQL: %s\n", line);

    fclose(file);

    FILE *outputFile = fopen("mysql_reduced.txt", "w"); // Create or replace "mysql_reduced.txt" for output
    if (outputFile == NULL) {
        perror("Error opening output file");
        return 1;
    }

    fprintf(outputFile, "%s\n", line);

    fclose(outputFile);

    return 0;
}

