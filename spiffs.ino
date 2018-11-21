#include "FS.h"
#include "SPIFFS.h"

#define FORMAT_SPIFFS_IF_FAILED true

String __ssid;
String __pw;
String __organization;
String __server;

void writeDefaultOrganization()
{
   File file = SPIFFS.open("/organization.txt", FILE_WRITE) ;
  if(!file || file.isDirectory()){
        Serial.println("- failed to open organization file for writing");
        return;
  }

  file.println("Daemyung");
  file.close();
  __organization = "Daemyung";
}

void writeDefaultWIFI()
{
   File file = SPIFFS.open("/wifi.txt", FILE_WRITE) ;
  if(!file || file.isDirectory()){
        Serial.println("- failed to open organization file for writing");
        return;
  }

  file.println("esp32");
  file.println("xxxxxx");
  file.close();
  __ssid = "esp32";
  __pw = "xxxxxx";
}

void writeDefaultServer()
{
  File file = SPIFFS.open("/server.txt", FILE_WRITE) ;
  if(!file || file.isDirectory()) {
        Serial.println("- failed to open organization file for writing");
        return;
  }

  file.println("35.185.153.239");
  file.close();
  __server = "35.185.153.239";
}



void saveWIFI( std::string &_ssid, std::string &_pw )
{
  String ssid = stdStringToArduinoString(_ssid);
  String pw = stdStringToArduinoString(_pw);
  saveWiFiSetting( SPIFFS, ssid, pw );
}


void saveWiFiSetting( fs::FS &fs, String ssid, String pw )
{
  __ssid = ssid;
  __pw = pw;  
  //delete 
  fs.remove( "/wifi.txt" );
  File file = fs.open("/wifi.txt" , "w");
  file.println( __ssid.c_str());
  file.println( __pw.c_str());
  file.close();

  loadSetting(fs);
}

void saveServer( fs::FS &fs, String server )
{
  __server = server;
  fs.remove("/server.txt" );
  File file = fs.open( "/server.txt" , "w" );
  file.println(__server.c_str());
  file.close();
}

char *getSSID()
{
  return const_cast<char *>(__ssid.c_str());
}

char *getPW()
{
  return const_cast<char *>(__pw.c_str());
}

char *getOrganization()
{
  return const_cast<char *>( __organization.c_str());
}

char *getServer()
{
  return const_cast<char *>( __server.c_str());
}


String readLine( File &file )
{
  char buffer[256];
  for ( int i = 0 ;i < sizeof(buffer) ; i ++ )
  {
    int ch = file.read() ;
    if ( ch != '\r' && ch != '\n' && ch > 0  )
      buffer[i] = ch;
    else
      buffer[i] = 0;
    if ( ch == '\n' || ch <= 0 )
      break;
  }
  String val(buffer);
  return val;
}

void loadSetting( fs::FS &fs )
{
  File file = fs.open("/organization.txt", FILE_READ) ;
  if(!file || file.isDirectory()){
        Serial.println("- failed to open organization file for reading");
        return;
  }
  Serial.println("open organization.txt");
  String organizationName = readLine(file);
  __organization = organizationName;
  Serial.print("read organization name :");
  Serial.print(organizationName);
  Serial.println("--");
  file.close();
  if ( __organization == "" )
    writeDefaultOrganization();
  
  file = fs.open("/wifi.txt",FILE_READ);
  if(!file || file.isDirectory()){
        Serial.println("- failed to open wifi file for reading");
        return;
  }
  String ssid = readLine(file);
  __ssid = ssid;
  String pw = readLine(file );
  __pw = pw;
  Serial.print("SSID, PW :" );
  Serial.print(ssid);
  Serial.print(" " );
  Serial.println(pw);
  file.close();
  if ( __ssid == "" ) {
    writeDefaultWIFI();
  }

  file = fs.open("/server.txt" ,FILE_READ);
   if(!file || file.isDirectory()){
        Serial.println("- failed to open server file for reading");
        return;
  }
  String server = readLine(file) ;
  __server = server;
  Serial.print("Server : " ) ;
  Serial.println( __server );
  file.close();
  if ( __server =="" )
    writeDefaultServer();
}
