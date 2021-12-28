#ifndef DBFTP_h
#define DBFTP_h

#include "Arduino.h"
#include <WiFiClient.h>
#include "FS.h"

class FtpClient
{

public:
    void SetFilename(String VarFile);
    void SetPathName(String VarPath);
    void SetFtpServer(String VarServer);
    void SetFtpPort(int VarPort);
    void SetFtpUser(String VarUser);
    void listAllFiles();
    void removeFile(String path_file);
    void removeAllFiles();
    String GetFilename();
    String GetPathName();
    String GetFtpServer();
    int GetFtpPort();
    String GetFtpUser();
    void SetDebug(bool VarDebug);
    byte Upload(boolean upload);
    String formatBytes(size_t bytes);

private:
    // private functions and alias
    WiFiClient client;
    WiFiClient dclient;
    byte eRcv();
    void efail();
    //String formatBytes(size_t bytes);

    // Private Variables etc

    int _pin;
    boolean debug = true; // true = more messages
    char outBuf[128];     // was 128
    char outCount;
    // change fileName to your file (8.3 format!)
    String fileName = "27_12_2021_11_13_31.csv";
    String path = "/27_12_2021_11_13_31.csv";
    String FTP_Server = "192.168.1.99";
    int FTP_Port = 21;
    String FTP_User = "vnminh";
    String FTP_Pwd = "M@tsuyaR&D2020khicon77";
    File fh; // SPIFFS file handle
};

#endif