
/////////////////////////////////////////////////////////////////////
//
// DNIT - Departamento Nacional de Infraestrutura de Transportes
// STE - Servicos Tecnicos de Engenharia, SA
//
// Autor    Andre Caceres Carrilho
// Contato  andrecarrilho@ste-simemp.com.br
// Data     08 jun 2022  created
// Fix1     30 jun 2022  fix bouding box computation
// Fix2     18 jul 2022  fix multiple segfault errors
//
/////////////////////////////////////////////////////////////////////

#include <sys/stat.h>

#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <vector>
#include <sstream>

#include <shapefil.h>
#include <args.hxx>

//#define _DEBUG 1
//#define _VERBOSE 1

#ifdef _VERBOSE
    #define SHP2GPX_MSG(x)      std::cout << x << std::endl;
#else
    #define SHP2GPX_MSG(x)
#endif

// https://stackoverflow.com/questions/12774207
bool FileExists(const std::string s_filename)  {
    struct stat buffer;
    return (stat (s_filename.c_str(), &buffer) == 0 );
}


struct GPXPoint {
    double          _lat;
    double          _lon;
    std::string     _name;

#ifdef _DEBUG
    void Print() {
        std::cout
            << "\t" << _name
            << "\t" << _lat << "\t" << _lon
            << std::endl;
    }
#endif

    std::string
    ToGPXString() {
        std::stringstream ss;
        ss << std::fixed;
        ss << std::setprecision(8);
        ss << "<wpt lat=\"" << _lat << "\" lon=\"" << _lon << "\">" << std::endl;
        ss << "  <name>" << _name << "</name>" << std::endl;
        ss << "  <sym>Flag, Blue</sym>" << std::endl;
      // elevacao (altura)  <ele>5.388971</ele>
        ss << "</wpt>" << std::endl;
        return ss.str();
    }
};

class Shapefile {
private:
    struct DBF_field_info_S {
        char    _szname[16];
        int     _width;
        int     _decimals;

#ifdef _DEBUG
        void Print() {
            std::cout << "\t" << _szname << "\t"
                << _width << "\t" << _decimals << std::endl;
        }
#endif
    };


private:
    SHPHandle       _shp_handle;
    DBFHandle       _dbf_handle;
public:
    Shapefile ( void)
        : _shp_handle(nullptr)
        , _dbf_handle(nullptr)
    { }

    ~Shapefile ( void)
        { _Close(); }

    bool
    Open(const char *pszShapeFile) {
        _Close();
        _shp_handle = SHPOpen(pszShapeFile, "rb");
        if (!_shp_handle)
            {   return false; }
        _dbf_handle = DBFOpen(pszShapeFile, "rb");
        if (!_dbf_handle)
            {   return false; }
#ifdef _DEBUG
        _DebugShpHandle(_shp_handle);
        _DebugDbfHandle(_dbf_handle);
#endif
        if(_shp_handle->nShapeType != SHPT_POINT) {
            SHP2GPX_MSG("Invalid Shapefile geometry type. Must be point.")
            _Close();
            return false;
        }
        if (_shp_handle->nRecords != _dbf_handle->nRecords) {
            SHP2GPX_MSG("Different record count between SHP and DBF files.")
            _Close();
            return false;
        }
        return true;
    }
    std::vector<GPXPoint>
    GetData( void) const {
        std::vector<GPXPoint> dat;
        dat.reserve(_dbf_handle->nRecords);

        // Case Insensitive
        int fieldIndex = DBFGetFieldIndex( _dbf_handle, "nome");
        if (fieldIndex < 0)
            { fieldIndex = DBFGetFieldIndex( _dbf_handle, "name"); }

        for (int i = 0; i < _dbf_handle->nRecords; ++i ) {
            GPXPoint current;

            SHPObject *iObj = SHPReadObject(_shp_handle, i);
            current._lat = iObj->padfY[0];
            current._lon = iObj->padfX[0];

            #if defined (_DEBUG) && defined(_VERBOSE)
                #if _VERBOSE > 1
                _DebugShpObject(iObj);
                #endif
            #endif

            SHPDestroyObject(iObj);

            if (fieldIndex >= 0 ) {
                current._name = DBFReadStringAttribute( _dbf_handle,
                    i, fieldIndex);
            } else {
                current._name = std::to_string(i+1);
            }
            dat.push_back(current);
        }

        return dat;
    }

private:
    void _Close ( void) {
        if (_shp_handle)
            {   SHPClose(_shp_handle); }
        if (_dbf_handle)
            {   DBFClose(_dbf_handle); }
    }
#ifdef _DEBUG
    static void _DebugShpObject ( SHPObject * _obj ) {
        std::cout << std::fixed;
        std::cout << std::setprecision(7);
        std::cout << "OBJ Type: " << _obj->nSHPType << std::endl;
        std::cout << "OBJ Num Vertices" << _obj->nVertices << std::endl;
        for ( int i = 0; i < _obj->nVertices; ++i ) {
            std::cout << "Vert #" << i+1 << " " <<
                _obj->padfX[i] << " " <<
                _obj->padfY[i] << /*" " <<
                _obj->padfZ[i] << */std::endl;

        }
    }
    static void _DebugShpHandle ( SHPHandle _hnd ) {
        std::cout << "SHP Type: " << _hnd->nShapeType << std::endl;
        std::cout << "SHP file size: " << _hnd->nFileSize << " bytes" << std::endl;
        std::cout << "SHP Num Records: " << _hnd->nRecords << std::endl;
    }
    static void _DebugDbfHandle ( DBFHandle _hnd ) {
        std::cout << "DBF Num Records: " << _hnd->nRecords << std::endl;
        std::cout << "DBF Num Fields: " << _hnd->nFields << std::endl;
    }
#endif
};

struct LLbounds {
    double      minlat, minlon;
    double      maxlat, maxlon;

    LLbounds()
        { Reset(); }

    void Reset() {
        minlat =  100.0; minlon =  400.0;
        maxlat = -100.0; maxlon = -400.0;
    }

    void Update(double _lat, double _lon) {
        minlat = std::min(minlat, _lat);
        maxlat = std::max(maxlat, _lat);
        minlon = std::min(minlon, _lon);
        maxlon = std::max(maxlon, _lon);
    }
};

std::string
GetGPXHeader(const std::vector<GPXPoint>& pts) {
    LLbounds box;

    for (auto pt : pts)
        box.Update(pt._lat, pt._lon);

    std::stringstream ss;
    ss << std::fixed;
    ss << std::setprecision(8);
    ss << "<?xml version=\"1.0\"?>" << std::endl;
    ss << "<gpx version=\"1.1\" creator=\"GDAL 3.0.4\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
    ss << " xmlns=\"http://www.topografix.com/GPX/1/1\" ";
    ss << "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">" << std::endl;
    ss << "<metadata>";
    ss << "<bounds minlat=\"" << box.minlat << "\" minlon=\"" << box.minlon <<
               "\" maxlat=\"" << box.maxlat << "\" maxlon=\"" << box.maxlon << "\" />";
    ss << "</metadata>\n";

    return ss.str();
}

// ALTERNATIVA AO STD::FILESYSTEM DEVIDO AO BUG:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78870
namespace PATH {
    template<class T>
    T base_name(T const & path, T const & delims = "/\\")
    {
      return path.substr(path.find_last_of(delims) + 1);
    }
    template<class T>
    T remove_extension(T const & filename)
    {
      typename T::size_type const p(filename.find_last_of('.'));
      return p > 0 && p != T::npos ? filename.substr(0, p) : filename;
    }
}

class Shp2Gpx_Args {
public:
    std::string         f_shp;
    std::string         f_gpx;

public:
    int Parse(int argc, char **argv)
    {
        args::ArgumentParser parser("Conversion from SHP to GPX");
        parser.LongPrefix("");
        parser.LongSeparator("=");
        args::HelpFlag help(parser, "HELP", "Show this help menu", {"help"});
        args::ValueFlag<std::string> input(parser, "SHAPE FILE", "ESRI Shape filename", {"shp"});
        args::ValueFlag<std::string> output(parser, "GPX FILE", "GPX filename", {"gpx"});
        try {
            parser.ParseCLI(argc, argv);
        }
        catch (args::Help) {
            std::cout << parser;
            return 1;
        }
        catch (args::ParseError e) {
            std::cerr << e.what() << std::endl;
            std::cerr << parser;
            return 1;
        }
        catch (args::ValidationError e) {
            std::cerr << e.what() << std::endl;
            std::cerr << parser;
            return 1;
        }
        if (!input || !output) {
            std::cout << parser;
            return 1;
        }

        f_shp = RemoveExtension(args::get(input));
        f_gpx = RemoveExtension(args::get(output)) + ".gpx";
        return 0;
    }

private:
    std::string  RemoveExtension(const std::string& s_path) {
        return PATH::remove_extension<std::string>(s_path);
    }
};

void RunFile(const std::string& f_in, const std::string& f_out) {
    std::ofstream dst(f_out.c_str());

    dst << std::fixed;
    dst << std::setprecision(7);

    Shapefile m_shape;

    if (!FileExists(f_in + ".shp" )) {
        SHP2GPX_MSG("SHP file does not exist in this directory. Check filename.")
        return;
    }
    if (!FileExists(f_in + ".dbf" )) {
        SHP2GPX_MSG("DBF file does not exist in this directory. Check filename.")
        return;
    }

    m_shape.Open(f_in.c_str());

    std::vector<GPXPoint> dat = m_shape.GetData();

    dst << GetGPXHeader(dat);
    for (auto pt : dat)
        dst << pt.ToGPXString();
}

int main(int argc, char **argv)
{
    Shp2Gpx_Args io_args;
    if (io_args.Parse(argc, argv))
        return 1;

#ifdef _DEBUG
    std::cout << "SHP file: " << io_args.f_shp << std::endl;
    std::cout << "GPX file: " << io_args.f_gpx << std::endl;
#endif

    RunFile(io_args.f_shp, io_args.f_gpx);
    return 0;
}
