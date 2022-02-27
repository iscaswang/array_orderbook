//
// This program is yet another implementation of orderbook which
// supports order match & deletion using array as container of price
// nodes instead of using link list again.
//
#pragma once

#include <string>
using namespace std;

#include "expr/iscaswang/comm/ds/double_list.h"
#include "../orderbook/commdef.h"

//
// basic info for each order
//
typedef struct OrderNode
{
    int32_t price_ = 0;
    string  id_;
    int32_t size_  = 0;
    OrderType type_= OrderType_Min_Invalid;

    bool operator==(const OrderNode& t) const
    {
        return id_ == t.id_;
    }

    OrderNode() {}
    OrderNode(int32_t price, const string& id, int32_t size, OrderType type) :
        price_(price), id_(id), size_(size), type_(type)
    {}

} OrderNode;

typedef bool (*OrderIdLessFunc)(const OrderNode& a, const OrderNode& b);
bool OrderIdLessString(const OrderNode& a, const OrderNode& b);
bool OrderIdLessInteger(const OrderNode& a, const OrderNode& b);

class Depth
{
public:
    Depth(int index_step,
          int step_size,
          int initial_size,
          int type,
          int tick_price,
          OrderIdLessFunc order_id_less_func
    );

    ~Depth();

    /*
     * match order node by price & size. Modify order_node with values
     * after match
     */
    void Match(OrderNode& order_node);

    /*
     * print current depth, including price and all nodes(size, id) of that price level
     */
    void Print();

    /*
     * add one order node into depth
     */
    void Add(OrderNode& order_node);

    /*
     * release all order nodes allocated in depth
     */
    void Clear();

    /*
     * delete order node from depth
     */
    void DeleteOrder(OrderNode &order_node);

    /*
     * get the index in price array since top
     */
    inline int GetIndexByPrice(int32_t price);

    /*
     * called only if new tick price is less than current tick price
     */
    void ResetTickPrice(int32_t price);

private:
    /*
     * create new link node array
     */
    LinkNode<OrderNode>** CreateLinkNodeArray(int size);

    /*
     * add order node into depth with price node index idx
     */
    void AddLinkNode(int idx, const OrderNode& order_node);

    /*
     * reset top index of price node in the array. Used after order matching
     * or deletion
     */
    void ResetTop();

    /*
     * reset bottom index of price node in the array. Used after order deletion.
     */
    void ResetBottom();

    int top_          = -1; // top of price_nodes_
    int bottom_       = -1; // bottom of price_nodes_
    int current_size_ = 0;  // current array size of price_nodes_
    int index_step_   = 0;  // index increment step for each variable tick price
    int step_size_    = 0;  // increase step_size_ on capacity enlarge
    int type_         = 0;  // order type, ask or bid
    int tick_price_   = 0;  // price for each tick

    LinkNode<OrderNode>**             price_nodes_;
    map<string, LinkNode<OrderNode>*> map_link_nodes_;
    OrderIdLessFunc                   order_id_less_func_;
};

class OrderBook
{
public:
    /*
     * tick_price  : the least price incr or decr for each order.
     * order_id_less_func: function for comparing OrderNode when sorting
     * initial_size: the initial array size for price nodes
     * step_size   : enlarge multiple step_size when more price nodes required
     */
    OrderBook(int32_t tick_price,
              OrderIdLessFunc order_id_less_func = OrderIdLessString,
              int initial_size = 1000,
              int step_size = 1000
    );

    /*
     * add order with specified type
     */
    void AddOrder(OrderNode order_node);

    /*
     * delete order with specified id
     */
    void DeleteOrder(OrderNode& order_node);

    /*
     * print the whole order, including ask & bid
     */
    void Print();

    /*
     * release all memory allocated for order nodes
     */
    void Clear();

    /*
     * tick price may change with latest price.
     * For example: VARIABLE.TICK.SIZE with value '0.001 10 0.005 50 0.01 100 0.05'
     */
    void ResetTickPrice(int32_t price);

private:
    int32_t tick_price_;
    Depth ask_;
    Depth bid_;
};
