String stdStringToArduinoString( std::string &str )
{
  char *p = (char *) malloc( str.length() +1 ) ;
  strcpy( p, str.c_str());
  String tmp(p);
  free(p);
  return tmp;
}
