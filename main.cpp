#include <string>
#include <iostream>
#include <map>
#include <list>
#include <unordered_map>
#include <vector>
#include <functional>
#include <sstream>
#include <memory>
#include <algorithm>
#include <numeric>
#include <stdexcept>



namespace matchingengine {

enum class OrderType { IOC, GFD };

enum class OrderSide { BUY, SELL };

using Price = int;
using Quantity = int;
using OrderId = std::string;


            // ===========
            // class Order
            // ===========

class Order {
public:
    OrderType m_orderType;
    OrderSide m_orderSide;
    Price     m_price;
    Quantity  m_quantity;
    OrderId   m_orderId;

    Order(OrderType orderType,
        OrderSide orderSide,
        Price price,
        Quantity quantity,
        const OrderId& orderId) :
        m_orderType(orderType),
        m_orderSide(orderSide),
        m_price(price),
        m_quantity(quantity),
        m_orderId(orderId) {}

    /// <summary>
    /// returns a string that prints out OrderType
    /// </summary>
    static std::string OrderTypeToString(OrderType orderType);

    /// <summary>
    /// returns a string that prints out OrderSide
    /// </summary>
    static std::string OrderSideToString(OrderSide orderSide);
};




            // =====================
            // class MatchingEngineI
            // =====================


/// <summary>
/// Interface class of class MatchingEngine,
/// for dependency injection and mocking in unit tests
/// </summary>
class MatchingEngineI
{
public:

    virtual ~MatchingEngineI() {}

    virtual void print() const = 0;

    virtual void processOrder(OrderType orderType,
        OrderSide orderSide,
        Price price,
        Quantity quantity,
        const OrderId& orderId) = 0;

    virtual void cancelOrder(const OrderId& orderId) = 0;

    virtual void modifyOrder(const OrderId& orderId,
        OrderSide newOrderSide,
        Price newPrice,
        Quantity newQuantity) = 0;
};


            // ====================
            // class MatchingEngine
            // ==================== 


class MatchingEngine : public MatchingEngineI {

public:

    /// <summary>
    /// execute message PRINT
    /// </summary>
    void print() const;

    /// <summary>
    /// process order BUY or SELL; trade order against queued orders,
    /// update queued orders (if queued order is completely traded delete it),
    /// and add the remaining untraded part to engine
    /// </summary>
    void processOrder(OrderType orderType,
        OrderSide orderSide,
        Price price,
        Quantity quantity,
        const OrderId& orderId);

    /// <summary>
    /// cancel order, remove order from engine; if orderId doesn't exist, no op
    /// </summary>
    void cancelOrder(const OrderId& orderId);

    /// <summary>
    /// modify order; if orderId doesn't exist, no op; if order type is IOC, no op
    /// </summary>
    void modifyOrder(const OrderId& orderId,
        OrderSide newOrderSide,
        Price newPrice,
        Quantity newQuantity);


private:
    using OrdersList = std::list<std::shared_ptr<Order> >;
    using PriceMap = std::map<Price, OrdersList, std::greater<Price> >;

    std::unordered_map<OrderId, std::shared_ptr<Order> > m_orderIdToOrder;

    PriceMap m_priceMapBuy;
    PriceMap m_priceMapSell;

    /// <summary>
    /// Insert order into a price map, no validation
    /// </summary>
    void insertIntoPriceMap(PriceMap& priceMap, std::shared_ptr<Order> order);

    /// <summary>
    /// print price and quanty summary of a price map
    /// </summary>
    void printPriceQuantitySummary(const PriceMap& priceMap, std::ostream& os) const;

    /// <summary>
    /// trade new order (sell) against all queued buy orders in m_priceMapBuy, this function
    /// updated new order and m_priceMapBuy, but doesn't insert new order into matching engine
    /// </summary>
    void tradeSellOrder(std::shared_ptr<Order> newOrder);

    /// <summary>
    /// trade new order (buy) against all queued sell orders in m_priceMapSell, this function
    /// updated new order and m_priceMapSell, but doesn't insert new order into matching engine
    /// </summary>
    void tradeBuyOrder(std::shared_ptr<Order> newOrder);

    /// <summary>
    /// erase an order from a price map
    /// </summary>
    void eraseOrderFromPriceMap(PriceMap& priceMap, Price price, const OrderId& orderId);

    /// <summary>
    /// function returns true if price, quantity, and orderId are valid, o.w. false
    /// </summary>
    bool isValidOrder(Price price, Quantity quantity, const OrderId& orderId) const;

    /// <summary>
    /// insert order, this function doesn't validate order
    /// </summary>
    void insertOrder(std::shared_ptr<Order> newOrder);

    /// <summary>
    /// function returns true if buy and sell orders are price cross, meaning
    /// buy price is equal or higher than sell price
    /// </summary>
    bool isPriceCross(std::shared_ptr<Order> buyOrder,
        std::shared_ptr<Order> sellOrder) const;

    /// <summary>
    /// print trade event to ostream
    /// </summary>
    void printTradeEvent(std::shared_ptr<Order> olderOrder,
        std::shared_ptr<Order> newOrder,
        Quantity tradeQuantity,
        std::ostream& os) const;
};


            // ======================
            // class MessageProcessor
            // ======================


class MessageProcessor
{
private:
    std::shared_ptr<MatchingEngineI>  m_matchingEngineI;

public:
    /// <summary>
    /// ctor, inject dependency
    /// </summary>
    MessageProcessor(std::shared_ptr<MatchingEngineI> matchingEngineI) :
        m_matchingEngineI(matchingEngineI) {}

    /// <summary>
    /// parse message, return tokens
    /// </summary>
    static std::vector<std::string> tokenizeMessage(const std::string& msg);

    /// <summary>
    /// convert a token string to order side;
    /// function returns true if succeeds, o.w. false
    /// </summary>
    static bool getOrderSideFromToken(const std::string& token,
        OrderSide& orderSide);

    /// <summary>
    /// convert a token string to order type;
    /// function returns true if succeeds, o.w. false
    /// </summary>
    static bool getOrderTypeFromToken(const std::string& token,
        OrderType& orderType);

    /// <summary>
    /// convert a token string to price;
    /// function returns true if succeeds, o.w. false
    /// </summary>
    static bool getPriceFromToken(const std::string& token,
        Price& price);

    /// <summary>
    /// convert a token string to quantity;
    /// function returns true if succeeds, o.w. false
    /// </summary>
    static bool getQuantityFromToken(const std::string& token,
        Quantity& quantity);

    /// <summary>
    /// convert a token string to order ID;
    /// this function will steal token's resource;
    /// function returns true if succeeds, o.w. false
    /// </summary>
    static bool getOrderIdFromToken(std::string& token,
        OrderId& orderId);

    /// <summary>
    /// listen to message, and invoke appropriate matching engine actions
    /// </summary>
    void listenToMessage(std::istream& is) const;

};


// cpp files ////////////////////////////////////////////////


            // -----------
            // class Order
            // -----------


std::string Order::OrderTypeToString(OrderType orderType)
{
    switch (orderType) {
    case OrderType::GFD:
        return "GFD";
    case OrderType::IOC:
        return "IOC";
    default:
        return "UNEXPECTED_ORDER_TYPE";
    }
}


std::string Order::OrderSideToString(OrderSide orderSide)
{
    switch (orderSide) {
    case OrderSide::BUY:
        return "BUY";
    case OrderSide::SELL:
        return "SELL";
    default:
        return "UNEXPECTED_ORDER_SIDE";
    }
}


            // --------------------
            // class MatchingEngine
            // --------------------


void MatchingEngine::insertIntoPriceMap(PriceMap& priceMap, std::shared_ptr<Order> order)
{
    if (priceMap.find(order->m_price) != priceMap.end()) {
        // this price already exists
        priceMap[order->m_price].push_back(order);
    }
    else {
        // new price
        priceMap.emplace(order->m_price, OrdersList{ order });
    }
}


void MatchingEngine::printPriceQuantitySummary(const PriceMap& priceMap, std::ostream& os) const
{
    for (const auto& priceEntry : priceMap) {
        const auto& orders = priceEntry.second;
        Quantity quantitySum = std::accumulate(orders.cbegin(),
            orders.cend(),
            0,
            [](Quantity sum, std::shared_ptr<Order> order) { return sum + order->m_quantity; });
        if (quantitySum > 0) {
            os << priceEntry.first << " " << quantitySum << "\n";
        }
    }
}


void MatchingEngine::print() const
{
    std::cout << "SELL:\n";
    printPriceQuantitySummary(m_priceMapSell, std::cout);
    std::cout << "BUY:\n";
    printPriceQuantitySummary(m_priceMapBuy, std::cout);
}


bool MatchingEngine::isValidOrder(Price price, Quantity quantity, const OrderId& orderId) const
{
    if (price <= 0 || quantity <= 0) {
        return false;
    }

    if (m_orderIdToOrder.find(orderId) != m_orderIdToOrder.cend()) {
        return false;
    }

    return true;
}


void MatchingEngine::insertOrder(std::shared_ptr<Order> newOrder)
{
    m_orderIdToOrder[newOrder->m_orderId] = newOrder;
    switch (newOrder->m_orderSide) {
    case OrderSide::BUY: {
        insertIntoPriceMap(m_priceMapBuy, newOrder);
    } break;
    case OrderSide::SELL: {
        insertIntoPriceMap(m_priceMapSell, newOrder);
    } break;
    }
}


bool MatchingEngine::isPriceCross(std::shared_ptr<Order> buyOrder,
    std::shared_ptr<Order> sellOrder) const
{
    if (buyOrder->m_orderSide != OrderSide::BUY || sellOrder->m_orderSide != OrderSide::SELL) {
        throw std::runtime_error("Wrong order sides");
    }
    return buyOrder->m_price >= sellOrder->m_price;
}


void MatchingEngine::printTradeEvent(std::shared_ptr<Order> olderOrder,
    std::shared_ptr<Order> newOrder,
    Quantity tradeQuantity,
    std::ostream& os) const
{
    os << "TRADE " << olderOrder->m_orderId
        << " " << olderOrder->m_price
        << " " << tradeQuantity
        << " " << newOrder->m_orderId
        << " " << newOrder->m_price
        << " " << tradeQuantity
        << "\n";
}


void MatchingEngine::tradeSellOrder(std::shared_ptr<Order> newOrder)
{
    if (newOrder->m_orderSide != OrderSide::SELL) {
        throw std::runtime_error("Wrong order side");
    }

    // for Sell order, just need to compare with queued buy orders from high price to low price
    for (auto buyOrdersPerPrice = m_priceMapBuy.begin(); buyOrdersPerPrice != m_priceMapBuy.end();) {
        for (auto buyOrder = buyOrdersPerPrice->second.begin(); buyOrder != buyOrdersPerPrice->second.end();) {

            if (newOrder->m_quantity <= 0) {
                // trade is done
                break;
            }

            if (isPriceCross(*buyOrder, newOrder)) {
                // matched
                Quantity tradeQuantity = std::min(newOrder->m_quantity, (*buyOrder)->m_quantity);
                printTradeEvent(*buyOrder, newOrder, tradeQuantity, std::cout);
                // update remaining quantities 
                (*buyOrder)->m_quantity -= tradeQuantity;
                newOrder->m_quantity -= tradeQuantity;

                if ((*buyOrder)->m_quantity <= 0) {
                    // remove buyOrder
                    m_orderIdToOrder.erase((*buyOrder)->m_orderId);
                    buyOrder = buyOrdersPerPrice->second.erase(buyOrder);
                }
                else {
                    ++buyOrder;
                }
            }
            else {
                // there no more buy order price equal or higher than newOrder (sell) price
                break;
            }
        }

        if (buyOrdersPerPrice->second.empty()) {
            // all queued buy orders at this price are traded
            m_priceMapBuy.erase(buyOrdersPerPrice++);
        }
        else {
            ++buyOrdersPerPrice;
        }
    }
}


void MatchingEngine::tradeBuyOrder(std::shared_ptr<Order> newOrder)
{
    if (newOrder->m_orderSide != OrderSide::BUY) {
        throw std::runtime_error("Wrong order side");
    }

    // for Buy orders, need to compare with queued sell orders from the first price that's equal or smaller than sell price
    for (auto sellOrdersPerPrice = m_priceMapSell.lower_bound(newOrder->m_price);
        sellOrdersPerPrice != m_priceMapSell.end();) {
        for (auto sellOrder = sellOrdersPerPrice->second.begin(); sellOrder != sellOrdersPerPrice->second.end();) {

            if (newOrder->m_quantity <= 0) {
                // trade is done
                break;
            }

            if (isPriceCross(newOrder, *sellOrder)) {
                // matched
                Quantity tradeQuantity = std::min(newOrder->m_quantity, (*sellOrder)->m_quantity);
                printTradeEvent(*sellOrder, newOrder, tradeQuantity, std::cout);
                // update remaining quantities
                (*sellOrder)->m_quantity -= tradeQuantity;
                newOrder->m_quantity -= tradeQuantity;

                if ((*sellOrder)->m_quantity <= 0) {
                    // remove sell order
                    m_orderIdToOrder.erase((*sellOrder)->m_orderId);
                    sellOrder = sellOrdersPerPrice->second.erase(sellOrder);
                }
                else {
                    ++sellOrder;
                }
            }
        }

        if (sellOrdersPerPrice->second.empty()) {
            // all queued sell orders at this price are traded
            m_priceMapSell.erase(sellOrdersPerPrice++);
        }
        else {
            ++sellOrdersPerPrice;
        }
    }
}


void MatchingEngine::processOrder(OrderType orderType,
    OrderSide orderSide,
    Price price,
    Quantity quantity,
    const OrderId& orderId)
{   
    // validate order
    if (!isValidOrder(price, quantity, orderId)) {
        return;
    }

    // create a new order
    std::shared_ptr<Order> newOrder = std::make_shared<Order>(orderType,
        orderSide,
        price,
        quantity,
        orderId);

    switch (orderSide) {
    case OrderSide::SELL:
        tradeSellOrder(newOrder);
        break;
    case OrderSide::BUY:
        tradeBuyOrder(newOrder);
        break;
    default:
        throw std::runtime_error("Unsupported order side!");
    }

    if (newOrder->m_quantity > 0 && newOrder->m_orderType == OrderType::GFD) {
        // new order has non-traded quantity and is of type GFD, need to queue it
        insertOrder(newOrder);
    }
}


void MatchingEngine::eraseOrderFromPriceMap(PriceMap& priceMap,
    Price price,
    const OrderId& orderId)
{
    auto ordersPerPrice = priceMap.find(price);
    if (ordersPerPrice != priceMap.end()) {
        auto orderItr = std::find_if(ordersPerPrice->second.begin(),
            ordersPerPrice->second.end(),
            [orderId](std::shared_ptr<Order> order) {
                return order->m_orderId == orderId;
            });
        if (orderItr != ordersPerPrice->second.end()) {
            ordersPerPrice->second.erase(orderItr);
        }

        if (ordersPerPrice->second.empty()) {
            priceMap.erase(price);
        }
    }
}


void MatchingEngine::cancelOrder(const OrderId& orderId)
{
    if (m_orderIdToOrder.find(orderId) == m_orderIdToOrder.cend()) {
        // order Id doesn't exist, no op
        return;
    }

    Price price = m_orderIdToOrder[orderId]->m_price;
    OrderSide orderSide = m_orderIdToOrder[orderId]->m_orderSide;
    switch (orderSide) {
    case OrderSide::BUY:
        eraseOrderFromPriceMap(m_priceMapBuy, price, orderId);
        break;
    case OrderSide::SELL:
        eraseOrderFromPriceMap(m_priceMapSell, price, orderId);
        break;
    default:
        throw std::runtime_error("Unsupported order side!");
    }

    m_orderIdToOrder.erase(orderId);
}


void MatchingEngine::modifyOrder(const OrderId& orderId,
    OrderSide newOrderSide,
    Price newPrice,
    Quantity newQuantity)
{
    if (m_orderIdToOrder.find(orderId) == m_orderIdToOrder.cend()) {
        // order doesn't exist, no op
        return;
    }

    auto& order = m_orderIdToOrder[orderId];

    if (order->m_orderType == OrderType::IOC) {
        // cannot modify IOC order, no op
        return;
    }

    OrderSide oldOrderSide = order->m_orderSide;
    Price oldPrice = order->m_price;
    
    order->m_price = newPrice;
    order->m_quantity = newQuantity;
    order->m_orderSide = newOrderSide;

    switch(oldOrderSide) {
    case OrderSide::BUY:
        eraseOrderFromPriceMap(m_priceMapBuy, oldPrice, orderId);
        break;
    case OrderSide::SELL:
        eraseOrderFromPriceMap(m_priceMapSell, oldPrice, orderId);
        break;
    default:
        throw std::runtime_error("Unsupported order side!");
    }

    switch (newOrderSide) {
    case OrderSide::BUY:
        insertIntoPriceMap(m_priceMapBuy, order);
        break;
    case OrderSide::SELL:
        insertIntoPriceMap(m_priceMapSell, order);
        break;
    default:
        throw std::runtime_error("Unsupported order side!");
    }
    
}


            // ----------------------
            // class MessageProcessor
            // ----------------------


std::vector<std::string> MessageProcessor::tokenizeMessage(const std::string& msg)
{
    std::stringstream ss(msg);
    std::vector<std::string> tokens;
    
    std::string token;

    while (getline(ss, token, ' ')) {
        tokens.push_back(std::move(token));
    }

    return tokens;
}


bool MessageProcessor::getOrderSideFromToken(const std::string& token,
    OrderSide& orderSide)
{
    if (token == "BUY") {
        orderSide = OrderSide::BUY;
        return true;
    }

    if (token == "SELL") {
        orderSide = OrderSide::SELL;
        return true;
    }

    return false;
}


bool MessageProcessor::getOrderTypeFromToken(const std::string& token,
    OrderType& orderType)
{
    if (token == "IOC") {
        orderType = OrderType::IOC;
        return true;
    }
    if (token == "GFD") {
        orderType = OrderType::GFD;
        return true;
    }
    return false;
}


bool MessageProcessor::getPriceFromToken(const std::string& token,
    Price& price)
{
    try {
        price = std::stoi(token);
    }
    catch (...) {
        return false;
    }
    return true;
}


bool MessageProcessor::getQuantityFromToken(const std::string& token,
    Quantity& quantity)
{
    try {
        quantity = std::stoi(token);
    }
    catch (...) {
        return false;
    }
    return true;
}


bool MessageProcessor::getOrderIdFromToken(std::string& token,
    OrderId& orderId)
{
    if (token.empty()) {
        return false;
    }
    orderId.swap(token);
    return true;
}


void MessageProcessor::listenToMessage(std::istream& is) const
{
    std::string line;
    while (getline(is, line)) {
        
        std::vector<std::string> tokens = tokenizeMessage(line);

        if (!tokens.empty()) {
            
            // BUY or SELL
            if (tokens[0] == "BUY" || tokens[0] == "SELL") {
                // expect 5 tokens
                if (tokens.size() != 5) {
                    continue;
                }

                OrderSide orderSide;
                bool success = getOrderSideFromToken(tokens[0], orderSide);
                if (!success) {
                    continue;
                }
                OrderType orderType;
                success = getOrderTypeFromToken(tokens[1], orderType);
                if (!success) {
                    continue;
                }
                Price price;
                success = getPriceFromToken(tokens[2], price);
                if (!success) {
                    continue;
                }
                Quantity quantity;
                success = getQuantityFromToken(tokens[3], quantity);
                if (!success) {
                    continue;
                }
                OrderId orderId;
                success = getOrderIdFromToken(tokens[4], orderId);
                if (!success) {
                    continue;
                }

                m_matchingEngineI->processOrder(orderType, orderSide, price, quantity, orderId);
            }

            // CANCEL
            if (tokens[0] == "CANCEL") {
                // expect 2 tokens
                if (tokens.size() != 2) {
                    continue;
                }

                OrderId orderId;
                bool success = getOrderIdFromToken(tokens[1], orderId);
                if (!success) {
                    continue;
                }

                m_matchingEngineI->cancelOrder(orderId);
            }

            // MODIFY
            if (tokens[0] == "MODIFY") {
                // expect 5 tokens
                if (tokens.size() != 5) {
                    continue;
                }

                OrderId orderId;
                bool success = getOrderIdFromToken(tokens[1], orderId);
                if (!success) {
                    continue;
                }
                OrderSide newOrderSide;
                success = getOrderSideFromToken(tokens[2], newOrderSide);
                if (!success) {
                    continue;
                }
                Price newPrice;
                success = getPriceFromToken(tokens[3], newPrice);
                if (!success) {
                    continue;
                }
                Quantity newQuantity;
                success = getQuantityFromToken(tokens[4], newQuantity);
                if (!success) {
                    continue;
                }
                
                m_matchingEngineI->modifyOrder(orderId, newOrderSide, newPrice, newQuantity);
            }

            // PRINT
            if (tokens[0] == "PRINT") {
                m_matchingEngineI->print();
            }
        }
    }
}

} // namespace matchingengine


///////////////////////////////////////////////////////


int main()
{
    using namespace matchingengine;
    
    MessageProcessor messageProcessor(std::make_shared<MatchingEngine>());
    messageProcessor.listenToMessage(std::cin);

    return 0;
}