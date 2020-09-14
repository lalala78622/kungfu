#include <pybind11/pybind11.h> //pybind11头文件
#include <pybind11/stl.h>//pybind11头文件
#include <iostream>
#include <kungfu/yijinjing/log/setup.h>//log头文件#include <kungfu/wingchun/msg.h>//平台数据结构头文
//件
#include <kungfu/wingchun/strategy/context.h>
#include <kungfu/wingchun/strategy/strategy.h>

namespace py = pybind11;

using namespace kungfu;
using namespace kungfu::wingchun;
using namespace kungfu::wingchun::strategy;
using namespace std;

//std::cout << "start" << std::endl;

//继承strategy接口，写自己的strategy类
class DemoStrategy : public Strategy
{
public:
	DemoStrategy(yijinjing::data::location_ptr home)
	{
		SPDLOG_INFO("cpp demo pre start2");
		std::cout << "cpp demo pre start3" << std::endl;
		yijinjing::log::copy_log_settings(home, "demo");
	};

	void pre_start(Context_ptr context) override
	{
		SPDLOG_INFO("cpp demo pre start");
	};

	void on_quote(Context_ptr context, const msg::data::Quote &quote) override
	{
		SPDLOG_INFO("cpp demo on quote");
	};
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