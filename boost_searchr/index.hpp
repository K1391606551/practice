#pragma once 
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <unordered_map>
#include "util.hpp"
namespace ns_index 
{
    struct DocInfo
    {
        std::string title;
        std::string content;
        std::string url;
        uint64_t doc_id; // 文档i:
    };

    struct InvertedElem 
    {
        uint64_t doc_id;
        std::string word;
        int weight;
    };
    // 倒排拉链
    typedef std::vector<InvertedElem> InvertedList;
    
    class Index
    {
    public:
        ~Index(){}
        static Index* GetInstance()
        {
            if(nullptr == instance)
            {
                mtx.lock();
                if(nullptr == instance)
                {
                    instance = new Index();
                }
                mtx.unlock();
            }
            
            return instance;
        }
        // 根据doc_id找到文档内容
        DocInfo *GetForwardIndex(uint64_t doc_id)
        {
            if(doc_id >= forward_index.size())
            {
                std::cerr << "doc_id out range, error!" << std::endl;
                return nullptr;
            }
            return &forward_index[doc_id];
        }

        // 根据关键字获取倒排拉链
        InvertedList *GetInveretList(const std::string &word)
        {
            auto iter = inverted_index.find(word); // 有疑问
            if(iter == inverted_index.end())
            {
                std::cerr << word << " have no InvertedList" << std::endl;
                return nullptr;
            }
            return &(iter->second);
        }

        // 构建文档索引
        // 根据去标签，格式化之后的文档，构建正排和倒排索引 
        // data/raw_html/raw.txt
        bool BuildIndex(const std::string &input) // parse处理完毕的数据交给我
        {
            std::ifstream in(input, std::ios::in | std::ios::binary);
            if(!in.is_open())
            {
                std::cerr << "sorry, " << input << " open error" << std::endl;
                return false;
            }
            std::string line;
            int count = 0;
            while(std::getline(in, line))
            {
                DocInfo *doc = BuildForwardIndex(line);
                if(nullptr == doc)
                {
                    std::cerr << "build " << line << " error" << std::endl; // for debug
                    continue; 
                }

                BuildInvertedIndex(*doc);
                count++;
                if(count % 50 == 0)
                    std::cout << "当前已经建立的索引文档： " << count << std::endl;
            }

            in.close();
            return true;
        }

    private:
        DocInfo *BuildForwardIndex(const std::string &line)
        {
            // 1.解析line，字符串切分
            const std::string sep = "\3";
            std::vector<std::string> results;
            ns_util::StringUtil::Split(line, &results, sep);
            if(3 != results.size())
            {
                return nullptr;
            }
            // 2.字符串进行填充到Docinfo
            DocInfo doc;
            doc.title = results[0];
            doc.content = results[1];
            doc.url = results[2];
            doc.doc_id = forward_index.size(); // 先进行保存id，在插入，对应的id就是当前doc在vector中的下标！
            // 3.插入到正排索引的vector
            forward_index.push_back(std::move(doc));

            return &forward_index.back();
        }

       bool BuildInvertedIndex(const DocInfo &doc)
       {
           struct word_cnt
           {
               int title_cnt = 0;
               int content_cnt = 0;
           };
           
           std::unordered_map<std::string, word_cnt> word_map; // 用来暂存词频的映射表
           std::vector<std::string> title_words;
           ns_util::JiebaUtil::CutString(doc.title, &title_words); 
           
           for(std::string s : title_words)
           {
               boost::to_lower(s); // 将我们的分词进行统一转化为小写的
               word_map[s].title_cnt++; // 如果存在就获取，如果不存在就新建
           }

           std::vector<std::string> content_words;
           ns_util::JiebaUtil::CutString(doc.content, &content_words);
           for(std::string s :content_words)
           {
               boost::to_lower(s); // 将我们的分词进行统一转化为小写的
               word_map[s].content_cnt++;
           }

           const int X = 10;
           const int Y = 1;

           for(auto &word_pair : word_map)
           {
               InvertedElem item;
               item.doc_id = doc.doc_id;
               item.word = word_pair.first;
               item.weight = X * word_pair.second.title_cnt + Y * word_pair.second.content_cnt; // 相关性
               InvertedList &inverted_list = inverted_index[word_pair.first]; // 不理解
               inverted_list.push_back(std::move(item));
           }
           
           return true;
       }
    private:
        // 正排索引的数据结果用数组，数组的下标就是文档id
        std::vector<DocInfo> forward_index;
        // 倒排索引一定是一个关键字和一组(个)InvertedElem对应
        std::unordered_map<std::string, InvertedList> inverted_index;        
    private: 
        Index(){}
        Index(const Index&) = delete;
        Index& operator=(const Index&) = delete;

        static Index* instance;
        static std::mutex mtx;
    };
     Index* Index::instance = nullptr;
     std::mutex Index::mtx;
}
