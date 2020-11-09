//
// Created by Keren Dong on 2019-09-06.
//

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>


#include <kungfu/yijinjing/log/setup.h>
#include <kungfu/wingchun/msg.h>
#include <kungfu/wingchun/strategy/context.h>
#include <kungfu/wingchun/strategy/strategy.h>
#include <vector>

namespace py = pybind11;

using namespace kungfu;
using namespace kungfu::wingchun;
using namespace kungfu::wingchun::strategy;

class DemoStrategy : public Strategy
{
public:
    DemoStrategy(yijinjing::data::location_ptr home)
    {
        yijinjing::log::copy_log_settings(home, "demo");
    };

    void pre_start(Context_ptr context) override 
    {
        SPDLOG_INFO("cpp demo pre start");
        context->add_account("xtp", "15015255", 20000000.0);
        std::vector<std::string> sse_tickers;
        sse_tickers.push_back("600036");
        context->subscribe("xtp", sse_tickers, "SSE");
    };

    void on_quote(Context_ptr context, const msg::data::Quote &quote) override 
    {
        SPDLOG_INFO("cpp demo on quote");
        uint64_t orderid = context->insert_order(quote.instrument_id, "SSE", "15015255", quote.ask_price[0], 100, PriceType::Limit, Side::Buy, Offset::Open);
    };
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

