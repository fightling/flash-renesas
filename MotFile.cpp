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
      SRecord _record = readLine();
      qDebug() << "MotFile::read: read SRecord:" << _record.toString();
      return _record;
    }
    unsigned long MotFile::readImage( QByteArray& _image )
    {
      SRecord _record = read();
      while( !_record.isData() )
        _record = read();
      unsigned long _pre = _record.address() % 0x100;
      unsigned long _start = _record.address() - _pre;
      _image.append( QByteArray( _pre, 0xff ) );
      qDebug() << "MotFile::readImage: starting image at" << QString::number(_start,16);
      while( !_record.isEmpty() || _record.type() >= SRecord::Start32 )
      {
        if( _record.isData() )
        {
          if( _record.address() < _start )
          {
            qDebug() << "MotFile::readImage: unexpected SRecord order";
            return 0;
          }
          _image.append( QByteArray( _record.address() - _start - _image.size(), 0xff ) );
          _image.append(_record.data());
        }
        _record = read();
      }
      if( _image.size() % 0x100 != 0 )
        _image.append( QByteArray( 0x100 - (_image.size() % 0x100), 0xff ) );
      return _start;
    }
  }
}
