#define _CRT_RAND_S
#include <cstdio>
#include <memory>
#include <string>
#include <array>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <fstream>
#include <ctime>
#include <chrono>
#include <string>
#include <sstream>
#include "AES.h"

// pls dont bully
using namespace std;

// EDIT THESE VALUES
const string POST_FILE = "C:/Users/trist/Desktop/CAT/Server - Client.txt"; // Path for SERVER - CLIENT communication
const string GET_FILE = "C:/Users/trist/Desktop/CAT/Client - Server.txt";  // Path for CLIENT - SERVER communication
const string GET_CACHE = "C:/Users/trist/Desktop/CAT/SERVER/CACHE.txt"; // Path for cache

// AES key
unsigned char key[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f };
//

unsigned int prevTime;
const uint16_t MAX_TIMEOUT = 256;
const unsigned int BLOCK_BYTES_LENGTH = 16 * sizeof(unsigned char);
unsigned int rng;



// I'm so sorry
string noResponse = "No response after ";

// Write to file
void Write(string path, string* data)
{
  // Create and open a text file
  ofstream MyFile(path);

  // Write to the file
  MyFile << *data;

  // Close the file
  MyFile.close();
}
// Read file
string Read(string path)
{
    string totalData;
    string data;

    // Open file
    ifstream ReadFile(path);

    // For each line
    // Not necessary because the client only writes one line
    // but I'm too lazy to edit it
    while (getline (ReadFile, data)) {
        totalData += data;
    }

    // Close file
    ReadFile.close();

    return totalData;

}





// Randomises rng
void Random()
{
    // According to the documentation "rand_s"
    // is relatively cryptographically secure
    // secure enough for IV

    // When I just called the function it didn't work
    // it returned hex or memory addresses
    rand_s(&rng);
}
// Generate 16 random bytes
void GenerateIV(unsigned char iv[16])
{
    for (int i=0;i<16;i++)
    {
        Random();
        iv[i] = rng % 256;
    }
}

// Takes a string and encrypts it (also handles padding).
// First 32 CHARACTERS are IV
string Encrypt(string plainText)
{

    // String to constant char arary
    const unsigned char* cData = (unsigned char*) plainText.c_str();

    // Gather some information about the plainText
    unsigned int blockCount = (plainText.length()/16) + 1;
    unsigned int mod = plainText.length() % 16;

    // Create new non constant array with cData, this is what gets encrypted
    unsigned char data[plainText.length()];
    for(int i=0;i<plainText.length();i++)
    {
        data[i] = cData[i];
    }

    // Add basic padding using the modulo
    for(int i=plainText.length();i<plainText.length()+mod;i++)
    {
        data[i] = 0x00;
    }


    // Generate IV (initialisation vectors) for AES
    unsigned char iv[16];
    GenerateIV(iv);

    // Length of cipherText
    unsigned int len;

    // Initialise AES class
    AES aes(256);

    // Encrypt
    unsigned char* out = aes.EncryptCBC(data, BLOCK_BYTES_LENGTH*blockCount, key, iv, len);



    string cipherText = "";
    char hex[4];

    // First hexify the IV (16 bytes so 32 characters) and add to cipherText
    for (int i=0;i<16;i++)
    {
        sprintf(hex, "%X", int(iv[i]));
        if (strlen(hex) == 1) cipherText += "0";
        cipherText += hex;
    }

    // Finally hexify and add the actual cipherText to the output
    for (int i=0;i<len;i++)
    {

        sprintf(hex, "%X", int(out[i]));
        if (strlen(hex) == 1) cipherText += "0";
        cipherText += hex;
    }


    delete[] out;

    return cipherText;
}

// Takes in string (min length 64, 32 characters of IV, rest is cipherText)
// and decrypts it
string Decrypt(string cipherText)
{

    // First get IV, 16 bytes so the first 32 (hex) characters
    unsigned char iv[16];
    for(int i=0; i<32; i+=2)
    {
        // Get 2 characters
        string sub = cipherText.substr(i,2);

        // and convert them from hex string to byte (char)
        iv[i/2] = (unsigned char) stoi(sub, nullptr, 16);
    }

    // Then get the actual cipherText (from 32 to length)
    unsigned char data[(cipherText.length()-32)/2];

    for(int i=32; i<cipherText.length(); i+=2)
    {
        // Get 2 characters
        string sub = cipherText.substr(i,2);

        // and convert them from hex string to byte (char)
        data[(i-32)/2] = (unsigned char) stoi(sub, nullptr, 16);
    }

    // Get block count
    unsigned int blockCount = ((cipherText.length()-32)/2/16) + 1;

    // Initialise AES class
    AES aes(256);

    // Decrypt
    unsigned char* out = aes.DecryptCBC(data, BLOCK_BYTES_LENGTH*blockCount, key, iv);


    string plainText;

    // Get the plainText until the first null (padding)
    for(int i =0;i<16*blockCount;i++)
    {

        if (out[i] == 0) break;

        plainText += out[i];
    }

    delete[] out;
    return plainText;
}

// "Get" function, can be edited for a http GET
string Get()
{
    string plainText;
    try
    {
        string data = Read(GET_FILE);
        plainText = Decrypt(data);
    }
    catch(...)
    {
        plainText = to_string(prevTime) + "|GET Error";
        cout << "GET ERROR" << endl;
    }


    return plainText;
}



// "Post" function, can be edited for a http POST
void Post(string data)
{

    try
    {
        string cipherText = Encrypt(data);
        Write(POST_FILE, &cipherText);
    }
    catch(...)
    {
        cout << "POST Error" << endl;
    }

}


// Get time since epoch in seconds
string CurrentTime()
{
    const auto p1 = std::chrono::system_clock::now();

    auto time = std::chrono::duration_cast<std::chrono::seconds>(
                   p1.time_since_epoch()).count();
    return to_string(time);
}


// Removes newline at the end of a string
void RemoveNewline(string* str)
{
    if (!(*str).empty() && (*str)[(*str).length()-1] == '\n') {
        (*str).erase((*str).length()-1);
    }

}




// Execute a command
string Command(string* command)
{
    // First post command together with the time
    Post(CurrentTime() + "|" + *command);

    // Get time of last response in both int and string form
    unsigned int currentTime = prevTime;
    string strCurrentTime;

    // Message is the total response
    string message;
    // Result is what gets returned
    string result;

    // Reset timeout
    uint16_t timeout = 1;

    // As long as the time responses' time is the same or less than do:
    while (currentTime <= prevTime)
    {
        // Sleep and double sleep delay every time up to MAX_TIMEOUT
        sleep(timeout);

        if (timeout < MAX_TIMEOUT) timeout = timeout << 1;
        else
        {
            // I'm so sorry
            return noResponse;
        }


        // Get current message and get the currentTime
        message = Get();
        stringstream streamMessage(message);

        // currentTime
        getline(streamMessage, strCurrentTime, '|');
        currentTime = stoi(strCurrentTime);



    }
    // Extract the result from the message
    result = message.substr(strCurrentTime.length()+1, message.length()-strCurrentTime.length());

    prevTime = currentTime;

    // Write currentTime to cache
    Write(GET_CACHE, &strCurrentTime);

    return result;

}
// Continuous commands (as if you have a normal cmd window open)
void ContinuousMode()
{
    // Preparing to prefix the line with the cwd
    // Example: C:\Users>
    string cd = "";
    bool iscd = false;

    string result;


    char command[256];


    while (true)
    {
        // Get and serialise input
        cout << cd << ">";
        cin.getline(command,sizeof(command));
        string strCommand = command;
        if (strCommand == "") continue;

        // If command is cd, prepare to set prefix of command
        if (strCommand.substr(0,2) == "cd") iscd = true;


        // Post command
        result = Command(&strCommand);


        cout << result << endl;


        // Set prefix
        if (iscd)
        {
            cd = result;
            RemoveNewline(&cd);
            iscd = false;
        }



    }

}


int main(int argc, char* argv[])
{
    // Check arguments
    if (argc < 2)
    {
        cout << "Invalid argument count. Use -h for help." << endl;
        return 0;
    }



    // Help
    if (strcmp(argv[1], "-h") == 0
     || strcmp(argv[1], "-H") == 0
     || strcmp(argv[1], "/h") == 0
     || strcmp(argv[1], "/H") == 0)
    {
        cout<<"\n   -h           : Display this help menu" << endl;
        cout << "   -c [COMMAND] : Execute single command on client" << endl;
        cout << "   -s           : Execute continuous string of commands" << endl;
        cout << "                  (Simulating a command prompt window)" << endl;
        return 0;
    }





    // I'm so sorry.
    noResponse += to_string(MAX_TIMEOUT);
    noResponse += " seconds";

    // Get time appended to previous message from cache
    try
    {

        prevTime = stoi(Read(GET_CACHE));
    }
    catch(...)
    {
        prevTime = 0;
    }


    // Continuous mode
    if (strcmp(argv[1], "-s") == 0
     || strcmp(argv[1], "-S") == 0
     || strcmp(argv[1], "/s") == 0
     || strcmp(argv[1], "/S") == 0)
    {
        ContinuousMode();
    }

    // Command mode
    if (strcmp(argv[1], "-c") == 0
     || strcmp(argv[1], "-C") == 0
     || strcmp(argv[1], "/c") == 0
     || strcmp(argv[1], "/C") == 0)
    {
        if (argc < 3)
        {
            cout << "Invalid command. Correct usage: SERVER -c [COMMAND]" << endl;
            return 0;
        }

        string strArg = argv[2];
        cout << Command(&strArg) << endl;

    }

    return 0;


}
