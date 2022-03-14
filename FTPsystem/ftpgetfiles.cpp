#include "_public.h"
#include "_ftp.h"

struct st_arg
{
  char host[31];           // 远程服务器的IP和端口。
  int  mode;               // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
  char username[31];       // 远程服务器ftp的用户名。
  char password[31];       // 远程服务器ftp的密码。
  char remotepath[301];    // 远程服务器存放文件的目录。
  char localpath[301];     // 本地文件存放的目录。
  char matchname[101];     // 待下载文件匹配的规则。
  char listfilename[301];  // 下载前列出服务器文件名的文件
  int  ptype;              // 文件下载成功后，远程服务器文件的处理方式：1-什么都不做，2-删除，3-备份
  char remotepathbak[301]; // 文件下载成功后，服务器文件的备份目录
  char okfilename[301];    // 下载成功文件的清单
  bool checkmtime;         //是否需要检查服务端文件的时间，true-需要，false-不需要，缺省为false。
  int  timeout;            //进程心跳的超时时间
  char pname[51];          //进程名，建议用"ftpgetfiles_后缀"的方式
} starg;

struct st_fileinfo
{
  char filename[301];       //文件名
  char mtime[21];           //文件时间
};

vector<struct st_fileinfo> vlistfile1;  // 已下载成功文件名的容器
vector<struct st_fileinfo> vlistfile2;  // 下载前列出服务器文件名的容器，从nlist文件中加载
vector<struct st_fileinfo> vlistfile3;  // 本次不需要下载的文件的容器
vector<struct st_fileinfo> vlistfile4;  // 本次需要下载的文件的容器

CLogFile logfile;
Cftp ftp;
CPActive PActive;                       //进程心跳

void EXIT(int sig);                  //程序退出和2，15号信号的处理函数
void _help();                        //帮助函数
bool _xmltoarg(char *strxmlbuffer);  //将xml解析到starg结构体中
bool _ftpgetfiles();                 //文件下载主函数
bool LoadListFile();                 //将文件目录加载到容器中
bool LoadOKFile();                   //将okfilename中的文件加载到容器vlistfile1
bool CompVector();                   //比较vlistfile2和vlistfile1，得到vlistfile3和vlistfile4
bool WriteToOkFile();                //将vlistfile3中的内容写入okfilename中，覆盖原来的okfilename
bool AppendToOKFile(struct st_fileinfo *stfileinfo);  //将下载成功的文件加载filename文件中

int main(int argc,char *argv[])
{
  // 小目标，把ftp服务上某目录中的文件下载到本地的目录中。

  if (argc!=3) { _help(); return -1; }

  // 关闭全部的信号和输入输出。
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
  // 但请不要用 "kill -9 +进程号" 强行终止。
  CloseIOAndSignal();
  signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

  // 打开日志文件。
  if (logfile.Open(argv[1],"a+")==false)
  {
    printf("打开日志文件失败（%s）。\n",argv[1]); return -1;
  }

  // 解析xml，得到程序运行的参数。
  if (_xmltoarg(argv[2])==false) return -1;
  PActive.AddPInfo(starg.timeout,starg.pname);  //把进程的心跳信息写出共享内存中
  // 登录ftp服务器
  if (ftp.login(starg.host,starg.username,starg.password,starg.mode) == false)
  {
    logfile.Write("ftp.login(%s,%s,%s) failed.\n",starg.host,starg.username,starg.password);
    return -1;
  }

  logfile.Write("ftp.login success\n");
  _ftpgetfiles();
  ftp.logout();
  return 0;
}

bool LoadOKFile()
{
  vlistfile1.clear();
  CFile File;
  //如果程序是第一次下载，okfilename是不存在的，并不是错误，返回true
  if ((File.Open(starg.okfilename,"r")) == false)
  {
    return true;
  }
  char strbuffer[501];
  struct st_fileinfo stfileinfo;
  while(true)
  {
    memset(&stfileinfo,0,sizeof(struct st_fileinfo));
    if (File.Fgets(strbuffer,300,true) == false)
    {
      break;
    }
    GetXMLBuffer(strbuffer,"filename",stfileinfo.filename);
    GetXMLBuffer(strbuffer,"mtime",stfileinfo.mtime);
    vlistfile1.push_back(stfileinfo);
  }
    return true;
}
bool CompVector()
  {
  vlistfile3.clear();
  vlistfile4.clear();
  int ii,jj;
  for (ii=0;ii<vlistfile2.size();++ii)
  {
    for (jj=0;jj<vlistfile1.size();++jj)
    {
      //如果找到了放入vlistfile3中
      if ((strcmp(vlistfile2[ii].filename,vlistfile1[jj].filename) == 0) && (strcmp(vlistfile2[ii].mtime,vlistfile1[jj].mtime) == 0))
      {
        vlistfile3.push_back(vlistfile2[ii]);
        break;
      }
    }
    //如果没有找到放入vlistfile4中
    if (jj==vlistfile1.size())
    {
      vlistfile4.push_back(vlistfile2[ii]);
    }
  }
  return true;
}
bool WriteToOKFile()
{
  CFile File;
  if (File.Open(starg.okfilename,"w") ==false)
  {
    logfile.Write("File.Open(%s) failed.\n",starg.okfilename);
    return false;
  }
  for (int ii=0;ii<vlistfile3.size();++ii)
  {
    File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n",vlistfile3[ii].filename,vlistfile3[ii].mtime);
  }
  return true;
}
bool _ftpgetfiles()
{
  //进入ftp服务器的存放文件的目录
  if (ftp.chdir(starg.remotepath) == false)
  {
    logfile.Write("ftp.chdir(%s) failed.\n",starg.remotepath);
    return false;
  }
  if (ftp.nlist(".",starg.listfilename) == false)
  {
    logfile.Write("ftp.nlist(%s) failed.\n",starg.listfilename);
    return false;
  }
  PActive.UptATime();
    //将ftp.nlist()得到的文件名放入容器vlistfile2中
  if (LoadListFile() ==false)
  {
    logfile.Write("LoadListFile() failed.\n");
    return false;
  }
  PActive.UptATime();
  if (starg.ptype == 1)
  {
    //加载okfile中的文件到容器vlisfile1中
    LoadOKFile();
    //对比vlistfile1和vlistfile2,得到listfile3和listfile4
    CompVector();
    //将vlistfile3中的内容写入okfilename中
    WriteToOKFile();
    //将vlistfile4的复制到vlistfile2中
    vlistfile2.clear();
    vlistfile2.swap(vlistfile4);
  }
  PActive.UptATime();
  //遍历容器vlistfile2
  char strremotefilename[301],strlocalfilename[301];
  for (int ii=0;ii<vlistfile2.size();++ii)
  {
    SNPRINTF(strremotefilename,sizeof(strremotefilename),300,"%s/%s",starg.remotepath,vlistfile2[ii].filename);
    SNPRINTF(strlocalfilename,sizeof(strlocalfilename),300,"%s/%s",starg.localpath,vlistfile2[ii].filename);
    //调用ftp.get()从服务器下载文件
    logfile.Write("get %s ...",strremotefilename);
    if (ftp.get(strremotefilename,strlocalfilename) == false)
    {
      logfile.WriteEx("failed.\n");
      break;
    }
    logfile.WriteEx("ok.\n");
    PActive.UptATime();
    //当ptype==1，把下载成功的文件记录追加到okfilename文件中
    if (starg.ptype==1)
    {
      AppendToOKFile(&vlistfile2[ii]);
    }
    //取文件后删除文件
    if (starg.ptype==2)
    {
      if (ftp.ftpdelete(strremotefilename) ==false)
      {
        logfile.Write("ftp.ftpdelete(%s) failed.\n",strremotefilename);
      }
    }
    //将文件转存到备份目录
    if (starg.ptype==3)
          {
      char strremotefilenamebak[301];
      SNPRINTF(strremotefilenamebak,sizeof(strremotefilenamebak),300,"%s/%s",starg.remotepathbak,vlistfile2[ii].filename);
      if (ftp.ftprename(strremotefilename,strremotefilenamebak) == false)
      {
        logfile.Write("ftp.ftprename(%s,%S) failed.",strremotefilename,strremotefilenamebak);
        return false;
      }
    }
  }
  return true;
}

bool LoadListFile()
{
  vlistfile2.clear();
  CFile File;
  if (File.Open(starg.listfilename,"r") ==false)
  {
     logfile.Write("File.Open(%s) failed.\n",starg.listfilename);
     return false;
  }
  struct st_fileinfo stfileinfo;
  while(true)
  {
    memset(&stfileinfo,0,sizeof(struct st_fileinfo));
    if (File.Fgets(stfileinfo.filename,300,true) == false)
    {
      break;
    }
    if (MatchStr(stfileinfo.filename,starg.matchname) == false)
    {
      continue;
    }
    if ((starg.ptype==1)&&(starg.checkmtime==true))
    {
      //获得ftp服务端文件的时间
      if (ftp.mtime(stfileinfo.filename)==false)
      {
        logfile.Write("ftp.mtime(%s) failed.\n",stfileinfo.filename);
        return false;
      }
      strcpy(stfileinfo.mtime,ftp.m_mtime);
    }
    vlistfile2.push_back(stfileinfo);
  }
//for (int ii=0;ii<vlistfile2.size();++ii)
//{
//  logfile.Write("filename=%s=\n",vlistfile2[ii].filename);
//}
  return true;
  }

void EXIT(int sig)
{
  printf("程序退出，sig=%d\n\n",sig);

  exit(0);
}

void _help()
{
  printf("\n");
  printf("Using:/project/tools1/bin/ftpgetfiles logfilename xmlbuffer\n\n");
  printf("Example:/project/tools1/bin/procctl 30 /project/tools1/bin/ftpgetfiles /log/idc/ftpgetfiles_surfdata.log \"<host>192.168.10.131:21</host><mode>1</mode><username>jyl</username><
password>1228488450jyl.</password><localpath>/idcdata/surfdata</localpath><remotepath>/tmp/idc/surfdata</remotepath><matchname>SURF_ZH*.XML,SURF_ZH*.CSV</matchname><listfilename>/idcdata
/ftplist/ftpgetfiles_surfdata.list</listfilename><ptype>3</ptype><remotepathbak>/tmp/idc/surfdatabak</remotepathbak><okfilename>/idcdata/ftplist/ftpgetfiles_surfdata.xml</okfilename><che
ckmtime>true</checkmtime><timeout>80</timeout><pname>ftpgetfiles_surfdata</pname>\"\n\n\n");
  printf("本程序是通用的功能模块，用于把远程ftp服务器的文件下载到本地目录。\n");
  printf("logfilename是本程序运行的日志文件。\n");
  printf("xmlbuffer为文件下载的参数，如下：\n");
  printf("<host>127.0.0.1:21</host> 远程服务器的IP和端口。\n");
  printf("<mode>1</mode> 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。\n");
  printf("<username>wucz</username> 远程服务器ftp的用户名。\n");
  printf("<password>wuczpwd</password> 远程服务器ftp的密码。\n");
  printf("<remotepath>/tmp/idc/surfdata</remotepath> 远程服务器存放文件的目录。\n");
  printf("<localpath>/idcdata/surfdata</localpath> 本地文件存放的目录。\n");
  printf("<matchname>SURF_ZH*.XML,SURF_ZH*.CSV</matchname> 待下载文件匹配的规则。"\
   "不匹配的文件不会被下载，本字段尽可能设置精确，不建议用*匹配全部的文件。\n");
  printf("<listfilename>/idcdata/ftplist/ftpgetfiles_surfdata.list</listfilename> 下载前列出服务文件名的文件。 \n");
  printf("<ptype>1</ptype> 文件下载成功后，远程服务器文件的处理方式：1-什么都不做，2-删除，3-备份，如果为3，要制定备份的目录\n");
  printf("<remotepathbak>/tmp/idc/surfdatabak</remotepathbak> 文件下载成功后，服务器文件的备份目录，此参数只有当ptype=3时才有效。\n\n");
  printf("<okfilename>/idcdata/ftplist/ftpgetfiles_surfdata.xml</okfilename> 已下载成功文件名清单。此参数只有ptype=1时有效。\n\n\n");
  printf("<checkmtime>true</checkmtime> 是否需要检查服务端文件的时间，true-需要，false-不需要，此参数只有当ptype=1时才有效，缺省为false。\n");
  printf("<timeout>80</timeout> 下载文件超时时间，单位：秒，视文件大小和网络宽度而定。\n");
  printf("<pname>ftpgetfiles_surfdata</pname> 进程名，尽可能采用易懂的，与其他进程不同的名称，方便故障排除。\n\n\n");
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

  GetXMLBuffer(strxmlbuffer,"matchname",starg.matchname,100);   // 待下载文件匹配的规则。
  if (strlen(starg.matchname)==0)
  { logfile.Write("matchname is null.\n");  return false; }

  GetXMLBuffer(strxmlbuffer,"listfilename",starg.listfilename,300);   // 下载前列出存放文件名的文件。
  if (strlen(starg.listfilename)==0)
  { logfile.Write("listfilename is null.\n");  return false; }

  GetXMLBuffer(strxmlbuffer,"ptype",&starg.ptype);   // 下载文件后服务器的操作
  if ( (starg.ptype!=1) && (starg.ptype!=2) && (starg.ptype!=3))
  { logfile.Write("ptype is error.\n");  return false; }

  GetXMLBuffer(strxmlbuffer,"remotepathbak",starg.remotepathbak,300);   // 下载后服务器文件备份目录
  if ((strlen(starg.remotepathbak)==0) && (starg.ptype==3))
  { logfile.Write("remotepathbak is null.\n");  return false; }

  GetXMLBuffer(strxmlbuffer,"okfilename",starg.okfilename,300);   // 成功下载的文件清单
  if ((strlen(starg.okfilename)==0) && starg.ptype==1)
  { logfile.Write("okfilename is null.\n");  return false;
  }
  GetXMLBuffer(strxmlbuffer,"checkmtime",&starg.checkmtime);  //是否需要检查服务端文件的时间，true-需要，false-不需要，此参数只有当ptype=1时才有效，缺省为false。`

  GetXMLBuffer(strxmlbuffer,"timeout",&starg.timeout);   // 成功下载的文件清单
  if ((starg.timeout==0))
  { logfile.Write("timeout is null.\n");  return false;
  }
  GetXMLBuffer(strxmlbuffer,"pname",starg.pname,50);   // 成功下载的文件清单
  if (strlen(starg.pname)==0)
  { logfile.Write("pname is null.\n");  return false;
  }
    return true;
}

bool AppendToOKFile(struct st_fileinfo *stfileinfo)
{
  CFile File;
  if (File.Open(starg.okfilename,"a") ==false)
  {
  logfile.Write("File.Open(%s) failed.\n",starg.okfilename);
  return false;
  }
  File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n",stfileinfo->filename,stfileinfo->mtime);
  return true;