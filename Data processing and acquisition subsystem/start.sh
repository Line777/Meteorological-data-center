################################
# 启动数据中心后台服务程序的脚本
################################

# 检查服务程序是否超时，配置在/etc/rc.local中用root用户执行
# /project/tools1/bin/procctl 30 /project/tools1/bin/checkproc

# 压缩数据中心后台服务程序的备份日志
/project/tools1/bin/procctl 300 /project/tools1/bin/gzipfiles /log/idc "*.log.20*" 0.04

#生成用于测试的全国气象站点观测的分钟数据
/project/tools1/bin/procctl 60 /project/idc1/bin/crtsurfdata /project/idc1/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata.log xml,json,csv

# 清理原始的全国气象站点观测分钟数据的目录 /tmp/idc/surfdata中的历史数据文件
/project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /tmp/idc/surfdata "*" 0.04

# 下载全国气象站点观测的分钟数据的xml
/project/tools1/bin/procctl 30 /project/tools1/bin/ftpgetfiles /log/idc/ftpgetfiles_surfdata.log "<host>192.168.10.131:21</host><mode>1</mode><username>jyl</username><password>1228488450
jyl.</password><localpath>/idcdata/surfdata</localpath><remotepath>/tmp/idc/surfdata</remotepath><matchname>SURF_ZH*.XML</matchname><listfilename>/idcdata/ftplist/ftpgetfiles_surfdata.li
st</listfilename><ptype>1</ptype><okfilename>/idcdata/ftplist/ftpgetfiles_surfdata.xml</okfilename><checkmtime>true</checkmtime><timeout>80</timeout><pname>ftpgetfiles_surfdata</pname>"

# 上传全国气象站点观测的分钟数据的xml文件
/project/tools1/bin/procctl 30 /project/tools1/bin/ftpputfiles /log/idc/ftpputfiles_surfdata.log "<host>127.0.0.1:21</host><mode>1</mode><username>wucz</username><password>wuczpwd</passw
ord><localpath>/tmp/idc/surfdata</localpath><remotepath>/tmp/ftpputest</remotepath><matchname>SURF_ZH*.JSON</matchname><ptype>1</ptype><okfilename>/idcdata/ftplist/ftpputfiles_surfdata.x
ml</okfilename><timeout>80</timeout><pname>ftpputfiles_surfdata</pname>"

# 清理原始的全国气象站点观测的分钟数据目录/tmp/idc/surfdata中的历史数据文件
/project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /idcdata/surfdata "*" 0.04

# 清理原始的全国气象站点观测的分钟数据目录/tmp/idc/surfdata中的历史数据文件
/project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /tmp/ftpputtest "*" 0.04
