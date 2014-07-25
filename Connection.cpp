#include "Connection.h"
#include <QThread>
#include <QDebug>

namespace Fkgo
{
  namespace Programmer
  {
    /// remote commands
    enum
    {
      /// program a page
      PROGRAM_PAGE  = 0x41,
      /// most significant byte
      MSB           = 0x48,
      /// clear remote status
      CLEAR_STATUS  = 0x50,
      /// format remote side
      ERASE         = 0xA7,
      ALL           = 0xD0,

      /// request remote status
      GET_STATUS    = 0x70,

      /// set remote baud rate 
      BAUD_9600     = 0xB0,
      BAUD_19200,
      BAUD_38400,
      BAUD_57600,
      BAUD_115200,

      /// unlock flash with ID
      UNLOCK        = 0xF5,

      /// poll version
      GET_VERSION   = 0xFB,

      /// read a page of memory
      READ_PAGE     = 0xFF
    };

    Connection::Connection() :
      port_(0)
    {
    }
    Connection::~Connection()
    {
      // close port 
      close();
    }
    void Connection::open( const QString& _portName, Device _device )
    {
      // port shall be closed
      Q_ASSERT(port_ == 0);
      // remember parameters
      portName_ = _portName;
      device_ = _device;
      // open port
      qDebug() << "Connection::Connection: opening port" << _portName << "...";
      if( 0 == port_ )
      {
        // create port instance
        port_ = new QextSerialPort(portName_, QextSerialPort::Polling);
        // setup port
        qDebug() << "Connection::Connection: set baud rate to 9600/8N1";
        port_->setBaudRate(BAUD9600);
        port_->setStopBits(STOP_1);
        port_->setParity(PAR_NONE);
        port_->setDataBits(DATA_8);
        // opern port
        if (port_->open(QIODevice::ReadWrite) == true)
          qDebug() << "Connection::Connection: successfully opened port" << _portName;
        else
        {
          // abort port on failure
          qDebug() << "Connection::Connection: failed to open port" << _portName;
          delete port_;
          port_ = 0;
        }
      }
    }
    void Connection::close()
    {
      // if port has been opened
      if( 0 != port_ )
      {
        // close port
        port_->close();
        delete port_;
        port_ = 0;
      }
    }
    Connection::Status Connection::autoBaud()
    {
      qDebug() << "Connection::autoBaud: connection to remote site";
      // check if port is open 
      if( 0 == port_ )
      {
        qDebug() << "Connection::autoBaud: not connected";
        return NotConnected;
      }
      // obligatorely wait 3 ms
      QThread::msleep(3);
      // write 16 single null bytes in distance of 20-40 ms
      for( int i=0; i<16; ++i )
      {
        // write null byte
        if( E_NO_ERROR != write(0) )
        {
          qDebug() << "Connection::autoBaud: cannot write";
          return NotConnected;
        }
        // wait
        QThread::msleep(40);
      }
      // read any garbage from the port
      read();
      return Ready;
    }
    Connection::Status Connection::baudRate()
    {
      qDebug() << "Connection::baudRate: negotiating baud rate";
      if( 0 == port_ )
        return NotConnected;
      // set baud rate to 9600
      qDebug() << "Connection::baudRate: set baud rate to 9600";
      // ensure basic baud rate
      port_->setBaudRate(BAUD9600);
      // send baud rate 9600 command
      if( E_NO_ERROR != write(BAUD_9600) )
      {
        qDebug() << "Connection::baudRate: not connected";
        return NotConnected;
      }
      // check if reply is unexpected
      if( read() != QByteArray(1,BAUD_9600) )
      {
        qDebug() << "Connection::baudRate: no response";
        // report failure
        return NotConnected;
      }
      // alternative (faster) baud rates
      const struct
      {
        /// command to send to remote side
        char cmd_;
        /// local system baud rate
        BaudRateType sys_;
      }
      _alternatives[] =
      {
//        { BAUD_115200, BAUD115200 },
        { BAUD_57600, BAUD57600 },
        { BAUD_38400, BAUD38400 },
        { BAUD_19200, BAUD19200 },
        // sentinel
        { 0,BAUD9600 },
      };
      // try to improve the baud rate
      for( size_t i=0; _alternatives[i].cmd_; ++i )
      {
        qDebug() << "Connection::Connection: asking for baud rate " << _alternatives[i].sys_;
        // send baud rate command
        write(_alternatives[i].cmd_);
        // wait
        QThread::msleep(10);
        // check if reply is unexpected
        if( read() == QByteArray(1,_alternatives[i].cmd_) )
        {
          // improve baud rate
          qDebug() << "Connection::Connection: set baud rate to" << _alternatives[i].sys_;
          port_->setBaudRate(_alternatives[i].sys_);
          // cancel baud rate improvement
          return Ready;
        }
      }
      return Ready;
    }
    BaudRateType Connection::baud() const
    {
      if( 0 == port_ )
        // Mama!
        return BAUD9600;
      // return baud rate from port instance
      return port_->baudRate();
    }
    Connection::Status Connection::version( QString& _version )
    {
      qDebug() << "Connection::version: asking for remote version";
      // write command:: get version
      qint64 _err = write(GET_VERSION);
      // check for error
      if( E_NO_ERROR != _err )
      {
        // clear result
        _version.clear();
        // seems not connected
        return NotConnected;
      }
      // read version string
      _version = read();
      // success
      return Ready;
    }
    Connection::Status Connection::status()
    {
      qDebug() << "Connection::status: asking for status";
      // request status up to 16 times
      for( int i=0; i<16; ++i )
      {
        if( i > 0 )
        {
          qDebug() << "Connection::status: Retry #" << i;
          // wait (retry after 1 second))
          QThread::msleep( 1000 );
        }
        // write command: get status
        qint64 _err = write(GET_STATUS);
        // check for error
        if( E_NO_ERROR != _err )
          return NotConnected;
        // read response
        QByteArray result = read();
        // check result
        if( 2 != result.size() )
          continue;
        // fetch flags from status response
        bool _ready = 0x80 == (result[0] & 0x80);
        bool _eraseFailed = 0x10 == (result[0] & 0x10);
        bool _locked = 0x0C != (result[1] & 0x0C);
        qDebug() << "Connection::status: got ready =" << _ready << ", erase failed =" << _eraseFailed << ", locked =" << _locked;
        // priorize status flags 
        if( !_ready )
          return Busy;
        if( _eraseFailed )
          return EraseFailed;
        if( _locked )
          return Locked;
        else
          return Ready;
      }
      qDebug() << "Connection::status: Timeout";
      // failed
      return Timeout;
    }
    Connection::Status Connection::unlock(const QByteArray& _id )
    {
      qDebug() << "Connection::unlock: unlocking with" << _id.toHex();
      // check if remote side is locked
      if( Locked != status() )
        return Ready;
      // check parameter
      if( _id.size() != 7 )
        return ParameterError;
      // prepare command sequence
      QByteArray _seq;
      {
        // write address
        switch(device_)
        {
        case R8C:
          _seq += UNLOCK;
          _seq += 0xdf;
          _seq += 0xff;
          _seq += (char)0x00;
          break;
        case M16C:
          _seq += UNLOCK;
          _seq += 0xdf;
          _seq += 0xff;
          _seq += 0x0f;
          break;
        case M32C:
          _seq += UNLOCK;
          _seq += 0xdf;
          _seq += 0xff;
          _seq += 0xff;
          break;
        case R32C:
          _seq += MSB;
          _seq += 0xff;
          _seq += UNLOCK;
          _seq += 0xdf;
          _seq += 0xff;
          _seq += 0xff;
          break;
        default:
          return NotImplemented;
        }
      }
      // add ID to the command sequence
      _seq += (char)_id.size();
      _seq += _id;
      // write command sequence
      qint64 _err = write(_seq);
      // check for error
      if( E_NO_ERROR != _err )
        return NotConnected;
      // wait for ready
      return waitForReady();
    }
    Connection::Status Connection::eraseAll()
    {
      // write command: clear status
      qint64 _err = write(CLEAR_STATUS);
      // check for error
      if( E_NO_ERROR != _err )
        return NotConnected;
      // prepare erase all command sequence
      QByteArray _seq;
      {
        _seq += ERASE;
        _seq += ALL;
      }
      // write command sequence
      _err = write(_seq);
      // check for error
      if( E_NO_ERROR != _err )
        return NotConnected;
      // wait for status
      return waitForReady();
    }
    Connection::Status Connection::programPage( unsigned long _address, const QByteArray& _bytes )
    {
      // prepare program page command sequence
      QByteArray _seq;
      {
        // 32 bit address when remote side is R32C
        if( device_ == R32C )
        {
          _seq += MSB;
          _seq += (_address >> 24) & 0xff;
        }
        _seq += PROGRAM_PAGE;
        _seq += (_address >> 8) & 0xff;
        _seq += (_address >> 16) & 0xff;
        _seq += _bytes;
      }
      // write command sequence
      qint64 _err = write(_seq);
      // check for error
      if( E_NO_ERROR != _err )
        return NotConnected;
      // wait for finish
      return waitForReady();
    }
    Connection::Status Connection::waitForReady()
    {
      // last status up to 16 times
      Status _status;
      for(int i=0; i<16; ++i )
      {
        // poll status
        _status = status();
        // check if ready
        if( Ready == _status )
          break;
        // check if just locked
        if( Locked == _status )
          break;
        // wait a 50th second
        QThread::msleep(20);
      }
      // return last status
      return _status;
    }
    Connection::Status Connection::readPage( unsigned long _address, QByteArray& _bytes )
    {
      // prepare request
      QByteArray _seq;
      {
        if( device_ == R32C )
        {
          _seq += MSB;
          _seq += (_address >> 24) & 0xff;
        }
        _seq += READ_PAGE;
        _seq += (_address >> 8) & 0xff;
        _seq += (_address >> 16) & 0xff;
      }
      // write request sequence
      qint64 _err = write(_seq);
      // check for error
      if( E_NO_ERROR != _err )
        return NotConnected;
      // wait for the response data
      for( int i=0; i<15; ++i )
      {
        _bytes = read();
        if( _bytes.size() == 256 )
          break;
        // wait a 20th second
        QThread::msleep(50);
      }
      // brrrr...
      return status();
    }
    qint64 Connection::write( const QByteArray& _bytes )
    {
      // check if there is a port to write
      if( 0 == port_ || !port_->isWritable() )
        return NotConnected;
      qDebug() << "Connection::write: writing" << _bytes.size() << "byte(s) =" << _bytes.toHex();
      // write given bytes to port
      if( port_->write(_bytes) < 0 )
        // failed
        return E_WRITE_FAILED;
      // flush port
      port_->flush();
      // ok
      return E_NO_ERROR;
    }
    qint64 Connection::write( char _byte )
    {
      // pack byte into sequence
      QByteArray _seq;
      {
        _seq += _byte;
      }
      // send byte
      return write(_seq);
    }
    QByteArray Connection::read()
    {
      // check if the port is open
      if( 0 == port_ )
      {
        qDebug() << "Connection::read: not connected";
        return QByteArray();
      }
      // check if port is readable
      if( !port_->isReadable() )
      {
        qDebug() << "Connection::read: not readable";
        return QByteArray();
      }
      for( int i=0; i<1000 && 0 == port_->bytesAvailable(); ++i )
        QThread::msleep(1);
      if( 0 == port_->bytesAvailable() )
      {
        qDebug() << "Connection::read: timeout";
        return QByteArray();
      }
      QThread::msleep(20);
      QByteArray _result = port_->readLine();
      qDebug() << "Connection::read: read" << _result.size() << "byte(s) =" << _result.toHex();
      return _result;
    }
  }
}
