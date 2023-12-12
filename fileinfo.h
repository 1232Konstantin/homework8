#ifndef FILEINFO_H
#define FILEINFO_H
#include <string>
#include <list>
#include <iostream>
#include <locale>
#include <vector>
#include <boost\crc.hpp>
#include <boost\filesystem.hpp>
#include <boost\algorithm\string.hpp>
#include <boost\algorithm\hex.hpp>
#include <boost\uuid\detail\md5.hpp>
#include <set>

#include <boost/nowide/args.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/nowide/filesystem.hpp>

namespace  fs= boost::filesystem;
namespace  alg=boost::algorithm;
using boost::uuids::detail::md5;







class HashStrategy
{
    //inline static std::vector<unsigned char> v;
    static uint32_t GetCrc32(size_t block_size, char*block_addr)
    {
        boost::crc_32_type result;
        result.process_bytes(block_addr, block_size);
        return result.checksum();
    }

    template<typename T>
    static std::string to_hex_string(T& val)
    {
        std::string res;
        const auto charDigest=reinterpret_cast<const char*>(&val);
        boost::algorithm::hex(charDigest, charDigest+sizeof(T), std::back_inserter(res));
        return res;
    }

public:
    static std::string calculate(size_t block_size, char*block_addr, std::string hash_alg)
    {
        if (hash_alg=="crc32")
        {
            uint32_t res=GetCrc32(block_size, block_addr);
            return to_hex_string(res);
        }
        if (hash_alg=="md5")
        {
            md5 hash;
            md5::digest_type digest;
            hash.process_bytes(block_addr, block_size);
            hash.get_digest(digest);
            return to_hex_string(digest);
        }
        else throw(std::logic_error("uncknown hash algorithm"));

    }
};




class FileInfo
{
    std::string m_check_sum;
    std::string m_path;
    size_t m_pointer=0;
    bool m_stop=false;
public:
    FileInfo() {}
    FileInfo(std::string path) : m_path(path){}
    FileInfo(const FileInfo& other)
    {
        m_check_sum =other.m_check_sum;
        m_path      =other.m_path;
        m_pointer   =other.m_pointer;
        m_stop      =other.m_stop;
    }

    std::string getPath() const {return m_path;}
    std::string getCS() const {return m_check_sum;}
    bool get_stop() const {return m_stop;}

    bool operator<(const FileInfo& other) const
    {
        return (m_check_sum<other.m_check_sum);
    }

    bool operator==(const FileInfo& other) const
    {
        return (m_check_sum==other.m_check_sum);
    }
    void calculate(size_t block_size, char*block_addr, std::string hash_alg)
        {
            //std::cout<<"m_pos"<<m_pointer<<std::endl;

            if (m_stop) return;
            setlocale(LC_ALL, ""); //Для работы с широкими символами
            boost::nowide::ifstream stream;
            stream.open(m_path, std::ios::binary);
            stream.seekg(m_pointer); //перемещаемся на точку где остановились в прошлый раз


            stream.read(block_addr, block_size);
            m_pointer+= block_size;
            //std::cout<<"read "<<block_size<<" "<<stream.gcount()<<std::endl;
            if (stream.gcount()==block_size)
            {
                m_check_sum+=HashStrategy::calculate(block_size, block_addr, hash_alg);
                //std::cout<<m_pointer<<" "<<m_stop<<" ->"<<m_check_sum<<std::endl;
            }
            else
            {
                m_stop=true;
                //std::cout<<"SDFE"<<std::endl;
                if (stream.gcount()!=0)
                {
                    std::vector<char> v;
                    v.resize(block_size);
                    memcpy(v.data(), block_addr,block_size);
                    m_check_sum+=HashStrategy::calculate(block_size, v.data(), hash_alg);

                }
            }

            stream.close();

        }
};








#endif // FILEINFO_H
