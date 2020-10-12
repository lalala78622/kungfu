<template>
<tr-dashboard title="策略完成进度">
    <div slot="dashboard-header">
        <tr-dashboard-header-item>
            <tr-search-input v-model.trim="searchKeyword"></tr-search-input>
        </tr-dashboard-header-item>
        <tr-dashboard-header-item>
            <i class="el-icon-refresh mouse-over" title="刷新" @click="handleRefresh"></i>
        </tr-dashboard-header-item>
        <tr-dashboard-header-item>
            <i class="el-icon-download mouse-over" title="导出" @click="dateRangeDialogVisiblity = true"></i>
        </tr-dashboard-header-item>
    </div>
    <tr-table
        v-if="rendererTable"
        :data="tableData"
        :schema="schema"
        :renderCellClass="renderCellClass"
    ></tr-table>
    <date-range-dialog 
    @confirm="handleConfirmDateRange"
    :visible.sync="dateRangeDialogVisiblity"    
    ></date-range-dialog>
</tr-dashboard>

</template>

<script>
import { mapGetters, mapState } from 'vuex'
import moment from 'moment'
import { debounce, throttleInsert, dealTrades } from "__gUtils/busiUtils"
import { writeCSV } from '__gUtils/fileUtils';
import DateRangeDialog from './DateRangeDialog';
import { offsetName, orderStatus, sideName, posDirection } from "__gConfig/tradingConfig";
const fs = require('fs');
const join = require('path').join;

export default {
    name: 'trades-record',
    props: {
        currentId: {
            type: String,
            default:''
        },
        moduleType: {
            type: String,
            default:''
        },
        getDataMethod: {
            type: Function,
            default: () => {}
        },
        //接收推送的数据
        nanomsgBackData: {
            type: Object,
            default: {}
        }
    },

    data() {
        this.throttleInsertTrade = throttleInsert(500, 'unshift')
        return {
            rendererTable: false,
            searchKeyword: '',
            dic: {},
            filter: {
                id: '',
                dateRange: null
            },
            getDataLock: false,
            tableData: Object.freeze([]),
            dateRangeDialogVisiblity: false
        }
    },

    components: {
        DateRangeDialog
    },

    computed:{
        ...mapState({
            tradingDay: state => state.BASE.tradingDay, //日期信息，包含交易日
        }),

        schema(){
            //this.moduleType = 'account'
            return [{
                type: 'text',
                label: '代码',
                prop: 'instrumentId'
            },{
                type: 'text',
                label: '买卖',
                prop: 'side'
            },{
                type: 'text',
                label: '开平',
                prop: 'offset'
            },{
                type: 'number',
                label: '成交量',
                prop: 'volume',
            },{
                type: 'number',
                label: '总量',
                prop: 'volume_total',
            },{
                type: 'text',
                label: '进度',
                prop: 'rate',
            },{
                type: 'text',
                label: '累计金额',
                prop: 'trade_money',
            },{
                type: 'text',
                label: '策略',
                prop: 'clientId'
            }]
        }
    },

    watch: {
        searchKeyword: debounce(function(val) {
            const t = this;
            t.filter.id = val;
        }),

        filter:{
            deep: true,
            handler() {
                const t = this;
                if(t.currentId) t.init();
            }
        },
        
        currentId(val) {
            const t = this;
            t.resetData();
            if(val) t.init();
        },

        //接收推送返回的数据
        nanomsgBackData(val) {
            const t = this;
            if(!val || t.getDataLock) return
            t.dealNanomsg(val)
        },

        tradingDay() {
            const t = this;
            t.resetData();
            if(t.currentId) t.init();
        }

    },

    mounted(){
        const t = this;
        t.rendererTable = true;
        t.resetData();
        if(t.currentId) t.init();
    },

    methods:{
        handleRefresh(){
            const t = this;
            t.resetData();
            if(t.currentId) t.init();
        },

        //选择日期以及保存
        handleConfirmDateRange(dateRange){
            const t = this;
            t.getDataMethod(t.currentId, {
                id: t.filter.id,
                dateRange
            }, t.tradingDay).then(res => {
                if(!res) return;
                t.$saveFile({
                    title: '成交记录',
                }).then(filename => {
                    if(!filename) return;
                    writeCSV(filename, res)
                })
            })
        },

        caculateRate(trade_val, total_str)
        {
            let caculate = Math.floor(trade_val*100 / Number(total_str))
            let rate = caculate.toString() + "%"
            return rate
        },

        getFilesNames(path){
            const t = this
            let files = fs.readdirSync(path);
            files.forEach(function (item, index) {
                let fPath = join(item);
                //window.alert("fPath:"+fPath)
                let filename = path + "/" + fPath
                fs.readFile(filename, 'utf-8', function(err, filedata){
                    if(err){
                        console.error(err);
                    }else{
                        //window.alert("filedata:"+filedata)
                        t.dic[fPath] = filedata
                    }
                })
            });

            /*for(let key in t.dic){
                window.alert(key+":"+t.dic[key])
            }*/
        },

        csvToObject(csvString){
            var csvarry = csvString.split("\r\n");
            var datas = [];

            for(var i = 0; i < csvarry.length; i++){
                var data = csvarry[i].split(",");
                if(data.length > 1){
                    datas.push(data);
                }
            }
            return datas;
        },

        getCsvConfig(){
            const t = this
            let filename = "instruct.csv"
            fs.readFile(filename, 'utf-8', function(err, data){
                if(err){
                    console.error(err);
                }else{
                    let result = t.csvToObject(data)
                    for(let i = 0; i < result.length; i++){

                        let strategy = result[i][2].substr(0,6);
                        let total_volume = result[i][3]
                        t.dic[strategy] = total_volume

                    }
                }
            })
        },

        //重置数据
        resetData() {
            const t = this;
            this.searchKeyword = '';
            this.filter = {
                id: '',
                dateRange: null
            };
            t.getDataLock = false;
            t.tableData = Object.freeze([]);
            return true;
        },

        init: debounce(function() {
            const t = this
            //t.getFilesNames("resources/kfc/twap")
            t.getCsvConfig()
            t.getData()
        }),

        //首次获取数据
        getData() {
            const t = this
            if(t.getDataLock) throw new Error('get-data-lock');
            //获得获取数据的方法名
            t.getDataLock = true
            t.tableData = Object.freeze([])
            //id:用户或者交易id，filter：需要筛选的数据
            //window.alert("getDataMethod")
            return t.getDataMethod(t.currentId, t.filter, t.tradingDay).then(res => {
                //window.alert("length="+res.length)
                if(!res || !res.length) {
                    t.tableData = Object.freeze([])
                    return;
                }
                //writeCSV("a.csv", res)
                t.tableData = Object.freeze(t.dealData(res))

            }).finally(() => t.getDataLock = false)
        },

        //对返回的数据进行处理
        dealData(data) {
            const t = this
            const historyData = data || []
            let tableData = historyData.map(item => dealTrades(item))

            let tableData2 = []; let table_dic = {};
            for(let i = 0; i < tableData.length; i++){
                //window.alert(tableData[i].instrumentId+tableData[i].volume)
                if(!(tableData[i].instrumentId in table_dic)){
                    table_dic[tableData[i].instrumentId] = 1
                    let total_volume = t.dic[tableData[i].instrumentId]
                    let rate_str = t.caculateRate(Number(tableData[i].volume), total_volume)
                    tableData[i].volume_total = total_volume
                    tableData[i].rate = rate_str
                    tableData[i].trade_money = Number(tableData[i].price) * Number(tableData[i].volume)
                    tableData2.push(tableData[i])
                }else{
                    for(let j = 0; j < tableData2.length; j++){
                        if(tableData2[j].instrumentId == tableData[i].instrumentId){
                            //window.alert("in")
                            tableData2[j].volume = Number(tableData2[j].volume) + Number(tableData[i].volume)
                            let rate_str = t.caculateRate(Number(tableData2[j].volume), tableData2[j].volume_total)
                            tableData2[j].rate = rate_str
                            tableData2[j].trade_money = Number(tableData2[j].trade_money) + Number(tableData[i].price) * Number(tableData[i].volume)
                            break
                        }
                    }
                }
            }
            return tableData2
        },


        dealNanomsg(data) {
            const t = this
            //当有日期筛选的时候，不需要推送数据
            if(t.filter.dateRange) return
            //如果存在筛选，则推送的数据也需要满足筛选条件
            const { id, dateRange } = t.filter
            const { trade_time } = data
            if(!((data.instrument_id.includes(id) || data.client_id.includes(id)) )) return
            const tradeData = {
                ...dealTrades(data),
                nano: true
            }
            t.throttleInsertTrade(tradeData).then(tradeList => {
                if(!tradeList) return;
                let oldTableData = t.tableData;
                //oldTableData = [...tradeList, ...oldTableData]
                
                for(let i = 0; i < oldTableData.length; i++){
                    for(let j = 0; j < tradeList.length; j++){
                        if(oldTableData[i].instrumentId == tradeList[j].instrumentId){
                            //window.alert("in")
                            //window.alert(tradeList.length.toString())
                            let total_volume = t.dic[tradeList[j].instrumentId]
                            oldTableData[i].volume = Number(oldTableData[i].volume) + Number(tradeList[j].volume)
                            let rate_str = t.caculateRate(Number(oldTableData[i].volume), total_volume)
                            oldTableData[i].rate = rate_str
                            oldTableData[i].trade_money = Number(oldTableData[i].trade_money) + Number(tradeList[j].price) * Number(tradeList[j].volume)
                            //break
                        }
                    }
                }

                t.tableData = Object.freeze(oldTableData)
                //t.tableData = tradeList
                /*let filename = "resources/kfc/twap/" + oldTableData[0].instrumentId + ".txt"
                //window.alert("filename2:"+filename)
                fs.readFile(filename, 'utf-8', function(err, filedata){
                    if(err){
                        console.error(err);
                    }else{
                        let add_up = 0
                        for(let i = 0; i < oldTableData.length; i++)
                        {
                            add_up += Number(oldTableData[i].volume)
                        }
                        let rate_str = t.caculateRate(add_up, filedata)

                        t.tableData = Object.freeze([{
                                            id: "no",
                                            updateTime: "no",
                                            updateTimeNum: 12,
                                            instrumentId: oldTableData[0].instrumentId,
                                            side: oldTableData[0].side,
                                            offset: oldTableData[0].offset,
                                            price: "no",
                                            volume: add_up.toString(),
                                            volume_total: filedata,
                                            rate: rate_str,
                                            clientId: oldTableData[0].clientId,
                                            accountId: "no"
                                        }])
                    }
                })*/
            })
           
        },

        renderCellClass(prop, item){
            if(prop === 'side'){
                if(item.side === '买') return 'red'
                else if(item.side === '卖') return 'green'
            }
            if(prop === 'offset'){
                if(item.offset === '开仓') return 'red'
                else if(item.offset === '平仓') return 'green'
            }
        }
    }
}
</script>

<style lang="scss">
.trades-record{
    height: 100%;
}
</style>