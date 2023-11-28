
#include <boost\program_options.hpp>
#include "fileinfo.h"

#include <wchar.h>
#include <stdlib.h>

namespace po = boost::program_options;
namespace nw=boost::nowide;
using namespace std;




//В данной реализации совместно boost.program_options буду использовать boost.nowide
//так как последняя обещает простую кросплатформенную работу с параметрами командной строки, содержащими символы Unicode


struct Context
{
    std::list<std::string> include_path;
    std::list<std::string> exclude_path;
    bool IncludeSubDir;
    size_t block_size;
    std::string mask;
    size_t min_file_size;
    std::string hash_alg;
    void print()
    {
        for (auto x: include_path) nw::cout<<"incl: "<<x<<endl;
        for (auto x: exclude_path) nw::cout<<"excl: "<<x<<endl;
        nw::cout<<"include sub dir:  "<<((IncludeSubDir)? "yes" : "no")<<endl;

        nw::cout<<"mask: "<<mask<<endl;
        nw::cout<<"min file size: "<<min_file_size<<endl;
        nw::cout<<"block size: "<<block_size<<endl;
        nw::cout<<"hash algorithm: "<<hash_alg<<endl;
        nw::cout<<"----------------------------------------------"<<endl;

    }
};


void  main_func(Context& context)
{

    list<fs::directory_entry> filelist;

    //Проверка файла на все условия: размер, маску
    auto tryFile=[&filelist, &context] (fs::directory_entry& entry )
    {
        if (!fs::is_directory(entry))
        {
             if (fs::file_size(entry)>=context.min_file_size)
             {
                 //nw::cout<<"--->add  "<<context.mask<<" "<<entry.path().extension().generic_string()<<endl;
                 std::string temp=entry.path().extension().generic_string();
                 if ((alg::contains(context.mask, temp.erase(0,1))) || (alg::contains(context.mask, std::string("*"))))
                 filelist.push_back(entry);
             }
        }
    };



    //Функция для создания полного списка файлов
    auto create_fileName_list=[&](std::string path, bool recursive)
    {
        if (recursive)
        {
            fs::recursive_directory_iterator it{path};
            while (it!=fs::recursive_directory_iterator{})
            {
                //nw::cout<<"# "<<*it<<endl;
                tryFile(*it);
                it++;

            }
        }
        else
        {
            fs::directory_iterator it{path};
            while (it!=fs::directory_iterator{})
            {
                //nw::cout<<"# "<<*it<<endl;
                tryFile(*it);
                it++;
            }
        }

    };

    //Функция проверки файла на принадлежность к исключенным каталогам
    auto not_exclude=[&context](std::string str)->bool
    {
        bool res=true;
            for (auto x: context.exclude_path)
            {
                //nw::cout<<"#$#" <<str<<" "<<x<<endl;
                if (str.compare(0, x.size(),x)==0)
                {
                    res=false;
                    break;
                }
            }
       return res;
    };

     //Проходим по всем файлам в заданных каталогах
    for (auto x: context.include_path)
    {
         create_fileName_list(x, context.IncludeSubDir);
    }

    if (filelist.size()<2)
    {
        cout<<"No path for checking..."<<std::endl;
    }


    std::multiset<FileInfo> multiset;
    std::vector<char> array;
    array.reserve(context.block_size);
    for(auto x: filelist)
    {
        fs::path patch1=fs::absolute(x.path());
        if (not_exclude(patch1.generic_string()))//проверка на принадлежность файла к исключенным каталогам
        {
            FileInfo myfile(patch1.generic_string());
            myfile.calculate(context.block_size, array.data(), context.hash_alg);
            //nw::cout<<"CS "<<myfile.getCS()<<endl;
            multiset.insert(myfile);
        }
    }

    //for(auto x : multiset) nw::cout<<"!!!"<<x.getPath()<<endl;
    //nw::cout<<endl;

    //вывод результатов на экран

    bool divider=false;
    for (auto it=multiset.begin(); it!=--(multiset.end()); it++)
    {
        auto it2=it;
        it2++;
        if (*it==*it2)
        {
            if (!divider) nw::cout<<(*it).getPath()<<endl;
            nw::cout<<(*it2).getPath()<<endl;
            divider=true;

        }
        else
        {
            if (divider)
            {
                nw::cout<<endl;
                divider=false;
            }
        }
    }


}







int main(int argc,char *argv[])
{
    try {
        boost::nowide::nowide_filesystem(); //Для переключения локали boost::filesystem на UTF8
        boost::nowide::args a(argc,argv); // Fix arguments - make them UTF-8

        Context context;

        po::options_description desc{"Options"};
        desc.add_options()
                ("help,h", "Show this screen")
                ("include,i", po::value<std::string>()->default_value("current"), "stringlist = including paths (divider '+', space into path's name is resrticted)")
                ("exclude,e", po::value<std::string>()->default_value("none"), "stringlist = excluding paths (divider '+'), space into path's name is resrticted")
                ("sub_dirs,r", po::value<size_t>()->default_value(1), "0 - subdirs off / 1 - subdir on")
                ("file_mask,m", po::value<std::string>()->default_value("*"), "stringlist =file extentions (divider '+')")
                ("file_size,s", po::value<size_t>()->default_value(1), "min file size (size_t)")
                ("block_size,b", po::value<size_t>()->default_value(100000), "block size (size_t)")
                ("chek_summ,c", po::value<std::string>()->default_value("crc32"), "chek summ algorithm (may be: crc32 or md5)");
        po::variables_map vm;
        po::store(parse_command_line(argc, argv, desc), vm);
        if (vm.count("help"))
            std::cout << desc << '\n';
        else
        {
            if (vm.count("include"))
            {
                auto str=vm["include"].as<std::string>();
                auto cur_path=fs::current_path().generic_string();
                vector<std::string> vector;
                if (str!="current")
                {

                    boost::algorithm::split(vector, str, [](char ch){return (ch=='+');});
                    for (auto x: vector)
                    {
                        if (fs::exists(x))
                        {
                            fs::path temp=fs::absolute(x);
                            auto str2=temp.generic_string();
                            context.include_path.push_back(str2);
                        }
                    }
                }
                else
                {
                    context.include_path.push_back(fs::absolute(cur_path).generic_string());
                }
            }
            if (vm.count("exclude"))
            {
                auto str=vm["exclude"].as<std::string>();
                auto cur_path=fs::current_path().generic_string();
                if (str!="none")
                {
                    vector<std::string> vector;
                    boost::algorithm::split(vector, str, [](char ch){return (ch=='+');});
                    for (auto x: vector)
                    {
                        if (fs::exists(x))
                        {
                            fs::path temp=fs::absolute(x);
                            auto str2=temp.generic_string();
                            context.exclude_path.push_back(str2);
                        }
                    }
                }
            }
            if (vm.count("sub_dirs"))
            {
                context.IncludeSubDir=static_cast<bool>(vm["sub_dirs"].as<size_t>());
            }
            if (vm.count("file_mask"))
            {
                auto str=vm["file_mask"].as<std::string>();
                if (str!="*")
                {
                    auto upper_str=str;
                    alg::to_upper(upper_str);
                    auto lower_str=str;
                    alg::to_lower(lower_str);
                    str=upper_str+'+'+lower_str;
                }
                context.mask=str;
            }
            if (vm.count("block_size"))
            {
                context.block_size=vm["block_size"].as<std::size_t>();
            }
            if (vm.count("file_size"))
            {
                context.min_file_size=vm["file_size"].as<std::size_t>();
            }
            if (vm.count("chek_summ"))
            {
                context.hash_alg=vm["chek_summ"].as<std::string>();
            }
        }
        // context.print();
        main_func(context);

    }
    catch (const std::exception &e) {
        std::cout << "EXCEPTION "<<e.what() << std::endl;
    }


    return 0;
}





