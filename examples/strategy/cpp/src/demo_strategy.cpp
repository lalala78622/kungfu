#include <pybind11/pybind11.h> //pybind11头文件
#include <pybind11/stl.h>//pybind11头文件
#include <iostream>
#include <kungfu/yijinjing/log/setup.h>//log头文件
#include <kungfu/wingchun/msg.h>//平台数据结构头文件
#include <kungfu/wingchun/strategy/context.h>
#include <kungfu/wingchun/strategy/strategy.h>
#include <kungfu/wingchun/common.h>
#include <thread>

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
    int64_t money_per_share = 1000000;

    std::map<std::string, int64_t> send_map;
    std::vector<std::string> tickers;
    bool start = false;
    int64_t first_time = 0;
    int64_t period = 600;//s

public:
	DemoStrategy(yijinjing::data::location_ptr home)
	{
		SPDLOG_INFO("cpp demo pre start2");
		
		yijinjing::log::copy_log_settings(home, "demo");
	};

	void pre_start(Context_ptr context) override
	{
        tickers.push_back("600000"); tickers.push_back("000001");

		SPDLOG_INFO("[pre_start]");
        std::vector<std::string> sse_tickers; std::vector<std::string> sze_tickers;
        for(auto it = tickers.begin(); it != tickers.end(); it++){
            if((*it).substr(0,1) == "6"){
                sse_tickers.push_back(*it);
            }else{
                sze_tickers.push_back(*it);
            }
        }
        SPDLOG_INFO("size:{} {}",sse_tickers.size(), sze_tickers.size());

        context->add_account("xtp", "15015255", 20000000.0);
        context->subscribe("xtp", sse_tickers, "SSE");//SZE
        context->subscribe("xtp", sze_tickers, "SZE");
        SPDLOG_INFO("subscribe finish");

        std::thread send_thread(&DemoStrategy::random_insert, this);
        send_thread.join();
        //int64_t now = getTimestamp();
	};

	void on_quote(Context_ptr context, const msg::data::Quote &quote) override
	{
		SPDLOG_INFO("[on_quote] instrument_id:{} price:{}",quote.instrument_id,quote.ask_price[0]);

        auto it = send_map.find(quote.instrument_id);
        if(it == send_map.end()){
            double dvolume_per_share = double(money_per_share) / quote.ask_price[0];
            //SPDLOG_INFO("dvolume_per_share:{}", dvolume_per_share);
            int64_t volume_per_share = floor(dvolume_per_share/100) * 100;
            SPDLOG_INFO("send_map instrument_id:{} volume_per_share:{}", quote.instrument_id, volume_per_share);
            send_map.insert(make_pair(quote.instrument_id, volume_per_share));
        }

        /*SPDLOG_INFO("[on_quote]: {} {}",quote.turnover, quote.volume);
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
        } */       
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

    void random_insert()
    {
        SPDLOG_INFO("[random_insert]");

    }
};


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