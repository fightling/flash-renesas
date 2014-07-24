#include "Connection.h"
#include "MotFile.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QString>

#define HEX(x) QString::number(x,16)

/** @brief main routine
 * @param _argc argument count
 * @param _argv argument array
 * @return exit value
 */
int main(int _argc, char *_argv[])
{
  // make application instance
  QCoreApplication _a(_argc, _argv);
  {
    _a.setApplicationName("m16c-flasher-utility");
    _a.setApplicationVersion("0.1");
  }
  // initialize argument parser
  QCommandLineParser _parser;
  {
    _parser.setApplicationDescription("M16C flash utility");
    _parser.addHelpOption();
    _parser.addVersionOption();
    _parser.addPositionalArgument("mot", QCoreApplication::translate("main", "MOT file to flash."));
    _parser.addPositionalArgument("port", QCoreApplication::translate("main", "Serial port to connect."));
    _parser.addPositionalArgument("id", QCoreApplication::translate("main", "Flash ID (default: 00:00:00:00:00:00:00 or ff:ff:ff:ff:ff:ff:ff)"),"[id]");
  }
  // Process the actual command line arguments given by the user
  _parser.process(_a);
  QTextStream _out(stdout);
  QTextStream _err(stderr);
  // get positional arguments
  QStringList _args = _parser.positionalArguments();
  // check minimum argument count
  if( _args.size() > 3 )
  {
    // to few parameters
    _err << "ERROR: too many parameters" << endl;
    // exit program
    exit(-1);
  }
  QString _mot;
  QString _port;
  {
    if( _args.size() >= 1 )
      _mot = _args[0];
    else
    {
      // to few parameters
      _err << "ERROR: too few parameters" << endl;
      exit(-1);
    }
    if( _args.size() >= 2 )
      _port = _args[1];
  }
  QByteArray _id;
  if( _args.size() > 2 )
  {
    // split argument into hopefully seven hexadecimal strings
    QStringList _ids = _args[2].split(':');
    // convert hexadecimal strings into a number array
    bool ok = true;
    for( int i=0; i<_ids.size() && ok; ++i )
      _id += _ids[i].toInt(&ok,16);
    // check if there wern't seven bytes
    if( !ok || _id.size() != 7 )
    {
      // invalid ID
      _err << "ERROR: invalid ID" << endl;
      // exit program
      exit(-1);
    }
  }
  using namespace Fkgo::Programmer;
  // create connection to the port given by parameter
  Connection _c;
  if( !_port.isEmpty() )
  {
    _out << "opening connection to port " << _port << endl;
    // create connection to the port given by parameter
    _c.open(_port,Connection::M16C);
    // connect to the microcontroller
    if( Connection::Ready != _c.autoBaud() )
    {
      _err << "ERROR: cannot connect" << endl;
      exit(-1);
    }
    if( Connection::Ready != _c.baudRate() )
    {
      _err << "ERROR: cannot negotiate baud rate" << endl;
      exit(-1);
    }
    _out << "negotiated baud rate: " << _c.baud() << endl;
    // get version
    QString _version;
    if( Connection::Ready != _c.version(_version) )
    {
      _err << "ERROR: cannot get version" << endl;
      exit(-1);
    }
    _out << "remote version: " << _version << endl;
    // unlock the microcontroller with the ID given by argument
    if( _id.isEmpty() )
    {
      _id = QByteArray(7,0); 
      if( Connection::Ready != _c.unlock(_id) )
      {
        _id = QByteArray(7,0xff); 
        if( Connection::Ready != _c.unlock(_id) )
        {
          _err << "ERROR: unlocking failed" << endl;
          exit(-1);
        }
      }
    }
    else if( Connection::Ready != _c.unlock(_id) )
    {
      _err << "ERROR: unlocking failed" << endl;
      exit(-1);
    }
    _out << "unlocked with: " << _id.toHex() << endl;
    // eraseAll
    _out << "erasing flash memory..." << endl;
    if( Connection::Ready != _c.eraseAll() )
    {
      _err << "ERROR: erasing flash memory failed" << endl;
      exit(-1);
    }
  }
  MotFile file(_mot);
  if( !file.open(QIODevice::ReadOnly | QIODevice::Text) )
  {
    _err << "ERROR: MOT file not found" << endl;
    exit(-1);
  }
  if( file.read().type() != SRecord::Header )
  {
    _err << "ERROR: unexpected file format" << endl;
    exit(-1);
  }
  QByteArray _image;
  // blank page
  const QByteArray _blank(0x100,0xff);
  unsigned long _start = file.readImage(_image);
  _out << "Writing image from " << HEX(_start) << " to " << HEX(_start+_image.size()) << " = " << _image.size()/1024 << "KB" << endl;
  unsigned long _count=0;
  for( int _cur = 0; _cur < _image.size(); _cur += 0x100)
  {
    QByteArray _page = _image.mid(_cur, 0x100);
    if( _page != _blank )
    {
      if( !_port.isEmpty() )
      {
        _out << "Writing page at address " << HEX(_start + _cur) << "\r" << flush;
        if( Connection::Ready != _c.programPage( _start + _cur, _page ) )
        {
          _err << "ERROR: programming page failed" << endl;
          exit(-1);
        }
      }
      else
        _out << HEX(_start + _cur) << ": " << _page.left(16).toHex() << "..." << _page.right(16).toHex() << endl;
      _count++;
    }
  }
  _out << "\n" << _count << " relevant pages = " << (_count*0x100)/1024 << "KB at " << HEX(_start) << ".." << HEX(_start + _image.size()-1) << endl;
  return 0;//_a.exec();
}

