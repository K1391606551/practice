#include "searcher.hpp"

const std::string input = "data/raw_html/raw.txt";
int main()
{
    ns_searcher::Searcher * search = new ns_searcher::Searcher();
    search->InitSearcher(input);

    std::string query;
    std::string json_string;

    while(true)
    {
        std::cout << "Please Enter You Search Query# ";
        std::getline(std::cin, query);

        search->Search(query, &json_string);

        std::cout << json_string << std::endl;
    }
    return 0;
}
