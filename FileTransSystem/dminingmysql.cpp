#include "_public.h"
#include "_mysql.h"

#define MAXFIELDCOUNT  100  // 结果集字段的最大数
// #define MAXFIELDLEN    500  // 结果集字段值的最大长度

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
  int  maxcount;         // 输出xml文件最大记录数，0表示无限制
  char starttime[52];    // 程序运行的时间区间
  char incfield[31];     // 递增字段名。
  char incfilename[301]; // 已抽取数据的递增字段最大值存放的文件。
  char connstr1[101];    // 以抽取数据的递增字段最大值存放在数据库的连接参数
  int  timeout;          // 进程心跳的超时时间。
  char pname[51];        // 进程名，建议用"dminingmysql_后缀"的方式。
}starg;

connection conn,conn1;
CLogFile logfile;
CPActive PActive;
char strxmlfilename[301];
char strfieldname[MAXFIELDCOUNT][31];  //结果集字段名数组，从starg.fieldstr解析得到
int  ifieldlen[MAXFIELDCOUNT];         //结果集字段的长度数组，从starg.fieldlen解析得到
int  ifieldcount;                      //strfieldname和ifieldlen数组中有效字段的个数
int  incfieldpos=-1;                   //递增字段在结果集数组中的位置。
int  MAXFIELDLEN=-1;
long imaxincvalue;

bool writeincfile();                   //把已抽取数据的最大id写入数据库表或starg.incfilename文件
bool readincfile();                    //从数据库表中或starg.incfilename文件中获取已抽取数据的最大id
void crtxmlfilename();
void EXIT(int sig);                    //程序退出和2，15号信号的处理函数
void _help();                          //帮助函数
bool _xmltoarg(char *strxmlbuffer);    //将xml解析到starg结构体中
bool _dminingmysql();                  //文件上传主函数
bool instarttime();

int main(int argc,char *argv[])
{

  if (argc!=3)
  {
    _help();
    return -1;
  }
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
  if (instarttime()==false）
  {
    return 0;
  }
  PActive.AddPInfo(starg.timeout,starg.pname);  //把进程的心跳信息写出共享内存中
  // 连接数据库
  if (conn.connecttodb(starg.connstr,starg.charset)!=0)
  {
    logfile.Write("connect database(%s) failed.\n%s\n",starg.connstr,conn.m_cda.message);
    return -1;
  }
  logfile.Write("connect database(%s) ok.\n",starg.connstr);
  // 连接本地数据库，用于存放以抽取数据的自增字段的最大值
  if (strlen(starg.connstr1)!=0)
  {
    if (conn1.connecttodb(starg.connstr1,starg.charset)!=0)
    {
      logfile.Write("connect database(%s) failed.\n%s\n",starg.connstr1,conn1.m_cda.message);
      return -1;
    }
    logfile.Write("connect database(%s) ok.\n",starg.connstr1);
  }
  _dminigmysql();
  return 0;
}

void crtxmlfilename()
{
  char strLocalTime[21];
  memset(strLocalTime,0,sizeof(strLocalTime));
  LocalTime(strLocalTime,"yyyymmddhh24miss");
  static iseq=1;
  SNPRINTF(strxmlfilename,300,sizeof(strxmlfilename),"%s/%s_%s_%s_%d.xml",starg.outpath,starg.bfilename,strLocalTime,starg.efilename,iseq++);
}
bool readincfile()
{
  imaxincvalue=0;
   //如果starg.incfield参数为空，表示不是增量抽取
  if (strlen(starg.incfield)==0)
  {
    return true;
  }
  if (strlen(starg.connstr1)!=0)
  {
    // 从数据库表中加载自增字段的最大值
    // create table T_MAXINCVALUE(pname varchar(50),maxincvalue numeric(15),primary key(pname));
    sqlstatement stmt(&conn1);
    stmt.prepare("select maxincvalue from T_MAXINCVALUE where pname=:1");
    stmt.bindin(1,starg.pname,50);
    stmt.bindout(1,&imaxincvalue);
    stmt.execute();
    stmt.next();
  }
  else
  {
    // 从文件中加载自增字段的最大值
    CFile File;
    if (File.Open(starg.incfilename,"r")== false)
    {
      return true;
    }
    char strtemp[31];
    File.FFGETS(strtemp,30);
    imaxincvalue=atol(strtemp,30);
  }
  logfile.Write("上次以抽取数据的位置（%s=%ld) \n",starg.infield,imaxincvalue);
  return true;
}   
      
bool writeincfile()
{   
  // 如果starg.incfield参数为空，表示不是增量抽取
  if (strlen(starg.incfield)==0)
  {
    return true;
  }
  if (strlen(starg.connstr1)!=0)
  {
    //从数据库表中加载自增字段的最大值
    sqlstatement stmt(&conn1);
    stmt.prepare("update T_MAXINCVALUE set maxincvalue=:1 where pname=:2");
    if (stmt.m_cda.rc==1146)
    {
      conn1.execute("create table T_MAXINCVALUE(pname varchar(50),maxincvalue numeric(15),primary key(pname))");
      conn1.execute("insert into T_MAXINCVALUE values('%s',0)",starg.pname);
      stmt.prepare("update T_MAXINCVALUE set maxincvalue=:1 where pname=:2");
    }
    stmt.bindin(1,&imaxincvalue);
        stmt.bindin(2,starg.pname,50);
    if (stmt.execute()!=0)
    {
      logfile.Write("stmt.execute() failed.\n%s\n%s\n",stmt.m_sql,stmt.m_cda.message);
      return false;
    }
    conn1.commit();
  }
  else
  {
    CFile File;
    if (File.Open(starg.incfilename,"w+")== false)
    {
      logfile.Write("File.Open(%s) failed \n",strarg.incfilename);
      return false;
    }
    File.Fprintf("%ld",imaxincvalue);
    File.Close();
  }
  return true;
}

bool _dminingmysql()
{
  // 从starg.incfilename文件中获取以抽取数据的最大id
  readincfile();
  sqlstatement stmt(&conn);
  stmt.prepare(starg.selectsql);
  char strfieldvalue[ifieldcount][MAXFIELDLEN+1];
  for (int ii=1;ii<=ifieldcount;ii++)
  {
    stmt.bindout(ii,strfieldvalue[ii-1],ifieldlen[ii-1]);
  }
  // 如果是增量抽取，绑定输入参数（以抽取数据的最大id)
  if (strlen(starg.incfield)!=0)
  {
    stmt.bindin(1,&imaxincvalue);
  }
  if (stmt.execute()!=0)
  {
    logfile.Write("stmt.execute() failed.\n%s\n%s\n",stmt.m_sql,stmt.m_cda.message);
    return false;
  }
  PActive.UptATime();
  CFile File;

  while (true)
  {
    memset(strfieldvalue,0,sizeof(strfieldvalue));
    if (stmt.next()!=0)
    {
          break;
    }
    if (File.IsOpened() == true)
    {
      crtxmlfilename();
      if (OpenForRename(strxmlfilename,"w+")==false)
      {
        logfile.Write("File.OpenForRename(%s) failed.\n",strxmlfilename);
        return false;
      }
      File.Fprintf("<data>\n");
    }
    for (int ii=1;ii<ifieldcount;ii++)
    {
      File.Fprintf("<%s>%s</%s>",strfieldname[ii-1],strfieldvalue[ii-1],strfieldname[ii-1]);
    }
    File.Fprintf("<endl/>\n");
    // 如果记录数达到starg.maxcount行就切换一个xml文件
    if((starg.maxcount>0) &&(stmt.m_cda.rpc%starg.maxcount ==0))
    {
      File.Fprintf("</data>\n");
      if (File.CloseAndRename() == false)
      {
        logfile.Write("File.CloseAndRename(%s) failed.\n",strxmlfilename);
        return false;
      }
      logfile.Write("生成文件%s(%d)。\n",strxmlfilename,starg.maxcount);
      PActive.UptATime();
    }
    if ((strlen(starg.incfield)!=0)&&(imaxincvalue<atol(strfieldvalue[incfieldpos])))
    {
      imaxincvalue=atol(strfieldvalue[incfieldpos]);
    }
  }

  if (File.IsOpened() == true)
  {
    File.Fprintf("</data>\n");
    if (File.CloseAndRename() == false)
    {
      logfile.Write("File.CloseAndRename(%s) failed.\n",strxmlfilename);
      return false;
    }
    if (starg.maxcount==0)
    {
      logfile.Write("生成文件%s(%d).\n",strxmlfilename,stmt.m_cda.rpc);
    }
    else
    {
      logfile.Write("生成文件%s(%d)。\n",strxmlfilename,stmt.m_cda.rpc%starg.maxcount);
    }  }
  if (stmt.m_cda.rpc>0)
  {
    writeincfile();
  }
  return true;
}

bool instarttime()
{
  if (strlen(starg.starttime)!=0)
  {
    char strHH24[3];
    memset(strHH24,0,sizeof(strHH24));
    LocalTime(strHH24,"hh24");
    if (strstr(starg.starttime,strHH24)==0)
    {
      return false;
    }
  }
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
  GetXMLBuffer(strxmlbuffer,"maxcount",&starg.maxcount);
  GetXMLBuffer(strxmlbuffer,"connstr1",starg.connstr1,100);
  GetXMLBuffer(strxmlbuffer,"timeout",&starg.timeout);   // 成功上传的文件清单
  if ((starg.timeout==0))
  { logfile.Write("timeout is null.\n");  return false;
  }
  GetXMLBuffer(strxmlbuffer,"pname",starg.pname,50);   // 成功上传的文件清单
  if (strlen(starg.pname)==0)
  { logfile.Write("pname is null.\n");  return false;
  }
  CCmdStr CmdStr;
  CmdStr.SpiltToCmd((starg.fieldlen,".");
  if (CmdStr.CmdCount()>MAXFIELDCOUNT)
  {
    logfile.Write("fieldlen的字段数太多，超出了最大限制%d。\n",MAXFIELDCOUNT);
    return false;
  }
  for (int ii=0;ii<CmdStr.CmdCount();ii++)
  {
    CmdStr.GetValue(ii,&ifieldlen[ii]);
    // if (ifieldlen[ii]>MAXFIELDLEN) 
    // {
    //   ifieldlen[ii]=MAXFIELDLEN;
    // }
    if (ifeildlen[ii]>MAXFIELDLEN)
    {
      MAXFIELDLEN=ifeildlen[ii];
    }
  }
  ifieldcount=CmdStr.CmdCount();
  CmdStr.SplitToCmd(starg.fieldstr,",");
  if (CmdStr.CmdCount()>MAXFIELDCOUNT)
  {
    logfile.Write("fieldstr的字段数太多，超出了最大限制%d.\n",MAXFIELDCOUNT);
    return false;
  }
   if (int ii=0;ii<CmdStr.CmdCount();ii++)
  {
    CmdStr.GetValue(ii,ifieldname[ii],30);
  }
  if (ifieldcount!=CmdStr.CmdCount())
  {
    logfile.Write("fieldstr和fieldlen的元素数量不一致.\n");
  }
  if (strlen(starg.incfield)!=0)
  {
    for (int ii=0;ii<ifieldcount;ii++)
    {
      if (strcmp(starg.incfield,strfieldname[ii])==0)
      {
        incfieldpos=ii;
        break;
      }
    }
    if (incfieldpos==-1)
    {
      logfile.Write("递增字段名%s不在列表%s中。\n",starg.incfield,starg.fieldstr);
      return false;
    }
    if ((strlen(starg.incfilename)==0)&&(strlen(starg.connstr1)==0))
    {
      logfile.Write("incfilename和connstr1参数必须二选一。\n");
      return false;
    }
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

  printf("Sample:/project/tools1/bin/procctl 3600 /project/tools1/bin/dminingmysql /log/idc/dminingmysql_ZHOBTCODE.log \"<connstr>192.168.10.131,root,mysqlpwd,mysql,3306</connstr><charse
t>utf8</charset><selectsql>select obtid,cityname,provname,lat,lon,height from T_ZHOBTCODE</selectsql><fieldstr>obtid,cityname,provname,lat,lon,height</fieldstr><fieldlen>10,30,30,10,10,1
0</fieldlen><bfilename>ZHOBTCODE</bfilename><efilename>HYCZ</efilename><outpath>/idcdata/dmindata</outpath><timeout>30</timeout><pname>dminingmysql_ZHOBTCODE</pname>\"\n\n");
  printf("       /project/tools1/bin/procctl   30 /project/tools1/bin/dminingmysql /log/idc/dminingmysql_ZHOBTMIND.log \"<connstr>192.168.10.131,root,mysqlpwd,mysql,3306</connstr><charse
t>utf8</charset><selectsql>select obtid,date_format(ddatetime,'%%%%Y-%%%%m-%%%%d %%%%H:%%%%i:%%%%s'),t,p,u,wd,wf,r,vis,keyid from T_ZHOBTMIND where keyid>:1 and ddatetime>timestampadd(mi
nute,-120,now())</selectsql><fieldstr>obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid</fieldstr><fieldlen>10,19,8,8,8,8,8,8,8,15</fieldlen><bfilename>ZHOBTMIND</bfilename><efilename>HYCZ</efilen
ame><outpath>/idcdata/dmindata</outpath><starttime></starttime><incfield>keyid</incfield><incfilename>/idcdata/dmining/dminingmysql_ZHOBTMIND_HYCZ.list</incfilename><timeout>30</timeout>
<pname>dminingmysql_ZHOBTMIND_HYCZ</pname><maxcount>1000</maxcount><connstr1>127.0.0.1,root,mysqlpwd,mysql,3306</connstr1>\"\n\n");
printf("       /project/tools1/bin/procctl   30 /project/tools1/bin/dminingmysql /log/idc/dminingmysql_ZHOBTMIND.log \"<connstr>192.168.10.131,root,mysqlpwd,mysql,3306</connstr><charse
t>utf8</charset><selectsql>select obtid,date_format(ddatetime,'%%%%Y-%%%%m-%%%%d %%%%H:%%%%i:%%%%s'),t,p,u,wd,wf,r,vis,keyid from T_ZHOBTMIND where keyid>:1 and ddatetime>timestampadd(mi
nute,-120,now())</selectsql><fieldstr>obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid</fieldstr><fieldlen>10,19,8,8,8,8,8,8,8,15</fieldlen><bfilename>ZHOBTMIND</bfilename><efilename>HYCZ</efilen
ame><outpath>/idcdata/dmindata</outpath><starttime></starttime><incfield>keyid</incfield><incfilename>/idcdata/dmining/dminingmysql_ZHOBTMIND_HYCZ.list</incfilename><timeout>30</timeout>
<pname>dminingmysql_ZHOBTMIND_HYCZ</pname><maxcount>1000</maxcount><connstr1>127.0.0.1,root,mysqlpwd,mysql,3306</connstr1>\"\n\n");
  printf("本程序是数据中心的公共功能模块，用于从MySQL数据库源表抽取数据，生成xml文件。\n");
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
  printf("maxcount    输出xml文件的最大记录数，缺省是0，表示无限制，如果本参数取值为0，注意适当加大timeout的取值，防止程序超时。\n");
  printf("starttime   程序运行的时间区间，例如02,13表示：如果程序启动时，踏中02时和13时则运行，其它时间不运行。"\
         "如果starttime为空，那么starttime参数将失效，只要本程序启动就会执行数据抽取，为了减少数据源"\
         "的压力，从数据库抽取数据的时候，一般在对方数据库最闲的时候时进行。\n");
  printf("incfield    递增字段名，它必须是fieldstr中的字段名，并且只能是整型，一般为自增字段。"\
          "如果incfield为空，表示不采用增量抽取方案。");
  printf("incfilename 已抽取数据的递增字段最大值存放的文件，如果该文件丢失，将重新抽取全部的数据。\n");
  printf("connstr1    已抽取数据的递增字段最大值存放的数据库的连接参数。connstr1和incfilename二选一，connstr1优先。");
  printf("timeout     本程序的超时时间，单位：秒。\n");
  printf("pname       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");
}
