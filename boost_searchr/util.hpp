#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include "cppjieba/Jieba.hpp"

namespace ns_util
{
    class FileUtil
    {
    public:
        static bool ReadFile(const std::string &file_path, std::string *out)
        {
            // C++中的文件操作
            std::ifstream in(file_path, std::ios::in);
            if(!in.is_open())
            {
                std::cerr << "open file" << file_path << " error" << std::endl;
                return false;
            }

            std::string line;
            while(std::getline(in, line)) // 如何理解getline读取到文件结束呢？？getline的返回值是一个&，while(bool)，本质是因为返回类型重载了强制类型转化
            {
                *out += line;
            }

            in.close();
            return true;
        }
    };

    class StringUtil
    {
    public:
        static void Split(const std::string &target, std::vector<std::string> *out, const std::string &sep)
        {
            // boost split 
            boost::split(*out, target, boost::is_any_of(sep), boost::token_compress_on);
        }
    };

    const char* const DICT_PATH = "./dict/jieba.dict.utf8";
    const char* const HMM_PATH = "./dict/hmm_model.utf8";
    const char* const USER_DICT_PATH = "./dict/user.dict.utf8";
    const char* const IDF_PATH = "./dict/idf.utf8";
    const char* const STOP_WORD_PATH = "./dict/stop_words.utf8";
    
    class JiebaUtil 
    {
    private:
        static cppjieba::Jieba jieba;  
    public:
        static void CutString(const std::string &src, std::vector<std::string> *out)
        {

            jieba.CutForSearch(src, *out);
        }
    };
    
    cppjieba::Jieba JiebaUtil::jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH,IDF_PATH, STOP_WORD_PATH);
};
