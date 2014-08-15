#include <QSerialPort>

namespace Fkgo
{
  /// flash programmer modules
  namespace Programmer
  {
    /// connection to program a flashable microcontroller
    struct Connection 
    {
    public:
      /// connection status
      enum Status
      {
        /// ready for new commands
        Ready,
        /// functionality not implemented yet
        NotImplemented,
        /// wrong parameter
        ParameterError,
        /// timeout reached while awaiting a reponse from microcontroller
        Timeout,
        /// microcontroller reports busy status
        Busy,
        /// microcontroller is Locked
        Locked,
        /// erase operation has been failed
        EraseFailed,
        /// cannot open communication port
        CantOpenPort,
        /// no connection 
        NotConnected
      };
      /// microcontroller type
      enum Device
      {
        R8C,
        M16C,
        M32C,
        R32C
      };

      /// @brief create a connection
      Connection();
      /// close and destroy an existing connection
      ~Connection();
      /** @brief open a connection
       * @param _portName name/path of the communictaion port
       * @param _device device type to communicate with
       */
      void open( const QString& _portName, Device _device );
      /// close existing connection
      void close();
      /// initiate communication at low baud rate
      Status autoBaud();
      /// return currently used baud rate
      QSerialPort::BaudRate baud() const;
      /// negotiate optimal baud rate
      Status baudRate();
      /// query version string from microcontroller
      Status version( QString& _version );
      /// query current status from microcontroller
      Status status();
      /// poll up to 16 tyimes for status until remote side reports Ready
      Status waitForReady();
      /// unlock microcontroller with given ID
      Status unlock(const QByteArray& _id );
      /// erase user ROM of the microcontroller
      Status eraseAll();
      /// program a page ofe flash memory into the microcontroller
      Status programPage( unsigned long address, const QByteArray& _bytes );
      /// read a page of flash memory from the microcontroller
      Status readPage( unsigned long _address, QByteArray& _bytes );

    protected:
      /// write multiple bytes to the microcontroller 
      qint64 write( const QByteArray& _bytes );
      /// write a single byte to the microcontroller 
      qint64 write( char _byte );
      /** @brief read what has been received from the microcontroller recently
       * @param _count bytes to read 
       */
      QByteArray read( int _count );
      QByteArray readLine();

    private:
      /// current communication port
      QSerialPort *port_;
      /// name of the current communication port
      QString portName_;
      /// current device type
      Device device_;
    };
  }
}

