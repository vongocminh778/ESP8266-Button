#include "FtpClient.h"
#include "Arduino.h"
#include <WiFiClient.h>
#include "FS.h"

void FtpClient::SetFilename(String VarFile)
{
    fileName = VarFile;
}

String FtpClient::GetFilename()
{
    return fileName;
}

//--------------------------------------------------------------------

void FtpClient::SetPathName(String VarPath)
{
    path = VarPath;
}

String FtpClient::GetPathName()
{
    return path;
}

//--------------------------------------------------------------------

void FtpClient::SetFtpServer(String VarServer)
{
    FTP_Server = VarServer;
}

String FtpClient::GetFtpServer()
{
    return FTP_Server;
}

//--------------------------------------------------------------------

void FtpClient::SetFtpPort(int VarPort)
{
    FTP_Port = VarPort;
}

int FtpClient::GetFtpPort()
{
    return FTP_Port;
}

//--------------------------------------------------------------------

void FtpClient::SetFtpUser(String VarUser)
{
    FTP_User = VarUser;
}

String FtpClient::GetFtpUser()
{
    return FTP_User;
}
//--------------------------------------------------------------------

void FtpClient::SetDebug(bool VarDebug)
{
    if (VarDebug)
    {
        debug = true;
    }
    else
    {
        debug = false;
    }
}

/*
------------------------------------------------------------------------------------ 
                          CLASS FUNCTIONS FTP SPECIFIC
------------------------------------------------------------------------------------
*/

byte FtpClient::Upload(boolean upload)
{
    Serial.println("Starting FTP Process..");
    if (upload)
    {
        fh = SPIFFS.open(path, "r");
        Serial.print("path : ");
        Serial.println(path);
    }
    else
    {
        SPIFFS.remove(path);
        fh = SPIFFS.open(path, "w");
        delay(10); // added when create new file
    }

    if (!fh)
    {
        if (debug)
            Serial.println("SPIFFS file open has failed!");
        return 0;
    }

    if (upload)
    {
        if (!fh.seek((uint32_t)0, SeekSet))
        {
            Serial.println("Rewind fail");
            fh.close();
            return 0;
        }
    }

    if (debug)
        Serial.println("SPIFFS file opened");

    if (client.connect(FTP_Server.c_str(), FTP_Port))
    { // 21 = FTP server
        if (debug)
            Serial.println("FTP Server connected");
    }
    else
    {
        fh.close();
        if (debug)
            Serial.println("Could not connect to FTP Server Check Settings....");
        return 0;
    }

    if (!eRcv())
        return 0;
    if (debug)
        Serial.println("Send USER");
    client.print("USER ");
    client.println(FTP_User);

    if (!eRcv())
        return 0;
    if (debug)
        Serial.println("Send PASSWORD");
    client.print("PASS ");
    client.println(FTP_Pwd);

    if (!eRcv())
        return 0;
    if (debug)
        Serial.println("Send SYST");
    client.println("SYST");

    if (!eRcv())
        return 0;
    if (debug)
        Serial.println("Send Type I");
    client.println("Type I");

    if (!eRcv())
        return 0;
    if (debug)
        Serial.println("Send PASV");
    client.println("PASV");

    if (!eRcv())
        return 0;

    char *tStr = strtok(outBuf, "(,");
    int array_pasv[6];
    for (int i = 0; i < 6; i++)
    {
        tStr = strtok(NULL, "(,");
        array_pasv[i] = atoi(tStr);
        if (tStr == NULL)
        {
            if (debug)
                Serial.println("Bad PASV Answer");
        }
    }
    unsigned int hiPort, loPort;
    hiPort = array_pasv[4] << 8;
    loPort = array_pasv[5] & 255;

    if (debug)
        Serial.print("Data port: ");
    hiPort = hiPort | loPort;
    if (debug)
        Serial.println(hiPort);

    if (dclient.connect(FTP_Server.c_str(), hiPort))
    {
        if (debug)
            Serial.println("Data connected");
    }
    else
    {
        if (debug)
            Serial.println("Data connection failed");
        client.stop();
        fh.close();
        return 0;
    }

    if (upload)
    {
        if (debug)
            Serial.println("Send STOR filename");
        client.print("STOR ");
        client.println(fileName);
    }
    else
    {
        if (debug)
            Serial.println("Send RETR filename");
        client.print("RETR ");
        client.println(fileName);
    }

    if (!eRcv())
    {
        dclient.stop();
        return 0;
    }

    if (upload)
    {
        if (debug)
            Serial.println("Writing");
#define bufSizeFTP 1460
        uint8_t clientBuf[bufSizeFTP];
        size_t clientCount = 0;

        while (fh.available())
        {
            clientBuf[clientCount] = fh.read();
            clientCount++;
            if (clientCount > (bufSizeFTP - 1))
            {
                dclient.write((const uint8_t *)&clientBuf[0], bufSizeFTP);
                clientCount = 0;
                delay(1);
            }
        }
        if (clientCount > 0)
            dclient.write((const uint8_t *)&clientBuf[0], clientCount);
    }
    else
    {
        while (dclient.connected())
        {
            while (dclient.available())
            {
                char c = dclient.read();
                fh.write(c);
                if (debug)
                    Serial.write(c);
            }
        }
    }

    dclient.stop();
    if (debug)
        Serial.println("Data disconnected");

    if (!eRcv())
        return 0;

    client.println("QUIT");

    if (!eRcv())
        return 0;

    client.stop();
    if (debug)
        Serial.println("Command disconnected");

    fh.close();
    if (debug)
        Serial.println("SPIFFS closed");
    return 1;
} // Upload()

//--------------------------------------------------------------------

byte FtpClient::eRcv()
{
    byte respCode;
    byte thisByte;
    int loopCount = 0;

    if (debug)
        Serial.println("Start eRcv Reset Counter..");
    while (!client.available())
    {
        delay(1);
        loopCount++;
        // if nothing received for 10 seconds, timeout
        if (loopCount > 3000)
        {
            client.stop();
            Serial.println("3 secTimeout");
            return 0;
        }
    }
    respCode = client.peek();
    outCount = 0;
    while (client.available())
    {
        thisByte = client.read();
        if (debug)
            Serial.write(thisByte); // MODDED HERE IF DEBUG
        if (outCount < 127)         // was 127
        {
            outBuf[outCount] = thisByte;
            outCount++;
            outBuf[outCount] = 0;
        }
    }

    if (respCode >= '4')
    {
        if (debug)
            Serial.println("Efail...");
        efail();
        return 0;
    }

    if (debug)
        Serial.println("Return DB Ok..");
    return 1;
} // end of eRcv()

//--------------------------------------------------------------------

void FtpClient::efail()
{
    byte thisByte = 0;

    client.println("QUIT");

    while (!client.available())
        delay(1);

    while (client.available())
    {
        thisByte = client.read();
        if (debug)
            Serial.write(thisByte); // MODDED DEBUG ADDITION
    }

    client.stop();
    if (debug)
        Serial.println("Command disconnected");
    fh.close();
    if (debug)
        Serial.println("Spiffs file closed");
} // efail

//--------------------------------------------------------------------

String FtpClient::formatBytes(size_t bytes)
{
    if (bytes < 1024)
    {
        return String(bytes) + "B";
    }
    else if (bytes < (1024 * 1024))
    {
        return String(bytes / 1024.0) + "KB";
    }
    else if (bytes < (1024 * 1024 * 1024))
    {
        return String(bytes / 1024.0 / 1024.0) + "MB";
    }
    else
    {
        return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
    }
}

void FtpClient::listAllFiles()
{

    if (!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    Dir dir = SPIFFS.openDir("");
    while (dir.next())
    {
        Serial.println("File name : " + dir.fileName());
        // Serial.println("Size : " + String(dir.fileSize()));
    }
}

void FtpClient::removeFile(String path_file)
{
    if (!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    bool flag = SPIFFS.remove(path_file);
    if (flag)
    {
        Serial.println("Remove file successfully!");
    }
    else
    {
        Serial.println("Fail to remove file!");
    }
}

void FtpClient::removeAllFiles()
{
    if (!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    Dir dir = SPIFFS.openDir("");
    while (dir.next())
    {
        Serial.println("File name : " + dir.fileName());
        SPIFFS.remove(dir.fileName());
    }
}