#include <pybind11/pybind11.h> //pybind11头文件
#include <pybind11/stl.h>//pybind11头文件
#include <iostream>
#include <kungfu/yijinjing/log/setup.h>//log头文件
#include <kungfu/wingchun/msg.h>//平台数据结构头文件
#include <kungfu/wingchun/strategy/context.h>
#include <kungfu/wingchun/strategy/strategy.h>
#include <kungfu/wingchun/common.h>

namespace py = pybind11;

using namespace kungfu;
using namespace kungfu::wingchun;
using namespace kungfu::wingchun::strategy;
using namespace std;

struct PriceMsg
{
    double price;
    int64_t timestamp;
};

//继承strategy接口，写自己的strategy类
class DemoStrategy : public Strategy
{
private:
    std::string source = "xtp";
    std::string account = "15015255";
    std::string exchange = "SSE";

    std::vector<PriceMsg> price_vec;
    bool start = false;
    int64_t first_time = 0;
    int64_t period = 30000;//ms

public:
	DemoStrategy(yijinjing::data::location_ptr home)
	{
		SPDLOG_INFO("cpp demo pre start2");
		
		yijinjing::log::copy_log_settings(home, "demo");
	};

	void pre_start(Context_ptr context) override
	{
		SPDLOG_INFO("[pre_start]");
        std::vector<std::string> tickers;
        tickers.push_back("600036");
        context->add_account("xtp", "15015255", 1000000.0);
        context->subscribe("xtp", tickers, "SSE");
        SPDLOG_INFO("subscribe finish");

        /*msg::data::Quote quote;
        quote.last_price = 100;
        std::string str = to_string(quote);
        cout<<str<<endl;*/

        /*nlohmann::json j;
        to_json(j, quote);
        cout<<j.dump(-1, ' ', false, nlohmann::json::error_handler_t::ignore)<<endl;*/

	};

	void on_quote(Context_ptr context, const msg::data::Quote &quote) override
	{
		/*SPDLOG_INFO("[on_quote] last_price:{} data_time{}",quote.last_price,quote.data_time);
        
        int64_t now = getTimestamp();
        PriceMsg pricemsg;
        pricemsg.price = quote.last_price;
        pricemsg.timestamp = now;
        price_vec.push_back(pricemsg);

        double total = 0; double average = 0;
        for(auto it = price_vec.begin(); it != price_vec.end();){
            if(now - it->timestamp > period){
                it = price_vec.erase(it);
                continue;
            }else{
                total += it->price;
                it++;
            }
        }
        if(price_vec.size() > 0){
            average = total / price_vec.size();
        }

        if(!start){
            start = true;
            first_time = now;
        }

        if(now - first_time > period && average != 0){
            SPDLOG_INFO("judge: {} {} {}",quote.bid_price[0], average, quote.ask_price[0]);
            if(quote.ask_price[0] > average){
                SPDLOG_INFO("will buy");
                //uint64_t orderid = context->insert_order(quote.instrument_id, exchange, account, quote.ask_price[0], 100, Limit, Buy, Open);
                //SPDLOG_INFO("orderid:{}",orderid);
            }else if(quote.bid_price[0] < average){
                SPDLOG_INFO("will sell");
                //uint64_t orderid = context->insert_order(quote.instrument_id, exchange, account, quote.bid_price[0], 100, Limit, Sell, Open);
                //SPDLOG_INFO("orderid:{}",orderid);                
            }
        }*/

        //turnover/volume
        SPDLOG_INFO("[on_quote]: {} {}",quote.turnover, quote.volume);
        double average = quote.turnover/quote.volume;
        SPDLOG_INFO("judge: {} {} {}",quote.bid_price[0], average, quote.ask_price[0]);
        if(quote.ask_price[0] > average){
            SPDLOG_INFO("will buy");
            uint64_t orderid = context->insert_order(quote.instrument_id, exchange, account, quote.ask_price[0], 100, PriceType::Limit, Side::Buy, Offset::Open);
            SPDLOG_INFO("orderid:{}",orderid);
        }else if(quote.bid_price[0] < average){
            SPDLOG_INFO("will sell");
            uint64_t orderid = context->insert_order(quote.instrument_id, exchange, account, quote.bid_price[0], 100, PriceType::Limit, Side::Sell, Offset::Open);
            SPDLOG_INFO("orderid:{}",orderid);                
        }        
	};

    void on_order(Context_ptr context, const msg::data::Order &order) override
    {
        SPDLOG_INFO("[on_order] order_id:{} msg:{}",order.order_id, order.error_msg);

    }

    void on_trade(Context_ptr context, const msg::data::Trade &trade) override
    {
        SPDLOG_INFO("[on_trade] order_id:{}",trade.order_id);
    }

    int64_t getTimestamp()
    {
        long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        return timestamp;
    }
};


/*
下面的代码作用是：通过pybind11 把c++
代码封装成python识别的数据格式
最后需要生成pyd文件（windows下）
或者是.so文件(linux下)，以便python导入。
生成上述文件，若用cmake生成工程，则需要
在CMakeLists.txt添加如下语句：
PYBIND11_ADD_MODULE(${PROJECT_NAME} SHARED ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE yijinjing wingchun)
*/
PYBIND11_MODULE(cpp_demo, m)
{
	py::class_<DemoStrategy, Strategy, std::shared_ptr<DemoStrategy>>(m, "Strategy")
		.def(py::init<yijinjing::data::location_ptr>())
		.def("pre_start", &DemoStrategy::pre_start)
		.def("post_start", &DemoStrategy::post_start)
		.def("pre_stop", &DemoStrategy::pre_stop)
		.def("post_stop", &DemoStrategy::post_stop)
		.def("on_trading_day", &DemoStrategy::on_trading_day)
		.def("on_quote", &DemoStrategy::on_quote)
		.def("on_entrust", &DemoStrategy::on_entrust)
		.def("on_transaction", &DemoStrategy::on_transaction)
		.def("on_order", &DemoStrategy::on_order)
		.def("on_trade", &DemoStrategy::on_trade);
}