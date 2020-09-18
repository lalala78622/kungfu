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
    std::string instrument_id;
    int64_t time;
    int64_t volume;
};
std::map<std::string, double> price_map;//instrument_id,price
std::vector<SENDSET> send_vec;
std::mutex send_mutex;
bool start = false;
int64_t first_time = 0;

bool comp(const SENDSET &a, const SENDSET &b) {
    return a.time < b.time;
}

std::string getExchang(std::string instrument_id)
{
    std::string exchange;
    if(instrument_id.substr(0,1) == "6"){
        exchange = "SSE";
    }else{
        exchange = "SZE";
    }        
    return exchange;
}

int64_t getSendTime(Context_ptr context)
{
    //int64_t SendTime = (getTimestamp() - firsttime) / 1000;
    int64_t SendTime = context->now() - first_time;
    return SendTime;
}

void InsertOrder(yijinjing::event_ptr event, Context_ptr context, std::string instrument_id, int64_t volume)
{
    auto it = price_map.find(instrument_id);
    if(it != price_map.end()){
        SPDLOG_INFO("will buy:{} {} {}",instrument_id, it->second, volume);
        uint64_t orderid = context->insert_order(it->first, getExchang(it->first), account, it->second, volume, PriceType::Limit, Side::Buy, Offset::Open);
    }

    auto it2 = send_vec.begin();
    if(it2 != send_vec.end()){

        std::string _instrument_id = it2->instrument_id;
        int64_t _volume = it2->volume;
        int64_t now = getSendTime(context);
        int64_t send_time = it2->time * 1e9 - now;
        SPDLOG_INFO("send_time = {}",send_time);
        send_vec.erase(it2);        

        context->add_timer(context->now() + send_time, std::bind(InsertOrder, std::placeholders::_1, context, _instrument_id, _volume));
    }else{
        SPDLOG_INFO("finish all.");
    }  
}

void Init_sendvec(yijinjing::event_ptr event, Context_ptr context){
    SPDLOG_INFO("Init_sendvec");
    sort(send_vec.begin(), send_vec.end(), comp);
    //start = true;
    //first_time = context->now();
    /*cout << "===========排序后================" << endl;
    for (auto it = send_vec.begin(); it != send_vec.end(); it++) {
        SPDLOG_INFO("time:{} instrument_id:{} volume:{}", it->time, it->instrument_id, it->volume);
    }*/

    auto it = send_vec.begin();
    if(it != send_vec.end()){
        SPDLOG_INFO("start InsertOrder");
        std::string instrument_id = it->instrument_id;
        int64_t volume = it->volume;       
        first_time = context->now();
        int64_t now = getSendTime(context);
        int64_t send_time = it->time * 1e9 - now;
        SPDLOG_INFO("send_time2 = {}",send_time);
        send_vec.erase(it); 
        context->add_timer(context->now() + send_time, std::bind(InsertOrder, std::placeholders::_1, context, instrument_id, volume));
    }
    SPDLOG_INFO("Init_sendvec end.");
}

//继承strategy接口，写自己的strategy类
class DemoStrategy : public Strategy
{
private:
    std::string source = "xtp";
    //std::string account = "15015255";
    int64_t money_per_share = 100000;

    //std::map<std::string, std::vector<SENDSET>> send_map;//instrument_id,SENDSET
    std::vector<std::string> tickers;
    //static bool start = false;
    //int64_t first_time = 0;
    int64_t period = 60;//s
    int Expect_times = 12;

public:
	DemoStrategy(yijinjing::data::location_ptr home)
	{
		SPDLOG_INFO("cpp demo pre start2");
		
		yijinjing::log::copy_log_settings(home, "demo");
	};

	void pre_start(Context_ptr context) override
	{
        tickers.push_back("000002"); /*tickers.push_back("000004");tickers.push_back("000005"); tickers.push_back("000006");
        tickers.push_back("000007"); tickers.push_back("000008");tickers.push_back("000009"); tickers.push_back("000010");
        tickers.push_back("000011"); tickers.push_back("000012");tickers.push_back("600004"); tickers.push_back("600006");
        tickers.push_back("600007"); tickers.push_back("600008");tickers.push_back("600009"); tickers.push_back("600010");
        tickers.push_back("600011"); tickers.push_back("600012");tickers.push_back("600015"); tickers.push_back("600016");*/

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

        //context->add_timer(context->now() + 15*1000000000, std::bind(Init_sendvec, std::placeholders::_1, context));
        //context->add_timer(context->now() + 10*1000000000, std::bind(&DemoStrategy::random_insert, std::placeholders::_1, context));
        //std::thread send_thread(&DemoStrategy::random_insert, this);
        //send_thread.join();
        
	};

    void post_start(Context_ptr context) override
    {
        SPDLOG_INFO("[post_start]");
        context->add_timer(context->now() + 15*1000000000, std::bind(Init_sendvec, std::placeholders::_1, context));
        //std::thread send_thread(&DemoStrategy::random_insert, this);
        //send_thread.join();
        SPDLOG_INFO("[post_start] end.");
    }

    /*void pre_stop(Context_ptr context) override
    {
        SPDLOG_INFO("[pre_stop]");
    
        SPDLOG_INFO("[pre_stop] end.");
    }

    void post_stop(Context_ptr context) override
    {
        SPDLOG_INFO("[post_stop]");

        SPDLOG_INFO("[post_stop] end.");        
    }*/

	void on_quote(Context_ptr context, const msg::data::Quote &quote) override
	{
		SPDLOG_INFO("[on_quote] instrument_id:{} price:{}",quote.instrument_id,quote.ask_price[0]);

        auto it = price_map.find(quote.instrument_id);
        if(it != price_map.end()){
            it->second = quote.ask_price[0];
        }
        else{
            double dvolume_per_share = double(money_per_share) / quote.ask_price[0];
            //SPDLOG_INFO("dvolume_per_share:{}", dvolume_per_share);
            int64_t volume_per_share = floor(dvolume_per_share/100) * 100;
            SPDLOG_INFO("price_map instrument_id:{} volume_per_share:{}", quote.instrument_id, volume_per_share);
            price_map.insert(make_pair(quote.instrument_id, quote.ask_price[0]));
            Produce_sendset(quote.instrument_id, volume_per_share);
        }

        /*if(start){
            int64_t now = getSendTime(context);
            auto it2 = send_vec.begin();
            if(it2 != send_vec.end()){
                if(now >= it2->time * 1e9){
                    std::string instrument_id = it2->instrument_id;
                    int64_t volume = it2->volume;
                    send_vec.erase(it2);
                    context->add_timer(context->now() + 1, std::bind(InsertOrder, std::placeholders::_1, context, instrument_id, volume));
                }
            }
        }*/

	};

    /*void on_order(Context_ptr context, const msg::data::Order &order) override
    {
        SPDLOG_INFO("[on_order] order_id:{} msg:{}",order.order_id, order.error_msg);

    }

    void on_trade(Context_ptr context, const msg::data::Trade &trade) override
    {
        SPDLOG_INFO("[on_trade] order_id:{}",trade.order_id);
    }*/

    static int64_t getTimestamp()
    {
        long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        return timestamp;
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
        
        //std::vector<SENDSET> sendset_vec;
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
            sendset.time = this_time; sendset.volume = this_volume; sendset.instrument_id = instrument_id;
            send_vec.push_back(sendset);
        }

        /*for(auto it = sendset_vec.begin(); it != sendset_vec.end(); it++){
            SPDLOG_INFO("sendset:{} {}",it->time, it->volume);
        }*/
        //send_map.insert(make_pair(instrument_id, sendset_vec));

    }

    //template <yijinjing::event_ptr event>
    /*static void random_insert(yijinjing::event_ptr event, Context_ptr context)
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
    }*/
};

void test(){
    int64_t start_time = time(0);
    while(1){
        int64_t now = time(0);
        if(now - start_time>=2){
            SPDLOG_INFO(">>>>>>>>>>>2");
            start_time = now;
        }
    }
}

int main(int argc, const char* argv[]){
    cout<<"main"<<endl;
    yijinjing::data::location_ptr home;
    DemoStrategy demo(home);
	Context_ptr context;
    demo.pre_start(context);
        std::thread send_thread(test);
        send_thread.join();
    SPDLOG_INFO("main end");
}

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