#include "Connection.h"
#include <QThread>
#include <QDebug>
#include <QSerialPortInfo>

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
        port_ = new QSerialPort(portName_);
        // setup port
        qDebug() << "Connection::Connection: set baud rate to 9600/8N1";
        port_->setBaudRate(QSerialPort::Baud9600);
        port_->setStopBits(QSerialPort::OneStop);
        port_->setParity(QSerialPort::NoParity);
        port_->setDataBits(QSerialPort::Data8);
        // opern port
        if( port_->open(QIODevice::ReadWrite) )
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
        if( write(0) != 1 )
        {
          qDebug() << "Connection::autoBaud: cannot write";
          return NotConnected;
        }
        // wait
        QThread::msleep(40);
      }
      // read any garbage from the port
      port_->clear();
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
      port_->setBaudRate(QSerialPort::Baud9600);
      // send baud rate 9600 command
      if( write(BAUD_9600) != 1 )
      {
        qDebug() << "Connection::baudRate: not connected";
        return NotConnected;
      }
      // check if reply is unexpected
      if( read(1) != QByteArray(1,BAUD_9600) )
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
        QSerialPort::BaudRate sys_;
      }
      _alternatives[] =
      {
        { BAUD_115200, QSerialPort::Baud115200 },
        { BAUD_57600, QSerialPort::Baud57600 },
        { BAUD_38400, QSerialPort::Baud38400 },
        { BAUD_19200, QSerialPort::Baud19200 },
        // sentinel
        { 0,QSerialPort::Baud9600 },
      };
      // try to improve the baud rate
      for( size_t i=0; _alternatives[i].cmd_; ++i )
      {
        if( QSerialPortInfo::standardBaudRates().contains(_alternatives[i].sys_) )
        {
          qDebug() << "Connection::Connection: asking for baud rate " << _alternatives[i].sys_;
          // send baud rate command
          write(_alternatives[i].cmd_);
          // wait
          QThread::msleep(10);
          // check if reply is unexpected
          if( read(1) == QByteArray(1,_alternatives[i].cmd_) )
          {
            // improve baud rate
            qDebug() << "Connection::Connection: set baud rate to" << _alternatives[i].sys_;
            port_->setBaudRate(_alternatives[i].sys_);
            // cancel baud rate improvement
            return Ready;
          }
        }
      }
      return Ready;
    }
    QSerialPort::BaudRate Connection::baud() const
    {
      if( 0 == port_ )
        // Mama!
        return QSerialPort::Baud9600;
      // return baud rate from port instance
      return (QSerialPort::BaudRate)port_->baudRate();
    }
    Connection::Status Connection::version( QString& _version )
    {
      qDebug() << "Connection::version: asking for remote version";
      // write command:: get version
      // check for error
      if( write(GET_VERSION) != 1 )
      {
        // clear result
        _version.clear();
        // seems not connected
        return NotConnected;
      }
      // read version string
      _version = read(8);
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
        if( write(GET_STATUS) != 1 )
          return NotConnected;
        // read response
        QByteArray result = read(2);
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
      if( write(_seq) != _seq.size() )
        return NotConnected;
      // wait for ready
      return waitForReady();
    }
    Connection::Status Connection::eraseAll()
    {
      // write command: clear status
      if( write(CLEAR_STATUS) != 1 )
        return NotConnected;
      // prepare erase all command sequence
      QByteArray _seq;
      {
        _seq += ERASE;
        _seq += ALL;
      }
      // write command sequence
      if( write(_seq) != _seq.size() )
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
      if( write(_seq) != _seq.size() )
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
      if( write(_seq) != _seq.size() )
        return NotConnected;
      // wait for the response data
      for( int i=0; i<15; ++i )
      {
        _bytes = read(256);
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
      qint64 _written = port_->write(_bytes);
      // write given bytes to port
      if( _written != _bytes.size() )
      {
        qDebug() << "Connection::write: ERROR: Could only write" << _written << "of" << _bytes.size() << "byte(s)";
        // failed
        return _written;
      }
      // flush port
      if( !port_->waitForBytesWritten(1000) )
      {
        qDebug() << "Connection::write: ERROR: Write timeout";
        return 0;
      }
      // ok
      return _written;
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
    QByteArray Connection::read( int _count )
    {
      Q_ASSERT(_count>0);
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
      // wait 1 second for response
      if( !port_->waitForReadyRead(1000) )
      {
        qDebug() << "Connection::read: timeout";
        return QByteArray();
      }
      // read the requestes amount of bytes
      QByteArray _result = port_->read(_count);
      // retry to read bytes until complete or timeout 
      for( int _i=0; _i<1000 && _result.size() < _count; ++_i )
      {
        port_->waitForReadyRead(10);
        _result += port_->read(_count-_result.size());
      }
      // could not read enough bytes?
      if( _result.size() != _count )
        qDebug() << "Connection::read: could not read the requested number of bytes!";a
      // report
      qDebug() << "Connection::read: read" << _result.size() << "/" << _count << "byte(s) =" << _result.toHex();
      return _result;
    }
  }
}
