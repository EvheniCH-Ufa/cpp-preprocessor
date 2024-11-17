// p05_03_08_SvojPreprocessorSReguljarnymiVyrazhenijamiISyrymiLiteralami.cpp 

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

bool IsInclude(const string& text_line, path& include_path, bool& is_as_path_type)
{
    {
        regex num_reg(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
        smatch m;

        if (regex_match(text_line, m, num_reg))
        {
            is_as_path_type = false;
            include_path = string(m[1]);
            return true;
        }
    }

    {
        regex num_reg(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
        smatch m;

        if (regex_match(text_line, m, num_reg))
        {
            is_as_path_type = true;
            include_path = string(m[1]);
            return true;
        }
    }

    return false;
};


pair<bool, path> SearchFileInIncludeDirectories(const path& file_name, const vector<path>& include_directories)
{
    // тут надо как там: к инклужной дирр аппенд и проверить на экзист и вернуть если что 
    for (const auto& includes_dir_path : include_directories)
    {
        path full_name_of_include = includes_dir_path;
        full_name_of_include.append(file_name.string());

        if (filesystem::exists(full_name_of_include) && (filesystem::status(full_name_of_include).type() == filesystem::file_type::regular))
        {
            return { true, full_name_of_include };
        }
    }
    return { false, path("") };
}

// это основная функция рекурсии: тут передаем путь обрабатываемого файла, файловый поток для записи, ну и вектор инклужных папок
bool Preprocess(const path& in_file, ofstream& out_stream, const vector<path>& include_directories)
{
    int line_num = 0;
    ifstream in_stream(in_file);
    string str;
    while (getline(in_stream, str))
    {
        ++line_num;
        path path_to_file;
        bool is_as_path_type; // false = #include<XXX>, true = #include"XXX/XXX"

        if (IsInclude(str, path_to_file, is_as_path_type))
        {
            // если строковое название, то смотрим сначала в текущей папке, а затем в инклужных, то есть в инклужных смотрим и <> и ""
            // сначала смотрим в текущей папке
            if (is_as_path_type) 
            {
                path full_name_of_include = in_file.parent_path();
                full_name_of_include.append(path_to_file.string());

                if (filesystem::exists(full_name_of_include))
                {
                    if (!Preprocess(full_name_of_include, out_stream, include_directories))
                    {
                        return false;
                    }
                    continue; // если нашли, то <...> не проверяем
                }
            }

            {   // тут уже смотрим в инклужных это и для <> и для "" (если не нашли в текущей) 
                auto [is_findet, full_path_to_file] = SearchFileInIncludeDirectories(path_to_file, include_directories);
                if (is_findet)
                {
                    if (!Preprocess(full_path_to_file, out_stream, include_directories))
                    {
                        return false;
                    }
                    continue;
                }
                else
                {
                    //unknown include file dummy.txt at file sources/a.cpp at line 8
                    //unknown include file <имя файла в директиве> at file <имя файла, где директива> at line <номер строки, где директива>
                    cout << "unknown include file " << path_to_file.string() << " at file " << in_file.string() <<" at line "<< line_num << "\n"s;
                    return false;
                }
            }
        }
        else
        {
            out_stream << str << "\n"s;
        }
    }
    return true;
};


// напишите эту функцию
// это первичная функция тут передаем путь обрабатываемого файла, путь выходного поток для записи, ну и вектор инклужных папок
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories)
{
    if (filesystem::exists(in_file))
    {
        ofstream out_stream(out_file);

        return Preprocess(in_file, out_stream, include_directories);
    }
    else
    {
        return false;
    }
};


string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return { (istreambuf_iterator<char>(stream)), istreambuf_iterator<char>() };
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file <<
            "// this comment before include\n"
            "#include \"dir1/b.h\"\n"
            "// text between b.h and c.h\n"
            "#include \"dir1/d.h\"\n"
            "\n"
            "int SayHello() {\n"
            "    cout << \"hello, world!\" << endl;\n"
            "#   include<dummy.txt>\n"
            "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
            "#include \"subdir/c.h\"\n"
            "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
            "#include <std1.h>\n"
            "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
            "#include \"lib/std2.h\"\n"
            "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

//    Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p, { "sources"_p / "include1"_p,"sources"_p / "include2"_p });
    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p, { "sources"_p / "include1"_p,"sources"_p / "include2"_p })));

    ostringstream test_out;
    test_out << "// this comment before include\n"
        "// text from b.h before include\n"
        "// text from c.h before include\n"
        "// std1\n"
        "// text from c.h after include\n"
        "// text from b.h after include\n"
        "// text between b.h and c.h\n"
        "// text from d.h before include\n"
        "// std2\n"
        "// text from d.h after include\n"
        "\n"
        "int SayHello() {\n"
        "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}
