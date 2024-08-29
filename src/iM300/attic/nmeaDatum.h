/*    NAME: Blake Cole                                      */
/*    ORGN: MIT                                             */
/*    FILE: nmeaDatum.h                                     */
/*    DATE: 01 APRIL 2020                                   */
/************************************************************/

#ifndef NMEADATUM_HEADER
#define NMEADATUM_HEADER

#include <string>

enum DATUM_STATUS : unsigned int
  { STATUS_NEW,      // nmeaDatum new
    STATUS_VALID,    // nmeaDatum valid
    STATUS_INVALID,  // nmeaDatum invalid
    STATUS_SENT      // nmeaDatum published
  };

//-----------------------------------------------------------------//
//                           nmeaDatum                             //
//-----------------------------------------------------------------//
class nmeaDatum{

public:
  // structure for data from specified source (e.g. "GPGGA"):
  nmeaDatum(std::string source="", int index=0)
  {
    m_status   = STATUS_NEW;
    m_index    = index;
    m_key      = "";
    m_dval     = 0.0;
    m_sval     = "";
    m_datatype = "";
    m_source   = source;
    m_time     = 0.0;
  };
  ~nmeaDatum() {};

  // SET functions:
  void  setStatus(DATUM_STATUS status) {m_status   = status;};
  void  setIndex(int index)            {m_index    = index;};
  void  setKey(std::string key)        {m_key      = key;};
  void  setValue(double value)         {m_dval     = value;};
  void  setValue(std::string value)    {m_sval     = value;};
  void  setDatatype(std::string type)  {m_datatype = type;};
  void  setSource(std::string source)  {m_source   = source;};
  void  setTime(std::string time)      {m_time     = time;};

  // GET functions:
  DATUM_STATUS  getStatus()    {return m_status;};
  int           getIndex()     {return m_index;};
  std::string   getKey()       {return m_key;};
  double        getDValue()    {return m_dval;};
  std::string   getSValue()    {return m_sval;};
  std::string   getDatatype()  {return m_datatype;};
  std::string   getSource()    {return m_source;};
  std::string   getTime()      {return m_time;};
  
private:
  DATUM_STATUS     m_status;    // nmea datum status
  int              m_index;     // index number (e.g. "GPGGA" = 0)
  std::string      m_key;       // nmea datum key
  double           m_dval;      // nmea (double) value
  std::string      m_sval;      // nmea (string) value
  std::string      m_datatype;  // nmea data type
  std::string      m_source;    // nmea source key (e.g. "GPGGA")
  std::string      m_time;      // UTC time of record
};

#endif
