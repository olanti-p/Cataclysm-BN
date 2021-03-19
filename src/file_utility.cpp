#include "file_utility.h"

#include "debug.h"
#include "filesystem.h"
#include "json.h"
#include "output.h"

#include <fstream>
#include <sstream>

static std::ios_base::openmode cata_ios_mode_to_std( std::ios_base::openmode dir, cata_ios_mode m )
{
    std::ios_base::openmode smode = dir;
    if( static_cast<int>( m ) & static_cast<int>( cata_ios_mode::binary ) ) {
        smode |= std::ios_base::binary;
    }
    if( static_cast<int>( m ) & static_cast<int>( cata_ios_mode::app ) ) {
        smode |= std::ios_base::app;
    }
    return smode;
}

#if defined (_WIN32) && !defined (_MSC_VER)
// On Linux/MacOS, UTF-8 paths work out of the box, and we pass the string as is.
// On Windows, narrow API does not recognize paths encoded with UTF-8,
// so we have to jump through hoops to use wide API:
// * with Visual Studio, there's a non-standard fstream constructor that
//   accepts wstring for path; we just need to convert UTF-8 to UTF-16.
// * when cross-compiling with mingw64, there's a non-standard __gnu_cxx::stdio_filebuf
//   that allows creating fstream from FILE, and _wfopen that allows opening FILE
//   using wide string for path.
// Although Windows 10 insider build 17035 (November 2017) enables narrow API to use UTF-8
// paths, we can't rely on it here for backwards compatibility.

cata_ofstream &cata_ofstream::operator=( cata_ofstream &&x )
{
    _stream = std::move( x._stream );
    _buffer = std::move( x._buffer );
    _file = x._file;
    x._file = nullptr;
    _binary = x._binary;
    _append = x._append;
    return *this;
}

cata_ofstream &cata_ofstream::open( const std::string &path )
{
    std::wstring mode;
    if( _append ) {
        mode = L"a";
    } else {
        mode = L"w";
    }
    if( _binary ) {
        mode += L"b";
    }

    _file = _wfopen( utf8_to_wstr( path ).c_str(), mode.c_str() );
    if( !_file ) {
        // failed
        return *this;
    }

    std::ios_base::openmode smode = std::ios_base::out;
    if( _binary ) {
        smode |= std::ios_base::binary;
    }
    if( _append ) {
        smode |= std::ios_base::app;
    }

    _buffer = std::make_unique<__gnu_cxx::stdio_filebuf<char>>( _file, smode );
    _stream = std::make_unique<std::ostream>( &*_buffer );

    return *this;
}

bool cata_ofstream::is_open()
{
    return _file;
}

void cata_ofstream::close()
{
    if( _stream ) {
        _stream->flush();
        _stream.reset();
    }
    _buffer.reset();
    if( _file ) {
        fclose( _file );
        _file = nullptr;
    }
}

#else // defined (_WIN32) && !defined (_MSC_VER)

cata_ofstream &cata_ofstream::operator=( cata_ofstream &&x )
{
    _stream = std::move( x._stream );
    _mode = x._mode;
    return *this;
}

cata_ofstream &cata_ofstream::open( const std::string &path )
{
    std::ios_base::openmode mode = cata_ios_mode_to_std( std::ios_base::out, _mode );

#if defined (_MSC_VER)
    _stream = std::make_unique<std::ofstream>( utf8_to_wstr( path ), mode );
#else
    _stream = std::make_unique<std::ofstream>( path, mode );
#endif
    return *this;
}

bool cata_ofstream::is_open()
{
    return _stream && _stream->is_open();
}

void cata_ofstream::close()
{
    if( _stream ) {
        _stream->close();
        _stream.reset();
    }
}

#endif // defined (_WIN32) && !defined (_MSC_VER)

cata_ofstream::cata_ofstream() = default;

cata_ofstream::cata_ofstream( cata_ofstream &&x )
{
    *this = std::move( x );
}

cata_ofstream::~cata_ofstream()
{
    close();
}

bool cata_ofstream::fail()
{
    return !_stream || _stream->fail();
}

bool cata_ofstream::bad()
{
    return !_stream || _stream->bad();
}

void cata_ofstream::flush()
{
    _stream->flush();
}

std::ostream &cata_ofstream::operator*()
{
    return *_stream;
}

std::ostream *cata_ofstream::operator->()
{
    return &*_stream;
}

#if defined (_WIN32) && !defined (_MSC_VER)

cata_ifstream &cata_ifstream::operator=( cata_ifstream &&x )
{
    _stream = std::move( x._stream );
    _buffer = std::move( x._buffer );
    _file = x._file;
    x._file = nullptr;
    _binary = x._binary;
    return *this;
}

cata_ifstream &cata_ifstream::open( const std::string &path )
{
    std::wstring mode = L"r";
    if( _binary ) {
        mode += L"b";
    }

    _file = _wfopen( utf8_to_wstr( path ).c_str(), mode.c_str() );
    if( !_file ) {
        // failed
        return *this;
    }

    std::ios_base::openmode smode = std::ios_base::in;
    if( _binary ) {
        smode |= std::ios_base::binary;
    }

    _buffer = std::make_unique<__gnu_cxx::stdio_filebuf<char>>( _file, smode );
    _stream = std::make_unique<std::istream>( &*_buffer );

    return *this;
}

bool cata_ifstream::is_open()
{
    return _file;
}

void cata_ifstream::close()
{
    if( _stream ) {
        _stream.reset();
    }
    _buffer.reset();
    if( _file ) {
        fclose( _file );
        _file = nullptr;
    }
}

#else // defined (_WIN32) && !defined (_MSC_VER)

cata_ifstream &cata_ifstream::operator=( cata_ifstream &&x )
{
    _stream = std::move( x._stream );
    _mode = x._mode;
    return *this;
}

cata_ifstream &cata_ifstream::open( const std::string &path )
{
    std::ios_base::openmode mode = cata_ios_mode_to_std( std::ios_base::in, _mode );

#if defined (_MSC_VER)
    _stream = std::make_unique<std::ifstream>( utf8_to_wstr( path ), mode );
#else
    _stream = std::make_unique<std::ifstream>( path, mode );
#endif
    return *this;
}

bool cata_ifstream::is_open()
{
    return _stream && _stream->is_open();
}

void cata_ifstream::close()
{
    if( _stream ) {
        _stream->close();
        _stream.reset();
    }
}

#endif // defined (_WIN32) && !defined (_MSC_VER)

cata_ifstream::cata_ifstream() = default;

cata_ifstream::cata_ifstream( cata_ifstream &&x )
{
    *this = std::move( x );
}

cata_ifstream::~cata_ifstream()
{
    close();
}

bool cata_ifstream::fail()
{
    return !_stream || _stream->fail();
}

bool cata_ifstream::bad()
{
    return !_stream || _stream->bad();
}

std::istream &cata_ifstream::operator*()
{
    return *_stream;
}

std::istream *cata_ifstream::operator->()
{
    return &*_stream;
}

void write_to_file( const std::string &path, const std::function<void( std::ostream & )> &writer )
{
    // Any of the below may throw. ofstream_wrapper will clean up the temporary path on its own.
    ofstream_wrapper fout( path, cata_ios_mode::binary );
    writer( fout.stream() );
    fout.close();
}

bool write_to_file( const std::string &path, const std::function<void( std::ostream & )> &writer,
                    const char *const fail_message )
{
    try {
        write_to_file( path, writer );
        return true;

    } catch( const std::exception &err ) {
        if( fail_message ) {
            popup( _( "Failed to write %1$s to \"%2$s\": %3$s" ), fail_message, path.c_str(), err.what() );
        }
        return false;
    }
}

ofstream_wrapper::ofstream_wrapper( const std::string &path, const cata_ios_mode mode )
    : path( path )

{
    open( mode );
}

ofstream_wrapper::~ofstream_wrapper()
{
    try {
        close();
    } catch( ... ) {
        // ignored in destructor
    }
}

std::istream &safe_getline( std::istream &ins, std::string &str )
{
    str.clear();
    std::istream::sentry se( ins, true );
    std::streambuf *sb = ins.rdbuf();

    while( true ) {
        int c = sb->sbumpc();
        switch( c ) {
            case '\n':
                return ins;
            case '\r':
                if( sb->sgetc() == '\n' ) {
                    sb->sbumpc();
                }
                return ins;
            case EOF:
                if( str.empty() ) {
                    ins.setstate( std::ios::eofbit );
                }
                return ins;
            default:
                str += static_cast<char>( c );
        }
    }
}

bool read_from_file( const std::string &path, const std::function<void( std::istream & )> &reader )
{
    try {
        cata_ifstream fin = std::move( cata_ifstream().mode( cata_ios_mode::binary ).open( path ) );
        if( !fin.is_open() ) {
            throw std::runtime_error( "opening file failed" );
        }
        reader( *fin );
        if( fin.bad() ) {
            throw std::runtime_error( "reading file failed" );
        }
        return true;

    } catch( const std::exception &err ) {
        debugmsg( _( "Failed to read from \"%1$s\": %2$s" ), path.c_str(), err.what() );
        return false;
    }
}

bool read_from_file_json( const std::string &path, const std::function<void( JsonIn & )> &reader )
{
    return read_from_file( path, [&reader]( std::istream & fin ) {
        JsonIn jsin( fin );
        reader( jsin );
    } );
}

bool read_from_file( const std::string &path, JsonDeserializer &reader )
{
    return read_from_file_json( path, [&reader]( JsonIn & jsin ) {
        reader.deserialize( jsin );
    } );
}

bool read_from_file_optional( const std::string &path,
                              const std::function<void( std::istream & )> &reader )
{
    // Note: slight race condition here, but we'll ignore it. Worst case: the file
    // exists and got removed before reading it -> reading fails with a message
    // Or file does not exists, than everything works fine because it's optional anyway.
    return file_exist( path ) && read_from_file( path, reader );
}

bool read_from_file_optional_json( const std::string &path,
                                   const std::function<void( JsonIn & )> &reader )
{
    return read_from_file_optional( path, [&reader]( std::istream & fin ) {
        JsonIn jsin( fin );
        reader( jsin );
    } );
}

bool read_from_file_optional( const std::string &path, JsonDeserializer &reader )
{
    return read_from_file_optional_json( path, [&reader]( JsonIn & jsin ) {
        reader.deserialize( jsin );
    } );
}

void ofstream_wrapper::open( cata_ios_mode mode )
{
    if( dir_exist( path ) ) {
        throw std::runtime_error( "target path is a directory" );
    }

    // Create a *unique* temporary path. No other running program should
    // use this path. If the file exists, it must be of a *former* program
    // instance and can savely be deleted.
    temp_path = path + "." + get_pid_string() + ".temp";

    if( file_exist( temp_path ) ) {
        remove_file( temp_path );
    }

    file_stream = std::move( cata_ofstream().mode( mode ).open( temp_path ) );
    if( !file_stream.is_open() ) {
        throw std::runtime_error( "opening file failed" );
    }
}

void ofstream_wrapper::close()
{
    if( !file_stream.is_open() ) {
        return;
    }

    file_stream.flush();
    bool failed = file_stream.fail();
    file_stream.close();
    if( failed ) {
        // Remove the incomplete or otherwise faulty file (if possible).
        // Failures from it are ignored as we can't really do anything about them.
        remove_file( temp_path );
        throw std::runtime_error( "writing to file failed" );
    }
    if( !rename_file( temp_path, path ) ) {
        // Leave the temp path, so the user can move it if possible.
        throw std::runtime_error( "moving temporary file \"" + temp_path + "\" failed" );
    }
}

std::string serialize_wrapper( const std::function<void( JsonOut & )> &callback )
{
    std::ostringstream buffer;
    JsonOut jsout( buffer );
    callback( jsout );
    return buffer.str();
}

void deserialize_wrapper( const std::function<void( JsonIn & )> &callback, const std::string &data )
{
    std::istringstream buffer( data );
    JsonIn jsin( buffer );
    callback( jsin );
}
