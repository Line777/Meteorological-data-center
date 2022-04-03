#include "_public.h"
#include "_mysql.h"

struct st_arg
{
  char connstr[101];     // 数据库的连接参数。
  char charset[51];      // 数据库的字符集。
  char selectsql[1024];  // 从数据源数据库抽取数据的SQL语句。
  char fieldstr[501];    // 抽取数据的SQL语句输出结果集字段名，字段名之间用逗号分隔。
  char fieldlen[501];    // 抽取数据的SQL语句输出结果集字段的长度，用逗号分隔。
  char bfilename[31];    // 输出xml文件的前缀。
  char efilename[31];    // 输出xml文件的后缀。
  char outpath[301];     // 输出xml文件存放的目录。
  char starttime[52];    // 程序运行的时间区间
  char incfield[31];     // 递增字段名。
  char incfilename[301]; // 已抽取数据的递增字段最大值存放的文件。
  int  timeout;          // 进程心跳的超时时间。
  char pname[51];        // 进程名，建议用"dminingmysql_后缀"的方式。
}starg;

CLogFile logfile;
CPActive PActive;

void EXIT(int sig);                  //程序退出和2，15号信号的处理函数
void _help();                        //帮助函数
bool _xmltoarg(char *strxmlbuffer);  //将xml解析到starg结构体中
bool _dminingmysql();                 //文件上传主函数

int main(int argc,char *argv[])
{

  if (argc!=3) { _help(); return -1; }

  // 关闭全部的信号和输入输出。
  // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
  // 但请不要用 "kill -9 +进程号" 强行终止。
  CloseIOAndSignal(); 
  // signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

  // 打开日志文件。
  if (logfile.Open(argv[1],"a+")==false)
  {
    printf("打开日志文件失败（%s）。\n",argv[1]); return -1;
  }

  // 解析xml，得到程序运行的参数。
  if (_xmltoarg(argv[2])==false) return -1;
  PActive.AddPInfo(starg.timeout,starg.pname);  //把进程的心跳信息写出共享内存中
  
  logfile.Write("ftp.login success\n");
  _dminigmysql();
  return 0;
}


bool _dminingmysql()
{
  return true;
}

// 把xml解析到参数starg结构中。
bool _xmltoarg(char *strxmlbuffer)
{
  memset(&starg,0,sizeof(struct st_arg));

  GetXMLBuffer(strxmlbuffer,"host",starg.host,30);   // 远程服务器的IP和端口。
  if (strlen(starg.host)==0)
  { logfile.Write("host is null.\n");  return false; }

  GetXMLBuffer(strxmlbuffer,"mode",&starg.mode);   // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
  if (starg.mode!=2)  starg.mode=1;

  GetXMLBuffer(strxmlbuffer,"username",starg.username,30);   // 远程服务器ftp的用户名。
  if (strlen(starg.username)==0)
  { logfile.Write("username is null.\n");  return false; }

  GetXMLBuffer(strxmlbuffer,"password",starg.password,30);   // 远程服务器ftp的密码。
  if (strlen(starg.password)==0)
  { logfile.Write("password is null.\n");  return false; }

  GetXMLBuffer(strxmlbuffer,"remotepath",starg.remotepath,300);   // 远程服务器存放文件的目录。
  if (strlen(starg.remotepath)==0)
  { logfile.Write("remotepath is null.\n");  return false; }

  GetXMLBuffer(strxmlbuffer,"localpath",starg.localpath,300);   // 本地文件存放的目录。
  if (strlen(starg.localpath)==0)
  { logfile.Write("localpath is null.\n");  return false; }
  
  GetXMLBuffer(strxmlbuffer,"matchname",starg.matchname,100);   // 待上传文件匹配的规则。
  if (strlen(starg.matchname)==0)
  { logfile.Write("matchname is null.\n");  return false; }

  GetXMLBuffer(strxmlbuffer,"ptype",&starg.ptype);   // 上传文件后服务器的操作
  if ( (starg.ptype!=1) && (starg.ptype!=2) && (starg.ptype!=3))
  { logfile.Write("ptype is error.\n");  return false; }

  GetXMLBuffer(strxmlbuffer,"localpathbak",starg.localpathbak,300);   // 上传后客户端文件备份目录
  if ((strlen(starg.localpathbak)==0) && (starg.ptype==3))
  { logfile.Write("localpathbak is null.\n");  return false; }

  GetXMLBuffer(strxmlbuffer,"okfilename",starg.okfilename,300);   // 成功上传的文件清单
  if ((strlen(starg.okfilename)==0) && starg.ptype==1) 
  { logfile.Write("okfilename is null.\n");  return false; 
  }

  GetXMLBuffer(strxmlbuffer,"timeout",&starg.timeout);   // 成功上传的文件清单
  if ((starg.timeout==0))
  { logfile.Write("timeout is null.\n");  return false;
  }
  GetXMLBuffer(strxmlbuffer,"pname",starg.pname,50);   // 成功上传的文件清单
  if (strlen(starg.pname)==0)
  { logfile.Write("pname is null.\n");  return false;
  }
  return true;
}

void EXIT(int sig)
{
  printf("程序退出，sig=%d\n\n",sig);

  exit(0);
}

void _help()
{
  printf("Using:/project/tools1/bin/dminingmysql logfilename xmlbuffer\n\n");

  printf("Sample:/project/tools1/bin/procctl 3600 /project/tools1/bin/dminingmysql /log/idc/dminingmysql_ZHOBTCODE.log \"<connstr>127.0.0.1,root,mysqlpwd,mysql,3306</connstr><charset>gbk
</charset><selectsql>select obtid,cityname,provname,lat,lon,height from T_ZHOBTCODE</selectsql><fieldstr>obtid,cityname,provname,lat,lon,height</fieldstr><fieldlen>10,30,30,10,10,10</fie
ldlen><bfilename>ZHOBTCODE</bfilename><efilename>HYCZ</efilename><outpath>/idcdata/dmindata</outpath>\"\n\n");
  printf("       /project/tools1/bin/procctl   30 /project/tools1/bin/dminingmysql /log/idc/dminingmysql_ZHOBTMIND.log \"<connstr>127.0.0.1,root,mysqlpwd,mysql,3306</connstr><charset>gbk
</charset><selectsql>select obtid,date_format(ddatetime,'%%%%Y-%%%%m-%%%%d %%%%H:%%%%i:%%%%s'),t,p,u,wd,wf,r,vis,keyid from t_zhobtmind where keyid>:1 and ddatetime>timestampadd(minute,-
120,now())</selectsql><fieldstr>obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid</fieldstr><fieldlen>10,19,8,8,8,8,8,8,8,15</fieldlen><bfilename>ZHOBTMIND</bfilename><efilename>HYCZ</efilename><o
utpath>/idcdata/dmindata</outpath><starttime></starttime><incfield>keyid</incfield><incfilename>/idcdata/dmining/dminingmysql_ZHOBTMIND_HYCZ.list</incfilename>\"\n\n");

  printf("本程序是数据中心的公共功能模块，用于从mysql数据库源表抽取数据，生成xml文件。\n");
  printf("logfilename 本程序运行的日志文件。\n");
  printf("xmlbuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

  printf("connstr     数据库的连接参数，格式：ip,username,password,dbname,port。\n");
  printf("charset     数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
  printf("selectsql   从数据源数据库抽取数据的SQL语句，注意：时间函数的百分号%需要四个，显示出来才有两个，被prepare之后将剩一个。\n");
  printf("fieldstr    抽取数据的SQL语句输出结果集字段名，中间用逗号分隔，将作为xml文件的字段名。\n");
  printf("fieldlen    抽取数据的SQL语句输出结果集字段的长度，中间用逗号分隔。fieldstr与fieldlen的字段必须一一对应。\n");
  printf("bfilename   输出xml文件的前缀。\n");
  printf("efilename   输出xml文件的后缀。\n");
  printf("outpath     输出xml文件存放的目录。\n");
  printf("starttime   程序运行的时间区间，例如02,13表示：如果程序启动时，踏中02时和13时则运行，其它时间不运行。"\
         "如果starttime为空，那么starttime参数将失效，只要本程序启动就会执行数据抽取，为了减少数据源"\
         "的压力，从数据库抽取数据的时候，一般在对方数据库最闲的时候时进行。\n");
  printf("incfield    递增字段名，它必须是fieldstr中的字段名，并且只能是整型，一般为自增字段。"\
          "如果incfield为空，表示不采用增量抽取方案。");
  printf("incfilename 已抽取数据的递增字段最大值存放的文件，如果该文件丢失，将重新抽取全部的数据。\n");
  printf("timeout     本程序的超时时间，单位：秒。\n");
  printf("pname       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");
}
