#include "SRecord.h"
#include <QString>
#include <QTextStream>

namespace Fkgo
{
  namespace Programmer
  {
    SRecord::SRecord() 
    {
    }
    SRecord::SRecord( const QByteArray& _source ) :
      QByteArray(_source)
    {
    }
    SRecord::Type SRecord::type() const
    {
      bool ok;
      // get type from S-Record
      Type _type = (Type)QString(mid(1,1)).toUInt(&ok,16);
      // check for conversion error
      if( !ok )
        return Error;
      // return S-Record type
      return _type;
    }
    unsigned int SRecord::count() const
    {
      bool ok;
      // get data count from S-Record
      unsigned int _count = QString(mid(2,2)).toUInt(&ok,16);
      // check for conversion error
      if( !ok )
        return 0;
      // return S-Record data count
      return _count;
    }
    unsigned int SRecord::addressCount() const
    {
      // check S-Record type and return corresponding address size
      switch( type() )
      {
      case 1:
      case 9:
        return 2;
      case 2:
      case 8:
        return 3;
      case 3:
      case 7:
        return 4;
      default:
        return 0;
      }
    }
    unsigned int SRecord::dataCount() const
    {
      return count() - addressCount() - 1;
    }
    unsigned long SRecord::address() const
    {
      bool ok;
      // get address from S-Record
      unsigned long _address = QString(mid(4,addressCount()*2)).toULong(&ok,16);
      // check for conversion error
      if( !ok )
        return 0;
      // return address
      return _address;
    }
    QByteArray SRecord::data() const
    {
      // will be the result value
      QByteArray _result;
      // cache count and address length
      unsigned int _dataCount = dataCount();
      unsigned int _addressCount = addressCount();
      // copy all data into result array
      for( unsigned int i=0; i<_dataCount; ++i )
      {
        bool ok;
        // read result byte, convert and add it to the result array
        _result += QString(mid(4+(_addressCount+i)*2,2)).toUInt(&ok,16);
        // check for conversion error
        if( !ok )
          // return empty result
          return QByteArray();
      }
      // return result
      return _result;
    }
    unsigned char SRecord::checksum() const
    {
      bool ok;
      // get checksum from S-Record
      unsigned char _checksum = QString(mid(4+count()*2-2,2)).toInt(&ok,16);
      // check for conversion error
      if( !ok )
        // return empty checksum
        return 0;
      // return checksum
      return _checksum;
    }
    bool SRecord::ok() const
    {
      // sum up all data
      unsigned int _sum = 0;
      {
        // sum count, address and data bytes
        for( int i=2; i<size()-2-2; ++i )
        {
          // read byte and convert and sum up
          bool ok;
          _sum += QString(mid(i*2,2)).toUInt(&ok,16);
          // check for conversion error
          if( !ok )
            // checksum can not be validated
            return false;
        }
      }
      // validate checksum
      return (_sum & 0xFF) == checksum();
    }
    QString SRecord::toString() const
    {
      QString _result;
      QTextStream _ts(&_result);
      _ts << "S" << QString::number(type(),16)
          << " " << QString::number(count(),16)
          << " " << QString::number(address(),16)
          << " " << data().toHex()
          << " " << QString::number(checksum(),16);
      return *_ts.string();
    }
    bool SRecord::isData() const
    {
      switch( type() )
      {
      case Data16:
      case Data24:
      case Data32:
        return true;
      default:
        return false;
      }
    }
  }
}
