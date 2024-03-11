alter table T_DATATYPE
   drop constraint FK_DATATYPE_DATATYPE;

alter table T_INTERCFG
   drop constraint FK_INTERCFG_DATATYPE;

drop table T_DATATYPE cascade constraints;

/*==============================================================*/
/* Table: T_DATATYPE                                            */
/*==============================================================*/
drop   sequence SEQ_DATATYPE;
create sequence SEQ_DATATYPE increment by 1 minvalue 1 nocycle;
create table T_DATATYPE 
(
   typeid             varchar2(30)         not null,
   ptypeid            varchar2(30),
   typename           varchar2(100),
   orderby            number(5),
   memo               varchar2(300),
   rsts               number(15)           default 1 not null,
   upttime            date                 default sysdate not null,
   keyid              number(15)           not null,
   constraint PK_DATATYPE primary key (typeid),
   constraint DATATYPE_KEYID unique (keyid)
);

comment on column T_DATATYPE.typeid is
'数据种类编号。';

comment on column T_DATATYPE.ptypeid is
'上级数据种类编号，如果是一级分类，本字段填空。';

comment on column T_DATATYPE.typename is
'种类名称。';

comment on column T_DATATYPE.orderby is
'显示顺序。';

comment on column T_DATATYPE.memo is
'备注。';

comment on column T_DATATYPE.rsts is
'记录状态，1-启用；2-禁用。';

comment on column T_DATATYPE.upttime is
'更新时间。';

comment on column T_DATATYPE.keyid is
'记录编号，从与本表同名的序列生成器中获取。';

alter table T_DATATYPE
   add constraint FK_DATATYPE_DATATYPE foreign key (ptypeid)
      references T_DATATYPE (typeid);


alter table T_INTERCFG
   drop constraint FK_INTERCFG_DATATYPE;

alter table T_USERANDINTER
   drop constraint FK_USERANDINTER_INTERCFG;

alter table T_USERLOG
   drop constraint FK_USERLOG_INTERCFG;

alter table T_USERLOGSTAT
   drop constraint FK_T_USERLO_REFERENCE_T_INTERC;

drop table T_INTERCFG cascade constraints;

/*==============================================================*/
/* Table: T_INTERCFG                                            */
/*==============================================================*/
drop   sequence SEQ_INTERCFG;
create sequence SEQ_INTERCFG increment by 1 minvalue 1 nocycle;
create table T_INTERCFG 
(
   intername          varchar2(30)         not null,
   typeid             varchar2(30),
   intercname         varchar2(100),
   selectsql          varchar2(1000)       not null,
   colstr             varchar2(300)        not null,
   bindin             varchar2(300),
   orderby            number(5),
   memo               varchar2(300),
   rsts               number(15)           default 1 not null,
   upttime            date                 default sysdate not null,
   keyid              number(15)           not null,
   constraint PK_INTERCFG primary key (intername),
   constraint INTERCFG_KEYID unique (keyid)
);

comment on table T_INTERCFG is
'本表存放了全部接口的配置参数。';

comment on column T_INTERCFG.intername is
'接口代码，英文名。';

comment on column T_INTERCFG.typeid is
'数据种类。';

comment on column T_INTERCFG.intercname is
'接口名称，中文名。';

comment on column T_INTERCFG.selectsql is
'接口SQL。';

comment on column T_INTERCFG.colstr is
'输出列名，列名之间用逗号分隔。';

comment on column T_INTERCFG.bindin is
'接口参数，参数之间用逗号分隔。';

comment on column T_INTERCFG.orderby is
'显示顺序。';

comment on column T_INTERCFG.memo is
'备注。';

comment on column T_INTERCFG.rsts is
'记录状态，1-启用；2-禁用。';

comment on column T_INTERCFG.upttime is
'更新时间。';

comment on column T_INTERCFG.keyid is
'记录编号，从与本表同名的序列生成器中获取。';

alter table T_INTERCFG
   add constraint FK_INTERCFG_DATATYPE foreign key (typeid)
      references T_DATATYPE (typeid);


alter table T_USERANDINTER
   drop constraint FK_USERANDINTER_USERINFO;

alter table T_USERLOG
   drop constraint FK_USERLOG_USERINFO;

alter table T_USERLOGSTAT
   drop constraint FK_T_USERLO_REFERENCE_T_USERIN;

drop table T_USERINFO cascade constraints;

/*==============================================================*/
/* Table: T_USERINFO                                            */
/*==============================================================*/
drop   sequence SEQ_USERINFO;
create sequence SEQ_USERINFO increment by 1 minvalue 1 nocycle;
create table T_USERINFO 
(
   username           varchar2(30)         not null,
   passwd             varchar2(30)         not null,
   appname            varchar2(50)         not null,
   ip                 varchar2(50),
   contacts           varchar2(50),
   tel                varchar2(50),
   email              varchar2(50),
   memo               varchar2(300),
   rsts               number(1)            default 1 not null,
   upttime            date                 default sysdate not null,
   keyid              number(15)           not null,
   constraint PK_USERINFO primary key (username),
   constraint USERINFO_KEYID unique (keyid)
);

comment on table T_USERINFO is
'本表存放了客户端的身份认证信息。';

comment on column T_USERINFO.username is
'用户名。';

comment on column T_USERINFO.passwd is
'密码。';

comment on column T_USERINFO.appname is
'应用名称。';

comment on column T_USERINFO.ip is
'绑定ip，多个ip之间用逗号分隔。';

comment on column T_USERINFO.contacts is
'联系人。';

comment on column T_USERINFO.tel is
'联系电话。';

comment on column T_USERINFO.email is
'联系信箱。';

comment on column T_USERINFO.memo is
'备注。';

comment on column T_USERINFO.rsts is
'记录状态，1-启用；2-禁用。';

comment on column T_USERINFO.upttime is
'更新时间。';

comment on column T_USERINFO.keyid is
'记录编号，从与本表同名的序列生成器中获取。';


alter table T_USERANDINTER
   drop constraint FK_USERANDINTER_INTERCFG;

alter table T_USERANDINTER
   drop constraint FK_USERANDINTER_USERINFO;

drop table T_USERANDINTER cascade constraints;

/*==============================================================*/
/* Table: T_USERANDINTER                                        */
/*==============================================================*/
create table T_USERANDINTER 
(
   username           varchar2(30)         not null,
   intername          varchar2(30)         not null,
   constraint PK_USERANDINTER primary key (intername, username)
);

comment on table T_USERANDINTER is
'本表存放了每个用户访问接口的权限信息。';

comment on column T_USERANDINTER.username is
'用户名。';

comment on column T_USERANDINTER.intername is
'接口名。';

alter table T_USERANDINTER
   add constraint FK_USERANDINTER_INTERCFG foreign key (intername)
      references T_INTERCFG (intername)
      on delete cascade;

alter table T_USERANDINTER
   add constraint FK_USERANDINTER_USERINFO foreign key (username)
      references T_USERINFO (username)
      on delete cascade;


alter table T_USERLOG
   drop constraint FK_USERLOG_INTERCFG;

alter table T_USERLOG
   drop constraint FK_USERLOG_USERINFO;

drop index IDX_USERLOG_3;

drop index IDX_USERLOG_2;

drop index IDX_USERLOG_1;

drop table T_USERLOG cascade constraints;

/*==============================================================*/
/* Table: T_USERLOG                                             */
/*==============================================================*/
drop   sequence SEQ_USERLOG;
create sequence SEQ_USERLOG increment by 1 minvalue 1 nocycle;
create table T_USERLOG 
(
   keyid              number(15)           not null,
   username           varchar2(30)         not null,
   intername          varchar2(30)         not null,
   upttime            date                 default sysdate not null,
   ip                 varchar2(30),
   rpc                number(8),
   constraint PK_USERLOG primary key (keyid)
);

comment on table T_USERLOG is
'本表存放了用户每次调用接口的信息。';

comment on column T_USERLOG.keyid is
'记录编号，从与本表同名的序列生成器中获取。';

comment on column T_USERLOG.username is
'用户名。';

comment on column T_USERLOG.intername is
'接口代码。';

comment on column T_USERLOG.upttime is
'更新时间。';

comment on column T_USERLOG.ip is
'客户端ip';

comment on column T_USERLOG.rpc is
'数据行数，调用接口时返回数据的行数。';

/*==============================================================*/
/* Index: IDX_USERLOG_1                                         */
/*==============================================================*/
create index IDX_USERLOG_1 on T_USERLOG (
   username ASC
);

/*==============================================================*/
/* Index: IDX_USERLOG_2                                         */
/*==============================================================*/
create index IDX_USERLOG_2 on T_USERLOG (
   intername ASC
);

/*==============================================================*/
/* Index: IDX_USERLOG_3                                         */
/*==============================================================*/
create index IDX_USERLOG_3 on T_USERLOG (
   upttime ASC
);

alter table T_USERLOG
   add constraint FK_USERLOG_INTERCFG foreign key (intername)
      references T_INTERCFG (intername);

alter table T_USERLOG
   add constraint FK_USERLOG_USERINFO foreign key (username)
      references T_USERINFO (username);


alter table T_USERLOGSTAT
   drop constraint FK_T_USERLO_REFERENCE_T_USERIN;

alter table T_USERLOGSTAT
   drop constraint FK_T_USERLO_REFERENCE_T_INTERC;

drop table T_USERLOGSTAT cascade constraints;

/*==============================================================*/
/* Table: T_USERLOGSTAT                                         */
/*==============================================================*/
drop   sequence SEQ_USERLOG;
create sequence SEQ_USERLOG increment by 1 minvalue 1 nocycle;
create table T_USERLOGSTAT 
(
   username           varchar2(30)         not null,
   intername          varchar2(30)         not null,
   ddatetie           date                 not null,
   rpc                number(8),
   keyid              number(15)           not null,
   constraint PK_USERLOGSTAT primary key (username, intername, ddatetie)
);

comment on column T_USERLOGSTAT.username is
'用户名。';

comment on column T_USERLOGSTAT.intername is
'接口代码。';

comment on column T_USERLOGSTAT.ddatetie is
'统计时段，精确到小时。';

comment on column T_USERLOGSTAT.rpc is
'数据行数，调用接口时返回数据的总行数。';

comment on column T_USERLOGSTAT.keyid is
'记录编号，从与本表同名的序列生成器中获取。';

alter table T_USERLOGSTAT
   add constraint FK_T_USERLO_REFERENCE_T_USERIN foreign key (username)
      references T_USERINFO (username);

alter table T_USERLOGSTAT
   add constraint FK_T_USERLO_REFERENCE_T_INTERC foreign key (intername)
      references T_INTERCFG (intername);

exit;
