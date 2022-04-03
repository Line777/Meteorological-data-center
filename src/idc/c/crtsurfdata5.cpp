// 本程序用于生成全国气象站点观测的分钟数据

#include "_public.h"

//全国气象站点参数数据结构体
struct st_stcode
{
  char provname[31];  //省
  char obtid[11];     //站号
  char obtname[31];   //站名
  double lat;         //纬度
  double lon;         //经度
  double height;      //海拔高度
};

// 全国气象站点分钟观测数据结构
struct st_surfdata
{
  char obtid[11];      // 站点代码。
  char ddatetime[21];  // 数据时间：格式yyyymmddhh24miss
  int  t;              // 气温：单位，0.1摄氏度。
  int  p;              // 气压：0.1百帕。
  int  u;              // 相对湿度，0-100之间的值。
  int  wd;             // 风向，0-360之间的值。
  int  wf;             // 风速：单位0.1m/s
  int  r;              // 降雨量：0.1mm。
  int  vis;            // 能见度：0.1米。
};


bool LoadSTCode(const char *inifile);  //将全国气象站点参数存入容器
void CrtSurfData();                    //模拟生成站点分钟观测数据
bool CrtSurfFile(const char* outpath,const char* datafmt);  //生成数据文件


char strddatetime[21];                 //观测数据时间
CLogFile logfile;                      //构造函数缺省默认单个日志大小最大为100M，设置参数范围在10-100
vector<struct st_stcode> vstcode;      //存放全国气象站点数据  
vector<struct st_surfdata> vsurfdata;  //存放站点的分钟观测数据的容器


int main(int argc,char* argv[])
{
  //inifile outpath logfile
  if (argc != 5) 
  {
    printf("Using:./crtsurfdata4 inifile outpath logfile datafmt\n");
    printf("Example:/project/idc1/bin/crtsurfdata4 /project/idc1/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata4.log xml,json,csv\n\n");
    printf("iniflie 全国气象站点参数文件名。\n");
    printf("outpath 全国气象站点数据文件存放的目录。\n");
    printf("logfile 本程序运行的日志文件名。 \n");
    printf("datafmt 生成数据文件格式，支持xml，json，csv三种格式\n\n");
    return -1;
  }
  if (logfile.Open(argv[3]) == false)
  {
    printf("logfile.Open(%s) failed.\n",argv[3]);
    return -1;
  }
  logfile.Write("crtsurfdata4 开始运行\n");
  //WriteEx函数与Write函数的区别：
  //Write函数在写日志时会写入时间，而WriteEx函数不会
  //logfile.WriteEx("crtsurfdata\n");
  
  //将全国气象站点参数加载到vstcode中
  if ( LoadSTCode(argv[1]) == false)
  {
    return -1;
  }
  CrtSurfData();  //模拟生成分钟观测数据
  if (strstr(argv[4],"xml") != 0)
  {
    CrtSurfFile(argv[2],"xml");
  }
  if (strstr(argv[4],"json") != 0)
  {
    CrtSurfFile(argv[2],"json");
  }
  if (strstr(argv[4],"csv") != 0)
  {
    CrtSurfFile(argv[2],"csv");
  }
  logfile.Write("crtsurfdata4 运行结束\n");
  return 0;
}

bool LoadSTCode (const char *inifile)
{
  CFile File;
  if (File.Open(inifile,"r") == false)
  {
    logfile.Write("File.Open(%s) failed \n",inifile);
    return false;
  }
  
  char strBuffer[301];
  CCmdStr CmdStr;
  struct st_stcode stcode;
  //从文件中每次读取一行数据
  while (true)
  {
    if ( File.Fgets(strBuffer,300,true) == false )
    {
      break;
    }
    //logfile.Write("=%s=\n",strBuffer);
    
    //将读取到的每行数据拆分
    CmdStr.SplitToCmd(strBuffer,",",true);
    if (CmdStr.CmdCount() !=  6)
    {
      continue;
    }
    CmdStr.GetValue(0,stcode.provname,30);
    CmdStr.GetValue(1,stcode.obtid,10);
    CmdStr.GetValue(2,stcode.obtname,30);
    CmdStr.GetValue(3,&stcode.lat);
    CmdStr.GetValue(4,&stcode.lon);
    CmdStr.GetValue(5,&stcode.height);   
    //将单个全国气象站站点参数的结构体存入容器
    vstcode.push_back(stcode);
  }
  //for(int ii=0; ii<vstcode.size();++ii)
  //{
  //  logfile.Write("provname=%s,obtid=%s,obtname=%s,lat=%.2f,lon=%.2f,height=%.2f\n",vstcode[ii].provname,vstcode[ii].obtid,
  //                vstcode[ii].obtname,vstcode[ii].lat,vstcode[ii].lon,vstcode[ii].height);
  //}    
  
  return true;

}

void CrtSurfData()
{
  srand(time(0));
  memset(strddatetime,0,sizeof(strddatetime));
  LocalTime(strddatetime,"yyyymmddhh24miss");
  struct st_surfdata stsurfdata;
  //遍历全国气象站点参数的容器
  for (int ii=0;ii<vstcode.size();++ii)
  { 
    memset(&stsurfdata,0,sizeof(struct st_surfdata));
    //使用随机数模拟参数
    strncpy(stsurfdata.obtid,vstcode[ii].obtid,10);  
    strncpy(stsurfdata.ddatetime,strddatetime,14);   
    stsurfdata.t=rand()%351;                         //气温:单位，0.1摄氏度
    stsurfdata.p=rand()%265+10000;                   //气压：0.1百帕
    stsurfdata.u=rand()%100+1;                       //相对湿度，0-100之间的值
    stsurfdata.wd=rand()%360;                        //风向，0-360之间的值
    stsurfdata.wf=rand()%150;                        //风速，单位0.1m/s
    stsurfdata.r=rand()%16;                          //降雨量；0.1mm
    stsurfdata.vis=rand()%5001+100000;               //能见度:0.1m
    //将分钟观测数据存入容器中
    vsurfdata.push_back(stsurfdata);
  } 
}
bool CrtSurfFile(const char* outpath,const char* datafmt)
{
  //创建临时文件，并向文件写，避免了在文件写入过程中，有其他文件读的问题
  //在临时文件完成后，再将临时文件更改为正式文件
  CFile File;
  //拼接生成的数据文件名
  char strFileName[301];
  sprintf(strFileName,"%s/SURF_ZH_%s_%d.%s",outpath,strddatetime,getpid(),datafmt);
  if (File.OpenForRename(strFileName,"w") == false)
  {
    logfile.Write("File.OpenForRename(%s) failed.",strFileName);
    return false;
  }
  if (strcmp(datafmt,"csv") == 0)
  {
    File.Fprintf("站点代码，数据时间，气温，气压，相对湿度，风向，风速，降雨量，能见度\n");
  }
  if (strcmp(datafmt,"xml") == 0)
  {
    File.Fprintf("<data>\n");
  }
  if (strcmp(datafmt,"json")==0) 
  {
    File.Fprintf("{\"data\":[\n");
  }
  for (int ii=0;ii<vsurfdata.size();++ii)
  {
    if (strcmp(datafmt,"csv") == 0)
    {
      File.Fprintf("%s,%s,%.1f,%.1f,%d,%d,%1.f,%1.f,%1.f\n",vsurfdata[ii].obtid,vsurfdata[ii].ddatetime,\
                   vsurfdata[ii].t/10.0,vsurfdata[ii].p/10.0,vsurfdata[ii].u,vsurfdata[ii].wd,\
                   vsurfdata[ii].wf/10.0,vsurfdata[ii].r/10.0,vsurfdata[ii].vis/10.0); 
    }
    if (strcmp(datafmt,"xml")==0) 
    {
      File.Fprintf("<obtid>%s</obtid><ddatetime>%s</ddatetime><t>%.1f</t><p>%.1f</p><u>%d</u><wd>%d</wd>"\
                   "<wf>%1.f</wf><r>%1.f</r><vis>%1.f</vis><endl/>\n",vsurfdata[i].obtid,vsurfdata[i].ddatetime,\
                   vsurfdata[i].t/10.0,vsurfdata[i].p/10.0,vsurfdata[i].u,vsurfdata[i].wd,vsurfdata[i].wf/10.0,\
                   vsurfdata[i].r/10.0,vsurfdata[i].vis/10.0);
    }
    if (strcmp(datafmt,"json")==0) 
    {
      File.Fprintf("{\"obtid\":\"%s\",\"ddatetime\":\"%s\",\"t\":\"%.1f\",\"p\":\"%.1f\",\"u\":\"%d\",\"wd\":\"%d\","\
                   "\"wf\":\"%.1f\",\"r\":\"%.1f\",\"vis\":\"%.1f\"}",vsurfdata[i].obtid,vsurfdata[i].ddatetime,\
                   vsurfdata[i].t/10.0,vsurfdata[i].p/10.0,vsurfdata[i].u,vsurfdata[i].wd,\
                   vsurfdata[i].wf/10.0,vsurfdata[i].r/10.0,vsurfdata[i].vis/10.0);

      if (i<vsurfdata.size()-1) 
      {
        File.Fprintf(",\n");
      } 
      else 
      {
        File.Fprintf("\n");
      }
    }

  }
  if (strcmp(datafmt,"xml") == 0)
  {
    File.Fprintf("</data>\n");
  }
  if (strcmp(datafmt,"json") == 0)
  {
    File.Fprintf("]}\n");
  }
  File.CloseAndRename();
  logfile.Write("生成数据文件%s成功,数据时间%s,记录数%d。\n",strFileName,strddatetime,vsurfdata.size());
  return true;
}
