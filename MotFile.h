#include "SRecord.h"
#include <QFile>

namespace Fkgo
{
  namespace Programmer
  {
    /// MOT file reader
    struct MotFile : QFile
    {
      /// open MOT file
      MotFile( const QString& _fileName );
      /// read an image out of the file
      unsigned long readImage( QByteArray& _image );
      /// read a single record from file
      SRecord read();
    };
  }
}
