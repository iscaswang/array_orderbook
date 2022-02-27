//
// Created by iscaswang on 2021/6/2.
//
#include <assert.h>

#include "comm/util/logutil.h"

#include "orderbook.h"

bool OrderIdLessString(const OrderNode& a, const OrderNode& b)
{
    return a.id_ < b.id_;
}

bool OrderIdLessInteger(const OrderNode& a, const OrderNode& b)
{
    return atoll(a.id_.c_str()) < atoll(b.id_.c_str());
}

const char* order_type_desc[] = {"ask", "bid"};

Depth::Depth(int index_step,
             int step_size,
             int initial_size,
             int type,
             int tick_price,
             OrderIdLessFunc order_id_less_func
    ) : index_step_(index_step), step_size_(step_size), type_(type),
    tick_price_(tick_price), order_id_less_func_(order_id_less_func)
{
    price_nodes_ = CreateLinkNodeArray(initial_size);
    current_size_ = initial_size;
}

Depth::~Depth()
{
    for(int i = 0; i < current_size_; i++)
    {
        if(price_nodes_[i] != NULL)
        {
            ClearLinkList(price_nodes_[i]);
        }
    }

    delete []price_nodes_;
}

void Depth::Match(OrderNode &order_node)
{
    if(top_ < 0) { return; }

    int elem_size = (bottom_ - top_ + current_size_) % current_size_;
    int i = 0, idx = top_;
    for(; (i <= elem_size) && (order_node.size_ > 0); i++, idx = (idx + 1) % current_size_)
    {
        if(price_nodes_[idx] == NULL) { continue; }
        LinkNode<OrderNode>* node = price_nodes_[idx];
        if((type_ == OrderType_Ask && price_nodes_[idx]->value_.price_ <= order_node.price_)
            || (type_ == OrderType_Bid && price_nodes_[idx]->value_.price_ >= order_node.price_)
        )
        {
            while(node && (order_node.size_ > 0))
            {
                if(node->value_.size_ > order_node.size_)
                {
                    LOG_RAW_STDOUT("fully consume %s price %d with size:%d, idx:%d",
                                order_type_desc[type_], node->value_.price_, order_node.size_, idx);
                    node->value_.size_ -= order_node.size_;
                    order_node.size_    = 0;
                }
                else
                {
                    LOG_RAW_STDOUT("partially consume %s price %d with size:%d, idx:%d",
                                order_type_desc[type_], node->value_.price_, node->value_.size_, idx);
                    order_node.size_ -= node->value_.size_;
                    map_link_nodes_.erase(node->value_.id_);
                    PopFrontLinkList(price_nodes_[idx]);
                    node = price_nodes_[idx];
                }
            }
        }
        else
        {
            break;
        }

        if(order_node.size_ == 0) break;
    }

    LOG_RAW_STDOUT("break consume on top:%d, idx:%d", top_, idx);

    ResetTop();
}

void Depth::Print()
{
    printf("%s order with top:%d, bottom:%d, current_size:%d\n",
           order_type_desc[type_], top_, bottom_, current_size_);
    if(top_ == -1)
    {
        return;
    }

    int elem_size = (bottom_ - top_ + current_size_) % current_size_;
    for(int i = 0, idx = top_; i <= elem_size; i++, idx = (idx + 1) % current_size_)
    {
        if(price_nodes_[idx] == NULL) { continue; }
        LinkNode<OrderNode>* node = price_nodes_[idx];
        printf("%d(%d): ", node->value_.price_, idx);
        while(node)
        {
            assert(price_nodes_[idx]->value_.price_ == node->value_.price_);
            printf("%d(%s) ", node->value_.size_, node->value_.id_.c_str());
            node = node->next_;
        }
        printf("\n");
    }
}

void Depth::Add(OrderNode &order_node)
{
    if(top_ == -1)
    {
        top_ = bottom_ = 0;
        AddLinkNode(top_, order_node);
        return;
    }

    int require_size = 0;
    if(type_ == OrderType_Ask)
    {
        if(order_node.price_ >= price_nodes_[top_]->value_.price_)
            require_size = std::abs(order_node.price_ - price_nodes_[top_]->value_.price_) / tick_price_;
        else
            require_size = std::abs(order_node.price_ - price_nodes_[bottom_]->value_.price_) / tick_price_;
    }
    else
    {
        if(order_node.price_ <= price_nodes_[top_]->value_.price_)
            require_size = std::abs(order_node.price_ - price_nodes_[top_]->value_.price_) / tick_price_;
        else
            require_size = std::abs(order_node.price_ - price_nodes_[bottom_]->value_.price_) / tick_price_;
    }

    if(require_size >= current_size_)
    {
        int enlarge_size = require_size / step_size_ * step_size_ + step_size_ - current_size_;
        LOG_RAW_STDOUT("enlarge array by %d", enlarge_size);
        LinkNode<OrderNode>** tmp = CreateLinkNodeArray(current_size_ + enlarge_size);

        // copy old nodes
        int new_top_price = price_nodes_[top_]->value_.price_;

        int elem_size = (bottom_ - top_ + current_size_) % current_size_;
        for(int i = 0, idx = top_; i <= elem_size; i++, idx = (idx + 1) % current_size_)
        {
            if(price_nodes_[idx] == NULL) { continue; }
            int new_index  = (price_nodes_[idx]->value_.price_ - new_top_price) * index_step_ / tick_price_;
            tmp[new_index] = price_nodes_[idx];
            bottom_        = new_index;
        }

        delete []price_nodes_;
        price_nodes_   = tmp;
        top_           = 0;
        current_size_ += enlarge_size;
        LOG_RAW_STDOUT("reset current %s top:%d, bottom:%d", order_type_desc[type_], top_, bottom_);

        Add(order_node);
    }
    else
    {
        int idx = GetIndexByPrice(order_node.price_);
        AddLinkNode(idx, order_node);
        if(type_ == OrderType_Ask)
        {
            if(order_node.price_ < price_nodes_[top_]->value_.price_)
            {
                top_ = idx;
            }
            if(order_node.price_ > price_nodes_[bottom_]->value_.price_)
            {
                bottom_ = idx;
            }
        }
        else if(type_ == OrderType_Bid)
        {
            if(order_node.price_ < price_nodes_[bottom_]->value_.price_)
            {
                bottom_ = idx;
            }
            if(order_node.price_ > price_nodes_[top_]->value_.price_)
            {
                top_ = idx;
            }
        }
        LOG_RAW_STDOUT("current %s top:%d, bottom:%d", order_type_desc[type_], top_, bottom_);
    }
}

void Depth::Clear()
{
    if(top_ == -1) { return; }

    bool overflow = false;
    for(int i = top_; ;i++)
    {
        if(i == current_size_)
        {
            overflow = true;
            i = 0;
        }

        if(i >= top_ && overflow)
        {
            break;
        }

        if(price_nodes_[i] != NULL)
        {
            LOG_RAW_STDOUT("clear %s price:%d", order_type_desc[type_], price_nodes_[i]->value_.price_);
            ClearLinkList(price_nodes_[i]);
        }
    }

    top_ = bottom_ = -1;
    map_link_nodes_.clear();
}

void Depth::AddLinkNode(int idx, const OrderNode &order_node)
{
    LOG_RAW_STDOUT("add price:%d into %s idx:%d", order_node.price_, order_type_desc[type_], idx);
    auto ret = InsertSortLinkList(price_nodes_[idx], order_node, false, order_id_less_func_);
    if(ret.first == false)
    {
        LOG_RAW_STDOUT("ignore order node with same id:%s", order_node.id_.c_str());
        return;
    }

    map_link_nodes_[order_node.id_] = ret.second;
}

void Depth::ResetTop()
{
    // search next non-empty price node since top_
    int i = 0, idx = top_;
    int elem_size = (bottom_ - top_ + current_size_) % current_size_;
    for(/*int i = 0, idx = top_*/; i <= elem_size; i++, idx = (idx + 1) % current_size_)
    {
        if(price_nodes_[idx] != NULL) { break; }
    }
    if(price_nodes_[idx] == NULL)
    {
        top_ = bottom_ = -1;
    }
    else
    {
        top_ = idx;
    }
}

void Depth::ResetBottom()
{
    // search previous non-empty price node since bottom_
    int i = 0, idx = bottom_;
    int elem_size = (bottom_ - top_ + current_size_) % current_size_;
    for(/*int i = 0, idx = top_*/; i <= elem_size; i++, idx = (idx - 1 + current_size_) % current_size_)
    {
        if(price_nodes_[idx] != NULL) { break; }
    }
    if(price_nodes_[idx] == NULL)
    {
        top_ = bottom_ = -1;
    }
    else
    {
        bottom_ = idx;
    }
}

void Depth::DeleteOrder(OrderNode &order_node)
{
    auto iter = map_link_nodes_.find(order_node.id_);
    if(iter == map_link_nodes_.end())
    {
        LOG_RAW_STDOUT("missing %s with order id:%s", order_type_desc[type_], order_node.id_.c_str());
        return;
    }

    LinkNode<OrderNode>* link_node = iter->second;
    int idx = GetIndexByPrice(link_node->value_.price_);
    LOG_RAW_STDOUT("clear %s order node at index:%d", order_type_desc[type_], idx);

    RemoveLinkNode(price_nodes_[idx], link_node);
    map_link_nodes_.erase(iter);

    /*
     * adjust top_ & bottom_ if necessary
     */
    if(idx == top_)
    {
        ResetTop();
    }
    else if(idx == bottom_)
    {
        ResetBottom();
    }
}

int Depth::GetIndexByPrice(int32_t price)
{
    int offset_top = (price - price_nodes_[top_]->value_.price_) / tick_price_;
    int idx = (top_ + offset_top * index_step_ + current_size_) % current_size_;
    return idx;
}

void Depth::ResetTickPrice(int32_t price)
{
    if(top_ == -1)  // it's safe to change tick price if current no OrderNode
    {
        tick_price_ = price;
        return;
    }

    if(tick_price_ <= price) { return; }
    int multiplies = tick_price_ / price;

    LinkNode<OrderNode>** tmp = CreateLinkNodeArray(multiplies * current_size_);

    int elem_size = (bottom_ - top_ + current_size_) % current_size_;
    for(int i = 0, idx = top_; i <= elem_size; i++, idx = (idx + 1) % current_size_)
    {
        if(price_nodes_[idx] == NULL) { continue; }
        int new_index  = multiplies * idx;
        tmp[new_index] = price_nodes_[idx];
    }

    delete []price_nodes_;
    price_nodes_   = tmp;
    top_          *= multiplies;
    bottom_       *= multiplies;
    current_size_ *= multiplies;
    LOG_RAW_STDOUT("reset %s with top:%d, bottom:%d, size:%d when reset tick price from %d to %d",
                order_type_desc[type_], top_, bottom_, current_size_, tick_price_, price);
    tick_price_    = price;
}

LinkNode<OrderNode>** Depth::CreateLinkNodeArray(int size)
{
    auto array = new LinkNode<OrderNode>*[size];
    for(int i = 0; i < size; i++)
    {
        array[i] = NULL;
    }

    return array;
}

OrderBook::OrderBook(int32_t tick_price, OrderIdLessFunc order_id_less_func, int initial_size, int step_size)
    : tick_price_(tick_price),
    ask_(1, initial_size, step_size, OrderType_Ask, tick_price, order_id_less_func),
    bid_(-1, initial_size, step_size, OrderType_Bid, tick_price, order_id_less_func)
{

}

void OrderBook::AddOrder(OrderNode order_node)
{
    // match opposite depth first before adding
    Depth* matched_depth = (order_node.type_ == OrderType_Ask ? &bid_ : &ask_);
    matched_depth->Match(order_node);

    if(order_node.size_ > 0)
    {
        Depth* same_depth = (order_node.type_ == OrderType_Ask ? &ask_ : &bid_);
        same_depth->Add(order_node);
    }
}

void OrderBook::DeleteOrder(OrderNode &order_node)
{
    Depth* matched_depth = (order_node.type_ == OrderType_Ask ? &ask_ : &bid_);
    matched_depth->DeleteOrder(order_node);
}

void OrderBook::Print()
{
    ask_.Print();
    bid_.Print();
}

void OrderBook::Clear()
{
    ask_.Clear();
    bid_.Clear();
}

void OrderBook::ResetTickPrice(int32_t price)
{
    //
    // tick price should be changed by multiplies
    //
    if((price > tick_price_ && price % tick_price_ != 0)
        || (price < tick_price_ && tick_price_ % price != 0)
    )
    {
        LOG_ERROR("invalid new tick price:%d against current:%d", price, tick_price_);
        return ;
    }

    if(tick_price_ <= price)
    {
        LOG_DEBUG("ignore new tick price:%d greater(equal) than current:%d", price, tick_price_);
        return;
    }

    ask_.ResetTickPrice(price);
    bid_.ResetTickPrice(price);
}
