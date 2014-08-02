#include "MotFile.h"
#include <QDebug>

namespace Fkgo
{
  namespace Programmer
  {
    MotFile::MotFile( const QString& _fileName ) :
      QFile(_fileName)
    {
    }
    SRecord MotFile::read()
    {
      // read one record from one line in file
      SRecord _record = readLine();
      // trace record
      qDebug() << "MotFile::read: read SRecord:" << _record.toString();
      // return it
      return _record;
    }
    unsigned long MotFile::readImage( QByteArray& _image )
    {
      // skip non data records at beginning 
      SRecord _record = read();
      while( !_record.isData() )
        _record = read();
      // calculate padding to next page alginment
      unsigned long _pre = _record.address() % 0x100;
      // calculate first page address
      unsigned long _start = _record.address() - _pre;
      // append padding bytes to image
      _image.append( QByteArray( _pre, 0xff ) );
      qDebug() << "MotFile::readImage: starting image at" << QString::number(_start,16);
      // until there are no more records
      while( !_record.isEmpty() || _record.type() >= SRecord::Start32 )
      {
        // check for data record
        if( _record.isData() )
        {
          // check unexpected record order
          if( _record.address() < _start )
          {
            // output and return error
            qDebug() << "MotFile::readImage: unexpected SRecord order";
            return 0;
          }
          // append 0xff if there is a gap between the records
          _image.append( QByteArray( _record.address() - _start - _image.size(), 0xff ) );
          // add record data
          _image.append(_record.data());
        }
        // read next record
        _record = read();
      }
      // align to page border
      if( _image.size() % 0x100 != 0 )
        _image.append( QByteArray( 0x100 - (_image.size() % 0x100), 0xff ) );
      // return start address
      return _start;
    }
  }
}
