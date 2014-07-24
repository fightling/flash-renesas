#include <QByteArray>

namespace Fkgo
{
  namespace Programmer
  {
    /// S-Record like in http://en.wikipedia.org/wiki/SREC_(file_format)
    struct SRecord : QByteArray
    {
      enum Type
      {
        Error = -1,
        Header,
        Data16,
        Data24,
        Data32,
        Reserved,
        Count16,
        Count24,
        Start32,
        Start24,
        Start16
      };
      /// create an empty S-Record
      SRecord();
      /** @brief create an S-Record based on given source data
       * @param _source arra of bytes to read
       */
      SRecord( const QByteArray& _source );
      /// return S-Record type
      Type type() const;
      /// return count of bytes in S-Record
      unsigned int count() const;
      /// return address length depending on S-Record type
      unsigned int addressCount() const;
      /// return count of bytes in data 
      unsigned int dataCount() const;
      /// return address from S-Record
      unsigned long address() const;
      /// return data from S-Record
      QByteArray data() const;
      /// return checksum from S-Record
      unsigned char checksum() const;
      /// return true if checksum matches the rest of the S-Record 
      bool ok() const;
      /// visualize S-Record in a string (for debugging purpose)
      QString toString() const;
      /// check if S-Record is a data record
      bool isData() const;
    };
  }
}
