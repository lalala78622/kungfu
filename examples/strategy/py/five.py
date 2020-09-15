import time
import kungfu.yijinjing.time as kft
from kungfu.wingchun.constants import *
SOURCE = "ctp"
account = "172634"
tickers = ["au2012"]
exchange = Exchange.SHFE
price_list = []
global first_time
first_time = 0
global start
start = False
period = 5 * 1e9

def pre_start(context):
    context.add_account(SOURCE, account, 1000000.0)
    context.subscribe(SOURCE, tickers, exchange)
    context.log.info("subscribe finished")


def on_quote(context, quote):
    context.log.info('[on_quote] {}, {}, {} ,{}, {}, {}'.format('instrument_id:',quote.instrument_id,'last_price:', quote.last_price, 'data_time:',quote.data_time))
    now = time.time()
    now_ns = int(now * 1e9)
    quote_ = [quote.last_price, now_ns]
    price_list.append(quote_)

    total = 0
    average = 0
    #now = time.time()
    #now_ns = int(now * 1e9)
    for price in price_list[:]:
        if now_ns - price[1] > period:
            price_list.remove(price)
        else:
            total += price[0]
    if len(price_list) > 0:
        average = total/(len(price_list))

    global start
    global first_time
    if not start:
        start = True
        first_time = now_ns

    if now_ns - first_time > period and average != 0:
        context.log.info('judge:{}, {}, {}'.format(quote.bid_price[0],average,quote.ask_price[0]))
        if quote.ask_price[0] > average:
            context.log.warning('will buy')
            order_id = context.insert_order(quote.instrument_id, exchange, account, quote.ask_price[0], 1, PriceType.Limit, Side.Buy, Offset.Open)
            context.log.warning('order_id={}'.format(order_id))
        elif quote.bid_price[0] < average:
            context.log.warning('will sell')
            order_id = context.insert_order(quote.instrument_id, exchange, account, quote.bid_price[0], 1, PriceType.Limit, Side.Sell, Offset.Open)
            context.log.warning('order_id={}'.format(order_id))

def on_transaction(context, transaction):
    context.log.info("[on_transaction] {} {}".format(transaction.instrument_id, transaction.exchange_id))
    pass

def on_entrust(context, entrust):
    context.log.info("[on_entrust] {} {}".format(entrust.instrument_id, entrust.exchange_id))
    pass

def on_order(context, order):
    context.log.info("[on_quote] {}".format(order))

def on_trade(context, trade):
    context.log.info("[on_quote] {}".format(trade))
    