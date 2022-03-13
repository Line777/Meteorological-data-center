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
  char matchname[101];     // 待上传文件匹配的规则。
  int  ptype;              // 文件上传成功后，客户端文件的处理方式：1-什么都不做，2-删除，3-备份
  char localpathbak[301];  // 文件上传成功后，客户端文件的备份目录
  char okfilename[301];    // 上传成功文件的清单
  int  timeout;            // 进程心跳的超时时间
  char pname[51];          // 进程名，建议用"ftpputfiles_后缀"的方式
} starg;

struct st_fileinfo
{
  char filename[301];       //文件名
  char mtime[21];           //文件时间
};

vector<struct st_fileinfo> vlistfile1;  // 已上传成功文件名的容器
vector<struct st_fileinfo> vlistfile2;  // 上传前列出客户端文件名的容器，从nlist文件中加载
vector<struct st_fileinfo> vlistfile3;  // 本次不需要上传的文件的容器
vector<struct st_fileinfo> vlistfile4;  // 本次需要上传的文件的容器

CLogFile logfile;
Cftp ftp;
CPActive PActive;                       //进程心跳

void EXIT(int sig);                  //程序退出和2，15号信号的处理函数
void _help();                        //帮助函数
bool _xmltoarg(char *strxmlbuffer);  //将xml解析到starg结构体中
bool _ftpputfiles();                 //文件上传主函数
bool LoadLocalFile();                 //将文件目录加载到容器中
bool LoadOKFile();                   //将okfilename中的文件加载到容器vlistfile1
bool CompVector();                   //比较vlistfile2和vlistfile1，得到vlistfile3和vlistfile4
bool WriteToOkFile();                //将vlistfile3中的内容写入okfilename中，覆盖原来的okfilename
bool AppendToOKFile(struct st_fileinfo *stfileinfo);  //将上传成功的文件加载filename文件中

int main(int argc,char *argv[])
{
  // 小目标，把ftp服务上某目录中的文件上传到本地的目录中。

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
  _ftpputfiles();
  ftp.logout();
  return 0;
}

bool LoadOKFile()
{
  vlistfile1.clear();
  CFile File;
  //如果程序是第一次上传，okfilename是不存在的，并不是错误，返回true
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
bool _ftpputfiles()
{
  //将localpath得到的文件名放入容器vlistfile2中
  if (LoadLocalFile() ==false)
  {
    logfile.Write("LoadLocalFile() failed.\n");
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
    //调用ftp.put()从服务器上传文件
    logfile.Write("put %s ...",strremotefilename);
    if (ftp.put(strlocalfilename,strremotefilename,true) == false)
    {
      logfile.WriteEx("failed.\n");
      break;
    }
    logfile.WriteEx("ok.\n");
    PActive.UptATime();
    //当ptype==1，把上传成功的文件记录追加到okfilename文件中
    if (starg.ptype==1)
    {
      AppendToOKFile(&vlistfile2[ii]);
    }
    //取文件后删除文件
    if (starg.ptype==2)
    {
      if (REMOVE(strlocalfilename) ==false)
      {
        logfile.Write("REMOVE(%s) failed.\n",strlocalfilename);
        return false;
      }
    }
    //将文件转存到备份目录
    if (starg.ptype==3)
    {
      char strlocalfilenamebak[301];
      SNPRINTF(strlocalfilenamebak,sizeof(strlocalfilenamebak),300,"%s/%s",starg.localpathbak,vlistfile2[ii].filename);
      if (RENAME(strlocalfilename,strlocalfilenamebak) == false)
      {
               logfile.Write("RENAME(%s,%S) failed.\n",strlocalfilename,strlocalfilenamebak);
        return false;
      }
    }
  }
  return true;
}

bool LoadLocalFile()
{
  vlistfile2.clear();
  CDir Dir;
  if (Dir.OpenDir(starg.localpath,starg.matchname) == false)
  {
    logfile.Write("Dir.OpenDir(%s) failed.\n",starg.localpath);
    return false;
  }
  struct st_fileinfo stfileinfo;
  while(true)
  {
    memset(&stfileinfo,0,sizeof(struct st_fileinfo));
    if (Dir.ReadDir()== false)
    {
      break;
    }
    strcpy(stfileinfo.filename,Dir.m_FileName);  //文件名，不包括目录名
    strcpy(stfileinfo.mtime,Dir.m_ModifyTime);     //文件时间
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
  printf("Using:/project/tools1/bin/ftpputfiles logfilename xmlbuffer\n\n");
  printf("Example:/project/tools1/bin/procctl 30 /project/tools1/bin/ftpputfiles /log/idc/ftpputfiles_surfdata.log \"<host>192.168.10.131:21</host><mode>1</mode><username>jyl</username><
password>1228488450jyl.</password><localpath>/tmp/idcdata/surfdata</localpath><remotepath>/tmp/ftpputest</remotepath><matchname>SURF_ZH*.JSON</matchname><ptype>3</ptype><localpathbak>/tm
p/idc/surfdatabak</localpathbak><okfilename>/idcdata/ftplist/ftpputfiles_surfdata.xml</okfilename><timeout>80</timeout><pname>ftpputfiles_surfdata</pname>\"\n\n\n");
  printf("本程序是通用的功能模块，用于把远程ftp服务器的文件上传到本地目录。\n");
    printf("logfilename是本程序运行的日志文件。\n");
  printf("xmlbuffer为文件上传的参数，如下：\n");
  printf("<host>127.0.0.1:21</host> 远程服务器的IP和端口。\n");
  printf("<mode>1</mode> 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。\n");
  printf("<username>wucz</username> 远程服务器ftp的用户名。\n");
  printf("<password>wuczpwd</password> 远程服务器ftp的密码。\n");
  printf("<remotepath>/tmp/idc/surfdata</remotepath> 远程服务器存放文件的目录。\n");
  printf("<localpath>/idcdata/surfdata</localpath> 本地文件存放的目录。\n");
  printf("<matchname>SURF_ZH*.JSON</matchname> 待上传文件匹配的规则。"\
   "不匹配的文件不会被上传，本字段尽可能设置精确，不建议用*匹配全部的文件。\n");
  printf("<ptype>1</ptype> 文件上传成功后，远程服务器文件的处理方式：1-什么都不做，2-删除，3-备份，如果为3，要制定备份的目录\n");
  printf("<localpathbak>/tmp/idc/surfdatabak</remotepathbak> 文件上传成功后，服务器文件的备份目录，此参数只有当ptype=3时才有效。\n\n");
  printf("<okfilename>/idcdata/ftplist/ftpputfiles_surfdata.xml</okfilename> 已上传成功文件名清单。此参数只有ptype=1时有效。\n\n\n");
  printf("<timeout>80</timeout> 上传文件超时时间，单位：秒，视文件大小和网络宽度而定。\n");
  printf("<pname>ftpputfiles_surfdata</pname> 进程名，尽可能采用易懂的，与其他进程不同的名称，方便故障排除。\n\n\n");
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
}
