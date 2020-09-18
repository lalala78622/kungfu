#include <pybind11/pybind11.h> //pybind11头文件
#include <pybind11/stl.h>//pybind11头文件
#include <iostream>
#include <kungfu/yijinjing/log/setup.h>//log头文件
#include <kungfu/wingchun/msg.h>//平台数据结构头文件
#include <kungfu/wingchun/strategy/context.h>
#include <kungfu/wingchun/strategy/strategy.h>
#include <kungfu/wingchun/common.h>
#include <thread>

#include "..\..\..\..\utils\rapidjson\include\document.h"
#include "..\..\..\..\utils\rapidjson\include\writer.h"
#include "..\..\..\..\utils\rapidjson\include\stringbuffer.h"

namespace py = pybind11;

using namespace kungfu;
using namespace kungfu::wingchun;
using namespace kungfu::wingchun::strategy;
using namespace std;

std::string account = "15015255";

struct SENDSET
{
    int time;
    int64_t volume;
};
std::map<std::string, std::vector<SENDSET>> send_map;
std::mutex send_mutex;
bool start = false;

//继承strategy接口，写自己的strategy类
class DemoStrategy : public Strategy
{
private:
    std::string source = "xtp";
    //std::string account = "15015255";
    int64_t money_per_share = 1000000;

    //std::map<std::string, std::vector<SENDSET>> send_map;//instrument_id,SENDSET
    std::vector<std::string> tickers;
    //static bool start = false;
    //int64_t first_time = 0;
    int64_t period = 30;//s
    int Expect_times = 5;

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
        srand((unsigned)time(NULL));
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

        //context->add_timer(context->now() + 10*1000000000, std::bind(&DemoStrategy::random_insert, std::placeholders::_1, context));
        //std::thread send_thread(&DemoStrategy::random_insert, this);
        //send_thread.join();
        
	};

    void post_start(Context_ptr context) override
    {
        SPDLOG_INFO("[post_start]");
        //std::thread send_thread(&DemoStrategy::random_insert, this);
        //send_thread.join();
        SPDLOG_INFO("[post_start] end.");
    }

    void pre_stop(Context_ptr context) override
    {
        SPDLOG_INFO("[pre_stop]");
    
        SPDLOG_INFO("[pre_stop] end.");
    }

    void post_stop(Context_ptr context) override
    {
        SPDLOG_INFO("[post_stop]");

        SPDLOG_INFO("[post_stop] end.");        
    }

	void on_quote(Context_ptr context, const msg::data::Quote &quote) override
	{
		SPDLOG_INFO("[on_quote] instrument_id:{} price:{}",quote.instrument_id,quote.ask_price[0]);

        std::unique_lock<std::mutex> lck(send_mutex);
        auto it = send_map.find(quote.instrument_id);
        if(it == send_map.end() && !start){
            double dvolume_per_share = double(money_per_share) / quote.ask_price[0];
            //SPDLOG_INFO("dvolume_per_share:{}", dvolume_per_share);
            int64_t volume_per_share = floor(dvolume_per_share/100) * 100;
            SPDLOG_INFO("send_map instrument_id:{} volume_per_share:{}", quote.instrument_id, volume_per_share);
            //send_map.insert(make_pair(quote.instrument_id, volume_per_share));
            Produce_sendset(quote.instrument_id, volume_per_share);
        }
        lck.unlock();

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

    static int64_t getTimestamp()
    {
        long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        return timestamp;
    }

    static int64_t getSendTime(int64_t firsttime)
    {
        //int64_t SendTime = (getTimestamp() - firsttime) / 1000;
        int64_t SendTime = getTimestamp() - firsttime;
        return SendTime;
    }

    static string getExchang(string instrument_id)
    {
        string exchange;
        if(instrument_id.substr(0,1) == "6"){
            exchange = "SSE";
        }else{
            exchange = "SZE";
        }        
        return exchange;
    }

    int RandT(double _min, double _max)
    { 
        return round(rand() / (double)RAND_MAX *(_max - _min) + _min);
    }

    int RoundVolume(int volume)
    {
        int round_volume = round(double(volume)/100) * 100;
        //cout<<volume<<" "<<round_volume<<endl;
        return round_volume;
    }

    void Produce_sendset(std::string instrument_id, int64_t volume_per_share)
    {
        int total_time = period;
        int total_volume = volume_per_share;
        double expect_times = Expect_times;
        //srand((unsigned)time(NULL));
        std::vector<SENDSET> sendset_vec;
        int total_times = RandT(0.5*expect_times, 1.5*expect_times);
        cout<<"start total_times="<<total_times<<endl;
        double per_time = total_time / total_times;
        double per_volume = total_volume / total_times;
        int this_time = 0;
        for(int i=0; i<total_times; i++){

            this_time += RandT(0.5*per_time, 1.5*per_time);
            int this_volume = RandT(0.5*per_volume, 1.5*per_volume);
            this_volume = RoundVolume(this_volume);
            //cout<<"this_time"<<this_time<<endl;
            //cout<<"this_volume"<<this_volume<<endl;
            
            int last_time = total_time - this_time;
            total_volume -= this_volume;
            //cout<<"last_time"<<last_time<<endl;
            
            if((total_times-i-1) != 0){
                per_time = (double)last_time / (total_times-i-1);
                per_volume = (double)total_volume / (total_times-i-1);
                //cout<<"per_time"<<per_time<<endl;
                //cout<<"per_volume"<<per_volume<<endl;
            }else{//最后一次
                if(this_time > total_time){
                    this_time = total_time;
                }
                this_volume += total_volume;
            }
            //cout<<"this_time"<<this_time<<endl;
            //cout<<"this_volume"<<this_volume<<endl;       
            SENDSET sendset;
            sendset.time = this_time; sendset.volume = this_volume;
            sendset_vec.push_back(sendset);
        }

        /*for(auto it = sendset_vec.begin(); it != sendset_vec.end(); it++){
            SPDLOG_INFO("sendset:{} {}",it->time, it->volume);
        }*/
        send_map.insert(make_pair(instrument_id, sendset_vec));

    }

    //template <yijinjing::event_ptr event>
    static void random_insert(yijinjing::event_ptr event, Context_ptr context)
    {
        SPDLOG_INFO("[random_insert]");
        int64_t first_time = getTimestamp();
        SPDLOG_INFO("first_time={}",first_time);

        while(1){
            int64_t now = getSendTime(first_time);
            std::unique_lock<std::mutex> lck(send_mutex);
            for(auto map_it = send_map.begin(); map_it != send_map.end();){
                start = true;
                auto vec_it = map_it->second.begin();
                if(vec_it != map_it->second.end()){
                    if(now >= vec_it->time * 1000){
                        SPDLOG_INFO("will buy:{} {} {}",map_it->first, vec_it->time, vec_it->volume);
                        //uint64_t orderid = context->insert_order(map_it->first, getExchang(map_it->first), account, quote.ask_price[0], vec_it->volume, PriceType::Any, Side::Buy, Offset::Open);
                        //SPDLOG_INFO("orderid:{}",orderid);                        
                        vec_it = map_it->second.erase(vec_it);
                    }
                    map_it++;
                }else{
                    SPDLOG_INFO("send finish:{}", map_it->first);
                    map_it = send_map.erase(map_it);
                }                
            }
            lck.unlock();

            if(start && send_map.size() == 0){
                break;
            }

        }

        SPDLOG_INFO("[random_insert] end.");
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