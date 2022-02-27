#include <iostream>
#include <fstream>
using namespace std;

#include "comm/util/logutil.h"
#include "comm/util/strutil.h"
#include "comm/kit/cmdline_parser.h"

#include "orderbook.h"

int initial_order_id   = -1;
int current_tick_price = 1;

void Help(int argc, char* argv[])
{
    fprintf(stderr, "usage:%s -h [-s size] [-f file] [-c comp]\n"
                    "where:\n"
                    "-s size: the fixed size of each bid, useful for testing T & R\n"
                    "-f file: the initial command file to load, useful for replay test\n"
                    "-c comp: the comparator for order id, support int & string only, default int\n"
                    "-t tick_price: the initial tick price, default 1\n"
                    "-o order_id: the start of auto-generated order id\n" ,
                    argv[0]);
    exit(0);
}

int GetOrderType(string& order_type, OrderType& type)
{
    static map<string, OrderType> map_order_type = {
            {"S", OrderType_Ask},
            {"B", OrderType_Bid},
    };

    if(map_order_type.find(order_type) == map_order_type.end())
    {
        LOG_ERROR("invalid order type:%s", order_type.c_str());
        return -1;
    }

    type = map_order_type[order_type];
    return (0);
}

void BuildOrderBookFromFile(OrderBook& book, const char* file)
{
    ifstream ifs(file, ios::in);
    if(!ifs.is_open())
    {
        LOG_ERROR("open quote filename:%s failed", file);
        return ;
    }

    string line;
    while(std::getline(ifs, line))
    {
        if(ifs.eof())
        {
            break;
        }

        line = CommUtil::LTrim(line);
        if((line[0] == '#') || (line.length() == 0))
        {
            continue;
        }

        vector<string> parts;
        CommUtil::SepString(line, ",", parts);
        if(parts.size() != 5)
        {
            LOG_ERROR("ignore line(%s) with fields:%zu", line.c_str(), parts.size());
            continue;
        }

        //LOG_DEBUG("line:%s", line.c_str());

        OrderNode info;
        int ret = GetOrderType(parts[2], info.type_);
        if(ret != 0)
        {
            LOG_ERROR("invalid type:%s for line:%s", parts[2].c_str(), line.c_str());
            continue;
        }

        info.id_ = parts[1];
        info.size_ = CommUtil::StrToUInt(parts[3]);
        info.price_ = CommUtil::StrToUInt(parts[4]);

        cout << endl << "Running: " << line << endl;

        switch(parts[0][0])
        {
            case 'A': book.AddOrder(info); break;
            case 'X': book.DeleteOrder(info); break;
            case 'T': book.ResetTickPrice(info.price_); break;
            default:
                LOG_ERROR("invalid action:%s", parts[0].c_str());
                continue;
        }

        /*
         * Print order book after each operation for debugging
         */
        book.Print();
    }

    ifs.close();
}

void ReadItem(const char* output, string& input)
{
    cout << output << ": ";
    cin >> input;
}

void ReadItem(const char* output, int32_t& v)
{
    string input;
    cout << output << ": ";
    cin >> input;
    v = CommUtil::StrToInt(input);
}

/*
 * For further test again order book constructed from file
 */
void ReadFromStdin(OrderBook& book, int size)
{
    cout << endl;
    cout << "----------------------------------" << endl;
    cout << "Reading Stdin for further process:" << endl;
    cout << "----------------------------------" << endl;

    while(true)
    {
        string order_action;
        ReadItem("select action[A,X,T,Q(quit)]", order_action);
        if(order_action[0] == 'Q' || order_action[0] == 'q')
        {
            break;
        }

        OrderNode order_node;
        int ret = 0;

        string order_type;
        ReadItem("select order type[S,B]", order_type);
        ret = GetOrderType(order_type, order_node.type_);
        if(ret != 0)
        {
            continue;
        }

        switch (order_action[0])
        {
            case 'A':
                if(initial_order_id != -1)
                {
                    order_node.id_ = CommUtil::ToStr(initial_order_id++);
                }
                else
                {
                    ReadItem("order id",    order_node.id_);
                }
                ReadItem("order price", order_node.price_);
                ReadItem("order size",  order_node.size_);
                book.AddOrder(order_node);
                break;

            case 'X':
                ReadItem("order id",    order_node.id_);
                book.DeleteOrder(order_node);
                order_node.price_ = order_node.size_ = 0;
                break;

            case 'T':   // reset tick price
                {
                    cout << "reset current tick price: " << current_tick_price << endl;
                    ReadItem("new tick price", order_node.price_);
                    book.ResetTickPrice(order_node.price_);
                }
                break;

            default:
                break;
        }

        LOG_DEBUG("Run user stdin:%s,%s,%s,%d,%d", order_action.c_str(),
                  order_node.id_.c_str(), order_type.c_str(), order_node.size_, order_node.price_
        );
        book.Print();
    }
}

int main(int argc, char* argv[])
{
    CommUtil::CmdLineParser parser("s:f:hc:o:t:");
    parser.Parse(argc, argv);
    if(parser.Has('h'))
    {
        Help(argc, argv);
    }

    OrderIdLessFunc less_func =
            (parser.Get('c') && (strcmp(parser.Get('c'), "int") == 0)) ? OrderIdLessInteger : OrderIdLessString;
    if(parser.Has('o'))
    {
        initial_order_id = CommUtil::StrToInt(parser.Get('o'));
    }
    if(parser.Has('t'))
    {
        current_tick_price = CommUtil::StrToInt(parser.Get('t'));
    }
    OrderBook book(current_tick_price, less_func, 10, 10);
    if(parser.Has('f'))
    {
        BuildOrderBookFromFile(book, parser.Get('f'));
    }

    int size = (parser.Has('s') ? parser.GetInt('s') : 0);
    ReadFromStdin(book, size);

    LOG_DEBUG("clearing orderbook");
    book.Clear();
    book.Print();
}
