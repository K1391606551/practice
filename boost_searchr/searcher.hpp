#pragma once 
#include "index.hpp"
#include "util.hpp"
#include <algorithm>
#include "jsoncpp/json/json.h"

namespace ns_searcher 
{
    struct InvertedElemPrint 
    {
        uint64_t doc_id = 0;
        int weight = 0;
        std::vector<std::string> words;

    };
    class Searcher
    {
    public:
        Searcher() {}
        ~Searcher() {}

        void InitSearcher(const std::string & input) 
        {
            // 1. 获取或者创建index对象
            index = ns_index::Index::GetInstance();
            std::cout << "获取单列成功..." << std::endl;
            // 2. 根据index对象建立索引
            index->BuildIndex(input);
            std::cout << "建立正排和倒排索引成功..." << std::endl;
        }
        
        // query: 搜索关键词
        // json_string: 返回给用户浏览器的搜索结构
        
        void Search(const std::string &query, std::string *json_string)
        {
            // 1.[分词]: 对我们的query进行按照searcher的要求进行分词
            std::vector<std::string> words;
            ns_util::JiebaUtil::CutString(query, &words);
            // 2.[触发]: 就是根据分词的各个"词"进行index查找
            // ns_index::InvertedList inverted_list_all;
            std::vector<InvertedElemPrint> inverted_list_all;

            std::unordered_map<uint64_t, InvertedElemPrint> token_map;
            
            for(std::string word : words)
            {
                boost::to_lower(word);

                ns_index::InvertedList *inverted_list = index->GetInveretList(word);
                if(nullptr == inverted_list)
                {
                    continue;
                }
                // inverted_list_all.insert(inverted_list_all.end(), inverted_list->begin(), inverted_list->end());
                for(const auto &elem: *inverted_list)
                {
                    auto &item = token_map[elem.doc_id];
                    item.doc_id = elem.doc_id;
                    item.weight += elem.weight;
                    item.words.push_back(elem.word);
                }
            }
            for(const auto &item: token_map)
            {
                inverted_list_all.push_back(std::move(item.second));
            }
            // 3.[合并排序]: 汇总查找结果，按照相关性进行降序排序
            // std::sort(inverted_list_all.begin(), inverted_list_all.end(),\
            //        [](const ns_index::InvertedElem &e1, const ns_index::InvertedElem &e2)\
            //        { return e1.weight > e2.weight; }); 
            std::sort(inverted_list_all.begin(), inverted_list_all.end(),\
                [](const InvertedElemPrint &e1, const InvertedElemPrint &e2)
                    { return e1.weight > e2.weight; }); 
            // 4.[构建]: 根据查找出的结果，构建json串---jsoncpp--- 通过jsoncpp完成序列化和反序列化
            Json::Value root;
            for(auto &item : inverted_list_all)
            {
                ns_index::DocInfo *doc = index->GetForwardIndex(item.doc_id);
                if(nullptr == doc)
                {
                    continue;
                }
                Json::Value elem;
                elem["title"] = doc->title;
                elem["desc"] = GetDesc(doc->content, item.words[0]); // content是文档的去标签内容，但不是我们想要的，我们想要的只是其中的一部分 TODO
                elem["url"] = doc->url;
                // for debug
                elem["id"] = static_cast<int>(item.doc_id);
                elem["weight"] = item.weight;
                root.append(elem);
            }

            // Json::StyledWriter writer; // 方便调试
            Json::FastWriter writer;
            *json_string = writer.write(root);
        }
    private: 
            std::string GetDesc(const std::string &content, const std::string &word)
            {
                const size_t prev_step = 50;
                const size_t next_step = 100;
                // size_t pos = content.find(word); // 不能用这个找
                
                // if(pos == std::string::npos)
                // {
                //     return "None1";
                // }
                auto iter = std::search(content.begin(), content.end(), word.begin(), word.end(), [](int x, int y)
                        { return std::tolower(x) == std::tolower(y); });
                if(iter == content.end()) return "None1";
                
                size_t pos = std::distance(content.begin(), iter);

                size_t start = 0;
                size_t end = content.size() - 1;
                
                // 注意无符号数的相减
                if(pos > start + prev_step) start = pos - prev_step;
                if(pos + next_step < end) end = pos + next_step;
                if(start > end) return "None2";

                std::string desc = content.substr(start, end -start);
                desc += "...";
                return desc;

            }
    private:
        ns_index::Index *index; // 供系统进行查找的索引
    };
}
