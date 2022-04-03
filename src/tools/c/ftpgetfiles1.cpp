#include "_public.h"
#include "_ftp.h"

struct st_arg
{
  char host[31];         //远程服务器ip和端口
  int  mode;             //传输模式，1-被动模式，2-主动模式，缺省默认为被动方式
  char username[31];     //远程服务ftp的用户名
  char password[31];     //远程服务器ftp的密码
  char remotepath[301];  //远程服务器存放文件的目录
  char localpath[301];   //本地文件存放的目录
  char matchname[101];   //带下载文件匹配的规则
} starg;

CLogFile logfile;
Cftp ftp;

void EXIT(int sig);                  //程序退出和2，15号信号的处理函数
void _help();                        //帮助函数
bool _xmltoarg(char *strxmlbuffer);  //将xml解析到参数starg结构体中

int main(int argc,char* argv[])
{
  //  for (int ii=0;ii<64;++ii)
  // {
  //    signal(ii,SIG_IGN);
  //  }
  //  signal(2,EXIT);
  //  signal(15,EXIT);
  if (argc!=3)
  {
    _help();
    return -1;
  }
  //CloseIOAndSignal();
  signal(SIGINT,EXIT);
  signal(SIGTERM,EXIT);
  if (logfile.Open(argv[1],"a+") == false)
  {
    printf("打开日志文件失败(%s)\n",argv[1]);
    return -1;
  }
  if (_xmltoarg(argv[2]) == false)
  {
    return -1;
  }
  return 0;
}

void _help()
{
   printf("\n");
   printf("Using:/project/tools1/bin/ftpgetfiles logfilename xmlbuffer\n\n");
   printf("Sample:/project/tools1/bin/procctl 30 /project/tools1/bin/ftpgetfiles /log/idc/ftpgetfiles_surfdata.log \"<host>192.168.10.131:21</host><mode>1</mode><username>jyl</username><password>1228488450jyl.</password><localpath>/idcdata/surfdata</localpath><remotepath>/tmp/idc/surfdata</remotepath><matchname>SURF_ZH*.XML,SURF_ZH*.CSV</matchname><listfilename>/idcdata/ftplist/ftpgetfiles_surfdata.list</listfilename><ptype>1</ptype><remotepathbak>/tmp/idc/surfdatabak</remotepathbak><okfilename>/idcdata/ftplist/ftpgetfiles_surfdata.xml</okfilename><checkmtime>true</checkmtime><timeout>80</timeout><pname>ftpgetfiles_surfdata</pname>\"\n\n\n");
   printf("本程序是通用的功能模块，用于把远程ftp服务端的文件下载到本地目录。\n");
   printf("logfilename是本程序运行的日志文件。\n");
   printf("xmlbuffer为文件下载的参数，如下：\n");
   printf("<host>192.168.10.131:21</host> 远程服务端的IP和端口。\n");
   printf("<mode>1</mode> 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。\n");
   printf("<username>jyl</username> 远程服务端ftp的用户名。\n");
   printf("<password>1228488450jyl.</password> 远程服务端ftp的密码。\n");
   printf("<remotepath>/tmp/idc/surfdata</remotepath> 远程服务端存放文件的目录。\n");
   printf("<localpath>/idcdata/surfdata</localpath> 本地文件存放的目录。\n");
   printf("<matchname>SURF_ZH*.XML,SURF_ZH*.CSV</matchname> 待下载文件匹配的规则。"\
          "不匹配的文件不会被下载，本字段尽可能设置精确，不建议用*匹配全部的文件。\n");
   printf("<listfilename>/idcdata/ftplist/ftpgetfiles_surfdata.list</listfilename> 下载前列出服务端文件名的文件。\n");
   printf("<ptype>1</ptype> 文件下载成功后，远程服务端文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。\n");
   printf("<remotepathbak>/tmp/idc/surfdatabak</remotepathbak> 文件下载成功后，服务端文件的备份目录，此参数只有当ptype=3时才有效。\n");
   printf("<okfilename>/idcdata/ftplist/ftpgetfiles_surfdata.xml</okfilename> 已下载成功文件名清单，此参数只有当ptype=1时才有效。\n");
   printf("<checkmtime>true</checkmtime> 是否需要检查服务端文件的时间，true-需要，false-不需要，此参数只有当ptype=1时才有效，缺省为false。\n");
   printf("<timeout>80</timeout> 下载文件超时时间，单位：秒，视文件大小和网络带宽而定。\n");
   printf("<pname>ftpgetfiles_surfdata</pname> 进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");
}

void EXIT(int sig)
{
  printf("程序退出，sig=%d\n\n",sig);
  exit(0);
}

bool _xmltoarg(char *strxmlbuffer)
{
  memset(&starg,0,sizeof(struct st_arg));
  GetXMLBuffer(strxmlbuffer,"host",starg.host,30);                //远程服务器ftp的IP和端口
  if (strlen(starg.host) == 0)
  {
    logfile.Write("host is null.\n");
    return false;
  }
  GetXMLBuffer(strxmlbuffer,"mode",&starg.mode);                  //远程服务器ftp的工作模式
  if (starg.mode != 2)
  {
    starg.mode=1;
  }
  GetXMLBuffer(strxmlbuffer,"username",starg.username,30);        //远程服务器ftp的用户名
  if (strlen(starg.username) == 0)
  {
    logfile.Write("username is null.\n");
    return false;
  }
  GetXMLBuffer(strxmlbuffer,"password",starg.password,30);       //远程服务器ftp的密码
  if (strlen(starg.password) == 0)
  {
    logfile.Write("password is null.\n");
    return false;
  }
  GetXMLBuffer(strxmlbuffer,"remotepath",starg.remotepath,300);  //远程服务器存放文件的目录
  if (strlen(starg.remotepath) == 0)
  {
    logfile.Write("remotepath is null.\n");
    return false;
  }
  GetXMLBuffer(strxmlbuffer,"localpath",starg.localpath,300);    //本地文件的存放
  if (strlen(starg.localpath) == 0)
  {
    logfile.Write("localpath is null.\n");
    return false;
  }
  GetXMLBuffer(strxmlbuffer,"matchname",starg.matchname,100);    //待下载文件匹配的规则
  if (strlen(starg.matchname) == 0)
  {
    logfile.Write("matchname is null.\n");
    return false;
  }
  return true;
}
