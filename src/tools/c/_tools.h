#ifndef _TOOLS_H
#define _TOOLS_H

#include "_public.h"
#include "_mysql.h"

// 表的列(字段)信息的结构体。
struct st_columns
{
  char  colname[31];  // 列名。
  char  datatype[31]; // 列的数据类型，分为number、date和char三大类。
  int   collen;       // 列的长度，number固定20，date固定19，char的长度由表结构决定。
  int   pkseq;        // 如果列是主键的字段，存放主键字段的顺序，从1开始，不是主键取值0。
};

// 获取表全部的列和主键列信息的类。
class CTABCOLS
{
public:
  CTABCOLS();

  int m_allcount;   // 全部字段的个数。
  int m_pkcount;    // 主键字段的个数。

  vector<struct st_columns> m_vallcols;  // 存放全部字段信息的容器。
  vector<struct st_columns> m_vpkcols;   // 存放主键字段信息的容器。

  char m_allcols[3001];  // 全部的字段名列表，以字符串存放，中间用半角的逗号分隔。
  char m_pkcols[301];    // 主键字段名列表，以字符串存放，中间用半角的逗号分隔。

  void initdata();  // 成员变量初始化。
  // 获取指定表的全部字段信息。
  bool allcols(connection *conn,char *tablename);

  // 获取指定表的主键字段信息。
  bool pkcols(connection *conn,char *tablename);
};

struct st_arg
{
  char connstr[101];     // 数据库的连接参数。
  char charset[51];      // 数据库的字符集。
  char inifilename[301]; // 数据入库的参数配置文件。
  char xmlpath[301];     // 待入库xml文件存放的目录。
  char xmlpathbak[301];  // xml文件入库后的备份目录。
  char xmlpatherr[301];  // 入库失败的xml文件存放的目录。
  int  timetvl;          // 本程序运行的时间间隔，本程序常驻内存。
  int  timeout;          // 本程序运行时的超时时间
  char pname[51];        // 本程序运行时的程序名。
};

#endif

