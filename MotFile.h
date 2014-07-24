#include "SRecord.h"
#include <QFile>

namespace Fkgo
{
  namespace Programmer
  {
    /// MOT file reader
    struct MotFile : QFile
    {
      MotFile( const QString& _fileName );
      unsigned long readImage( QByteArray& _image );
      SRecord read();
    };
  }
}
