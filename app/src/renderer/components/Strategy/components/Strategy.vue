<template>
<tr-dashboard title="策略">
    <div slot="dashboard-header">
        <tr-dashboard-header-item>
            <tr-search-input v-model.trim="searchKeyword"></tr-search-input>
        </tr-dashboard-header-item>
        <tr-dashboard-header-item>
            <el-button size="mini" @click="handleAddStrategy" title="添加">添加</el-button>
        </tr-dashboard-header-item>
        <tr-dashboard-header-item>
            <el-button size="mini" @click="handleAddStrategies" title="批量添加">批量添加</el-button>
        </tr-dashboard-header-item>
        <tr-dashboard-header-item>
            <el-button size="mini" @click="startAllStrategies" title="批量启动">批量启动</el-button>
        </tr-dashboard-header-item>
        <tr-dashboard-header-item>
            <el-button size="mini" @click="stopAllStrategies" title="全部停止">全部停止</el-button>
        </tr-dashboard-header-item>
    </div>
    <div class="table-body">
        <el-table
        :data="strategyListFilter"
        @row-click="handleRowClick"
        :row-class-name="handleSelectRow"
        size="small"
        height="100%"
        v-if="renderTable"
        >
            <el-table-column
                prop="strategy_id"
                label="代码"
                sortable
                :show-overflow-tooltip="true"         
                >
            </el-table-column>
            <el-table-column
                label="状态"
                sortable  
                show-overflow-tooltip
            >
                <template slot-scope="props">
                    <tr-status 
                    v-if="$utils.ifProcessRunning(props.row.strategy_id, processStatus)"
                    :value="processStatus[props.row.strategy_id]"
                    ></tr-status>
                    <tr-status v-else></tr-status>
                </template>
            </el-table-column>
              <el-table-column
                label="运行"
                sortable    
            >
                <template slot-scope="props" v-if="props.row.strategy_id">
                    <el-switch
                    :value="$utils.ifProcessRunning(props.row.strategy_id, processStatus)"
                    @change="e => handleStrategySwitch(e, props.row)"
                    ></el-switch> 
                </template>
            </el-table-column>
            <el-table-column
                label="类型"
                sortable    
                prop="strategy_type" 
                :show-overflow-tooltip="true"
            >
            </el-table-column>
            <el-table-column
                label="运行时间"
                sortable    
                prop="run_time" 
                :show-overflow-tooltip="true"
            >
            </el-table-column>
          
            <el-table-column
                label="" 
                width="110px"  
                align="right"       
            >
                <template slot-scope="props">
                    <span class="tr-oper" @click.stop="handleSetStrategy(props.row)"><i class="mouse-over el-icon-setting"></i></span>
                    <span class="tr-oper" @click.stop="handleEditStrategy(props.row)"><i class="mouse-over el-icon-edit-outline"></i></span>
                    <span class="tr-oper-delete" @click.stop="handleDeleteStrategy(props.row)"><i class="mouse-over el-icon-delete"></i></span>
                </template>
            </el-table-column>
        </el-table>
    </div>

    <el-dialog 
    width="450px" 
    title="批量添加策略"
    v-if="setStrategiesDialogVisiblity"
    :visible.sync="setStrategiesDialogVisiblity" 
    :close-on-click-modal="false"
    :close-on-press-escape="true"
    @close="handleClearAddStrategiesDialog"
    @keyup.enter.native="addStrategies"
    >
        <el-form ref="setStrategiesForm" label-width="90px" :model="setStrategiesForm">
            <!-- 自定义部分 -->
            <el-form-item
                label="开始时间"
                prop="startTime"
                :rules="[
                { required: true, message: '请输入开始时间', trigger: 'blur' },
                { min: 1, max: 20, message: '长度不能超过 20 个字符', trigger: 'blur' },
                {validator: chineseValidator, trigger: 'blur'},
                {validator: noZeroAtFirstValidator, trigger: 'blur'}
                ]"
            >
                <el-input 
                v-model.trim="setStrategiesForm.startTime" 
                :disabled="'add' == 'set'"
                 placeholder="请输入开始时间"
                 ></el-input>
            </el-form-item>
            <el-form-item
                label="结束时间"
                prop="endTime"
                :rules="[
                { required: true, message: '请输入结束时间', trigger: 'blur' },
                { min: 1, max: 20, message: '长度不能超过 20 个字符', trigger: 'blur' },
                {validator: chineseValidator, trigger: 'blur'},
                {validator: noZeroAtFirstValidator, trigger: 'blur'}
                ]"
            >
                <el-input 
                v-model.trim="setStrategiesForm.endTime" 
                :disabled="'add' == 'set'"
                 placeholder="请输入结束时间"
                 ></el-input>
            </el-form-item>
            <el-form-item
            label="指令单"
            prop="csvPath"
            :rules="[
                { required: true, message: '请选择csv文件路径', trigger: 'blur' },
            ]"
            >
                <span class="path-selection-in-dialog text-overflow" :title="setStrategiesForm.csvPath">{{setStrategiesForm.csvPath}}</span>
                <el-button size="mini" icon="el-icon-more" @click="handleBindStrategiesFolder"></el-button>
            </el-form-item>
        </el-form>
        <div slot="footer" class="dialog-footer">
            <el-button  size="mini" @click="handleClearAddStrategiesDialog">取 消</el-button>
            <el-button type="primary" size="mini" @click="addStrategies">确 定</el-button>
        </div>
    </el-dialog>

    <el-dialog 
    width="340px" 
    :title="setStrategyDialogType == 'add' ? '添加策略' : '设置策略'"  
    v-if="setStrategyDialogVisiblity"
    :visible.sync="setStrategyDialogVisiblity" 
    :close-on-click-modal="false"
    :close-on-press-escape="true"
    @close="handleClearAddStrategyDialog"
    @keyup.enter.native="handleConfirmAddEditorStrategy"
    >
        <el-form ref="setStrategyForm" label-width="90px" :model="setStrategyForm">
            <!-- 自定义部分 -->
            <el-form-item
                label="策略ID"
                prop="strategyId"
                :rules="[
                { required: true, message: '请输入策略名称', trigger: 'blur' },
                { min: 1, max: 20, message: '长度不能超过 20 个字符', trigger: 'blur' },
                {validator: validateDuplicateStrategyId, trigger: 'blur'},
                {validator: chineseValidator, trigger: 'blur'},
                {validator: specialStrValidator, trigger: 'blur'},
                {validator: noZeroAtFirstValidator, trigger: 'blur'}
                ]"
            >
                <el-input 
                v-model.trim="setStrategyForm.strategyId" 
                :disabled="setStrategyDialogType == 'set'"
                 placeholder="请输入策略名称"
                 ></el-input>
            </el-form-item>
            <el-form-item
            label="入口文件"
            prop="strategyPath"
            :rules="[
                { required: true, message: '请选择策略入口文件路径', trigger: 'blur' },
            ]"
            >
                <span class="path-selection-in-dialog text-overflow" :title="setStrategyForm.strategyPath">{{setStrategyForm.strategyPath}}</span>
                <el-button size="mini" icon="el-icon-more" @click="handleBindStrategyFolder"></el-button>
            </el-form-item>
        </el-form>
        <div slot="footer" class="dialog-footer">
            <el-button  size="mini" @click="handleClearAddStrategyDialog">取 消</el-button>
            <el-button type="primary" size="mini" @click="handleConfirmAddEditorStrategy">确 定</el-button>
        </div>
    </el-dialog>
</tr-dashboard>

</template>
<script>
import path from 'path';
import { remote } from 'electron';
import { mapState, mapGetters } from 'vuex';
import { openWin } from '__gUtils/busiUtils';
import { deleteProcess } from '__gUtils/processUtils';
import { outputJson } from '__gUtils/fileUtils';
import * as STRATEGY_API from '__io/db/strategy';
import { switchStrategy } from '__io/actions/strategy';
import { debounce } from '__gUtils/busiUtils';
import { chineseValidator, specialStrValidator, noZeroAtFirstValidator } from '__assets/validator';
const fs = require('fs');

const BrowserWindow = require('electron').remote.BrowserWindow

export default {
    data(){
        this.chineseValidator = chineseValidator;
        this.specialStrValidator = specialStrValidator;
        this.noZeroAtFirstValidator = noZeroAtFirstValidator;
        return {
            searchKeyword: '',
            searchKeywordDebounce: '',
            setStrategiesDialogVisiblity: false,
            setStrategyDialogVisiblity: false,
            setStrategyDialogType: '',
            setStrategyForm: {
                strategyId: '',
                strategyPath: "",
            },
            setStrategiesForm: {
                startTime: '',
                endTime: '',
                csvPath: '',
            },
            renderTable: false,
        }
    },

    beforeMount(){
        const t = this;
        t.getStrategyList();
    },

    mounted(){
        const t = this;
        t.renderTable = true;
    },

    watch: {
        searchKeyword: debounce(function (value) {
            this.searchKeywordDebounce = value
        }),
    },

    computed: {
        ...mapState({
            currentStrategy: state => state.STRATEGY.currentStrategy,
            strategyList: state => state.STRATEGY.strategyList,
            processStatus: state => state.BASE.processStatus
        }),

        strategyListFilter(){
            const t = this;
            if(!t.searchKeywordDebounce) return t.strategyList;
            return t.strategyList.filter(s => (s.strategy_id.indexOf(t.searchKeywordDebounce) !== -1))
        },
    },

    methods: {
        //绑定策略路径
        handleBindStrategyFolder(){
            const t = this;
            const dialog = remote.dialog;
            dialog.showOpenDialog({
                properties: ['openFile']
            }, (strategyPath) => {
                if(!strategyPath || !strategyPath[0]) return;
                t.setStrategyForm.strategyPath = strategyPath[0];
                t.$refs['setStrategyForm'].validate() //手动进行再次验证，因数据放在span中，改变数据后无法触发验证
            })
        },

        handleBindStrategiesFolder(){
            const t = this;
            const dialog = remote.dialog;
            dialog.showOpenDialog({
                properties: ['openFile']
            }, (csvPath) => {
                if(!csvPath || !csvPath[0]) return;
                t.setStrategiesForm.csvPath = csvPath[0];
                t.$refs['setStrategiesForm'].validate() //手动进行再次验证，因数据放在span中，改变数据后无法触发验证
            })
        },

        //选中行的背景颜色
        handleSelectRow(row) {
            const t = this;
            if(row.row.strategy_id == t.currentStrategy.strategy_id) {
                return 'selected-bg'
            }
        },

        //设置当前strategy
        handleRowClick(row) {
            const t = this;
            t.$store.dispatch('setCurrentStrategy', row)
        },
        
        //添加策略
        handleAddStrategy() {
            const t = this;
            t.setStrategyDialogVisiblity = true;
            t.setStrategyDialogType = 'add'
        },

        handleAddStrategies() {
            const t = this;
            t.setStrategiesDialogVisiblity = true
        },

        //删除策略
        handleDeleteStrategy(row){
            const t = this;
            const strategy_id = row.strategy_id + ''
            const strategyProcessStatus = t.$utils.ifProcessRunning(strategy_id, t.processStatus)
            if(strategyProcessStatus){
                t.$message.warning("需先停止该策略运行！")
                return;
            }

            t.$confirm(`删除策略 ${strategy_id} 会删除所有相关信息，确认删除吗？`, '提示', {confirmButtonText: '确 定', cancelButtonText: '取 消'})
            .then(() => t.$store.dispatch('deleteStrategy', strategy_id))
            .then(() => {
                 //如果删除的项是选中的项，则默认选中第一项,如果没有项了，则当前项为空对象{}
                if (t.currentStrategy.strategy_id === strategy_id) {
                    const currentStrategy = t.strategyList.length ? t.strategyList[0] : {}
                    t.$store.dispatch('setCurrentStrategy', currentStrategy)
                }
            })
            .then(() => deleteProcess(strategy_id))
            .then(() => t.$message.success('操作成功！'))
            .catch((err) => {
                if(err == 'cancel') return
                t.$message.error(err.message || '操作失败！')
            })
        },

        //编辑策略
        handleEditStrategy(row){
            const t = this;
            t.$utils.openWin(`code/${row.strategy_id}`, BrowserWindow)
        },

        //设置策略
        handleSetStrategy(row){
            const t = this;
            t.setStrategyDialogVisiblity = true;
            t.setStrategyDialogType = 'set'
            t.setStrategyForm = {
                strategyId: row.strategy_id,
                strategyPath: row.strategy_path
            }
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

        copyIt(from, to) {
          fs.writeFileSync(to, fs.readFileSync(from));
          //fs.createReadStream(src).pipe(fs.createWriteStream(dst));大文件复制
        },

        writeTime(){
            const t = this;
            //t.$refs['setStrategiesForm'].validate(valid => {
            //    if(valid){
                const csvPath = t.setStrategiesForm.csvPath;
                //window.alert(csvPath)
                t.copyIt(csvPath, "instruct.csv")

                    const startTime = t.setStrategiesForm.startTime;
                    const endTime = t.setStrategiesForm.endTime;
                    let config_json = {}
                    config_json["beginTime"] = startTime
                    config_json["endTime"] = endTime
                    outputJson('config.json', config_json)
            /*    }else{
                    return false
                }
            })*/
        },

        addStrategies(){
            const t = this;
            //window.alert("addStrategies")
            t.$refs['setStrategiesForm'].validate(valid => {
                if(valid){

                    t.writeTime()
                    t.handleClearAddStrategiesDialog()

                    let filename = "instruct.csv"
                    fs.readFile(filename, 'utf-8', function(err, data){
                        if(err){
                            console.error(err);
                        }else{
                            STRATEGY_API.deleteTradeData()
                            let result = t.csvToObject(data)
                            for(let i = 0; i < result.length; i++){
                                //window.alert(result[i][2])
                                //let strategy = "total3" /*+ i.toString()*/;
                                let strategy = result[i][2].substr(0,6);
                                let strategyPath = "../../../../../../examples/strategy/cpp/build/Release/cpp_demo.cp37-win_amd64.pyd";
                                //let strategyPath = "../../cpp_demo.cp37-win_amd64.pyd";
                                //window.alert("Promise")
                                let firstStepPromise = new Promise(resolve => resolve()) // 添加编辑行为不一样；
                                firstStepPromise.then(() => {
                                    //window.alert("in")
                                    const strategyMethod = STRATEGY_API.addStrategy
                                    //window.alert("in2")
                                    strategyMethod(strategy, strategyPath)
                                    .then(() => t.getStrategyList())//get new list
                                    .then(() => {
                                        //window.alert("sql")
                                        let trade_id = "no" + strategy
                                        STRATEGY_API.insertTradeData(strategy, trade_id)
                                    })
                                    .catch((err) => {
                                        if(err == 'cancel') return
                                        t.$message.error(err.message || '操作失败！')
                                    })
                                })
                            }
                        }
                    })
                }else{
                    //window.alert("false")
                    return false
                }
            })
        },

        //确认添加/编辑策略
        handleConfirmAddEditorStrategy(){
            const t = this;
            t.$refs['setStrategyForm'].validate(valid => {
                if(valid){
                    const strategy = t.setStrategyForm.strategyId.toString();
                    const strategyPath = t.setStrategyForm.strategyPath;
                    let firstStepPromise = new Promise(resolve => resolve()) // 添加编辑行为不一样；
                    if(t.setStrategyDialogType === 'add'){
                        firstStepPromise = t.$confirm(`添加后策略ID不能更改，确认添加 ${strategy} 吗？`, '提示', {
                            confirmButtonText: '确 定',
                            cancelButtonText: '取 消',
                        })
                    }
                    firstStepPromise.then(() => {
                        //判断是添加还是修改数据库内容
                        const strategyMethod = t.setStrategyDialogType === 'add' ? STRATEGY_API.addStrategy : STRATEGY_API.updateStrategyPath
                        strategyMethod(strategy, strategyPath)
                        .then(() => t.getStrategyList())//get new list
                        .then(() => {
                            t.$message.success((t.setStrategyDialogType === 'add'? '添加' : '修改') + '成功！')
                            t.handleClearAddStrategyDialog()//clear
                        })
                        .catch((err) => {
                            if(err == 'cancel') return
                            t.$message.error(err.message || '操作失败！')
                        })
                    })
                }else{
                    return false
                }
            })
        },

        //启停策略
        handleStrategySwitch(value, strategy){
            const t = this;
            const strategyId = strategy.strategy_id;
            switchStrategy(strategyId, value).then(({ type, message }) => t.$message[type](message));
        },

        //关闭添加strategy弹窗, refresh数据
        handleClearAddStrategyDialog(){
            const t = this;
            t.setStrategyForm = { strategyId: '', strategyPath: '' };
            t.setStrategyDialogVisiblity = false;
            t.setStrategyDialogType = ''
        },

        handleClearAddStrategiesDialog(){
            const t = this;
            t.setStrategiesForm = { startTime: '', endTime: '', csvPath: '' };
            t.setStrategiesDialogVisiblity = false
        },

        //check策略是否重复
        validateDuplicateStrategyId(rule, value, callback){
            const t = this;
            const ifDuplicate = t.strategyList.filter(s => (s.strategy_id === value)).length !== 0
            if(ifDuplicate && t.setStrategyDialogType !== 'set'){
                callback(new Error('该策略ID已存在！'))
            }else{
                callback()
            }
        },

        startAllStrategies(){
            const t = this;
            return new Promise(resolve => {
                t.$store.dispatch('getStrategyList').then(strategyList => {
                    resolve(strategyList)

                    for(let i = 0; i < strategyList.length; i++){
                        //window.alert("startAllStrategies:"+strategyList[i].strategy_id+" "+strategyList[i].strategy_path)
                        let strategyId = strategyList[i].strategy_id
                        let value = true
                        switchStrategy(strategyId, value).then(({ type, message }) => t.$message[type](message));
                    }

                })
            }) 
        },

        stopAllStrategies(){
            const t = this;
            return new Promise(resolve => {
                t.$store.dispatch('getStrategyList').then(strategyList => {
                    resolve(strategyList)

                    for(let i = 0; i < strategyList.length; i++){
                        //window.alert("startAllStrategies:"+strategyList[i].strategy_id+" "+strategyList[i].strategy_path)
                        let strategyId = strategyList[i].strategy_id
                        let value = false
                        switchStrategy(strategyId, value).then(({ type, message }) => t.$message[type](message));
                    }

                })
            }) 
        },

        //获取策略列表
        getStrategyList(){
            const t = this;
            return new Promise(resolve => {
                t.$store.dispatch('getStrategyList').then(strategyList => {
                    resolve(strategyList)
                    //设置第一条为currentStrategy
                    if(!t.currentStrategy.strategy_id){
                        t.$store.dispatch('setCurrentStrategy', strategyList[0] || {})
                    }
                })
            }) 
        }
    }
}
</script>
<style lang="scss">
@import '@/assets/scss/skin.scss';
.strategy-mouse{
    &:hover{
        cursor: pointer;
    }
}

.strategy-list .el-input .el-input__inner{
    height: 23px;
    line-height: 23px;
    padding: 0;
}

</style>
